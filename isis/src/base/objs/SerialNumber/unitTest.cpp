/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include "FileName.h"
#include "Pvl.h"
#include "SerialNumber.h"
#include "ObservationNumber.h"
#include "Cube.h"
#include "Preference.h"

using namespace Isis;
using namespace std;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  PvlGroup g("Instrument");
  g += PvlKeyword("SpacecraftName", "MySpacecraft");
  g += PvlKeyword("InstrumentId", "MyInstrumentId");
  g += PvlKeyword("SpacecraftClockCount", "987654321");

  PvlObject o("IsisCube");
  o.addGroup(g);

  Pvl p;
  p.addObject(o);

  cout << SerialNumber::Compose(p) << endl;

  p.findGroup("Instrument", Pvl::Traverse).deleteKeyword("InstrumentId");
  cout << SerialNumber::Compose(p) << endl;

  Cube cube;
  cube.open("$ISISTESTDATA/isis/src/base/unitTestData/isisTruth.cub");
  cout << SerialNumber::Compose(cube, true) << endl;

  FileName file("$ISISTESTDATA/isis/src/lo/unitTestData/3133_h1.cub");
  Pvl p1(file.expanded());

  cout << SerialNumber::Compose(p1) << endl;

  cout << endl << "Testing ObservationKeys" << endl;

  cout << ObservationNumber::Compose(p1) << endl;
  return (0);
}
