/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Orthographic.h"

#include <cfloat>
#include <cmath>
#include <iomanip>

#include <QDebug>
#include "Constants.h"
#include "IException.h"
#include "TProjection.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"

using namespace std;
namespace Isis {
  /**
   * Constructs an Orthographic object
   *
   * @param label This argument must be a Label containing the proper mapping
   *              information as indicated in the Projection class. Additionally,
   *              the orthographic projection requires the center longitude to be
   *              defined in the keyword CenterLongitude.
   *
   * @param allowDefaults If set to false the constructor expects that a keyword
   *                      of CenterLongitude will be in the label. Otherwise it
   *                      will attempt to compute the center longitude using the
   *                      middle of the longitude range specified in the labels.
   *                      Defaults to false.
   *
   * @throws IException
   */
  Orthographic::Orthographic(Pvl &label, bool allowDefaults) :
    TProjection::TProjection(label) {
    try {
      // Try to read the mapping group
      PvlGroup &mapGroup = label.findGroup("Mapping", Pvl::Traverse);

      /*
       * Compute and write the default center longitude if allowed and
       * necessary.
       */
      if ((allowDefaults) && (!mapGroup.hasKeyword("CenterLongitude"))) {
        double lon = (m_minimumLongitude + m_maximumLongitude) / 2.0;
        mapGroup += PvlKeyword("CenterLongitude", Isis::toString(lon));
      }

      /*
       * Compute and write the default center latitude if allowed and
       * necessary
       */
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

      //Restrict center longitude to avoid converting between domains.
      if ((m_centerLongitude < -360.0) || (m_centerLongitude > 360.0)) {
        std::string msg = "The center longitude cannot exceed [-360, 360]. "
                      "[" + toString(m_centerLongitude) + "] is not valid";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      // convert to radians, adjust for longitude direction
      m_centerLongitude *= PI / 180.0;
      m_centerLatitude *= PI / 180.0;
      if (m_longitudeDirection == PositiveWest) m_centerLongitude *= -1.0;

      // Calculate sine & cosine of center latitude
      m_sinph0 = sin(m_centerLatitude);
      m_cosph0 = cos(m_centerLatitude);

      /*
       * This projection has a limited lat/lon range (can't do the world)
       * So let's make sure that our lat/lon range is properly restricted!
       * The equation: m_sinph0 * sinphi + m_cosph0 * cosphi * coslon tells us
       * if we are inside of the projection.
       *
       * m_sinph0 = sin(center lat)
       * sinphi = sin(lat)
       * m_cosph0 = cos(center lat)
       * cosphi = cos(lat)
       * coslon = cos(lon - centerLon)
       *
       * Let's apply this equation at the extremes to minimize our lat/lon range
       */
      double sinphi, cosphi, coslon;

      /* Can we project at the minlat, center lon? If not, then we should move
       * up the min lat to be inside the image.
       */
      sinphi = sin(m_minimumLatitude * PI / 180.0);
      cosphi = cos(m_minimumLatitude * PI / 180.0);
      coslon = 1.0; // at lon=centerLon: cos(lon-centerLon) = cos(0) = 1
      if (m_sinph0 * sinphi + m_cosph0 * cosphi * coslon < 1E-10) {
        /*solve for x: a * sin(x) + b * cos(x) * 1 = 0
         *      a * sin(x) + b * cos(x) = 0
         *      a * sin(x) = - b * cos(x)
         *      -(a * sin(x)) / b = cos(x)
         *      -(a / b) = cos(x) / sin(x)
         *      -(b / a) = sin(x) / cos(x)
         *      -(b / a) = tan(x)
         *      arctan(-(b / a)) = x
         *      arctan(-(m_cosph0 / m_sinph0)) = x
         */
        double newMin = atan2(- m_cosph0, m_sinph0) * 180.0 / PI;
        if (newMin > m_minimumLatitude) {
          m_minimumLatitude = newMin;
        } // else something else is off (i.e. longitude range)
      }

      //Restrict the longitude range to 360 degrees to simplify comparisons.
      if ((m_maximumLongitude - m_minimumLongitude) > 360.0) {
        std::string msg = "The longitude range cannot exceed 360 degrees.";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      sinphi = sin(m_minimumLatitude * PI / 180.0);
      cosphi = cos(m_minimumLatitude * PI / 180.0);

      /* Can we project at the maxlat, center lon? If not, then we should move
       * down the max lat to be inside the image.
       */
      sinphi = sin(m_maximumLatitude * PI / 180.0);
      cosphi = cos(m_maximumLatitude * PI / 180.0);
      coslon = 1.0; // at lon=centerLon: cos(lon-centerLon) = cos(0) = 1
      if (m_sinph0 * sinphi + m_cosph0 * cosphi * coslon < 1E-10) {
        // see above equations for latitude
        double newMax = atan2(- m_cosph0, m_sinph0) * 180.8 / PI;
        if (newMax < m_maximumLatitude && newMax > m_minimumLatitude) {
          m_maximumLatitude = newMax;
        } // else something else is off (i.e. longitude range)
      }
    }
    catch(IException &e) {
      std::string message = "Invalid label group [Mapping]";
      throw IException(e, IException::Io, message, _FILEINFO_);
    }
  }

  //! Destroys the Orthographic object
  Orthographic::~Orthographic() {
  }

  /**
   * Compares two Projection objects to see if they are equal
   *
   * @param proj Projection object to do comparison on
   *
   * @return bool Returns true if the Projection objects are equal, and false if
   *              they are not
   */
  bool Orthographic::operator== (const Projection &proj) {
    if (!Projection::operator==(proj)) return false;
    // dont do the below it is a recusive plunge
    Orthographic *ortho = (Orthographic *) &proj;
    if ((ortho->m_centerLongitude != m_centerLongitude) ||
        (ortho->m_centerLatitude != m_centerLatitude)) return false;
    return true;
  }

  /**
    * Returns the name of the map projection, "Orthographic"
    *
    * @return QString Name of projection, "Orthographic"
    */
   QString Orthographic::Name() const {
     return "Orthographic";
   }

   /**
    * Returns the version of the map projection
    *
    *
    * @return QString Version number
    */
   QString Orthographic::Version() const {
     return "1.0";
   }

   /**
    * Returns the center latitude, in degrees.
    *
    * **NOTE** In the case of Orthographic projections, there is NO latitude
    * that is entirely true to scale. The only true scale for this projection is
    * at the single point, (center latitude, center longitude).
    *
    * @return double The center latitude.
    */
   double Orthographic::TrueScaleLatitude() const {
     //Snyder pg. 45
     // no distortion at center of projection (centerLatitude, centerLongitude)
     return m_centerLatitude * 180.0 / PI;// Change to true scale
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
  bool Orthographic::SetGround(const double lat, const double lon) {
    // Convert longitude to radians & clean up
    m_longitude = lon;
    double lonRadians = lon * PI / 180.0;
    if (m_longitudeDirection == PositiveWest) lonRadians *= -1.0;

    // Now convert latitude to radians & clean up ... it must be planetographic
    m_latitude = lat;
    double latRadians = lat;
    if (IsPlanetocentric()) latRadians = ToPlanetographic(latRadians);
    latRadians *= PI / 180.0;

    // Compute helper variables
    double deltaLon = (lonRadians - m_centerLongitude);
    double sinphi = sin(latRadians);
    double cosphi = cos(latRadians);
    double coslon = cos(deltaLon);

    // Lat/Lon cannot be projected
    double g =  m_sinph0 * sinphi + m_cosph0 * cosphi * coslon;
    if ((g <= 0.0) && (fabs(g) > 1.0e-10)) {
      m_good = false;
      return m_good;
    }

    // Compute the coordinates
    double x = m_equatorialRadius * cosphi * sin(deltaLon);
    double y = m_equatorialRadius * (m_cosph0 * sinphi - m_sinph0 * cosphi * coslon);

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
  bool Orthographic::SetCoordinate(const double x, const double y) {
    // Save the coordinate
    SetXY(x, y);

    // Declare instance variables and calculate rho
    double rho, con, z, sinz, cosz;
    const double epsilon = 1.0e-10;
    rho = sqrt(GetX() * GetX() + GetY() * GetY());

    /* Error calculating rho - should be less than equatorial radius.
     *
     * This if statement included "fabs(rho - m_equatorialRadius) < 1E-10 ||"
     * to make the method more stable for limbs but this caused a false failure
     * for some images when rho == m_equatorialRadius.
     */
    if (rho > m_equatorialRadius) {
      m_good = false;
      return m_good;
    }

    // Calculate the latitude and longitude
    m_longitude = m_centerLongitude;
    if (fabs(rho) <= epsilon) {
      m_latitude = m_centerLatitude;
    }
    else {
      con = rho / m_equatorialRadius;
      if (con > 1.0) con = 1.0;
      if (con < -1.0) con = -1.0;
      z = asin(con);
      sinz = sin(z);
      cosz = cos(z);
      con = cosz * m_sinph0 + GetY() * sinz * m_cosph0 / rho;
      if (con > 1.0) con = 1.0;
      if (con < -1.0) con = -1.0;
      m_latitude = asin(con);
      con = fabs(m_centerLatitude) - HALFPI;
      if (fabs(con) <= epsilon) {
        if (m_centerLatitude >= 0.0) {
          m_longitude += atan2(GetX(), -GetY());
        }
        else {
          m_longitude += atan2(GetX(), GetY());
        }
      }
      else {
        con = cosz - m_sinph0 * sin(m_latitude);
        if ((fabs(con) >= epsilon) || (fabs(GetX()) >= epsilon)) {
          m_longitude += atan2(GetX() * sinz * m_cosph0, con * rho);
        }
      }
    }

    // Convert to degrees
    m_latitude *= 180.0 / PI;
    m_longitude *= 180.0 / PI;

    // Cleanup the longitude
    if (m_longitudeDirection == PositiveWest) m_longitude *= -1.0;

    /*
     * When the longitude range is 0 to 360 and the seam is within the 180 displayable degrees,
     * the longitude needs to be converted to its 360 lon domain counterpart. However, if the
     * range is shifted out of the 0 to 360 range, the conversion is not necessary. For example,
     * if the specified range is -180 to 180 and the clon is 0, the lon -90 is valid but will
     * be converted to 270, which does not work with the comparison. The same idea applies if
     * the range is 200 - 500 and the clon is 360. We want to display 270 to 450 (270 - 360 and
     * 0 - 90). However, if 450 is converted to the 360 domain it becomes 90 which is no longer
     * within the original 200 to 500 range.
     */
    // These need to be done for circular type projections
    m_longitude = To360Domain(m_longitude);
    if (m_longitudeDomain == 180) m_longitude = To180Domain(m_longitude);

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
  bool Orthographic::XYRange(double &minX, double &maxX,
                             double &minY, double &maxY) {
    double lat, lon;

    //Restrict lon range to be between -360 and 360
    double adjustedLon;
    double adjustedMinLon = To360Domain(MinimumLongitude());
    double adjustedMaxLon = To360Domain(MaximumLongitude());
    bool correctedMinLon = false;

    if (adjustedMinLon >= adjustedMaxLon) {
      adjustedMinLon -= 360;
      correctedMinLon = true;
    }

    // Check the corners of the lat/lon range
    XYRangeCheck(m_minimumLatitude, m_minimumLongitude);
    XYRangeCheck(m_maximumLatitude, m_minimumLongitude);
    XYRangeCheck(m_minimumLatitude, m_maximumLongitude);
    XYRangeCheck(m_maximumLatitude, m_maximumLongitude);

    // Walk top and bottom edges
    for (lat = m_minimumLatitude; lat <= m_maximumLatitude; lat += 0.01) {
      lon = m_minimumLongitude;
      XYRangeCheck(lat, lon);

      lon = m_maximumLongitude;
      XYRangeCheck(lat, lon);
    }

    // Walk left and right edges
    for (lon = m_minimumLongitude; lon <= m_maximumLongitude; lon += 0.01) {
      lat = m_minimumLatitude;
      XYRangeCheck(lat, lon);

      lat = m_maximumLatitude;
      XYRangeCheck(lat, lon);
    }

    /*
     * Walk the limb.
     * When the pair is valid and within the correct range, reassign min/max x/y.
     *
     *                      , - ~ ~ ~ - ,
     *                  , '               ' ,
     *               ,       _______         ,
     *               ,       |       |         , <----- Walking the limb would not affect an
     *              ,        |       |          ,       image that does not extend to the
     *              ,        |_______|          ,       limits of the projection.
     *              ,   ________________________,_
     *               , |                        , |
     *                ,|                       ,  |
     *                 |,___________________,_'___| <-- This corner would wrap around.
     *                    ' - , _ _ _ ,  '              The image would rotate because the
     *                                                  center lat was closer to the pole.
     *                                                  This would allow for a more completely
     *                                                  longitude range.
     *                           _o__o__
     *                         o|       |o <------------This is the rotated view. If the image
     *                        o |_______| o             limits extend over the pole (due to the
     *                        o     *     o             offset of the center lat) it is possible
     *                         o         o              to have a lon range that is larger than
     *                            o  o                  180 degrees.
     *
     */

    for (double angle = 0.0; angle <= 360.0; angle += 0.01) {
      double x = m_equatorialRadius * cos(angle * PI / 180.0);
      double y = m_equatorialRadius * sin(angle * PI / 180.0);

      if (SetCoordinate(x, y)){

        adjustedLon =    To360Domain(m_longitude);
        if (adjustedLon > To360Domain(MinimumLongitude()) && correctedMinLon) {
          adjustedLon -= 360;
        }
        if (m_latitude <= m_maximumLatitude &&
            adjustedLon <= adjustedMaxLon &&
            m_latitude >= m_minimumLatitude &&
            adjustedLon >= adjustedMinLon) {
          m_minimumX = qMin(x, m_minimumX);
          m_maximumX = qMax(x, m_maximumX);
          m_minimumY = qMin(y, m_minimumY);
          m_maximumY = qMax(y, m_maximumY);
          XYRangeCheck(m_latitude, adjustedLon);
        }
      }
    }
    // Make sure everything is ordered
    if (m_minimumX >= m_maximumX ||
        m_minimumY >= m_maximumY)
      return false;

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
  PvlGroup Orthographic::Mapping() {
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
  PvlGroup Orthographic::MappingLatitudes() {
    PvlGroup mapping = TProjection::MappingLatitudes();
    mapping += m_mappingGrp["CenterLatitude"];

    return mapping;
  }

  /**
   * This function returns the longitude keywords that this projection uses
   *
   * @return PvlGroup The longitude keywords that this projection uses
   */
  PvlGroup Orthographic::MappingLongitudes() {
    PvlGroup mapping = TProjection::MappingLongitudes();

    mapping += m_mappingGrp["CenterLongitude"];

    return mapping;
  }
} // end namespace isis

/**
 * This is the function that is called in order to instantiate an
 * Orthographic object.
 *
 * @param lab Cube labels with appropriate Mapping information.
 *
 * @param allowDefaults If the label does not contain the value for
 *                      CenterLongitude, this method indicates
 *                      whether the constructor should compute this value.
 *
 * @return @b Isis::Projection* Pointer to an Orthographic projection
 *         object.
 */
extern "C" Isis::Projection *OrthographicPlugin(Isis::Pvl &lab,
    bool allowDefaults) {
  return new Isis::Orthographic(lab, allowDefaults);
}
