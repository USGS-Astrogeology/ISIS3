/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iomanip>
#include <iostream>
#include <vector>
#include <utility>

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
 *   @history 2009-08-19 Tracie Sucharski, Added all new tests, including
 *                         ir and vis, normal and hires modes with offsets.
 *   @history 2012-12-03 Tracie Sucharski, Added new test image to test validity even if none of
 *                         pixels' centers intersect the ground.  Error checking was added to
 *                         SetGround which returns false if any of the min/max lat/lons are invalid.
 *   @history 2013-10-23 Tracie Sucharski, Change known values because of changes to the Vims
 *                         Camera model.  The adjustment to the Ir times is once again being
 *                         calculated in code-the label keywords for Ir exposure and interline
 *                         delay is incorrect.  The camera model is also now doing calculations
 *                         in x, y, z instead of lat, lon.
 *   @history 2014-06-23 Ian Humphrey - Modified hard coded /usgs/cpkgs/ paths to
 *                           relative pathnames. Fixes #2054.
 *   @history 2015-08-12 Ian Humphrey and Makayla Shepherd - Added tests for spacecraft and
 *                           instrument name methods.
 */
int main(void) {
  Preference::Preferences(true);

  cout << "Unit Test for VimsCamera..." << endl;
  /*
   * Sample/Line TestLineSamp points changed for the VimsCamera,
   *   tolerance increased. This unitTest has been modified enough that you
   *   should not use this as a template when making a new camera unit test.
   */
  try {
    // These should be lat/lon at center of image. To obtain these numbers for a new cube/camera,
    // set both the known lat and known lon to zero and copy the unit test output "Latitude off by: "
    // and "Longitude off by: " values directly into these variables.

    //  For vims, testing ir and vis, hires and normal with offsets and swath
    //   sizes that are not 1,1,64,64 (full field of view)
    //  Ir Normal full field of view:                   CM_1515951157_1.ir.cub
    //  Ir Normal partial field of view (17,17,48,48):  CM_1514390782_1.ir.cub
    //  Vis Normal partial field of view (17,17,48,48): CM_1514390782_1.vis.cub
    //  Ir Hires partial field of view (17,26,48,36):   CM_1515945709_1.ir.cub
    //  Vis Hires partial field of view (17,26,48,36):  CM_1515945709_1.vis.cub

    //  Tested separatedly:  Ir Normal partial field of view -none of the    C1465336166_1.ir.cub
    //      pixels' centers intersect with the ground.
    //      However, right around the bottom of pixel, samp:3 line:4 and top of samp:3 line:5 there
    //      are some intersections, which means we can get lat/lon, but we cannot back project.
    char files[5][1024] = { "$ISISTESTDATA/isis/src/cassini/unitTestData/CM_1515951157_1.ir.cub",
                            "$ISISTESTDATA/isis/src/cassini/unitTestData/CM_1514390782_1.ir.cub",
                            "$ISISTESTDATA/isis/src/cassini/unitTestData/CM_1514390782_1.vis.cub",
                            "$ISISTESTDATA/isis/src/cassini/unitTestData/CM_1515945709_1.ir.cub",
                            "$ISISTESTDATA/isis/src/cassini/unitTestData/CM_1515945709_1.vis.cub"
                          };

    double knownLat[5] = { -0.4635396765968510,
                           -34.8446732028169848,
                           -41.4357957282659370,
                           -42.7683454790732966,
                           -37.4139298028795153
                         };
    double knownLon[5] = { 198.1302329741679102,
                           123.5608203785339327,
                           131.7215892768255969,
                           202.6731689530125493,
                           213.5977879626166782
                         };

    vector< pair <int, int > > corners;
    //  CM_1515951157_1.ir.cub
    corners.push_back(std::make_pair(25, 30));
    corners.push_back(std::make_pair(40, 30));
    corners.push_back(std::make_pair(25, 45));
    corners.push_back(std::make_pair(40, 45));
    //  CM_1514390782_1.ir.cub
    corners.push_back(std::make_pair(22, 20));
    corners.push_back(std::make_pair(40, 20));
    corners.push_back(std::make_pair(22, 33));
    corners.push_back(std::make_pair(40, 33));

    //  CM_1514390782_1.vis.cub
    corners.push_back(std::make_pair(23, 20));
    corners.push_back(std::make_pair(40, 20));
    corners.push_back(std::make_pair(23, 33));
    corners.push_back(std::make_pair(40, 33));

    //  CM_1515945709_1.ir.cub
    corners.push_back(std::make_pair(21, 14));
    corners.push_back(std::make_pair(48, 14));
    corners.push_back(std::make_pair(23, 24));
    corners.push_back(std::make_pair(48, 26));

    //  CM_1515945709_1.vis.cub
    corners.push_back(std::make_pair(26, 8));
    corners.push_back(std::make_pair(36, 8));
    corners.push_back(std::make_pair(26, 29));
    corners.push_back(std::make_pair(36, 29));

    //  C1465336166_1.ir.cub
    corners.push_back(std::make_pair(26, 8));
    corners.push_back(std::make_pair(36, 8));
    corners.push_back(std::make_pair(26, 29));
    corners.push_back(std::make_pair(36, 29));

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

      // Test all four corners to make sure the conversions are right
      cout << "For upper left corner ..." << endl;
      TestLineSamp(cam, corners[i*4].first, corners[i*4].second);

      cout << "For upper right corner ..." << endl;
      TestLineSamp(cam, corners[i*4+1].first, corners[i*4+1].second);

      cout << "For lower left corner ..." << endl;
      TestLineSamp(cam, corners[i*4+2].first, corners[i*4+2].second);

      cout << "For lower right corner ..." << endl;
      TestLineSamp(cam, corners[i*4+3].first, corners[i*4+3].second);

      double samp = cam->Samples() / 2;
      double line = cam->Lines() / 2;
      cout << "For center pixel position ..." << endl;

      if(!cam->SetImage(samp, line)) {
        cout << "ERROR" << endl;
        return 0;
      }

      if(abs(cam->UniversalLatitude() - knownLat[i]) < 1E-8) {
        cout << "Latitude OK" << endl;
      }
      else {
        cout << setprecision(16) << "Latitude off by: " << cam->UniversalLatitude() - knownLat[i] << endl;
      }

      if(abs(cam->UniversalLongitude() - knownLon[i]) < 1E-8) {
        cout << "Longitude OK" << endl;
      }
      else {
        cout << setprecision(16) << "Longitude off by: " << cam->UniversalLongitude() - knownLon[i] << endl;
      }
      cout << endl;
    }

    //  Test C1465336166_1.ir.cub
    //string file = "/usgs/cpkgs/isis3/data/cassini/testData/C1465336166_1.ir.cub";
    Cube c("$ISISTESTDATA/isis/src/cassini/unitTestData/C1465336166_1.ir.cub", "r");
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

    //  Test a non-intersecting pixel
    double samp = 3.;
    double line = 4.;
    if (!cam->SetImage(samp, line)) {
      cout << "Sample:3  Line:4   No Intersection" << endl;
    }
    //  Test intersecting pixel and back project
    samp = 3.0121;
    line = 4.39113;
    cout << "Sample:3.0121    Line:4.39113" << endl;
    double expectedLat = -19.3962073091522598;
    double expectedLon = 45.5092093638429773;
    if (!cam->SetImage(samp, line)) {
      cout << "ERROR" << endl;
    }

    if (abs(cam->UniversalLatitude() - expectedLat) < 1E-8) {
      cout << "Latitude OK" << endl;
    }
    else {
      cout << setprecision(16) << "Latitude off by: " << cam->UniversalLatitude() - expectedLat << endl;
    }

    if(abs(cam->UniversalLongitude() - expectedLon) < 1E-8) {
      cout << "Longitude OK" << endl;
    }
    else {
      cout << setprecision(16) << "Longitude off by: " << cam->UniversalLongitude() - expectedLon << endl;
    }

    cout << endl;

    // Test name methods
    cout << endl << "Testing name methods ..." << endl;
    cout << "Spacecraft Name Long: " << cam->spacecraftNameLong().toStdString() << endl;
    cout << "Spacecraft Name Short: " << cam->spacecraftNameShort().toStdString() << endl;
    cout << "Instrument Name Long: " << cam->instrumentNameLong().toStdString() << endl;
    cout << "Instrument Name Short: " << cam->instrumentNameShort().toStdString() << endl << endl;
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
    if(fabs(deltaSamp) < 0.196) deltaSamp = 0;
    if(fabs(deltaLine) < 0.196) deltaLine = 0;
    cout << "DeltaSample = " << deltaSamp << endl;
    cout << "DeltaLine = " << deltaLine << endl << endl;
  }
  else {
    cout << "DeltaSample = ERROR" << endl;
    cout << "DeltaLine = ERROR" << endl << endl;
  }
}
