/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iomanip>
#include <iostream>

#include <QList>
#include <QString>

#include "Camera.h"
#include "CameraFactory.h"
#include "FileName.h"
#include "IException.h"
#include "iTime.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "VoyagerCamera.h"

using namespace std;
using namespace Isis;

void TestLineSamp(Camera *cam, double samp, double line);

int main(void) {
  Preference::Preferences(true);

  cout << "Unit Test for Voyager..." << endl;
  /*
   * Sample/Line TestLineSamp points were changed for this test.
   * The tolerance for Sample/Line to Lat/Lon and back was increased
   * for this test.
   */
  try {
    // These should be lat/lon at center of image. To obtain these numbers for a new cube/camera,
    // set both the known lat and known lon to zero and copy the unit test output "Latitude off by: "
    // and "Longitude off by: " values directly into these variables.
    double knownLat = -1.03098148697020941;
    double knownLon = 82.0423364316989279;

    Cube c("$ISISTESTDATA/isis/src/voyager/unitTestData/c1639118.imq.cub", "r");
    VoyagerCamera *cam = (VoyagerCamera *) CameraFactory::Create(c);
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

    // Test Shutter Open/Close
    const PvlGroup &inst = c.label()->findGroup("Instrument", Pvl::Traverse);
    double exposureDuration = ((double) inst["ExposureDuration"])/1000;
    QString stime = QString::fromStdString(inst["StartTime"]);
    double et; // StartTime keyword is the center exposure time
    str2et_c(stime.toLatin1().data(), &et);
    pair <iTime, iTime> shuttertimes = cam->ShutterOpenCloseTimes(et, exposureDuration);
    cout << "Shutter open = " << shuttertimes.first.Et() << endl;
    cout << "Shutter close = " << shuttertimes.second.Et() << endl << endl;

    // Test all four corners to make sure the conversions are right
    cout << "For upper left corner ..." << endl;
    TestLineSamp(cam, 1.0, 1.0);

    cout << "For upper right corner ..." << endl;
    TestLineSamp(cam, cam->Samples() - 5.0, 16.0);

    cout << "For lower left corner ..." << endl;
    TestLineSamp(cam, 12.0, cam->Lines() - 12.0);

    cout << "For lower right corner ..." << endl;
    TestLineSamp(cam, cam->Samples() - 4.0, cam->Lines());

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
      cout << setprecision(16) << "Latitude off by: " << cam->UniversalLatitude() - knownLat
           << endl;
    }

    if(abs(cam->UniversalLongitude() - knownLon) < 1E-10) {
      cout << "Longitude OK" << endl;
    }
    else {
      cout << setprecision(16) << "Longitude off by: " << cam->UniversalLongitude() - knownLon
           << endl;
    }

    // Test name methods
    cout << endl << "Testing name methods" << endl << endl;
    QList<QString> files;
    files.append("$ISISTESTDATA/isis/src/voyager/unitTestData/c1639118.imq.cub"); // Voyager1 NAC
    files.append("$ISISTESTDATA/isis/src/voyager/unitTestData/c1639241.cropped.cub"); // Voyager1 WAC
    files.append("$ISISTESTDATA/isis/src/voyager/unitTestData/c2065022.cropped.cub"); // Voyager2 NAC
    files.append("$ISISTESTDATA/isis/src/voyager/unitTestData/c4397840.cropped.cub"); // Voyager2 WAC

    for (int i = 0; i < files.size(); i++) {
      Cube cu(files[i].toStdString(), "r");
      VoyagerCamera *vCam = (VoyagerCamera *) CameraFactory::Create(cu);
      cout << "Spacecraft Name Long: " << vCam->spacecraftNameLong().toStdString() << endl;
      cout << "Spacecraft Name Short: " << vCam->spacecraftNameShort().toStdString() << endl;
      cout << "Instrument Name Long: " << vCam->instrumentNameLong().toStdString() << endl;
      cout << "Instrument Name Short: " << vCam->instrumentNameShort().toStdString() << endl << endl;
    }

    // Test exception: camera is not a supported Kaguya camera
    cout << endl << "Testing exceptions:" << endl << endl;
    Cube test("$ISISTESTDATA/isis/src/hayabusa/unitTestData/st_2530292409_v.cub", "r");
    VoyagerCamera testCam(test);
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
    if(fabs(deltaSamp) < 0.05) deltaSamp = 0;
    if(fabs(deltaLine) < 0.04) deltaLine = 0;
    cout << "DeltaSample = " << deltaSamp << endl;
    cout << "DeltaLine = " << deltaLine << endl << endl;
  }
  else {
    cout << "DeltaSample = ERROR" << endl;
    cout << "DeltaLine = ERROR" << endl << endl;
  }
}
