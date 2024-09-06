/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include "FileName.h"
#include "Pvl.h"
#include "ObservationNumber.h"
#include "Cube.h"
#include "Preference.h"

using namespace Isis;
using namespace std;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  FileName file("$ISISTESTDATA/isis/src/lo/unitTestData/3133_h1.cub");
  Pvl p1(file.expanded());

  cout << ObservationNumber::Compose(p1) << endl;

  return (0);
}
