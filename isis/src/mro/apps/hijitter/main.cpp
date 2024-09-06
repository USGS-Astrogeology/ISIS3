/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "Camera.h"
#include "CameraDetectorMap.h"
#include "CameraFactory.h"
#include "CSVReader.h"
#include "FileList.h"
#include "FileName.h"
#include "iTime.h"
#include "IString.h"
#include "LineScanCameraDetectorMap.h"
#include "Pipeline.h"
#include "ProgramLauncher.h"
#include "Progress.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "SpecialPixel.h"
#include "TextFile.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;

// method prototypes
void init(FileList &inList);
FileName masterCcdFileName(FileList &inList, int masterCcdNumber);
void processNoprojFiles(Pipeline &p);
void cropLines(QString inFile, double eTime1, double eTime2, int & line1, int & line2, int & numLines);
void ephemerisTimeFromJitterFile(QString jitterFile, double & eTime1, double & eTime2);


// global variable declarations
vector<QString> g_ccdFiles;
vector<QString> g_tempFiles;
vector<int> g_ccdNumbers;
int g_firstFilter;
int g_numFiles;

// avgOffsets[i][Sample = 0, Line = 1]
// where i is in the table just above
// lineOff's declaration
double g_avgOffsets[11][2];

/*   line offset table
   ------------------------
    i | ccd-ccd | lineoff[i]
      | overlap |
   ------------------------
    0 |   0-1   |  574
    1 |   1-2   | -622
    2 |   2-3   |  620
    3 |   3-4   | -586
    4 |   4-5   |  584
    5 |   5-6   | -600
    6 |   6-7   |  597
    7 |   7-8   | -576
    8 |   8-9   |  607
    9 |  10-11  |  606
   10 |  12-13  |  606      */

const double g_lineOff[11] = {
   574,
  -622,
   620,
  -586,
   584,
  -600,
   597,
  -576,
   607,
   606,
   606
};

void IsisMain() {
  // clear global vectors and intitialize global ints
  g_ccdFiles.clear();
  g_tempFiles.clear();
  g_ccdNumbers.clear();
  g_firstFilter = 0;
  g_numFiles = 0;

  // Get user interface
  UserInterface &ui = Application::GetUserInterface();

  FileList inputList(ui.GetFileName("FROMLIST"));

  int masterFileNum = ui.GetInteger("MASTER");

  // This will set the following global variables: g_ccdFiles, g_ccdNumbers,
  // g_numFiles, g_firstFilter
  init(inputList);

  // The first pipeline will create the match cube file
  //
  // cubeatt FROM="masterCcdFileName.cub" TO="./matchMaster.cub"
  // spiceinit FROM="./matchMaster.cub" ATTACH="NO"
  // spicefit FROM="./matchMaster.cub"
  Pipeline matchfilePipeline("Match File Pipeline: cubeatt >> spicefit >> spiceinit");

  matchfilePipeline.SetInputFile(masterCcdFileName(inputList, masterFileNum));
  matchfilePipeline.SetOutputFile(FileName("$TEMPORARY/matchMaster.cub"));
  g_tempFiles.push_back(FileName("$TEMPORARY/matchMaster.cub").expanded());

  matchfilePipeline.KeepTemporaryFiles(false);

  matchfilePipeline.AddToPipeline("cubeatt");
  matchfilePipeline.Application("cubeatt").SetInputParameter("FROM", true);
  matchfilePipeline.Application("cubeatt").SetOutputParameter("TO", "copy");

  matchfilePipeline.AddToPipeline("spiceinit");
  matchfilePipeline.Application("spiceinit").SetInputParameter("FROM", false);
  matchfilePipeline.Application("spiceinit").AddConstParameter("ATTACH", "NO");

  matchfilePipeline.AddToPipeline("spicefit");
  matchfilePipeline.Application("spicefit").SetInputParameter("FROM", false);

  matchfilePipeline.Run();

  // The main hijitter pipeline
  //
  // FIRST PASS:
  //
  // for each file in file list
  // cubeatt FROM="originalFileName.cub" TO="./noproj.copy.FROM1.cub"
  //
  // for i = 1 to i = numFiles
  // spiceinit FROM="./noproj.copy.FROM1.cub" ATTACH="NO"
  //
  // echo  ./noproj.copy.FROM1.cub ./noproj.copy.FROM2.cub ... ./noproj.copy.FROM(numFiles).cub > ./appjit.lis
  // appjit FROMLIST="./appjit.lis" JITTER="jitterFileName.txt"
  //
  // for i = 1 to i = numFiles
  // noproj FROM="./noproj.copy.FROM1.cub" TO="./noproj.FROM1.cub" MATCH="./matchMaster.cub"
  Pipeline mainPipeline("Main hijitter Pipeline: cubeatt >> spiceinit >> appjit >> noproj");

  mainPipeline.SetInputListFile("FROM");
  mainPipeline.SetOutputFile(FileName("$TEMPORARY/noproj"));

  mainPipeline.KeepTemporaryFiles(false);

  mainPipeline.AddToPipeline("cubeatt");
  mainPipeline.Application("cubeatt").SetInputParameter("FROM", true);
  mainPipeline.Application("cubeatt").SetOutputParameter("TO", "copy");

  mainPipeline.AddToPipeline("spiceinit");
  mainPipeline.Application("spiceinit").SetInputParameter("FROM", false);
  mainPipeline.Application("spiceinit").AddConstParameter("ATTACH", "NO");

  mainPipeline.AddToPipeline("appjit");
  mainPipeline.Application("appjit").SetInputParameter("FROMLIST",
                                            PipelineApplication::LastAppOutputListNoMerge,
                                            false);
  mainPipeline.Application("appjit").AddParameter("JITTER", "JITTER");
  mainPipeline.Application("appjit").AddParameter("DEGREE", "DEGREE");

  if (ui.WasEntered("JITTERCK"))  mainPipeline.AddPause();

  mainPipeline.AddToPipeline("noproj");
  mainPipeline.Application("noproj").SetInputParameter("FROM", true);
  mainPipeline.Application("noproj").AddConstParameter("MATCH",
                                            FileName("$TEMPORARY/matchMaster.cub").expanded());
  mainPipeline.Application("noproj").SetOutputParameter("TO", FileName("$TEMPORARY/noproj").expanded());

  mainPipeline.Prepare();

  QString masterFile = mainPipeline.Application("cubeatt").GetOutputs()[masterFileNum - g_firstFilter];
  mainPipeline.Application("appjit").AddConstParameter("MASTER", masterFile);

  mainPipeline.Run();


  if (ui.WasEntered("JITTERCK"))  {
    // run main hijitter pipeline again with the same parameters
    //
    // for each file in file list
    // cubeatt FROM="originalFileName.cub" TO="./noproj.copy.FROM1.cub"
    //
    // for i = 1 to i = numFiles
    // spiceinit FROM="./noproj.copy.FROM1.cub" ATTACH="NO"
    //
    // echo  ./noproj.copy.FROM1.cub ./noproj.copy.FROM2.cub ... ./noproj.copy.FROM(numFiles).cub > ./appjit.lis
    // appjit FROMLIST="./appjit.lis" JITTER="jitterFileName.txt"
    //
    // for i = 1 to i = numFiles
    // noproj FROM="./noproj.copy.FROM1.cub" TO="./noproj.FROM1.cub" MATCH="./matchMaster.cub"
    mainPipeline.Run();
  }

  // the outputs from this pipeline are temporary files created by cubeatt
  std::vector<QString> outputs = mainPipeline.OriginalBranches();
  for (int i = 0; i < g_numFiles; i++) {
    g_tempFiles.push_back(FileName("$TEMPORARY/noproj."  + outputs[i] + ".cub").expanded());
  }

  // Do some calculations, delete the final outputs from the pipeline
  processNoprojFiles(mainPipeline);

  // run main hijitter pipeline with new parameters:
  //
  // for each file in file list
  // cubeatt FROM="originalFileName.cub" TO="originalFileName.jitter.copy.FROM1.cub"
  // spiceinit FROM="originalFileName.jitter.copy.FROM1.cub" ATTACH="NO"
  //
  // echo  originalFileName.jitter.copy.FROM1.cub originalFileName.jitter.copy.FROM2.cub   ...  > ./appjit.lis
  // appjit FROMLIST="./appjit.lis" JITTER="jitterFileName.txt" MASTER="masterCcdFileName.jitter.copy.FROM1.cub" PITCHRATE="2.95810564663024e-05"
  // YAW="-9.06833084756325e-04"
  //
  // for each file in file list
  // noproj FROM="originalFileName.jitter.copy.FROM1.cub" TO="originalFileName.jitter.cub" MATCH="./matchMaster.cub"
  // editlab FROM="originalFileName.jitter.cub" OPTIONS="SETKEY" GRPNAME="Instrument" KEYWORD="ImageJitterCorrected" VALUE="1"
  mainPipeline.SetOutputListFile("TOLIST");
  mainPipeline.Application("noproj").SetOutputParameter("TO", "jitter");

  mainPipeline.Prepare();

  masterFile = mainPipeline.Application("cubeatt").GetOutputs()[masterFileNum - g_firstFilter];
  mainPipeline.Application("appjit").AddConstParameter("MASTER", masterFile);
  mainPipeline.AddToPipeline("editlab");
  mainPipeline.Application("editlab").SetInputParameter("FROM", true);
  mainPipeline.Application("editlab").AddConstParameter("OPTIONS", "SETKEY");
  mainPipeline.Application("editlab").AddConstParameter("GRPNAME", "Instrument");
  mainPipeline.Application("editlab").AddConstParameter("KEYWORD", "ImageJitterCorrected");
  mainPipeline.Application("editlab").AddConstParameter("VALUE", "1");

  mainPipeline.Run();

  if (ui.WasEntered("JITTERCK")) {
    QString params = "FROM=" + masterFile + " TO=" + ui.GetFileName("JITTERCK");

    try {
      Progress ckwriterProg;
      ckwriterProg.SetText("Running ckwriter");
      ckwriterProg.SetMaximumSteps(1);
      ckwriterProg.CheckStatus();

      // ckwriter FROM=masterCcdFileName.cub TO=jitterCkFileName
      ProgramLauncher::RunIsisProgram("ckwriter", params);
    }
    catch(IException &e) {
      std::string msg = "Creation of the output ck, " +
        ui.GetFileName("JITTERCK") + " failed.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    // run main hijitter pipeline with same parameters as last run:
    //
    // for each file in file list
    // cubeatt FROM="originalFileName.cub" TO="originalFileName.jitter.copy.FROM1.cub"
    // spiceinit FROM="originalFileName.jitter.copy.FROM1.cub" ATTACH="NO"
    //
    // echo  originalFileName.jitter.copy.FROM1.cub originalFileName.jitter.copy.FROM2.cub   ...  > ./appjit.lis
    // appjit FROMLIST="./appjit.lis" JITTER="jitterFileName.txt" MASTER="masterFile.jitter.copy.FROM1.cub" PITCHRATE="2.95810564663024e-05" YAW="-9.06833084756325e-04"
    //
    // for each file in file list
    // noproj FROM="originalFileName.jitter.copy.FROM1.cub" TO="originalFileName.jitter.cub" MATCH="./matchMaster.cub"
    // editlab FROM="originalFileName.jitter.cub" OPTIONS="SETKEY" GRPNAME="Instrument" KEYWORD="ImageJitterCorrected" VALUE="1"
    mainPipeline.Run();
  }

  // Crop the lines using the jitter file if crop is ebabled
  if (ui.GetBoolean("CROP")) {
    double eTime1, eTime2;
    QString jitterFile = ui.GetAsString("JITTER");

    ephemerisTimeFromJitterFile(jitterFile, eTime1, eTime2);
    Progress cropProg;
    cropProg.SetText("Cropping output files");
    cropProg.SetMaximumSteps(g_numFiles + 1);
    cropProg.CheckStatus();

    for (int i = 0; i < g_numFiles; i++) {
      int line1 = 0, line2 = 0;
      int numLines = 0;

      cropLines(mainPipeline.FinalOutput(i), eTime1, eTime2, line1, line2, numLines);
      Pipeline pcrop("Crop Pipeline");
      pcrop.KeepTemporaryFiles(false);

      QString tag = "crop" + toString(i);
      QString inFile(mainPipeline.FinalOutput(i));
      QString outFile = "temp_"+tag+".cub";

      pcrop.SetInputFile(FileName(inFile));
      pcrop.SetOutputFile(FileName(outFile));

      pcrop.AddToPipeline("crop", tag);
      pcrop.Application(tag).SetInputParameter ("FROM",   false);
      pcrop.Application(tag).SetOutputParameter("TO",     "crop");
      pcrop.Application(tag).AddConstParameter ("LINE",   toString(line1));
      pcrop.Application(tag).AddConstParameter ("NLINES", toString(numLines));
      pcrop.Run();

      remove(inFile.toLatin1().data());
      rename(outFile.toLatin1().data(), inFile.toLatin1().data());
      cropProg.CheckStatus();
    }
  }

  for (unsigned int tempFile = 0; tempFile < g_tempFiles.size(); tempFile++) {
    remove(g_tempFiles[tempFile].toLatin1().data());
  }




  g_tempFiles.clear();
  g_ccdFiles.clear();

}

/**
 * This method will validate and return the file name corresponding to
 * the given master ccd number.
 *
 * @param inList Input file list
 * @param masterCcdNumber Number of the master CCD
 *
 * @return FileName Name of the file containing the given master CCD
 */
FileName masterCcdFileName(FileList &inList, int masterCcdNumber) {
  if (g_ccdFiles.empty()) {
    std::string msg = "Global variables are not initialized.";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }
  if (!g_ccdFiles.empty() && g_ccdFiles[masterCcdNumber].isEmpty()) {
    std::string msg = "File containing master CCD [" + toString(masterCcdNumber) + "] is not in the input file list.";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  return g_ccdFiles[masterCcdNumber];
}

/**
 * This method will validate the input file list and set global
 * variables: g_ccdFiles, g_ccdNumbers, g_numFiles,
 * g_firstFilter
 *
 * @param inList Input file list entered by the user
 */
void init(FileList &inList) {

  if (!g_ccdFiles.empty()) {
    std::string msg = "Global variable have already been initialized.";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }

  g_ccdFiles.resize(14);
  g_numFiles = 0;

  int lastCcdNum = -1;
  for (int i = 0; i < inList.size(); i++) {
    try {
      FileName currentFileName(inList[i]);
      Pvl labels(currentFileName.expanded());
      PvlGroup &inst = labels.findGroup("Instrument", Pvl::Traverse);

      QString ccdKeywordValue = QString::fromStdString(inst["CcdId"]);
      int ccdNumber = 0;
      if (ccdKeywordValue.indexOf("RED") == 0) {
        ccdNumber = (int) toInt((QString)ccdKeywordValue.mid(3));
      }
      else if (ccdKeywordValue.indexOf("IR") == 0) {
        ccdNumber = (int) toInt((QString)ccdKeywordValue.mid(2));
      }
      else if (ccdKeywordValue.indexOf("BG") == 0) {
        ccdNumber = (int) toInt((QString)ccdKeywordValue.mid(2));
      }
      else {
        std::string msg = "CcdId value of [" + ccdKeywordValue + "] found in ["
                      + inList[i].toString() + "] not supported. Valid values "
                      "include RED0-RED9, IR10-IR11, BG12-BG13";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      if (ccdNumber < 0 || ccdNumber > 13) {
        std::string msg = "CcdId value of [" + ccdKeywordValue + "] found in ["
                      + inList[i].toString() + "] not supported. Valid values "
                      "include RED0-RED9, IR10-IR11, BG12-BG13";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      if (lastCcdNum == -1) {
        lastCcdNum = ccdNumber;
        g_firstFilter = ccdNumber;
        g_numFiles++;
      }
      else if (lastCcdNum < ccdNumber) {
        lastCcdNum = ccdNumber;
        g_numFiles++;
      }
      else { // last CCD number was larger than this CCD number
        std::string msg = "The input file list must be in ascending order from RED0 to BG13";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      g_ccdFiles[ccdNumber] = inList[i].toString();
      g_ccdNumbers.push_back(ccdNumber);
    }
    catch(IException &e) {
      std::string msg = "File [" + inList[i].toString() + "] is not a valid MRO cube";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }


  // the following is commented out since we will allow nonconsecutive ccds as
  // long as for each ccd in the list, there is at least one overlapping ccd

//  bool foundFirstRedCcd = false;
//  bool foundLastRedCcd = false;
//  QString lastRedCcdFound = "";
//  for (unsigned int i = 0; i < 10; i++) {
//    if (!foundFirstRedCcd && !g_ccdFiles[i].empty()) {
//      foundFirstRedCcd = true;
//    }
//    else if (foundFirstRedCcd && !foundLastRedCcd && g_ccdFiles[i].empty()) {
//      foundLastRedCcd = true;
//      lastRedCcdFound = toString(i-1);
//    }
//    else if (foundFirstRedCcd && foundLastRedCcd && !g_ccdFiles[i].empty()) {
//      std::string msg = "Invalid file list. All red CCDs listed must be consecutive. "
//                    "Input list has files containing [RED" + lastRedCcdFound
//                    + "] and [RED" + toString((int)i) + "], but the CCDs between "
//                    "are not represented in the given file list.";
//      throw IException(IException::User, msg, _FILEINFO_);
//    }
//  }
//  if ( foundFirstRedCcd && !foundLastRedCcd ) {
//    std::string msg = "Invalid file list. Adjacent CCD not in the input file list "
//                  "for [RED" + g_firstFilter + "].";
//    throw IException(IException::User, msg, _FILEINFO_);
//  }


  // Verify that each ccds has at least one overlapping ccd
  bool previousCcdEmpty = false;
  for (unsigned int i = 0; i < g_ccdFiles.size(); i++) {
    if (g_ccdFiles[i].isEmpty()) {
      previousCcdEmpty = true;
    }
    else { // this ccd is present in the list files
      bool nextCcdEmpty = true;
      if (i != 13) {
        nextCcdEmpty = g_ccdFiles[i+1].isEmpty();
      }
      // if no overlapping CCD is in the list, throw an error:
      if ( (i < 9 && previousCcdEmpty && nextCcdEmpty)
           || (i == 9 && previousCcdEmpty)
           || (i == 10 && nextCcdEmpty)
           || (i == 11 && previousCcdEmpty)
           || (i == 12 && nextCcdEmpty)
           || (i == 13 && previousCcdEmpty) ) {

        QString color = "";
        if (i < 10) color = "RED";
        else if (i < 12) color = "IR";
        else color = "BG";

        QString overlappingCcds = "";
        if (i == 0 || i == 10 || i == 12) {
          overlappingCcds = "CCD [" + color + toString((int) (i+1) ) + "] is";
        }
        else if (i == 9 || i == 11 || i == 13) {
          overlappingCcds = "CCD [" + color + toString((int) (i-1)) + "] is";
        }
        else {
          overlappingCcds = "CCDs [RED" + toString((int) (i-1))
                         + "] and [RED" + toString((int) (i+1)) + "] are";
        }
        std::string msg = "Invalid file list. A file containing the CCD [" + color
                      + toString((int) i) + "] is in the input file list, "
                      "but adjacent " + overlappingCcds + " not in the list.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      previousCcdEmpty = false;
    }
  }
  return;
}





/**
 *
 *  @param p
 *
 */
void processNoprojFiles(Pipeline &p) {
  UserInterface &ui = Application::GetUserInterface();

  // This will be decremented on error, it's easier this way

  // numOffsets is an unnecesary variable. It should equal offsetIndeces.size()
  // int numOffsets = g_numFiles - 1;
  vector<int> offsetIndices;

  Progress hijitregProg;
  hijitregProg.SetText("Running hijitreg");
  hijitregProg.SetMaximumSteps(1);
  hijitregProg.CheckStatus();

  std::vector<QString> outputs = p.OriginalBranches();

  for (int i = 0; i < g_numFiles - 1; i++) {
    // If this CCD and the consecutive CCD are both in the input list,
    // use the current cubes in the pipeline to create an output flat
    // file for this overlap from the hijireg program. This will
    // calculate avg offsets for the overlaps.
    //
    // Note that the consecutive CCD pairs (9,10) and (11,12) do not
    // overlap since 0-9 are red, 10-11 are near-infrared, and 12-13
    // are blue-green. For this reason, we don't run hijitreg for the
    // CCDs 9, 11, and 13.
    if (g_ccdNumbers[i] != 9 && g_ccdNumbers[i] != 11 && g_ccdNumbers[i] != 13
        && g_ccdNumbers[i+1] == g_ccdNumbers[i] + 1) {

      QString tempDir = FileName("$TEMPORARY").expanded();
      QString flatFileName = tempDir + "/first" + toString(g_ccdNumbers[i])
                            + "-" + toString(g_ccdNumbers[i+1]) + ".flat";
      QString params = "";
      params += "FROM=" + tempDir + "/noproj." + outputs[i] + ".cub";
      params += " MATCH=" + tempDir + "/noproj." + outputs[i + 1] + ".cub";
      params += " REGDEF=" + ui.GetFileName("REGDEF");
      params += " FLAT=" + flatFileName;

      try {
        // hijitreg FROM=$TEMPORARY/noproj.FROM1.cub MATCH=
        ProgramLauncher::RunIsisProgram("hijitreg", params);
      }
      catch(IException &e) {
        // numOffsets--;
        continue; // to next file in for loop
      }

      // Read offsets

      TextFile flatFile(flatFileName);
      g_tempFiles.push_back(flatFileName);

      // set the offset index value (0-10)
      // Note: we know that g_ccdNumbers is not 9, 11, 13 since these cases are
      // already excluded above
      int offsetIndex = 0;
      if (g_ccdNumbers[i] < 9) {
        offsetIndex = g_ccdNumbers[i];
      }
      if (g_ccdNumbers[i] == 10) {
        offsetIndex = 9;
      }
      if (g_ccdNumbers[i] == 12) {
        offsetIndex = 10;
      }

      g_avgOffsets[offsetIndex][0] = Isis::Null;
      g_avgOffsets[offsetIndex][1] = Isis::Null;

      QString line;
      try {
        while( flatFile.GetLine(line, false) &&
               (g_avgOffsets[offsetIndex][0] == Isis::Null
                || g_avgOffsets[offsetIndex][1] == Isis::Null) ) {
          line = QString(line).simplified();
          int pos = line.indexOf("Average Sample Offset: ");

          if (pos != -1) {
            // cut off text before our number (start pos + strlen + 1)
            line = line.mid(pos + strlen("Average Sample Offset: "));

            // cut off text after our number
            line = line.mid(0, line.indexOf(" "));

            g_avgOffsets[offsetIndex][0] = (double)toDouble((QString)line);
            pos = -1;
          }

          pos = line.indexOf("Average Line Offset: ");

          if (pos != -1) {
            // cut off text before our number (start pos + strlen + 1)
            line = line.mid(pos + strlen("Average Line Offset: "));

            // cut off text after our number
            line = line.mid(0, line.indexOf(" "));

            g_avgOffsets[offsetIndex][1] = (double)toDouble((QString)line);
            pos = -1;
          }
        }
      }
      catch(IException &e) {
        //std::string msg = "Unable to find average sample/line offsets in hijitreg results for CCDs [" + toString(i) + "-" + toString(i+1) + "]";
        //throw iException::Message(e, iException::Programmer, msg, _FILEINFO_);
        // numOffsets--;
        continue; // to next file in for loop
      }
      // if the loop doesn't throw an error, add the offsetIndex to the list
      offsetIndices.push_back(offsetIndex);
    }
  }

  if (offsetIndices.size() == 0) {
    std::string msg = "Unable to calculate average sample/line offsets from hijitreg results";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }

  Cube cube(FileName("$TEMPORARY/noproj." + outputs[0] + ".cub").expanded(), "r");
  Camera *cam = CameraFactory::Create(cube);

  double lineRate = cam->DetectorMap()->LineRate();

  delete cam;
  cam = NULL;

  // pitchRate (radians/sec) = 0.000001 *
  // sum(averageLineOffset[i]/(lineOff[i] * lineRate) / 9.,
  // where i = (0,8), averageLineOffset[i] is the lineOffset from first0-1.flat,
  // lineOff[i] is listed in table below.
  double pitchRate = 0.0;

  // yaw (radians) = sum( arctan(averageSampleOffset[i])/lineOff[i]) / 9.,
  // where i = (0,8), the angles are small enough that the arctan is optional,
  // averageSampleOffset[i] is the sampleOffset from first0-1.flat,
  double yaw = 0.0;

  double numOffsets = (double) offsetIndices.size();
  for (unsigned int i = 0; i < offsetIndices.size(); i++) {
    int index = offsetIndices[i];
    if (IsSpecial(g_avgOffsets[index][0]) || IsSpecial(g_avgOffsets[index][1])) continue;

    pitchRate += 0.000001 * (g_avgOffsets[index][1] / (g_lineOff[index] * lineRate)) / numOffsets;
    yaw += atan(g_avgOffsets[index][0] / g_lineOff[index]) / numOffsets;
  }

  p.Application("appjit").AddConstParameter("PITCHRATE", toString(pitchRate));
  p.Application("appjit").AddConstParameter("YAW", toString(yaw));
  return;
}

/**
 * Get the start and end ephemeris time from the jitter file
 *
 * @author Sharmila Prasad (12/20/2011)
 *
 * @param jitterFile - jitter file
 * @param eTime1 - Start time
 * @param eTime2 - End time
 */
void ephemerisTimeFromJitterFile(const QString jitterFile, double & eTime1, double & eTime2) {
  CSVReader jitter(jitterFile, true, 0, ' ', false, true);
  int iRows = jitter.rows();
  CSVReader::CSVAxis csvArr;

  eTime1=0, eTime2=0;

  double temp=0;
  for (int j=0; j<iRows; j++) {
    csvArr = jitter.getRow(j);
    int iArrSize = csvArr.dim();
    for (int i=0; i<iArrSize; i++) {
      csvArr[i] = csvArr[i].trimmed().remove(QRegExp("(^,*|,*$)"));
      temp = toDouble(QString(csvArr[i]));
      if (!i && temp == 0) {
        break;
      }
      // Ephemeris Time
      if (i==2) {
        if (eTime1 == 0) {
          eTime1 = temp;
        }
        else {
          eTime2 = temp;
        }
      }
    }
  }
}

/**
 * cropLines for an image given the start and end ephemeris times
 *
 * @author Sharmila Prasad (12/21/2011)
 *
 * @param inFile   - Input image file
 * @param eTime1   - Ephemeris start time
 * @param eTime2   - Ephemeris end time
 * @param line1    - Calculated start line for cropping
 * @param line2    - Calculated end line for cropping
 * @param numLines - Number of lines in the image to be cropped
 */
void cropLines(QString inFile, double eTime1, double eTime2, int & line1, int & line2, int & numLines) {

  Cube *inCube = new Cube;
  inCube->open(inFile);

  Camera *cam = inCube->camera();

  iTime etTime = cam->Spice::cacheStartTime();
  double etStart = etTime.Et();
  double lineRate = ((LineScanCameraDetectorMap*) cam->DetectorMap())->LineRate();

  int imgLines = inCube->lineCount();
  line1 = int ((eTime1 - etStart) / lineRate + 0.5 );
  line2 = int ((eTime2 - etStart) / lineRate + 0.5 );

  if (line2 > imgLines)
    numLines = imgLines - line1 + 1;
  else
    numLines = line2 - line1 + 1;

  inCube->close();
  delete(inCube);
}
