/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "chan1m32isis.h"

#include <math.h>

#include <QDebug>
#include <QString>

#include "Application.h"
#include "BoxcarCachingAlgorithm.h"
#include "Brick.h"
#include "Cube.h"
#include "CubeAttribute.h"
#include "FileName.h"
#include "IException.h"
#include "ImportPdsTable.h"
#include "iTime.h"
#include "NaifStatus.h"
#include "OriginalLabel.h"
#include "PixelType.h"
#include "ProcessByLine.h"
#include "ProcessBySample.h"
#include "ProcessImportPds.h"
#include "Progress.h"
#include "Pvl.h"
#include "Table.h"
#include "UserInterface.h"

using namespace std;

namespace Isis {

  void writeCube(Buffer &in);
  void writeCubeWithDroppedLines(Buffer &in);
  void importImage(QString outputParamName, ProcessImportPds::PdsFileType fileType);
  void importImage(QString outputParamName, ProcessImportPds::PdsFileType fileType, UserInterface &ui);
  void translateChandrayaan1M3Labels(Pvl &pdsLabel, Cube *ocube, Table &utcTable,
                                     ProcessImportPds::PdsFileType fileType);
  void flip(Buffer &in);
  void flipUtcTable(Table &utcTable);
  Cube *g_oCube;
  Brick *g_oBuff;
  int g_totalLinesAdded;
  double g_expectedLineRate;
  Table *g_utcTable;
  PvlGroup g_results("Results");


  Pvl chan1m32isis(UserInterface &ui) {
    Pvl log;
    importImage("TO", (ProcessImportPds::PdsFileType)(ProcessImportPds::Rdn|ProcessImportPds::L0), ui);
    log.addGroup(g_results);
    importImage("LOC", ProcessImportPds::Loc, ui);
    importImage("OBS", ProcessImportPds::Obs, ui);
    return log;
  }


  void importImage(QString outputParamName, ProcessImportPds::PdsFileType fileType) {
    UserInterface &ui = Application::GetUserInterface();
    importImage(outputParamName, fileType, ui);
  }


  void importImage(QString outputParamName, ProcessImportPds::PdsFileType fileType, UserInterface &ui) {
    if (!ui.WasEntered(outputParamName)) {
      return;
    }

    g_oCube = NULL;
    g_oBuff = NULL;
    g_totalLinesAdded = 0;
    g_utcTable = NULL;
    double calcOutputLines = 0;

    ProcessImportPds importPds;
    importPds.Progress()->SetText((QString)"Writing " + outputParamName + " file");

    FileName in = ui.GetFileName("FROM");

    Pvl pdsLabel(in.expanded().toStdString());
    if (fileType == (ProcessImportPds::L0 | ProcessImportPds::Rdn)) {
      //  Is this a L0 or L1B product?
      if ((std::string) pdsLabel["PRODUCT_TYPE"] == "RAW_IMAGE") {
        fileType = ProcessImportPds::L0;
      }
      else {
        fileType = ProcessImportPds::Rdn;
      }
    }

    // Convert the pds file to a cube
    try {
      importPds.SetPdsFile(in.expanded(), "", pdsLabel, fileType);
    }
    catch(IException &e) {
      QString msg = "Input file [" + in.expanded() +
                   "] does not appear to be a Chandrayaan 1 M3 detached PDS label";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }

    bool samplesNeedFlipped = false;
    bool linesNeedFlipped = false;
    if (fileType != ProcessImportPds::L0) {
      // M3 PDS L1B images may be flipped/mirrored in sample and/or line to visually appear with
      // north nearly up. The ISIS camera model does not take this into account, so this post
      // acquisition processing needs to be removed. There are four possible flip/mirror mode
      // combinations.
      // 1.  Descending yaw / Forward orbit limb - No changes in sample or line
      // 2.  Descending yaw / Reverse orbit limb - Samples are reversed, first sample on west side
      //                                           of image
      // 3.  Ascending yaw / Forward orbit limb - Lines/times are reversed so northernmost image
      //                                          line first, Samples are reversed, first sample on
      //                                          west side of image
      // 4.  Ascending yaw / Reverse orbit limb - Lines/times are reversed so northernmost image
      //                                          line first,
      QString yawDirection = QString::fromStdString(pdsLabel["CH1:SPACECRAFT_YAW_DIRECTION"]);
      QString limbDirection = QString::fromStdString(pdsLabel["CH1:ORBIT_LIMB_DIRECTION"]);
      samplesNeedFlipped = ( ((yawDirection == "REVERSE") && (limbDirection == "DESCENDING")) ||
                             ((yawDirection == "FORWARD") && (limbDirection == "ASCENDING")) );
      linesNeedFlipped = (limbDirection == "ASCENDING");
    }

    // The following 2 commented lines can be used for testing purposes, No flipping will be done with
    // these lines uncommented i.e. north is always up, lons always pos east to the right.
    //    samplesNeedFlipped = false;
    //    linesNeedFlipped = false;

    {
      // Calculate the number of output lines that should be present from the start and end times
      // in the UTC table.
      int outputLines;
      if (fileType == ProcessImportPds::Rdn || fileType == ProcessImportPds::Loc ||
          fileType == ProcessImportPds::Obs) {
        g_utcTable = &(importPds.ImportTable("UTC_FILE"));

        if (g_utcTable->Records() >= 1) {

          QString instMode = QString::fromStdString(pdsLabel["INSTRUMENT_MODE_ID"]);
          // Initialize to the value for a GLOBAL mode observation
          g_expectedLineRate = 0.10176;
          if (instMode == "TARGET") {
            g_expectedLineRate = 0.05088;
          }

          // The UTC line time table has been flipped in the same manner as the image lines, thus fabs
          // Search the time table for gaps to come up with an output cube number of lines
          // The times in the table are documented as the time at the center of the exposure/frame, so
          // consecutive records in the time table should differ by the exposure rate, if not then
          // there is a potential gap.
          // This was calculated in the previous version of this code, but there is a minor difference
          // between the calculation and the following brute force method.
          outputLines = 0;
          for (int rec = 0; rec < g_utcTable->Records() - 1; rec++) {
            outputLines++; // One for this line

            iTime thisEt(QString::fromStdString((std::string)(*g_utcTable)[rec]["UtcTime"]));
            iTime nextEt(QString::fromStdString((std::string)(*g_utcTable)[rec+1]["UtcTime"]));
            double delta = fabs(nextEt - thisEt); // Time table may be assending or decenting times

            while (delta > g_expectedLineRate * 1.9) {
              outputLines++; // Big enough gap to need more line(s)
              delta -= g_expectedLineRate;
            }
          }
          outputLines++; // One more for the last line

          iTime firstEt(QString::fromStdString((std::string)(*g_utcTable)[0]["UtcTime"]));
          iTime lastEt(QString::fromStdString((std::string)(*g_utcTable)[g_utcTable->Records()-1]["UtcTime"]));
          calcOutputLines = fabs((lastEt + g_expectedLineRate / 2.0) -
                                 (firstEt - g_expectedLineRate / 2.0)) / g_expectedLineRate;
        }
        else {
          QString msg = "Input file [" + in.expanded() +
                       "] does not appear to have any records in the UTC_FILE table";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }
      else {
        outputLines = importPds.Lines();
        calcOutputLines = outputLines;
      }

      // Since the output cube possibly has more lines then the input PDS image, due to dropped
      // lines, we have to write the output cube instead of letting ProcessImportPds do it for us.
      g_oCube = new Cube();
      if (fileType == ProcessImportPds::L0) {
        g_oCube->setPixelType(importPds.PixelType());
      }
      g_oCube->setDimensions(importPds.Samples(), outputLines, importPds.Bands());
      g_oCube->create(ui.GetCubeName(outputParamName));
      g_oCube->addCachingAlgorithm(new BoxcarCachingAlgorithm());

      g_oBuff = new Isis::Brick(importPds.Samples(), outputLines, importPds.Bands(),
                                importPds.Samples(), 1, 1, importPds.PixelType(), true);
      g_oBuff->setpos(0);

      if (fileType == ProcessImportPds::L0) {
        importPds.StartProcess(writeCube);
      }
      else {
        importPds.StartProcess(writeCubeWithDroppedLines);
        g_results += PvlKeyword("LinesFlipped", std::to_string(linesNeedFlipped));
        g_results += PvlKeyword("SamplesFlipped", std::to_string(samplesNeedFlipped));
        g_results += PvlKeyword("LinesAdded", std::to_string(g_totalLinesAdded));
        g_results += PvlKeyword("OutputLines", std::to_string(outputLines));
        g_results += PvlKeyword("CalculatedOutputLines", std::to_string(calcOutputLines));
      }

      delete g_oBuff;

      // If the image lines need flipped then so does the UTC table, if it exists.
      // This does not need to be done before the main processing because the flipping of
      // the image is done after the main processing.
      if (fileType != ProcessImportPds::L0) {
        if (linesNeedFlipped) {
          flipUtcTable(*g_utcTable);
        }
      }

      translateChandrayaan1M3Labels(pdsLabel, g_oCube, *g_utcTable, fileType);

      if (fileType != ProcessImportPds::L0) g_oCube->write(*g_utcTable);

      importPds.WriteHistory(*g_oCube);
      importPds.Finalize();

      g_oCube->close();
      delete g_oCube;

    }

    CubeAttributeInput inAttribute;
    if (linesNeedFlipped) {
      ProcessBySample flipLines;
      flipLines.Progress()->SetText("Flipping Lines");
      Cube *cube = flipLines.SetInputCube(ui.GetCubeName(outputParamName), inAttribute);
      cube->reopen("rw");
      flipLines.ProcessCubeInPlace(flip, false);
    }

    if (samplesNeedFlipped) {
      ProcessByLine flipSamples;
      flipSamples.Progress()->SetText("Flipping Samples");
      Cube *cube = flipSamples.SetInputCube(ui.GetCubeName(outputParamName), inAttribute);
      cube->reopen("rw");
      flipSamples.ProcessCubeInPlace(flip, false);
    }
  }


  // Processing function for writing all input PDS lines to the output cube.
  // No dropped lines are inserted.
  void writeCube(Buffer &in) {

    for (int i = 0; i < in.size(); i++) {
      (*g_oBuff)[i] = in[i];
    }

    g_oCube->write(*g_oBuff);
    (*g_oBuff)++;
  }


  // Processing function for writing all input PDS lines to the output cube with dropped lines
  // inserted where the time table shows gaps.
  void writeCubeWithDroppedLines(Buffer &in) {

    // Always write the current line to the output cube first
    for (int i = 0; i < in.size(); i++) {
      (*g_oBuff)[i] = in[i];
    }

    g_oCube->write(*g_oBuff);
    (*g_oBuff)++;

    // Now check the UTC_TIME table and see if there is a gap (missing lines) after the TIME record
    // for the current line. If there are, add as many lines as are necessary to fill the gap.
    // Since the PDS files are in BIL order we are writeing to the ISIS cube in that order, so we
    // do not need to write NULL lines to fill a gap until we have written the last band of line N,
    // and we don't have to check for gaps after the last lines of the PDS file.

    if (in.Band() == g_oCube->bandCount() && in.Line() < g_utcTable->Records()) {

      QString tt = QString::fromStdString((std::string)(*g_utcTable)[in.Line() - 1]["UtcTime"]);
      QString ttt = QString::fromStdString((std::string)(*g_utcTable)[in.Line()]["UtcTime"]);

      iTime thisEt(QString::fromStdString((std::string)(*g_utcTable)[in.Line() - 1]["UtcTime"]));
      iTime nextEt(QString::fromStdString((std::string)(*g_utcTable)[in.Line()]["UtcTime"]));

      double delta = fabs(nextEt - thisEt);

      double linesToAdd = (delta / g_expectedLineRate) - 1.0;

      if (linesToAdd > 0.9) {

        // Create a NULL line
        for (int i = 0; i < in.size(); i++) {
          (*g_oBuff)[i] = Null;
        }

        while (linesToAdd > 0.9) {

          for (int band = 0; band < g_oCube->bandCount(); band++) {
            g_oCube->write(*g_oBuff);
            (*g_oBuff)++;
          }
          linesToAdd--;
          g_totalLinesAdded++;
        }
      }
    }
  }


  // Transfere the needed PDS labels to the ISIS Cube and update them where necessary
  void translateChandrayaan1M3Labels(Pvl& pdsLabel, Cube *ocube, Table& utcTable,
                                     ProcessImportPds::PdsFileType fileType) {
    Pvl outLabel;

    // Translate the archive group
    FileName transFile("$ISISROOT/appdata/translations/Chandrayaan1M3Archive.trn");
    PvlToPvlTranslationManager archiveXlator(pdsLabel, transFile.expanded());
    archiveXlator.Auto(outLabel);
    ocube->putGroup(outLabel.findGroup("Archive", Pvl::Traverse));

    // Translate the instrument group
    transFile = "$ISISROOT/appdata/translations/Chandrayaan1M3Instrument.trn";
    PvlToPvlTranslationManager instrumentXlator(pdsLabel, transFile.expanded());
    instrumentXlator.Auto(outLabel);

    PvlGroup &inst = outLabel.findGroup("Instrument", Pvl::Traverse);

    // The start and stop times for M3 in the PDS file look to have been truncated.
    // Update them with the times from the UTC table if we have one.
    if (fileType != ProcessImportPds::L0) {

      // The original START/STOPTIME keywords and the UTC table did not have accurate enough times to
      // allow spiceinit to work on ck and spk  kernels generated by ckwriter and spkwriter after
      // jigsaw, so use the clock counts to update these keywords.
      NaifStatus::CheckErrors();

      QString lsk = "$base/kernels/lsk/naif????.tls";
      FileName lskName(lsk);
      lskName = lskName.highestVersion();
      furnsh_c(lskName.expanded().toLatin1().data());

      QString sclk = "$chandrayaan1/kernels/sclk/aig_ch1_sclk_complete_biased_m1p???.tsc";
      FileName sclkName(sclk);
      sclkName = sclkName.highestVersion();
      furnsh_c(sclkName.expanded().toLatin1().data());

      SpiceInt sclkCode = -86;

      // Remmoved when we found out the lable counts are not as correct as we need. We use the time
      // tables instead (see below)
      //QString startTime = inst["SpacecraftClockStartCount"];
      //double et;
      //scs2e_c(sclkCode, startTime.toAscii().data(), &et);
      //iTime startEt(et);
      //inst.findKeyword("StartTime").setValue(startEt.UTC());

      //QString stopTime = inst["SpacecraftClockStopCount"];
      //scs2e_c(sclkCode, stopTime.toAscii().data(), &et);
      //iTime stopEt(et);
      //inst.findKeyword("StopTime").setValue(stopEt.UTC());

      // Replace code above with this
      // The start and stop times in the PDS labels do not match the UTC table times.
      // Assume the UTC table times are better, so change the labels to match the table
      // The start and stop clock counts need to match the start/stop time, so convert the times
      // to new clock counts.
      iTime firstEt(QString::fromStdString((std::string)(*g_utcTable)[0]["UtcTime"]));
      iTime lastEt(QString::fromStdString((std::string)(*g_utcTable)[utcTable.Records()-1]["UtcTime"]));

      // The table is in assending order
      // The table contains the middle of the exposure. include times to cover the beginning of
      // line 1 and the end of line NL
      if (firstEt < lastEt) {
        firstEt = firstEt - (g_expectedLineRate / 2.0);
        lastEt = lastEt + (g_expectedLineRate / 2.0);
      }
      else {
        firstEt = lastEt - (g_expectedLineRate / 2.0);
        lastEt = firstEt + (g_expectedLineRate / 2.0);
      }

      inst.findKeyword("StartTime").setValue(firstEt.UTC().toStdString());
      SpiceChar startClockString[100];
      sce2s_c (sclkCode, firstEt.Et(), 100, startClockString);
      QString startClock(startClockString);
      inst.findKeyword("SpacecraftClockStartCount").setValue(startClock.toStdString());

      inst.findKeyword("StopTime").setValue(lastEt.UTC().toStdString());
      SpiceChar stopClockString[100];
      sce2s_c (sclkCode, lastEt.Et(), 100, stopClockString);
      QString stopClock(stopClockString);
      inst.findKeyword("SpacecraftClockStopCount").setValue(stopClock.toStdString());
    }

    ocube->putGroup(inst);

    if (fileType == ProcessImportPds::L0 || fileType == ProcessImportPds::Rdn) {
      // Setup the band bin group
      QString bandFile = "$chandrayaan1/bandBin/bandBin.pvl";
      Pvl bandBinTemplate(bandFile.toStdString());
      PvlObject modeObject = bandBinTemplate.findObject(pdsLabel["INSTRUMENT_MODE_ID"]);
      PvlGroup bandGroup = modeObject.findGroup("BandBin");
      //  Add OriginalBand
      int numBands;
      if ((std::string)pdsLabel["INSTRUMENT_MODE_ID"] == "TARGET") {
        numBands = 256;
      }
      else {
        numBands = 85;
      }
      PvlKeyword originalBand("OriginalBand");
      for (int i = 1; i <= numBands; i++) {
        originalBand.addValue(std::to_string(i));
      }
      bandGroup += originalBand;
      ocube->putGroup(bandGroup);

      if (fileType == ProcessImportPds::Rdn) {
        // Setup the radiometric calibration group for the image cube
        PvlGroup calib("RadiometricCalibration");
        PvlKeyword solar = pdsLabel["SOLAR_DISTANCE"];
        calib += PvlKeyword("Units", "W/m2/um/sr");
        calib += PvlKeyword("SolarDistance", std::to_string((double)solar), solar.unit());
        calib += PvlKeyword("DetectorTemperature", std::to_string((double)pdsLabel["DETECTOR_TEMPERATURE"]));
        calib += PvlKeyword("SpectralCalibrationFileName",
                            pdsLabel["CH1:SPECTRAL_CALIBRATION_FILE_NAME"]);
        calib += PvlKeyword("RadGainFactorFileName",
                            pdsLabel["CH1:RAD_GAIN_FACTOR_FILE_NAME"]);
        calib += PvlKeyword("GlobalBandpassFileName",
                            pdsLabel["CH1:SPECTRAL_CALIBRATION_FILE_NAME"]);
        ocube->putGroup(calib);
      }
    }

    // Setup the band bin group
    else if (fileType == ProcessImportPds::Loc) {
      PvlGroup bandBin("BandBin");
      PvlKeyword bandNames = pdsLabel.findObject("LOC_IMAGE", PvlObject::Traverse)["BAND_NAME"];
      bandNames.setName("Name");
      bandBin += bandNames;
      PvlKeyword bandUnits("Units", "(Degrees, Degrees, Meters)");
      bandBin += bandUnits;
      ocube->putGroup(bandBin);
    }
    else if (fileType == ProcessImportPds::Obs) {
      PvlGroup bandBin("BandBin");
      PvlKeyword bandNames = pdsLabel.findObject("OBS_IMAGE", PvlObject::Traverse)["BAND_NAME"];
      bandNames.setName("Name");
      bandBin += bandNames;
      ocube->putGroup(bandBin);
    }

    // Setup the kernel group
    PvlGroup kern("Kernels");
    kern += PvlKeyword("NaifFrameCode", "-86520");
    ocube->putGroup(kern);

    OriginalLabel origLabel(pdsLabel);
    ocube->write(origLabel);
  }


  void flip(Buffer &in) {
    for(int i = 0; i < in.size() / 2; i++) {
      swap(in[i], in[in.size() - i - 1]);
    }
  }


  void flipUtcTable(Table &utcTable) {
    int nrecs = utcTable.Records();
    for (int i = 0; i < nrecs / 2; i++) {
      TableRecord rec1 = utcTable[i];
      TableRecord rec2 = utcTable[nrecs - i - 1];
      utcTable.Update(rec1, nrecs - i - 1);
      utcTable.Update(rec2, i);
    }
  }
}
