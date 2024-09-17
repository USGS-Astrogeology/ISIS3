/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iomanip>
#include <iostream>

#include "Camera.h"
#include "CameraFactory.h"
#include "FileName.h"
#include "IException.h"
#include "LroWideAngleCamera.h"
#include "Preference.h"
#include "Pvl.h"

using namespace std;
using namespace Isis;

// 2013-12-19 We will have new CK kernels with temperature dependence coming
// that will affect the lat/lon values here.  Also must get a new FK, IK and
// IAK.  Don't forget to move the new test cube to $ISISTESTDATA/isis/src/lro/unitTestData!

void TestLineSamp(Camera *cam, double samp, double line);

int main(void) {
  Preference::Preferences(true);

  cout << "Unit Test for LroWideAngleCamera..." << endl;

  try {

    // Support different camera model versions thusly...
    Cube cube;
    cube.open("$ISISTESTDATA/isis/src/lro/unitTestData/wacCameraTest.cub");
    int cmVersion = CameraFactory::CameraVersion(cube);

    // These should be lat/lon at center of image. To obtain these numbers for a new cube/camera,
    // set both the known lat and known lon to zero and copy the unit test output "Latitude off by: "
    // and "Longitude off by: " values directly into these variables.
    double knownLat, knownLon;

    // Camera Version 1's test is provided here for easy testing with an old version of the camera,
    //   this is generally not executed
    if (cmVersion == 1) {
     knownLat = -70.69638475050628;
     knownLon =  244.3314992195277;
     cube.open("$ISISTESTDATA/isis/src/lro/unitTestData/wacCameraTest.cub.cv1");
    }
    else {
      // Version 2 or higher difference caused by new CK and comprehensive IK
      //   kernel support and no longer using a DEM
      knownLat = -70.7067960917672735;
      knownLon = 244.3369098738304217;
    }


    Camera *cam = CameraFactory::Create(cube);
    cout << "FileName: " << FileName(cube.fileName().toStdString()).name() << endl;
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

    // Test all four corners to make sure the conversions are right
    cout << "For upper left corner ..." << endl;
    TestLineSamp(cam, 1.0, 15.0);

    cout << "For upper right corner ..." << endl;
    TestLineSamp(cam, cam->Samples(), 15.0);

    cout << "For lower left corner ..." << endl;
    TestLineSamp(cam, 1.0, 56);

    cout << "For lower right corner ..." << endl;
    TestLineSamp(cam, cam->Samples(), 56);

    double samp = cam->Samples() / 2;
    double line = cam->Lines() / 2;
    cout << "For center pixel position ..." << endl;

    if(!cam->SetImage(samp, line)) {
      cout << "ERROR" << endl;
      return 0;
    }

    if(abs(cam->UniversalLatitude() - knownLat) < 1E-10) {
      cout << "Latitude OK" << endl;
    }
    else {
      cout << setprecision(16) << "Latitude off by: " << cam->UniversalLatitude() - knownLat << endl;
    }

    if(abs(cam->UniversalLongitude() - knownLon) < 1E-10) {
      cout << "Longitude OK" << endl;
    }
    else {
      cout << setprecision(16) << "Longitude off by: " << cam->UniversalLongitude() - knownLon << endl;
    }

    Cube c2("$ISISTESTDATA/isis/src/lro/unitTestData/out.uv.even.cub", "r");
    Camera *cam2 = CameraFactory::Create(c2);
    // Test name methods for WAC-UV
    cout << endl << endl << "Testing name methods ..." << endl;
    cout << "Spacecraft Name Long: " << cam2->spacecraftNameLong().toStdString() << endl;
    cout << "Spacecraft Name Short: " << cam2->spacecraftNameShort().toStdString() << endl;
    cout << "Instrument Name Long: " << cam2->instrumentNameLong().toStdString() << endl;
    cout << "Instrument Name Short: " << cam2->instrumentNameShort().toStdString() << endl << endl;

    // Test exceptions for determining names
    cout << endl << "Testing exceptions ..." << endl << endl;
    Cube test("$ISISTESTDATA/isis/src/hayabusa/unitTestData/st_2530292409_v.cub", "r");
    LroWideAngleCamera lCam(test);
  }
  catch(IException &e) {
    e.print();
  }
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
