/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "LunarAzimuthalEqualArea.h"

#include <cmath>
#include <cfloat>

#include "Constants.h"
#include "IException.h"
#include "TProjection.h"
#include "Pvl.h"
#include "PvlGroup.h"

using namespace std;
namespace Isis {
  /**
  * Constructs a LunarAzimuthalEqualArea object
  *
  * @param label This argument must be a Label containing the proper mapping
  *              information as indicated in the Projection class. Additionally,
  *              the LunarAzimuthalEqualArea projection requires the
  *              center longitude to be defined in the keyword CenterLongitude.
  *
  * @throw IException::Unknown - "Invalid label group [Mapping]";
  */
  LunarAzimuthalEqualArea::LunarAzimuthalEqualArea(
    Pvl &label) : TProjection::TProjection(label) {
    try {
      // Try to read the mapping group
      PvlGroup &mapGroup = label.findGroup("Mapping", Pvl::Traverse);

      // Get the max libration
      m_maxLibration = mapGroup["MaximumLibration"];
      m_maxLibration *= PI / 180.0;
    }
    catch(IException &e) {
      std::string message = "Invalid label group [Mapping]";
      throw IException(IException::Unknown, message, _FILEINFO_);
    }
  }

  //! Destroys the LunarAzimuthalEqualArea object
  LunarAzimuthalEqualArea::~LunarAzimuthalEqualArea() {
  }

  /**
   * Compares two Projection objects to see if they are equal
   *
   * @param proj Projection object to do comparison on
   *
   * @return bool Returns true if the Projection objects are equal, and false if
   *              they are not
   */
   bool LunarAzimuthalEqualArea::operator== (const TProjection &proj) {
     if (!Projection::operator==(proj))
       return false;
     // dont use != (it is a recusive plunge)
     //  if (Projection::operator!=(proj)) return false;
   
     LunarAzimuthalEqualArea *LKAEA = (LunarAzimuthalEqualArea *) &proj;
     if (LKAEA->m_maxLibration != m_maxLibration)
       return false;
     return true;
   }

  /**
   * Returns the name of the map projection, "LunarAzimuthalEqualArea"
   *
   * @return QString Name of projection, "LunarAzimuthalEqualArea"
   */
   QString LunarAzimuthalEqualArea::Name() const {
     return "LunarAzimuthalEqualArea";
   }

  /**
   * Returns the version of the map projection
   *
   *
   * @return QString Version number
   */
   QString LunarAzimuthalEqualArea::Version() const {
     return "0.1";
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
  bool LunarAzimuthalEqualArea::SetGround(const double lat,
                                          const double lon) {
    // Convert longitude to radians
    m_longitude = lon;
    double lonRadians = lon * Isis::PI / 180.0;
    if (m_longitudeDirection == PositiveWest)
      lonRadians *= -1.0;

    // Now convert latitude to radians... it must be planetographic
    m_latitude = lat;
    double latRadians = lat;
    if (IsPlanetocentric())
      latRadians = ToPlanetographic(latRadians);
    latRadians *= Isis::PI / 180.0;

    double x, y;
    if (lonRadians == 0.0 && latRadians == 0.0) {
      x = 0.0;
      y = 0.0;
      SetComputedXY(x, y);
      m_good = true;
      return true;
    }

    double E = acos(cos(latRadians) * cos(lonRadians));
    double test = (sin(lonRadians) * cos(latRadians)) / sin(E);

    if (test > 1.0) test = 1.0;
    else if (test < -1.0) test = -1.0;

    double D = HALFPI - asin(test);
    if (latRadians < 0.0)
      D = -D;

    double radius = m_equatorialRadius;
    double PFAC = (HALFPI + m_maxLibration) / HALFPI;
    double RP = radius * sin(E / PFAC);

    x = RP * cos(D);
    y = RP * sin(D);

    SetComputedXY(x, y);
    m_good = true;
    return true;

  } // of SetGround


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
  bool LunarAzimuthalEqualArea::SetCoordinate(const double x,
      const double y) {
    // Save the coordinate
    SetXY(x, y);

    double RP = sqrt((x * x) + (y * y));

    double lat, lon;
    if (y == 0.0 && x == 0.0) {
      lat = 0.0;
      lon = 0.0;
      return true;
    }

    double radius = m_equatorialRadius;

    double D = atan2(y, x);
    double test = RP / radius;
    if (abs(test) > 1.0) {
      return false;
    }

    double EPSILON = 0.0000000001;
    double PFAC = (HALFPI + m_maxLibration) / HALFPI;
    double E = PFAC * asin(RP / radius);

    lat = HALFPI - (acos(sin(D) * sin(E)));

    if (abs(HALFPI - abs(lat)) <= EPSILON) {
      lon = 0.0;
    }
    else {
      test = sin(E) * cos(D) / sin(HALFPI - lat);
      if (test > 1.0) test = 1.0;
      else if (test < -1.0) test = -1.0;

      lon = asin(test);
    }

    if (E >= HALFPI) {
      if (lon <= 0.0) lon = -PI - lon;
      else lon = PI - lon;
    }

    // Convert to degrees
    m_latitude = lat * 180.0 / Isis::PI;
    m_longitude = lon * 180.0 / Isis::PI;

    // Cleanup the latitude
    if (IsPlanetocentric())
      m_latitude = ToPlanetocentric(m_latitude);

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
  bool LunarAzimuthalEqualArea::XYRange(double &minX, double &maxX,
                                        double &minY, double &maxY) {
    // Check the corners of the lat/lon range
    XYRangeCheck(m_minimumLatitude, m_minimumLongitude);
    XYRangeCheck(m_maximumLatitude, m_minimumLongitude);
    XYRangeCheck(m_minimumLatitude, m_maximumLongitude);
    XYRangeCheck(m_maximumLatitude, m_maximumLongitude);

    // If the latitude range contains 0
    if ((m_minimumLatitude < 0.0) && (m_maximumLatitude > 0.0)) {
      XYRangeCheck(0.0, m_minimumLongitude);
      XYRangeCheck(0.0, m_maximumLongitude);
    }

    // If the longitude range contains 0
    if ((m_minimumLongitude < 0.0) && (m_maximumLongitude > 0.0)) {
      XYRangeCheck(m_minimumLatitude, 0.0);
      XYRangeCheck(m_maximumLatitude, 0.0);
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
  PvlGroup LunarAzimuthalEqualArea::Mapping() {
    PvlGroup mapping = TProjection::Mapping();
    mapping += m_mappingGrp["MaximumLibration"];
    return mapping;
  }

} // end namespace isis

/** 
 * This is the function that is called in order to instantiate a 
 * LunarAzimuthalEqualArea object.
 *  
 * @param lab Cube labels with appropriate Mapping information.
 *  
 * @param allowDefaults This input parameter is currently not 
 *                      utilized.
 * 
 * @return @b Isis::Projection* Pointer to a LunarAzimuthalEqualArea
 *                              projection object.
 */
extern "C" Isis::TProjection *LunarAzimuthalEqualAreaPlugin(Isis::Pvl &lab,
    bool allowDefaults) {
  return new Isis::LunarAzimuthalEqualArea(lab);
}
