/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iomanip>

#include "Camera.h"
#include "CameraStatistics.h"
#include "Cube.h"
#include "IException.h"
#include "Pvl.h"
#include "Preference.h"
#include "Statistics.h"

using namespace Isis;
using namespace std;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  try {
    cout << "UnitTest for Camera Statistics" << endl;
    Cube cube;
    cube.open("$ISISTESTDATA/isis/src/clementine/unitTestData/lna1391h.cub");
    Camera *cam = cube.camera();
    cout << setprecision(6);

    CameraStatistics camStats(cam, 1, 1);
    Pvl statsPvl = camStats.toPvl();
    cout << endl;

    for (int i = 0; i < statsPvl.groups(); i++) {
      PvlGroup &group = statsPvl.group(i);
      cout << group.name() << ":" << endl;

      for (int j = 0; j < group.keywords(); j++) {
        PvlKeyword &keyword = group[j];
        cout << "  " << keyword.name() << " = " << std::stod(keyword[0]) << endl;
      }

      cout << endl;
    }
  }
  catch(IException &e) {
    e.print();
  }
}
