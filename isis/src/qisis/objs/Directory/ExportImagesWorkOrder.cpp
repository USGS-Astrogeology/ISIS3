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
#include "ExportImagesWorkOrder.h"

#include <QDebug>
#include <QFileDialog>
#include <QFuture>
#include <QInputDialog>
#include <QtConcurrentMap>

#include "Cube.h"
#include "CubeAttribute.h"
#include "Image.h"
#include "ImageList.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Project.h"

namespace Isis {

  ExportImagesWorkOrder::ExportImagesWorkOrder(Project *project) :
      WorkOrder(project) {

    // This is an asynchronous workorder and not undoable
    m_isSynchronous = false;
    m_isUndoable = false;

    QAction::setText(tr("Export I&mages..."));
  }


  ExportImagesWorkOrder::ExportImagesWorkOrder(const ExportImagesWorkOrder &other) :
      WorkOrder(other) {
  }


  ExportImagesWorkOrder::~ExportImagesWorkOrder() {

  }


  ExportImagesWorkOrder *ExportImagesWorkOrder::clone() const {
    return new ExportImagesWorkOrder(*this);
  }


  /**
   * Currently, this work order only works with either no data (file menu) or with any 
   *   number of images.
   * 
   * @param controls The current context we're inquiring about
   * 
   * @return bool True if this work order functions with the given image list
   */
  bool ExportImagesWorkOrder::isExecutable(ImageList *images) {
    if (images) {
      return (!images->isEmpty());
    }
    return false;
  }


  /**
   * Prompts the user for input. If there is no context, we ask the user to select an image list.
   *   Once we have images (via context or asking the user), we then ask for a output directory.
   *   The relevant data is stored in internalData().
   * 
   * @return bool 
   */
  bool ExportImagesWorkOrder::setupExecution() {
    bool success = WorkOrder::setupExecution();

    if (success) {
      QStringList internalData;

      if (imageList()->isEmpty()) {
        QStringList imageListNames;
        foreach (ImageList *list, project()->images()) {
          imageListNames.append(list->name());
        }

        imageListNames.sort();

        QString choice = QInputDialog::getItem(NULL, tr("Select Image List"),
            tr("Please choose a list of images to export."), imageListNames, 0, false, &success);

        internalData.append(choice);

        QUndoCommand::setText(tr("Export image list [%1]").arg(choice));
      }
      else {
        QUndoCommand::setText(tr("Export [%1] images").arg(imageList()->count()));
      }

      QString destination = QFileDialog::getExistingDirectory(NULL, tr("Export Images"), ".");

      if (destination.isEmpty()) {
        success = false;
      }
      internalData.append(destination);

      setInternalData(internalData);
    }

    return success;
  }


  /**
   * Use internalData() and write the images into the output directory. Stores errors in 
   *   m_warning which will be reported in postSyncRedo().
   */
  void ExportImagesWorkOrder::execute() {
    ImageList *list = imageList();

    if (list->isEmpty()) {
      list = project()->imageList(internalData()[0]);
    }


    QString destination = internalData().last();
    ProjectImageExportFunctor functor(destination);

    QFuture<void *> future = QtConcurrent::mapped(*list, functor);

    setProgressRange(0, list->count());

    QThreadPool::globalInstance()->releaseThread();
    for (int i = 0; i < list->count(); i++) {
      setProgressValue(i);

      // Wait for the result before updating the progress to the next value... it's still
      //   working on writing N images, just progress is going 1 by 1.
      future.resultAt(i);
    }
    QThreadPool::globalInstance()->reserveThread();

    // Get the errors that occurred during the mapped in order to report them in postSyncRedo
    m_warning = QString::fromStdString(functor.errors().toString());
  }


  /**
   * Display any warnings that occurred during the asynchronous computations.
   */
  void ExportImagesWorkOrder::postExecution() {
    if (!m_warning.isEmpty()) {
      project()->warn(m_warning);
      m_warning.clear();
    }
  }


  /**
   * Create an image export functor that will copy the image's cubes into the given destination 
   *   directory.
   * 
   * @param destination The directory to copy cubes into in the () operator.
   */
  ExportImagesWorkOrder::ProjectImageExportFunctor::ProjectImageExportFunctor(
      QString destination) : m_errors(new IException()), m_numErrors(new int(0)) {

    m_destination = destination;
  }


  /**
   * Copy the functor. This will share the error reporting data across instances so that you can 
   *   access the errors gathered during a QtConcurrentMap (which copies the functor).
   * 
   * @param other The functor to copy
   */
  ExportImagesWorkOrder::ProjectImageExportFunctor::ProjectImageExportFunctor(
      const ProjectImageExportFunctor &other) : m_errors(other.m_errors),
      m_numErrors(other.m_numErrors) {

    m_destination = other.m_destination;
  }


  /**
   * Destroys the functor
   */
  ExportImagesWorkOrder::ProjectImageExportFunctor::~ProjectImageExportFunctor() {
  }


  /**
   * Write the given image's cube into the destination folder (preserves the base name). 
   * 
   * @param imageToExport Image to export
   * 
   * @return void* Always NULL, used for QtConcurrentMap compatibility
   */
  void *ExportImagesWorkOrder::ProjectImageExportFunctor::operator()(Image * const &imageToExport) {
    try {
      FileName outputFileName =
          m_destination.toStdString() + "/" + FileName(imageToExport->fileName().toStdString()).baseName();

      // Copy the image, free the copy from memory because we don't need it.
      delete imageToExport->cube()->copy(outputFileName, CubeAttributeOutput());
      // Avoid opening too many cubes
      imageToExport->closeCube();
    }
    catch (IException &e) {
      QMutexLocker locker(&m_errorsLock);
      m_errors->append(e);
      (*m_numErrors)++;
    }

    return NULL;
  }


  /**
   * Get the accumulated error list from this functor's run. This will return a default-constructed 
   *   (empty/blank) exception if no errors were encountered. 
   * 
   * @return IException A list of errors that occurred during the cube copies.
   */
  IException ExportImagesWorkOrder::ProjectImageExportFunctor::errors() const {
    IException result;

    result.append(*m_errors);

    if (*m_numErrors != 0) {
      result.append(
          IException(IException::Unknown,"Failed to export [" + std::to_string(*m_numErrors) + "] images",
                     _FILEINFO_));
    }

    return result;
  }
}
