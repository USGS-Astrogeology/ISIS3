/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iomanip>
#include <iostream>

#include "Camera.h"
#include "CameraFactory.h"
#include "Distance.h"
#include "NewHorizonsLorriCamera.h"
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
 * Unit test for New Horizons MVIC TDI Camera Model
 *
 * @author ????-??-?? Unknown
 *
 * @internal
 *   @history 2016-10-27 Kristin Berry -
 */
int main(void) {
  Preference::Preferences(true);

  cout << "Unit Test for NewHorizonsLorriCamera..." << endl;
  try {
    // These should be lat/lon at center of image. To obtain these numbers for a new cube/camera,
    // set both the known lat and known lon to zero and copy the unit test output "Latitude off by: "
    // and "Longitude off by: " values directly into these variables.
    double knownLat = -8.2967175607848702;
    double knownLon = 210.1990629768775705;
    // double knownLat = 0.0;
    // double knownLon = 0.0;

    Cube c("$ISISTESTDATA/isis/src/newhorizons/unitTestData/lor_0034821014_0x630_sci_1_v2.cub", "r");
    NewHorizonsLorriCamera *cam = (NewHorizonsLorriCamera *) CameraFactory::Create(c);
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
    double exposureDuration = ((double) inst["ExposureDuration"])/1000;
    QString stime = QString::fromStdString(inst["StartTime"]);
    double et; // StartTime keyword is the center exposure time
    str2et_c(stime.toLatin1().data(), &et);
    pair <iTime, iTime> shuttertimes = cam->ShutterOpenCloseTimes(et, exposureDuration);
    cout << "Shutter open = " << shuttertimes.first.Et() << endl;
    cout << "Shutter close = " << shuttertimes.second.Et() << endl << endl;

    // Test all four corners to make sure the conversions are right
    cout << "For upper left corner ..." << endl;
    TestLineSamp(cam, 255.0, 490.0);

    cout << "For upper right corner ..." << endl;
    TestLineSamp(cam, 426.0, 473.0);

    cout << "For lower left corner ..." << endl;
    TestLineSamp(cam, 258.0, 578.0);

    cout << "For lower right corner ..." << endl;
    TestLineSamp(cam, 412.0, 594.0);

    double samp = 358.0;
    double line = 534.0;
    double deltaT = 5000.0;
    cout << "For center pixel position ..." << endl;

    cout << "Testing SetImage without deltaT..." << endl;
    if (!cam->SetImage(samp, line)) {
      cout << "ERROR" << endl;
      return 0;
    }
    else {
      cout << "Sample: " << cam->Sample() << endl;
      cout << "Line: " << cam->Line() << endl;
      cout << "RightAscension: " << cam->RightAscension() << endl;
      cout << "Declination: " << cam->Declination() << endl;
      cout << "PlanetocentricLatitude: " << cam->UniversalLatitude() << endl;
      cout << "PositiveEast360Longitude: " << cam->UniversalLongitude() << endl;
      cout << "EphemerisTime: " << cam->time().Et() << endl;
      cout << "NorthAzimuth: " << cam->NorthAzimuth() << endl;
      cout << "SunAzimuth: " << cam->SunAzimuth() << endl;
      cout << "SpacecraftAzimuth: " << cam->SpacecraftAzimuth() << endl;
      cout << "OffNadirAngle: " << cam->OffNadirAngle() << endl;
      cout << "CelestialNorthClockAngle: " << cam->CelestialNorthClockAngle() << endl;
      cout << "RaDecResolution: " << cam->RaDecResolution()  << endl;

      double pB[3];
      cam->Coordinate(pB);
      cout << "BodyFixedCoordinate: " << pB[0] << endl;
      cout << "BodyFixedCoordinate: " << pB[1] << endl;
      cout << "BodyFixedCoordinate: " << pB[2] << endl;
      cout << "LocalRadius: " << cam->LocalRadius().meters() << endl;
      cout << "SampleResolution: " << cam->SampleResolution() << endl;
      cout << "LineResolution: " << cam->LineResolution() << endl;
      cout << "ObliqueDetectorResolution: " << cam->ObliqueDetectorResolution() << endl;
      cout << "ObliqueLineResolution: " << cam->ObliqueLineResolution() << endl;
      cout << "ObliqueSampleResolution: " << cam->ObliqueSampleResolution() << endl;
      cout << "ObliquePixelResolution: " << cam->ObliquePixelResolution() << endl;
    }

    cout << "Testing SetImage with deltaT..." << endl;
    if(!cam->SetImage(samp, line, deltaT)) {
      cout << "ERROR" << endl;
      return 0;
    }
    else {
      cout << "Sample: " << cam->Sample() << endl;
      cout << "Line: " << cam->Line() << endl;
      cout << "RightAscension: " << cam->RightAscension() << endl;
      cout << "Declination: " << cam->Declination() << endl;
      cout << "PlanetocentricLatitude: " << cam->UniversalLatitude() << endl;
      cout << "PositiveEast360Longitude: " << cam->UniversalLongitude() << endl;
      cout << "EphemerisTime: " << cam->time().Et() << endl;
      cout << "NorthAzimuth: " << cam->NorthAzimuth() << endl;
      cout << "SunAzimuth: " << cam->SunAzimuth() << endl;
      cout << "SpacecraftAzimuth: " << cam->SpacecraftAzimuth() << endl;
      cout << "OffNadirAngle: " << cam->OffNadirAngle() << endl;
      cout << "CelestialNorthClockAngle: " << cam->CelestialNorthClockAngle() << endl;
      cout << "RaDecResolution: " << cam->RaDecResolution()  << endl;

      double pB[3];
      cam->Coordinate(pB);
      cout << "BodyFixedCoordinate: " << pB[0] << endl;
      cout << "BodyFixedCoordinate: " << pB[1] << endl;
      cout << "BodyFixedCoordinate: " << pB[2] << endl;
      cout << "LocalRadius: " << cam->LocalRadius().meters() << endl;
      cout << "SampleResolution: " << cam->SampleResolution() << endl;
      cout << "LineResolution: " << cam->LineResolution() << endl;
      cout << "ObliqueDetectorResolution: " << cam->ObliqueDetectorResolution() << endl;
      cout << "ObliqueLineResolution: " << cam->ObliqueLineResolution() << endl;
      cout << "ObliqueSampleResolution: " << cam->ObliqueSampleResolution() << endl;
      cout << "ObliquePixelResolution: " << cam->ObliquePixelResolution() << endl;
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
