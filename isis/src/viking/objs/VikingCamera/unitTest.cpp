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
#include "VikingCamera.h"

using namespace std;
using namespace Isis;

void TestLineSamp(Camera *cam, double samp, double line);

int main(void) {
  Preference::Preferences(true);

  cout << "Unit Test for VikingCamera..." << endl;
  /* VIKING: The lon difference tolerance was increased for this
   * camera model test in order for it to pass on Gala.
   */
  try {
    // These should be lat/lon at center of image. To obtain these numbers for a new cube/camera,
    // set both the known lat and known lon to zero and copy the unit test output "Latitude off by: "
    // and "Longitude off by: " values directly into these variables.
    // double knownLat = -24.27445959155795;
    // double knownLon = 180.6234165504677;
    // new lat/lon values used due to change in LeastSquares class (this changes ReseauDistortionMap
    double knownLat[1] = { -24.2744713106319 };
    double knownLon[1] = { 180.6234120834806 };
    QList<QString> files;
    files.append("$ISISTESTDATA/isis/src/viking/unitTestData/f348b26.cub"); // Viking2 VISB

    for (int i = 0; i < files.size(); i++) {
      Cube c(files[i], "r");
      VikingCamera *cam = (VikingCamera *) CameraFactory::Create(c);
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

      // changed tolerance to allow hiclops to pass
      if(abs(cam->UniversalLatitude() - knownLat[i]) < 1.18E-05) {
        cout << "Latitude OK" << endl;
      }
      else {
        cout << setprecision(16) << "Latitude off by: " << cam->UniversalLatitude() - knownLat[i] << endl;
      }

      // changed tolerance to allow hiclops to pass
      if(abs(cam->UniversalLongitude() - knownLon[i]) < 4.47E-6) {
        cout << "Longitude OK" << endl;
      }
      else {
        cout << setprecision(16) << "Longitude off by: " << cam->UniversalLongitude() - knownLon[i] << endl;
      }
      cout << endl << "--------------------------------------------" << endl;
    }

    // Test the name methods
    cout << endl << "Testing name methods:" << endl << endl;
    files.append("$ISISTESTDATA/isis/src/viking/unitTestData/f006a03.cropped.cub"); // Viking1 VISA
    files.append("$ISISTESTDATA/isis/src/viking/unitTestData/f387a06.cropped.cub"); // Viking1 VISB
    files.append("$ISISTESTDATA/isis/src/viking/unitTestData/f004b33.cropped.cub"); // Viking2 VISA
    for (int i = 0; i < files.size(); i++) {
      Cube c(files[i], "r");
      VikingCamera *cam = (VikingCamera *) CameraFactory::Create(c);
      cout << "Spacecraft Name Long: " << cam->spacecraftNameLong() << endl;
      cout << "Spacecraft Name Short: " << cam->spacecraftNameShort() << endl;
      cout << "Instrument Name Long: " << cam->instrumentNameLong() << endl;
      cout << "Instrument Name Short: " << cam->instrumentNameShort() << endl << endl;
    }

    // Test exception: camera is not a supported Kaguya camera
    cout << endl << "Testing exceptions:" << endl << endl;
    Cube test("$ISISTESTDATA/isis/src/hayabusa/unitTestData/st_2530292409_v.cub", "r");
    VikingCamera vCam(test);
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
    if(fabs(deltaSamp) < 0.001) deltaSamp = 0;
    if(fabs(deltaLine) < 0.001) deltaLine = 0;
    cout << "DeltaSample = " << deltaSamp << endl;
    cout << "DeltaLine = " << deltaLine << endl << endl;
  }
  else {
    cout << "DeltaSample = ERROR" << endl;
    cout << "DeltaLine = ERROR" << endl << endl;
  }
}
