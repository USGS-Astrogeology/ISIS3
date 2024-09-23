/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Mollweide.h"


#include <cfloat>
#include <cmath>
#include <iomanip>

#include "Constants.h"
#include "IException.h"
#include "TProjection.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"

using namespace std;


namespace Isis {


  /**
   * @brief Constructs a Mollweide object.
   *
   * @param label This argument must be a Label containing the proper mapping
   *              information as indicated in the Projection class. Additionally,
   *              the Mollweide projection requires the center longitude and the
   *              equatorial radius to be defined in the keyword CenterLongitude
   *              and EquatorialRadius.
   *
   * @param allowDefaults If set to false the constructor expects that a keyword
   *                      of CenterLongitude will be in the label. Otherwise it
   *                      will attempt to compute the center longitude using the
   *                      middle of the longitude range specified in the labels.
   *                      Defaults to false
   *
   * @throws IException
   */  
  Mollweide::Mollweide(Pvl &label, bool allowDefaults) :
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

  //! Destroys the Mollweide object
  Mollweide::~Mollweide() {
  }


  /**
   * @brief Compares two Projection objects to see if they are equal
   *
   * @param proj Projection object to do comparison on
   *
   * @return bool Returns true if the Projection objects are equal, and false if
   *              they are not
   */
  bool Mollweide::operator== (const Projection &proj) {
    if (!TProjection::operator==(proj)) return false;
    // dont do the below it is a recursive plunge
    //  if (TProjection::operator!=(proj)) return false;
    Mollweide *moll = (Mollweide *) &proj;
    if (moll->m_centerLongitude != m_centerLongitude) return false;
    return true;
  }


  /**
   * @brief Returns the name of the map projection, "Mollweide"
   *
   * @return QString Name of projection, "Mollweide"
   */
  QString Mollweide::Name() const {
    return "Mollweide";
  }


  /**
   * @brief Returns the version of the map projection
   * @return QString Version number
   */
  QString Mollweide::Version() const {
    return "1.0";
  }


  /**
   * @brief The Newton-Rapheson method is used to find an iterative solution for:
   *
   *       @f[ 2\theta+\sin(2\theta) = \pi \sin(\phi) @f]
   *
   * Where:
   *
   * @f{eqnarray*}
   * &\theta = \text{ The\;\;auxiliary\;\;variable\;\;being\;\;solved\;\;for.}\\
   * &\phi = \text{ The\;\;latitude\;\;(in radians).}\\
   * @f]
   *
   * This method achieves rapid convergence for small latitudes, and slower convergence near
   * the poles.
   *
   * @param phi The latitude value.
   * @param result  The final theta value.
   * @return @b bool Returns true if the method converges, and false if it does not.
   */
  bool Mollweide::newton_rapheson(double phi, double &result) {
    
    double dtheta = 1.0;
    int niter = 0;
    double theta[2];
  
    theta[0] = asin(2*phi/PI);
    theta[1]=0.0;
    
    //If this condition is too strict, a larger epsilon value than DBL_EPSILON
    //can be used to decrease the number of iterations.
    while (dtheta > DBL_EPSILON) {

      theta[1] = theta[0] - (2*theta[0]+sin(2*theta[0]) -(Isis::PI)*sin(phi))/(2+2*cos(theta[0]));
      dtheta = fabs(theta[1]-theta[0]);
      theta[0] = theta[1];
      niter++;
      
      if (niter > 15000000) {
        //cout << setprecision(10) << phi*(180/PI) << "," << niter << endl;
        return false;
      }
    }
    result = theta[1];


    //cout << setprecision(10) << phi*(180/PI) << "," << niter << endl;
    return true;
  }


  /**
   * @brief Set lat/lon and attempt to calculate x/y values
   *
   * This method is used to set the latitude/longitude (assumed to be of the
   * correct LatitudeType, LongitudeDirection, and LongitudeDomain). The Set
   * forces an attempted calculation of the projection X/Y values. This may or
   * may not be successful and a status is returned as such.
   *
   * @param lat Latitude value to project
   *
   * @param lon Longitude value to project
   *
   * @return @b bool Returns true if successful, false otherwise.
   */
  bool Mollweide::SetGround(const double lat, const double lon) {

    // Convert to radians   
    m_latitude = lat;
    m_longitude = lon;
    double theta;
    double latRadians = lat * PI / 180.0;
    double lonRadians = lon * PI / 180.0;
    if (m_longitudeDirection == PositiveWest) lonRadians *= -1.0;

    // Compute the coordinate
    double deltaLon = lonRadians - m_centerLongitude;

    if (newton_rapheson(latRadians,theta) ) {

      double x = (2*sqrt(2)/PI )*m_equatorialRadius*(deltaLon)*cos(theta);   
      double y = sqrt(2)*m_equatorialRadius*sin(theta);
      
      SetComputedXY(x, y);
      m_good = true;
      return m_good;
    }
    else {

      m_good = false;
      return m_good;
    }
  }


  /**
   * @brief This method is used to set the projection x/y. The Set forces an attempted
   * calculation of the corresponding latitude/longitude position. This may or
   * may not be successful and a status is returned as such.
   *
   * @param x X coordinate of the projection in units that are the same as the
   *          radii in the label
   *
   * @param y Y coordinate of the projection in units that are the same as the
   *          radii in the label
   *
   * @return @b bool Returns true if successful, false otherwise.
   */
  bool Mollweide::SetCoordinate(const double x, const double y) {
    // Save the coordinate
    
    SetXY(x, y);
  
    double theta = asin(y/(m_equatorialRadius*sqrt(2)));

    // Compute latitude and make sure it is not above 90
    m_latitude = asin((2*theta+sin(2*theta))/(Isis::PI));

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

    double cosLat = cos(m_latitude);

    if (cosLat <= DBL_EPSILON) {
      m_longitude = m_centerLongitude;
    }

    else {
    m_longitude = m_centerLongitude+(Isis::PI)*GetX()/(2*m_equatorialRadius*sqrt(2)*cos(theta));
    }


    // Convert to degrees
    m_latitude *= 180.0 / PI;
    m_longitude *= 180.0 / PI;

    // Cleanup the longitude
    if (m_longitudeDirection == PositiveWest) m_longitude *= -1.0;


    // Our double precision is not good once we pass a certain magnitude of
    //   longitude. Prevent failures down the road by failing now.
    m_good = (fabs(m_longitude) < 1E10);


    return m_good;
  }


  /**
   * @brief Find x/y range from lat/lon range
   *
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
   * @return @b bool Returns true if successful, false otherwise.
   */
  bool Mollweide::XYRange(double &minX, double &maxX,
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
   * @return @b PvlGroup The keywords that this projection uses.
   */
  PvlGroup Mollweide::Mapping()  {
    PvlGroup mapping = TProjection::Mapping();

    mapping += m_mappingGrp["CenterLongitude"];

    return mapping;
  }


  /**
   * This function returns the latitude keywords that this projection uses.
   *
   * @return @b PvlGroup The latitude keywords that this projection uses.
   */
  PvlGroup Mollweide::MappingLatitudes() {
    PvlGroup mapping = TProjection::MappingLatitudes();

    return mapping;
  }


  /**
   * This function returns the longitude keywords that this projection uses.
   *
   * @return @b PvlGroup The longitude keywords that this projection uses
   */
  PvlGroup Mollweide::MappingLongitudes() {
    PvlGroup mapping = TProjection::MappingLongitudes();

    mapping += m_mappingGrp["CenterLongitude"];

    return mapping;
  }

} // end namespace isis


/** 
 * This is the function that is called in order to instantiate a 
 * Mollweide object.
 *  
 * @param lab Cube labels with appropriate Mapping information.
 *  
 * @param allowDefaults Indicates whether CenterLongitude are allowed to 
 *                      be computed using the middle of the longitude
 *                      range specified in the labels.
 * 
 * @return @b Isis::Projection* Pointer to a Mollweide projection object.
 */
extern "C" Isis::TProjection *MollweidePlugin(Isis::Pvl &lab,
    bool allowDefaults) {
  return new Isis::Mollweide(lab, allowDefaults);
}
