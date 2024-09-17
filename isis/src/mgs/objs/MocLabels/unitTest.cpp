/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Cube.h"
#include "IException.h"
#include "Pvl.h"
#include "MocLabels.h"
#include "Preference.h"

using namespace std;
using namespace Isis;

int main() {
  Preference::Preferences(true);

  try {
    cout << "Unit test for MocLabels" << endl;
    cout << "MocWideAngleCamera cub test..." << endl;
    Cube c1("$ISISTESTDATA/isis/src/mgs/unitTestData/ab102401.cub", "r");
    MocLabels lab1(c1);

    cout << "NarrowAngle?  " << lab1.NarrowAngle() << endl;
    cout << "WideAngle?  " << lab1.WideAngle() << endl;
    cout << "WideAngleRed?  " << lab1.WideAngleRed() << endl;
    cout << "WideAngleBlue?  " << lab1.WideAngleBlue() << endl;
    cout << "Crosstrack Summing = " << lab1.CrosstrackSumming() << endl;
    cout << "Downtrack Summing = " << lab1.DowntrackSumming() << endl;
    cout << "First Line Sample = " << lab1.FirstLineSample() << endl;
    cout << "Focal Plane Temperature = " << lab1.FocalPlaneTemperature() << endl;
    cout << "Line Rate = " << lab1.LineRate() << endl;
    cout << "Exposure Duration = " << lab1.ExposureDuration() << endl;
    cout << "Start Time = " << lab1.StartTime().toStdString() << endl;
    cout << "Detectors = " << lab1.Detectors() << endl;
    cout << "StartDetector(1) = " << lab1.StartDetector(1) << endl;
    cout << "EndDetector(1) = " << lab1.EndDetector(1) << endl;
    cout << "Sample = " << lab1.Sample(lab1.StartDetector(1)) << endl;
    cout << "EphemerisTime = " << lab1.EphemerisTime(1) << endl;
    cout << "Gain = " << lab1.Gain() << endl;
    cout << "Offset = " << lab1.Offset() << endl << endl;

    cout << "MocNarrowAngleCamera cub test..." << endl;
    Cube c2("$ISISTESTDATA/isis/src/mgs/unitTestData/fha00491.lev1.cub", "r");
    MocLabels lab2(c2);

    cout << "NarrowAngle?  " << lab2.NarrowAngle() << endl;
    cout << "WideAngle?  " << lab2.WideAngle() << endl;
    cout << "WideAngleRed?  " << lab2.WideAngleRed() << endl;
    cout << "WideAngleBlue?  " << lab2.WideAngleBlue() << endl;
    cout << "Crosstrack Summing = " << lab2.CrosstrackSumming() << endl;
    cout << "Downtrack Summing = " << lab2.DowntrackSumming() << endl;
    cout << "First Line Sample = " << lab2.FirstLineSample() << endl;
    cout << "Focal Plane Temperature = " << lab2.FocalPlaneTemperature() << endl;
    cout << "Line Rate = " << lab2.LineRate() << endl;
    cout << "Exposure Duration = " << lab2.ExposureDuration() << endl;
    cout << "Start Time = " << lab2.StartTime().toStdString() << endl;
    cout << "Detectors = " << lab2.Detectors() << endl;
    cout << "StartDetector(1) = " << lab2.StartDetector(1) << endl;
    cout << "EndDetector(1) = " << lab2.EndDetector(1) << endl;
    cout << "Sample = " << lab2.Sample(lab2.StartDetector(1)) << endl;
    cout << "EphemerisTime = " << lab2.EphemerisTime(1) << endl;
    cout << "Gain = " << lab2.Gain() << endl;
    cout << "Offset = " << lab2.Offset() << endl << endl;

  }
  catch(IException &e) {
    e.print();
  }
}
