/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <cfloat>
#include <cmath>
#include <iomanip>
#include <iostream>

#include "Constants.h"
#include "IException.h"
#include "Preference.h"
#include "Projection.h"
#include "ProjectionFactory.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "SpecialPixel.h"
#include "Target.h"
#include "TProjection.h"
#include "WorldMapper.h"

/**
  * @internal
  *   @history 2016-08-26 Kelvin Rodriguez - Changed invalidValue varible to have
  *                           both an int and double version to avoid undefined behavior in
  *                           trying to convert double to int. Part of porting to OSX 10.11
  *   @history 2016-12-28 Kristin Berry - Updated to test inLatitude range and two new
  *                           inLongitudeRange functions.
 */

using namespace std;
using namespace Isis;
class MyProjection : public Isis::TProjection {
  public:
    // create a child class
    MyProjection(Pvl &lab) :
      TProjection(lab) {
    }
    // override pure virtual methods
    QString Name() const {
      return "None";
    }
    QString Version() const {
      return "1.0";
    }

    // override other virtual methods
    virtual double TrueScaleLatitude() const {
      return 45.0;
    }
    bool SetGround(const double lat, const double lon) {
      if((lat < -90.0) || (lat > 90.0)) {
        m_good = false;
      }
      else {
        m_latitude = lat;
        m_longitude = lon;
        double x = lon * 10.0;
        double y = lat + 90.0;
        SetComputedXY(x, y);
        m_good = true;
      }
      return m_good;
    }
    virtual bool SetCoordinate(const double x, const double y) {
      SetXY(x, y);
      m_longitude = GetX() / 10.0;
      m_latitude = GetY() - 90.0;
      m_good = true;
      return m_good;
    }
    bool XYRange(double &minX, double &maxX, double &minY, double &maxY) {
      minX = DBL_MAX;
      minY = DBL_MAX;
      maxX = -DBL_MAX;
      maxY = -DBL_MAX;
      if(!m_groundRangeGood) return false;
      XYRangeCheck(m_minimumLatitude, m_minimumLongitude);
      XYRangeCheck(m_minimumLatitude, m_maximumLongitude);
      XYRangeCheck(m_maximumLatitude, m_minimumLongitude);
      XYRangeCheck(m_maximumLatitude, m_maximumLongitude);
      minX = m_minimumX;
      minY = m_minimumY;
      maxX = m_maximumX;
      maxY = m_maximumY;
      return true;
    }

    bool testXYRangeOblique(double &minX, double &maxX,
                            double &minY, double &maxY) {
      return xyRangeOblique(minX, maxX, minY, maxY);
    }

    // create wrappers to test protected methods
    bool testinLongitudeRange(double longitude) { 
      return inLongitudeRange(longitude);
    }

    bool testinLongitudeRange(double minLon, double maxLon, double longitude) {
      return inLongitudeRange(minLon, maxLon, longitude);
    }

    bool testinLatitudeRange(double latitude) {
      return inLatitudeRange(latitude);
    }


    // create output method to print results of private methods
    void Output() const {
      cout << "tCompute(0,sin(0)):             " << tCompute(0.0, 0.0) << endl;
      cout << "tCompute(pi/4,sin(pi/4)):       " << tCompute(HALFPI / 2.0, sin(HALFPI / 2.0)) << endl;
      cout << "tCompute(pi/2,sin(pi/2)):       " << tCompute(HALFPI, sin(HALFPI)) << endl;
      cout << "mCompute(sin(0),cos(0)):        " << mCompute(sin(0.0), cos(0.0)) << endl;
      cout << "mCompute(sin(pi/4),cos(pi/4)):  " << mCompute(sin(HALFPI / 2.0), cos(HALFPI / 2.0)) << endl;
      cout << "e4Compute():                    " << e4Compute() << endl;
      cout << "phi2Compute(0):                 " << phi2Compute(0.0) << endl;
      cout << "phi2Compute(10):                " << phi2Compute(10.0) << endl;
      cout << "phi2Compute(100):               " << phi2Compute(100.0) << endl;
      cout << "phi2Compute(1000):              " << phi2Compute(1000.0) << endl;
      cout << "qCompute(sin(0)):               " << qCompute(0.0) << endl;
    }

    double testqCompute(const double sinPhi) const {
      return qCompute(sinPhi);
    }
};

class EmptyProjection : public TProjection {
  public:
    EmptyProjection(Pvl &lab) :
      TProjection(lab) {
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
  cout << "Unit test for Projection ..." << endl << endl;
  cout << "///////////////////////////////////////////////////////////" << endl;
  cout << "Test Error Throws from the contructor...\n" << endl;

  cout << "Test for missing Mapping Group" << endl;
  Pvl lab;
  Doit(lab);
  cout << endl;

  cout << "Test for missing Equatorial Radius in the mapping group" << endl;
  lab.addGroup(PvlGroup("Mapping"));
  PvlGroup &mg = lab.findGroup("Mapping");
  Doit(lab);
  cout << endl;

  cout << "Test for missing polar radius in the mapping group" << endl;
  mg += PvlKeyword("EquatorialRadius", "-1.0");
  Doit(lab);
  cout << endl;

  cout << "Test for invalid Equatoral Radius value" << endl;
  mg += PvlKeyword("PolarRadius", "-0.95");
  Doit(lab);
  cout << endl;

  cout << "Test for invalid polar radius value" << endl;
  mg["EquatorialRadius"] = "1.0";
  mg += PvlKeyword("PolarRadius", "-0.95");
  Doit(lab);
  cout << endl;

  cout << "Test for missing latitude type in the mapping group" << endl;
  mg["PolarRadius"] = "0.95";
  Doit(lab);
  cout << endl;

  cout << "Test for invalid latitude type value" << endl;
  mg += PvlKeyword("LatitudeType", "Planeto");
  Doit(lab);
  cout << endl;

  cout << "Test for missing longitude direction in the mapping group" << endl;
  mg["LatitudeType"] = "Planetographic";
  Doit(lab);
  cout << endl;

  cout << "Test for invalid longitude direction value" << endl;
  mg += PvlKeyword("LongitudeDirection", "Up");
  Doit(lab);
  cout << endl;

  cout << "Test for missing longitude domain in the mapping group" << endl;
  mg["LongitudeDirection"] = "PositiveEast";
  Doit(lab);
  cout << endl;

  cout << "Test for invalid longitude domain value in the mapping group" << endl;
  mg += PvlKeyword("LongitudeDomain", "75");
  Doit(lab);
  cout << "///////////////////////////////////////////////////////////" << endl;
  cout << endl;

  mg["LongitudeDomain"] = "360";
  mg += PvlKeyword("ProjectionName", "MyProjection");

  cout << "Projection Specifications" << endl;
  MyProjection p(lab);
  // test methods that return properties of the target
  cout << "Equatorial Radius:         " << p.EquatorialRadius() << endl;
  cout << "Polar Radius:              " << p.PolarRadius() << endl;
  cout << "Eccentricity:              " << p.Eccentricity() << endl;
  // test methods that return properties of the projection
  cout << "Is Equatorial Cylindrical: " << p.IsEquatorialCylindrical() << endl;
  cout << "Latitude Type:             " << p.LatitudeTypeString().toStdString() << endl;
  cout << "Is Planetographic:         " << p.IsPlanetographic() << endl;
  cout << "Is Planetocentric:         " << p.IsPlanetocentric() << endl;
  cout << "Longitude Direction:       " << p.LongitudeDirectionString().toStdString() << endl;
  cout << "Is PositiveEast:           " << p.IsPositiveEast() << endl;
  cout << "Is PositiveWest:           " << p.IsPositiveWest() << endl;
  cout << "Longitude Domain:          " << p.LongitudeDomainString().toStdString() << endl;
  cout << "Has 360 domain:            " << p.Has360Domain() << endl;
  cout << "Has 180 domain:            " << p.Has180Domain() << endl;
  cout << "Has ground range:          " << p.HasGroundRange() << endl;
  cout << "Rotation:                  " << p.Rotation() << endl;
  cout << endl;

  cout << "Testing conversion methods" << endl;
  cout << "Bring -50   into 360 Domain:  " << p.To360Domain(-50.0) << endl;
  cout << "Bring   0-e into 360 Domain:  " << p.To360Domain(0.0 - 1E-10) << endl;
  cout << "Bring   0   into 360 Domain:  " << p.To360Domain(0.0) << endl;
  cout << "Bring   0+e into 360 Domain:  " << p.To360Domain(0.0 + 1E-10) << endl;
  cout << "Bring  50   into 360 Domain:  " << p.To360Domain(50.0) << endl;
  cout << "Bring 360-e into 360 Domain:  " << p.To360Domain(360.0 - 1E-10) << endl;
  cout << "Bring 360   into 360 Domain:  " << p.To360Domain(360.0) << endl;
  cout << "Bring 360+e into 360 Domain:  " << p.To360Domain(360.0 + 1E-10) << endl;
  cout << "Bring 380   into 360 Domain:  " << p.To360Domain(380.0) << endl;
  cout << endl;
  cout << "Bring 240  into 180 Domain:  " << p.To180Domain(240.0) << endl;
  cout << "Bring 140  into 180 Domain:  " << p.To180Domain(140.0) << endl;
  cout << "Bring -180 into 180 Domain:  " << p.To180Domain(-180.0) << endl;
  cout << "Bring 180  into 180 Domain:  " << p.To180Domain(180.0) << endl;
  cout << endl;

  cout << "Change -90 to planetographic: " << p.ToPlanetographic(-90.0) << endl;
  cout << "Change -45 to planetographic: " << p.ToPlanetographic(-45.0) << endl;
  cout << "Change   0 to planetographic: " << p.ToPlanetographic(0.0) << endl;
  cout << "Change  45 to planetographic: " << p.ToPlanetographic(45.0) << endl;
  cout << "Change  90 to planetographic: " << p.ToPlanetographic(90.0) << endl;

  cout << "Change -90 to planetocentric: " << p.ToPlanetocentric(-90.0) << endl;
  cout << "Change -45 to planetocentric: " << p.ToPlanetocentric(-45.0) << endl;
  cout << "Change   0 to planetocentric: " << p.ToPlanetocentric(0.0) << endl;
  cout << "Change  45 to planetocentric: " << p.ToPlanetocentric(45.0) << endl;
  cout << "Change  90 to planetocentric: " << p.ToPlanetocentric(90.0) << endl;
  cout << endl;

  cout << "ocentric to ographic to ocentric = " << p.ToPlanetocentric(p.ToPlanetographic(45.0)) << endl;
  cout << "ographic to ocentric to ographic = " << p.ToPlanetographic(p.ToPlanetocentric(45.0)) << endl;
  cout << endl;

  //Test exceptions

  cout << "///////////////////////////////////////////////////////////" << endl;
  cout << "Test More Error Throws...\n" << endl;
  cout << "Testing unordered latitude range" << endl;
  mg += PvlKeyword("MinimumLatitude", "45.0");
  mg += PvlKeyword("MaximumLatitude", "-80.0");
  mg += PvlKeyword("MinimumLongitude", "15.0");
  mg += PvlKeyword("MaximumLongitude", "-190.0");
  Doit(lab);
  cout << endl;

  cout << "Testing invalid minimum latitude" << endl;
  mg["MinimumLatitude"] = "-95.0";
  Doit(lab);
  cout << endl;

  cout << "Testing invalid maximum latitude" << endl;
  mg["MinimumLatitude"].setValue("45.0", "units");
  mg["MaximumLatitude"].setValue("95.0", "units");
  Doit(lab);
  cout << endl;

  cout << "Testing unordered longitude range" << endl;
  mg["MaximumLatitude"].setValue("80.0", "units");
  Doit(lab);
  cout << "///////////////////////////////////////////////////////////" << endl;
  cout << endl;

  mg["MaximumLongitude"] = "190.0";

  cout << "Testing xyRange methods...\n" << endl;
  MyProjection p2(lab);
  cout << "Get ground range from the labels...  " << endl;
  cout << "Has as longitude range:  " << p2.HasGroundRange() << endl;
  cout << "Minimum latitude:        " << p2.MinimumLatitude() << endl;
  cout << "Maximum latitude:        " << p2.MaximumLatitude() << endl;
  cout << "Minimum longitude:       " << p2.MinimumLongitude() << endl;
  cout << "Maximum longitude:       " << p2.MaximumLongitude() << endl;
  cout << endl; 

  cout << "Testing inLatitudeRange, inLongitudeRange methods...\n" << endl;
  cout << "InLongitudeRange (15,190,0):   " << p2.testinLongitudeRange(15.0, 190.0, 0.0) << endl;
  cout << "InLongitudeRange (15,190,100): " << p2.testinLongitudeRange(15.0,190.0, 100.0) << endl;
  cout << "InLongitudeRange (100):        " << p2.testinLongitudeRange(100.0) << endl;
  cout << "InLongitudeRange (-12):        " << p2.testinLongitudeRange(-12.0) << endl;
  cout << "InLatitudeRange (-70):         " << p2.testinLatitudeRange(-70.0) << endl;
  cout << "InLatitudeRange (70):          " << p2.testinLatitudeRange(70.0) << endl;
  cout << endl; 

  double minX, maxX, minY, maxY;
  p2.XYRange(minX, maxX, minY, maxY);
  cout << "Find coordinate range ...  " << endl;
  cout << "Minimum X:              " << minX << endl;
  cout << "Maximum X:              " << maxX << endl;
  cout << "Minimum Y:              " << minY << endl;
  cout << "Maximum Y:              " << maxY << endl;

  p2.testXYRangeOblique(minX,  maxX,  minY,  maxY);
  cout << "Find coordinate range using xyRangeOblique...  " << endl;
  cout << "Minimum X:              " << minX << endl;
  cout << "Maximum X:              " << maxX << endl;
  cout << "Minimum Y:              " << minY << endl;
  cout << "Maximum Y:              " << maxY << endl;
  cout << endl;

  cout << "Testing Ground coordinate routines" << endl;
  cout << "Setting latitude to (-91,  0):  " << p2.SetGround(-91.0, 0.0) << endl;
  cout << "Is Good:                        " << p2.IsGood() << endl;
  cout << "Setting latitude to ( 91,  0):  " << p2.SetGround(91.0, 0.0) << endl;
  cout << "Is Good:                        " << p2.IsGood() << endl;
  cout << "Setting position to (60,  -5):  " << p2.SetGround(60.0, -5.0) << endl;
  cout << "Is Good:                        " << p2.IsGood() << endl;
  cout << "Latitude:                       " << p2.Latitude() << endl;
  cout << "Longitude:                      " << p2.Longitude() << endl;
  cout << "XCoord:                         " << p2.XCoord() << endl;
  cout << "YCoord:                         " << p2.YCoord() << endl;
  cout << "UniversalLatitude:              " << p2.UniversalLatitude() << endl;
  cout << "UniversalLongitude:             " << p2.UniversalLongitude() << endl;
  cout << endl;


  cout << "Testing Universal Ground coordinate routines" << endl;
  cout << "Setting position to (57.3920057293825,  355):  " << p2.SetUniversalGround(57.3920057293825, -5.0) << endl;
  cout << "Is Good:                                       " << p2.IsGood() << endl;
  cout << "Latitude:                                      " << p2.Latitude() << endl;
  cout << "Longitude:                                     " << p2.Longitude() << endl;
  cout << "XCoord:                                        " << p2.XCoord() << endl;
  cout << "YCoord:                                        " << p2.YCoord() << endl;
  cout << "UniversalLatitude:                             " << p2.UniversalLatitude() << endl;
  cout << "UniversalLongitude:                            " << p2.UniversalLongitude() << endl;
  cout << endl;

  /* The following projection is used to test the ToPlanetographic() methods
   *   at the latitude boundaries (-90 and 90 degrees). qFuzzyCompare() methods
   *   were added to accommodate for double imprecission.
   */
  Isis::Pvl latRangeTest;
  latRangeTest.addGroup(Isis::PvlGroup("Mapping"));
  Isis::PvlGroup &latTestGroup = latRangeTest.findGroup("Mapping");
  latTestGroup += Isis::PvlKeyword("TargetName", "Moon");
  latTestGroup += Isis::PvlKeyword("ProjectionName", "PolarStereographic");
  latTestGroup += Isis::PvlKeyword("EquatorialRadius", "1737400.0");
  latTestGroup += Isis::PvlKeyword("PolarRadius", "1737400.0");
  latTestGroup += Isis::PvlKeyword("LatitudeType", "Planetocentric");
  latTestGroup += Isis::PvlKeyword("LongitudeDirection", "PositiveEast");
  latTestGroup += Isis::PvlKeyword("LongitudeDomain", "360");
  latTestGroup += Isis::PvlKeyword("Scale", "5.0");
  latTestGroup += Isis::PvlKeyword("MinimumLatitude", "-90.0");
  latTestGroup += Isis::PvlKeyword("MaximumLatitude", "-45.0");
  latTestGroup += Isis::PvlKeyword("MinimumLongitude", "0.0");
  latTestGroup += Isis::PvlKeyword("MaximumLongitude", "360.0");
  latTestGroup += Isis::PvlKeyword("CenterLatitude", "-90.0");
  latTestGroup += Isis::PvlKeyword("CenterLongitude", "0.0");
  Isis::TProjection *latTestProjection =
      (Isis::TProjection *) Isis::ProjectionFactory::Create(latRangeTest);

  cout << "PolarStereographic Projection Specifications" << endl;
  // test methods that return properties of the target
  cout << "Equatorial Radius:         " << latTestProjection->EquatorialRadius() << endl;
  cout << "Polar Radius:              " << latTestProjection->PolarRadius() << endl;
  cout << "Eccentricity:              " << latTestProjection->Eccentricity() << endl;
  // test methods that return properties of the projection
  cout << "Is Equatorial Cylindrical: " << latTestProjection->IsEquatorialCylindrical() << endl;
  cout << "Latitude Type:             " << latTestProjection->LatitudeTypeString().toStdString() << endl;
  cout << "Is Planetographic:         " << latTestProjection->IsPlanetographic() << endl;
  cout << "Is Planetocentric:         " << latTestProjection->IsPlanetocentric() << endl;
  cout << "Longitude Direction:       " << latTestProjection->LongitudeDirectionString().toStdString() << endl;
  cout << "Is PositiveEast:           " << latTestProjection->IsPositiveEast() << endl;
  cout << "Is PositiveWest:           " << latTestProjection->IsPositiveWest() << endl;
  cout << "Longitude Domain:          " << latTestProjection->LongitudeDomainString().toStdString() << endl;
  cout << "Has 360 domain:            " << latTestProjection->Has360Domain() << endl;
  cout << "Has 180 domain:            " << latTestProjection->Has180Domain() << endl;
  cout << "Has ground range:          " << latTestProjection->HasGroundRange() << endl;
  cout << "Rotation:                  " << latTestProjection->Rotation() << endl << endl;
  try {
    cout << "Setting position to (-90.000000000000767, 0.0)" << endl;
    latTestProjection->SetUniversalGround(-90.000000000000767, 0.0);
    cout << "Is Good:                                       "
         << latTestProjection->IsGood() << endl;
    cout << "Latitude:                                      "
         << latTestProjection->Latitude() << endl;
    cout << "Longitude:                                     "
         << latTestProjection->Longitude() << endl;
    cout << "XCoord:                                        "
         << latTestProjection->XCoord() << endl;
    cout << "YCoord:                                        "
         << latTestProjection->YCoord() << endl;
    cout << "UniversalLatitude:                             "
         << latTestProjection->UniversalLatitude() << endl;
    cout << "UniversalLongitude:                            "
         << latTestProjection->UniversalLongitude() << endl;
    cout << endl;
  }
  catch(IException &error) {
    error.print();
  }

  try {
    latTestProjection->SetUniversalGround(90.000000000000767, 0.0);
    cout << "Setting position to (90.000000000000767, 0.0)" << endl;
    cout << "Is Good:                                       "
         << latTestProjection->IsGood() << endl;
    cout << "Latitude:                                      "
         << latTestProjection->Latitude() << endl;
    cout << "Longitude:                                     "
         << latTestProjection->Longitude() << endl;
    cout << "XCoord:                                        "
         << latTestProjection->XCoord() << endl;
    cout << "YCoord:                                        "
         << latTestProjection->YCoord() << endl;
    cout << "UniversalLatitude:                             "
         << latTestProjection->UniversalLatitude() << endl;
    cout << "UniversalLongitude:                            "
         << latTestProjection->UniversalLongitude() << endl;
    cout << endl;
  }
  catch(IException &error) {
    error.print();
  }

  cout << "Testing projection coordinate routines" << endl;
  cout << "Setting x/y position to (-2550,15):  " << p2.SetCoordinate(-2250.0, 15.0) << endl;
  cout << "Is Good:                             " << p2.IsGood() << endl;
  cout << "Latitude:                            " << p2.Latitude() << endl;
  cout << "Longitude:                           " << p2.Longitude() << endl;
  cout << "XCoord:                              " << p2.XCoord() << endl;
  cout << "YCoord:                              " << p2.YCoord() << endl;
  cout << "UniversalLatitude:                   " << p2.UniversalLatitude() << endl;
  cout << "UniversalLongitude:                  " << p2.UniversalLongitude() << endl;
  cout << "WorldX:                              " << p2.WorldX() << endl;
  cout << "WorldY:                              " << p2.WorldY() << endl;
  cout << endl;

  p2.SetWorldMapper(new MyMapper());

  cout << "Testing world coordinate routines" << endl;
  cout << "Setting world x/y position to (-4500,45):  " << p2.SetWorld(-4500.0, 45.0) << endl;
  cout << "Is Good:                                   " << p2.IsGood() << endl;
  cout << "Latitude:                                  " << p2.Latitude() << endl;
  cout << "Longitude:                                 " << p2.Longitude() << endl;
  cout << "XCoord:                                    " << p2.XCoord() << endl;
  cout << "YCoord:                                    " << p2.YCoord() << endl;
  cout << "UniversalLatitude:                         " << p2.UniversalLatitude() << endl;
  cout << "UniversalLongitude:                        " << p2.UniversalLongitude() << endl;
  cout << "WorldX:                                    " << p2.WorldX() << endl;
  cout << "WorldY:                                    " << p2.WorldY() << endl;
  cout << "ToProjectionX (-4500):                     " << p2.ToProjectionX(-4500.0) << endl;
  cout << "ToProjectionY (45):                        " << p2.ToProjectionY(45.0) << endl;
  cout << "ToWorldX:                                  " << p2.ToWorldX(p2.ToProjectionX(-4500.0)) << endl;
  cout << "ToWorldY:                                  " << p2.ToWorldY(p2.ToProjectionY(45.0)) << endl;
  cout << "Resolution:                                " << p2.Resolution() << endl;
  cout << "Scale:                                     " << p2.Scale() << endl;
  cout << "TrueScaleLatitude:                         " << p2.TrueScaleLatitude() << endl;
  cout << endl;

  cout << "Testing IsSky method" << endl;
  cout << p2.IsSky() << endl;
  mg += PvlKeyword("TargetName", "SKY");
  Doit(lab);
  MyProjection p3(lab);
  cout << p3.IsSky() << endl;
  cout << endl;

  cout << "Testing string routines" << endl;
  cout << p2.LatitudeTypeString().toStdString() << endl;
  cout << p2.LongitudeDirectionString().toStdString() << endl;
  cout << p2.LongitudeDomainString().toStdString() << endl;
  cout << endl;

  cout << "Testing Name and comparision routines" << endl;
  cout << "Name:        " << p2.Name().toStdString() << endl;
  cout << "Version:     " << p2.Version().toStdString() << endl;
  cout << "operator==:  " << (p == p2) << endl;
  cout << "operator!=:  " << (p != p2) << endl;

  mg["LongitudeDirection"] = "PositiveWest";
  mg["LongitudeDomain"] = "180";
  EmptyProjection noproj(lab);
  cout << endl;

  cout << "Testing no projection" << endl;
  noproj.SetUniversalGround(45.0, 270.0);
  cout << "Latitude:    " << noproj.Latitude() << endl;
  cout << "Longitude:   " << noproj.Longitude() << endl;
  cout << endl;

  cout << "Testing radius methods " << endl;
  cout << noproj.LocalRadius() << endl;
  cout << noproj.LocalRadius(0.0) << endl;
  cout << noproj.LocalRadius(90.0) << endl;
  cout << noproj.LocalRadius(-90.0) << endl;
  cout << endl;
  cout << "///////////////////////////////////////////////////////////" << endl;
  cout << "Test Error Throws for null input for LocalRadius " << endl;
  double nullLatitude = Null;
  try {
    noproj.LocalRadius(nullLatitude);
  }
  catch(IException &error) {
    error.print();
  }
  cout << "///////////////////////////////////////////////////////////" << endl;
  cout << endl;

  cout << "Testing compute methods " << endl;
  p.Output();
  cout << endl;
  cout << "///////////////////////////////////////////////////////////" << endl;
  cout << "Test Error Throw for compute methods..." << endl;
  // qCompute is not used for spherical targets
  // set polar radius = equatorial radius
  mg["PolarRadius"] = "1.0";
  Doit(lab);
  MyProjection p3a(lab);
  try {
    p3a.testqCompute(0);
  }
  catch(IException &error) {
    error.print();
  }
  mg["PolarRadius"] = "0.95";
  cout << "///////////////////////////////////////////////////////////" << endl;
  cout << endl;

  cout << "Testing static conversion methods " << endl;
  cout << " 0 degrees in hours: " << p.ToHours(0.0) << endl;
  cout << " 0 degrees in HMS format: " << p.ToHMS(0.0).toStdString() << endl;
  cout << " 0 degrees in DMS format: " << p.ToDMS(0.0).toStdString() << endl;
  cout << " 30.5 degrees in hours: " << p.ToHours(30.5) << endl;
  cout << " 30.5 degrees in HMS format: " << p.ToHMS(30.5).toStdString() << endl;
  cout << " 30.5 degrees in DMS format: " << p.ToDMS(30.5).toStdString() << endl;
  cout << " 40.3472 degrees in hours: " << p.ToHours(40.3472) << endl;
  cout << " 40.3472 degrees in HMS format: " << p.ToHMS(40.3472).toStdString() << endl;
  cout << " 40.3472 degrees in DMS format: " << p.ToDMS(40.3472).toStdString() << endl;
  cout << " 45 degrees in Hours: " << p.ToHours(45.0) << endl;
  cout << " 45 degrees in HMS format: " << p.ToHMS(45.0).toStdString() << endl;
  cout << " 45 degrees in DMS format: " << p.ToDMS(45.0).toStdString() << endl;
  cout << " 180 degrees in Hours: " << p.ToHours(180.0) << endl;
  cout << " 180 degrees in HMS format: " << p.ToHMS(180.0).toStdString() << endl;
  cout << " 180 degrees in DMS format: " << p.ToDMS(180.0).toStdString() << endl;
  cout << " 360 degrees in Hours: " << p.ToHours(360.0) << endl;
  cout << " 360 degrees in HMS format: " << p.ToHMS(360.0).toStdString() << endl;
  cout << " 360 degrees in DMS format: " << p.ToDMS(360.0).toStdString() << endl;
  cout << "-390 To180Domain:          " << p.To180Domain(-390) << endl;
  cout << "-390 To360Domain:          " << p.To360Domain(-390) << endl;
  cout << " 50 to Planetocentric (sphere): " << p.ToPlanetocentric(50, 180000, 180000) << endl;
  cout << " 50 to Planetographic (sphere): " << p.ToPlanetographic(50, 180000, 180000) << endl;
  cout << "-30 ToPositiveEast (180 domain):  " << p.ToPositiveEast(-30, 180) << endl;
  cout << " 30 ToPositiveEast (360 domain): " << p.ToPositiveEast(30, 360) << endl;
  cout << " 30 ToPositiveWest (180 domain): " << p.ToPositiveWest(30, 180) << endl;
  cout << "-30 ToPositiveWest (360 domain):  " << p.ToPositiveWest(-30, 360) << endl;

  cout << endl;
  cout << "///////////////////////////////////////////////////////////" << endl;
  cout << "Test Error Throws for invalid inputs to conversion methods " << endl;

  // Keep an double and an int for used for invalid data.
  // Seperating the two prevents undefined behavior from trying to
  // convert Isis::Null to an integer.
  double invalidValue = Null;
  int invalidInt = -INT_MAX;
  try {
    TProjection::To180Domain(invalidValue);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    TProjection::To360Domain(invalidValue);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    p.ToPlanetocentric(-100);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    p.ToPlanetocentric(100);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    p.ToPlanetocentric(invalidValue);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    TProjection::ToPlanetocentric(invalidValue, 1, 1);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    p.ToPlanetographic(invalidValue);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    p.ToPlanetographic(-100);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    p.ToPlanetographic(100);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    TProjection::ToPlanetographic(invalidValue, 1, 1);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    TProjection::ToPositiveEast(invalidValue, 180);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    TProjection::ToPositiveEast(0, invalidInt);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    TProjection::ToPositiveWest(invalidValue, 360);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    TProjection::ToPositiveWest(0, invalidInt);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    p.ToProjectionX(invalidValue);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    p.ToProjectionY(invalidValue);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    p.ToWorldX(invalidValue);
  }
  catch(IException &error) {
    error.print();
  }
  try {
    p.ToWorldY(invalidValue);
  }
  catch(IException &error) {
    error.print();
  }
  cout << "///////////////////////////////////////////////////////////" << endl;
  cout << endl;
  cout << endl;

  cout << "Rotation Tests" << endl;
  mg += PvlKeyword("Rotation", "90.0");
  mg["LongitudeDirection"] = "PositiveEast";
  MyProjection p4(lab);
  cout << "Rotation:     " << p4.Rotation() << endl;
  cout << "Testing Ground coordinate routines" << endl;
  cout << "Setting latitude to (-91,  0):  " << p4.SetGround(-91.0, 0.0) << endl;
  cout << "Is Good:                        " << p4.IsGood() << endl;
  cout << "Setting latitude to ( 91,  0):  " << p4.SetGround(91.0, 0.0) << endl;
  cout << "Is Good:                        " << p4.IsGood() << endl;
  cout << "Setting position to (60,  -5):  " << p4.SetGround(60.0, -5.0) << endl;
  cout << "Is Good:                        " << p4.IsGood() << endl;
  cout << "Latitude:                       " << p4.Latitude() << endl;
  cout << "Longitude:                      " << p4.Longitude() << endl;
  cout << "XCoord:                         " << p4.XCoord() << endl;
  cout << "YCoord:                         " << p4.YCoord() << endl;
  cout << "UniversalLatitude:              " << p4.UniversalLatitude() << endl;
  cout << "UniversalLongitude:             " << p4.UniversalLongitude() << endl;
  cout << endl;

  cout << "Testing projection coordinate routines" << endl;
  cout << "Setting x/y position to (150,50):  " << p4.SetCoordinate(150.0, 50.0) << endl;
  cout << "Is Good:                             " << p4.IsGood() << endl;
  cout << "Latitude:                            " << p4.Latitude() << endl;
  cout << "Longitude:                           " << p4.Longitude() << endl;
  cout << "XCoord:                              " << p4.XCoord() << endl;
  cout << "YCoord:                              " << p4.YCoord() << endl;
  cout << "UniversalLatitude:                   " << p4.UniversalLatitude() << endl;
  cout << "UniversalLongitude:                  " << p4.UniversalLongitude() << endl;
  cout << "WorldX:                              " << p4.WorldX() << endl;
  cout << "WorldY:                              " << p4.WorldY() << endl;
  cout << endl;

  Pvl mapping;
  mapping.addGroup(p4.Mapping());
  cout << "Testing Mapping() methods" << endl;
  cout << "Mapping() = " << endl;
  cout << mapping << endl;
  mapping.deleteGroup("Mapping");
  mapping.addGroup(p4.MappingLatitudes());
  cout << "MappingLatitudes() = " << endl;
  cout << mapping << endl;
  mapping.deleteGroup("Mapping");
  mapping.addGroup(p4.MappingLongitudes());
  cout << "MappingLongitudes() = " << endl;
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
