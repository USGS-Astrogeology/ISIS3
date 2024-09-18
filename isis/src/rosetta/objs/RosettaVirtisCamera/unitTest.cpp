/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iomanip>
#include <iostream>
#include <algorithm>

#include "Camera.h"
#include "CameraFactory.h"
#include "RosettaVirtisCamera.h"
#include "IException.h"
#include "iTime.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlGroup.h"

using namespace std;
using namespace Isis;

void TestLineSamp(Camera *cam, double samp, double line);
int TestCamera(Camera *cam, Cube c, double lines, double knownLat, double knownLon);

/**
 *
 * Unit test for DawnVirCamera.
 *
 * @author ????-??-?? Unknown
 *
 * @internal
 *   @history 2018-08-28 Kris Becker - Initial unit test for Rosetta VIRTIS
 *            instrument
 *   @history 2019-07-02 Krisitn Berry - Update to test only level 3 (calibrated) images, as only
 *            calibrated images are currently supported by the camera model.
 */
int main(void) {
  Preference::Preferences(true);

  cout << "Unit Test for RosettaVirtisCamera..." << endl;
  try {
    // These should be lat/lon at center of image. To obtain these numbers for a new cube/camera,
    // set both the known lat and known lon to zero and copy the unit test output "Latitude off by: "
    // and "Longitude off by: " values directly into these variables.
    double knownLat = 55.0877030799486249;
    double knownLon = 9.3721561274843133;

    cout << endl << "Teting Level 3 (Calibrated) VIRTIS-M-VIS Cube ..." << endl;
    Cube visCube("$ISISTESTDATA/isis/src/rosetta/unitTestData/V1_00388238556.cub", "r");
    double lines = 100.0;
    RosettaVirtisCamera *cam = (RosettaVirtisCamera *) CameraFactory::Create(visCube);
    TestCamera(cam, visCube, lines, knownLat, knownLon);

    cout << endl << "Teting Level 3 (Calibrated) VIRTIS-M-IR Cube ..." << endl;
    knownLat = 29.1974649731145028;
    knownLon = 346.8749209987247468;
    lines = 67;
    Cube irCube("$ISISTESTDATA/isis/src/rosetta/unitTestData/I1_00382172310.cub", "r");
    RosettaVirtisCamera *cam2 = (RosettaVirtisCamera *) CameraFactory::Create(irCube);
    TestCamera(cam2, irCube, lines, knownLat, knownLon);
  }
  catch(IException &e) {
    e.print();
  }
}

int TestCamera(Camera *cam, Cube c, double lines, double knownLat, double knownLon) {
  cout << "FileName: " << FileName(c.fileName().toStdString()).name() << endl;
  cout << "CK Frame: " << cam->instrumentRotation()->Frame() << endl << endl;
  cout.setf(std::ios::fixed);
  cout << setprecision(9);

  // Test kernel IDs
  cout << "Kernel IDs: " << endl;
  cout << "CK Frame ID = " << cam->CkFrameId() << endl;
  cout << "CK Reference ID = " << cam->CkReferenceId() << endl;
  cout << "SPK Target ID = " << cam->SpkTargetId() << endl;
  cout << "SPK Reference ID = " << cam->SpkReferenceId() << endl << endl;

  // Test name methods
  cout << "Spacecraft Name Long: " << cam->spacecraftNameLong().toStdString() << endl;
  cout << "Spacecraft Name Short: " << cam->spacecraftNameShort().toStdString() << endl;
  cout << "Instrument Name Long: " << cam->instrumentNameLong().toStdString() << endl;
  cout << "Instrument Name Short: " << cam->instrumentNameShort().toStdString() << endl << endl;

  // Test Shutter Open/Close
  std::pair< double, double > imgTimes = cam->StartEndEphemerisTimes();
  cout << "Start Time: " << imgTimes.first << "\n";
  cout << "End Time:   " << imgTimes.second << "\n";

  // Test all four corners to make sure the conversions are right
  // For Rosetta, could not find images with the comet at all 4 corners, so these are
  // actually corners of the comet, not the image. Values were chosen to approximate
  // the corners on two different images (VIS and IR).

  // good.
  cout << "For upper left corner ..." << endl;
  TestLineSamp(cam, 138.0, 10.0);

  cout << "For upper right corner ..." << endl;
  TestLineSamp(cam, 165.0, 19.0);

  // fixme?
  cout << "For lower left corner ..." << endl;
  TestLineSamp(cam, 138.0, 55.0);

  cout << "For lower right corner ..." << endl;
  TestLineSamp(cam, 130.0, 55.0);

  double samp = 128.0;
  double line = lines/2.0;
  cout << "For center pixel position ..." << endl;

  if(!cam->SetImage(samp, line)) {
    cout << "ERROR" << endl;
    return 0;
  }

  if(abs(cam->UniversalLatitude() - knownLat) < 6E-12) {
    cout << "Latitude OK" << endl;
  }
  else {
    cout << setprecision(16) << "Latitude off by: " << cam->UniversalLatitude() - knownLat << endl;
  }

  if(abs(cam->UniversalLongitude() - knownLon) < 6E-12) {
    cout << "Longitude OK" << endl;
  }
  else {
    cout << setprecision(16) << "Longitude off by: " << cam->UniversalLongitude() - knownLon << endl;
  }
  return 0;
}

void TestLineSamp(Camera *cam, double samp, double line) {
  bool success = cam->SetImage(samp, line);

  if(success) {
    success = cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude());
  }

  if(success) {
    double deltaSamp = samp - cam->Sample();
    double deltaLine = line - cam->Line();
    if(fabs(deltaSamp) < 0.01) deltaSamp = 0;
    if(fabs(deltaLine) < 0.01) deltaLine = 0;
    cout << "DeltaSample = " << deltaSamp << endl;
    cout << "DeltaLine = " << deltaLine << endl << endl;
  }
  else {
    cout << "DeltaSample = ERROR" << endl;
    cout << "DeltaLine = ERROR" << endl << endl;
  }
}
