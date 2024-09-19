/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <cfloat>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <climits>

#include "Constants.h"
#include "IException.h"
#include "Planar.h"
#include "Preference.h"
#include "Projection.h"
#include "ProjectionFactory.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "RingPlaneProjection.h"
#include "SpecialPixel.h"
#include "WorldMapper.h"

/**
  * @internal
  *   @history 2016-08-26 Kelvin Rodriguez - Changed invalidValue varible to have
  *                           both an int and double version to avoid undefined behavior in
  *                           trying to convert double to int. Part of porting to OSX 10.11
 */

using namespace std;
using namespace Isis;

class MyProjection : public RingPlaneProjection {
  public:
    // create a child class
    MyProjection(Pvl &lab) : RingPlaneProjection(lab) {
    }
    // override pure virtual methods
    QString Name() const {
      return "None";
    }
    QString Version() const {
      return "1.0";
    }

    // override other virtual methods
    virtual double TrueScaleRingRadius() const {
      return 45.0;
    }
    bool SetGround(const double ringRadius, const double ringLongitude) {
      if (ringRadius <= 0.0) {
        m_good = false;
      }
      else {
        m_ringRadius = ringRadius;
        m_ringLongitude = ringLongitude;
        double x = ringLongitude * 10.0;
        double y = ringRadius + 90.0;
        SetComputedXY(x, y);
        m_good = true;
      }
      return m_good;
    }
    virtual bool SetCoordinate(const double x, const double y) {
      SetXY(x, y);
      m_ringLongitude = GetX() / 10.0;
      m_ringRadius = GetY() - 90.0;
      m_good = true;
      return m_good;
    }
    bool XYRange(double &minX, double &maxX, double &minY, double &maxY) {
      XYRangeCheck(Null, Null);
      minX = DBL_MAX;
      minY = DBL_MAX;
      maxX = -DBL_MAX;
      maxY = -DBL_MAX;
      if(!m_groundRangeGood) return false;
      XYRangeCheck(m_minimumRingRadius, m_minimumRingLongitude);
      XYRangeCheck(m_minimumRingRadius, m_maximumRingLongitude);
      XYRangeCheck(m_maximumRingRadius, m_minimumRingLongitude);
      XYRangeCheck(m_maximumRingRadius, m_maximumRingLongitude);
      minX = m_minimumX;
      minY = m_minimumY;
      maxX = m_maximumX;
      maxY = m_maximumY;
      return true;
    }

};

class EmptyProjection : public RingPlaneProjection {
  public:
    EmptyProjection(Pvl &lab) : RingPlaneProjection(lab) {
    }

    // pure virtuals, must be overriden, no need to test since MyProjection will test these
    QString Name() const {
      return "None";
    }
    QString Version() const {
      return "1.0";
    }
};


class MyMapper : public WorldMapper {
  public:
    MyMapper() : WorldMapper() {};
    double ProjectionX(const double worldX) const {
      return worldX / 2.0;
    };
    double ProjectionY(const double worldY) const {
      return worldY / 3.0;
    };
    double WorldX(const double projectionX) const {
      return projectionX * 2.0;
    };
    double WorldY(const double projectionY) const {
      return projectionY * 3.0;
    };
    virtual double Resolution() const {
      return 0.5;
    }
};



// unitTest main
int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  // create a MyProjection object with the given labels
  void Doit(Pvl & lab);

  cout.precision(13);
  cout << "Unit test for RingPlaneProjection ..." << endl << endl;
  cout << "///////////////////////////////////////////////////////////" << endl;
  cout << "Test Error Throws from the constructor...\n" << endl;

  cout << "Test for missing Mapping Group" << endl;
  Pvl lab;  // Error throws from Projection
  Doit(lab);
  cout << endl;

  lab.addGroup(PvlGroup("Mapping"));
  PvlGroup &mg = lab.findGroup("Mapping");
  Doit(lab);
  cout << endl;

  cout << "Test for missing ring longitude direction in the mapping group" << endl;
  Doit(lab);
  cout << endl;

  cout << "Test for invalid ring longitude direction value" << endl;
  mg += PvlKeyword("RingLongitudeDirection", "Up");
  Doit(lab);
  cout << endl;

  cout << "Test for missing ring longitude domain in the mapping group" << endl;
  mg["RingLongitudeDirection"] = "CounterClockwise";
  Doit(lab);
  cout << endl;

  cout << "Test for invalid ring longitude domain value in the mapping group" << endl;
  mg += PvlKeyword("RingLongitudeDomain", "75");
  Doit(lab);
  cout << "///////////////////////////////////////////////////////////" << endl;
  cout << endl;

  mg["RingLongitudeDomain"] = "360";
  mg += PvlKeyword("ProjectionName", "MyProjection");

  cout << "Projection Specifications" << endl;
  MyProjection p(lab);
  // test methods that return properties of the projection
  cout << "Is Equatorial Cylindrical: " << p.IsEquatorialCylindrical() << endl;
  cout << "Ring Longitude Direction:  " << p.RingLongitudeDirectionString() << endl;
  cout << "Is Clockwise:              " << p.IsClockwise() << endl;
  cout << "Is CounterClockwise:       " << p.IsCounterClockwise() << endl;
  cout << "Ring Longitude Domain:     " << p.RingLongitudeDomainString() << endl;
  cout << "Has 360 domain:            " << p.Has360Domain() << endl;
  cout << "Has 180 domain:            " << p.Has180Domain() << endl;
  cout << "Has ground range:          " << p.HasGroundRange() << endl;
  cout << "Rotation:                  " << p.Rotation() << endl;
  cout << "Scale:                     " << p.Scale() << endl;
  cout << endl;
 cout << endl;

  //Test exceptions

  cout << "///////////////////////////////////////////////////////////" << endl;
  cout << "Test More Error Throws...\n" << endl;
  cout << "Testing invalid minimum ring radius..." << endl;
  mg += PvlKeyword("MinimumRingRadius", "-45.0");
  mg += PvlKeyword("MaximumRingRadius", "-80.0");
  mg += PvlKeyword("MinimumRingLongitude", "15.0");
  mg += PvlKeyword("MaximumRingLongitude", "-190.0");
  Doit(lab);
  cout << endl;

  cout << "Testing invalid maximum ring radius" << endl;
  mg["MinimumRingRadius"] = "80.0";
  Doit(lab);
  cout << endl;

  cout << "Testing unordered ring radius range" << endl;
  mg["MaximumRingRadius"].setValue("45.0", "units");
  Doit(lab);
  cout << endl;

  cout << "Testing unordered ring longitude range" << endl;
  mg["MaximumRingRadius"].setValue("180.0", "units");
  Doit(lab);
  cout << "///////////////////////////////////////////////////////////" << endl;
  cout << endl;

  mg["MaximumRingLongitude"] = "190.0";

  cout << "Testing xyRange methods...\n" << endl;
  MyProjection p2(lab);
  cout << "Get ground range from the labels...  " << endl;
  cout << "Has as ring longitude range:  " << p2.HasGroundRange() << endl;
  cout << "Minimum ring radius:          " << p2.MinimumRingRadius() << endl;
  cout << "Maximum ring radius:          " << p2.MaximumRingRadius() << endl;
  cout << "Minimum ring longitude:       " << p2.MinimumRingLongitude() << endl;
  cout << "Maximum ring longitude:       " << p2.MaximumRingLongitude() << endl;

  double minX, maxX, minY, maxY;
  if (p2.XYRange(minX, maxX, minY, maxY)) {
    cout << "Find coordinate range ...  " << endl;
    cout << "Minimum X:              " << minX << endl;
    cout << "Maximum X:              " << maxX << endl;
    cout << "Minimum Y:              " << minY << endl;
    cout << "Maximum Y:              " << maxY << endl;
    cout << endl;
  }

  cout << "Testing Ground coordinate routines" << endl;
  cout << "Setting ring radius to (91,  0):       " << p2.SetGround(91.0, 0.0) << endl;
  cout << "Is Good:                               " << p2.IsGood() << endl;
  cout << "Setting ring longitude to ( 0,  91):   " << p2.SetGround(0., 91.0) << endl;
  cout << "Is Good:                               " << p2.IsGood() << endl;
  cout << "Setting position to (60,  -5):         " << p2.SetGround(60.0, -5.0) << endl;
  cout << "Is Good:                               " << p2.IsGood() << endl;
  cout << "Ring Radius:                           " << p2.RingRadius() << endl;
  cout << "Local Ring Radius:                     " << p2.LocalRadius() << endl;
  cout << "Ring Longitude:                        " << p2.RingLongitude() << endl;
  cout << "XCoord:                                " << p2.XCoord() << endl;
  cout << "YCoord:                                " << p2.YCoord() << endl;
  cout << "Universal Ring Radius:                 " << p2.UniversalRingRadius() << endl;
  cout << "Universal Ring Longitude:              " << p2.UniversalRingLongitude() << endl;
  cout << endl;


  cout << "Testing Universal Ground coordinate routines" << endl;
  cout << "Setting position to (57.3920057293825,  355):  " << p2.SetUniversalGround(57.3920057293825, 355.0) << endl;
  cout << "Is Good:                                       " << p2.IsGood() << endl;
  cout << "Ring Radius:                                   " << p2.RingRadius() << endl;
  cout << "Ring Longitude:                                " << p2.RingLongitude() << endl;
  cout << "XCoord:                                        " << p2.XCoord() << endl;
  cout << "YCoord:                                        " << p2.YCoord() << endl;
  cout << "Universal Ring Radius:                         " << p2.UniversalRingRadius() << endl;
  cout << "Universal Ring Longitude:                      " << p2.UniversalRingLongitude() << endl;
  cout << "Setting bad position..." << p2.SetUniversalGround((double) Null, (double) Null) << endl;
  cout << endl;

  /* The following projection is used to test the ToPlanetographic() methods
   *   at the ring radius boundaries (2000 and 20000 meters). qFuzzyCompare() methods
   *   were added to accommodate for double imprecission.
   */
  Isis::Pvl radRangeTest;
  radRangeTest.addGroup(Isis::PvlGroup("Mapping"));
  Isis::PvlGroup &radTestGroup = radRangeTest.findGroup("Mapping");
  radTestGroup += Isis::PvlKeyword("TargetName", "Saturn");
  radTestGroup += Isis::PvlKeyword("ProjectionName", "Planar");
  radTestGroup += Isis::PvlKeyword("RingLongitudeDirection", "Clockwise");
  radTestGroup += Isis::PvlKeyword("RingLongitudeDomain", "180");
  radTestGroup += Isis::PvlKeyword("Scale", "5.0");
  radTestGroup += Isis::PvlKeyword("MinimumRingRadius", "2000.0");
  radTestGroup += Isis::PvlKeyword("MaximumRingRadius", "20000.0");
  radTestGroup += Isis::PvlKeyword("MinimumRingLongitude", "0.0");
  radTestGroup += Isis::PvlKeyword("MaximumRingLongitude", "360.0");
  radTestGroup += Isis::PvlKeyword("CenterRingRadius", "12000.0");
  radTestGroup += Isis::PvlKeyword("CenterRingLongitude", "0.0");
  Isis::Planar *radTestProjection = (Isis::Planar *) Isis::ProjectionFactory::Create(radRangeTest);

  cout << "Planar Projection Specifications" << endl;
  // test methods that return properties of the projection
  cout << "Is Equatorial Cylindrical: " << radTestProjection->IsEquatorialCylindrical() << endl;
  cout << "Ring Longitude Direction:  " << radTestProjection->RingLongitudeDirectionString() << endl;
  cout << "Is Clockwise:              " << radTestProjection->IsClockwise() << endl;
  cout << "Is CounterClockwise:       " << radTestProjection->IsCounterClockwise() << endl;
  cout << "Ring Longitude Domain:     " << radTestProjection->RingLongitudeDomainString() << endl;
  cout << "Has 360 domain:            " << radTestProjection->Has360Domain() << endl;
  cout << "Has 180 domain:            " << radTestProjection->Has180Domain() << endl;
  cout << "Has ground range:          " << radTestProjection->HasGroundRange() << endl;
  cout << "Rotation:                  " << radTestProjection->Rotation() << endl << endl;
  try {
    cout << "Setting position to (9000.0, 0.0)" << endl;
    radTestProjection->SetUniversalGround(9000.0, 0.0);
    cout << "Is Good:                     "
         << radTestProjection->IsGood() << endl;
    cout << "Ring Radius:                 "
         << radTestProjection->RingRadius() << endl;
    cout << "Ring Longitude:              "
         << radTestProjection->RingLongitude() << endl;
    cout << "XCoord:                      "
         << radTestProjection->XCoord() << endl;
    cout << "YCoord:                      "
         << radTestProjection->YCoord() << endl;
    cout << "Universal Ring Radius:       "
         << radTestProjection->UniversalRingRadius() << endl;
    cout << "Universal Ring Longitude:    "
         << radTestProjection->UniversalRingLongitude() << endl;
    cout << endl;
  }
  catch(IException &error) {
    error.print();
  }

  try {
    radTestProjection->SetUniversalGround(9000.0, 360.0);
    cout << "Setting position to (9000.0, 360.0)" << endl;
    cout << "Is Good:                     "
         << radTestProjection->IsGood() << endl;
    cout << "Ring Radius:                 "
         << radTestProjection->RingRadius() << endl;
    cout << "Ring Longitude:              "
         << radTestProjection->RingLongitude() << endl;
    cout << "XCoord:                      "
         << radTestProjection->XCoord() << endl;
    cout << "YCoord:                      "
         << radTestProjection->YCoord() << endl;
    cout << "Universal Ring Radius:       "
         << radTestProjection->UniversalRingRadius() << endl;
    cout << "Universal Ring Longitude:    "
         << radTestProjection->UniversalRingLongitude() << endl;
    cout << endl;
  }
  catch(IException &error) {
    error.print();
  }

  cout << "Testing == operator Projection conditions..."  << p.Name().toStdString() << endl;
  cout << "Projection 1 name and resolution = " << p.Name().toStdString() << " " << p.Resolution() << endl;
  cout << "Projection 2 name and resolution = " << p2.Name().toStdString() << " " << p2.Resolution() << endl;
  cout << "Projection 3 name and resolution = " << radTestProjection->Name().toStdString() << " " << radTestProjection->Resolution() << endl;

  if (p == p2)
     cout << "Projection 1 = Projection 2" << endl;
  else
    cout << "Projection 1 != Projection 2" << endl;

  if (p == *radTestProjection)
     cout << "Projection 1 = Projection3" << endl;
  else
    cout << "Projection 1 != Projection3" << endl;
  cout << endl;

  cout << "Testing projection coordinate routines" << endl;
  cout << "Setting x/y position to (-90,15):    " << p2.SetCoordinate(-90.0, 15.0) << endl;
  cout << "Is Good:                             " << p2.IsGood() << endl;
  cout << "Ring Radius:                         " << p2.RingRadius() << endl;
  cout << "Ring Longitude:                      " << p2.RingLongitude() << endl;
  cout << "XCoord:                              " << p2.XCoord() << endl;
  cout << "YCoord:                              " << p2.YCoord() << endl;
  cout << "Universal Ring Radius:               " << p2.UniversalRingRadius() << endl;
  cout << "Universal Ring Longitude:            " << p2.UniversalRingLongitude() << endl;
  cout << "WorldX:                              " << p2.WorldX() << endl;
  cout << "WorldY:                              " << p2.WorldY() << endl;
  cout << endl;

  p2.SetWorldMapper(new MyMapper());

  cout << "Testing world coordinate routines" << endl;
  cout << "Setting world x/y position to (-45,45):    " << p2.SetWorld(-45.0, 45.0) << endl;
  cout << "Is Good:                                   " << p2.IsGood() << endl;
  cout << "Ring Radius:                               " << p2.RingRadius() << endl;
  cout << "Ring Longitude:                            " << p2.RingLongitude() << endl;
  cout << "XCoord:                                    " << p2.XCoord() << endl;
  cout << "YCoord:                                    " << p2.YCoord() << endl;
  cout << "Universal Ring Radius:                     " << p2.UniversalRingRadius() << endl;
  cout << "Universal Ring Longitude:                  " << p2.UniversalRingLongitude() << endl;
  cout << "WorldX:                                    " << p2.WorldX() << endl;
  cout << "WorldY:                                    " << p2.WorldY() << endl;
  // cout << "ToProjectionX (-4500):                     " << p2.ToProjectionX(-4500.0) << endl;
  // cout << "ToProjectionY (45):                        " << p2.ToProjectionY(45.0) << endl;
  // cout << "ToWorldX:                                  " << p2.ToWorldX(p2.ToProjectionX(-4500.0)) << endl;
  // cout << "ToWorldY:                                  " << p2.ToWorldY(p2.ToProjectionY(45.0)) << endl;
  cout << "Resolution:                                " << p2.Resolution() << endl;
  cout << "Scale:                                     " << p2.Scale() << endl;
  cout << "True Scale Ring Radius:                    " << p2.TrueScaleRingRadius() << endl;
  cout << endl;

  cout << "Testing IsSky method" << endl;
  cout << p2.IsSky() << endl;
  mg += PvlKeyword("TargetName", "SKY");
  mg["RingLongitudeDirection"] = "Clockwise";
  Doit(lab);
  MyProjection p3(lab);
  cout << p3.IsSky() << endl;
  cout << endl;

  cout << "Testing == operator condition in class..." << endl;
  cout << "Projection 1 name and resolution             = " << p.Name().toStdString() << " " << p.Resolution() << endl;
  cout << "Projection 1 ring longitude direction        = " << p.RingLongitudeDirectionString() << endl;
  cout << "Projection 4 name and resolution             = " << p3.Name().toStdString() << " " << p3.Resolution() << endl;
  cout << "Projection 4 ring longitude direction string = " << p3.RingLongitudeDirectionString() << endl;
  if (p == p3)
     cout << "Projection 1 = Projection 4" << endl;
  else
    cout << "Projection 1 != Projection 4" << endl;

  cout << endl;

  cout << "Testing string routines" << endl;
  cout << p2.RingLongitudeDirectionString() << endl;
  cout << p2.RingLongitudeDomainString() << endl;
  cout << endl;

  cout << "Testing Name and comparision routines" << endl;
  cout << "Name:        " << p2.Name().toStdString() << endl;
  cout << "Version:     " << p2.Version().toStdString() << endl;
  cout << "operator==:  " << (p == p2) << endl;
  cout << "operator!=:  " << (p != p2) << endl;

  mg["RingLongitudeDirection"] = "CounterClockwise";
  mg["RingLongitudeDomain"] = "180";
  EmptyProjection noproj(lab);
  cout << endl;

  cout << "Testing no projection" << endl;
  noproj.SetUniversalGround(45.0, 270.0);
  cout << "Ring Radius:               " << noproj.RingRadius() << endl;
  cout << "Ring Longitude:            " << noproj.RingLongitude() << endl;
  cout << "Is clockwise?              " << noproj.IsClockwise() << endl;
  cout << "Is counterclockwise?       " << noproj.IsCounterClockwise() << endl;
  cout << "Ring Longitude direction = " << noproj.RingLongitudeDirectionString() << endl;
  cout << "Has 180 domain?            " << noproj.Has180Domain() << endl;
  cout << "Has 360 domain?            " << noproj.Has360Domain() << endl;
  cout << "Ring Longitude domain    = " << noproj.RingLongitudeDomainString() << endl;
  cout << "True scale ring radius   = " << noproj.TrueScaleRingRadius() << endl;
  double badvalue = Null;
  if (noproj.XYRange(badvalue, badvalue, badvalue,badvalue))
    cout << "Bad range" << endl;
  cout << endl;

  cout << "Testing ring radius methods " << endl;
  cout << noproj.LocalRadius() << endl;
  cout << endl;
  cout << "///////////////////////////////////////////////////////////" << endl;
  cout << endl;

  cout << "Testing bad coordinates " << endl;
  cout << "Testing bad ring radius/ring longitude... Is good?  " << noproj.SetGround(Null, Null) << endl;
  cout << "Testing bad x/y... Is good?  " << noproj.SetCoordinate(Null, Null) << endl;
  cout << endl;
  cout << "///////////////////////////////////////////////////////////" << endl;
  cout << endl;

  cout << endl;
  cout << "///////////////////////////////////////////////////////////" << endl;
  cout << "Test Error Throws for invalid inputs to conversion methods " << endl;

  // Keep an double and an int for used for invalid data.
  // Seperating the two prevents undefined behavior from trying to
  // convert Isis::Null to an integer.
  double invalidDouble = Null;
  int invalidInt = -INT_MAX;
  try {
    RingPlaneProjection::To180Domain(invalidDouble);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    RingPlaneProjection::To360Domain(invalidDouble);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    RingPlaneProjection::ToClockwise(invalidDouble, 180);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    RingPlaneProjection::ToClockwise(0, invalidInt);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    RingPlaneProjection::ToClockwise(invalidDouble, 360);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    RingPlaneProjection::ToCounterClockwise(0, invalidInt);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    RingPlaneProjection::ToCounterClockwise(invalidDouble, 360);
  }
  catch(IException &error) {
    error.print();
  }
  cout << endl;
  cout << "///////////////////////////////////////////////////////////" << endl;

  cout << "Testing conversion routines with valid inputs..." << endl;
  try {
    cout << "60 clockwise converted to " << p.ToCounterClockwise(60., 360) << " in 360 domain. " << endl;
    cout << "60 clockwise converted to " << p.ToCounterClockwise(60., 180) << " in 180 domain. " << endl;
  }
  catch(IException &error) {
    error.print();
  }
  try {
    cout << "60 counterclockwise converted to " << p3.ToClockwise(60., 360) << " in 360 domain. " << endl;
    cout << "60 counterclockwise converted to " << p3.ToClockwise(60., 180) << " in 180 domain. " << endl;
  }
  catch(IException &error) {
    error.print();
  }
  try {
    p.ToWorldX(invalidDouble);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    p.ToWorldY(invalidDouble);
  }
  catch(IException &error) {
    error.print();
  }
  cout << "///////////////////////////////////////////////////////////" << endl;
  cout << endl;
  cout << endl;

  // Add remaining keywords for Mapping() test
  mg += PvlKeyword("PixelResolution", "1.0");
  mg += PvlKeyword("Scale", "1.0");
  mg += PvlKeyword("UpperLeftCornerX", "1.0");
  mg += PvlKeyword("UpperLeftCornerY", "1.0");

  cout << "Rotation Tests" << endl;
  mg += PvlKeyword("Rotation", "90.0");
  mg["RingLongitudeDirection"] = "Clockwise";
  MyProjection p4(lab);
  cout << "Rotation:   " << p4.Rotation() << endl;
  cout << "Testing Ground coordinate routines" << endl;
  cout << "Setting ring radius to (-91,  0):    " << p4.SetGround(-91.0, 0.0) << endl;
  cout << "Is Good:                             " << p4.IsGood() << endl;
  cout << "Setting ring radius to ( 9001,  0):  " << p4.SetGround(9001.0, 0.0) << endl;
  cout << "Is Good:                             " << p4.IsGood() << endl;
  cout << "Setting position to (60,  -5):       " << p4.SetGround(60.0, -5.0) << endl;
  cout << "Is Good:                             " << p4.IsGood() << endl;
  cout << "Ring Radius:                         " << p4.RingRadius() << endl;
  cout << "Ring Longitude:                      " << p4.RingLongitude() << endl;
  cout << "XCoord:                              " << p4.XCoord() << endl;
  cout << "YCoord:                              " << p4.YCoord() << endl;
  cout << "Universal Ring Radius:               " << p4.UniversalRingRadius() << endl;
  cout << "Universal Ring Longitude:            " << p4.UniversalRingLongitude() << endl;
  cout << endl;

  cout << "Testing projection coordinate routines" << endl;
  cout << "Setting x/y position to (150,50):    " << p4.SetCoordinate(150.0, 50.0) << endl;
  cout << "Is Good:                             " << p4.IsGood() << endl;
  cout << "Ring Radius:                         " << p4.RingRadius() << endl;
  cout << "Ring Longitude:                      " << p4.RingLongitude() << endl;
  cout << "XCoord:                              " << p4.XCoord() << endl;
  cout << "YCoord:                              " << p4.YCoord() << endl;
  cout << "Universal Ring Radius:               " << p4.UniversalRingRadius() << endl;
  cout << "Universal Ring Longitude:            " << p4.UniversalRingLongitude() << endl;
  cout << "WorldX:                              " << p4.WorldX() << endl;
  cout << "WorldY:                              " << p4.WorldY() << endl;
  cout << endl;

  Pvl mapping;
  mapping.addGroup(p4.Mapping());
  cout << "Testing Mapping() methods" << endl;
  cout << "Mapping() = " << endl;
  cout << mapping << endl;
  mapping.deleteGroup("Mapping");
  mapping.addGroup(p4.MappingRingRadii());
  cout << "MappingRingRadii() = " << endl;
  cout << mapping << endl;
  mapping.deleteGroup("Mapping");
  mapping.addGroup(p4.MappingRingLongitudes());
  cout << "MappingRingLongitudes() = " << endl;
  cout << mapping << endl;
  mapping.deleteGroup("Mapping");
  cout << endl;

  //SetUpperLeftCorner(Displacement x, Displacement y)
}


void Doit(Pvl &lab) {
  try {
    MyProjection p(lab);
  }
  catch(IException &error) {
    error.print();
  }
}
