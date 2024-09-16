/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <cmath>

#include "ProcessByLine.h"
#include "Pixel.h"
#include "Camera.h"
#include "IException.h"
#include "Table.h"

using namespace std;
using namespace Isis;

bool conv;
double dcconst;
bool useDcconst;
double focalPlaneTemp;
double exposureDuration;
int offsetModeID;
double gain;
double avgFF;
double cr;
double dist;
double C2;
double correctedExposureDuration;

const double ACO = 1.062;
const double BCO = -0.1153E-02;
const double CCO = 0.6245E-05;
const double DCO = -0.1216E-07;
const double C3 = 7.13;
const double C4 = -8.177; // Post-readout offset
const double C5 = 15.56;
const double DT = .00068;

void UvVisCal(std::vector< Isis::Buffer * > &in, std::vector< Isis::Buffer * > &out);
void FixTemp(int);

void IsisMain() {
  // We will be processing by line
  ProcessByBrick p;
  UserInterface &ui = Application::GetUserInterface();

  // Use the def file for filter constants
  Pvl uvvisDef("$clementine1/calibration/uvvis/uvvis.def");

  // Setup the input and output cubes
  Cube *icube = p.SetInputCube("FROM");

  Cube *dccube;
  if(ui.WasEntered("DCFILE")) {
    dccube = p.SetInputCube("DCFILE");
  }
  else {
    QString dcfileloc = "$clementine1/calibration/uvvis/";
    dcfileloc += "dark_5_15_96.cub";
    CubeAttributeInput cubeAtt;
    dccube = p.SetInputCube(dcfileloc, cubeAtt);
  }

  QString filter = QString::fromStdString((icube->group("BandBin"))["FilterName"]);
  filter = filter.toLower();

  Cube *ffcube;
  if(ui.WasEntered("FFFILE")) {
    ffcube = p.SetInputCube("FFFILE");
  }
  else {
    // compute default fffile
    double compressRatio = (icube->group("Instrument"))["EncodingCompressionRatio"];

    // check to see if cube is compressed or uncompressed
    if(compressRatio == 1.0) {
      QString fffileLoc = "$clementine1/calibration/uvvis/";
      fffileLoc += "lu" + filter + "_uncomp_flat_long.cub";
      CubeAttributeInput cubeAtt;
      ffcube = p.SetInputCube(fffileLoc, cubeAtt);
    }
    else {
      QString fffileLoc = "$clementine1/calibration/uvvis/";
      fffileLoc += "lu" + filter + "_comp_flat_long.cub";
      CubeAttributeInput cubeAtt;
      ffcube = p.SetInputCube(fffileLoc, cubeAtt);
    }
  }

  Cube *ocube = p.SetOutputCube("TO");

  avgFF = uvvisDef.findGroup("Filter" + filter.toUpper().toStdString())["AVGFF"];
  cr = uvvisDef.findGroup("Filter" + filter.toUpper().toStdString())["CO"];
  gain = uvvisDef.findGroup("GainModeID" + icube->group("Instrument")["GainModeID"][0])["GAIN"];

  useDcconst = ui.WasEntered("DCCONST");
  if(useDcconst) {
    dcconst = ui.GetDouble("DCCONST");
  }
  else {
    dcconst = 0.0;
  }

  conv = ui.GetBoolean("CONV");
  exposureDuration = icube->group("Instrument")["ExposureDuration"];
  offsetModeID = icube->group("Instrument")["OffsetModeID"];

  if((QString::fromStdString(icube->group("Instrument")["FocalPlaneTemperature"])).compare("UNK") == 0) {
    //if FocalPlaneTemp is unknown set it to zero
    focalPlaneTemp = 0.0;
  }
  else {
    focalPlaneTemp = icube->group("Instrument")["FocalPlaneTemperature"];
  }

  Camera *cam = icube->camera();
  bool camSuccess = cam->SetImage(icube->sampleCount() / 2, icube->lineCount() / 2);

  if(!camSuccess) {
    throw IException(IException::Unknown, "Unable to calculate the Solar Distance for this cube.", _FILEINFO_);
  }

  dist = cam->SolarDistance();

  // If temp. correction set to true, or focal plane temp is zero then use temperature correction
  if(ui.GetBoolean("TCOR") || abs(focalPlaneTemp) <= DBL_EPSILON) {
    // Temperature correction requires the use of the mission phase
    //   (PRELAUNCH, EARTH, LUNAR) and the product ID.
    QString productID = QString::fromStdString((icube->group("Archive")["ProductID"]));
    QChar missionPhase = (QString::fromStdString(((icube->group("Archive"))["MissionPhase"]))).at(0);
    QString n1subQString(productID.mid(productID.indexOf('.') + 1, productID.length() - 1));
    QString n2subQString(productID.mid(4, productID.indexOf('.') - 5));
    int n1 = n1subQString.toInt();
    int n2 = n2subQString.toInt();
    int phase = 0;

    if(missionPhase == 'L') {
      phase = 0;
    }
    else if(missionPhase == 'E') {
      phase = 1;
    }
    else if(missionPhase == 'P') {
      phase = 2;
    }
    else {
      throw IException(IException::Unknown, "Invalid Mission Phase", _FILEINFO_);
    }

    // This formula makes the primary search critera the original product ID's extension,
    //   the secondary search criteria the mission phase and finally the numerical part of the
    //   original product ID.
    int imageID = (100000 * n1) + (10000 * phase) + n2;
    FixTemp(imageID);
  }

  if(focalPlaneTemp <= 0.0) {
    focalPlaneTemp = 272.5;
  }

  // Start the processing
  p.SetBrickSize(icube->sampleCount(), icube->lineCount(), 1);
  p.StartProcess(UvVisCal);

  // Add the radiometry group
  PvlGroup calgrp("Radiometry");
  calgrp += PvlKeyword("FlatFieldFile", ffcube->fileName().toStdString());

  if(ui.GetString("DARKCURRENT").compare("DCFILE") == 0) {
    calgrp += PvlKeyword("DarkCurrentFile", dccube->fileName().toStdString());
  }
  else {
    calgrp += PvlKeyword("DarkCurrentConstant", std::to_string(dcconst));
  }

  calgrp += PvlKeyword("CorrectedFocalPlaneTemp", std::to_string(focalPlaneTemp));
  calgrp += PvlKeyword("C1", std::to_string(avgFF));
  calgrp += PvlKeyword("C2", std::to_string(C2));
  calgrp += PvlKeyword("C3", std::to_string(C3));
  calgrp += PvlKeyword("C4", std::to_string(C4));
  calgrp += PvlKeyword("C5", std::to_string(C5));
  calgrp += PvlKeyword("CR", std::to_string(cr));
  calgrp += PvlKeyword("FrameTransferTimePerRow", std::to_string(cr));
  calgrp += PvlKeyword("Gain", std::to_string(gain));
  calgrp += PvlKeyword("CorrectedExposureDuration", std::to_string(correctedExposureDuration));
  calgrp += PvlKeyword("ConvertToRadiance", std::to_string(conv));

  calgrp += PvlKeyword("ACO", std::to_string(ACO));
  calgrp += PvlKeyword("BCO", std::to_string(BCO));
  calgrp += PvlKeyword("CCO", std::to_string(CCO));
  calgrp += PvlKeyword("DCO", std::to_string(DCO));

  ocube->putGroup(calgrp);
  p.EndProcess();
}

void UvVisCal(std::vector< Isis::Buffer * > &in, std::vector< Isis::Buffer * > &out) {
#define INPUT_CUBE (*in[0])
#define DC_CUBE (*in[1])
#define FF_CUBE (*in[2])
#define OUTPUT_CUBE (*out[0])

  C2 = 0.003737 * exp(0.0908 * (focalPlaneTemp - 273.15));
  correctedExposureDuration = exposureDuration + 0.0494;

  double dc = 0.0;
  bool valid = false;
  double *sum, *ro;

  sum = new double[INPUT_CUBE.SampleDimension()];
  ro = new double[INPUT_CUBE.SampleDimension()];

  for(int iSample = 0; iSample < INPUT_CUBE.SampleDimension(); iSample++) {
    ro[iSample] = 0.0;
    sum[iSample] = 0.0;

    for(int iLine = 0; iLine < INPUT_CUBE.LineDimension(); iLine ++) {
      valid = false;
      int index = INPUT_CUBE.SampleDimension() * iLine + iSample;

      if(Pixel::IsSpecial(INPUT_CUBE[index])) {
        OUTPUT_CUBE[index] = INPUT_CUBE[index];

        if(Pixel::IsHigh(INPUT_CUBE[index])) {
          sum[iSample] += 255;
        }
      }
      else {
        if(Pixel::IsSpecial(FF_CUBE[index])) {  //check DCFILE
          OUTPUT_CUBE[index] = Isis::Null;
          sum[iSample] += INPUT_CUBE[index];
        }
        else {
          valid = true;
          if(!useDcconst) {
            dc = DC_CUBE[index];
          }
          else {
            dc = dcconst;
          }
        }
      }

      if(valid) {
        //Global Offset Corrections
        double step1_dn = INPUT_CUBE[index] - (C4 * offsetModeID) - C5;
        //Gain Correction
        double step2_dn = step1_dn / gain;
        //Pixel dependent dark current correction
        double step3_dn = step2_dn - (dc + C3);
        //Non-linearity correction
        double xmul = ACO + (BCO * step3_dn) + (CCO * pow(step3_dn, 2)) + (DCO * pow(step3_dn, 3));
        double corstep3_dn = step3_dn * xmul;
        //Tempurature-dependent offset correction
        double rotim = 60.05 + 0.05 * iLine;
        double u = correctedExposureDuration + rotim;
        double step4_dn = corstep3_dn - (C2 * u);
        OUTPUT_CUBE[index] = step4_dn;
        sum[iSample] += step4_dn;
      }
    }

    ro[iSample] = sum[iSample] * DT / (correctedExposureDuration + (288.0 * DT));
  }

  for(int iLine = 0; iLine < INPUT_CUBE.LineDimension(); iLine ++) {
    for(int iSample = 0; iSample < INPUT_CUBE.SampleDimension(); iSample++) {
      valid = false;
      int index = INPUT_CUBE.SampleDimension() * iLine + iSample;
      if(!Pixel::IsSpecial(INPUT_CUBE[index])) {
        if(Pixel::IsSpecial(FF_CUBE[index])) {  //check FFFILE
          OUTPUT_CUBE[index] = Isis::Null;
        }
        else {
          valid = true;
        }

        if(valid) {
          // Frame transfer correction
          double step5_dn = OUTPUT_CUBE[index] - ro[iSample];
          // Flat-field and exposure time normalization (Units=counts/ms)
          double step6_dn = step5_dn / (FF_CUBE[index] * correctedExposureDuration);
          // Normalize to sun-moon distance of 1AU
          double step7_dn = step6_dn * pow(dist, 2);
          // Conversion to radience (L=mW/sr-cm*cm)
          double L = step7_dn / avgFF;
          // Conversion to reflectance
          if(conv) {
            OUTPUT_CUBE[index] = step7_dn * cr;
          }
          else {
            OUTPUT_CUBE[index] = L;
          }
        }
      }
    }
  }

  delete[] sum;
  delete[] ro;
}

/*
 * In the ISIS2 Fortran we noticed FIXTEMP uses a REAL*4 to store the RIMGID.
 * This results in the last digit being lost precision. The table that
 * it looks in for data is also store as a REAL*4 again resulting
 * in a loss in precision. We believe this makes the lookup table inaccurate.
 */
void FixTemp(int imgID) {
  std::string table = "$clementine1/calibration/uvvis/uvvisTemperature.tbl";
  Table t("FocalPlaneTemperatures", table);
  float currID = t[0]["ImageID"];
  int currIndex = 0;

  do {
    currID = t[currIndex]["ImageID"];
    currIndex ++;
  }
  while((imgID > currID) && (currIndex < t.Records()));
  currIndex --;  // Fixes an out-of-bounds segmentation fault

  // Make sure currIndex makes sense
  if(currIndex < 0 || currIndex >= t.Records()) {
    focalPlaneTemp = 0;
    return;
  }

  // Value too small - look up in the table and see if we're closer
  if(currID < imgID) {
    double diff = imgID - currID;
    if((currIndex + 1 < t.Records()) && abs((float)t[currIndex + 1]["ImageID"] - imgID) < diff) {
      currIndex ++;
    }
  }
  // Value too big - look down in the table and see if we're closer
  else if(currID > imgID) {
    double diff = currID - imgID;
    if(currIndex - 1 >= 0 && abs((float)t[currIndex - 1]["ImageID"] - imgID) < diff) {
      currIndex --;
    }
  }

  focalPlaneTemp = (float)t[currIndex]["Temp"];
}
