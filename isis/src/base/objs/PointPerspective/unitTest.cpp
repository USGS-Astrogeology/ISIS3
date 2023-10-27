/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <iomanip>
#include "PointPerspective.h"
#include "IException.h"
#include "ProjectionFactory.h"
#include "Preference.h"
#include "TProjection.h"

using namespace Isis;
using namespace std;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << "UNIT TEST FOR PointPerspective" << endl << endl;

  Pvl lab;
  lab.addGroup(PvlGroup("Mapping"));
  PvlGroup &mapGroup = lab.findGroup("Mapping");
  mapGroup += PvlKeyword("EquatorialRadius", std::to_string(1.0));
  mapGroup += PvlKeyword("PolarRadius", std::to_string(1.0));
  mapGroup += PvlKeyword("LatitudeType", "Planetographic");
  mapGroup += PvlKeyword("LongitudeDirection", "PositiveEast");
  mapGroup += PvlKeyword("LongitudeDomain", std::to_string(180));
  mapGroup += PvlKeyword("MinimumLatitude", std::to_string(0.0));
  mapGroup += PvlKeyword("MaximumLatitude", std::to_string(80.0));
  mapGroup += PvlKeyword("MinimumLongitude", std::to_string(0.0));
  mapGroup += PvlKeyword("MaximumLongitude", std::to_string(80.0));
  mapGroup += PvlKeyword("ProjectionName", "PointPerspective");

  cout << "Test missing center longitude keyword ..." << endl;
  try {
    PointPerspective p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  mapGroup += PvlKeyword("CenterLongitude", std::to_string(0.0));

  cout << "Test missing center latitude keyword..." << endl;
  try {
    PointPerspective p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  mapGroup += PvlKeyword("CenterLatitude", std::to_string(0.0));

  cout << "Test missing distance keyword..." << endl;
  try {
    PointPerspective p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  mapGroup += PvlKeyword("Distance", std::to_string(.00562));

  try {
    TProjection *p = (TProjection *) ProjectionFactory::Create(lab);
    //  Isis::PointPerspective p(lab);

    cout << "Test TrueScaleLatitude method... " << endl;
    cout << "TrueScaleLatitude = " << p->TrueScaleLatitude() << endl;
    cout << endl;

    cout << "Test SetGround method ... " << endl;
    cout << std::setprecision(9);
    cout << "Setting ground to (41,-74)" << endl;
    p->SetGround(41.0, -74.0);
    cout << "Latitude:               " << p->Latitude() << endl;
    cout << "Longitude:              " << p->Longitude() << endl;
    cout << "XCoord:                 " << p->XCoord() << endl;
    cout << "YCoord:                 " << p->YCoord() << endl;
    cout << endl;


    cout << "Test SetCoordinate method ... " << endl;
    cout << "Setting coordinate to (251640.079, 226487.551)" << endl;
    p->SetCoordinate(251640.079, 226487.551);
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

    TProjection *s = p;
    cout << "Test Name and comparision method ... " << endl;
    cout << "Name:       " << s->Name() << endl;
    cout << "operator==  " << (*s == *s) << endl;
    cout << endl;

    cout << "Test default computation ... " << endl;
    mapGroup.deleteKeyword("CenterLongitude");
    mapGroup.deleteKeyword("CenterLatitude");
    PointPerspective p2(lab, true);
    cout << lab << endl;
    cout << endl;

    cout << "Testing Mapping() methods ... " << endl;
    cout <<"This test outputs Table 27 (p.174) of Map Projections - A Working Manual" << endl;
    cout << "USGS Professional Paper 1395" << endl;
    cout << "Author:  John P. Snyder" << endl;
    cout << endl;
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

    cout<< "Testing SetGround..." <<endl;

    double lat = 80.0;
    double lon;

    while (lat >= 0) {
      lon = 80.0;
      while (lon >=0.0) {
      p->SetGround(lat,lon);
      cout << "<" << setprecision(4) << fixed << lat << "," << lon <<"> = ";
      cout << setprecision(4) << fixed << "<" << p->XCoord();
      cout << setprecision(4) << fixed << "," << p->YCoord() << ">" << endl;
      lon -= 10.0;
      }
      lat -=10.0;
    }

    cout << endl;
    cout << "Testing SetCoordinate..." << endl;
    cout <<"This is taken from the numerical example given on p. 321";
    cout << " of Map Projections - A Working Manual" << endl;
    cout << "USGS Professional Paper 1395" << endl;
    cout << "Author:  John P. Snyder" << endl;
    cout << endl;

    double x = 247194.09;
    double y = 222485.96;

    Pvl lab1;
    lab1.addGroup(PvlGroup("Mapping"));
    PvlGroup &mapGroup = lab1.findGroup("Mapping");
    mapGroup += PvlKeyword("EquatorialRadius", std::to_string(6371000));
    mapGroup+= PvlKeyword("PolarRadius",std::to_string(6371000));
    mapGroup += PvlKeyword("LatitudeType", "Planetographic");
    mapGroup += PvlKeyword("LongitudeDirection", "PositiveEast");
    mapGroup += PvlKeyword("LongitudeDomain", std::to_string(180));
    mapGroup += PvlKeyword("MinimumLatitude", std::to_string(-90.0));
    mapGroup += PvlKeyword("MaximumLatitude", std::to_string(90.0));
    mapGroup += PvlKeyword("MinimumLongitude", std::to_string(-180.0));
    mapGroup += PvlKeyword("MaximumLongitude", std::to_string(180.0));
    mapGroup += PvlKeyword("CenterLongitude", std::to_string(-77.0));
    mapGroup += PvlKeyword("CenterLatitude", std::to_string(39.0));
    mapGroup += PvlKeyword("Distance", std::to_string(500000));
    mapGroup += PvlKeyword("ProjectionName", "PointPerspective");

    TProjection *p1 = (TProjection *) ProjectionFactory::Create(lab1);

    p1->SetCoordinate(x,y);
    cout << "Latitude:                    " << p1->Latitude() << endl;
    cout << "Longitude:                   " << p1->Longitude() << endl;
    cout << "XCoord:                      " << p1->XCoord() << endl;
    cout << "YCoord:                      " << p1->YCoord() << endl;
  }
  catch(IException &e) {
    e.print();
  }
}
