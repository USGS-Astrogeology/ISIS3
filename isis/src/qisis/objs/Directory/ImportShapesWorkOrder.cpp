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
#include "ImportShapesWorkOrder.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrentMap>

#include "Cube.h"
#include "CubeAttribute.h"
#include "FileName.h"
#include "IException.h"
#include "Project.h"
#include "ProjectItem.h"
#include "ProjectItemModel.h"
#include "SaveProjectWorkOrder.h"
#include "TextFile.h"

namespace Isis {

  /**
   * Creates a work order to import a shape model.
   *
   * @param *project Pointer to the project this work order belongs to
   */
  ImportShapesWorkOrder::ImportShapesWorkOrder(Project *project) :
      WorkOrder(project) {
    // This workorder is synchronous and undoable.
    m_isUndoable = true;
    m_isSynchronous = false;
    m_newShapes = NULL;
    m_list = NULL;

    QAction::setText(tr("Import &Shape Models and Ground source..."));
    QUndoCommand::setText(tr("Import Shape Models and Ground source"));
    setModifiesDiskState(true);
  }


  /**
   * Creates a copy of the other ImportShapesWorkOrder
   *
   * @param &other ImportShapesWorkOrder to copy the state from
   */
  ImportShapesWorkOrder::ImportShapesWorkOrder(const ImportShapesWorkOrder &other) :
      WorkOrder(other) {
    m_newShapes = NULL;
    m_list = other.m_list;
  }


  /**
   * Destructor
   */
  ImportShapesWorkOrder::~ImportShapesWorkOrder() {
    delete m_newShapes;
    m_newShapes = NULL;

    delete m_list;
    m_list = NULL;
  }


  /**
   * This method clones the current ImportShapesWorkOrder and returns it.
   *
   * @return ImportShapesWorkOrder Clone
   */
  ImportShapesWorkOrder *ImportShapesWorkOrder::clone() const {
    return new ImportShapesWorkOrder(*this);
  }


  /**
   * This method returns true if the user clicked on a project tree node with the text "Shapes".
   * This is used by Directory::supportedActions(DataType data) to determine what actions are
   * appended to context menus.
   *
   * @param item The ProjectItem that was clicked
   *
   * @return bool True if the user clicked on a project tree node named "Shapes"
   */
  bool ImportShapesWorkOrder::isExecutable(ProjectItem *item) {
    if (item) {
      return (item->text() == "Shapes");
    }

    return false;
  }


  /**
   * @brief Prompt the user for shape files to import and whether to copy DN data in to project.
   *
   * This method is designed to be implemented by children work orders, but they need
   * to call this version inside of their setupExecution (at the beginning).

   * State should only be set in the parent WorkOrder class in this method. You can set arbitrary
   *   state using setInternalData(). This method is always executed in the GUI thread and is the
   *   only place to ask the user questions.
   *
   * If this method returns false the workorder will be cancelled and will not be executed.
   *
   * @return @b bool Returns True is the user selected shape files to import, false
   *                 if the user cancelled the import.
   */
  bool ImportShapesWorkOrder::setupExecution() {
    WorkOrder::setupExecution();

    QStringList fileNames = QFileDialog::getOpenFileNames(
        qobject_cast<QWidget *>(parent()),
        tr("Import Shape Model Images"), "",
        tr("Isis cubes and list files (*.cub *.lis);;All Files (*)"));

    QStringList stateToSave;

    if (!fileNames.isEmpty()) {
      for (const QString &str : fileNames) {
        FileName fileName(str.toStdString());
        if (QString::fromStdString(fileName.extension()) == "lis") {
          TextFile listFile(QString::fromStdString(fileName.expanded()));
          QString path = QString::fromStdString(fileName.path());
          QString lineOfListFile;

          while (listFile.GetLine(lineOfListFile)) {
            if (lineOfListFile.contains(path)){
              stateToSave.append(lineOfListFile);
            }
            else {
              stateToSave.append(path + "/" + lineOfListFile);
            }
          }
        }
        else {
          stateToSave.append(QString::fromStdString(fileName.original()));
        }
      }
    }

    QMessageBox::StandardButton copyImagesAnswer = QMessageBox::No;
    if (!fileNames.isEmpty()) {
      copyImagesAnswer = QMessageBox::question(qobject_cast<QWidget *>(parent()),
               tr("Copy Shape Model Cubes into Project"),
               tr("Should images (DN data) be copied into project?"),
               QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
               QMessageBox::Yes);
    }

    bool copyDnData = (copyImagesAnswer == QMessageBox::Yes);

    stateToSave.prepend(copyDnData? "copy" : "nocopy");

    if (fileNames.count() > 1) {
      QUndoCommand::setText(tr("Import %1 Shape Model Images").arg(stateToSave.count() - 1));
    }
    else if (fileNames.count() == 1) {
      QUndoCommand::setText(tr("Import %1").arg(fileNames.first()));
    }

    setInternalData(stateToSave);

    bool doImport = fileNames.count() > 0 && copyImagesAnswer != QMessageBox::Cancel;

    return doImport;
  }


  /**
    * @brief delete the imported shapes from the disk.
    *
    * Note: postUndoExecution() deletes shapes from project.
    */
  void ImportShapesWorkOrder::undoExecution() {
    if (m_list && project()->shapes().size() > 0 ) {
      project()->waitForShapeReaderFinished();
      // Remove the shapes from disk.
      m_list->deleteFromDisk( project() );
      // Remove the shapes from the model, which updates the tree view.
      ProjectItem *currentItem =
          project()->directory()->model()->findItemData( QVariant::fromValue(m_list) );
      project()->directory()->model()->removeItem(currentItem);
    }
  }

  /**
    * @brief delete the imported shapes from the project.
    */
  void ImportShapesWorkOrder::postUndoExecution() {
    if (m_list && project()->shapes().size() > 0 ) {
      foreach (Shape *shape, *m_list) {
        delete shape;
      }
      delete m_list;
    }
  }

  /**
    * @brief Creates a project shape folder and copies the shape cubes into it. This will create
    * the *.ecub and .cub files inside of the project.
    */
  void ImportShapesWorkOrder::execute() {
    if (internalData().count() > 0) {
      importConfirmedShapes(internalData().mid(1), (internalData()[0] == "copy"));
      project()->setClean(false);
    }
  }

  /**
    * @brief Add the imported shapes into the project.
    *
    * If there was an error on import display a error message to the user.
    */
  void ImportShapesWorkOrder::postExecution() {
    if (m_newShapes && !m_newShapes->isEmpty()) {
      project()->addShapes(*m_newShapes);
      m_list = project()->shapes().last();

      delete m_newShapes;
      m_newShapes = NULL;
    }
    else {
      project()->undoStack()->undo();
    }

    if (m_warning != "") {
      project()->warn(m_warning);
    }
  }


  /**
   * OriginalFileToProjectFunctor constructor
   *
   * @param *guiThread The thread for the gui
   * @param destinationFolder The folder to copy the DN data to
   * @param copyDnData Determines if the DN data will be copied to the project
   */
  ImportShapesWorkOrder::OriginalFileToProjectCubeFunctor::OriginalFileToProjectCubeFunctor(
      QThread *guiThread, QDir destinationFolder, bool copyDnData) : m_errors(new IException),
      m_numErrors(new int(0)) {
    m_destinationFolder = destinationFolder;
    m_copyDnData = copyDnData;
    m_guiThread = guiThread;
  }


  /**
   * Copy constructor
   *
   * @param &other OriginalFileToProjectCubeFunctor to copy
   */
  ImportShapesWorkOrder::OriginalFileToProjectCubeFunctor::OriginalFileToProjectCubeFunctor(
      const OriginalFileToProjectCubeFunctor &other) : m_errors(other.m_errors),
      m_numErrors(other.m_numErrors) {
    m_destinationFolder = other.m_destinationFolder;
    m_copyDnData = other.m_copyDnData;
    m_guiThread = other.m_guiThread;
  }


  /**
   * Destructor
   */
  ImportShapesWorkOrder::OriginalFileToProjectCubeFunctor::~OriginalFileToProjectCubeFunctor() {
    m_destinationFolder = QDir();
    m_copyDnData = false;
    m_guiThread = NULL;
  }


  /**
   * Creates ecubs and copies the DN data of the cubes, if m_copyDnData is true.
   *
   * @param &original Imported shape cube
   *
   * @return Cube Copy of the imported shape cube
   */
  Cube *ImportShapesWorkOrder::OriginalFileToProjectCubeFunctor::operator()(
      const FileName &original) {
    Cube *result = NULL;

    if (*m_numErrors < 20) {
      try {
        QString destination = QFileInfo(m_destinationFolder, QString::fromStdString(original.name()))
                                .absoluteFilePath();
        Cube *input = new Cube(original, "r");

        if (m_copyDnData) {
          Cube *copiedCube = input->copy(destination.toStdString(), CubeAttributeOutput());
          delete input;
          input = copiedCube;
        }

        FileName externalLabelFile(destination.toStdString());
        externalLabelFile = externalLabelFile.setExtension("ecub");

        Cube *projectShape = input->copy(externalLabelFile, CubeAttributeOutput("+External"));

        if (m_copyDnData) {
          // Make sure the external label has a fully relative path to the DN data
          projectShape->relocateDnData(FileName(destination.toStdString()).name());
        }

        //  Set new ecub to readOnly.  When closing cube, the labels were being re-written because
        // the cube was read/write. This caused a segfault when imported large number of images
        // because of a label template file being opened too many times. 
        projectShape->reopen();

        delete input;

        result = projectShape;
      }
      catch (IException &e) {
        m_errorsLock.lock();

        m_errors->append(e);
        (*m_numErrors)++;

        m_errorsLock.unlock();
      }
    }

    return result;
  }


  /**
   * Returns the errors from importing
   *
   * @return IException The import errors
   */
  IException ImportShapesWorkOrder::OriginalFileToProjectCubeFunctor::errors() const {
    IException result;

    result.append(*m_errors);

    if (*m_numErrors >= 20) {
      result.append(
          IException(IException::Unknown,
                     tr("Aborted import shapes due to a high number of errors"),
                     _FILEINFO_));
    }
    return result;
  }


  /**
   * Creates a project shape folder and copies the shape cubes into it. This will create the *.ecub
   * and .cub files inside of the project.
   * This can be thought of as:
   * <pre>
   *   mkdir project/Shapes/import1
   *   cp in1.cub in2.cub project/Shapes/import1
   * </pre>
   *
   * This should be called in a non-GUI thread
   *
   * @param confirmedShapes This is a list of shape model cube file names outside of the
   *                   project folder
   * @param copyDnData If this is true, this will create both the *.cub and *.ecub files in the
   *                   project. Otherwise, only the external label files (*.ecub) will be created
   *                   inside of the project.
   */
  void ImportShapesWorkOrder::importConfirmedShapes(QStringList confirmedShapes,
                                                             bool copyDnData) {
    try {
      if (!confirmedShapes.isEmpty()) {
        QDir folder = project()->addShapeFolder("import");

        setProgressRange(0, confirmedShapes.count());

        m_newShapes = new ShapeList;
        m_newShapes->reserve(confirmedShapes.count());

        QStringList confirmedShapesFileNames;
        QStringList confirmedShapesIds;

        foreach (QString confirmedShape, confirmedShapes) {
          QStringList fileNameAndId = confirmedShape.split(",");

          confirmedShapesFileNames.append(fileNameAndId.first());

          if (fileNameAndId.count() == 2) {
            confirmedShapesIds.append(fileNameAndId.last());
          }
          else {
            confirmedShapesIds.append(QString());
          }
        }

        std::vector<std::string> stdStringList;
        stdStringList.reserve(confirmedShapesFileNames.size());
        for (const QString &qstr : confirmedShapesFileNames) {
          stdStringList.push_back(qstr.toStdString());
        }

        OriginalFileToProjectCubeFunctor functor(thread(), folder, copyDnData);
        QFuture<Cube *> future = QtConcurrent::mapped(stdStringList, functor);

        QStringList newInternalData;
        newInternalData.append(internalData().first());

        QThreadPool::globalInstance()->releaseThread();
        for (int i = 0; i < confirmedShapes.count(); i++) {
          setProgressValue(i);

          Cube *cube = future.resultAt(i);

          if (cube) {
            Shape *newShape = new Shape(future.resultAt(i));

            if (confirmedShapesIds[i].isEmpty()) {
              confirmedShapesIds[i] = newShape->id();
            }
            else {
              newShape->setId(confirmedShapesIds[i]);
            }

            QStringList ShapeInternalData;
            ShapeInternalData.append(confirmedShapesFileNames[i]);
            ShapeInternalData.append(confirmedShapesIds[i]);

            newInternalData.append(ShapeInternalData.join(","));

            m_newShapes->append(newShape);

            newShape->moveToThread(thread());
            newShape->displayProperties()->moveToThread(thread());

            newShape->closeCube();
          }
        }
        QThreadPool::globalInstance()->reserveThread();

        m_warning = functor.errors().toString();

        m_newShapes->moveToThread(thread());

        setInternalData(newInternalData);
      }
    }
    catch (IException &e) {
      QMessageBox::critical(NULL, tr("Error"), tr(e.what()));
    }
  }
}
