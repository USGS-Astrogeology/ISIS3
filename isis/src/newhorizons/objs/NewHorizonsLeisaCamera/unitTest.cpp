/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iomanip>
#include <iostream>

#include "Camera.h"
#include "CameraFactory.h"
#include "NewHorizonsLeisaCamera.h"
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

  cout << "Unit Test for NewHorizonsLeisaCamera..." << endl;
  try {
    // These should be lat/lon at center of image. To obtain these numbers for a new cube/camera,
    // set both the known lat and known lon to zero and copy the unit test output "Latitude off by: "
    // and "Longitude off by: " values directly into these variables. -- in progress
    double knownLat = 12.5782232447537528;
    double knownLon = 23.5337593470257218;

    Cube c("$ISISTESTDATA/isis/src/newhorizons/unitTestData/lsb_0034933739_0x53c_sci_1.cub", "r");
    NewHorizonsLeisaCamera *cam = (NewHorizonsLeisaCamera *) CameraFactory::Create(c);
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

    double uLLine = 194.0;
    double uRLine = 60.0;
    double lLLine = 800.0;
    double lRLine = 900.0;
    for (int i = 0; i < 256; i++) {
      // Test four corners to make sure the conversions are right. The test image doesn't have target
      // data in the corners, so values were specifically choosen.
      cout << "Test forward and backward line/samp to lat/lon delta for Band #" << i+1 << endl;
      cout << "  For upper left corner (1.0, " << uLLine + i << ") ..." << endl;
      cam->SetBand(i+1);
      TestLineSamp(cam, 1.0, uLLine + i);

      cout << "  For upper right corner (256.0, " << uRLine + i << ") ..." << endl;
      TestLineSamp(cam, 256.0, uRLine + i);

      cout << "  For lower left corner (1.0, " << lLLine + i << ") ..." << endl;
      TestLineSamp(cam, 1.0, lLLine + i);

      cout << "  For lower right corner (256.0, " << lRLine + i << ") ..." << endl;
      TestLineSamp(cam, 256.0, lRLine + i);
    }

    double samp = 256.0/2.0;
    double line = 677;
    cam->SetBand(1);
    cout << "For center pixel position ..." << endl;

    if (!cam->SetImage(samp, line)) {
      cout << "ERROR call SetImage "<< samp << " " << line << endl;
    }

    if (abs(cam->UniversalLatitude() - knownLat) < 6E-14) {
      cout << "Latitude OK" << endl;
    }
    else {
      cout << setprecision(16) << "Latitude off by: " << cam->UniversalLatitude() - knownLat << endl;
    }

    if (abs(cam->UniversalLongitude() - knownLon) < 6E-14) {
      cout << "Longitude OK" << endl << endl;
    }
    else {
      cout << setprecision(16) << "Longitude off by: " <<
              cam->UniversalLongitude() - knownLon << endl << endl;
    }

    // Test the band dependent flag getter
    cout << "The bands of this camera have different gemometry for each band = " <<
            !cam->IsBandIndependent() << endl << endl;


    // Test trying to set an illeagal band number
    try {
      cam->SetBand(257);
    }
    catch (IException &e) {
      e.print();
      cout << endl << endl;
    }
  }
  catch (IException &e) {
    e.print();
  }
}


void TestLineSamp(Camera *cam, double samp, double line) {

  if (cam->SetImage(samp, line)) {
    if (cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude())) {
      double deltaSamp = samp - cam->Sample();
      double deltaLine = line - cam->Line();
      if (fabs(deltaSamp) < 0.001) deltaSamp = 0.0;
      if (fabs(deltaLine) < 0.001) deltaLine = 0.0;
      cout << "  DeltaSample = " << deltaSamp << endl;
      cout << "  DeltaLine = " << deltaLine << endl << endl;
    }
    else {
      cout << "  DeltaSample = ERROR" << endl;
      cout << "  DeltaLine = ERROR" << endl << endl;
    }
  }
  else {
    cout << "  Error in SetImage (" << samp << ", " << line << ")" << endl << endl;
  }
}
