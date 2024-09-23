/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <QList>
#include <QString>
#include <QStringList>

#include "Buffer.h"
#include "Camera.h"
#include "CubeAttribute.h"
#include "IException.h"
#include "ProcessByLine.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "SpecialPixel.h"
#include "UserInterface.h"
#include "IString.h"

using namespace std;
using namespace Isis;

void calibration(vector<Buffer *> &in,
                 vector<Buffer *> &out);
PvlObject fetchCoefficients(Pvl &calibration, QList<QString> &hierarchy);
void checkCoefficients(PvlObject &coefficients, QStringList keyNames);

// Linearity correction
bool linear;

// calibration constants
double omegaNaught;
double sunDist;
double gainC;
double offsetC;
double deltaExpo;

// calculated values
double aCoef;
double newExpo;
double XMLT;
double w1;
double dist1;

// linear constants
double bHighEnd;
double kPowerOf;
double normalizingPower;

void IsisMain() {
  // Will be processing by line
  ProcessByLine p;

  UserInterface &ui = Application::GetUserInterface();

  Cube * incube = p.SetInputCube("FROM");
  Cube * ocube = p.SetOutputCube("TO");

  // Check for projection
  if (incube->isProjected()) {
    std::string msg = "The cube [" + ui.GetCubeName("FROM").toStdString() + "] has a projection" +
                  " and cannot be radiometrically calibrated";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // access important label objects, will be used later.
  PvlObject isiscube = incube->label()->findObject("IsisCube");
  PvlGroup instrument = isiscube.findGroup("Instrument");
  PvlGroup archive = isiscube.findGroup("Archive");
  PvlGroup bandbin = isiscube.findGroup("BandBin");

  // Verify not radiometrically corrected
  if (isiscube.hasGroup("Radiometry")) {
    std::string msg = "Cube [" + ui.GetCubeName("FROM").toStdString() + "] has already been" +
                  " radiometrically corrected";
    throw IException(IException::User,msg,_FILEINFO_);
  }

  // Verify Voyager spacecraft and get number, 1 or 2
  QString scNumber = QString::fromStdString(instrument["SpacecraftName"][0]);
  if (scNumber != "VOYAGER_1" && scNumber != "VOYAGER_2") {
    std::string msg = "The cube [" + ui.GetCubeName("FROM").toStdString() + "] does not appear" +
                  " to be a Voyager image";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  scNumber = scNumber[8];

  // Open calibration file to find constants and files
  Pvl calibra(FileName("$voyager" + scNumber.toStdString() +
                           "/calibration/voycal.pvl").expanded());
  PvlObject calib;
  QList<QString> hierarchy;
  try {
    // Search voycal.pvl for appropriate object
    hierarchy.append(QString::fromStdString(instrument.findKeyword("SpacecraftName")[0]));
    hierarchy.append(QString::fromStdString(instrument.findKeyword("InstrumentId")[0]));
    hierarchy.append(
        QString("ShutterMode" + QString::fromStdString(instrument.findKeyword("CameraState2")[0])));
    hierarchy.append(QString::fromStdString(archive.findKeyword("MissionPhaseName")[0]));
    hierarchy.append(
        QString("ScanRate" + QString::fromStdString(instrument.findKeyword("CameraState1")[0])));
    hierarchy.append(
        QString(QString::fromStdString(bandbin["FilterName"][0]) + "_" + QString::fromStdString(bandbin["FilterNumber"][0])));

    calib = fetchCoefficients(calibra, hierarchy);

    checkCoefficients(calib, QStringList{"OmegaNaught", "SunDistance", "GainCorrection",
                                         "OffsetCorrection", "DeltaExposureTime"});
  }
  catch (IException &e) {
    string msg = "Could not find match in [voycal.pvl] calibration file,";
    msg += " the error was: ";
    msg += e.what();
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }

  // Get appropriate calibration files
  CubeAttributeInput in1;
  p.SetInputCube(QString::fromStdString(FileName("$voyager" + scNumber.toStdString() + "/calibration/" +
                          (std::string)calib["OffsetCorrectionFile"]).expanded()), in1);
  CubeAttributeInput in2;
  p.SetInputCube(QString::fromStdString(FileName("$voyager" + scNumber.toStdString() + "/calibration/" +
                          (std::string)calib["GainCorrectionFile"]).expanded()), in2);

  // Constants from voycal.pvl for correction
  omegaNaught = calib["OmegaNaught"];
  sunDist = calib["SunDistance"];
  gainC = calib["GainCorrection"];
  offsetC = calib["OffsetCorrection"];
  deltaExpo = calib["DeltaExposureTime"];

  // If we are doing a linear correction as well, go here
  linear = ui.GetBoolean("LINEAR");
  if (linear) {
    Pvl linearity(FileName("$voyager" + scNumber.toStdString() +
                     "/calibration/voylin.pvl").expanded());

    PvlObject lin;
    try {
      // Search voylin.pvl for appropriate object
      lin = fetchCoefficients(linearity, hierarchy);

      checkCoefficients(lin ,QStringList{"NormalizingPower",
                                         "B_HighEndNon-LinearityCorrection",
                                         "K_PowerOfNon-Linearity"});
    }
    catch (IException &e) {
      string msg = "Could not find match in [voylin.pvl] calibration file,";
      msg += " the error was: ";
      msg += e.what();
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    // Calculated constant for linear correction
    aCoef = ((double)lin["NormalizingPower"] - (double)lin["B_HighEndNon-LinearityCorrection"]) /
            (double)lin["NormalizingPower"];

    // Constants from file for linear correction
    bHighEnd = (double)lin["B_HighEndNon-LinearityCorrection"];
    kPowerOf = (double)lin["K_PowerOfNon-Linearity"];
    normalizingPower = (double)lin["NormalizingPower"];
  }

  // Other calculated constants for all corrections
  // Modified exposure duration
  newExpo = ((double)instrument["ExposureDuration"] * 1000) + (double)calib["DeltaExposureTime"];

  // Camera for sun position/distance, so we have to have spice data
  Camera * cam = incube->camera();
  double sunPos[3];
  cam->sunPosition(sunPos);
  // Magnitude of sun vector ||s||
  dist1 = sqrt(sunPos[0]*sunPos[0] + sunPos[1]*sunPos[1] + sunPos[2]*sunPos[2]);

  w1 = (omegaNaught/1000) * ((sunDist * sunDist) / (dist1 * dist1));

  XMLT = 1.0/(newExpo * w1);

  PvlGroup calgrp("Radiometry");
  // Regular calibration equation and constants
  calgrp.addComment("Calibration equation in voycal:");
  calgrp.addComment("OUT(i,j) = (IN(i,j)*GAIN)+DCF(i,j)*XMLT*FFF(i,j)");
  calgrp.addComment("XMLT = 1.0/(EXPO*W1)");
  calgrp.addComment("EXPO = EXPODUR + DELTAEXPO");
  calgrp.addComment("W1 = (W0/1000)*(SUNDIST^2/CALCDIST^2)");
  calgrp.addComment("DCF = OffsetCorrectionFile, FFF = GainCorrectionFile");
  calgrp.addComment("IN = InputCube, GAIN = GainCorrection");
  calgrp.addKeyword(calib[0]);
  calgrp.addKeyword(calib[1]);
  calgrp.addKeyword(calib[2]);
  calgrp.addKeyword(calib[3]);
  calgrp.addKeyword(calib[4]);
  calgrp.addKeyword(calib[5]);
  calgrp.addKeyword(calib[6]);
  calgrp.addKeyword(PvlKeyword("CalcSunDistance",toString(dist1)));
  calgrp.addKeyword(instrument["ExposureDuration"]);
  calgrp.addKeyword(PvlKeyword("XMLT",toString(XMLT)));
  calgrp.addKeyword(PvlKeyword("Omega_W1",toString(w1)));
  calgrp.addKeyword(PvlKeyword("CalcExpoDuration",toString(newExpo)));

  // Linear correction equation and constants
  if (linear) {
    PvlKeyword linearity = PvlKeyword("LinearityCorrection","True");
    linearity.addComment("Linearity correction equation:");
    linearity.addComment("OUT(i,j) = LIN( (IN(i,j)*GAIN)+DCF(i,j) )*XMLT*FFF(i,j)");
    linearity.addComment("LIN(X) = ACOEF*X+BCOEF*(X/XNORM)^KPOWER");
    linearity.addComment("BCOEF = B_HighEndeNon-LinearityCorrection");
    linearity.addComment("XNORM = NormalizingPower");
    linearity.addComment("KPOWER = K_PowerOfNon-Linearity");
    calgrp.addKeyword(linearity);
    calgrp.addKeyword(PvlKeyword("ACoefficient",toString(aCoef)));
    calgrp.addKeyword(PvlKeyword("B_HighEndNon-LinearityCorrection",toString(bHighEnd)));
    calgrp.addKeyword(PvlKeyword("K_PowerOfNon-Linearity",toString(kPowerOf)));
    calgrp.addKeyword(PvlKeyword("NormalizingPower",toString(normalizingPower)));
  }
  else {
    calgrp.addKeyword(PvlKeyword("LinearityCorrection","False"));
  }

  // Add Radiometry group
  ocube->putGroup(calgrp);

  p.StartProcess(calibration);
  p.EndProcess();
}

// calibration method
void calibration(vector<Buffer *> &in, vector<Buffer *> &out) {

  Buffer &inp = *in[0];      // Input Cube
  Buffer &dcf = *in[1];      // Dark Current File
  Buffer &fff = *in[2];      // Flat Field File
  Buffer &outp = *out[0];    // Output Cube

  for (int i = 0; i < inp.size(); i++) {
    if(IsSpecial(inp[i])) {
      outp[i] = inp[i];
    }
    else if(IsSpecial(fff[i]) || IsSpecial(dcf[i])) {
      outp[i] = Isis::Null;
    }
    else {
      // Initial calculation
      double dnraw = (inp[i] * gainC) + dcf[i];
      // Linear option
      if (linear) {
        dnraw = aCoef * dnraw + bHighEnd * (pow((dnraw / normalizingPower), kPowerOf));
      }
      outp[i] = XMLT * fff[i] * dnraw;
    }
  }
}


/**
 * Fetches all the coefficients for a calibration in a single top-level
 * PvlObject.  The calibration PVL will be explored from the top-down looking
 * for keywords.  Objects named in the hierarchy list will be explored down
 * the chain in the order listed.  So, for example, if hierarchy is the list
 * ["A", "B", "C"], then this function will assume that "A" is a child of the
 * top-level calibration PvlObject, "B" is a child of "A", and "C" is a child
 * of "B".
 *
 * If any keyword found in a child object further down the chain conflicts
 * with a keyword found higher up, the child object's keyword will overwrite
 * the value of that keyword in the coefficients object.  In this way,
 * calibration files can define top-level "default" coefficients that apply to
 * the entire mission, and can provide camera- or filter-specific coefficients
 * to overwrite them as needed.
 *
 * @param calibration The mission-specific calibration PVL file
 * @param hierarchy A list of hierarchical PvlObject names to search
 *
 * @return A single object containing all coefficients
 */
PvlObject fetchCoefficients(Pvl &calibration, QList<QString> &hierarchy) {
  // All coefficients go into one top-level object without any children
  PvlObject coefficients;

  // Add all the keywords from the calibration PVL top-level object
  for (int k = 0; k < calibration.keywords(); k++)
    coefficients.addKeyword(calibration[k]);

  // Iterate over every object in the hierarchy looking for coefficient
  // keywords.  The first string is the name of the first object, the second
  // the name of the next object down, etc. We start from the top-level PVL
  // object and work our way down.
  bool validHierarchy = true;
  PvlObject *parent = &calibration;
  for (int o = 0; o < hierarchy.size() && validHierarchy; o++) {
    QString objectName = hierarchy[o];

    if (parent->hasObject(objectName.toStdString())) {
      // The object named in the hierarchy exists in the calibration file, so
      // grab it
      PvlObject &object = parent->findObject(objectName.toStdString());

      // Find all the keywords at the object level
      for (int k = 0; k < object.keywords(); k++) {
        PvlKeyword &keyword = object[k];
        if (coefficients.hasKeyword(keyword.name())) {
          // The coefficients object already has a value for this coefficient
          // keyword.  Because this one is lower down the chain, it is more
          // specifically defined, thus we should use it instead.
          PvlKeyword &coefficient = coefficients.findKeyword(keyword.name());
          for (int i = 0; i < coefficient.size(); i++) {
            coefficient[i] = keyword[i];
          }
        }
        else {
          // This is a new keyword, so add it to the coefficients object
          coefficients.addKeyword(object[k]);
        }
      }

      // Update the parent object for the next object down the chain
      parent = &object;
    }
    else {
      // We've reached a dead end, our hierarchy no longer makes sense, so
      // bail out
      validHierarchy = false;
    }
  }

  return coefficients;
}


void checkCoefficients(PvlObject &coefficients, QStringList keyNames) {
  QStringList missingCoeffs;
  foreach(const QString &key, keyNames) {
    if (!coefficients.hasKeyword(key.toStdString())) {
      missingCoeffs.append(key);
    }
  }
  if (!missingCoeffs.isEmpty()) {
    throw IException(
        IException::Programmer,
        "Coefficients [" + missingCoeffs.join(", ").toStdString() + "] were not found in the "
        "calibration PVL for the input data.  Consider adding a default.",
        _FILEINFO_);
  }
}
