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

void TestLineSamp(Camera *cam, double samp, double line);

int main(void) {
  Preference::Preferences(true);

  cout << "Unit Test for IdealCamera..." << endl;
  /*
   *  IdealCamera unit test tests two images instead of the typical one. Increased tolerances
   *  for line/samp->lat/lon->line/samp
   */
  try {
    // These should be lat/lon at center of image. To obtain these numbers for a new cube/camera,
    // set both the known lat and known lon to zero and copy the unit test output "Latitude off by: "
    // and "Longitude off by: " values directly into these variables.
    double knownLat[2] = {22.79610724064016, -82.44364258830991};
    double knownLon[2] = {225.0055144115362, 75.87598490596933};
    char files[2][1024] = { "$ISISTESTDATA/isis/src/base/unitTestData/ab102401_ideal.cub", "$ISISTESTDATA/isis/src/base/unitTestData/f319b18_ideal.cub" };
    Camera *cam;

    for(unsigned int i = 0; i < sizeof(knownLat) / sizeof(double); i++) {
      Cube cube(files[i], "r");
      cam = CameraFactory::Create(cube);
      cout << "FileName: " << FileName(files[i]).name() << endl;
      cout << "CK Frame: " << cam->instrumentRotation()->Frame() << endl << endl;

      // Test name methods
      cout << "Spacecraft Name Long: " << cam->spacecraftNameLong().toStdString() << endl;
      cout << "Spacecraft Name Short: " << cam->spacecraftNameShort().toStdString() << endl;
      cout << "Instrument Name Long: " << cam->instrumentNameLong().toStdString() << endl;
      cout << "Instrument Name Short: " << cam->instrumentNameShort().toStdString() << endl << endl;

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

      if(!cam->SetImage(samp, line)) {
        cout << "ERROR" << endl;
        return 0;
      }

      if(abs(cam->UniversalLatitude() - knownLat[i]) < 1E-10) {
        cout << "Latitude OK" << endl;
      }
      else {
        cout << setprecision(16) << "Latitude off by: " << cam->UniversalLatitude() - knownLat[i] << endl;
      }

      if(abs(cam->UniversalLongitude() - knownLon[i]) < 1E-10) {
        cout << "Longitude OK" << endl;
      }
      else {
        cout << setprecision(16) << "Longitude off by: " << cam->UniversalLongitude() - knownLon[i] << endl;
      }
      cout << endl << "--------------------------------------------" << endl;
    }

    // Test kernel ID messages
    cout << "Kernel ID error messages: " << endl;
    try{ cam->CkFrameId(); }
    catch (IException &e){ e.print(); }
    try{ cam->CkReferenceId(); }
    catch (IException &e){ e.print(); }
    try{ cam->SpkTargetId(); }
    catch (IException &e){ e.print(); }
    try{ cam->SpkReferenceId(); }
    catch (IException &e){ e.print(); }
    delete cam;
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
    if(fabs(deltaSamp) < 0.013) deltaSamp = 0;
    if(fabs(deltaLine) < 0.0016) deltaLine = 0;
    cout << "DeltaSample = " << deltaSamp << endl;
    cout << "DeltaLine = " << deltaLine << endl << endl;
  }
  else {
    cout << "DeltaSample = ERROR" << endl;
    cout << "DeltaLine = ERROR" << endl << endl;
  }
}
