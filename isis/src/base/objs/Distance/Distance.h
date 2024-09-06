#ifndef Distance_h
#define Distance_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <QString>

class QString;

namespace Isis {
  class Displacement;

  /**
   * @brief Distance measurement, usually in meters
   *
   * This class is designed to encapsulate the concept of a distance. This is
   *   typically used for Radius and XYZ values but is also available as a
   *   general purpose class. This class does not accept negative values.
   *
   * @ingroup Utility
   *
   * @author 2010-10-12 Steven Lambright
   *
   * @internal
   *   @history 2011-04-18 Steven Lambright Improved special pixel math handling
   *   @history 2012-02-16 Steven Lambright - Brought up to method and member
   *                           naming standards.
   *   @history 2012-03-22 Steven Lambright - Added toString().
   *   @history 2012-03-22 Steven Lambright - Added solarRadii(), setSolarRadii(), and
   *                           the unit SolarRadii. References #1232.
   */
  class Distance {
    public:
      /**
       * This is a list of available units to access and store Distances in.
       *   These values can be passed to the constructor to specify which
       *   unit the double you are passing in is in.
       */
      enum Units {
        //! The distance is being specified in meters
        Meters,
        //! The distance is being specified in kilometers
        Kilometers,
        //! The distance is being specified in pixels
        Pixels,
        /**
         * "Solar radius is a unit of distance used to express the size of stars in astronomy equal
         *   to the current radius of the Sun."
         *
         * We're using 6.9599*10^8 meters, because that's the actual unit value, even though the
         *   radius has been more accurately calculated to 6.96342*10^8 m.
         *
         * http://en.wikipedia.org/wiki/Solar_radius
         * http://www.astro.wisc.edu/~dolan/constants.html
         * https://www.cfa.harvard.edu/~dfabricant/huchra/ay145/constants.html
         */
        SolarRadii
      };

      Distance();
      Distance(double distance, Units distanceUnit);
      Distance(double distanceInPixels, double pixelsPerMeter);
      Distance(const Distance &distanceToCopy);
      virtual ~Distance();

      double meters() const;
      void setMeters(double distanceInMeters);

      double kilometers() const;
      void setKilometers(double distanceInKilometers);

      double pixels(double pixelsPerMeter = 1.0) const;
      void setPixels(double distanceInPixels, double pixelsPerMeter = 1.0);

      double solarRadii() const;
      void setSolarRadii(double distanceInSolarRadii);

      QString toString() const;
      bool isValid() const;

      bool operator >(const Distance &otherDistance) const;
      bool operator <(const Distance &otherDistance) const;

      /**
       * Compare two distances with the greater than or equal to operator.
       *
       * @param otherDistance This is the distance we're comparing to, i.e. on
       *     the right hand side of the operator when used
       * @return True if this distance is greater than or equal to the given
       *     distance
       */
      bool operator >=(const Distance &otherDistance) const {
        return *this > otherDistance || *this == otherDistance;
      }


      /**
       * Compare two distances with the less than or equal to operator.
       *
       * @param otherDistance This is the distance we're comparing to, i.e. on
       *   the right hand side of the operator when used
       * @return True if this distance is less than or equal to the given
       *   distance
       */
      bool operator <=(const Distance &otherDistance) const {
        return *this < otherDistance || *this == otherDistance;
      }


      /**
       * Compare two distances with the == operator. Two uninitialized distances
       *   are equal to each other.
       *
       * @param otherDistance This is the distance we're comparing to, i.e. on
       *     the right hand side of the operator when used
       * @return True if this distance is equal to the given distance
       */
      bool operator ==(const Distance &otherDistance) const {
        return meters() == otherDistance.meters();
      }


      /**
       * Compare two distances with the != operator. Two uninitialized distances
       *   are equal to each other.
       *
       * @param otherDistance This is the distance we're comparing to, i.e. on
       *     the right hand side of the operator when used
       * @return True if this distance is not equal to the given distance
       */
      bool operator !=(const Distance &otherDistance) const {
        return meters() != otherDistance.meters();
      }


      Distance &operator =(const Distance &distanceToCopy);
      Distance operator +(const Distance &distanceToAdd) const;
      Displacement operator -(const Distance &distanceToSub) const;
      double operator /(const Distance &distanceToDiv) const;
      Distance operator /(const double &valueToDiv) const;
      Distance operator *(const double &valueToMult) const;
      friend Distance operator *(double mult, Distance dist);
      void operator +=(const Distance &distanceToAdd);
      void operator -=(const Distance &distanceToSub);
      void operator /=(const double &valueToDiv);
      void operator *=(const double &valueToMult);

    protected:
      virtual double distance(Units distanceUnit) const;
      virtual void setDistance(const double &distance, Units distanceUnit);

    private:
      /**
       * This is the distance value that this class is encapsulating, always
       *   stored in meters.
       */
      double m_distanceInMeters;
  };
}

#endif
