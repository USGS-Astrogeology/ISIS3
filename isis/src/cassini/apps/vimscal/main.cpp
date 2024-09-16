/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <cfloat>
#include <cmath>

#include <vector>
#include <stdio.h>

#include <QDebug>
#include <QFile>

#include "Camera.h"
#include "EndianSwapper.h"
#include "iTime.h"
#include "IException.h"
#include "IString.h"
#include "LeastSquares.h"
#include "LineManager.h"
#include "PolynomialUnivariate.h"
#include "ProcessByLine.h"
#include "ProcessByBrick.h"
#include "ProgramLauncher.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "Spice.h"
#include "Statistics.h"
#include "Table.h"
#include "UserInterface.h"

using namespace Isis;
using namespace std;

//! map from <sample, band> to dark correction value
map< pair<int, int>, double > sampleBasedDarkCorrections;

//! map from <line, band> to dark correction value
map< pair<int, int>, double > lineBasedDarkCorrections;

QString yearString;
QString timeString;

//! specific energy corrections for each band of the cube
vector<double> specificEnergyCorrections;

//! Bandwidth centers
vector<double> bandwidthVector;
vector<double> averageBandwidthVector;

//! list of temp files that need deleted
vector<QString> tempFiles;

//! solar remove coefficient
double solarRemoveCoefficient;

//! The calibration file containing multiplier information.
static Pvl g_configFile;

//! Calibration multipliers
static double g_solar(1.0);
static double g_ir(1.0);
static double g_vis(1.0);
static double g_wavecal(1.0);

//! The calibration version.  It's the name of the directory
//! in $cassini/calibration/vims where all of the radiometric
//! calibration cubes are kept.
QString calVersion;
QString g_startTime;

//! Output in I/F units
bool iof;

//!
bool g_visBool;

QString g_oCubeName;

void calculateDarkCurrent(Cube *);
void calculateVisDarkCurrent(Cube *);
void calculateIrDarkCurrent(Cube *);

void chooseFlatFile(Cube *, ProcessByLine *);

void calculateSpecificEnergy(Cube *);
void calculateSolarRemove(Cube *, ProcessByLine *);
void loadCalibrationValues();
void updateWavelengths(Cube *icube);


void normalize(Buffer &in,Buffer &out);
void normalize1(Buffer &out);
void calibrate(vector<Buffer *> &in, vector<Buffer *> &out);

QString createCroppedFile(Cube *icube, QString cubeFileName, bool flatFile = false);
void GetOffsets(const Pvl &lab, int &finalSampOffset, int &finalLineOffset);

// This is the results group
PvlGroup calibInfo;

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  //load the appropriate multipliers and the correct calibration version

  tempFiles.clear();
  specificEnergyCorrections.clear();
  bandwidthVector.clear();
  averageBandwidthVector.clear();
  sampleBasedDarkCorrections.clear();
  lineBasedDarkCorrections.clear();
  solarRemoveCoefficient = 1.0;
  iof = (ui.GetString("UNITS") == "IOF");

  calibInfo = PvlGroup("RadiometricCalibration");

  loadCalibrationValues();

  ProcessByLine p;
  Cube *icube = p.SetInputCube("FROM");
  PvlGroup &inst = icube->group("Instrument");


  g_visBool = (inst["Channel"][0] != "IR");

  bool isVims = true;

  try {
    isVims = (icube->label()->findGroup("Instrument",
              Pvl::Traverse)["InstrumentId"][0] == "VIMS");
  }
  catch(IException &e) {
    isVims = false;
  }


  try {
    timeString = QString::fromStdString(inst["StartTime"]);

  }
  catch(IException &e) {

    std::string msg = "The label for the input cube [" + ui.GetAsString("FROM").toStdString() +
        "] does not have a start time in the Instrument group.";
    throw IException(IException::User,msg,_FILEINFO_);

  }

  iTime startTime(timeString);
  //Determine the year string to access the appropriate calibration file
  yearString= QString::number(startTime.Year() );


  if(!isVims) {
    std::string msg = "The input cube [" + ui.GetAsString("FROM").toStdString() +
        "] is not a Cassini VIMS cube";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  if(icube->label()->findObject("IsisCube").hasGroup("AlphaCube")) {
    std::string msg = "The input cube [" + ui.GetAsString("FROM").toStdString()
        + "] has had its dimensions modified and can not be calibrated";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  calibInfo += PvlKeyword("OutputUnits", ((iof) ? "I/F" : "Specific Energy"));

  // done first since it's likely to cause an error if one exists
  calculateSolarRemove(icube, &p);

  if(ui.GetBoolean("DARK"))
    calculateDarkCurrent(icube);

  chooseFlatFile(icube, &p);
  calculateSpecificEnergy(icube);
  updateWavelengths(icube);

  Cube *outCube = p.SetOutputCube("TO");
  g_oCubeName = outCube->fileName();

  p.StartProcess(calibrate);


  outCube->putGroup(calibInfo);
  // Rename group to Results for writing to Log
  calibInfo.setName("Results");
  Application::Log(calibInfo);

  p.EndProcess();


  //This is used to create normalized spectral plots
#if 0
  ProcessByBrick p1;
  CubeAttributeInput iatt;
  CubeAttributeOutput oatt;
  p1.SetBrickSize(1,1,icube->bandCount());
  p1.SetInputCube(outCube);
  p1.SetOutputCube(g_oCubeName+"_norm.cub",oatt,outCube->sampleCount(),outCube->lineCount(),
                   outCube->bandCount());
  p1.StartProcess(normalize);
   p.EndProcess();
  p1.EndProcess();

#endif


  for(unsigned int i = 0; i < tempFiles.size(); i++) {
    QFile::remove(tempFiles[i]);
  }

  tempFiles.clear();
}


void normalize(Buffer &in,Buffer &out) {

  double normalizer = in[54];


        for (int i =0; i < in.size(); i++)  {

          if (!IsSpecial(in[i])) {

           out[i] = in[i]/normalizer;

          }
          else{
              out[i]=in[i];
          }
        }
}


/**
 * This applies the calculated calibration coefficients to the file.
 *
 * @param inBuffers
 * @param outBuffers
 */
void calibrate(vector<Buffer *> &inBuffers, vector<Buffer *> &outBuffers) {
  Buffer *inBuffer = inBuffers[0];
  Buffer *flatFieldBuffer = inBuffers[1];
  Buffer *solarRemoveBuffer = NULL; // this is optional


  if (inBuffers.size() > 2) {
    inBuffer = inBuffers[0];
    solarRemoveBuffer = inBuffers[1];
    flatFieldBuffer = inBuffers[2];
  }

  Buffer *outBuffer = outBuffers[0];

  for (int i = 0; i < inBuffer->size(); i++) {
    (*outBuffer)[i] = (*inBuffer)[i];

    if(IsSpecial((*outBuffer)[i])) continue;

    map< pair<int, int>, double>::iterator darkCorrection =
      sampleBasedDarkCorrections.find(pair<int, int>(i + 1, inBuffer->Band()));

    //Darkfield correction
    if (darkCorrection != sampleBasedDarkCorrections.end()) {
      (*outBuffer)[i] -= darkCorrection->second;
    }

    darkCorrection = lineBasedDarkCorrections.find(pair<int,
                                                   int>(inBuffer->Line(), inBuffer->Band()));


    if (darkCorrection != lineBasedDarkCorrections.end()) {
      if (!IsSpecial(darkCorrection->second))
        (*outBuffer)[i] -= darkCorrection->second;
      else
        (*outBuffer)[i] = Null;
    }

    //Flatfield correction
    if (!IsSpecial((*flatFieldBuffer)[i]) && !IsSpecial((*outBuffer)[i])) {
      (*outBuffer)[i] /= (*flatFieldBuffer)[i];
    }


    //(1) Convert from DN/sec to photons/sec using RC19
    //(2) Then convert from photons/sec to specific intensity
    if (inBuffer->Band() <= (int)specificEnergyCorrections.size() && !IsSpecial((*outBuffer)[i]))
    {
      (*outBuffer)[i] *= specificEnergyCorrections[inBuffer->Band()-1];
    }


    //Convert to I/F/  Equation (3) in the white paper
    if (iof && solarRemoveBuffer && !IsSpecial((*outBuffer)[i])) {
      (*outBuffer)[i] = (*outBuffer)[i] /( g_solar*(*solarRemoveBuffer)[i]
                                           / solarRemoveCoefficient) * Isis::PI  ;
    }

  }
}


/**
 * This calculates the values necessary to convert from
 * specific energy to I/F. A cube is used as part of the
 * equation (which probably just contains a vector of values)
 * so p->SetInputCube(...) will be called with the appropriate
 * filename.
 *
 * @param *icube A pointer to the input cube
 * @param *p A pointer to the ProcessByLine object
 */
void calculateSolarRemove(Cube *icube, ProcessByLine *p) {
  UserInterface &ui = Application::GetUserInterface();
  if(ui.GetString("UNITS") != "IOF") return;

  Camera *cam = NULL;

  try {
    cam = icube->camera();
  }
  catch(IException &e) {
    std::string msg = "Unable to create a camera model from [" +
                  icube->fileName().toStdString() + "]. Please run "
                  "spiceinit on this file";
    throw IException(e, IException::Unknown, msg, _FILEINFO_);
  }


  solarRemoveCoefficient = -1;

  // try center first
  if(cam->SetImage(icube->sampleCount() / 2, icube->lineCount() / 2)) {
    solarRemoveCoefficient = cam->SolarDistance() * cam->SolarDistance();
  }

  // try 4 corners
  if(solarRemoveCoefficient < 0 && cam->SetImage(1.0, 1.0)) {
    solarRemoveCoefficient = cam->SolarDistance() * cam->SolarDistance();
  }

  if(solarRemoveCoefficient < 0 && cam->SetImage(icube->sampleCount(), 1.0)) {
    solarRemoveCoefficient = cam->SolarDistance() * cam->SolarDistance();
  }

  if(solarRemoveCoefficient < 0 && cam->SetImage(icube->sampleCount(),
      icube->lineCount())) {
    solarRemoveCoefficient = cam->SolarDistance() * cam->SolarDistance();
  }

  if(solarRemoveCoefficient < 0 && cam->SetImage(1.0, icube->lineCount())) {
    solarRemoveCoefficient = cam->SolarDistance() * cam->SolarDistance();
  }

  // try center of 4 edges
  if(solarRemoveCoefficient < 0 && cam->SetImage(icube->sampleCount() / 2, 1.0)) {
    solarRemoveCoefficient = cam->SolarDistance() * cam->SolarDistance();
  }

  if(solarRemoveCoefficient < 0 && cam->SetImage(icube->sampleCount(),
      icube->lineCount() / 2)) {
    solarRemoveCoefficient = cam->SolarDistance() * cam->SolarDistance();
  }

  if(solarRemoveCoefficient < 0 && cam->SetImage(icube->sampleCount() / 2,
      icube->lineCount())) {
    solarRemoveCoefficient = cam->SolarDistance() * cam->SolarDistance();
  }

  if(solarRemoveCoefficient < 0 && cam->SetImage(1.0, icube->lineCount() / 2)) {
    solarRemoveCoefficient = cam->SolarDistance() * cam->SolarDistance();
  }

  // Default to original vimscal's solar distance if we can't find a target
  if(solarRemoveCoefficient < 0) {
    solarRemoveCoefficient = 81.595089;
    /*
    std::string msg = "Unable to project image at four corners, center of edges or ";
    msg += "at center. The solar distance can not be calculated, try using";
    msg += " [UNITS=SPECENERGY] on [";
    msg += icube->FileName() + "]";
    throw iException::Message(iException::Camera, msg, _FILEINFO_);
    */
  }

  //bool vis = (icube->label()->
  //            findGroup("Instrument", Pvl::Traverse)["Channel"][0] != "IR");

  QString attributes;

  // vis is bands 1-96, ir is bands 97-352 in this calibration file
  if(g_visBool) {
    attributes = "+1-96";
  }
  else {
    attributes = "+97-352";
  }

  CubeAttributeInput iatt(attributes.toStdString());

  //QString solarFilePath = "$cassini/calibration/vims/solar_v????.cub";
  QString solarFilePath = "$cassini/calibration/vims/"+calVersion+
     "/solar-spectrum/"+"solar."+yearString+"_v????.cub";

  FileName solarFileName(solarFilePath.toStdString());

  solarFileName = solarFileName.highestVersion();

  calibInfo += PvlKeyword("SolarColorFile",
                          solarFileName.originalPath() + "/" + solarFileName.name());

  p->SetInputCube(createCroppedFile(icube, QString::fromStdString(solarFileName.expanded())), iatt);
}


/**
 * @brief Loads the approrpriate constants which need to be multiplied by
 * the values in the calibration cubes during the calibration phase.
 * Also loads the current radiometric calibration version.
 */
void loadCalibrationValues() {

  FileName calibFile("$cassini/calibration/vims/vimsCalibration????.trn");
  calibFile = calibFile.highestVersion();

  //Pvl configFile;
  g_configFile.read(calibFile.expanded());
  PvlGroup &multipliers = g_configFile.findGroup("CalibrationMultipliers");

  calVersion = QString::fromStdString(multipliers["version"]);

  g_solar = std::stod(multipliers["solar"][0]);

  g_ir = std::stod(multipliers["IR"][0]);

  g_vis = std::stod(multipliers["VIS"][0]);

  g_wavecal = std::stod(multipliers["wave-cal"][0]);



  calibInfo += PvlKeyword("CalibrationVersion", calVersion.toStdString());
  calibInfo += PvlKeyword("SolarMultiplier",QString::number(g_solar,'f',2).toStdString());
  calibInfo += PvlKeyword("IR_Multiplier",QString::number(g_ir,'f',2).toStdString());
  calibInfo += PvlKeyword("VIS_Multiplier",QString::number(g_vis,'f',2).toStdString());
  calibInfo += PvlKeyword("Wave-CalMultiplier",QString::number(g_wavecal,'f',2).toStdString());


}


/**
 * @brief Updates the BandBin::Center keyword value in the input cube's label
 * with new wavelength values for RC 19 (Radiometric Calibration version 19).
 * This is due to the wavelength calibration drift.
 * @param icube  The original input cube prior to calibration
 */
void updateWavelengths(Cube *icube) {

  PvlGroup &inst = icube->label()->findGroup("Instrument", Pvl::Traverse);
  bool vis = (inst["Channel"][0] != "IR");


  PvlGroup &bandBin = icube->label()->findGroup("BandBin",Pvl::Traverse);

  QString bandwidthFile = "$cassini/calibration/vims/"+calVersion+"/band-wavelengths/wavelengths."+
      yearString+"_v????.cub";

  QString averageBandwidthFile = "$cassini/calibration/vims/"+calVersion+"/band-wavelengths/"+
      "wavelengths_average_v????.cub";


  FileName bandwidthFileName = FileName(bandwidthFile.toStdString());
  FileName averageBandwidthFileName = FileName(averageBandwidthFile.toStdString());
  bandwidthFileName =bandwidthFileName.highestVersion();
  averageBandwidthFileName = averageBandwidthFileName.highestVersion();

  Cube averageBandwidthCube;
  Cube bandwidthCube;
  bandwidthCube.open(QString::fromStdString(bandwidthFileName.expanded()));
  averageBandwidthCube.open(QString::fromStdString(averageBandwidthFileName.expanded()));

  calibInfo += PvlKeyword("BandwidthFile",
                          bandwidthFileName.originalPath() + "/" + bandwidthFileName.name());

  calibInfo += PvlKeyword("AverageBandwidthFile",
                          averageBandwidthFileName.originalPath() + "/"
                          + averageBandwidthFileName.name());

  LineManager bandwidthMgr(bandwidthCube);
  LineManager averageBandwidthMgr(averageBandwidthCube);

  for (int i =0; i < icube->bandCount();i++) {

    Statistics bandwidthStats;
    Statistics averageBandwidthStats;

    if (vis) {
      averageBandwidthMgr.SetLine(1,i+1);
      bandwidthMgr.SetLine(1, i + 1);
    }
    else {
      // ir starts at band 97
      averageBandwidthMgr.SetLine(1,i+97);
      bandwidthMgr.SetLine(1, i + 97);
    }

    bandwidthCube.read(bandwidthMgr);
    bandwidthStats.AddData(bandwidthMgr.DoubleBuffer(), bandwidthMgr.size());
    bandwidthVector.push_back(bandwidthStats.Average());

    averageBandwidthCube.read(averageBandwidthMgr);
    averageBandwidthStats.AddData(averageBandwidthMgr.DoubleBuffer(), averageBandwidthMgr.size());
    averageBandwidthVector.push_back(averageBandwidthStats.Average());
  }

  QString bandbinCenterString("(");
  QString averageBandbinString("(");

  for (unsigned int i =0; i < bandwidthVector.size()-1; i++) {
    bandbinCenterString+=QString::number(bandwidthVector[i]);
    bandbinCenterString+=",";

    averageBandbinString+=QString::number(averageBandwidthVector[i]);
    averageBandbinString+=",";
  }

  bandbinCenterString+=QString::number(bandwidthVector[bandwidthVector.size()-1]);
  bandbinCenterString+=")";

  averageBandbinString+=QString::number(averageBandwidthVector[averageBandwidthVector.size()-1]);
  averageBandbinString+=")";



  PvlKeyword &centerBand = bandBin.findKeyword("Center");
  centerBand.setValue(bandbinCenterString.toStdString());

  PvlKeyword averageBand("MissionAverage");
  averageBand.setValue(averageBandbinString.toStdString());
  bandBin.addKeyword(averageBand);
  centerBand.setValue(bandbinCenterString.toStdString());

}


/**
 * @brief This calculates the coefficients for specific energy corrections
 */
void calculateSpecificEnergy(Cube *icube) {
  PvlGroup &inst = icube->label()->findGroup("Instrument", Pvl::Traverse);
  //bool vis = (inst["Channel"][0] != "IR");


  double multiplier = 1.0;
  double coefficient = 1.0;

  if(inst["GainMode"][0] == "HIGH") {
    coefficient /= 2;
  }

  if(g_visBool && inst["SamplingMode"][0] == "HI-RES") {
    coefficient *= 3;
  }

  if(g_visBool) {
    coefficient /= std::stod(inst["ExposureDuration"][1]) / 1000.0;
  }
  else {

  //Discrepancies between the VIMS and Spacecraft clock necessitated a
  //conversion multiplier, which is in the USGS: ISIS version of vimscal
  //but was not part of the University of Arizona pipeline.  Below is the
  //text describing this problem:
  /**
    I. VIMS ISIS Camera Model - Three subtasks were included in this work:
       Timing Discrepancy - The first involved accounting for a timing discrepancy recorded in
       the VIMS EDRs. In 2012 (following an inquiry from Mark Showalter) we identified a
       timing error that arose because the S/C clock and VIMS internal clock run at slightly
       different rates (the VIMS internal clock rate is 1.01725 slower than the
       S/C USO-driven clock). The instrument timing reported in the VIMS_IR EDR labels
       includes exposure duration, interline delay, and intercube delay. We had been under the
       misimpression that these times had been converted to S/C clock units at JPL during the
       creation of the EDRs. This was not the case and the labels are, in fact, in units of
       the VIMS internal clock. This error propagates into errors in geometric reconstruction
       that become particularly severe for very long IR exposures. During reconstruction,
       each VIMS IR spectral sample (pixel) is assigned an acquisition time (based on timing
       data in the labels) that is used to index into the SPICE kernel data bases in order to
       derived necessary geometric information for that pixel. As a result of the use of the wrong
       time base, the geometric parameters so derived were in error.

       Prior to making any changes the VIMS internal clock rate was remeasured using inflight
       sequences in which both the VIMS and S/C clock times were recorded for each VIMS_IR pixel.
       These results showed that the VIMS internal clock rate was the same as measured prelaunch
       to a precision of 10-6. We then made changes to ISIS S/W to convert these erroneous times
       reported in the EDR headers to UTC time. This required modifying the VIMS IR Camera model
       in ISIS-3 (as well as in ISIS-2, used in the VIMS data processing pipeline at the
       University of Arizona). Bob Brown is preparing a document for inclusion in PDS EDR
       deliveries describing the problem and procedure to convert the time parameters as given in
       EDR labels (EXPOSURE_DURATION, INTERLINE_DELAY_DURATION, and INTERFRAME_DELAY_DURATION).
    */

    //USGS
    coefficient /= (std::stod(inst["ExposureDuration"][0]) * 1.01725) / 1000.0 - 0.004;


    //University of Arizona
    //coefficient /= (toDouble(inst["ExposureDuration"][0]) ) / 1000.0 - 0.004;
  }



  //QString specEnergyFile = "$cassini/calibration/vims/ir_perf_v????.cub";



  QString specEnergyFile = "$cassini/calibration/vims/"+calVersion+"/RC19-mults/RC19."
      +yearString+"_v????.cub";

  QString visPerfFile = "$cassini/calibration/vims/vis_perf_v????.cub";


  //B multiplier
  //QString waveCalFile = "$cassini/calibration/vims/wavecal_v????.cub";

  QString waveCalFile = "$cassini/calibration/vims/"+calVersion+"/wave-cal/wave.cal."+
      yearString+"_v????.cub";


  FileName specEnergyFileName(specEnergyFile.toStdString());
  specEnergyFileName = specEnergyFileName.highestVersion();

  FileName visPerfFileName(visPerfFile.toStdString());
  visPerfFileName = visPerfFileName.highestVersion();

  FileName waveCalFileName(waveCalFile.toStdString());
  waveCalFileName = waveCalFileName.highestVersion();


  Cube specEnergyCube;
  specEnergyCube.open(QString::fromStdString(specEnergyFileName.expanded()));

  Cube visPerfCube;
  visPerfCube.open(QString::fromStdString(visPerfFileName.expanded()));

  Cube waveCalCube;
  waveCalCube.open(QString::fromStdString(waveCalFileName.expanded()));

  calibInfo += PvlKeyword("SpecificEnergyFile",
                          specEnergyFileName.originalPath() + "/" + specEnergyFileName.name());
  if (g_visBool) {
    calibInfo += PvlKeyword("VisPerfFile",
                            visPerfFileName.originalPath() + "/" + visPerfFileName.name());
  }
  calibInfo += PvlKeyword("WavelengthCalibrationFile",
                          waveCalFileName.originalPath() + "/" + waveCalFileName.name());

  LineManager specEnergyMgr(specEnergyCube);
  LineManager visPerfMgr(specEnergyCube);
  LineManager waveCalMgr(waveCalCube);

  for(int i = 0; i < icube->bandCount(); i++) {
    Statistics specEnergyStats;
    Statistics visPerfStats;
    Statistics waveCalStats;

    if(g_visBool) {
      specEnergyMgr.SetLine(1, i + 1);
      visPerfMgr.SetLine(1, i + 1);
      waveCalMgr.SetLine(1, i + 1);
      multiplier=1;

    }
    else {
      //specEnergyMgr.SetLine(1, i + 1);  //vimscalold
      specEnergyMgr.SetLine(1,i+96+1);
      // ir starts at band 97
      waveCalMgr.SetLine(1, i + 96 + 1);
      multiplier = g_ir;
    }

    specEnergyCube.read(specEnergyMgr);
    waveCalCube.read(waveCalMgr);

    specEnergyStats.AddData(specEnergyMgr.DoubleBuffer(), specEnergyMgr.size());
    waveCalStats.AddData(waveCalMgr.DoubleBuffer(), waveCalMgr.size());


    //Determine Specific Intensity
    //  I = [(Raw_{DN} = Dark)/flatfield]*B*C
    //  B = waveCalStats.Average()
    //  C = specEnergyStats.Average()
    //Equation 1 in the white paper (C = specificEnergyStats.Average(),
    //B = waveCalStats.Average() )

    double bandCoefficient = coefficient * (multiplier*specEnergyStats.Average())
        *(g_wavecal*waveCalStats.Average() );

    if (g_visBool) {
      visPerfCube.read(visPerfMgr);
      visPerfStats.AddData(visPerfMgr.DoubleBuffer(), visPerfMgr.size());

      bandCoefficient *= visPerfStats.Average();
    }


    specificEnergyCorrections.push_back(bandCoefficient);
  }

}


/**
 * This decides if we have a VIS or IR dark current correction and calls the
 * appropriate method.
 *
 * @param currCube
 */
void calculateDarkCurrent(Cube *icube) {
  PvlGroup &inst = icube->label()->findGroup("Instrument", Pvl::Traverse);
  bool vis = (inst["Channel"][0] != "IR");

  calibInfo += PvlKeyword("Vis", ((vis) ? "true" : "false"));

  if(vis) {
    calculateVisDarkCurrent(icube);
  }
  else {
    calculateIrDarkCurrent(icube);
  }
}


/**
 * This populates darkCorrections with the result of the equation:
 *   dark = a + x * b
 * for each line,band. a, b are from the "vis_*_dark_model.tab" files
 * and x is the ExposureDuration.
 *
 * @param icube
 */
void calculateVisDarkCurrent(Cube *icube) {
  PvlGroup &inst = icube->label()->findGroup("Instrument", Pvl::Traverse);

  // This is the dark current corrections for VIS
  bool hires = ((inst["SamplingMode"][0] == "HIGH") || (inst["SamplingMode"][0] == "HI-RES"));
  QString calFile = "$cassini/calibration/vims/vis_";

  if(hires) {
    calFile += "hires";
  }
  else {
    calFile += "lowres";
  }

  calFile += "_dark_model_v????.tab";

  FileName calFileName(calFile.toStdString());
  calFileName = calFileName.highestVersion();

  calibInfo += PvlKeyword("DarkCurrentFile", calFileName.originalPath() + "/" + calFileName.name());

  calFile = QString::fromStdString(calFileName.expanded());

  EndianSwapper swapper("LSB");

  FILE *calFilePtr = fopen(calFile.toLatin1().data(), "r");

  double visExposure = std::stod(inst["ExposureDuration"][1]);

  int sampleOffset, lineOffset;
  GetOffsets(*icube->label(), sampleOffset, lineOffset);

  /**
   * Reading in one parameter at a time:
   *   parameter 1 = constant coefficient
   *   parameter 2 = exposure coefficient
   *   param1 + param2*exposure = dark correction
   *
   * Do byte swapping where necessary.
   */
  for(int parameter = 1; parameter <= 2; parameter ++) {
    for(int band = 1; band <= 96; band++) {
      for(int sample = 1; sample <= 64; sample++) {
        float calData;

        if(fread(&calData, sizeof(calData), 1, calFilePtr) != 1) {
          // error!
          std::string msg = "Error reading file [" + calFile.toStdString() + "]";
          throw IException(IException::Io, msg, _FILEINFO_);
        }

        int associatedSample = sample - sampleOffset + 1;

        calData = swapper.Float(&calData);
        pair<int, int> index = pair<int, int>(associatedSample, band);

        map< pair<int, int>, double>::iterator pos = sampleBasedDarkCorrections.find(index);
        if(pos == sampleBasedDarkCorrections.end()) {
          sampleBasedDarkCorrections.insert(pair< pair<int, int>, double>(index, calData));
        }
        else {
          (*pos).second = (*pos).second + visExposure * calData;
        }
      }
    }
  }

  fclose(calFilePtr);

  // If spectral summing is on, go in sets of 8 and set darks to the average
  /*
  PvlGroup &archive = icube->Label()->findGroup("Archive", Pvl::Traverse);
  if(archive["SpectralSummingFlag"][0] == "ON") {
    for(int band = 1; band <= 96; band += 8) {
      for(int sample = 1; sample <= 64; sample++) {
        Statistics stats;

        // we're calculating an average of 8 of these values through the bands
        for(int avgIndex = 0; avgIndex < 8; avgIndex ++) {
          // this wont go out of bounds as long as we have 8 as the increment
          pair<int,int> index = pair<int,int>(sample, band+avgIndex);
          stats.AddData(sampleBasedDarkCorrections.find(index)->second);
        }

        // now set the values to the average
        for(int setIndex = 0; setIndex < 8; setIndex ++) {
          pair<int,int> index = pair<int,int>(sample, band+setIndex);
          if(!setIndex) printf("Changing %.4f to %.4f\n",
          sampleBasedDarkCorrections.find(index)->second, stats.Average());
          sampleBasedDarkCorrections.find(index)->second = stats.Average();
        }
      }
    }
  }*/
}


/**
 * This calculates the dark current corrections for IR. If
 * IRDARKAVG is false, then it translates the sideplane data
 * into the lineBasedDarkCorrections map directly and does nothing further
 * with the data. Otherwise, this will apply a least squares linear
 * fit (the original script did chi-squared, but this is okay) for
 * each band and uses the points on the line instead of the sideplane
 * data directly.
 *
 * @param icube
 */
void calculateIrDarkCurrent(Cube *icube) {
  UserInterface &ui = Application::GetUserInterface();

  bool found = false;

  // verify if IR we have sideplane data
  for(int obj = 0; !found && obj < icube->label()->objects(); obj++) {
    PvlObject &object = icube->label()->object(obj);

    if(object.name() != "Table") continue;

    if(object.hasKeyword("Name") && object["Name"][0] == "SideplaneIr") found = true;
  }

  if(!found) {
    calibInfo += PvlKeyword("SideplaneCorrection", "None");
    return;
  }

  Table sideplane("SideplaneIr", ui.GetCubeName("FROM").toStdString());

  // If spectal summing is on OR compressor_id isnt N/A then
  //   just return.
  PvlGroup &archive = icube->label()->findGroup("Archive", Pvl::Traverse);

  // If dark subtracted (compressorid is valid) and cant do linear
  //   correction (spectral editing flag on) then do not do dark
  if(archive["CompressorId"][0] != "N/A" &&
      archive["SpectralEditingFlag"][0] == "ON") {
    calibInfo += PvlKeyword("SideplaneCorrection", "None");
    return;
  }

  // If subtracted (compressor id is valid) and dont do linear then return
  if(archive["CompressorId"][0] != "N/A" && ui.GetBoolean("IRORIGDARK") == true) {
    calibInfo += PvlKeyword("SideplaneCorrection", "None");
    return;
  }

  if(archive["SpectralSummingFlag"][0] == "ON") return;

  // Insert the sideplane data into our lineBasedDarkCorrections map (line,band to correction)
  for(int line = 1; line <= icube->lineCount(); line++) {
    for(int band = 1; band <= icube->bandCount(); band++) {
      pair<int, int> index = pair<int, int>(line, band);
      int value = (int)sideplane[(line-1)*icube->bandCount() + (band-1)][2];

      if(value != 57344)
        lineBasedDarkCorrections.insert(pair< pair<int, int>, double>(index, value));
      else
        lineBasedDarkCorrections.insert(pair< pair<int, int>, double>(index, Null));
    }
  }

  if(ui.GetBoolean("IRORIGDARK") == true) {
    calibInfo += PvlKeyword("SideplaneCorrection", "Sideplane");
    return;
  }

  // do linear fits
  for(int band = 1; band <= icube->bandCount(); band++) {
    PolynomialUnivariate basis(1);
    LeastSquares lsq(basis);

    for(int line = 1; line <= icube->lineCount(); line++) {
      pair<int, int> index = pair<int, int>(line, band);
      map< pair<int, int>, double>::iterator val = lineBasedDarkCorrections.find(index);

      if(val != lineBasedDarkCorrections.end()) {
        vector<double> input;
        input.push_back(line);
        double expected = val->second;

        if(!IsSpecial(expected))
          lsq.AddKnown(input, expected);
      }
    }

    if(lsq.Rows() == 0) return;
    lsq.Solve();

    double coefficients[2] = {
      basis.Coefficient(0),
      basis.Coefficient(1)
    };

    for(int line = 1; line <= icube->lineCount(); line++) {
      pair<int, int> index = pair<int, int>(line, band);

      map< pair<int, int>, double>::iterator val = lineBasedDarkCorrections.find(index);
      if(val != lineBasedDarkCorrections.end()) {
        double currentDark = val->second;

        if(!IsSpecial(currentDark)) {
          double newDark = coefficients[0] + line * coefficients[1];

          // initial dark applied by compressor
          if(archive["CompressorId"][0] != "N/A") {
            // input is in (dn-dark) units
            // (dn-dark) - (fit-dark) = dn-fit
            newDark -= currentDark;
          }

          val->second = newDark;
        }
      }
    }
  }

  if(archive["CompressorId"][0] != "N/A") {
    calibInfo += PvlKeyword("SideplaneCorrection", "Fit Delta");
  }
  else {
    calibInfo += PvlKeyword("SideplaneCorrection", "Fit");
  }
}

/**
 * @brief This calls p->SetInputCube with the appropriate flat file needed for
 * icube.
 *
 * @param icube
 * @param p
 */
void chooseFlatFile(Cube *icube, ProcessByLine *p) {

  UserInterface &ui = Application::GetUserInterface();

  QString flatField= ui.GetString("FLATFIELD");
  QString signature="";

  PvlGroup &inst = icube->label()->findGroup("Instrument", Pvl::Traverse);
  bool vis = (inst["Channel"][0] != "IR");
  bool hires = ((inst["SamplingMode"][0] == "HIGH") || (inst["SamplingMode"][0] == "HI-RES"));

//#if 0

if (!vis) {
  if (flatField == "2006FLAT") {
    signature="352_";

  }

  else if (flatField == "2006SSFLAT") {
    signature = "ss_352_";

  }

  else if (flatField == "2013FLAT") {

    signature = "2013_";
  }

 }


//#endif


  QString calFile = "$cassini/calibration/vims/flatfield/";

  if(vis) {
    calFile += "vis_";
  }
  else {
    calFile += "ir_";
  }

  if(hires) {

   //calFile += "hires_flatfield_v????.cub";
   calFile += "hires_flatfield_"+signature+"v????.cub";
  }
  else {
      //calFile += "ss_flat_352_v????.cub";
    calFile += "flatfield_"+signature+"v????.cub";

  }

  FileName calibrationFileName(calFile.toStdString());
  calibrationFileName = calibrationFileName.highestVersion();

  calibInfo += PvlKeyword("FlatFile", calibrationFileName.originalPath() +
                          "/" + calibrationFileName.name());

  CubeAttributeInput iatt;
  p->SetInputCube(createCroppedFile(icube, QString::fromStdString(calibrationFileName.expanded()), true), iatt);
}


/**
 * This makes our calibration files match the input cube described
 * by the swath keywords.
 *
 * @param icube
 * @param cubeFileName
 *
 * @return QString
 */
QString createCroppedFile(Cube *icube, QString cubeFileName, bool flatFile) {
  int sampOffset = 1;
  int lineOffset = 1;

  if(flatFile) {
    GetOffsets(*icube->label(), sampOffset, lineOffset);
  }


  QString appArgs = "from=" + cubeFileName + " ";
  appArgs += "sample=" + QString::number(sampOffset) + " ";
  appArgs += "line=" + QString::number(lineOffset) + " ";
  appArgs += "nsamples=" + QString::number(icube->sampleCount()) + " ";
  appArgs += "nlines=" + QString::number(icube->lineCount()) + " ";

  FileName tempFile("$TEMPORARY/tmp_" + FileName(cubeFileName.toStdString()).baseName() +
                    "_" + FileName(icube->fileName().toStdString()).name());

  appArgs += "to=" + QString::fromStdString(tempFile.expanded());

  ProgramLauncher::RunIsisProgram("crop", appArgs);
  tempFiles.push_back(QString::fromStdString(tempFile.expanded()));
  return QString::fromStdString(tempFile.expanded());
}


void GetOffsets(const Pvl &lab, int &finalSampOffset, int &finalLineOffset) {
  const PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);

  //  Get sample/line offsets
  int sampOffset = inst ["XOffset"];
  int lineOffset = inst ["ZOffset"];

  //  Get swath width/length which will be image size unless occultation image
  int swathWidth = inst ["SwathWidth"];
  int swathLength = inst ["SwathLength"];

  bool vis = (inst["Channel"][0] != "IR");
  finalSampOffset = sampOffset;
  finalLineOffset = lineOffset;

  QString samplingMode = QString::fromStdString(inst["SamplingMode"]).toUpper();
  if(vis) {
    if(samplingMode == "NORMAL") {
      finalSampOffset = sampOffset - 1;
      finalLineOffset = lineOffset - 1;
    }
    else if(samplingMode == "HI-RES") {
      finalSampOffset = (3 * ((sampOffset - 1) + swathWidth / 2)) - swathWidth / 2;
      finalLineOffset = (3 * (lineOffset + swathLength / 2)) - swathLength / 2;
    }
    else {
      std::string msg = "Unsupported sampling mode [" + samplingMode.toStdString() + "]";
      throw IException(IException::Io, msg, _FILEINFO_);
    }
  }
  else {
    if(samplingMode == "NORMAL") {
      finalSampOffset = sampOffset - 1;
      finalLineOffset = lineOffset - 1;
    }
    else if(samplingMode == "HI-RES") {
      finalSampOffset = 2 * ((sampOffset - 1) + ((swathWidth - 1) / 4));
      finalLineOffset = lineOffset - 1;
    }
    else if(samplingMode == "NYQUIST") {
      std::string msg = "Cannot process NYQUIST (undersampled) mode ";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    else {
      std::string msg = "Unsupported sampling mode [" + samplingMode.toStdString() + "]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
  }

  finalSampOffset ++;
  finalLineOffset ++;
}
