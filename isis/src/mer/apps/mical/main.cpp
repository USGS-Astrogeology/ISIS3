/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#define GUIHELPERS

#include "Isis.h"
#include "ProcessByLine.h"
#include "Pvl.h"
#include "UserInterface.h"
#include "IException.h"
#include "MiCalibration.h"
#include "iTime.h"
#include "Brick.h"
#include "Histogram.h"

#include <cmath>
#include <map>

using namespace std;
using namespace Isis;

//helper button functions in the code
void  helperButtonLogCalKernel();

map <QString, void *> GuiHelpers() {
  map <QString, void *> helper;
  helper ["helperButtonLogCalKernel"] = (void *) helperButtonLogCalKernel;
  return helper;
}

namespace gbl {
  void Calibrate(vector<Buffer *>&in, vector<Buffer *>&out);

  Mer::MiCalibration *mi;
  double sunAU;

  double ReferencePixelValue;

  int useReferenceValue;
  int useZeroExposureValue;
  int useActiveAreaValue;
}

PvlGroup calgrp;
QString stagestop;

void IsisMain() {
  //We will be processing by line
  ProcessByLine p;


  UserInterface &ui = Application::GetUserInterface();
  stagestop = ui.GetAsString("CALSTAGE");

  Cube *pack = p.SetInputCube("FROM");
  PvlGroup calgrp("Radiometry");

  //check if the image is calibrated
  if(pack->hasGroup("Radiometry")) {
    std::string msg = "The MI image [" + pack->fileName().toStdString() + "] has already "
                 "been radiometrically calibrated";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  //Open the calibration kernel that contains constrnts for each camera
  QString calKernelFile;
  if(ui.WasEntered("CALKERNEL")) {
    calKernelFile = ui.GetFileName("CALKERNEL");
    cout << "use the user kernel" << endl;
  }
  else {
    calKernelFile = p.MissionData("mer", "calibration/mical.ker.???", true);
    cout << "use the system kernel" << endl;
  }
  Pvl calKernel(calKernelFile.toStdString());
//  gbl::mi = new Mer::MiCalibration(*(pack->GetCube()), calKernel);
  gbl::mi = new Mer::MiCalibration(*(pack), calKernel);
  calgrp += PvlKeyword("CalibrationKernel", calKernelFile.toStdString());


  // See if User entered a temperature and call setTemperature functions
  if(ui.WasEntered("CCDtemp")) {
    gbl::mi->SetCCDTemperature(ui.GetDouble("CCDtemp"));
  }
  if(ui.WasEntered("PCBtemp")) {
    gbl::mi->SetPCBTemperature(ui.GetDouble("PCBtemp"));
  }

  iTime startTime = gbl::mi->StartTime();
  double ETstartTime = startTime.Et();
  //Get the distance between Mars and the Sun at the given time in
  // Astronomical Units (AU)
  QString bspKernel = p.MissionData("base", "/kernels/spk/de???.bsp", true);
  furnsh_c(bspKernel.toLatin1().data());
  QString satKernel = p.MissionData("base", "/kernels/spk/mar???.bsp", true);
  furnsh_c(satKernel.toLatin1().data());
  QString pckKernel = p.MissionData("base", "/kernels/pck/pck?????.tpc", true);
  furnsh_c(pckKernel.toLatin1().data());
  double sunpos[6], lt;
  spkezr_c("sun", ETstartTime, "iau_mars", "LT+S", "mars", sunpos, &lt);
  double dist = vnorm_c(sunpos);
  double kmperAU = 1.4959787066E8;
  gbl::sunAU = dist / kmperAU;
  unload_c(bspKernel.toLatin1().data());
  unload_c(satKernel.toLatin1().data());
  unload_c(pckKernel.toLatin1().data());


  //See what calibtation values the user wants to apply
  //run SetReferencePixelModel if RPCORRECTION for gui is false no correction is done
  gbl::useReferenceValue = 1;
  gbl::useZeroExposureValue = 1;
  gbl::useActiveAreaValue = 1;
  // if user wants NO reference value correction or if shutter effect
  // correction is true set the user value to zero and set label output values
  // to reflect no correction.
  if(!ui.GetBoolean("RPCORRECTION") || gbl::mi->ShutterEffectCorrectionFlag() == "TRUE") {
    gbl::useReferenceValue = 0;
    calgrp += PvlKeyword("ReferencePixelValueSource", "N/A");
    calgrp += PvlKeyword("ReferencePixelValue", "0");
    calgrp += PvlKeyword("ReferencePixelImage", "NoCorrection");
  }
  //  find out if user entered a ERP file.  If yes then get the AVG for the
  //  Reference Pixel value.  If no file is entered then use the model to
  //  get the reference value.
  else if(ui.WasEntered("REFPIXIMAGE")) {
    Brick *b;
    Cube ERPfile;
    ERPfile.open(ui.GetCubeName("REFPIXIMAGE"));
    b = new Brick(11, 201, 1, ERPfile.pixelType());
    b->SetBasePosition(4, 412, 1);
    ERPfile.read(*b);

    Statistics stat;
    b->SetBaseLine(5);

    stat.AddData(b->DoubleBuffer(), b->size());
    gbl::ReferencePixelValue = stat.Average();

    calgrp += PvlKeyword("ReferencePixelValueSource", "ERPImage");
    calgrp += PvlKeyword("ReferencePixelValueImage", ui.GetCubeName("REFPIXIMAGE").toStdString());
    calgrp += PvlKeyword("ReferencePixelValue", std::to_string(gbl::ReferencePixelValue));
  }
  else {
    gbl::ReferencePixelValue = gbl::mi->ReferencePixelModel();
    calgrp += PvlKeyword("ReferencePixelValueSource", "ERPModel");
    calgrp += PvlKeyword("ReferenceModel", std::to_string(gbl::ReferencePixelValue));
  }
  // if user wants NO zero exposure or if shutter effect correction is true
  // set the user value to zero and set label output to reflect no correction
  if(!ui.GetBoolean("ZECORRECTION") || gbl::mi->ShutterEffectCorrectionFlag() == "TRUE") {
    gbl::useZeroExposureValue = 0;
    calgrp += PvlKeyword("ZeroExposureValue", "0");
    calgrp += PvlKeyword("ZeroExposureImage", "NoCorrection");
  }
  else {
    calgrp += PvlKeyword("ZeroExposureValue", std::to_string(gbl::mi->ZeroExposureValue()));
    calgrp += PvlKeyword("ZeroExposureImage", gbl::mi->ZeroExposureImage().toStdString());
  }
  // If user wants NO active area set user value to zero and set the label
  // output values to reflect no correction.
  if(!ui.GetBoolean("AACORRECTION")) {
    gbl::useActiveAreaValue = 0;
    calgrp += PvlKeyword("ActiveAreaValue", "0");
    calgrp += PvlKeyword("ActiveAreaImage", "NoCorrection");
  }
  else {
    calgrp += PvlKeyword("ActiveAreaValue", std::to_string(gbl::mi->ActiveAreaValue()));
    calgrp += PvlKeyword("ActiveAreaImage", gbl::mi->ActiveAreaImage().toStdString());
  }


  // get the Ref, Zero, and Act images in to buffers
  CubeAttributeInput att;
  p.SetInputCube(gbl::mi->ReferencePixelImage(), att);
  p.SetInputCube(gbl::mi->ZeroExposureImage(), att);
  p.SetInputCube(gbl::mi->ActiveAreaImage(), att);
  //  The flatfield image used is dependent on if
  //  the dust cover is open or closed.
  //

  if(ui.WasEntered("FLATFIELD")) {
    p.SetInputCube(ui.GetCubeName("FLATFIELD"), att);
    if(stagestop == "FLAT" || stagestop == "IOF") {
      calgrp += PvlKeyword("FlatFieldImage", gbl::mi->FlatImageOpen().toStdString());
    }
  }
  else {
    cout << "TEST of cover " << gbl::mi->FilterName().toStdString() << endl;
    if(gbl::mi->FilterName() == "MI_OPEN") {
      p.SetInputCube(gbl::mi->FlatImageOpen(), att);
      if(stagestop == "FLAT" || stagestop == "IOF") {
        calgrp += PvlKeyword("FlatFieldImage", gbl::mi->FlatImageOpen().toStdString());
      }
    }
    else if(gbl::mi->FilterName() == "MI_CLOSED") {
      p.SetInputCube(gbl::mi->FlatImageClosed(), att);
      if(stagestop == "FLAT" || stagestop == "IOF") {
        calgrp += PvlKeyword("FlatFieldImage", gbl::mi->FlatImageClosed().toStdString());
      }
    }
  }


  //calculate full model value and output to the labels.
  double fullModel = gbl::mi->ReferencePixelModel() +
                     gbl::mi->ZeroExposureValue() +
                     gbl::mi->ActiveAreaValue();
  calgrp += PvlKeyword("DarkCurrentFullModel", std::to_string(fullModel));

  //Add temperature values to the radiometry group
  calgrp += PvlKeyword("CCDTemperture", std::to_string(gbl::mi->CCDTemperatureCorrect()));
  calgrp += PvlKeyword("PCBTemperature", std::to_string(gbl::mi->PCBTemperature()));
  if(stagestop == "IOF") {
    calgrp += PvlKeyword("OmegaNaught", std::to_string(gbl::mi->OmegaNaught()));
    calgrp += PvlKeyword("SunAU", std::to_string(gbl::sunAU));
  }

//write Radiometry group to the output cube.
  Cube *opack = p.SetOutputCube("TO");
  opack->putGroup(calgrp);

  p.StartProcess(gbl::Calibrate);
  p.EndProcess();
}

//Line processing routine

void gbl::Calibrate(vector<Buffer *>&in, vector<Buffer *>&out) {
  Buffer &ibuf = *in[0];
  Buffer &rbuf = *in[1];
  Buffer &zbuf = *in[2];
  Buffer &abuf = *in[3];
  Buffer &fbuf = *in[4];
  Buffer &obuf = *out[0];

//  Do the dark current correction  (Note that if shutter effect correction
// flag is  true then the userReferenceValue and useZeroExposureValue
// are set to zero so there is no correction.
  for(int samp = 0; samp < ibuf.size(); samp++) {
    obuf[samp] = ibuf[samp] - (((gbl::ReferencePixelValue + rbuf[samp])
                                * useReferenceValue) +
                               ((gbl::mi->ZeroExposureValue() * zbuf[samp]) * useZeroExposureValue) +
                               ((gbl::mi->ActiveAreaValue() * abuf[samp]) * useActiveAreaValue));
  }
  if(stagestop == "DARK") return;

  //--------------------------------------------------------------------
  // Add Desmear correction

  if(mi->ShutterEffectCorrectionFlag() == "FALSE" &&
      (mi->ExposureDuration() < 1000 || mi->ExposureDuration() > 0) && ibuf.size() != 1024) {

    double smearScale = mi->TransferTime() / mi->ExposureDuration() / obuf.size();
    static vector<double> smear;
    if(obuf.Line() == 1) {
      smear.resize(obuf.size());
      for(int i = 0; i < obuf.size(); i++) {
        smear[i] = 0.0;
      }
    }

    for(int samp = 0; samp < obuf.size(); samp++) {

      if(IsValidPixel(obuf[samp])) {
        if(obuf.Line() == 1) {
          smear[samp] = obuf[samp] * smearScale;
          obuf[samp] = obuf[samp];
        }
        else {
          smear[samp] = obuf[samp] * smearScale +
                        smear[samp] * (1.0 - smearScale);
          obuf[samp] = obuf[samp] - smear[samp];
          if(obuf[samp] <= 0.0) {
            obuf[samp] = Isis::Null;
          }
        }
      }
      else {
        obuf[samp] = obuf[samp];
      }
    }
  }

  //-------------------------------------------------
  //Do the flat field correction
  //
  for(int i = 0; i < obuf.size(); i++) {
    obuf[i] = obuf[i] / fbuf[i];
  }
  if(stagestop == "FLAT") return;
  //--------------------------------------------------
  // Do the I/F conversion
  //convert exposure duration from milli_seconds to seconds
  double exposureSeconds = gbl::mi->ExposureDuration() / 1000.0;

  for(int i = 0; i < obuf.size(); i++) {
    if(gbl::mi->FilterName() == "MI_OPEN") {
      obuf[i] = obuf[i] * (pow(gbl::sunAU, 2) / (exposureSeconds * gbl::mi->OmegaNaught()));
    }
    else if(gbl::mi->FilterName() == "MI_CLOSED") {
      obuf[i] = obuf[i] * (pow(gbl::sunAU, 2) /
                           (exposureSeconds * gbl::mi->OmegaNaught()) / .53);
    }
  }
}

//Helper Button Function to display cal Kernel in the log area
void helperButtonLogCalKernel() {
  UserInterface &ui = Application::GetUserInterface();
  QString calKernelFile;
  if(ui.WasEntered("CALKERNEL")) {
    calKernelFile = ui.GetFileName("CALKERNEL");
  }
  else {
    //    calKernelFile = p.MissionData("mer","/calibration/mical.ker.???",true);
    cout << "use the system kernel" << endl;
  }

  Pvl p;
  p.read(calKernelFile.toStdString());
  QString OQString = "********** Output of [" + calKernelFile + "] *********";
  Application::GuiLog(OQString);
  Application::GuiLog(p);
}
