/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

// $Id: mdis2isis.cpp 5059 2013-03-11 16:37:00Z slambright@GS.DOI.NET $
#include "Isis.h"

#include <cfloat>
#include <cstdio>
#include <QString>

#include "ProcessImportPds.h"
#include "ProcessByLine.h"

#include "UserInterface.h"
#include "FileName.h"
#include "IException.h"
#include "iTime.h"
#include "Buffer.h"
#include "TextFile.h"
#include "CSVReader.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlSequence.h"
#include "LineManager.h"
#include "OriginalLabel.h"
#include "SpecialPixel.h"
#include "tnt_array1d.h"
#include "tnt_array1d_utils.h"

using namespace std;
using namespace Isis;

typedef TNT::Array1D<double> LutTable;

//  Establish MDIS DN maximums
static const double WACValidMaximum = 3600.0;
static const double NACValidMaximum = 3400.0;

LutTable lut;
Pvl TranslateMdisEdrLabels(FileName &labelFile, const QString &target = "");
int CreateFilterSpecs(const QString &instId, int filter_code,
                      PvlGroup &bandbin, QString &naifId);
void UnlutData(Buffer &data);
LutTable LoadLut(Pvl &label, QString &tableused, QString &lutid);
Cube *outCube = NULL;
double validMaxDn = WACValidMaximum;  //  Assumes the WAC


/**
 * @brief Helper function to convert values to doubles
 *
 * @param T Type of value to convert
 * @param value Value to convert
 *
 * @return double Converted value
 */
template <typename T> double ToDouble(const T &value) {
  return (IString(value.toStdString()).Trim(" \r\t\n").ToDouble());
}

template <typename T> int ToInteger(const T &value) {
  return (IString(value.toStdString()).Trim(" \r\t\n").ToInteger());
}


void IsisMain() {
  ProcessImportPds p;
  Pvl pdsLabel;
  UserInterface &ui = Application::GetUserInterface();
  bool needsUnlut = false;

  // Get the input filename and make sure it is a MESSENGER/MDIS EDR
  FileName inFile = ui.GetFileName("FROM").toStdString();
  IString id;
  bool projected;
  Pvl lab(inFile.expanded());

  try {
    needsUnlut = (int) lab.findKeyword("MESS:COMP12_8");
    // Check for NAC imager
    if((int) lab.findKeyword("MESS:IMAGER") == 1) validMaxDn = NACValidMaximum;
    id =  (std::string)lab.findKeyword("MISSION_NAME");
    projected = lab.hasObject("IMAGE_MAP_PROJECTION");
  }
  catch(IException &e) {
    std::string msg = "Unable to read [MISSION] from input file [" +
                  inFile.expanded() + "]";
    throw IException(e, IException::Io, msg, _FILEINFO_);
  }

  //Checks if in file is rdr
  if(projected) {
    std::string msg = "[" + inFile.name() + "] appears to be an rdr file.";
    msg += " Use pds2isis.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  id.ConvertWhiteSpace();
  id.Compress();
  id.Trim(" ");
  if(id != "MESSENGER") {
    std::string msg = "Input file [" + inFile.expanded() + "] does not appear to be " +
                 "in MESSENGER EDR format. MISSION_NAME is [" + id + "]";
    throw IException(IException::Io, msg, _FILEINFO_);
  }

  QString target;
  if(ui.WasEntered("TARGET")) {
    target = ui.GetString("TARGET");
  }

  // Perform PDS/EDR source keyword translations to ISIS label keywords
  p.SetPdsFile(QString::fromStdString(inFile.expanded()), "", pdsLabel);
  Pvl outLabel = TranslateMdisEdrLabels(inFile, target);
  PvlKeyword sourceId("SourceProductId", '"' + inFile.baseName() + '"');

  //  Create YearDoy keyword in Archive group
  iTime stime( QString::fromStdString(outLabel.findGroup("Instrument", Pvl::Traverse)["StartTime"][0]));
  PvlKeyword yeardoy("YearDoy", Isis::toString(stime.Year()*1000 + stime.DayOfYear()));
  (void) outLabel.findGroup("Archive", Pvl::Traverse).addKeyword(yeardoy);

  if(ui.GetBoolean("UNLUT") == false || !needsUnlut) {
    // We're not going to unlut the data, so just set output cube
    //   and let ProcessImportPds do the writing for us.
    CubeAttributeOutput &outAtt = ui.GetOutputAttribute("TO");
    outCube = p.SetOutputCube(ui.GetCubeName("TO"), outAtt);

    // Write the Instrument, BandBin, Archive, and Kernels groups to the output
    // cube label
    PvlGroup &group =  outLabel.findGroup("Instrument", Pvl::Traverse);
    group.addKeyword(PvlKeyword("Unlutted", Isis::toString((int)!needsUnlut)));
    outCube->putGroup(group);
    outCube->putGroup(outLabel.findGroup("BandBin", Pvl::Traverse));

    group = outLabel.findGroup("Archive", Pvl::Traverse);
    group.addKeyword(sourceId, Pvl::Replace);
    outCube->putGroup(group);

    outCube->putGroup(outLabel.findGroup("Kernels", Pvl::Traverse));

    outCube = NULL;

    //  Set valid ranges
    p.SetNull(DBL_MIN, 0.0);
    p.SetHIS(validMaxDn, DBL_MAX);

    p.StartProcess();
  }
  else {
    // Unlut is indicated, so we need to handle the conversion and the cube
    // writing.   Also will enforce DN limits.
    QString lutfile, lutid;
    lut = LoadLut(lab, lutfile, lutid);

    outCube = new Cube();
    outCube->setDimensions(p.Samples(), p.Lines(), p.Bands());
    outCube->create(ui.GetCubeName("TO"));

    PvlGroup &group =  outLabel.findGroup("Instrument", Pvl::Traverse);
    group.addKeyword(PvlKeyword("Unlutted", Isis::toString((int)true)));
    group.addKeyword(PvlKeyword("LutInversionTable", lutfile.toStdString()));
    outCube->label()->findObject("IsisCube").addGroup(group);

    group = outLabel.findGroup("Archive", Pvl::Traverse);
    sourceId.addValue('"' + lutid.toStdString() + '"');
    group.addKeyword(sourceId);
    outCube->label()->findObject("IsisCube").addGroup(group);

    outCube->label()->findObject("IsisCube").addGroup(outLabel.findGroup("BandBin", Pvl::Traverse));
    outCube->label()->findObject("IsisCube").addGroup(outLabel.findGroup("Kernels", Pvl::Traverse));

    p.StartProcess(UnlutData);

    OriginalLabel ol(Pvl(inFile.expanded()));
    outCube->write(ol);
    outCube->close();
    delete outCube;
  }

  // All finished with the ImportPds object
  p.EndProcess();
}

Pvl TranslateMdisEdrLabels(FileName &labelFile, const QString &target) {
  //Create a PVL to store the translated labels
  Pvl outLabel;

  std::string transDir = "$ISISROOT/appdata/translations/";

  // Get a filename for the MESSENGER EDR label
  Pvl labelPvl(labelFile.expanded());

  // Translate the Instrument group
  FileName transFile(transDir + "MessengerMdisInstrument.trn");
  PvlToPvlTranslationManager instrumentXlater(labelPvl, QString::fromStdString(transFile.expanded()));
  instrumentXlater.Auto(outLabel);

  // Translate the BandBin group
  transFile  = transDir + "MessengerMdisBandBin.trn";
  PvlToPvlTranslationManager bandBinXlater(labelPvl, QString::fromStdString(transFile.expanded()));
  bandBinXlater.Auto(outLabel);

  // Translate the Archive group
  transFile  = transDir + "MessengerMdisArchive.trn";
  PvlToPvlTranslationManager archiveXlater(labelPvl, QString::fromStdString(transFile.expanded()));
  archiveXlater.Auto(outLabel);

  // Create the Kernel Group
  PvlGroup kerns("Kernels");
  PvlGroup &bandbin(outLabel.findGroup("BandBin", Pvl::Traverse));
  PvlGroup &instGrp(outLabel.findGroup("Instrument", Pvl::Traverse));
  QString instId =  QString::fromStdString(instGrp["InstrumentId"]);
  QString naifCode;

  // Establish Filter specific keywords
  CreateFilterSpecs(instId, (int) instGrp["FilterWheelPosition"], bandbin,
                    naifCode);
  kerns += PvlKeyword("NaifIkCode", naifCode.toStdString());
  outLabel.addGroup(kerns);

//  If the user specifed the target explicitly or it doesn't exist, create
//  something so the camera will always work
  if(instGrp.findKeyword("TargetName").isNull() || (!target.isEmpty())) {
    if(!target.isEmpty()) {
      instGrp["TargetName"] = target.toStdString();
    }
    else {
      instGrp["TargetName"] = "Sky";
    }
  }

//  Compute the gimble pivot angle and write to the label
  double pivotCounter = (double) instGrp["PivotPosition"];
  double pivotAngle   = pivotCounter / ((double)(2 << 15)) * 180.0;
  instGrp += PvlKeyword("PivotAngle", Isis::toString(pivotAngle), "Degrees");

  return outLabel;
}

/**
 * @brief Determine filter code from filter wheel position code
 *
 * This routine will determine the true filter wheel from the
 * MESS:FW_POS in and image EDR keyword.  This routine will
 * open the file
 * $Messenger/calibration/mdisCalibration????.trn"
 *
 * @param filter_code The value of the MESS:FW_POS keyword in
 *                    the EDR label.
 * @return int Valid filter number between 1 and 12, otherwise
 *             returns 0.
 */
int CreateFilterSpecs(const QString &instId, int filter_code,
                      PvlGroup  &bandbin, QString &naifCode) {

  //  WAC Filter table
  struct {
    int filter;
    const char *code;
    const char *name;
    const char *center;
    const char *width;
  } WACfilters[] = {
    {  1, "A",   "700 BW 5",  "698.8",   "5.3" },
    {  2, "B", "700 BW 600",  "700.0", "600.0" },
    {  3, "C",  "480 BW 10",  "479.9",  "10.1" },
    {  4, "D",   "560 BW 5",  "558.9",   "5.8" },
    {  5, "E",   "630 BW 5",  "628.8",   "5.5" },
    {  6, "F",  "430 BW 40",  "433.2",  "18.1" },
    {  7, "G",   "750 BW 5",  "748.7",   "5.1" },
    {  8, "H",   "950 BW 7",  "947.0",   "6.2" },
    {  9, "I", "1000 BW 15",  "996.2",  "14.3" },
    { 10, "J",   "900 BW 5",  "898.8",   "5.1" },
    { 11, "K", "1020 BW 40", "1012.6",  "33.3" },
    { 12, "L",   "830 BW 5",  "828.4",   "5.2" }
  };



  naifCode = "NULL";
  int filter(0);
  QString name, center, width;

  if(instId == "MDIS-NAC") {
    naifCode =  "-236820";
    filter = 2;
    name = "748 BP 53";
    center = "747.7";
    width = "52.6";
  }
  else if(instId == "MDIS-WAC") {
    //  Set up WAC calibration file
    FileName calibFile("$messenger/calibration/mdisCalibration????.trn");
    calibFile = calibFile.highestVersion();
    Pvl config(calibFile.expanded());

    PvlGroup &confgrp = config.findGroup("FilterWheel");
    int tolerance = confgrp["EncoderTolerance"];

    naifCode =  "-236800";
    for(int filterTry = 1; filterTry <= 12 ; filterTry++) {
      int idealPosition = confgrp["EncoderPosition" + Isis::toString(filterTry)];
      if((filter_code <= (idealPosition + tolerance)) &&
          (filter_code >= (idealPosition - tolerance))) {
        int fno = filterTry - 1;
        filter = WACfilters[fno].filter;
        name   = WACfilters[fno].name;
        center = WACfilters[fno].center;
        width  = WACfilters[fno].width;
        break;
      }
    }
  }
  else {
    //  Not the expected instrument
    std::string msg = "Unknown InstrumentId [" + instId.toStdString() + "], image does not " +
                 "appear to be from the MESSENGER/MDIS Camera";
    throw IException(IException::Io, msg, _FILEINFO_);
  }

  if(!name.isEmpty()) {
    bandbin.addKeyword(PvlKeyword("Number", Isis::toString(filter)), PvlContainer::Replace);
    bandbin.addKeyword(PvlKeyword("Name", name.toStdString()), PvlContainer::Replace);
    bandbin.addKeyword(PvlKeyword("Center", center.toStdString(), "NM"), PvlContainer::Replace);
    bandbin.addKeyword(PvlKeyword("Width", width.toStdString(), "NM"), PvlContainer::Replace);
  }
  else {
    //  If we reach here, we cannot validate the number - set it to unknown
    bandbin.addKeyword(PvlKeyword("Number", "Unknown"), PvlContainer::Replace);
  }

  return (0);
}

void UnlutData(Buffer &data) {
  LineManager out(*outCube);
  out.SetLine(data.Line(), data.Band());

  for(int i = 0; i < data.size(); i++) {
    int dnvalue = (int)(data[i] + 0.5);
    if(dnvalue < 0 || dnvalue > 255) {
      throw IException(IException::User,
                       "In the input file a value of [" +
                       IString(data[i]) +
                       "] was found. Unlutted images should only contain values 0 to 255.",
                       _FILEINFO_);
    }

    out[i] = lut[dnvalue];
  }

  outCube->write(out);
}

LutTable LoadLut(Pvl &label, QString &tableused, QString &froot) {
  int tableToUse = label.findKeyword("MESS:COMP_ALG");

  FileName tableFile("$messenger/calibration/LUT_INVERT/MDISLUTINV_?.TAB");
  tableFile = tableFile.highestVersion();
  tableused = QString::fromStdString(tableFile.originalPath() + "/" + tableFile.name());
  froot = QString::fromStdString(tableFile.baseName());

  CSVReader csv(QString::fromStdString(tableFile.expanded()));

  int nRows = csv.rows();
  if(nRows != 256) {
    std::ostringstream mess;
    mess << "MDIS LUT Inversion table, " << tableFile.expanded()
         << ", should contain 256 rows but has " << nRows;
    throw IException(IException::User, mess.str(), _FILEINFO_);
  }

  int nCols = csv.columns();
  if(nCols != 9) {
    std::ostringstream mess;
    mess << "MDIS LUT Inversion table, " << tableFile.expanded()
         << ", should contain 9 columns but has " << nCols;
    throw IException(IException::User, mess.str(), _FILEINFO_);
  }

  LutTable mlut(nRows, Null); // 8 bit => 12 bit, 2^8 = 256 conversion values

  // reset the lookup values to a known bad value
  int tableRow = tableToUse + 1;
  for(int i = 0; i < nRows; i++) {
    CSVReader::CSVAxis row = csv.getRow(i);
    int dn8 = ToInteger(row[0]);

    // Gut check!
    if((dn8 < 0) || (dn8 >= nRows)) {
      std::ostringstream mess;
      mess << "Index (" << dn8 << ") at line " << i + 1
           << " is invalid inMDIS LUT Inversion table "
           << tableFile.expanded() << " - valid range is 0 <= index < 256!";
      throw IException(IException::User, mess.str(), _FILEINFO_);
    }

    double dn16 = ToDouble(row[tableRow]);
    if(dn16 > validMaxDn) dn16 = His;
    mlut[dn8] = dn16;
  }

  //  Ensure the 0th pixel is NULL
  mlut[0] = Null;

  // cout << "Lut Table " << mlut << endl;
  return (mlut);
}
