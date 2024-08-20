/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"
#include "Application.h"
#include "FileList.h"
#include "IException.h"
#include "UserInterface.h"
#include "IString.h"
#include "Preference.h"
#include "ProgramLauncher.h"

using namespace std;
using namespace Isis;

void IsisMain(){

  UserInterface &ui = Application::GetUserInterface();

  // Determine which listing files will be created
  bool reportHiInc = ui.WasEntered("HIGHINCLIST");
  bool reportNoFile = ui.WasEntered("NOFILELIST");
  bool reportNadirSpc = ui.WasEntered("NADIRSPCLIST");
  bool reportImageGap = ui.WasEntered("IMAGEGAPLIST");

  // Get all user parameters before proceeding with
  // processing

  // Input file options
  FileList cubes;
  Pvl &pref = Preference::Preferences();
  QString pathName = QString::fromStdString(pref.findGroup("DataDirectory")["Temporary"]) + "/";
  if (ui.WasEntered("TOPATH")) {
    pathName = (QString)ui.GetString("TOPATH") + "/";
  }
  QString outFile;
  if (ui.WasEntered("FROMLIST")) {
    cubes.read(ui.GetFileName("FROMLIST"));
  }
  else {
    QString msg = "Error: FROMLIST file must be specified";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Processing Options
  bool processNight  = ui.GetBoolean("NIGHT");
  bool ignoreAtmCorr = ui.GetBoolean("ATMOSCORR");

  // Output Information for GIS Software Package
  bool logCamStats = ui.GetBoolean("INFO");
  bool footPrintInit = false;
  QString logFileName;
  if (logCamStats) {
    footPrintInit = ui.GetBoolean("FOOTPRINT");
    if (!ui.WasEntered("TOSTAT")) {
      logCamStats = false;
    } else {
      logFileName = ui.GetAsString("TOSTAT");
    }
  }

  // do stuff to build the file that will go into the spreadsheet
  // LOG_STATS is true will create a flat file of statistics.
  ofstream logFileos;
  if (logCamStats) {
    FileName logFile(logFileName);
    if (logFile.fileExists()) {
      logFileos.open(logFileName.toLatin1().data(),ios::app);
    }
    else {
      logFileos.open(logFileName.toLatin1().data(),ios::out);
      logFileos << "Filename," << "Duration," << "Summing,"<< "IncidenceAverage," <<
              "ResolutionAverage,"<< "IncidenceMinimum," << "IncidenceMaximum," <<  "Gaps,"<<endl;
    }
  }

  // Get Clean up options
  bool pdsSanFile;
  bool rmInput      = ui.GetBoolean("RMINPUT");
  bool rmHiIncInput = ui.GetBoolean("RMHIGHINC");

  // Create high incidence report file if requested
  FileName hiIncList;
  ofstream hiIncListos;
  if (rmHiIncInput && reportHiInc) {
    hiIncList = ui.GetFileName("HIGHINCLIST");
    IString hiIncListStr = hiIncList.expanded();
    if (hiIncList.fileExists()) {
      hiIncListos.open(hiIncListStr.c_str(),ios::app);
    }
    else {
      hiIncListos.open(hiIncListStr.c_str(),ios::out);
      hiIncListos << "List of filenames with high incidence angles" << endl;
    }
  }

  // Create missing RDR file report if requested
  FileName noFileList;
  ofstream noFileListos;
  if (reportNoFile) {
    noFileList = ui.GetFileName("NOFILELIST");
    IString noFileListStr = noFileList.expanded();
    if (noFileList.fileExists()) {
      noFileListos.open(noFileListStr.c_str(),ios::app);
    }
    else {
      noFileListos.open(noFileListStr.c_str(),ios::out);
      noFileListos << "List of files not processed and reason for not processing" << endl;
    }
  }

  // Create Nadir pointing file report if requested
  FileName nadirSpcList;
  ofstream nadirSpcListos;
  if (reportNadirSpc) {
    nadirSpcList = ui.GetFileName("NADIRSPCLIST");
    IString nadirSpcListStr = nadirSpcList.expanded();
    if (nadirSpcList.fileExists()) {
      nadirSpcListos.open(nadirSpcListStr.c_str(),ios::app);
    }
    else {
      nadirSpcListos.open(nadirSpcListStr.c_str(),ios::out);
      nadirSpcListos << "List of Nadir pointing files" << endl;
    }
  }

  // Create image gap file report if requested
  FileName imageGapList;
  ofstream imageGapListos;
  if (reportImageGap) {
    imageGapList = ui.GetFileName("IMAGEGAPLIST");
    IString imageGapListStr = imageGapList.expanded();
    if (imageGapList.fileExists()) {
      imageGapListos.open(imageGapListStr.c_str(),ios::app);
    }
    else {
      imageGapListos.open(imageGapListStr.c_str(),ios::out);
      imageGapListos << "List of files with gaps" << endl;
    }
  }

  // Main processing loop
  for (int i = 0; i < cubes.size(); i++) {

  try {
    pdsSanFile = false;

    // Open the input file
    FileName infile = cubes[i];
    QString inFileStr = infile.expanded();

    if (!infile.fileExists()) {
      if (reportNoFile) {
        noFileListos << infile.baseName() << " not processed because RDR file is missing" << endl;
      }
      continue;
    }

    Pvl lab(inFileStr.toStdString());

    // Exit if not Themis image
    IString instrumentID = IString(lab["INSTRUMENT_ID"][0]);
    if (instrumentID.UpCase() != "THEMIS") {
      if (reportNoFile) {
        noFileListos << infile.baseName() << " not processed because is not a THEMIS image" << endl;
      }
      QString msg = "Error: Not a Themis Image";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Verify for "IR" Detector ID
    IString detectorID = IString(lab["DETECTOR_ID"][0]);
    if (detectorID.UpCase() != "IR") {
      if (reportNoFile) {
        noFileListos << infile.baseName() << " not processed because is not an IR THEMIS image" << endl;
      }
      QString msg = "Error: Not an IR Themis Image";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if (inFileStr.indexOf("pds_san") != -1) {
      pdsSanFile = true;
    }

    QString duration =QString::fromStdString(lab.findObject("SPECTRAL_QUBE",Isis::PvlObject::Traverse)["IMAGE_DURATION"]);

    // Make sure we have THEMIS IR at wavelength 12.57.
    // 12.57um wavelength has the best signal to noise ratio, and 14.88um wavelenght is atmospheric band
    PvlKeyword bandcenter = lab.findGroup("BAND_BIN",Pvl::Traverse)["BAND_BIN_CENTER"];
    bool band12_57 = false;
    bool band14_88 = false;
    int procBand = 0;
    int atmosBand = 0;
    for (int index=0; index<bandcenter.size(); index++) {
      if (bandcenter[index] == "12.57") {
        procBand = index + 1;
        band12_57 = true;
      }
      if (bandcenter[index] == "14.88") {
        atmosBand = index + 1;
        band14_88 = true;
      }
    }

    if (!band12_57) {
      if (reportNoFile) {
        noFileListos << infile.baseName() << " not processed because is missing filter 12.57" << endl;
      }
      QString msg = "Filter 12.57 not found in input file [" + inFileStr + "]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    // Run thm2isis
    QString output = (infile.baseName()) + ".cub";
    QString parameters = "FROM=" + cubes[i].toString() + " TO=" + output;
    ProgramLauncher::RunIsisProgram("thm2isis", parameters);

    //Run spiceinit
    QString input = output;
    parameters = "FROM=" + input + " CKRECON=yes CKPREDICTED=yes CKNADIR=yes";
    ProgramLauncher::RunIsisProgram("spiceinit", parameters);
    if (reportNadirSpc) {
      Pvl spclab(input.toStdString());
      PvlGroup kernels = spclab.findGroup("Kernels", Pvl::Traverse);
      bool isNadir = false;
      for (int j=0; j < kernels["InstrumentPointing"].size(); j++) {
        if (QString::fromStdString(kernels["InstrumentPointing"][j]).toUpper() == "NADIR") {
          isNadir = true;
        }
      }
      if (isNadir) {
        nadirSpcListos << infile.baseName() << endl;
      }
    }

    // Create a temporary pvl and fill with camstats used to test incedence
    FileName tstat1;
    QString tmpcamstats1 = infile.baseName() + "_tmpcamstats1";
    tstat1 = FileName::createTempFile("$TEMPORARY/" + tmpcamstats1 + ".pvl");
    QString tempstat1 = tstat1.expanded();

    parameters = "FROM=" + input + " TO=" + tempstat1 + " linc = 100 sinc = 100";
    ProgramLauncher::RunIsisProgram("camstats", parameters);
    Pvl p1;
    p1.read(tempstat1.toStdString());
    double incAngle =p1.findGroup("IncidenceAngle",Pvl::Traverse)["IncidenceMinimum"];

    // if do not process night and incidence angle > 90, then exit
    if (!processNight && incAngle >= 90) {
      remove (tstat1.expanded().toLatin1().data());
      if (rmHiIncInput) {
        if (reportHiInc) {
          hiIncListos << infile.baseName() << endl;
        }
        if (!pdsSanFile) {
          remove(inFileStr.toLatin1().data());
          remove(input.toLatin1().data());
        }
      }
      if (reportNoFile) {
        noFileListos << infile.baseName() << " not processed because average incidence angle >= 90 and DAY images requested" << endl;
        remove(output.toLatin1().data());
      }
      QString msg = "The average incidence angle of [" + cubes[i].toString() + "] is over 90";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // if process night and incidence angle < 90, then exit
    if (processNight && incAngle < 90) {
      remove (tstat1.expanded().toLatin1().data());
      if (rmHiIncInput) {
        if (reportHiInc) {
          hiIncListos << infile.baseName() << endl;
        }
        if (!pdsSanFile) {
          remove(inFileStr.toLatin1().data());
          remove(input.toLatin1().data());
        }
      }
      if (reportNoFile) {
        noFileListos << infile.baseName() << " not processed because average incidence angle < 90 and NIGHT images requested" << endl;
        remove(output.toLatin1().data());
      }
      QString msg = "The average incidence angle of [" + cubes[i].toString() + "] is over 90";
      throw IException(IException::User,msg,_FILEINFO_);
    }

    // Run thmdriftcor
    // Perform atmospheric correction using filter 10 or 14.88um wavelength.
    // Note we use wavelength 12.57um in geologic mosaics.
    if (band14_88 && incAngle < 90 && !ignoreAtmCorr)  {
      output = (infile.baseName()) + "_driftcorr.cub";
      parameters = "FROM=" + input + "+" + toString(procBand) +
                   " ATM=" + input + "+" + toString(atmosBand) + " TO=" + output;
      ProgramLauncher::RunIsisProgram("thmdriftcor", parameters);
      remove (input.toLatin1().data());
    }
    else {
      output = (infile.baseName()) + "_no_driftcorr.cub";
      parameters = "FROM=" + input + "+" + toString(procBand) + " TO=" + output;
      ProgramLauncher::RunIsisProgram("stretch", parameters);
      remove (input.toLatin1().data());
    }

    // Run cosi, for incidence < 90 (day images)
    // if night is true then skip cosi
    if (incAngle < 90) {
      input = output;
      output = (infile.baseName()) + "_cosi.cub";
      parameters = "FROM=" + input + " TO=" + output;
      ProgramLauncher::RunIsisProgram("cosi", parameters);
      remove (input.toLatin1().data());
    }

    // Run cubenorm
    input = output;
    output = (infile.baseName()) + "cubenorm.cub";
    parameters = "FROM=" + input + " TO=" + output;
    ProgramLauncher::RunIsisProgram("cubenorm", parameters);
    remove (input.toLatin1().data());

    //Run lineeq
    input=output;
    outFile = pathName + infile.baseName() + ".lev1.cub";
    parameters ="FROM=" + input + " TO=" + outFile;
    ProgramLauncher::RunIsisProgram("lineeq", parameters);
    remove (input.toLatin1().data());

    remove (tstat1.expanded().toLatin1().data());

    //************************************
    // Run findgaps
    // Create temporary pvl for gaps
    QString sgap = "no";
    FileName tgaps;
    QString tmpstats1 = infile.baseName() + "_tmpstats1";
    tgaps = FileName::createTempFile("$TEMPORARY/" + tmpstats1 + ".pvl");
    QString tempgaps = tgaps.expanded();

    //parameters = "FROM=" + IString(ui.GetFileName("TO")) +
    //             " TO=" + tempgaps + " CORTOL=.3";
    //ProgramLauncher::RunIsisProgram("findgaps", parameters);
    parameters = "FROM=" + outFile +
                 " TO=" + tempgaps + " APPEND=no";
    ProgramLauncher::RunIsisProgram("stats", parameters);
    Pvl tg;
    tg.read(tempgaps.toStdString());
    QString totalpixels = QString::fromStdString(tg.findGroup("Results",Pvl::Traverse)["TotalPixels"]);
    QString validpixels = QString::fromStdString(tg.findGroup("Results",Pvl::Traverse)["ValidPixels"]);
    //if (tg.hasGroup("Gap")) {
    if (IString(totalpixels).ToInteger() != IString(validpixels).ToInteger()) {
      cout << tg << endl;
      sgap = "yes";
      if (reportImageGap) {
        imageGapListos << infile.baseName() << endl;
      }
    }
    remove (tgaps.expanded().toLatin1().data());

    // Create temporary pvl and fill with camstats
    FileName tstat2;
    QString tmpcamstats2 = infile.baseName() + "_tmpcamstats2";
    tstat2 = FileName::createTempFile("$TEMPORARY/" + tmpcamstats2 + ".pvl");
    QString tempstat2 = tstat2.expanded();

    parameters = "FROM=" + outFile +
                 " TO=" + tempstat2 + " linc = 100 sinc = 100";
    ProgramLauncher::RunIsisProgram("camstats", parameters);

    Pvl p2;
    p2.read(tempstat2.toStdString());
    QString incavg = QString::fromStdString(p2.findGroup("IncidenceAngle",Pvl::Traverse)["IncidenceAverage"]);
    QString  resavg = QString::fromStdString(p2.findGroup("Resolution",Pvl::Traverse)["ResolutionAverage"]);
    QString incmin = QString::fromStdString(p2.findGroup("IncidenceAngle",Pvl::Traverse)["IncidenceMinimum"]);
    QString incmax = QString::fromStdString(p2.findGroup("IncidenceAngle",Pvl::Traverse)["IncidenceMaximum"]);
    remove (tstat2.expanded().toLatin1().data());

    double summing = 1;
    FileName tosum = FileName(outFile);
    Pvl sumlab(tosum.expanded().toStdString());
    PvlGroup InstGrp = sumlab.findGroup("Instrument",Pvl::Traverse);
    if (InstGrp.hasKeyword("SpatialSumming")) {
      summing = InstGrp["SpatialSumming"];
    }

    // do stuff to build the file that will go into the spreadsheet
    // Add statistics to flat file.
    if (logCamStats) {
      logFileos << infile.baseName() << "," << duration << "," << summing << "," <<
        incavg << "," << resavg << "," << incmin << "," << incmax << "," << sgap << "," <<endl;
    }

    // Run footprint stuff if requested for GIS input
    if (footPrintInit) {
      parameters = "FROM=" + outFile +
                   " TO=" + (QString)(infile.baseName()) + ".gml" +
                   " LABEL=" + (QString)(infile.baseName());
      ProgramLauncher::RunIsisProgram("isis2gml", parameters);
    }

    if (rmInput && !pdsSanFile) {
      remove(inFileStr.toLatin1().data());
    }
  }
  catch(IException &e) {
  }
  }
}
