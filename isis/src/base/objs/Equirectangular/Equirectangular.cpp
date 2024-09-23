/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Equirectangular.h"

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
   * Constructs a Equirectangular object.
   *
   * @param label This argument must be a Label containing the proper mapping
   *              information as indicated in the IsisProjection class.
   *              Additionally, the equirectangular projection requires the center
   *              longitude to be defined in the keyword CenterLongitude as well
   *              as the center latitude in CenterLatitude.
   *
   * @param allowDefaults (Default value is false) If set to false the
   *                      constructor requires that the keywords CenterLongitude
   *                      and CenterLatitude exist in the label. Otherwise, if
   *                      they do not exist they will be computed and written to
   *                      the label using the middle of the latitude/longitude range.
   *
   * @throw IException  - "Cannot project without [CenterLongitude] value. Keyword 
   *                       does not exist in the labels and defaults are not allowed."
   * @throw IException  - "Cannot project without [CenterLatitude] value. Keyword 
   *                       does not exist in the labels and defaults are not allowed."
   * @throw IException  - "Keyword value for CenterLatitude is too close to the pole" 
   * @throw IException  - "Invalid label group [Mapping]." 
   *  
   * @internal 
   *   @history 2013-04-26 Jeannie Backer - Modified so that default center
   *                           lat/lon values are at the center of the lat/lon ranges,
   *                           respectively. This was done to be consistent with
   *                           other projection defaults.
   */
  Equirectangular::Equirectangular(Pvl &label, bool allowDefaults) :
    TProjection::TProjection(label) {
    try {
      // Try to read the mapping group
      PvlGroup &mapGroup = label.findGroup("Mapping", Pvl::Traverse);

      // Compute the default value if allowed and needed
      if (!mapGroup.hasKeyword("CenterLongitude")) {
        if (allowDefaults) {
          double lon = (m_minimumLongitude + m_maximumLongitude) / 2.0;
          mapGroup += PvlKeyword("CenterLongitude", Isis::toString(lon));
        }
        else {
          std::string message = "Cannot project using Equirectangular Cylindrical";
          message += " without [CenterLongitude] value.  Keyword does not exist";
          message += " in labels and defaults are not allowed.";
          throw IException(IException::Unknown, message, _FILEINFO_);
        }
      }

      if (!mapGroup.hasKeyword("CenterLatitude")) {
        if (allowDefaults) {
          double lat = (m_minimumLatitude + m_maximumLatitude) / 2.0;
          mapGroup += PvlKeyword("CenterLatitude", Isis::toString(lat));
        }
        else {
          std::string message = "Cannot project using Equirectangular Cylindrical";
          message += " without [CenterLatitude] value.  Keyword does not exist";
          message += " in labels and defaults are not allowed.";
          throw IException(IException::Unknown, message, _FILEINFO_);
        }
      }

      // Get the center longitude, convert to radians, adjust for longitude
      // direction
      m_centerLongitude = mapGroup["CenterLongitude"];
      m_centerLongitude *= PI / 180.0;
      if (m_longitudeDirection == PositiveWest) m_centerLongitude *= -1.0;

      // Get the center latitude, the radius at the clat, and convert to radians
      m_centerLatitude = mapGroup["CenterLatitude"];
      m_clatRadius = LocalRadius(m_centerLatitude);
      m_centerLatitude *= PI / 180.0;

      // This keyword is just for user's information, and was put in for Hirise
      if (!mapGroup.hasKeyword("CenterLatitudeRadius")) {
        mapGroup += PvlKeyword("CenterLatitudeRadius");
      }

      mapGroup["CenterLatitudeRadius"] = Isis::toString(m_clatRadius);

      // Compute cos of the center latitude and make sure it is valid as
      // we will be dividing with it later on
      m_cosCenterLatitude = cos(m_centerLatitude);
      if (fabs(m_cosCenterLatitude) < DBL_EPSILON) {
        std::string message = "Keyword value for CenterLatitude is "
                         "too close to the pole";
        throw IException(IException::Io, message, _FILEINFO_);
      }
    }
    catch(IException &e) {
      std::string message = "Invalid label group [Mapping]";
      throw IException(e, IException::Io, message, _FILEINFO_);
    }
  }

  //! Destroys the Equirectangular object.
  Equirectangular::~Equirectangular() {
  }

  /**
   * Compares two Projection objects to see if they are equal
   *
   * @param proj Projection object to do comparison on
   *
   * @return bool Returns true if the Projection objects are equal, and false if
   *              they are not
   */
  bool Equirectangular::operator== (const Projection &proj) {
    if (!Projection::operator==(proj)) return false;
    // dont do the below it is a recusive plunge
    //  if (Projection::operator!=(proj)) return false;
    Equirectangular *equi = (Equirectangular *) &proj;
    if (equi->m_centerLongitude != m_centerLongitude) return false;
    if (equi->m_centerLatitude != m_centerLatitude) return false;
    return true;
  }

  /**
   * Returns the name of the map projection, "Equirectangular"
   *
   * @return QString Name of projection, "Equirectangular"
   */
  QString Equirectangular::Name() const {
    return "Equirectangular";
  }

  /**
   * Returns the version of the map projection
   *
   * @return QString Version number
   */
  QString Equirectangular::Version() const {
    return "1.0";
  }

  /**
   * Returns the latitude of true scale, in degrees. In the case of 
   * Equirectangular it is the center latitude. 
   *
   * @return double The center latitude, in degrees.
   */
  double Equirectangular::TrueScaleLatitude() const {
    return m_centerLatitude * 180.0 / PI;
  }

  /**
   * Indicates whether the projection is Equitorial Cylindrical.
   * 
   * @return @b bool True if the projection is cylindrical. 
   */
  bool Equirectangular::IsEquatorialCylindrical() {
    return true;
  }

  /**
   * This method is used to set the latitude/longitude (assumed to be of the
   * correct LatitudeType, LongitudeDirection, and LongitudeDomain. The Set
   * forces an attempted calculation of the projection X/Y values. This may or
   * may not be successful and a status is returned as such.
   *
   * @param lat Latitude value to project, in degreees.
   * @param lon Longitude value to project, in degreees.
   *
   * @return bool Indicates whether the ground values were set.
   */
  bool Equirectangular::SetGround(const double lat, const double lon) {
    // Convert to radians
    m_latitude = lat;
    m_longitude = lon;
    double latRadians = lat * PI / 180.0;
    double lonRadians = lon * PI / 180.0;
    if (m_longitudeDirection == PositiveWest) lonRadians *= -1.0;

    // Compute the coordinate
    double deltaLon = (lonRadians - m_centerLongitude);
    double x = m_clatRadius * m_cosCenterLatitude * deltaLon;
    double y = m_clatRadius * latRadians;
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
   * @return bool Indicates whether the (x,y) coordinate was set.
   */
  bool Equirectangular::SetCoordinate(const double x, const double y) {
    // Save the coordinate
    SetXY(x, y);

    // Compute latitude and make sure it is not above 90
    m_latitude = GetY() / m_clatRadius;
    if ((fabs(m_latitude) - HALFPI) > DBL_EPSILON) {
      m_good = false;
      return m_good;
    }

    // Compute longitude
    m_longitude = m_centerLongitude +
                  GetX() / (m_clatRadius * m_cosCenterLatitude);

    // Convert to degrees
    m_latitude *= 180.0 / PI;
    m_longitude *= 180.0 / PI;

    // Cleanup the longitude
    if (m_longitudeDirection == PositiveWest) m_longitude *= -1.0;
    // Do these if the projection is circular
    //  m_longitude = To360Domain (m_longitude);
    //  if (m_longitudeDomain == 180) m_longitude = To180Domain(m_longitude);

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
   * @return bool Indicates whether the method was successful.
   */
  bool Equirectangular::XYRange(double &minX, double &maxX,
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
   * This function returns a PvlGroup containing the keywords that this 
   * projection uses, namely CenterLatitude and CenterLongitude.
   *
   * @return PvlGroup The keywords that this projection uses
   */
  PvlGroup Equirectangular::Mapping() {
    PvlGroup mapping = TProjection::Mapping();

    mapping += m_mappingGrp["CenterLatitude"];
    mapping += m_mappingGrp["CenterLongitude"];

    return mapping;
  }

  /**
   * This function returns a PvlGroup containing the latitude keywords that this
   * projection uses, namely CenterLatitude. 
   *
   * @return PvlGroup The latitude keywords that this projection uses
   */
  PvlGroup Equirectangular::MappingLatitudes() {
    PvlGroup mapping = TProjection::MappingLatitudes();

    mapping += m_mappingGrp["CenterLatitude"];

    return mapping;
  }

  /**
   * This function returns a PvlGroup containing the longitude keywords that
   * this projection uses, namely CenterLongitude. 
   *
   * @return PvlGroup The longitude keywords that this projection uses
   */
  PvlGroup Equirectangular::MappingLongitudes() {
    PvlGroup mapping = TProjection::MappingLongitudes();

    mapping += m_mappingGrp["CenterLongitude"];

    return mapping;
  }

}

/** 
 * This is the function that is called in order to instantiate a 
 * Equirectangular object.
 *  
 * @param lab Cube labels with appropriate Mapping information.
 *  
 * @param allowDefaults Indicates whether CenterLongitude and 
 *                      CenterLatitude are allowed to be computed.
 * 
 * @return @b Isis::Projection* Pointer to a Equirectangular 
 *                              projection object.
 */
extern "C" Isis::Projection *EquirectangularPlugin(Isis::Pvl &lab,
    bool allowDefaults) {
  return new Isis::Equirectangular(lab, allowDefaults);
}

