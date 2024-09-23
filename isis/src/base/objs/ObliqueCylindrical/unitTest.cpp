/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <iomanip>
#include <cmath>
#include "IException.h"
#include "ObliqueCylindrical.h"
#include "ProjectionFactory.h"
#include "Preference.h"
#include "TProjection.h"

using namespace Isis;
using namespace std;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << "UNIT TEST FOR ObliqueCylindrical" << endl << endl;

  Pvl lab;
  lab.addGroup(PvlGroup("Mapping"));
  PvlGroup &mapGrp = lab.findGroup("Mapping");

  mapGrp += PvlKeyword("EquatorialRadius", "2575000.0");
  mapGrp += PvlKeyword("PolarRadius", "2575000.0");

  mapGrp += PvlKeyword("PoleLatitude", "22.858149");
  mapGrp += PvlKeyword("PoleLongitude", "297.158602");

  mapGrp += PvlKeyword("LatitudeType", "Planetocentric");
  mapGrp += PvlKeyword("LongitudeDirection", "PositiveWest");
  mapGrp += PvlKeyword("LongitudeDomain", "360");
  mapGrp += PvlKeyword("ProjectionName", "ObliqueCylindrical");

  mapGrp += PvlKeyword("MinimumLatitude", "-90");
  mapGrp += PvlKeyword("MaximumLatitude", "0.92523");
  mapGrp += PvlKeyword("MinimumLongitude", "-0.8235");
  mapGrp += PvlKeyword("MaximumLongitude", "180.5");

  cout << "Test missing pole rotation keyword ..." << endl;
  try {
    ObliqueCylindrical p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  // Add the missing keyword "PoleRotation"
  mapGrp += PvlKeyword("PoleRotation", "45.7832");

  // testing operator ==
  cout << "Testing operator == ..." << endl;
  try {
    ObliqueCylindrical p1(lab);
    ObliqueCylindrical p2(lab);
    bool flag = (p1 == p2);
    if(flag) {
      cout << "(p1==p2) = True" << endl;
    }
    else {
      cout << "*** Error ****" << endl;
    }
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  try {

    TProjection *p = (TProjection *) ProjectionFactory::Create(lab);

    cout << "Test X,Y,Z Axis Vector Calculations ... " << endl;
    cout << "Map Group Data (X[0]):      " << IString::ToDouble(mapGrp["XAxisVector"][0]) << endl;
    cout << "Map Group Data (X[1]):      " << IString::ToDouble(mapGrp["XAxisVector"][1]) << endl;
    cout << "Map Group Data (X[2]):      " << IString::ToDouble(mapGrp["XAxisVector"][2]) << endl;
    cout << "Map Group Data (Y[0]):      " << IString::ToDouble(mapGrp["YAxisVector"][0]) << endl;
    cout << "Map Group Data (Y[1]):      " << IString::ToDouble(mapGrp["YAxisVector"][1]) << endl;
    cout << "Map Group Data (Y[2]):      " << IString::ToDouble(mapGrp["YAxisVector"][2]) << endl;
    cout << "Map Group Data (Z[0]):      " << IString::ToDouble(mapGrp["ZAxisVector"][0]) << endl;
    cout << "Map Group Data (Z[1]):      " << IString::ToDouble(mapGrp["ZAxisVector"][1]) << endl;
    cout << "Map Group Data (Z[2]):      " << IString::ToDouble(mapGrp["ZAxisVector"][2]) << endl;
    cout << endl;

    const double X = -2646.237039, Y = -537.814519;
    cout << setprecision(13);
    cout << "Test SetCoordinate method ... " << endl;
    cout << "Setting coordinate to (" << X << "," << Y << ")" << endl;
    p->SetCoordinate(X, Y);
    cout << "Latitude:               " << p->Latitude() << endl;
    cout << "Longitude:              " << p->Longitude() << endl;
    cout << "XCoord:                 " << p->XCoord() << endl;
    cout << "YCoord:                 " << p->YCoord() << endl;
    cout << endl;

    cout << "Test SetGround method ... " << endl;
    cout << std::setprecision(10);
    cout << "Setting ground to (" << p->Latitude() << "," << p->Longitude() << ")" << endl;
    p->SetGround(p->Latitude(), p->Longitude());
    cout << "Latitude:               " << p->Latitude() << endl;
    cout << "Longitude:              " << p->Longitude() << endl;
    cout << "XCoord:                 " << p->XCoord() << endl;
    cout << "YCoord:                 " << p->YCoord() << endl;
    cout << endl;

    cout << "Test XYRange method ... " << endl;
    cout << std::setprecision(8);
    double minX = 0.0, maxX = 1.0, minY = 2.0, maxY = 3.0;
    p->XYRange(minX, maxX, minY, maxY);
    cout << "\n\nMinimum X:  " << minX << endl;
    cout << "Maximum X:  " << maxX << endl;
    cout << "Minimum Y:  " << minY << endl;
    cout << "Maximum Y:  " << maxY << endl;
    cout << endl;

    Projection *s = p;
    cout << "Test Name and comparision method ... " << endl;
    cout << "Name:       " << s->Name().toStdString() << endl;
    cout << "operator==  " << (*s == *s) << endl;
    cout << endl;

    mapGrp["PoleRotation"] = "43.8423";
    ObliqueCylindrical different(lab);
    cout << "Test Name and comparision method with differing data... " << endl;
    cout << "Name:       " << s->Name().toStdString() << endl;
    cout << "operator==  " << (different == *s) << endl;
    cout << endl;

    cout << "Testing Mapping() methods ... " << endl;

    Pvl tmp1;
    Pvl tmp2;
    Pvl tmp3;
    tmp1.addGroup(p->Mapping());
    tmp2.addGroup(p->MappingLatitudes());
    tmp3.addGroup(p->MappingLongitudes());

    cout << "Mapping() = " << endl;
    cout << tmp1 << endl;
    cout << "MappingLatitudes() = " << endl;
    cout << tmp2 << endl;
    cout << "MappingLongitudes() = " << endl;
    cout << tmp3 << endl;
    cout << endl;
  }
  catch(IException &e) {
    e.print();
  }
}



