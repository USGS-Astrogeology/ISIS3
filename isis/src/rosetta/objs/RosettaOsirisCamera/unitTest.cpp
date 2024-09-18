/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iomanip>
#include <iostream>

#include "Camera.h"
#include "CameraFactory.h"
#include "LinearAlgebra.h"
#include "RosettaOsirisCamera.h"
#include "RosettaOsirisCameraDistortionMap.h"
#include "IException.h"
#include "iTime.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlGroup.h"

using namespace std;
using namespace Isis;

void TestLineSamp(Camera *cam, double samp, double line);

int main(void) {
  Preference::Preferences(true);

  QString testNACFile("$ISISTESTDATA/isis/src/rosetta/unitTestData/n20100710t154539230id20f22.cub");
  QString testWACFile("$ISISTESTDATA/isis/src/rosetta/unitTestData/W20100710T153440162ID30F13.cub");

  cout << "Unit Test for RosettaOsirisCameraDistortionMap..." << endl;
  try {
    Cube c(testNACFile.toStdString(), "r");
    Camera *cam = c.camera();
    RosettaOsirisCameraDistortionMap *testMap = new RosettaOsirisCameraDistortionMap(cam);
    cout << "Create default distortion map" << endl;
    cout << endl;

    cout << "Set distorted coordinates" << endl;
    double dx = 0.5;
    double dy = 0.75;
    testMap->SetFocalPlane(dx, dy);
    cout << "  Distorted coordinates: ("
         << toString( testMap->FocalPlaneX() ) << ", "
         << toString( testMap->FocalPlaneY() ) << ")" << endl;
    cout << "  Undistorted coordinates: ("
         << toString( testMap->UndistortedFocalPlaneX() ) << ", "
         << toString( testMap->UndistortedFocalPlaneY() ) << ")" << endl;
    cout << endl;

    cout << "Set undistorted coordinates" << endl;
    double ux = 0.25;
    double uy = 0.1;
    testMap->SetUndistortedFocalPlane(ux, uy);
    cout << "  Distorted coordinates: ("
         << toString( testMap->FocalPlaneX() ) << ", "
         << toString( testMap->FocalPlaneY() ) << ")" << endl;
    cout << "  Undistorted coordinates: ("
         << toString( testMap->UndistortedFocalPlaneX() ) << ", "
         << toString( testMap->UndistortedFocalPlaneY() ) << ")" << endl;
    cout << endl;

    cout << "Modify the coefficient matrices" << endl << endl;
    LinearAlgebra::Matrix toUndistX = LinearAlgebra::zeroMatrix(4, 4);
    LinearAlgebra::Matrix toUndistY = LinearAlgebra::zeroMatrix(4, 4);
    toUndistX(0, 0) =  1.0; toUndistX(1, 1) =  2.0; toUndistX(2, 2) =  3.0; toUndistX(3, 3) =  4.0;
    toUndistY(0, 0) = -1.0; toUndistY(1, 0) = -2.0; toUndistY(2, 0) = -3.0; toUndistY(3, 0) = -4.0;

    testMap->setUnDistortedXMatrix(toUndistX);
    testMap->setUnDistortedYMatrix(toUndistY);

    cout << "Set distorted coordinates" << endl;
    dx = 1.0;
    dy = 1.0;
    testMap->SetFocalPlane(dx, dy);
    cout << "  Distorted coordinates: ("
         << toString( testMap->FocalPlaneX() ) << ", "
         << toString( testMap->FocalPlaneY() ) << ")" << endl;
    cout << "  Undistorted coordinates: ("
         << toString( testMap->UndistortedFocalPlaneX() ) << ", "
         << toString( testMap->UndistortedFocalPlaneY() ) << ")" << endl;
    cout << endl;

    cout << "Set distorted coordinates" << endl;
    dx = 0.0;
    dy = 1.0;
    testMap->SetFocalPlane(dx, dy);
    cout << "  Distorted coordinates: ("
         << toString( testMap->FocalPlaneX() ) << ", "
         << toString( testMap->FocalPlaneY() ) << ")" << endl;
    cout << "  Undistorted coordinates: ("
         << toString( testMap->UndistortedFocalPlaneX() ) << ", "
         << toString( testMap->UndistortedFocalPlaneY() ) << ")" << endl;
    cout << endl;

    cout << "Set distorted coordinates" << endl;
    dx = 1.0;
    dy = 0.0;
    testMap->SetFocalPlane(dx, dy);
    cout << "  Distorted coordinates: ("
         << toString( testMap->FocalPlaneX() ) << ", "
         << toString( testMap->FocalPlaneY() ) << ")" << endl;
    cout << "  Undistorted coordinates: ("
         << toString( testMap->UndistortedFocalPlaneX() ) << ", "
         << toString( testMap->UndistortedFocalPlaneY() ) << ")" << endl;
    cout << endl;

    cout << "Set undistorted coordinates" << endl;
    ux = 1.0;
    uy = 1.0;
    testMap->SetUndistortedFocalPlane(ux, uy);
    cout << "  Distorted coordinates: ("
         << toString( testMap->FocalPlaneX() ) << ", "
         << toString( testMap->FocalPlaneY() ) << ")" << endl;
    cout << "  Undistorted coordinates: ("
         << toString( testMap->UndistortedFocalPlaneX() ) << ", "
         << toString( testMap->UndistortedFocalPlaneY() ) << ")" << endl;
    cout << endl;

    cout << "Set undistorted coordinates" << endl;
    ux = 0.0;
    uy = 1.0;
    testMap->SetUndistortedFocalPlane(ux, uy);
    cout << "  Distorted coordinates: ("
         << toString( testMap->FocalPlaneX() ) << ", "
         << toString( testMap->FocalPlaneY() ) << ")" << endl;
    cout << "  Undistorted coordinates: ("
         << toString( testMap->UndistortedFocalPlaneX() ) << ", "
         << toString( testMap->UndistortedFocalPlaneY() ) << ")" << endl;
    cout << endl;

    cout << "Set undistorted coordinates" << endl;
    ux = 1.0;
    uy = 0.0;
    testMap->SetUndistortedFocalPlane(ux, uy);
    cout << "  Distorted coordinates: ("
         << toString( testMap->FocalPlaneX() ) << ", "
         << toString( testMap->FocalPlaneY() ) << ")" << endl;
    cout << "  Undistorted coordinates: ("
         << toString( testMap->UndistortedFocalPlaneX() ) << ", "
         << toString( testMap->UndistortedFocalPlaneY() ) << ")" << endl;
    cout << endl;
  }
  catch (IException &e) {
    e.print();
  }

  cout << "Unit Test for RosettaOsirisCamera..." << endl;
  try {
    // These should be lat/lon at center of image. To obtain these numbers for a new cube/camera,
    // set both the known lat and known lon to zero and copy the unit test output "Latitude off by: "
    // and "Longitude off by: " values directly into these variables.
    double knownLat = 66.7031631205829569;
    double knownLon = 95.7688045622462880;

    Cube c(testNACFile.toStdString(), "r");
    RosettaOsirisCamera *cam = (RosettaOsirisCamera *) CameraFactory::Create(c);
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

    // Test four pixels to make sure the conversions are right

    // The asteroid doesn't fill the full image, and the kernels are imperfect
    // so, test the corners of the asteroid
    cout << "For upper left corner of asteroid ..." << endl;
    TestLineSamp(cam, 400.0, 1300.0);

    cout << "For upper right corner of asteriod ..." << endl;
    TestLineSamp(cam, 1575.0, 1190.0);

    cout << "For lower left corner of asteriod..." << endl;
    TestLineSamp(cam, 540.0, 1600.0);

    cout << "For lower right corner of asteroid..." << endl;
    TestLineSamp(cam, 1600.0, 1560.0);

    double samp = 1024.0;
    double line = 1024.0;
    cout << "For center pixel position ..." << endl;

    if (!cam->SetImage(samp, line)) {
      cout << "ERROR" << endl;
      return 0;
    }

    if (abs(cam->UniversalLatitude() - knownLat) < 6E-14) {
      cout << "Latitude OK" << endl;
    }
    else {
      cout << setprecision(16) << "Latitude off by: " << cam->UniversalLatitude() - knownLat << endl;
    }

    if (abs(cam->UniversalLongitude() - knownLon) < 6E-14) {
      cout << "Longitude OK" << endl;
    }
    else {
      cout << setprecision(16) << "Longitude off by: " << cam->UniversalLongitude() - knownLon << endl;
    }

    cout << endl << "Test WAC with subwindowing..." << endl << endl;

    Cube wacCube(testWACFile.toStdString(), "r");
    RosettaOsirisCamera *wideAngleCam = (RosettaOsirisCamera *) CameraFactory::Create(wacCube);

    cout << "For upper left corner of asteroid ..." << endl;
    TestLineSamp(wideAngleCam, 215.0, 230.0);

    cout << "For upper right corner of asteriod ..." << endl;
    TestLineSamp(wideAngleCam, 304.0, 235.0);

    cout << "For lower left corner of asteriod..." << endl;
    TestLineSamp(wideAngleCam, 230.0, 303.0);

    cout << "For lower right corner of asteroid..." << endl;
    TestLineSamp(wideAngleCam, 299.0, 312.0);
  }
  catch (IException &e) {
    e.print();
  }
}

void TestLineSamp(Camera *cam, double samp, double line) {
  bool success = cam->SetImage(samp, line);

  if (success) {
    success = cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude());
  }

  if (success) {
    double deltaSamp = samp - cam->Sample();
    double deltaLine = line - cam->Line();
    if (fabs(deltaSamp) < 0.001) deltaSamp = 0.0;
    if (fabs(deltaLine) < 0.001) deltaLine = 0.0;
    cout << "DeltaSample = " << deltaSamp << endl;
    cout << "DeltaLine = " << deltaLine << endl << endl;
  }
  else {
    cout << "DeltaSample = ERROR" << endl;
    cout << "DeltaLine = ERROR" << endl << endl;
  }
}
