/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "WorkOrder.h"

#include <QDebug>
#include <QFutureWatcher>
#include <QMutex>
#include <QMutexLocker>
#include <QtConcurrentRun>
#include <QTimer>
#include <QXmlStreamWriter>

#include "ControlList.h"
#include "IException.h"
#include "ImageList.h"
#include "IString.h"
#include "ProgressBar.h"
#include "Project.h"
#include "ProjectItem.h"
#include "ShapeList.h"
#include "Template.h"


namespace Isis {
  /**
   * @brief Create a work order that will work with the given project.
   * @param project The Project that this work order should be interacting with
   * @param parent The Qt-relationship parent
   * @throws IException::Programmer This exception is thrown if the WorkOrder is
   * not attached to a Project.
   */
  WorkOrder::WorkOrder(Project *project) : QAction(project) {
    m_project = project;

    m_context = NoContext;
    m_data = "";
    m_imageList = new ImageList;
    m_shapeList = new ShapeList;
    m_controlList = NULL;
    m_correlationMatrix = CorrelationMatrix();
    m_guiCamera = GuiCameraQsp();
    m_targetBody = TargetBodyQsp();
    m_template = NULL;
    m_fileItem = FileItemQsp();

    m_isUndoable = true;
    m_isSavedToHistory = true;
    m_isSynchronous = true;

    m_createsCleanState = false;
    m_modifiesDiskState = false;
    m_status = WorkOrderNotStarted;
    m_queuedAction = NoQueuedAction;
    m_transparentConstMutex = NULL;
    m_elapsedTimer = NULL;

    m_futureWatcher = new QFutureWatcher<void>;
    m_transparentConstMutex = new QMutex;

    m_progressRangeMinValue = 0;
    m_progressRangeMaxValue = 100;
    m_progressValue = 0;

    m_secondsElapsed = 0.0;

    if (!m_project) {
        throw IException(IException::Programmer,
            "Work orders cannot be created without a project.", _FILEINFO_);
    }

    connect(this, SIGNAL(triggered()),
            this, SLOT(addCloneToProject()));
    connect(m_futureWatcher, SIGNAL(finished()),
            this, SLOT(executionFinished()));
  }


  /**
   * @brief Copy the work order 'other' into this (new) instance.
   * @param other The work order being copied.
   * @throws IException::Unknown  This Excecption is thrown if the WorkOrder being copies
   * is currently running.
   */
  WorkOrder::WorkOrder(const WorkOrder &other) :
      QAction(other.icon(), ((QAction &)other).text(), other.parentWidget()),
      QUndoCommand(((QUndoCommand &)other).text()) {
    // Copy the action's what's this and tool tip (hover text).
    QAction::setWhatsThis(other.whatsThis());
    QAction::setToolTip(other.toolTip());

    m_transparentConstMutex = NULL;
    m_elapsedTimer = NULL;
    m_project = other.project();
    m_context = other.m_context;
    m_imageIds = other.m_imageIds;
    m_imageList = new ImageList(*other.m_imageList);
    m_shapeIds = other.m_shapeIds;
    m_shapeList = new ShapeList(*other.m_shapeList);
    m_correlationMatrix = other.m_correlationMatrix;
    m_controlList = other.m_controlList;
    m_guiCamera = other.m_guiCamera;
    m_targetBody = other.m_targetBody;
    m_fileItem = other.m_fileItem;
    m_internalData = other.m_internalData;
    m_template = other.m_template;

    m_isUndoable = other.m_isUndoable;
    m_isSavedToHistory = other.m_isSavedToHistory;
    m_isSynchronous = other.m_isSynchronous;

    m_createsCleanState = other.m_createsCleanState;
    m_modifiesDiskState = other.m_modifiesDiskState;

    m_executionTime = other.m_executionTime;

    m_status = other.m_status;
    m_queuedAction = other.m_queuedAction;

    m_secondsElapsed = other.m_secondsElapsed;

    m_progressRangeMinValue = other.m_progressRangeMinValue;
    m_progressRangeMaxValue = other.m_progressRangeMaxValue;
    m_progressValue = other.m_progressValue;

    m_futureWatcher = new QFutureWatcher<void>;
    m_transparentConstMutex = new QMutex;

    if (!other.isInStableState()) {
      throw IException(IException::Unknown,
          "Can not copy work order ["+((QUndoCommand &)other).text().toStdString()+"] because it is currently running",
          _FILEINFO_);
    }

    connect(this, SIGNAL(triggered()),
            this, SLOT(addCloneToProject()));
    connect(m_futureWatcher, SIGNAL(finished()),
            this, SLOT(executionFinished()));

    listenForImageDestruction();
    listenForShapeDestruction();
  }


  /**
   * @brief The Destructor
   */
  WorkOrder::~WorkOrder() {

    delete m_imageList;
    delete m_shapeList;
    delete m_futureWatcher;
    delete m_progressBarDeletionTimer;
    delete m_progressBarUpdateTimer;
    delete m_transparentConstMutex;

    m_nextWorkOrder = NULL;
    m_previousWorkOrder = NULL;
    m_project = NULL;
    m_transparentConstMutex = NULL;
  }


  /**
   * @brief Re-implement this method if your work order utilizes controls for data
   * in order to operate. For example, "CnetEditorViewWorkOrder" works sometimes on controls
   * - the logic in side of CnetEditorViewWorkOrder::isExecutable() determines whethere or not a
   * user is prompted with this work order as a possibility.
   * @param context This is an enum variable with two values:   NoContext,ProjectContext.
   * @return @b bool Upon re-implementation, returns True if the WorkOrder is executable, and False
   * if it is not.
   */
  bool WorkOrder::isExecutable(Context context) {
    return false;
  }


  /**
   * @brief Re-implement this method if your work order utilizes images for data in order to
   *  operate. For example, "Footprint2DViewWorkOrder" works sometimes on images - the logic
   * in side of Footprint2DViewWorkOrder::isExecutable(ImageList) determines whethere or not a user
   * is prompted with this work order as a possibility.
   * @param images An image list that this work order should execute on
   * @return @b bool Upon re-implementation, returns True if the WorkOrder is executable, and False
   * if it is not.
   */
  bool WorkOrder::isExecutable(ImageList *images) {
    return false;
  }


  /**
   * @brief Re-implement this method if your work order utilizes shapes for data in order to
   *  operate. For example, "ImportShapeWorkOrder" works on shapes - the logic
   * in side of ImportShapeWorkOrder::isExecutable(ShapeList) determines whethere or not a user is
   * prompted with this work order as a possibility.
   * @param shapes A shape list that this work order should execute on
   * @return @b bool Upon re-implementation, returns True if the WorkOrder is executable, and False
   * if it is not.
   */
  bool WorkOrder::isExecutable(ShapeList *shapes) {
    return false;
  }


  /**
   * @brief Re-implement this method if your work order utilizes a control
   * for data in order to operate.
   * @param control A control networks.
   * @return @b bool Upon re-implementation, returns True if the WorkOrder is executable, and False
   * if it is not.
   */
//bool WorkOrder::isExecutable(Control *control) {
//  return false;
//}


  /**
   * @brief Re-implement this method if your work order utilizes a control list (a list of control
   * networks) for data in order to operate.
   * @param controls A list of control networks.
   * @return @b bool Upon re-implementation, returns True if the WorkOrder is executable, and False
   * if it is not.
   */
  bool WorkOrder::isExecutable(ControlList *controls) {
    return false;
  }


  bool WorkOrder::isExecutable(CorrelationMatrix correlationMatrix) {
    return false;
  }


  /**
   * @brief Sets the context data for this WorkOrder
   * @param context This is an enum variable with two values:   NoContext,ProjectContext.
   */
  void WorkOrder::setData(Context context) {
    m_context = context;
  }

  void WorkOrder::setData(QString data) {
    m_data = data;
  }


  /**
   * @brief Sets the ImageList data for this WorkOrder
   * @param images A pointer to the updated ImageList.
   */
  void WorkOrder::setData(ImageList *images) {
    m_imageIds.clear();
    delete m_imageList;

    m_imageList = new ImageList(*images);
    listenForImageDestruction();
  }


  /**
   * @brief Sets the ShapeList data for this WorkOrder
   * @param images A pointer to the updated ShapeList.
   */
  void WorkOrder::setData(ShapeList *shapes) {
    m_shapeIds.clear();
    delete m_shapeList;

    m_shapeList = new ShapeList(*shapes);
    listenForShapeDestruction();
  }


  /**
   * @brief Sets the Control data for this WorkOrder.
   * @param controls.  A pointer to the Control
   */
//void WorkOrder::setData(Control *control) {
//  m_control = control;
//}


  /**
   * @brief Sets the ControlList data for this WorkOrder.
   * @param controls.  A pointer to the ControlList (which is a list of control
   * networks).
   */
  void WorkOrder::setData(ControlList *controls) {
    m_controlList = controls;
  }


  /**
   * @brief Sets the CorrelationMatrix data for this WorkOrder.
   * @param correlationMatrix The matrix data.
   */
  void WorkOrder::setData(CorrelationMatrix correlationMatrix) {
    m_correlationMatrix = correlationMatrix;
  }




  /**
   * @brief Sets the TargetBody data for this WorkOrder.
   * @param targetBody A QSharedPointer to the TargetBody.
   */
  void WorkOrder::setData(TargetBodyQsp targetBody) {
    m_targetBody = targetBody;
  }


  /**
   * @brief Sets the TargetBody data for this WorkOrder.
   * @param targetBody A QSharedPointer to the TargetBody.
   */
  void WorkOrder::setData(Template *currentTemplate) {
    m_template = currentTemplate;
  }


  /**
   * @brief Sets the GuiCamera data for this WorkOrder.
   * @param guiCamera A QSharedPointer to the GuiCamera.
   */
  void WorkOrder::setData(GuiCameraQsp guiCamera) {
    m_guiCamera = guiCamera;
  }


  /**
   * @brief Sets the FileItem data for this WorkOrder.
   * @param FileItem A QSharedPointer to the FileItem.
   */
  void WorkOrder::setData(FileItemQsp fileItem) {
    m_fileItem = fileItem;
  }


  /**
   * @brief Sets the internal data to the data stored in a ProjectItem.
   * @param item The item containing the data.
   */
  void WorkOrder::setData(ProjectItem *item) {
    if ( item->isProject() ) {
      setData( ProjectContext );
    }
    else if ( item->isImageList() ) {
      setData( item->imageList() );
    }
    else if ( item->isImage() ) {
      ImageList *imageList = new ImageList(this);
      imageList->append( item->image() );
      setData(imageList);
    }
    else if ( item->isShapeList() ) {
      setData( item->shapeList() );
    }
    else if ( item->isShape() ) {
      ShapeList *shapeList = new ShapeList(this);
      shapeList->append( item->shape() );
      setData(shapeList);
    }
    else if (item->isControlList()) {
      setData( item->controlList() );
    }
    else if ( item->isControl() ) {
      ControlList *controlList = new ControlList(this);
      controlList->append( item->control() );
      setData(controlList);
//    //setData(*controlList);
    }
    else if ( item->isCorrelationMatrix() ) {
      setData( item->correlationMatrix() );
    }
    else if ( item->isTargetBody() ) {
      setData( item->targetBody() );
    }
    else if ( item->isGuiCamera() ) {
      setData( item->guiCamera() );
    }
    else if ( item->isFileItem() ) {
      setData( item->fileItem() );
    }
    else if ( item->isTemplate() ) {
      setData( item->getTemplate() );
    }
  }





  /**
   * @brief Re-implement this method if your work order utilizes a control list (a list of control
   * networks) for data in order to operate.
   * @param controls A list of control networks.
   * @return @b bool Upon re-implementation, returns True if the WorkOrder is executable, and False
   * if it is not.
   */
  bool WorkOrder::isExecutable(TargetBodyQsp targetBody) {
    return false;
  }


  /**
   * @brief Re-implement this method if your work order utilizes a control list (a list of control
   * networks) for data in order to operate.
   * @param controls A list of control networks.
   * @return @b bool Upon re-implementation, returns True if the WorkOrder is executable, and False
   * if it is not.
   */
  bool WorkOrder::isExecutable(Template *currentTemplate) {
    return false;
  }


  /**
   * @brief Re-implement this method if your WorkOrder utilizes GuiCameraQsp (a QSharedPointer to a
   * GuiCamera object) for data in order to operate.
   * @param GuiCameraQsp
   * @return @b bool Upon re-implementation, returns True if the WorkOrder is executable, and False
   * if it is not.
   */
  bool WorkOrder::isExecutable(GuiCameraQsp guiCamera) {
    return false;
  }


  /**
   * @brief Re-implement this method if your WorkOrder utilizes FileItemQsp (a QSharedPointer to a
   * FileItem object) for data in order to operate.
   * @param FileItemQsp
   * @return @b bool Upon re-implementation, returns True if the WorkOrder is executable, and False
   * if it is not.
   */
  bool WorkOrder::isExecutable(FileItemQsp fileItem) {
    return false;
  }


  /**
   * @brief Determines if the WorkOrder is execuatable on the data stored in
   * a ProjectItem.
   * @param item (ProjectItem *) The item containing the data.
   * @return @b bool Returns True if the WorkOrder is executable on data
   * stored in a ProjectItem.
   */
  bool WorkOrder::isExecutable(ProjectItem *item) {
    if ( !item ) {
      return false;
    }
    else if ( item->isProject() ) {
      return isExecutable( ProjectContext );
    }
    else if ( item->isImageList() ) {
      return isExecutable( item->imageList() );
    }
    else if ( item->isImage() ) {
      ImageList *imageList = new ImageList();
      imageList->append( item->image() );
      bool ret = isExecutable(imageList);
      imageList->deleteLater();
      return ret;
    }
    else if ( item->isShapeList() ) {
      return isExecutable( item->shapeList() );
    }
    else if ( item->isShape() ) {
      ShapeList *shapeList = new ShapeList();
      shapeList->append( item->shape() );
      bool ret = isExecutable(shapeList);
      shapeList->deleteLater();
      return ret;
    }
    else if ( item->isControlList() ) {
      return isExecutable (item -> controlList() );
    }
    else if ( item->isControl() ) {
      ControlList *controlList = new ControlList();
      controlList->append( item->control() );
      bool ret = isExecutable(controlList);
      controlList->deleteLater();
      return ret;
    }
    else if ( item->isCorrelationMatrix() ) {
      return isExecutable( item->correlationMatrix() );
    }
    else if ( item->isTargetBody() ) {
      //return isExecutable( item->targetBody() ) || isExecutable( item->targetBody().data() );
        return isExecutable(item->targetBody());
    }
    else if ( item->isGuiCamera() ) {
      //return isExecutable( item->guiCamera() ) || isExecutable( item->guiCamera().data() );
      return isExecutable( item->guiCamera() );
    }
    else if ( item->isFileItem() ) {
      return isExecutable( item->fileItem() );
    }
    else if ( item->isTemplate() ) {
      return isExecutable( item->getTemplate() );
    }

    return false;
  }

  /**
   * @brief:  Saves a WorkOrder to a data stream.
   * The XML output format looks like this:
   *
   * <pre>
   *   <workOrder actionText="..." undoText="..." type="..." status="...">
   *     <images>
   *       <image id="..." />
   *     </images>
   *
   *     <internalDataValues>
   *       <dataValue value="..." />
   *     </internalDataValues>
   *   </workOrder>
   * </pre>
   * @param @b QXmlStreamWriter stream The data stream we are saving the WorkOrder to.
   * @throws IException::Unknown  This exception is thrown if save is called while the
   * WorkOrder is currently running.
   */
  void WorkOrder::save(QXmlStreamWriter &stream) const {
    if (!isInStableState()) {
      throw IException(IException::Programmer,
                       "Can not store an unstable work order. The work order ["+bestText().toStdString()+"] is currently "
                          "working",
                       _FILEINFO_);
    }

    stream.writeStartElement("workOrder");

    stream.writeAttribute("actionText", QAction::text());
    stream.writeAttribute("undoText", QUndoCommand::text());
    stream.writeAttribute("executionTime", m_executionTime.toString());
    stream.writeAttribute("type", metaObject()->className());
    stream.writeAttribute("status", toString(m_status));

    if (m_imageIds.count()) {
      stream.writeStartElement("images");

      foreach (QString imageId, m_imageIds) {
        stream.writeStartElement("image");
        stream.writeAttribute("id", imageId);
        stream.writeEndElement();
      }

      stream.writeEndElement();
    }

    if (m_shapeIds.count()) {
      stream.writeStartElement("shapes");

      foreach (QString shapeId, m_shapeIds) {
        stream.writeStartElement("shape");
        stream.writeAttribute("id", shapeId);
        stream.writeEndElement();
      }

      stream.writeEndElement();
    }

    if (m_internalData.count()) {
      stream.writeStartElement("internalDataValues");

      foreach (QString str, m_internalData) {
        stream.writeStartElement("dataValue");
        stream.writeAttribute("value", str);
        stream.writeEndElement();
      }

      stream.writeEndElement();
    }

    if (m_context != NoContext) {
      stream.writeStartElement("context");

      QString contextStr = "ProjectContext";
      stream.writeAttribute("value", contextStr);

      stream.writeEndElement();
    }

    stream.writeEndElement();
  }


  /**
   * @brief Sets the next WorkOrder in the sequence.
   * @param nextWorkOrder The next WorkOrder.
   */
  void WorkOrder::setNext(WorkOrder *nextWorkOrder) {
    m_nextWorkOrder = nextWorkOrder;
  }


  /**
   * @brief Sets the previous WorkOrder in the sequence.
   * @param previousWorkOrder The previous WorkOrder.
   */
  void WorkOrder::setPrevious(WorkOrder *previousWorkOrder) {
    m_previousWorkOrder = previousWorkOrder;
  }


  /**
   * @brief Returns a pointer to the ImageList for this WorkOrder.
   * @return @b (ImageList*) A pointer to the ImageList.
   */
  ImageList *WorkOrder::imageList() {
    QMutexLocker locker(project()->workOrderMutex());
    if (!m_imageList) {
      bool anyImagesAreNull = false;

      m_imageList = new ImageList;

      foreach (QString id, m_imageIds) {
        Image *image = project()->image(id);
        m_imageList->append(image);

        if (!image) {
          anyImagesAreNull = true;
        }
      }

      if (anyImagesAreNull) {
        delete m_imageList;
      }
      else {
        listenForImageDestruction();
      }
    }

    return m_imageList;
  }


  /**
   * @briefReturns a pointer to the ShapeList for this WorkOrder.
   * @return @b (ShapeList*) A pointer to the ShapeList.
   */
  ShapeList *WorkOrder::shapeList() {
    QMutexLocker locker(project()->workOrderMutex());
    if (!m_shapeList) {
      bool anyShapesAreNull = false;

      m_shapeList = new ShapeList;

      foreach (QString id, m_shapeIds) {
        Shape *shape = project()->shape(id);
        m_shapeList->append(shape);

        if (!shape) {
          anyShapesAreNull = true;
        }
      }

      if (anyShapesAreNull) {
        delete m_shapeList;
      }
      else {
        listenForShapeDestruction();
      }
    }

    return
      m_shapeList;
  }


  /**
   * @brief Returns the CorrleationMatrix for this WorkOrder
   * @return A CorrelationMatrix.
   */
  CorrelationMatrix WorkOrder::correlationMatrix() {
    QMutexLocker locker(project()->workOrderMutex());
    return m_correlationMatrix;
  }


  /**
   * @brief Returns the Control List for this WorkOrder (a list of control networks).
   * @return QPointer<ControlList>  Returns m_controlList.
   */
  QPointer<ControlList> WorkOrder::controlList() {
    QMutexLocker locker(project()->workOrderMutex());
    return m_controlList;
  }


  /**
   * @brief A thread-safe method for retrieving a pointer to the imageList.
   * @return @b (ImageList *) A pointer to the image list for this WorkOrder.
   */
  const ImageList *WorkOrder::imageList() const {
    QMutexLocker lock(m_transparentConstMutex);
    return const_cast<WorkOrder *>(this)->imageList();
  }


  /**
   * @brief A thread-safe method for retrieving a pointer to the shapeList.
   * @return @b (ShapeList *) A pointer to the shape list for this WorkOrder.
   */
  const ShapeList *WorkOrder::shapeList() const {
    QMutexLocker lock(m_transparentConstMutex);
    return const_cast<WorkOrder *>(this)->shapeList();
  }


  /**
   * @brief WorkOrder::getTemplate
   * @return @b QSharedPointer Returns a shared pointer to the Template.
   */
  Template *WorkOrder::getTemplate() {
    QMutexLocker locker(project()->workOrderMutex());
    return m_template;
  }


  /**
   * @brief WorkOrder::targetBody
   * @return @b QSharedPointer Returns a shared pointer to the TargetBody.
   */
  TargetBodyQsp WorkOrder::targetBody() {
    QMutexLocker locker(project()->workOrderMutex());
    return m_targetBody;
  }


  /**
   * @brief WorkOrder::guiCamera
   * @return @b QSharedPointer Returns a shared pointer to the guiCamera.
   */
  GuiCameraQsp WorkOrder::guiCamera() {
    QMutexLocker locker(project()->workOrderMutex());
    return m_guiCamera;
  }


  /**
   * @brief WorkOrder::fileItem
   * @return @b QSharedPointer Returns a shared pointer to the fileItem.
   */
  FileItemQsp WorkOrder::fileItem() {
    QMutexLocker locker(project()->workOrderMutex());
    return m_fileItem;
  }


  /**
   * @brief Indicate workorder dependency
   * This is a virtual function whose role in child classes is to determine
   * if this WorkOrder depends on the WorkOrder passed in as an argument.
   * @param WorkOrder * The WorkOrder we are checking for dependency with this one.
   * @return @b bool Returns True if there is a dependency, and False if there is no
   * dependency.
   *
   */
  bool WorkOrder::dependsOn(WorkOrder *) const {
    return true;
  }


  /**
   * @brief Generate unique action names
   * We don't use action text anymore because Directory likes to rename our actions.
   * It converts a set of actions that have the same text, like Zoom Fit, to be in a menu named
   * Zoom Fit with items that name their widgets. Widget names are unhelpful as a description
   * of the action.
   * @see Directory::restructureActions
   * @return @b QString A textual description of the action.
   */
  QString WorkOrder::bestText() const {
    QString result = QUndoCommand::text().remove("&").remove("...");

    // if the QUndoCommand has no text, create a warning
    if (result.isEmpty()) {
      // get the name of the work order
      result = QString(metaObject()->className()).remove("Isis::").remove("WorkOrder")
                   .replace(QRegExp("([a-z0-9])([A-Z])"), "\\1 \\2");
      qWarning() << QString("WorkOrder::bestText(): Work order [%1] has no QUndoCommand text")
                    .arg(result);
    }

    return result;
  }


  /**
   * @brief Returns true if this work order is undoable, otherwise false. This needs to be set to
   *        true and createsCleanState needs to be set to false in order for a WorkOrder to appear
   *        on the undoStack. This should be done by setting the member variable and should not be
   *        overloading this function.
   *
   * @return @b (bool) Returns True if this work order is undoable, false if it is not.
   */
  bool WorkOrder::isUndoable() const {
    QMutexLocker locker(project()->workOrderMutex());
    return m_isUndoable;
  }


  /**
   * @brief Returns true if this work order is to be shown in History, otherwise false.
   *
   * @return @b (bool) Returns True if this work order is to be shown in History
   */
  bool WorkOrder::isSavedToHistory() const {
     QMutexLocker locker(project()->workOrderMutex());
     return m_isSavedToHistory;
  }


  /**
   * @brief Returns true if this work order is run synchronously, otherwise false.
   *
   * @return @b (bool) Returns True if this work order is run synchronously
   */
  bool WorkOrder::isSynchronous() const {
    QMutexLocker locker(project()->workOrderMutex());
    return m_isSynchronous;
  }


  /**
   * @brief Returns the CleanState status (whether the Project has been saved to disk or not). If
   *        this is set to true the work order will avoid being put on the undo stack, meaning it
   *        will not be undoable. It will also set the undo stack to a clean state, by doing this it
   *        makes all previous workorders done before the save to be no longer undoable.
   * @return @b Returns True if the Project has been saved to disk.  False if it has not.
   */
  bool WorkOrder::createsCleanState() const {
    QMutexLocker locker(project()->workOrderMutex());
    return m_createsCleanState;
  }


  /**
   * @brief Gets the execution time of this WorkOrder.
   * @return @b QDateTime The execution time.
   */
  QDateTime WorkOrder::executionTime() const {
    QMutexLocker locker(project()->workOrderMutex());
    return m_executionTime;
  }


  /**
   * @brief Returns the finished state of this WorkOrder.
   * @return @b bool Returns True if the WorkOrder is finished, False otherwise.
   */
  bool WorkOrder::isFinished() const {
    return m_status == WorkOrderFinished;
  }


  /**
   * @brief  Returns the redoing status of this WorkOrder.
   * @return @b bool Returns True if the WorkOrder is executing a redo.  Returns False if it is not.
   */
  bool WorkOrder::isRedoing() const {
    QMutexLocker locker(project()->workOrderMutex());
    return m_status == WorkOrderRedoing;
  }


  /**
   * @brief Returns the WorkOrder redone status.
   * @return @b bool Returns True if the WorkOrder has completed a Redo.  False if it has not.
   */
  bool WorkOrder::isRedone() const {
    QMutexLocker locker(project()->workOrderMutex());
    return m_status == WorkOrderRedone;
  }


  /**
   * @brief Returns the WorkOrderUndoing state.
   * @return @b bool Returns True if the current status is WorkOrderUndoing, False otherwise.
   */
  bool WorkOrder::isUndoing() const {
    QMutexLocker locker(project()->workOrderMutex());
    return m_status == WorkOrderUndoing;
  }


  /**
   * @brief Returns the WorkOrder undo status.
   * @return @b bool Returns True if the WorkOrder has been undone.  False if it has not.
   */
  bool WorkOrder::isUndone() const {
    QMutexLocker locker(project()->workOrderMutex());
    return m_status == WorkOrderUndone;
  }


  /**
   * @brief Returns the modified disk state.
   * @return @b Returns True if the WorkOrder has modified the Project on disk to perform
   * it's actions.  Returns False if it has not.
   */
  bool WorkOrder::modifiesDiskState() const {
    QMutexLocker locker(project()->workOrderMutex());
    return m_modifiesDiskState;
  }


  /**
   * @brief Gets the next WorkOrder.
   * @return @b QPointer pointing to the next WorkOrder.
   */
  WorkOrder *WorkOrder::next() const {
    QMutexLocker locker(project()->workOrderMutex());
    return m_nextWorkOrder;
  }


  /**
   * @brief Gets the previous WorkOrder.
   * @return @b QPointer pointing to the previous WorkOrder.
   */
  WorkOrder *WorkOrder::previous() const {
    QMutexLocker locker(project()->workOrderMutex());
    return m_previousWorkOrder;
  }


  /**
   * @brief WorkOrder::statusText
   * @return @b QString A string representation of the current WorkOrder status.
   */
  QString WorkOrder::statusText() const {
    QMutexLocker locker(project()->workOrderMutex());
    QString result = toString(m_status);

    if (m_secondsElapsed) {
      // QTime can't format in the way that I want (0-n minutes, 00-59 seconds, no hours
      //   displayed)... so do it manually.
      // Expected output format examples: 0:01, 0:55, 1:30, 55:55, 90:00, 100:12
      int seconds = qRound(m_secondsElapsed) % 60;
      int minutes = qRound(m_secondsElapsed) / 60;
      result += tr(" (elapsed: %1:%2)").arg(minutes).arg(seconds, 2, 10, QChar('0'));
    }

    return result;
  }


  /**
   * @brief Returns the ProgressBar.
   * @return A QPointer to the ProgessBar.
   */
  ProgressBar *WorkOrder::progressBar() {
    QMutexLocker locker(project()->workOrderMutex());
    return m_progressBar;
  }


  /**
   * @brief Attempts to query the current status of the WorkOrder.
   * @param statusString The status we want information about.
   * @return @b WorkOrderStatus Returns WorkOrderUnknownStatus if the statusString
   * does not match the current status.  If there is a result, then it returns
   * the status which matches the statusString.
   */
  WorkOrder::WorkOrderStatus WorkOrder::fromStatusString(QString statusString) {
    statusString = statusString.toUpper();
    WorkOrderStatus result = WorkOrderUnknownStatus;

    for (WorkOrderStatus possibleResult = WorkOrderUnknownStatus;
         possibleResult <= WorkOrderLastStatus;
         possibleResult = (WorkOrderStatus)(possibleResult + 1)) {
      if (statusString == toString(possibleResult).toUpper()) {
        result = possibleResult;
      }
    }

    return result;
  }


  /**
   * @brief Gets the current status of the WorkOrder.
   * @param status An enumeration of all possible WorkOrder states.
   * @return @b QString Returns a string representation of the current status of the WorkOrder.
   */
  QString WorkOrder::toString(WorkOrderStatus status) {
    QString result;

    switch (status) {
      case WorkOrderUnknownStatus:
        result = tr("Unknown");
        break;
      case WorkOrderNotStarted:
        result = tr("Not Started");
        break;
      case WorkOrderRedoing:
        result = tr("In Progress");
        break;
      case WorkOrderRedone:
        result = tr("Completed");
        break;
      case WorkOrderUndoing:
        result = tr("Undoing");
        break;
      case WorkOrderUndone:
        result = tr("Undone");
        break;
      case WorkOrderFinished:
        result = tr("Finished");
        break;
    }

    return result;
  }


  /**
   * @brief Starts (or enqueues) a redo. This should not be re-implemented by children.
   */
  void WorkOrder::redo() {
    if (!isInStableState()) {
      m_queuedAction = RedoQueuedAction;
    }

    if (!isRedone()) {
      bool mustQueueThisRedo = false;

      WorkOrder *dependency = NULL;
      WorkOrder *current = this;
      while (current->previous() && !dependency) {
        if (!current->previous()->isRedone() && !current->previous()->isFinished()) {
          WorkOrder *possibleDependency = current->previous();

          if (dependsOn(possibleDependency)) {
            connect(possibleDependency, SIGNAL(finished(WorkOrder *)),
                    this, SLOT(attemptQueuedAction()));
            dependency = possibleDependency;
            mustQueueThisRedo = true;
          }
        }

        current = current->previous();
      }

      if (!imageList()) {
        connect(project(), SIGNAL(imagesAdded(ImageList *)),
                this, SLOT(attemptQueuedAction()));
        mustQueueThisRedo = true;
      }

      if (!shapeList()) {
        connect(project(), SIGNAL(shapesAdded(ShapeList *)),
                this, SLOT(attemptQueuedAction()));
        mustQueueThisRedo = true;
      }

      if (mustQueueThisRedo && !isUndoing() && !isRedoing()) {

        m_queuedAction = RedoQueuedAction;

        QString queueStatusText;

        if (dependency) {
          QString dependencyText = dependency->bestText();

          if (dependencyText.count() > 5) {
            dependencyText = dependencyText.mid(0, 5) + "...";
          }

          queueStatusText = tr("Wait for [%1]").arg(dependencyText);
        }
        else if (!imageList()) {
          queueStatusText = tr("Wait for images");
        }
        else if (!shapeList()) {
          queueStatusText = tr("Wait for shapes");
        }

        resetProgressBar();
        m_progressBar->setValue(m_progressBar->minimum());
        m_progressBar->setText(queueStatusText);
        m_progressBar->update();
      }

      if (m_queuedAction == NoQueuedAction) {
        m_status = WorkOrderRedoing;
        emit statusChanged(this);

        resetProgressBar();
        m_progressBar->setText("Starting...");
        m_progressBar->update();

        delete m_elapsedTimer;
        m_elapsedTimer = new QElapsedTimer;
        m_elapsedTimer->start();

        if (isSynchronous()) {
          execute();
          executionFinished();
        }
        else {
          m_progressBar->setText("Running...");
          m_progressBar->update();
          // queue the workorder for asynchronous execution
          QFuture<void> future = QtConcurrent::run(this, &WorkOrder::execute);
          // executionFinished() is called via the finished signal.  The
          // connection is setup in the constructor.
          m_futureWatcher->setFuture(future);
        }
      }
    }
    else {
      setProgressToFinalText();
    }
  }


  /**
   * @brief Starts (or enqueues) an undo.
   * This should not be re-implemented by children.
   *
   */
  void WorkOrder::undo() {
    if (!isInStableState()) {
      m_queuedAction = UndoQueuedAction;
    }

    if (!isUndone() && m_status != WorkOrderNotStarted) {
      WorkOrder *dependency = NULL;
      WorkOrder *current = this;
      while (current->next() && !dependency) {
        if (!current->next()->isUndone() && !current->next()->isFinished() &&
            current->next()->m_status != WorkOrderNotStarted) {
          connect(current->next(), SIGNAL(finished(WorkOrder *)),
                  this, SLOT(attemptQueuedAction()));
          m_queuedAction = UndoQueuedAction;
          dependency = current->next();
        }

        current = current->next();
      }

      if (dependency && !isUndoing() && !isRedoing()) {
        QString prevText = dependency->bestText();

        if (prevText.count() > 5) {
          prevText = prevText.mid(0, 5) + "...";
        }

        resetProgressBar();
        m_progressBar->setValue(m_progressBar->minimum());
        m_progressBar->setText(tr("Undo after [%1]").arg(prevText));
        m_progressBar->update();
      }

      if (m_queuedAction == NoQueuedAction) {
        m_status = WorkOrderUndoing;
        emit statusChanged(this);

        resetProgressBar();
        m_progressBar->setText("Starting Undo...");
        m_progressBar->update();

        delete m_elapsedTimer;
        m_elapsedTimer = new QElapsedTimer;
        m_elapsedTimer->start();

        if (isSynchronous()) {
          undoExecution();
          executionFinished();
        }
        else {
          m_progressBar->setText("Undoing...");
          m_progressBar->update();
          // queue the workorder for asynchronous execution
          QFuture<void> future = QtConcurrent::run(this, &WorkOrder::undoExecution);
          // executionFinished() is called via the finished signal.  The
          // connection is setup in the constructor.
          m_futureWatcher->setFuture(future);
        }
      }
    }
    else {
      setProgressToFinalText();
    }
  }

  /**
   * @brief Enables the work order.
   *
   * Enables the work order so that it can be triggered (clicked).
   *
   * @see Directory::initializeActions()
   */
  void WorkOrder::enableWorkOrder() {
    setEnabled(true);
  }


  /**
   * @brief Disables the work order.
   *
   * Disables the work order so it cannot be triggered (grayed-out).
   *
   * @see Directory::initializeActions()
   */
  void WorkOrder::disableWorkOrder() {
    setEnabled(false);
  }


  /**
   * @brief This sets up the state for the work order.  Child should implement this to get user
   *        input.
   *
   * This method is designed to be implemented by children work orders, but they need
   * to call the base class setupExecution (at the beginning).
   *
   * State should only be set in the WorkOrder class in this method. You can set arbitrary
   * state using setInternalData(). Call setData(ImageList),
   * setInternalData(QStringList), etc... with all of the data/state necessary to perform the
   * work order. This could be a list of file names, an ImageList of images you're viewing,
   * or really anything else. This method is always executed in the GUI thread and is the
   * only place to ask the user questions.
   *
   * The actual work is done in execute(), using only state (data) stored in the
   * WorkOrder class. You do not have to call execute() - this is done for you
   * by WorkOrder::redo().  WorkOrder::redo() is called from Project::addToProject() when the
   * workOrder is pushed onto the undo stack.
   *
   * If this method returns false the workorder will be cancelled and will not be executed.
   *
   * @return @b bool Returns True upon successful preparation of the WorkOrder, False if this
   *            operation should be cancelled (the user clicked cancel, the operation turns
   *            out to be impossible, etc). This prevents the work order from executing and
   *            it will not be entered into the history.
   */
  bool WorkOrder::setupExecution() {
    // We're finished at this point if we save/open a project, we're not finished if we need to do
    //   redo()
    if (createsCleanState() || !isUndoable()) {
      m_status = WorkOrderFinished;

      emit statusChanged(this);
    }

    m_executionTime = QDateTime::currentDateTime();

    resetProgressBar();

    if (createsCleanState()  || !isUndoable()) {
      setProgressToFinalText();
    }
    else {
      m_progressBar->setText("Initializing...");
    }

    return true;
  }


  /**
   * @brief return the workorder project directory
   * Returns the Directory object of the Project this WorkOrder is attached to.
   * @return @b (Directory *) A pointer to the Directory.
   */
  Directory *WorkOrder::directory() const {
    return project()->directory();
  }


  /**
   * @brief Returns the Project this WorkOrder is attached to.
   * @return  @b (Project *) A pointer to the Project.
   * @throws  IException::Programmer "This work order no longer has a project."
   */
  Project *WorkOrder::project() const {
    if (!m_project) {
      throw IException(IException::Programmer,
          "This work order no longer has a project.", _FILEINFO_);
    }

    return m_project;
  }


  /**
   * @brief Sets the internal data for this WorkOrder.
   *
   * WorkOrders may not use member variables to store data.  Any data
   * needed for the workorder should be saved in to the base WorkOrder
   * using setInternalData.
   * @param data The data to set the internal data to.
   */
  void WorkOrder::setInternalData(QStringList data) {
    m_internalData = data;
  }


  /**
   * @brief Gets the minimum value of the progress range of the WorkOrder.
   * @return @b int The minimum value.
   */
  int WorkOrder::progressMin() const {
    QMutexLocker locker(project()->workOrderMutex());
    return m_progressRangeMinValue;
  }


  /**
   * @brief Gets the maximum value of the progress range of the WorkOrder.
   * @return @b int The maximum value.
   */
  int WorkOrder::progressMax() const {
    QMutexLocker locker(project()->workOrderMutex());
    return m_progressRangeMaxValue;
  }


  /**
   * @brief Gets the current progress value of the WorkOrder.
   * @return @b int Returns the current progress value.
   */
  int WorkOrder::progressValue() const {
    QMutexLocker locker(project()->workOrderMutex());
    return m_progressValue;
  }


  /**
   * @brief Sets the progress range of the WorkOrder.
   * @param minValue The progress range minimum value.
   * @param maxValue The progress range maximum value.
   */
  void WorkOrder::setProgressRange(int minValue, int maxValue) {
    m_progressRangeMinValue = minValue;
    m_progressRangeMaxValue = maxValue;
  }


  /**
   * @brief Sets the current progress value for the WorkOrder.
   * @param int value The value to set the current progress to.
   */
  void WorkOrder::setProgressValue(int value) {
    m_progressValue = value;
  }


  /**
   * @brief Gets the internal data for this WorkOrder.
   * @return @b QStringList Returns the internal data object.
   */
  QStringList WorkOrder::internalData() const {
    QMutexLocker locker(project()->workOrderMutex());
    return m_internalData;
  }


  /**
   * @brief Execute the workorder.
   * Execute() does the actual work in the work order. All necessary data for the execution (and
   * undo) of the workorder should have been saved in the workorder prior to execute().  Execute()
   * is also called to redo a workorder for redoable workorders.  If the workorder is a synchrounous
   * workorder the workorder will be run on the GUI thread, otherwise it will be queued and run on a
   * separate thread.
   *
   * For Synchronous workorders:
   * State should only be read from the parent WorkOrder class in this method. You can set state to
   * postExecution() safely. This method is always executed in the GUI thread.
   *
   * For asynchronous workorders:
   * State can be read from the parent WorkOrder class while in
   *   this method. You can set state to be used in postExecution() safely. Please be wary of
   *   creating QObjects inside of this method because they will associated with the pooled thread
   *   and must be moved back to the GUI thread with QObject::moveToThread().  You can update
   *   progress by calling setProgressRange() and setProgressValue(). Do not manipulate any
   *   GUI objects here.
   */
  void WorkOrder::execute() {
  }


  /**
   * @brief Perform any necessary actions after execution of a workorder.
   *
   * State can be read from the parent WorkOrder class and from state set in either synchronous() or
   *   asyncRedo() while in this method. You can not set state to be used in any of the undo code
   *   safely. This method is always executed in the GUI thread and has no progress.
   */
  void WorkOrder::postExecution() {
  }


  /**
   * @brief Execute the steps necessary to undo this workorder.
   * The workorder should have all state necessary to undo itself stored in the workorder.
   *
   * For synchronous workorders:
   *   State should only be read from the parent WorkOrder class in this method. You can set state
   *   to be used in postUndoExecution() safely. This method is always executed in the GUI
   *   thread and has no progress.
   *
   * For Asynchronous workorders:
   *   State can be read from the parent WorkOrder class while in this method. You can set state
   *   to be used in postSyncUndo() safely. Please be wary of deleting QObjects inside of this
   *   method because they will cause unpredictable crashes. This method is never executed in the
   *   GUI thread. You can update progress by calling setProgressRange() and setProgressValue().
   *   Do not manipulate any GUI objects here.
   */
  void WorkOrder::undoExecution() {
  }


  /**
   * @brief Perform any steps necessary after an undo of a workorder.
   *
   *  State can be read from the parent WorkOrder class and from state set undoExecution() while
   *  in this method. You can not set state to be used in any of the redo code
   *  safely. This method is always executed in the GUI thread and has no progress.
   */
  void WorkOrder::postUndoExecution() {
  }


  /**
   * @brief  Runs a copy of the current WorkOrder and stores it in the project.
   */
  void WorkOrder::addCloneToProject() {
    if (project()) {
      project()->addToProject(clone());
    }
  }


  /**
   * @brief Determines if the WorkOrder is in a stable state, or if it's busy doing something.
   * @return @b bool Returns True if the WorkOrder is in a stable state, and False if it is not.
   */
  bool WorkOrder::isInStableState() const {
    bool result = true;

    if (isRedoing() || isUndoing() || m_queuedAction != NoQueuedAction) {
      result = false;
    }

    return result;
  }


  /**
   * @brief Checks to see if we have lost any images in the ImageList.  If we have, then
   * destroy the entire list.  This will send a signal that the list needs to be rebuilt if
   * requested.
   */
  void WorkOrder::listenForImageDestruction() {
    m_imageIds.clear();
    foreach (Image *image, *m_imageList) {
      if (image) {
        m_imageIds.append(image->id());

        // If we lose any images, destroy the entire list. This will let us know that we need to
        //   rebuild it, if needed, when requested.
        connect(image, SIGNAL(destroyed(QObject *)),
                this, SLOT(clearImageList()));
      }
    }
  }


  /**
   * @brief Checks to see if we have lost any shapes in the ShapeList.  If we have, then
   * destroy the entire list.  This will send a signal that the list needs to be rebuilt if
   * requested.
   *
   * TODO 2016-07-26 TLS Combine this with listenForImageDestruction() - Basically duplicate
   *    code.
   */
  void WorkOrder::listenForShapeDestruction() {
    m_shapeIds.clear();
    foreach (Shape *shape, *m_shapeList) {
      if (shape) {
        m_shapeIds.append(shape->id());

        // If we lose any shapes, destroy the entire list. This will let us know that we need to
        //   rebuild it, if needed, when requested.
        connect(shape, SIGNAL(destroyed(QObject *)),
                this, SLOT(clearShapeList()));
      }
    }
  }


  /**
   * @brief Resets the ProgressBar
   */
  void WorkOrder::resetProgressBar() {
    delete m_progressBarDeletionTimer;

    if (!m_progressBar) {
      m_progressBar = new ProgressBar;
      emit creatingProgress(this);
    }

    if (!m_progressBarUpdateTimer) {
      m_progressBarUpdateTimer = new QTimer;
      connect(m_progressBarUpdateTimer, SIGNAL(timeout()),
              this, SLOT(updateProgress()));
      m_progressBarUpdateTimer->start(100);
    }

    m_progressRangeMinValue = 0;
    m_progressRangeMaxValue = 100;
    m_progressValue = 0;
  }


  /**
   * @brief Sets the ProgressBar to display the final status of the operation.
   */
  void WorkOrder::setProgressToFinalText() {
    if (m_progressBar) {
      if (isRedone()) {
        m_progressBar->setText(tr("Finished"));
      }
      else if (isUndone() || m_status == WorkOrderNotStarted) {
        m_progressBar->setText(tr("Undone"));
      }

      if (m_progressBar->minimum() != 0 || m_progressBar->maximum() != 0) {
        m_progressBar->setValue(m_progressBar->maximum());
      }
      else {
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(100);
      }

      delete m_progressBarDeletionTimer;
      m_progressBarDeletionTimer = new QTimer;
      m_progressBarDeletionTimer->setSingleShot(true);

      m_progressBarDeletionTimer->start(5 * 1000); // 5 seconds

      m_progressBar->update();
    }
  }


  /**
   * @brief Attempts to execute an action on the action queue.
   */
  void WorkOrder::attemptQueuedAction() {
    QueuedWorkOrderAction queued = m_queuedAction;
    m_queuedAction = NoQueuedAction;

    if (queued == RedoQueuedAction && m_status != WorkOrderRedone) {
      redo();
    }
    else if (queued == UndoQueuedAction && m_status != WorkOrderUndone) {
      undo();
    }
  }


  /**
   * @brief Signals the Project that the WorkOrder is finished, deletes
   * the update time for the Progress bar, and sets the finished status.
   */
  void WorkOrder::executionFinished() {
    delete m_progressBarUpdateTimer;

    WorkOrderStatus finishedStatus = WorkOrderRedone;
    void (WorkOrder::*postMethod)() = &WorkOrder::postExecution;

    if (isUndoing()) {
      finishedStatus = WorkOrderUndone;
      postMethod = &WorkOrder::postUndoExecution;
    }

    (this->*postMethod)();

    m_status = finishedStatus;

    m_secondsElapsed = m_elapsedTimer->elapsed() / 1000.0;

    delete m_elapsedTimer;
    m_elapsedTimer = NULL;

    emit statusChanged(this);
    setProgressToFinalText();
    emit finished(this);

    attemptQueuedAction();
  }


  /**
   * @brief Clears the list of images.
   */
  void WorkOrder::clearImageList() {
    delete m_imageList;
  }


  /**
   * @brief Clears the list of shapes.
   */
  void WorkOrder::clearShapeList() {
    delete m_shapeList;
  }


  /**
   * @brief Updates the progress bar.
   */
  void WorkOrder::updateProgress() {
    if (m_progressBar && (isRedoing() || isUndoing())) {
      m_progressBar->setRange(m_progressRangeMinValue, m_progressRangeMaxValue);
      m_progressBar->setValue(m_progressValue);
    }
  }


  /**
   * @brief WorkOrder::startRedo  This function is currently empty.
   */
  void WorkOrder::startRedo() {
  }


  /**
   * @brief Declare that this work order is saving the project.
   * This makes the work order not appear in the undo stack (cannot undo/redo), and instead it is
   * marked as a 'clean' state of the project. The QUndoCommand undo/redo will never be called.
   * The default for createsCleanState is false. If this is set to true all wrokorders before this
   * call will be locked.
   * @param createsCleanState True if this work order is going to save the project to disk,
   * False otherwise.
   */
  void WorkOrder::setCreatesCleanState(bool createsCleanState) {
    m_createsCleanState = createsCleanState;
  }


  /**
   * @brief.  By default, m_modifiesDiskState is False.  If a WorkOrder modifies the Project
   * on disk as a result of it's action, this should be set to true.
   * @param changesProjectOnDisk True if this WorkOrder modifies the Project on disk.  False
   * if it does not.
   */
  void WorkOrder::setModifiesDiskState(bool changesProjectOnDisk) {
    m_modifiesDiskState = changesProjectOnDisk;
  }
}
