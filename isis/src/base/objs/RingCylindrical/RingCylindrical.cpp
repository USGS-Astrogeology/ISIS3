/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "RingCylindrical.h"

#include <cmath>
#include <cfloat>

#include "Constants.h"
#include "IException.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "RingPlaneProjection.h"
#include "SpecialPixel.h"

using namespace std;
namespace Isis {

  /**
   * Constructs a RingCylindrical object.
   *
   * @param label This argument must be a Label containing the proper mapping
   *              information as indicated in the Projection class.
   *              Additionally, the ring cylindrical projection requires the
   *              center azimuth to be defined in the keyword CenterRingLongitude.
   *
   * @param allowDefaults If set to false, the constructor requires that the
   *                      keyword CenterRingLongitude exist in the label. Otherwise
   *                      if it does not exist it will be computed and written
   *                      to the label using the middle of the azimuth range
   *                      as specified in the labels. Defaults to false
   *
   * @throws IException
   */
  RingCylindrical::RingCylindrical(Pvl &label, bool allowDefaults) :
    RingPlaneProjection::RingPlaneProjection(label) {
    try {
      // Try to read the mapping group
      PvlGroup &mapGroup = label.findGroup("Mapping", Pvl::Traverse);

      // Compute the default value if allowed and needed
      if ((allowDefaults) && (!mapGroup.hasKeyword("CenterRingLongitude"))) {
        double az = (m_minimumRingLongitude + m_maximumRingLongitude) / 2.0;
        mapGroup += PvlKeyword("CenterRingLongitude", std::to_string(az));
      }

      // Compute and write the default center radius if allowed and
      // necessary
      if ((allowDefaults) && (!mapGroup.hasKeyword("CenterRingRadius"))) {
        double radius = (m_minimumRingRadius + m_maximumRingRadius) / 2.0;
        mapGroup += PvlKeyword("CenterRingRadius", std::to_string(radius));
      }

      // Get the center ring radius and center ring longitude.
      m_centerRingLongitude = mapGroup["CenterRingLongitude"];
      m_centerRingRadius = mapGroup["CenterRingRadius"];

      // Because the center radius is used to scale the y values of the projection, it cannot be 0.
      // For now, we will reset the center to the radius of Saturn.
      // Perhaps we should fail and issue an error message??? TODO
      if (m_centerRingRadius == 0.0) {
        m_centerRingRadius = 18000.;
      }

      //  Convert to radians, adjust for azimuth direction
      m_centerRingLongitude *= DEG2RAD;
      if (m_ringLongitudeDirection == Clockwise) m_centerRingLongitude *= -1.0;
    }
    catch (IException &e) {
      string message = "Invalid label group [Mapping]";
      throw IException(e, IException::Io, message, _FILEINFO_);
    }
  }

  //! Destroys the RingCylindrical object
  RingCylindrical::~RingCylindrical() {
  }

  /**
   * Compares two Projection objects to see if they are equal
   *
   * @param proj Projection object to do comparison on
   *
   * @return @b bool Returns true if the Projection objects are equal, and false
   *              if they are not
   */
  bool RingCylindrical::operator== (const Projection &proj) {
    if (!Projection::operator==(proj)) return false;
    // dont do the below it is a recusive plunge
    //  if (Projection::operator!=(proj)) return false;
    RingCylindrical *ringCyl = (RingCylindrical *) &proj;
    if ((ringCyl->m_centerRingLongitude != m_centerRingLongitude) ||
        (ringCyl->m_centerRingRadius != m_centerRingRadius)) return false;
    return true;
  }

  /**
   * Returns the name of the map projection, "RingCylindrical"
   *
   * @return @b QString Name of projection, "RingCylindrical"
   */
  QString RingCylindrical::Name() const {
    return "RingCylindrical";
  }

  /**
   * Returns the version of the map projection
   *
   * @return @b QString Version number
   */
  QString RingCylindrical::Version() const {
    return "1.0";
  }


  /**
   * This method returns true if the projection is
   *   equatorial cylindrical. In other words, if the
   *   projection is cylindrical and an image projected at 0 is
   *   the same as an image projected at 360.
   *
   *
   * @return @b bool true if the projection is equatorial
   *         cylindrical
   */
  bool RingCylindrical::IsEquatorialCylindrical() {
    return true;
  }


   /**
    * Returns the center radius, in meters.
    *
    * TODO: Correct this comment for planar projection, assuming right now scale
    * at center of projection.
    * (believe scale is uniform across planar projection)
    *
    * **NOTE** In the case of Planar projections, there is NO radius
    * that is entirely true to scale. The only true scale for this projection is
    * at the single point, (center radius, center azimuth).
    *
    * @return @b double The center radius.
    */
  double RingCylindrical::TrueScaleRingRadius() const {
    return m_centerRingRadius;
  }


   /**
    * Returns the center longitude, in degrees.
    *
    * @return @b double The center longitude.
    */
  double RingCylindrical::CenterRingLongitude() const {
    double dir = 1.0;
    if (m_ringLongitudeDirection == Clockwise) dir = -1.0;
    return m_centerRingLongitude * RAD2DEG * dir;
  }


   /**
    * Returns the center radius, in meters.
    *
    * @return @b double The center radius.
    */
  double RingCylindrical::CenterRingRadius() const {
    return m_centerRingRadius;
  }


  /**
   * This method is used to set the radius/longitude (assumed to be of the
   * correct RingLongitudeDirection, and RingLongitudeDomain. The Set
   * forces an attempted calculation of the projection X/Y values. This may or
   * may not be successful and a status is returned as such.
   *
   * @param ringRadius Ring radius value to project
   *
   * @param ringLongitude Ring longitude (azimuth) value to project
   *
   * @return @b bool Indicates whether the x and y values were successfully
   *         calculated and whether the ground values were successfully set.
   */
  bool RingCylindrical::SetGround(const double ringRadius, const double ringLongitude) {
    //TODO Add  scalar to make azimuth distance equivalent to radius distance at center rad
    // Convert to azimuth to radians and adjust
    m_ringLongitude = ringLongitude;
    double ringLongitudeRadians = ringLongitude * DEG2RAD;
    if (m_ringLongitudeDirection == Clockwise) ringLongitudeRadians *= -1.0;

    // Check to make sure radius is valid
    if (ringRadius <= 0) {
      m_good = false;
      // cout << "Unable to set radius. The given radius value ["
      //      << IString(ringRadius) << "] is invalid." << endl;
      // throw IException(IException::Unknown,
      //                  "Unable to set radius. The given radius value ["
      //                  + IString(ringRadius) + "] is invalid.",
      //                  _FILEINFO_);
      return m_good;
    }
    m_ringRadius = ringRadius;

    // Compute helper variable
    double deltaAz = (ringLongitudeRadians - m_centerRingLongitude);

    // Compute the coordinates
    if (ringRadius ==0) {
      // TODO How should we handle this case? We should use epsilon probably instead of 0 too
      m_good = false;
      return m_good;
    }
    double x = deltaAz *  m_centerRingRadius;
    double y =  m_centerRingRadius - ringRadius;
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
   * @return @b bool Indicates whether the ring radius and longitude were
   *         successfully calculated and whether the coordinate was
   *         successfully set.
   */
  bool RingCylindrical::SetCoordinate(const double x, const double y) {
    // Save the coordinate
    SetXY(x, y);

    // Compute radius and make sure it is valid
    // m_ringRadius = GetY() + m_centerRingRadius;
    m_ringRadius = m_centerRingRadius - GetY();
    if (m_ringRadius < m_minimumRingRadius || m_ringRadius > m_maximumRingRadius) {
      m_good = false;
      return m_good;
    }

    // Compute azimuth
    m_ringLongitude =  m_centerRingLongitude + GetX() / m_centerRingRadius;

    // Convert to degrees
    m_ringLongitude *= RAD2DEG;

    // Cleanup the azimuth
    if (m_ringLongitudeDirection == Clockwise) m_ringLongitude *= -1.0;
    // Do these if the projection is circular
    // m_ringLongitude = To360Domain (m_ringLongitude);
    // if (m_ringLongitudeDomain == 180) m_ringLongitude = To180Domain(m_ringLongitude);

    m_good = true;
    return m_good;
  }


  /**
   * This function returns the keywords that this projection uses. For
   * RingCylindrical, this is the CenterRingRadius and CenterRingLongitude.
   *
   * @return @b PvlGroup The keywords that this projection uses
   */
  PvlGroup RingCylindrical::Mapping() {
    PvlGroup mapping = RingPlaneProjection::Mapping();

    mapping += PvlKeyword("CenterRingRadius", std::to_string(m_centerRingRadius));
    double dir = 1.0;
    if (m_ringLongitudeDirection == Clockwise) dir = -1.0;
    double lonDegrees = m_centerRingLongitude*RAD2DEG*dir;
    mapping += PvlKeyword("CenterRingLongitude", std::to_string(lonDegrees));

    return mapping;
  }

  /**
   * This function returns the radii keywords that this projection uses. For
   * RingCylindrical, this is the CenterRingRadius.
   *
   * @return @b PvlGroup The latitude keywords that this projection uses
   */
  PvlGroup RingCylindrical::MappingRingRadii() {
    PvlGroup mapping = RingPlaneProjection::MappingRingRadii();

    if (HasGroundRange())
      mapping += m_mappingGrp["CenterRingRadius"];

    return mapping;
  }


  /**
   * This function returns the longitude keywords that this projection uses. For
   * RingCylindrical, this is the CenterRingLongitude.
   *
   * @return @b PvlGroup The ring longitude keywords that this projection uses
   */
  PvlGroup RingCylindrical::MappingRingLongitudes() {
    PvlGroup mapping = RingPlaneProjection::MappingRingLongitudes();

    if (HasGroundRange())
      mapping += m_mappingGrp["CenterRingLongitude"];

    return mapping;
  }


      /**
       * This method is used to determine the x/y range which completely covers the
       * area of interest specified by the radius/lon range. The radius/longitude
       * range may be obtained from the labels. This method should not be used if
       * HasGroundRange is false. The purpose of this method is to return the x/y
       * range so it can be used to compute how large a map may need to be. For
       * example, how big a piece of paper is needed or how large of an image needs
       * to be created. This is method and therefore must be written by the derived
       * class (e.g., RingCylindrical). The method may fail as indicated by its return
       * value.
       *
       *
       * @param &minX Reference to the address where the minimum x
       *             coordinate value will be written.  The Minimum x projection
       *             coordinate calculated by this method covers the
       *             radius/longitude range specified in the labels.
       *
       * @param &maxX Reference to the address where the maximum x
       *             coordinate value will be written.  The Maximum x projection
       *             coordinate calculated by this method covers the
       *             radius/longitude range specified in the labels.
       *
       * @param &minY Reference to the address where the minimum y
       *             coordinate value will be written.  The Minimum y projection
       *             coordinate calculated by this method covers the
       *             radius/longitude range specified in the labels.
       *
       * @param &maxY Reference to the address where the maximum y
       *             coordinate value will be written.  The Maximum y projection
       *             coordinate calculated by this method covers the
       *             radius/longitude range specified in the labels.
       *
       * @return @b bool Indicates whether the method was able to determine the X/Y
       *              Range of the projection.  If yes, minX, maxX, minY, maxY will
       *              be set with these values.
       *
       */
      bool RingCylindrical::XYRange(double &minX, double &maxX,
                               double &minY, double &maxY) {

        double rad, az;

        // Check the corners of the rad/az range
        XYRangeCheck(m_minimumRingRadius, m_minimumRingLongitude);
        XYRangeCheck(m_maximumRingRadius, m_minimumRingLongitude);
        XYRangeCheck(m_minimumRingRadius, m_maximumRingLongitude);
        XYRangeCheck(m_maximumRingRadius, m_maximumRingLongitude);

        // Walk top and bottom edges in half pixel increments
        double radiusInc = 2. * (m_maximumRingRadius - m_minimumRingRadius) / PixelResolution();

        for (rad = m_minimumRingRadius; rad <= m_maximumRingRadius; rad += radiusInc) {
          az = m_minimumRingLongitude;
          XYRangeCheck(rad, az);

          az = m_maximumRingLongitude;
          XYRangeCheck(rad, az);
        }

        // Walk left and right edges
        for (az = m_minimumRingLongitude; az <= m_maximumRingLongitude; az += 0.01) {
          rad = m_minimumRingRadius;
          XYRangeCheck(rad, az);

          rad = m_maximumRingRadius;
          XYRangeCheck(rad, az);
        }

        // Make sure everything is ordered
        if (m_minimumX >= m_maximumX) return false;
        if (m_minimumY >= m_maximumY) return false;

        // Return X/Y min/maxs
        // m_maximumX = m_maximumRingRadius*cos(m_maximumRingLongitude);
        // m_minimumX = -m_maximumX;
        // m_maximumY = m_maximumRingRadius*sin(m_maximumRingLongitude);
        // m_minimumY = -m_maximumY;

        minX = m_minimumX;
        maxX = m_maximumX;
        minY = m_minimumY;
        maxY = m_maximumY;

        return true;
      }
} // end namespace isis

/**
 * This is the function that is called in order to instantiate a
 * RingCylindrical object.
 *
 * @param lab Cube labels with appropriate Mapping information.
 *
 * @param allowDefaults Indicates whether CenterLongitude is allowed to be
 *                      computed.
 *
 * @return @b Isis::Projection* Pointer to a RingCylindrical
 *                              projection object.
 */
extern "C" Isis::Projection *RingCylindricalPlugin(Isis::Pvl &lab,
    bool allowDefaults) {
  return new Isis::RingCylindrical(lab, allowDefaults);
}
