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
#include "ImportImagesWorkOrder.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrentMap>

#include "Camera.h"
#include "Cube.h"
#include "CubeAttribute.h"
#include "FileName.h"
#include "Project.h"
#include "ProjectItem.h"
#include "ProjectItemModel.h"
#include "SaveProjectWorkOrder.h"
#include "Target.h"
#include "TextFile.h"

namespace Isis {

  /**
   * @brief Creates an asynchronous WorkOrder for importing images to the project.
   *
   * @param Project *project Project to import images into.
   */
  ImportImagesWorkOrder::ImportImagesWorkOrder(Project *project) :
      WorkOrder(project) {
    // This is an asynchronous work order
    m_isSynchronous = false;
    m_newImages = NULL;
    m_list = NULL;

    QAction::setText(tr("Import &Images..."));
    QUndoCommand::setText(tr("Import Images"));
    setModifiesDiskState(true);
  }


  /**
   * @brief Copies the WorkOrder.
   *
   * @param ImportImagesWorkOrder &other The other work order to copy state from.
   */
  ImportImagesWorkOrder::ImportImagesWorkOrder(const ImportImagesWorkOrder &other) :
      WorkOrder(other) {
    m_newImages = NULL;
    m_list = other.m_list;
  }


  /**
   * @brief Destructor.
   *
   * Releases the memory for the m_newImages member.
   */
  ImportImagesWorkOrder::~ImportImagesWorkOrder() {
    delete m_newImages;
    m_newImages = NULL;

    m_list = NULL;
  }


  /**
   * @brief Creates a clone of this work order.
   *
   * @see WorkOrder::clone()
   *
   * @return ImportImagesWorkOrder* Returns a pointer to the newly cloned work order.
   */
  ImportImagesWorkOrder *ImportImagesWorkOrder::clone() const {
    return new ImportImagesWorkOrder(*this);
  }


  /**
   * This method returns true if the user clicked on a project tree node with the text "Images".
   * This is used by Directory::supportedActions(DataType data) to determine what actions are
   * appended to context menus.
   *
   * @param item The ProjectItem that was clicked
   *
   * @return bool True if the user clicked on a project tree node named "Shapes"
   */
  bool ImportImagesWorkOrder::isExecutable(ProjectItem *item) {

      if (item) {
        return (item->text() == "Images");
      }

     return false;
  }


  /**
   * @brief Sets up this work order before being executed.
   *
   * First invokes WorkOrder's setupExecution(). Prompts the user for cubes and image
   * list files to import and stores them via a setInternalData() call. If there are more than 100
   * images to import, the user is prompted if they want to save their project before the import
   * occurs. If yes, a SaveProjectWorkOrder will be executed. This setup is considered successful
   * if the user does not hit cancel on a dialog prompt and if there is at least one image has been
   * selected by the user to import. This method was renamed from execute() to setupExecution()
   * according to the WorkOrder redesign.
   *
   * @see WorkOrder::setupExecution()
   *
   * @return bool Returns true if the setup was successful.
   */
  bool ImportImagesWorkOrder::setupExecution() {
    try {
      WorkOrder::setupExecution();

      QStringList fileNames = QFileDialog::getOpenFileNames(
          qobject_cast<QWidget *>(parent()),
          tr("Import Images"), "",
          tr("Isis cubes and list files (*.cub *.lis);;All Files (*)"));

      QStringList* stateToSave = new QStringList();

      if (!fileNames.isEmpty()) {
        for (const QString &str : fileNames) {
          FileName fileName(str.toStdString());
          if (fileName.extension() == "lis") {
            TextFile listFile(QString::fromStdString(fileName.expanded()));
            QString path = QString::fromStdString(fileName.path());
            QString lineOfListFile;

            while (listFile.GetLine(lineOfListFile)) {
              FileName relFileName(path.toStdString() + "/" + lineOfListFile.toStdString());
              if (relFileName.fileExists() ) {
                stateToSave->append(path + "/" + lineOfListFile);
              }
              else {
                FileName absFileName(lineOfListFile.toStdString());
                if ( absFileName.fileExists() && lineOfListFile.startsWith("/") ) {
                  stateToSave->append(lineOfListFile);
                }

                else {
                  project()->warn("File " + lineOfListFile + " not found");
                }
              }
            }
          }
          else {
            stateToSave->append(QString::fromStdString(fileName.original()));
          }
        }

        QMessageBox::StandardButton saveProjectAnswer = QMessageBox::No;
        if (stateToSave->count() >= 100 && project()->isTemporaryProject()) {
          saveProjectAnswer = QMessageBox::question(qobject_cast<QWidget *>(parent()),
                   tr("Save Project Before Importing Images"),
                   tr("Would you like to save your project <b>before</b> importing images? It can be "
                      "slow to save your project after these images have been loaded if you do not "
                      "save now. <br><br>IMPORTANT: WHEN IMPORTING LARGE DATA SETS, SAVING YOUR "
                      "PROJECT BEFORE IMPORTING IS HIGHLY RECOMMENDED."),
                   QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                   QMessageBox::Yes);
        }

        if (saveProjectAnswer == QMessageBox::Yes) {
          SaveProjectWorkOrder saveWorkOrder(project());
          saveWorkOrder.trigger();
        }

        QMessageBox::StandardButton copyImagesAnswer = QMessageBox::No;
        if (!stateToSave->isEmpty() && saveProjectAnswer != QMessageBox::Cancel) {
          copyImagesAnswer = QMessageBox::question(qobject_cast<QWidget *>(parent()),
                   tr("Copy Images into Project"),
                   tr("Should images (DN data) be copied into project?"),
                   QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                   QMessageBox::No);
        }

        bool copyDnData = (copyImagesAnswer == QMessageBox::Yes);

        stateToSave->prepend(copyDnData? "copy" : "nocopy");

        if (fileNames.count() > 1) {
          QUndoCommand::setText(tr("Import %1 Images").arg(stateToSave->count() - 1));
        }
        else if (fileNames.count() == 1 && stateToSave->count() > 2) {
          QUndoCommand::setText(tr("Import %1 Images from %2").arg(
                        QString::number(stateToSave->count() - 1), fileNames.first()));
        }
        else {
          QUndoCommand::setText(tr("Import %1").arg(fileNames.first()));
        }

        // The internal data will look like: [ copy|nocopy, img1, img2, ... ]
        setInternalData(*stateToSave);

        bool doImport = stateToSave->count() > 1 && saveProjectAnswer != QMessageBox::Cancel &&
                        copyImagesAnswer != QMessageBox::Cancel;

        return doImport;
      }

    }
    catch (IException &e) {
      QMessageBox::critical(NULL, tr("Error"), tr(e.what()));
    }
    return false;

  }


  /**
   * @brief Undoes the work order's execute.
   *
   * After this ImportImagesWorkOrder has executed and finished (all the images have
   * been read), this removes the images from this import from disk in the project's directory.
   * This was renamed from asyncUndo() to undoExecution() according to the WorkOrder redesign.
   *
   * @see WorkOrder::undoExecution()
   */
  void ImportImagesWorkOrder::undoExecution() {
    if (m_list && project()->images().size() > 0 ) {
      project()->waitForImageReaderFinished();
      // Remove the images from disk.
      m_list->deleteFromDisk( project() );
      // Remove the images from the model, which updates the tree view.
      ProjectItem *currentItem =
          project()->directory()->model()->findItemData( QVariant::fromValue(m_list) );
      project()->directory()->model()->removeItem(currentItem);
    }
  }


  /**
   * @brief Cleans up memory (images) after the undo execution occurs.
   *
   * After the undoExecution() occurs, this cleans up memory that was allocated for
   * the images from this import. This was renamed from postSyncUndo() to postUndoExecution()
   * according to the WorkOrder redesign.
   *
   * @see WorkOrder::postUndoExecution()
   */
  void ImportImagesWorkOrder::postUndoExecution() {
    if (m_list && project()->images().size() > 0 ) {
      foreach (Image *image, *m_list) {
        delete image;
      }
      delete m_list;
    }
  }


  /**
   * @brief Executes the work order.
   *
   * This actually "does" the work order task. In this case, this imports the images
   * into memory and copies any necessary data to disk. This was renamed from asyncRedo() to
   * execute() according to the WorkOrder redesign.
   *
   * @see ImportImagesWorkOrder::importConfirmedImages(QStringList confirmedImages, bool copyDnData)
   * @see WorkOrder::execute()
   */
  void ImportImagesWorkOrder::execute() {
    try {
      QObject tmpObj;
      if (internalData().count() > 0) {
        // Recall in setupExecution() that first element in internal data is copy|nocopy,
        // and rest of elements are the expanded names of images to import.
        importConfirmedImages(internalData().mid(1), (internalData()[0] == "copy"));
        project()->setClean(false);
      }
    }
    catch (IException &e) {
        QMessageBox::critical(NULL, tr("Error"), tr(e.what()));
    }
  }


  /**
   * @brief Associates the imported images to the project.
   *
   * After execute finishes, associates the imported images to the project. This will
   * also notify the project if there are any warnings that occurred related to the import. This
   * was renamed from postSyncRedo() to postExecution() according to the WorkOrder redesign.
   *
   * @see Project::addImages(Imagelist newImages)
   * @see WorkOrder::postExecution()
   */
  void ImportImagesWorkOrder::postExecution() {
    try {
      if (!m_newImages->isEmpty()) {
        project()->addImages(*m_newImages);
        m_list = project()->images().last();

        delete m_newImages;
        m_newImages = NULL;
      }
    }
    catch (IException &e) {
      m_status = WorkOrderFinished;
      m_warning.append(e.what());
    }
    if (m_warning != "") {
      project()->warn(m_warning);
    }
  }


  /**
   * @brief Creates the internal functor.
   *
   * This functor is used for copying an image to be imported into the project.
   *
   * @param QThread *guiThread Pointer to the thread that this ImportImagesWorkOrder lives in (was
   * created in).
   * @param QDir destinationFolder Where to copy the image to.
   * @param bool copyDnData Indicates whether or not to copy the image data or just the labels.
   */
  ImportImagesWorkOrder::OriginalFileToProjectCubeFunctor::OriginalFileToProjectCubeFunctor(
      QThread *guiThread, QDir destinationFolder, bool copyDnData) : m_errors(new IException),
      m_numErrors(new int(0)) {
    m_destinationFolder = destinationFolder;
    m_copyDnData = copyDnData;
    m_guiThread = guiThread;
  }


  /**
   * Copies the other functor object (copy constructor).
   *
   * @param OriginalFileToProjectCubeFunctor &other The other functor to copy state from.
   */
  ImportImagesWorkOrder::OriginalFileToProjectCubeFunctor::OriginalFileToProjectCubeFunctor(
      const OriginalFileToProjectCubeFunctor &other) : m_errors(other.m_errors),
      m_numErrors(other.m_numErrors) {
    m_destinationFolder = other.m_destinationFolder;
    m_copyDnData = other.m_copyDnData;
    m_guiThread = other.m_guiThread;
  }


  /**
   * @brief Destructor.
   *
   * Resets the internal members to default values.
   */
  ImportImagesWorkOrder::OriginalFileToProjectCubeFunctor::~OriginalFileToProjectCubeFunctor() {
    m_destinationFolder = QDir();
    m_copyDnData = false;
    m_guiThread = NULL;
  }


  /**
   * @brief Overloads the callable operator to invoke this functor.
   *
   * Copies an image to be imported for this ImportImagesWorkOrder into the
   * associated project. If we are not copying the image data, the a .ecub file will be created that
   * points to the original cube. Otherwise, a .cub will be copied into the project and a .ecub
   * will be created in the project that references the copied cube.
   * Note that if too many errors occur, the copying
   * will not proceed for remaining images in the import and a NULL pointer will be returned.
   *
   * @param FileName &original Original file name of the image to be copied.
   *
   * @return Cube* Returns a copy of the original cube.
   */
  Cube *ImportImagesWorkOrder::OriginalFileToProjectCubeFunctor::operator()(
      const FileName &original) {
    Cube *result = NULL;

    // As long as we haven't encountered 20 errors related to importing images, we can continue
    // to import images.
    if (*m_numErrors < 20) {
      try {
        QString destination = QFileInfo(m_destinationFolder, QString::fromStdString(original.name())).absoluteFilePath();
        Cube *input = new Cube(original, "r");

        if (m_copyDnData) {
          Cube *copiedCube = input->copy(destination.toStdString(), CubeAttributeOutput());
          delete input;
          input = copiedCube;
        }

        FileName externalLabelFile(destination.toStdString());
        externalLabelFile = externalLabelFile.setExtension("ecub");

        Cube *projectImage = input->copy(externalLabelFile, CubeAttributeOutput("+External"));

        if (m_copyDnData) {
          // Make sure the external label has a fully relative path to the DN data
          projectImage->relocateDnData(FileName(destination.toStdString()).name());
        }

        //  Set new ecub to readOnly.  When closing cube, the labels were being re-written because
        // the cube was read/write. This caused a segfault when imported large number of images
        // because of a label template file being opened too many times.
        projectImage->reopen();

        delete input;

        result = projectImage;
      }
      // When we encounter an exception, update the m_errors and m_numErrors to with the exception
      // that occurred.
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
   * @brief Indicates if any errors occurred during the import.
   *
   * Returns an IException that details any errors that occurred during the import.
   * Note that if there have been 20 or more errors, the exception returned will indicate that the
   * import was aborted because too many errors have occurred.
   *
   * @return IExecption Returns an IException indicating what errors occured during the import.
   */
  IException ImportImagesWorkOrder::OriginalFileToProjectCubeFunctor::errors() const {
    IException result;

    result.append(*m_errors);

    if (*m_numErrors >= 20) {
      result.append(
          IException(IException::Unknown,"Aborted import images due to a high number of errors",
                     _FILEINFO_));
    }
    return result;
  }


  /**
   * @brief Imports the images.
   *
   * Creates a project image folder and copies the cubes into it. This will create
   * the *.ecub and *.cub files inside of the project.
   * This can be thought of as:
   * <pre>
   *   mkdir project/images/import1
   *   cp in1.cub in2.cub project/images/import1
   * </pre>
   *
   * This should be called in a non-GUI thread.
   *
   * @param confirmedImages This is a list of cube file names outside of the project folder
   * @param copyDnData If this is true, this will create both the *.cub and *.ecub files in the
   *                   project. Otherwise, only the external label files (*.ecub) will be created
   *                   inside of the project.
   */
  void ImportImagesWorkOrder::importConfirmedImages(QStringList confirmedImages, bool copyDnData) {
    try {
      if (!confirmedImages.isEmpty()) {
        QDir folder = project()->addImageFolder("import");

        setProgressRange(0, confirmedImages.count());

        // We are creating a new QObject within an asynchronous execute(), which means that this
        // variable, m_newImages, has thread affinity with a thread in the gloabal thread pool
        // (i.e. m_newImages lives in a thread in the global thread pool).
        // see WorkOrder::redo().
        m_newImages = new ImageList;
        m_newImages->reserve(confirmedImages.count());

        QStringList confirmedImagesFileNames;
        QStringList confirmedImagesIds;

        foreach (QString confirmedImage, confirmedImages) {
          QStringList fileNameAndId = confirmedImage.split(",");
          confirmedImagesFileNames.append(fileNameAndId.first());

          // Determine if there was already a unique id provided for the file.
          if (fileNameAndId.count() == 2) {
            confirmedImagesIds.append(fileNameAndId.last());
          }
          else {
            confirmedImagesIds.append(QString());
          }
        }

        std::vector<std::string> stdStringList;
        stdStringList.reserve(confirmedImagesFileNames.size());
        for (const QString &qstr : confirmedImagesFileNames) {
          stdStringList.push_back(qstr.toStdString());
        }

        OriginalFileToProjectCubeFunctor functor(thread(), folder, copyDnData);
        // Start concurrently copying the images to import.
        QFuture<Cube *> future = QtConcurrent::mapped(stdStringList, functor);

        // The new internal data will store the copied files as well as their associated unique id's.
        QStringList newInternalData;
        newInternalData.append(internalData().first());

        // By releasing a thread from the global thread pool, we are effectively temporarily
        // increasing the max number of available threads. This is useful when a thread goes to sleep
        // waiting for more work, so we can allow other threads to continue.
        // See Qt's QThreadPool::releaseThread() documentation.
        QThreadPool::globalInstance()->releaseThread();
        for (int i = 0; i < confirmedImages.count(); i++) {
          setProgressValue(i);

          // This will wait for the result at i to finish (the functor invocation finishes) and
          // get the cube.
          Cube *cube = future.resultAt(i);

          if (cube) {

            // Confirm that the target body and the gui camera do not exist before creating and 
            // and adding them for each image. Since a target may be covered by many cameras and a 
            // camera may cover many targets, have to get tricky with the checking.
            QString instrumentId = QString::fromStdString(cube->label()->findGroup("Instrument", 
                              PvlObject::FindOptions::Traverse).findKeyword("InstrumentId")[0]);
            QString targetName = QString::fromStdString(cube->label()->findGroup("Instrument", 
                              PvlObject::FindOptions::Traverse).findKeyword("TargetName")[0]);
            if (!project()->hasTarget(targetName)) {
              Camera *camera = cube->camera();
              Target *target = camera->target();
              project()->addTarget(target);
              
              if (!project()->hasCamera(instrumentId)) {
                project()->addCamera(camera);
              }
            }
            else if (!project()->hasCamera(instrumentId)) {
              Camera *camera = cube->camera();
              project()->addCamera(camera);
            }

            // Create a new image from the result in the thread spawned in WorkOrder::redo().
            Image *newImage = new Image(cube);
            newImage->closeCube();
            // Memory for cube is deleted in Image::closeCube()
            cube = NULL;

            // Either use a unique id that was already provided or create one for the new image.
            if (confirmedImagesIds[i].isEmpty()) {
              confirmedImagesIds[i] = newImage->id();
            }
            else {
              newImage->setId(confirmedImagesIds[i]);
            }

            QStringList imageInternalData;
            imageInternalData.append(confirmedImagesFileNames[i]);
            imageInternalData.append(confirmedImagesIds[i]);

            newInternalData.append(imageInternalData.join(","));

            m_newImages->append(newImage);

            // Move the new image back and its display properities to the GUI thread.
            // Note: thread() returns the GUI thread because this ImportImagesWorkOrder lives
            // (was created) in the GUI thread.
            newImage->moveToThread(thread());
            newImage->displayProperties()->moveToThread(thread());
          }
        }
        // Since we temporarily increased the max thread count (by releasing a thread), make sure
        // to re-reserve the thread for the global thread pool's accounting.
        // See Qt's QThreadPool::reserveThread().
        QThreadPool::globalInstance()->reserveThread();

        m_warning = QString::fromStdString(functor.errors().toString());

        // Recall that m_newImages has thread affinity with a thread in the global thread pool.
        // Move it to the GUI-thread because these threads in the pool do not run in an event loop,
        // so they cannot process events.
        // See https://doc.qt.io/qt-5/threads-qobject.html#per-thread-event-loop
        // See http://doc.qt.io/qt-5/threads-technologies.html#comparison-of-solutions
        m_newImages->moveToThread(thread());

        if (m_newImages->isEmpty()) {
          folder.removeRecursively();
        }

        setInternalData(newInternalData);
      }
    }
    catch (IException &e) {
        QMessageBox::critical(NULL, tr("Error"), tr(e.what()));
    }
  }
}
