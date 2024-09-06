/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <string>

#include "ThemisVisCamera.h"
#include "SpecialPixel.h"
#include "ProcessByLine.h"
#include "CameraFactory.h"
#include "PushFrameCameraGroundMap.h"

using namespace Isis;
using namespace std;

void TrimFramelets(Buffer &, Buffer &);
bool NeedsTrimmed(int);
int frameletSize;
int frameletTopTrimSize, frameletBottomTrimSize,
    frameletLeftTrimSize, frameletRightTrimSize;

void CalculateBottomTrim(Cube *icube);

void IsisMain() {
  // Grab the file to import
  ProcessByLine p;
  UserInterface &ui = Application::GetUserInterface();

  Cube *icube = p.SetInputCube("FROM");

  // Make sure it is a Themis EDR/RDR
  try {
    if(icube->group("Instrument")["InstrumentID"][0] != "THEMIS_VIS") {
      FileName inFileName = ui.GetCubeName("FROM");
      std::string msg = "This program is intended for use on THEMIS VIS images only. [";
      msg += inFileName.expanded() + "] does not appear to be a THEMIS VIS image.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }
  catch(IException &e) {
    throw IException(e, IException::User,
                     "Unable to run thmvistrim with the given input cube.", _FILEINFO_);
  }

  p.SetOutputCube("TO");

  frameletSize = 192 / std::stoi(icube->group("Instrument")["SpatialSumming"][0]);
  frameletTopTrimSize = ui.GetInteger("TOPTRIM");
  frameletLeftTrimSize = ui.GetInteger("LEFTTRIM");
  frameletRightTrimSize = ui.GetInteger("RIGHTTRIM");

  if(ui.WasEntered("BOTTOMTRIM")) {
    frameletBottomTrimSize = ui.GetInteger("BOTTOMTRIM");
  }
  else {
    CalculateBottomTrim(icube);
  }

  p.StartProcess(TrimFramelets);
  p.EndProcess();
}

void TrimFramelets(Buffer &inBuffer, Buffer &outBuffer) {
  if(NeedsTrimmed(inBuffer.Line())) {
    for(int i = 0; i < outBuffer.size(); i++) {
      outBuffer[i] = Isis::Null;
    }
  }
  else {
    for(int i = 0; i < outBuffer.size(); i++) {
      if((i > frameletLeftTrimSize) && (i < outBuffer.size() - frameletRightTrimSize)) {
        outBuffer[i] = inBuffer[i];
      }
      else {
        outBuffer[i] = Isis::Null;
      }
    }
  }
}

bool NeedsTrimmed(int line) {
  int frameletLine = (line - 1) % frameletSize + 1;
  return (frameletLine <= frameletTopTrimSize) || (frameletLine > (frameletSize - frameletBottomTrimSize));
}

/**
 * This method uses the cube's camera to determine how much
 *   overlap exists. The lat,lon for the beginning of the
 *   second framelet is calculated, and then we determine where
 *   that lat,lon occurs in the first framelet. That occurring line
 *   minus the framelet size is how much vertical overlap there is.
 *   The top overlap is subtracted from the overlap because there is
 *   that much less vertical overlap.
 *
 * @param icube The input themis vis cube
 */
void CalculateBottomTrim(Cube *icube) {
  frameletBottomTrimSize = 0;

  if(icube->camera() == NULL) {
    string msg = "A camera is required to automatically calculate the bottom "
                 "trim of a cube. Please run spiceinit on the input cube";
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }

  // We really don't care at all about the original camera. What's needed is
  //   a known even-framelet camera and a known odd-framelet camera. In order
  //   to get these, we change the cube labels in a local copy and create an
  //   odd framelet and an even framelet camera.
  Pvl &cubeLabels = *icube->label();
  PvlKeyword &framelets = cubeLabels.findGroup("Instrument", Pvl::Traverse)["Framelets"];
  framelets = "Even";
  Camera *camEven = CameraFactory::Create(*icube);

  framelets = "Odd";
  Camera *camOdd = CameraFactory::Create(*icube);

  // Framelet 2 is even, so let's use the even camera to find the lat,lon at it's beginning
  if(camEven->SetImage(1, frameletSize + 1)) {
    double framelet2StartLat = camEven->UniversalLatitude();
    double framelet2StartLon = camEven->UniversalLongitude();

    // Let's figure out where this is in the nearest odd framelet (hopefully framelet 1)
    if(camOdd->SetUniversalGround(framelet2StartLat, framelet2StartLon)) {
      // The equivalent line to the start of framelet 2 is this found line
      int equivalentLine = (int)(camOdd->Line() + 0.5);

      // Trim the vertical overlap...
      frameletBottomTrimSize = frameletSize - equivalentLine;

      // Compensate for the top trim...
      frameletBottomTrimSize -= frameletTopTrimSize;

      // This will happen if the top trim is bigger than the overlap,
      //   make sure the bottom trim is good
      if(frameletBottomTrimSize < 0) frameletBottomTrimSize = 0;
    }
  }

  delete camEven;
  delete camOdd;
}
