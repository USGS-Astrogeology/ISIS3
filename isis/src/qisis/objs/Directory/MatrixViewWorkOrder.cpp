/**
 * @file
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are
 *   public domain. See individual third-party library and package descriptions
 *   for intellectual property information, user agreements, and related
 *   information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or
 *   implied, is made by the USGS as to the accuracy and functioning of such
 *   software and related material nor shall the fact of distribution
 *   constitute any such warranty, and no responsibility is assumed by the
 *   USGS in connection therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html
 *   in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */
#include "MatrixViewWorkOrder.h"

#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "CorrelationMatrix.h"
#include "Directory.h"
#include "IException.h"
#include "MatrixSceneWidget.h"
#include "Project.h"

namespace Isis {

  /**
   * This method sets the text of the work order.
   *
   * @param project The Project that we are going to find the correlation matrix of.
   *
   */
  MatrixViewWorkOrder::MatrixViewWorkOrder(Project *project) :
      WorkOrder(project) {
    QAction::setText( tr("View Correlation &Matrix...") );
    m_isSavedToHistory = false;
  }

  /**
   * This method is for debugging.
   *
   *  @param other The MatrixViewWorkOrder being debugged.
   */
  MatrixViewWorkOrder::MatrixViewWorkOrder(const MatrixViewWorkOrder &other) :
      WorkOrder(other) {
        qDebug() << "matrix good?";
  }

  /**
   * Destructor
   */
  MatrixViewWorkOrder::~MatrixViewWorkOrder() {
  }

  /**
   * This method clones the MatrixViewWorkOrder
   *
   * @return @b MatrixViewWorkOrder Returns a clone of the MatrixViewWorkOrder
   */
  MatrixViewWorkOrder *MatrixViewWorkOrder::clone() const {
    return new MatrixViewWorkOrder(*this);
  }


  /**
   * This check is used by Directory::supportedActions(DataType data).
   *
   * @param matrix The correlation matrix that we want to view.
   *
   * @return @b bool True if data (passed to this method as the matrix parameter by
   *                 supportedActions) is of type CorrelationMatrix.
   */
  bool MatrixViewWorkOrder::isExecutable(CorrelationMatrix matrix) {
    return matrix.isValid();
  }


  /**
   * If WorkOrder::execute() returns true, a new matrix view is created.
   *
   * @return @b bool True if WorkOrder::execute() returns true.
   */
  bool MatrixViewWorkOrder::setupExecution() {
    bool success = WorkOrder::setupExecution();

    if (success) {
      QStringList viewOptions;

      QList<MatrixSceneWidget *> existingViews = project()->directory()->matrixViews();
      int viewToUse = -1;

      if ( existingViews.count() ) {
        for (int i = 0; i < existingViews.count(); i++) {
          viewOptions.append( existingViews[i]->windowTitle() );
        }
      }

      viewOptions.append( tr("New Matrix View") );

      if (viewOptions.count() > 1) {
        QString selected = QInputDialog::getItem(NULL, tr("View to see matrix in"),
            tr("Which view would you like your\nmatrix to be put into?"),
            viewOptions, viewOptions.count() - 1, false, &success);

        viewToUse = viewOptions.indexOf(selected);
      }
      else {
        viewToUse = viewOptions.count() - 1;
      }

      bool newView = false;
      if (viewToUse == viewOptions.count() - 1) {
        newView = true;

        QUndoCommand::setText( tr("View matrix in new matrix view") );

      }
      else if (viewToUse != -1) {
        MatrixSceneWidget *matrixView = existingViews[viewToUse];

        // Do we need this? If the view already exists has this already been done?
        CorrelationMatrix corrMat = correlationMatrix();
        corrMat.computeCorrelationMatrix();
        matrixView->setUpOptions(corrMat);
        matrixView->drawElements(corrMat);
        matrixView->drawGrid(corrMat);
      }

      QStringList internalData;
      internalData.append( QString::number(viewToUse) );
      internalData.append(newView? "new view" : "existing view");
      setInternalData(internalData);
    }

    return success;
  }


  /**
   * @brief This method returns true if other depends on a MatrixViewWorkOrder
   *
   * @param other we want to check for dependancies
   *
   * @return @b bool True if WorkOrder depends on a MatrixViewWorkOrder
   *
   */
  bool MatrixViewWorkOrder::dependsOn(WorkOrder *other) const {
    // depend on types of ourselves.
    return dynamic_cast<MatrixViewWorkOrder *>(other);
  }


  /**
   * @brief This method computes and displays the correlation matrix.
   */
  void MatrixViewWorkOrder::execute() {
    try {
      MatrixSceneWidget *matrixViewToUse = project()->directory()->addMatrixView();
      CorrelationMatrix corrMat = correlationMatrix();
      /*
       * Since computing the correlation matrix is potentially time consuming we only want to
       * do it if the user wants to see it.
       */
      corrMat.computeCorrelationMatrix();
      matrixViewToUse->setUpOptions(corrMat);
      matrixViewToUse->drawElements(corrMat);
      matrixViewToUse->drawGrid(corrMat);
      if (matrixViewToUse == NULL) {
        std::string msg = "The Correlation Matrix for this bundle could not be displayed";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
      project()->setClean(false);
    }
    catch (IException &e) {
      m_status = WorkOrderFinished;
      QMessageBox::critical(NULL, tr("Error"), tr(e.what()));
    }
  }


  /**
   * @brief This method deletes the last matrix viewed.
   */
  void MatrixViewWorkOrder::undoExecution() {
    if (internalData()[1] == "new view") {
      delete project()->directory()->matrixViews().last();
    }
  }
}
