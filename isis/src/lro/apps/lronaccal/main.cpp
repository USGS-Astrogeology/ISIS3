/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"
#include "ProcessByLine.h"
#include "SpecialPixel.h"
#include "Message.h"
#include "Camera.h"
#include "iTime.h"
#include "IException.h"
#include "TextFile.h"
#include "Brick.h"
#include "Table.h"
#include "PvlGroup.h"
#include "Statistics.h"
#include <fstream>
#include <QTextStream>
#include <QDir>
#include <QRegExp>
#include <QString>
#include <vector>

using namespace std;
using namespace Isis;

// Working functions and parameters
void ResetGlobals();
void CopyCubeIntoVector(QString &fileString, vector<double> &data);
void ReadTextDataFile(QString &fileString, vector<double> &data);
void ReadTextDataFile(QString &fileString, vector<vector<double> > &data);
void Calibrate(Buffer &in, Buffer &out);
void RemoveMaskedOffset(Buffer &line);
void CorrectDark(Buffer &in);
void CorrectNonlinearity(Buffer &in);
void CorrectFlatfield(Buffer &in);
void RadiometricCalibration(Buffer &in);
void GetNearestDarkFile(QString fileString, QString &file);
void GetNearestDarkFilePair(QString &fileString, QString &file0, QString &file1);
void GetCalibrationDirectory(QString calibrationType, QString &calibrationDirectory);
void GetWeightedDarkAverages();
bool AllowedSpecialPixelType(double pixelValue);

#define LINE_SIZE 5064
#define MAXNONLIN 600
#define SOLAR_RADIUS 695500
#define KM_PER_AU 149597871
#define MASKED_PIXEL_VALUES 8

/**
 * DarkFileInfo comparison object.
 *
 * Used for sorting DarkFileInfo objects. Sort first by difference from NAC time
 *
 */
struct DarkFileComparison {
  int nacTime;

  DarkFileComparison(int nacTime)
  {
    this->nacTime = nacTime;
  }

  // sort dark files by distance from NAC time
  bool operator() ( int A,  int B) {
    if (abs(nacTime - A) < abs(nacTime - B))
      return true;
    return false;
  }
};

double g_radianceLeft, g_radianceRight, g_iofLeft, g_iofRight, g_imgTime;
double g_exposure; // Exposure duration
double g_solarDistance; // average distance in [AU]

bool g_summed, g_masked, g_maskedLeftOnly, g_dark, g_nonlinear, g_flatfield, g_radiometric, g_iof, g_isLeftNac;
bool g_nearestDark, g_nearestDarkPair, g_customDark;
vector<int> g_maskedPixelsLeft, g_maskedPixelsRight;
vector<double> g_avgDarkLineCube0, g_avgDarkLineCube1, g_linearOffsetLine, g_flatfieldLine, g_darkTimes, g_weightedDarkTimeAvgs;
vector<vector<double> > g_linearityCoefficients;
Buffer *g_darkCube0, *g_darkCube1;

/**
 * This is the main lronaccal method. Lronaccal is used to calibrate LRO images
 * *
 * @internal
 *   @history 2020-01-06 Victor Silva - Added option for base calibration directory
 *   @history 2020-07-19 Victor Silva - Updated dark calibration to use dark file option
 *                                      custom, nearest dark, or nearest dark pair.
 *   @history 2021-01-09 Victor Silva - Added code to check for exp_code = zero and if so
 *                                      then use only exp_code_zero dark files for dark calibration
 *
 */
void IsisMain() {
  ResetGlobals();
  // We will be processing by line
  ProcessByLine p;

  // Setup the input and make sure it is a ctx file
  UserInterface &ui = Application::GetUserInterface();

  g_masked = ui.GetBoolean("MASKED");
  g_dark = ui.GetBoolean("DARK");
  g_nonlinear = ui.GetBoolean("NONLINEARITY");
  g_flatfield = ui.GetBoolean("FLATFIELD");
  g_radiometric = ui.GetBoolean("RADIOMETRIC");
  g_iof = (ui.GetString("RADIOMETRICTYPE") == "IOF");

  Isis::Pvl lab(ui.GetFileName("FROM"));
  Isis::PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);

  // Check if it is a NAC image
  QString instId = (QString) inst["InstrumentId"];
  instId = instId.toUpper();
  if(instId != "NACL" && instId != "NACR") {
    QString msg = "This is not a NAC image.  lrocnaccal requires a NAC image.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // And check if it has already run through calibration
  if(lab.findObject("IsisCube").hasGroup("Radiometry")) {
    QString msg = "This image has already been calibrated";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  if(lab.findObject("IsisCube").hasGroup("AlphaCube")) {
    QString msg = "This application can not be run on any image that has been geometrically transformed (i.e. scaled, rotated, sheared, or reflected) or cropped.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  if(instId == "NACL")
    g_isLeftNac = true;
  else
    g_isLeftNac = false;

  if((int) inst["SpatialSumming"] == 1)
    g_summed = false;
  else
    g_summed = true;

  g_exposure = inst["LineExposureDuration"];

  Cube *iCube = p.SetInputCube("FROM", OneBand);

  // If there is any pixel in the image with a DN > 1000
  //  then the "left" masked pixels are likely wiped out and useless
  if(iCube->statistics()->Maximum() > 1000)
    g_maskedLeftOnly = true;

  QString flatFile, offsetFile, coefficientFile;

  if(g_masked) {
    QString maskedFile = ui.GetAsString("MASKEDFILE");

    if(maskedFile.toLower() == "default" || maskedFile.length() == 0){
      GetCalibrationDirectory("", maskedFile);
      maskedFile = maskedFile + instId + "_MaskedPixels.????.pvl";
    }

    FileName maskedFileName(maskedFile);
    if(maskedFileName.isVersioned())
      maskedFileName = maskedFileName.highestVersion();
    if(!maskedFileName.fileExists()) {
      QString msg = maskedFile + " does not exist.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    Pvl maskedPvl(maskedFileName.expanded());
    PvlKeyword maskedPixels;
    int cutoff;
    if(g_summed) {
      maskedPixels = maskedPvl["Summed"];
      cutoff = LINE_SIZE / 4;
    }
    else {
      maskedPixels = maskedPvl["FullResolution"];
      cutoff = LINE_SIZE / 2;
    }

    for(int i = 0; i < maskedPixels.size(); i++)
      if((g_isLeftNac && toInt(maskedPixels[i]) < cutoff) || (!g_isLeftNac && toInt(maskedPixels[i]) > cutoff))
        g_maskedPixelsLeft.push_back(toInt(maskedPixels[i]));
      else
        g_maskedPixelsRight.push_back(toInt(maskedPixels[i]));
  }

  vector <QString> darkFiles;

  if(g_dark) {
    QString darkFileType = ui.GetString("DARKFILETYPE");
    darkFileType = darkFileType.toUpper();
		if (darkFileType == "CUSTOM") {
			g_customDark = true;
      ui.GetAsString("DARKFILE", darkFiles);
		}
    else if (darkFileType == "PAIR" || darkFileType == "")
      g_nearestDarkPair = true;
    else if (darkFileType == "NEAREST"){
      g_nearestDark = true;
    }
    else {
      QString msg = "Error: Dark File Type selection failed.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    //Options are NEAREST, PAIR, and CUSTOM
    if(g_customDark){
      if(darkFiles.size() == 1 && darkFiles[0] != "") {
        CopyCubeIntoVector(darkFiles[0], g_avgDarkLineCube0);
      }
      else {
        QString msg = "Custom dark file not provided. Please provide file or choose another option.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    else {
      QString darkFile;
      g_imgTime = iTime(inst["StartTime"][0]).Et();
      GetCalibrationDirectory("nac_darks", darkFile);
      darkFile = darkFile + instId + "_AverageDarks_*T";
      
      if(g_summed)
        darkFile += "_Summed";
      // use exp0 dark files if cube's exp_code=0
      Isis::PvlGroup &pvl_archive_group = lab.findGroup("Archive", Pvl::Traverse);
      if((int) pvl_archive_group["LineExposureCode"] == 0)
        darkFile += "_exp0";

      darkFile += ".????.cub";

      if(g_nearestDark){
        darkFiles.resize(1);
        GetNearestDarkFile(darkFile, darkFiles[0]);
      }
      else {
        darkFiles.resize(2);
        GetNearestDarkFilePair(darkFile, darkFiles[0], darkFiles[1]);
        //get weigted time avgs
        if(g_darkTimes.size() == 2)
          GetWeightedDarkAverages();
      }
    }
  }

  if(g_nonlinear) {
    offsetFile = ui.GetAsString("OFFSETFILE");

    if(offsetFile.toLower() == "default" || offsetFile.length() == 0) {
      GetCalibrationDirectory("", offsetFile);
      offsetFile = offsetFile + instId + "_LinearizationOffsets";
      if(g_summed)
        offsetFile += "_Summed";
      offsetFile += ".????.cub";
    }
    CopyCubeIntoVector(offsetFile, g_linearOffsetLine);
    coefficientFile = ui.GetAsString("NONLINEARITYFILE");
    if(coefficientFile.toLower() == "default" || coefficientFile.length() == 0) {
      GetCalibrationDirectory("", coefficientFile);
      coefficientFile = coefficientFile + instId + "_LinearizationCoefficients.????.txt";
    }
    ReadTextDataFile(coefficientFile, g_linearityCoefficients);
  }

  if(g_flatfield) {
    flatFile = ui.GetAsString("FLATFIELDFILE");

    if(flatFile.toLower() == "default" || flatFile.length() == 0) {
      GetCalibrationDirectory("", flatFile);
      flatFile = flatFile + instId + "_Flatfield";
      if(g_summed)
        flatFile += "_Summed";
      flatFile += ".????.cub";
    }
    CopyCubeIntoVector(flatFile, g_flatfieldLine);
  }

  if(g_radiometric) {
    QString radFile = ui.GetAsString("RADIOMETRICFILE");

    if(radFile.toLower() == "default" || radFile.length() == 0){
      GetCalibrationDirectory("", radFile);
      radFile = radFile + "NAC_RadiometricResponsivity.????.pvl";
    }

    FileName radFileName(radFile);
    if(radFileName.isVersioned())
      radFileName = radFileName.highestVersion();
    if(!radFileName.fileExists()) {
      QString msg = radFile + " does not exist.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    Pvl radPvl(radFileName.expanded());

    if(g_iof) {
      iTime startTime((QString) inst["StartTime"]);

      try {
        Camera *cam;
        cam = iCube->camera();
        cam->setTime(startTime);
        g_solarDistance = cam->sunToBodyDist() / KM_PER_AU;

      }
      catch(IException &e) {
        // Failed to instantiate a camera, try furnishing kernels directly
        try {

          double etStart = startTime.Et();
          // Get the distance between the Moon and the Sun at the given time in
          // Astronomical Units (AU)
          QString bspKernel1 = p.MissionData("lro", "/kernels/tspk/moon_pa_de421_1900-2050.bpc", false);
          QString bspKernel2 = p.MissionData("lro", "/kernels/tspk/de421.bsp", false);
          furnsh_c(bspKernel1.toLatin1().data());
          furnsh_c(bspKernel2.toLatin1().data());
          QString pckKernel1 = p.MissionData("base", "/kernels/pck/pck?????.tpc", true);
          QString pckKernel2 = p.MissionData("lro", "/kernels/pck/moon_080317.tf", false);
          QString pckKernel3 = p.MissionData("lro", "/kernels/pck/moon_assoc_me.tf", false);
          furnsh_c(pckKernel1.toLatin1().data());
          furnsh_c(pckKernel2.toLatin1().data());
          furnsh_c(pckKernel3.toLatin1().data());
          double sunpos[6], lt;
          spkezr_c("sun", etStart, "MOON_ME", "LT+S", "MOON", sunpos, &lt);
          g_solarDistance = vnorm_c(sunpos) / KM_PER_AU;
          unload_c(bspKernel1.toLatin1().data());
          unload_c(bspKernel2.toLatin1().data());
          unload_c(pckKernel1.toLatin1().data());
          unload_c(pckKernel2.toLatin1().data());
          unload_c(pckKernel3.toLatin1().data());
        }
        catch(IException &e) {
          QString msg = "Unable to find the necessary SPICE kernels for converting to IOF";
          throw IException(e, IException::User, msg, _FILEINFO_);
        }
      }
      g_iofLeft = radPvl["IOF_LEFT"];
      g_iofRight = radPvl["IOF_RIGHT"];
    }
    else {
      g_radianceLeft = radPvl["Radiance_LEFT"];
      g_radianceRight = radPvl["Radiance_RIGHT"];
    }
  }

  // Setup the output cube
  Cube *ocube = p.SetOutputCube("TO");

  // Start the line-by-line calibration sequence
  p.StartProcess(Calibrate);

  PvlGroup calgrp("Radiometry");
  if(g_masked) {
    PvlKeyword darkColumns("DarkColumns");
    for(unsigned int i = 0; i < g_maskedPixelsLeft.size(); i++)
      darkColumns += toString(g_maskedPixelsLeft[i]);
    for(unsigned int i = 0; i < g_maskedPixelsRight.size(); i++)
      darkColumns += toString(g_maskedPixelsRight[i]);
    calgrp += darkColumns;
  }
  if(g_dark){
    PvlKeyword darks("DarkFiles");
    darks.addValue(darkFiles[0]);
    if(g_nearestDark)
      calgrp += PvlKeyword("DarkFileType", "NearestDarkFile");
    else if (g_nearestDarkPair){
      calgrp += PvlKeyword("DarkFileType", "NearestDarkFilePair");
      darks.addValue(darkFiles[1]);
    }
    else
      calgrp += PvlKeyword("DarkFileType", "CustomDarkFile");

    calgrp += darks;
  }
  if(g_nonlinear) {
    calgrp += PvlKeyword("NonlinearOffset", offsetFile);
    calgrp += PvlKeyword("LinearizationCoefficients", coefficientFile);
  }
  if(g_flatfield)
    calgrp += PvlKeyword("FlatFile", flatFile);
  if(g_radiometric) {
    if(g_iof) {
      calgrp += PvlKeyword("RadiometricType", "IOF");
      if(g_isLeftNac)
        calgrp += PvlKeyword("ResponsivityValue", toString(g_iofLeft));
      else
        calgrp += PvlKeyword("ResponsivityValue", toString(g_iofRight));
    }
    else {
      calgrp += PvlKeyword("RadiometricType", "AbsoluteRadiance");
      if(g_isLeftNac)
        calgrp += PvlKeyword("ResponsivityValue", toString(g_radianceLeft));
      else
        calgrp += PvlKeyword("ResponsivityValue", toString(g_radianceRight));
    }
    calgrp += PvlKeyword("SolarDistance", toString(g_solarDistance));
  }
  ocube->putGroup(calgrp);
  p.EndProcess();
}

/**
 * This method resets global variables
 *
 */
void ResetGlobals() {
  g_exposure = 1.0; // Exposure duration
  g_solarDistance = 1.01; // average distance in [AU]

  g_maskedPixelsLeft.clear();
  g_maskedPixelsRight.clear();

  g_radianceLeft = 1.0;
  g_radianceRight = 1.0;
  g_iofLeft = 1.0;
  g_iofRight = 1.0;

  g_summed = true;
  g_masked = true;
  g_dark = true;
  g_nonlinear = true;
  g_flatfield = true;
  g_radiometric = true;
  g_iof = true;
  g_isLeftNac = true;
  g_maskedLeftOnly = false;
  g_nearestDarkPair = false;
  g_nearestDark = false;
  g_customDark = false;
  g_avgDarkLineCube0.clear();
  g_avgDarkLineCube1.clear();
  g_linearOffsetLine.clear();
  g_darkTimes.clear();
  g_weightedDarkTimeAvgs.clear();
  g_flatfieldLine.clear();
  g_linearityCoefficients.clear();
  g_imgTime = 0.0;
}

/**
 * This method processes buffer by line to calibrate
 *
 * @param in Buffer to hold 1 line of cube data
 * @param out Buffer to hold 1 line of cube data
 *
 */
void Calibrate(Buffer &in, Buffer &out) {
  for(int i = 0; i < in.size(); i++)
    out[i] = in[i];

  if(g_masked)
    RemoveMaskedOffset(out);

  if(g_dark)
    CorrectDark(out);

  if(g_nonlinear)
    CorrectNonlinearity(out);

  if(g_flatfield)
    CorrectFlatfield(out);

  if(g_radiometric)
    RadiometricCalibration(out);
}

/**
 * Read text data file - overloaded method
 *
 * @param fileString QString
 * @param data vector of double
 *
 */
void ReadTextDataFile(QString &fileString, vector<double> &data) {
  FileName filename(fileString);
  if(filename.isVersioned())
    filename = filename.highestVersion();
  if(!filename.fileExists()) {
    QString msg = fileString + " does not exist.";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  TextFile file(filename.expanded());
  QString lineString;
  unsigned int line = 0;
  while(file.GetLine(lineString)) {
    data.push_back(toDouble(lineString.split(QRegExp("[ ,;]")).first()));
    line++;
  }
  fileString = filename.expanded();
}

/**
 * Read the text data file - overloaded method
 *
 * @param fileString QString
 * @param data multi-dimensional vector of double
 *
 */
void ReadTextDataFile(QString &fileString, vector<vector<double> > &data) {
  FileName filename(fileString);
  if(filename.isVersioned())
    filename = filename.highestVersion();
  if(!filename.fileExists()) {
    QString msg = fileString + " does not exist.";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  TextFile file(filename.expanded());
  QString lineString;
  while(file.GetLine(lineString)) {
    vector<double> line;
    lineString = lineString.simplified().remove(QRegExp("^[ ,]*")).trimmed();

    QStringList lineTokens = lineString.split(QRegExp("[ ,]"), QString::SkipEmptyParts);
    foreach (QString value, lineTokens) {
      line.push_back(toDouble(value));
    }

    data.push_back(line);
  }

  fileString = filename.expanded();
}

/**
 * Remove masked offset
 *
 * @param in Buffer
 *
 */
void RemoveMaskedOffset(Buffer &in) {
  int numMasked = MASKED_PIXEL_VALUES;
  if(g_summed)
    numMasked /= 2;

  vector<Statistics> statsLeft(numMasked, Statistics());
  vector<Statistics> statsRight(numMasked, Statistics());

  vector<int> leftRef(numMasked, 0);
  vector<int> rightRef(numMasked, 0);

  for(unsigned int i = 0; i < g_maskedPixelsLeft.size(); i++) {
    statsLeft[g_maskedPixelsLeft[i] % numMasked].AddData(&in[g_maskedPixelsLeft[i]], 1);
    leftRef[g_maskedPixelsLeft[i] % numMasked] += g_maskedPixelsLeft[i];
  }

  for(unsigned int i = 0; i < g_maskedPixelsRight.size(); i++) {
    statsRight[g_maskedPixelsRight[i] % numMasked].AddData(&in[g_maskedPixelsRight[i]], 1);
    rightRef[g_maskedPixelsRight[i] % numMasked] += g_maskedPixelsRight[i];
  }

  // left/rightRef is the center (average) of all the masked pixels in the set
  for(int i = 0; i < numMasked; i++) {
    leftRef[i] /= statsLeft[i].TotalPixels();
    rightRef[i] /= statsRight[i].TotalPixels();
  }

  if(g_maskedLeftOnly) {
    for(int i = 0; i < in.size(); i++) {
      in[i] -= statsLeft[i % numMasked].Average();
    }
  }
  else {
    // If we are using both sides, we interpolate between them

    for(int i = 0; i < in.size(); i++) {
      in[i] -= (statsLeft[i % numMasked].Average() * (rightRef[i % numMasked] - i) + statsRight[i % numMasked].Average()
                * (i - leftRef[i % numMasked])) / (rightRef[i % numMasked] - leftRef[i % numMasked]);
    }
  }
}

/**
 * Dark Correction - will find 2 nearest dark files to perform
 *                   dark correction of the pixel being processed
 *
 * @param in Buffer
 *
 */
void CorrectDark(Buffer &in) {
  for (int i = 0; i < in.size(); i++) {
    if(g_nearestDarkPair &&
       (!IsSpecial(in[i]) || AllowedSpecialPixelType(in[i])) &&
       (!IsSpecial(g_avgDarkLineCube0[i]) || AllowedSpecialPixelType(g_avgDarkLineCube0[i])) &&
       (!IsSpecial(g_avgDarkLineCube1[i]) || AllowedSpecialPixelType(g_avgDarkLineCube1[i])) &&
       (!IsSpecial(in[i]) || AllowedSpecialPixelType(in[i])) ){
      double w0 = g_weightedDarkTimeAvgs[0];
      double w1 = g_weightedDarkTimeAvgs[1];
      double pixelDarkAvg = (g_avgDarkLineCube0[i]*w0)+(g_avgDarkLineCube1[i]*w1);

      in[i] -= pixelDarkAvg;

    } else if
      ((!IsSpecial(g_avgDarkLineCube0[i]) || AllowedSpecialPixelType(g_avgDarkLineCube0[i])) &&
       (!IsSpecial(in[i]) || AllowedSpecialPixelType(in[i])) ) {

      in[i] -= g_avgDarkLineCube0[i];

    }
    else {

      in[i] = Isis::Null;

    }
  }
}

/**
 * Correct non-linearity of the pixel being processed
 *
 *
 * @param in Buffer
 */
void CorrectNonlinearity(Buffer &in) {
  for(int i = 0; i < in.size(); i++) {
    if(!IsSpecial(in[i])) {
      in[i] += g_linearOffsetLine[i];

      if(in[i] < MAXNONLIN) {
        if(g_summed)
          in[i] -= (1.0 / (g_linearityCoefficients[2* i ][0] * pow(g_linearityCoefficients[2* i ][1], in[i])
                           + g_linearityCoefficients[2* i ][2]) + 1.0 / (g_linearityCoefficients[2* i + 1][0] * pow(
                                 g_linearityCoefficients[2* i + 1][1], in[i]) + g_linearityCoefficients[2* i + 1][2])) / 2;
        else
          in[i] -= 1.0 / (g_linearityCoefficients[i][0] * pow(g_linearityCoefficients[i][1], in[i])
                          + g_linearityCoefficients[i][2]);
      }
    }
    else
      in[i] = Isis::Null;
  }
}

void CorrectFlatfield(Buffer &in) {
  for(int i = 0; i < in.size(); i++) {
    if(!IsSpecial(in[i]) && g_flatfieldLine[i] > 0)
      in[i] /= g_flatfieldLine[i];
    else
      in[i] = Isis::Null;
  }
}

/**
 * Radiometric Calibration of the pixel being processed
 *
 *
 * @param in Buffer
 */
void RadiometricCalibration(Buffer &in) {
  for(int i = 0; i < in.size(); i++) {
    if(!IsSpecial(in[i])) {
      in[i] /= g_exposure;
      if(g_iof) {
        if(g_isLeftNac)
          in[i] = in[i] * pow(g_solarDistance, 2) / g_iofLeft;
        else
          in[i] = in[i] * pow(g_solarDistance, 2) / g_iofRight;
      }
      else {
        if(g_isLeftNac)
          in[i] = in[i] / g_radianceLeft;
        else
          in[i] = in[i] / g_radianceRight;
      }
    }
    else
      in[i] = Isis::Null;
  }
}

/**
 * This method returns an QString containing the path of an
 * LRO calibration directory
 *
 * @param calibrationType
 * @param calibrationDirectory Path of the calibration directory
 *
 * @internal
 *   @history 2020-01-06 Victor Silva - Added option for base calibration directory
 */
void GetCalibrationDirectory(QString calibrationType, QString &calibrationDirectory) {
  PvlGroup &dataDir = Preference::Preferences().findGroup("DataDirectory");
  QString missionDir = (QString) dataDir["LRO"];
  if(calibrationType != "")
    calibrationType += "/";

  calibrationDirectory = missionDir + "/calibration/" + calibrationType;
}

/**
 * Finds the best dark files for NAC calibration.
 *
 * GetNearestDarkFile will get the dark file with the
 * closest time (before or after) to the image time
 * to be used for calibration.
 *
 * @param fileString String pattern defining dark files to search
 * @param file0 Filename of dark file 1
 */
void GetNearestDarkFile(QString fileString, QString &file) {
  FileName filename(fileString);
  QString basename = FileName(filename.baseName()).baseName(); // We do it twice to remove the ".????.cub"
  // create a regular expression to capture time from filenames
  QString regexPattern(basename);
  regexPattern.replace("*", "([0-9\\.-]*)");
  QRegExp regex(regexPattern);
  // create a filter for the QDir to only load files matching our name
  QString filter(basename);
  filter.append(".*");
  // get a list of dark files that match our basename
  QDir dir( filename.path(), filter );
  vector<int> matchedDarkTimes;
  matchedDarkTimes.reserve(dir.count());
  // Loop through all files in the dir that match our basename and extract time
  for (unsigned int i=0; i < dir.count(); i++) {
    // match against our regular expression
    int pos = regex.indexIn(dir[i]);
    if (pos == -1) {
      continue; // filename did not match basename regex (time contain non-digit)
    }
    // Get a list of regex matches. Item 0 should be the full QString, item 1 is time.
    QStringList texts = regex.capturedTexts();
    if (texts.size() < 1) {
      continue; // could not find time
    }
    // extract time from regex texts
    bool timeOK;
    int fileTime = texts[1].toInt(&timeOK);
    if (!timeOK) {
      continue; // time was not a valid numeric value
    }
    matchedDarkTimes.push_back(fileTime);
  }
  // sort the files by distance from nac time
  DarkFileComparison darkComp((int)g_imgTime);
  sort(matchedDarkTimes.begin(), matchedDarkTimes.end(), darkComp);
  int darkTime = matchedDarkTimes[0];
  int fileTimeIndex = fileString.indexOf("*T");
  file = fileString;
  file.replace(fileTimeIndex, 1, toString(darkTime));
  CopyCubeIntoVector(file, g_avgDarkLineCube0);
}

/**
 * Finds the best dark files for NAC calibration.
 *
 * GetNearestDarkFilePair will get the average between the two darks files
 * that the image lies between (time-wise).

 * If this pair is not found, the nearest dark file will be used
 * for calibration.
 *
 * @param fileString String pattern defining dark files to search (ie. lro/calibration/nac_darks/NAC*_AverageDarks_*T_.????.cub)
 * @param file0 Filename of dark file 1
 * @param file1 Filename of dark file 2
 */
void GetNearestDarkFilePair(QString &fileString, QString &file0, QString &file1) {
  FileName filename(fileString);
  QString basename = FileName(filename.baseName()).baseName(); // We do it twice to remove the ".????.cub"
  // create a regular expression to capture time from filenames
  QString regexPattern(basename);
  regexPattern.replace("*", "([0-9\\.-]*)");
  QRegExp regex(regexPattern);
  // create a filter for the QDir to only load files matching our name
  QString filter(basename);
  filter.append(".*");
  // get a list of dark files that match our basename
  QDir dir( filename.path(), filter );
  vector<int> matchedDarkTimes;
  matchedDarkTimes.reserve(dir.count());
  if (dir.count() < 1){
    QString msg = "Could not find any dark file of type " + filter + ".\n";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  // Loop through all files in the dir that match our basename and extract time
  for (unsigned int i=0; i < dir.count(); i++) {
    // match against our regular expression
    int pos = regex.indexIn(dir[i]);
    if (pos == -1) {
      continue; // filename did not match basename regex (time contain non-digit)
    }
    // Get a list of regex matches. Item 0 should be the full QString, item 1
    // is time.
    QStringList texts = regex.capturedTexts();
    if (texts.size() < 1) {
      continue; // could not find time
    }
    // extract time from regex texts
    bool timeOK;
    int fileTime = texts[1].toInt(&timeOK);
    if (!timeOK) {
      continue; // time was not a valid numeric value
    }
    matchedDarkTimes.push_back(fileTime);
  }
  // sort the files by distance from nac time
  DarkFileComparison darkComp((int)g_imgTime);
  sort(matchedDarkTimes.begin(), matchedDarkTimes.end(), darkComp);

  int fileTimeIndex = fileString.indexOf("*T");
  int t0 = 0;
  int t1 = 0;
  //Let's find the first time before the image
  for(size_t i = 0; i < matchedDarkTimes.size(); i++){
    if(matchedDarkTimes[i] <= (int)g_imgTime){
      t0 = matchedDarkTimes[i];
      break;
    }
  }
  //Let's find the second time
  for (size_t i = 0; i < matchedDarkTimes.size(); i++) {
    if (matchedDarkTimes[i] >= (int)g_imgTime) {
      t1 = matchedDarkTimes[i];
      break;
    }
  }
  if((t0 && t1) && (t0!=t1)){
    int timeDayDiff =  abs(t1 -t0)/86400.0;
    
	//check time range between darks is within 45 day window
  if (timeDayDiff < 0  || timeDayDiff > 45) {
      QString msg = "Could not find a pair of dark files within 45 day range that includes the image [" + basename + "]. Check to make sure your set of dark files is complete.\n";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    else {
      file0 = fileString;
      file0.replace(fileTimeIndex, 1, toString(t0));
      CopyCubeIntoVector(file0, g_avgDarkLineCube0);
      g_darkTimes.push_back(t0);
      file1 = fileString;
      file1.replace(fileTimeIndex, 1, toString(t1));
      CopyCubeIntoVector(file1, g_avgDarkLineCube1);
      g_darkTimes.push_back(t1);
    }
  }
  else {
    g_nearestDark = true;
    g_nearestDarkPair = false;
    int darkTime = matchedDarkTimes[0];
    file0 = fileString;
    file0.replace(fileTimeIndex, 1, toString(darkTime));
    CopyCubeIntoVector(file0, g_avgDarkLineCube0);
    g_darkTimes.push_back(darkTime);
  }
}

/**
 * This method copies cube into vector
 * LRO calibration directory
 *
 * @param fileString QString pointer
 * @param data vector of double
 *
 */
void CopyCubeIntoVector(QString &fileString, vector<double> &data) {
  Cube cube;
  FileName filename(fileString);
  if(filename.isVersioned())
    filename = filename.highestVersion();
  if(!filename.fileExists()) {
    QString msg = fileString + " does not exist.";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  cube.open(filename.expanded());
  Brick brick(cube.sampleCount(), cube.lineCount(), cube.bandCount(), cube.pixelType());
  brick.SetBasePosition(1, 1, 1);
  cube.read(brick);
  data.clear();
  for(int i = 0; i < cube.sampleCount(); i++)
    data.push_back(brick[i]);

  fileString = filename.expanded();

  if(data.empty()){
    QString msg = "Copy from + " + fileString + " into vector failed.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

}
/**
 * Allow special pixel types
 *
 * @param pixelValue double
 *
 * @return bool
 *
 */
bool AllowedSpecialPixelType(double pixelValue) {
  bool result = false;
  result = result || IsHisPixel(pixelValue);
  result = result || IsLisPixel(pixelValue);
  result = result || IsHrsPixel(pixelValue);
  result = result || IsLrsPixel(pixelValue);
  return result;
}

/**
 * Get weighted time average for calculating pixel dark
 * average
 *
 * @param w0 double Weighted Time Average for dark file
 * @param w1 double Weighted time Average for dark file
 *
 */
void GetWeightedDarkAverages() {

 int iTime = (int)g_imgTime;
 int t0 = 0;
 int t1 = 0;

 if (!g_darkTimes.empty()){
  if (g_darkTimes.size() == 2){
    t0 = g_darkTimes[0];
    t1 = g_darkTimes[1];
    double weight0 =
     (( t1!=iTime ) * ( (t1 > iTime ) * ( t1 - iTime) ))
     / (((( t1!=iTime ) * ( (t1 > iTime ) * ( t1 - iTime) )) +
      (( t0!=iTime ) * ( (t0 < iTime ) * ( iTime - t0) )) ) * 1.0);

    double weight1 = (( t0!=iTime ) * ( (t0 < iTime ) * ( iTime - t0) ))
     / (((( t1!=iTime ) * ( (t1 > iTime ) * ( t1 - iTime) )) +
      (( t0!=iTime ) * ( (t0 < iTime ) * ( iTime - t0) )) ) * 1.0);

    g_weightedDarkTimeAvgs.clear();
    g_weightedDarkTimeAvgs.push_back(weight0);
    g_weightedDarkTimeAvgs.push_back(weight1);
    }
  }
}
