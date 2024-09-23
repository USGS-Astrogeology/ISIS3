/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <QString>

#include "Cube.h"
#include "IString.h"
#include "FileName.h"
#include "ProcessImportPds.h"
#include "Pvl.h"
#include "PvlKeyword.h"
#include "PvlGroup.h"
#include "UserInterface.h"

using namespace Isis;
using namespace std;

void addTableKeywords(Pvl *isisLabel, Pvl pdsLabelPvl);

void IsisMain() {
  // Get user interface
  UserInterface &ui = Application::GetUserInterface();
  // Copy the input image to specified output cube in BSQ format
  FileName from = ui.GetFileName("FROM").toStdString();
  QString pdsLabelFile = QString::fromStdString(from.expanded());
  Pvl pdsLabelPvl;
  ProcessImportPds p;
  p.SetPdsFile(pdsLabelFile, "", pdsLabelPvl);

  QString instId = QString::fromStdString(pdsLabelPvl["INSTRUMENT_ID"][0]);
  if(instId != "HIRISE_IDEAL_CAMERA") {
    std::string msg = "Invalid PDS label [" + from.expanded() + "]. The PDS product"
                  " must be from an Ideal camera model derived from a HiRISE"
                  " image. The INSTRUMENT_ID = [" + instId.toStdString() + "] is unsupported"
                  " by pds2hideal.";
    throw IException(IException::Io, msg, _FILEINFO_);
  }

  Cube *outputCube = p.SetOutputCube("TO");
  Pvl otherGroups;
  // translate the band bin and archive groups to this pvl
  p.TranslatePdsLabels(otherGroups);

  p.ImportTable("INSTRUMENT_POINTING_TABLE");
  p.ImportTable("INSTRUMENT_POSITION_TABLE");
  p.ImportTable("SUN_POSITION_TABLE");
  p.ImportTable("BODY_ROTATION_TABLE");
  p.StartProcess();

  // add translated values from band bin and archive groups to the output cube
  outputCube->putGroup(otherGroups.findGroup("BandBin"));
  outputCube->putGroup(otherGroups.findGroup("Archive"));

  PvlGroup kernelGroup("Kernels");
  kernelGroup += PvlKeyword("NaifIkCode", toString(-74699));
  kernelGroup += PvlKeyword("TargetPosition", "Table");
  kernelGroup += PvlKeyword("InstrumentPointing", "Table");
  kernelGroup += PvlKeyword("InstrumentPosition", "Table");
  QString shapeModelPath = ui.GetString("SHAPEMODELPATH");
  if (!shapeModelPath.endsWith('/')) {
    shapeModelPath += "/";
  }
  QString shapeModelValue = shapeModelPath + QString::fromStdString(pdsLabelPvl["SHAPE_MODEL"][0]);
  kernelGroup += PvlKeyword("ShapeModel", shapeModelValue.toStdString());
  outputCube->putGroup(kernelGroup);


  Pvl *isisLabel = outputCube->label();
  PvlToPvlTranslationManager labelXlater(pdsLabelPvl,
                                 "$ISISROOT/appdata/translations/MroHiriseIdealPdsImportLabel.trn");
  labelXlater.Auto(*isisLabel);

  PvlObject &naifKeywords = isisLabel->findObject("NaifKeywords");
  PvlKeyword bodyRadii("BODY499_RADII");
  bodyRadii.addValue(pdsLabelPvl["A_AXIS_RADIUS"]);
  bodyRadii.addValue(pdsLabelPvl["B_AXIS_RADIUS"]);
  bodyRadii.addValue(pdsLabelPvl["C_AXIS_RADIUS"]);
  naifKeywords += bodyRadii;

  PvlObject &isisCubeObject = isisLabel->findObject("IsisCube");
  // Compute and add SOFTWARE_NAME to the Archive Group
  QString sfname = "Isis " + Application::Version() + " " +
            Application::GetUserInterface().ProgramName();
  PvlGroup &archiveGroup = isisCubeObject.findGroup("Archive");
  archiveGroup += PvlKeyword("SOFTWARE_NAME", sfname.toStdString());

  PvlObject &pdsImageObj = pdsLabelPvl.findObject("IMAGE");
  double samples = double(pdsImageObj["LINE_SAMPLES"]);
  double lines = double(pdsImageObj["LINES"]);
  double firstSamp = double(pdsImageObj["FIRST_LINE_SAMPLE"]);
  double firstLine = double(pdsImageObj["FIRST_LINE"]);
  double sourceLines = double(pdsImageObj["SOURCE_LINES"]);
  double sourceSamps = double(pdsImageObj["SOURCE_LINE_SAMPLES"]);
  if (sourceLines != lines) {
    // this image is cropped, create an AlphaCube group
    PvlGroup alphaCube("AlphaCube");
    alphaCube += PvlKeyword("AlphaSamples", toString(sourceSamps));
    alphaCube += PvlKeyword("AlphaLines", toString(sourceLines));
    alphaCube += PvlKeyword("AlphaStartingSample", toString(firstSamp));
    alphaCube += PvlKeyword("AlphaEndingSample", toString((double) firstSamp + samples));
    alphaCube += PvlKeyword("AlphaStartingLine", toString(firstLine));
    alphaCube += PvlKeyword("AlphaEndingLine", toString((double) firstLine + lines));
    alphaCube += PvlKeyword("BetaSamples", toString(samples));
    alphaCube += PvlKeyword("BetaLines", toString(lines));
    isisCubeObject += alphaCube;
  }

  addTableKeywords(isisLabel, pdsLabelPvl);
  p.EndProcess();
}

/**
 * This method will add the appropriate keywords from the TABLE objects of the
 * input labels to the Table objects in output Isis labels.
 *
 * @param isisLabel Pointer to the output file's label
 * @param pdsLabelPvl A Pvl containing the input pds file's label.
 *
 */
void addTableKeywords(Pvl *isisLabel, Pvl pdsLabelPvl) {
  // add keywords to appropriate tables
   for (int i = 0; i < isisLabel->objects(); i++) {
     if (isisLabel->object(i).name() == "Table") {
       PvlKeyword keyword;
       if (QString::fromStdString((isisLabel->object(i)["Name"])) == "InstrumentPointing") {
         keyword = pdsLabelPvl.findObject("INSTRUMENT_POINTING_TABLE")["TIME_DEPENDENT_FRAMES"];
         keyword.setName("TimeDependentFrames");
         isisLabel->object(i) += keyword;
         keyword = pdsLabelPvl.findObject("INSTRUMENT_POINTING_TABLE")["CONSTANT_FRAMES"];
         keyword.setName("ConstantFrames");
         isisLabel->object(i) += keyword;
         keyword = pdsLabelPvl.findObject("INSTRUMENT_POINTING_TABLE")["CONSTANT_ROTATION"];
         keyword.setName("ConstantRotation");
         isisLabel->object(i) += keyword;
         keyword = pdsLabelPvl.findObject("INSTRUMENT_POINTING_TABLE")["CK_TABLE_START_TIME"];
         keyword.setName("CkTableStartTime");
         isisLabel->object(i) += keyword;
         keyword = pdsLabelPvl.findObject("INSTRUMENT_POINTING_TABLE")["CK_TABLE_END_TIME"];
         keyword.setName("CkTableEndTime");
         isisLabel->object(i) += keyword;
         keyword = pdsLabelPvl.findObject("INSTRUMENT_POINTING_TABLE")["CK_TABLE_ORIGINAL_SIZE"];
         keyword.setName("CkTableOriginalSize");
         isisLabel->object(i) += keyword;
       }
       if (QString::fromStdString((isisLabel->object(i)["Name"])) == "InstrumentPosition") {
         keyword = pdsLabelPvl.findObject("INSTRUMENT_POSITION_TABLE")["CACHE_TYPE"];
         keyword.setName("CacheType");
         isisLabel->object(i) += keyword;
         keyword = pdsLabelPvl.findObject("INSTRUMENT_POSITION_TABLE")["SPK_TABLE_START_TIME"];
         keyword.setName("SpkTableStartTime");
         isisLabel->object(i) += keyword;
         keyword = pdsLabelPvl.findObject("INSTRUMENT_POSITION_TABLE")["SPK_TABLE_END_TIME"];
         keyword.setName("SpkTableEndTime");
         isisLabel->object(i) += keyword;
         keyword = pdsLabelPvl.findObject("INSTRUMENT_POSITION_TABLE")["SPK_TABLE_ORIGINAL_SIZE"];
         keyword.setName("SpkTableOriginalSize");
         isisLabel->object(i) += keyword;
       }
       if (QString::fromStdString((isisLabel->object(i)["Name"])) == "BodyRotation") {
         keyword = pdsLabelPvl.findObject("BODY_ROTATION_TABLE")["TIME_DEPENDENT_FRAMES"];
         keyword.setName("TimeDependentFrames");
         isisLabel->object(i) += keyword;
         keyword = pdsLabelPvl.findObject("BODY_ROTATION_TABLE")["CK_TABLE_START_TIME"];
         keyword.setName("CkTableStartTime");
         isisLabel->object(i) += keyword;
         keyword = pdsLabelPvl.findObject("BODY_ROTATION_TABLE")["CK_TABLE_END_TIME"];
         keyword.setName("CkTableEndTime");
         isisLabel->object(i) += keyword;
         keyword = pdsLabelPvl.findObject("BODY_ROTATION_TABLE")["CK_TABLE_ORIGINAL_SIZE"];
         keyword.setName("CkTableOriginalSize");
         isisLabel->object(i) += keyword;
         keyword = pdsLabelPvl.findObject("BODY_ROTATION_TABLE")["SOLAR_LONGITUDE"];
         keyword.setName("SolarLongitude");
         isisLabel->object(i) += keyword;
       }
       if (QString::fromStdString((isisLabel->object(i)["Name"])) == "SunPosition") {
         keyword = pdsLabelPvl.findObject("SUN_POSITION_TABLE")["CACHE_TYPE"];
         keyword.setName("CacheType");
         isisLabel->object(i) += keyword;
         keyword = pdsLabelPvl.findObject("SUN_POSITION_TABLE")["SPK_TABLE_START_TIME"];
         keyword.setName("SpkTableStartTime");
         isisLabel->object(i) += keyword;
         keyword = pdsLabelPvl.findObject("SUN_POSITION_TABLE")["SPK_TABLE_END_TIME"];
         keyword.setName("SpkTableEndTime");
         isisLabel->object(i) += keyword;
         keyword = pdsLabelPvl.findObject("SUN_POSITION_TABLE")["SPK_TABLE_ORIGINAL_SIZE"];
         keyword.setName("SpkTableOriginalSize");
         isisLabel->object(i) += keyword;
       }
     }
   }
}
