/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "LambertConformal.h"

#include <cmath>
#include <cfloat>

#include "Constants.h"
#include "IException.h"
#include "IString.h"
#include "Projection.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"

using namespace std;
namespace Isis {
  /**
   * Constructs a Lambert Conformal object
   *
   * @param label This argument must be a Label containing the proper mapping
   *              information as indicated in the Projection class. Additionally,
   *              the lambertconformal projection requires the center longitude
   *              to be defined in the keyword CenterLongitude, and the first and
   *              second standard parallels defined in the keywords
   *              FirstStandardParallel and SecondStandardParallel.
   *
   * @param allowDefaults If set to false the constructor expects that a keyword
   *                      of CenterLongitude, FirstStandardParallel, and
   *                      SecondStandardParallel will be in the label. Otherwise
   *                      it will attempt to compute the center longitude using
   *                      the middle of the longitude range specified in the
   *                      labels. Defaults to false.
   *
   * @throw IException
   */
  LambertConformal::LambertConformal(Pvl &label, bool allowDefaults) :
    TProjection::TProjection(label) {
    try {
      // Try to read the mapping group
      PvlGroup &mapGroup = label.findGroup("Mapping", Pvl::Traverse);

      // Compute and write the default center longitude if allowed and
      // necessary
      if ((allowDefaults) && (!mapGroup.hasKeyword("CenterLongitude"))) {
        double lon = (m_minimumLongitude + m_maximumLongitude) / 2.0;
        mapGroup += PvlKeyword("CenterLongitude", std::to_string(lon));
      }

      // Compute and write the default center latitude if allowed and
      // necessary
      if ((allowDefaults) && (!mapGroup.hasKeyword("CenterLatitude"))) {
        double lat = (m_minimumLatitude + m_maximumLatitude) / 2.0;
        mapGroup += PvlKeyword("CenterLatitude", std::to_string(lat));
      }

      // Get the center longitude  & latitude
      m_centerLongitude = mapGroup["CenterLongitude"];
      m_centerLatitude = mapGroup["CenterLatitude"];
      if (IsPlanetocentric()) {
        m_centerLatitude = ToPlanetographic(m_centerLatitude);
      }

      // Test to make sure center longitude is valid
      if (fabs(m_centerLongitude) > 360.0) {
        IString message = "Central Longitude [" + IString(m_centerLongitude);
        message += "] must be between -360 and 360";
        throw IException(IException::Unknown, message, _FILEINFO_);
      }

      // convert to radians, adjust for longitude direction
      m_centerLongitude *= PI / 180.0;
      if (m_longitudeDirection == PositiveWest) m_centerLongitude *= -1.0;

      // Get the standard parallels & convert them to ographic
      m_par1 = mapGroup["FirstStandardParallel"];
      if (IsPlanetocentric()) {
        m_par1 = ToPlanetographic(m_par1);
      }
      m_par2 = mapGroup["SecondStandardParallel"];
      if (IsPlanetocentric()) {
        m_par2 = ToPlanetographic(m_par2);
      }

      // Test to make sure standard parallels are valid
      if (fabs(m_par1) > 90.0 || fabs(m_par2) > 90.0) {
        QString message = "Standard Parallels must between -90 and 90";
        throw IException(IException::Unknown, message, _FILEINFO_);
      }
      if (fabs(m_par1 + m_par2) < DBL_EPSILON) {
        QString message = "Standard Parallels cannot be symmetric to the equator";
        throw IException(IException::Unknown, message, _FILEINFO_);
      }
      // Removed because this test only works for northern hemisphere
      // Just reorder the parallels so p1 is at the larger radius of the two
      //if (m_par1 > m_par2) {
      //  QString message = "Standard Parallels must be ordered";
      //  throw IException::Message(IException::Projection,message,_FILEINFO_);
      //}

      // Reorder the parallels so p1 is closer to the equator than p2
      // Therefore p2 is nearest the apex of the cone
      if (fabs(m_par1) > fabs(m_par2)) {
        double tmp = m_par2;
        m_par2 = m_par1;
        m_par1 = tmp;
      }

      // Test to make sure center latitude is valid
      // The pole opposite the apex can not be used as the clat (i.e., origin of
      // the projection) it projects to infinity
      // Given: p2 is closer to the apex than p1, and p2 must be on the same
      // side of the equator as the apex (due to the reording of p1 and p2 above)
      // Test for cone pointed south "v"
      if ((m_par2 < 0.0) && (fabs(90.0 - m_centerLatitude) < DBL_EPSILON)) {
        IString message = "Center Latitude [" + IString(m_centerLatitude);
        message += "] is not valid, it projects to infinity "
                   "for standard parallels [";
        message += IString(m_par1) + "," + IString(m_par2) + "]";
        throw IException(IException::Unknown, message, _FILEINFO_);
      }
      // Test for cone pointed north "^"
      else if ((m_par2 > 0.0) && (fabs(-90.0 - m_centerLatitude) < DBL_EPSILON)) {
        IString message = "Center Latitude [" + IString(m_centerLatitude);
        message += "] is not valid, it projects to infinity "
                   "for standard parallels [";
        message += IString(m_par1) + "," + IString(m_par2) + "]";
        throw IException(IException::Unknown, message, _FILEINFO_);
      }
      // convert clat to radians
      m_centerLatitude *= PI / 180.0;

      // Convert standard parallels to radians
      m_par1 *= PI / 180.0;
      m_par2 *= PI / 180.0;

      // Compute the Snyder's m and t values for the standard parallels and the
      // center latitude
      double sinpar1 = sin(m_par1);
      double cospar1 = cos(m_par1);
      double m1 = mCompute(sinpar1, cospar1);
      double t1 = tCompute(m_par1, sinpar1);

      double sinpar2 = sin(m_par2);
      double cospar2 = cos(m_par2);
      double m2 = mCompute(sinpar2, cospar2);
      double t2 = tCompute(m_par2, sinpar2);

      double sinclat = sin(m_centerLatitude);
      double tclat = tCompute(m_centerLatitude, sinclat);

      // Calculate Snyder's n, f, and rho
      if (fabs(m_par1 - m_par2) >= DBL_EPSILON) {
        m_n = log(m1 / m2) / log(t1 / t2);
      }
      else {
        m_n = sinpar1;
      }
      m_f = m1 / (m_n * pow(t1, m_n));
      m_rho = m_equatorialRadius * m_f * pow(tclat, m_n);
    }
    catch(IException &e) {
      QString message = "Invalid label group [Mapping]";
      throw IException(e, IException::Io, message, _FILEINFO_);
    }
  }

  //! Destroys the LambertConformal object
  LambertConformal::~LambertConformal() {
  }

  /**
   * Compares two Projection objects to see if they are equal
   *
   * @param proj Projection object to do comparison on
   *
   * @return bool Returns true if the Projection objects are equal, and false if
   *              they are not
   */
  bool LambertConformal::operator== (const Projection &proj) {
    if (!Projection::operator==(proj)) return false;
    // dont do the below it is a recusive plunge
    //  if (Projection::operator!=(proj)) return false;
    LambertConformal *lamb = (LambertConformal *) &proj;
    if ((lamb->m_centerLongitude != m_centerLongitude) ||
        (lamb->m_centerLatitude != m_centerLatitude)) return false;
    return true;
  }

  /**
   * Returns the name of the map projection, "LambertConformal"
   *
   * @return QString Name of projection, "LambertConformal"
   */
  QString LambertConformal::Name() const {
    return "LambertConformal";
  }

  /**
   * Returns the version of the map projection
   *
   *
   * @return QString Version number
   */
  QString LambertConformal::Version() const {
    return "1.0";
  }

  /**
   * Returns the latitude of true scale (in the case of LambertConformal
   * it is the smaller of the two standard parallels)
   *
   * @return double The true scale latitude, in degrees
   */
  double LambertConformal::TrueScaleLatitude() const {
    if (m_par1 > m_par2) return m_par2 * 180.0 / PI;
    else return m_par1 * 180.0 / PI;
  }

  /**
   * This method is used to set the latitude/longitude (assumed to be of the
   * correct LatitudeType, LongitudeDirection, and LongitudeDomain. The Set
   * forces an attempted calculation of the projection X/Y values. This may or
   * may not be successful and a status is returned as such.
   *
   * @param lat Latitude value to project
   *
   * @param lon Longitude value to project
   *
   * @return bool
   */
  bool LambertConformal::SetGround(const double lat, const double lon) {
    // Convert longitude to radians & clean up
    m_longitude = lon;
    double lonRadians = lon * PI / 180.0;
    if (m_longitudeDirection == PositiveWest) lonRadians *= -1.0;

    // Now convert latitude to radians & clean up ... it must be planetographic
    m_latitude = lat;
    double latRadians = lat;
    if (IsPlanetocentric()) latRadians = ToPlanetographic(latRadians);
    latRadians *= PI / 180.0;

    // Check for special cases & calculate rh, theta, and snyder t
    double rh;
    if (fabs(fabs(latRadians) - HALFPI) < DBL_EPSILON) {
      // Lat/Lon point cannot be projected
      if (latRadians *m_n <= 0.0) {
        m_good = false;
        return m_good;
      }
      else rh = 0.0;
    }
    else {
      double sinlat = sin(latRadians);
      // Lat/Lon point cannot be projected
      double t = tCompute(latRadians, sinlat);
      rh = m_equatorialRadius * m_f * pow(t, m_n);
    }
    double theta = m_n * (lonRadians - m_centerLongitude);

    // Compute the coordinate
    double x = rh * sin(theta);
    double y = m_rho - rh * cos(theta);
    SetComputedXY(x, y);

    m_good = true;
    return m_good;
  }

  /**
   * This method is used to set the projection x/y. The Set forces an attempted
   * calculation of the corresponding latitude/longitude position. This may or
   * may not be successful and a status is returned as such.
   *
   * @param x X coordinate of the projection in units that are the same as the
   *          radii in the label
   *
   * @param y Y coordinate of the projection in units that are the same as the
   *          radii in the label
   *
   * @return bool
   */
  bool LambertConformal::SetCoordinate(const double x, const double y) {
    // Save the coordinate
    SetXY(x, y);

    // Get the sign of Snyder's n
    double sign;
    if (m_n >= 0) sign = 1.0;
    else sign = -1.0;

    double temp = m_rho - GetY();
    double rh = sign * sqrt(GetX() * GetX() + temp * temp);

    double theta;
    if (rh != 0) theta = atan2(sign * GetX(), sign * temp);
    else theta = 0.0;

    // Compute latitude and longitude
    if (rh != 0 || m_n > 0) {
      double t = pow(rh / (m_equatorialRadius * m_f), 1.0 / m_n);
      m_latitude = phi2Compute(t);
    }
    else m_latitude = -HALFPI;
    m_longitude = theta / m_n + m_centerLongitude;


    // Convert to degrees
    m_latitude *= 180.0 / PI;
    m_longitude *= 180.0 / PI;

    // Cleanup the longitude
    if (m_longitudeDirection == PositiveWest) m_longitude *= -1.0;
    // These need to be done for circular type projections
    //  m_longitude = To360Domain (m_longitude);
    //  if (m_longitudeDomain == 180) m_longitude = To180Domain(m_longitude);

    // Cleanup the latitude
    if (IsPlanetocentric()) m_latitude = ToPlanetocentric(m_latitude);

    m_good = true;
    return m_good;
  }

  /**
   * This method is used to determine the x/y range which completely covers the
   * area of interest specified by the lat/lon range. The latitude/longitude
   * range may be obtained from the labels. The purpose of this method is to
   * return the x/y range so it can be used to compute how large a map may need
   * to be. For example, how big a piece of paper is needed or how large of an
   * image needs to be created. The method may fail as indicated by its return
   * value.
   *
   * @param minX Minimum x projection coordinate which covers the latitude
   *             longitude range specified in the labels.
   *
   * @param maxX Maximum x projection coordinate which covers the latitude
   *             longitude range specified in the labels.
   *
   * @param minY Minimum y projection coordinate which covers the latitude
   *             longitude range specified in the labels.
   *
   * @param maxY Maximum y projection coordinate which covers the latitude
   *             longitude range specified in the labels.
   *
   * @return bool
   *
   * @history 2009-03-03  Tracie Sucharski, Undo the PositiveWest adjustment to
   *                         the center longitude because it is done twice, once
   *                         in the constructor and again in SetGround.
   *
   * @history 2009-03-20  Stuart Sides, Modified the validity check for center
   *                      latitude. It now only tests for the pole opposite the
   *                      apex of the cone.
   */
  bool LambertConformal::XYRange(double &minX, double &maxX, 
                                 double &minY, double &maxY) {

    // Test the four corners
    XYRangeCheck(m_minimumLatitude, m_minimumLongitude);
    XYRangeCheck(m_minimumLatitude, m_maximumLongitude);
    XYRangeCheck(m_maximumLatitude, m_minimumLongitude);
    XYRangeCheck(m_maximumLatitude, m_maximumLongitude);

    // Decide which pole the apex of the cone is above
    // Remember p1 is now closest to the equator and p2 is closest to one of the
    // poles
    bool north_hemi = true;
    // Stuart Sides 2008-08-15
    // This test was removed because the reordering of p1 and p2 in the
    // constructor made it incorrect
    //if (fabs(m_par1) > fabs(m_par2)) north_hemi = false;
    if (m_par2 < 0.0) north_hemi = false;
    if ((m_par1 == m_par2) && (m_par1 < 0.0)) north_hemi = false;

    double cLonDeg = m_centerLongitude * 180.0 / PI;

    //  This is needed because the SetGround class applies the PositiveWest
    //  adjustment which was already done in the constructor.
    if (m_longitudeDirection == PositiveWest) cLonDeg = cLonDeg * -1.0;

    double pole_north, min_lat_north, max_lat_north, londiff;
    // North Pole
    if (north_hemi) {
      m_latitude = 90.0;
      m_longitude = cLonDeg;

      //Unable to project at the pole
      if (!SetGround(m_latitude, m_longitude)) {
        m_good = false;
        return m_good;
      }

      pole_north = YCoord();
      m_latitude = m_minimumLatitude;

      //Unable to project at the pole
      if (!SetGround(m_latitude, m_longitude)) {
        m_good = false;
        return m_good;
      }

      min_lat_north = YCoord();
      double y = min_lat_north + 2.0 * (pole_north - min_lat_north);

      //Unable to project opposite the center longitude
      if (!SetCoordinate(XCoord(), y)) {
        m_good = false;
        return m_good;
      }

      londiff = fabs(cLonDeg - m_longitude) / 2.0;
      m_longitude = cLonDeg - londiff;
      for (int i = 0; i < 3; i++) {
        if ((m_longitude >= m_minimumLongitude) 
            && (m_longitude <= m_maximumLongitude)) {
          m_latitude = m_minimumLatitude;
          XYRangeCheck(m_latitude, m_longitude);
        }
        m_longitude += londiff;
      }

    }
    // South Pole
    else {
      m_latitude = -90.0;
      m_longitude = cLonDeg;

      //Unable to project at the pole
      if (!SetGround(m_latitude, m_longitude)) {
        m_good = false;
        return m_good;
      }

      pole_north = YCoord();
      m_latitude = m_maximumLatitude;

      //Unable to project at the pole
      if (!SetGround(m_latitude, m_longitude)) {
        m_good = false;
        return m_good;
      }

      max_lat_north = YCoord();
      double y = max_lat_north - 2.0 * (max_lat_north - pole_north);

      //Unable to project opposite the center longitude
      if (!SetCoordinate(XCoord(), y)) {
        m_good = false;
        return m_good;
      }

      londiff = fabs(cLonDeg - m_longitude) / 2.0;
      m_longitude = cLonDeg - londiff;
      for (int i = 0; i < 3; i++) {
        if ((m_longitude >= m_minimumLongitude) 
            && (m_longitude <= m_maximumLongitude)) {
          m_latitude = m_maximumLatitude;
          XYRangeCheck(m_latitude, m_longitude);
        }
        m_longitude += londiff;
      }
    }

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
   * This function returns the keywords that this projection uses.
   *
   * @return PvlGroup The keywords that this projection uses
   */
  PvlGroup LambertConformal::Mapping() {
    PvlGroup mapping = TProjection::Mapping();

    mapping += m_mappingGrp["CenterLatitude"];
    mapping += m_mappingGrp["CenterLongitude"];
    mapping += m_mappingGrp["FirstStandardParallel"];
    mapping += m_mappingGrp["SecondStandardParallel"];

    return mapping;
  }

  /**
   * This function returns the latitude keywords that this projection uses
   *
   * @return PvlGroup The latitude keywords that this projection uses
   */
  PvlGroup LambertConformal::MappingLatitudes() {
    PvlGroup mapping = TProjection::MappingLatitudes();

    mapping += m_mappingGrp["CenterLatitude"];
    mapping += m_mappingGrp["FirstStandardParallel"];
    mapping += m_mappingGrp["SecondStandardParallel"];

    return mapping;
  }

  /**
   * This function returns the longitude keywords that this projection uses
   *
   * @return PvlGroup The longitude keywords that this projection uses
   */
  PvlGroup LambertConformal::MappingLongitudes() {
    PvlGroup mapping = TProjection::MappingLongitudes();

    mapping += m_mappingGrp["CenterLongitude"];

    return mapping;
  }
} // end namespace isis

/** 
 * This is the function that is called in order to instantiate a 
 * LambertConformal object.
 *  
 * @param lab Cube labels with appropriate Mapping information.
 *  
 * @param allowDefaults If the labels do not contain the values for 
 *                      CenterLongitude, FirstStandardParallel, and
 *                      SecondStandardParallel, this method indicates
 *                      whether the constructor should compute these values.
 * 
 * @return @b Isis::Projection* Pointer to a LambertConformal projection object.
 */
extern "C" Isis::Projection *LambertConformalPlugin(Isis::Pvl &lab,
    bool allowDefaults) {
  return new Isis::LambertConformal(lab, allowDefaults);
}

