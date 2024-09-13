/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <iomanip>

#include "TransverseMercator.h"
#include "IException.h"
#include "ProjectionFactory.h"
#include "Preference.h"
#include "TProjection.h"

using namespace Isis;
using namespace std;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << "UNIT TEST FOR TransverseMercator" << endl << endl;
  cout << "Part 1: Sphere..." << endl;

  Pvl lab;
  lab.addGroup(PvlGroup("Mapping"));
  PvlGroup &mapGroup = lab.findGroup("Mapping");
  mapGroup += PvlKeyword("EquatorialRadius", std::to_string(1.0));
  mapGroup += PvlKeyword("PolarRadius", std::to_string(1.0));
  mapGroup += PvlKeyword("LatitudeType", "Planetographic");
  mapGroup += PvlKeyword("LongitudeDirection", "PositiveEast");
  mapGroup += PvlKeyword("LongitudeDomain", std::to_string(180));
  mapGroup += PvlKeyword("MinimumLatitude", std::to_string(-70.0));
  mapGroup += PvlKeyword("MaximumLatitude", std::to_string(70.0));
  mapGroup += PvlKeyword("MinimumLongitude", std::to_string(-90.0));
  mapGroup += PvlKeyword("MaximumLongitude", std::to_string(-60.0));
  mapGroup += PvlKeyword("ProjectionName", "TransverseMercator");

  cout << "Test missing center longitude keyword ..." << endl;
  try {
    TransverseMercator p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  mapGroup += PvlKeyword("CenterLongitude", std::to_string(-75.0));

  cout << "Test missing center latitude keyword..." << endl;
  try {
    TransverseMercator p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  mapGroup += PvlKeyword("CenterLatitude", std::to_string(0.0));

  cout << "Test missing scale factor keyword..." << endl;
  try {
    TransverseMercator p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  mapGroup += PvlKeyword("ScaleFactor", std::to_string(1.0));

  try {
    TProjection *p = (TProjection *) ProjectionFactory::Create(lab);
    //  TransMercator p(lab);

    cout << "Test SetGround method ... " << endl;
    cout << std::setprecision(9);
    cout << "Setting ground to (40.5,-73.5)" << endl;
    p->SetGround(40.5, -73.5);
    cout << "Latitude:               " << p->Latitude() << endl;
    cout << "Longitude:              " << p->Longitude() << endl;
    cout << "XCoord:                 " << p->XCoord() << endl;
    cout << "YCoord:                 " << p->YCoord() << endl;
    cout << endl;


    cout << "Test SetCoordinate method ... " << endl;
    cout << "Setting coordinate to (0.0199077372, 0.707027609)" << endl;
    p->SetCoordinate(0.0199077372, 0.707027609);
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
    cout << "Name:       " << s->Name().toStdString() << endl;
    cout << "operator==  " << (*s == *s) << endl;
    cout << endl;

    cout << "Test default computation ... " << endl;
    mapGroup.deleteKeyword("CenterLongitude");
    mapGroup.deleteKeyword("CenterLatitude");
    mapGroup.deleteKeyword("ScaleFactor");
    TransverseMercator p2(lab, true);
    cout << lab << endl;
    cout << endl;

    cout << "Unit test was obtained from:" << endl << endl;
    cout << "  Map Projections - A Working Manual" << endl;
    cout << "  USGS Professional Paper 1395 by John P. Snyder" << endl;
    cout << "  Pages 268-269" << endl << endl;
  }
  catch(IException &e) {
    e.print();
  }

  cout << endl << "Part 2: Ellipsoid..." << endl;

  Pvl lab2;
  lab2.addGroup(PvlGroup("Mapping"));
  PvlGroup &mapGroup2 = lab2.findGroup("Mapping");
  mapGroup2 += PvlKeyword("EquatorialRadius", std::to_string(6378206.4));
  mapGroup2 += PvlKeyword("PolarRadius", std::to_string(6356583.8));
  mapGroup2 += PvlKeyword("LatitudeType", "Planetographic");
  mapGroup2 += PvlKeyword("LongitudeDirection", "PositiveEast");
  mapGroup2 += PvlKeyword("LongitudeDomain", std::to_string(180));
  mapGroup2 += PvlKeyword("MinimumLatitude", std::to_string(-70.0));
  mapGroup2 += PvlKeyword("MaximumLatitude", std::to_string(70.0));
  mapGroup2 += PvlKeyword("MinimumLongitude", std::to_string(-90.0));
  mapGroup2 += PvlKeyword("MaximumLongitude", std::to_string(-60.0));
  mapGroup2 += PvlKeyword("ProjectionName", "TransverseMercator");
  mapGroup2 += PvlKeyword("CenterLongitude", std::to_string(-75.0));
  mapGroup2 += PvlKeyword("CenterLatitude", std::to_string(0.0));
  mapGroup2 += PvlKeyword("ScaleFactor", std::to_string(0.9996));
  cout << endl;

  try {
    TProjection *p = (Isis::TProjection *) ProjectionFactory::Create(lab2);
    //  TransMercator p(lab);

    cout << "Test SetGround method ... " << endl;
    cout << std::setprecision(9);
    cout << "Setting ground to (40.5,-73.5)" << endl;
    p->SetGround(40.5, -73.5);
    cout << "Latitude:               " << p->Latitude() << endl;
    cout << "Longitude:              " << p->Longitude() << endl;
    cout << "XCoord:                 " << p->XCoord() << endl;
    cout << "YCoord:                 " << p->YCoord() << endl;
    cout << endl;


    cout << "Test SetCoordinate method ... " << endl;
    cout << "Setting coordinate to (127106.467, 4484124.43)" << endl;
    p->SetCoordinate(127106.467, 4484124.43);
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

    cout << "Unit test was obtained from:" << endl << endl;
    cout << "  Map Projections - A Working Manual" << endl;
    cout << "  USGS Professional Paper 1395 by John P. Snyder" << endl;
    cout << "  Pages 269-270" << endl;
  }
  catch(IException &e) {
    e.print();
  }
}

