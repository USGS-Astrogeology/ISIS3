/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "csminit.h"

#include <QList>
#include <QString>
#include <QStringList>

#include "csm/csm.h"
#include "csm/GeometricModel.h"
#include "csm/Isd.h"
#include "csm/Model.h"
#include "csm/NitfIsd.h"
#include "csm/Plugin.h"

#include "Blob.h"
#include "Camera.h"
#include "CameraFactory.h"
#include "CSMCamera.h"
#include "Cube.h"
#include "IException.h"
#include "ImagePolygon.h"
#include "Process.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "SpiceRotation.h"

using namespace std;
using json = nlohmann::json;

namespace Isis {

  /**
   * csminit a cube in an Application
   *
   * @param ui The Application UI
   * @param(out) log The Pvl that attempted models will be logged to
   */
  void csminit(UserInterface &ui, Pvl *log) {
    // We are not processing the image data, so this process object is just for
    // managing the Cube in memory and adding history
    Process p;
    // Get the cube here so that we check early if it doesn't exist
    Cube *cube = p.SetInputCube(ui.GetCubeName("FROM"), ui.GetInputAttribute("FROM"), ReadWrite);

    // We have to call this to get the plugin list loaded.
    CameraFactory::initPlugin();

    // These three variables are the main product of the following if/else statement
    QString pluginName;
    QString modelName;
    csm::Model *model = nullptr;

    if (ui.WasEntered("ISD") && ui.WasEntered("STATE")) {
      std::string message = "Cannot enter both [ISD] and [STATE]. Please enter either [ISD] or [STATE].";
      throw IException(IException::User, message, _FILEINFO_);
    }

    else if (!ui.WasEntered("ISD") && !ui.WasEntered("STATE")) {
      std::string message = "Either an ISD or a State string must be entered.";
      throw IException(IException::User, message, _FILEINFO_);
    }

    else if (ui.WasEntered("ISD")) {
      QString isdFilePath = ui.GetFileName("ISD");

      QList<QStringList> possibleModels;
      for (const csm::Plugin * plugin : csm::Plugin::getList()) {
        QString currentPluginName = QString::fromStdString(plugin->getPluginName());
        if (ui.WasEntered("PLUGINNAME") && currentPluginName != ui.GetString("PLUGINNAME")) {
          continue;
        }

        for (size_t modelIndex = 0; modelIndex < plugin->getNumModels(); modelIndex++) {
          QString currentModelName = QString::fromStdString(plugin->getModelName(modelIndex));
          if (ui.WasEntered("MODELNAME") && currentModelName != ui.GetString("MODELNAME")) {
            continue;
          }

          csm::Isd fileIsd(isdFilePath.toStdString());
          if (plugin->canModelBeConstructedFromISD(fileIsd, currentModelName.toStdString())) {
            QStringList modelSpec = {
                currentPluginName,
                currentModelName,
                QString::fromStdString(fileIsd.format())};
            possibleModels.append(modelSpec);
            continue; // If the file ISD works, don't check the other ISD formats
          }

          csm::Nitf21Isd nitf21Isd(isdFilePath.toStdString());
          if (plugin->canModelBeConstructedFromISD(nitf21Isd, currentModelName.toStdString())) {
            QStringList modelSpec = {
                currentPluginName,
                currentModelName,
                QString::fromStdString(nitf21Isd.format())};
            possibleModels.append(modelSpec);
            continue; // If the NITF 2.1 ISD works, don't check the other ISD formats
          }
        }
      }

      if (possibleModels.size() > 1) {
        std::string message = "Multiple models can be created from the ISD [" + isdFilePath.toStdString() + "]. "
                          "Re-run with the PLUGINNAME and MODELNAME parameters. "
                          "Possible plugin & model names:\n";
        for (const QStringList &modelSpec : possibleModels) {
          message += "Plugin [" + modelSpec[0].toStdString() + "], Model [" + modelSpec[1].toStdString() + "]\n";
        }
        throw IException(IException::User, message, _FILEINFO_);
      }

      if (possibleModels.empty()) {
        std::string message = "No loaded model could be created from the ISD [" + isdFilePath.toStdString() + "]."
                          "Loaded plugin & model names:\n";
        for (const csm::Plugin * plugin : csm::Plugin::getList()) {
          std::string currentPluginName = plugin->getPluginName();
          for (size_t modelIndex = 0; modelIndex < plugin->getNumModels(); modelIndex++) {
            std::string modelName = plugin->getModelName(modelIndex);
            message += "Plugin [" + currentPluginName + "], Model [" + modelName + "]\n";
          }
        }
        throw IException(IException::User, message, _FILEINFO_);
      }

      // If we are here, then we have exactly 1 model
      QStringList modelSpec = possibleModels.front();

      if (modelSpec.size() != 3) {
        std::string message = "Model specification [" + modelSpec.join(" ").toStdString() + "] has [" + Isis::toString(modelSpec.size()) + "] elements "
          "when it should have 3 elements.";
        throw IException(IException::Programmer, message, _FILEINFO_);
      }

      pluginName = modelSpec[0];
      modelName = modelSpec[1];
      QString isdFormat = modelSpec[2];

      const csm::Plugin *plugin = csm::Plugin::findPlugin(pluginName.toStdString());
      if (plugin == NULL) {
        std::string message = "Cannot find requested Plugin: [" + pluginName.toStdString() + "].";
        throw IException(IException::User, message, _FILEINFO_);
      }

      csm::Isd fileIsd(isdFilePath.toStdString());
      csm::Nitf21Isd nitf21Isd(isdFilePath.toStdString());
      if (isdFormat == QString::fromStdString(fileIsd.format())) {
        model = plugin->constructModelFromISD(fileIsd, modelName.toStdString());
      }
      else if (isdFormat == QString::fromStdString(nitf21Isd.format())) {
        model = plugin->constructModelFromISD(nitf21Isd, modelName.toStdString());
      }
      else {
        std::string message = "Invalid ISD format specifications [" + isdFormat.toStdString() + "].";
        throw IException(IException::Programmer, message, _FILEINFO_);
      }
    } // end of ISD if statement

    else if (ui.WasEntered("STATE")) {
      FileName stateFilePath = ui.GetFileName("STATE").toStdString();

      std::ifstream file(stateFilePath.expanded());
      std::stringstream buffer;
      buffer << file.rdbuf();
      QString stateString = QString::fromStdString(buffer.str());

      if (!ui.WasEntered("PLUGINNAME") && !ui.WasEntered("MODELNAME")) {
        std::string message = "When using a State string, PLUGINNAME and MODELNAME must be specified";
        throw IException(IException::Programmer, message, _FILEINFO_);
      }
      pluginName = ui.GetString("PLUGINNAME");
      modelName = ui.GetString("MODELNAME");

      const csm::Plugin *plugin = csm::Plugin::findPlugin(pluginName.toStdString());
      if (plugin == NULL) {
        std::string message = "Cannot find requested Plugin: [" + pluginName.toStdString() + "].";
        throw IException(IException::User, message, _FILEINFO_);
      }

      // TODO: Add warning argument and use message from csm::Warning for Isis::IException error.
      if (plugin->canModelBeConstructedFromState(modelName.toStdString(), stateString.toStdString())){
        model = plugin->constructModelFromState(stateString.toStdString());
      }
      else {
        std::string message = "Could not construct sensor model using STATE string and MODELNAME: [" + modelName.toStdString() + "]";
        throw IException(IException::Programmer, message, _FILEINFO_);
      }
    } // end of State else statement

    string modelState = model->getModelState();

    // Making copies of original Pvl Groups from input label so they can be restored if csminit fails.
    PvlGroup originalInstrument;
    PvlGroup originalKernels;
    PvlGroup originalCsmInfo;
    if (cube->hasGroup("Instrument")) {
      originalInstrument = cube->group("Instrument");
    }

    if (cube->hasGroup("Kernels")) {
      originalKernels = cube->group("Kernels");
    }

    if (cube->hasGroup("CsmInfo")) {
      originalCsmInfo = cube->group("CsmInfo");
    }

    if (!cube->hasGroup("Instrument")) {
      cube->putGroup(PvlGroup("Instrument"));
    }
    PvlGroup &instrumentGroup = cube->group("Instrument");
    if (ui.WasEntered("TARGETNAME")) {
      instrumentGroup.addKeyword(PvlKeyword("TargetName", ui.GetString("TARGETNAME").toStdString()), Pvl::Replace);
    }
    // If the user doesn't specify a target name, then we will still need
    // something on the label for the Target & ShapeModel so add Unknown
    else if (!instrumentGroup.hasKeyword("TargetName")) {
      PvlKeyword targetKey("TargetName", "Unknown");
      targetKey.addComment("Radii will come from the CSM model");
      instrumentGroup.addKeyword(targetKey, Pvl::Replace);
    }

    if (!instrumentGroup.hasKeyword("InstrumentId")) {
      PvlKeyword instrumentIdKey("InstrumentId", model->getSensorIdentifier());
      instrumentGroup.addKeyword(instrumentIdKey, Pvl::Replace);
    }

    // Populate the CsmInfo group with useful information
    cube->deleteGroup("CsmInfo");
    PvlGroup infoGroup("CsmInfo");
    infoGroup += PvlKeyword("CSMPlatformID", model->getPlatformIdentifier());
    infoGroup += PvlKeyword("CSMInstrumentId", model->getSensorIdentifier());
    infoGroup += PvlKeyword("ReferenceTime", model->getReferenceDateAndTime());
    csm::GeometricModel *modelWithParams = dynamic_cast<csm::GeometricModel*>(model);

    if (modelWithParams) {
      PvlKeyword paramNames("ModelParameterNames");
      PvlKeyword paramUnits("ModelParameterUnits");
      PvlKeyword paramTypes("ModelParameterTypes");
      for (const csm::GeometricModel::Parameter &param : modelWithParams->getParameters()) {
        paramNames += param.name;
        paramUnits += param.units;
        switch (param.type) {
          case csm::param::NONE:
            paramTypes += "NONE";
            break;

          case csm::param::FICTITIOUS:
            paramTypes += "FICTITIOUS";
            break;

          case csm::param::REAL:
            paramTypes += "REAL";
            break;

          case csm::param::FIXED:
            paramTypes += "FIXED";
            break;

          default:
            paramTypes += "UNKNOWN";
            break;
        }
      }

      infoGroup += paramNames;
      infoGroup += paramUnits;
      infoGroup += paramTypes;
    }
    cube->putGroup(infoGroup);

    // Update existing Kernels Group or create new one and add shapemodel if provided
    if (!cube->hasGroup("Kernels")) {
      cube->putGroup(PvlGroup("Kernels"));
    }
    PvlGroup &kernelsGroup = cube->group("Kernels");

    if (ui.WasEntered("SHAPEMODEL")) {
      // TODO validate the shapemodel
      kernelsGroup.addKeyword(PvlKeyword("ShapeModel", ui.GetFileName("SHAPEMODEL").toStdString()), Pvl::Replace);
    }
    else {
      kernelsGroup.addKeyword(PvlKeyword("ShapeModel", "Null"), Pvl::Replace);
    }

    // Get rid of keywords from spiceinit
    if (kernelsGroup.hasKeyword("LeapSecond")) {
      kernelsGroup.deleteKeyword("LeapSecond");
    }
    if (kernelsGroup.hasKeyword("TargetAttitudeShape")) {
      kernelsGroup.deleteKeyword("TargetAttitudeShape");
    }
    if (kernelsGroup.hasKeyword("TargetPosition")) {
      kernelsGroup.deleteKeyword("TargetPosition");
    }
    if (kernelsGroup.hasKeyword("InstrumentPointing")) {
      kernelsGroup.deleteKeyword("InstrumentPointing");
    }
    if (kernelsGroup.hasKeyword("InstrumentPointingQuality")) {
      kernelsGroup.deleteKeyword("InstrumentPointingQuality");
    }
    if (kernelsGroup.hasKeyword("Instrument")) {
      kernelsGroup.deleteKeyword("Instrument");
    }
    if (kernelsGroup.hasKeyword("SpacecraftClock")) {
      kernelsGroup.deleteKeyword("SpacecraftClock");
    }
    if (kernelsGroup.hasKeyword("InstrumentPositionQuality")) {
      kernelsGroup.deleteKeyword("InstrumentPositionQuality");
    }
    if (kernelsGroup.hasKeyword("InstrumentPosition")) {
      kernelsGroup.deleteKeyword("InstrumentPosition");
    }
    if (kernelsGroup.hasKeyword("InstrumentAddendum")) {
      kernelsGroup.deleteKeyword("InstrumentAddendum");
    }
    if (kernelsGroup.hasKeyword("EXTRA")) {
      kernelsGroup.deleteKeyword("EXTRA");
    }
    if (kernelsGroup.hasKeyword("Source")) {
      kernelsGroup.deleteKeyword("Source");
    }
    if (kernelsGroup.hasKeyword("SpacecraftPointing")) {
      kernelsGroup.deleteKeyword("SpacecraftPointing");
    }
    if (kernelsGroup.hasKeyword("SpacecraftPosition")) {
      kernelsGroup.deleteKeyword("SpacecraftPosition");
    }
    if (kernelsGroup.hasKeyword("CameraVersion")) {
      kernelsGroup.deleteKeyword("CameraVersion");
    }
    if (kernelsGroup.hasKeyword("ElevationModel")) {
      kernelsGroup.deleteKeyword("ElevationModel");
    }
    if (kernelsGroup.hasKeyword("Frame")) {
      kernelsGroup.deleteKeyword("Frame");
    }
    if (kernelsGroup.hasKeyword("StartPadding")) {
      kernelsGroup.deleteKeyword("StartPadding");
    }
    if (kernelsGroup.hasKeyword("EndPadding")) {
      kernelsGroup.deleteKeyword("EndPadding");
    }
    if (kernelsGroup.hasKeyword("RayTraceEngine")) {
      kernelsGroup.deleteKeyword("RayTraceEngine");
    }
    if (kernelsGroup.hasKeyword("OnError")) {
      kernelsGroup.deleteKeyword("OnError");
    }
    if (kernelsGroup.hasKeyword("Tolerance")) {
      kernelsGroup.deleteKeyword("Tolerance");
    }


    if (cube->label()->hasObject("NaifKeywords")) {
      cube->label()->deleteObject("NaifKeywords");
    }

    // Save off all old Blobs to restore in the case of csminit failure
    Blob originalCsmStateBlob("CSMState", "String");
    if (cube->hasBlob("CSMState", "String")) {
      cube->read(originalCsmStateBlob);
    }

    Table originalInstrumentPointing("InstrumentPointing");
    if (cube->hasTable("InstrumentPointing")) {
      originalInstrumentPointing = cube->readTable("InstrumentPointing");
    }

    Table originalInstrumentPosition("InstrumentPosition");
    if (cube->hasTable("InstrumentPosition")) {
      originalInstrumentPosition = cube->readTable("InstrumentPosition");
    }

    Table originalBodyRotation("BodyRotation");
    if (cube->hasTable("BodyRotation")) {
      originalBodyRotation = cube->readTable("BodyRotation");
    }

    Table originalSunPosition("SunPosition");
    if (cube->hasTable("SunPosition")) {
      originalSunPosition = cube->readTable("SunPosition");
    }

    Table originalCameraStatistics("CameraStatistics");
    if (cube->hasTable("CameraStatistics")) {
      originalCameraStatistics = cube->readTable("CameraStatistics");
    }

    ImagePolygon originalFootprint;
    if (cube->hasBlob("ImageFootprint", "Polygon")) {
      originalFootprint = cube->readFootprint();
    }

    // Remove blob from old csminit run
    cube->deleteBlob("CSMState", "String");

    // Remove tables from spiceinit before writing to the cube
    cube->deleteBlob("InstrumentPointing", "Table");
    cube->deleteBlob("InstrumentPosition", "Table");
    cube->deleteBlob("BodyRotation", "Table");
    cube->deleteBlob("SunPosition", "Table");
    cube->deleteBlob("CameraStatistics", "Table");
    cube->deleteBlob("Footprint", "Polygon");

    // Create our CSM State blob as a string and add the CSM string to the Blob.
    Blob csmStateBlob("CSMState", "String");
    csmStateBlob.setData(modelState.c_str(), modelState.size());
    PvlObject &blobLabel = csmStateBlob.Label();
    blobLabel += PvlKeyword("ModelName", modelName.toStdString());
    blobLabel += PvlKeyword("PluginName", pluginName.toStdString());
    cube->write(csmStateBlob);

    // Try to add naif keywords
    if (ui.WasEntered("ISD")) {
      QString isdFilePath = ui.GetFileName("ISD");
      std::ifstream isdFileStream(isdFilePath.toStdString());
      try {
        json isd = json::parse(isdFileStream);

        if (isd.contains("naif_keywords")) {
          json aleNaifKeywords = isd["naif_keywords"];
          PvlObject naifKeywords = PvlObject("NaifKeywords", aleNaifKeywords);
          *cube->label() += naifKeywords;
        }
      }
      catch (json::parse_error &e) {
        // continue if we hit a parse error
      }
    }

    try {
      CameraFactory::Create(*cube);
      p.WriteHistory(*cube);
    }
    catch (IException &e) {
      // Restore the original groups on the label
      cube->deleteGroup("Instrument");
      if (originalInstrument.keywords() != 0) {
        cube->putGroup(originalInstrument);
      }

      cube->deleteGroup("Kernels");
      if (originalKernels.keywords() != 0) {
        cube->putGroup(originalKernels);
      }

      cube->deleteGroup("CsmInfo");
      if (originalCsmInfo.keywords() != 0) {
        cube->putGroup(originalCsmInfo);
      }

      cube->deleteBlob("CSMState", "String");

      // Restore the original blobs
      if (originalCsmStateBlob.Size() != 0) {
        cube->write(originalCsmStateBlob);
      }

      if (originalInstrumentPointing.Records() != 0) {
        cube->write(originalInstrumentPointing);
      }

      if (originalInstrumentPosition.Records() != 0) {
        cube->write(originalInstrumentPosition);
      }

      if (originalBodyRotation.Records() != 0) {
        cube->write(originalBodyRotation);
      }

      if (originalSunPosition.Records() != 0) {
        cube->write(originalSunPosition);
      }

      if (originalCameraStatistics.Records() != 0) {
        cube->write(originalCameraStatistics);
      }


      if (originalFootprint.Polys() != NULL &&
          originalFootprint.Polys()->getNumGeometries() != 0) {
        cube->write(originalFootprint);
      }

      std::string message = "Failed to create a CSMCamera.";
      throw IException(e, IException::Unknown, message, _FILEINFO_);
    }
  }
}
