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
#include "ImportControlNetWorkOrder.h"

#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QtConcurrentMap>

#include "Control.h"
#include "ControlList.h"
#include "ControlNet.h"
#include "IException.h"
#include "Progress.h"
#include "Project.h"
#include "ProjectItem.h"
#include "ProjectItemModel.h"

namespace Isis {

  /**
   * Creates a work order to import a control network.
   *
   * @param *project Pointer to the project this work order belongs to
   */
  ImportControlNetWorkOrder::ImportControlNetWorkOrder(Project *project) :
      WorkOrder(project) {

    // This is an asynchronous workorder
    m_isSynchronous = false;
    m_list = NULL;
    m_watcher = NULL;
    m_isUndoable = false;

    QAction::setText(tr("Import &Control Networks..."));

    setModifiesDiskState(true);

    m_watcher = new QFutureWatcher<Control *>;
    connect(m_watcher, SIGNAL(resultReadyAt(int)), this, SLOT(cnetReady(int)));
  }


  /**
   * Creates a copy of the other ImportControlNetWorkOrder
   *
   * @param &other ImportControlNetsWorkOrder to copy the state from
   */
  ImportControlNetWorkOrder::ImportControlNetWorkOrder(const ImportControlNetWorkOrder &other) :
      WorkOrder(other) {
    m_watcher = NULL;
    m_watcher = new QFutureWatcher<Control *>;
    connect(m_watcher, SIGNAL(resultReadyAt(int)), this, SLOT(cnetReady(int)));
  }


  /**
   * Destructor
   */
  ImportControlNetWorkOrder::~ImportControlNetWorkOrder() {
    delete m_watcher;
    m_watcher = NULL;
  }


  /**
   * This method clones the current ImportControlNetWorkOrder and returns it.
   *
   * @return ImportControlNetWorkOrder Clone
   */
  ImportControlNetWorkOrder *ImportControlNetWorkOrder::clone() const {
    return new ImportControlNetWorkOrder(*this);
  }


  /**
   * This method returns true if the user clicked on a project tree node with the text
   * "Control Networks".
   * This is used by Directory::supportedActions(DataType data) to determine what actions are
   * appended to context menus.
   *
   * @param item The ProjectItem that was clicked
   *
   * @return bool True if the user clicked on a project tree node named "Control Network"
   */
  bool ImportControlNetWorkOrder::isExecutable(ProjectItem *item) {

    if (item) {
      return (item->text() == "Control Networks");
    }
    return false;
  }


  /**
   * @brief Sets up the work order for execution.
   *
   * This method prompts the user for a control net to open. That control net is then
   * saved using setInternalData data. This method was renamed from execute() to setupExecution()
   *
   * @see WorkOrder::setupExecution()
   *
   * @return bool Returns a boolean. This boolean is true if the internal data was set correctly.
   */
  bool ImportControlNetWorkOrder::setupExecution() {
    QUndoCommand::setText(tr("Import Control Networks"));

    WorkOrder::setupExecution();

    QStringList cnetFileNames = QFileDialog::getOpenFileNames(
        qobject_cast<QWidget *>(parent()),
        tr("Import Control Networks"), "",
        tr("Isis control nets (*.net);;All Files (*)"));

    if (!cnetFileNames.isEmpty()) {
      QUndoCommand::setText(tr("Import %1 Control Networks").arg(cnetFileNames.count()));
    }

    setInternalData(cnetFileNames);

    return internalData().count() > 0;
  }

  /**
   * @brief Imports the control network asynchronously.
   *
   * This method asynchronously imports the control net. This method replaces both
   * syncRedo() and asyncRedo().
   */
  void ImportControlNetWorkOrder::execute() {
    QDir cnetFolder = project()->addCnetFolder("controlNetworks");

    QStringList cnetFileNames = internalData();

    QList< QPair<FileName, Progress *> > cnetFileNamesAndProgress;
    for (const QString &str : cnetFileNames) {
      FileName fileName(str.toStdString());
      Progress *readProgress = new Progress;
      cnetFileNamesAndProgress.append(qMakePair(fileName, readProgress));
      readProgress->DisableAutomaticDisplay();
      m_readProgresses.append(readProgress);
    }

    CreateControlsFunctor functor(project(), cnetFolder);
    m_watcher->setFuture(QtConcurrent::mapped(cnetFileNamesAndProgress,
                                              functor));

    while (!m_watcher->isFinished()) {
      setProgressRange(0, 100 * m_readProgresses.count());
      int totalProgress = 0;

      for (int i = 0; i < m_readProgresses.count(); i++) {
        Progress *progress = m_readProgresses[i];

        if (m_watcher->future().isResultReadyAt(i)) {
          totalProgress += 100;
        }
        else if (progress->MaximumSteps() > 0) {
          double progressPercent = progress->CurrentStep() / (double)progress->MaximumSteps();
          //  Estimating the Read is 90% and Write 10% (ish)
          totalProgress += qRound(progressPercent * 90);
        }
      }

      setProgressValue(totalProgress);

      m_warning = functor.errors().toString();

      QThread::yieldCurrentThread();
    }
  }

  /**
   * @brief Clears progress.
   *
   * This method clears the progresses created in execute(). This method was renamed
   * from postSyncRedo() to postExecution().
   */
  void ImportControlNetWorkOrder::postExecution() {

    if (m_warning != "") {
      project()->warn(m_warning);
    }

    foreach (Progress *progress, m_readProgresses) {
      delete progress;
    }
    m_status = WorkOrderFinished;
    m_readProgresses.clear();

    // If one control network was imported, no active control has been set, and no
    // other control networks exist in the project, then activeControl() will set
    // the active control to the newly imported control network.
    project()->activeControl();
  }

  /**
   * CreateControlsFunctor constructor
   *
   * @param project The project
   * @param destinationFolder The directory to copy to
   */
  ImportControlNetWorkOrder::CreateControlsFunctor::CreateControlsFunctor(
    Project *project, QDir destinationFolder) : m_errors(new IException) {
    m_project = project;
    m_destinationFolder = destinationFolder;
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
  IException ImportControlNetWorkOrder::CreateControlsFunctor::errors() const {
    IException result;

    result.append(*m_errors);

    return result;
  }


  /**
   * Reads and writes the control network(s) asynchronously
   *
   * @param &cnetFileNameAndProgress QPair of control net filenames, and the progress
   *
   * @return Control Pointer to the Control created from the import
   */
  Control *ImportControlNetWorkOrder::CreateControlsFunctor::operator()(
    const QPair<FileName, Progress *> &cnetFileNameAndProgress) {

    Control *control = NULL;
    try {
      QString cnetFileName = QString::fromStdString(cnetFileNameAndProgress.first.original());
      ControlNet *cnet = new ControlNet();
      cnet->SetMutex(m_project->mutex());
      cnet->ReadControl(cnetFileName, cnetFileNameAndProgress.second);

      QString baseFilename = QString::fromStdString(FileName(cnetFileName.toStdString()).name());
      QString destination = m_destinationFolder.canonicalPath() + "/" + baseFilename;

      cnet->Write(destination);

      delete cnet;
      cnet = NULL;

      control = new Control(m_project, destination);
      control->closeControlNet();
    }
    catch (IException &e) {
      m_errors->append(e);
      return NULL;
    }
    return control;
  }


  /**
   * Adds the control net to the project
   *
   * @param ready Index of the control net that is ready
   */
  void ImportControlNetWorkOrder::cnetReady(int ready) {
    QMutexLocker locker(project()->workOrderMutex());
    Control *control = m_watcher->resultAt(ready);

    if (control) {
      project()->addControl(control);
      m_list = project()->controls().last();
      project()->setClean(false);
    }
    else {
      m_list = NULL;
    }
  }
}
