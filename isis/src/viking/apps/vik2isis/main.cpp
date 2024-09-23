/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <cstdio>
#include <QString>
#include "stdlib.h"

#include "ProcessImportPds.h"
#include "UserInterface.h"
#include "FileName.h"
#include "IException.h"
#include "Pvl.h"
#include "IString.h"

using namespace std;
using namespace Isis;

void TranslateVikingLabels(Pvl &pdsLabel, Cube *ocube);

void IsisMain() {
  // We should be processing a PDS file
  ProcessImportPds p;
  UserInterface &ui = Application::GetUserInterface();
  FileName in = ui.GetFileName("FROM").toStdString();

  std::string tempName = "$TEMPORARY/" + in.baseName() + ".img";
  FileName temp(tempName);
  bool tempFile = false;

  // This program handles both compressed and decompressed files.
  // To discover if a file is compressed attempt to create a Pvl
  // object.  If this fails then the file must be compressed, so
  // decompress the file using vdcomp.
  try {
    Pvl compressionTest(in.expanded());

    if(compressionTest.groups() == 0 && compressionTest.objects() == 0 &&
        compressionTest.keywords() < 2) {
      throw IException(IException::Programmer, "", _FILEINFO_);
    }
  }
  catch(IException &e) {
    tempFile = true;
    QString command = QString::fromStdString("$ISISROOT/bin/vdcomp " + in.expanded() + " " +
                     temp.expanded() + " > /dev/null 2>&1");
    int returnValue = system(command.toLatin1().data()) >> 8;
    if(returnValue) {
      std::string msg = "Error running vdcomp";
      IException::ErrorType msgTarget = IException::Programmer;
      switch(returnValue) {
        case 1:
          msg =  "Vik2Isis called vdcomp and help mode was triggered.\n";
          msg += "Were any parameters passed?";
          break;
        case 2:
          msg =  "vdcomp could not write its output file.\n" +
                 msg + "Check disk space or for duplicate filename.";
          break;
        case 3:
          msg = "vdcomp could not open the input file!";
          break;
        case 4:
          msg = "vdcomp could not open its output file!";
          break;
        case 5:
          msg = "vdcomp: Out of memory in half_tree!";
          break;
        case 6:
          msg = "vdcomp: Out of memory in new_node";
          break;
        case 7:
          msg = "vdcomp: Invalid byte count in dcmprs";
          break;
        case 42:
          msg = "Input file [" + in.name() + "] has\ninvalid or" +
                " corrupted line header table!";
          msgTarget = IException::User;
          break;
      }
      throw IException(msgTarget, msg, _FILEINFO_);
    }
    in = temp.expanded();
  }

  // Convert the pds file to a cube
  Pvl pdsLabel;
  try {
    p.SetPdsFile(QString::fromStdString(in.expanded()), "", pdsLabel);
  }
  catch(IException &e) {
    std::string msg = "Input file [" + in.expanded() +
                 "] does not appear to be a Viking PDS product";
    throw IException(e, IException::User, msg, _FILEINFO_);
  }

  Cube *ocube = p.SetOutputCube("TO");
  p.StartProcess();
  TranslateVikingLabels(pdsLabel, ocube);
  p.EndProcess();

  if(tempFile) remove(temp.expanded().c_str());
  return;
}

void TranslateVikingLabels(Pvl &pdsLabel, Cube *ocube) {
  // Setup the archive group
  PvlGroup arch("Archive");
  arch += PvlKeyword("DataSetId", pdsLabel["DATA_SET_ID"]);
  arch += PvlKeyword("ProductId", pdsLabel["IMAGE_ID"]);
  arch += PvlKeyword("MissonPhaseName", pdsLabel["MISSION_PHASE_NAME"]);
  arch += PvlKeyword("ImageNumber", pdsLabel["IMAGE_NUMBER"]);
  arch += PvlKeyword("OrbitNumber", pdsLabel["ORBIT_NUMBER"]);
  ocube->putGroup(arch);

  // Setup the instrument group
  // Note SpacecraftClockCount used to be FDS_COUNT
  PvlGroup inst("Instrument");
  inst += PvlKeyword("SpacecraftName", pdsLabel["SPACECRAFT_NAME"]);
  inst += PvlKeyword("InstrumentId", pdsLabel["INSTRUMENT_NAME"]);
  inst += PvlKeyword("TargetName", pdsLabel["TARGET_NAME"]);

  QString stime = QString::fromStdString(pdsLabel["IMAGE_TIME"]);
  stime.remove(QRegExp("Z$"));
  inst += PvlKeyword("StartTime", stime.toStdString());

  inst += PvlKeyword("ExposureDuration",
                     pdsLabel["EXPOSURE_DURATION"], "seconds");
  inst += PvlKeyword("SpacecraftClockCount", pdsLabel["IMAGE_NUMBER"]);
  inst += PvlKeyword("FloodModeId", pdsLabel["FLOOD_MODE_ID"]);
  inst += PvlKeyword("GainModeId", pdsLabel["GAIN_MODE_ID"]);
  inst += PvlKeyword("OffsetModeId", pdsLabel["OFFSET_MODE_ID"]);
  ocube->putGroup(inst);

  // Setup the band bin group
  PvlGroup bandBin("BandBin");
  QString filterName = QString::fromStdString(pdsLabel["FILTER_NAME"]);
  bandBin += PvlKeyword("FilterName", filterName.toStdString());

  int filterId;
  double filterCenter = 0.0;
  double filterWidth = 0.0;
  if(filterName == "BLUE") {
    filterId = 1;
    filterCenter = 0.470000;
    filterWidth = 0.180000;
  }
  if(filterName == "MINUS_BLUE") {
    filterId = 2;
    filterCenter = 0.550000;
    filterWidth = 0.220000;
  }
  if(filterName == "VIOLET") {
    filterId = 3;
    filterCenter = 0.440000;
    filterWidth = 0.120000;
  }
  if(filterName == "CLEAR") {
    filterId = 4;
    filterCenter = 0.520000;
    filterWidth = 0.350000;
  }
  if(filterName == "GREEN") {
    filterId = 5;
    filterCenter = 0.530000;
    filterWidth = 0.100000;
  }
  if(filterName == "RED") {
    filterId = 6;
    filterCenter = 0.590000;
    filterWidth = 0.150000;
  }
  bandBin += PvlKeyword("FilterId", Isis::toString(filterId));
  bandBin += PvlKeyword("Center", Isis::toString(filterCenter), "micrometers");
  bandBin += PvlKeyword("Width", Isis::toString(filterWidth), "micrometers");
  ocube->putGroup(bandBin);

  // Setup the kernel group
  PvlGroup kern("Kernels");
  int spn;
  if(QString::fromStdString(pdsLabel["SPACECRAFT_NAME"]) == "VIKING_ORBITER_1") {
    if(QString::fromStdString(pdsLabel["INSTRUMENT_NAME"]) ==
        "VISUAL_IMAGING_SUBSYSTEM_CAMERA_A") {
      kern += PvlKeyword("NaifFrameCode", "-27001");
    }
    else {
      kern += PvlKeyword("NaifFrameCode", "-27002");
    }
    spn = 1;
  }
  else {
    if(QString::fromStdString(pdsLabel["INSTRUMENT_NAME"]) ==
        "VISUAL_IMAGING_SUBSYSTEM_CAMERA_A") {
      kern += PvlKeyword("NaifFrameCode", "-30001");
    }
    else {
      kern += PvlKeyword("NaifFrameCode", "-30002");
    }
    spn = 2;
  }
  ocube->putGroup(kern);

  // Set up the nominal reseaus group
  PvlGroup res("Reseaus");
  Pvl nomRes("$viking" + Isis::toString(spn) + "/reseaus/nominal.pvl");
  PvlKeyword samps, lines, type, valid;
  lines = PvlKeyword("Line");
  samps = PvlKeyword("Sample");
  type = PvlKeyword("Type");
  valid = PvlKeyword("Valid");
  QString prefix;
  if(QString::fromStdString(pdsLabel["SPACECRAFT_NAME"]) == "VIKING_ORBITER_1") {
    if(QString::fromStdString(pdsLabel["INSTRUMENT_NAME"]) ==
        "VISUAL_IMAGING_SUBSYSTEM_CAMERA_A") {
      prefix = "VO1_VISA_";
    }
    else {
      prefix = "VO1_VISB_";
    }
  }
  else {
    if(QString::fromStdString(pdsLabel["INSTRUMENT_NAME"]) ==
        "VISUAL_IMAGING_SUBSYSTEM_CAMERA_A") {
      prefix = "VO2_VISA_";
    }
    else {
      prefix = "VO2_VISB_";
    }
  }
  PvlKeyword key = nomRes.findKeyword(prefix.toStdString() + "RESEAUS");
  int numRes = nomRes[prefix.toStdString() + "NUMBER_RESEAUS"];
  for(int i = 0; i < numRes * 3; i += 3) {
    lines += key[i];
    samps += key[i+1];
    type += key[i+2];
    valid += 0;
  }
  res += lines;
  res += samps;
  res += type;
  res += valid;
  if(prefix == "VO1_VISA_") {
    res += PvlKeyword("Template", "$viking1/reseaus/vo1.visa.template.cub");
  }
  else if(prefix == "VO1_VISB_") {
    res += PvlKeyword("Template", "$viking1/reseaus/vo1.visb.template.cub");
  }
  else if(prefix == "VO2_VISA_") {
    res += PvlKeyword("Template", "$viking2/reseaus/vo2.visa.template.cub");
  }
  else res += PvlKeyword("Template", "$viking2/reseaus/vo2.visb.template.cub");
  res += PvlKeyword("Status", "Nominal");
  ocube->putGroup(res);
}
