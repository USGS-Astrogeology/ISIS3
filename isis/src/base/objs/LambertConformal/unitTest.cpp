/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <iomanip>
#include "LambertConformal.h"
#include "IException.h"
#include "IString.h"
#include "ProjectionFactory.h"
#include "Preference.h"
#include "TProjection.h"

using namespace Isis;
using namespace std;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << "UNIT TEST FOR LambertConformal" << endl << endl;

  Pvl lab;
  lab.addGroup(PvlGroup("Mapping"));
  PvlGroup &mapGroup = lab.findGroup("Mapping");
  mapGroup += PvlKeyword("EquatorialRadius", "1.0");
  mapGroup += PvlKeyword("PolarRadius", "1.0");
  mapGroup += PvlKeyword("LatitudeType", "Planetographic");
  mapGroup += PvlKeyword("LongitudeDirection", "PositiveEast");
  mapGroup += PvlKeyword("LongitudeDomain", "180");
  mapGroup += PvlKeyword("MinimumLatitude", "20.0");
  mapGroup += PvlKeyword("MaximumLatitude", "80.0");
  mapGroup += PvlKeyword("MinimumLongitude", "-180.0");
  mapGroup += PvlKeyword("MaximumLongitude", "180.0");
  mapGroup += PvlKeyword("ProjectionName", "LambertConformal");

  cout << "Test missing center longitude keyword ..." << endl;
  try {
    LambertConformal p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  mapGroup += PvlKeyword("CenterLongitude", "-96.0");

  cout << "Test missing center latitude keyword..." << endl;
  try {
    LambertConformal p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  mapGroup += PvlKeyword("CenterLatitude", "23.0");

  cout << "Test missing first standard parallel keyword..." << endl;
  try {
    LambertConformal p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  mapGroup += PvlKeyword("FirstStandardParallel", "33");

  cout << "Test missing second standard parallel keyword..." << endl;
  try {
    LambertConformal p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  mapGroup += PvlKeyword("SecondStandardParallel", "45");

  try {
    // Isis::Projection &p = *Isis::ProjectionFactory::Create(lab);
    TProjection *p = (TProjection *) ProjectionFactory::Create(lab);
    //  Isis::LambertConformal p(lab);

    cout << "Test SetGround method ... " << endl;
    cout << std::setprecision(9);
    cout << "Setting ground to (35,-75)" << endl;
    p->SetGround(35.0, -75.0);
    cout << "Latitude:               " << p->Latitude() << endl;
    cout << "Longitude:              " << p->Longitude() << endl;
    cout << "XCoord:                 " << p->XCoord() << endl;
    cout << "YCoord:                 " << p->YCoord() << endl;
    cout << endl;


    cout << "Test SetCoordinate method ... " << endl;
    cout << "Setting coordinate to (0.29667846, 0.246211229)" << endl;
    p->SetCoordinate(0.29667846, 0.246211229);
    cout << "Latitude:               " << p->Latitude() << endl;
    cout << "Longitude:              " << p->Longitude() << endl;
    cout << "XCoord:                 " << p->XCoord() << endl;
    cout << "YCoord:                 " << p->YCoord() << endl;
    cout << endl;
    p->SetCoordinate(0.0, 0.0);

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
    LambertConformal p2(lab, true);
    cout << lab << endl;
    cout << endl;

    cout << "Test TrueScaleLatitude method... " << endl;
    cout << "TrueScaleLatitude = " << p->TrueScaleLatitude() << endl;
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

    cout << "Test invalid combinations of mapping parameters ..." << endl;

    mapGroup.deleteKeyword("CenterLatitude");
    mapGroup += PvlKeyword("CenterLatitude", "-90.0");
    try {
      LambertConformal p(lab);
    }
    catch(IException &e) {
      e.print();
    }
    cout << endl;

    mapGroup.deleteKeyword("CenterLatitude");
    mapGroup += PvlKeyword("CenterLatitude", "90.0");
    mapGroup.deleteKeyword("FirstStandardParallel");
    mapGroup += PvlKeyword("FirstStandardParallel", "-60");

    try {
      LambertConformal p(lab);
    }
    catch(IException &e) {
      e.print();
    }
    cout << endl;





    cout << "Unit test was obtained from:" << endl << endl;
    cout << "  Map Projections - A Working Manual" << endl;
    cout << "  USGS Professional Paper 1395 by John P. Snyder" << endl;
    cout << "  Pages 295-297" << endl;
  }
  catch(IException &e) {
    e.print();
  }
}

