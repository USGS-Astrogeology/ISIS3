/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "RadarSlantRangeMap.h"

#include "IString.h"
#include "iTime.h"
#include "PvlSequence.h"

using namespace std;

namespace Isis {
  /** Radar ground to slant range map constructor
   *
   * Create a map from ground range distance to slant range
   * distance on a radar instrument
   *
   * @param parent        the parent camera that will use this distortion map
   *
   */
  RadarSlantRangeMap::RadarSlantRangeMap(Camera *parent, double groundRangeResolution) :
    CameraDistortionMap(parent, 1.0) {

    p_camera = parent;
    p_et = DBL_MAX;
    p_a[0] = p_a[1] = p_a[2] = p_a[3] = 0.0;

    // Need to come up with an initial guess when solving for ground
    // range given slantrange. We will compute the ground range at the
    // near and far edges of the image by evaluating the sample-to-
    // ground-range equation: r_gnd=(S-1)*groundRangeResolution
    // at the edges of the image. We also need to add some padding to
    // allow for solving for coordinates that are slightly outside of
    // the actual image area. Use S=-0.25*p_camera->Samples() and
    // S=1.25*p_camera->Samples().
    // The above approach works nicely for a level 1 image but the
    // sensor model at level2 doesn't have access to the level 1 
    // number of samples. Instead, we will calculate initial guesses
    // on the fly in SetUndistortedFocalPlane.
    p_tolerance = 0.1; // Default tolerance is a tenth of a meter
    p_maxIterations = 30;

  }

  /** Set the ground range and compute a slant range
   *
   */
  bool RadarSlantRangeMap::SetFocalPlane(const double dx, const double dy) {
    p_focalPlaneX = dx;  // dx is a ground range distance in meters
    p_focalPlaneY = dy;  // dy is Doppler shift in htz and should always be 0

    if (p_et != p_camera->time().Et()) ComputeA();
    double slantRange = p_a[0] + p_a[1] * dx + p_a[2] * dx * dx + p_a[3] * dx * dx * dx; // meters

    p_camera->SetFocalLength(slantRange);
    p_undistortedFocalPlaneX = slantRange / p_rangeSigma;
    p_undistortedFocalPlaneY = 0;

    return true;
  }

  /** Set the slant range and compute a ground range
   *
   */
  bool RadarSlantRangeMap::SetUndistortedFocalPlane(const double ux,
      const double uy) {
    p_undistortedFocalPlaneX = ux * p_rangeSigma;    // ux converts to slant range in meters
    p_undistortedFocalPlaneY = uy * p_dopplerSigma;  // uy converts to Doppler shift in htz and should always be 0

    if (p_et != p_camera->time().Et()) ComputeA();

    double slant = p_undistortedFocalPlaneX;
    // First trap the case where no iteration is needed. Since this occurs at
    // the first pixel of the image, it's a real possibility to encounter.
    if (fabs(slant-p_a[0]) < p_tolerance) {
      p_focalPlaneX = 0.0;
      p_focalPlaneY = 0.0;
      return true;
    }
    // Now calculate two guesses by the first and second iterations of
    // Newton's method, where the zeroth iteration is at ground range = 0.
    // The nature of the slant range function is such that, in the region
    // of validity (including all image data) the Newton approximations are
    // always too high, but the second one is within a few meters.  We therefore
    // add 10 m of “windage” to it to bracket the root.
    // Performing a third Newton iteration would give a satisfactory solution in
    // the data area but raises the problem of trapping errors outside this region
    // where the polynomial is not so well behaved.
    // Use the “min” variables temporarily to hold the first approximation
    p_initialMinGroundRangeGuess = (slant - p_a[0]) / p_a[1];
    double minGroundRangeGuess = slant - (p_a[0] + p_initialMinGroundRangeGuess *
       (p_a[1] + p_initialMinGroundRangeGuess * (p_a[2] + p_initialMinGroundRangeGuess * p_a[3])));
    // Now the max is the second approximation
    p_initialMaxGroundRangeGuess = p_initialMinGroundRangeGuess + minGroundRangeGuess /
       (p_a[1] + p_initialMinGroundRangeGuess * (2.0 * p_a[2] + p_initialMinGroundRangeGuess *
       3.0 * p_a[3]));
    double maxGroundRangeGuess = slant - (p_a[0] + p_initialMaxGroundRangeGuess *
                                          (p_a[1] + p_initialMaxGroundRangeGuess * (p_a[2] +
                                              p_initialMaxGroundRangeGuess * p_a[3])));
    // Finally, apply the “windage” to bracket the root.
    p_initialMinGroundRangeGuess = p_initialMaxGroundRangeGuess - 10.0;
    minGroundRangeGuess = slant - (p_a[0] + p_initialMinGroundRangeGuess *
       (p_a[1] + p_initialMinGroundRangeGuess * (p_a[2] + p_initialMinGroundRangeGuess * p_a[3])));

    // If both guesses are on the same side of zero, we need to expand the bracket range
    // to include a zero-crossing. 
    if ((minGroundRangeGuess < 0.0 && maxGroundRangeGuess < 0.0) ||
        (minGroundRangeGuess > 0.0 && maxGroundRangeGuess > 0.0)) {

      int maxBracketIters = 10; 
      
      float xMin = p_initialMinGroundRangeGuess;
      float xMax = p_initialMaxGroundRangeGuess;

      float funcMin = minGroundRangeGuess;
      float funcMax = maxGroundRangeGuess; 

      for (int j=0; j<maxBracketIters; j++) {

        //distance between guesses
        float dist = abs(abs(xMin) - abs(xMax));
        
        // move move the x-value of the closest root twice as far away from the other root as it was
        // before to extend the bracket range. 
        if (abs(funcMin) <= abs(funcMax)) {
          //min is closer
          xMin = xMax - 2*dist; 
        }
        else {
          //max is closer
          xMax = xMin + 2*dist;
        }

        funcMin = slant -(p_a[0]+ xMin *(p_a[1] + xMin * (p_a[2] + xMin * p_a[3])));
        funcMax = slant - (p_a[0] + xMax *(p_a[1] + xMax * (p_a[2] + xMax * p_a[3])));
        
        // if we've successfully bracketed the root, we can break. 
        if ((funcMin <= 0.0 && funcMax >= 0.0) || (funcMin >= 0.0 && funcMax <= 0.0)){
          p_initialMinGroundRangeGuess = xMin;
          p_initialMaxGroundRangeGuess = xMax;
          minGroundRangeGuess = funcMin;
          maxGroundRangeGuess = funcMax; 
          break; 
        }
      }
    }
    
    // If the ground range guesses at the 2 extremes of the image are equal
    // or they have the same sign, then the ground range cannot be solved for.
    // The only case where they are equal should be at zero, which we already trapped.
    if ((minGroundRangeGuess == maxGroundRangeGuess) ||
        (minGroundRangeGuess < 0.0 && maxGroundRangeGuess < 0.0) ||
       (minGroundRangeGuess > 0.0 && maxGroundRangeGuess > 0.0)) {
      return false;
    }

    // Use Wijngaarden/Dekker/Brent algorithm to find a root of the function:
    // g(groundRange) = slantRange - (p_a[0] + groundRange * (p_a[1] +
    //                  groundRange * (p_a[2] + groundRange * p_a[3])))
    // The algorithm used is a combination of the bisection method with the
    // secant method.
    int iter = 0;
    double eps = 3.E-8;
    double ax = p_initialMinGroundRangeGuess;
    double bx = p_initialMaxGroundRangeGuess;
    double fax = minGroundRangeGuess;
    double fbx = maxGroundRangeGuess;
    double fcx = fbx;
    double cx = 0.0;
    double d = 0.0;
    double e = 0.0;
    double tol1;
    double xm;
    double p, q, r, s, t;

    do {
      iter++;
      if (fbx * fcx > 0.0) {
        cx = ax;
        fcx = fax;
        d = bx - ax;
        e = d;
      }
      if (fabs(fcx) < fabs(fbx)) {
        ax = bx;
        bx = cx;
        cx = ax;
        fax = fbx;
        fbx = fcx;
        fcx = fax;
      }
      tol1 = 2.0 * eps * fabs(bx) + 0.5 * p_tolerance;
      xm = 0.5 * (cx - bx);
      if (fabs(xm) <= tol1 || fbx == 0.0) {
        p_focalPlaneX = bx;
        p_focalPlaneY = 0.0;
        return true;
      }
      if (fabs(e) >= tol1 && fabs(fax) > fabs(fbx)) {
        s = fbx / fax;
        if (ax == cx) {
          p = 2.0 * xm * s;
          q = 1.0 - s;
        }
        else {
          q = fax / fcx;
          r = fbx / fcx;
          p = s * (2.0 * xm * q * (q - r) - (bx - ax) * (r - 1.0));
          q = (q - 1.0) * (r - 1.0) * (s - 1.0);
        }
        if (p > 0.0) q = -q;
        p = fabs(p);
        t = 3.0 * xm * q - fabs(tol1 * q);
        if (t > fabs(e * q)) t = fabs(e * q);
        if (2.0 * p < t) {
          e = d;
          d = p / q;
        }
        else {
          d = xm;
          e = d;
        }
      }
      else {
        d = xm;
        e = d;
      }
      ax = bx;
      fax = fbx;
      if (fabs(d) > tol1) {
        bx = bx + d;
      }
      else {
        if (xm >= 0.0) {
          t = fabs(tol1);
        }
        else {
          t = -fabs(tol1);
        }
        bx = bx + t;
      }
      fbx = slant - (p_a[0] + bx * (p_a[1] + bx * (p_a[2] + bx * p_a[3])));
    }
    while (iter <= p_maxIterations);

    return false;
  }

  /** Load the ground range/slant range coefficients from the
   *  RangeCoefficientSet keyword
   *
   */
  void RadarSlantRangeMap::SetCoefficients(PvlKeyword &keyword) {
    PvlSequence seq;
    seq = keyword;
    for (int i = 0; i < seq.Size(); i++) {
      // TODO:  Test array size to be 4 if not throw error
      std::vector<std::string> array = seq[i];
      double et;
      utc2et_c(array[0].c_str(), &et);
      p_time.push_back(et);
      p_a0.push_back(Isis::toDouble(array[1]));
      p_a1.push_back(Isis::toDouble(array[2]));
      p_a2.push_back(Isis::toDouble(array[3]));
      p_a3.push_back(Isis::toDouble(array[4]));
      // TODO:  Test that times are ordered if not throw error
      // Make the mrf2isis program sort them if necessary
    }
  }

  /** Set new A-coefficients based on the current ephemeris time.
   *  The A-coefficients used will be those with the closest
   *  ephemeris time to the current ephemeris time.
   */
  void RadarSlantRangeMap::ComputeA() {
    double currentEt = p_camera->time().Et();

    std::vector<double>::iterator pos = lower_bound(p_time.begin(), p_time.end(), currentEt);

    int index = p_time.size() - 1;
    if (currentEt <= p_time[0]) {
      p_a[0] = p_a0[0];
      p_a[1] = p_a1[0];
      p_a[2] = p_a2[0];
      p_a[3] = p_a3[0];
    }
    else if (currentEt >= p_time.at(index)) {
      p_a[0] = p_a0[index];
      p_a[1] = p_a1[index];
      p_a[2] = p_a2[index];
      p_a[3] = p_a3[index];
    } else { 
      index = pos - p_time.begin();
      double weight = (currentEt - p_time.at(index-1)) / 
                      (p_time.at(index) - p_time.at(index-1));
      p_a[0] = p_a0[index-1] * (1.0 - weight) + p_a0[index] * weight;
      p_a[1] = p_a1[index-1] * (1.0 - weight) + p_a1[index] * weight;
      p_a[2] = p_a2[index-1] * (1.0 - weight) + p_a2[index] * weight;
      p_a[3] = p_a3[index-1] * (1.0 - weight) + p_a3[index] * weight;
    }
  }

  /** Set the weight factors for slant range and Doppler shift
   *
   */
  void RadarSlantRangeMap::SetWeightFactors(double range_sigma, double doppler_sigma) {
    p_rangeSigma = range_sigma; // meters scaling factor
    p_dopplerSigma = doppler_sigma; // htz scaling factor
  }
}
