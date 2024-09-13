/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <iomanip>
#include "IException.h"
#include "Robinson.h"
#include "ProjectionFactory.h"
#include "Pvl.h"
#include "PvlKeyword.h"
#include "Preference.h"

using namespace std;
using namespace Isis;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << "Unit Test For Robinson Projection" << endl << endl;

  Pvl lab;
  lab.addGroup(PvlGroup("Mapping"));
  PvlGroup &mapGroup = lab.findGroup("Mapping");
  mapGroup += PvlKeyword("EquatorialRadius", "1.0");
  mapGroup += PvlKeyword("PolarRadius", "1.0");
  mapGroup += PvlKeyword("LatitudeType", "Planetocentric");
  mapGroup += PvlKeyword("LongitudeDirection", "PositiveEast");
  mapGroup += PvlKeyword("LongitudeDomain", "180");
  mapGroup += PvlKeyword("MinimumLatitude", "-90.0");
  mapGroup += PvlKeyword("MaximumLatitude", "90.0");
  mapGroup += PvlKeyword("MinimumLongitude", "-180.0");
  mapGroup += PvlKeyword("MaximumLongitude", "180.0");
  mapGroup += PvlKeyword("CenterLongitude", "0.0");
  mapGroup += PvlKeyword("ProjectionName", "Robinson");

  cout << "Test missing center longitude keyword ..." << endl;
  try {
    Robinson p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  mapGroup += PvlKeyword("CenterLongitude", "0.0");

  try {
    TProjection *p = (TProjection *) ProjectionFactory::Create(lab);

    cout << "Test SetGround method ... " << endl;
    cout << std::setprecision(16);
    cout << "Setting ground to (30, 50)" << endl;
    p->SetGround(30.0, 50.0);
    cout << "Latitude:               " << p->Latitude() << endl;
    cout << "Longitude:              " << p->Longitude() << endl;
    cout << "XCoord:                 " << p->XCoord() << endl;
    cout << "YCoord:                 " << p->YCoord() << endl;
    cout << endl;


    cout << "Test SetCoordinate method ... " << endl;
    cout << "Setting coordinate to (0.7110052493604421, 0.5030556)" << endl;
    p->SetCoordinate(0.7110052493604421, 0.5030556);
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

    cout << "Test default computation ... " << endl;
    mapGroup.deleteKeyword("CenterLongitude");
    Robinson p2(lab, true);
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

    delete p;
  }
  catch(IException &e) {
    e.print();
  }

  mapGroup.addKeyword(PvlKeyword("EquatorialRadius", "6370997.0"), Pvl::Replace);
  try {
    TProjection *p = (TProjection *) ProjectionFactory::Create(lab);
    cout << endl;

    cout << "Test SetGround method ... " << endl;
    cout << std::setprecision(16);
    cout << "Setting ground to (-47, -102)" << endl;
    p->SetGround(-47, -102);
    cout << "Latitude:               " << p->Latitude() << endl;
    cout << "Longitude:              " << p->Longitude() << endl;
    cout << "XCoord:                 " << p->XCoord() << endl;
    cout << "YCoord:                 " << p->YCoord() << endl;
    cout << endl;

    cout << "Test SetCoordinate method ... " << endl;
    cout << "Setting coordinate to (-8521072.970798777, -5009016.797941369)" << endl;
    p->SetCoordinate(-8521076.0, -5009012.0);
    cout << "Latitude:               " << p->Latitude() << endl;
    cout << "Longitude:              " << p->Longitude() << endl;
    cout << "XCoord:                 " << p->XCoord() << endl;
    cout << "YCoord:                 " << p->YCoord() << endl;
    cout << endl;

    cout << "Test SetGround method ... " << endl;
    cout << std::setprecision(16);
    cout << "Setting ground to (0, 360)" << endl;
    p->SetGround(0, 360);
    cout << "Latitude:               " << p->Latitude() << endl;
    cout << "Longitude:              " << p->Longitude() << endl;
    cout << "XCoord:                 " << p->XCoord() << endl;
    cout << "YCoord:                 " << p->YCoord() << endl;
    cout << endl;

    cout << "Test SetCoordinate method ... " << endl;
    cout << "Setting coordinate to (33973592.32994721, 0)" << endl;
    p->SetCoordinate(33973592.32994721, 0);
    cout << "Latitude:               " << p->Latitude() << endl;
    cout << "Longitude:              " << p->Longitude() << endl;
    cout << "XCoord:                 " << p->XCoord() << endl;
    cout << "YCoord:                 " << p->YCoord() << endl;
    cout << endl;

    delete p;
  }
  catch(IException &e) {
    e.print();
  }


  cout << "Unit test was obtained from:" << endl << endl;
  cout << "  The Robinson Projection-A Computation Algorithm by John P. Snyder" << endl;
  cout << "  Cartography and Geographic Information Systems" << endl;
  cout << "  Vol. 17, No. 4, 1990, Pages 301-305" << endl;

}



