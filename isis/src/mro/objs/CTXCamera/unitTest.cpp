/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iomanip>

#include "Camera.h"
#include "CameraFactory.h"
#include "FileName.h"
#include "IException.h"
#include "Preference.h"
#include "Pvl.h"

using namespace std;
using namespace Isis;

/**
 * @brief unitTest for MRO CTX Camera Model
 *
 * This is the unitTest for the Mars Reconnaissance Orbiter Context Camera (CTX) camera model.
 *
 * @author ????-??-?? Unknown
 *
 * @internal
 *   @history 2012-12-27  Tracie Sucharski, Add image with SpatialSumming=2.
*/

void TestLineSamp(Camera *cam, double samp, double line);

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << "Unit Test for CTXCamera..." << endl;

  /** CTX: The line,samp to lat,lon to line,samp tolerance was increased for this
   *  camera model test.
   */
  try {
    // These should be lat/lon at center of image. To obtain these numbers for a new cube/camera,
    // set both the known lat and known lon to zero and copy the unit test output "Latitude off by: "
    // and "Longitude off by: " values directly into these variables.
    double knownLat[3] = { -22.00026732942671, -15.08829558278137, -40.6918887505667115 };
    double knownLon[3] = { 307.9160921848336, 309.8677351454377, 102.4181750964207964 };
    char files[3][1024] = { "$ISISTESTDATA/isis/src/mro/unitTestData/ctx_pmoi_i_00003.bottom.cub",
                            "$ISISTESTDATA/isis/src/mro/unitTestData/ctx_pmoi_i_00003.top.cub",
                            "$ISISTESTDATA/isis/src/mro/unitTestData/G02_019106_1390_XN_41S257W.cub" };

    for(unsigned int i = 0; i < sizeof(knownLat) / sizeof(double); i++) {
      Cube c(files[i], "r");
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

      // Test name methods
      cout << "Spacecraft Name Long: " << cam->spacecraftNameLong().toStdString() << endl;
      cout << "Spacecraft Name Short: " << cam->spacecraftNameShort().toStdString() << endl;
      cout << "Instrument Name Long: " << cam->instrumentNameLong().toStdString() << endl;
      cout << "Instrument Name Short: " << cam->instrumentNameShort().toStdString() << endl << endl;

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
