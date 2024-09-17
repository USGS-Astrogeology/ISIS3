/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>
#include <iomanip>
#include <iostream>

#include "Camera.h"
#include "CameraFactory.h"
#include "FileName.h"
#include "IException.h"
#include "Preference.h"
#include "Pvl.h"

using namespace std;
using namespace Isis;

void TestLineSamp(Camera *cam, double samp, double line);
/**
 * @internal
 *   @history 2009-08-03 Jeannie Walldren - Changed known lat
 *                           and lon.
 *   @history 2009-08-06 Jeannie Walldren - Added cmath include
 *                           and changed calls to abs() to fabs() since the
 *                           abs() function takes integer values while the
 *                           fabs() takes floats. Changed Center Lat tolerance
 *                           from 1E-10 to 2E-10 since the difference on Darwin powerpc
 *                           was 1.93E-10.  Changed Center Lon tolerance from
 *                           1E-10 to 1.1E-10 since the difference on Darwin powerpc
 *                           was 1.03E-10.
 *   @history 2010-02-24 Christopher Austin - Changed the knowLat/Lon
 *                           in accordence with the system changes
 *   @history 2011-05-03 Jeannie Walldren - Updated unitTest to test
 *                           for new methods. Added Isis Disclaimer to file.
 *   @history 2015-08-12 Ian Humphrey and Makayla Shepherd - Added tests for spacecraft and
 *                           instrument name methods.
 */

int main(void) {
  Preference::Preferences(true);

  cout << "Unit Test for MocNarrowAngleCamera..." << endl;
  try {
    // These should be lat/lon at center of image. To obtain these numbers for a new cube/camera,
    // set both the known lat and known lon to zero and copy the unit test output "Latitude off by: "
    // and "Longitude off by: " values directly into these variables.
    //    double knownLat = -9.931519304695483;
    //    double knownLon = 286.6184572896669;
    //    double knownLat = -9.931519304671058;
    //    double knownLon = 286.6184572896659;
    //  blackflag
    //    double knownLat = -9.931519304119526;
    //    double knownLon = 286.6184572896647;
    //  deet (current)
    //    double knownLat = -9.931519304735847;
    //    double knownLon = 286.6184572896974;
    double knownLat = -9.931519304735847;
    double knownLon = 286.6184572896974;

    Cube c("$ISISTESTDATA/isis/src/mgs/unitTestData/fha00491.lev1.cub", "r");
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

    if(fabs(cam->UniversalLatitude() - knownLat) < 2E-10) {
      cout << "Latitude OK" << endl;
    }
    else {
      cout << setprecision(16) << "Latitude off by: " << cam->UniversalLatitude() - knownLat << endl;
    }

    if(fabs(cam->UniversalLongitude() - knownLon) < 1.1E-10) {
      cout << "Longitude OK" << endl;
    }
    else {
      cout << setprecision(16) << "Longitude off by: " << cam->UniversalLongitude() - knownLon << endl;
    }

    // Test name methods
    cout << endl << endl << "Testing name methods ..." << endl;
    cout << "Spacecraft Name Long: " << cam->spacecraftNameLong().toStdString() << endl;
    cout << "Spacecraft Name Short: " << cam->spacecraftNameShort().toStdString() << endl;
    cout << "Instrument Name Long: " << cam->instrumentNameLong().toStdString() << endl;
    cout << "Instrument Name Short: " << cam->instrumentNameShort().toStdString() << endl;
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
