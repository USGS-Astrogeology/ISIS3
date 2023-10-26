/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "ObliqueCylindrical.h"

#include <cfloat>
#include <cmath>

#include <SpiceUsr.h>

#include "Constants.h"
#include "IException.h"
#include "TProjection.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "NaifStatus.h"

using namespace std;
namespace Isis {
  /**
   * Constructs an ObliqueCylindrical object.
   *
   * @param label This argument must be a Label containing the proper mapping
   *              information as indicated in the Projection class.
   *              Additionally, the Oblique cylindrical projection
   *              requires the Pole latitude, longitude and
   *              rotation.
   *
   * @param allowDefaults This does nothing currently
   *
   * @throws IException
   */
  ObliqueCylindrical::ObliqueCylindrical(Pvl &label, bool allowDefaults) :
    TProjection::TProjection(label) {
    try {
      // Try to read the mapping group
      PvlGroup &mapGroup = label.findGroup("Mapping", Pvl::Traverse);

      m_poleLatitude = mapGroup["PoleLatitude"];

      // All latitudes must be planetographic
      if (IsPlanetocentric()) {
        m_poleLatitude = ToPlanetographic(m_poleLatitude);
      }

      if (m_poleLatitude < -90 || m_poleLatitude > 90) {
        throw IException(IException::Unknown,
                         "Pole latitude must be between -90 and 90.",
                         _FILEINFO_);
      }

      m_poleLongitude = mapGroup["PoleLongitude"];

      if (m_poleLongitude < -360 || m_poleLongitude > 360) {
        throw IException(IException::Unknown,
                         "Pole longitude must be between -360 and 360.",
                         _FILEINFO_);
      }

      m_poleRotation = mapGroup["PoleRotation"];

      if (m_poleRotation < -360 || m_poleRotation > 360) {
        throw IException(IException::Unknown,
                         "Pole rotation must be between -360 and 360.",
                         _FILEINFO_);
      }

      bool calculateVectors = false;

      // Check vectors for the right array size
      if (!mapGroup.hasKeyword("XAxisVector") 
          || mapGroup["XAxisVector"].size() != 3) {
        calculateVectors = true;
      }

      if (!mapGroup.hasKeyword("YAxisVector") 
          || mapGroup["YAxisVector"].size() != 3) {
        calculateVectors = true;
      }

      if (!mapGroup.hasKeyword("ZAxisVector") 
          || mapGroup["ZAxisVector"].size() != 3) {
        calculateVectors = true;
      }

      if (!calculateVectors) {
        // Read in vectors
        m_xAxisVector.push_back(std::stod(mapGroup["XAxisVector"][0]));
        m_xAxisVector.push_back(std::stod(mapGroup["XAxisVector"][1]));
        m_xAxisVector.push_back(std::stod(mapGroup["XAxisVector"][2]));

        m_yAxisVector.push_back(std::stod(mapGroup["YAxisVector"][0]));
        m_yAxisVector.push_back(std::stod(mapGroup["YAxisVector"][1]));
        m_yAxisVector.push_back(std::stod(mapGroup["YAxisVector"][2]));

        m_zAxisVector.push_back(std::stod(mapGroup["ZAxisVector"][0]));
        m_zAxisVector.push_back(std::stod(mapGroup["ZAxisVector"][1]));
        m_zAxisVector.push_back(std::stod(mapGroup["ZAxisVector"][2]));
      }
      else {
        // Calculate the vectors and store them in the labels
        // The vectors are useful for processing later on, but are
        // not actually used here
        double rotationAngle = m_poleRotation * (PI / 180.0);
        double latitudeAngle = (90.0 - m_poleLatitude) * (PI / 180.0);
        double longitudeAngle = (360.0 - m_poleLongitude) * (PI / 180.0);
        double pvec[3][3];

        NaifStatus::CheckErrors();
        eul2m_c(rotationAngle, latitudeAngle, longitudeAngle, 3, 2, 3, pvec);
        NaifStatus::CheckErrors();

        // Reset the vector keywords
        if (mapGroup.hasKeyword("XAxisVector")) {
          mapGroup.deleteKeyword("XAxisVector");
        }
        if (mapGroup.hasKeyword("YAxisVector")) {
          mapGroup.deleteKeyword("YAxisVector");
        }
        if (mapGroup.hasKeyword("ZAxisVector")) {
          mapGroup.deleteKeyword("ZAxisVector");
        }

        mapGroup += PvlKeyword("XAxisVector");
        mapGroup += PvlKeyword("YAxisVector");
        mapGroup += PvlKeyword("ZAxisVector");

        // Store calculations both in vector and PVL
        for (int i = 0; i < 3; i++) {
          m_xAxisVector.push_back(pvec[0][i]); //X[i]
          m_yAxisVector.push_back(pvec[1][i]); //Y[i]
          m_zAxisVector.push_back(pvec[2][i]); //Z[i]

          mapGroup["XAxisVector"] += std::to_string(pvec[0][i]); //X[i]
          mapGroup["YAxisVector"] += std::to_string(pvec[1][i]); //Y[i]
          mapGroup["ZAxisVector"] += std::to_string(pvec[2][i]); //Z[i]
        }
      }

      init();
    }
    catch(IException &e) {
      QString message = "Invalid label group [Mapping]";
      throw IException(e, IException::Io, message, _FILEINFO_);
    }
  }

  //! Destroys the ObliqueCylindrical object
  ObliqueCylindrical::~ObliqueCylindrical() {
  }

  /**
   * Compares two Projection objects to see if they are equal
   *
   * @param proj Projection object to do comparison on
   *
   * @return bool Returns true if the Projection objects are equal, and false if
   *              they are not
   */
  bool ObliqueCylindrical::operator== (const Projection &proj) {
    if (!Projection::operator==(proj)) return false;

    ObliqueCylindrical *obProjection = (ObliqueCylindrical *) &proj;

    if (obProjection->poleLatitude()  != poleLatitude())  return false;
    if (obProjection->poleLongitude() != poleLongitude()) return false;
    if (obProjection->poleRotation()  != poleRotation())  return false;

    return true;
  }

  /**
   * Returns the name of the map projection, "ObliqueCylindrical"
   *
   * @return QString Name of projection, "ObliqueCylindrical"
   */
  QString ObliqueCylindrical::Name() const {
    return "ObliqueCylindrical";
  }

  /**
   * Returns the version of the map projection
   *
   *
   * @return std::QString Version number
   */
  QString ObliqueCylindrical::Version() const {
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
  bool ObliqueCylindrical::SetGround(const double lat, const double lon) {
    double normalLat, normalLon;    // normal lat/lon
    double obliqueLat, obliqueLon;    // oblique lat/lon copy

    // Store lat,lon
    m_latitude = lat;
    m_longitude = lon;

    // Use oblat,oblon as radians version of lat,lon now for calculations
    normalLat = m_latitude * PI / 180.0;
    normalLon = m_longitude * PI / 180.0;
    if (m_longitudeDirection == PositiveWest) normalLon *= -1.0;


    /***************************************************************************
    * Calculate the oblique lat/lon from the normal lat/lon
    ***************************************************************************/
    double poleLatitude = (m_poleLatitude * PI / 180.0);
    double poleLongitude = (m_poleLongitude * PI / 180.0);
    double poleRotation = (m_poleRotation * PI / 180.0);

    obliqueLat = asin(sin(poleLatitude) * sin(normalLat) +
                      cos(poleLatitude) * cos(normalLat) 
                      * cos(normalLon - poleLongitude));

    obliqueLon = atan2(cos(normalLat) * sin(normalLon - poleLongitude),
                       sin(poleLatitude) * cos(normalLat) 
                       * cos(normalLon - poleLongitude) -
                         cos(poleLatitude) * sin(normalLat)) - poleRotation;

    while(obliqueLon < - PI) {
      obliqueLon += (2.0 * PI);
    }

    while(obliqueLon >= PI) {
      obliqueLon -= (2.0 * PI);
    }

    // Compute the coordinate
    double x = m_equatorialRadius * obliqueLon;
    double y = m_equatorialRadius * obliqueLat;
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
  bool ObliqueCylindrical::SetCoordinate(const double x, const double y) {
    // Save the coordinate
    SetXY(x, y);

    /***************************************************************************
    * Calculate the oblique latitude and check to see if it is outside or equal
    * to [-90,90]. If it is, return with an error.
    ***************************************************************************/
    m_latitude = GetY() / m_equatorialRadius;

    if (abs(abs(m_latitude) - HALFPI) < DBL_EPSILON) {
      m_good = false;
      return m_good;
    }

    /***************************************************************************
    * The check to see if m_equatorialRadius == 0 is done in the init function
    ***************************************************************************/
    m_longitude = GetX() / m_equatorialRadius;

    /***************************************************************************
    * Convert the oblique lat/lon to normal lat/lon
    ***************************************************************************/
    double obliqueLat, obliqueLon;
    double poleLatitude = (m_poleLatitude * PI / 180.0);
    double poleLongitude = (m_poleLongitude * PI / 180.0);
    double poleRotation = (m_poleRotation * PI / 180.0);

    obliqueLat = asin(sin(poleLatitude) * sin(m_latitude) -
                      cos(poleLatitude) * cos(m_latitude) * cos(m_longitude + poleRotation));

    obliqueLon = atan2(cos(m_latitude) * sin(m_longitude + poleRotation),
                       sin(poleLatitude) * cos(m_latitude) * cos(m_longitude + poleRotation) +
                       cos(poleLatitude) * sin(m_latitude)) + poleLongitude;

    /***************************************************************************
    * Convert the latitude/longitude to degrees and apply target longitude  
    *  direction correction to the longitude
    ***************************************************************************/
    m_latitude = obliqueLat * 180.0 / PI;
    m_longitude = obliqueLon * 180.0 / PI;

    // Cleanup the longitude
    if (m_longitudeDirection == PositiveWest) m_longitude *= -1.0;

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
   * This function works for most cases, especially on smaller areas. However,
   * larger areas are likely to fail due to numerous discontinuities and a lack
   * of a mathematical algorithm to solve the range. This method works by 
   * searching the boundaries, using DoSearch, and then searching lines tangent 
   * to discontinuities.
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
  bool ObliqueCylindrical::XYRange(double &minX, double &maxX,
                                   double &minY, double &maxY) {
    return xyRangeOblique(minX, maxX, minY, maxY);
  }


  /**
   * This function returns the keywords that this projection uses.
   *
   * @return PvlGroup The keywords that this projection uses
   */
  PvlGroup ObliqueCylindrical::Mapping() {
    PvlGroup mapping = TProjection::Mapping();

    mapping += m_mappingGrp["PoleLatitude"];
    mapping += m_mappingGrp["PoleLongitude"];
    mapping += m_mappingGrp["PoleRotation"];

    return mapping;
  }

  /**
   * This function returns the latitude keywords that this projection uses
   *
   * @return PvlGroup The latitude keywords that this projection uses
   */
  PvlGroup ObliqueCylindrical::MappingLatitudes() {
    PvlGroup mapping = TProjection::MappingLatitudes();

    return mapping;
  }

  /**
   * This function returns the longitude keywords that this projection uses
   *
   * @return PvlGroup The longitude keywords that this projection uses
   */
  PvlGroup ObliqueCylindrical::MappingLongitudes() {
    PvlGroup mapping = TProjection::MappingLongitudes();

    return mapping;
  }

  /**
   * Returns the value of the pole latitude.
   * 
   * @return @b double The pole latitude.
   * 
   */
  double ObliqueCylindrical::poleLatitude() const {
        return m_poleLatitude;
  }

  /**
   * Returns the value of the pole longitude.
   * 
   * @return @b double The pole longitude.
   * 
   */
  double ObliqueCylindrical::poleLongitude() const {
    return m_poleLongitude;
  }

  /**
   * Returns the value of the pole rotation.
   * 
   * @return @b double The pole rotation.
   * 
   */
  double ObliqueCylindrical::poleRotation() const {
    return m_poleRotation;
  }

  void ObliqueCylindrical::init() {
    /***************************************************************************
    * Apply target correction for longitude direction
    ***************************************************************************/
    if (m_longitudeDirection == PositiveWest) m_longitude *= -1.0;
    if (m_longitudeDirection == PositiveWest) m_poleLongitude *= -1.0;

    /***************************************************************************
    * Check that m_equatorialRadius isn't zero because we'll divide by it later
    ***************************************************************************/
    if (abs(m_equatorialRadius) <= DBL_EPSILON) {
      throw IException(IException::Unknown,
                       "The input center latitude is too close to a pole "
                       "which will result in a division by zero.",
                       _FILEINFO_);
    }
  }

} // end namespace isis

/** 
 * This is the function that is called in order to instantiate an 
 * ObliqueCylindrical object. 
 *  
 * @param lab Cube labels with appropriate Mapping information.
 *  
 * @param allowDefaults Currently, this parameter is unused.
 * 
 * @return @b Isis::Projection* Pointer to an ObliqueCylindrical 
 *         projection object.
 */
extern "C" Isis::Projection *ObliqueCylindricalPlugin(Isis::Pvl &lab,
    bool allowDefaults) {
  return new Isis::ObliqueCylindrical(lab, allowDefaults);
}
