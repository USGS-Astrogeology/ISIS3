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
#include "SensorGetInfoWorkOrder.h"

#include <QtDebug>

#include <QFileDialog>
#include <QInputDialog>

#include "Directory.h"
#include "IException.h"
#include "Project.h"
#include "GuiCamera.h"
#include "SensorInfoWidget.h"

namespace Isis {

  /**
  * @brief Creates a WorkOrder that will retrieve target body info.
  * @param project The Project that this WorkOrder should be interacting with.
  *
  */
  SensorGetInfoWorkOrder::SensorGetInfoWorkOrder(Project *project) :
      WorkOrder(project) {
    QAction::setText(tr("Get Info...") );
    // Currently undo is not implemented.
    m_isUndoable =false;
    m_isSavedToHistory = false;
  }


  /**
   * @brief Copies the WorkOrder 'other' into this new instance.
   * @param other The WorkOrder being copied.
   */
  SensorGetInfoWorkOrder::SensorGetInfoWorkOrder(const SensorGetInfoWorkOrder &other) :
      WorkOrder(other) {
  }


  /**
   * @brief The Destructor.
   */
  SensorGetInfoWorkOrder::~SensorGetInfoWorkOrder() {
  }


  /**
   * @brief Returns a copy of the current instance of this WorkOrder.
   * @return  @b (SensorGetInfoWorkOrder *) A copy of this object.
   */
  SensorGetInfoWorkOrder *SensorGetInfoWorkOrder::clone() const {
    return new SensorGetInfoWorkOrder(*this);
  }


  /**
   * @brief Determine if we already have a view for this camera.
   * If we do, then do not
   * redisplay the view, and return false.
   *
   * @param guiCamera  A Camera object contained within a Project.
   * @return @b bool Return True if we we already have a view for the guiCamera object.
   * Return False otherwise.
   */
  bool SensorGetInfoWorkOrder::isExecutable(GuiCameraQsp guiCamera) {
    if (!guiCamera)
      return false;

    // if we already have a view for this camera, don't redisplay
    QList<SensorInfoWidget *> existingViews = project()->directory()->sensorInfoViews();
    for (int i = 0; i < existingViews.size(); i++) {
      if (existingViews.at(i)->objectName() == guiCamera->displayProperties()->displayName() )
        return false;
    }

    return true;
  }


  /**
   * @brief Attempts to retrieve target body info from the camera and display it
   * in a view.
   * @return  @b bool True if successful, False if not.
   */
  bool SensorGetInfoWorkOrder::setupExecution() {
    bool success = WorkOrder::setupExecution();

    if (success) {
      QString sensorDisplayName = guiCamera()->displayProperties()->displayName();
      QUndoCommand::setText(tr("Get %1 sensor info").arg(sensorDisplayName) );

      QStringList internalData;
      internalData.append(sensorDisplayName);
      setInternalData(internalData);
    }

    return success;
  }


  /**
   * @brief Determines whether a WorkOrder depends upon SensorGetInfoWorkerOrder
   * @param other  The work order being checked for dependency.
   * @return @b bool True if dependent, False if not
   */
  bool SensorGetInfoWorkOrder::dependsOn(WorkOrder *other) const {
    // depend on types of ourselves.
    return dynamic_cast<SensorGetInfoWorkOrder *>(other);
  }


  /**
   * @brief  Redisplays the sensor view.
   */
  void SensorGetInfoWorkOrder::execute() {

    SensorInfoWidget *sensorInfoWidget =
        project()->directory()->addSensorInfoView(guiCamera() );


    if (!sensorInfoWidget) {
      std::string msg = "error displaying sensor info";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * @brief  Deletes the last view.  Currently this function is not implemented.
   */
  void SensorGetInfoWorkOrder::undoExecution() {
    //delete project()->directory()->cnetEditorViews().last();
  }
}
