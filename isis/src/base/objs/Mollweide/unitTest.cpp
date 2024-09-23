/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <cfloat>
#include <cmath>
#include <iostream>
#include <iomanip>

#include "Constants.h"
#include "IException.h"
#include "Mollweide.h"
#include "ProjectionFactory.h"
#include "Preference.h"
#include "TProjection.h"

using namespace Isis;
using namespace std;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << "Unit Test For the Mollweide Projection" << endl << endl;

  Pvl lab;
  lab.addGroup(PvlGroup("Mapping"));
  PvlGroup &mapGroup = lab.findGroup("Mapping");
  mapGroup += PvlKeyword("EquatorialRadius", toString(0.7071067811865475));
  mapGroup += PvlKeyword("PolarRadius", toString(0.7071067811865475));
  mapGroup += PvlKeyword("LatitudeType", "Planetocentric");
  mapGroup += PvlKeyword("LongitudeDirection", "PositiveEast");
  mapGroup += PvlKeyword("LongitudeDomain", toString(180));
  mapGroup += PvlKeyword("MinimumLatitude", toString(-90.0));
  mapGroup += PvlKeyword("MaximumLatitude", toString(90.0));
  mapGroup += PvlKeyword("MinimumLongitude", toString(-180.0));
  mapGroup += PvlKeyword("MaximumLongitude", toString(180.0));
  mapGroup += PvlKeyword("ProjectionName", "Mollweide");


  cout << "Test missing center longitude keyword ..." << endl;

try {
    Mollweide p(lab);
  }
  catch(IException &e) {
    e.print();
  }

  cout << endl;

  mapGroup += PvlKeyword("CenterLongitude", toString(0.0));
  try {
    TProjection *p = (TProjection *) ProjectionFactory::Create(lab);


    cout << "Test SetGround method ... " << endl;
    cout << std::setprecision(16);

    double lat=90.0;
    double lon=90.0;

    cout << "Longitude = 90, while latitude is in the range [0:90]:" << endl;
    cout << endl;

    while (lat >= 0.0) {

    p->SetGround(lat,lon);        
    cout << setprecision(6) << fixed <<"Latitude:\t" << p->Latitude();
    cout << "\tXCoord:\t\t"  << p->XCoord();
    cout << "\tYCoord:\t\t" << p->YCoord();
    cout << endl;
    lat -= 5.0;
      
    }


    cout << endl;
    cout << "Test SetCoordinate method ... " << endl;
    cout << endl;

    cout << "Setting coordinate to (0.0,1.0)" << endl;
    p->SetCoordinate(0.0, 1.0);
    cout << "Latitude:               " << p->Latitude() << endl;
    cout << "Longitude:              " << p->Longitude() << endl;
    cout << endl;

    cout << "Setting coordinate to (1.0,0.0)" << endl;
    p->SetCoordinate(1.0, 0.0);
    cout << "Latitude:               " << p->Latitude() << endl;
    cout << "Longitude:              " << p->Longitude() << endl;
    cout << endl;

    cout << "Setting coordinate to (0.8059072939585296690978566,0.5920417498322624316742235)";
    cout << endl;
    p->SetCoordinate(0.8059072939585296690978566, 0.5920417498322624316742235);
    cout << "Latitude:               " << p->Latitude() << endl;
    cout << "Longitude:              " << p->Longitude() << endl;
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
    Mollweide p2(lab, true);
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
    cout << "  Pages 249-252" << endl;

  }
  catch(IException &e) {
    e.print();
  }
}



