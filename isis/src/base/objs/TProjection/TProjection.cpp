/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "TProjection.h"

#include <QObject>

#include <cfloat>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <vector>

#include <SpiceUsr.h>

#include "Constants.h"
#include "Displacement.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Longitude.h"
#include "NaifStatus.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "SpecialPixel.h"
#include "Target.h"
#include "WorldMapper.h"

using namespace std;
namespace Isis {
  /**
   * Constructs an empty TProjection object.
   *
   * @param label A PVL object containing map projection labels. These labels
   *            are fully described in the Isis Map Projection Users Guide. A
   *            brief example follows:
   *                   @code
   *                       Group = Mapping
   *                         EquatorialRadius = 3396190.0
   *                         PolarRadius = 3376200.0
   *                         LongitudeDirection = PositiveEast
   *                         LongitudeDomain = 360
   *                         LatitudeType = Planetographic
   *                         MinimumLatitude = 10.8920539924144
   *                         MaximumLatitude = 34.7603960060206
   *                         MinimumLongitude = 219.72432466275
   *                         MaximumLongitude = 236.186050244411
   *                         PixelResolution = 1387.31209461362
   *                         ProjectionName = SimpleCylindrical
   *                         CenterLongitude = 220.0
   *                       EndGroup
   *                     End
   *                   @endcode
   *
   * @throw IException::Unknown - "Projection failed. No target radii 
   *            available through keywords [EquatorialRadius and
   *             PolarRadius] or [TargetName]."
   * @throw IException::Unknown - "Projection failed. Invalid value 
   *            for keyword [EquatorialRadius]. It must be greater than
   *            zero."
   * @throw IException::Unknown - "Projection failed. Invalid value 
   *            for keyword [PolarRadius]. It must be greater than zero."
   * @throw IException::Unknown - "Projection failed. Invalid value 
   *            for keyword [LatitudeType] must be [Planetographic or
   *             Planetocentric]"
   * @throw IException::Unknown - "Projection failed. Invalid value 
   *            for keyword [LongitudeDirection] must be [PositiveWest or
   *            PositiveEast]"
   * @throw IException::Unknown - "Projection failed. Invalid value 
   *            for keyword [LongitudeDomain] must be [180 or 360]"
   * @throw IException::Unknown - "Projection failed. 
   *            [MinimumLatitude] is outside the range of [-90:90]"
   * @throw IException::Unknown - "Projection failed. 
   *            [MaximumLatitude] is outside the range of [-90:90]"
   * @throw IException::Unknown - "Projection failed. 
   *            [MinimumLatitude,MaximumLatitude] are not properly ordered"
   * @throw IException::Unknown - "Projection failed. 
   *            [MinimumLongitude,MaximumLongitude] are not properly ordered"
   * @throw IException::Unknown - "Projection failed. Invalid keyword 
   *            value(s). [EquatorialRadius] must be greater than or equal to
   *            [PolarRadius]"
   * @throw IException::Unknown - "Projection failed. Invalid label 
   *            group [Mapping]"
   * 
   */
  TProjection::TProjection(Pvl &label) : Projection::Projection(label) {
    try {
      // Mapping group is read by the parent
      // Get the radii from the EquatorialRadius and PolarRadius keywords
      if ((m_mappingGrp.hasKeyword("EquatorialRadius")) &&
          (m_mappingGrp.hasKeyword("PolarRadius"))) {
        m_equatorialRadius = m_mappingGrp["EquatorialRadius"];
        m_polarRadius = m_mappingGrp["PolarRadius"];
      }
      // Get the radii
      try {
         PvlGroup radii = Target::radiiGroup(label, m_mappingGrp);
         m_equatorialRadius = radii["EquatorialRadius"];
         m_polarRadius = radii["PolarRadius"];
      }
      catch (IException &e) {
        std::string msg = "Projection failed. No target radii are available "
                      "through keywords [EquatorialRadius and PolarRadius] "
                      "or [TargetName].";
        throw IException(e, IException::Unknown, msg, _FILEINFO_);
      }

      // Check the radii for validity
      if (m_equatorialRadius <= 0.0) {
        std::string msg = "Projection failed. Invalid value for keyword "
                      "[EquatorialRadius]. It must be greater than zero";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
      if (m_polarRadius <= 0.0) {
        std::string msg = "Projection failed. Invalid value for keyword "
                      "[PolarRadius]. It must be greater than zero";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }

      // Get the LatitudeType
      if ((std::string) m_mappingGrp["LatitudeType"] == "Planetographic") {
        m_latitudeType = Planetographic;
      }
      else if ((std::string) m_mappingGrp["LatitudeType"] == "Planetocentric") {
        m_latitudeType = Planetocentric;
      }
      else {
        QString msg = "Projection failed. Invalid value for keyword "
                      "[LatitudeType] must be "
                      "[Planetographic or Planetocentric]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }

      // Get the LongitudeDirection
      if ((std::string) m_mappingGrp["LongitudeDirection"] == "PositiveWest") {
        m_longitudeDirection = PositiveWest;
      }
      else if ((std::string) m_mappingGrp["LongitudeDirection"] == "PositiveEast") {
        m_longitudeDirection = PositiveEast;
      }
      else {
        std::string msg = "Projection failed. Invalid value for keyword "
                      "[LongitudeDirection] must be "
                      "[PositiveWest or PositiveEast]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }

      // Get the LongitudeDomain
      if ((std::string) m_mappingGrp["LongitudeDomain"] == "360") {
        m_longitudeDomain = 360;
      }
      else if ((std::string) m_mappingGrp["LongitudeDomain"] == "180") {
        m_longitudeDomain = 180;
      }
      else {
        std::string msg = "Projection failed. Invalid value for keyword "
                      "[LongitudeDomain] must be [180 or 360]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
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

        if ((m_minimumLatitude < -90.0) || (m_minimumLatitude > 90.0)) {
          std::string msg = "Projection failed. "
                        "[MinimumLatitude] of [" + std::to_string(m_minimumLatitude)
                        + "] is outside the range of [-90:90]";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }

        if ((m_maximumLatitude < -90.0) || (m_maximumLatitude > 90.0)) {
          std::string msg = "Projection failed. "
                        "[MaximumLatitude] of [" + std::to_string(m_maximumLatitude)
                        + "] is outside the range of [-90:90]";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }

        if (m_minimumLatitude >= m_maximumLatitude) {
          std::string msg = "Projection failed. "
                        "[MinimumLatitude,MaximumLatitude] of ["
                        + std::to_string(m_minimumLatitude) + ","
                        + std::to_string(m_maximumLatitude) + "] are not "
                        + "properly ordered";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }

        if (m_minimumLongitude >= m_maximumLongitude) {
          std::string msg = "Projection failed. "
                        "[MinimumLongitude,MaximumLongitude] of ["
                        + std::to_string(m_minimumLongitude) + "," 
                        + std::to_string(m_maximumLongitude) + "] are not "
                        + "properly ordered";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }

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
      if (m_equatorialRadius < m_polarRadius) {
        std::string msg = "Projection failed. Invalid keyword value(s). "
                      "[EquatorialRadius] = " + std::to_string(m_equatorialRadius)
                      + " must be greater than or equal to [PolarRadius] = "
                      + std::to_string(m_polarRadius);
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
      else {
        m_eccentricity = 1.0 -
                         (m_polarRadius * m_polarRadius) /
                         (m_equatorialRadius * m_equatorialRadius);
        m_eccentricity = sqrt(m_eccentricity);
      }

      // initialize the rest of the x,y,lat,lon member variables
      m_latitude = Null;
      m_longitude = Null;

      // If we made it to this point, we have what we need for a triaxial projection
      setProjectionType(Triaxial);
    }
    catch (IException &e) {
      std::string msg = "Projection failed.  Invalid label group [Mapping]";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
  }

  //! Destroys the TProjection object
  TProjection::~TProjection() {
  }

  /**
   * This method determines whether two map projection objects are equal by 
   * comparing the equatorial radius, polar radius, latitude type, longitude 
   * direction, resolution, and projection name. 
   *  
   * @param proj A reference to a TProjection object to which this TProjection 
   *             will be compared.
   *
   * @return bool Indicates whether the TProjection objects are equivalent.
   */
  bool TProjection::operator== (const Projection &proj) {
    if (!Projection::operator==(proj)) return false;
    TProjection *tproj = (TProjection *) &proj;
    if (EquatorialRadius() != tproj->EquatorialRadius()) return false;
    if (PolarRadius() != tproj->PolarRadius()) return false;
    if (IsPlanetocentric() != tproj->IsPlanetocentric()) return false;
    if (IsPositiveWest() != tproj->IsPositiveWest()) return false;
    return true;
  }


  /**
   * This returns the equatorial radius of the target. The radius was
   * obtained from the label during object construction.
   *
   * @return double
   */
  double TProjection::EquatorialRadius() const {
    return m_equatorialRadius;
  }

  /**
   * This returns the polar radius of the target. The radius was obtained
   * from the label during object construction.
   *
   * @return double
   */
  double TProjection::PolarRadius() const {
    return m_polarRadius;
  }

  /**
   * This returns the eccentricity of the target, 
   *  
   * @f[ 
   * e = \sqrt{1 - \frac{PR^2}{ER^2}} 
   * @f] 
   * where PR is the polar radius and ER is the equatorial radius. Since 
   * polar and equatorial radii are required to be greater than zero, it 
   * follows that @f$ 0 \le e < 1 @f$ .  Note that if the body is 
   * spherical, then PR = ER and so @a e = 0. 
   *
   * @return double
   */
  double TProjection::Eccentricity() const {
    return m_eccentricity;
  }

  /**
   * This method returns the local radius in meters at the specified latitude
   * position.  For this method, the local radius is defined as the distance 
   * from the center of the planet to the surface of the planet at the given 
   * latitude. 
   *  
   *  @f[ 
   * LR = \frac{ER*PR}{\sqrt{PR^2 \cos^2(LAT)+ER^2 \sin^2(LAT)}}
   * @f] 
   *  
   * @param latitude A latitude in degrees (assumed to be of the correct
   *                 LatitudeType).
   *  
   * @throw IException::Unknown - "The given latitude is invalid." 
   *  
   * @return double The value for the local radius, in meters, at the given 
   *                latitude.
   */
  double TProjection::LocalRadius(double latitude) const {
    if (latitude == Null) {
      throw IException(IException::Unknown, 
                       "Unable to calculate local radius. The given latitude value [" 
                       + std::to_string(latitude) + "] is invalid.", 
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

  /**
   * This method returns the local radius in meters at the current latitude
   * position. This is only usable if the use of SetGround or SetCoordinate was
   * successful.
   *
   * @return double The value for the local radius, in meters, at the current 
   *                latitude.
   */
  double TProjection::LocalRadius() const {
    return LocalRadius(m_latitude);
  }


  /**
   * This method returns the latitude of true scale. It is a virtual function 
   * and if it is not overriden the default latitude of true scale is 0 (at 
   * the equator). Otherwise it is projection specific. For example, the 
   * center latitude for Mercator, Equidistant, or a parallel for conic 
   * projections. This method is used by the Scale routine to ensure the 
   * local radius is used in the computation. 
   *
   * @return double The latitude where the projection is not distorted.
   */
  double TProjection::TrueScaleLatitude() const {
    return 0.0;
  }

  /**
   * This method returns true if the projection is
   *   equatorial cylindrical. In other words, if the
   *   projection is cylindrical and an image projected at 0 is
   *   the same as an image projected at 360.
   *
   *
   * @return bool true if the projection is equatorial cylindrical
   */
  bool TProjection::IsEquatorialCylindrical() {
    return false;
  }

  /**
   * This indicates if the latitude type is planetocentric (as opposed to
   * planetographic). The latitude type was obtained from the label during
   * object construction.
   *
   * @return bool
   */
  bool TProjection::IsPlanetocentric() const {
    return m_latitudeType == Planetocentric;
  }

  /**
   * This indicates if the latitude type is planetographic (as opposed to
   * planetocentric). The latitude type was obtained from the label during
   * object construction.
   *
   * @return bool
   */
  bool TProjection::IsPlanetographic() const {
    return m_latitudeType == Planetographic;
  }

  /**
   * This method converts a planetographic latitude to a planetocentric 
   * latitude. It utilizes the equatorial and polar radii found in the 
   * labels to perform the computation. 
   *
   * @param lat Planetographic latitude to convert.
   *  
   * @see ToPlanetocentric(lat, eRadius, pRadius) 
   *  
   * @return double The latitude, converted to planetocentric.
   */
  double TProjection::ToPlanetocentric(const double lat) const {
    return ToPlanetocentric(lat, m_equatorialRadius, m_polarRadius);
  }

  /**
   * This method converts a planetographic latitude to a planetocentric 
   * latitude.
   *
   * @param lat Planetographic latitude to convert.
   * @param eRadius Equatorial radius.
   * @param pRadius Polar radius
   *
   * @throw IException::Unknown - "The given latitude is invalid." 
   *  
   * @return double The latitude, converted to planetocentric.
   */
  double TProjection::ToPlanetocentric(const double lat,
                                      double eRadius, double pRadius) {
    if (lat == Null || abs(lat) > 90.0) {
      throw IException(IException::Unknown, 
                       "Unable to convert to Planetocentric. The given latitude value [" 
                       + std::to_string(lat) + "] is invalid.", 
                       _FILEINFO_);
    }
    double mylat = lat;
    if (abs(mylat) < 90.0) {  // So tan doesn't fail
      mylat *= PI / 180.0;
      mylat = atan(tan(mylat) * (pRadius / eRadius) *
                   (pRadius / eRadius));
      mylat *= 180.0 / PI;
    }
    return mylat;
  }

  /**
   * This method converts a planetocentric latitude to a planetographic
   * latitude. It utilizes the equatorial and polar radii found in the 
   * labels to perform the computation. 
   *
   * @param lat Planetocentric latitude to convert.
   *
   * @see ToPlanetographic(lat, eRadius, pRadius)
   *  
   * @return double The latitude, converted to planetographic.
   */
  double TProjection::ToPlanetographic(const double lat) const {
    return ToPlanetographic(lat,  m_equatorialRadius, m_polarRadius);
  }

  /**
   * This method converts a planetocentric latitude to a planetographic
   * latitude. It is static so that a projection object does not need to
   * exist.
   *
   * @param lat Planetocentric latitude to convert.
   * @param eRadius Equatorial radius.
   * @param pRadius Polar radius
   *
   * @throw IException::Unknown - "The given latitude is invalid." 
   *  
   * @return double The latitude, converted to planetographic.
   */
  double TProjection::ToPlanetographic(double lat,
                                      double eRadius, double pRadius) {
    //Account for double rounding error.
    if (qFuzzyCompare(fabs(lat), 90.0)) {
      lat = qRound(lat);
    }
    if (lat == Null || fabs(lat) > 90.0) {
      throw IException(IException::Unknown, 
                       "Unable to convert to Planetographic. The given latitude value [" 
                       + std::to_string(lat) + "] is invalid.", 
                       _FILEINFO_);
    }
    double mylat = lat;
    if (fabs(mylat) < 90.0) {  // So tan doesn't fail
      mylat *= PI / 180.0;
      mylat = atan(tan(mylat) * (eRadius / pRadius) *
                   (eRadius / pRadius));
      mylat *= 180.0 / PI;
    }
    return mylat;
  }

  /**
   * This method returns the latitude type as a string. It will return either
   * Planetocentric or Planetographic.
   *
   * @return string The latitude type, "Planetocentric" or "Planetographic".
   */
  QString TProjection::LatitudeTypeString() const {
    if (m_latitudeType == Planetographic) return "Planetographic";
    return "Planetocentric";
  }

  /**
   * This indicates if the longitude direction type is positive west (as
   * opposed to postive east). The longitude type was obtained from the 
   * label during object construction.
   *
   * @return bool
   */
  bool TProjection::IsPositiveEast() const {
    return m_longitudeDirection == PositiveEast;
  }

  /**
   * This indicates if the longitude direction type is positive east (as
   * opposed to postive west). The longitude type was obtained from the 
   * label during object construction.
   *
   * @return bool
   */
  bool TProjection::IsPositiveWest() const {
    return m_longitudeDirection == PositiveWest;
  }

  /**
   * This method converts a longitude into the positive east direction.
   *
   * @param lon Longitude to convert into the positive east direction.
   * @param domain Must be an integer value of 180 (for -180 to 180) or 360 (for 
   *            0 to 360).
   *  
   * @throw IException::Unknown - "The given longitude is invalid." 
   * @throw IException::Unknown - "Unable to convert longitude.  Domain is
   *            not 180 or 360."
   *  
   * @return double Longitude value, in positive east direction.
   */
  double TProjection::ToPositiveEast(const double lon, const int domain) {
    if (lon == Null) {
      throw IException(IException::Unknown, 
                       "Unable to convert to PositiveEast. The given longitude value [" 
                       + std::to_string(lon) + "] is invalid.", 
                       _FILEINFO_);
    }
    double mylon = lon;

    mylon *= -1;

    if (domain == 360) {
      mylon = To360Domain(mylon);
    }
    else if (domain == 180) {
      mylon = To180Domain(mylon);
    }
    else {
      std::string msg = "Unable to convert longitude.  Domain [" + std::to_string(domain) 
                    + "] is not 180 or 360.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    return mylon;
  }

  /**
   * This method converts a longitude into the positive west direction.
   *
   * @param lon Longitude to convert into the positive west direction.
   * @param domain Must be an integer value of 180 (for -180 to 180) or 360 (for 
   *            0 to 360).
   *
   * @throw IException::Unknown - "The given longitude is invalid." 
   * @throw IException::Unknown - "Unable to convert longitude.  Domain is
   *            not 180 or 360."
   *  
   * @return double Longitude value, in positive west direction.
   */
  double TProjection::ToPositiveWest(const double lon, const int domain) {
    if (lon == Null) {
      throw IException(IException::Unknown, 
                       "Unable to convert to PositiveWest. The given longitude value [" 
                       + std::to_string(lon) + "] is invalid.", 
                       _FILEINFO_);
    }
    double mylon = lon;

    mylon *= -1;

    if (domain == 360) {
      mylon = To360Domain(mylon);
    }
    else if (domain == 180) {
      mylon = To180Domain(mylon);
    }
    else {
      std::string msg = "Unable to convert longitude.  Domain [" + std::to_string(domain)
                    + "] is not 180 or 360.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    return mylon;
  }

  /**
   * This method returns the longitude direction as a string. It will return
   * either PositiveEast or PositiveWest.
   *
   * @return string The longitude direction, "PositiveEast" or 
   *         "PositiveWest".
   */
  QString TProjection::LongitudeDirectionString() const {
    if (m_longitudeDirection == PositiveEast) return "PositiveEast";
    return "PositiveWest";
  }

  /**
   * This indicates if the longitude domain is -180 to 180 (as opposed to 0
   * to 360). The longitude domain was obtained from the label during object
   * construction.
   *
   * @return bool
   */
  bool TProjection::Has180Domain() const {
    return m_longitudeDomain == 180;
  }

  /**
   * This indicates if the longitude domain is 0 to 360 (as opposed to -180
   * to 180). The longitude domain was obtained from the label during object
   * construction.
   *
   * @return bool
   */
  bool TProjection::Has360Domain() const {
    return m_longitudeDomain == 360;
  }

  /**
   * This method converts a longitude into the -180 to 180 domain. It will leave
   * the longitude unchanged if it is already in the domain.
   *
   * @param lon Longitude to convert into the -180 to 180 domain.
   *
   * @throw IException::Unknown - "The given longitude is invalid." 
   *  
   * @return double The longitude, converted to 180 domain.
   */
  double TProjection::To180Domain(const double lon) {
    if (lon == Null) {
      throw IException(IException::Unknown, 
                       "Unable to convert to 180 degree domain. The given longitude value [" 
                       + std::to_string(lon) + "] is invalid.", 
                       _FILEINFO_);
    }
    return Isis::Longitude(lon, Angle::Degrees).force180Domain().degrees();
  }

  /**
   * This method converts a longitude into the 0 to 360 domain. It will leave 
   * the longitude unchanged if it is already in the domain.
   *
   * @param lon Longitude to convert into the 0 to 360 domain.
   *
   * @return double The longitude, converted to 360 domain.
   */
  double TProjection::To360Domain(const double lon) {
    if (lon == Null) {
      throw IException(IException::Unknown, 
                       "Unable to convert to 360 degree domain. The given longitude value [" 
                       + std::to_string(lon) + "] is invalid.", 
                       _FILEINFO_);
    }
    double result = lon;

    if ( (lon < 0.0 || lon > 360.0) &&
        !qFuzzyCompare(lon, 0.0) && !qFuzzyCompare(lon, 360.0)) {
     result = Isis::Longitude(lon, Angle::Degrees).force360Domain().degrees();
    }

    return result;
  }

  /**
   * This method returns the longitude domain as a string. It will return either
   * 180 or 360.
   *
   * @return string The longitude domain, "180" or "360".
   */
  QString TProjection::LongitudeDomainString() const {
    if (m_longitudeDomain == 360) return "360";
    return "180";
  }

  /**
   * This returns the minimum latitude of the area of interest. The value 
   * was obtained from the labels during object construction. This method 
   * can only be used if HasGroundRange returns a true.
   *
   * @return double
   */
  double TProjection::MinimumLatitude() const {
    return m_minimumLatitude;
  }

  /**
   * This returns the maximum latitude of the area of interest. The value 
   * was obtained from the labels during object construction. This method 
   * can only be used if HasGroundRange returns a true.
   *
   * @return double
   */
  double TProjection::MaximumLatitude() const {
    return m_maximumLatitude;
  }

  /**
   * This returns the minimum longitude of the area of interest. The value
   * was obtained from the labels during object construction. This method 
   * can only be used if HasGroundRange returns a true.
   *
   * @return double
   */
  double TProjection::MinimumLongitude() const {
    return m_minimumLongitude;
  }

  /**
   * This returns the maximum longitude of the area of interest. The value
   * was obtained from the labels during object construction. This method 
   * can only be used if HasGroundRange returns a true.
   *
   * @return double
   */
  double TProjection::MaximumLongitude() const {
    return m_maximumLongitude;
  }

  /**
   * This method is used to set the latitude/longitude (assumed to be of the
   * correct LatitudeType, LongitudeDirection, and LongitudeDomain. The Set
   * forces an attempted calculation of the projection X/Y values. This may or
   * may not be successful and a status is returned as such. Usually this method
   * is overridden in a dervied class, for example, Sinusoidal. If not the
   * default method simply copies lat/lon to x/y which is no projection.
   *
   * @param lat Latitude value to project
   * @param lon Longitude value to project
   *
   * @return bool Indicates whether the method was successful.
   */
  bool TProjection::SetGround(const double lat, const double lon) {
    if (lat == Null || lon == Null) {
      m_good = false;
      return m_good;
    }
    else {
      m_latitude = lat;
      m_longitude = lon;
      m_good = true;
      SetComputedXY(lon, lat);
    }
    return m_good;
  }

  /**
   * This method is used to set the projection x/y. The Set forces an attempted
   * calculation of the corresponding latitude/longitude position. This may or
   * may not be successful and a status is returned as such.  Usually this 
   * method is overridden in a dervied class, for example, Sinusoidal. If not 
   * the default method simply copies x/y to  lat/lon to x/y which is no 
   * projection. 
   *
   * @param x X coordinate of the projection in units that are the same as the
   *          radii in the label
   * @param y Y coordinate of the projection in units that are the same as the
   *          radii in the label
   *
   * @return bool Indicates whether the method was successful.
   */
  bool TProjection::SetCoordinate(const double x, const double y) {
    if (x == Null || y == Null) {
      m_good = false;
    }
    else {
      m_good = true;
      SetXY(x, y);
      m_latitude = XCoord();
      m_longitude = YCoord();
    }
    return m_good;
  }


  /**
   * This returns a latitude with correct latitude type as specified in the
   * label object. The method can only be used if SetGround, SetCoordinate,
   * SetUniversalGround, or SetWorld return with success. Success can also
   * be checked using the IsGood method.
   *
   * @return double
   */
  double TProjection::Latitude() const {
    return m_latitude;
  }

  /**
   * This returns a longitude with correct longitude direction and domain as
   * specified in the label object. The method can only be used if 
   * SetGround, SetCoordinate, SetUniversalGround, or SetWorld return with 
   * success. Success can also be checked using the IsGood method. 
   *
   * @return double
   */
  double TProjection::Longitude() const {
    return m_longitude;
  }


  /**
   * This method is used to set the latitude/longitude which must be
   * Planetocentric (latitude) and PositiveEast/Domain360 (longitude). The Set
   * forces an attempted calculation of the projection X/Y values. This may or
   * may not be successful and a status is returned as such.
   *
   * @param lat Planetocentric Latitude value to project
   * @param lon PositiveEast, Domain360 Longitude value to project
   *
   * @return bool Indicates whether the method was successful.
   */
  bool TProjection::SetUniversalGround(const double lat, const double lon) {
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


  /**
   * This method is used to set the latitude/longitude. The Set
   * forces an attempted calculation of the projection X/Y values. This may or
   * may not be successful and a status is returned as such. This version does
   * not adjust the longitude based on the longitude domain.
   *
   * @param lat Planetocentric Latitude value to project
   * @param lon PositiveEast, Domain360 Longitude value to project
   *
   * @return bool Indicates whether the method was successful.
   */
  bool TProjection::SetUnboundUniversalGround(const double lat, const double lon) {
    if (lat == Null || lon == Null) {
      m_good = false;
      return m_good;
    }
    // Deal with the longitude first
    m_longitude = lon;
    if (m_longitudeDirection == PositiveWest) m_longitude = -lon;

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


  /**
   * This returns a universal latitude (planetocentric). The method can only be
   * used if SetGround, SetCoordinate, SetUniversalGround, or SetWorld return
   * with success. Success can also be checked using the IsGood method.
   *
   * @return double The universal latitude.
   */
  double TProjection::UniversalLatitude() {
    double lat = m_latitude;
    if (m_latitudeType == Planetographic) lat = ToPlanetocentric(lat);
    return lat;
  }

  /**
   * This returns a universal longitude (positive east in 0 to 360 domain). The
   * method can only be used if SetGround, SetCoordinate, SetUniversalGround, or
   * SetWorld return with success. Success can also be checked using the IsGood
   * method.
   *
   * @return double The universal longitude.
   */
  double TProjection::UniversalLongitude() {
    double lon = m_longitude;
    if (m_longitudeDirection == PositiveWest) lon = -lon;
    lon = To360Domain(lon);
    return lon;
  }


  /**
   * This method returns the scale for mapping world coordinates into projection
   * coordinates. For example, if the world coordinate system is an image then
   * this routine returns the number of pixels per degree. Likewise, if the
   * world coordinate system is a piece of paper, it might return the number of
   * inches of paper per degree. If the SetWorldMapper method is not invoked
   * then this method returns 1.0
   *
   * @return double The scale for mapping.
   */
  double TProjection::Scale() const {
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


  /**
   * This method is used to determine the x/y range which completely covers the
   * area of interest specified by the lat/lon range. The latitude/longitude
   * range may be obtained from the labels. This method should not be used if
   * HasGroundRange is false. The purpose of this method is to return the x/y
   * range so it can be used to compute how large a map may need to be. For
   * example, how big a piece of paper is needed or how large of an image needs
   * to be created. This is method and therefore must be written by the derived
   * class (e.g., Sinusoidal). The method may fail as indicated by its return
   * value.
   *
   * 
   * @param &minX Reference to the address where the minimum x 
   *             coordinate value will be written.  The Minimum x projection
   *             coordinate calculated by this method covers the
   *             latitude/longitude range specified in the labels.
   *
   * @param &maxX Reference to the address where the maximum x 
   *             coordinate value will be written.  The Maximum x projection
   *             coordinate calculated by this method covers the
   *             latitude/longitude range specified in the labels.
   *
   * @param &minY Reference to the address where the minimum y 
   *             coordinate value will be written.  The Minimum y projection
   *             coordinate calculated by this method covers the
   *             latitude/longitude range specified in the labels.
   *
   * @param &maxY Reference to the address where the maximum y 
   *             coordinate value will be written.  The Maximum y projection
   *             coordinate calculated by this method covers the
   *             latitude/longitude range specified in the labels.
   * 
   * @return bool Indicates whether the method was able to determine the X/Y 
   *              Range of the projection.  If yes, minX, maxX, minY, maxY will
   *              be set with these values.
   *
   */
  bool TProjection::XYRange(double &minX, double &maxX, 
                           double &minY, double &maxY) {
    if (minX == Null || maxX == Null || minY == Null || maxY == Null) {
      return false;
    }
    if (m_groundRangeGood) {
      minX = m_minimumLongitude;
      maxX = m_maximumLongitude;
      minY = m_minimumLatitude;
      maxY = m_maximumLatitude;
      return true;
    }
    return false;
  }

  /**
   * This convience function is established to assist in the development of the
   * XYRange virtual method. It allows the developer to test ground points
   * (lat/lon) to see if they produce a minimum/maximum projection coordinate.
   *  
   * This method will first verify that the given latitude and longitude values 
   * are not Null. If so, the method will change the status of the object to 
   * good=false before returning (i.e IsGood() returns false). 
   *  
   * If both of those test pass, it attempts to set the ground to the given 
   * latitude and longitude and, if successful, compares the new x and y with 
   * the saved min/max x and y values. 
   *  
   * For example in Sinusoidal this method is called in the following way: 
   *    @code
   *       bool Sinusoidal::XYRange(double &minX, double &maxX,
   *                                double &minY, double &maxY) {
   *  
   *         // Check the corners of the lat/lon range
   *         XYRangeCheck (m_minimumLatitude,m_minimumLongitude);
   *         XYRangeCheck (m_maximumLatitude,m_minimumLongitude);
   *         XYRangeCheck (m_minimumLatitude,m_maximumLongitude);
   *         XYRangeCheck (m_maximumLatitude,m_maximumLongitude);
   *
   *         // In case the latitude crosses the equator, check there
   *         if (inLatitudeRange(0.0)) {
   *           XYRangeCheck (0.0,m_minimumLongitude);
   *           XYRangeCheck (0.0,m_maximumLongitude);
   *         }
   *  
   *         // Make sure everything is ordered
   *         if (m_minimumX >= m_maximumX) return false;
   *         if (m_minimumY >= m_maximumY) return false;
   *
   *         // Return X/Y min/maxs
   *         minX = m_minimumX;
   *         maxX = m_maximumX;
   *         minY = m_minimumY;
   *         maxY = m_maximumY;
   *         return true;
   *      }
   *    @endcode
   *  
   * Note: It is a good habit to call the inLatitudeRange and inLongitudeRange 
   * functions to verify that the values are in range before passing it to 
   * XYRangeCheck. 
   *  
   * @see inLongitudeRange(minLon, maxLon, longitude) 
   * @see inLongitudeRange(longitude) 
   * @see inLatitudeRange(latitude) 
   *  
   * @param latitude Test for min/max projection coordinates at this latitude
   * @param longitude Test for min/max projection coordinates at this longitude
   */
  void TProjection::XYRangeCheck(const double latitude, const double longitude) {

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


  /**
   * Determine whether the given longitude is within the range of the given min 
   * and max longitudes. 
   *  
   * Note: It is a good habit to call this function to verify that a value is 
   * in range before passing it to XYRangeCheck. 
   *  
   * @see inLongitudeRange(longitude) 
   * @see inLatitudeRange(latitude) 
   * @see XYRangeCheck(latitude, longitude) 
   *  
   * @param minLon The lower end of longitude range.
   * @param maxLon The upper end of longitude range.
   * @param longitude The longitude to check.
   * 
   * @return @b bool Indicates whether minLon <= longitude <= maxLon.
   */
  bool TProjection::inLongitudeRange(double minLon, 
                                     double maxLon, 
                                     double longitude) {
    // get the min/max range closest to 0.0 lon
    double adjustedLon =    To360Domain(longitude);
    double adjustedMinLon = To360Domain(minLon);
    double adjustedMaxLon = To360Domain(maxLon); 

    if (adjustedMinLon > adjustedMaxLon) {
      if (adjustedLon > adjustedMinLon) {
        adjustedLon -= 360;
      }
      adjustedMinLon -= 360;
    }

    // if this range covers all longitudes, then the given longitude is clearly in range
    if (qFuzzyCompare(maxLon - minLon, 360.0)) {
      return true;
    }
    else if (adjustedMinLon <= adjustedLon && adjustedLon <= adjustedMaxLon) { 
      return true;
    }
    else {
      return false;
    }
  }


  /**
   * Determine whether the given longitude is within the range of the 
   * MinimumLongitude and MaximumLongitude range of this projection. 
   *  
   * Note: It is a good habit to call this function to verify that a value is 
   * in range before passing it to XYRangeCheck. 
   *  
   * @see inLongitudeRange(minLon, maxLon, longitude) 
   * @see inLatitudeRange(latitude) 
   * @see XYRangeCheck(latitude, longitude) 
   *  
   * @param longitude The longitude to check.
   * 
   * @return @b bool Indicates whether MinimumLongitude <= longitude <= MaximumLongitude.
   */
  bool TProjection::inLongitudeRange(double longitude) {
    return inLongitudeRange(MinimumLongitude(), MaximumLongitude(), longitude);
  }


  /**
   * Determine whether the given latitude is within the range of the 
   * MinimumLatitude and MaximumLatitude range of this projection. 
   *  
   * Note: It is a good habit to call this function to verify that a value is 
   * in range before passing it to XYRangeCheck. 
   *  
   * @see inLongitudeRange(minLon, maxLon, longitude) 
   * @see inLongitudeRange(longitude) 
   * @see XYRangeCheck(latitude, longitude) 
   *  
   * @param latitude The latitude to check.
   * 
   * @return @b bool Indicates whether MinimumLatitude <= latitude <= MaximumLatitude.
   */
  bool TProjection::inLatitudeRange(double latitude) {
    if (MaximumLatitude() - MinimumLatitude() == 180) {
      return true;
    }
    else if (MinimumLatitude() <= latitude && latitude <= MaximumLatitude()) { 
      return true;
    }
    else {
      return false;
    }
  }


  /** 
   * This method is used to find the XY range for oblique aspect projections 
   * (non-polar projections) by "walking" around each of the min/max lat/lon. 
   *  
   * @param minX Minimum x projection coordinate which covers the latitude
   *             longitude range specified in the labels.
   * @param maxX Maximum x projection coordinate which covers the latitude
   *             longitude range specified in the labels.
   * @param minY Minimum y projection coordinate which covers the latitude
   *             longitude range specified in the labels.
   * @param maxY Maximum y projection coordinate which covers the latitude
   *             longitude range specified in the labels.
   * 
   * @return @b bool Indicates whether the method was successful.
   * @see XYRange()
   * @author Stephen Lambright 
   * @internal
   *   @history 2011-07-02 Jeannie Backer - Moved this code from
   *                           ObliqueCylindrical class to its own method here.
   *   @history 2012-11-30 Debbie A. Cook - Changed to use TProjection instead of Projection.
   *                           References #775.
   */
  bool TProjection::xyRangeOblique(double &minX, double &maxX, 
                                  double &minY, double &maxY) {
    if (minX == Null || maxX == Null || minY == Null || maxY == Null) {
      return false;
    }
    //For oblique, we'll have to walk all 4 sides to find out min/max x/y values
    if (!HasGroundRange()) return false; // Don't have min/max lat/lon, 
                                        //can't continue

    m_specialLatCases.clear();
    m_specialLonCases.clear();

    // First, search longitude for min X/Y
    double minFoundX1, minFoundX2;
    double minFoundY1, minFoundY2;

    // Search for minX between minlat and maxlat along minlon
    doSearch(MinimumLatitude(), MaximumLatitude(), 
             minFoundX1, MinimumLongitude(), true, true, true);
    // Search for minX between minlat and maxlat along maxlon
    doSearch(MinimumLatitude(), MaximumLatitude(), 
             minFoundX2, MaximumLongitude(), true, true, true);
    // Search for minY between minlat and maxlat along minlon
    doSearch(MinimumLatitude(), MaximumLatitude(), 
             minFoundY1, MinimumLongitude(), false, true, true);
    // Search for minY between minlat and maxlat along maxlon
    doSearch(MinimumLatitude(), MaximumLatitude(), 
             minFoundY2, MaximumLongitude(), false, true, true);

    // Second, search latitude for min X/Y
    double minFoundX3, minFoundX4;
    double minFoundY3, minFoundY4;

    // Search for minX between minlon and maxlon along minlat
    doSearch(MinimumLongitude(), MaximumLongitude(), 
             minFoundX3, MinimumLatitude(), true, false, true);
    // Search for minX between minlon and maxlon along maxlat
    doSearch(MinimumLongitude(), MaximumLongitude(), 
             minFoundX4, MaximumLatitude(), true, false, true);
    // Search for minY between minlon and maxlon along minlat
    doSearch(MinimumLongitude(), MaximumLongitude(), 
             minFoundY3, MinimumLatitude(), false, false, true);
    // Search for minY between minlon and maxlon along maxlat
    doSearch(MinimumLongitude(), MaximumLongitude(), 
             minFoundY4, MaximumLatitude(), false, false, true);

    // We've searched all possible minimums, go ahead and store the lowest
    double minFoundX5 = min(minFoundX1, minFoundX2);
    double minFoundX6 = min(minFoundX3, minFoundX4);
    m_minimumX = min(minFoundX5, minFoundX6);

    double minFoundY5 = min(minFoundY1, minFoundY2);
    double minFoundY6 = min(minFoundY3, minFoundY4);
    m_minimumY = min(minFoundY5, minFoundY6);

    // Search longitude for max X/Y
    double maxFoundX1, maxFoundX2;
    double maxFoundY1, maxFoundY2;

    // Search for maxX between minlat and maxlat along minlon
    doSearch(MinimumLatitude(), MaximumLatitude(), 
             maxFoundX1, MinimumLongitude(), true, true, false);
    // Search for maxX between minlat and maxlat along maxlon
    doSearch(MinimumLatitude(), MaximumLatitude(), 
             maxFoundX2, MaximumLongitude(), true, true, false);
    // Search for maxY between minlat and maxlat along minlon
    doSearch(MinimumLatitude(), MaximumLatitude(), 
             maxFoundY1, MinimumLongitude(), false, true, false);
    // Search for maxY between minlat and maxlat along maxlon
    doSearch(MinimumLatitude(), MaximumLatitude(), 
             maxFoundY2, MaximumLongitude(), false, true, false);

    // Search latitude for max X/Y
    double maxFoundX3, maxFoundX4;
    double maxFoundY3, maxFoundY4;

    // Search for maxX between minlon and maxlon along minlat
    doSearch(MinimumLongitude(), MaximumLongitude(), 
             maxFoundX3, MinimumLatitude(), true, false, false);
    // Search for maxX between minlon and maxlon along maxlat
    doSearch(MinimumLongitude(), MaximumLongitude(), 
             maxFoundX4, MaximumLatitude(), true, false, false);
    // Search for maxY between minlon and maxlon along minlat
    doSearch(MinimumLongitude(), MaximumLongitude(), 
             maxFoundY3, MinimumLatitude(), false, false, false);
    // Search for maxY between minlon and maxlon along maxlat
    doSearch(MinimumLongitude(), MaximumLongitude(), 
             maxFoundY4, MaximumLatitude(), false, false, false);

    // We've searched all possible maximums, go ahead and store the highest
    double maxFoundX5 = max(maxFoundX1, maxFoundX2);
    double maxFoundX6 = max(maxFoundX3, maxFoundX4);
    m_maximumX = max(maxFoundX5, maxFoundX6);

    double maxFoundY5 = max(maxFoundY1, maxFoundY2);
    double maxFoundY6 = max(maxFoundY3, maxFoundY4);
    m_maximumY = max(maxFoundY5, maxFoundY6);

    // Look along discontinuities for more extremes
    vector<double> specialLatCases = m_specialLatCases;
    for (unsigned int specialLatCase = 0; 
        specialLatCase < specialLatCases.size(); 
        specialLatCase ++) {
      double minX, maxX, minY, maxY;

      // Search for minX between minlon and maxlon along latitude discontinuities
      doSearch(MinimumLongitude(), MaximumLongitude(), 
               minX, specialLatCases[specialLatCase], true,  false, true);
      // Search for minY between minlon and maxlon along latitude discontinuities
      doSearch(MinimumLongitude(), MaximumLongitude(), 
               minY, specialLatCases[specialLatCase], false, false, true);
      // Search for maxX between minlon and maxlon along latitude discontinuities
      doSearch(MinimumLongitude(), MaximumLongitude(), 
               maxX, specialLatCases[specialLatCase], true,  false, false);
      // Search for maxX between minlon and maxlon along latitude discontinuities
      doSearch(MinimumLongitude(), MaximumLongitude(), 
               maxY, specialLatCases[specialLatCase], false, false, false);

      m_minimumX = min(minX, m_minimumX);
      m_maximumX = max(maxX, m_maximumX);
      m_minimumY = min(minY, m_minimumY);
      m_maximumY = max(maxY, m_maximumY);
    }

    vector<double> specialLonCases = m_specialLonCases;
    for (unsigned int specialLonCase = 0; 
        specialLonCase < specialLonCases.size(); 
        specialLonCase ++) {
      double minX, maxX, minY, maxY;

      // Search for minX between minlat and maxlat along longitude discontinuities
      doSearch(MinimumLatitude(), MaximumLatitude(), 
               minX, specialLonCases[specialLonCase], true,  true, true);
      // Search for minY between minlat and maxlat along longitude discontinuities
      doSearch(MinimumLatitude(), MaximumLatitude(), 
               minY, specialLonCases[specialLonCase], false, true, true);
      // Search for maxX between minlat and maxlat along longitude discontinuities
      doSearch(MinimumLatitude(), MaximumLatitude(), 
               maxX, specialLonCases[specialLonCase], true,  true, false);
      // Search for maxY between minlat and maxlat along longitude discontinuities
      doSearch(MinimumLatitude(), MaximumLatitude(), 
               maxY, specialLonCases[specialLonCase], false, true, false);

      m_minimumX = min(minX, m_minimumX);
      m_maximumX = max(maxX, m_maximumX);
      m_minimumY = min(minY, m_minimumY);
      m_maximumY = max(maxY, m_maximumY);
    }

    m_specialLatCases.clear();
    m_specialLonCases.clear();

    // Make sure everything is ordered
    if (m_minimumX >= m_maximumX) return false;
    if (m_minimumY >= m_maximumY) return false;

    // Return X/Y min/maxs
    minX = m_minimumX;
    maxX = m_maximumX;
    minY = m_minimumY;
    maxY = m_maximumY;

    return true;
  }

  /**
   * This method searches for extreme (min/max/discontinuity) coordinate values
   * along the constBorder line between minBorder and maxBorder (that is, 
   * across latitudes/longitudes). This method locates the extrema by utilizing
   * the findExtreme() method until the coordinate values converge. Then, 
   * extremeVal parameter is updated with this value before returning. 
   *  
   * Discontinuities are stored in m_specialLatCases and m_specialLonCases so 
   * they may be checked again later, which creates significantly more accuracy 
   * in some cases. 
   *  
   * @param minBorder Minimum latitude or longitude to search between.
   * @param maxBorder Maximum latitude or longitude to search between.
   * @param extremeVal The resulting global coordinate value (min or max 
   *            value for x or y, depending on findMin and searchX) on the
   *            constBorder.
   * @param constBorder The latitude or longitude that remains constant.  The 
   *            method will step along this border.
   * @param searchX Indicates whether the method is searching for a min or max 
   *            x-coordinate.  If false the method searches for min or max
   *            y-coordinate.
   * @param searchLongitude Indicates whether the method will search 
   *            along a longitude. If true, constBorder is longitude and all
   *            other borders are latitudes. If false, the method searches a
   *            latitude (constBorder is a lat, other borders lons).
   * @param findMin Indicates whether the method is looking for a minimum 
   *            coordinate value. If false, the method is looking for a maximum
   *            value.
   * @author Steven Lambright 
   * @internal
   *   @history 2011-07-02 Jeannie Backer - Moved this code from
   *                           ObliqueCylindrical class to its own method
   *                           here. Added condition to do-while loop for
   *                           more precision. Replaced hard-coded
   *                           TOLERANCE with 1/2 pixel resolution.
   *                           (Defaults to 0.5)
   */
  void TProjection::doSearch(double minBorder, double maxBorder, 
                            double &extremeVal, const double constBorder, 
                            bool searchX, bool searchLongitude, bool findMin) {
    if (minBorder == Null || maxBorder == Null || constBorder == Null) {
      return;
    }
    const double TOLERANCE = PixelResolution()/2;
    const int NUM_ATTEMPTS = (unsigned int)DBL_DIG; // It's unsafe to go past 
                                                    // this precision

    double minBorderX, minBorderY, maxBorderX, maxBorderY;
    int attempts = 0;

    do {
      findExtreme(minBorder, maxBorder, minBorderX, minBorderY, maxBorderX, 
                  maxBorderY, constBorder, searchX, searchLongitude, findMin);
      if (minBorderX == Null && maxBorderX == Null 
          && minBorderY == Null && maxBorderY == Null ) {
        attempts = NUM_ATTEMPTS;
        continue;
      }
      attempts ++;
    }
    while ((fabs(minBorderX - maxBorderX) > TOLERANCE 
           || fabs(minBorderY - maxBorderY) > TOLERANCE)
           && (attempts < NUM_ATTEMPTS)); 
    // check both x and y distance in case symmetry of map
    // For example, if minBorderX = maxBorderX but minBorderY = -maxBorderY,
    // these points may not be close enough.

    if (attempts >= NUM_ATTEMPTS) {
      // We zoomed in on a discontinuity because our range never shrank, this
      // will need to be rechecked later. 
      // *min and max border should be nearly identical, so it doesn't matter
      //  which is used here
      if (searchLongitude) {
        m_specialLatCases.push_back(minBorder);
      }
      else {
        m_specialLonCases.push_back(minBorder);
      }
    }

    // These values will always be accurate, even over a discontinuity
    if (findMin) {
      if (searchX) extremeVal = min(minBorderX, maxBorderX);
      else         extremeVal = min(minBorderY, maxBorderY);
    }
    else {
      if (searchX) extremeVal = max(minBorderX, maxBorderX);
      else         extremeVal = max(minBorderY, maxBorderY);
    }
    return;
  }

  /**
   * Searches for extreme (min/max/discontinuity) coordinate values across 
   * latitudes/longitudes. 
   *  
   * This method looks for these extrema along the constBorder between minBorder 
   * and maxBorder by stepping along constBorder (10 times) from the minBorder 
   * and maxBorder. Then, the range of this extreme value is recorded in 
   * minBorder and maxBorder and the coordinate values corresponding to these 
   * new borders are stored in minBorderX, minBorderY, maxBorderX and 
   * maxBorderY. 
   *  
   * This function should be used by calling it repeatedly until minBorderX and 
   * minBorderY do not equal maxBorderX and maxBorderY, respectively. 
   * Discontinuities will cause the minBorderX, minBorderY, maxBorderX and 
   * maxBorderY to never converge. If minBorderX never comes close to maxBorderX 
   * or minBorderY never comes close to maxBorderY, then between minBorder and 
   * maxBorder is the value of the most extreme value. In this case, either the 
   * smaller or larger of the x or y values found will be correct, depending on 
   * the values of findMin and searchX. 
   *  
   *  
   * 
   *
   * @param minBorder Minimum latitude or longitude to search between. This 
   *            value gets updated to a more precise range.
   * @param maxBorder Maximum latitude or longitude to search between. This 
   *            value gets updated to a more precise range.
   * @param minBorderX The x-value corresponding to the lower resultant 
   *            minBorder and the constBorder, which is more accurate when
   *            nearly equal to maxBorderX.
   * @param minBorderY The y-value corresponding to the lower resultant 
   *            minBorder and the constBorder, which is more accurate when
   *            nearly equal to maxBorderY.
   * @param maxBorderX The x-value corresponding to the higher resultant 
   *            maxBorder and the constBorder, which is more accurate when
   *            nearly equal to minBorderX.
   * @param maxBorderY The y-value corresponding to the higher resultant 
   *            maxBorder and the constBorder, which is more accurate when
   *            nearly equal to minBorderY.
   * @param constBorder The latitude or longitude that remains constant.  The 
   *            method will step along this border.
   * @param searchX Indicates whether the method is searching for a min or max 
   *            x-coordinate.  If false the method searches for min or max
   *            y-coordinate.
   * @param searchLongitude Indicates whether the method will search 
   *            along a longitude. If true, constBorder is longitude and all
   *            other borders are latitudes. If false, the method searches a
   *            latitude (constBorder is a lat, other borders lons).
   * @param findMin Indicates whether the method is looking for a minimum 
   *            coordinate value. If false, the method is looking for a maximum
   *            value.
   * @author Stephen Lambright 
   * @internal
   *   @history 2011-07-02 Jeannie Backer - Moved this code from
   *                           ObliqueCylindrical class to its own method here.
   *                           Replaced parameters minVal and maxVal with
   *                           minBorderX, minBorderY, maxBorderX, and
   *                           maxBorderY.
   */
  void TProjection::findExtreme(double &minBorder,  double &maxBorder,
                               double &minBorderX, double &minBorderY,
                               double &maxBorderX, double &maxBorderY, 
                               const double constBorder, bool searchX, 
                               bool searchLongitude, bool findMin) {
    if (minBorder == Null || maxBorder == Null || constBorder == Null) {
      minBorderX = Null;
      minBorderY = minBorderX;
      minBorderY = minBorderX;
      return;
    }
    if (!searchLongitude && (fabs(fabs(constBorder) - 90.0) < DBL_EPSILON)) {
      // it is impossible to search "along" a pole
      setSearchGround(minBorder, constBorder, searchLongitude);
      minBorderX = XCoord();
      minBorderY = YCoord();
      maxBorderY = minBorderY;
      return;
    }
    // Always do 10 steps
    const double STEP_SIZE = (maxBorder - minBorder) / 10.0;
    const double LOOP_END = maxBorder + (STEP_SIZE / 2.0); // This ensures we do 
                                                           // all of the steps
                                                           // properly
    double currBorderVal = minBorder;
    setSearchGround(minBorder, constBorder, searchLongitude);

    // this makes sure that the initial currBorderVal is valid before entering
    // the loop below
    if (!m_good){
      // minBorder = currBorderVal+STEP_SIZE < LOOP_END until setGround is good?
      // then, if still not good return?
      while (!m_good && currBorderVal <= LOOP_END) {
        currBorderVal+=STEP_SIZE;
        if (searchLongitude && (currBorderVal - 90.0 > DBL_EPSILON)) {
          currBorderVal = 90.0;
        }
        setSearchGround(currBorderVal, constBorder, searchLongitude);
      }
      if (!m_good) {
        minBorderX = Null;
        minBorderY = Null;
        return;
      }
    }

    // save the values of three consecutive steps from the minBorder towards
    // the maxBorder along the constBorder. initialize these three border
    // values (the non-constant lat or lon)
    double border1 = currBorderVal;
    double border2 = currBorderVal;
    double border3 = currBorderVal;

    // save the coordinate (x or y) values that correspond to the first
    // two borders that are being saved.
    // initialize these two coordinate values (x or y)
    double value1 = (searchX) ? XCoord() : YCoord();
    double value2 = value1;

    // initialize the extreme coordinate value 
    // -- this is the largest coordinate value found so far
    double extremeVal2 = value2;

    // initialize the extreme border values
    // -- these are the borders on either side of the extreme coordinate value
    double extremeBorder1 = minBorder;
    double extremeBorder3 = minBorder;

    while (currBorderVal <= LOOP_END) {

      // this conditional was added to prevent trying to SetGround with an
      // invalid latitude greater than 90 degrees. There is no need check for
      // latitude less than -90 since we start at the minBorder (already
      // assumed to be valid) and step forward toward (and possibly past)
      // maxBorder
      if (searchLongitude && (currBorderVal - 90.0 > DBL_EPSILON)) {
        currBorderVal = 90.0;
      }

      // update the current border value along constBorder
      currBorderVal += STEP_SIZE;
      setSearchGround(currBorderVal, constBorder, searchLongitude);
      if (!m_good){ 
        continue;
      } 
                     
      // update the border and coordinate values 
      border3 = border2;
      border2 = border1;
      border1 = currBorderVal;                                             
      value2 = value1;
      value1 = (searchX) ? XCoord() : YCoord();

      if ((findMin && value2 < extremeVal2) 
          || (!findMin && value2 > extremeVal2)) {
        // Compare the coordinate value associated with the center border with 
        // the current extreme. If the updated coordinate value is more extreme
        // (smaller or larger, depending on findMin), then we update the
        // extremeVal and it's borders.
        extremeVal2 = value2;

        extremeBorder3 = border3;
        extremeBorder1 = border1;
      }
    }

    // update min/max border values to the values on either side of the most 
    // extreme coordinate found in this call to this method
    
    minBorder = extremeBorder3; // Border 3 is lagging and thus smaller

    // since the loop steps past the original maxBorder, we want to retain 
    // the original maxBorder value so we don't go outside of the original
    // min/max range given
    if (extremeBorder1 <= maxBorder ) {
      maxBorder = extremeBorder1; // Border 1 is leading and thus larger
    }

    // update minBorder coordinate values
    setSearchGround(minBorder, constBorder, searchLongitude);
    // if (!m_good){
    //   this should not happen since minBorder has already been verified in 
    //   the while loop above
    // }

    minBorderX = XCoord();
    minBorderY = YCoord();

    // update maxBorder coordinate values
    setSearchGround(maxBorder, constBorder, searchLongitude);
    // if (!m_good){
    //   this should not happen since maxBorder has already been verified in
    //   the while loop above
    // }

    maxBorderX = XCoord();
    maxBorderY = YCoord();
    return;
  }

  /**
   * This function sets the ground for the given border values.  It calls the 
   * SetGround(lat, lon) method with the appropriate lat/lon values, depending 
   * on whether variableIsLat is true. 
   *  
   * This method is used by doSearch and findExtreme in order to set the ground 
   * correctly each time. 
   *
   * @param variableBorder The latitude or longitude that is variable in the 
   *            search methods.
   * @param constBorder The latitude or longitude that is constant in the search 
   *            methods.
   * @param variableIsLat Indicates whether variableBorder is the latittude 
   *            value and constBorder is the longitude value. If false,
   *            variableBorder is the longitude value and constBorder is the
   *            latitude value.
   * @author Stephen Lambright 
   * @internal
   *   @history 2011-07-02 Jeannie Backer - Moved this code from
   *                           ObliqueCylindrical class to its own method here.
   *                           Added error.
   */
  void TProjection::setSearchGround(const double variableBorder, 
                                   const double constBorder, 
                                   bool variableIsLat) {
    if (variableBorder == Null || constBorder == Null) {
      return;
    }
    double lat, lon;
    if (variableIsLat) {
      lat = variableBorder;
      lon = constBorder;
    }
    else {
      lat = constBorder;
      lon = variableBorder;
    }
    SetGround(lat, lon);
    return;
  }


  /**
   * This function returns the keywords that this projection uses.
   *
   * @return PvlGroup The keywords that this projection uses
   */
  PvlGroup TProjection::Mapping() {
    PvlGroup mapping("Mapping");

    QStringList keyNames;
    keyNames << "TargetName" << "ProjectionName" << "EquatorialRadius" << "PolarRadius"
             << "LatitudeType" << "LongitudeDirection" << "LongitudeDomain"
             << "PixelResolution" << "Scale" << "UpperLeftCornerX" << "UpperLeftCornerY"
             << "MinimumLatitude" << "MaximumLatitude" << "MinimumLongitude" << "MaximumLongitude"
             << "Rotation";

    foreach (QString keyName, keyNames) {
      if (m_mappingGrp.hasKeyword(keyName.toStdString())) {
        mapping += m_mappingGrp[keyName.toStdString()];
      }
    }

    return mapping;
  }


  /**
   * This function returns the latitude keywords that this projection uses
   *
   * @return PvlGroup The latitude keywords that this projection uses
   */
  PvlGroup TProjection::MappingLatitudes() {
    PvlGroup mapping("Mapping");

    if (HasGroundRange()) {
      mapping += m_mappingGrp["MinimumLatitude"];
      mapping += m_mappingGrp["MaximumLatitude"];
    }

    return mapping;
  }

  /**
   * This function returns the longitude keywords that this projection uses
   *
   * @return PvlGroup The longitude keywords that this projection uses
   */
  PvlGroup TProjection::MappingLongitudes() {
    PvlGroup mapping("Mapping");

    if (HasGroundRange()) {
      mapping += m_mappingGrp["MinimumLongitude"];
      mapping += m_mappingGrp["MaximumLongitude"];
    }

    return mapping;
  }

  /**
   * A convience method to compute Snyder's @a q equation (3-12) for a given 
   * latitude, @f$\phi@f$ 
   *  
   * @f[ 
   * q = (1 - e^2) \left[ \frac{\sin(\phi)}{1 - e^2 
   * \sin^2(\phi)} 
   * - 
   * \frac{1}{2e} \ln\left(\frac{1 - e \sin(\phi)}{1 + e 
   *      \sin(\phi)}\right) \right]
   * @f] 
   * where @f$e@f$ is the eccentricity for the body. 
   * 
   * @param sinPhi The sine value for a latitude, phi. 
   *  
   * @throw IException::Unknown - "Snyder's q variable should only be 
   *            computed for ellipsoidal projections."
   *  
   * @return @b double Value for Snyder's q variable. 
   */
  double TProjection::qCompute(const double sinPhi) const {
    if (m_eccentricity < DBL_EPSILON) {
      QString msg = "Snyder's q variable should only be computed for "
                    "ellipsoidal projections.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    double eSinPhi = m_eccentricity * sinPhi;
    return (1 - m_eccentricity * m_eccentricity)
           * (sinPhi / (1 - eSinPhi * eSinPhi) 
               - 1 / (2 * m_eccentricity) * log( (1 - eSinPhi) / (1 + eSinPhi) ));
           // Note: We know that q is well defined since 
           //   0 < e < 1 and -1 <= sin(phi) <= 1 
           //   implies that -1 < e*sin(phi) < 1
           //   Thus, there are no 0 denominators and the log domain is 
           //   satisfied, (1-e*sin(phi))/(1+e*sin(phi)) > 0
  }

  /**
   * A convience method to compute latitude angle phi2 given small t, from 
   * Syder's recursive equation (7-9)
   *  
   * @f[ 
   * \phi_{i+1} = \frac{\pi}{2} - 2\arctan\left(t 
   *      \left[\frac{1-e\sin(\phi_i)}{1+e\sin(\phi_i)}\right]^{e/2}\right)
   * @f]
   * where @f$e@f$ is the eccentricity for the body and @f$ \phi_0 = 
   * \frac{\pi}{2} - 2\arctan(t) @f$ . 
   *  
   * @param t small t
   *  
   * @throw IException::Unknown - "Failed to converge in Projection::phi2Compute()"
   * @return double The value for the latitude.
   */
  double TProjection::phi2Compute(const double t) const {
    double localPhi = HALFPI - 2.0 * atan(t);
    double halfEcc = 0.5 * Eccentricity();
    double difference = DBL_MAX;
    int iteration = 0;
    // a failure in this loop means an exception will be thrown, which is
    //   an expensive operation. So letting let the loop iterate quite a bit
    //   is not a big deal. Also, the user will be unable to project at all
    //   if this fails so more reason to let this loop iterate: better to
    //   function slow than not at all.
    const int MAX_ITERATIONS = 45;

    while ((iteration < MAX_ITERATIONS) && (difference > 0.0000000001))  {
      double eccTimesSinphi = Eccentricity() * sin(localPhi);
      double newPhi = HALFPI -
                      2.0 * atan(t * pow((1.0 - eccTimesSinphi) /
                                         (1.0 + eccTimesSinphi), halfEcc));
      difference = fabs(newPhi - localPhi);
      localPhi = newPhi;
      iteration++;
    }

    if (iteration >= MAX_ITERATIONS) {
      std::string msg = "Failed to converge in TProjection::phi2Compute()";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    return localPhi;
  }

  /**
   * A convience method to compute Snyder's @a m equation (14-15) for a given 
   * latitude, @f$\phi@f$ 
   *  
   * @f[ 
   * m = \frac{\cos(\phi)}{\sqrt{1-e^2 \sin^2(\phi)}} 
   * @f] 
   * where @f$e@f$ is the eccentricity for the body. 
   *  
   * @param sinphi sine of phi
   * @param cosphi cosine of phi
   *
   * @return double Value for Snyder's m variable.
   */
  double TProjection::mCompute(const double sinphi, const double cosphi) const {
    double eccTimesSinphi = Eccentricity() * sinphi;
    double denominator = sqrt(1.0 - eccTimesSinphi * eccTimesSinphi);
    return cosphi / denominator;
  }

  /**
   * A convience method to compute Snyder's @a t equation (15-9) for a given 
   * latitude, @f$\phi@f$ 
   *  
   * @f[ 
   * t = 
   * \frac{\tan\left(\frac{\pi}{4} - \frac{\phi}{2}\right)}
   *      {\left[\frac{1-e\sin(\phi)}
   *             {1+e\sin(\phi)}\right]^{e/2}}
   * @f] 
   * where @f$e@f$ is the eccentricity for the body. 
   *
   * @param phi  phi
   * @param sinphi sin of phi
   *
   * @return double The value for Snyder's t variable.
   */
  double TProjection::tCompute(const double phi, const double sinphi) const {
    if ((HALFPI) - fabs(phi) < DBL_EPSILON) return 0.0;

    double eccTimesSinphi = Eccentricity() * sinphi;
    double denominator  = pow((1.0 - eccTimesSinphi) /
                              (1.0 + eccTimesSinphi),
                              0.5 * Eccentricity());
    return tan(0.5 * (HALFPI - phi)) / denominator;
  }

  /**
   * A convience method to compute 
   *  
   * @f[ 
   * e4 = 
   * \sqrt{(1+e )^{1+e}(1-e)^{1-e}}
   * @f] 
   * where @a e is the eccentricity of the body.
   *  
   * @return double The value for the e4 formula.
   */
  double TProjection::e4Compute() const {
    double onePlusEcc = 1.0 + Eccentricity();
    double oneMinusEcc = 1.0 - Eccentricity();

    return sqrt(pow(onePlusEcc, onePlusEcc) *
                pow(oneMinusEcc, oneMinusEcc));
  }

} //end namespace isis


