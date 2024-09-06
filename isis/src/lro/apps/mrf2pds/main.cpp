/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"
#include "UserInterface.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Pvl.h"
#include "ProcessExportMiniRFLroPds.h"
#include "PvlToPvlTranslationManager.h"
#include "OriginalLabel.h"
#include "SerialNumberList.h"
#include "Constants.h"
#include "Statistics.h"
#include "NaifStatus.h"
#include "Spice.h"
#include <fstream>
#include <iostream>
#include <cmath>

using namespace std;
using namespace Isis;

void FixLabel(Pvl &pcPdsLbl, bool &pbLevel2);
void GetSourceProductID(QString psSrcListFile, QString psSrcType, Pvl &pcPdsLabel);
void GetUserLabel(QString psUserLbl, Pvl &pdsLabel, bool pbLevel2);

static unsigned int iCheckSum = 0;

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  FileName inFile = ui.GetCubeName("FROM");

  // Set the processing object
  ProcessExportMiniRFLroPds cProcess;

  // Setup the input cube
  Cube *cInCube = cProcess.SetInputCube("FROM");
  Pvl *cInLabel =  cInCube->label();

  // Get the output label file
  FileName outFile(ui.GetFileName("TO", "lbl"));
  QString outFileName(outFile.expanded());

  cProcess.SetDetached(outFileName);

  cProcess.SetExportType(ProcessExportPds::Fixed);

  //Set the resolution to  Kilometers
  cProcess.SetPdsResolution(ProcessExportPds::Kilometer);

  // 32bit
  cProcess.SetOutputType(Isis::Real);
  cProcess.SetOutputNull(Isis::NULL4);
  cProcess.SetOutputLrs(Isis::LOW_REPR_SAT4);
  cProcess.SetOutputLis(Isis::LOW_INSTR_SAT4);
  cProcess.SetOutputHrs(Isis::HIGH_REPR_SAT4);
  cProcess.SetOutputHis(Isis::HIGH_INSTR_SAT4);
  cProcess.SetOutputRange(-DBL_MAX, DBL_MAX);

  cProcess.SetOutputEndian(Isis::Msb);

  // Turn off Keywords
  cProcess.ForceScalingFactor(false);
  cProcess.ForceSampleBitMask(false);
  cProcess.ForceCoreNull(false);
  cProcess.ForceCoreLrs(false);
  cProcess.ForceCoreLis(false);
  cProcess.ForceCoreHrs(false);
  cProcess.ForceCoreHis(false);

  // Standard label Translation
  Pvl &pdsLabel = cProcess.StandardPdsLabel(ProcessExportPds::Image);

  // bLevel => Level 2 = True, Level 3 = False
  bool bLevel2 = cInCube->hasGroup("Instrument");

  // Translate the keywords from the original EDR PDS label that go in
  // this RDR PDS label for Level2 images only
  if(bLevel2) {
    OriginalLabel cOriginalBlob = cInCube->readOriginalLabel();
    Pvl cOrigLabel;
    PvlObject cOrigLabelObj = cOriginalBlob.ReturnLabels();
    cOrigLabelObj.setName("OriginalLabelObject");
    cOrigLabel.addObject(cOrigLabelObj);

    // Translates the ISIS labels along with the original EDR labels
    cOrigLabel.addObject(*(cInCube->label()));
    PvlToPvlTranslationManager cCubeLabel2(cOrigLabel, "$ISISROOT/appdata/translations/AllMrfExportOrigLabel.trn");
    cCubeLabel2.Auto(pdsLabel);


    if(cInLabel->findObject("IsisCube").findGroup("Instrument").hasKeyword("MissionName")) {
      PvlKeyword &cKeyMissionName = cInLabel->findObject("IsisCube").findGroup("Instrument").findKeyword("MissionName");
      int sFound = QString::fromStdString(cKeyMissionName[0]).indexOf("CHANDRAYAAN");
      if(sFound != -1) {
        cCubeLabel2 = PvlToPvlTranslationManager(cOrigLabel, "$ISISROOT/appdata/translations/Chandrayaan1MrfExportOrigLabel.trn");
        cCubeLabel2.Auto(pdsLabel);
      }
      else {
        cCubeLabel2 = PvlToPvlTranslationManager(cOrigLabel, "$ISISROOT/appdata/translations/LroMrfExportOrigLabel.trn");
        cCubeLabel2.Auto(pdsLabel);
      }
    }
  }
  else { //Level3 - add Band_Name keyword
    PvlGroup &cBandBinGrp = cInCube->group("BandBin");
    PvlKeyword cKeyBandBin = PvlKeyword("BAND_NAME");
    PvlKeyword cKeyInBandBin;
    if(cBandBinGrp.hasKeyword("OriginalBand")) {
      cKeyInBandBin = cBandBinGrp.findKeyword("OriginalBand");
    }
    else if(cBandBinGrp.hasKeyword("FilterName")) {
      cKeyInBandBin = cBandBinGrp.findKeyword("FilterName");
    }
    for(int i = 0; i < cKeyInBandBin.size(); i++) {
      cKeyBandBin += cKeyInBandBin[i];
    }
    PvlObject &cImageObject(pdsLabel.findObject("IMAGE"));
    cImageObject += cKeyBandBin;
  }

  // Get the Sources Product ID if entered for Level2 only as per example
  if(ui.WasEntered("SRC") && bLevel2) {
    QString sSrcFile = ui.GetFileName("SRC");
    QString sSrcType = ui.GetString("TYPE");
    GetSourceProductID(sSrcFile, sSrcType, pdsLabel);
  }

  // Get the User defined Labels
  if(ui.WasEntered("USERLBL")) {
    QString sUserLbl = ui.GetFileName("USERLBL");
    GetUserLabel(sUserLbl, pdsLabel, bLevel2);
  }

  // Calculate CheckSum
  Statistics *cStats =  cInCube->statistics();
  iCheckSum = (unsigned int)cStats->Sum();

  FixLabel(pdsLabel, bLevel2);

  // Add an output format template to the PDS PVL
  // Distinguish betweeen Level 2 and 3 images by calling the camera()
  // function as only non mosaic images(Level2) have a camera
  if(bLevel2) {
    pdsLabel.setFormatTemplate("$ISISROOT/appdata/translations/MrfPdsLevel2.pft");
  }
  else {
    pdsLabel.setFormatTemplate("$ISISROOT/appdata/translations/MrfPdsLevel3.pft");
  }

  int iFound = outFileName.indexOf(".lbl");
  outFileName.replace(iFound, 4, ".img");
  ofstream oCube(outFileName.toLatin1().data());
  cProcess.OutputDetachedLabel();
  //cProcess.OutputLabel(oCube);
  cProcess.StartProcess(oCube);
  oCube.close();
  cProcess.EndProcess();
}

/**
 * Reads the file with User input in PVL format and substitutes
 * non empty keyvalues with the existing values in the output
 * PVL.
 *
 * @author sprasad (11/10/2009)
 *
 * @param psUserLbl - User Label File
 * @param pcPdsLbl  - Output Pds PVL
 */
void GetUserLabel(QString psUserLbl, Pvl &pcPdsLbl, bool pbLevel2) {
  Pvl cUsrPvl(psUserLbl.toStdString());

  /*if (pbLevel2 &&
      (cUsrPvl.hasKeyword("SPACECRAFT_CLOCK_START_COUNT") ||
       cUsrPvl.hasKeyword("SPACECRAFT_CLOCK_STOP_COUNT")  ||
       cUsrPvl.hasKeyword("START_TIME") ||
       cUsrPvl.hasKeyword("STOP_TIME"))) {

       std::string msg = "Unsupported User defined keywords for Level2";
       throw Isis::iException::Message(Isis::iException::User,msg,_FILEINFO_);
  }*/

  // Add all the keywords in the root
  for(int j = 0; j < cUsrPvl.keywords(); j++) {
    if(pcPdsLbl.hasKeyword(cUsrPvl[j].name())) {
      PvlKeyword &cKey = pcPdsLbl.findKeyword(cUsrPvl[j].name());
      cKey.clear();
      cKey.setValue(cUsrPvl[j][0]);
    }
    else
      pcPdsLbl.addKeyword(cUsrPvl[j]);
  }

  // Add keywords in the objects
  for(int j = 0; j < cUsrPvl.objects(); j++) {
    PvlObject cUsrObject = cUsrPvl.object(j);
    if(pcPdsLbl.hasObject(cUsrObject.name())) {
      PvlObject &cObject = pcPdsLbl.findObject(cUsrObject.name());
      for(int k = 0; k < cUsrObject.keywords(); k++) {
        PvlKeyword cUsrKeyword = cUsrObject[k];
        if(cObject.hasKeyword(cUsrKeyword.name())) {
          PvlKeyword &cKey = cObject.findKeyword(cUsrKeyword.name());
          cKey.clear();
          cKey.setValue(cUsrKeyword[0]);
        }
        else {
          cObject.addKeyword(cUsrKeyword);
        }
      }
    }
  }
}

/**
 * Read the input file containing source id's for a mosaic and
 * write into the output PVL with  SOURCE_PRODUCT_ID keyname
 *
 * @author sprasad (11/10/2009)
 *
 * @param psSrcListFile - File containing source id's
 * @param pcPdsLbl      - Output PVL
 */
void GetSourceProductID(QString psSrcListFile, QString psSrcType, Pvl &pcPdsLbl) {
  PvlKeyword cKeySrcPrdId;

  if(pcPdsLbl.hasKeyword("SOURCE_PRODUCT_ID")) {
    pcPdsLbl.deleteKeyword("SOURCE_PRODUCT_ID");
  }

  cKeySrcPrdId.setName("SOURCE_PRODUCT_ID");

  // List File name
  if(psSrcType == "LIST") {
    SerialNumberList cSnl = psSrcListFile;
    for(int i = 0; i < cSnl.size(); i++) {
      cKeySrcPrdId += cSnl.serialNumber(i).toStdString();
    }
  }
  // ID's - add directly to the PvlKeyword
  else {
    ifstream inIdFile;
    inIdFile.open(psSrcListFile.toLatin1().data(), std::ios::in);
    char buff[501];
    inIdFile.getline(buff, 500);
    while(!inIdFile.eof()) {
      if(buff[0] != '\n' && buff[0] != '\0') {
        cKeySrcPrdId += buff;
      }
      inIdFile.getline(buff, 500);
    }
    inIdFile.close();
  }
  pcPdsLbl += cKeySrcPrdId;
}

/**
 * Update, Add , Delete the labels in the output PVL to
 * match the required Mini RF PDS Label
 *
 * @author sprasad (11/10/2009)
 *
 * @param pcPdsLbl - Output PVL
 */
void FixLabel(Pvl &pcPdsLbl, bool &pbLevel2) {
  // Level 3
  if(!pbLevel2) {
    if(pcPdsLbl.hasKeyword("LINE_EXPOSURE_DURATION")) {
      pcPdsLbl.deleteKeyword("LINE_EXPOSURE_DURATION");
    }
    if(pcPdsLbl.hasKeyword("ORBIT_NUMBER")) {
      pcPdsLbl.deleteKeyword("ORBIT_NUMBER");
    }
    if(pcPdsLbl.hasKeyword("INCIDENCE_ANGLE")) {
      pcPdsLbl.deleteKeyword("INCIDENCE_ANGLE");
    }
    if(pcPdsLbl.hasKeyword("INSTRUMENT_MODE_ID")) {
      pcPdsLbl.deleteKeyword("INSTRUMENT_MODE_ID");
    }
    if(pcPdsLbl.hasKeyword("INSTRUMENT_MODE_DESC")) {
      pcPdsLbl.deleteKeyword("INSTRUMENT_MODE_DESC");
    }
    if(pcPdsLbl.hasKeyword("LOOK_DIRECTION")) {
      pcPdsLbl.deleteKeyword("LOOK_DIRECTION");
    }
  }

  // Additional keywords and update existing keywords
  if(pcPdsLbl.hasKeyword("LABEL_RECORDS")) {
    pcPdsLbl.deleteKeyword("LABEL_RECORDS");
  }

  if(!pcPdsLbl.hasKeyword("PRODUCER_FULL_NAME")) {
    PvlKeyword cKeyPrdFullName = PvlKeyword("PRODUCER_FULL_NAME", "USGS AstroGeology Flagstaff");
    pcPdsLbl += cKeyPrdFullName;
  }

  if(!pcPdsLbl.hasKeyword("PRODUCER_INSTITUTION_NAME")) {
    PvlKeyword cKeyPrdInstName = PvlKeyword("PRODUCER_INSTITUTION_NAME", "USGS AstroGeology");
    pcPdsLbl += cKeyPrdInstName;
  }

  if(!pcPdsLbl.hasKeyword("MISSION_NAME")) {
    PvlKeyword cKeyMissionName = PvlKeyword("MISSION_NAME", "LUNAR RECONNAISSANCE ORBITER");
    pcPdsLbl += cKeyMissionName;
  }

  if(!pcPdsLbl.hasKeyword("PRODUCER_ID")) {
    PvlKeyword cKeyPrdID = PvlKeyword("PRODUCER_ID", "USGS");
    pcPdsLbl += cKeyPrdID;
  }

  time_t startTime = time(NULL);
  struct tm *tmbuf = gmtime(&startTime);
  char timestr[80];
  strftime(timestr, 80, "%Y-%m-%dT%H:%M:%S", tmbuf);
  PvlKeyword cKeyPrdCreationTime("PRODUCT_CREATION_TIME", timestr);
  pcPdsLbl += cKeyPrdCreationTime;

  PvlKeyword cKeySoftware("SOFTWARE_NAME", "ISIS3");
  pcPdsLbl += cKeySoftware;

  PvlKeyword cKeySoftwareVersion("SOFTWARE_VERSION_ID", Application::Version().toStdString());
  pcPdsLbl += cKeySoftwareVersion;

  // Specific to IMAGE Object
  PvlObject &cImageObject(pcPdsLbl.findObject("IMAGE"));
  if(cImageObject.hasKeyword("OFFSET")) {
    cImageObject.deleteKeyword("OFFSET");
  }

  // Update SAMPLE_TYPE from "IEEE_" to "PC_"
  if(cImageObject.hasKeyword("SAMPLE_TYPE")) {
    PvlKeyword &cSampleType = cImageObject.findKeyword("SAMPLE_TYPE");
    QString sVal = QString::fromStdString(cSampleType[0]);
    int iFound = sVal.indexOf('_');
    if(iFound >= 0) {
      sVal.replace(0, iFound, "PC");
      cSampleType.setValue(sVal.toStdString());
    }
  }

  if(pbLevel2) {
    char buff[62];
    snprintf(buff, sizeof(buff),  "%d", iCheckSum);
    PvlKeyword cKeyChkSum("CHECKSUM", buff);
    cImageObject += cKeyChkSum;
  }

  // Projection object
  PvlObject &cProjectionObject(pcPdsLbl.findObject("IMAGE_MAP_PROJECTION"));
  if(cProjectionObject.hasKeyword("PROJECTION_LATITUDE_TYPE")) {
    cProjectionObject.deleteKeyword("PROJECTION_LATITUDE_TYPE");
  }

  if(!cProjectionObject.hasKeyword("COORDINATE_SYSTEM_TYPE")) {
    PvlKeyword cKeyCordSysType = PvlKeyword("COORDINATE_SYSTEM_TYPE", " ");
    cProjectionObject += cKeyCordSysType;
  }

  if(cProjectionObject.hasKeyword("MAP_PROJECTION_TYPE")) {
    PvlKeyword cKeyPrjType = cProjectionObject.findKeyword("MAP_PROJECTION_TYPE");
    if(cKeyPrjType[0] == "OBLIQUE CYLINDRICAL") {
      PvlKeyword &cKeyCenLon = cProjectionObject.findKeyword("CENTER_LONGITUDE");
      cKeyCenLon.setValue("0.0 <DEG>");
      PvlKeyword &cKeyCenLat = cProjectionObject.findKeyword("CENTER_LATITUDE");
      cKeyCenLat.setValue("0.0 <DEG>");

      if(pbLevel2) {
        // Get the X,Y,Z values from Proj X Axis Vector
        PvlKeyword cKeyObXProj = cProjectionObject.findKeyword("OBLIQUE_PROJ_X_AXIS_VECTOR");
        double x, y, z;
        x = IString(cKeyObXProj[0]).ToDouble();
        y = IString(cKeyObXProj[1]).ToDouble();
        z = IString(cKeyObXProj[2]).ToDouble();

        // Calculate Reference Latitude and Longitude
        // angle in degrees = angle in radians * 180 / Pi
        /*double dRad,dLon,dLat;
        reclat_c(xAxis,&dRad,&dLon,&dLat);
        dLat = dLat * 180.0 / Isis::PI;
        dLon = dLon * 180.0 / Isis::PI;
        */

        double dLon, dLat;
        dLon = atan(y / x) * (180 / Isis::PI);
        dLat = atan(z / (sqrt(pow(x, 2) + pow(y, 2)))) * (180 / Isis::PI);

        PvlKeyword cKeyRefLon("REFERENCE_LONGITUDE");
        QString iStr = toString(dLon, 6);
        iStr += " <DEG>";
        cKeyRefLon.setValue(iStr.toStdString());
        cProjectionObject.addKeyword(cKeyRefLon);

        PvlKeyword cKeyRefLat("REFERENCE_LATITUDE");
        iStr = toString(dLat, 6);
        iStr += " <DEG>";
        cKeyRefLat.setValue(iStr.toStdString());
        cProjectionObject.addKeyword(cKeyRefLat);
      }
      else { // Level3 Projection object
        if(cProjectionObject.hasKeyword("OBLIQUE_PROJ_POLE_LATITUDE")) {
          cProjectionObject.deleteKeyword("OBLIQUE_PROJ_POLE_LATITUDE");
        }
        if(cProjectionObject.hasKeyword("OBLIQUE_PROJ_POLE_LONGITUDE")) {
          cProjectionObject.deleteKeyword("OBLIQUE_PROJ_POLE_LONGITUDE");
        }
        if(cProjectionObject.hasKeyword("OBLIQUE_PROJ_POLE_ROTATION")) {
          cProjectionObject.deleteKeyword("OBLIQUE_PROJ_POLE_ROTATION");
        }
        if(cProjectionObject.hasKeyword("OBLIQUE_PROJ_X_AXIS_VECTOR")) {
          cProjectionObject.deleteKeyword("OBLIQUE_PROJ_X_AXIS_VECTOR");
        }
        if(cProjectionObject.hasKeyword("OBLIQUE_PROJ_Y_AXIS_VECTOR")) {
          cProjectionObject.deleteKeyword("OBLIQUE_PROJ_Y_AXIS_VECTOR");
        }
        if(cProjectionObject.hasKeyword("OBLIQUE_PROJ_Z_AXIS_VECTOR")) {
          cProjectionObject.deleteKeyword("OBLIQUE_PROJ_Z_AXIS_VECTOR");
        }
      }
    }
  }

  PvlKeyword cKeyDataSetMapProj("^DATA_SET_MAP_PROJECTION", "DSMAP.CAT");
  cProjectionObject += cKeyDataSetMapProj;

  if(cProjectionObject.hasKeyword("LINE_PROJECTION_OFFSET")) {
    PvlKeyword &cLineProjOffset = cProjectionObject.findKeyword("LINE_PROJECTION_OFFSET");
    QString sVal = QString::fromStdString(cLineProjOffset[0]);
    int iFound = sVal.indexOf('<');
    if(iFound >= 0) {
      sVal.remove(iFound, 1);
      cLineProjOffset.setValue(sVal.toStdString());
    }
  }

  if(cProjectionObject.hasKeyword("SAMPLE_PROJECTION_OFFSET")) {
    PvlKeyword &cSampleProjOffset = cProjectionObject.findKeyword("SAMPLE_PROJECTION_OFFSET");
    QString sVal = QString::fromStdString(cSampleProjOffset[0]);
    int iFound = sVal.indexOf('<');
    if(iFound >= 0) {
      sVal.remove(iFound, 1);
      cSampleProjOffset.setValue(sVal.toStdString());
    }
  }

  if(cProjectionObject.hasKeyword("MAP_SCALE")) {
    PvlKeyword &cMapScale = cProjectionObject.findKeyword("MAP_SCALE");
    QString sVal = QString::fromStdString(cMapScale[0]);
    int iFound = sVal.indexOf("EL");
    if(iFound >= 0) {
      sVal.remove(iFound, 2);
      cMapScale.setValue(sVal.toStdString());
    }
  }
}
