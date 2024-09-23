/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "CameraPointInfo.h"
#include "Preference.h"
#include "PvlGroup.h"

#include <cmath>
#include <iostream>
#include <iomanip>

using namespace Isis;
using namespace std;

void PrintResults(PvlGroup &grp);

int main() {
  Preference::Preferences(true);

  // The class being tested
  CameraPointInfo cpi;

  // It is necessary to delete the FileName keyword for the test to pass
  // this is because the directory it is run from may change
  // under normal usage FileName is always included

  cpi.SetCube("$ISISTESTDATA/isis/src/base/unitTestData/CameraPointInfo/unitTest1.cub");
  PvlGroup *grp = cpi.SetImage(1, 1);
  PrintResults(*grp);

  cpi.SetCube("$ISISTESTDATA/isis/src/base/unitTestData/CameraPointInfo/unitTest1.cub");
  PvlGroup *too = cpi.SetGround(-84.5, 15.0);
  PrintResults(*too);

  // We have ownership, so, delete
  delete grp;
  grp = NULL;
  delete too;
  too = NULL;
  return 0;
}

void LowerPrecision(PvlKeyword &keyword) {
  if (keyword.name() != "LookDirectionCamera") {
    double value = IString::ToDouble(keyword[0]);
    value = round(value * 1000) / 1000.0;
    keyword[0] = toString(value);
  }
  else {
    for (int i = 0; i < 3; i++) {
      double value = IString::ToDouble(keyword[i]);
      value = round(value * 10000000000) / 10000000000.0;
      keyword[i] = toString(value);
    }
  }
}


void PrintResults(PvlGroup &grp) {
  grp.deleteKeyword("FileName");

  LowerPrecision(grp["NorthAzimuth"]);
  LowerPrecision(grp["SpacecraftAzimuth"]);
  LowerPrecision(grp["SubSolarAzimuth"]);
  LowerPrecision(grp["SubSolarGroundAzimuth"]);
  LowerPrecision(grp["SubSpacecraftGroundAzimuth"]);
  LowerPrecision(grp["OffNadirAngle"]);
  LowerPrecision(grp["Emission"]);
  LowerPrecision(grp["LookDirectionCamera"]);

  cout << grp << endl;
}
