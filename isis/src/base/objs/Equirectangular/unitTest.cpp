/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <iomanip>
#include "IException.h"
#include "Equirectangular.h"
#include "TProjection.h"
#include "ProjectionFactory.h"
#include "Preference.h"

using namespace std;
using namespace Isis;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << "UNIT TEST FOR Equirectangular" << endl << endl;

  Pvl lab;
  lab.addGroup(PvlGroup("Mapping"));
  PvlGroup &mapGroup = lab.findGroup("Mapping");
  mapGroup += PvlKeyword("EquatorialRadius", std::to_string(1.0));
  mapGroup += PvlKeyword("PolarRadius", std::to_string(1.0));
  mapGroup += PvlKeyword("LatitudeType", "Planetocentric");
  mapGroup += PvlKeyword("LongitudeDirection", "PositiveEast");
  mapGroup += PvlKeyword("LongitudeDomain", std::to_string(180));
  mapGroup += PvlKeyword("MinimumLatitude", std::to_string(-90.0));
  mapGroup += PvlKeyword("MaximumLatitude", std::to_string(90.0));
  mapGroup += PvlKeyword("MinimumLongitude", std::to_string(-180.0));
  mapGroup += PvlKeyword("MaximumLongitude", std::to_string(180.0));
  mapGroup += PvlKeyword("ProjectionName", "Equirectangular");

  cout << "Test missing center longitude keyword ..." << endl;
  try {
    Equirectangular p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  mapGroup += PvlKeyword("CenterLongitude", std::to_string(-90.0));

  cout << "Test missing center latitude keyword ..." << endl;
  try {
    Equirectangular p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  mapGroup += PvlKeyword("CenterLatitude", std::to_string(0.0));

  Projection &proj = *ProjectionFactory::Create(lab);
  TProjection *p = (TProjection *) &proj;
  cout << "Projection Name:        " << p->Name().toStdString() << endl;
  cout << "Version:                " << p->Version().toStdString() << endl;
  cout << "Equatorial Cylindrical? " << p->IsEquatorialCylindrical() << endl;
  cout << endl;
  cout << "Projection Name:        " << p->Name().toStdString() << endl;
  cout << "Version:                " << p->Version().toStdString() << endl;
  cout << "Equatorial Cylindrical? " << p->IsEquatorialCylindrical() << endl;
  cout << endl;

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

  cout << "Test TrueScaleLatitude method..." << endl;
  cout << "TrueScaleLatitude = " << p->TrueScaleLatitude() << endl;
  cout << endl;

  Projection *s = &proj;
  cout << "Test Name and comparision methods ... " << endl;
  cout << "Name:       " << s->Name().toStdString() << endl;
  cout << "operator==  " << (*s == *s) << endl;
  cout << endl;

  cout << "Testing allow defaults option ... " << endl;
  mapGroup.deleteKeyword("CenterLongitude");
  mapGroup.deleteKeyword("CenterLatitude");
  mapGroup.findKeyword("MinimumLatitude").setValue(std::to_string(0.0));
  mapGroup.findKeyword("MinimumLongitude").setValue(std::to_string(0.0));
  mapGroup.findKeyword("LongitudeDirection").setValue("PositiveWest");
  Equirectangular p2(lab, true);
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

  std::cout << "Check Invalid Latitude" << std::endl;
  mapGroup.addKeyword(PvlKeyword("CenterLatitude", std::to_string(90.0)), Pvl::Replace);
  std::cout << mapGroup << std::endl;
  try {
    Equirectangular p2(lab);
  }
  catch(IException &e) {
    e.print();
  }
}



