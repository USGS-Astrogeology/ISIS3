/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Distance.h"

#include "Displacement.h"
#include "IException.h"
#include "IString.h"
#include "SpecialPixel.h"

namespace Isis {

  /**
   * This initializes the distance to an invalid state. You must set the
   *   distance later on with operator= or one of the Set methods.
   */
  Distance::Distance() {
    setDistance(Null, Meters);
  }


  /**
   * This is the general purpose constructor for Distance. This will initialize
   *   with the given distance. If Pixels are supplied as the units, then a
   *   default pixels/meter = 1.0 will be used.
   *
   * @param distance The distance to initialize with, must be in units of
   *     distanceUnit and not be negative
   * @param distanceUnit The unit distance is in, can be any value in
   *     Distance::Units
   */
  Distance::Distance(double distance, Units distanceUnit) {
    if(distanceUnit == Pixels)
      distanceUnit = Meters;

    setDistance(distance, distanceUnit);
  }


  /**
   * This is a constructor for Distance given pixels with a conversion ratio.
   *   This will initialize with the pixels converted to meters.
   *
   * @param distanceInPixels The distance to initialize with, must be in units
   *     of pixels and should not be negative
   * @param pixelsPerMeter The pixels/meter conversion factor
   */
  Distance::Distance(double distanceInPixels, double pixelsPerMeter) {
    setDistance(distanceInPixels / pixelsPerMeter, Meters);
  }


  /**
   * This is the copy constructor for Distance. The distance passed in will be
   *   exactly duplicated.
   *
   * @param distanceToCopy This is the distance we are making an exact duplicate
   *    of
   */
  Distance::Distance(const Distance &distanceToCopy) {
    // Use meters because it is the stored format, no precision loss
    setDistance(distanceToCopy.meters(), Meters);
  }


  /**
   * This will free the memory allocated by this instance of the Distance class.
   */
  Distance::~Distance() {
    // This will help debug memory problems, better to reset to obviously bad
    //   values in case we're used after we're deleted.
    m_distanceInMeters = Null;
  }


  /**
   * Get the distance in meters.
   *
   * @return Current distance, in meters, guaranteed to be >= 0.0
   */
  double Distance::meters() const {
    return distance(Meters);
  }


  /**
   * Set the distance in meters.
   *
   * @param distanceInMeters This is the value to set this distance to, given in
   *     meters. This will throw an exception if the value is negative.
   */
  void Distance::setMeters(double distanceInMeters) {
    setDistance(distanceInMeters, Meters);
  }


  /**
   * Get the distance in kilometers.
   *
   * @return Current distance, in kilometers, guaranteed to be >= 0.0
   */
  double Distance::kilometers() const {
    return distance(Kilometers);
  }


  /**
   * Set the distance in kilometers.
   *
   * @param distanceInKilometers This is the value to set this distance to,
   *     given in kilometers. This will throw an exception if the value is
   *     negative.
   */
  void Distance::setKilometers(double distanceInKilometers) {
    setDistance(distanceInKilometers, Kilometers);
  }


  /**
   * Get the distance in pixels using the given conversion ratio.
   *
   * @param pixelsPerMeter Pixels/Meters conversion ratio to use, stored data
   *         is always in meters
   * @return Current distance, in pixels, guaranteed to be >= 0.0 if
   *         pixelsPerMeter is positive
   */
  double Distance::pixels(double pixelsPerMeter) const {
    return distance(Meters) * pixelsPerMeter;
  }


  /**
   * Set the distance in pixels.
   *
   * @param distanceInPixels This is the value to set this distance to,
   *     given in pixels. This will throw an exception if the distance is
   *     negative after the conversion to meters.
   * @param pixelsPerMeter Pixels/Meters conversion ratio to use, stored data
   *         is always in meters
   */
  void Distance::setPixels(double distanceInPixels, double pixelsPerMeter) {
    setDistance(distanceInPixels / pixelsPerMeter, Meters);
  }


  /**
   * Get the distance in solar radii (a unit of ~696,265km).
   *
   * @return Current distance, in solar radii, guaranteed to be >= 0.0
   */
  double Distance::solarRadii() const {
    return distance(SolarRadii);
  }


  /**
   * Set the distance in solar radii.
   *
   * @param distanceInSolarRadii This is the value to set this distance to,
   *     given in solar radii. This will throw an exception if the value is
   *     negative.
   */
  void Distance::setSolarRadii(double distanceInSolarRadii) {
    setDistance(distanceInSolarRadii, SolarRadii);
  }


  /**
   * Get a textual representation of this distance.
   *
   * @return XXX meters (or empty string if not valid).
   */
  QString Distance::toString() const {
    QString string;

    if (isValid())
      string = QString::number(meters()) + " meters";

    return string;
  }


  /**
   * Test if this distance has been initialized or not
   *
   * @return True if this distance has been initialized.
   */
  bool Distance::isValid() const {
    return distance(Meters) != Null;
  }


  /**
    * Compare two distances with the greater than operator.
    *
    * @param otherDistance This is the distance we're comparing to, i.e. on
    *     the right hand side of the operator when used
    * @return True if this distance is greater than the given distance
    */
  bool Distance::operator >(const Distance &otherDistance) const {
    if(!isValid() || !otherDistance.isValid()) {
      IString msg = "Distance has not been initialized, you must initialize "
          "it first before comparing with another distance using [>]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return meters() > otherDistance.meters();
  }


  /**
    * Compare two distances with the less than operator.
    *
    * @param otherDistance This is the distance we're comparing to, i.e. on
    *     the right hand side of the operator when used
    * @return True if this distance is less than the given distance
    */
  bool Distance::operator <(const Distance &otherDistance) const {
    if(!isValid() || !otherDistance.isValid()) {
      IString msg = "Distance has not been initialized, you must initialize "
          "it first before comparing with another distance using [<]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return meters() < otherDistance.meters();
  }


  /**
   * Assign this distance to the value of another distance.
   *
   * @param distanceToCopy This is the distance we are to duplicate exactly
   * @return Resulting distance, a reference to this distance after assignment
   */
  Distance &Distance::operator =(const Distance &distanceToCopy) {
    if(this == &distanceToCopy) return *this;

    setDistance(distanceToCopy.meters(), Meters);

    return *this;
  }


  /**
   * Add another distance to this distance (1km + 1m = 1005m)
   *
   * @param distanceToAdd This is the distance we are adding to ourselves
   * @return Resulting distance, self not modified
   */
  Distance Distance::operator +(const Distance &distanceToAdd) const {
    if(!isValid() || !distanceToAdd.isValid()) return Distance();

    return Distance(meters() + distanceToAdd.meters(), Meters);
  }


  /**
   * Subtract another distance from this distance (1km - 1m = 995m). This could
   *   throw an exception if the result is negative.
   *
   * @param distanceToSub This is the distance we are subtracting from ourself
   * @return Resulting distance, self not modified
   */
  Displacement Distance::operator -(const Distance &distanceToSub) const {
    if(!isValid() || !distanceToSub.isValid()) return Displacement();

    Displacement result(meters() - distanceToSub.meters(),
        Displacement::Meters);

    return result;
  }


  /**
   * Divide another distance into this distance (5m / 1m = 5).
   *
   * @param distanceToDiv This is the divisor displacement (denominator)
   * @return Resulting value
   */
  double Distance::operator /(const Distance &distanceToDiv) const {
    if(!isValid() || !distanceToDiv.isValid()) return Null;

    double result = meters() / distanceToDiv.meters();
    return result;
  }


  /**
   * Divide a value from this distance (5m / 2 = 2.5m).
   *
   * @param valueToDiv This is the divisor displacement (denominator)
   * @return Resulting value
   */
  Distance Distance::operator /(const double &valueToDiv) const {
    if(!isValid() || IsSpecial(valueToDiv)) return Distance();

    Distance result = Distance(meters() / valueToDiv, Meters);

    return result;
  }


  /**
   * Multiply this distance by a value (5m * 2 = 10m).
   *
   * @param valueToMult This is the value to multiply by
   * @return Resulting value
   */
  Distance Distance::operator *(const double &valueToMult) const {
    if(!isValid() || IsSpecial(valueToMult)) return Distance();

    Distance result = Distance(meters() * valueToMult, Meters);

    return result;
  }


  /**
   * Multiply this distance by a value (5m * 2 = 10m).
   *
   * @param mult This is the value to multiply by
   * @param dist This is the distance to multiply into
   * @return Resulting value
   */
  Distance operator *(double mult, Distance dist) {
    Distance result = dist * mult;
    return result;
  }


  /**
   * Add and assign the given distance to ourselves.
   *
   * @param distanceToAdd This is the distance we are to duplicate exactly
   */
  void Distance::operator +=(const Distance &distanceToAdd) {
    if(!isValid() || !distanceToAdd.isValid()) {
      setDistance(Null, Meters);
    }
    else {
      setDistance(meters() + distanceToAdd.meters(), Meters);
    }
  }


  /**
   * Subtract and assign the given distance from ourself. This could throw
   *   an exception if the result is negative, in which case the new value is
   *   never applied.
   *
   * @param distanceToSub This is the distance we are to subtract
   */
  void Distance::operator -=(const Distance &distanceToSub) {
    if(!isValid() || !distanceToSub.isValid()) {
      setDistance(Null, Meters);
    }
    else {
      setDistance(meters() - distanceToSub.meters(), Meters);
    }
  }


  /**
   * Divide this distance by a value and assign the result to ourself.
   *
   * @param valueToDiv This is the displacement we are to divide by
   */
  void Distance::operator /=(const double &valueToDiv) {
    if(!isValid() || IsSpecial(valueToDiv)) {
      setDistance(Null, Meters);
    }
    else {
      setDistance(meters() / valueToDiv, Meters);
    }
  }


  /**
   * Multiply this distance by a value and assign the result to ourself.
   *
   * @param valueToMult This is the value we are going to multiply by
   */
  void Distance::operator *=(const double &valueToMult) {
    if(!isValid() || IsSpecial(valueToMult)) {
      setDistance(Null, Meters);
    }
    else {
      setDistance(meters() * valueToMult, Meters);
    }
  }


  /**
   * This is a helper method to access distances in a universal manner with
   *   uniform error checking.
   *
   * @param distanceUnit Unit of the return value. If this is invalid, an
   *     exception will be thrown.
   * @return The distance in units of distanceUnit
   */
  double Distance::distance(Units distanceUnit) const {
    double distanceInMeters = m_distanceInMeters;
    double resultingDistance = Null;

    if(m_distanceInMeters == Null) return Null;

    switch(distanceUnit) {
      case Meters:
        resultingDistance = distanceInMeters;
        break;

      case Kilometers:
        resultingDistance = distanceInMeters / 1000.0;
        break;

      case Pixels: {
        IString msg = "Cannot call distance() with pixels, ask for another "
                      "unit";
        throw IException(IException::Programmer, msg, _FILEINFO_);
        break;
      }

      case SolarRadii:
        resultingDistance = distanceInMeters / 6.9599e8;
        break;
    }

    if(resultingDistance == Null) {
      IString msg = "Distance does not understand the enumerated value [" +
          IString(distanceUnit) + "] as a unit";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return resultingDistance;
  }


  /**
   * This is a helper method to set distances in a universal manner with
   *   uniform error checking.
   *
   * @param distance The distance, in units of distanceUnit, to set this class
   *     to. If this is negative an exception will be thrown and the state
   *     unmodified.
   * @param distanceUnit Unit of distance. If this is invalid, an
   *     exception will be thrown and the state left unmodified.
   */
  void Distance::setDistance(const double &distance, Units distanceUnit) {
    double distanceInMeters = Null;

    if(IsSpecial(distance)) {
      m_distanceInMeters = Null;
      return;
    }

    switch(distanceUnit) {
      case Meters:
        distanceInMeters = distance;
        break;

      case Kilometers:
        distanceInMeters = distance * 1000.0;
        break;

      case Pixels: {
        IString msg = "Cannot setDistance with pixels, must convert to another "
            "unit first";
        throw IException(IException::Programmer, msg, _FILEINFO_);
        break;
      }

      case SolarRadii:
        distanceInMeters = distance * 6.9599e8;
        break;
    }

    if(distanceInMeters == Null) {
      IString msg = "Distance does not understand the enumerated value [" +
        IString(distanceUnit) + "] as a unit";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (distanceInMeters < 0.0) {
      IString msg = "Negative distances are not supported, the value [" +
        IString(distanceInMeters) + " meters] cannot be stored in the Distance "
        "class";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    m_distanceInMeters = distanceInMeters;
  }
}
