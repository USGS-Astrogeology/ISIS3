/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <iomanip>

#include <QString>

#include "IException.h"
#include "Planar.h"
#include "ProjectionFactory.h"
#include "Preference.h"
#include "RingPlaneProjection.h"

using namespace std;
using namespace Isis;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << "UNIT TEST FOR Planar Projection" << endl << endl;

  Pvl lab;
  lab.addGroup(PvlGroup("Mapping"));
  PvlGroup &mapGroup = lab.findGroup("Mapping");
  mapGroup += PvlKeyword("ProjectionName", "Planar");
  mapGroup += PvlKeyword("TargetName", "Saturn");
  mapGroup += PvlKeyword("RingLongitudeDirection", "Clockwise");
  mapGroup += PvlKeyword("RingLongitudeDomain", "180");
  mapGroup += PvlKeyword("MinimumRingRadius", "0.0");
  mapGroup += PvlKeyword("MaximumRingRadius", "2000000.0");
  mapGroup += PvlKeyword("MinimumRingLongitude", "-20.0");
  mapGroup += PvlKeyword("MaximumRingLongitude", "130.0");

  cout << "Test missing center azimuth keyword ..." << endl;
  try {
    Planar p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  mapGroup += PvlKeyword("CenterRingLongitude", "0.0");

  cout << "Test missing CenterRingRadius keyword ..." << endl;
  try {
    Planar p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  mapGroup += PvlKeyword("CenterRingRadius", "200000.0");

  try {
    Isis::Planar *p = (Planar *) ProjectionFactory::Create(lab);

    // Projection 1 test
    cout << "Projection 1 parameters..." << endl;
    cout << "Projection version         = " << p->Version().toStdString() << endl;
    cout << "  Projection name          =  " <<  p->Name().toStdString()  << endl;
    cout << "  Target name              =  " << (std::string) mapGroup["TargetName"]  << endl;
    cout << "  RingLongitude direction  = " << p->RingLongitudeDirectionString() << endl;
    cout << "  RingLongitude domain     = " << p->RingLongitudeDomainString() << endl;
    cout << "  Minimum ring radius      = " << p->MinimumRingRadius() << endl;
    cout << "  Maximum ring radius      = " << p->MaximumRingRadius() << endl;
    cout << "  Minimum ring longitude   = " << p->MinimumRingLongitude() << endl;
    cout << "  Maximum ring longitude   = " << p->MaximumRingLongitude() << endl;
    cout << "  Center ring radius       = " << p->CenterRingRadius() << endl;
    cout << "  Center ring longitude    = " << p->CenterRingLongitude() << endl;
    cout << endl;

    // Test TrueScaleRingRadius method
     cout << "Test TrueScaleRingRadius method..." << endl;
     cout << "TrueScaleRingRadius = " << p->TrueScaleRingRadius() << endl;
     cout << endl;

    // SetGround(const double ring radius, const double az)
    cout << "Test SetGround method ... " << endl;
    cout << std::setprecision(16);
    cout << "Setting ground to (1000.0,45.0)" << endl;
    p->SetGround(1000.0, 45.0);
    cout << "RingRadius:             " << p->LocalRadius() << endl;
    cout << "RingLongitude:          " << p->RingLongitude() << endl;
    cout << "XCoord:                 " << p->XCoord() << endl;
    cout << "YCoord:                 " << p->YCoord() << endl;
    cout << endl;

    cout << "Test SetCoordinate method ... " << endl;
    cout << "Setting coordinate to (0.2617993877991494,-0.8726646259971648)" << endl;
    p->SetCoordinate(0.2617993877991494, -0.8726646259971648);
    cout << "RingRadius:             " << p->LocalRadius() << endl;
    cout << "RingLongitude:          " << p->RingLongitude() << endl;
    cout << "XCoord:                 " << p->XCoord() << endl;
    cout << "YCoord:                 " << p->YCoord() << endl;
    cout << endl;
  
    // XYRange method not implemented yet. Do we need it?
   cout << "Test XYRange method ... " << endl;
   double minX, maxX, minY, maxY;
    if (p->XYRange(minX, maxX, minY, maxY)) {
      cout << "Minimum X:  " << minX << endl;
      cout << "Maximum X:  " << maxX << endl;
      cout << "Minimum Y:  " << minY << endl;
      cout << "Maximum Y:  " << maxY << endl;
      cout << endl;
    } 

    Planar *s = p;
    cout << "Test Name and comparision methods ... " << endl;
    cout << "Name:       " << s->Name().toStdString() << endl;
    cout << "operator==  " << (*s == *s) << endl;
    cout << endl;

    cout << "Testing default options in constructor with Projection 2 ... " << endl;
    mapGroup.deleteKeyword("CenterRingLongitude");
    mapGroup.deleteKeyword("CenterRingRadius");
    Planar p2(lab, true);
    cout << lab << endl;
    cout << "Default projection parameters == Original projection ?" << (*p == p2) << endl;
    cout << endl;
    cout << endl;

    cout << "Testing more SetGround conditions...RingLongitudeDirection = CounterClockwise and RingLongitudeDomain = 360" << endl;
    mapGroup["RingLongitudeDirection"].setValue("CounterClockwise");
    mapGroup["RingLongitudeDomain"].setValue("360");
    Planar p3(lab, true);

    // Projection 3 test
    cout << "Projection 3 parameters..." << endl;
    cout << "  Projection name          =  " <<  p3.Name().toStdString()  << endl;
    cout << "  Target name              =  " << (std::string) mapGroup["TargetName"]  << endl;
    cout << "  RingLongitude direction  = " << p3.RingLongitudeDirectionString() << endl;
    cout << "  RingLongitude domain     = " << p3.RingLongitudeDomainString() << endl;
    cout << "  Minimum ring radius      = " << p3.MinimumRingRadius() << endl;
    cout << "  Maximum ring radius      = " << p3.MaximumRingRadius() << endl;
    cout << "  Minimum ring longitude   = " << p3.MinimumRingLongitude() << endl;
    cout << "  Maximum ring longitude   = " << p3.MaximumRingLongitude() << endl;
    cout << "  Center ring radius       = " << p3.CenterRingRadius() << endl;
    cout << "  Center ring longitude    = " << p3.CenterRingLongitude() << endl;
    cout << endl;

    cout << "  Setting ground to (1000.0,45.0)" << endl;
    p3.SetGround(1000.0, 45.0);
    cout << "    RingRadius:       =  " << p3.LocalRadius() << endl;
    cout << "    RingLongitude:    =  " << p3.RingLongitude() << endl;
    cout << "    XCoord:           =  " << p3.XCoord() << endl;
    cout << "    YCoord:           =  " << p3.YCoord() << endl;
    cout << endl;

    cout << "Testing SetGround error condition..." << endl;
    cout << "  Setting ground to (-1000.0,45.0)" << endl;
    try {
      p3.SetGround(-1000.0, 45.0);
    }
    catch(IException &e) {
      e.print();
    }
    cout << endl;

    cout << "Testing more SetCoordinate methods ... " << endl;
    cout << "Setting coordinate to (0.2617993877991494,-0.8726646259971648)" << endl;
    p3.SetCoordinate(0.2617993877991494, -0.8726646259971648);
    cout << "RingRadius:             " << p3.LocalRadius() << endl;
    cout << "RingLongitude:          " << p3.RingLongitude() << endl;
    cout << "XCoord:                 " << p3.XCoord() << endl;
    cout << "YCoord:                 " << p3.YCoord() << endl;
    cout << endl;
    cout << "Setting coordinate to (0.2617993877991494,0.8726646259971648)" << endl;
    p3.SetCoordinate(0.2617993877991494, 0.8726646259971648);
    cout << "RingRadius:             " << p3.LocalRadius() << endl;
    cout << "RingLongitude:          " << p3.RingLongitude() << endl;
    cout << "XCoord:                 " << p3.XCoord() << endl;
    cout << "YCoord:                 " << p3.YCoord() << endl;
    cout << endl;
    cout << endl;

    cout << "Testing Mapping() methods ... " << endl;

    mapGroup.deleteKeyword("MinimumRingRadius");
    mapGroup.deleteKeyword("MaximumRingRadius");
    mapGroup.deleteKeyword("MinimumRingLongitude");
    mapGroup.deleteKeyword("MaximumRingLongitude");

    // Projection 4 test
    Planar p4(lab, true);
    cout << "Projection 4 parameters...No range" << endl;
    cout << "  Projection name          =  " <<  p4.Name().toStdString()  << endl;
    cout << "  Target name              =  " << (std::string) mapGroup["TargetName"]  << endl;
    cout << "  RingLongitude direction  = " << p4.RingLongitudeDirectionString() << endl;
    cout << "  RingLongitude domain     = " << p4.RingLongitudeDomainString() << endl;
    cout << "  Center ring radius       = " << p4.CenterRingRadius() << endl;
    cout << "  Center ring longitude    = " << p4.CenterRingLongitude() << endl;
    cout << endl;

    Pvl tmp1;
    Pvl tmp2;
    Pvl tmp3;
    tmp1.addGroup(p->Mapping());
    tmp2.addGroup(p->MappingRingRadii());
    tmp3.addGroup(p->MappingRingLongitudes());

    cout << "Mapping() = " << endl;
    cout << tmp1 << endl;
    cout << "MappingRadii() = " << endl;
    cout << tmp2 << endl;
    cout << "MappingRingLongitudes() = " << endl;
    cout << tmp3 << endl;
    cout << endl;
  }
  catch(IException &e) {
    e.print();
  }
}



