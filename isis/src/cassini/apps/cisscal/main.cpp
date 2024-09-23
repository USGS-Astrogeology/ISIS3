/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"
#include <algorithm>// sort, unique
#include <cmath>
#include <cstdlib>
#include <iterator>//unique
#include <iostream> // unique
#include <QString>
#include <sstream>
#include <vector>
#include "Brick.h"
#include "Buffer.h"
#include "Camera.h"
#include "CisscalFile.h"
#include "CissLabels.h"
#include "Cube.h"
#include "DarkCurrent.h"
#include "FileName.h"
#include "LeastSquares.h"
#include "NumericalApproximation.h"
#include "Pixel.h"
#include "PolynomialUnivariate.h"
#include "ProcessByLine.h"
#include "Preference.h"
#include "PvlGroup.h"
#include "SpecialPixel.h"
#include "Stretch.h"
#include "Table.h"
#include "UserInterface.h"
#include "IException.h"
#include "IString.h"
#include <QDir>

using namespace Isis;
using namespace std;

// Working functions and parameters
namespace gbl {
  //global methods
  void BitweightCorrect(Buffer &in);
  void Calibrate(vector<Buffer *> &in, vector<Buffer *> &out);
  void ComputeBias();
  void CopyInput(Buffer &in);
  void CreateBitweightStretch(FileName bitweightTable);
  FileName FindBitweightFile();
  vector<double> OverclockFit();
  void Linearize();
  void FindDustRingParameters();
  FileName FindFlatFile();
  void FindCorrectionFactors();
  void FindSensitivityCorrection();
  void DNtoElectrons();
  void FindShutterOffset();
  void DivideByAreaPixel();
  void FindEfficiencyFactor(QString fluxunits);
  QString GetCalibrationDirectory(QString calibrationType);

  //global variables
  CissLabels *cissLab;
  Cube *incube;
  PvlGroup calgrp;
  Stretch stretch;
  int numberOfOverclocks;
  vector <double> bias;
  vector <vector <double> > bitweightCorrected;
  //dark subtraction variables
  vector <vector <double> > dark_DN;
  // flatfield variables
  FileName dustFile;
  FileName dustFile2;
  FileName mottleFile;
  double strengthFactor;
  bool dustCorrection, mottleCorrection, flatCorrection;
  //DN to Flux variables
  double trueGain;
  bool divideByExposure;
  Brick *offset;
  double solidAngle;
  double opticsArea;
  double sumFactor;
  double efficiencyFactor;
  //correction factor variables
  double correctionFactor;
  bool sensCorrection;
  double sensVsTimeCorr;
}

void IsisMain() {
  // Initialize Globals
  UserInterface &ui = Application::GetUserInterface();
  gbl::cissLab = new CissLabels(ui.GetCubeName("FROM"));
  gbl::incube = NULL;
  gbl::stretch.ClearPairs();
  gbl::numberOfOverclocks = 0;
  gbl::bias.clear();
  gbl::dustFile = "";
  gbl::mottleFile = "";
  gbl::strengthFactor = 1.0;
  gbl::dustCorrection = false;
  gbl::mottleCorrection = false;
  gbl::flatCorrection = false;
  gbl::trueGain = 1.0;
  gbl::divideByExposure = false;
  gbl::offset = NULL; // initialize pointer to null or 0?
  gbl::solidAngle = 1.0;
  gbl::opticsArea = 1.0;
  gbl::sumFactor  = 1.0;
  gbl::efficiencyFactor = 1.0;
  gbl::correctionFactor = 1.0;
  gbl::sensCorrection = false;
  gbl::sensVsTimeCorr = 1.0;

  // Set up our ProcessByLine objects
  // we will take 2 passes through the input cube
  ProcessByLine firstpass;
  ProcessByLine secondpass;
  // for the first pass, use the input cube.
  gbl::incube = firstpass.SetInputCube("FROM"); // CopyInput() or BitweightCorrect() parameter in
  // for the second pass, set input cube to "FROM" due to requirements of
  // ProcessByLine that there be at least 1 input buffer before setting the
  // output buffer, however this cube is not used in the Calibrate method,
  // instead the bitweightCorrected vector is used as the initial values before
  // the rest of the calibration steps are performed.
  gbl::incube = secondpass.SetInputCube("FROM"); // Calibrate() parameter in[0]
  // we need to set output cube at the beginning of the program to check right
  // away for CubeCustomization IsisPreference and throw an error, if necessary.
  Cube *outcube = secondpass.SetOutputCube("TO"); // Calibrate() parameter out[0]

  // resize 2dimensional vectors
  gbl::bitweightCorrected.resize(gbl::incube->sampleCount());
  gbl::dark_DN.resize(gbl::incube->sampleCount());
  for(unsigned int i = 0; i < gbl::bitweightCorrected.size(); i++) {
    gbl::bitweightCorrected[i].resize(gbl::incube->lineCount());
    gbl::dark_DN[i].resize(gbl::incube->lineCount());
  }

  // Add the radiometry group
  gbl::calgrp.setName("Radiometry");
  gbl::calgrp += PvlKeyword("CisscalVersion", "3.9.1");

  // The first ProcessByLine pass will either compute bitweight values or copy input values

  // BITWEIGHT CORRECTION
  gbl::calgrp += PvlKeyword("BitweightCorrectionPerformed", "Yes");
  gbl::calgrp.findKeyword("BitweightCorrectionPerformed").addComment("Bitweight Correction Parameters");
  // Bitweight correction is not applied to Lossy-compressed or Table-converted images
  if(gbl::cissLab->CompressionType() == "Lossy") {
    gbl::calgrp.findKeyword("BitweightCorrectionPerformed").setValue("No: Lossy compressed");
    gbl::calgrp += PvlKeyword("BitweightFile", "Not applicable: No bitweight correction");
    firstpass.Progress()->SetText("Lossy compressed: skip bitweight correction as insignificant.\nCopying input image...");
    firstpass.StartProcess(gbl::CopyInput);
    firstpass.EndProcess();
  }
  else if(gbl::cissLab->DataConversionType() == "Table") {
    gbl::calgrp.findKeyword("BitweightCorrectionPerformed").setValue("No: Table converted");
    gbl::calgrp += PvlKeyword("BitweightFile", "Not applicable: No bitweight correction");
    firstpass.Progress()->SetText("Table converted: skip bitweight correction as insignificant.\nCopying input image...");
    firstpass.StartProcess(gbl::CopyInput);
    firstpass.EndProcess();
  }
  // Skip bitweight correction for GainState==0, there is no data for this case,
  // see ground calibration report 5.1.9 Uneven Bit Weighting
  else if(gbl::cissLab->GainState() == 0) {
    gbl::calgrp.findKeyword("BitweightCorrectionPerformed").setValue("No: No bitweight calibration file for GainState 0.");
    gbl::calgrp += PvlKeyword("BitweightFile", "Not applicable: No bitweight correction.");
    firstpass.Progress()->SetText("No bitweight calibration file for GainState 0: skip bitweight correction.\nCopying input image...");
    firstpass.StartProcess(gbl::CopyInput);
    firstpass.EndProcess();
  }
  else { // Bitweight correction
    FileName bitweightFile = gbl::FindBitweightFile();
    if(!bitweightFile.fileExists()) { // bitweight file not found, stop calibration
      // Remove the output cube since it will be empty at this point
      outcube->close(true);
      throw IException(IException::Io,
                       "Unable to calibrate image. BitweightFile ***"
                       + bitweightFile.expanded() + "*** not found.", _FILEINFO_);
    }
    else {
      gbl::calgrp += PvlKeyword("BitweightFile", bitweightFile.original());
      gbl::CreateBitweightStretch(bitweightFile);
      firstpass.Progress()->SetText("Computing bitweight correction...");
      firstpass.StartProcess(gbl::BitweightCorrect);
      firstpass.EndProcess();
    }
  }// THIS ENDS FIRST PROCESS

  // Compute global values needed for the rest of the calibration steps

  //Subtract bias (debias)
  gbl::ComputeBias();

  //Dark current subtraction
  try {
    DarkCurrent dark(*gbl::cissLab);
    gbl::dark_DN = dark.ComputeDarkDN();
    gbl::calgrp += PvlKeyword("DarkSubtractionPerformed", "Yes");
    gbl::calgrp.findKeyword("DarkSubtractionPerformed").addComment("Dark Current Subtraction Parameters");
    gbl::calgrp += PvlKeyword("DarkParameterFile", dark.DarkParameterFile().original());
    if(gbl::cissLab->NarrowAngle()) {
      gbl::calgrp += PvlKeyword("BiasDistortionTable", dark.BiasDistortionTable().original());
    }
    else {
      gbl::calgrp += PvlKeyword("BiasDistortionTable", "ISSWA: No bias distortion table used");
    }
  }
  catch(IException &e) { // cannot perform dark current, stop calibration
    e.print();
    // Remove the output cube since it will be empty at this point
    outcube->close(true);
    throw IException(e, IException::Unknown,
                     "Unable to calibrate image. Dark current calculations failed.",
                     _FILEINFO_);
  }

  //Linearity Correction
  gbl::Linearize();

  //Dust Ring Correction
  gbl::FindDustRingParameters();
  //Flat Field Correction
  FileName flatFile = gbl::FindFlatFile();

  //DN to Flux Correction
  gbl::DNtoElectrons();
  gbl::FindShutterOffset();
  gbl::DivideByAreaPixel();
  gbl::FindEfficiencyFactor(ui.GetString("UNITS"));

  //Correction Factor
  // Set the remaining necessary input cube files for second pass
  CubeAttributeInput att;
  gbl::FindCorrectionFactors();
  gbl::FindSensitivityCorrection();
  if(gbl::flatCorrection) { // Calibrate() parameter in[1]
    secondpass.SetInputCube(QString::fromStdString(flatFile.original()), att);
  }
  if(gbl::dustCorrection) { // Calibrate() parameter in[2]
    secondpass.SetInputCube(QString::fromStdString(gbl::dustFile.original()), att);
  }
  if(gbl::mottleCorrection) { // Calibrate() parameter in[3]
    secondpass.SetInputCube(QString::fromStdString(gbl::mottleFile.original()), att);
  }
  if (gbl::dustCorrection && !gbl::cissLab->AntibloomingOn()) {  // Calibrate() parameter in[4]
    secondpass.SetInputCube(QString::fromStdString(gbl::dustFile2.original()), att);
  }
  // this pass will call the Calibrate method
  secondpass.Progress()->SetText("Calibrating image...");
  outcube->putGroup(gbl::calgrp);
  secondpass.StartProcess(gbl::Calibrate);
  secondpass.EndProcess();
  gbl::calgrp.clear();

  return;

} //END MAIN

/**
 * This calibration method runs through all calibration steps.
 * It takes a vector of input buffers that contains the
 * input image and, if needed, the flat field image, the
 * dustring correction image, and the mottle correction image.
 * The vector of output buffers will only contain one element:
 * the output image.
 *
 * @param in Vector of pointers to input buffers for the second
 *           process in IsisMain()
 * @param out Vector of pointers to output buffer.
 * @internal
 *   @history 2008-11-05 Jeannie Walldren - Original version
 *   @history 2009-05-27 Jeannie Walldren - Added polarization
 *            factor correction.  Updated ConstOffset value per
 *            new idl cisscal version, 3.6.
 *   @history 2017-06-08 Cole Neubauer - Added dustfile2 correction.
 *            Updated ConstOffset value to match new idl cisscal version, 3.8.
 *   @history 2019-08-14 Kaitlyn Lee - Removed jupiter correction. Added
 *            check for ShutterStateId, and if it is Disabled,
 *            do not subtract an offset from the exposure time.
 *            Added Sensitivity vs Time Correction after
 *            correction factors.
 *            Matches cisscal version 3.9.1.
 */
void gbl::Calibrate(vector<Buffer *> &in, vector<Buffer *> &out) {
  Buffer *flat = 0;
  Buffer *dustCorr = 0;
  Buffer *mottleCorr = 0;
  Buffer *dustCorr2 = 0;
  if(gbl::flatCorrection) {
    flat = in[1];
  }
  if(gbl::dustCorrection) {
    dustCorr = in[2];
    dustCorr2= in[4];
    if(gbl::mottleCorrection) {
      mottleCorr = in[3];
    }
  }
  Buffer &outLine = *out[0];
  //get line index of output
  int lineIndex = outLine.Line() - 1;
  for(unsigned int sampIndex = 0; sampIndex < gbl::bitweightCorrected.size(); sampIndex++) {
    if(IsValidPixel(gbl::bitweightCorrected[sampIndex][lineIndex])) {

      ///////////////////////////////////////////////////////////////////////////////////////////////////////////
      // STEP 1) set output to bitweight corrected values
      outLine[sampIndex] = bitweightCorrected[sampIndex][lineIndex];

      ///////////////////////////////////////////////////////////////////////////////////////////////////////////
      // STEP 2)  remove bias (debias)
      if(gbl::numberOfOverclocks) {
        outLine[sampIndex] = outLine[sampIndex] - gbl::bias[lineIndex];
      }
      else {
        outLine[sampIndex] = outLine[sampIndex] - gbl::bias[0];
      }

      ///////////////////////////////////////////////////////////////////////////////////////////////////////////
      // idl cisscal step "REMOVE 2-HZ NOISE" skipped
      // -- this is more of a filter than calibration

      ///////////////////////////////////////////////////////////////////////////////////////////////////////////
      // STEP 3) remove dark current
      outLine[sampIndex] = outLine[sampIndex] - gbl::dark_DN[sampIndex][lineIndex];

      ///////////////////////////////////////////////////////////////////////////////////////////////////////////
      // idl cisscal step "ANTI-BLOOMING CORRECTION" skipped
      // -- this is more of a filter than calibration

      ///////////////////////////////////////////////////////////////////////////////////////////////////////////
      // STEP 4)  linearity correction (linearize)
      if(outLine[sampIndex] < 0) {
        outLine[sampIndex] = (outLine[sampIndex]) * (gbl::stretch.Map(0));
      }
      else {
        outLine[sampIndex] = (outLine[sampIndex]) * (gbl::stretch.Map((int) outLine[sampIndex]));
      }
      ///////////////////////////////////////////////////////////////////////////////////////////////////////////
      // STEP 5)  flatfield correction
      // 5a1: dust ring correction
      if(gbl::dustCorrection) {
        outLine[sampIndex] = (outLine[sampIndex]) * ((*dustCorr)[sampIndex]);
        if (!gbl::cissLab->AntibloomingOn()) {
            outLine[sampIndex] = (outLine[sampIndex] * ((*dustCorr2)[sampIndex]));
        }
        // 5a2: mottle correction
        if(gbl::mottleCorrection) {
          outLine[sampIndex] = (outLine[sampIndex]) * (1 - ((gbl::strengthFactor) * ((*mottleCorr)[sampIndex]) / 1000.0));
        }
      }
      // 5b: divide by flats
      if(gbl::flatCorrection) {
        if(Isis::IsSpecial((*flat)[sampIndex])){
          (*flat)[sampIndex] = 1;
        }
        outLine[sampIndex] = outLine[sampIndex] / ((*flat)[sampIndex]);
      }
      ///////////////////////////////////////////////////////////////////////////////////////////////////////////
      // STEP 6)  convert DN to flux
      // 6a DN to Electrons
      outLine[sampIndex] = outLine[sampIndex] * gbl::trueGain;
      // 6b Divide By Exposure Time
      //  JPL confirm that these values must be subtracted thus:
      if(gbl::divideByExposure) {
        double exposureTime = gbl::cissLab->ExposureDuration() - (*gbl::offset)[gbl::offset->Index(sampIndex+1, 1, 1)];
        // IDL documentation:
        //  Need to develop way to subtract constant offset discussed in
        //  section 4.3 of Ground Calibration Report. Right now just use 1 ms
        //  for all cases.
        //  CORRECTION: New analysis of Vega images points to a value more like
        //  2.85 ms (correct to within about 0.05 ms for the NAC, less certain
        //  for the WAC. Use this until better data available. (12/1/2005 - BDK)
        //  UPDATE: Used azimuthal ring scans to pin down WAC to around 1.8 ms.
        //  (1/18/2006 - BDK)
        //  UPDATE #2: S58 SPICA data gives WAC shutter offset of about 2.86 ms.
        //  (9/21/2010 - BDK)
        //  UPDATE #3: Rhea SATCAL obs from rev 163 give NAC offset of 2.74 and
        //  WAC offset of 2.67 ms, much less noisy results than using stars.
        //  (1/31/2013 - BDK)
        //  UPDATE #4: From S100 Vega (WAC) and 77/78 Tau (NAC): NAC offset of
        //  2.51, WAC offset of 2.63, but analysis far more noisy than for Rhea
        //  so keep previous values. (8/4/2017 - BDK)

        double ConstOffset;
        if(gbl::cissLab->ShutterStateId() != "Disabled") {
          if(gbl::cissLab->InstrumentId() == "ISSNA") {
            ConstOffset = 2.75;
          }
          else {
            ConstOffset = 2.67;
          }
          exposureTime = exposureTime - ConstOffset;
        }
        outLine[sampIndex] = outLine[sampIndex] * 1000 / exposureTime;  // 1000 to scale ms to seconds
      }
      // 6c Divide By Area Pixel
      outLine[sampIndex] = outLine[sampIndex] * gbl::sumFactor / (gbl::solidAngle * gbl::opticsArea);
      // 6d Divide By Efficiency
      outLine[sampIndex] = outLine[sampIndex] / gbl::efficiencyFactor;
      ///////////////////////////////////////////////////////////////////////////////////////////////////////////
      // STEP 7)  correction factors
      // 7a Correction Factors
      outLine[sampIndex] = outLine[sampIndex] / gbl::correctionFactor;
      if (outLine[sampIndex] < 0) {
        outLine[sampIndex] = 0;
      }
      // 7b Sensitivity vs Time Correction
      if(gbl::sensCorrection) {
        outLine[sampIndex] = outLine[sampIndex] * gbl::sensVsTimeCorr;
      }
      ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    }
    else if (IsSpecial(bitweightCorrected[sampIndex][lineIndex])) {
      outLine[sampIndex] = bitweightCorrected[sampIndex][lineIndex];
    }
    else {
      outLine[sampIndex] = bitweightCorrected[sampIndex][lineIndex];
      if (outLine[sampIndex] < 0) {
        outLine[sampIndex] = 0;
      }
    }
  }
  return;
}


//=====4 Bitweight Methods=======================================================================//
// These methods are modelled after IDL CISSCAL's cassimg_bitweightcorrect.pro
/**
 * The purpose of this method is to copy the input to output if
 * no bitweight correction occurs.
 *
 * @param in Input buffer for the first process in IsisMain()
 * @internal
 *   @history 2008-11-05 Jeannie Walldren - Original version
 *   @history 2008-12-22 Jeannie Walldren - Fixed bug in calls
 *            to resize() method.
 *   @history 2009-01-26 Jeannie Walldren - Moved resizing of
 *            2-dimensional vectors to main method
 */
void gbl::CopyInput(Buffer &in) {
  // find line index
  int lineIndex = in.Line() - 1;
  for(int sampIndex = 0; sampIndex < in.size(); sampIndex++) {
    // assign input value to image vector
    gbl::bitweightCorrected[sampIndex][lineIndex] = in[sampIndex];
  }
  return;
}

/**
 * This method is modelled after IDL CISSCAL's
 * cassimg_bitweightcorrect.pro.  The purpose is to correct the
 * image for uneven bit weights.  This is done using one of
 * several tables developed from the ground calibration
 * exercises table depends on InstrumentId, GainModeId, and
 * OpticsTemperature.
 *
 * @param in Input buffer for the first process in IsisMain()
 * @internal
 *   @history 2008-11-05 Jeannie Walldren - Original version
 *   @history 2009-01-26 Jeannie Walldren - Moved resizing of
 *            2-dimensional vectors to main method
 */
void gbl::BitweightCorrect(Buffer &in) {
  // find line index
  int lineIndex = in.Line() - 1;
  // loop through samples of this line
  for(int sampIndex = 0; sampIndex < in.size(); sampIndex++) {
    // map bitweight corrected image output values to buffer input values
    if(IsValidPixel(in[sampIndex])) {
      gbl::bitweightCorrected[sampIndex][lineIndex] = gbl::stretch.Map(in[sampIndex]);
    }
    else {
      gbl::bitweightCorrected[sampIndex][lineIndex] = in[sampIndex];
    }
  }
  return;
}


/**
 * This method sets up the strech for the conversion from file.
 * It is used by the BitweightCorrect() method to map LUT
 * values.
 *
 * @param bitweightTable Name of the bitweight table for this
 *                       image.
 *
 * @internal
 *   @history 2008-11-05 Jeannie Walldren - Original version
 */
void gbl::CreateBitweightStretch(FileName bitweightTable) {
  CisscalFile *stretchPairs = new CisscalFile(QString::fromStdString(bitweightTable.original()));
  // Create the stretch pairs
  double stretch1 = 0, stretch2;
  gbl::stretch.ClearPairs();
  for(int i = 0; i < stretchPairs->LineCount(); i++) {
    QString line;
    stretchPairs->GetLine(line);
    line = line.simplified().trimmed();

    QStringList tokens = line.split(QRegExp("[, ]"), Qt::SkipEmptyParts);
    foreach (QString token, tokens) {
      stretch2 = token.toDouble();
      gbl::stretch.AddPair(stretch1, stretch2);
      stretch1 = stretch1 + 1.0;
    }
  }
  stretchPairs->Close();
  return;
}

/**
 * This method is modelled after IDL CISSCAL's
 * cassimg_bitweightcorrect.pro.  The purpose is to find the
 * look up table file name for this image.
 *
 *  The table to be used depends on:
 *    Camera    NAC or WAC
 *    GainState  1, 2 or 3 <=> GainModeId 95, 29, or 12 Optics temp.  -10, +5 or
 *     +25
 *
 * @return <B>FileName</B> Name of the bitweight file
 *
 * @internal
 *   @history 2008-11-05 Jeannie Walldren - Original version
 */
FileName gbl::FindBitweightFile() {
  // Get the directory where the CISS bitweight directory is
  QString bitweightName;

  if(gbl::cissLab->NarrowAngle()) {
    bitweightName += "nac";
  }
  else {
    bitweightName += "wac";
  }
  QString gainState(QString::number(gbl::cissLab->GainState()));
  bitweightName = bitweightName + "g" + gainState;

  if(gbl::cissLab->FrontOpticsTemp() < -5.0) {
    bitweightName += "m10_bwt.tab";
  }
  else if(gbl::cissLab->FrontOpticsTemp() < 25.0) {
    bitweightName += "p5_bwt.tab";
  }
  else {
    bitweightName += "p25_bwt.tab";
  }
  return FileName(gbl::GetCalibrationDirectory("bitweight").toStdString() + bitweightName.toStdString());
}
//=====End Bitweight Methods=====================================================================//

//=====2 Debias Methods============================================================================//
//These methods are modelled after IDL CISSCAL's cassimg_debias.pro

/**
 * This method is modelled after IDL CISSCAL's
 *  cassimg_debias.pro.  The purpose is to compute the bias
 *  (zero-exposure DN level of CCD chip) to be subtracted in the
 *  Calibrate() method.
 *  There are two ways to do this
 *      1. (DEFAULT) using overclocked pixel array taken out of
 *         binary line prefix
 *      2. subtract BiasMeanStrip value found in labels
 *
 *  @internal
 *   @history 2008-11-05 Jeannie Walldren - Original version
 *   @history 2009-05-27 Jeannie Walldren - Commented out table
 *            if-statement as done idl cisscal version 3.6.
 *   @history 2019-08-14 Kaitlyn Lee - Added check for a corrupt
 *            bias strip mean to match cisscal version 3.9.1.
 */
void gbl::ComputeBias() {
  gbl::calgrp += PvlKeyword("BiasSubtractionPerformed", "Yes");
  gbl::calgrp.findKeyword("BiasSubtractionPerformed").addComment("Bias Subtraction Parameters");

  QString fsw(gbl::cissLab->FlightSoftwareVersion());
  double flightSoftwareVersion;

  if(fsw == "Unknown") {
    flightSoftwareVersion = 0.0;// cassimg_readlabels.pro sets this to 1.3, we treat this as 1.2???
  }
  else {
    flightSoftwareVersion = fsw.toDouble();
  }
  //  check overclocked pixels exist
  if(gbl::cissLab->CompressionType() != "Lossy") {
    if(flightSoftwareVersion < 1.3) { // (1.2=CAS-ISS2 or Unknown=0.0=CAS-ISS)
      gbl::numberOfOverclocks = 1;
    }
    else { //if(1.3=CAS-ISS3 or 1.4=CAS-ISS4)
      gbl::numberOfOverclocks = 2;
    }
    gbl::calgrp += PvlKeyword("BiasSubtractionMethod", "Overclock fit");
  }
  // otherwise overclocked pixels are invalid and must use bias strip mean where possible
  else { // overclocks array is corrupt for lossy images (see cassimg_readvic.pro)
#if 0
    // 2009-04-27 Jeannie Walldren
    //   following code comment out in new idl cisscal version, 3.6:
    if(gbl::cissLab->DataConversionType() == "Table") { // Lossy + Table = no debias
      gbl::calgrp.findKeyword("BiasSubtractionPerformed").setValue("No: Table converted and Lossy compressed");
      gbl::calgrp += PvlKeyword("BiasSubtractionMethod", "Not applicable: No bias subtraction");
      gbl::calgrp += PvlKeyword("NumberOfOverclocks", "Not applicable: No bias subtraction");
      gbl::bias.resize(1);
      gbl::bias[0] = 0.0;
      return;
    }
#endif

    // according to SIS if 1.2 or 1.3 and Lossy, ignore bias strip mean - invalid data
    if(flightSoftwareVersion <= 1.3) { // Lossy + 1.2 or 1.3 = no debias
      gbl::calgrp.findKeyword("BiasSubtractionPerformed").setValue("No: Lossy compressed on CAS-ISS2 or CAS-ISS3");
      gbl::calgrp += PvlKeyword("BiasSubtractionMethod", "Not applicable: No bias subtraction");
      gbl::calgrp += PvlKeyword("NumberOfOverclocks", "Not applicable: No bias subtraction");
      gbl::bias.resize(1);
      gbl::bias[0] = 0.0;
      return;
    }
    gbl::calgrp += PvlKeyword("BiasSubtractionMethod", "Bias strip mean");
    gbl::numberOfOverclocks = 0;//overclocks array is corrupt for lossy images
  }
  //Choose bias subtraction method
  if(gbl::numberOfOverclocks) {        // use overclocked pixels as default
    gbl::bias = gbl::OverclockFit();
  }
  else {  // use BiasStripMean in image label if can't use overclock

    // Corrupt bias strip mean
    if(gbl::cissLab->BiasStripMean() < 0.0) {
      gbl::calgrp.findKeyword("BiasSubtractionPerformed").setValue("No: Bias strip mean value corrupt.");
      gbl::calgrp += PvlKeyword("BiasSubtractionMethod", "Not applicable: No bias subtraction");
      gbl::calgrp += PvlKeyword("NumberOfOverclocks", "Not applicable: No bias subtraction");
      gbl::bias.resize(1);
      gbl::bias[0] = 0.0;
      return;
    }

    gbl::bias.resize(1);
    gbl::bias[0] = gbl::cissLab->BiasStripMean();
  }
  gbl::calgrp += PvlKeyword("NumberOfOverclocks", Isis::toString(gbl::numberOfOverclocks));
  return;
}

/**
 * This method is modelled after IDL CISSCAL's
 * cassimg_define.pro method, CassImg::OverclockAvg().  This
 * access function computes line-averaged overclocked pixel
 * values and returns a linear fit of these values.
 *
 * @return <B>vector <double> </B> Results of linear fit
 *
 *  @internal
 *   @history 2008-11-05 Jeannie Walldren - Original version
 */
vector<double> gbl::OverclockFit() {
  vector< vector <double> > overclocks;
  //  Read overclocked info from table saved during ciss2isis
  //  table should have 3 columns:
  //     -col 3 is the "average" of the overclocked pixels
  //       -if there are 2 overclocks, columns 1 and 2 contain them
  //       -otherwise column 1 is all null and we use column 2
  Table overClkTable = gbl::incube->readTable("ISS Prefix Pixels");
  for(int i = 0; i < overClkTable.Records(); i++) {
    overclocks.push_back(overClkTable[i]["OverclockPixels"]);
  }

  vector<double> overclockfit, eqn, avg;
  PolynomialUnivariate poly(1);
  LeastSquares lsq(poly);
  //get overclocked averages
  for(unsigned int i = 0; i < overclocks.size(); i++) {
    avg.push_back(overclocks[i][2]);
  }
  if(avg[0] > 2 * avg[1]) {
    avg[0] = avg[1];
  }
  // initialize eqn
  eqn.push_back(0.0);
  for(unsigned int i = 0; i < avg.size(); i++) {
    //  if avg is a special pixel, we must change to integer values so we don't throw off the linear fit
    if(avg[i] == Isis::NULL2) {
      avg[i] = 0;
    }
    if(avg[i] == Isis::HIGH_REPR_SAT2) {
      if(gbl::cissLab->DataConversionType() == "Table") {
        avg[i] = 4095;
      }
      else {
        avg[i] = 255;
      }
    }
    eqn[0] = (double)(i + 1);
    //  add to least squares variable
    lsq.AddKnown(eqn, avg[i]);
  }
  // solve linear fit
  lsq.Solve(LeastSquares::QRD);
  for(unsigned int i = 0; i < overclocks.size(); i++) {
    eqn[0] = (double)(i + 1);
    overclockfit.push_back(lsq.Evaluate(eqn));
  }
  //return a copy of the vector of linear fitted overclocks
  // this will be used as the bias
  return overclockfit;
}
//=====End Debias Methods========================================================================//


//=====Subtract Dark Methods=====================================================================//
// THESE ARE CONTAINED IN THE CLASS DARKCURRENT
//=====End Subtract Dark Methods=================================================================//


//=====1 Linearize Methods=========================================================================//
//This method is modelled after IDL CISSCAL's cassimg_linearise.pro

/**
 * This method is modelled after IDL CISSCAL's
 * cassimg_linearise.pro.  The purpose is to correct the image
 * for non-linearity.
 *
 *  @internal
 *   @history 2008-11-05 Jeannie Walldren - Original version
 */
void gbl::Linearize() {
//  These are the correction factor tables from the referenced documents
//  For each gain state there are a list of DNs where measurements
//    were performed and the corresponding correction factors C
//  The correction is then performed as DN'=DN*Cdn
//  Where Cdn is an interpolation for C from the tabulated values

  QString lut;
  int gainState = gbl::cissLab->GainState();
  if(gbl::cissLab->NarrowAngle()) {
    switch(gainState) {
      case 0:
        lut = "NAC0.lut";
        break;
      case 1:
        lut = "NAC1.lut";
        break;
      case 2:
        lut = "NAC2.lut";
        break;
      case 3:
        lut = "NAC3.lut";
        break;
      default:
        throw IException(IException::Unknown,
                         "Input file contains invalid GainState. See Software Interface Specification (SIS), Version 1.1, page 86.",
                         _FILEINFO_);
    }
  }
  else {
    switch(gainState) {
      case 0:
        lut = "WAC0.lut";
        break;
      case 1:
        lut = "WAC1.lut";
        break;
      case 2:
        lut = "WAC2.lut";
        break;
      case 3:
        lut = "WAC3.lut";
        break;
      default:
        throw IException(IException::Unknown,
                         "Input file contains invalid GainState. See Software Interface Specification (SIS), Version 1.1, page 86.",
                         _FILEINFO_);
    }
  }
  vector <double> DN_VALS, C_VALS;
  // Get the directory where the CISS linearize directory is.
  FileName linearLUT(gbl::GetCalibrationDirectory("linearize").toStdString() + lut.toStdString());

  if(!linearLUT.fileExists()) {
    throw IException(IException::Io,
                     "Unable to calibrate image. LinearityCorrectionTable ***"
                     + linearLUT.expanded() + "*** not found.", _FILEINFO_);
  }
  gbl::calgrp += PvlKeyword("LinearityCorrectionPerformed", "Yes");
  gbl::calgrp.findKeyword("LinearityCorrectionPerformed").addComment("Linearity Correction Parameters");
  gbl::calgrp += PvlKeyword("LinearityCorrectionTable", linearLUT.original());

  TextFile *pairs = new TextFile(QString::fromStdString(linearLUT.original()));
  for(int i = 0; i < pairs->LineCount(); i++) {
    QString line;
    pairs->GetLine(line, true);
    line = line.simplified();
    QStringList tokens = line.split(" ");
    DN_VALS.push_back(tokens.takeFirst().toDouble());
    C_VALS.push_back(tokens.takeFirst().toDouble());
  }
  pairs->Close();

  //  ASSUMPTION: C will not change significantly over fractional DN
  //  If this is not the case, then can perform simple second interpolation
  //  between DNs while mapping LUT onto the image
  NumericalApproximation linearInterp(NumericalApproximation::Linear);
  for(unsigned int i = 0; i < DN_VALS.size(); i++) {
    linearInterp.AddData(DN_VALS[i], C_VALS[i]);
  }

  // Create the stretch pairs
  gbl::stretch.ClearPairs();
  for(unsigned int i = 0; i < 4096; i++) {
    double j = linearInterp.Evaluate((double) i);
    gbl::stretch.AddPair(i, j);
  }
  //  Map LUT onto image, defending against out-of-range DN values
  return;
}
//=====End Linearize Methods=====================================================================//


//=====2 Flatfield Methods=========================================================================//
// These methods are modelled after IDL CISSCAL's cassimg_dustringcorrect.pro and cassimg_dividebyflats.pro

/**
 * This method is modelled after IDL CISSCAL's
 * cassimg_dustringcorrect.pro.  The purpose is to find the
 * files and value needed to perform dustring correction and
 * mottle correction: dustFile, mottleFile, strengthFactor.
 *
 * @internal
 *   @history 2008-11-05 Jeannie Walldren - Original version
 *   @history 2009-05-27 Jeannie Walldren - Changed to read
 *            effective wavelength from 5th column of file
 *            (rather than 3rd) since effective wavelength file
 *            updated with new idl cisscal version, 3.6.  Added
 *            effective wavelength to calibration group in
 *            labels.
 *   @history 2019-08-14 Kaitlyn Lee - Added check for
 *            ShutterStateId to disable dust ring correction.
 *            Matches cisscal version 3.9.1.
 */
void gbl::FindDustRingParameters() {
  //  No dustring or mottle correction for WAC
  if(gbl::cissLab->WideAngle()) {
    gbl::dustCorrection = false;
    gbl::mottleCorrection = false;
    gbl::calgrp += PvlKeyword("DustRingCorrectionPerformed", "No: ISSWA");
    gbl::calgrp.findKeyword("DustRingCorrectionPerformed").addComment("DustRing Correction Parameters");
    gbl::calgrp += PvlKeyword("DustRingFile", "Not applicable: No dustring correction");
    gbl::calgrp += PvlKeyword("MottleCorrectionPerformed", "No: dustring correction");
    gbl::calgrp += PvlKeyword("MottleFile", "Not applicable: No dustring correction");
    gbl::calgrp += PvlKeyword("EffectiveWavelengthFile", "Not applicable: No dustring correction");
    gbl::calgrp += PvlKeyword("StrengthFactor", "Not applicable: No dustring correction");
    return;
  }

  // Disable dust ring and mottle correction if ShutterStateId is Disabled
  if(gbl::cissLab->ShutterStateId() == "Disabled") {
    gbl::dustCorrection = false;
    gbl::mottleCorrection = false;
    gbl::calgrp += PvlKeyword("DustRingCorrectionPerformed", "No: ShutterStateId is Disabled.");
    gbl::calgrp.findKeyword("DustRingCorrectionPerformed").addComment("DustRing Correction Parameters");
    gbl::calgrp += PvlKeyword("DustRingFile", "Not applicable: No dustring correction");
    gbl::calgrp += PvlKeyword("MottleCorrectionPerformed", "No: dustring correction");
    gbl::calgrp += PvlKeyword("MottleFile", "Not applicable: No dustring correction");
    gbl::calgrp += PvlKeyword("EffectiveWavelengthFile", "Not applicable: No dustring correction");
    gbl::calgrp += PvlKeyword("StrengthFactor", "Not applicable: No dustring correction");
    return;
  }

  // dustring correct for NAC
  gbl::dustCorrection = true;
  gbl::calgrp += PvlKeyword("DustRingCorrectionPerformed", "Yes");
  gbl::calgrp.findKeyword("DustRingCorrectionPerformed").addComment("DustRing Correction Parameters");
  // needs to check directory for current epochs and compare
  double imgNumber = gbl::cissLab->ImageNumber();
  long largestEpochFileNum = 0;
  QStringList fileList = QDir(gbl::GetCalibrationDirectory("dustring")).entryList();
  for(int i = 0; i < fileList.count(); i++){
    long currentEpoch = Isis::toInt(FileName(fileList[i].toStdString()).baseName().substr(13, 10));
    if (currentEpoch > largestEpochFileNum && currentEpoch <= imgNumber) {
        largestEpochFileNum = currentEpoch;
    }
  }

  // get name of dust file
  gbl::dustFile = (gbl::GetCalibrationDirectory("dustring").toStdString() + "nac_dustring_" + Isis::toString(largestEpochFileNum)
                   + "." + gbl::cissLab->InstrumentModeId().toStdString() + ".cub");
  if(!gbl::dustFile.fileExists()) { // dustring file not found, assume file uses old dustring files
    gbl::dustFile = (gbl::GetCalibrationDirectory("dustring").toStdString() + "nac_dustring_venus."
                    + gbl::cissLab->InstrumentModeId().toStdString() + ".cub");
    if(!gbl::dustFile.fileExists()) { // dustring file not found, stop calibration
      throw IException(IException::Io,
                      "Unable to calibrate image. DustRingFile ***"
                      + gbl::dustFile.expanded() + "*** not found.", _FILEINFO_);
    }
  }
  gbl::calgrp += PvlKeyword("DustRingFile", gbl::dustFile.original());
  // if anti-blooming correction is off correct ring at sample=887, line=388
  if (!gbl::cissLab->AntibloomingOn()) {
    gbl::dustFile2 = (gbl::GetCalibrationDirectory("dustring").toStdString() + "nac_dustring_aboff"
                      + "." + gbl::cissLab->InstrumentModeId().toStdString() +".cub");
    if(!gbl::dustFile2.fileExists()) { // dustring file not found, stop calibration
      throw IException(IException::Io,
                       "Unable to calibrate image. DustRingFile2 ***"
                       + gbl::dustFile2.expanded() + "*** not found.", _FILEINFO_);
    }
    gbl::calgrp += PvlKeyword("DustRingFile2", gbl::dustFile2.original());
  }

  // No Mottling correction for images before sclk=1444733393: (i.e., 2003-286T10:28:04)
  if(gbl::cissLab->ImageNumber() < 1455892746) {
    gbl::mottleFile = "";
    gbl::mottleCorrection = false;
    gbl::calgrp += PvlKeyword("MottleCorrectionPerformed", "No: Image before 2003-286T10:28:04");
    gbl::calgrp += PvlKeyword("MottleFile", "Not applicable: No mottle correction");
    gbl::calgrp += PvlKeyword("EffectiveWavelengthFile", "Not applicable: No mottle correction");
    gbl::calgrp += PvlKeyword("StrengthFactor", "Not applicable: No mottle correction");
    return;
  }

  // Mottling correction for images after 2003-286T10:28:04
  gbl::mottleFile = (gbl::GetCalibrationDirectory("dustring").toStdString() + "nac_mottle_1444733393."+
                      gbl::cissLab->InstrumentModeId().toStdString() + ".cub");
  if(!gbl::mottleFile.fileExists()) { // mottle file not found, stop calibration
    throw IException(IException::Io,
                     "Unable to calibrate image. MottleFile ***"
                     + gbl::mottleFile.expanded() + "*** not found.", _FILEINFO_);
  }
  gbl::mottleCorrection = true;
  gbl::calgrp += PvlKeyword("MottleCorrectionPerformed", "Yes");
  gbl::calgrp += PvlKeyword("MottleFile", gbl::mottleFile.original());

  // determine strength factor, need effective wavelength of filter
  vector <int> filterIndex(2);
  filterIndex[0] = gbl::cissLab->FilterIndex()[0];
  filterIndex[1] = gbl::cissLab->FilterIndex()[1];

  if((filterIndex[0] == 17 || filterIndex[0] == 21 || filterIndex[0] == 22 || filterIndex[0] ==  23 \
      || filterIndex[0] == 35) && filterIndex[1] == 18) { //filter combo CL1 or P0 or P60 or P120 or IRP0/CL2
    filterIndex[0] = -1;
  }

  if((filterIndex[0] < 17 && filterIndex[1] < 17) || (filterIndex[0] >= 17 && filterIndex[1] >= 17)) {
    gbl::strengthFactor = 0.0;
    // use effective wavelength to estimate strength factor:
    FileName effectiveWavelength(gbl::GetCalibrationDirectory("efficiency").toStdString() + "na_effwl.tab");
    if(!effectiveWavelength.fileExists()) { // effectivewavelength file not found, stop calibration
      throw IException(IException::Io,
                       "Unable to calibrate image. EffectiveWavelengthFile ***"
                       + effectiveWavelength.expanded() + "*** not found.", _FILEINFO_);
    }
    gbl::calgrp += PvlKeyword("EffectiveWavelengthFile", effectiveWavelength.original());
    CisscalFile *effwlDB = new CisscalFile(QString::fromStdString(effectiveWavelength.original()));
    QString col1, col2, col3, col4, col5;
    double effwl;
    for(int i = 0; i < effwlDB->LineCount(); i++) {
      QString line;
      effwlDB->GetLine(line);
      line = line.simplified();

      QStringList cols = line.split(" ");

      col1 = cols.takeFirst();
      if(col1 == gbl::cissLab->FilterName()[0]) {
        col2 = cols.takeFirst();
        if(col2 == gbl::cissLab->FilterName()[1]) {
          col3 = cols.takeFirst();  // central wavelength of filter combo
          col4 = cols.takeFirst();  // full-width at half-maximum (FWHM) of filter combo
          col5 = cols.takeFirst();  // effective wavelength
          if(col5 == "") {
            //       Couldn't find a match in the database
            gbl::calgrp.findKeyword("MottleCorrectionPerformed").setValue("Yes: EffectiveWavelengthFile contained no factor for filter combination, used strengthFactor of 1.0");
            gbl::strengthFactor = 1.0;
          }
          else {
            effwl = col5.toDouble();
            gbl::calgrp += PvlKeyword("EffectiveWavelength", Isis::toString(effwl));
            gbl::strengthFactor = 1.30280 - 0.000717552 * effwl;
          }
          break;
        }
        else {
          continue;
        }
      }
      else {
        continue;
      }
    }
    effwlDB->Close();
    if(gbl::strengthFactor == 0.0) {
      gbl::calgrp.findKeyword("MottleCorrectionPerformed").setValue("Yes: EffectiveWavelengthFile contained no factor for filter combination, used strengthFactor of 1.0");
      gbl::strengthFactor = 1.0;
    }
  }
  else {//if(filterIndex[0] > 17 || filterIndex[1] > 17 )
    gbl::calgrp += PvlKeyword("EffectiveWavelengthFile", "No effective wavelength file used");
    switch(filterIndex[0]) {
      case 0:
        gbl::strengthFactor = 1.119;
        break;
      case 1:
        gbl::strengthFactor = 1.186;
        break;
      case 3:
        gbl::strengthFactor = 1.00;
        break;
      case 6:
        gbl::strengthFactor = 0.843;
        break;
      case 8:
        gbl::strengthFactor = 0.897;
        break;
      case 10:
        gbl::strengthFactor = 0.780;
        break;
      case -1:
        gbl::strengthFactor = 0.763;
        break;
      default:
        switch(filterIndex[1]) {
          case 2:
            gbl::strengthFactor = 1.069;
            break;
          case 4:
            gbl::strengthFactor = 0.833;
            break;
          case 5:
            gbl::strengthFactor = 0.890;
            break;
          case 7:
            gbl::strengthFactor = 0.997;
            break;
          case 9:
            gbl::strengthFactor = 0.505;
            break;
          case 11:
            gbl::strengthFactor = 0.764;
            break;
          case 12:
            gbl::strengthFactor = 0.781;
            break;
          case 13:
            gbl::strengthFactor = 0.608;
            break;
          case 14:
            gbl::strengthFactor = 0.789;
            break;
          case 15:
            gbl::strengthFactor = 0.722;
            break;
          case 16:
            gbl::strengthFactor = 0.546;
            break;
          default:
            throw IException(IException::Unknown,
                             "Input file contains invalid FilterName. See Software Interface Specification (SIS) Appendix A, Table 8.2.",
                             _FILEINFO_);
        }
    }
  }
  gbl::calgrp += PvlKeyword("StrengthFactor", Isis::toString(gbl::strengthFactor));
  return;
}

/**
 * This method is modelled after IDL CISSCAL's
 * cassimg_dividebyflats.pro.  The purpose is to find the flat
 * field file needed to correct the image for sensitivity
 * variations across the field by dividing by flat field image.
 *
 * @return <B>FileName</B> Name of the flat file for this image.
 * @internal
 *   @history 2008-11-05 Jeannie Walldren - Original version
 *   @history 2019-08-14 Kaitlyn Lee - Added check for
 *            ShutterStateId to disable flat field correction.
 *            Matches cisscal version 3.9.1.
 */
FileName gbl::FindFlatFile() {
  // Disable flat field correction if ShutterStateId is Disabled
  if(gbl::cissLab->ShutterStateId() == "Disabled") {
    gbl::calgrp += PvlKeyword("FlatfieldCorrectionPerformed", "No: ShutterStateId is Disabled.");
    gbl::calgrp.findKeyword("FlatfieldCorrectionPerformed").addComment("Flatfield Correction Parameters");
    gbl::calgrp += PvlKeyword("SlopeDataBase", "Not applicable: No flat field correction");
    gbl::flatCorrection = false;
    return "";
  }

  //  There is a text database file in the slope files directory
  //   that maps filter combinations (and camera temperature)
  //   to the corresponding slope field files.
  // according to slope_info.txt, slope_db_1 is original, slope_db_2 is the best and slope_db_3 is newest but has some issues
  FileName flatFile;
  FileName slopeDatabaseName(gbl::GetCalibrationDirectory("slope").toStdString() + "slope_db_2.tab");
  if(!slopeDatabaseName.fileExists()) { // slope database not found, stop calibration
    throw IException(IException::Io,
                     "Unable to calibrate image. SlopeDataBase ***"
                     + slopeDatabaseName.expanded() + "*** not found.", _FILEINFO_);
  }
  gbl::calgrp += PvlKeyword("FlatfieldCorrectionPerformed", "Yes");
  gbl::calgrp.findKeyword("FlatfieldCorrectionPerformed").addComment("Flatfield Correction Parameters");
  gbl::calgrp += PvlKeyword("SlopeDataBase", slopeDatabaseName.original());
  gbl::flatCorrection = true;

  // Find the best-match flat file
  //  Choose a nominal optics temp name as per ISSCAL
  QString frontOpticsTemp("");
  if(gbl::cissLab->FrontOpticsTemp() < -5.0) {
    frontOpticsTemp += "m10";
  }
  else if(gbl::cissLab->FrontOpticsTemp() < 25.0) {
    frontOpticsTemp += "p5";
  }
  else {
    frontOpticsTemp += "p25";
  }
  //  Require match for instrument, temperature range name, Filter1, filter2
  CisscalFile *slopeDB = new CisscalFile(QString::fromStdString(slopeDatabaseName.original()));
  QString col1, col2, col3, col4, col5, col6, col7, col8;
  for(int i = 0; i < slopeDB->LineCount(); i++) {
    QString line;
    slopeDB->GetLine(line);  //assigns value to line
    line = line.simplified();
    QStringList cols = line.split(" ");
    cols.replaceInStrings(QRegExp("(^'|'$)"), "");

    col1 = cols.takeFirst();
    if(col1 == gbl::cissLab->InstrumentId()) {
      col2 = cols.takeFirst();
      if((col2 == frontOpticsTemp) || (gbl::cissLab->WideAngle())) {
        col3 = cols.takeFirst();
        if(col3 == gbl::cissLab->FilterName()[0]) {
          col4 = cols.takeFirst();
          if(col4 == gbl::cissLab->FilterName()[1]) {
            col5 = cols.takeFirst();// col5 = gainstate (not used)
            col6 = cols.takeFirst();// col6 = antiblooming state (not used)
            col7 = cols.takeFirst();// col7 = file number (not used)
            col8 = cols.takeFirst();// col8 = slope file name
            break;
          }
          else {
            continue;
          }
        }
        else {
          continue;
        }
      }
      else {
        continue;
      }
    }
    else {
      continue;
    }
  }
  slopeDB->Close();

  if(col8 == "") {  // error in slope database, stop calibration
    // Couldn't find a match in the database
    throw IException(IException::Io,
                     "Unable to calibrate image. SlopeDataBase contained no factor for combination:"
                     + gbl::cissLab->InstrumentId().toStdString()  + ":" + frontOpticsTemp.toStdString() + ":"
                     + gbl::cissLab->FilterName()[0].toStdString() + ":" + gbl::cissLab->FilterName()[1].toStdString() + ".",
                     _FILEINFO_);
  }
  //Column 8 contains version of slopefile from which our flatfiles are derived
  int j = col8.indexOf(".");
  //attatch version number to "flat" by skipping
  // the first 5 characters("SLOPE") and skipping
  // any thing after "." ("IMG")
  col8 = "flat" + col8.mid(5, (j - 5) + 1);
  flatFile = (gbl::GetCalibrationDirectory("slope/flat").toStdString() + col8.toStdString()
              + gbl::cissLab->InstrumentModeId().toStdString() + ".cub");
  gbl::calgrp += PvlKeyword("FlatFile", flatFile.original());
  if(!flatFile.fileExists()) { // flat file not found, stop calibration
    throw IException(IException::Io,
                     "Unable to calibrate image. FlatFile ***"
                     + flatFile.expanded() + "*** not found.", _FILEINFO_);
  }
  return flatFile;
}
//=====End Flatfield Methods=====================================================================//

//=====5 Convert DN to Flux Methods================================================================//
// These methods are modelled after IDL CISSCAL's cassimg_dntoelectrons.pro, cassimg_dividebyexpot.pro,
// cassimg_dividebyareapixel.pro, cassimg_dividebyefficiency.pro

/**
 * This method is modelled after IDL CISSCAL's
 * cassimg_dntoelectrons.pro.  The purpose is to find the true
 * gain needed to multiply image by gain constant (convert DN to
 * electrons).
 *
 *  @internal
 *   @history 2008-11-05 Jeannie Walldren - Original version
 *   @history 2009-05-27 Jeannie Walldren - Removed return
 *            statement and added else statement so that the
 *            TrueGain keyword is added in any case.
 */
void gbl::DNtoElectrons() {
  gbl::calgrp += PvlKeyword("DNtoFluxPerformed", "Yes");
  gbl::calgrp.findKeyword("DNtoFluxPerformed").addComment("DN to Flux Parameters");
  gbl::calgrp += PvlKeyword("DNtoElectrons", "Yes");
  //  Gain used for an image is documented by the GainModID attribute
  //  of the image. Nominal values are as follow:
  //
  //  Attribute  Gain  Usual  Nominal Gain
  //   Value  state    mode  (e- per DN)
  //  "1400K"    0      SUM4      215
  //   "400K"    1      SUM2       95
  //   "100K"    2      FULL       29
  //    "40K"    3      FULL       12
  if(gbl::cissLab->NarrowAngle()) {
    switch(gbl::cissLab->GainState()) {
      case 0:
        gbl::trueGain = 30.27 / 0.135386;
        break;
      case 1:
        gbl::trueGain = 30.27 / 0.309569;
        break;
      case 2:
        gbl::trueGain = 30.27 / 1.0;
        break;
      case 3:
        gbl::trueGain = 30.27 / 2.357285;
        break;
      default: // invalid gainstate, unable to convert DN to electrons, stop calibration
        throw IException(IException::Unknown,
                         "Input file contains invalid GainState. See Software Interface Specification (SIS), Version 1.1, page 86.",
                         _FILEINFO_);
    }
  }
  else {
    switch(gbl::cissLab->GainState()) {
      case 0:
        gbl::trueGain = 27.68 / 0.125446;
        break;
      case 1:
        gbl::trueGain = 27.68 / 0.290637;
        break;
      case 2:
        gbl::trueGain = 27.68 / 1.0;
        break;
      case 3:
        gbl::trueGain = 27.68 / 2.360374;
        break;
      default: // invalid gainstate, unable to convert DN to electrons, stop calibration
        throw IException(IException::Unknown,
                         "Input file contains invalid GainState. See Software Interface Specification (SIS), Version 1.1, page 86.",
                         _FILEINFO_);
    }
  }
  gbl::calgrp += PvlKeyword("TrueGain", Isis::toString(gbl::trueGain));
  return;
}

/**
 * This method is modelled after IDL CISSCAL's
 * cassimg_dividebyexpot.pro.  The purpose is to find the
 * shutter offset needed to divide a Cassini image by corrected
 * exposure time, correcting for shutter offset effects (sample
 * dependency of actual exposure time).
 *
 * @internal
 *   @history 2008-11-05 Jeannie Walldren - Original version
 */
void gbl::FindShutterOffset() {
  //  Don't do this for zero-exposure images!
  if(gbl::cissLab->ExposureDuration() == 0.0) { // exposuretime = 0, stop calibration
    throw IException(IException::Unknown,
                     "Unable to calibrate image.  Cannot divide by exposure time for zero exposure image.",
                     _FILEINFO_);
  }
  gbl::calgrp += PvlKeyword("DividedByExposureTime", "Yes");
  gbl::divideByExposure = true;
  //  Define whereabouts of shutter offset files
  QString offsetFileName("");
  if(gbl::cissLab->NarrowAngle()) {
    offsetFileName += (gbl::GetCalibrationDirectory("offset") + "nacfm_so_");
  }
  else {
    offsetFileName += (gbl::GetCalibrationDirectory("offset") + "wacfm_so_");
  }
  if(gbl::cissLab->FrontOpticsTemp() < -5.0) {
    offsetFileName += "m10.";
  }
  else if(gbl::cissLab->FrontOpticsTemp() < 25.0) {
    offsetFileName += "p5.";
  }
  else {
    offsetFileName += "p25.";
  }
  offsetFileName += (gbl::cissLab->InstrumentModeId() + ".cub");
  FileName shutterOffsetFile(offsetFileName.toStdString());
  if(!shutterOffsetFile.fileExists()) { // shutter offset file not found, stop calibration
    throw IException(IException::Io,
                     "Unable to calibrate image. ShutterOffsetFile ***"
                     + shutterOffsetFile.expanded() + "*** not found.", _FILEINFO_);
  }
  gbl::calgrp += PvlKeyword("ShutterOffsetFile", shutterOffsetFile.original());
  Cube offsetCube;
  offsetCube.open(QString::fromStdString(shutterOffsetFile.original()));
  gbl::offset = new Brick(gbl::incube->sampleCount(), 1, 1, offsetCube.pixelType());
  gbl::offset->SetBasePosition(1, 1, 1);
  offsetCube.read(*gbl::offset);
  offsetCube.close();
  return;
  // Pixel value is now flux (electrons per second)
}

/**
 * This method is modelled after IDL CISSCAL's
 *  cassimg_dividebyareapixel.pro.  The purpose is to find the
 *  values needed to normalise the image by dividing by area of
 *  optics and by solid angle subtended by a pixel.
 *
 * @internal
 *   @history 2008-11-05 Jeannie Walldren - Original version
 *   @history 2019-08-14 Kaitlyn Lee - Added check for
 *            ShutterStateId. Updated solid angle and optics
 *            area values. Matches cisscal version 3.9.1.
 */
void gbl::DivideByAreaPixel() {
  // Disable flat field correction if ShutterStateId is Disabled
  if(gbl::cissLab->ShutterStateId() == "Disabled") {
    gbl::calgrp += PvlKeyword("DividedByAreaPixel", "No: ShutterStateId is Disabled.");
    gbl::calgrp += PvlKeyword("SolidAngle", "Not applicable: No division by area pixel");
    gbl::calgrp += PvlKeyword("OpticsArea", "Not applicable: No division by area pixel");
    gbl::calgrp += PvlKeyword("SumFactor", "Not applicable: No division by area pixel");
  return;
    return;
  }
  //  These values as per ISSCAL
  //  SolidAngle is (FOV of Optics) / (Number of Pixels)
  //  OpticsArea is (Diameter of Primary Mirror)^2 * Pi/4
  //      Optics areas below come from radii in "Final Report, Design
  //      and Analysis of Filters for the Cassini Narrow and Wide
  //      Optics" by David Hasenauer, May 19, 1994.

  //  We will adjust here for the effects of SUM modes
  //  (which effectively give pixels of 4 or 16 times normal size)

  gbl::calgrp += PvlKeyword("DividedByAreaPixel", "Yes");
  if(gbl::cissLab->NarrowAngle()) {
    gbl::solidAngle = 3.58885 * pow(10.0, -11.0);
    gbl::opticsArea = 284.86;
  }
  else {
    gbl::solidAngle = 3.56994 * pow(10.0, -9);
    gbl::opticsArea = 29.43;
  }
  //  Normalize summed images to real pixels

  // sumFactor is the inverse of the square of the summing mode,
  // it was expressed in IDL as the following:
  //       [gbl::sumFactor = (gbl::incube->sampleCount()/1024.0)*(gbl::incube->lineCount()/1024.0);]
  gbl::sumFactor = 1 / pow(gbl::cissLab->SummingMode(), 2.0);
  gbl::calgrp += PvlKeyword("SolidAngle", Isis::toString(gbl::solidAngle));
  gbl::calgrp += PvlKeyword("OpticsArea", Isis::toString(gbl::opticsArea));
  gbl::calgrp += PvlKeyword("SumFactor", Isis::toString(gbl::sumFactor));
  return;
}

/**
 * This method is modelled after IDL CISSCAL's
 * cassimg_dividebyefficiency.pro. The purpose is to find the
 * efficiency factor for the given flux units. This value is
 * used to correct the image for filter and CCD efficiency.
 *
 * @b Note: For "I/F", The results diverge from the IDL results
 * due to differences in the way they calculate solar distance.
 * However, the DN results are still within 0.2% after we divide
 * by efficiency factor.
 *
 * @internal
 *   @history 2008-11-05 Jeannie Walldren - Original version of
 *            FindEfficiencyFactor_IoverF() and
 *            FindEfficiencyFactor_IntensityUnits() written.
 *   @history 2009-02-12 Jeannie Walldren - Modified
 *            FindEfficiencyFactor_IoverF() code to make a
 *            second attempt to find the planet if the
 *            Isis::Camera class fails to find it at the center
 *            point of the cube. Previously, if the target was
 *            not found, an exception was thrown. Now the
 *            SubSpacecraftPoint() method from Isis::Camera
 *            class is used to try to locate the target before
 *            throwing the exception.
 *   @history 2009-05-27 Jeannie Walldren - Joined
 *            FindEfficiencyFactor_IoverF() and
 *            FindEfficiencyFactor_IntensityUnits() into
 *            FindEfficiencyFactor() per new idl cisscal
 *            version, 3.6, updates. This version no longer uses
 *            the effic_db (now called omega0) to calculate the
 *            efficiency factor for intensity units.  Now, the
 *            method is not far off from the I/F method.
 *   @history 2019-08-14 Kaitlyn Lee - Added check for
 *            ShutterStateId. Matches cisscal version 3.9.1.
 */

void gbl::FindEfficiencyFactor(QString fluxunits) {
  // Disable flat field correction if ShutterStateId is Disabled
  if(gbl::cissLab->ShutterStateId() == "Disabled") {
    gbl::calgrp += PvlKeyword("DividedByEfficiency", "No: ShutterStateId is Disabled.");
    gbl::calgrp += PvlKeyword("EfficiencyFactorMethod", "Not applicable: No division by efficiency");
    gbl::calgrp += PvlKeyword("TransmissionFile", "Not applicable: No division by efficiency");
    gbl::calgrp += PvlKeyword("QuantumEfficiencyFile", "Not applicable: No division by efficiency");
    gbl::calgrp += PvlKeyword("SpectralFile", "Not applicable: No division by efficiency");
    gbl::calgrp += PvlKeyword("SolarDistance", "Not applicable: No division by efficiency");
    gbl::calgrp += PvlKeyword("EfficiencyFactor", "Not applicable: No division by efficiency");
    gbl::calgrp += PvlKeyword("TotalEfficiency", "Not applicable: No division by efficiency");
    return;
  }

  // for polarized filter combinations, use corresponding clear transmission:
  QString filter1 = gbl::cissLab->FilterName()[0];
  QString filter2 = gbl::cissLab->FilterName()[1];
  if(filter1 == "IRP0" || filter1 == "P120" || filter1 == "P60" || filter1 == "P0"
      || filter2 == "IRP90" || filter2 == "IRP0") {
    if(gbl::cissLab->InstrumentId() == "ISSNA") {
      filter1 = "CL1";
    }
    if(gbl::cissLab->InstrumentId() == "ISSWA") {
      filter2 = "CL2";
    }
  }

  gbl::calgrp += PvlKeyword("DividedByEfficiency", "Yes");
  gbl::calgrp += PvlKeyword("EfficiencyFactorMethod", fluxunits.toStdString());
  vector<double> lambda; // lambda will contain all wavelength vectors used

  //--- 1) CREATE LINEAR APPROXIMATION FROM SYSTEM TRANSMISSION FILE ----------
  // find system transmission file (T0*T1*T2*QE)

  FileName transfile(gbl::GetCalibrationDirectory("efficiency/systrans").toStdString()
                     + gbl::cissLab->InstrumentId().toLower().toStdString()
                     + filter1.toLower().toStdString()
                     + filter2.toLower().toStdString() + "_systrans.tab");
  if(!transfile.fileExists()) { // transmission file not found, stop calibration
    throw IException(IException::Io,
                     "Unable to calibrate image. TransmissionFile ***"
                     + transfile.expanded() + "*** not found.", _FILEINFO_);
  }
  // read in system transmission to find transmitted wavelength and flux
  gbl::calgrp += PvlKeyword("TransmissionFile", transfile.original());
  CisscalFile *trans = new CisscalFile(QString::fromStdString(transfile.original()));
  vector<double> wavelengthT, transmittedFlux;
  double x, y;
  for(int i = 0; i < trans->LineCount(); i++) {
    QString line;
    trans->GetLine(line);  //assigns value to line
    line = line.simplified().trimmed();
    if(line == "") {
      break;
    }
    x = line.split(" ")[0].toDouble();
    y = line.split(" ")[1].toDouble();
    wavelengthT.push_back(x);
    transmittedFlux.push_back(y);
  }
  trans->Close();
  // wavelength and flux are in descending order, reverse to ascending order
  if(wavelengthT[0] > wavelengthT.back()) {
    reverse(wavelengthT.begin(), wavelengthT.end());
    reverse(transmittedFlux.begin(), transmittedFlux.end());
  }
  lambda = wavelengthT;
  // Create Linear approximation from the transmitted data
  NumericalApproximation newtrans(NumericalApproximation::Linear);
  for(unsigned int i = 0; i < transmittedFlux.size(); i++) {
    newtrans.AddData(wavelengthT[i], transmittedFlux[i]);
  }

  //--- 2) CREATE LINEAR APPROXIMATION FROM QUANTUM EFFICIENCY FILE -----------
  // find quantum efficiency file
  FileName qecorrfile;
  if(gbl::cissLab->NarrowAngle()) {
    qecorrfile = gbl::GetCalibrationDirectory("correction").toStdString() + "nac_qe_correction.tab";
  }
  else {
    qecorrfile = gbl::GetCalibrationDirectory("correction").toStdString() + "wac_qe_correction.tab";
  }
  if(!qecorrfile.fileExists()) { // quantum efficiency file not found, stop calibration
    throw IException(IException::Io,
                     "Unable to calibrate image. QuantumEfficiencyFile ***"
                     + qecorrfile.expanded() + "*** not found.", _FILEINFO_);
  }
  // read qe file to find qe wavelength and correction
  gbl::calgrp += PvlKeyword("QuantumEfficiencyFile", qecorrfile.original());
  CisscalFile *qeCorr = new CisscalFile(QString::fromStdString(qecorrfile.original()));
  vector<double> wavelengthQE, qecorrection;
  for(int i = 0; i < qeCorr->LineCount(); i++) {
    QString line;
    qeCorr->GetLine(line);  //assigns value to line
    line = line.simplified().trimmed();
    if(line == "") {
      break;
    }
    x = line.split(" ").first().toDouble();
    y = line.split(" ").last().toDouble();
    wavelengthQE.push_back(x);
    qecorrection.push_back(y);
    lambda.push_back(x);
  }
  qeCorr->Close();
  // wavelength and qecorr are in descending order, reverse to ascending order
  if(wavelengthQE[0] > wavelengthQE.back()) {
    reverse(wavelengthQE.begin(), wavelengthQE.end());
    reverse(qecorrection.begin(), qecorrection.end());
  }
  // Create Linear approximation from the qe data
  NumericalApproximation newqecorr(NumericalApproximation::Linear);
  for(unsigned int i = 0; i < qecorrection.size(); i++) {
    newqecorr.AddData(wavelengthQE[i], qecorrection[i]);
  }


  // these variables will be defined in the if-statement
  QString units;
  double minlam, maxlam;
  vector<double> fluxproduct1, fluxproduct2;

  if(fluxunits == "INTENSITY") {
    gbl::calgrp += PvlKeyword("SpectralFile", "Not applicable: Intensity Units chosen");
    gbl::calgrp += PvlKeyword("SolarDistance", "Not applicable: Intensity Units chosen");
    //--- 3a) SORT AND MAKE LAMBDA UNIQUE, REMOVE OUTLIERS, -------------------
    //---     FIND FLUX PRODUCTS TO BE INTERPOLATED ---------------------------
    units = "phot/cm^2/s/nm/ster";

    // lambda domain min is the largest of the minimum wavelength values (rounded up)
    minlam = ceil(max(wavelengthT.front(), wavelengthQE.front()));
    // lambda domain max is the smallest of the maximum wavelength values (rounded down)
    maxlam = floor(min(wavelengthT.back(), wavelengthQE.back()));

    // NumericalApproximation requires lambda to be sorted
    sort(lambda.begin(), lambda.end());
    // NumericalApproximation requires lambda to be unique
    vector<double>::iterator it = unique(lambda.begin(), lambda.end());
    lambda.resize(it - lambda.begin());

    // remove any values that fall below minlam
    while(lambda[0] < minlam) {
      lambda.erase(lambda.begin());
    }

    // remove any values that fall above maxlam
    while(lambda[lambda.size()-1] > maxlam) {
      lambda.erase(lambda.end() - 1);
    }

    for(unsigned int i = 0; i < lambda.size(); i++) {
      double a = newtrans.Evaluate(lambda[i]);
      double b = newqecorr.Evaluate(lambda[i]);
      fluxproduct1.push_back(a * b);
    }
    fluxproduct2 = fluxproduct1;
  }
  else {// if(fluxunits == "I/F") {
    //--- 3b) CALCULATE SOLAR DISTANCE, ---------------------------------------
    // ---    CREATE LINEAR APPROXIMATION FROM SPECTRAL FILE ------------------
    //---     SORT AND MAKE LAMBDA UNIQUE, REMOVE OUTLIERS, -------------------
    //---     FIND FLUX PRODUCTS TO BE INTERPOLATED ---------------------------

    units = "I/F";

    // find spectral file
    FileName specfile(gbl::GetCalibrationDirectory("efficiency").toStdString() + "solarflux.tab");
    if(!specfile.fileExists()) { // spectral file not found, stop calibration
      throw IException(IException::Io,
                       "Unable to calibrate image using I/F. SpectralFile ***"
                       + specfile.expanded() + "*** not found.", _FILEINFO_);
    }
    gbl::calgrp += PvlKeyword("SpectralFile", specfile.original());

    // get distance from sun (AU):
    double angstromsToNm = 10.0;
    double distFromSun = 0;
    try {
      Camera *cam = gbl::incube->camera();
      bool camSuccess = cam->SetImage(gbl::incube->sampleCount() / 2, gbl::incube->lineCount() / 2);
      if(!camSuccess) {// the camera was unable to find the planet at the center of the image
        double lat, lon;
        // find values for lat/lon directly below spacecraft
        cam->subSpacecraftPoint(lat, lon);
        // use these values to set the ground coordinates
        cam->SetUniversalGround(lat, lon);
      }
      distFromSun = cam->SolarDistance();
    }
    catch(IException &e) { // unable to get solar distance, can't divide by efficiency, stop calibration
      throw IException(IException::Unknown,
                       "Unable to calibrate image using I/F. Cannot calculate Solar Distance using Isis::Camera object.",
                       _FILEINFO_);
    }
    if(distFromSun <= 0) { // solar distance <= 0, can't divide by efficiency, stop calibration
      throw IException(IException::Unknown,
                       "Unable to calibrate image using I/F. Solar Distance calculated is less than or equal to 0.",
                       _FILEINFO_);
    }
    gbl::calgrp += PvlKeyword("SolarDistance", Isis::toString(distFromSun));

    // read spectral file to find wavelength and flux
    CisscalFile *spectral = new CisscalFile(QString::fromStdString(specfile.original()));
    vector<double> wavelengthF, flux;
    for(int i = 0; i < spectral->LineCount(); i++) {
      QString line;
      spectral->GetLine(line);  //assigns value to line
      line = line.simplified().trimmed();
      if(line == "") {
        break;
      }
      x = line.split(" ").first().toDouble() / angstromsToNm;
      y = line.split(" ").last().toDouble() * angstromsToNm;
      wavelengthF.push_back(x);
      flux.push_back(y);
      lambda.push_back(x);
    }
    spectral->Close();
    // wavelength is in descending order, reverse to ascending order
    if(wavelengthF[0] > wavelengthF.back()) {
      reverse(wavelengthF.begin(), wavelengthF.end());
      reverse(flux.begin(), flux.end());
    }
    // Create Linear Approximation
    NumericalApproximation newflux(NumericalApproximation::Linear);
    for(unsigned int i = 0; i < flux.size(); i++) {
      newflux.AddData(wavelengthF[i], flux[i]);
    }

    // lambda domain min is the largest of the minimum wavelength values (rounded up)
    minlam = ceil(max(wavelengthF.front(), max(wavelengthT.front(), wavelengthQE.front())));
    // lambda domain max is the smallest of the maximum wavelength values (rounded down)
    maxlam = floor(min(wavelengthF.back(), min(wavelengthT.back(), wavelengthQE.back())));

    // NumericalApproximation requires lambda to be sorted
    sort(lambda.begin(), lambda.end());
    // NumericalApproximation requires lambda to be unique
    vector<double>::iterator it = unique(lambda.begin(), lambda.end());
    lambda.resize(it - lambda.begin());

    // remove any values that fall below minlam
    while(lambda[0] < minlam) {
      lambda.erase(lambda.begin());
    }

    // remove any values that fall above maxlam
    while(lambda[lambda.size()-1] > maxlam) {
      lambda.erase(lambda.end() - 1);
    }

    for(unsigned int i = 0; i < lambda.size(); i++) {
      double a = newtrans.Evaluate(lambda[i]);
      double b = newqecorr.Evaluate(lambda[i]);
      double c = newflux.Evaluate(lambda[i]) / (Isis::PI * pow(distFromSun, 2.0));
      fluxproduct1.push_back(a * b * c);
      fluxproduct2.push_back(a * b);
    }
  }

  //--- 4) CALCULATE EFFICIENCY FACTOR AND TOTAL EFFICIENCY -------------------
  //---    USING LAMBDA AND FLUX PRODUCTS -------------------------------------
  NumericalApproximation spline1(NumericalApproximation::CubicNatural);
  NumericalApproximation spline2(NumericalApproximation::CubicNatural);
  spline1.AddData(lambda, fluxproduct1);
  spline2.AddData(lambda, fluxproduct2);
  gbl::efficiencyFactor = spline1.BoolesRule(spline1.DomainMinimum(), spline1.DomainMaximum());
  double efficiency = spline2.BoolesRule(spline2.DomainMinimum(), spline2.DomainMaximum());
  gbl::calgrp += PvlKeyword("EfficiencyFactor", Isis::toString(gbl::efficiencyFactor), units.toStdString());
  gbl::calgrp += PvlKeyword("TotalEfficiency", Isis::toString(efficiency));

  // Cannot divide by 0.0
  if(gbl::efficiencyFactor == 0) {
    throw IException(IException::Unknown,
                     "Unable to calibrate image using I/F.  Cannot divide by efficiency factor of 0.",
                     _FILEINFO_);
  }
  return;
}


//=====End DN to Flux Methods====================================================================//


//=====2 Correction Factors Methods================================================================//

/**
 *  This method is modelled after IDL CISSCAL's
 *  cassimg_correctionfactors.pro.  The purpose is to find the
 *  correction factor, i.e. the value used to correct the image
 *  for ad-hoc factors.
 *
 * @internal
 *   @history 2008-11-05 Jeannie Walldren - Original version
 *   @history 2009-05-27 Jeannie Walldren - Renamed from
 *            FindCorrectionFactor since code was added to find
 *            the polarization correction factor, when
 *            available.
 *   @history 2017-06-08 Cole Neubauer - removed polarization correcton
 *            factor and added Jupiter correction factor to match 3.8 update
 *   @history 2019-08--14 Kaitlyn Lee - Removed Jupiter correction factor
 *            since it was removed in the 3.9.1 update.
 */
void gbl::FindCorrectionFactors() {
  // Disable correction factor if ShutterStateId is Disabled
  if(gbl::cissLab->ShutterStateId() == "Disabled") {
    gbl::calgrp += PvlKeyword("CorrectionFactorPerformed", "No: ShutterStateId is Disabled.");
    gbl::calgrp.findKeyword("CorrectionFactorPerformed").addComment("Correction Factor Parameters");
    gbl::calgrp += PvlKeyword("CorrectionFactorFile", "Not applicable: No correction factions.");
    return;
  }

  QString filter1 = gbl::cissLab->FilterName()[0];
  QString filter2 = gbl::cissLab->FilterName()[1];
  if(filter1 == "IRP0" || filter1 == "P120" || filter1 == "P60" || filter1 == "P0"
      || filter2 == "IRP90" || filter2 == "IRP0") {
    if(gbl::cissLab->InstrumentId() == "ISSNA") {
      filter1 = "CL1";
    }
    if(gbl::cissLab->InstrumentId() == "ISSWA") {
      filter2 = "CL2";
   }
  }
  // First Apply Standard Correction Factors
  // Get the directory where the CISS calibration directories are.
  FileName correctionFactorFile(gbl::GetCalibrationDirectory("correction").toStdString() + "correctionfactors_qecorr.tab");
  if(!correctionFactorFile.fileExists()) { // correction factor file not found, stop calibration
    throw IException(IException::Io,
                     "Unable to calibrate image. CorrectionFactorFile ***"
                     + correctionFactorFile.expanded() + "*** not found.", _FILEINFO_);
  }
  gbl::calgrp += PvlKeyword("CorrectionFactorPerformed", "Yes");
  gbl::calgrp.findKeyword("CorrectionFactorPerformed").addComment("Correction Factor Parameters");

  gbl::calgrp += PvlKeyword("CorrectionFactorFile", correctionFactorFile.original());
  CisscalFile *corrFact = new CisscalFile(QString::fromStdString(correctionFactorFile.original()));
  gbl::correctionFactor = 0.0;
  QString col1, col2, col3, col4;
  for(int i = 0; i < corrFact->LineCount(); i++) {
    QString line;
    corrFact->GetLine(line);  //assigns value to line
    line = line.simplified().trimmed();
    QStringList cols = line.split(" ");
    col1 = cols.takeFirst();
    if(col1 == gbl::cissLab->InstrumentId()) {
      col2 = cols.takeFirst();
      if(col2 == filter1) {
        col3 = cols.takeFirst();
        if(col3 == filter2) {
          col4 = cols.takeFirst();
          if(col4 == "") {
            gbl::correctionFactor = 1.0;
            // dividing by correction factor of 1.0 implies this correction is not performed
            gbl::calgrp.findKeyword("CorrectionFactorPerformed").setValue("No: CorrectionFactorFile contained no factor for filter combination");
          }
          else {
            gbl::correctionFactor = col4.toDouble();
          }
          break;
        }
        else {
          continue;
        }
      }
      else {
        continue;
      }
    }
    else {
      continue;
    }
  }
  corrFact->Close();

  // if no factor was found for instrument ID and filter combination
  if(gbl::correctionFactor == 0.0) {
    gbl::correctionFactor = 1.0;
    // dividing by correction factor of 1.0 implies this correction is not performed
    gbl::calgrp.findKeyword("CorrectionFactorPerformed").setValue("No: CorrectionFactorFile contained no factor for filter combination");
    gbl::calgrp.findKeyword("CorrectionFactorPerformed").addComment("Correction Factor Parameters");

  }
  gbl::calgrp += PvlKeyword("CorrectionFactor", Isis::toString(gbl::correctionFactor));
  return;
}


/**
 *  This method is modelled after IDL CISSCAL's
 *  cassimg_sensvstime.pro. IDL documentation:
 *    Sensitivity vs. time correction derived from stellar photometry:
 *
 *    NAC, all data (~8% total decline from S03 to S100):
 *      slope = -1.89457e-10
 *
 *    WAC, all data (~3% total decline from S17 to S100):
 *      slope = -9.28360e-11
 *
 * @internal
 *   @history 2019-08-14 Kaitlyn Lee - Original version
 */
void gbl::FindSensitivityCorrection() {
  if(gbl::cissLab->ShutterStateId() == "Disabled") {
    gbl::calgrp += PvlKeyword("SensitivityCorrectionPerformed", "No: ShutterStateId is Disabled.");
    gbl::sensCorrection = false;
    gbl::calgrp += PvlKeyword("SensVsTimeCorr", "Not applicable: No Sensitivity correction.");
    return;
  }
  double imgNumber = gbl::cissLab->ImageNumber();
  // Values taken from IDL
  double imgNumberS03 = 1.47036e9;
  double imgNumberS17 = 1.51463e9;

  if(gbl::cissLab->InstrumentId() == "ISSNA" && imgNumber < imgNumberS03) {
    gbl::calgrp += PvlKeyword("SensitivityCorrectionPerformed", "No: No NAC correction before S03");
    gbl::sensCorrection = false;
    gbl::calgrp += PvlKeyword("SensVsTimeCorr", "Not applicable: No NAC correction before S03");
    return;
  }
  if(gbl::cissLab->InstrumentId() == "ISSWA" && imgNumber < imgNumberS17) {
    gbl::calgrp += PvlKeyword("SensitivityCorrectionPerformed", "No: No WAC correction before S17");
    gbl::sensCorrection = false;
    gbl::calgrp += PvlKeyword("SensVsTimeCorr", "Not applicable: No WAC correction before S17");
    return;
  }

  if(gbl::cissLab->InstrumentId() == "ISSNA") {
    gbl::sensVsTimeCorr = 1.0 + (1.89457e-10 * (imgNumber - imgNumberS03));
  }
  else if(gbl::cissLab->InstrumentId() == "ISSWA") {
    gbl::sensVsTimeCorr = 1.0 + (9.28360e-11 * (imgNumber - imgNumberS17));
  }
  gbl::calgrp += PvlKeyword("SensitivityCorrectionPerformed", "Yes");
  gbl::calgrp.findKeyword("SensitivityCorrectionPerformed").addComment("Sensitivity vs Time Correction Parameters");
  gbl::sensCorrection = true;
  gbl::calgrp += PvlKeyword("SensVsTimeCorr", Isis::toString(gbl::sensVsTimeCorr));
}


//=====End Correction Factor Methods=============================================================//

/**
 * This method returns an QString containing the path of a
 * Cassini calibration directory
 *
 * @param calibrationType
 * @return <b>IString</b> Path of the calibration directory
 *
 * @internal
 *   @history 2008-11-05 Jeannie Walldren - Original version
 */
QString gbl::GetCalibrationDirectory(QString calibrationType) {
  // Get the directory where the CISS calibration directories are.
  PvlGroup &dataDir = Preference::Preferences().findGroup("DataDirectory");
  QString missionDir = QString::fromStdString(dataDir["Cassini"]);
  return missionDir + "/calibration/" + calibrationType + "/";
}
