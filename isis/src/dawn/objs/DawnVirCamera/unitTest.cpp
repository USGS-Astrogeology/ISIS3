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
#include "DawnVirCamera.h"
#include "IException.h"
#include "iTime.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlGroup.h"

using namespace std;
using namespace Isis;

void TestLineSamp(Camera *cam, double samp, double line);

/**
 *
 * Unit test for DawnVirCamera.
 *
 * @author ????-??-?? Unknown
 *
 * @internal
 *   @history 2016-10-28 Kristin Berry - Updated known latitude, longitude, image start time,
 *                           and image stop time. References #4476.
 */
int main(void) {
  Preference::Preferences(true);

  cout << "Unit Test for DawnVirCamera..." << endl;
  try {
    // These should be lat/lon at center of image. To obtain these numbers for a new cube/camera,
    // set both the known lat and known lon to zero and copy the unit test output "Latitude off by: "
    // and "Longitude off by: " values directly into these variables.
    double knownLat = 10.0318264323263371;
    double knownLon = 272.0561372647773055;

    Cube c("$ISISTESTDATA/isis/src/dawn/objs/DawnVirCamera/VIR_VIS_1B_1_362681635_1.cub", "r");
    DawnVirCamera *cam = (DawnVirCamera *) CameraFactory::Create(c);
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
    //const PvlGroup &inst = p.findGroup("Instrument", Pvl::Traverse);
    std::pair< double, double > imgTimes = cam->StartEndEphemerisTimes();
    cout << "Start Time: " << imgTimes.first << "\n";
    cout << "End Time:   " << imgTimes.second << "\n";
    // Test all four corners to make sure the conversions are right
    cout << "For upper left corner ..." << endl;
    TestLineSamp(cam, 130.0, 26.0);

    cout << "For upper right corner ..." << endl;
    TestLineSamp(cam, 134.0, 26.0);

    cout << "For lower left corner ..." << endl;
    TestLineSamp(cam, 130.0, 30.0);

    cout << "For lower right corner ..." << endl;
    TestLineSamp(cam, 134.0, 30.0);

    double samp = cam->Samples() / 2;
    double line = cam->Lines() / 2;
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
