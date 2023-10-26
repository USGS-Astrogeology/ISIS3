/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "PolarStereographic.h"

#include <cmath>
#include <cfloat>
#include <iostream>

#include "Constants.h"
#include "IException.h"
#include "TProjection.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"

using namespace std;
namespace Isis {
  /**
   * Constructs a PolarStereographic object.
   *
   * @param label This argument must be a Pvl containing the proper mapping
   *              information as indicated in the Projection class. Additionally,
   *              the polar stereographic projection requires the center
   *              longitude and latitude to be defined in the keyword
   *              CenterLongitude and CenterLatitude respectively.
   *
   * @param allowDefaults If set to false the constructor expects that a keyword
   *                      of CenterLongitude and CenterLatitude will be in the
   *                      label. Otherwise it will attempt to compute them using
   *                      the middle of the longitude range specified in the
   *                      labels. The center latitude will be set to one of the
   *                      poles depending on the average of the latitude range.
   *                      Defaults to false.
   *
   * @throws IException - An error is thrown if the label does not contain 
   *             the keyword CenterLongitude or latitude.
   */
  PolarStereographic::PolarStereographic(Pvl &label, bool allowDefaults) :
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
        if (lat > 0.0) {
          mapGroup += PvlKeyword("CenterLatitude", std::to_string(90.0));
        }
        else {
          mapGroup += PvlKeyword("CenterLatitude", std::to_string(-90.0));
        }
      }

      // Get the center longitude, convert to radians and adjust for longitude
      // direction
      m_centerLongitude = mapGroup["CenterLongitude"];
      m_centerLongitude *= PI / 180.0;
      if (m_longitudeDirection == PositiveWest) m_centerLongitude *= -1.0;

      // Get the center latitude, make sure it is ographic, and convert to
      // radians.
      m_centerLatitude = mapGroup["CenterLatitude"];
      if (m_centerLatitude == 0) {
        QString msg = "Invalid value for keyword [CenterLatitude] in map file.";
        msg += "  CenterLatitude cannot equal 0.0";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      if (IsPlanetocentric()) {
        m_centerLatitude = ToPlanetographic(m_centerLatitude);
      }
      m_centerLatitude *= PI / 180.0;

      // Compute some constants
      m_e4 = e4Compute();
      m_signFactor = 1.0;
      if (m_centerLatitude < 0.0) m_signFactor = -1.0;

      if ((HALFPI - fabs(m_centerLatitude)) > DBL_EPSILON) {
        m_poleFlag = true;  // We are not at a pole
        double phi = m_signFactor * m_centerLatitude;
        double sinphi = sin(phi);
        double cosphi = cos(phi);
        m_m = mCompute(sinphi, cosphi);
        m_t = tCompute(phi, sinphi);
        if (fabs(m_t) < DBL_EPSILON) m_poleFlag = false;
      }
      else {
        m_poleFlag = false;  // Implies we are at a pole
        m_m = 0; 
        m_t = 0;
      }
    }
    catch(IException &e) {
      QString message = "Invalid label group [Mapping]";
      throw IException(e, IException::Unknown, message, _FILEINFO_);
    }
  }
  

  //! Destroys the PolarStereographic object
  PolarStereographic::~PolarStereographic() {
  }
  

  /**
   * Compares two Projection objects to see if they are equal
   *
   * @param proj Projection object to do comparison on
   *
   * @return bool Returns true if the Projection objects are equal, and false if
   *              they are not
   */
  bool PolarStereographic::operator== (const Projection &proj) {
    if (!Projection::operator==(proj)) return false;
    // dont do the below it is a recusive plunge
    //  if (Projection::operator!=(proj)) return false;
    PolarStereographic *pola = (PolarStereographic *) &proj;
    if (pola->m_centerLongitude != m_centerLongitude) return false;
    if (pola->m_centerLatitude != m_centerLatitude) return false;
    return true;
  }
  

  /**
   * Returns the name of the map projection, "PolarStereographic"
   *
   * @return QString Name of projection, "PolarStereographic"
   */
  QString PolarStereographic::Name() const {
    return "PolarStereographic";
  }

  
  /**
   * Returns the version of the map projection
   *
   *
   * @return QString Version number
   */
  QString PolarStereographic::Version() const {
    return "1.0";
  }

  
  /**
   * Returns the latitude of true scale.  Note that in the case of Polar 
   * Stereographic, the only true scale point is at the pole around 
   * which the projection is centered.  Scale increases away from the 
   * center point. 
   *
   * @return double
   */
  double PolarStereographic::TrueScaleLatitude() const {
    return m_centerLatitude * 180.0 / PI;
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
  bool PolarStereographic::SetGround(const double lat, const double lon) {
    // Fix up longitude
    m_longitude = lon;
    double lonRadians = lon * PI / 180.0;
    if (m_longitudeDirection == PositiveWest) lonRadians *= -1.0;

    // Now do latitude ... it must be planetographic
    m_latitude = lat;
    double latRadians = lat;
    if (IsPlanetocentric()) latRadians = ToPlanetographic(latRadians);
    latRadians = latRadians * PI / 180.0;

    // Compute easting and northing
    double lamda = m_signFactor * (lonRadians - m_centerLongitude);
    double phi = m_signFactor * latRadians;
    double sinphi = sin(phi);
    double t = tCompute(phi, sinphi);

    double dist;
    if (m_poleFlag) {
      dist = m_equatorialRadius * m_m * t / m_t;
    }
    else {
      dist = m_equatorialRadius * 2.0 * t / m_e4;
    }

    double x = m_signFactor * dist * sin(lamda);
    double y = -(m_signFactor * dist * cos(lamda));
    SetComputedXY(x, y);

    m_good = true;

    //So we don't project the wrong pole.
    if (qFuzzyCompare(lat * m_signFactor, -90.0))
      m_good = false;

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
  bool PolarStereographic::SetCoordinate(const double x, const double y) {
    // Save the coordinate
    SetXY(x, y);

    double east = m_signFactor * GetX();
    double north = m_signFactor * GetY();
    double dist = sqrt(east * east + north * north);

    double t;
    if (m_poleFlag) {
      t = dist * m_t / (m_m * m_equatorialRadius); // Snyder eqn (21-40)
    }
    else {
      t = dist * m_e4 / (2.0 * m_equatorialRadius); //Snyder eqn (24-39) 
                                                    //when latitude of true scale is North Polar
    }

    // Compute the latitude
    double phi = phi2Compute(t);
    m_latitude = m_signFactor * phi;

    if (fabs(m_latitude) > HALFPI) {
      QString msg = "X,Y causes latitude to be outside [-90,90] "
                   "in PolarStereographic Class";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Compute the longitude
    if (dist == 0.0) {
      m_longitude = m_signFactor * m_centerLongitude;
    }
    else {
      m_longitude = m_signFactor * atan2(east, -north) + m_centerLongitude;
    }

    // Cleanup the longitude
    m_longitude *= 180.0 / PI;
    if (m_longitudeDirection == PositiveWest) m_longitude *= -1.0;
    m_longitude = To360Domain(m_longitude);
    if (m_longitudeDomain == 180) m_longitude = To180Domain(m_longitude);

    // Cleanup the latitude
    m_latitude *= 180.0 / PI;
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
  bool PolarStereographic::XYRange(double &minX, double &maxX,
                                   double &minY, double &maxY) {
    // Check the corners of the lat/lon range
    XYRangeCheck(m_minimumLatitude, m_minimumLongitude);
    XYRangeCheck(m_maximumLatitude, m_minimumLongitude);
    XYRangeCheck(m_minimumLatitude, m_maximumLongitude);
    XYRangeCheck(m_maximumLatitude, m_maximumLongitude);

    // Find the closest longitude >= to the minimum longitude that is offset
    // from the center longitude by a multiple of 90.
    double lon1 = m_centerLongitude * 180.0 / PI;
    if (m_longitudeDirection == PositiveWest) lon1 *= -1.0;
    while(lon1 > m_minimumLongitude) lon1 -= 90.0;
    while(lon1 < m_minimumLongitude) lon1 += 90.0;

    while(lon1 <= m_maximumLongitude) {
      XYRangeCheck(m_minimumLatitude, lon1);
      XYRangeCheck(m_maximumLatitude, lon1);
      lon1 += 90.0;
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
  PvlGroup PolarStereographic::Mapping() {
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
  PvlGroup PolarStereographic::MappingLatitudes() {
    PvlGroup mapping = TProjection::MappingLatitudes();

    mapping += m_mappingGrp["CenterLatitude"];

    return mapping;
  }

  
  /**
   * This function returns the longitude keywords that this projection uses
   *
   * @return PvlGroup The longitude keywords that this projection uses
   */
  PvlGroup PolarStereographic::MappingLongitudes() {
    PvlGroup mapping = TProjection::MappingLongitudes();

    mapping += m_mappingGrp["CenterLongitude"];

    return mapping;
  }
}


/**
 * This is the function that is called in order to instantiate an 
 * PolarStereographic object.
 *  
 * @param lab Cube labels with appropriate Mapping information.
 *  
 * @param allowDefaults If the label does not contain the values for 
 *                      CenterLongitude and CenterLatitude, this method
 *                      indicates whether the constructor should compute
 *                      these values.
 * 
 * @return @b Isis::Projection* Pointer to a PolarStereographic 
 *                  projection object.
 */
extern "C" Isis::Projection *PolarStereographicPlugin(Isis::Pvl &lab,
    bool allowDefaults) {
  return new Isis::PolarStereographic(lab, allowDefaults);
}
