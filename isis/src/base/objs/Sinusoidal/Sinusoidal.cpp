/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Sinusoidal.h"

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
   * Constructs a Sinusoidal object.
   *
   * @param label This argument must be a Label containing the proper mapping
   *              information as indicated in the Projection class. Additionally,
   *              the sinusoidal projection requires the center longitude to be
   *              defined in the keyword CenterLongitude.
   *
   * @param allowDefaults If set to false the constructor expects that a keyword
   *                      of CenterLongitude will be in the label. Otherwise it
   *                      will attempt to compute the center longitude using the
   *                      middle of the longitude range specified in the labels.
   *                      Defaults to false
   *
   * @throws IException
   */
  Sinusoidal::Sinusoidal(Pvl &label, bool allowDefaults) :
      TProjection::TProjection(label) {
    try {
      // Try to read the mapping group
      PvlGroup &mapGroup = label.findGroup("Mapping", Pvl::Traverse);

      // Compute and write the default center longitude if allowed and
      // necessary
      if ((allowDefaults) && (!mapGroup.hasKeyword("CenterLongitude"))) {
        double lon = (m_minimumLongitude + m_maximumLongitude) / 2.0;
        mapGroup += PvlKeyword("CenterLongitude", toString(lon));
      }

      // Get the center longitude
      m_centerLongitude = mapGroup["CenterLongitude"];

      // convert to radians, adjust for longitude direction
      m_centerLongitude *= PI / 180.0;
      if (m_longitudeDirection == PositiveWest) m_centerLongitude *= -1.0;
    }
    catch(IException &e) {
      std::string message = "Invalid label group [Mapping]";
      throw IException(e, IException::Io, message, _FILEINFO_);
    }
  }

  //! Destroys the Sinusoidal object
  Sinusoidal::~Sinusoidal() {
  }

  /**
   * Compares two Projection objects to see if they are equal
   *
   * @param proj Projection object to do comparison on
   *
   * @return bool Returns true if the Projection objects are equal, and false if
   *              they are not
   */
  bool Sinusoidal::operator== (const Projection &proj) {
    if (!TProjection::operator==(proj)) return false;
    // dont do the below it is a recusive plunge
    //  if (TProjection::operator!=(proj)) return false;
    Sinusoidal *sinu = (Sinusoidal *) &proj;
    if (sinu->m_centerLongitude != m_centerLongitude) return false;
    return true;
  }

  /**
   * Returns the name of the map projection, "Sinusoidal"
   *
   * @return QString Name of projection, "Sinusoidal"
   */
  QString Sinusoidal::Name() const {
    return "Sinusoidal";
  }

  /**
   * Returns the version of the map projection
   *
   *
   * @return QString Version number
   */
  QString Sinusoidal::Version() const {
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
  bool Sinusoidal::SetGround(const double lat, const double lon) {
    // Convert to radians
    m_latitude = lat;
    m_longitude = lon;
    double latRadians = lat * PI / 180.0;
    double lonRadians = lon * PI / 180.0;
    if (m_longitudeDirection == PositiveWest) lonRadians *= -1.0;

    // Compute the coordinate
    double deltaLon = (lonRadians - m_centerLongitude);
    double x = m_equatorialRadius * deltaLon * cos(latRadians);
    double y = m_equatorialRadius * latRadians;
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
  bool Sinusoidal::SetCoordinate(const double x, const double y) {
    // Save the coordinate
    SetXY(x, y);

    // Compute latitude and make sure it is not above 90
    m_latitude = GetY() / m_equatorialRadius;
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
      m_longitude = m_centerLongitude + GetX() / (m_equatorialRadius * coslat);
    }

    // Convert to degrees
    m_latitude *= 180.0 / PI;
    m_longitude *= 180.0 / PI;

    // Cleanup the longitude
    if (m_longitudeDirection == PositiveWest) m_longitude *= -1.0;
    // These need to be done for circular type projections
    //  m_longitude = To360Domain (m_longitude);
    //  if (m_longitudeDomain == 180) m_longitude = To180Domain(m_longitude);

    // Our double precision is not good once we pass a certain magnitude of
    //   longitude. Prevent failures down the road by failing now.
    m_good = (fabs(m_longitude) < 1E10);

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
  bool Sinusoidal::XYRange(double &minX, double &maxX,
                           double &minY, double &maxY) {
    // Check the corners of the lat/lon range
    XYRangeCheck(m_minimumLatitude, m_minimumLongitude);
    XYRangeCheck(m_maximumLatitude, m_minimumLongitude);
    XYRangeCheck(m_minimumLatitude, m_maximumLongitude);
    XYRangeCheck(m_maximumLatitude, m_maximumLongitude);

    // If the latitude crosses the equator check there
    if ((m_minimumLatitude < 0.0) && (m_maximumLatitude > 0.0)) {
      XYRangeCheck(0.0, m_minimumLongitude);
      XYRangeCheck(0.0, m_maximumLongitude);
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
  PvlGroup Sinusoidal::Mapping()  {
    PvlGroup mapping = TProjection::Mapping();

    mapping += m_mappingGrp["CenterLongitude"];

    return mapping;
  }

  /**
   * This function returns the latitude keywords that this projection uses
   *
   * @return PvlGroup The latitude keywords that this projection uses
   */
  PvlGroup Sinusoidal::MappingLatitudes() {
    PvlGroup mapping = TProjection::MappingLatitudes();

    return mapping;
  }

  /**
   * This function returns the longitude keywords that this projection uses
   *
   * @return PvlGroup The longitude keywords that this projection uses
   */
  PvlGroup Sinusoidal::MappingLongitudes() {
    PvlGroup mapping = TProjection::MappingLongitudes();

    mapping += m_mappingGrp["CenterLongitude"];

    return mapping;
  }

} // end namespace isis

/** 
 * This is the function that is called in order to instantiate a 
 * Sinusoidal object.
 *  
 * @param lab Cube labels with appropriate Mapping information.
 *  
 * @param allowDefaults Indicates whether CenterLongitude are allowed to 
 *                      be computed using the middle of the longitude
 *                      range specified in the labels.
 * 
 * @return @b Isis::Projection* Pointer to a Sinusoidal projection object.
 */
extern "C" Isis::TProjection *SinusoidalPlugin(Isis::Pvl &lab,
    bool allowDefaults) {
  return new Isis::Sinusoidal(lab, allowDefaults);
}
