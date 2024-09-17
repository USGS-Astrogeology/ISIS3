/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iomanip>
#include <iostream>

#include <QStringList>

#include "Camera.h"
#include "CameraFactory.h"
#include "FileName.h"
#include "IException.h"
#include "LroNarrowAngleCamera.h"
#include "Preference.h"
#include "Pvl.h"
#include "iTime.h"

using namespace std;
using namespace Isis;

void TestLineSamp(Camera *cam, double samp, double line);

int main(void) {
  Preference::Preferences(true);

  /**
   *  LRO NAC: The line,samp to lat,lon to line,samp tolerance was increased for
   *    this camera model test. This test was re-written and should NOT be used
   *    as a template for other camera model unit tests.
   */
  cout << "Unit Test for LroNarrowAngleCamera..." << endl;
  try {
    double knownLat = -83.2598150072595899;
    double knownLon = 353.9497987082821737;
    Cube c("$ISISTESTDATA/isis/src/lro/unitTestData/M111607830RE_crop.cub", "r");
    Camera *cam = CameraFactory::Create(c);

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

    // Test all four corners to make sure the conversions are right
    cout << "For upper left corner ..." << endl;
    TestLineSamp(cam, 1.0, 1.0);

    cout << "For upper right corner ..." << endl;
    TestLineSamp(cam, cam->Samples(), 1.0);

    cout << "For lower left corner ..." << endl;
    TestLineSamp(cam, 1.0, cam->Lines());

    cout << "For lower right corner ..." << endl;
    TestLineSamp(cam, cam->Samples(), cam->Lines());

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
    // Test name methods
    cout << endl << endl << "Testing name methods ..." << endl;
    QStringList files;
    files.append("$ISISTESTDATA/isis/src/lro/unitTestData/M111607830RE_crop.cub");
    files.append("$ISISTESTDATA/isis/src/lro/unitTestData/M1153718003LE.reduced.cub");

    for (int i = 0; i < files.size(); i++) {
      Cube n(files[i].toStdString(), "r");
      Camera *nCam = CameraFactory::Create(n);
      cout << "Spacecraft Name Long: " << nCam->spacecraftNameLong().toStdString() << endl;
      cout << "Spacecraft Name Short: " << nCam->spacecraftNameShort().toStdString() << endl;
      cout << "Instrument Name Long: " << nCam->instrumentNameLong().toStdString() << endl;
      cout << "Instrument Name Short: " << nCam->instrumentNameShort().toStdString() << endl << endl;
    }

    // Test exception: camera is not a supported Kaguya camera
    cout << endl << "Testing exceptions:" << endl << endl;
    Cube test("$ISISTESTDATA/isis/src/hayabusa/unitTestData/st_2530292409_v.cub", "r");
    LroNarrowAngleCamera testCam(test);
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
