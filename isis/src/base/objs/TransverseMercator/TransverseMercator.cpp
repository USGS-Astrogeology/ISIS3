/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "TransverseMercator.h"

#include <cmath>
#include <cfloat>

#include "Constants.h"
#include "IException.h"
#include "TProjection.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"

using namespace std;
namespace Isis {
  /**
   * Constructs a TransverseMercator object
   *
   * @param label This argument must be a Label containing the proper mapping
   *              information as indicated in the Projection class. Additionally,
   *              the transversemercator projection requires the center longitude
   *              to be defined in the keyword CenterLongitude, and the scale
   *              factor to be defined in the keyword ScaleFactor.
   *
   * @param allowDefaults If set to false the constructor expects that a keyword
   *                      of CenterLongitude and ScaleFactor will be in the label.
   *                      Otherwise it will attempt to compute the center
   *                      longitude using the middle of the longitude range
   *                      specified in the label, and the scale factor will
   *                      default to 1.0. Defaults to false.
   *
   * @throws IException
   */
  TransverseMercator::TransverseMercator(Pvl &label, bool allowDefaults) :
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

      // make sure the center latitude value is valid
      if (fabs(m_centerLatitude) >= 90.0) {
        string msg = "Invalid Center Latitude Value. Must be between -90 and 90";
        throw IException(IException::Io, msg, _FILEINFO_);
      }

      // make sure the center longitude value is valid
      if (fabs(m_centerLongitude) > 360.0) {
        string msg = "Invalid Center Longitude Value. Must be between -360 and 360";
        throw IException(IException::Io, msg, _FILEINFO_);
      }

      // convert latitude to planetographic if it is planetocentric
      if (IsPlanetocentric()) {
        m_centerLatitude = ToPlanetographic(m_centerLatitude);
      }

      // convert to radians and adjust for longitude direction
      if (m_longitudeDirection == PositiveWest) m_centerLongitude *= -1.0;
      m_centerLatitude *= PI / 180.0;
      m_centerLongitude *= PI / 180.0;

      // Compute other necessary variables. See Snyder, page 61
      m_eccsq = Eccentricity() * Eccentricity();
      m_esp = m_eccsq;
      m_e0 = 1.0 - 0.25 * m_eccsq * (1.0 + m_eccsq / 16.0 * (3.0 + 1.25 * m_eccsq));
      m_e1 = 0.375 * m_eccsq * (1.0 + 0.25 * m_eccsq * (1.0 + 0.468750 * m_eccsq));
      m_e2 = 0.058593750 * m_eccsq * m_eccsq * (1.0 + 0.750 * m_eccsq);
      m_e3 = m_eccsq * m_eccsq * m_eccsq * (35.0 / 3072.0);
      m_ml0 = m_equatorialRadius * (m_e0 * m_centerLatitude - m_e1 * sin(2.0 * m_centerLatitude) +
                                    m_e2 * sin(4.0 * m_centerLatitude) - m_e3 * sin(6.0 * m_centerLatitude));

      // Set flag for sphere or ellipsiod
      m_sph = true; // Sphere
      if (Eccentricity() >= .00001) {
        m_sph = false; // Ellipsoid
        m_esp = m_eccsq / (1.0 - m_eccsq);
      }

      // Get the scale factor
      if ((allowDefaults) && (!mapGroup.hasKeyword("ScaleFactor"))) {
        mapGroup += PvlKeyword("ScaleFactor", std::to_string(1.0));
      }
      m_scalefactor = mapGroup["ScaleFactor"];
    }
    catch(IException &e) {
      string message = "Invalid label group [Mapping]";
      throw IException(e, IException::Io, message, _FILEINFO_);
    }
  }

  //! Destroys the TransverseMercator object
  TransverseMercator::~TransverseMercator() {
  }

  /**
   * Compares two Projection objects to see if they are equal
   *
   * @param proj Projection object to do comparison on
   *
   * @return bool Returns true if the Projection objects are equal, and false if
   *              they are not
   */
  bool TransverseMercator::operator== (const Projection &proj) {
    if (!Projection::operator==(proj)) return false;
    // dont do the below it is a recusive plunge
    //  if (Projection::operator!=(proj)) return false;
    TransverseMercator *trans = (TransverseMercator *) &proj;
    if ((trans->m_centerLongitude != m_centerLongitude) ||
        (trans->m_centerLatitude != m_centerLatitude)) return false;
    return true;
  }

  /**
   * Returns the name of the map projection, "TransverseMercator"
   *
   * @return string Name of projection, "TransverseMercator"
   */
  QString TransverseMercator::Name() const {
    return "TransverseMercator";
  }

  /**
   * Returns the version of the map projection
   *
   *
   * @return string Version number
   */
  QString TransverseMercator::Version() const {
    return "1.0";
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
  bool TransverseMercator::SetGround(const double lat, const double lon) {
    // Get longitude & fix direction
    m_longitude = lon;
    if (m_longitudeDirection == PositiveWest) m_longitude *= -1.0;

    double cLonDeg = m_centerLongitude * 180.0 / PI;
    double deltaLon = m_longitude - cLonDeg;
    while(deltaLon < -360.0) deltaLon += 360.0;
    while(deltaLon > 360.0) deltaLon -= 360.0;
    double deltaLonRads = deltaLon * PI / 180.0;

    // Now convert latitude to radians & clean up ... it must be planetographic
    m_latitude = lat;
    double latRadians = m_latitude * PI / 180.0;
    if (IsPlanetocentric()) {
      latRadians = ToPlanetographic(m_latitude) * PI / 180.0;
    }

    // distance along the meridian fromthe Equator to the latitude phi
    // see equation (3-21) on pages 61, 17.
    double M = m_equatorialRadius * (m_e0 * latRadians 
                                      - m_e1 * sin(2.0 * latRadians)
                                      + m_e2 * sin(4.0 * latRadians) 
                                      - m_e3 * sin(6.0 * latRadians));

    // Declare variables
    const double epsilon = 1.0e-10;

    // Sphere Conversion
    double x, y;
    if (m_sph) {
      double cosphi = cos(latRadians);
      double b = cosphi * sin(deltaLonRads);

      // Point projects to infinity
      if (fabs(fabs(b) - 1.0) <= epsilon) {
        m_good = false;
        return m_good;
      }
      x = 0.5 * m_equatorialRadius * m_scalefactor * log((1.0 + b) / (1.0 - b));

      // If arcosine argument is too close to 1, con=0.0 because arcosine(1)=0
      double con = cosphi * cos(deltaLonRads) / sqrt(1.0 - b * b);
      if (fabs(con) > 1.0) {
        con = 0.0;
      }
      else {
        con = acos(con);
      }
      if (m_latitude < 0.0) con = -con;
      y = m_equatorialRadius * m_scalefactor * (con - m_centerLatitude);
    }

    // Ellipsoid Conversion
    else {
      if (fabs(HALFPI - fabs(latRadians)) < epsilon) {
        x = 0.0;
        y = m_scalefactor * (M - m_ml0);
      }
      else {
        // Define Snyder's variables for ellipsoidal projections, page61
        double sinphi = sin(latRadians);
        double cosphi = cos(latRadians);
        double A = cosphi * deltaLonRads;        // see equation (8-15), page 61
        double Asquared = A * A;
        double C = m_esp * cosphi * cosphi;      // see equation (8-14), page 61
        double tanphi = tan(latRadians);
        double T = tanphi * tanphi;              // see equation (8-13), page 61
        double N = m_equatorialRadius / sqrt(1.0 - m_eccsq * sinphi * sinphi);
                                                 // see equation (4-20), page 61

        x = m_scalefactor * N * A 
               * (1.0 + Asquared / 6.0 * (1.0 - T + C + Asquared / 20.0 
                                 *(5.0 - 18.0*T + T*T + 72.0*C - 58.0*m_esp)));
        y = m_scalefactor 
               * (M - m_ml0 + N*tanphi*(Asquared * (0.5 + Asquared / 24.0 *
                             (5.0 - T + 9.0*C + 4.0*C*C + Asquared / 30.0
                             *(61.0 - 58.0*T + T*T + 600.0*C - 330.0*m_esp)))));
      }
    }

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
  bool TransverseMercator::SetCoordinate(const double x, const double y) {
    // Save the coordinate
    SetXY(x, y);

    // Declare & Initialize variables
    double f, g, h, temp, con, phi, dphi, sinphi, cosphi, tanphi;
    double c, cs, t, ts, n, rp, d, ds;
    const double epsilon = 1.0e-10;

    // Sphere Conversion
    if (m_sph) {
      f = exp(GetX() / (m_equatorialRadius * m_scalefactor));
      g = 0.5 * (f - 1.0 / f);
      temp = m_centerLatitude + GetY() / (m_equatorialRadius * m_scalefactor);
      h = cos(temp);
      con = sqrt((1.0 - h * h) / (1.0 + g * g));
      if (con > 1.0) con = 1.0;
      if (con < -1.0) con = -1.0;
      m_latitude = asin(con);
      if (temp < 0.0) m_latitude = -m_latitude;
      m_longitude = m_centerLongitude;
      if (g != 0.0 || h != 0.0) {
        m_longitude = atan2(g, h) + m_centerLongitude;
      }
    }

    // Ellipsoid Conversion
    else if (!m_sph) {
      con = (m_ml0 + GetY() / m_scalefactor) / m_equatorialRadius;
      phi = con;
      for (int i = 1; i < 7; i++) {
        dphi = ((con + m_e1 * sin(2.0 * phi) - m_e2 * sin(4.0 * phi)
                 + m_e3 * sin(6.0 * phi)) / m_e0) - phi;
        phi += dphi;
        if (fabs(dphi) <= epsilon) break;
      }

      // Didn't converge
      if (fabs(dphi) > epsilon) {
        m_good = false;
        return m_good;
      }
      if (fabs(phi) >= HALFPI) {
        if (GetY() >= 0.0) m_latitude = fabs(HALFPI);
        if (GetY() < 0.0) m_latitude = - fabs(HALFPI);
        m_longitude = m_centerLongitude;
      }
      else {
        sinphi = sin(phi);
        cosphi = cos(phi);
        tanphi = tan(phi);
        c = m_esp * cosphi * cosphi;
        cs = c * c;
        t = tanphi * tanphi;
        ts = t * t;
        con = 1.0 - m_eccsq * sinphi * sinphi;
        n = m_equatorialRadius / sqrt(con);
        rp = n * (1.0 - m_eccsq) / con;
        d = GetX() / (n * m_scalefactor);
        ds = d * d;
        m_latitude = phi - (n * tanphi * ds / rp) * (0.5 - ds /
                     24.0 * (5.0 + 3.0 * t + 10.0 * c - 4.0 * cs - 9.0 *
                             m_esp - ds / 30.0 * (61.0 + 90.0 * t + 298.0 * c +
                                 45.0 * ts - 252.0 * m_esp - 3.0 * cs)));


        // Latitude cannot be greater than + or - halfpi radians (or 90 degrees)
        if (fabs(m_latitude) > HALFPI) {
          m_good = false;
          return m_good;
        }
        m_longitude = m_centerLongitude 
                      + (d * (1.0 - ds / 6.0 *
                              (1.0 + 2.0 * t + c - ds / 20.0 * (5.0 - 2.0 * c +
                                   28.0 * t - 3.0 * cs + 8.0 * m_esp + 24.0 * ts))) / cosphi);
      }
    }

    // Convert to Degrees
    m_latitude *= 180.0 / PI;
    m_longitude *= 180.0 / PI;

    // Cleanup the longitude
    if (m_longitudeDirection == PositiveWest) m_longitude *= -1.0;
    // These need to be done for circular type projections
    m_longitude = To360Domain(m_longitude);
    if (m_longitudeDomain == 180) m_longitude = To180Domain(m_longitude);

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
   */
  bool TransverseMercator::XYRange(double &minX, double &maxX, 
                                   double &minY, double &maxY) {
    // Check the corners of the lat/lon range
    XYRangeCheck(m_minimumLatitude, m_minimumLongitude);
    XYRangeCheck(m_maximumLatitude, m_minimumLongitude);
    XYRangeCheck(m_minimumLatitude, m_maximumLongitude);
    XYRangeCheck(m_maximumLatitude, m_maximumLongitude);

    // convert center latitude to degrees & test
    double clat = m_centerLatitude * 180.0 / PI;

    if (clat > m_minimumLatitude &&
        clat < m_maximumLatitude) {
      XYRangeCheck(clat, m_minimumLongitude);
      XYRangeCheck(clat, m_maximumLongitude);
    }

    // convert center longitude to degrees & test
    double clon = m_centerLongitude * 180.0 / PI;
    if (clon > m_minimumLongitude &&
        clon < m_maximumLongitude) {
      XYRangeCheck(m_minimumLatitude, clon);
      XYRangeCheck(m_maximumLatitude, clon);
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
  PvlGroup TransverseMercator::Mapping() {
    PvlGroup mapping = TProjection::Mapping();

    mapping += m_mappingGrp["CenterLatitude"];
    mapping += m_mappingGrp["CenterLongitude"];
    mapping += m_mappingGrp["ScaleFactor"];

    return mapping;
  }

  /**
   * This function returns the latitude keywords that this projection uses
   *
   * @return PvlGroup The latitude keywords that this projection uses
   */
  PvlGroup TransverseMercator::MappingLatitudes() {
    PvlGroup mapping = TProjection::MappingLatitudes();

    mapping += m_mappingGrp["CenterLatitude"];

    return mapping;
  }

  /**
   * This function returns the longitude keywords that this projection uses
   *
   * @return PvlGroup The longitude keywords that this projection uses
   */
  PvlGroup TransverseMercator::MappingLongitudes() {
    PvlGroup mapping = TProjection::MappingLongitudes();

    mapping += m_mappingGrp["CenterLongitude"];

    return mapping;
  }
} // end namespace isis

/** 
 * This is the function that is called in order to instantiate a 
 * TransverseMercator object.
 *  
 * @param lab Cube labels with appropriate Mapping information.
 *  
 * @param allowDefaults Indicates whether CenterLongitude and ScaleFactor 
 *                      are allowed to be computed using the middle of the
 *                      longitude range specified in the label, and the
 *                      scale factor will default to 1.0.
 * 
 * @return @b Isis::Projection* Pointer to a TransverseMercator 
 *                              projection object.
 */
extern "C" Isis::Projection *TransverseMercatorPlugin(Isis::Pvl &lab,
    bool allowDefaults) {
  return new Isis::TransverseMercator(lab, allowDefaults);
}


