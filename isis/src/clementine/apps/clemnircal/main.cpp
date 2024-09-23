/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"
#include "ProcessByLine.h"
#include "Pixel.h"
#include "IException.h"
#include "TextFile.h"
#include "Table.h"
#include <cmath>

using namespace std;
using namespace Isis;

const double digitalOffset = 8.30690;
const double globalBias = 2.15547;
const double globalDarkCoefficient = 0.730;
const double vConstant = -0.954194;

double gainFactor;
double absoluteCoefficient;
double saturationThreshold = 255.0;
double cryonorm = 10000.0; // This will almost always be 10,000
double cryocoolerDuration;
double optimalExposureDuration;

int numCoefficients = -1; // This will almost always be 1 or 2
int offsetModeID = 0;

bool convertToNull = false;
bool doThermalCorrection = false;

std::vector<double> thermBgCoefficients;

void NirCal(std::vector< Isis::Buffer * > &in, std::vector< Isis::Buffer * > &out);

void IsisMain() {
  // We will be processing by line
  ProcessByLine p;
  UserInterface &ui = Application::GetUserInterface();

  // Basic settings
  convertToNull = ui.GetBoolean("BPFLAG");
  doThermalCorrection = ui.GetBoolean("THCOR");
  absoluteCoefficient = ui.GetDouble("ABSCOEF");
  saturationThreshold = ui.GetDouble("HISAT");

  // Do files (filter + product ID matters)
  Cube *icube = p.SetInputCube("FROM");
  Cube *ocube = p.SetOutputCube("TO");
  Cube *ffcube, *ofcube, *afcube, *dccube, *biascube, *bpcube;

  QString filter = QString::fromStdString((icube->group("BandBin"))["FilterName"]);
  filter = filter.toLower();
  QString productID = QString::fromStdString((icube->group("Archive"))["ProductID"]);
  QString orbit = productID.mid(productID.indexOf('.') + 1, productID.length() - 1);

  // If hemisphere code greater than 'I' set to 'n' else set to 's'
  char hemisphereCode = (productID[productID.indexOf('.')-1] > 'I') ? 'n' : 's';
  QString compressionType = QString::fromStdString((icube->group("Instrument"))["EncodingFormat"]);
  offsetModeID = (icube->group("Instrument"))["OffsetModeID"];
  int gainModeID = (icube->group("Instrument"))["GainModeID"];
  QString gainModeIDStr = QString::number(gainModeID);
  double exposureDuration = (double)(icube->group("Instrument"))["ExposureDuration"];
  optimalExposureDuration = (exposureDuration * 0.984675) + 0.233398;
  cryocoolerDuration = (icube->group("Instrument"))["CryocoolerDuration"];

  if(ui.WasEntered("FFFILE")) {
    ffcube = p.SetInputCube("FFFILE");
  }
  else {
    QString fffileLoc = "$clementine1/calibration/nir/";
    fffileLoc += "newnir_flat_" + filter + ".cub";
    CubeAttributeInput cubeAtt;
    ffcube = p.SetInputCube(fffileLoc, cubeAtt);
  }

  if(ui.WasEntered("OFFILE")) {
    ofcube = p.SetInputCube("OFFILE");
  }
  else {
    QString fffileLoc = "$clementine1/calibration/nir/nirorbitflats/";
    fffileLoc += "nir_orbflat_" + orbit + "_" + filter + ".cub";
    CubeAttributeInput cubeAtt;
    ofcube = p.SetInputCube(fffileLoc, cubeAtt);
  }

  QString afFileTableLoc = "";
  if(ui.WasEntered("AFFILE")) {
    afcube = p.SetInputCube("AFFILE");
  }
  else {
    QString affileLoc;
    afFileTableLoc = "$clementine1/calibration/nir/nir.addflats.dat";

    TextFile aFFileTable(afFileTableLoc);
    int numLines = aFFileTable.LineCount();
    for(int i = 0; i < numLines; i++) {
      QString line;
      aFFileTable.GetLine(line, true);
      line = line.simplified();
      QStringList tokens = line.split(" ");
      if(tokens.count() > 6 && orbit == tokens.takeFirst() &&
          filter == tokens.takeFirst() &&
          gainModeID == (tokens.takeFirst().toInt()) &&
          offsetModeID == (tokens.takeFirst().toInt()) &&
          (int)exposureDuration == (tokens.takeFirst().toInt()) &&
          QString(hemisphereCode) == tokens.takeFirst()) {
        tokens.takeFirst();
        affileLoc = tokens.takeFirst();
        break;
      }
    }

    if(affileLoc.compare("") == 0) {
      affileLoc = "zeros.cub";
    }

    QString gainFactorDef = "$clementine1/calibration/nir/";
    gainFactorDef += "clemnircal.def";
    Pvl gainFactorData(gainFactorDef.toStdString());
    QString group = "GainModeID";
    group += QString::number(gainModeID);

    if(!gainFactorData.hasGroup(group.toStdString())) {
      std::string err = "The Gain Factor for Gain Mode ID [";
      err += Isis::toString(gainModeID);
      err += "] could not be found in clemnircal.def";
      throw IException(IException::Programmer, err, _FILEINFO_);
    }

    gainFactor = (gainFactorData.findGroup(group.toStdString()))["GAIN"];

    if(abs(gainFactor) < DBL_EPSILON) {
      std::string err = "The Gain Factor for Gain Mode ID [";
      err += Isis::toString(gainModeID);
      err += "] can not be zero.";
      throw IException(IException::Programmer, err, _FILEINFO_);
    }

    affileLoc = "$clementine1/calibration/nir/nirmodeflats/" + affileLoc;
    CubeAttributeInput cubeAtt;
    afcube = p.SetInputCube(affileLoc, cubeAtt);
  }

  if(ui.WasEntered("DCFILE")) {
    dccube = p.SetInputCube("DCFILE");
  }
  else {
    QString dcfileLoc = "$clementine1/calibration/nir/";

    if(compressionType.compare("CLEM-JPEG-0") == 0) {
      dcfileLoc += "dark_nir_cmp0.cub";
    }
    else {
      dcfileLoc += "dark_nir.cub";
    }

    CubeAttributeInput cubeAtt;
    dccube = p.SetInputCube(dcfileLoc, cubeAtt);
  }

  if(ui.WasEntered("BIASFILE")) {
    biascube = p.SetInputCube("BIASFILE");
  }
  else {
    QString biasfileLoc = "$clementine1/calibration/nir/";

    if(compressionType.compare("CLEM-JPEG-0") == 0) {
      biasfileLoc += "bias_nir_cmp0.cub";
    }
    else {
      biasfileLoc += "bias_nir.cub";
    }

    CubeAttributeInput cubeAtt;
    biascube = p.SetInputCube(biasfileLoc, cubeAtt);
  }

  if(ui.WasEntered("BPFILE")) {
    bpcube = p.SetInputCube("BPFILE");
  }
  else {
    QString bpfileLoc = "$clementine1/calibration/nir/";
    if(compressionType.compare("CLEM-JPEG-0") == 0) {
      bpfileLoc += "badpix_nir_cmp0.v3.cub";
    }
    else {
      bpfileLoc += "badpix_nir.v3.cub";
    }

    CubeAttributeInput cubeAtt;
    bpcube = p.SetInputCube(bpfileLoc, cubeAtt);
  }

  // We need thermal data
  QString thermTbl = "$clementine1/calibration/nir/";
  thermTbl += "nir" + filter + ".therm.dat";

  TextFile thermTable(thermTbl);
  int numLines = thermTable.LineCount();
  QString line;
  for(int i = 0; i < numLines; i++) {
    thermTable.GetLine(line);

    // In some lines there's a beginning space, and in others there isn't. To make it consitent,
    // I'm adding a space to every line and using Compress() to remove any extras. Compress() is
    // necessary in order to ensure data integrity from line to line also.
    line = " " + line;
    line = line.simplified().trimmed();
    QStringList tokens = line.split(" ");

    if(orbit.toInt() == tokens.takeFirst().toInt()) { // if orbits match, get data
      cryonorm = tokens.takeFirst().toDouble();
      numCoefficients = tokens.takeFirst().toInt();

      // Read in coefficients
      thermBgCoefficients.push_back(tokens.takeFirst().toDouble());

      for(int iCoeff = 0; iCoeff < numCoefficients; iCoeff ++) {
        thermBgCoefficients.push_back(tokens.takeFirst().toDouble());
      }
      break;
    }
  }

  if(numCoefficients == 0) {
    std::string err = "The orbit [" + orbit.toStdString() + "] could not be located in the thermal corrections table [" + thermTbl.toStdString() + "].";
    throw IException(IException::Unknown, err, _FILEINFO_);
  }

  // Start the processing
  p.StartProcess(NirCal);

  // Add the radiometry group
  PvlGroup calgrp("Radiometry");
  calgrp += PvlKeyword("FlatFieldFile", ffcube->fileName().toStdString());
  calgrp += PvlKeyword("OrbitFlatFieldFile", ofcube->fileName().toStdString());
  calgrp += PvlKeyword("AdditiveFile", afcube->fileName().toStdString());
  calgrp += PvlKeyword("DarkCurrentFile", dccube->fileName().toStdString());
  calgrp += PvlKeyword("BiasFile", biascube->fileName().toStdString());
  calgrp += PvlKeyword("BadPixelFile", bpcube->fileName().toStdString());

  //Table files
  calgrp += PvlKeyword("ThermalCorrectionTable", thermTbl.toStdString());
  calgrp += PvlKeyword("AdditiveFileTable", afFileTableLoc.toStdString());

  calgrp += PvlKeyword("DigitalOffset", Isis::toString(digitalOffset));
  calgrp += PvlKeyword("GlobalBias", Isis::toString(globalBias));
  calgrp += PvlKeyword("GlobalDarkCoefficient", Isis::toString(globalDarkCoefficient));
  calgrp += PvlKeyword("V", Isis::toString(vConstant));
  //Calculated in processing routine
  calgrp += PvlKeyword("GainFactor", Isis::toString(gainFactor));
  calgrp += PvlKeyword("AbsoluteCoefficient", Isis::toString(absoluteCoefficient));
  calgrp += PvlKeyword("CryoNorm", Isis::toString(cryonorm));
  calgrp += PvlKeyword("OptimalExposureDuration", Isis::toString(optimalExposureDuration));

  ocube->putGroup(calgrp);
  p.EndProcess();
}

void NirCal(std::vector< Isis::Buffer * > &in, std::vector< Isis::Buffer * > &out) {
  Isis::Buffer &incube = *in[0];
  Isis::Buffer &ffcube = *in[1];
  Isis::Buffer &ofcube = *in[2];
  Isis::Buffer &afcube = *in[3];
  Isis::Buffer &dccube = *in[4];
  Isis::Buffer &biascube = *in[5];
  Isis::Buffer &badpixelcube = *in[6];
  Isis::Buffer &outcube = *out[0];

  //Compute the thermal background correction
  double backgroundCorr = 0.0;

  for(int iCoefficient = 0; iCoefficient <= numCoefficients; iCoefficient ++) {
    backgroundCorr += thermBgCoefficients[iCoefficient] * pow(cryocoolerDuration / cryonorm, iCoefficient);
  }

  //No correction for thermal shape is required
  double thermalShapeCorrection = 0.0;

  for(int iSample = 0; iSample < incube.SampleDimension(); iSample ++) {
    // If the bad pixel cube has 0.0 as a value, then this is a known bad pixel
    if(badpixelcube[iSample] == 0.0) {
      outcube[iSample] = Isis::Null;
      continue;
    }
    else if(Pixel::IsSpecial(incube[iSample])) {
      outcube[iSample] = incube[iSample];
      continue;
    }
    else if(incube[iSample] >= saturationThreshold) {
      outcube[iSample] = Isis::Hrs;
      continue;
    }
    else if(Pixel::IsSpecial(ffcube[iSample]) || ffcube[iSample] == 0.0) {
      // In ISIS2 code the flat field cube was not being checked for
      //   high saturations, and math was still performed on the
      //   files. So, either the result was near zero (because it's
      //   a dividend) or there are no high saturations in this cube
      //   ever.
      outcube[iSample] = Isis::Null;
      continue;
    }
    // Can't do math on special pixels...
    else if(Pixel::IsSpecial(dccube[iSample]) ||
            Pixel::IsSpecial(afcube[iSample]) ||
            Pixel::IsSpecial(ofcube[iSample]) ||
            Pixel::IsSpecial(biascube[iSample]) ||
            Pixel::IsSpecial(badpixelcube[iSample])) {
      outcube[iSample] = Isis::Null;
      continue;
    }

    // Gain factor already checked for zero values
    double term1 = (incube[iSample] - digitalOffset) / gainFactor;

    double term2 = term1 - globalBias - biascube[iSample] - (offsetModeID * vConstant);

    double term3 = term2 / optimalExposureDuration;

    double term4 = term3 - globalDarkCoefficient - dccube[iSample];

    double term5 = term4 - backgroundCorr - thermalShapeCorrection;

    double term6 = term5 / ffcube[iSample];

    double term7 = term6 / ofcube[iSample];

    double term8 = term7 - afcube[iSample];


    outcube[iSample] = term8 * absoluteCoefficient;
  }
}
