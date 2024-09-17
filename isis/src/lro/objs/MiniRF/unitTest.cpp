/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

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

void TestLineSamp(Isis::Camera *cam, double samp, double line);

int main(void) {
  Isis::Preference::Preferences(true);

  cout << "Unit Test for MiniRFCamera..." << endl;
  try {
    // These should be lat/lon at center of image. To obtain these numbers for a new cube/camera,
    // set both the known lat and known lon to zero and copy the unit test output "Latitude off by: "
    // and "Longitude off by: " values directly into these variables.
    double knownLat = 85.5973879396895398;
    double knownLon = 264.7361454607174664;

    Cube c("$ISISTESTDATA/isis/src/chandrayaan1/unitTestData/FSR_CDR_LV1_01801_0R.cub", "r");
    Camera *cam = Isis::CameraFactory::Create(c);
    cout << "FileName: " << FileName(c.fileName().toStdString()).name() << endl;
    cout << "CK Frame: " << cam->instrumentRotation()->Frame() << endl << endl;
    cout.setf(std::ios::fixed);
    cout << setprecision(9);

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

    if( !cam->SetImage(samp, line) ) {
      std::cout << "ERROR" << std::endl;
      return 0;
    }

    if(abs(cam->UniversalLatitude() - knownLat) < 1E-10) {
      cout << "Latitude OK" << endl;
    }
    else {
      cout << setprecision(16) << "Latitude off by: " << cam->UniversalLatitude() - knownLat << endl;
    }

    if(abs(cam->UniversalLongitude() - knownLon) < 2E-10) {
      cout << "Longitude OK" << endl;
    }
    else {
      cout << setprecision(16) << "Longitude off by: " << cam->UniversalLongitude() - knownLon << endl;
    }

    cout << endl << "RightAscension = " << cam->RightAscension() << endl;
    cout << "Declination = " << cam->Declination() << endl;

    Cube c2("$ISISTESTDATA/isis/src/lro/unitTestData/LSZ_04970_1CD_XKU_71S272_V1.reduced.cub", "r");
    Camera *cam2 = CameraFactory::Create(c2);

    // Test name methods
    cout << endl << endl << "Testing name methods ..." << endl;
    cout << "Spacecraft Name Long: " << cam->spacecraftNameLong().toStdString() << endl;
    cout << "Spacecraft Name Short: " << cam->spacecraftNameShort().toStdString() << endl;
    cout << "Instrument Name Long: " << cam->instrumentNameLong().toStdString() << endl;
    cout << "Instrument Name Short: " << cam->instrumentNameShort().toStdString() << endl << endl;

    cout << "Spacecraft Name Long: " << cam2->spacecraftNameLong().toStdString() << endl;
    cout << "Spacecraft Name Short: " << cam2->spacecraftNameShort().toStdString() << endl;
    cout << "Instrument Name Long: " << cam2->instrumentNameLong().toStdString() << endl;
    cout << "Instrument Name Short: " << cam2->instrumentNameShort().toStdString() << endl << endl;

    // Test kernel ID messages
    cout << endl << "Kernel ID error messages: " << endl;
    try{
      cam->CkFrameId();
    }
    catch(IException &e){
       e.print();
    }
    try{
      cam->CkReferenceId();
    }
    catch (IException &e){
      e.print();
    }
    try{
      cam->SpkTargetId();
    }
    catch (IException &e){
      e.print();
    }
    try{
      cam->SpkReferenceId();
    }
    catch (IException &e){
       e.print();
    }

   // Test a Level-2 image
    cout << endl << "Testing a Level-2 cube: " << endl << endl;

    Cube c3("$ISISTESTDATA/isis/src/lro/unitTestData/LSB_00291_1CD_XIU_89S206_V1_c2m.cub", "r");

    Camera *cam3 = CameraFactory::Create(c3);

    // Just test the center pixel to make sure the Camera still works on Level-2
    // images

    cout << "For a central pixel position ..." << endl;
    samp = 2014;
    line = 1026;

    if ( !cam3->SetImage(samp, line) ) {
      cout << "ERROR" << endl;
      return 0;
    }
    else {
      cout << "SetImage succeeded." << endl;
    }
  }
  catch(Isis::IException &e) {
    e.print();
  }
}

void TestLineSamp(Isis::Camera *cam, double samp, double line) {
  bool success = cam->SetImage(samp, line);

  if(success) {
    success = cam->SetUniversalGround( cam->UniversalLatitude(), cam->UniversalLongitude() );
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
