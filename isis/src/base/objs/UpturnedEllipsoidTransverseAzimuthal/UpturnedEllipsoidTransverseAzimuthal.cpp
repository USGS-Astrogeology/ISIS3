/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "UpturnedEllipsoidTransverseAzimuthal.h"

#include <cmath>
#include <cfloat>
#include <iostream>
#include <iomanip>

#include <QDebug>

#include "Constants.h"
#include "IException.h"
#include "IString.h"
#include "TProjection.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "SpecialPixel.h"

using namespace std;
namespace Isis {
  /**
   * Constructs an upturned ellipsoid transverse azimuthal map projection object
   *
   * @param label This argument must be a label containing the proper mapping
   *              information, as indicated in the Projection class.
   *              Additionally, the UpturnedEllipsoidTransverseAzimuthal projection
   *              requires the center longitude and center latitude to be
   *              defined in the keywords CenterLongitude and CenterLatitude,
   *              respectively.
   *
   * @param allowDefaults If set to false, the constructor expects that
   *                      keywords of CenterLongitude and CenterLatitude will be
   *                      in the label. Otherwise, it will attempt to compute
   *                      the center longitude and center latitude using
   *                      the midpoints of the longitude and latitude ranges
   *                      specified in the labels, respectively. Defaults to
   *                      false.
   *
   * @author 2016-03-18 Jeannie Backer 
   */
  UpturnedEllipsoidTransverseAzimuthal::UpturnedEllipsoidTransverseAzimuthal(Pvl &label,
                                               bool allowDefaults) :
      TProjection::TProjection(label) {
    try {
      // This algorithm can be found in the professional paper 
      // Cartographic Projections for Small Bodies of the Solar System
      // by Maria E Fleis, Kira B Shingareva,
      // Michael M Borisov, Michael V Alexandrovich, and Philip Stooke
      
      // Try to read the mapping group
      PvlGroup &mapGroup = label.findGroup("Mapping", Pvl::Traverse);

      // Compute and write the default center longitude if allowed and
      // necessary
      double centerLongitude = 0.0;
      if (!mapGroup.hasKeyword("CenterLongitude")) {
        if (allowDefaults) {
          // centerLongitude still 0.0 here
          mapGroup += PvlKeyword("CenterLongitude", Isis::toString(centerLongitude), "Degrees");
        }
        else {
          std::string message = "Cannot project using upturned ellipsoid Transverse Azimuthal";
          message += " without [CenterLongitude] value.  Keyword does not exist";
          message += " in labels and defaults are not allowed.";
          throw IException(IException::Unknown, message, _FILEINFO_);
        }
      }
      else {
        centerLongitude = mapGroup["CenterLongitude"];
      }

      if (MinimumLongitude() < centerLongitude - 90.0) {
        std::string message = "MinimumLongitude ["  
                          + Isis::toString(MinimumLongitude()) 
                          + "] is invalid. Must be within -90 degrees of the CenterLongitude ["
                          + Isis::toString(centerLongitude) + "].";
        throw IException(IException::Unknown, message, _FILEINFO_);
      }
      if (MaximumLongitude() > centerLongitude + 90.0) {
        std::string message = "MaximumLongitude ["  
                          + Isis::toString(MaximumLongitude()) 
                          + "] is invalid. Must be within +90 degrees of the CenterLongitude ["
                          + Isis::toString(centerLongitude) + "].";
        throw IException(IException::Unknown, message, _FILEINFO_);
      }

      // Get the center longitude  & latitude
      // initialize member variables
      init(centerLongitude);
    }
    catch(IException &e) {
      std::string message = "Invalid label group [Mapping]";
      throw IException(e, IException::Unknown, message, _FILEINFO_);
    }
  }
  

  /**
   * Destroys the UpturnedEllipsoidTransverseAzimuthal object
   * 
   * @author 2016-03-18 Jeannie Backer 
   */
  UpturnedEllipsoidTransverseAzimuthal::~UpturnedEllipsoidTransverseAzimuthal() {
  }


  /**
   * Compares two Projection objects to see if they are equal
   *
   * @param proj Projection object to do comparison on
   *
   * @return @b bool Returns true if the Projection objects are equal, and false if
   *              they are not
   * 
   * @author 2016-03-18 Jeannie Backer 
   */
  bool UpturnedEllipsoidTransverseAzimuthal::operator== (const Projection &proj) {
    // don't do the below it is a recursive plunge
    //  if (Projection::operator!=(proj)) return false;

    // all other data members calculated using m_lambda0, m_a and m_b.
    UpturnedEllipsoidTransverseAzimuthal *tcyl = (UpturnedEllipsoidTransverseAzimuthal *) &proj;
    if ((tcyl->m_lambda0 != m_lambda0) ||
        (tcyl->m_a != m_a) ||
        (tcyl->m_b != m_b)) return false;

    return true;
  }


  /**
   * Returns the name of the map projection.
   *
   * @return @b QString Name of projection
   * 
   * @author 2016-03-18 Jeannie Backer 
   */
  QString UpturnedEllipsoidTransverseAzimuthal::Name() const {
    return "UpturnedEllipsoidUpturnedEllipsoidTransverseAzimuthal";
  }


  /**
   * Returns the version of the map projection.
   *
   * @return @b QString Version number
   */
  QString  UpturnedEllipsoidTransverseAzimuthal::Version() const {
    return "1.0";
  }


  /**
   * Initialize member variables. 
   * 
   * @param centerLatitude The center latitude value found in the labels of the 
   *                       mapping group.
   * @param centerLongitude The center longitude value found in the labels of 
   *                       the mapping group.
   * 
   * 
   * @author 2016-03-18 Jeannie Backer 
   */
  void UpturnedEllipsoidTransverseAzimuthal::init(double centerLongitude) {

    // adjust for positive east longitude direction and convert to radians 
    if (IsPositiveEast()) {
      m_lambda0 = centerLongitude * DEG2RAD;
    }
    else {
      m_lambda0 = ToPositiveEast(centerLongitude, 360) * DEG2RAD;
    }
    if (Has180Domain()) {
      m_lambda0 = To180Domain(m_lambda0);
    }
    else {
      m_lambda0 = To360Domain(m_lambda0);
    }

    // Initialize miscellaneous protected data elements
    m_good = false;

    m_minimumX = DBL_MAX;
    m_maximumX = -DBL_MAX;
    m_minimumY = DBL_MAX;
    m_maximumY = -DBL_MAX;

    // Descriptions of the variables a, e
    double axis1 = EquatorialRadius();
    double axis2 = PolarRadius();
    m_a = qMax(axis1, axis2); // major axis. generally, this is the larger of the equatorial axes
    m_b = qMin(axis1, axis2); // minor axis. generally, this is the polar axis
    m_e = Eccentricity();  // e = sqrt(1 - (b/a)^2) 
                           // must be 0 <= e < 1
    if (qFuzzyCompare(0.0, m_e)) {
      m_e = 0.0;
    }

    // to reduce calculations, calculate some scalar constants:
    double t0 = m_b / m_a; // equals sqrt( 1 - m_e * m_e );
    m_t  = t0 * t0; // equals 1 - m_e * m_e ;
    m_t1 = m_e * t0; // equals e/sqrt( 1 - m_e * m_e );
    double k1 = 2 * m_a * exp(m_t1 * atan(m_t1));
    // k = radius of the Equator of transverse graticule on Azimuthal projection
    //     under condition of no distortion in the center of projection
    m_k = k1 * m_t;
  }


  /**
   * This method is used to set the ground latitude/longitude values and 
   * compute the corresponding x/y values of the projection. The method 
   * assumes the latitude and longitude given to be non-Null and of the 
   * correct LatitudeType, LongitudeDirection, and LongitudeDomain. If the 
   * computation of the x/y coordinate values is unsuccessful, this method 
   * returns false.
   *
   * @param lat Latitude value to project, in degrees.  Must be between -90 
   *            and 90.
   * @param lon Longitude value to project, in degrees 
   *  
   * @return @b bool Indicates whether the x/y coordinate calculation was 
   *         successful.
   * 
   * @author 2016-03-18 Jeannie Backer 
   */
  bool UpturnedEllipsoidTransverseAzimuthal::SetGround(const double lat, 
                                                       const double lon) {
    // when lat/lon is Null we can't set ground
    if (lat == Null || lon == Null) {
      m_good = false;
      return m_good;
    }

    // convert given lat to radians, planetocentric
    // phiNorm = The planetocentric latitude, in normal aspect.
    double phiNorm;
    // when lat just barely beyond pole, set equal to pole
    if (qFuzzyCompare(90.0, qAbs(lat)) && qAbs(lat) > 90.0) {
      phiNorm = copysign(HALFPI, lat); // returns sign(lat) * PI/2
    }
    // if well beyond pole, it's bad
    else if (qAbs(lat) > 90.0) {
      m_good = false;
      return m_good;
    }
    else if (IsPlanetocentric()) {
      phiNorm = lat * DEG2RAD;
    }
    else {
      // equations expect ocentric latitudes.
      phiNorm = ToPlanetocentric(lat) * DEG2RAD;
    }

    // convert given lon to positive east, then subtract center lon to get 
    // lambdaNorm = longitude east of the center of projection 
    double lambdaNorm;
    if (IsPositiveEast()) {
      lambdaNorm = lon * DEG2RAD - m_lambda0;
    }
    else {
      lambdaNorm = ToPositiveEast(lon, 360) * DEG2RAD - m_lambda0;
    }

    // Compute the rectangular x,y coordinate using the appropriate set of equations below
    double x,y;

    // NOTE: z = the angular distance from the center of projection.
    double cosz = cos(phiNorm) * cos(lambdaNorm);

    // First, we take care of the edge cases where
    // 1. a rounding error causes cosz to be outside of the range of cosine
    // 2. z == 0 (this could be handled by the next set of equations,
    //            but taking care of it here reduces calculations.)
    if (cosz >= 1) {
      // This is the origin,
      // at lat = equator, lon = centerlon 
      x = 0.0;
      y = 0.0;
    }

    // This condition is used for the set of equations that are
    // written to exclude the singularities where z == 0 or z == PI
    // (i.e. the given longitude is equal to the center longitude
    // or 180 degrees from it).
    // Use these equations for 0.5 < cosz < 1.0, i.e. 0 < z < PI/3
    else if (cosz > 0.5) { //  && cosz < 1 is implied by bounds on cosine
      // use pythagorean identity to get sine
      double sinz = sqrt( 1 - cosz*cosz ); // due to restrictions above sinz != 0
      // phi = the latitude at "upturned" ellipsoid of revolution
      // NOTE: Since cos(z) > 0.5 or cos(z) < -0.5,
      //       there is no risk of zero denominator when dividing by cosz
      double phi = HALFPI - atan2( sinz, m_t * cosz );
      double sinPhi = sin(phi);
      // rhoOverTanZ = rho/tanz
      //       where rho = the radius of latitude circle (transverse graticule)
      // NOTE: We know sin(phi) = -1 only if
      //       phi = PI/2 - arctan(angle) = -PI/2 or 3PI/2
      //       but the range of arctangent is (-PI/2, PI/2),
      //       so there is no risk of zero denominator dividing by (1+sin(phi))
      double rhoOverTanZ = m_k * sinPhi / ( ( 1 + sinPhi )
                                            * m_t
                                            * exp( m_t1 * atan( m_t1 * sinPhi ) ) );
      x = rhoOverTanZ * tan(lambdaNorm);
      y = rhoOverTanZ * (sin(phiNorm) / cosz ); // Note: cos(z) > 0.5, so no dividing by zero
    }

    // This condition is used for the set of equations that are
    // written to exclude the singularity where z == PI/2.
    // Use these equations for -1 <= cosz <= 0.5, i.e. PI/3 <= z < PI.
    else { 
      // For this case, we will restrict z near multiples of PI using a
      // tolerance value in order to avoid singularities at 0 and PI. We
      // define zmin and zmax by:
      // 
      // zmin = minimal value of angular distance from the center of projection
      //      = 0 + angular tolerance
      // zmax = maximal value of angular distance from the center of projection
      //        in the neighborhood of point opposite to the center of projection
      //      = PI - angular tolerance
      double tolerance = 0.0016;//???
      double coszmax = cos(PI - tolerance);
      double lambdaModulus = fmod(lambdaNorm, TWOPI);
      if (cosz < coszmax) {
        cosz = coszmax; // prevents cosz from equalling -1
        // The following prevent lambdaNorm from being too close to +/-PI
        
        // if the given longitude is within tolerance of -PI and less than -PI,
        // set it to -PI - tolerance)
        //if (qFuzzyCompare(lambdaNorm, -PI) && lambdaNorm <= -PI) {
        if (-PI - tolerance < lambdaModulus && lambdaModulus <= -PI) {
          lambdaNorm = -PI - tolerance;
        }
        // if the given longitude is within tolerance of -PI and greater than -PI, 
        // set it to -zmax (i.e. -PI + tolerance)
        // 
        //if (qFuzzyCompare(lambdaNorm, -PI) && lambdaNorm > -PI) {
        else if (-PI < lambdaModulus && lambdaModulus <= -PI + tolerance) {
          lambdaNorm = -PI + tolerance;
        }
        // if the given longitude is within tolerance of PI and less than PI,
        // set it to zmax (i.e. PI - tolerance)
        // 
        //if (qFuzzyCompare(lambdaNorm, PI) && lambdaNorm <= PI) {
        else if (PI - tolerance < lambdaModulus && lambdaModulus <= PI) {
          lambdaNorm = PI - tolerance;
        }
        // if the given longitude is within tolerance of PI and greater than PI,
        // set it to PI + tolerance
        // 
        //if (qFuzzyCompare(lambdaNorm, PI) && lambdaNorm > PI) {
        else if (PI < lambdaModulus && lambdaModulus < PI + tolerance) {
          lambdaNorm = PI + tolerance;
        }
      }
      // use pythagorean identity to get sine
      double sinz = sqrt( 1 - cosz*cosz ); // due to restrictions above, we know sinz != 0

      // phi = latitude at "upturned" ellipsoid of revolution
      // NOTE: On the interval, PI/3 <= z < PI,
      //       we know 0 < sin(z) <= 1,
      //       so there is no risk of zero denom when dividing by sinz
      double phi = atan2( m_t * cosz, sinz );
      double sinPhi = sin(phi);
      // Note: We know sin(phi) = -1 only if
      //       phi = arctan(angle) = -PI/2
      //       but the range of arctangent is the open interval (-PI/2, PI/2),
      //       so there is no risk of zero denominator dividing by (1+sin(phi))
      double rhoOverSinZ = m_k * cos(phi) / ( ( 1 + sinPhi )
                                              * sinz
                                              * exp( m_t1 * atan(m_t1 * sinPhi ) ) );
      x = rhoOverSinZ * cos(phiNorm) * sin(lambdaNorm);
      y = rhoOverSinZ * sin(phiNorm);
    }

    // set member data and return status
    m_good = true;
    m_latitude = lat;
    m_longitude = lon; 
    SetComputedXY(x, y); // sets m_x = x and m_y = y and handles rotation
                         // Note: if x or y are Null, sets m_good back to false
    return m_good;
  }


  /**
   * This method is used to set the projection x/y and compute the 
   * corresponding latitude/longitude position. The method assumes the x/y are 
   * not Null. If the computation of the latitude/longitude position values is 
   * unsuccessful, this method returns false. 
   *
   * @param x X coordinate of the projection in units that are the same as the
   *          radii in the label
   *
   * @param y Y coordinate of the projection in units that are the same as the
   *          radii in the label
   *
   * @return bool Indicates whether the latitude/longitude position 
   *         calculation was successful.
   * 
   * @author 2016-03-18 Jeannie Backer 
   */
  bool UpturnedEllipsoidTransverseAzimuthal::SetCoordinate(const double x, 
                                                           const double y) {
    if (x == Null || y == Null) {
      m_good = false;
      return m_good;
    }
    // Save the coordinate
    SetXY(x, y);

    if (qFuzzyCompare(x + 1.0,  1.0) && qFuzzyCompare(y + 1.0,  1.0)) {
      // origin
      m_latitude = 0.0;
      m_longitude = m_lambda0 * RAD2DEG;
    }
    else {

      // We are given the following forward equations:
      // 
      //     x = (rho/sin(z)) * cos(phiNorm) * sin(lambdaNorm)
      //     y = (rho/sin(z)) * sin(phiNorm)
      //     rho/sin(z) = ( k*cos(phi) / [(1+sin(phi))*sin(z)*e^{t1*arctan(t1*sin(phi)}] )
      //     phi = arctan( (1-e^2)*cos(z) / sin(z) )
      // 
      // So, after simplifying, we can verify x^2 + y^2 = rho^2
      // Thus, we have two solutions for rho:
      // 
      //     rho = +/- sqrt(x^2 + y^2)
      //     rho = ( k*cos(phi) / [(1+sin(phi))*e^{t1*arctan(t1*sin(phi)}] )
      // 
      // Now, we can use Newton's root finding method for the function
      //     f(phi) = g(phi) - h(phi) on [-pi/2, pi/2]
      // where
      //     g(phi) = k*cos(phi) / [(1+sin(phi))*e^{t1*arctan(t1*sin(phi)}]
      // and
      //     h(phi) = sqrt(x^2 + y^2).
      // 
      // After simplifying, the derivative with respect to phi is
      //     f'(phi) = -k * (1 + t1^2) /
      //               [(1 + sin(phi)) * e^{t1 * arctan(t1 * sin(phi)} * (1 + t1^2 * sin^2(phi))]

      double phi0, fphi0, fprimephi0, phi1;
      bool converged = false;
      int iterations = 0;
      double tolerance = 10e-10;
      phi0 = 0.0; // start at the equator???
      while (!converged && iterations < 1000) {
        fphi0 = m_k * cos(phi0) / ((1 + sin(phi0)) * exp(m_t1 * atan( m_t1 * sin(phi0) )))
                - sqrt(x*x + y*y);

        fprimephi0 = -m_k * (1 + m_t1 * m_t1)
                     / ((1 + sin(phi0)) 
                        * exp(m_t1 * atan( m_t1 * sin(phi0) )) 
                        * (1 + m_t1 * m_t1 * sin(phi0) * sin(phi0)));
        phi1 = phi0 - fphi0 / fprimephi0;
        // if phi wrapped at the poles, make sure phi is on [-pi/2, pi/2]
        if (qAbs(phi1) > HALFPI) {
          double phiDegrees = To180Domain(phi1*RAD2DEG);
          if (phiDegrees > 90.0) {
            phiDegrees -= 90.0;
          }
          if (phiDegrees < -180.0) {
            phiDegrees += 90.0;
          }
          phi1 = phiDegrees * DEG2RAD;
        }
        if (qAbs(phi0 - phi1) < tolerance) {
          converged = true;
        }
        else {
          phi0 = phi1;
          iterations++;
        }
      }

      if (!converged) {
        m_good = false;
        return m_good;
      }

      // Now we have phi, the latitude at "upturned" ellipsoid of revolution.
      double phi = phi1;

      // Now use the forward equation for phi to solve for z, the angular distance from the center
      // of projection:
      // 
      //     phi = arctan( (1-e^2) * cos(z) / sin(z))
      // 
      double z = atan2((1 - m_e * m_e), tan(phi)); // see handling of cylind???

      // Get phiNorm,the planetocentric latitude, in normal aspect. The range of arcsine is
      // [-pi/2, pi/2] so we are guaranteed to get an angle between the poles. Use the forward
      // equation:
      // 
      //     y = (rho / sinz ) * sin(phiNorm)
      //double rho = ( m_k * cos(phi) / ((1+sin(phi))*exp(m_e, m_t1*arctan(m_t1*sin(phi))) );
      double rho = sqrt(x*x + y*y);
      double phiNorm = asin( y * sin(z) / rho );

      // Get lambdaNorm, the longitude east of lambda0, using the forward equation
      //     cos(z) = cos(phiNorm) * cos(lambdaNorm)
      double lambdaNorm;

      // make sure we are in the correct quadrant...
      double cosLambdaNorm = cos(z) / cos(phiNorm);    
      if (cosLambdaNorm > 1.0) {
        lambdaNorm = 0.0;
      }
      else if (cosLambdaNorm < -1.0) {
        lambdaNorm = PI; // +/- PI doesn't matter here... same answer either way
      }
      else if (x >= 0 && y >= 0) {
        lambdaNorm = acos(cosLambdaNorm);
      }
      else if (x < 0 && y >= 0) {// fix conditional???
        lambdaNorm = -acos(cosLambdaNorm);
      }
      else if (y < 0 && x >= 0) {// fix conditional???
        lambdaNorm = acos(cosLambdaNorm);
      }
      else { // y < 0 && x < 0
        lambdaNorm = -acos(cosLambdaNorm);
      }

      // calculations give positive east longitude
      m_longitude = (lambdaNorm + m_lambda0) * RAD2DEG;
      m_latitude = phiNorm * RAD2DEG;
    }

    // Cleanup the latitude
    if (IsPlanetocentric()) {
      m_latitude = ToPlanetocentric(m_latitude);
    }

    // Cleanup the longitude
    if (IsPositiveWest()) {
      m_longitude = ToPositiveWest(m_longitude, m_longitudeDomain);
    }

    if (Has180Domain()) {
      m_longitude = To180Domain(m_longitude);
    }
    else {
      // Do this because longitudeDirection could cause (-360,0)
      m_longitude = To360Domain(m_longitude);
    }

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
   * @param &minX Reference to the address where the minimum x 
   *             coordinate value will be written.  The Minimum x projection
   *             coordinate calculated by this method covers the
   *             latitude/longitude range specified in the labels.
   *
   * @param &maxX Reference to the address where the maximum x 
   *             coordinate value will be written.  The Maximum x projection
   *             coordinate calculated by this method covers the
   *             latitude/longitude range specified in the labels.
   *
   * @param &minY Reference to the address where the minimum y 
   *             coordinate value will be written.  The Minimum y projection
   *             coordinate calculated by this method covers the
   *             latitude/longitude range specified in the labels.
   *
   * @param &maxY Reference to the address where the maximum y 
   *             coordinate value will be written.  The Maximum y projection
   *             coordinate calculated by this method covers the
   *             latitude/longitude range specified in the labels.
   * 
   * @return @b bool Indicates whether the method was able to determine the X/Y 
   *              Range of the projection.  If yes, minX, maxX, minY, maxY will
   *              be set with these values.
   *
   * 
   * @author 2016-03-18 Jeannie Backer 
   */
  bool UpturnedEllipsoidTransverseAzimuthal::XYRange(double &minX, double &maxX, 
                                      double &minY, double &maxY) {

    // First check combinations of the lat/lon range boundaries
    XYRangeCheck(m_minimumLatitude, m_minimumLongitude);
    XYRangeCheck(m_maximumLatitude, m_minimumLongitude);
    XYRangeCheck(m_minimumLatitude, m_maximumLongitude);
    XYRangeCheck(m_maximumLatitude, m_maximumLongitude);

    double centerLongitude = m_lambda0 * RAD2DEG;

    bool centerLongitudeInRange =    TProjection::inLongitudeRange(centerLongitude);
    bool centerLongitude90InRange =  TProjection::inLongitudeRange(centerLongitude + 90.0);
    bool centerLongitude180InRange = TProjection::inLongitudeRange(centerLongitude + 180.0);
    bool centerLongitude270InRange = TProjection::inLongitudeRange(centerLongitude + 270.0);

    if (centerLongitudeInRange) {
      XYRangeCheck(m_minimumLatitude, centerLongitude);
      XYRangeCheck(m_maximumLatitude, centerLongitude);
    }
    if (centerLongitude90InRange) {
      XYRangeCheck(m_minimumLatitude, centerLongitude + 90.0);
      XYRangeCheck(m_maximumLatitude, centerLongitude + 90.0);
    }
    if (centerLongitude180InRange) {
      XYRangeCheck(m_minimumLatitude, centerLongitude + 180.0);
      XYRangeCheck(m_maximumLatitude, centerLongitude + 180.0);
    }
    if (centerLongitude270InRange) {
      XYRangeCheck(m_minimumLatitude, centerLongitude + 270.0);
      XYRangeCheck(m_maximumLatitude, centerLongitude + 270.0);
    }

    if (TProjection::inLatitudeRange(0.0)) {
      XYRangeCheck(0.0, m_minimumLongitude);
      XYRangeCheck(0.0, m_maximumLongitude);
      if (centerLongitudeInRange) {
        XYRangeCheck(0.0, centerLongitude);
      }
      if (centerLongitude90InRange) {
        XYRangeCheck(0.0, centerLongitude + 90.0);
      }
      if (centerLongitude180InRange) {
        XYRangeCheck(0.0, centerLongitude + 180.0);
      }
      if (centerLongitude270InRange) {
        XYRangeCheck(0.0, centerLongitude + 270.0);
      }
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
   * This function returns a PvlGroup containing the keywords that this 
   * projection uses. For example, 
   * @code 
   * Group = Mapping
   *   ProjectionName     = UpturnedEllipsoidTransverseAzimuthal
   *   EquatorialRadius   = 1.0
   *   PolarRadius        = 1.0
   *   LatitudeType       = Planetographic
   *   LongitudeDirection = PositiveEast
   *   LongitudeDomain    = 180
   *   PixelResolution    = 0.001
   *   MinimumLatitude    = 20.0
   *   MaximumLatitude    = 80.0
   *   MinimumLongitude   = -180.0
   *   MaximumLongitude   = 180.0
   *   CenterLongitude    = 0
   * End_Group
   * End
   *
   * @return @b PvlGroup The keywords that this projection uses.
   * 
   * @author 2016-03-18 Jeannie Backer 
   */
  PvlGroup UpturnedEllipsoidTransverseAzimuthal::Mapping() {
    PvlGroup mapping = TProjection::Mapping();
    mapping += PvlKeyword("CenterLongitude", Isis::toString(m_lambda0 * RAD2DEG));
    return mapping;
  }


  /**
   * This function returns a PvlGroup containing the latitude keywords that this 
   * projection uses. For example, 
   * @code 
   * Group = Mapping
   *   MinimumLatitude    = 20.0
   *   MaximumLatitude    = 80.0
   * End_Group
   * End 
   *
   * @return @b PvlGroup The latitude keywords that this projection uses.
   * 
   * @author 2016-03-18 Jeannie Backer 
   */
  PvlGroup UpturnedEllipsoidTransverseAzimuthal::MappingLatitudes() {
    return TProjection::MappingLatitudes();
  }


  /**
   * This function returns a PvlGroup containing the keyword CenterLongitude, 
   * the only longitude keywords that this projection uses. For example, 
   * @code 
   * Group = Mapping
   *   MinimumLongitude   = -180.0
   *   MaximumLongitude   = 180.0
   *   CenterLongitude    = 0
   * End_Group
   * End
   *
   * @return @b PvlGroup The longitude keywords that this projection uses.
   * 
   * @author 2016-03-18 Jeannie Backer 
   */
  PvlGroup UpturnedEllipsoidTransverseAzimuthal::MappingLongitudes() {
    PvlGroup mapping = TProjection::MappingLongitudes();
    mapping += PvlKeyword("CenterLongitude", Isis::toString(m_lambda0 * RAD2DEG));
    return mapping;
  }

} // end namespace isis

/** 
 * This is the function that is called in order to instantiate a 
 * UpturnedEllipsoidTransverseAzimuthal object. 
 *  
 * @param lab Cube labels with appropriate Mapping information.
 *  
 * @param allowDefaults Indicates whether CenterLatitude and CenterLongitude 
 *                      are allowed to be calculated by the midpoints of the
 *                      latitude and longitude ranges.
 * 
 * @return @b Isis::Projection* Pointer to a UpturnedEllipsoidTransverseAzimuthal 
 *                              projection object.
 * 
 * @author 2016-03-18 Jeannie Backer 
 */
extern "C" Isis::Projection *UpturnedEllipsoidTransverseAzimuthalPlugin(Isis::Pvl &lab,
                                                                          bool allowDefaults) {
  return new Isis::UpturnedEllipsoidTransverseAzimuthal(lab, allowDefaults);
}

