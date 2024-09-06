/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <istream>
#include <sstream>
#include <string>

#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTextStream>

#include <SpiceUsr.h>

#include "FileName.h"
#include "History.h"
#include "IException.h"
#include "IString.h"
#include "NaifStatus.h"
#include "ProcessImportPds.h"
#include "ProgramLauncher.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlToPvlTranslationManager.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;

void TranslateVoyagerLabels(Pvl &inputLab, Cube *ocube);
void ConvertComments(FileName file);
QByteArray fixLabels(QString fileName, History *hist);

void IsisMain() {
  // We should be processing a PDS file
  ProcessImportPds p;
  UserInterface &ui = Application::GetUserInterface();
  FileName in = ui.GetFileName("FROM");

  QString tempName = "$TEMPORARY/" + in.baseName() + ".img";
  FileName temp(tempName);

  bool tempFile = false;

  // input files are compressed, use vdcomp to decompress
  QString ext = in.extension().toUpper();
  if (ext == "IMQ") {
    try {
      QString command = "$ISISROOT/bin/vdcomp " + in.expanded() + " " + temp.expanded();
      // don't pretend vdcomp is a standard Isis program, just run it
      ProgramLauncher::RunSystemCommand(command);
      in = temp.expanded();
      ConvertComments(in);
      tempFile = true;
    }
    catch (IException &e) {
      throw IException(IException::Io,
                       "Unable to decompress input file ["
                       + in.name() + "].", _FILEINFO_);
    }
  }

  // Convert the pds file to a cube
  Pvl *pdsLabel = new Pvl();

  // Preparse the IMG to fix messed up labels

  History *hist = new History();
  QByteArray pdsData = fixLabels(in.expanded(), hist);

  QTextStream pdsTextStream(&pdsData);
  istringstream pdsStream(pdsTextStream.readAll().toStdString());

  pdsStream >> *pdsLabel;

  if (pdsLabel->hasKeyword("LBL") &&
      pdsLabel->findKeyword("LBL").isNull()) {
    std::string msg = "No label information.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  if (pdsLabel->hasKeyword("PRODUCT_TYPE") &&
  !pdsLabel->findKeyword("PRODUCT_TYPE").isEquivalent("DECOMPRESSED_RAW_IMAGE")) {
    std::string msg = "ISIS does not currently support images of product types other than raw.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  try {
    p.SetPdsFile(*pdsLabel, in.expanded());
  }
  catch (IException &e) {
    std::string msg = "Unable to set PDS file. Decompressed input file ["
                 + in.name() + "] does not appear to be a PDS product";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  Cube *ocube = p.SetOutputCube("TO");
  p.StartProcess();
  try  {
    TranslateVoyagerLabels(*pdsLabel, ocube);
  }
  catch (IException &e) {
    e.print();
  }

  ocube->write(*hist);

  p.EndProcess();

  delete pdsLabel;
  if (tempFile) QFile::remove(temp.expanded());
}

/**
 * Converts / * Some comment
 * to       #   Some comment
 * without the extra space.
 */
void ConvertComments(FileName file) {
  char tmp[10240];

  for(unsigned int i = 0; i < sizeof(tmp) / sizeof(char); i++)
    tmp[i] = '\0';

  unsigned int lineStartPos = 0;
  fstream stream;
  QString filename = file.expanded();
  stream.open(filename.toLatin1().data(), fstream::in | fstream::out | fstream::binary);

  lineStartPos = stream.tellg();
  stream.getline(tmp, sizeof(tmp) / sizeof(char));
  while(!QString(tmp).startsWith("END") && stream.good()) {
    QString lineOfData(tmp);

    if(lineOfData.contains("/*") &&
       !lineOfData.contains("*/")) {
      lineOfData = lineOfData.mid(0, lineOfData.indexOf("/*")) + "# " +
                   lineOfData.mid(lineOfData.indexOf("/*") + 2);
      stream.seekp(lineStartPos);
      stream.write(lineOfData.toLatin1().data(), lineOfData.length());
    }

    lineStartPos = stream.tellg();
    stream.getline(tmp, sizeof(tmp) / sizeof(char));
  }

  stream.close();
}


/**
 * @brief Translate labels into Isis3
 *
 * @param inputLabel Reference to pvl label from the input image
 * @param ocube Pointer to output cube
 * @internal
 *   @history 2009-03-11 Jeannie Walldren - Original Version
 *   @history 2015-07-22 Kristin Berry - Added NaifStatus::CheckErrors()
 */
void TranslateVoyagerLabels(Pvl &inputLab, Cube *ocube) {
  Pvl inputLabel(inputLab);

  // Get the directory where the Voyager translation tables are
  QString missionDir = "$ISISROOT/appdata";
  FileName transFile(missionDir + "/translations/voyager.trn");

  // Get the translation manager ready
  PvlToPvlTranslationManager labelXlater(inputLabel, transFile.expanded());

  // Pvl output label
  Pvl *outputLabel = ocube->label();
  labelXlater.Auto(*(outputLabel));

  // Add needed keywords that are not in the translation table
  PvlGroup &inst = outputLabel->findGroup("Instrument", Pvl::Traverse);

  // Add Camera_State_1 and Camera_State_2
  // Camera_State_1 is the first number in ScanModeId
  // Camera_State_2 is from ShutterModeId and is 1 or 0, it is only 1 if
  // it is WA and BSIMAN or BOTSIM
  PvlKeyword sModeId = inst["ScanModeId"];
  QString cs1 = QString::fromStdString(sModeId[0]).split(":").first();
  inst.addKeyword(PvlKeyword("CameraState1",cs1.toStdString()));

  QString shutterMode = QString::fromStdString(inst["ShutterModeId"]);
  QString cam = QString::fromStdString(inst["InstrumentId"]);
  if (cam == "WIDE_ANGLE_CAMERA" && (shutterMode == "BOTSIM" || shutterMode == "BSIMAN")) {
    inst.addKeyword(PvlKeyword("CameraState2","1"));
  }
  else {
    inst.addKeyword(PvlKeyword("CameraState2","0"));
  }

  // Translate the band bin group information
  if(QString::fromStdString(inst["InstrumentId"]) == "WIDE_ANGLE_CAMERA") {
    FileName bandBinTransFile(missionDir + "/translations/voyager_wa_bandbin.trn");
    PvlToPvlTranslationManager labelXlater(inputLabel, bandBinTransFile.expanded());
    labelXlater.Auto(*(outputLabel));
  }
  else {
    FileName bandBinTransFile(missionDir + "/translations/voyager_na_bandbin.trn");
    PvlToPvlTranslationManager labelXlater(inputLabel, bandBinTransFile.expanded());
    labelXlater.Auto(*(outputLabel));
  }

  // Add units of measurement to keywords from translation table
  inst.findKeyword("ExposureDuration").setUnits("seconds");

  PvlGroup &bandBin = outputLabel->findGroup("BandBin", Pvl::Traverse);
  bandBin.findKeyword("Center").setUnits("micrometers");
  bandBin.findKeyword("Width").setUnits("micrometers");

  // Setup the kernel group
  PvlGroup kern("Kernels");
  QString spacecraftNumber;
  int spacecraftCode = 0;

  QString instId = QString::fromStdString(inst.findKeyword("InstrumentId"));

  if(QString::fromStdString(inst.findKeyword("SpacecraftName")) == "VOYAGER_1") {
    spacecraftNumber = "1";
    if(instId == "NARROW_ANGLE_CAMERA") {
      spacecraftCode = -31101;
      kern += PvlKeyword("NaifFrameCode", std::to_string(spacecraftCode));
      instId = "issna";
    }
    else if (instId == "WIDE_ANGLE_CAMERA") {
      spacecraftCode = -31102;
      kern += PvlKeyword("NaifFrameCode", std::to_string(spacecraftCode));
      instId = "isswa";
    }
    else {
      std::string msg = "Instrument ID [" + instId + "] does not match Narrow or " +
                    "Wide angle camera. The cube was created, but the labels were not translated. "
                    "To create a cube with translated labels, re-run this application with "
                    "INSTRUMENT set to NAC or WAC.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }
  else if(QString::fromStdString(inst.findKeyword("SpacecraftName")) == "VOYAGER_2") {
    spacecraftNumber = "2";
    if(instId == "NARROW_ANGLE_CAMERA") {
      spacecraftCode = -32101;
      kern += PvlKeyword("NaifFrameCode", std::to_string(spacecraftCode));
      instId = "issna";
    }
    else if (instId == "WIDE_ANGLE_CAMERA") {
      spacecraftCode = -32102;
      kern += PvlKeyword("NaifFrameCode", std::to_string(spacecraftCode));
      instId = "isswa";
    }
    else {
      std::string msg = "Instrument ID [" + instId + "] does not match Narrow or " +
                    "Wide angle camera. The cube was created, but the labels were not translated. "
                    "To create a cube with translated labels, re-run this application with "
                    "INSTRUMENT set to NAC or WAC.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }
  else {
    std::string msg = "Spacecraft name [" + QString::fromStdString(inst.findKeyword("SpacecraftName")) +
                 "] does not match Voyager1 or Voyager2 spacecraft";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  ocube->putGroup(kern);

  // Modify time to remove Z from end
  QString time = QString::fromStdString(inst.findKeyword("StartTime")[0]);
  time.remove("Z");
  inst.findKeyword("StartTime").setValue(time.toStdString());

  // Fix image number - remove the period, if Wide angle camera and one of two
  // shutter modes, we must fix the wide angle image number for use below.
  // Before #####.##     After #######
  QString imgNumber = QString::fromStdString(inst["SpacecraftClockCount"][0]);
  imgNumber.replace(".", "");
  // Save this change
  inst["SpacecraftClockCount"] = imgNumber.toStdString();

  // From vgrfixlabel documentation in Isis2.
  // Wide Angle (WA) images off of CD's will have a fake image
  // number (NA image_number + scan_mode_id) written over the
  // image_number label, which would have gotten the FSC count
  // rather than the FSD count if a Narrow Angle was shuttered
  // simultaneously.  What I want is to figure out the NA image-
  // number, which will allow me to find the shutter time (same
  // for both NA & WA).  So run backward, WA image_number - scan_
  // mode_id.
  // If BSIMAN or BOTSIM and WA, go ahead.
  if((inst["ShutterModeId"][0] == "BSIMAN" ||
      inst["ShutterModeId"][0] == "BOTSIM") &&
      inst["InstrumentId"][0] == "WIDE_ANGLE_CAMERA") {
    QString scanId = QString::fromStdString(inst["ScanModeId"][0]);
    int scanNum = toInt(scanId.mid(0, 1));
    int imgNum = toInt(imgNumber);

    // We'll use this later, however, we do not write it to the labels.
    // if we didn't get in here, we'll be using the original image number,
    // otherwise, we'll use this modified image number.
    imgNumber = QString::number(imgNum - scanNum);
  }

  // This next section handles modifying the starttime slightly and requires
  // the leapsecond kernel, the spacecraft clock kernel, and a correct
  // spacecraft clock.
  // This functionality was copied from Isis2 by Mackenzie Boyd, the
  // documentation from Isis2 is here: (from vgrfixlabel.c)
  /*******************************************************************
    Get spacecraftClock from image FSC.

    Calculate the START_TIME keyword value from FSC to get fractional
    seconds (PDS START_TIME provided is only to the nearest whole
    second).  The algorithm below was extracted from the NAIF
    document Viking Orbiter Time Tag Analysis and Restoration by
    Boris Semenov and Chuck Acton and was modified for Voyager by
    Debbie Cook and K Teal Thompson.

    1.  Determine instCode, spacecraftCode.
        Get FSC (FDS_COUNT) from IMAGE_NUMBER to use as
        spacecraftClock.  This was already done above when the
        IMAGE_NUMBER was read into imagenum.
        //Already accomplished above

    2.  Need to calc the image number for wide Angle images when
        SHUTTER_MODE_ID = BOTSIM or BSIMAN because the IMAGE_NUMBER
        on the labels is set to the readout count instead of the FDS
        (FLIGHT DATA SUBSYSTEM) count.  In the old code, SCAN_MODE_ID
        is used to add to the image_number to get a new image_number
        for WA.  In this code, take WA image number and subtract
        scan_mode id to get the narrow angle image_number.
        // This is also accomplished above

    3.  Load a leap second kernel and the appropriate FSC spacecraft
        clock kernel based on the spacecraft (Voyager 1 or Voyager
        2).  Then convert image_number/spacecraftClock to et.


    4.  Convert et to UTC calendar format and write to labels as
        START_TIME


  ********************************************************************/

  // We've already handled a couple of the steps mentioned above.
  NaifStatus::CheckErrors();
  //* 3 *//
  // Leapsecond kernel
  QString lsk = "$base/kernels/lsk/naif????.tls";
  FileName lskName(lsk);
  lskName = lskName.highestVersion();
  furnsh_c(lskName.expanded().toLatin1().data());

  // Spacecraft clock kernel
  QString sclk = "$ISISDATA/voyager";
  sclk.append(spacecraftNumber);
  sclk.append("/kernels/sclk/vg");
  sclk.append(spacecraftNumber);
  sclk.append("?????.tsc");
  FileName sclkName(sclk);
  sclkName = sclkName.highestVersion();
  furnsh_c(sclkName.expanded().toLatin1().data());

  // The purpose of the next two steps, getting the spacecraft clock count,
  // are simply to get the partition, the very first number 1/...
  double approxEphemeris = 0.0;
  utc2et_c(QString::fromStdString(inst["StartTime"][0]).toLatin1().data(), &approxEphemeris);
  char approxSpacecraftClock[80];

  // sce2s_c requires the spacecraft number, not the instrument number as
  // we've found elsewhere, either -31 or -32 in this case.
  int spacecraftClockNumber = -30;
  spacecraftClockNumber -= toInt(spacecraftNumber);
  sce2s_c(spacecraftClockNumber, approxEphemeris, 80, approxSpacecraftClock);

  /*
   * For our next trick, we will substitute the image number we got earlier
   * into this image number. The image number is in the format #######,
   * 7 digits, our freshly gotten image number is in the form #/#####:##:##
   * We want to save the first digit, before the /, and then substitute
   * the previously found image number into the next 7 digits, keeping the colon,
   * and then removing the last :## colon and two digits. Here goes.
   * BTW, our imageNumber from above and this number may be identical,
   * in which case this won't change anything in it. However, since the
   * conversion from ET to spacecraftClock is a range of ETs all mapping to one
   * clock count, even if we don't modifiy the spacecraft clock count, we'll
   * come out with a slightly different starttime which is, in theory, more accurate.
   * To rephrase, ETs are continuous, Spacecraft clock is discrete. For later
   * we will use the Starttime, the ET, so we want it to match the spacecraft clock
   * exactly. But, the original starttime holds valuable information... enough.
   */

  // Get first digit and the /
  QString newClockCount = QString(approxSpacecraftClock).mid(0, 2);
  // Get first five digits of imgNumber and append
  newClockCount.append(imgNumber.mid(0, 5));
  // I'll stop commenting now, you can read code as well as me
  newClockCount.append(":");
  // I lied ;) get the last two digits.
  newClockCount.append(imgNumber.mid(5, 2));

  scs2e_c(spacecraftClockNumber, newClockCount.toLatin1().data(), &approxEphemeris);

  //* 4 *//
  char utcOut[25];
  et2utc_c(approxEphemeris, "ISOC", 3, 26, utcOut);
  NaifStatus::CheckErrors();
  inst["StartTime"].setValue(utcOut);

  // Set up the nominal reseaus group
  PvlGroup res("Reseaus");
  Pvl nomRes("$voyager" + spacecraftNumber.toStdString() + "/reseaus/nominal.pvl");
  PvlKeyword samps, lines, type, valid;
  lines = PvlKeyword("Line");
  samps = PvlKeyword("Sample");
  type = PvlKeyword("Type");
  valid = PvlKeyword("Valid");

  PvlKeyword key = nomRes.findKeyword("VG" + spacecraftNumber.toStdString() + "_"
                                      + instId.toUpper().toStdString() + "_RESEAUS");
  int numRes = nomRes["VG" + spacecraftNumber.toStdString() + "_" + instId.toUpper().toStdString()
                      + "_NUMBER_RESEAUS"];
  for(int i = 0; i < numRes * 3; i += 3) {
    lines += key[i];
    samps += key[i+1];
    type += key[i+2];
    valid += "0";
  }
  res += lines;
  res += samps;
  res += type;
  res += valid;
  res += PvlKeyword("Template", "$voyager" + spacecraftNumber.toStdString() + "/reseaus/vg"
                    + spacecraftNumber.toStdString() + "." + instId.toLower().toStdString()
                    + ".template.cub");
  res += PvlKeyword("Status", "Nominal");
  ocube->putGroup(res);
  NaifStatus::CheckErrors();
}


/**
 * @brief   Fixes the broken tags in the IMG file. The IMG file is loaded into memory,
 *          and after the fixes are applied, the corrected labels are returned as
 *          a QByteArray.
 * @param   fileName The full path to the Voyager input IMG
 * @return  A QByteArray containing the modified Voyager labels. This is fed to
 *          a QTextStream which is then fed to a Pvl object.
 * @internal
 *   @history 2017-06-28 Marjorie Hahn - Original Version
 */
QByteArray fixLabels(QString fileName, History *hist){

  QByteArray null;
  QFile imgFile;

  imgFile.setFileName(fileName);

  if (!imgFile.open(QFile::ReadOnly|QIODevice::Text))
    return null;

  // Read the IMG file into a byte array
  QByteArray fileData = imgFile.readAll();
  QByteArray labels;

  QString labelEnd("\nEND");
  int ix = fileData.lastIndexOf(labelEnd.toUtf8());

  labels = fileData.left(ix + labelEnd.size());
  PvlObject hEntry = Isis::iApp->History();

  // Check if the instrument name is valid
  if (labels.contains(QByteArray("INSTRUMENT_NAME\n"))) {

    // Only set name to Unknown and warn the user about a missing INSTRUMENT_NAME
    // if it wasn't specified by the user.
    UserInterface &ui = Application::GetUserInterface();
    if (ui.GetString("INSTRUMENT") == "AUTOMATIC") {
      labels.replace("INSTRUMENT_NAME", "INSTRUMENT_NAME                  = Unknown");
      PvlGroup insNameWarning("Warning");
      PvlKeyword insNameMsg("Message", "The INSTRUMENT_NAME for [" + fileName.toStdString() + "] is empty."
                                + "The InstrumentId in the output cube will instead be set to "
                                + "[Unknown] and the labels will not translate. To create a cube "
                                + "with translated labels, re-run this "
                                + "application with INSTRUMENT set to NAC or WAC.");

      insNameWarning += insNameMsg;
      Application::Log(insNameWarning);

      hEntry += insNameWarning;
    }
    else if (ui.GetString("INSTRUMENT") == "NAC") {
      labels.replace("INSTRUMENT_NAME", "INSTRUMENT_NAME                  = NARROW_ANGLE_CAMERA");
    }
    else if (ui.GetString("INSTRUMENT") == "WAC") {
      labels.replace("INSTRUMENT_NAME", "INSTRUMENT_NAME                  = WIDE_ANGLE_CAMERA");
    }
  }

  // Check if image id is valid
  int imageIdIndex = labels.indexOf(QByteArray("IMAGE_ID"));
  int i = imageIdIndex;
  char c = 0;
  bool replaceImageId = false;
  while (c != '\n' && i != -1) {
    c = labels[i];

    // If character is out of range, image id is corrupt and must be replaced
    if (c < 0 || c > 127) {
      replaceImageId = true;
    }
    i++;
  }

  QFileInfo fi(fileName);
  QString baseName = fi.baseName();


  if (replaceImageId == true) {
    labels.replace(imageIdIndex,
                   i - imageIdIndex - 1,
                   QByteArray("IMAGE_ID                         = ").append(baseName.toUtf8()));
    PvlGroup insIdWarning("Warning");
    PvlKeyword insIdMsg("Message", "The IMAGE_ID for [" + fileName.toStdString() + "] is corrupted. The ProductId "
                              + "in the output cube will instead be set to [" + baseName.toStdString() + "].");
    insIdWarning += insIdMsg;
    Application::Log(insIdWarning);

    hEntry += insIdWarning;
  }
  hist->AddEntry(hEntry);

  return labels;
}
