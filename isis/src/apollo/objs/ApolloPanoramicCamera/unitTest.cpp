/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iomanip>
#include <iostream>

#include "ApolloPanoramicCamera.h"
#include "ApolloPanoramicDetectorMap.h"
#include "CameraDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "Camera.h"
#include "CameraFactory.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlGroup.h"

using namespace std;
using namespace Isis;

void TestLineSamp(Camera *cam, double samp, double line);

int main(void) {
  Preference::Preferences(true);
  cout << "Unit Test for ApolloCamera..." << endl;
  try {



    // These should be lat/lon at center of image. To obtain these numbers for a new cube/camera,
    // set both the known lat and known lon to zero and copy the unit test output "Latitude off by: "
    // and "Longitude off by: " values directly into these variables.
    double knownLat = 18.1052476112276679;
    double knownLon = 6.3330663560966673;

    Cube cubeTL(FileName("$ISISTESTDATA/isis/src/apollo/unitTestData/TL.cub").expanded(), "r");
    ApolloPanoramicCamera *camTL = (ApolloPanoramicCamera *) CameraFactory::Create(cubeTL);

    Cube cubeBL(FileName("$ISISTESTDATA/isis/src/apollo/unitTestData/BL.cub").expanded(), "r");
    ApolloPanoramicCamera *camBL = (ApolloPanoramicCamera *) CameraFactory::Create(cubeBL);

    Cube cubeM(FileName("$ISISTESTDATA/isis/src/apollo/unitTestData/M.cub").expanded(), "r");
    ApolloPanoramicCamera *camM = (ApolloPanoramicCamera *) CameraFactory::Create(cubeM);

    Cube cubeTR(FileName("$ISISTESTDATA/isis/src/apollo/unitTestData/TR.cub").expanded(), "r");
    ApolloPanoramicCamera *camTR = (ApolloPanoramicCamera *) CameraFactory::Create(cubeTR);

    Cube cubeBR(FileName("$ISISTESTDATA/isis/src/apollo/unitTestData/BR.cub").expanded(), "r");
    ApolloPanoramicCamera *camBR = (ApolloPanoramicCamera *) CameraFactory::Create(cubeBR);

    cout << "FileName: " << FileName(cubeTL.fileName().toStdString()).name() << endl;
    cout << "CK Frame: " << camTL->instrumentRotation()->Frame() << endl << endl;
    cout.setf(std::ios::fixed);
    cout << setprecision(9);

    // Test kernel IDs
    cout << "Kernel IDs: " << endl;
    cout << "CK Frame ID = " << camTL->CkFrameId() << endl;
    cout << "CK Reference ID = " << camTL->CkReferenceId() << endl;
    cout << "SPK Target ID = " << camTL->SpkTargetId() << endl;
    cout << "SPK Reference ID = " << camTL->SpkReferenceId() << endl << endl;


    // Test all four corners to make sure the conversions are right
    cout << "For upper left corner ..." << endl;
    TestLineSamp(camTL, 1.0, 1.0);

    cout << "For upper right corner ..." << endl;
    TestLineSamp(camTR, 1.0, 1.0);

    cout << "For lower left corner ..." << endl;
    TestLineSamp(camBL, 1.0, 1.0);

    cout << "For lower right corner ..." << endl;
    TestLineSamp(camBR, 1.0, 1.0);

     cout << "For center pixel position ..." << endl;

    if(!camM->SetImage(1.0, 1.0)) {
      cout << "ERROR" << endl;
      return 0;
    }

    if(abs(camM->UniversalLatitude() - knownLat) < 1E-10) {
      cout << "Latitude OK" << endl;
    }
    else {
      cout << setprecision(16) << "Latitude off by: " << camM->UniversalLatitude() - knownLat << endl;
    }

    if(abs(camM->UniversalLongitude() - knownLon) < 1E-10) {
      cout << "Longitude OK" << endl;
    }
    else {
      cout << setprecision(16) << "Longitude off by: " << camM->UniversalLongitude() - knownLon << endl;
    }

    //TODO - test name methods for Apollo16 and 17 Panoramic
    // Test name methods
    cout << endl << endl << "Testing name methods ..." << endl;
    cout << "Spacecraft Name Long: " << camM->spacecraftNameLong().toStdString() << endl;
    cout << "Spacecraft Name Short: " << camM->spacecraftNameShort().toStdString() << endl;
    cout << "Instrument Name Long: " << camM->instrumentNameLong().toStdString() << endl;
    cout << "Instrument Name Short: " << camM->instrumentNameShort().toStdString() << endl << endl;

    // Test exception: camera is not a supported Kaguya camera
    cout << endl << "Testing exceptions:" << endl << endl;
    Cube test("$ISISTESTDATA/isis/src/hayabusa/unitTestData/st_2530292409_v.cub", "r");
    ApolloPanoramicCamera pCam(test);

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
    cout << "DeltaLine = " << deltaLine << endl;
  }
  else {
    cout << "DeltaSample = ERROR ";
    cout << "DeltaLine = ERROR" << endl;
  }
}
