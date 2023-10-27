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
#include "Longitude.h"
#include "Preference.h"
#include "Projection.h"
#include "ProjectionFactory.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "SpecialPixel.h"
#include "WorldMapper.h"

using namespace std;
using namespace Isis;
class MyProjectionType : public Projection {
  public:
    // create a child subclass
    MyProjectionType (Pvl &lab) : Projection(lab) {
      if ((m_mappingGrp.hasKeyword("EquatorialRadius")) &&
          (m_mappingGrp.hasKeyword("PolarRadius"))) {
        m_equatorialRadius = m_mappingGrp["EquatorialRadius"];
        m_polarRadius = m_mappingGrp["PolarRadius"];
      }
      // else  if (m_mappingGrp.hasKeyword("TargetName")) {
      //   PvlGroup radii = TargetRadii((string)m_mappingGrp["TargetName"]);
      //   m_equatorialRadius = radii["EquatorialRadius"];
      //   m_polarRadius = radii["PolarRadius"];
      // }


      // Get the LatitudeType
      if ((std::string) m_mappingGrp["LatitudeType"] == "Planetographic") {
        m_latitudeType = Planetographic;
      }
      else if ((std::string) m_mappingGrp["LatitudeType"] == "Planetocentric") {
        m_latitudeType = Planetocentric;
      }

      // Get the LongitudeDirection
      if ((std::string) m_mappingGrp["LongitudeDirection"] == "PositiveWest") {
        m_longitudeDirection = PositiveWest;
      }
      else if ((std::string) m_mappingGrp["LongitudeDirection"] == "PositiveEast") {
        m_longitudeDirection = PositiveEast;
      }

      // Get the LongitudeDomain
      if ((std::string) m_mappingGrp["LongitudeDomain"] == "360") {
        m_longitudeDomain = 360;
      }
      else if ((std::string) m_mappingGrp["LongitudeDomain"] == "180") {
        m_longitudeDomain = 180;
      }

      // Get the ground range if it exists
      m_groundRangeGood = false;
      if ((m_mappingGrp.hasKeyword("MinimumLatitude")) &&
          (m_mappingGrp.hasKeyword("MaximumLatitude")) &&
          (m_mappingGrp.hasKeyword("MinimumLongitude")) &&
          (m_mappingGrp.hasKeyword("MaximumLongitude"))) {
        m_minimumLatitude  = m_mappingGrp["MinimumLatitude"];
        m_maximumLatitude  = m_mappingGrp["MaximumLatitude"];
        m_minimumLongitude = m_mappingGrp["MinimumLongitude"];
        m_maximumLongitude = m_mappingGrp["MaximumLongitude"];
        m_groundRangeGood = true;
      }
      else {
        // if no ground range is given, initialize the min/max lat/lon to 0
        m_minimumLatitude  = 0.0;
        m_maximumLatitude  = 0.0;
        m_minimumLongitude = 0.0;
        m_maximumLongitude = 0.0;
      }

      // Initialize miscellaneous protected data elements
      m_eccentricity = 1.0 -
                       (m_polarRadius * m_polarRadius) /
                       (m_equatorialRadius * m_equatorialRadius);
      m_eccentricity = sqrt(m_eccentricity);

      // initialize the rest of the x,y,lat,lon member variables
      m_latitude = Null;
      m_longitude = Null;
      setProjectionType(Triaxial);
    }

    enum LatitudeType { Planetocentric, Planetographic};
    enum LongitudeDirection { PositiveEast, PositiveWest};

    // override pure virtual methods
    virtual bool operator== (const MyProjectionType &proj) {
      if (!Projection::operator== (proj)) return false;
      if (EquatorialRadius() != proj.EquatorialRadius()) return false;
      if (PolarRadius() != proj.PolarRadius()) return false;
      if (IsPlanetocentric() != proj.IsPlanetocentric()) return false;
      if (IsPositiveWest() != proj.IsPositiveWest()) return false;
      // if (Resolution() != proj.Resolution()) return false;
      // if (Name() != proj.Name()) return false;
      return true;
    }

    virtual double LocalRadius(double latitude) const {
      if (latitude == Null) {
        throw IException(IException::Unknown, 
                       "Unable to calculate local radius. The given latitude value [" 
                       + IString(latitude) + "] is invalid.", 
                       _FILEINFO_);
      }
      double a = m_equatorialRadius;
      double c = m_polarRadius;
      // to save calculations, if the target is spherical, return the eq. rad
      if (a - c < DBL_EPSILON) {
        return a;
      }
      else {
        double lat = latitude * PI / 180.0;
        return  a * c / sqrt(pow(c * cos(lat), 2) + pow(a * sin(lat), 2));
      }
    }

    virtual double LocalRadius() const {
      return LocalRadius(m_latitude);
    }

    virtual QString Name() const = 0;
    virtual QString Version() const = 0;
    virtual double TrueScaleLatitude() const  {return 0.0;}

    bool SetGround(const double lat, const double lon) {
      if ((lat < -90.0) || (lat > 90.0)) {
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
      m_longitude = GetX(
) / 10.0;
      m_latitude = GetY() - 90.0;
      m_good = true;
      return m_good;
   }

    bool SetUniversalGround(const double lat, const double lon) {
      if (lat == Null || lon == Null) {
        m_good = false;
        return m_good;
      }
      // Deal with the longitude first
      m_longitude = lon;
      if (m_longitudeDirection == PositiveWest) m_longitude = -lon;
      if (m_longitudeDomain == 180) {
        m_longitude = To180Domain(m_longitude);
      }
      else {
        // Do this because longitudeDirection could cause (-360,0)
        m_longitude = To360Domain(m_longitude);
      }

      // Deal with the latitude
      if (m_latitudeType == Planetographic) {
        m_latitude = ToPlanetographic(lat);
      }
      else {
        m_latitude = lat;
      }

      // Now the lat/lon are in user defined coordinates so set them
      return SetGround(m_latitude, m_longitude);
    }

    double MyPixelResolution() const {
      return PixelResolution();
    }

    double Scale() const {
      if (m_mapper != NULL) {
        double lat = TrueScaleLatitude() * PI / 180.0;
        double a = m_polarRadius * cos(lat);
        double b = m_equatorialRadius * sin(lat);
        double localRadius = m_equatorialRadius * m_polarRadius /
                             sqrt(a * a + b * b);

        return localRadius / m_mapper->Resolution();
      }
      else {
        return 1.0;
      }
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

    PvlGroup Mapping() {
      PvlGroup mapping("Mapping");

      if (m_mappingGrp.hasKeyword("TargetName")) {
        mapping += m_mappingGrp["TargetName"];
      }

      mapping += m_mappingGrp["ProjectionName"];

      if (m_mappingGrp.hasKeyword("EquatorialRadius") && m_mappingGrp.hasKeyword("PolarRadius")) {
        mapping += m_mappingGrp["EquatorialRadius"];
        mapping += m_mappingGrp["PolarRadius"];
      }

      mapping += m_mappingGrp["LatitudeType"];
      mapping += m_mappingGrp["LongitudeDirection"];
      mapping += m_mappingGrp["LongitudeDomain"];

      if (m_mappingGrp.hasKeyword("PixelResolution")) {
        mapping += m_mappingGrp["PixelResolution"];
      }
      if (m_mappingGrp.hasKeyword("Scale")) {
        mapping += m_mappingGrp["Scale"];
      }
      if (m_mappingGrp.hasKeyword("UpperLeftCornerX")) {
        mapping += m_mappingGrp["UpperLeftCornerX"];
      }
      if (m_mappingGrp.hasKeyword("UpperLeftCornerY")) {
        mapping += m_mappingGrp["UpperLeftCornerY"];
      }

      if (HasGroundRange()) {
        mapping += m_mappingGrp["MinimumLatitude"];
        mapping += m_mappingGrp["MaximumLatitude"];
        mapping += m_mappingGrp["MinimumLongitude"];
        mapping += m_mappingGrp["MaximumLongitude"];
      }

      if (m_mappingGrp.hasKeyword("Rotation")) {
        mapping += m_mappingGrp["Rotation"];
      }

      return mapping;
    }

    void XYRangeCheck(const double latitude, const double longitude) {
      if (latitude == Null || longitude == Null) {
        m_good = false;
        return;
      }

      SetGround(latitude, longitude);
      if (!IsGood()) return;

      if (XCoord() < m_minimumX) m_minimumX = XCoord();
      if (XCoord() > m_maximumX) m_maximumX = XCoord();
      if (YCoord() < m_minimumY) m_minimumY = YCoord();
      if (YCoord() > m_maximumY) m_maximumY = YCoord();
      return;
    }

    // Add any remaining methods needed for testing
    double EquatorialRadius() const {return m_equatorialRadius; }
    double PolarRadius() const {return m_polarRadius; }
    bool IsPlanetocentric() const {return m_latitudeType == Planetocentric; }
    bool IsPositiveWest() const {return m_longitudeDirection == PositiveWest; }

    double To180Domain(const double lon) {
       double tlon = Longitude(lon, Angle::Degrees).force180Domain().degrees();
       return tlon;
    }

    double To360Domain(const double lon) {
      double result = lon;

      if ( (lon < 0.0 || lon > 360.0) && !qFuzzyCompare(lon, 0.0) && !qFuzzyCompare(lon, 360.0)) {
        result = Isis::Longitude(lon, Angle::Degrees).force360Domain().degrees();
      }
      return result;
    }

    double ToPlanetographic(double lat) {
      //Account for double rounding error.
      if (qFuzzyCompare(fabs(lat), 90.0)) {
        lat = qRound(lat);
      }
      double mylat = lat;
      mylat *= PI / 180.0;
      mylat = atan(tan(mylat) * (m_equatorialRadius / m_polarRadius) *
                   (m_equatorialRadius / m_polarRadius));
      mylat *= 180.0 / PI;
      return mylat;
    }
  
  protected:
    double m_latitude;
    double m_longitude; 
    LatitudeType m_latitudeType;
    LongitudeDirection m_longitudeDirection;
    int m_longitudeDomain; 
    double m_equatorialRadius; 
    double m_polarRadius; 
    double m_eccentricity; 
    double m_minimumLatitude;
    double m_maximumLatitude;
    double m_minimumLongitude;
    double m_maximumLongitude;
};

class MyProjection : public MyProjectionType {
  public:
   // create a child class
    MyProjection(Pvl &lab) :
      MyProjectionType(lab) {}

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
};

class MyProjection2 : public MyProjectionType {
  public:
   // create a child class
    MyProjection2(Pvl &lab) :
      MyProjectionType(lab) {}

    // override pure virtual methods
    QString Name() const {
      return "Something";
    }
    QString Version() const {
      return "1.0";
    }

    // override other virtual methods
    virtual double TrueScaleLatitude() const {
      return 45.0;
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
  cout << "Test Error Throws from the constructor...\n" << endl;

  cout << "Test for missing Mapping Group" << endl;
  Pvl lab;
  Doit(lab);
  cout << "///////////////////////////////////////////////////////////" << endl;
  cout << endl;

  lab.addGroup(PvlGroup("Mapping"));
  PvlGroup &mg = lab.findGroup("Mapping");
  mg += PvlKeyword("EquatorialRadius", "1.0");
  mg += PvlKeyword("PolarRadius", "0.95");
  mg += PvlKeyword("LatitudeType", "Planetographic");
  mg += PvlKeyword("LongitudeDirection", "PositiveEast");
  mg += PvlKeyword("LongitudeDomain", "360");
  mg += PvlKeyword("ProjectionName", "MyProjection");
  mg += PvlKeyword("MinimumLatitude", "45");
  mg += PvlKeyword("MaximumLatitude", "80.0");
  mg += PvlKeyword("MinimumLongitude", "15.0");
  mg += PvlKeyword("MaximumLongitude", "190.0");

  cout << "Projection Specifications" << endl;
  MyProjection p(lab);
  MyProjection2 pMy2(lab);
  // test methods that return properties of the projection
  cout << "Is Equatorial Cylindrical: " << p.IsEquatorialCylindrical() << endl;
  cout << "Has ground range:          " << p.HasGroundRange() << endl;
  cout << "Rotation:                  " << p.Rotation() << endl;
  cout << "Pixel Resolution:      " << p.MyPixelResolution() << endl;
  cout << "Resolution:      " << p.Resolution() << endl;
  cout << "Projection name = " << p.Name() << endl;
  cout << "Pixel resolution = " << p.MyPixelResolution() << endl;
  cout << endl;

  //Test exceptions
  Doit(lab);
  cout << "///////////////////////////////////////////////////////////" << endl;
  cout << endl;
  cout << "Test == operator options..." << endl;
  cout << endl;

  cout << "Test == operator with names not matching, but resolutions do" << endl;
  cout << "Projection 1 resolution = " << p.Resolution() << endl;
  cout << "Projection 2 resolution = " << pMy2.Resolution() << endl;
  cout << "Projection 1 name = " << p.Name() << endl;
  cout << "Projection 2 name = " << pMy2.Name() << endl;
  if (p == pMy2) 
    cout << "Result:   Match"  << endl;
  else
    cout << "Result:   No match" << endl;
  cout << endl;

  cout << "Test == operator with pixel resolutions not matching, but names do" << endl;
  mg += PvlKeyword("PixelResolution", std::to_string(2.0));
  MyProjection pMy3(lab);
  pMy3.SetWorldMapper(new MyMapper());
  cout << "Projection 1 resolution = " << p.Resolution() << endl;
  cout << "Projection 2 resolution = " << pMy3.Resolution() << endl;
  cout << "Projection 1 name = " << p.Name() << endl;
  cout << "Projection 2 name = " << pMy3.Name() << endl;

  if (p == pMy3) 
    cout << "Result:  Match"  << endl;
  else
    cout << "Result:  No match" << endl;
  cout << endl;

  cout << "Test == operator with both resolution and name matching" << endl;
  MyProjection pMy4(lab);
  cout << "Projection 1 resolution = " << p.Resolution() << endl;
  cout << "Projection 2 resolution = " << pMy3.Resolution() << endl;
  cout << "Projection 1 name = " << p.Name() << endl;
  cout << "Projection 2 name = " << pMy3.Name() << endl;

  if (p == pMy4) 
    cout << "Result:  Match"  << endl;
  else
    cout << "Result:  No match" << endl;
  cout << endl;

  cout << "///////////////////////////////////////////////////////////" << endl;
  cout << endl;

  cout << "Testing xyRange methods...\n" << endl;
  MyProjection p2(lab);
  cout << "Projection 2 name = " << p2.Name() << endl;
  cout << "Get ground range from the labels...  " << endl;
  cout << "Has a ground range:  " << p2.HasGroundRange() << endl;

  double minX, maxX, minY, maxY;
  p2.XYRange(minX, maxX, minY, maxY);
  cout << "Find coordinate range ...  " << endl;
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
  cout << "XCoord:                         " << p2.XCoord() << endl;
  cout << "YCoord:                         " << p2.YCoord() << endl;
  cout << endl;


  cout << "Testing Universal Ground coordinate routines" << endl;
  cout << "Setting position to (57.3920057293825,  355):  " << p2.SetUniversalGround(57.3920057293825, -5.0) << endl;
  cout << "Is Good:                                       " << p2.IsGood() << endl;
  cout << "XCoord:                                        " << p2.XCoord() << endl;
  cout << "YCoord:                                        " << p2.YCoord() << endl;
  cout << endl;

  Isis::Pvl latRangeTest;
  latRangeTest.addGroup(Isis::PvlGroup("Mapping"));
  Isis::PvlGroup &latTestGroup = latRangeTest.findGroup("Mapping");
  latTestGroup += Isis::PvlKeyword("TargetName", "Moon");
  latTestGroup += Isis::PvlKeyword("ProjectionName", "PolarStereographic");
  latTestGroup += Isis::PvlKeyword("EquatorialRadius", std::to_string(1737400.0));
  latTestGroup += Isis::PvlKeyword("PolarRadius", std::to_string(1737400.0));
  latTestGroup += Isis::PvlKeyword("LatitudeType", "Planetocentric");
  latTestGroup += Isis::PvlKeyword("LongitudeDirection", "PositiveEast");
  latTestGroup += Isis::PvlKeyword("LongitudeDomain", std::to_string(360));
  latTestGroup += Isis::PvlKeyword("Scale", std::to_string(5.0));
  latTestGroup += Isis::PvlKeyword("MinimumLatitude", std::to_string(-90.0));
  latTestGroup += Isis::PvlKeyword("MaximumLatitude", std::to_string(-45.0));
  latTestGroup += Isis::PvlKeyword("MinimumLongitude", std::to_string(0.0));
  latTestGroup += Isis::PvlKeyword("MaximumLongitude", std::to_string(360.0));
  latTestGroup += Isis::PvlKeyword("CenterLatitude", std::to_string(-90.0));
  latTestGroup += Isis::PvlKeyword("CenterLongitude", std::to_string(0.0));
  Isis::Projection &latTestProjection = *Isis::ProjectionFactory::Create(latRangeTest);
  
 cout << "PolarStereographic Projection Specifications" << endl;
  // test methods that return properties of the projection
  cout << "Is Equatorial Cylindrical: " << latTestProjection.IsEquatorialCylindrical() << endl;
  cout << "Has ground range:          " << latTestProjection.HasGroundRange() << endl;
  cout << "Rotation:                  " << latTestProjection.Rotation() << endl << endl;
  try {
    cout << "Setting position to (-90.000000000000767, 0.0)" << endl;
    latTestProjection.SetUniversalGround(-90.000000000000767, 0.0);
    cout << "Is Good:                                       "
         << latTestProjection.IsGood() << endl;
    cout << "XCoord:                                        "
         << latTestProjection.XCoord() << endl;
    cout << "YCoord:                                        "
         << latTestProjection.YCoord() << endl;
    cout << endl;
  }
  catch(IException &error) {
    error.print();
  }


  cout << "Testing projection coordinate routines" << endl;
  cout << "Setting x/y position to (-2550,15):  " << p2.SetCoordinate(-2250.0, 15.0) << endl;
  cout << "Is Good:                             " << p2.IsGood() << endl;
  cout << "XCoord:                              " << p2.XCoord() << endl;
  cout << "YCoord:                              " << p2.YCoord() << endl;
  cout << "WorldX:                              " << p2.WorldX() << endl;
  cout << "WorldY:                              " << p2.WorldY() << endl;
  cout << endl;

  p2.SetWorldMapper(new MyMapper());

  double invalidValue = Null;
  cout << "Testing world coordinate routines" << endl;
  cout << "Setting world x/y position to (-4500,45):  " << p2.SetWorld(-4500.0, 45.0) << endl;
  cout << "Is Good:                                   " << p2.IsGood() << endl;
  cout << "XCoord:                                    " << p2.XCoord() << endl;
  cout << "YCoord:                                    " << p2.YCoord() << endl;
  cout << "WorldX:                                    " << p2.WorldX() << endl;
  cout << "WorldY:                                    " << p2.WorldY() << endl;
  cout << "ToProjectionX (-4500):                     " << p2.ToProjectionX(-4500.0) << endl;
  cout << "ToProjectionY (45):                        " << p2.ToProjectionY(45.0) << endl;
  cout << "ToWorldX:                                  " << p2.ToWorldX(p2.ToProjectionX(-4500.0)) << endl;
  cout << "ToWorldY:                                  " << p2.ToWorldY(p2.ToProjectionY(45.0)) << endl;
  cout << "Resolution:                                " << p2.Resolution() << endl;
  cout << "Scale:                                     " << p2.Scale() << endl;
  cout << endl;

  cout << "Testing IsSky method" << endl;
  cout << p2.IsSky() << endl;
  mg += PvlKeyword("TargetName", "SKY");
  Doit(lab);
  MyProjection p3(lab);
  cout << p3.IsSky() << endl;
  cout << endl;

  cout << "Testing Name and comparision routines" << endl;
  cout << "Name:        " << p2.Name() << endl;
  cout << "ProjectionType = " << p2.projectionType() << endl;
  cout << "Version:     " << p2.Version() << endl;
  cout << "operator==:  " << (p == p2) << endl;
  cout << "operator!=:  " << (p != p2) << endl;

  cout << "Testing static conversion methods " << endl;
  cout << " 0 degrees in hours: " << p.ToHours(0.0) << endl;
  cout << " 0 degrees in HMS format: " << p.ToHMS(0.0) << endl;
  cout << " 0 degrees in DMS format: " << p.ToDMS(0.0) << endl;
  cout << " 30.5 degrees in hours: " << p.ToHours(30.5) << endl;
  cout << " 30.5 degrees in HMS format: " << p.ToHMS(30.5) << endl;
  cout << " 30.5 degrees in DMS format: " << p.ToDMS(30.5) << endl;
  cout << " 40.3472 degrees in hours: " << p.ToHours(40.3472) << endl;
  cout << " 40.3472 degrees in HMS format: " << p.ToHMS(40.3472) << endl;
  cout << " 40.3472 degrees in DMS format: " << p.ToDMS(40.3472) << endl;
  cout << " 45 degrees in Hours: " << p.ToHours(45.0) << endl;
  cout << " 45 degrees in HMS format: " << p.ToHMS(45.0) << endl;
  cout << " 45 degrees in DMS format: " << p.ToDMS(45.0) << endl;
  cout << " 180 degrees in Hours: " << p.ToHours(180.0) << endl;
  cout << " 180 degrees in HMS format: " << p.ToHMS(180.0) << endl;
  cout << " 180 degrees in DMS format: " << p.ToDMS(180.0) << endl;
  cout << " 360 degrees in Hours: " << p.ToHours(360.0) << endl;
  cout << " 360 degrees in HMS format: " << p.ToHMS(360.0) << endl;
  cout << " 360 degrees in DMS format: " << p.ToDMS(360.0) << endl;

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

  cout << "Rotation Tests" << endl;
  mg += PvlKeyword("Rotation", std::to_string(90.0));
  mg["LongitudeDirection"] = "PositiveEast";
  mg.deleteKeyword("EquatorialRadius");
  mg.deleteKeyword("PolarRadius");
  mg["TargetName"] = "Moon";
  MyProjection p4(lab);
  cout << "Rotation:     " << p4.Rotation() << endl;
  cout << "Testing Ground coordinate routines" << endl;
  cout << "Setting latitude to (-91,  0):  " << p4.SetGround(-91.0, 0.0) << endl;
  cout << "Is Good:                        " << p4.IsGood() << endl;
  cout << "Setting latitude to ( 91,  0):  " << p4.SetGround(91.0, 0.0) << endl;
  cout << "Is Good:                        " << p4.IsGood() << endl;
  cout << "Setting position to (60,  -5):  " << p4.SetGround(60.0, -5.0) << endl;
  cout << "Is Good:                        " << p4.IsGood() << endl;
  cout << "XCoord:                         " << p4.XCoord() << endl;
  cout << "YCoord:                         " << p4.YCoord() << endl;
  cout << endl;

  cout << "Testing projection coordinate routines" << endl;
  cout << "Setting x/y position to (150,50):  " << p4.SetCoordinate(150.0, 50.0) << endl;
  cout << "Is Good:                             " << p4.IsGood() << endl;
  cout << "XCoord:                              " << p4.XCoord() << endl;
  cout << "YCoord:                              " << p4.YCoord() << endl;
  cout << "WorldX:                              " << p4.WorldX() << endl;
  cout << "WorldY:                              " << p4.WorldY() << endl;
  cout << endl;

  Pvl mapping;
  mapping.addGroup(p4.Mapping());
  cout << "Testing Mapping() methods" << endl;
  // cout << "Mapping() = " << endl;
  cout << mapping << endl;
  mapping.deleteGroup("Mapping");
  cout << endl;

}


void Doit(Pvl &lab) {
  try {
    MyProjection p(lab);
  }
  catch(IException &error) {
    error.print();
  }
}
