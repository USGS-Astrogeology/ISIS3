/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <iomanip>
#include "IException.h"
#include "PolarStereographic.h"
#include "ProjectionFactory.h"
#include "Preference.h"
#include "TProjection.h"

using namespace Isis;
using namespace std;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << "Unit Test For PolarStereographic" << endl << endl;
  cout << std::setprecision(14);

  Pvl lab;
  lab.addGroup(PvlGroup("Mapping"));
  PvlGroup &mg = lab.findGroup("Mapping");
  mg += PvlKeyword("EquatorialRadius", toString(6378388.0));
  mg += PvlKeyword("PolarRadius", toString(6356911.9));
  mg += PvlKeyword("LatitudeType", "Planetographic");
  mg += PvlKeyword("LongitudeDirection", "PositiveEast");
  mg += PvlKeyword("LongitudeDomain", toString(180));
  mg += PvlKeyword("MinimumLatitude", toString(-90.0));
  mg += PvlKeyword("MaximumLatitude", toString(0.0));
  mg += PvlKeyword("MinimumLongitude", toString(-180.0));
  mg += PvlKeyword("MaximumLongitude", toString(180.0));
  mg += PvlKeyword("ProjectionName", "PolarStereographic");


  cout << "Test missing center longitude keyword ..." << endl;
  try {
    PolarStereographic p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;
  mg += PvlKeyword("CenterLongitude", toString(-100.0));

  cout << "Test missing center latitude keyword ..." << endl;
  try {
    PolarStereographic p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;
  cout << "Test invalid center latitude keyword ..." << endl;
  mg += PvlKeyword("CenterLatitude", toString(0.0));
  try {
    PolarStereographic p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;
  mg.addKeyword(PvlKeyword("CenterLatitude", toString(-71.0)), PvlGroup::Replace);

  try {
    TProjection *p = (TProjection *) ProjectionFactory::Create(lab);
    //  Isis::PolarStereographic p(lab);

    cout << "Test SetGround method ... " << endl;
    cout << "Setting ground to (-75,150)" << endl;
    cout << "Successful (1-yes, 0-no): " << p->SetGround(-75.0, 150.0) << endl;
    cout << "Latitude:               " << p->Latitude() << endl;
    cout << "Longitude:              " << p->Longitude() << endl;
    cout << "XCoord:                 " << p->XCoord() << endl;
    cout << "YCoord:                 " << p->YCoord() << endl;
    cout << endl;
    
    cout << "Setting ground to (-90.0, 0.0)" << endl;

    cout << "Successful (1-yes, 0-no): " << p->SetGround(-90.0, 0.0) << endl;
    cout << "Latitude:               " << p->Latitude() << endl;
    cout << "Longitude:              " << p->Longitude() << endl;
    cout << "XCoord:                 " << p->XCoord() << endl;
    cout << "YCoord:                 " << p->YCoord() << endl;
    cout << endl;

    cout << "Setting ground to (90.0, 0.0)" << endl;

    cout << "Successful (1-yes, 0-no): " << p->SetGround(90.0, 0.0) << endl;
    cout << "Latitude:               " << p->Latitude() << endl;
    cout << "Longitude:              " << p->Longitude() << endl;
    cout << "XCoord:                 " << p->XCoord() << endl;
    cout << "YCoord:                 " << p->YCoord() << endl;
    cout << endl;

    cout << "Test SetCoordinate method ... " << endl;
    cout << "Setting coordinate to (-1540033.620970689,-560526.3978025292)" << endl;
    p->SetCoordinate(-1540033.620970689, -560526.3978025292);
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

    cout << "Test TrueScaleLatitude method... " << endl;
    cout << "TrueScaleLatitude = " << p->TrueScaleLatitude() << endl;
    cout << endl;

    TProjection *s = p;
    cout << "Test Name and comparision method ... " << endl;
    cout << "Name:       " << s->Name().toStdString() << endl;
    cout << "operator==  " << (*s == *s) << endl;
    cout << endl;

    cout << "Test default computation ... " << endl;
    mg.deleteKeyword("CenterLongitude");
    mg.deleteKeyword("CenterLatitude");
    PolarStereographic p2(lab, true);
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

    cout << "Unit test was obtained from:" << endl << endl;
    cout << "  Map Projections - A Working Manual" << endl;
    cout << "  USGS Professional Paper 1395 by John P. Snyder" << endl;
    cout << "  Pages 315-319" << endl;
  }
  catch(IException &e) {
    e.print();
  }
}
