/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Projection.h"

#include <QObject>

#include <cfloat>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <vector>

#include <SpiceUsr.h>

#include "Constants.h"
#include "Displacement.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Longitude.h"
#include "NaifStatus.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "RingPlaneProjection.h"
#include "SpecialPixel.h"
#include "TProjection.h"
#include "WorldMapper.h"

using namespace std;
namespace Isis {
  /**
   * Constructs an empty Projection object.
   *
   * @param label A PVL object containing map projection labels. These labels
   *            are fully described in the Isis Map Projection Users Guide. A
   *            brief example follows:
   *                   @code
   *                       Group = Mapping
   *                         EquatorialRadius = 3396190.0
   *                         PolarRadius = 3376200.0
   *                         LongitudeDirection = PositiveEast
   *                         LongitudeDomain = 360
   *                         LatitudeType = Planetographic
   *                         MinimumLatitude = 10.8920539924144
   *                         MaximumLatitude = 34.7603960060206
   *                         MinimumLongitude = 219.72432466275
   *                         MaximumLongitude = 236.186050244411
   *                         PixelResolution = 1387.31209461362
   *                         ProjectionName = SimpleCylindrical
   *                         CenterLongitude = 220.0
   *                       EndGroup
   *                     End
   *                   @endcode
   *
   * @throw IException::Unknown - "Projection failed. No target radii
   *            available through keywords [EquatorialRadius and
   *             PolarRadius] or [TargetName]."
   * @throw IException::Unknown - "Projection failed. Invalid value
   *            for keyword [EquatorialRadius]. It must be greater than
   *            zero."
   * @throw IException::Unknown - "Projection failed. Invalid value
   *            for keyword [PolarRadius]. It must be greater than zero."
   * @throw IException::Unknown - "Projection failed. Invalid value
   *            for keyword [LatitudeType] must be [Planetographic or
   *             Planetocentric]"
   * @throw IException::Unknown - "Projection failed. Invalid value
   *            for keyword [LongitudeDirection] must be [PositiveWest or
   *            PositiveEast]"
   * @throw IException::Unknown - "Projection failed. Invalid value
   *            for keyword [LongitudeDomain] must be [180 or 360]"
   * @throw IException::Unknown - "Projection failed.
   *            [MinimumLatitude] is outside the range of [-90:90]"
   * @throw IException::Unknown - "Projection failed.
   *            [MaximumLatitude] is outside the range of [-90:90]"
   * @throw IException::Unknown - "Projection failed.
   *            [MinimumLatitude,MaximumLatitude] are not properly ordered"
   * @throw IException::Unknown - "Projection failed.
   *            [MinimumLongitude,MaximumLongitude] are not properly ordered"
   * @throw IException::Unknown - "Projection failed. Invalid keyword
   *            value(s). [EquatorialRadius] must be greater than or equal to
   *            [PolarRadius]"
   * @throw IException::Unknown - "Projection failed. Invalid label
   *            group [Mapping]"
   *
   */
  Projection::Projection(Pvl &label) : m_mappingGrp("Mapping") {
    try {
      // Try to read the mapping group
      m_mappingGrp = label.findGroup("Mapping", Pvl::Traverse);

      // TODO** Try to generalize these to keep in parent Projection class and use for both azimuth and longitude
      // Get the RingLongitudeDomain or LongitudeDomain
      // if ((string) m_mappingGrp["LongitudeDomain"] == "360") {
      //   m_longitudeDomain = 360;
      // }
      // else if ((string) m_mappingGrp["LongitudeDomain"] == "180") {
      //   m_longitudeDomain = 180;
      // }
      // else {
      //   IString msg = "Projection failed. Invalid value for keyword "
      //                 "[LongitudeDomain] must be [180 or 360]";
      //   throw IException(IException::Unknown, msg, _FILEINFO_);
      // }

      // Get the map rotation
      m_rotation = 0.0;
      if (m_mappingGrp.hasKeyword("Rotation")) {
        m_rotation = m_mappingGrp["Rotation"];
      }

      // Initialize miscellaneous protected data elements
      m_good = false;

      m_pixelResolution = 1.0;
      if (m_mappingGrp.hasKeyword("PixelResolution")) {
        m_pixelResolution = m_mappingGrp["PixelResolution"];
      }

      m_minimumX = DBL_MAX;
      m_maximumX = -DBL_MAX;
      m_minimumY = DBL_MAX;
      m_maximumY = -DBL_MAX;

      m_mapper = NULL;

      m_sky = false;
      if (m_mappingGrp.hasKeyword("TargetName")) {
        QString str = QString::fromStdString(m_mappingGrp["TargetName"]);
        if (str.toUpper() == "SKY") m_sky = true;
      }

      // initialize the rest of the x,y,lat,lon member variables
      m_x = Null;
      m_y = Null;
    }
    catch(IException &e) {
      std::string msg = "Projection failed.  Invalid label group [Mapping]";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
  }

  //! Destroys the Projection object
  Projection::~Projection() {
    delete m_mapper;
    m_mapper = NULL;
  }

  /**
   * This method determines whether two map projection objects are equal by
   * comparing the  resolution, and projection name.
   *
   * @param proj A reference to a Projection object to which this Projection
   *             will be compared.
   *
   * @return bool Indicates whether the Projection objects are equivalent.
   */
  bool Projection::operator== (const Projection &proj) {
    if (Resolution() != proj.Resolution()) return false;
    if (Name() != proj.Name()) return false;
    return true;
  }


  /**
   * This method determines whether two map projection objects are not equal.
   * True is returned if they have at least some differences in the
   * radii, latitude type, longitude direction, resolution, or projection name.
   *
   * @param proj A reference to a Projection object to which this Projection
   *             will be compared.
   * @return bool Indicates whether the Projection objects are not equivalent.
   */
  bool Projection::operator!= (const Projection &proj) {
    return !(*this == proj);
  }


  /**
   * Sets the projection subclass type
   *
   * @param ptype The projection subclass type
   */
  void Projection::setProjectionType(const ProjectionType ptype) {
    m_projectionType = ptype;
  }


  /**
   * Returns an enum value for the projection type
   *
   * @return projection subclass type
   */
  Projection::ProjectionType Projection::projectionType() const {
    return m_projectionType;
  }


  /**
   * Returns true if projection is sky and false if it is land
   *
   * @return bool
   */
  bool Projection::IsSky() const {
    return m_sky;
  }


  /**
   * This method returns true if the projection is
   *   equatorial cylindrical. In other words, if the
   *   projection is cylindrical and an image projected at 0 is
   *   the same as an image projected at 360.
   *
   *
   * @return bool true if the projection is equatorial cylindrical
   */
  bool Projection::IsEquatorialCylindrical() {
    return false;
  }

  /**
   * This indicates if the longitude direction type is positive west (as
   * opposed to postive east). The longitude type was obtained from the
   * label during object construction.
   *
   * @return bool
   */
  // bool Projection::IsPositiveEast() const {
  //   return m_longitudeDirection == PositiveEast;
  // }

  /**
   * This indicates if the longitude direction type is positive east (as
   * opposed to postive west). The longitude type was obtained from the
   * label during object construction.
   *
   * @return bool
   */
  // bool Projection::IsPositiveWest() const {
  //   return m_longitudeDirection == PositiveWest;
  // }


  /**
   * This method returns the longitude direction as a string. It will return
   * either PositiveEast or PositiveWest.
   *
   * @return string The longitude direction, "PositiveEast" or
   *         "PositiveWest".
   */
  // string Projection::LongitudeDirectionString() const {
  //   if (m_longitudeDirection == PositiveEast) return "PositiveEast";
  //   return "PositiveWest";
  // }

  /**
   * This indicates if the longitude domain is -180 to 180 (as opposed to 0
   * to 360). The longitude domain was obtained from the label during object
   * construction.
   *
   * @return bool
   */
  // bool Projection::Has180Domain() const {
  //   return m_longitudeDomain == 180;
//  }

  /**
   * This indicates if the longitude domain is 0 to 360 (as opposed to -180
   * to 180). The longitude domain was obtained from the label during object
   * construction.
   *
   * @return bool
   */
  // bool Projection::Has360Domain() const {
  //   return m_longitudeDomain == 360;
  // }

  /**
   * This method converts a longitude into the -180 to 180 domain. It will leave
   * the longitude unchanged if it is already in the domain.
   *
   * @param lon Longitude to convert into the -180 to 180 domain.
   *
   * @throw IException::Unknown - "The given longitude is invalid."
   *
   * @return double The longitude, converted to 180 domain.
   */
  // double Projection::To180Domain(const double lon) {
  //   if (lon == Null) {
  //     throw IException(IException::Unknown,
  //                      "Unable to convert to 180 degree domain. The given longitude value ["
  //                      + IString(lon) + "] is invalid.",
  //                      _FILEINFO_);
  //   }
  //   return Isis::Longitude(lon, Angle::Degrees).force180Domain().degrees();
  // }

  /**
   * This method converts a longitude into the 0 to 360 domain. It will leave
   * the longitude unchanged if it is already in the domain.
   *
   * @param lon Longitude to convert into the 0 to 360 domain.
   *
   * @return double The longitude, converted to 360 domain.
   */
  // double Projection::To360Domain(const double lon) {
  //   if (lon == Null) {
  //     throw IException(IException::Unknown,
  //                      "Unable to convert to 360 degree domain. The given longitude value ["
  //                      + IString(lon) + "] is invalid.",
  //                      _FILEINFO_);
  //   }
  //   double result = lon;

  //   if ( (lon < 0.0 || lon > 360.0) &&
  //       !qFuzzyCompare(lon, 0.0) && !qFuzzyCompare(lon, 360.0)) {
  //    result = Isis::Longitude(lon, Angle::Degrees).force360Domain().degrees();
  //   }

  //   return result;
  // }

  /**
   * This method returns the longitude domain as a string. It will return either
   * 180 or 360.
   *
   * @return string The longitude domain, "180" or "360".
   */
  // string Projection::LongitudeDomainString() const {
  //   if (m_longitudeDomain == 360) return "360";
  //   return "180";
  // }

  /**
   * This indicates that the labels contained minimum and maximum ground
   * coordinatess (e.g., a ground range coverage). If the projection has
   * ground range coverage then the MinimumLatitude, MaximumLatitude,
   * MinimumLongitude, and MaximumLongitude methods or comparable
   * methods for other projection types can be used. The ground range
   * coverage essentially defines the area of user interest.
   *
   * @return bool
   */
  bool Projection::HasGroundRange() const {
    return m_groundRangeGood;
  }


  /**
   * Returns the value of the Rotation keyword from the mapping group.
   *
   * @return double The rotation of the map.
   */
  double Projection::Rotation() const {
    return m_rotation;
  }


  /**
   * This indicates if the last invocation of SetGround, SetCoordinate,
   * SetUniversalGround, or SetWorld was with successful or not. If there was
   * success then the Latitude, Longitude, XCoord, YCoord,
   * UniversalLatitude, UniversalLongitude, WorldX, and WorldY methods can
   * be utilized.
   *
   * @return bool True if the last call to SetGround, SetCoordinate,
   *         SetUniversalGround, or SetWorld was successful.
   */
  bool Projection::IsGood() const {
    return m_good;
  }


  /**
   * This returns the projection X provided SetGround, SetCoordinate,
   * SetUniversalGround, or SetWorld returned with success. Success can also
   * be checked using the IsGood method. The units of X will be in the same .
   * units as the radii obtained from the label.
   *
   * @return double
   */
  double Projection::XCoord() const {
    return m_x;
  }


  /**
   * This returns the projection Y provided SetGround, SetCoordinate,
   * SetUniversalGround, or SetWorld returned with success. Success can also
   * be checked using the IsGood method. The units of Y will be in the same
   * units as the radii obtained from the label.
   *
   * @return double
   */
  double Projection::YCoord() const {
    return m_y;
  }


  /**
   * This method is used to set the lat/lon or radius/azimuth (i.e. ring longitude)
   * coordinate, depending on the projection type. The Set forces an attempted
   * calculation of the projection X/Y values. This may or may not be successful
   * and a status is returned as such.
   *
   * @param coord1 Latitude (planetocentric) or ring radius to project
   * @param coord2 Longitude or ring longitude to project. The value passed in
   *               should be PositiveEast, Domain360.
   *
   * @return bool Indicates whether the method was successful.
   */
  bool Projection::SetUniversalGround(const double coord1, const double coord2) {
    if (coord1 == Null || coord2 == Null) {
      m_good = false;
      return m_good;
    }
    if (projectionType() == Triaxial) {
      TProjection *tproj = (TProjection *) this;
      return tproj->SetUniversalGround(coord1, coord2);
    }
    else {
      RingPlaneProjection *rproj = (RingPlaneProjection *) this;
      return rproj->SetUniversalGround(coord1, coord2);
    }
  }


  /**
   * This method is used to set the lat/lon or radius/azimuth (i.e. ring longitude)
   * coordinate, depending on the projection type. The Set forces an attempted
   * calculation of the projection X/Y values. This may or may not be successful
   * and a status is returned as such. This method will not adjust the longitude
   * coordinate based on the longitude domain.
   *
   * @param coord1 Latitude (planetocentric) or ring radius to project
   * @param coord2 Longitude or ring longitude to project. The value passed in
   *               should be PositiveEast, Domain360.
   *
   * @return bool Indicates whether the method was successful.
   */
  bool Projection::SetUnboundUniversalGround(const double coord1, const double coord2) {
    if (coord1 == Null || coord2 == Null) {
      m_good = false;
      return m_good;
    }
    if (projectionType() == Triaxial) {
      TProjection *tproj = (TProjection *) this;
      return tproj->SetUnboundUniversalGround(coord1, coord2);
    }
    else {
      RingPlaneProjection *rproj = (RingPlaneProjection *) this;
      return rproj->SetUniversalGround(coord1, coord2);
    }
  }


  /**
   * If desired the programmer can use this method to set a world mapper to
   * be used in the SetWorld, WorldX, and WorldY methods. Mappers typically
   * transform a projection coordinate (x/y) into the desired working
   * coordinate system, for example, cube pixels or inches on a piece of
   * paper. They transform in both directions (world to projection and
   * projection to world). This allows for conversions from line/sample to
   * latitude/longitude and vice versa. This projection will take ownership
   * of the WorldMapper pointer.
   *
   * @param mapper Pointer to the mapper
   */
  void Projection::SetWorldMapper(WorldMapper *mapper) {
    m_mapper = mapper;
  }

  /**
   * This method is used to set a world coordinate. A world coordinate is a
   * different coordinate type that has a one-to-one mapping to the projection
   * coordinate system. For example, mapping pixel samples and lines to
   * projection x's and y's. The Set forces an attempted calculation of the
   * corresponding latitude/longitude position. This may or may not be
   * successful and a status is returned as such. Note that is only applies if
   * the Projection object was given an WorldMapper object during construction.
   * If an WorldMapper was not supplied then SetWorld operates exactly the same
   * as SetCoordinate (impling that world coordinate and projection coordinate
   * are identical).
   *
   * @param worldX World X coordinate in units that are specified by the
   *            WorldMapper object (e.g., pixels, millimeters, etc)
   * @param worldY World Y coordinate in units that are specified by the
   *            WorldMapper object (e.g., pixels, millimeters, etc)
   *
   * @return bool Indicates whether the method was successful.
   */
  bool Projection::SetWorld(const double worldX, const double worldY) {
    double projectionX;
    double projectionY;

    if (m_mapper != NULL) {
      projectionX = m_mapper->ProjectionX(worldX);
      projectionY = m_mapper->ProjectionY(worldY);
    }
    else {
      projectionX = worldX;
      projectionY = worldY;
    }

    return SetCoordinate(projectionX, projectionY);
  }

  /**
   * This returns the world X coordinate provided SetGround, SetCoordinate,
   * SetUniversalGround, or SetWorld returned with success. Success can also be
   * checked using the IsGood method. The units of X will be in the units as
   * specified by the WorldMapper object which was given to the SetWorldMapper
   * method. If a mapper object was not given then world coordinates are the
   * same as the projection coordinates (i.e., WorldX and XCoord will return the
   * same value).
   *
   * @return double The world X coordinate.
   */
  double Projection::WorldX() const {
    if (m_mapper != NULL) {
      return m_mapper->WorldX(m_x);
    }
    else {
      return m_x;
    }
  }

  /**
   * This returns the world Y coordinate provided SetGround, SetCoordinate,
   * SetUniversalGround, or SetWorld returned with success. Success can also be
   * checked using the IsGood method. The units of Y will be in the units as
   * specified by the WorldMapper object which was given to the SetWorldMapper.
   * If a mapper object was not given then world coordinates are the same as the
   * projection coordinates (i.e., WorldY and YCoord will return the same
   * value).
   *
   * @return double The world Y coordinate.
   */
  double Projection::WorldY() const {
    if (m_mapper != NULL) {
      return m_mapper->WorldY(m_y);
    }
    else {
      return m_y;
    }
  }

  /**
   * This method converts a projection x value to a world x value. For example,
   * if the world coordinate system is an image then this method converts a
   * projection x to a sample position. Note that if SetWorldMapper is not used
   * then this routine simply returns the value of the argument. That is, no
   * mapping occurs.
   *
   * @param projectionX Projection x value in meters
   *
   * @throw IException::Unknown - "The given x-value is invalid."
   *
   * @return double The world X coordinate value.
   */
  double Projection::ToWorldX(const double projectionX) const {
    if (projectionX == Null) {
      throw IException(IException::Unknown,
                       "Unable to convert to world x.  The given x-value ["
                       + IString(projectionX) + "] is invalid.",
                       _FILEINFO_);
    }
    if (m_mapper != NULL) {
      return m_mapper->WorldX(projectionX);
    }
    else {
      return projectionX;
    }
  }

  /**
   * This method converts a projection y value to a world y value. For example,
   * if the world coordinate system is an image then this method converts a
   * projection y to a line position. Note that if SetWorldMapper is not used
   * then this routine simply returns the value of the argument. That is, no
   * mapping occurs.
   *
   * @param projectionY Projection y value in meters
   *
   * @throw IException::Unknown - "The given y-value is invalid."
   *
   * @return double The world Y coordinate value.
   */
  double Projection::ToWorldY(const double projectionY) const {
    if (projectionY == Null) {
      throw IException(IException::Unknown,
                       "Unable to convert to world y.  The given y-value ["
                       + IString(projectionY) + "] is invalid.",
                       _FILEINFO_);
    }
    if (m_mapper != NULL) {
      return m_mapper->WorldY(projectionY);
    }
    else {
      return projectionY;
    }
  }

  /**
   * This method converts a world x value to a projection x value. For example,
   * if the world coordinate system is an image then this method converts a
   * sample position to a projection x value. Note that if SetWorldMapper is not
   * used then this routine simply returns the value of the argument. That is,
   * no mapping occurs.
   *
   * @param worldX World x coordinate
   *
   * @throw IException::Unknown - "The given x-value is invalid."
   *
   * @return double The projection X coordinate value.
   */
  double Projection::ToProjectionX(const double worldX) const {
    if (worldX == Null) {
      throw IException(IException::Unknown,
                       "Unable to convert to projection x.  The given x-value ["
                       + IString(worldX) + "] is invalid.",
                       _FILEINFO_);
    }
    if (m_mapper != NULL) {
      return m_mapper->ProjectionX(worldX);
    }
    else {
      return worldX;
    }
  }

  /**
   * This method converts a world y value to a projection y value. For example,
   * if the world coordinate system is an image then this method converts a line
   * position to a projection y value. Note that if SetWorldMapper is not used
   * then this routine simply returns the value of the argument. That is, no
   * mapping occurs.
   *
   * @param worldY World y coordinate
   *
   * @throw IException::Unknown - "The given y-value is invalid."
   *
   * @return double The projection Y coordinate value.
   */
  double Projection::ToProjectionY(const double worldY) const {
    if (worldY == Null) {
      throw IException(IException::Unknown,
                       "Unable to convert to projection y.  The given y-value ["
                       + IString(worldY) + "] is invalid.",
                       _FILEINFO_);
    }
    if (m_mapper != NULL) {
      return m_mapper->ProjectionY(worldY);
    }
    else {
      return worldY;
    }
  }

  /**
   * This method returns the resolution for mapping world coordinates into
   * projection coordinates. For example, if the world coordinate system is an
   * image then this routine returns the number of meters per pixel. Likewise,
   * if the world coordinate system is a piece of paper, it might return the
   * number of meters per inch of paper. If the SetWorldMapper method is not
   * invoked then this method returns 1.0
   *
   * @return double The resolution, in appropriate units.
   */
  double Projection::Resolution() const {
    if (m_mapper != NULL) {
      return m_mapper->Resolution();
    }
    else {
      return 1.0;
    }
  }


  /**
   * Converts the given angle (in degrees) to hours by using the ratio 15
   * degrees per hour.
   *
   * @param angle Angle in degrees to be converted to hours
   *
   * @return double The number of hours for the given angle.
   */
  double Projection::ToHours(double angle) {
    return angle / 15.0;
  }

  /**
   * Converts the given angle (in degrees) to degrees, minutes, seconds.
   * Outputs in the form xxx yym zz.zzzs, for example, 206.291 degrees
   * is 206 17m 27.6s
   *
   * @param angle Angle in degrees to be converted to degrees, minutes, seconds
   *
   * @return string The angle in Degrees, minutes, seconds
   */
  QString Projection::ToDMS(double angle) {
    int iangle = (int)angle;
    double mins = abs(angle - iangle) * 60.0;
    int imins = (int)mins;
    double secs = (mins - imins) * 60.0;
    int isecs = (int)secs;
    double frac = (secs - isecs) * 1000.0;
    if (frac >= 1000.0) {
      frac -= 1000.0;
      isecs++;
    }
    if (isecs >= 60) {
      isecs -= 60;
      imins++;
    }
    if (imins >= 60) {
      imins -= 60;
      iangle++;
    }
    stringstream s;
    s << iangle << " " << setw(2) << setfill('0')
      << imins << "m " << setw(2) << setfill('0') << isecs << "."  <<
      setprecision(3) << frac << "s";
    return s.str().c_str();
  }

  /**
   * Converts the given angle (in degrees) to hours, minutes, seconds.
   * Outputs in the form xxh yym zz.zzzs  For example, 206.291 will be
   * 13h 45m 09.84s
   *
   * @param angle Angle in degrees to be converted to hours, minutes, seconds
   *
   * @return string The angle in Hours, minutes, seconds
   */
  QString Projection::ToHMS(double angle) {
    double tangle = angle;
    while (tangle < 0.0) tangle += 360.0;
    while (tangle > 360.0) tangle -= 360.0;
    double hrs = ToHours(tangle);
    int ihrs = (int)(hrs);
    double mins = (hrs - ihrs) * 60.0;
    int imins = (int)(mins);
    double secs = (mins - imins) * 60.0;
    int isecs = (int)(secs);
    double msecs = (secs - isecs) * 1000.0;
    int imsecs = (int)(msecs + 0.5);
    if (imsecs >= 1000) {
      imsecs -= 1000;
      isecs++;
    }
    if (isecs >= 60) {
      isecs -= 60;
      imins++;
    }
    if (imins >= 60) {
      imins -= 60;
      ihrs++;
    }
    stringstream s;
    s << setw(2) << setfill('0') << ihrs << "h " << setw(2) << setfill('0') <<
      imins << "m " << setw(2) << setfill('0') << isecs << "." << imsecs << "s";
    return s.str().c_str();
  }

  /**
   * This protected method is a helper for derived classes.  It
   * takes unrotated x and y values, rotates them using the
   * rotation angle data member, and stores the results in the
   * current x and y data members.
   *
   * @param x The unrotated x coordinate.
   * @param y The unrotated y coordinate.
   */
  void Projection::SetComputedXY(double x, double y) {
    if (x == Null || y == Null) {
      m_good = false;
      return;
    }
    if (m_rotation == 0.0) {
      m_x = x;
      m_y = y;
    }
    else {
      double rot = m_rotation * PI / 180.0;
      m_x = x * cos(rot) + y * sin(rot);
      m_y = y * cos(rot) - x * sin(rot);
    }
  }

  /**
   * This protected method is a helper for derived classes.  It
   * takes a rotated x,y and stores them in the current x and y
   * data members.
   *
   * @param x The rotated x coordinate.
   * @param y The rotated y coordinate.
   */
  void Projection::SetXY(double x, double y) {
    if (x == Null || y == Null) {
      m_good = false;
    }
    m_x = x;
    m_y = y;
    return;
  }

  /**
   * Calculates the unrotated form of current x value.
   *
   * @return double The unrotated x-value.
   */
  double Projection::GetX() const {
    if (m_rotation == 0.0) return m_x;
    double rot = m_rotation * PI / 180.0;
    return m_x * cos(rot) - m_y * sin(rot);
  }

  /**
   * Calculates the unrotated form of the current y value.
   *
   * @return double The unrotated y-value.
   */
  double Projection::GetY() const {
    if (m_rotation == 0.0) return m_y;
    double rot = m_rotation * PI / 180.0;
    return m_y * cos(rot) + m_x * sin(rot);
  }

  /**
   * Returns the pixel resolution value from the PVL mapping group in meters/pixel.
   *
   * @return double pixel resolution in meters/pixel
   */
  double Projection::PixelResolution() const {
    return m_pixelResolution;
  }


  // /**
  //  * This method is used to find the XY range for oblique aspect projections
  //  * (non-polar projections) by "walking" around each of the min/max lat/lon.
  //  *
  //  * @param minX Minimum x projection coordinate which covers the latitude
  //  *             longitude range specified in the labels.
  //  * @param maxX Maximum x projection coordinate which covers the latitude
  //  *             longitude range specified in the labels.
  //  * @param minY Minimum y projection coordinate which covers the latitude
  //  *             longitude range specified in the labels.
  //  * @param maxY Maximum y projection coordinate which covers the latitude
  //  *             longitude range specified in the labels.
  //  *
  //  * @return @b bool Indicates whether the method was successful.
  //  * @see XYRange()
  //  * @author Stephen Lambright
  //  * @internal
  //  *   @history 2011-07-02 Jeannie Backer - Moved this code from
  //  *                           ObliqueCylindrical class to its own method here.
  //  */
  // bool Projection::xyRangeOblique(double &minX, double &maxX,
  //                                 double &minY, double &maxY) {
  //   if (minX == Null || maxX == Null || minY == Null || maxY == Null) {
  //     return false;
  //   }
  //   //For oblique, we'll have to walk all 4 sides to find out min/max x/y values
  //   if (!HasGroundRange()) return false; // Don't have min/max lat/lon,
  //                                       //can't continue

  //   m_specialLatCases.clear();
  //   m_specialLonCases.clear();

  //   // First, search longitude for min X/Y
  //   double minFoundX1, minFoundX2;
  //   double minFoundY1, minFoundY2;

  //   // Search for minX between minlat and maxlat along minlon
  //   doSearch(MinimumLatitude(), MaximumLatitude(),
  //            minFoundX1, MinimumLongitude(), true, true, true);
  //   // Search for minX between minlat and maxlat along maxlon
  //   doSearch(MinimumLatitude(), MaximumLatitude(),
  //            minFoundX2, MaximumLongitude(), true, true, true);
  //   // Search for minY between minlat and maxlat along minlon
  //   doSearch(MinimumLatitude(), MaximumLatitude(),
  //            minFoundY1, MinimumLongitude(), false, true, true);
  //   // Search for minY between minlat and maxlat along maxlon
  //   doSearch(MinimumLatitude(), MaximumLatitude(),
  //            minFoundY2, MaximumLongitude(), false, true, true);

  //   // Second, search latitude for min X/Y
  //   double minFoundX3, minFoundX4;
  //   double minFoundY3, minFoundY4;

  //   // Search for minX between minlon and maxlon along minlat
  //   doSearch(MinimumLongitude(), MaximumLongitude(),
  //            minFoundX3, MinimumLatitude(), true, false, true);
  //   // Search for minX between minlon and maxlon along maxlat
  //   doSearch(MinimumLongitude(), MaximumLongitude(),
  //            minFoundX4, MaximumLatitude(), true, false, true);
  //   // Search for minY between minlon and maxlon along minlat
  //   doSearch(MinimumLongitude(), MaximumLongitude(),
  //            minFoundY3, MinimumLatitude(), false, false, true);
  //   // Search for minY between minlon and maxlon along maxlat
  //   doSearch(MinimumLongitude(), MaximumLongitude(),
  //            minFoundY4, MaximumLatitude(), false, false, true);

  //   // We've searched all possible minimums, go ahead and store the lowest
  //   double minFoundX5 = min(minFoundX1, minFoundX2);
  //   double minFoundX6 = min(minFoundX3, minFoundX4);
  //   m_minimumX = min(minFoundX5, minFoundX6);

  //   double minFoundY5 = min(minFoundY1, minFoundY2);
  //   double minFoundY6 = min(minFoundY3, minFoundY4);
  //   m_minimumY = min(minFoundY5, minFoundY6);

  //   // Search longitude for max X/Y
  //   double maxFoundX1, maxFoundX2;
  //   double maxFoundY1, maxFoundY2;

  //   // Search for maxX between minlat and maxlat along minlon
  //   doSearch(MinimumLatitude(), MaximumLatitude(),
  //            maxFoundX1, MinimumLongitude(), true, true, false);
  //   // Search for maxX between minlat and maxlat along maxlon
  //   doSearch(MinimumLatitude(), MaximumLatitude(),
  //            maxFoundX2, MaximumLongitude(), true, true, false);
  //   // Search for maxY between minlat and maxlat along minlon
  //   doSearch(MinimumLatitude(), MaximumLatitude(),
  //            maxFoundY1, MinimumLongitude(), false, true, false);
  //   // Search for maxY between minlat and maxlat along maxlon
  //   doSearch(MinimumLatitude(), MaximumLatitude(),
  //            maxFoundY2, MaximumLongitude(), false, true, false);

  //   // Search latitude for max X/Y
  //   double maxFoundX3, maxFoundX4;
  //   double maxFoundY3, maxFoundY4;

  //   // Search for maxX between minlon and maxlon along minlat
  //   doSearch(MinimumLongitude(), MaximumLongitude(),
  //            maxFoundX3, MinimumLatitude(), true, false, false);
  //   // Search for maxX between minlon and maxlon along maxlat
  //   doSearch(MinimumLongitude(), MaximumLongitude(),
  //            maxFoundX4, MaximumLatitude(), true, false, false);
  //   // Search for maxY between minlon and maxlon along minlat
  //   doSearch(MinimumLongitude(), MaximumLongitude(),
  //            maxFoundY3, MinimumLatitude(), false, false, false);
  //   // Search for maxY between minlon and maxlon along maxlat
  //   doSearch(MinimumLongitude(), MaximumLongitude(),
  //            maxFoundY4, MaximumLatitude(), false, false, false);

  //   // We've searched all possible maximums, go ahead and store the highest
  //   double maxFoundX5 = max(maxFoundX1, maxFoundX2);
  //   double maxFoundX6 = max(maxFoundX3, maxFoundX4);
  //   m_maximumX = max(maxFoundX5, maxFoundX6);

  //   double maxFoundY5 = max(maxFoundY1, maxFoundY2);
  //   double maxFoundY6 = max(maxFoundY3, maxFoundY4);
  //   m_maximumY = max(maxFoundY5, maxFoundY6);

  //   // Look along discontinuities for more extremes
  //   vector<double> specialLatCases = m_specialLatCases;
  //   for (unsigned int specialLatCase = 0;
  //       specialLatCase < specialLatCases.size();
  //       specialLatCase ++) {
  //     double minX, maxX, minY, maxY;

  //     // Search for minX between minlon and maxlon along latitude discontinuities
  //     doSearch(MinimumLongitude(), MaximumLongitude(),
  //              minX, specialLatCases[specialLatCase], true,  false, true);
  //     // Search for minY between minlon and maxlon along latitude discontinuities
  //     doSearch(MinimumLongitude(), MaximumLongitude(),
  //              minY, specialLatCases[specialLatCase], false, false, true);
  //     // Search for maxX between minlon and maxlon along latitude discontinuities
  //     doSearch(MinimumLongitude(), MaximumLongitude(),
  //              maxX, specialLatCases[specialLatCase], true,  false, false);
  //     // Search for maxX between minlon and maxlon along latitude discontinuities
  //     doSearch(MinimumLongitude(), MaximumLongitude(),
  //              maxY, specialLatCases[specialLatCase], false, false, false);

  //     m_minimumX = min(minX, m_minimumX);
  //     m_maximumX = max(maxX, m_maximumX);
  //     m_minimumY = min(minY, m_minimumY);
  //     m_maximumY = max(maxY, m_maximumY);
  //   }

  //   vector<double> specialLonCases = m_specialLonCases;
  //   for (unsigned int specialLonCase = 0;
  //       specialLonCase < specialLonCases.size();
  //       specialLonCase ++) {
  //     double minX, maxX, minY, maxY;

  //     // Search for minX between minlat and maxlat along longitude discontinuities
  //     doSearch(MinimumLatitude(), MaximumLatitude(),
  //              minX, specialLonCases[specialLonCase], true,  true, true);
  //     // Search for minY between minlat and maxlat along longitude discontinuities
  //     doSearch(MinimumLatitude(), MaximumLatitude(),
  //              minY, specialLonCases[specialLonCase], false, true, true);
  //     // Search for maxX between minlat and maxlat along longitude discontinuities
  //     doSearch(MinimumLatitude(), MaximumLatitude(),
  //              maxX, specialLonCases[specialLonCase], true,  true, false);
  //     // Search for maxY between minlat and maxlat along longitude discontinuities
  //     doSearch(MinimumLatitude(), MaximumLatitude(),
  //              maxY, specialLonCases[specialLonCase], false, true, false);

  //     m_minimumX = min(minX, m_minimumX);
  //     m_maximumX = max(maxX, m_maximumX);
  //     m_minimumY = min(minY, m_minimumY);
  //     m_maximumY = max(maxY, m_maximumY);
  //   }

  //   m_specialLatCases.clear();
  //   m_specialLonCases.clear();

  //   // Make sure everything is ordered
  //   if (m_minimumX >= m_maximumX) return false;
  //   if (m_minimumY >= m_maximumY) return false;

  //   // Return X/Y min/maxs
  //   minX = m_minimumX;
  //   maxX = m_maximumX;
  //   minY = m_minimumY;
  //   maxY = m_maximumY;

  //   return true;
  // }

  /**
   * This method searches for extreme (min/max/discontinuity) coordinate values
   * along the constBorder line between minBorder and maxBorder (that is,
   * across latitudes/longitudes). This method locates the extrema by utilizing
   * the findExtreme() method until the coordinate values converge. Then,
   * extremeVal parameter is updated with this value before returning.
   *
   * Discontinuities are stored in m_specialLatCases and m_specialLonCases so
   * they may be checked again later, which creates significantly more accuracy
   * in some cases.
   *
   * @param minBorder Minimum latitude or longitude to search between.
   * @param maxBorder Maximum latitude or longitude to search between.
   * @param extremeVal The resulting global coordinate value (min or max
   *            value for x or y, depending on findMin and searchX) on the
   *            constBorder.
   * @param constBorder The latitude or longitude that remains constant.  The
   *            method will step along this border.
   * @param searchX Indicates whether the method is searching for a min or max
   *            x-coordinate.  If false the method searches for min or max
   *            y-coordinate.
   * @param searchLongitude Indicates whether the method will search
   *            along a longitude. If true, constBorder is longitude and all
   *            other borders are latitudes. If false, the method searches a
   *            latitude (constBorder is a lat, other borders lons).
   * @param findMin Indicates whether the method is looking for a minimum
   *            coordinate value. If false, the method is looking for a maximum
   *            value.
   * @author Steven Lambright
   * @internal
   *   @history 2011-07-02 Jeannie Backer - Moved this code from
   *                           ObliqueCylindrical class to its own method
   *                           here. Added condition to do-while loop for
   *                           more precision. Replaced hard-coded
   *                           TOLERANCE with 1/2 pixel resolution.
   *                           (Defaults to 0.5)
   */
  // void Projection::doSearch(double minBorder, double maxBorder,
  //                           double &extremeVal, const double constBorder,
  //                           bool searchX, bool searchLongitude, bool findMin) {
  //   if (minBorder == Null || maxBorder == Null || constBorder == Null) {
  //     return;
  //   }
  //   const double TOLERANCE = m_pixelResolution/2;
  //   const int NUM_ATTEMPTS = (unsigned int)DBL_DIG; // It's unsafe to go past
  //                                                   // this precision

  //   double minBorderX, minBorderY, maxBorderX, maxBorderY;
  //   int attempts = 0;

  //   do {
  //     findExtreme(minBorder, maxBorder, minBorderX, minBorderY, maxBorderX,
  //                 maxBorderY, constBorder, searchX, searchLongitude, findMin);
  //     if (minBorderX == Null && maxBorderX == Null
  //         && minBorderY == Null && maxBorderY == Null ) {
  //       attempts = NUM_ATTEMPTS;
  //       continue;
  //     }
  //     attempts ++;
  //   }
  //   while ((fabs(minBorderX - maxBorderX) > TOLERANCE
  //          || fabs(minBorderY - maxBorderY) > TOLERANCE)
  //          && (attempts < NUM_ATTEMPTS));
  //   // check both x and y distance in case symmetry of map
  //   // For example, if minBorderX = maxBorderX but minBorderY = -maxBorderY,
  //   // these points may not be close enough.

  //   if (attempts >= NUM_ATTEMPTS) {
  //     // We zoomed in on a discontinuity because our range never shrank, this
  //     // will need to be rechecked later.
  //     // *min and max border should be nearly identical, so it doesn't matter
  //     //  which is used here
  //     if (searchLongitude) {
  //       m_specialLatCases.push_back(minBorder);
  //     }
  //     else {
  //       m_specialLonCases.push_back(minBorder);
  //     }
  //   }

  //   // These values will always be accurate, even over a discontinuity
  //   if (findMin) {
  //     if (searchX) extremeVal = min(minBorderX, maxBorderX);
  //     else         extremeVal = min(minBorderY, maxBorderY);
  //   }
  //   else {
  //     if (searchX) extremeVal = max(minBorderX, maxBorderX);
  //     else         extremeVal = max(minBorderY, maxBorderY);
  //   }
  //   return;
  // }

  /**
   * Searches for extreme (min/max/discontinuity) coordinate values across
   * latitudes/longitudes.
   *
   * This method looks for these extrema along the constBorder between minBorder
   * and maxBorder by stepping along constBorder (10 times) from the minBorder
   * and maxBorder. Then, the range of this extreme value is recorded in
   * minBorder and maxBorder and the coordinate values corresponding to these
   * new borders are stored in minBorderX, minBorderY, maxBorderX and
   * maxBorderY.
   *
   * This function should be used by calling it repeatedly until minBorderX and
   * minBorderY do not equal maxBorderX and maxBorderY, respectively.
   * Discontinuities will cause the minBorderX, minBorderY, maxBorderX and
   * maxBorderY to never converge. If minBorderX never comes close to maxBorderX
   * or minBorderY never comes close to maxBorderY, then between minBorder and
   * maxBorder is the value of the most extreme value. In this case, either the
   * smaller or larger of the x or y values found will be correct, depending on
   * the values of findMin and searchX.
   *
   *
   *
   *
   * @param minBorder Minimum latitude or longitude to search between. This
   *            value gets updated to a more precise range.
   * @param maxBorder Maximum latitude or longitude to search between. This
   *            value gets updated to a more precise range.
   * @param minBorderX The x-value corresponding to the lower resultant
   *            minBorder and the constBorder, which is more accurate when
   *            nearly equal to maxBorderX.
   * @param minBorderY The y-value corresponding to the lower resultant
   *            minBorder and the constBorder, which is more accurate when
   *            nearly equal to maxBorderY.
   * @param maxBorderX The x-value corresponding to the higher resultant
   *            maxBorder and the constBorder, which is more accurate when
   *            nearly equal to minBorderX.
   * @param maxBorderY The y-value corresponding to the higher resultant
   *            maxBorder and the constBorder, which is more accurate when
   *            nearly equal to minBorderY.
   * @param constBorder The latitude or longitude that remains constant.  The
   *            method will step along this border.
   * @param searchX Indicates whether the method is searching for a min or max
   *            x-coordinate.  If false the method searches for min or max
   *            y-coordinate.
   * @param searchLongitude Indicates whether the method will search
   *            along a longitude. If true, constBorder is longitude and all
   *            other borders are latitudes. If false, the method searches a
   *            latitude (constBorder is a lat, other borders lons).
   * @param findMin Indicates whether the method is looking for a minimum
   *            coordinate value. If false, the method is looking for a maximum
   *            value.
   * @author Stephen Lambright
   * @internal
   *   @history 2011-07-02 Jeannie Backer - Moved this code from
   *                           ObliqueCylindrical class to its own method here.
   *                           Replaced parameters minVal and maxVal with
   *                           minBorderX, minBorderY, maxBorderX, and
   *                           maxBorderY.
   */
  // void Projection::findExtreme(double &minBorder,  double &maxBorder,
  //                              double &minBorderX, double &minBorderY,
  //                              double &maxBorderX, double &maxBorderY,
  //                              const double constBorder, bool searchX,
  //                              bool searchLongitude, bool findMin) {
  //   if (minBorder == Null || maxBorder == Null || constBorder == Null) {
  //     minBorderX = Null;
  //     minBorderY = minBorderX;
  //     minBorderX = minBorderX;
  //     minBorderY = minBorderX;
  //     return;
  //   }
  //   if (!searchLongitude && (fabs(fabs(constBorder) - 90.0) < DBL_EPSILON)) {
  //     // it is impossible to search "along" a pole
  //     setSearchGround(minBorder, constBorder, searchLongitude);
  //     minBorderX = XCoord();
  //     minBorderY = YCoord();
  //     maxBorderX = minBorderX;
  //     maxBorderY = minBorderY;
  //     return;
  //   }
  //   // Always do 10 steps
  //   const double STEP_SIZE = (maxBorder - minBorder) / 10.0;
  //   const double LOOP_END = maxBorder + (STEP_SIZE / 2.0); // This ensures we do
  //                                                          // all of the steps
  //                                                          // properly
  //   double currBorderVal = minBorder;
  //   setSearchGround(minBorder, constBorder, searchLongitude);

  //   // this makes sure that the initial currBorderVal is valid before entering
  //   // the loop below
  //   if (!m_good){
  //     // minBorder = currBorderVal+STEP_SIZE < LOOP_END until setGround is good?
  //     // then, if still not good return?
  //     while (!m_good && currBorderVal <= LOOP_END) {
  //       currBorderVal+=STEP_SIZE;
  //       if (searchLongitude && (currBorderVal - 90.0 > DBL_EPSILON)) {
  //         currBorderVal = 90.0;
  //       }
  //       setSearchGround(currBorderVal, constBorder, searchLongitude);
  //     }
  //     if (!m_good) {
  //       minBorderX = Null;
  //       minBorderY = minBorderX;
  //       minBorderX = minBorderX;
  //       minBorderY = minBorderX;
  //       return;
  //     }
  //   }

  //   // save the values of three consecutive steps from the minBorder towards
  //   // the maxBorder along the constBorder. initialize these three border
  //   // values (the non-constant lat or lon)
  //   double border1 = currBorderVal;
  //   double border2 = currBorderVal;
  //   double border3 = currBorderVal;

  //   // save the coordinate (x or y) values that correspond to the first
  //   // two borders that are being saved.
  //   // initialize these two coordinate values (x or y)
  //   double value1 = (searchX) ? XCoord() : YCoord();
  //   double value2 = value1;

  //   // initialize the extreme coordinate value
  //   // -- this is the largest coordinate value found so far
  //   double extremeVal2 = value2;

  //   // initialize the extreme border values
  //   // -- these are the borders on either side of the extreme coordinate value
  //   double extremeBorder1 = minBorder;
  //   double extremeBorder3 = minBorder;

  //   while (currBorderVal <= LOOP_END) {

  //     // this conditional was added to prevent trying to SetGround with an
  //     // invalid latitude greater than 90 degrees. There is no need check for
  //     // latitude less than -90 since we start at the minBorder (already
  //     // assumed to be valid) and step forward toward (and possibly past)
  //     // maxBorder
  //     if (searchLongitude && (currBorderVal - 90.0 > DBL_EPSILON)) {
  //       currBorderVal = 90.0;
  //     }

  //     // update the current border value along constBorder
  //     currBorderVal += STEP_SIZE;
  //     setSearchGround(currBorderVal, constBorder, searchLongitude);
  //     if (!m_good){
  //       continue;
  //     }

  //     // update the border and coordinate values
  //     border3 = border2;
  //     border2 = border1;
  //     border1 = currBorderVal;
  //     value2 = value1;
  //     value1 = (searchX) ? XCoord() : YCoord();

  //     if ((findMin && value2 < extremeVal2)
  //         || (!findMin && value2 > extremeVal2)) {
  //       // Compare the coordinate value associated with the center border with
  //       // the current extreme. If the updated coordinate value is more extreme
  //       // (smaller or larger, depending on findMin), then we update the
  //       // extremeVal and it's borders.
  //       extremeVal2 = value2;

  //       extremeBorder3 = border3;
  //       extremeBorder1 = border1;
  //     }
  //   }

  //   // update min/max border values to the values on either side of the most
  //   // extreme coordinate found in this call to this method

  //   minBorder = extremeBorder3; // Border 3 is lagging and thus smaller

  //   // since the loop steps past the original maxBorder, we want to retain
  //   // the original maxBorder value so we don't go outside of the original
  //   // min/max range given
  //   if (extremeBorder1 <= maxBorder ) {
  //     maxBorder = extremeBorder1; // Border 1 is leading and thus larger
  //   }

  //   // update minBorder coordinate values
  //   setSearchGround(minBorder, constBorder, searchLongitude);
  //   // if (!m_good){
  //   //   this should not happen since minBorder has already been verified in
  //   //   the while loop above
  //   // }

  //   minBorderX = XCoord();
  //   minBorderY = YCoord();

  //   // update maxBorder coordinate values
  //   setSearchGround(maxBorder, constBorder, searchLongitude);
  //   // if (!m_good){
  //   //   this should not happen since maxBorder has already been verified in
  //   //   the while loop above
  //   // }

  //   maxBorderX = XCoord();
  //   maxBorderY = YCoord();
  //   return;
  // }

  /**
   * This function sets the ground for the given border values.  It calls the
   * SetGround(lat, lon) method with the appropriate lat/lon values, depending
   * on whether variableIsLat is true.
   *
   * This method is used by doSearch and findExtreme in order to set the ground
   * correctly each time.
   *
   * @param variableBorder The latitude or longitude that is variable in the
   *            search methods.
   * @param constBorder The latitude or longitude that is constant in the search
   *            methods.
   * @param variableIsLat Indicates whether variableBorder is the latittude
   *            value and constBorder is the longitude value. If false,
   *            variableBorder is the longitude value and constBorder is the
   *            latitude value.
   * @author Stephen Lambright
   * @internal
   *   @history 2011-07-02 Jeannie Backer - Moved this code from
   *                           ObliqueCylindrical class to its own method here.
   *                           Added error.
   */
  // void Projection::setSearchGround(const double variableBorder,
  //                                  const double constBorder,
  //                                  bool variableIsLat) {
  //   if (variableBorder == Null || constBorder == Null) {
  //     return;
  //   }
  //   double lat, lon;
  //   if (variableIsLat) {
  //     lat = variableBorder;
  //     lon = constBorder;
  //   }
  //   else {
  //     lat = constBorder;
  //     lon = variableBorder;
  //   }
  //   SetGround(lat, lon);
  //   return;
  // }

  /**
   * This method sets the UpperLeftCornerX and UpperLeftCornerY keywords in the
   * projection mapping group, in meters.
   *
   * @param x the upper left corner x value
   * @param y the upper left corner y value
   */
  void Projection::SetUpperLeftCorner(const Displacement &x,
                                      const Displacement &y) {
    PvlKeyword xKeyword("UpperLeftCornerX", toString(x.meters()), "meters");
    PvlKeyword yKeyword("UpperLeftCornerY", toString(y.meters()), "meters");
    m_mappingGrp.addKeyword(xKeyword,Pvl::Replace);
    m_mappingGrp.addKeyword(yKeyword,Pvl::Replace);
  }
} //end namespace isis
