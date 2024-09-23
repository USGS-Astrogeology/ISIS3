/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Mercator.h"

#include <cmath>
#include <cfloat>

#include "IException.h"
#include "Constants.h"
#include "TProjection.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"

using namespace std;
namespace Isis {
  /**
   * Constructs a Mercator object
   *
   * @param label This argument must be a Label containing the proper mapping
   *              information as indicated in the Projection class. Additionally,
   *              the mercator projection requires the center longitude to be
   *              defined in the keyword CenterLongitude.
   *
   * @param allowDefaults If set to false the constructor expects that a keyword
   *                      of CenterLongitude will be in the label. Otherwise it
   *                      will attempt to compute the center longitude using the
   *                      middle of the longitude range specified in the labels.
   *                      Defaults to false.
   *
   * @throw IException
   */
  Mercator::Mercator(Pvl &label, bool allowDefaults) :
    TProjection::TProjection(label) {
    try {
      // Try to read the mapping group
      PvlGroup &mapGroup = label.findGroup("Mapping", Pvl::Traverse);

      // Compute and write the default center longitude if allowed and
      // necessary
      if ((allowDefaults) && (!mapGroup.hasKeyword("CenterLongitude"))) {
        double lon = (m_minimumLongitude + m_maximumLongitude) / 2.0;
        mapGroup += PvlKeyword("CenterLongitude", Isis::toString(lon));
      }

      // Compute and write the default center latitude if allowed and
      // necessary
      if ((allowDefaults) && (!mapGroup.hasKeyword("CenterLatitude"))) {
        double lat = (m_minimumLatitude + m_maximumLatitude) / 2.0;
        mapGroup += PvlKeyword("CenterLatitude", Isis::toString(lat));
      }

      // Get the center longitude  & latitude
      m_centerLongitude = mapGroup["CenterLongitude"];
      m_centerLatitude = mapGroup["CenterLatitude"];
      if (IsPlanetocentric()) {
        m_centerLatitude = ToPlanetographic(m_centerLatitude);
      }

      // convert to radians, adjust for longitude direction
      m_centerLongitude *= PI / 180.0;
      m_centerLatitude *= PI / 180.0;
      if (m_longitudeDirection == PositiveWest) m_centerLongitude *= -1.0;

      // Compute the scale factor
      double cos_clat = cos(m_centerLatitude);
      double sin_clat = sin(m_centerLatitude);
      double m_eccsq = Eccentricity() * Eccentricity();
      m_scalefactor = cos_clat / sqrt(1.0 - m_eccsq * sin_clat * sin_clat);
    }
    catch (IException &e) {
      std::string message = "Invalid label group [Mapping]";
      throw IException(e, IException::Io, message, _FILEINFO_);
    }
  }

  //! Destroys the Mercator object
  Mercator::~Mercator() {
  }

  /**
   * Compares two Projection objects to see if they are equal
   *
   * @param proj Projection object to do comparison on
   *
   * @return bool Returns true if the Projection objects are equal, and false if
   *              they are not
   */
  bool Mercator::operator== (const Projection &proj) {
    if (!Projection::operator==(proj)) return false;
    // don't do the below it is a recusive plunge
    //  if (Projection::operator!=(proj)) return false;
    Mercator *merc = (Mercator *) &proj;
    if ((merc->m_centerLongitude != m_centerLongitude) ||
        (merc->m_centerLatitude != m_centerLatitude)) return false;
    return true;
  }

  /**
   * Returns the name of the map projection, "Mercator"
   *
   * @return QString Name of projection, "Mercator"
   */
  QString Mercator::Name() const {
    return "Mercator";
  }

  /**
   * Returns the version of the map projection
   *
   *
   * @return QString Version number
   */
  QString Mercator::Version() const {
    return "1.0";
  }

  /**
   * Returns the latitude of true scale in degrees.  For 
   * Mercator projections, it is the center latitude. 
   *
   * @return double The center latitude, in degrees.
   */
  double Mercator::TrueScaleLatitude() const {
    return m_centerLatitude * 180.0 / PI;
  }

  /**
   * Indicates whether the projection is Equitorial Cylindrical.
   * 
   * @return @b bool True if the projection is cylindrical. 
   */
  bool Mercator::IsEquatorialCylindrical() {
    return true;
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
  bool Mercator::SetGround(const double lat, const double lon) {
    // Convert longitude to radians & clean up
    m_longitude = lon;
    double lonRadians = lon * PI / 180.0;
    if (m_longitudeDirection == PositiveWest) lonRadians *= -1.0;

    // Now convert latitude to radians & clean up ... it must be planetographic
    m_latitude = lat;
    double latRadians = lat;
    if (IsPlanetocentric()) latRadians = ToPlanetographic(latRadians);
    latRadians *= PI / 180.0;

    // Make sure latitude value is not too close to either pole
    if (fabs(fabs(m_latitude) - 90.0) <= DBL_EPSILON) {
      m_good = false;
      return m_good;
    }

    // Compute the coordinate
    double deltaLon = (lonRadians - m_centerLongitude);
    double x = m_equatorialRadius * deltaLon * m_scalefactor;
    double sinphi = sin(latRadians);
    double t = tCompute(latRadians, sinphi);
    double y = -m_equatorialRadius * m_scalefactor * log(t);
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
  bool Mercator::SetCoordinate(const double x, const double y) {
    // Save the coordinate
    SetXY(x, y);

    // Compute Snyder's t
    double snyders_t = exp(-GetY() / (m_equatorialRadius * m_scalefactor));

    // Compute latitude and make sure it is not above 90
    m_latitude = phi2Compute(snyders_t);
    if (fabs(m_latitude) > HALFPI) {
      if (fabs(HALFPI - fabs(m_latitude)) > DBL_EPSILON) {
        m_good = false;
        return m_good;
      }
      else if (m_latitude < 0.0) {
        m_latitude = -HALFPI;
      }
      else {
        m_latitude = HALFPI;
      }
    }

    // Compute longitude
    double coslat = cos(m_latitude);
    if (coslat <= DBL_EPSILON) {
      m_longitude = m_centerLongitude;
    }
    else {
      m_longitude = m_centerLongitude + GetX() /
                    (m_equatorialRadius * m_scalefactor);
    }

    // Convert to degrees
    m_latitude *= 180.0 / PI;
    m_longitude *= 180.0 / PI;

    // Cleanup the longitude
    if (m_longitudeDirection == PositiveWest) m_longitude *= -1.0;
    // These need to be done for circular type projections
    // m_longitude = To360Domain (m_longitude);
    // if (m_longitudeDomain == 180) m_longitude = To180Domain(m_longitude);

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
  bool Mercator::XYRange(double &minX, double &maxX, 
                         double &minY, double &maxY) {
    // Check the corners of the lat/lon range
    XYRangeCheck(m_minimumLatitude, m_minimumLongitude);
    XYRangeCheck(m_maximumLatitude, m_minimumLongitude);
    XYRangeCheck(m_minimumLatitude, m_maximumLongitude);
    XYRangeCheck(m_maximumLatitude, m_maximumLongitude);

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
  PvlGroup Mercator::Mapping() {
    PvlGroup mapping = TProjection::Mapping();

    mapping += m_mappingGrp["CenterLatitude"];
    mapping += m_mappingGrp["CenterLongitude"];

    return mapping;
  }

  /**
   * This function returns the latitude keywords that this projection uses
   *
   * @return PvlGroup The latitude keywords that this projection uses
   */
  PvlGroup Mercator::MappingLatitudes() {
    PvlGroup mapping = TProjection::MappingLatitudes();

    mapping += m_mappingGrp["CenterLatitude"];

    return mapping;
  }

  /**
   * This function returns the longitude keywords that this projection uses
   *
   * @return PvlGroup The longitude keywords that this projection uses
   */
  PvlGroup Mercator::MappingLongitudes() {
    PvlGroup mapping = TProjection::MappingLongitudes();

    mapping += m_mappingGrp["CenterLongitude"];

    return mapping;
  }

} // end namespace isis

/** 
 * This is the function that is called in order to instantiate a 
 * Mercator object.
 *  
 * @param lab Cube labels with appropriate Mapping information.
 *  
 * @param allowDefaults Indicates whether CenterLongitude are allowed to 
 *                      be computed using the middle of the longitude
 *                      range specified in the labels.
 * 
 * @return @b Isis::Projection* Pointer to a Mercator projection 
 *         object.
 */
extern "C" Isis::Projection *MercatorPlugin(Isis::Pvl &lab,
    bool allowDefaults) {
  return new Isis::Mercator(lab, allowDefaults);
}


