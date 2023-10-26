/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Latitude.h"

#include <cmath>

#include "Constants.h"
#include "Distance.h"
#include "IException.h"
#include "IString.h"
#include "PvlGroup.h"
#include "QString"
#include "SpecialPixel.h"
#include "Target.h"

namespace Isis {
  
  /**
   * Create a blank Latitude object without Planetographic support.
   */
  Latitude::Latitude() : Angle() {
    
    m_equatorialRadius = NULL;
    m_polarRadius = NULL;

    m_errors = AllowPastPole;
  }


  /**
   * Create and initialize a Latitude value without planetographic support.
   *
   * @param latitude The latitude value this instance will represent,
   *     in the planetocentric coordinate system
   * @param latitudeUnits The angular units of the latitude value (degs, rads)
   * @param errors Error checking conditions
   * 
   * @see ErrorChecking
   * @see CoordinateType
   */
  Latitude::Latitude(double latitude, Angle::Units latitudeUnits,
                     ErrorChecking errors) : Angle() {

    m_equatorialRadius = NULL;
    m_polarRadius = NULL;

    m_errors = errors;

    setPlanetocentric(latitude, latitudeUnits);
  }


  /**
   * Create and initialize a Latitude value in the planetocentric domain within
   * the given angle.
   *
   * @param latitude The latitude value this instance will represent
   * @param errors Error checking conditions
   */
  Latitude::Latitude(Angle latitude, ErrorChecking errors) : Angle() {
    
    m_equatorialRadius = NULL;
    m_polarRadius = NULL;

    m_errors = errors;

    setPlanetocentric(latitude.radians(), Radians);
  }


  /**
   * Create and initialize a latitude value using the mapping group's latitude
   * units and radii.
   *
   * @param latitude The latitude value this instance will represent,
   *     in the mapping group's units
   * @param mapping A mapping group
   * @param errors Error checking conditions
   * 
   * @throws IException::Unknown "Unable to create Latitude object from given mapping group."
   * @throws IException::Programmer "Latitude type is not recognized"
   * 
   * @see ErrorChecking
   * @see CoordinateType
   */
  Latitude::Latitude(Angle latitude, PvlGroup mapping,
                     ErrorChecking errors) : Angle(latitude) {
    
    m_equatorialRadius = NULL;
    m_polarRadius = NULL;

    if (mapping.hasKeyword("EquatorialRadius") && mapping.hasKeyword("PolarRadius")) {
      m_equatorialRadius = new Distance(std::stod(mapping["EquatorialRadius"][0]),
                                        Distance::Meters);
      m_polarRadius = new Distance(std::stod(mapping["PolarRadius"][0]),
                                   Distance::Meters);
    }
    else {
      try {
        PvlGroup radiiGrp = Target::radiiGroup(QString::fromStdString(mapping["TargetName"][0]));
        m_equatorialRadius = new Distance(std::stod(radiiGrp["EquatorialRadius"][0]),
                                          Distance::Meters);
        m_polarRadius = new Distance(std::stod(radiiGrp["PolarRadius"][0]),
                                     Distance::Meters);
      }
      catch (IException &e) {
        QString msg = "Unable to create Latitude object from given mapping group.";
        throw IException(e, IException::Unknown, msg, _FILEINFO_);
      }
    }

    m_errors = errors;

    if (mapping["LatitudeType"][0] == "Planetographic") {
      setPlanetographic(latitude.radians(), Radians);
    }
    else if (mapping["LatitudeType"][0] == "Planetocentric") {
      setPlanetocentric(latitude.radians(), Radians);
    }
    else {
      std::string msg = "Latitude type [" + mapping["LatitudeType"][0] +
        "] is not recognized";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Create and initialize a latitude value using the latitude units and the
   * mapping group's radii.
   *
   * @param latitude The latitude value this instance will represent,
   *     in the mapping group's units
   * @param mapping A mapping group
   * @param latitudeUnits The angular units of the latitude value (degs, rads)
   * @param errors Error checking conditions
   * 
   * @throws IException::Unknown "Unable to create Latitude object from given mapping group."
   * @throws IException::Programmer "Latitude type is not recognized"
   *
   * @see ErrorChecking
   * @see CoordinateType
   */
  Latitude::Latitude(double latitude,
                     PvlGroup mapping,
                     Angle::Units latitudeUnits,
                     ErrorChecking errors) : Angle(latitude, latitudeUnits) {

    m_equatorialRadius = NULL;
    m_polarRadius = NULL;

    if (mapping.hasKeyword("EquatorialRadius") && mapping.hasKeyword("PolarRadius")) {
      m_equatorialRadius = new Distance(std::stod(mapping["EquatorialRadius"][0]),
                                        Distance::Meters);
      m_polarRadius = new Distance(std::stod(mapping["PolarRadius"][0]),
                                   Distance::Meters);
    }
    else {
      try {
        PvlGroup radiiGrp = Target::radiiGroup(QString::fromStdString(mapping["TargetName"][0]));
        m_equatorialRadius = new Distance(std::stod(radiiGrp["EquatorialRadius"][0]),
                                          Distance::Meters);
        m_polarRadius = new Distance(std::stod(radiiGrp["PolarRadius"][0]),
                                     Distance::Meters);
      }
      catch (IException &e) {
        QString msg = "Unable to create Latitude object from given mapping group.";
        throw IException(e, IException::Unknown, msg, _FILEINFO_);
      }
    }

    m_errors = errors;

    if (mapping["LatitudeType"][0] == "Planetographic") {
      setPlanetographic(latitude, latitudeUnits);
    }
    else if (mapping["LatitudeType"][0] == "Planetocentric") {
      setPlanetocentric(latitude, latitudeUnits);
    }
    else {
      std::string msg = "Latitude type [" + mapping["LatitudeType"][0] +
        "] is not recognized";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Create and initialize a Latitude value with planetographic support.
   *
   * @param latitude The latitude value this instance will represent,
   *     in planetocentric
   * @param equatorialRadius Radius of the target (planet) at the equator
   * @param polarRadius Radius of the target (planet) at the poles
   * @param latType The coordinate system of the latitude parameter
   * @param latitudeUnits The angular units of the latitude value (degs, rads)
   * @param errors Error checking conditions
   * 
   * @throws IException::Programmer "Enumeration value [latType] is not a valid CoordinateType"
   *
   * @see ErrorChecking
   * @see CoordinateType
   */
  Latitude::Latitude(double latitude,
                     Distance equatorialRadius, Distance polarRadius,
                     CoordinateType latType,
                     Angle::Units latitudeUnits,
                     ErrorChecking errors) : Angle(latitude, latitudeUnits) {
    
    m_equatorialRadius = NULL;
    m_polarRadius = NULL;

    m_equatorialRadius = new Distance(equatorialRadius);
    m_polarRadius = new Distance(polarRadius);

    m_errors = errors;

    if (latType == Planetocentric) {
      setPlanetocentric(latitude, latitudeUnits);
    }
    else if (latType == Planetographic) {
      setPlanetographic(latitude, latitudeUnits);
    }
    else {
      QString msg = "Enumeration value [" + toString(latType) + "] is not a valid CoordinateType";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * This copies the given latitude exactly.
   *
   * @param latitudeToCopy The latitude we're duplicating
   */
  Latitude::Latitude(const Latitude &latitudeToCopy) : Angle(latitudeToCopy) {
    
    m_equatorialRadius = NULL;
    m_polarRadius = NULL;

    m_errors = latitudeToCopy.m_errors;

    if (latitudeToCopy.m_equatorialRadius) {
      m_equatorialRadius = new Distance(*latitudeToCopy.m_equatorialRadius);
    }

    if (latitudeToCopy.m_polarRadius) {
      m_polarRadius = new Distance(*latitudeToCopy.m_polarRadius);
    }
  }


  /**
   * This cleans up the Latitude class.
   */
  Latitude::~Latitude() {
    if (m_equatorialRadius) {
      delete m_equatorialRadius;
      m_equatorialRadius = NULL;
    }

    if (m_polarRadius) {
      delete m_polarRadius;
      m_polarRadius = NULL;
    }
  }


  /**
   * Get the latitude in the planetocentric (universal) coordinate system.
   * 
   * @param units The angular units to get the latitude in
   *
   * @see CoordinateType
   * 
   * @return The Planetocentric latitude value
   */
  double Latitude::planetocentric(Angle::Units units) const {
    return angle(units);
  }


  /**
   * Set the latitude given a value in the Planetocentric coordinate system
   *
   * @param latitude The planetographic latitude to set ourselves to
   * @param units The angular units latitude is in
   */
  void Latitude::setPlanetocentric(double latitude, Angle::Units units) {
    setAngle(latitude, units);
  }


  /**
   * Get the latitude in the planetographic coordinate system. If this instance was 
   *   not constructed with the planetary radii, then an exception will be thrown.
   *
   * @param units The angular units to get the latitude in
   * 
   * @throws IException::Programmer "The latitude cannot be converted to Planetographic 
   *     without the planetary radii, please use the other Latitude constructor"
   * @throws IException::Programmer "Latitudes outside of the -90/90 range cannot be 
   *     converted between Planetographic and Planetocentric"
   * @throws IException::Programmer "Invalid planetographic latitudes are not currently 
   *     supported"
   * 
   * @see CoordinateType
   *
   * @return The Planetographic latitude value
   */
  double Latitude::planetographic(Angle::Units units) const {
    
    if (m_equatorialRadius == NULL || m_polarRadius == NULL) {
      QString msg = "Latitude [" + toString(true) + "] cannot "
          "be converted to Planetographic without the planetary radii, please "
          "use the other Latitude constructor.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (*this > Angle(90.0, Angle::Degrees) ||
        *this < Angle(-90.0, Angle::Degrees)) {
      QString msg = "Latitudes outside of the -90/90 range cannot be converted "
          "between Planetographic and Planetocentric";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (!isValid()) {
      QString msg = "Invalid planetographic latitudes are not currently "
          "supported";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    double ographicLatitude = atan(tan(radians()) *
        (*m_equatorialRadius / *m_polarRadius) *
        (*m_equatorialRadius / *m_polarRadius));

    // This theoretically should just be an angle, but make it a Latitude so
    //   we can access angle
    return Latitude(ographicLatitude, Angle::Radians).angle(units);
  }


  /**
   * Set the latitude given a value in the Planetographic coordinate system
   *
   * @param latitude The planetographic latitude to set ourselves to
   * @param units The angular units latitude is in
   * 
   * @throws IException::Programmer "The latitude cannot be converted to Planetographic 
   *     without the planetary radii, please use the other Latitude constructor"
   * @throws IException::Programmer "Latitudes outside of the -90/90 range cannot be 
   *     converted between Planetographic and Planetocentric"
   * @throws IException::Programmer "Invalid planetographic latitudes are not currently 
   *     supported"
   */
  void Latitude::setPlanetographic(double latitude, Angle::Units units) {
    
    if (m_equatorialRadius == NULL || m_polarRadius == NULL) {
      QString msg = "Latitude [" + Isis::toString(latitude) + " degrees] cannot be "
          "converted to Planetocentic without the planetary radii, please use "
          "the other Latitude constructor.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    Angle inputAngle(latitude, units);

    if (inputAngle > Angle(90.0, Angle::Degrees) ||
        inputAngle < Angle(-90.0, Angle::Degrees)) {
      QString msg = "Latitudes outside of the -90/90 range cannot be converted "
          "between Planetographic and Planetocentric";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Since the Angle constructor handles special pixels, we will never get to this line of code
    // when passing in a special pixel.
    // Left this here just in case the functionality in Angle changes.
    if (IsSpecial(latitude)) {  
      QString msg = "Invalid planetographic latitudes are not currently "
          "supported";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    double ocentricLatitude = atan(tan(inputAngle.radians()) *
        (*m_polarRadius / *m_equatorialRadius) *
        (*m_polarRadius / *m_equatorialRadius));

    // Sometimes the trig functions return the negative of the expected value at the pole.
    if ((ocentricLatitude > 0) != (inputAngle.radians() > 0)) {
      ocentricLatitude *= -1;
    }

    setAngle(ocentricLatitude, Angle::Radians);
  }


  /**
   * Get the error checking status. This indicates if the Latitude object will
   * throw an error when set to an angle less than -90 degrees or greater than
   * 90 degrees.
   * 
   * @return @b ErrorChecking The error checking status.
   */
  Latitude::ErrorChecking Latitude::errorChecking() const {
    return m_errors;
  }


  /**
   * Set the error checking status. If set to ThrowAllErrors, then an exception
   * will be thrown if the Latitude object is set to an angle less than -90
   * degrees or greater than 90 degrees. If set to AllowPastPole, then no
   * exception will be thrown.
   * 
   * @param error The new error checking status.
   */
  void Latitude::setErrorChecking(ErrorChecking errors) {
    m_errors = errors;
  }


  /**
   * Checks if this latitude value is within the given range.  Defines the
   * range as the change from the minimum latitude to the maximum latitude (an
   * angle), and returns whether the change from the minimum latitude to this
   * latitude is less than or equal to the maximum change allowed (the range).
   *
   * @param min The beginning of the valid latitude range
   * @param max The end of the valid latitude range
   * 
   * @throws IException::User "The minimum latitude degrees is greater 
   *     than the maximum latitude degrees"
   *
   * @return Whether the latitude is in the given range
   */
  bool Latitude::inRange(Latitude min, Latitude max) const {
    
    // Validity check on the range
    if (min > max) {
      QString msg = "Minimum latitude [" + min.toString(true) + 
                    "] is greater than maximum latitude [" + 
                    max.toString(true) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Provide a little wriggle room for precision problems
    Angle epsilon(DBL_EPSILON, Angle::Degrees);
    Latitude adjustedMin = min - epsilon;
    Latitude adjustedMax = max + epsilon;

    // Is this latitude between the min and the max
    return *this >= adjustedMin && *this <= adjustedMax;
  }


  /**
   * This assigns another latitude to this one - making this latitude an
   *   exact duplicate of the other.
   *
   * @param latitudeToCopy The latitude we are assigning from
   * 
   * @return A reference to the dublicate latitude
   */
  Latitude& Latitude::operator=(const Latitude & latitudeToCopy) {
    
    if (this == &latitudeToCopy) return *this;

    m_equatorialRadius = NULL;
    m_polarRadius = NULL;

    m_errors = latitudeToCopy.m_errors;

    if (latitudeToCopy.m_equatorialRadius) {
      m_equatorialRadius = new Distance(*latitudeToCopy.m_equatorialRadius);
    }

    if (latitudeToCopy.m_polarRadius) {
      m_polarRadius = new Distance(*latitudeToCopy.m_polarRadius);
    }

    setPlanetocentric(latitudeToCopy.planetocentric());

    return *this;
  }


  /**
   * Adds an angle to this latitude. The adding method is determined by the
   *   latitude type.
   *
   * @param angleToAdd the latitude being added to this one
   * @param mapping the mapping group from a projection
   * 
   * @throws IException::Unknown "Unable to add angle to Latitude object 
   *     from given mapping group."
   * @throws IException::Programmer "Latitude type is not recognized"
   * 
   * @return The result of adding an angle to the latitude
   */
  Latitude Latitude::add(Angle angleToAdd, PvlGroup mapping) {

    CoordinateType latType;

    Distance equatorialRadius;
    Distance polarRadius;
    if (mapping.hasKeyword("EquatorialRadius") && mapping.hasKeyword("PolarRadius")) {
      equatorialRadius = Distance(std::stod(mapping["EquatorialRadius"][0]),
                                  Distance::Meters);
      polarRadius = Distance(std::stod(mapping["PolarRadius"][0]),
                             Distance::Meters);
    }
    else {
      try {
        PvlGroup radiiGrp = Target::radiiGroup(QString::fromStdString(mapping["TargetName"][0]));
        equatorialRadius = Distance(std::stod(radiiGrp["EquatorialRadius"][0]),
                                    Distance::Meters);
        polarRadius = Distance(std::stod(radiiGrp["PolarRadius"][0]),
                               Distance::Meters);
      }
      catch (IException &e) {
        QString msg = "Unable to add angle to Latitude object from given mapping group.";
        throw IException(e, IException::Unknown, msg, _FILEINFO_);
      }
    }

    if (mapping["LatitudeType"][0] == "Planetocentric")
      latType = Planetocentric;
    else if (mapping["LatitudeType"][0] == "Planetographic")
      latType = Planetographic;
    else {
      std::string msg = "Latitude type [" + mapping["LatitudeType"][0] + "] is not recognized";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return add(angleToAdd, equatorialRadius, polarRadius, latType);
  }


  /**
   * Adds another latitude to this one. Handles planetographic latitudes.
   *
   * @param angleToAdd the latitude being added to this one
   * @param equatorialRadius Radius of the target (planet) at the equator
   * @param polarRadius Radius of the target (planet) at the poles
   * @param latType Planetocentric or Planetographic
   * 
   * @return The result of adding another latitude
   */
  Latitude Latitude::add(Angle angleToAdd, 
                         Distance equatorialRadius, 
                         Distance polarRadius,
                         CoordinateType latType) {
    Latitude result;

    switch (latType) {
      case Planetocentric:
        result = Latitude(planetocentric() + angleToAdd.radians(), equatorialRadius, polarRadius,
                          latType, Radians, m_errors);
        break;

      case Planetographic:
        result = Latitude(planetographic() + angleToAdd.radians(), equatorialRadius, polarRadius,
                          latType, Radians, m_errors);
        break;
    }

    return result;
  }


  /**
   * We're overriding this method in order to do -90/90 degree checking
   *
   * @param angle The numeric value of the angle
   * @param units The units the angle is in (radians or degrees typically)
   * 
   * @throws IException::Programmer "Latitudes past 90 degrees are not valid. 
   *     The latitude is not allowed"
   */
  void Latitude::setAngle(const double &angle, 
                          const Angle::Units &units) {
    
    // Check for passing 90 degrees if that error checking is on
    if (!IsSpecial(angle) && (m_errors & AllowPastPole) != AllowPastPole) {
      Angle tmpAngle(angle, units);
      if (tmpAngle > Angle(90, Angle::Degrees) ||
          tmpAngle < Angle(-90, Angle::Degrees)) {
        QString msg = "Latitudes past 90 degrees are not valid. The latitude [" 
                      + Isis::toString(tmpAngle.degrees(), 8) + "] is not allowed";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }
    Angle::setAngle(angle, units);
  }
}
