/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Robinson.h"

#include <cmath>
#include <cfloat>

#include "Constants.h"
#include "IException.h"
#include "Projection.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"

const double EPSILON = 1.0e-10;

using namespace std;
namespace Isis {

  /**
   * Constructs a Robinson object.
   *
   * @param label This argument must be a Label containing the proper mapping
   *              information as indicated in the Projection class. Additionally,
   *              the robinson projection requires the center longitude to be
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
  Robinson::Robinson(Pvl &label, bool allowDefaults) :
      TProjection::TProjection(label) {
    try {

      // Initialize coefficients
      // PR  Add xtra element at beginning to mimic 1-based arrays to match Snyder's code for
      // easier debugging.
      m_pr << 0 << -.062 <<
                         0 <<
                     0.062 <<
                     0.124 <<
                     0.186 <<
                     0.248 <<
                     0.310 <<
                     0.372 <<
                     0.434 <<
                     0.4958 <<
                     0.5571 <<
                     0.6176 <<
                     0.6769 <<
                     0.7346 <<
                     0.7903 <<
                     0.8435 <<
                     0.8936 <<
                     0.9394 <<
                     0.9761 <<
                     1.;
      // XLR
      m_xlr << 0 << 0.9986 <<
                          1.0 <<
                       0.9986 <<
                       0.9954 <<
                         0.99 <<
                       0.9822 <<
                        0.973 << 
                         0.96 <<
                       0.9427 <<
                       0.9216 <<
                       0.8962 <<
                       0.8679 <<
                        0.835 << 
                       0.7986 <<
                       0.7597 <<
                       0.7186 <<
                       0.6732 <<
                       0.6213 <<
                       0.5722 <<
                       0.5322;

      // Try to read the mapping group
      PvlGroup &mapGroup = label.findGroup("Mapping", Pvl::Traverse);

      // Compute and write the default center longitude if allowed and
      // necessary
      if ((allowDefaults) && (!mapGroup.hasKeyword("CenterLongitude"))) {
        double lon = (m_minimumLongitude + m_maximumLongitude) / 2.0;
        mapGroup += PvlKeyword("CenterLongitude", std::to_string(lon));
      }

      // Get the center longitude
      m_centerLongitude = mapGroup["CenterLongitude"];

      // convert to radians, adjust for longitude direction
      m_centerLongitude *= DEG2RAD;
      if (m_longitudeDirection == PositiveWest) m_centerLongitude *= -1.0;
    }
    catch(IException &e) {
      QString message = "Invalid label group [Mapping]";
      throw IException(e, IException::Io, message, _FILEINFO_);
    }
  }

  //! Destroys the Robinson object
  Robinson::~Robinson() {
  }

  /**
   * Compares two Projection objects to see if they are equal
   *
   * @param proj Projection object to do comparison on
   *
   * @return bool Returns true if the Projection objects are equal, and false if
   *              they are not
   */
  bool Robinson::operator== (const Projection &proj) {
    if (!Projection::operator==(proj)) return false;
    // dont do the below it is a recusive plunge
    //  if (Projection::operator!=(proj)) return false;
    Robinson *robin = (Robinson *) &proj;
    if (robin->m_centerLongitude != m_centerLongitude) return false;
    return true;
  }

  /**
   * Returns the name of the map projection, "Robinson"
   *
   * @return QString Name of projection, "Robinson"
   */
  QString Robinson::Name() const {
    return "Robinson";
  }

  /**
   * Returns the version of the map projection
   *
   *
   * @return QString Version number
   */
  QString Robinson::Version() const {
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
  bool Robinson::SetGround(const double lat, const double lon) {
    // Convert to radians
    m_latitude = lat;
    m_longitude = lon;
    double latRadians = lat * DEG2RAD;
    double lonRadians = lon * DEG2RAD;
    if (m_longitudeDirection == PositiveWest) lonRadians *= -1.0;

    // Compute the coordinate
    double deltaLon = (lonRadians - m_centerLongitude);
    double p2 = fabs(latRadians / 5.0 / DEG2RAD);
    long ip1 = (long) (p2 - EPSILON);
    if (ip1 > 17) {
      return false;
    }

    // Stirling's interpolation formula (using 2nd Diff.)
    //
    p2 -= (double) ip1;
    double x = 0.8487 * m_equatorialRadius * (m_xlr[ip1 + 2] + p2 * (m_xlr[ip1 + 3] -
                         m_xlr[ip1 + 1]) / 2.0 +
                         p2 * p2 * (m_xlr[ip1 + 3] - 2.0 * m_xlr[ip1 + 2] +
                         m_xlr[ip1 + 1])/2.0) * deltaLon;

    double y = 1.3523 * m_equatorialRadius * (m_pr[ip1 + 2] + p2 * (m_pr[ip1 + 3] -
                         m_pr[ip1 +1]) / 2.0 + p2 * p2 * (m_pr[ip1 + 3] -
                         2.0 * m_pr[ip1 + 2] + m_pr[ip1 + 1]) / 2.0);
    if (lat < 0) y *= -1.;

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
  bool Robinson::SetCoordinate(const double x, const double y) {
    m_latitude = 0;
    m_longitude = 0;
    // Save the coordinate
    SetXY(x, y);

    double yy = y / m_equatorialRadius / 1.3523;
    double phid = yy * 90.0;
    double p2 = fabs(phid / 5.0);
    long ip1 = (long) (p2 - EPSILON);
    if (ip1 == 0) ip1 = 1;
    if (ip1 > 17) {
      return false;
    }

    // Stirling's interpolation formula as used in forward transformation is 
    //   reversed for first estimation of LAT. from rectangular coordinates. LAT.
    //   is then adjusted by iteration until use of forward series provides correct 
    //   value of Y within tolerance.
    //
    double u, v, t, c;
    double y1;

    for (int i = 0;;) {
      u = m_pr[ip1 + 3] - m_pr[ip1 + 1];
      v = m_pr[ip1 + 3] - 2.0 * m_pr[ip1 + 2] + m_pr[ip1 + 1];
      t = 2.0 * (fabs(yy) - m_pr[ip1 + 2]) / u;
      c = v / u;
      p2 = t * (1.0 - c * t * (1.0 - 2.0 * c * t));

      if ((p2 >= 0.0) || (ip1 == 1)) {
        phid = (p2 + (double) ip1 ) * 5.0;
        if (y < 0) phid *= -1;

        do {
          p2 = fabs(phid / 5.0);
          ip1 = (long) (p2 - EPSILON);
          if (ip1 > 17) {
            return false;
          }
          p2 -= (double) ip1;

          y1 = 1.3523 * m_equatorialRadius * (m_pr[ip1 +2] + p2 *(m_pr[ip1 + 3] -
                        m_pr[ip1 +1]) / 2.0 + p2 * p2 * (m_pr[ip1 + 3] -
                        2.0 * m_pr[ip1 + 2] + m_pr[ip1 + 1])/2.0);
          if (y < 0) y1 *= -1.;

          phid -= (90.0 * (y1 - y) / m_equatorialRadius / 1.3523);
          i++;
          if (i > 75) {
            return false;
          }
        } while (fabs(y1 - y) > .00001);
        break;
      }
      else {
        ip1--;
        if (ip1 < 0) {
          return false;
        }
      }
    }

    // Compute latitude and make sure it is not above 90
    m_latitude  = phid;

    // Calculate  longitude. using final latitude. with transposed forward Stirling's 
    //   interpolation formula.
    m_longitude = m_centerLongitude + x / m_equatorialRadius / 0.8487 / (m_xlr[ip1 + 2] +
                          p2 * (m_xlr[ip1 + 3] - m_xlr[ip1 + 1]) / 2.0 +
                          p2 * p2 * (m_xlr[ip1 + 3] - 2.0 * m_xlr[ip1 + 2] + 
                          m_xlr[ip1 + 1]) / 2.0);
    m_longitude *= RAD2DEG;
    if (m_longitudeDirection == PositiveWest) m_longitude *= -1.0;

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
  bool Robinson::XYRange(double &minX, double &maxX,
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
  PvlGroup Robinson::Mapping()  {
    PvlGroup mapping = TProjection::Mapping();

    mapping += m_mappingGrp["CenterLongitude"];

    return mapping;
  }

  /**
   * This function returns the latitude keywords that this projection uses
   *
   * @return PvlGroup The latitude keywords that this projection uses
   */
  PvlGroup Robinson::MappingLatitudes() {
    PvlGroup mapping = TProjection::MappingLatitudes();

    return mapping;
  }

  /**
   * This function returns the longitude keywords that this projection uses
   *
   * @return PvlGroup The longitude keywords that this projection uses
   */
  PvlGroup Robinson::MappingLongitudes() {
    PvlGroup mapping = TProjection::MappingLongitudes();

    mapping += m_mappingGrp["CenterLongitude"];

    return mapping;
  }

} // end namespace isis

/** 
 * This is the function that is called in order to instantiate a 
 * Robinson object.
 *  
 * @param lab Cube labels with appropriate Mapping information.
 *  
 * @param allowDefaults Indicates whether CenterLongitude are allowed to 
 *                      be computed using the middle of the longitude
 *                      range specified in the labels.
 * 
 * @return @b Isis::Projection* Pointer to a Robinson projection object.
 */
extern "C" Isis::Projection *RobinsonPlugin(Isis::Pvl &lab,
    bool allowDefaults) {
  return new Isis::Robinson(lab, allowDefaults);
}
