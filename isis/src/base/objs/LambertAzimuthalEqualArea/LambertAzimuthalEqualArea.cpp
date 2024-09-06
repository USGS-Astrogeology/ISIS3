/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "LambertAzimuthalEqualArea.h"

#include <cmath>
#include <cfloat>

#include <iostream>
#include <iomanip>

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
   * Constructs a Lambert Azimuthal Equal Area object
   *
   * @param label This argument must be a label containing the proper mapping
   *              information, as indicated in the Projection class.
   *              Additionally, the LambertAzimuthalEqualArea projection
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
   * @throw IException::Unknown - "Cannot project using Lambert Azimuthal
   *             equal-area without Center Longitude value.  Keyword does not
   *             exist in labels and defaults are not allowed."
   * @throw IException::Unknown - "Cannot project using Lambert Azimuthal 
   *             equal-area without Center Latitude value.  Keyword does not
   *             exist in labels and defaults are not allowed."
   * @throw IException::Unknown - "[MinimumLongitude,MaximumLongitude]
   *             range is invalid.  No more than 360 degree difference is
   *             allowed."
   * @throw IException::Unknown - "[MinimumLongitude,MaximumLongitude]
   *             range is invalid.  No more than 360 degree distance from
   *             CenterLongitude is allowed."
   * @throw IException::Unknown - "Invalid label group [Mapping]";
   * 
   * @author 2012-07-25 Jeannie Backer 
   * @internal
   *   @history 2012-07-25 Jeannie Backer - Original version.
   */
  LambertAzimuthalEqualArea::LambertAzimuthalEqualArea(
      Pvl &label, 
      bool allowDefaults) :
      TProjection::TProjection(label) {
    try {
      // This algorithm can be found in the USGS Professional Paper 1395
      // Map Projections--A Working Manual by John P. Snyder
      
      // Try to read the mapping group
      PvlGroup &mapGroup = label.findGroup("Mapping", Pvl::Traverse);

      // Compute and write the default center longitude if allowed and
      // necessary
      if (!mapGroup.hasKeyword("CenterLongitude")) {
        if (allowDefaults) {
          double centerLon = (MinimumLongitude() + MaximumLongitude()) / 2.0;
          mapGroup += PvlKeyword("CenterLongitude", std::to_string(centerLon), "Degrees");
        }
        else {
          std::string message = "Cannot project using Lambert Azimuthal equal-area";
          message += " without [CenterLongitude] value.  Keyword does not exist";
          message += " in labels and defaults are not allowed.";
          throw IException(IException::Unknown, message, _FILEINFO_);
        }
      }

      // Compute and write the default center latitude if allowed and
      // necessary
      if (!mapGroup.hasKeyword("CenterLatitude")) {
        if (allowDefaults) {
          double centerLat = (MinimumLatitude() + MaximumLatitude()) / 2.0;
          mapGroup += PvlKeyword("CenterLatitude", std::to_string(centerLat), "Degrees");
        }
        else {
          std::string message = "Cannot project using Lambert Azimuthal equal-area";
          message += " without [CenterLatitude] value.  Keyword does not exist";
          message += " in labels and defaults are not allowed.";
          throw IException(IException::Unknown, message, _FILEINFO_);
        }
      }

      // Get the center longitude  & latitude
      double centerLongitude = mapGroup["CenterLongitude"];
      double centerLatitude = mapGroup["CenterLatitude"];

      if (fabs(MinimumLongitude() - centerLongitude) > 360.0 ||
          fabs(MaximumLongitude() - centerLongitude) > 360.0) {
        IString message = "[MinimumLongitude,MaximumLongitude] range of [";
        message += IString(MinimumLongitude())+","+IString(MaximumLongitude());
        message += "] is invalid.  No more than 360 degrees from the "
                   "CenterLongitude [" + IString(centerLongitude) + "] is allowed.";
        throw IException(IException::Unknown, message, _FILEINFO_);
      }

      if (MaximumLongitude() - MinimumLongitude() > 360.0) {
        IString message = "[MinimumLongitude,MaximumLongitude] range of ["  
                        + IString(MinimumLongitude()) + "," 
                        + IString(MaximumLongitude()) + "] is invalid. "
                        "No more than 360 degree range width is allowed.";
        throw IException(IException::Unknown, message, _FILEINFO_);
      }

      // initialize member variables
      init(centerLatitude, centerLongitude);
    }
    catch(IException &e) {
      IString message = "Invalid label group [Mapping]";
      throw IException(e, IException::Unknown, message, _FILEINFO_);
    }
  }
  
  /**
   * Destroys the LambertAzimuthalEqualArea object
   * 
   * @author 2012-07-25 Jeannie Backer 
   * @internal
   *   @history 2012-07-25 Jeannie Backer - Original version.
   */
  LambertAzimuthalEqualArea::~LambertAzimuthalEqualArea() {
  }

  /**
   * Compares two Projection objects to see if they are equal
   *
   * @param proj Projection object to do comparison on
   *
   * @return bool Returns true if the Projection objects are equal, and false if
   *              they are not
   * 
   * @author 2012-07-25 Jeannie Backer 
   * @internal
   *   @history 2012-07-25 Jeannie Backer - Original version.
   */
  bool LambertAzimuthalEqualArea::operator== (const Projection &proj) {
    if (!Projection::operator==(proj)) return false;
    // don't do the below it is a recursive plunge
    //  if (Projection::operator!=(proj)) return false;
    LambertAzimuthalEqualArea *lama = (LambertAzimuthalEqualArea *) &proj;
    if ((lama->m_phi1 != m_phi1) ||
        (lama->m_lambda0 != m_lambda0) ||
        (lama->m_a != m_a) ||
        (lama->m_e != m_e)) return false;
    // all other data members calculated using m_lambda0, m_phi1, m_a and m_e.
    return true;
  }

  /**
   * Returns the name of the map projection.
   *
   * @return QString Name of projection
   * 
   * @author 2012-07-25 Jeannie Backer 
   * @internal
   *   @history 2012-07-25 Jeannie Backer - Original version.
   */
  QString LambertAzimuthalEqualArea::Name() const {
    return "LambertAzimuthalEqualArea";
  }

  /**
   * Returns the version of the map projection.
   *
   * @return QString Version number
   */
  QString  LambertAzimuthalEqualArea::Version() const {
    return "1.0";
  }

  /**
   * Returns the latitude of true scale.  For Lambert Azimuthal, the center 
   * latitude in degrees is returned. 
   *  
   * **NOTE** In the case of Lambert Azimuthal equal area projections, there is 
   * NO latitude that is entirely true to scale. The only true scale for this 
   * projection is the single point at (center latitude, center longitude). 
   *
   * @return double The center latitude.
   * 
   * @author 2012-07-25 Jeannie Backer 
   * @internal
   *   @history 2012-07-25 Jeannie Backer - Original version.
   */
  double LambertAzimuthalEqualArea::TrueScaleLatitude() const {
    //Snyder pg. 
    // no distortion at center of projection (centerLatitude, centerLongitude)
    return m_phi1 * 180.0 / PI;
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
   * @throw Exception::Unknown - "[CenterLongitude] is outside the range of
   *             [-360, 360]"
   * @throw Exception::Unknown - "[CenterLatitude] is outside the range of
   *             [-90, 90]"
   * @throw Exception::Unknown - "Invalid latitude and/or longitude range. 
   *               Non-polar Lambert Azimuthal equal-area projections can not
   *               project the antipodal point on the body."
   * 
   * @author 2012-07-25 Jeannie Backer 
   * @internal
   *   @history 2012-07-25 Jeannie Backer - Original version.
   */
  void LambertAzimuthalEqualArea::init(double centerLatitude, 
                                       double centerLongitude) {
    // Initialize miscellaneous protected data elements
    m_good = false;

    m_minimumX = DBL_MAX;
    m_maximumX = -DBL_MAX;
    m_minimumY = DBL_MAX;
    m_maximumY = -DBL_MAX;

    // Test to make sure center longitude is valid
    if (fabs(centerLongitude) > 360.0) {
      IString message = "CenterLongitude [" + IString(centerLongitude);
      message += "] is outside the range of [-360, 360]";
      throw IException(IException::Unknown, message, _FILEINFO_);
    } 
 
    // Test to make sure center lat is valid
    if (fabs(centerLatitude) > 90.0) {
      IString message = "CenterLatitude [" + IString(centerLatitude);
      message += "] is outside the range of [-90, 90]";
      throw IException(IException::Unknown, message, _FILEINFO_);
    }

    // We need to convert to planetographic since phi is geographic latitude 
    // (see phi and phi1 - Snyder pg ix)
    if (IsPlanetocentric()) {
      centerLatitude = ToPlanetographic(centerLatitude);
    }

    // adjust for positive east longitude direction 
    // (see lambda and lambda0 - Snyder pg ix)
    if (m_longitudeDirection == PositiveWest) {
      centerLongitude = -1.0*centerLongitude;
    }

    // Descriptions of the variables a, e, lambda0, and phi1 
    // can be found in Snyder text on pgs viii-ix, pg 187
    m_a = EquatorialRadius();
    m_e = Eccentricity();  // e = sqrt(1 - (PolarRadius/EqRadius)^2) 
                           // must be 0 <= e < 1

    // Snyder variables used for spherical projection
    m_lambda0 = centerLongitude*PI / 180.0; // convert to radians
    m_phi1 = centerLatitude*PI / 180.0;     // convert to radians
    m_sinPhi1 = sin(m_phi1);
    m_cosPhi1 = cos(m_phi1);

    // flags for determinining which formulas to use
    m_spherical = false;
    m_northPolarAspect = false;
    m_southPolarAspect = false;
    m_equatorialAspect = false;

    if (qFuzzyCompare(0.0, m_e)) {
      m_e = 0.0;
      m_spherical = true;
    }
    if (qFuzzyCompare(HALFPI, m_phi1)) {
      m_phi1 = HALFPI;
      m_northPolarAspect = true;
    }
    if (qFuzzyCompare(-HALFPI, m_phi1)) {
      m_phi1 = -HALFPI;
      m_southPolarAspect = true;
    }
    if (qFuzzyCompare(0.0, m_phi1)) {
      m_phi1 = 0.0;
      m_equatorialAspect = true;
    }

    // Snyder ellipsoid variables will not be used
    m_qp = Null;
    m_q1 = Null;
    m_m1 = Null;
    m_beta1 = Null;
    m_sinBeta1 = Null;
    m_cosBeta1 = Null;
    m_Rq = Null;
    m_D = Null;

    // other Snyder variables
    m_h = Null;
    m_k = Null;

    // if eccentricity = 0, we are projecting a sphere.
    if (!m_spherical) {
      // Calculate Snyder ellipsoid variables
      initEllipsoid();
    }

    // Check if the antipodal point is in the lat/lon ranges.
    // 
    // We can only allow this if we have a polar projection.
    // Otherwise, we cannot SetGround() for the antipodal point since it is
    // projected as a circle, not a single point with (x,y) value.
    // 
    // The antipodal point is defined by the coordinates
    //     (-centerLat, centerLon-180) or (-centerLat, centerLon+180)
    if (!m_northPolarAspect && !m_southPolarAspect) {
      if (-centerLatitude >= MinimumLatitude() 
          && -centerLatitude <= MaximumLatitude()
          && (  (MinimumLongitude() <= centerLongitude - 180 &&
                 MaximumLongitude() >= centerLongitude - 180)
             || (MinimumLongitude() <= centerLongitude + 180 &&
                 MaximumLongitude() >= centerLongitude + 180)  ) ) {
        IString message = "Invalid latitude and/or longitude range. ";
        message += "Non-polar Lambert Azimuthal equal-area "
                   "projections can not project the antipodal point on "
                   "the body.";
        throw IException(IException::Unknown, message, _FILEINFO_);
      }
    }
  }

  /**
   * Initialize member variables needed for projecting an ellipsoid 
   * 
   * @author 2012-07-25 Jeannie Backer 
   * @internal
   *   @history 2012-07-25 Jeannie Backer - Original version.
   */
  void LambertAzimuthalEqualArea::initEllipsoid() {
    m_spherical = false;

    // Decided not to use qCompute method here to reduce chance of roundoff 
    // error since q for polar simplifies 
    // m_qp = qCompute(1.0); //q for polar, sin(phi) = sin(pi/2) = 1.0
    m_qp = 1 - (1 - m_e * m_e) / (2 * m_e) * log( (1 - m_e) / (1 + m_e) );
           // Snyder eqn (3-12) pg 187, use phi = pi/2 (polar)
           // Note: We know that qp is well defined since 0 < e < 1, thus
           //       1+e != 0 (no 0 denominator) and (1-e)/(1+e) > 0 (log domain)
    if (!m_northPolarAspect && !m_southPolarAspect) {
      // we only need to compute these variables for oblique projections

      m_q1 = qCompute(m_sinPhi1); // Snyder eqn (3-12) pg 187, use phi = phi1
      m_m1 = mCompute(m_sinPhi1, m_cosPhi1); // Snyder eqn (14-15) pg 187, 
             //use phi = phi1, thus m1 = cosPhi1/sqrt(1-eSinPhi1*eSinPhi1)
      m_beta1 = asin(m_q1 / m_qp); // Snyder eqn (3-11) pg 187, use q = q1
            // Note: We can show mathematically that beta1 is well defined
            //       First, since 0 < eccentricity < 1, we have (1-e)/(1+e) < 1
            //       Thus, ln((1-e)/(1+e)) < 0 and so qp > 1 (no 0 denominator)
            //       Next, verify the domain for arcsine, -1 <= q1/qp <= 1 
            //       Since not polar, |phi1| < 90, and since ellipsoid 0 < e < 1
            //       Then |sinPhi1| < 1, so e|sinPhi1| < e
            //       So, 1 < (1+e|sinPhi1|)/(1-e|sinPhi1|) < (1+e)/(1-e).
            //       0 < ln[(1+e|sinPhi1|)/(1-e|sinPhi1|)] < ln[(1+e)/(1-e)]
            //       Note that (1-e^2)/(2e) > 0 and recall -ln(x) = ln(1/x) 
            // So |q1| = |sinPhi1 - (1-e^2)/(2e)ln[(1-eSinPhi1)/(1+eSinPhi1)]|
            //         = |sinPhi1 + (1-e^2)/(2e)ln[(1+eSinPhi1)/(1-eSinPhi1)]|
            //        <= |sinPhi1| + |(1-e^2)/(2e)ln[(1+eSinPhi1)/(1-eSinPhi1)]|
            //         = |sinPhi1| +(1-e^2)/(2e) |ln[(1+eSinPhi1)/(1-eSinPhi1)]|
            //         <   1   + (1-e^2)/(2e) |ln[(1+eSinPhi1)/(1-eSinPhi1)]|
            //        <=   1   + (1-e^2)/(2e) ln[(1+e|SinPhi1|)/(1-e|SinPhi1|)]
            //         <   1   + (1-e^2)/(2e) ln[(1+e)/(1-e)]
            //         = 1 - (1-e^2)/(2e)ln[(1-e)/(1+e)]
            //         = qp
      m_sinBeta1 = sin(m_beta1);
      m_cosBeta1 = cos(m_beta1);
      m_Rq = m_a * sqrt(m_qp / 2); // Snyder eqn (3-13) pg 16, 187
            // Note: We know Rq is well defined since qp > 1 (see m_beta1)
      m_D  = m_a * m_m1 / (m_Rq * m_cosBeta1); // Snyder eqn (24-20) pg 187
            // Note: We know D is well-defined since
            //       cosBeta1 = 0 implies beta1 = +/-pi/2, 
            //       which in turn implies q1 = +/-qp
            //       This should only happen if polar aspect, 
            //       Furthermore, Rq = 0 is impossible since a > 0 and q > 1
    }
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
   * @return bool Indicates whether the x/y coordinate calculation was 
   *         successful.
   * 
   * @author 2012-07-25 Jeannie Backer 
   * @internal
   *   @history 2012-07-25 Jeannie Backer - Original version.
   */
  bool LambertAzimuthalEqualArea::SetGround(const double lat, 
                                            const double lon) {
    // when lat/lon is Null or lat is "beyond" the poles, we can't set ground
    if (fabs(lat) - 90.0 > DBL_EPSILON || lat == Null || lon == Null) {
      if (!qFuzzyCompare(90.0, fabs(lat))) {
        m_good = false;
        return m_good;
      }
    }
 
    m_longitude = lon; 
    m_latitude = lat;

    // assign input values to Snyder's phi and lambda variables 
    // convert to radians, positive east longitude, planetographic
    double phi     = lat * PI / 180.0;
    double lambda  = lon * PI / 180.0;

    // when lat just barely beyond pole, set equal to pole
    if (lat > 90.0 && qFuzzyCompare(90.0, lat)) {
      phi = HALFPI;
    }
    if (lat < -90.0 && qFuzzyCompare(-90.0, lat)) {
      phi = -HALFPI;
    }

    if (m_longitudeDirection == PositiveWest) lambda *= -1.0;
    if (IsPlanetocentric()) phi = ToPlanetographic(phi);

    if (!m_spherical) return setGroundEllipsoid(phi, lambda);

    // calculate the following once to reduce computations 
    double sinPhi   = sin(phi);
    double cosPhi   = cos(phi);
    double sinLambdaDiff = sin(lambda - m_lambda0);
    double cosLambdaDiff = cos(lambda - m_lambda0);

    // Use the variables defined above to compute the x,y coordinate
    double x,y;

    // In this case, Spherical Radius = Equitorial Radius, 
    // so Snyder's R = Snyder's a variable pg ix
    double R = m_a;
    if (m_northPolarAspect ) {
      double sinQuarterPiMinusHalfPhi = sin(PI/4-phi/2); 
      x = 2*R*sinQuarterPiMinusHalfPhi*sinLambdaDiff; //Snyder eqn (24-3) pg 186
      y = -2*R*sinQuarterPiMinusHalfPhi*cosLambdaDiff;//Snyder eqn (24-4) pg 186
      m_h = cos(PI/4-phi/2); // Snyder eqn (24-6) pg 186
      m_k = 1/m_h; // Snyder eqn (24-5) and (24-6) pg 186 
                   // - recall sec(theta) = 1/cos(theta)
    }
    else if (m_southPolarAspect ) {
      double cosQuarterPiMinusHalfPhi = cos(PI/4-phi/2);
      x = 2*R*cosQuarterPiMinusHalfPhi*sinLambdaDiff; //Snyder eqn (24-8) pg 186
      y = 2*R*cosQuarterPiMinusHalfPhi*cosLambdaDiff; //Snyder eqn (24-9) pg 186
      m_h = sin(PI/4-phi/2); // Snyder eqn (24-11) pg 186
      m_k = 1/m_h; // Snyder eqn (24-10) and (24-11) pg 186
    }
    else { // spherical oblique aspect (this includes equatorial)

      // Check if (phi, lambda) is the antipodal point (point diametrically
      // opposite the center point of projection)
      if (qFuzzyCompare(-m_phi1, phi) // phi = -phi1
          && fabs(fmod(lambda-m_lambda0, PI)) < DBL_EPSILON //lambda-lambda0=k*PI
          && fabs(fmod(lambda-m_lambda0, 2*PI)) > DBL_EPSILON) {  // k is odd
        // We don't allow this for oblique aspect projections.  This point,
        // at the opposite side of the planet, is projected as a circle of
        // radius 2R - we can't return a unique (x,y) value.
        m_good = false;
        return m_good;
      }

      // First, be sure that the denominator will not be zero
      double trigTerms;
      if (m_equatorialAspect) {
        // If m_phi1 == 0, the general case below cancels alegbraically
        // to the following equations, however, the simplified equations
        // are used here to reduce chance of roundoff errors
        // -- Snyder eq (24-14) pg 186
        trigTerms = cosPhi*cosLambdaDiff;
      }
      else {
        // general case for oblique projections -- Snyder eq (24-2) pg 185
        double sinProduct = m_sinPhi1 * sinPhi;
        double cosProduct = m_cosPhi1 * cosPhi * cosLambdaDiff;
        trigTerms = sinProduct + cosProduct;
      }
      // make sure when we add 1 to trigTerms that we are not near zero
      if (qFuzzyCompare(-1.0, trigTerms)) {
          m_good = false;
          return m_good;
      }
      double denominator = 1 + trigTerms;
      double kprime = sqrt(2/denominator); // Snyder eqn (24-2) or (24-14) pg 185-186
      x = R*kprime*cosPhi*sinLambdaDiff; // Snyder eqn (22-4) pg 185
      if (m_equatorialAspect) {
        // If m_phi1 == 0, the general case below cancels alegbraically
        // to the following, however, use the simplified equations for y
        // to reduce the likelihood of roundoff errors
        // -- Snyder eq (24-13) pg 186
        y = R*kprime*sinPhi;
      }
      else { // spherical general oblique aspect -- Snyder eqn (22-5) pg 185
        y = R*kprime*(m_cosPhi1*sinPhi - m_sinPhi1*cosPhi*cosLambdaDiff); 
      }
      // these are the relative scale factors for oblique aspect
      m_k = kprime;
      m_h = 1/m_k;
    }

    SetComputedXY(x, y); // sets m_x = x and m_y = y and handles rotation
    m_good = true;
    return m_good;
  }

  /**
   * This method is used to set the ground latitude/longitude and compute the 
   * corresponding x/y projection values for an ellipsoidal target.
   *
   * @param phi Latitude value to project, in radians. 
   * @param lambda Longitude value to project, in radians.
   * 
   * @return bool Indicates whether the x/y coordinate calculation was 
   *         successful.
   * 
   * @author 2012-07-25 Jeannie Backer 
   * @internal
   *   @history 2012-07-25 Jeannie Backer - Original version.
   */
  bool LambertAzimuthalEqualArea::setGroundEllipsoid(double phi, 
                                                     double lambda) {
    // calculate the following once to reduce computations 
    double sinPhi   = sin(phi);
    double cosPhi   = cos(phi);
    double sinLambdaDiff = sin(lambda - m_lambda0);
    double cosLambdaDiff = cos(lambda - m_lambda0);

    // Use the variables passed in to compute the x,y coordinate
    double x,y;

    // projecting an ellipsoid
    double q  = qCompute(sinPhi); //q for given lat
    if (qFuzzyCompare(PI/2, phi)) {
      q = m_qp; // reduce roundoff error
    }
    if (qFuzzyCompare(-PI/2, phi)) {
      q = -m_qp; // reduce roundoff error
    }
    // q = (1-e^2)*(sinphi/(1-e^2sinphi^2))-1/(2e)*log((1-esinphi)/(1+esinphi)))

    double m  = mCompute(sinPhi, cosPhi); // m = cosPhi/sqrt(1-eSinPhi*eSinPhi)
    if (m_northPolarAspect) {
      double rho = m_a* sqrt(m_qp - q); // Snyder eqn (24-23) pg 188
          // Note: Rho is well defined (qp >= q) as shown in initEllipsoid()
      x = rho * sinLambdaDiff; // Snyder eqn (21-30) pg 188
      y = -1.0 * rho * cosLambdaDiff; // Snyder eqn (21-31) pg 188
      if (qFuzzyCompare(m_phi1, phi)) {
        m_k = 1; // Since True scale at clat/clon - see Snyder page 190, table 29
      }
      else{
        m_k = rho / (m_a * m); // Snyder eqn (21-32) pg 188
      }
      m_h = 1 / m_k; // Snyder paragraph after eqn (24-23) pg 188

    }
    else if (m_southPolarAspect) {
      double rho = m_a* sqrt(m_qp + q); // Snyder eqn (24-25) pg 188
          // Note: Rho is well defined (qp >= -q) as shown in initEllipsoid()
      x = rho * sinLambdaDiff; // Snyder eqn (21-30) pg 188
      y = rho * cosLambdaDiff; // Snyder eqn (24-24) pg 188
      if (qFuzzyCompare(m_phi1, phi)) {
        m_k = 1; // Since True scale at clat/clon - see Snyder page 190, table 29
      }
      else{
        m_k = rho / (m_a * m); // Snyder eqn (21-32) pg 188
      }
      m_h = 1 / m_k;
    }
    else { // ellipsoidal oblique aspect
      // following comment is impossible -- see initEllipsoid(),  |q1| > |qp|
      // if (fabs(q) > fabs(m_qp)) {
      //  m_good = false;
      //  return m_good;
      // }
      double beta  = asin( q/m_qp); // Snyder eqn (3-11) pg 187
          // Note: beta is well defined- see initEllipsoid() m_beta1

      // calculate the following once to reduce computations when defining 
      // Snyder's variable B
      double sinBeta  = sin(beta);
      double cosBeta  = cos(beta);

      if (m_equatorialAspect) {
        // If phi1 == 0, the general case below cancels alegbraically to 
        // the following equations, however, the simplified equations are 
        // used here to reduce chance of roundoff errors
        double trigTerm = cosBeta*cosLambdaDiff;
        if (qFuzzyCompare(-1, trigTerm)) { // avoid divide by zero & possible roundoff errors
          // This happens when trying to project the antipodal point.
          // lambda = lambda0 +/- 180 and beta = 0 (that is, phi = 0)
          // Thus denominator = 1 + (1)(-1) = 0
          // We don't allow this for equatorial aspect projections.  This point,
          // at the opposite side of the planet, is projected as a circle of
          // radius 2R - we can't return a unique (x,y) value.
          m_good = false;
          return m_good;
        }
        double denominator = 1 + trigTerm;
        x = m_a * cosBeta * sinLambdaDiff * sqrt(2/denominator); 
            // Snyder eqn (24-21) pg 187
        y = (m_Rq*m_Rq/m_a) * sinBeta * sqrt(2/denominator); 
            // Snyder eqn (24-22) pg 187
            // Note: Clearly 1 + cosBeta*cosLambdaDiff >= 0
            // 1 + cosBeta*cosLambdaDiff = 0 implies 2 cases
            // 1) cosBeta = 1 and cosLambdaDiff = -1
            //    So, beta = 0 and lambdaDiff = 180
            //    Then, q = 0, which implies phi = 0 = phi1
            //    this only happens when projecting opposite side of equator
            // 2) cosBeta = -1 and cosLambdaDiff = 1
            //    So, beta = 180 impossible since beta is defined by asin 
            //    (range of asin is -90 to 90)
            //    Then, q = 0, which implies phi = 0 = phi1
            //    this only happens when projecting opposite side of equator
            // Therefore x and y are well defined
      }
      else { // ellipsoidal General oblique aspect
        double sinProduct = m_sinBeta1 * sinBeta;
        double cosProduct = m_cosBeta1 * cosBeta * cosLambdaDiff;
        double trigTerms = sinProduct + cosProduct;
        if (qFuzzyCompare(-1, trigTerms)) { // avoid divide by zero & possible roundoff errors
          // This happens when trying to project the antipodal point.
          // lambda = lambda0 + 180 and beta = -beta1 (that is, phi = -phi1)
          // Then, by odd symmetry of sine function, sinBeta = -sinBeta1,
          // by even symmetry of cosine, cosBeta = cosBeta1
          // and cosLambdaDiff = cos(180) = -1
          // Thus denominator = 1 - sin^2(beta) - cos^2(beta) = 1-1 = 0
          // We don't allow this for oblique aspect projections.  This point,
          // at the opposite side of the planet, is projected as a circle of
          // radius 2R - we can't return a unique (x,y) value.
          m_good = false;
          return m_good;
        }
        double denominator = 1 + trigTerms;
        double kprime = sqrt(2/denominator); // Snyder eqn (24-2) page 185
        double B = m_Rq * kprime; // Snyder eqns (24-19) pg 187

        x = B * m_D * cosBeta*sinLambdaDiff; // Snyder eqn (24-17) pg 187
        y = (B/m_D) * (m_cosBeta1*sinBeta - m_sinBeta1*cosBeta*cosLambdaDiff); 
            // Snyder eqn (24-18) pg 187
            // Note: y is well defined since D = 0 implies a = 0 or m1 = 0
            //       we know a > 0,
            //       so m1 = 0 would imply cosPhi1 = 0, thus phi1 = +/- 90
            //       but this calculation is not used for polar aspect,
            //       so y doesn't have zero denominator
      }
      // There are no ellipsoidal values for the scale factors h and k unless 
      // using the polar aspects, Snyder pg 26.
    }
    SetComputedXY(x, y); // sets m_x = x and m_y = y and handles rotation

    m_good = true;
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
   * @author 2012-07-25 Jeannie Backer 
   * @internal
   *   @history 2012-07-25 Jeannie Backer - Original version.
   */
  bool LambertAzimuthalEqualArea::SetCoordinate(const double x, 
                                                const double y) {
    if (x == Null || y == Null) {
      m_good = false;
      return m_good;
    }
    // Save the coordinate
    SetXY(x, y);

    // 2012-06-11
    // Uncomment following line if ellipsoid projection is implemented
    if (!m_spherical) return setCoordinateEllipsoid(x,y);

    double phi;    // latitude value to be calculated
    double lambda; // longitude value to be calculated

    // Spherical Radius = Equatorial Radius, so
    // Snyder's R = Snyder's a variable pg ix
    double R = m_a;

    double rho = sqrt(x*x+y*y); // Snyder eqn (20-18) pg 187
    if (rho < DBL_EPSILON) { // this implies (x,y) = (0,0), 
                             // so project to center lat/lon values
      phi = m_phi1;
      lambda = m_lambda0;
    }
    else {
      if (fabs(rho/(2*R)) > 1 + DBL_EPSILON) {// given (x,y) point is off planet 
                                             // projection if distance from
                                             // origin (rho) is greater than
                                             // 2*radius of sphere 
          m_good = false;
          return m_good;
      }
      else if (fabs(rho/(2*R)) > 1) { // if ratio is slightly larger than 1 
                                     // due to rounding error, then fix so we 
                                     // can take the arcsin
        rho = 2*R;
      }
      double c = 2*asin(rho/(2*R)); // c is angular distance, 
                                    //Snyder eqn (24-16) pg 187
      double sinC = sin(c);
      double cosC = cos(c);

      // verify the following is in the domain of the arcsin function
      if (fabs(cosC*m_sinPhi1+y*sinC*m_cosPhi1/rho) > 1) {
        m_good = false;
        return m_good;
      }
      phi = asin(cosC*m_sinPhi1+y*sinC*m_cosPhi1/rho);//Snyder eq (20-14) pg 186
      if (m_northPolarAspect ) { // spherical north polar aspect
        lambda = m_lambda0 + atan2(x,-y); // Snyder eqn (20-16) pg 187, 
                                          // need to use atan2 to correct for
                                          // quadrant - see pg 150
      }
      else if (m_southPolarAspect ) { // spherical north polar aspect
        lambda = m_lambda0 + atan2(x,y); // Snyder eqn (20-17) pg 187, 150
      }
      else { // spherical oblique aspect
        double denominator = rho*m_cosPhi1*cosC - y*m_sinPhi1*sinC;
        // atan2 well defined for denominator = 0 
        // if numerator is also 0, returns 0
        lambda = m_lambda0 + atan2(x * sinC, denominator);
                 // Snyder eqn (20-15) pg 186
      }
    }

    m_latitude = phi * 180.0 / PI;
    m_longitude = lambda * 180.0 / PI;

    // Cleanup the longitude
    if (m_longitudeDirection == PositiveWest) {
      m_longitude *= -1.0;
    }

    if (m_longitudeDomain == 180) {
      m_longitude = To180Domain(m_longitude);
    }
    else {
      // Do this because longitudeDirection could cause (-360,0)
      m_longitude = To360Domain(m_longitude);
    }

    // Cleanup the latitude
    if (IsPlanetocentric()) {
      m_latitude = ToPlanetocentric(m_latitude);
    }

    m_good = true;
    return m_good;
  }

  /**
   * This method is used to set the coordinate x/y values and compute the 
   * corresponding latitude/longitude position values for an ellipsoidal target.
   * 
   * @param x The x coordinate of the projection in units that are the same as the
   *          radii in the label
   *
   * @param y The y coordinate of the projection in units that are the same as 
   *          the radii in the label
   * 
   * @return bool Indicates whether the latitude/longitude position 
   *         calculation was successful.
   * 
   * @author 2012-07-25 Jeannie Backer 
   * @internal
   *   @history 2012-07-25 Jeannie Backer - Original version.
   */
  bool LambertAzimuthalEqualArea::setCoordinateEllipsoid(const double x, 
                                                         const double y) { 
    double phi;    // latitude value to be calculated
    double lambda; // longitude value to be calculated

    // ellipsoidal projection
    double q;
    // for polar aspects
    double rho = sqrt(x*x+y*y); // Snyder eqn (20-18) pg 190 
    if (m_northPolarAspect ) { // ellipsoidal north polar aspect
      q = m_qp - (rho*rho/(m_a*m_a)); // Snyder eqn (24-31) pg 190
      lambda = m_lambda0 + atan2(x,-y); // Snyder eqn (20-16) pgs 190, 150
    }
    else if (m_southPolarAspect ) { // ellipsoidal south polar aspect
      q = -1.0*(m_qp - (rho*rho/(m_a*m_a))); // Snyder eqn (24-31) pg 190
      lambda = m_lambda0 + atan2(x,y); // Snyder eqn (20-17) pg 190, 150
    }
    else {// ellipsoidal oblique aspect 
      double xD = x/m_D; // m_D = 0 implies polar aspect 
                         // (see setGroundEllipsoid() general oblique case
      double Dy = m_D*y;
      double rho = sqrt((xD*xD)+(Dy*Dy)); // Snyder eqn (24-28) pg 189
      if (fabs(rho) > fabs(2*m_Rq)) {
        m_good = false;
        return m_good;
      }
      double Ce = 2*asin(rho/(2*m_Rq)); // Snyder eqn (24-29) pg 189 
                                        // TYPO IN TEXT, parentheses required 
                                        // around denominator (see usage pg 335)
           // Note: Ce well defined since Rq = 0 is impossible (a > 0 and q > 1)
      double sinCe = sin(Ce);
      double cosCe = cos(Ce);

      if (rho < DBL_EPSILON) { // pg 189, first line appears to be typo in text, 
                               // below is our interpretation.
        // if rho = 0, then x=0 and (D=0 or y=0), so it
        // makes sense that lambda = lambda0 in this case.
        q = m_qp * m_sinBeta1;
        lambda = m_lambda0;
      }
      else {
        q = m_qp * (cosCe * m_sinBeta1 + m_D * y * sinCe * m_cosBeta1 / rho);
            // Snyder eqn (24-27) pg 188
        double numerator = x * sinCe;
        double denominator = m_D * rho * m_cosBeta1 * cosCe 
                             - m_D * m_D * y * m_sinBeta1 * sinCe;
        // atan2 well defined for denominator = 0 
        // if numerator is also 0, returns 0
        lambda = m_lambda0 + atan2(numerator, denominator);
                 // Snyder eqn (24-26) pg 188
      }
    }

    if (qFuzzyCompare(fabs(q), fabs(m_qp))) { 
      phi = copysign(HALFPI,q);// Snyder eqn (14-20) pg 189, 
                                        //(see definition of qp on pg 187)
    }
    else {
      if (fabs(q) > 2) {
        // verify that q/2 is in the domain of arcsin
        m_good = false;
        return m_good;
      }
      phi = asin(q/2); // Snyder pg 189 above (14-20) describes iteration 
                       // process and initial value for phi
      double phiOld = phi;
      bool converge = false;
      double tolerance = 1e-10; // tolerance same as Projection::phi2Compute()
                               // should we attempt to relate tolerance to pixel resolution ???
      int maximumIterations = 100;
      int i = 0;
      while (!converge) {
        i++;
        // calculate values to reduce calculations:
        double sinPhi = sin(phi);
        double eSinPhi = m_e*sinPhi;
        double cosPhi = cos(phi);
        double squareESinPhi = eSinPhi*eSinPhi;
        double oneMinusSquareESinPhi = 1 - squareESinPhi;
        // find next iteration of phi
        phi += oneMinusSquareESinPhi*oneMinusSquareESinPhi / (2 * cosPhi)
               * (q / (1 - m_e * m_e) 
                  - sinPhi / (oneMinusSquareESinPhi) 
                  + log( (1 - eSinPhi) / (1 + eSinPhi) ) / (2 * m_e));
               //eqn (3-16) pg 188
        if (fabs(phiOld - phi) < tolerance) {
          converge = true;
        }
        if (i > maximumIterations) {
          m_good = false;
          return m_good;
        }
        phiOld = phi;
      }
    }
    m_latitude = phi * 180.0 / PI;
    m_longitude = lambda * 180.0 / PI;

    // Cleanup the longitude
    if (m_longitudeDirection == PositiveWest) {
      m_longitude *= -1.0;
    }

    if (m_longitudeDomain == 180) {
      m_longitude = To180Domain(m_longitude);
    }
    else {
      // Do this because longitudeDirection could cause (-360,0)
      m_longitude = To360Domain(m_longitude);
    }

    // Cleanup the latitude
    if (IsPlanetocentric()) {
      m_latitude = ToPlanetocentric(m_latitude);
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
   * @return bool Indicates whether the method was able to determine the X/Y 
   *              Range of the projection.  If yes, minX, maxX, minY, maxY will
   *              be set with these values.
   *
   * 
   * @author 2012-07-25 Jeannie Backer 
   * @internal
   *   @history 2012-07-25 Jeannie Backer - Original version.
   */
  bool LambertAzimuthalEqualArea::XYRange(double &minX, double &maxX, 
                                          double &minY, double &maxY) {
    // Global Polar projection
    if ((m_northPolarAspect && qFuzzyCompare(-90.0, MinimumLatitude()))
        || (m_southPolarAspect && qFuzzyCompare(90.0, MaximumLatitude()))) {
      // for polar aspect projections, the antipodal point is the opposite pole.
      // if it is included in lat range, the bounding circle will exist, no
      // matter the longitude range.
      double eRad = EquatorialRadius();
      double pRad = PolarRadius();
      double maxCoordValue = 0;

      if (m_spherical) {
        maxCoordValue = 2*EquatorialRadius();
      }
      else {
        maxCoordValue = sqrt(2*eRad*eRad 
                             + pRad*pRad*log((1+m_e)/(1-m_e))/m_e);
      }
      m_minimumX = -maxCoordValue;
      m_maximumX =  maxCoordValue;
      m_minimumY = -maxCoordValue;
      m_maximumY =  maxCoordValue;
    } 
    // Polar projection, not including antipodal point
    else if (m_northPolarAspect || m_southPolarAspect) {
      return xyRangeLambertAzimuthalPolar(minX, maxX, minY, maxY);
    }
    // Oblique projection
    else { // oblique cases (includes equatorial aspect)
      if (xyRangeOblique(minX, maxX, minY, maxY)) {
        // Make sure that the calculations didn't go beyond the accepatable x,y
        // values.  As far as we know, the x, y values should not exceed
        // 2*LocalRadius(-phi)
        double maxCoordValue = 2*LocalRadius(-m_phi1*180/PI);
        if (m_minimumX < -maxCoordValue) m_minimumX = -maxCoordValue;
        if (m_maximumX >  maxCoordValue) m_maximumX =  maxCoordValue;
        if (m_minimumX < -maxCoordValue) m_minimumX = -maxCoordValue;
        if (m_maximumY >  maxCoordValue) m_maximumY =  maxCoordValue;
      }
      else {
        return false;
      }
    }

    // If we haven't already returned, we have a Global Polar projection or an
    // Oblique projection that did not fail xyRangeOblique
    
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
   * This method is used to determine the x/y range for the area of interest. It
   * is called by the XYRange() method if the projection is Lambert Azimuthal 
   * Polar and the opposite pole is not projected. The method may fail as 
   * indicated by its return value. 
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
   * @return bool Indicates whether the method was able to determine the X/Y 
   *              Range of the projection.  If yes, minX, maxX, minY, maxY will
   *              be set with these values.
   *
   * @see XYRange()
   * 
   * @author 2012-07-25 Jeannie Backer 
   * @internal
   *   @history 2012-07-25 Jeannie Backer - Original version.
   */
  bool LambertAzimuthalEqualArea::xyRangeLambertAzimuthalPolar(double &minX, 
                                                               double &maxX, 
                                                               double &minY, 
                                                               double &maxY) {
    // Test the four combinations of min/max lat/lon
    XYRangeCheck(MinimumLatitude(), MinimumLongitude());
    XYRangeCheck(MinimumLatitude(), MaximumLongitude());
    XYRangeCheck(MaximumLatitude(), MinimumLongitude());
    XYRangeCheck(MaximumLatitude(), MaximumLongitude());

    double centerLonDeg = m_lambda0 * 180.0 / PI;
    if (m_longitudeDirection == PositiveWest) centerLonDeg = centerLonDeg * -1.0;

    // Since this projection is Polar aspect, 4 longitudes will lie
    // directly on the horizontal and vertical axes of the
    // projection.
    // test combinations with each of these longitudes:
    // 
    // down  (negative vertical axis) - center long for north polar
    // up    (positive vertical axis) - center long for south polar
    // left  (negative horizontal axis)
    // right (positive horizontal axis)
    // 
    for (double lon = centerLonDeg; lon < centerLonDeg + 360; lon += 90) {
      checkLongitude(lon);
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
   * Performs XYRangeCheck on polar projections for the given longitude. The 
   * direction may be the negative y-axis (center longitude), positive y-axis 
   * (center longitude + 180 degrees), negative x-axis (center longitude 
   * - 90 degrees) or positive x-axis (center longitude + 90 degrees); 
   *  
   * @param longitude There are 4 valid longitude values: center longitude, 
   *                  center longitude + 90, center longitude + 180 or center
   *                  longitude
   *                  - 90.
   * @throw IException::Programmer "checkLongitude() should only be called 
   *            for a polar aspect projection."
   * 
   * @author 2012-07-25 Jeannie Backer 
   * @internal
   *   @history 2012-07-25 Jeannie Backer - Original version.
   */
  void LambertAzimuthalEqualArea::checkLongitude(double longitude) {

    double centerLatDeg = m_phi1 * 180.0 / PI;

    double innerLatitude, outerLatitude;
    if (m_northPolarAspect) {
      innerLatitude = MaximumLatitude();
      outerLatitude = MinimumLatitude();
    }
    else if (m_southPolarAspect) {
      innerLatitude = MinimumLatitude();
      outerLatitude = MaximumLatitude();
    }
    else {
      IString message = "checkLongitude() should only be called for a "
                    "polar aspect projection. CenterLatitude is [";
      message = message + IString(centerLatDeg) + "] degrees.";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    // Check whether longitude is in the min/max longitude range
    bool thisLongitudeInMinMaxRange = false;
    if (MaximumLongitude() - MinimumLongitude() == 360) {
      thisLongitudeInMinMaxRange = true;
    }
    double adjustedLon =    To360Domain(longitude);
    double adjustedMinLon = To360Domain(MinimumLongitude());
    double adjustedMaxLon = To360Domain(MaximumLongitude()); 

    if (adjustedMinLon > adjustedMaxLon) {
      if (adjustedLon > adjustedMinLon) {
        adjustedLon -= 360;
      }
      adjustedMinLon -= 360;
    }
    // if lon value for this axis is between min lon and max lon
    if (adjustedMinLon <= adjustedLon 
        && adjustedLon <= adjustedMaxLon) { 
      thisLongitudeInMinMaxRange = true;
    }
    else {
      thisLongitudeInMinMaxRange = false;
    }

    if (thisLongitudeInMinMaxRange) {
      // any shape that includes this longitude
      XYRangeCheck(outerLatitude, longitude);
    }
    else {
      // determine which boundary value (minlon or maxlon)
      // is closer to given longitude 
      double distToMinLon  = fabs(adjustedMinLon - adjustedLon);
      double distToMaxLon = fabs(adjustedMaxLon - adjustedLon);

      if (distToMinLon >= 180 ) {
        distToMinLon = fabs(360 - distToMinLon);
      }
      if (distToMaxLon >= 180 ) {
        distToMaxLon = fabs(360 - distToMaxLon);
      }

      double nearestBoundary = 0;
      if (distToMinLon < distToMaxLon) {
        nearestBoundary = MinimumLongitude();
      }
      else {
        nearestBoundary = MaximumLongitude();
      }

      // shapes that come within 90 degrees of the given longitude
      if (distToMinLon <= 90 + DBL_EPSILON 
          || distToMaxLon <= 90 + DBL_EPSILON) {
        XYRangeCheck(outerLatitude, nearestBoundary);
      }
      // shapes more than 90 degrees from the longitude that include the origin
      else if (qFuzzyCompare(MaximumLatitude(), centerLatDeg)) {
        XYRangeCheck(centerLatDeg,longitude);
      }
      // shapes more than 90 degrees from the longitude without the origin
      else {
        XYRangeCheck(innerLatitude, nearestBoundary);
      }
    }
    return;
  }

  /**
   * This function returns a PvlGroup containing the keywords that this 
   * projection uses. For example, 
   * @code 
   * Group = Mapping
   *   ProjectionName     = LambertAzimuthalEqualArea
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
   *   CenterLatitude     = 0
   *   CenterLongitude    = 0
   * End_Group
   * End
   *
   * @return PvlGroup The keywords that this projection uses.
   * 
   * @author 2012-07-25 Jeannie Backer 
   * @internal
   *   @history 2012-07-25 Jeannie Backer - Original version.
   */
  PvlGroup LambertAzimuthalEqualArea::Mapping() {
    PvlGroup mapping = TProjection::Mapping();

    mapping += m_mappingGrp["CenterLatitude"];
    mapping += m_mappingGrp["CenterLongitude"];

    return mapping;
  }

  /**
   * This function returns a PvlGroup containing the keyword CenterLatitude, the
   * only latitude keywords that this projection uses. For example, 
   * @code 
   * Group = Mapping
   *   MinimumLatitude    = 20.0
   *   MaximumLatitude    = 80.0
   *   CenterLatitude     = 0
   * End_Group
   * End 
   *
   * @return PvlGroup The latitude keywords that this projection uses.
   * 
   * @author 2012-07-25 Jeannie Backer 
   * @internal
   *   @history 2012-07-25 Jeannie Backer - Original version.
   */
  PvlGroup LambertAzimuthalEqualArea::MappingLatitudes() {
    PvlGroup mapping = TProjection::MappingLatitudes();

    mapping += m_mappingGrp["CenterLatitude"];

    return mapping;
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
   * @return PvlGroup The longitude keywords that this projection uses.
   * 
   * @author 2012-07-25 Jeannie Backer 
   * @internal
   *   @history 2012-07-25 Jeannie Backer - Original version.
   */
  PvlGroup LambertAzimuthalEqualArea::MappingLongitudes() {
    PvlGroup mapping = TProjection::MappingLongitudes();

    mapping += m_mappingGrp["CenterLongitude"];

    return mapping;
  }

  // There are no ellipsoidal values for the scale factors h and k unless 
  // using the polar aspects, Snyder pg 26.

  // These methods are not currently used in other Projection classes. However,
  // if similar methods are implemented for other projections, we should create
  // a virtual prototype in the parent Projection. Currently since they do not
  // exist in Projection, we can not call them if ProjectionFactory was used to
  // create the object.
  /**
   * Returns the relative scale factor along a meridian of longitude. This is 
   * only calculated when the setGround method has been successfully called to 
   * find the x,y coordinate.  For ellipsoidal targets, the relative scale 
   * factor can only be computed for polar aspect projections.  If the center 
   * latitude is a pole, the relative scale factor can not be calculated for 
   * the opposite pole. 
   *  
   * @return double The relative scale factor along the ground longitude.
   * 
   * @author 2012-07-25 Jeannie Backer 
   * @internal
   *   @history 2012-07-25 Jeannie Backer - Original version.
   */
  double LambertAzimuthalEqualArea::relativeScaleFactorLongitude() const {
    validateRelativeScaleFactor();
    return m_h;
  };

  /**
   * Returns the relative scale factor along a parallel of latitude. This is 
   * only calculated when the setGround method has been successfully called to 
   * find the x,y coordinate.  For ellipsoidal targets, the relative scale 
   * factor can only be computed for polar aspect projections.  If the center 
   * latitude is a pole, the relative scale factor can not be calculated for 
   * the opposite pole. 
   *  
   * @return double The relative scale factor along the ground latitude.
   * 
   * @author 2012-07-25 Jeannie Backer 
   * @internal
   *   @history 2012-07-25 Jeannie Backer - Original version.
   */
  double LambertAzimuthalEqualArea::relativeScaleFactorLatitude() const {
    validateRelativeScaleFactor();
    return m_k;
  }

  /**
   * Verifies whether the relative scale factor can be 
   * computed. 
   *
   * @throw IException::Unknown - "Projection failed. Relative scale factor 
   *            can not be computed."
   * @throw IException::Unknown "For ellipsidal bodies, relative scale factor 
   *            can only be computed for polar aspect projections."
   * @throw IException::Unknown - "Relative scale factor can not be computed 
   *            for north polar aspect projection when ground is set to
   *            latitude -90."
   * @throw IException::Unknown - "Relative scale factor can not be computed 
   *            for south polar aspect projection when ground is set to
   *            latitude 90."
   * @throw IException::Unknown -"Relative scale factor can not be computed." 
   *  
   * @return double The relative scale factor along a latitude.
   * 
   * @author 2012-07-25 Jeannie Backer 
   * @internal
   *   @history 2012-07-25 Jeannie Backer - Original version.
   */
  void LambertAzimuthalEqualArea::validateRelativeScaleFactor() const {
    if (!m_good) {
      IString message = "Projection failed or ground and coordinates have not "
                        "been set.  Relative scale factor can not be computed.";
      throw IException(IException::Unknown, message, _FILEINFO_);
    }
    if (!m_spherical && !(m_northPolarAspect || m_southPolarAspect)) {
      IString message = "For ellipsidal bodies, relative scale factor can only "
                        "be computed for polar aspect projections.";
      throw IException(IException::Unknown, message, _FILEINFO_);
    }
    if (m_northPolarAspect && qFuzzyCompare(-90.0, m_latitude)) {
        IString message = "Relative scale factor can not be computed for north "
                          "polar aspect projection when ground is set to "
                          "latitude -90.";
      throw IException(IException::Unknown, message, _FILEINFO_);
    }
    if (m_southPolarAspect && qFuzzyCompare(90.0, m_latitude)) {
        IString message = "Relative scale factor can not be computed for south "
                          "polar aspect projection when ground is set to "
                          "latitude 90.";
      throw IException(IException::Unknown, message, _FILEINFO_);
    }
    if (m_k == Null || m_h == Null) {
      IString message = "Relative scale factor can not be computed.";
      throw IException(IException::Unknown, message, _FILEINFO_);
    }
  }

} // end namespace isis

/** 
 * This is the function that is called in order to instantiate a 
 * LambertAzimuthalEqualArea object. 
 *  
 * @param lab Cube labels with appropriate Mapping information.
 *  
 * @param allowDefaults Indicates whether CenterLatitude and CenterLongitude 
 *                      are allowed to be calculated by the midpoints of the
 *                      latitude and longitude ranges.
 * 
 * @return @b Isis::Projection* Pointer to a LambertAzimuthalEqualArea 
 *                              projection object.
 * 
 * @author 2012-07-25 Jeannie Backer 
 * @internal
 *   @history 2012-07-25 Jeannie Backer - Original version.
 */
extern "C" Isis::Projection *LambertAzimuthalEqualAreaPlugin(Isis::Pvl &lab,
    bool allowDefaults) {
  return new Isis::LambertAzimuthalEqualArea(lab, allowDefaults);
}

