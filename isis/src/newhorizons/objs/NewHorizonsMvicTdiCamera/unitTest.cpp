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
#include "NewHorizonsMvicTdiCamera.h"
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
 *   @history 2016-10-27 Kristin Berry - Added testing for new SetImage with time offset.
 *                           References #4476.
 */
int main(void) {
  Preference::Preferences(true);

  cout << "Unit Test for New HorizonsMvicTdiFrameCamera..." << endl;
  try {
    // These should be lat/lon at center of image. To obtain these numbers for a new cube/camera,
    // set both the known lat and known lon to zero and copy the unit test output "Latitude off by: "
    // and "Longitude off by: " values directly into these variables.
    double knownLat = -10.1772088449130020;
    double knownLon = 339.7338889883354796;

    Cube c("$ISISTESTDATA/isis/src/newhorizons/unitTestData/mc0_0034942918_0x536_sci_1.cub", "r");
    NewHorizonsMvicTdiCamera *cam = (NewHorizonsMvicTdiCamera *) CameraFactory::Create(c);
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
    cout << "For upper left  ..." << endl;
    TestLineSamp(cam, 2484.0, 310.0);

    cout << "For upper right corner ..." << endl;
    TestLineSamp(cam, 2528.0, 310.0);

    cout << "For lower left corner ..." << endl;
    TestLineSamp(cam, 2484.0, 350.0);

    cout << "For lower right corner ..." << endl;
    TestLineSamp(cam, 2528.0, 350.0);

    double samp = 2503.0;
    double line = 330.0;
    cout << "For center pixel position ..." << endl;

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
    }

    //testing SetImage with deltaT
    double deltaT = 0.5;
    if (!cam->SetImage(samp, line, deltaT)) {
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
    }

    if (abs(cam->UniversalLatitude() - knownLat) < 7E-12) {
      cout << "Latitude OK" << endl;
    }
    else {
      cout << setprecision(16) << "Latitude off by: " << cam->UniversalLatitude() - knownLat << endl;
    }

    if (abs(cam->UniversalLongitude() - knownLon) < 7E-12) {
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
