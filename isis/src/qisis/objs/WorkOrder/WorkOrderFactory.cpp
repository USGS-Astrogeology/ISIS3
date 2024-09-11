#include "WorkOrderFactory.h"

#include "BundleObservationViewWorkOrder.h"
#include "CnetEditorViewWorkOrder.h"
#include "ControlHealthMonitorWorkOrder.h"
#include "CubeDnViewWorkOrder.h"
#include "ExportControlNetWorkOrder.h"
#include "ExportImagesWorkOrder.h"
#include "Footprint2DViewWorkOrder.h"
#include "IException.h"
#include "ImageFileListViewWorkOrder.h"
#include "ImageListActionWorkOrder.h"
#include "ImportControlNetWorkOrder.h"
#include "ImportImagesWorkOrder.h"
#include "ImportShapesWorkOrder.h"
#include "ImportMapTemplateWorkOrder.h"
#include "ImportRegistrationTemplateWorkOrder.h"
#include "IString.h"
#include "JigsawWorkOrder.h"
#include "MatrixViewWorkOrder.h"
#include "MoveDownOneSceneWorkOrder.h"
#include "MoveToBottomSceneWorkOrder.h"
#include "MoveToTopSceneWorkOrder.h"
#include "MoveUpOneSceneWorkOrder.h"
#include "OpenProjectWorkOrder.h"
#include "RemoveImagesWorkOrder.h"
#include "RenameProjectWorkOrder.h"
#include "SaveProjectAsWorkOrder.h"
#include "SaveProjectWorkOrder.h"
#include "SensorGetInfoWorkOrder.h"
#include "SetActiveControlWorkOrder.h"
#include "SetActiveImageListWorkOrder.h"
#include "TargetGetInfoWorkOrder.h"
#include "TemplateEditViewWorkOrder.h"

namespace Isis {
  /**
   * This instantiates a work order given a project and a type name (class name in a string).
   *
   * Ownership is passed to the caller. The work orders are QObject's so please be mindful of which
   *   thread they are in.
   *
   * @param project The project to give to the work order constructor
   * @param type The work order type (class name) - for example "Isis::ImportImagesWorkOrder"
   *
   * @throws IException::Unknown "Could not create the work order through WorkOrderFactory"
   *
   * @return @b WorkOrder Returns the instantiated WorkOrder
   */
  WorkOrder *WorkOrderFactory::create(Project *project, QString type) {
    WorkOrder *result = NULL;

    tryType<BundleObservationViewWorkOrder>(type, project, result);
    tryType<CnetEditorViewWorkOrder>(type, project, result);
    tryType<ControlHealthMonitorWorkOrder>(type, project, result);
    tryType<CubeDnViewWorkOrder>(type, project, result);
    tryType<ExportImagesWorkOrder>(type, project, result);
    tryType<ExportControlNetWorkOrder>(type, project, result);
    tryType<Footprint2DViewWorkOrder>(type, project, result);
    tryType<ImageFileListViewWorkOrder>(type, project, result);
    tryType<ImageListActionWorkOrder>(type, project, result);
    tryType<ImportControlNetWorkOrder>(type, project, result);
    tryType<ImportImagesWorkOrder>(type, project, result);
    tryType<ImportShapesWorkOrder>(type, project, result);
    tryType<ImportMapTemplateWorkOrder>(type, project, result);
    tryType<ImportRegistrationTemplateWorkOrder>(type, project, result);
    tryType<JigsawWorkOrder>(type, project, result);
    tryType<MatrixViewWorkOrder>(type, project, result);
    tryType<MoveDownOneSceneWorkOrder>(type, project, result);
    tryType<MoveToBottomSceneWorkOrder>(type, project, result);
    tryType<MoveToTopSceneWorkOrder>(type, project, result);
    tryType<MoveUpOneSceneWorkOrder>(type, project, result);
    tryType<OpenProjectWorkOrder>(type, project, result);
    tryType<RemoveImagesWorkOrder>(type, project, result);
    tryType<RenameProjectWorkOrder>(type, project, result);
    tryType<SaveProjectAsWorkOrder>(type, project, result);
    tryType<SaveProjectWorkOrder>(type, project, result);
    tryType<SensorGetInfoWorkOrder>(type, project, result);
    tryType<SetActiveControlWorkOrder>(type, project, result);
    tryType<SetActiveImageListWorkOrder>(type, project, result);
    tryType<TargetGetInfoWorkOrder>(type, project, result);
    tryType<TemplateEditViewWorkOrder>(type, project, result);

    if (!result) {
      throw IException(IException::Unknown,
                       "Could not create work order of type ["+type.toStdString()+"] through "
                                   "WorkOrderFactory",
                       _FILEINFO_);
    }

    return result;
  }
}
