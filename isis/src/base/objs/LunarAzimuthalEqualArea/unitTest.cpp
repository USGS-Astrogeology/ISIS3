/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iostream>
#include <iomanip>
#include <cmath>

#include "LunarAzimuthalEqualArea.h"
#include "IException.h"
#include "IString.h"
#include "ProjectionFactory.h"
#include "Preference.h"
#include "TProjection.h"

using namespace Isis;
using namespace std;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << "UNIT TEST FOR LunarAzimuthalEqualArea\n\n";

  Pvl lab;
  lab.addGroup(PvlGroup("Mapping"));
  PvlGroup &mapGroup = lab.findGroup("Mapping");
  mapGroup += PvlKeyword("EquatorialRadius", std::to_string(6378206.4));
  mapGroup += PvlKeyword("PolarRadius", std::to_string(6378206.4));
  mapGroup += PvlKeyword("LatitudeType", "Planetographic");
  mapGroup += PvlKeyword("LongitudeDirection", "PositiveEast");
  mapGroup += PvlKeyword("LongitudeDomain", std::to_string(180));
  mapGroup += PvlKeyword("MinimumLatitude", std::to_string(-30.0));
  mapGroup += PvlKeyword("MaximumLatitude", std::to_string(-15.0));
  mapGroup += PvlKeyword("MinimumLongitude", std::to_string(-30.0));
  mapGroup += PvlKeyword("MaximumLongitude", std::to_string(-15.0));
  mapGroup += PvlKeyword("MaximumLibration", std::to_string(1));
  mapGroup += PvlKeyword("ProjectionName",
                               "LunarAzimuthalEqualArea");

  try {
    TProjection *p = (TProjection *) ProjectionFactory::Create(lab);

    cout << "\n******* Test SetGround and SetCoordinate methods ********"
         << "\n\nfor all lat and lon in [-90..90] step 45"
         << "\n  first call SetGround"
         << "\n  then call SetCoordinate"
         << "\n  trigger fail if SetCoordinate doesn't"
         << "\n  return the original coordinate"
         << "\n  note that at the poles the lon is not relevant!\n\n";

    cout << std::setprecision(11);
    for(int i = -90; i <= 90; i += 45) {
      for(int j = -90; j <= 90; j += 45) {
        cout << " original Lat: " << setw(4) << i << "   original Lon: "
             << setw(4) << j << "\n"
             << "testing SetCoordinate and SetGround";

        p->SetGround(i * 1.0, j * 1.0);
        p->SetCoordinate(p->XCoord(), p->YCoord());

        if((fabs(fabs(p->Latitude()) - fabs(i)) > 0.00001) ||
            (fabs(i) != 90 &&  fabs(fabs(p->Longitude()) - fabs(j)) > 0.00001))
          cout << " ......................... [ FAILED ]\n\n";
        else
          cout << " ......................... [   OK   ]\n\n";
      }
    }
    cout << "\nTesting for SetGround and SetCoordiante done!"
         << "\nNow test the Name method, the == operator and finally"
         << "\nprint the output of the XYRange and Mapping methods...\n\n\n";

    cout << "Name method returns \"LunarAzimuthalEqualArea\"................ ";
    if(p->Name() == "LunarAzimuthalEqualArea")
      cout << "[   OK   ]\n";
    else
      cout << "[ FAILED ]\n";

    Projection *s = p;
    cout << "\nProjection * s = p; // p is pointer to this projection"
         << "\n(*s == *p) returns ";
    if(*s == *p)
      cout << "true! ..................................... [   OK   ]\n";
    else
      cout << "false! .................................... [ FAILED ]\n";
    mapGroup["PolarRadius"].setValue(std::to_string(42));
    Pvl tmp1;
    TProjection *p2 = (TProjection *) ProjectionFactory::Create(lab);
    tmp1.addGroup(p2->Mapping());
    s = p2;
    cout << "created a second Projection reference p2"
         << "\nusing the same pvl as p except for that PolarRadius = 42"
         << "\nPvl for p2...\n" << tmp1
         << "\ns = &p2"
         << "\n(*s == p) returns ";
    if(*s == *p)
      cout << "true! ..................................... [ FAILED ]\n";
    else
      cout << "false! .................................... [   OK   ]\n";

    double minX, maxX, minY, maxY;
    p->XYRange(minX, maxX, minY, maxY);
    cout << "\nXYRange..."
         << "\n  Minimum X:  " << minX
         << "\n  Maximum X:  " << maxX
         << "\n  Minimum Y:  " << minY
         << "\n  Maximum Y:  " << maxY << "\n";

    Pvl tmp2;
    tmp2.addGroup(p->Mapping());

  }
  catch(IException &e) {
    e.print();
  }
}

