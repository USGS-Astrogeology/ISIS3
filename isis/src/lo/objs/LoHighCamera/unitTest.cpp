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
#include "LoHighCamera.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlGroup.h"

using namespace std;
using namespace Isis;

void TestLineSamp(Camera *cam, double samp, double line);

int main(void) {
  Preference::Preferences(true);

  cout << "Unit Test for LoHighCamera..." << endl;
  /**
   * LO: DeltaSample/Line tolerance increased for this mission.
   */
  try {
    // These should be lat/lon at center of image. To obtain these numbers for a new cube/camera,
    // set both the known lat and known lon to zero and copy the unit test output "Latitude off by: "
    // and "Longitude off by: " values directly into these variables.
    double knownLat[3] = { -3.8709488902507347, 69.16103791616183, 41.5550783983903855 };
    double knownLon[3] = { 342.3331666742689094, 317.6341072210002, 245.3158115000968849 };

    QList<QString> files;
    files.append("$ISISTESTDATA/isis/src/lo/unitTestData/3133_h1.cropped.cub"); // LO3 High
    files.append("$ISISTESTDATA/isis/src/lo/unitTestData/4164H_Full_mirror.cub"); // L04 High
    files.append("$ISISTESTDATA/isis/src/lo/unitTestData/5006_high_res_1.cropped.cub"); // L05 High

    for (int i = 0; i < files.size(); i++) {
      Cube c(files[i], "r");
      LoHighCamera *cam = (LoHighCamera *) CameraFactory::Create(c);
      cout << "FileName: " << FileName(c.fileName()).name() << endl;
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
      cout << "Spacecraft Name Long: " << cam->spacecraftNameLong() << endl;
      cout << "Spacecraft Name Short: " << cam->spacecraftNameShort() << endl;
      cout << "Instrument Name Long: " << cam->instrumentNameLong() << endl;
      cout << "Instrument Name Short: " << cam->instrumentNameShort() << endl << endl;

      // Test Shutter Open/Close

      const PvlGroup &inst = c.label()->findGroup("Instrument", Pvl::Traverse);
      // approximate 1 tenth of a second since Lunar Orbiter did not provide
      double exposureDuration = .1;
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

      if(abs(cam->UniversalLatitude() - knownLat[i]) < 1E-10) {
        cout << "Latitude OK" << endl;
      }
      else {
        cout << setprecision(16) << "Latitude off by: " << cam->UniversalLatitude() - knownLat[i]
        << endl;
      }

      if(abs(cam->UniversalLongitude() - knownLon[i]) < 1E-10) {
        cout << "Longitude OK" << endl;
      }
      else {
        cout << setprecision(16) << "Longitude off by: " << cam->UniversalLongitude() - knownLon[i]
        << endl;
      }
      cout << endl << "--------------------------------------------" << endl;
    }

     // Test exception: camera is not a supported Kaguya camera
    cout << endl << "Testing exceptions:" << endl << endl;
    Cube test("$ISISTESTDATA/isis/src/hayabusa/unitTestData/st_2530292409_v.cub", "r");
    LoHighCamera lhc(test);
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
    if(fabs(deltaSamp) < 0.1) deltaSamp = 0;
    if(fabs(deltaLine) < 0.1) deltaLine = 0;
    cout << "DeltaSample = " << deltaSamp << endl;
    cout << "DeltaLine = " << deltaLine << endl << endl;
  }
  else {
    cout << "DeltaSample = ERROR" << endl;
    cout << "DeltaLine = ERROR" << endl << endl;
  }
}
