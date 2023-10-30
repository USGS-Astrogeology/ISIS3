/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
// $Id: unitTest.cpp,v 1.6 2009/01/22 22:43:29 kbecker Exp $
#include <iomanip>
#include <iostream>

#include "Camera.h"
#include "CameraFactory.h"
#include "FileName.h"
#include "IException.h"
#include "iTime.h"
#include "IssWACamera.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlGroup.h"

using namespace std;
using namespace Isis;

void TestLineSamp(Camera *cam, double samp, double line);

int main(void) {
  Preference::Preferences(true);

  cout << "Unit Test for IssWACamera..." << endl;
  /*
   * Sample/Line TestLineSamp points changed for the IssWACamera
   */
  try {

    Cube c("$ISISTESTDATA/isis/src/cassini/unitTestData/W1525116136_1.cub", "r");
    IssWACamera *cam = (IssWACamera *) CameraFactory::Create(c);
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
    cout << "\nFor upper left corner ..." << endl;
    TestLineSamp(cam, 0.5, 0.5);

    cout << "\nFor upper right corner ..." << endl;
    TestLineSamp(cam, cam->Samples() + 0.5, 0.5);

    cout << "\nFor lower left corner ..." << endl;
    TestLineSamp(cam, 0.5, cam->Lines() + 0.5);

    cout << "\nFor lower right corner ..." << endl;
    TestLineSamp(cam, cam->Samples() + 0.5, cam->Lines() + 0.5);

    double samp = cam->Samples() / 2.0 + 0.5;
    double line = cam->Lines() / 2.0 + 0.5;
    cout << "\nFor center pixel position ..." << endl;
    TestLineSamp(cam, samp, line);

    // Test name methods
    cout << endl << endl << "Testing name methods ..." << endl;
    cout << "Spacecraft Name Long: " << cam->spacecraftNameLong() << endl;
    cout << "Spacecraft Name Short: " << cam->spacecraftNameShort() << endl;
    cout << "Instrument Name Long: " << cam->instrumentNameLong() << endl;
    cout << "Instrument Name Short: " << cam->instrumentNameShort() << endl << endl;
  }
  catch(IException &e) {
    e.print();
  }
}

void TestLineSamp(Camera *cam, double samp, double line) {
  cout << "Line, Sample: " << line << ", " << samp << endl;
  bool success = cam->SetImage(samp, line);

  if(success) {
    cout << "Lat, Long:    " << cam->UniversalLatitude() << ", "
         << cam->UniversalLongitude() << endl;
    double westlon = -cam->UniversalLongitude();
    while(westlon < 0.0) westlon += 360.0;
    cout << "WestLon:      " << westlon << endl;
  }
  else {
    cout << "Point not on planet!\n";
  }
}
