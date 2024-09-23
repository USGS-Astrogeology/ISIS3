/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <iomanip>
#include "IException.h"
#include "SimpleCylindrical.h"
#include "ProjectionFactory.h"
#include "Preference.h"
#include "TProjection.h"

using namespace Isis;
using namespace std;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << "UNIT TEST FOR SimpleCylindrical" << endl << endl;

  Pvl lab;
  lab.addGroup(PvlGroup("Mapping"));
  PvlGroup &mapGrp = lab.findGroup("Mapping");
  mapGrp += PvlKeyword("EquatorialRadius", Isis::toString(1.0));
  mapGrp += PvlKeyword("PolarRadius", Isis::toString(1.0));
  mapGrp += PvlKeyword("LatitudeType", "Planetocentric");
  mapGrp += PvlKeyword("LongitudeDirection", "PositiveEast");
  mapGrp += PvlKeyword("LongitudeDomain", Isis::toString(180));
  mapGrp += PvlKeyword("MinimumLatitude", Isis::toString(-90.0));
  mapGrp += PvlKeyword("MaximumLatitude", Isis::toString(90.0));
  mapGrp += PvlKeyword("MinimumLongitude", Isis::toString(-180.0));
  mapGrp += PvlKeyword("MaximumLongitude", Isis::toString(180.0));
  mapGrp += PvlKeyword("ProjectionName", "SimpleCylindrical");

  cout << "Test missing center longitude keyword ..." << endl;
  try {
    SimpleCylindrical p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  try {
    mapGrp += PvlKeyword("CenterLongitude", Isis::toString(-90.0));
    TProjection *p = (TProjection *) ProjectionFactory::Create(lab);

    cout << "Test SetGround method ... " << endl;
    cout << std::setprecision(16);
    cout << "Setting ground to (-50,-75)" << endl;
    p->SetGround(-50.0, -75.0);
    cout << "Latitude:               " << p->Latitude() << endl;
    cout << "Longitude:              " << p->Longitude() << endl;
    cout << "XCoord:                 " << p->XCoord() << endl;
    cout << "YCoord:                 " << p->YCoord() << endl;
    cout << endl;


    cout << "Test SetCoordinate method ... " << endl;
    cout << "Setting coordinate to (0.2617993877991494,-0.8726646259971648)" << endl;
    p->SetCoordinate(0.2617993877991494, -0.8726646259971648);
    cout << "Latitude:               " << p->Latitude() << endl;
    cout << "Longitude:              " << p->Longitude() << endl;
    cout << "XCoord:                 " << p->XCoord() << endl;
    cout << "YCoord:                 " << p->YCoord() << endl;
    cout << endl;

    cout << "Test XYRange method ... " << endl;
    double minX, maxX, minY, maxY;
    p->XYRange(minX, maxX, minY, maxY);
    cout << "Minimum X:  " << minX << endl;
    cout << "Maximum X:  " << maxX << endl;
    cout << "Minimum Y:  " << minY << endl;
    cout << "Maximum Y:  " << maxY << endl;
    cout << endl;

    Projection *s = p;
    cout << "Test Name and comparision method ... " << endl;
    cout << "Name:       " << s->Name().toStdString() << endl;
    cout << "operator==  " << (*s == *s) << endl;
    cout << endl;

    cout << "Testing default option ... " << endl;
    mapGrp.deleteKeyword("CenterLongitude");
    SimpleCylindrical p2(lab, true);
    cout << lab << endl;
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



