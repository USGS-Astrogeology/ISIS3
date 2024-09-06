/**
 * @file
 * $Revision: 1.19 $
 * $Date: 2010/03/22 19:44:53 $
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
#include "SaveProjectAsWorkOrder.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrentMap>

#include "Cube.h"
#include "CubeAttribute.h"
#include "FileName.h"
#include "Project.h"

namespace Isis {

  /**
   * Creates a work order to save the project to a new location. This work order is
   * synchronous and not undoable.
   *
   * @param Project *project Pointer to the project this work order belongs to.
   */
  SaveProjectAsWorkOrder::SaveProjectAsWorkOrder(Project *project) :
      WorkOrder(project) {
    // This work order is not undoable
    m_isUndoable = false;
    QAction::setText(tr("Save Project &As"));
    setCreatesCleanState(true);
  }


  /**
   * @brief Copy constructor.
   *
   * Creates a copy of the other SaveProjectAsWorkOrder.
   *
   * @param SaveProjectAsWorkOrder &other The other work order to copy state from.
   */
  SaveProjectAsWorkOrder::SaveProjectAsWorkOrder(const SaveProjectAsWorkOrder &other) :
      WorkOrder(other) {
  }


  /**
   * @brief Destructor.
   *
   * Destructor to clean up any memory that this work order allocates.
   */
  SaveProjectAsWorkOrder::~SaveProjectAsWorkOrder() {

  }


  /**
   * @brief Creates a clone of this work order.
   *
   * @return SaveProjectWorkOrder* Pointer to the newly cloned work order.
   */
  SaveProjectAsWorkOrder *SaveProjectAsWorkOrder::clone() const {
    return new SaveProjectAsWorkOrder(*this);
  }


  /**
   * @brief Sets up this work order prior to execution.
   *
   * This prompts the user for a location to save the project to and what name to save
   * the project as. If the user provides an empty name, then this setup fails.
   *
   * @see WorkOrder::setupExecution()
   *
   * @return bool Returns true if the provided project name is not empty, false otherwise.
   */
  bool SaveProjectAsWorkOrder::setupExecution() {
    bool success = WorkOrder::setupExecution();

    if (success) {
      QString newDestination =
          QFileDialog::getSaveFileName(NULL, QString("Project Location"), QString("."));

      if (!newDestination.isEmpty()) {
        QUndoCommand::setText(tr("Save project to [%1]") .arg(newDestination));
        QString realPath = QFileInfo(newDestination + "/").absolutePath();
        setInternalData(QStringList(realPath));
      }
      else {
        success = false;
      }
    }

    return success;
  }


  /**
   * @brief Executes the work order.
   *
   * Saves the project with the name and destination acquired in setupExecution.
   */
  void SaveProjectAsWorkOrder::execute() {
    QString destination = internalData().first();
    if (!destination.isEmpty()) {
      project()->save(destination.toStdString());
      project()->open(destination);
      project()->setClean(true);
    }
  }
}
