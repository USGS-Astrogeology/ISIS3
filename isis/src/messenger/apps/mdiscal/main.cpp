/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

// $Id: mdiscal.cpp 6715 2016-04-28 17:58:43Z tsucharski@GS.DOI.NET $
#include "Isis.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

#include <QString>

#include "DarkModelPixel.h"
#include "MdisCalUtils.h"
#include "MultivariateStatistics.h"
#include "ProcessByLine.h"
#include "ProgramLauncher.h"
#include "Spice.h"
#include "Statistics.h"
#include "TextFile.h"

using namespace Isis;
using namespace std;


// Global variables
// dark current
/**
 * Enumeration to determine the type of dark current correction to be applied,
 * if any.
 */
enum MdisDarkCurrentMode {
  DarkCurrentNone,     //!< No dark current correction applied.
  DarkCurrentStandard, //!< Standard dark current correction applied.
  DarkCurrentLinear,   //!< Linear dark current correction applied.
  DarkCurrentModel     //!< Model dark current correction applied.
};
MdisDarkCurrentMode g_darkCurrentMode;
bool g_convertDarkToNull;
bool g_isNarrowAngleCamera;
bool g_isBinnedData;
double g_exposureDuration;
double g_ccdTemperature;
int g_filterNumber;
Pvl g_configFile;
Statistics g_darkStrip;
vector<double> g_calibrationValues;
vector<double> g_prevLineData;
vector<double> g_smearData;
// static global variables
static int g_nDarkColumns = 0;
static int g_nValidDark = 0;
static int g_nSampsToNull = 0;
static double g_smearComponent = 3.4;
// Empirical correction factor...
// NOTE: Names of variables/methods related to this correction were changed
// from event or contamination event to empirical correction as requested by
// the Messenger mission since it is a long-lived problem and they don't know
// the cause at this time.
static double g_empiricalCorrectionFactor = 1.0;
// Limit functionality for aiding dark analysis
static bool g_applyFlatfield = true;
static bool g_applyRadiometric = true;
// Absolute coefficents
static double g_absCoef = 1.0;
// I/F variables
static double g_solarDist = 1.0;
static double g_Ff = 1.0;
static double g_iof = 1.0;   //!< I/F value for observation
static DarkModelPixel* g_model = 0;


// Local function prototypes
FileName determineFlatFieldFile();
void gatherDarkStatistics(Buffer& in);
void calibrate(vector<Buffer *>& in, vector<Buffer *>& out);

void IsisMain() {

  const QString mdiscalProgram = "mdiscal";
  // 2015-09-02 Jeannie Backer - Increased cdr version to 6 since we added a new parameter, ECFACTOR
  const QString mdiscalVersion = "1.6";
  const QString mdiscalRevision = "$Revision: 6715 $";
  QString mdiscalRuntime = Application::DateTime();

  // Specify the version of the CDR generated
  // 2015-09-02 Jeannie Backer - Increased cdr version to 5
  const int cdrVersion = 5;


  // We will be processing by column in case of a linear dark current fit. This will make the
  // calibration a one pass system in this case, rather than two.
  ProcessByLine p;
  FileName calibFile("$messenger/calibration/mdisCalibration????.trn");
  calibFile = calibFile.highestVersion();
  g_configFile.read(calibFile.expanded());

  // Initialize variables
  g_calibrationValues.clear();
  g_prevLineData.clear();
  g_convertDarkToNull = true;
  g_isNarrowAngleCamera = true;
  g_isBinnedData = true;
  g_darkCurrentMode = (MdisDarkCurrentMode) - 1;
  g_exposureDuration = 0.0;
  g_ccdTemperature = 0.0; // This needs figured out!
  g_filterNumber = 1;  // Sufficent for the NAC!
  g_model = 0;
  g_empiricalCorrectionFactor = 1.0;

  Cube *icube = p.SetInputCube("FROM");
  PvlGroup &inst = icube->group("Instrument");
  g_isNarrowAngleCamera = ((QString)inst["InstrumentId"] == "MDIS-NAC");
  g_exposureDuration = inst["ExposureDuration"];
  g_exposureDuration /= 1000.0; // convert from milliseconds to seconds

  // Determine dark columns
  int fpuBin = inst["FpuBinningMode"];
  int pxlBin = inst["PixelBinningMode"];

  g_nDarkColumns = 4 / (fpuBin + 1);    // DPU binning gives 2 dark cols
  if (pxlBin > 2) g_nDarkColumns = 0;   // MP binning > 2x2 yields no darks
  else if (pxlBin > 0) g_nDarkColumns /= (pxlBin + 1); // Might be 1 if wo/DPU + MP 2x2

  // Determine number of valid darks.  For no binning will have 3.  All combos
  // that have 2x2 total binning will give 1 valid dark.  All other options
  // have no valid dark columns.
  g_nValidDark = std::min(g_nDarkColumns, 3);
  if (g_nValidDark < 3) {
    if ((fpuBin + pxlBin) > 1) g_nValidDark = 0;
    else g_nValidDark = std::min(g_nValidDark, 1);
  }

  // Determine number of samples/columns to NULL.  For no binning it will yield 4
  // columns to NULL.  For DPU but no MP binning, 3;  For no DPU but MP binning,
  // 2x2 yields 3, 4x4 and 8x8 yields 1.
  g_nSampsToNull = (pxlBin < 2) ? 0 : ((pxlBin > 2) ? 1 : 3);  // Only MP here
  g_nSampsToNull = std::min(std::max(g_nDarkColumns + 1, g_nSampsToNull), 4);  // No more than 4!
  g_darkStrip.Reset();

  g_ccdTemperature = icube->group("Archive")["CCDTemperature"];

  // Binned data only applies to FPUBIN mode.  Pixel binning must be dealt
  // with specially in other calibration support components
  g_isBinnedData = (fpuBin == 1);

  // Get the trusted filter number
  if (!g_isNarrowAngleCamera) {
    g_filterNumber = ((int)(icube->group("BandBin")["Number"])) - 1;
  }
  else {
    g_filterNumber = 1;  // For the NAC
  }

  UserInterface& ui = Application::GetUserInterface();
  g_convertDarkToNull = !ui.GetBoolean("KEEPDARK");
  if (!g_convertDarkToNull) g_nSampsToNull = 0;


  QString darkCurr = ui.GetString("DARKCURRENT");
  g_applyFlatfield = ui.GetBoolean("FLATFIELD");
  g_applyRadiometric = ui.GetBoolean("RADIOMETRIC");

  if (icube->bandCount() != 1) {
    throw IException(IException::User,
                     "MDIS images may only contain one band", _FILEINFO_);
  }

  if (icube->sampleCount() < 3) {
    QString msg = "Unable to obtain dark current data. Expected a sample dimension of at least 3";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  if ((int)icube->group("Instrument")["Unlutted"] == false) {// Unlutted == 0
    throw IException(IException::User,
                     "Calibration can only be performed on unlutted data.", _FILEINFO_);
  }


  // Check for cases where certain g_models cannot be computed.
  // These would be for cases where more than two factors of compression
  // occur.  For this case, only the g_model can be used and only if the
  // exposure time < 2 secs.
  if (g_nValidDark <= 0) {
    // Both cases require dark pixels, g_model does not
    if ((darkCurr == "STANDARD") || (darkCurr == "LINEAR")) {
      QString mess = "Warning: There are no valid dark current pixels which are required"
                     " for " + darkCurr + " calibration. Changing dark correction method to MODEL.";
      IException ie(IException::User, mess, _FILEINFO_);
      ie.print();
      darkCurr = "MODEL";
    }

    // Model cannot be used for exposure times > 1.0 <sec>
    if ((darkCurr == "MODEL") && (g_exposureDuration > 1.0)) {
      darkCurr = "NONE";
      QString mess = "Warning: There are no valid dark current pixels and the dark model correction"
                     " can not be used when the exposure duration exceeds 1000 milliseconds."
                     " Changing dark correction method to NONE.";
      IException ie(IException::User, mess, _FILEINFO_);
      ie.print();
    }
  }

  unique_ptr<DarkModelPixel> darkModel;
  if (darkCurr == "NONE") {
    g_darkCurrentMode = DarkCurrentNone;
  }
  else if (darkCurr == "STANDARD") {
    g_darkCurrentMode = DarkCurrentStandard;
    g_calibrationValues.resize(icube->lineCount());
  }
  else if (darkCurr == "LINEAR") {
    g_darkCurrentMode = DarkCurrentLinear;
    g_calibrationValues.resize(icube->lineCount());
  }
  else {//if (darkCurr == "MODEL") { ...in this case g_nValidDark > 0
    if (g_exposureDuration > 1.0) {
      // set processing to standard
      g_darkCurrentMode = DarkCurrentLinear;
      g_calibrationValues.resize(icube->lineCount());
      darkCurr = "STANDARD";

      QString mess = "Warning: Dark model correction can not be used when the"
                     " exposure duration exceeds 1000 milliseconds."
                     " Changing dark correction method to STANDARD.";
      IException ie(IException::User, mess, _FILEINFO_);
      ie.print();

    }
    else {
      g_darkCurrentMode = DarkCurrentModel;
    }
  }

  QString darkCurrentFile = "";
  if (g_darkCurrentMode != DarkCurrentNone) {
    if (g_darkCurrentMode != DarkCurrentModel) {
      p.Progress()->SetText("Gathering Dark Current Statistics");
      p.StartProcess(gatherDarkStatistics);
    }
    else {
      // read in dark current table variables and report the filename used
      darkModel = unique_ptr<DarkModelPixel>(new DarkModelPixel(pxlBin,
                                                              g_ccdTemperature,
                                                              g_exposureDuration));
      darkCurrentFile = darkModel->loadCoefficients(g_isNarrowAngleCamera, g_isBinnedData);
      g_model = darkModel.get();
    }
  }

  // We need to figure out our flat-field file
  if (g_darkCurrentMode == DarkCurrentLinear) {
    // We need to perform a linear regression with our data,
    // convert statistics to a line.
    double *xdata = new double[g_calibrationValues.size()];
    double *ydata = new double[g_calibrationValues.size()];

    for (unsigned int x = 0; x < g_calibrationValues.size(); x++) {
      xdata[x] = x;
      ydata[x] = g_calibrationValues[x];
    }

    // Perform a regression
    MultivariateStatistics stats;
    stats.AddData(xdata, ydata, g_calibrationValues.size());

    // y = A + Bx
    double a, b;
    stats.LinearRegression(a, b);
    delete[] xdata;
    delete[] ydata;

    // Store a,b in calibration data instead of our line
    g_calibrationValues.resize(2);
    g_calibrationValues[0] = a;
    g_calibrationValues[1] = b;
  }

  // Compute the (new) absolute calibration
  QString respfile = "";
  vector<double> rsp = loadResponsivity(g_isNarrowAngleCamera, g_isBinnedData,
                                        g_filterNumber + 1, respfile);
  // g_absCoef = 1.0 / (rsp[0] * ((rsp[2] * g_ccdTemperature) + rsp[1]));
  double t = 1.0;
  double rt = rsp[0];
  double response = 0;
  for (unsigned int i = 1; i < rsp.size(); i++) {
    response += rt * (rsp[i] * t);
    t *= g_ccdTemperature;
  }
  g_absCoef = 1.0 / response;

  // Retrieve filter dependant SMEAR component
  QString smearfile = "";
  g_smearComponent = loadSmearComponent(g_isNarrowAngleCamera, g_filterNumber + 1,
                                        smearfile);

  // Get s/c clock count
  QString startTime = inst["SpacecraftClockCount"];

  // Retrieve empirical correction parameter
  QString empiricalCorrectionFile   = "";
  QString empiricalCorrectionDate   = "";
  QString empiricalCorrectionFactor = "";
  bool applyECFactor = ui.GetBoolean("ECFACTOR") && !g_isNarrowAngleCamera;// Not applicable to NAC
  if (applyECFactor) {  // Get correction for WAC filters
    empiricalCorrectionFile = "";
    g_empiricalCorrectionFactor = loadEmpiricalCorrection(startTime, g_filterNumber + 1,
                                                          empiricalCorrectionFile,
                                                          empiricalCorrectionDate, icube);
    empiricalCorrectionFactor = toString(g_empiricalCorrectionFactor);
  }
  else {
    // already initialized g_empiricalCorrectionFactor = 1.0;
    empiricalCorrectionFile   = "N/A";
    empiricalCorrectionDate   = "N/A";
    empiricalCorrectionFactor = "N/A";
  }

  // Compute I/F if requested by user
  g_iof = 1.0;
  bool applyIOF = ui.GetBoolean("IOF");
  if (!g_applyRadiometric) applyIOF = false;
  bool validIOF = false;
  QString solirrfile = "";
  if (applyIOF) {
    PvlGroup& inst = icube->group("Instrument");
    QString target = inst["TargetName"];
    QString startTime = inst["SpacecraftClockCount"];
    if (sunDistanceAU(startTime, target, g_solarDist, icube)) {
      vector<double> sol = loadSolarIrr(g_isNarrowAngleCamera, g_isBinnedData,
                                        g_filterNumber + 1, solirrfile);
      g_Ff = sol[2];
      g_iof = pi_c() * (g_solarDist * g_solarDist) / g_Ff;
      validIOF = true;
    }
    else {
      // already set g_iof = 1.0;
      validIOF = false;
    }
  }

  // Determine if we need to subsample the flat field should pixel binning
  // occurred
  QString reducedFlat = "";
  FileName flatfield = determineFlatFieldFile();
  if (pxlBin > 0) {
    QString scale(toString(pxlBin));
    FileName newflat = FileName::createTempFile("$temporary/"
                                                + flatfield.baseName() + "_reduced.cub");
    reducedFlat = newflat.expanded();
    QString parameters = "FROM=" + flatfield.expanded() +
       " TO="   + newflat.expanded() +
       " MODE=SCALE" +
       " LSCALE=" + scale +
       " SSCALE=" + scale;
    try {
      // iApp->Exec("reduce", parameters);
      ProgramLauncher::RunIsisProgram("reduce", parameters);
      reducedFlat = newflat.expanded();
    }
    catch (IException&) {
      remove(reducedFlat.toLatin1().data());
      throw;
    }
    CubeAttributeInput att;
    p.SetInputCube(reducedFlat, att);
  }
  else {
    CubeAttributeInput att;
    p.SetInputCube(flatfield.expanded(), att);
  }

  // Set output file for processing
  Cube *ocube = p.SetOutputCube("TO");

  try {
    p.Progress()->SetText("Calibrating MDIS Cube");
    p.StartProcess(calibrate);
  }
  catch (...) {
    if (!reducedFlat.isEmpty()) remove(reducedFlat.toLatin1().data());
    throw;
  }

  // Remove the temporary reduced input file if generated
  if (!reducedFlat.isEmpty()) remove(reducedFlat.toLatin1().data());

  // Log calibration activity
  PvlGroup calibrationLog("RadiometricCalibration");
  calibrationLog.addKeyword(PvlKeyword("SoftwareName", mdiscalProgram));
  calibrationLog.addKeyword(PvlKeyword("SoftwareVersion", mdiscalVersion));
  calibrationLog.addKeyword(PvlKeyword("ProcessDate", mdiscalRuntime));
  calibrationLog.addKeyword(PvlKeyword("DarkCurrentModel", darkCurr));

  if (g_darkCurrentMode == DarkCurrentLinear) {
    QString equation = "Y = " + toString(g_calibrationValues[0]) + QString(" + ")
                       + toString(g_calibrationValues[1]) + QString("x");
    calibrationLog.addKeyword(PvlKeyword("DarkCurrentEquation", (QString)equation));
  }
  else if (g_darkCurrentMode == DarkCurrentModel) {
    calibrationLog.addKeyword(PvlKeyword("DarkCurrentFile", darkCurrentFile));
  }

  calibrationLog.addKeyword(PvlKeyword("BinnedImage", toString((int)g_isBinnedData)));
  calibrationLog.addKeyword(PvlKeyword("FilterNumber", toString(g_filterNumber + 1)));
  if (g_applyFlatfield) {
    calibrationLog.addKeyword(PvlKeyword("FlatFieldFile",
                                         flatfield.originalPath() + "/" + flatfield.name()));
  }
  else {
    calibrationLog.addKeyword(PvlKeyword("FlatFieldFile", "N/A"));
  }
  calibrationLog.addKeyword(PvlKeyword("CalibrationFile",
                                       calibFile.originalPath() + "/" + calibFile.name()));
  calibrationLog.addKeyword(PvlKeyword("ResponsivityFile", respfile));
  calibrationLog.addKeyword(PvlKeyword("SmearCompFile", smearfile));
  PvlKeyword rspKey("Response", toString(rsp[0]));
  for (unsigned int i = 1; i < rsp.size(); i++) {
    rspKey.addValue(std::to_string(rsp[i]));
  }
  calibrationLog.addKeyword(rspKey);
  calibrationLog.addKeyword(PvlKeyword("SmearComponent", toString(g_smearComponent)));

  QString calibType;
  if (applyIOF  && validIOF) {
    calibrationLog.addKeyword(PvlKeyword("Units", "I over F"));
    calibrationLog.addKeyword(PvlKeyword("SolarDistance", toString(g_solarDist), "AU"));
    calibrationLog.addKeyword(PvlKeyword("SolarIrrFile", solirrfile));
    calibrationLog.addKeyword(PvlKeyword("FilterIrradianceFactor", toString(g_Ff)));
    calibrationLog.addKeyword(PvlKeyword("IOFFactor", toString(g_iof)));
    calibType = "IF";
  }
  else if (g_applyRadiometric) {
    calibrationLog.addKeyword(PvlKeyword("Units", "W / (m**2 micrometer sr)"));
    calibType = "RA";
  }
  else {
    calibrationLog.addKeyword(PvlKeyword("Units", "DN"));
    calibType = "DN";
  }

  calibrationLog.addKeyword(PvlKeyword("EmpiricalCorrectionFile", empiricalCorrectionFile));
  calibrationLog.addKeyword(PvlKeyword("EmpiricalCorrectionDate", empiricalCorrectionDate));
  calibrationLog.addKeyword(PvlKeyword("EmpiricalCorrectionFactor", empiricalCorrectionFactor));


  calibrationLog.addKeyword(PvlKeyword("DarkStripColumns", toString(g_nDarkColumns)),
                            Pvl::Replace);
  calibrationLog.addKeyword(PvlKeyword("ValidDarkColumns", toString(g_nValidDark)),
                            Pvl::Replace);
  if (g_darkStrip.TotalPixels() > 0) {
    double avgDark = (g_darkStrip.ValidPixels() > 0) ? g_darkStrip.Average() : 0.0;
    calibrationLog.addKeyword(PvlKeyword("DarkStripMean", toString(avgDark)),
                                         Pvl::Replace);
  }

  // Report nulled sample count
  calibrationLog.addKeyword(PvlKeyword("LeftSamplesNulled", toString(g_nSampsToNull)));

  // Handle updates of ProductId and SourceProduct Id keywords
  PvlGroup& archive = ocube->group("Archive");
  PvlKeyword key = archive["ProductId"];
  QString orgProdId = key[0];
  QString newProdId = orgProdId + "_" + calibType + "_" + toString(cdrVersion);
  newProdId[0] = 'C';
  key.setValue(quote(newProdId));
  archive.addKeyword(key, Pvl::Replace);

  // Now SourceProductId
  if (archive.hasKeyword("SourceProductId")) {
    key = archive["SourceProductId"];
    for (int i = 0; i < key.size(); i++) {
      key[i] = quote(key[i]);
    }
  }
  else {
    key = PvlKeyword("SourceProductId", quote(orgProdId));
  }

  if (!darkCurrentFile.isEmpty()) {
    key.addValue(quote(FileName(darkCurrentFile).baseName()));
  }
  key.addValue(quote(flatfield.baseName()));
  key.addValue(quote(FileName(respfile).baseName()));
  // key.addValue(quote(FileName(smearfile).baseName()));
  if (validIOF) {
    key.addValue(quote(FileName(solirrfile).baseName()));
  }
  archive.addKeyword(key, Pvl::Replace);

  // Write Calibration group to file and log
  ocube->putGroup(calibrationLog);
  Application::Log(calibrationLog);
//  g_configFile.clear();
}

FileName determineFlatFieldFile() {
  QString filename = "$messenger/calibration/FLAT/";

  // FileName consists of binned/notbinned, camera, and filter
  filename += "MDIS";
  filename += ((g_isNarrowAngleCamera) ? "NAC" : "WAC");
  filename += ((g_isBinnedData) ? "_BINNED_" : "_NOTBIN_");
  filename += "FLAT";
  if (g_isNarrowAngleCamera) {
    // NAC spec is simpler
    filename += "_?.cub";
  }
  else {
    // add a zero if the filter is 1-digit
    filename += "_FIL";
    if (g_filterNumber < 9) filename += "0";
    filename += toString(g_filterNumber + 1);
    filename += "_?.cub";
  }

  FileName final(filename);
  final = final.highestVersion();
  return final;
}


void gatherDarkStatistics(Buffer& in) {

  // Some situations cannot use these processes for calibration
  g_calibrationValues[in.Line() - 1] = Isis::Null;

  if (g_nValidDark > 0) {
    if (g_darkCurrentMode == DarkCurrentStandard) {
      // Figure out the median (Isis::Statistics wont do this for us)
      // because we have repeated numbers, put them into a sorted array size
      // of no more than 3 and take the middle
      vector<double> calibValues;
      int nDark = g_nValidDark;
      for (int i = 0; i < nDark; i++) {
        calibValues.push_back(in[i]);
      }
      sort(calibValues.begin(), calibValues.end());

      // grab the middle element in the array for the median
      g_calibrationValues[in.Line() - 1] = calibValues[nDark / 2];
    }
    else if (g_darkCurrentMode == DarkCurrentLinear) {
      // Presently the linear regression only uses the first sample in the
      // dark current data
      g_calibrationValues[in.Line() - 1] = in[0];
    }
  }
}

void calibrate(vector<Buffer *>& in, vector<Buffer *>& out) {
  Buffer& imageIn   = *in[0];
  Buffer& flatField = *in[1];
  Buffer& imageOut  = *out[0];
  double t2 = g_smearComponent / imageIn.SampleDimension();
  //g_exposureDuration is in seconds, but we need to work in ms.
  double exposureTime = g_exposureDuration * 1000.0;

  if (imageIn.Line() == 1) {
    g_prevLineData.resize(imageIn.SampleDimension());
    g_smearData.resize(imageIn.SampleDimension());
    double added = 16.0*t2/exposureTime;

    for (unsigned int i = 0; i < g_prevLineData.size(); i++) {
      g_prevLineData[i] = added;
      g_smearData[i] = 0.0;
    }
  }

  for (int i = 0; i < imageIn.size(); i++) {

    // Check for special pixel in input image and pass through
    if (Isis::IsSpecial(imageIn[i])) {
      imageOut[i] = imageIn[i];
      continue;
    }

    if (g_applyFlatfield) {
      // If the flat field pixel is special, can't calibrate so set to NULL
      // and pass through (unlikely).
      if (Isis::IsSpecial(flatField[i])) {
        imageOut[i] = Isis::Null;
        continue;
      }
    }
    //462
    //if (i == 25 && imageIn.Line() == 25) std::cout <<  "In: " << imageIn[i] << std::endl;

    // Step 1: Perform dark current corrections
    if (g_darkCurrentMode == DarkCurrentNone) {
      imageOut[i] = imageIn[i];
    }
    else if (g_darkCurrentMode == DarkCurrentStandard) {
      imageOut[i] = imageIn[i] - g_calibrationValues[imageIn.Line() - 1];
    }
    else if (g_darkCurrentMode == DarkCurrentLinear) {
      // Linear: out = in - bestfitline = in - (A + Bx)
      imageOut[i] = imageIn[i] - (g_calibrationValues[0] + g_calibrationValues[1] *
                                  (imageIn.Line() - 1));
    }
    else if (g_darkCurrentMode == DarkCurrentModel) {
      imageOut[i] = imageIn[i] - g_model->getDarkPixel(i, imageIn.Line() - 1);
    }

    // Step 2: Perform linearity correction
    if (g_isNarrowAngleCamera == true) {
      if (imageOut[i] <= 0.0) {
        imageOut[i] /= 0.912031;
      }
      else {
        imageOut[i] /= 0.011844 * log10(imageOut[i]) + 0.912031;
      }
    }
    else {
      // Wide angle camera
      if (imageOut[i] <= 0.0) {
        imageOut[i] /= 0.936321;
      }
      else {
        imageOut[i] /= 0.008760 * log10(imageOut[i]) + 0.936321;
      }
    }

    // Step 3: Readout Smear Correction (ms -> seconds)
    if (exposureTime > 0.0) {
      g_smearData[i] += t2 / exposureTime * g_prevLineData[i];
      imageOut[i] -= g_smearData[i];
    }

    g_prevLineData[i] = imageOut[i];

    // Step 4: Uniformity (flat field)
    if (g_applyFlatfield) {
      imageOut[i] /= flatField[i]; // divide by flat field
    }  // End of flat field

    // Step 5: Absolute coefficient
    // Using g_exposureDuration (in seconds). This gives ~ the same results
    // as the previous version of mdiscal did. Using exposureTime gives
    // a factor of 1000 smaller value, as one would expect.
    if (g_exposureDuration > 0.0) {
      imageOut[i] = imageOut[i] / g_exposureDuration * g_absCoef;
    }

    // Step 6:  Convert to I/F units
    // NOTE: if g_applyRadiometric is false or iof was not valid,
    //       then iof was set to 1.0, so no change
    imageOut[i] *= g_iof;

    // Step 7: Apply empirical correction factor
    // Apply empirical correction factor to final result w/o.r.f output units
    // NOTE: if empirical correction is false, then the EC factor was set to 1.0, so no change
    imageOut[i] *= g_empiricalCorrectionFactor;
  }

  // Compute dark current statistics
  for (int j = 0; j < g_nValidDark; j++) {
    g_darkStrip.AddData(imageOut[j]);
  }

  // Null specified columns (2011-04-20 - KJB)
  for (int n = 0; n < g_nSampsToNull; n++) {
    imageOut[n] = Isis::Null;
  }

}
