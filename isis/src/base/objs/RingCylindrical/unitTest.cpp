/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <iomanip>
#include "IException.h"
#include "RingCylindrical.h"
#include "ProjectionFactory.h"
#include "Preference.h"
#include "Projection.h"

using namespace Isis;
using namespace std;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << "UNIT TEST FOR RingCylindrical" << endl << endl;

  Pvl lab;
  lab.addGroup(PvlGroup("Mapping"));
  PvlGroup &mapGrp = lab.findGroup("Mapping");
  mapGrp += PvlKeyword("ProjectionName", "RingCylindrical");
  mapGrp += PvlKeyword("TargetName", "Saturn");
  mapGrp += PvlKeyword("RingLongitudeDirection", "Clockwise");
  mapGrp += PvlKeyword("RingLongitudeDomain", "180");
  // mapGrp += PvlKeyword("MinimumRingRadius", "0.0");
  mapGrp += PvlKeyword("MinimumRingRadius", "18000.0");
  mapGrp += PvlKeyword("MaximumRingRadius", "20000000.0");
  mapGrp += PvlKeyword("MinimumRingLongitude", "-20.0");
  mapGrp += PvlKeyword("MaximumRingLongitude", "130.0");

  cout << "Test missing center ring longitude keyword ..." << endl;
  try {
    RingCylindrical p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  mapGrp += PvlKeyword("CenterRingLongitude", "0.0");

  cout << "Test missing CenterRingRadius keyword ..." << endl;
  try {
    RingCylindrical p(lab);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  mapGrp += PvlKeyword("CenterRingRadius", "200000.0");

  try {
    RingCylindrical *p = (RingCylindrical *) ProjectionFactory::Create(lab);

    // Projection 1 test
    cout << "Projection 1 parameters..." << endl;
    cout << "Is equatorial cylindrical? = " << p->IsEquatorialCylindrical() << endl;
    cout << "Projection version         = " << p->Version() << endl;
    cout << "  Projection name          =  " <<  p->Name()  << endl;
    cout << "  Target name              =  " << (std::string) mapGrp["TargetName"]  << endl;
    cout << "  Ring Longitude direction = " << p->RingLongitudeDirectionString() << endl;
    cout << "  Ring Longitude domain    = " << p->RingLongitudeDomainString() << endl;
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

    cout << "Test SetGround method ... " << endl;
    cout << std::setprecision(16);
    cout << "Setting ground to (20000.0, 45.0)" << endl;
    p->SetGround(20000.0, 45.0);
    cout << "Ring Radius:            " << p->RingRadius() << endl;
    cout << "Ring Longitude:         " << p->RingLongitude() << endl;
    cout << "XCoord:                 " << p->XCoord() << endl;
    cout << "YCoord:                 " << p->YCoord() << endl;
    cout << endl;

    cout << "Test SetCoordinate method ... " << endl;
    cout << "Setting coordinate to (-157079.6326794896, 180000)" << endl;
    p->SetCoordinate(-157079.6326794896, 180000);
    cout << "Ring Radius:            " << p->RingRadius() << endl;
    cout << "Ring Longitude:         " << p->RingLongitude() << endl;
    cout << "XCoord:                 " << p->XCoord() << endl;
    cout << "YCoord:                 " << p->YCoord() << endl;
    cout << endl;

    cout << "Test SetCoordinate method for radius larger than max ring radius... " << endl;
    // cout << "Setting coordinate to (-157079.6326794896, 19800001)" << endl;
    // bool good = p->SetCoordinate(-157079.6326794896, 20000001);
    cout << "Setting coordinate to (-157079.6326794896, -20000001)" << endl;
    bool good = p->SetCoordinate(-157079.6326794896, -20000001);
    cout << "Set Coordinate is good? " << good << endl;
    cout << "Ring Radius:            " << p->RingRadius() << endl;
    cout << "Maximum Ring Radius:    " << mapGrp["MaximumRingRadius"][0] << endl;
    cout << endl;

    cout << "Test SetCoordinate method for radius smaller than min ring radius... " << endl;
    cout << "Setting coordinate to (-1570.6326794896, 184000.5)" << endl;
    good = p->SetCoordinate(-1570.6326794896, 184000.5);
    cout << "Set Coordinate is good? " << good << endl;
    cout << "Ring Radius:            " << p->RingRadius() << endl;
    cout << "Minimum Ring Radius:    " << mapGrp["MinimumRingRadius"][0] << endl;
    cout << endl;

    // cout << "Test SetCoordinate method ... " << endl;
    // cout << "Setting coordinate to (0.2617993877991494,-0.8726646259971648)" << endl;
    // p->SetCoordinate(0.2617993877991494, -0.8726646259971648);
    // cout << "Ring Radius:            " << p->RingRadius() << endl;
    // cout << "Ring Longitude:         " << p->RingLongitude() << endl;
    // cout << "XCoord:                 " << p->XCoord() << endl;
    // cout << "YCoord:                 " << p->YCoord() << endl;
    // cout << endl;

    cout << "Test XYRange method ... " << endl;
    double minX, maxX, minY, maxY;
    p->XYRange(minX, maxX, minY, maxY);
    cout << "Minimum X:  " << minX << endl;
    cout << "Maximum X:  " << maxX << endl;
    cout << "Minimum Y:  " << minY << endl;
    cout << "Maximum Y:  " << maxY << endl;
    cout << endl;

    RingCylindrical *s = p;
    cout << "Test Name and comparision method ... " << endl;
    cout << "Name:       " << s->Name() << endl;
    cout << "operator==  " << (*s == *s) << endl;
    mapGrp["CenterRingRadius"].setValue("10.0");
    RingCylindrical s2(lab);
    cout << "operator==  " << (*s == s2) << endl;
    cout << endl;

    cout << "Testing default options in constructor with Projection 2 ... " << endl;
    mapGrp.deleteKeyword("CenterRingLongitude");
    mapGrp.deleteKeyword("CenterRingRadius");
    RingCylindrical p2(lab, true);
    cout << lab << endl;
    cout << "Default projection parameters == Original projection ?" << (*p == p2) << endl;
    cout << endl;
    cout << endl;

    cout << "Testing more SetGround conditions...RingLongitudeDirection = CounterClockwise and RingLongitudeDomain = 360" << endl;
    mapGrp["RingLongitudeDirection"].setValue("CounterClockwise");
    mapGrp["RingLongitudeDomain"].setValue("360");
    RingCylindrical p3(lab, true);

    // Projection 3 test
    cout << "Projection 3 parameters..." << endl;
    cout << "  Projection name           =  " <<  p3.Name()  << endl;
    cout << "  Target name               =  " << (std::string) mapGrp["TargetName"]  << endl;
    cout << "  Ring Longitude direction  = " << p3.RingLongitudeDirectionString() << endl;
    cout << "  Ring Longitude domain     = " << p3.RingLongitudeDomainString() << endl;
    cout << "  Minimum ring radius       = " << p3.MinimumRingRadius() << endl;
    cout << "  Maximum ring radius       = " << p3.MaximumRingRadius() << endl;
    cout << "  Minimum ring longitude    = " << p3.MinimumRingLongitude() << endl;
    cout << "  Maximum ring longitude    = " << p3.MaximumRingLongitude() << endl;
    cout << "  Center ring radius        = " << p3.CenterRingRadius() << endl;
    cout << "  Center ring longitude     = " << p3.CenterRingLongitude() << endl;
    cout << endl;

    cout << "  Setting ground to (20000.0,45.0)" << endl;
    p3.SetGround(20000.0, 45.0);
    cout << "    Ring Radius:      =  " << p3.LocalRadius() << endl;
    cout << "    Ring Longitude:   =  " << p3.RingLongitude() << endl;
    cout << "    XCoord:           =  " << p3.XCoord() << endl;
    cout << "    YCoord:           =  " << p3.YCoord() << endl;
    cout << endl;

    cout << "Testing SetGround error conditions..." << endl;
    cout << "...Testing SetGround with radius < 0..." << endl;
    cout << "    Setting ground to (-1000.0,45.0)" << endl;
    if (!p3.SetGround(-1000.0, 45.0))
      cout << "   SetGround failed" << endl;
    cout << endl;

    cout << "...Testing SetGround with radius = 0..." << endl;
    cout << "    Setting ground to (0.0,45.0)" << endl;
    if(!p3.SetGround(0.0, 45.0))
      cout << "   SetGround failed" << endl;
    cout << endl;

    cout << "Testing more SetCoordinate methods ... " << endl;
    cout << "Setting coordinate to (0.2617993877991494,-0.8726646259971648)" << endl;
    p3.SetCoordinate(0.2617993877991494, -0.8726646259971648);
    cout << "Ring Radius:            " << p3.LocalRadius() << endl;
    cout << "Ring Longitude:         " << p3.RingLongitude() << endl;
    cout << "XCoord:                 " << p3.XCoord() << endl;
    cout << "YCoord:                 " << p3.YCoord() << endl;
    cout << endl;
    cout << "Setting coordinate to (0.2617993877991494,0.8726646259971648)" << endl;
    p3.SetCoordinate(0.2617993877991494, 0.8726646259971648);
    cout << "Ring Radius:            " << p3.LocalRadius() << endl;
    cout << "Ring Longitude:         " << p3.RingLongitude() << endl;
    cout << "XCoord:                 " << p3.XCoord() << endl;
    cout << "YCoord:                 " << p3.YCoord() << endl;
    cout << endl;
    cout << endl;

    cout << "Testing Mapping() methods ... " << endl;

    // Mapping methods with ground range
    cout << "   Mapping with ground range ... " << endl;
    Pvl tmp1;
    Pvl tmp2;
    Pvl tmp3;
    tmp1.addGroup(p->Mapping());
    tmp2.addGroup(p->MappingRingRadii());
    tmp3.addGroup(p->MappingRingLongitudes());

    cout << "Mapping() = " << endl;
    cout << tmp1 << endl;
    cout << "MappingRingRadii() = " << endl;
    cout << tmp2 << endl;
    cout << "MappingRingLongitudes() = " << endl;
    cout << tmp3 << endl;
    cout << endl;
    
    mapGrp.deleteKeyword("MinimumRingRadius");
    mapGrp.deleteKeyword("MaximumRingRadius");
    mapGrp.deleteKeyword("MinimumRingLongitude");
    mapGrp.deleteKeyword("MaximumRingLongitude");

    // Projection 4 test
    // Mapping methods with no ground range
    RingCylindrical p4(lab, true);
    cout << "Projection 4 parameters...No range" << endl;
    cout << "  Projection name           =  " <<  p4.Name()  << endl;
    cout << "  Target name               =  " << (std::string) mapGrp["TargetName"]  << endl;
    cout << "  Ring Longitude direction  = " << p4.RingLongitudeDirectionString() << endl;
    cout << "  Ring Longitude domain     = " << p4.RingLongitudeDomainString() << endl;
    cout << "  Center ring radius        = " << p4.CenterRingRadius() << endl;
    cout << "  Center ring longitude     = " << p4.CenterRingLongitude() << endl;
    cout << endl;
    Pvl tmpNoRange1;
    Pvl tmpNoRange2;
    Pvl tmpNoRange3;
    tmpNoRange1.addGroup(p4.Mapping());
    tmpNoRange2.addGroup(p4.MappingRingRadii());
    tmpNoRange3.addGroup(p4.MappingRingLongitudes());

    cout << "Mapping() = " << endl;
    cout << tmpNoRange1 << endl;
    cout << "MappingRingRadii() = " << endl;
    cout << tmpNoRange2 << endl;
    cout << "MappingRingLongitudes() = " << endl;
    cout << tmpNoRange3 << endl;
    cout << endl;

  }
  catch(IException &e) {
    e.print();
  }
}



