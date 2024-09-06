/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <cmath>

#include "ProcessByLine.h"
#include "Camera.h"
#include "SpecialPixel.h"
#include "iTime.h"
#include "IException.h"
#include "Brick.h"

using namespace std;
using namespace Isis;

// Working functions and parameters
void Mar10Cal (std::vector< Isis::Buffer* > &inCubes,
    std::vector< Isis::Buffer* > &outCubes);

Cube coCube;
Brick * coef;

bool useBlem;
bool mask;
double correctedExp;
double sunDist;
double absCoef;
double xparm;


void IsisMain() {
  // We will be processing by line
  ProcessByLine p;

  // Setup the input and make sure it is a mariner10 file
  UserInterface & ui = Application::GetUserInterface();

  Isis::Pvl lab(ui.GetCubeName("FROM").toStdString());
  Isis::PvlGroup & inst = lab.findGroup("Instrument", Pvl::Traverse);

  QString mission = QString::fromStdString(inst["SpacecraftName"]);
  if (mission != "Mariner_10") {
    string msg = "This is not a Mariner 10 image.  Mar10cal requires a Mariner 10 image.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  Cube * icube = p.SetInputCube("FROM", OneBand);

  // If it is already calibrated then complain
  if (icube->hasGroup("Radiometry")) {
    std::string msg = "This Mariner 10 image [" + icube->fileName() + "] has "
                  "already been radiometrically calibrated";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Get label parameters we will need for calibration equation
  QString instId = QString::fromStdString(inst["InstrumentId"]);
  QString camera = instId.mid(instId.size()-1);

  QString filter = QString::fromStdString(icube->group("BandBin")["FilterName"]);
  filter = filter.toUpper().mid(0,3);

  QString target = QString::fromStdString(inst["TargetName"]);

  iTime startTime(QString::fromStdString(inst["StartTime"]));

  double exposure = inst["ExposureDuration"];
  double exposureOffset = 0.0;
  if (ui.WasEntered("EXPOFF")) {
    exposureOffset = ui.GetDouble("EXPOFF");
  }
  else {
    if (camera == "A") {
      exposureOffset = 0.316;
    }
    else if (camera == "B") {
      exposureOffset = 3.060;
    }
    else {
      std::string msg = "Camera [" + camera + "] is not supported.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }
  correctedExp = exposure + exposureOffset;

  Cube * dcCube;
  if (ui.WasEntered ("DCCUBE") ) {
    dcCube = p.SetInputCube("DCCUBE");
  }
  else {
    //  Mercury Dark current
    //   ??? NOTE:  Need to find Mark's dc for venus and moon ????
    QString dcFile("$mariner10/calibration/mariner_10_" + camera +
                  "_dc.cub");
    CubeAttributeInput cubeAtt;
    dcCube = p.SetInputCube(dcFile, cubeAtt);
  }


  //  Open blemish removal file
  Cube * blemCube = 0;
  useBlem = (ui.GetBoolean("BLEMMASK")) ? true : false;
  if (useBlem) {
    QString blemFile("$mariner10/calibration/mariner_10_blem_" + camera + ".cub");
    CubeAttributeInput cubeAtt;
    blemCube = p.SetInputCube(blemFile, cubeAtt);
  }

  if (filter == "FAB" || filter == "WAF") {
    std::string msg = "Filter type [" + filter + "] is not supported at this time.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  if (ui.WasEntered ("COEFCUBE")) {
    coCube.open(ui.GetCubeName("COEFCUBE"));
  }
  else {
    FileName coFile("$mariner10/calibration/mariner_10_" + filter + "_" +
        camera + "_coef.cub");
    coCube.open(coFile.expanded());
  }
  coef = new Brick(icube->sampleCount(), 1, 6, coCube.pixelType());

  if (ui.WasEntered("ABSCOEF")) {
    absCoef = ui.GetDouble("ABSCOEF");
  }
  else {
    if (camera == "A") {
      absCoef = 16.0;
    }
    else if (camera == "B") {
      absCoef = 750.0;
    }
    else {
      std::string msg = "Camera [" + camera + "] is not supported.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  mask = ui.GetBoolean("MASK");
  xparm = ui.GetDouble("XPARM");

  // Get the distance between Mars and the Sun at the given time in
  // Astronomical Units (AU)
  Camera * cam = icube->camera();
  bool camSuccess = cam->SetImage(icube->sampleCount()/2,icube->lineCount()/2);
  if (!camSuccess) {
    throw IException(IException::Unknown,
        "Unable to calculate the Solar Distance on [" +
        icube->fileName() + "]", _FILEINFO_);
  }
  sunDist = cam->SolarDistance();

  // Setup the output cube
  Cube  *ocube = p.SetOutputCube("TO");

  // Add the radiometry group
  PvlGroup calgrp("Radiometry");

  calgrp += PvlKeyword("DarkCurrentCube", dcCube->fileName().toStdString());
  if (useBlem) {
    calgrp += PvlKeyword("BlemishRemovalCube", blemCube->fileName().toStdString());
  }
  calgrp += PvlKeyword("CoefficientCube", coCube.fileName().toStdString());
  calgrp += PvlKeyword("AbsoluteCoefficient", std::to_string(absCoef));

  ocube->putGroup(calgrp);

  // Start the line-by-line calibration sequence
  p.StartProcess(Mar10Cal);
  p.EndProcess();
}

// Line processing routine
void Mar10Cal (std::vector<Isis::Buffer *> & inCubes,
    std::vector<Isis::Buffer *> & outCubes) {

  Buffer & in = *inCubes[0];
  Buffer & dc = *inCubes[1];
  Buffer & out = *outCubes[0];
  Buffer * blem = (useBlem) ? inCubes[2] : 0;

  coef->SetBasePosition(1, in.Line(), 1);
  coCube.read(*coef);

  // Loop and apply calibration
  for (int samp = 0; samp < in.size(); samp++) {
    // Handle special pixels
    if (IsSpecial(in[samp])) {
      out[samp] = in[samp];
    }
    else if (IsSpecial(dc[samp]) || (useBlem && IsSpecial((*blem)[samp]))) {
      out[samp] = Isis::Null;
    }
    else if (
        IsSpecial(coef->at(coef->Index(samp+1, in.Line(), 1))),
        IsSpecial(coef->at(coef->Index(samp+1, in.Line(), 2))),
        IsSpecial(coef->at(coef->Index(samp+1, in.Line(), 3))),
        IsSpecial(coef->at(coef->Index(samp+1, in.Line(), 4))),
        IsSpecial(coef->at(coef->Index(samp+1, in.Line(), 5))),
        IsSpecial(dc.at(dc.Index(samp+1, in.Line(), 1)))) {
      out[samp] = Isis::Null;
    }
    else if (mask && (in[samp] < coef->at(coef->Index(samp+1, in.Line(), 5)) ||
          in[samp] > coef->at(coef->Index(samp+1, in.Line(), 6)))) {
      out[samp] = Isis::Null;
    }
    else if (blem && blem->at(blem->Index(samp+1, in.Line(), 1)) == Isis::Null) {
      out[samp] = Isis::Null;
    }
    else { //  OK, all pixels look good, calibrate
      // Subtract space derived DC file from M10 image
      double dcCorrected = in[samp] - dc[samp];
      if (dcCorrected <= 0.0) {
        out[samp] = Isis::Null;
      }
      else {
        double x = xparm;
        for (int iteration = 0; iteration < 9; iteration++) {

          // Ax^3 + Bx^2 + Cx + D = 0 'normal cubic equation'
          double numerator =
            (coef->at(coef->Index(samp+1, in.Line(), 4)) * pow(x, 3)) +
            (coef->at(coef->Index(samp+1, in.Line(), 3)) * pow(x, 2)) +
            (coef->at(coef->Index(samp+1, in.Line(), 2)) * x) +
            (coef->at(coef->Index(samp+1, in.Line(), 1)) - dcCorrected);

          // Ax^2 + Bx + C = 0
          double denominator =
            (3 * coef->at(coef->Index(samp+1, in.Line(), 4)) * pow(x, 2)) +
            (2 * coef->at(coef->Index(samp+1, in.Line(), 3)) * x) +
            coef->at(coef->Index(samp+1, in.Line(), 2));

          if (denominator != 0.0) x -= numerator / denominator;
        }

        out[samp] = (x * pow(sunDist, 2)) * absCoef / correctedExp;
      }
    }
  }
}
