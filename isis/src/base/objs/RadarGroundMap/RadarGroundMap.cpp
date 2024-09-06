/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "RadarGroundMap.h"
#include "iTime.h"
#include "Latitude.h"
#include "Longitude.h"
#include "Target.h"

using namespace std;

namespace Isis {
  RadarGroundMap::RadarGroundMap(Camera *parent, Radar::LookDirection ldir,
                                 double waveLength) :
    CameraGroundMap(parent) {
    p_camera = parent;
    p_lookDirection = ldir;
    p_waveLength = waveLength;

    // Angular tolerance based on radii and
    // slant range (Focal length)
//    Distance radii[3];
//    p_camera->radii(radii);
//    p_tolerance = p_camera->FocalLength() / radii[2];
//    p_tolerance *= 180.0 / Isis::PI;
    p_tolerance = .0001;

    // Compute a default time tolerance to a 1/20 of a pixel
    double et1 = p_camera->Spice::cacheStartTime().Et();
    double et2 = p_camera->Spice::cacheEndTime().Et();
    p_timeTolerance = (et2 - et1) / p_camera->Lines() / 20.0;
  }

  /** Compute ground position from slant range
   *
   * @param ux Slant range distance in meters scaled to focal plane
   * @param uy Doppler shift (always 0.0)
   * @param uz Not used
   *
   * @return conversion was successful
   */
  bool RadarGroundMap::SetFocalPlane(const double ux, const double uy,
                                     double uz) {

    SpiceRotation *bodyFrame = p_camera->bodyRotation();
    SpicePosition *spaceCraft = p_camera->instrumentPosition();

    // Get spacecraft position and velocity to create a state vector
    std::vector<double> Ssc(6);
    // Load the state into Ssc
    vequ_c((SpiceDouble *) & (spaceCraft->Coordinate()[0]), &Ssc[0]);
    vequ_c((SpiceDouble *) & (spaceCraft->Velocity()[0]), &Ssc[3]);

    // Rotate state vector to body-fixed
    std::vector<double> bfSsc(6);
    bfSsc = bodyFrame->ReferenceVector(Ssc);

    // Extract body-fixed position and velocity
    std::vector<double> Vsc(3);
    std::vector<double> Xsc(3);
    vequ_c(&bfSsc[0], (SpiceDouble *) & (Xsc[0]));
    vequ_c(&bfSsc[3], (SpiceDouble *) & (Vsc[0]));

    // Compute intrack, crosstrack, and radial coordinate
    SpiceDouble i[3];
    vhat_c(&Vsc[0], i);

    SpiceDouble c[3];
    SpiceDouble dp;
    dp = vdot_c(&Xsc[0], i);
    SpiceDouble p[3], q[3];
    vscl_c(dp, i, p);
    vsub_c(&Xsc[0], p, q);
    vhat_c(q, c);

    SpiceDouble r[3];
    vcrss_c(i, c, r);

    // What is the initial guess for R
    Distance radii[3];
    p_camera->radii(radii);
    SpiceDouble R = radii[0].kilometers();

    SpiceDouble lat = DBL_MAX;
    SpiceDouble lon = DBL_MAX;

    double slantRangeSqr = (ux * p_rangeSigma) / 1000.;   // convert to meters, then km
    slantRangeSqr = slantRangeSqr * slantRangeSqr;
    SpiceDouble X[3];

    // The iteration code was moved to its own method so that it can be run multiple times
    // if necessary. The first iteration should suffice for those pixels that have shallow
    // slopes. For those pixels that lie on steep slopes (up to 2x the incidence angle), then
    // an additional iteration call is needed. In the future, we may need to add more calls
    // to the iteration method if the slope is greater than 2x the incidence angle. The
    // slope variable will need to be halved each time the iteration method is called until
    // a solution is found. So, for example, if we needed to call the iteration method a third
    // time, the slope variable would be set to .25.
    bool useSlopeEqn = false;
    double slope = .5;
    bool success = Iterate(R,slantRangeSqr,c,r,X,lat,lon,Xsc,useSlopeEqn,slope);

    if(!success) {
      R = radii[0].kilometers();
      useSlopeEqn = true;
      success = Iterate(R,slantRangeSqr,c,r,X,lat,lon,Xsc,useSlopeEqn,slope);
    }

    if(!success) return false;

    lat = lat * 180.0 / Isis::PI;
    lon = lon * 180.0 / Isis::PI;
    while(lon < 0.0) lon += 360.0;

    // Compute body fixed look direction
    std::vector<double> lookB;
    lookB.resize(3);
    lookB[0] = X[0] - Xsc[0];
    lookB[1] = X[1] - Xsc[1];
    lookB[2] = X[2] - Xsc[2];

    std::vector<double> lookJ = bodyFrame->J2000Vector(lookB);
    SpiceRotation *cameraFrame = p_camera->instrumentRotation();
    std::vector<double> lookC = cameraFrame->ReferenceVector(lookJ);

    SpiceDouble unitLookC[3];
    vhat_c(&lookC[0], unitLookC);

    return p_camera->Sensor::SetUniversalGround(lat, lon);
  }

  /** Iteration loop for computing ground position from slant range
   *
   * @param ux Slant range distance in meters scaled to focal plane
   * @param uy Doppler shift (always 0.0)
   * @param uz Not used
   *
   * @return conversion was successful
   */
  bool RadarGroundMap::Iterate(SpiceDouble &R, const double &slantRangeSqr, const SpiceDouble c[],
                               const SpiceDouble r[], SpiceDouble X[], SpiceDouble &lat,
                               SpiceDouble &lon, const std::vector<double> &Xsc,
                               const bool &useSlopeEqn, const double &slope) {

    lat = DBL_MAX;
    lon = DBL_MAX;
    SpiceDouble lastR = DBL_MAX;
    SpiceDouble rlat;
    SpiceDouble rlon;
    int iter = 0;
    do {
      double normXsc = vnorm_c(&Xsc[0]);
      double alpha = (R * R - slantRangeSqr - normXsc * normXsc) /
                     (2.0 * vdot_c(&Xsc[0], c));

      double arg = slantRangeSqr - alpha * alpha;
      if(arg < 0.0) return false;

      double beta = sqrt(arg);
      if(p_lookDirection == Radar::Left) beta *= -1.0;

      SpiceDouble alphac[3], betar[3];
      vscl_c(alpha, c, alphac);
      vscl_c(beta, r, betar);

      vadd_c(alphac, betar, alphac);
      vadd_c(&Xsc[0], alphac, X);

      // Convert X to lat,lon
      lastR = R;
      reclat_c(X, &R, &lon, &lat);

      rlat = lat * 180.0 / Isis::PI;
      rlon = lon * 180.0 / Isis::PI;
      if(useSlopeEqn) {
        R = lastR + slope * (p_camera->LocalRadius(rlat, rlon).kilometers() - lastR);
      } else {
        R = p_camera->LocalRadius(rlat, rlon).kilometers();
      }
      
      iter++;
    }
    while(fabs(R - lastR) > p_tolerance && iter < 100);

    if(fabs(R - lastR) > p_tolerance) return false;
 
    return true;
  }

  /** Compute undistorted focal plane coordinate from ground position
   *
   * @param lat planetocentric latitude in degrees
   * @param lon planetocentric longitude in degrees
   *
   * @return conversion was successful
   */
  bool RadarGroundMap::SetGround(const Latitude &lat, const Longitude &lon) {
    Distance localRadius(p_camera->LocalRadius(lat, lon));

    if(!localRadius.isValid()) {
      return false;
    }

    return SetGround(SurfacePoint(lat, lon, p_camera->LocalRadius(lat, lon)));
  }

  /** Compute undistorted focal plane coordinate from ground position that includes a local radius
   *
   * @param lat planetocentric latitude in degrees
   * @param lon planetocentric longitude in degrees
   * @param radius local radius in meters
   *
   * @return conversion was successful
   */
  bool RadarGroundMap::SetGround(const SurfacePoint &surfacePoint) {
    // Get the ground point in rectangular coordinates (X)
    if(!surfacePoint.Valid()) return false;

    SpiceDouble X[3];
    surfacePoint.ToNaifArray(X);

    // Compute lower bound for Doppler shift
    double et1 = p_camera->Spice::cacheStartTime().Et();
    p_camera->Sensor::setTime(et1);
    double xv1 = ComputeXv(X);

    // Compute upper bound for Doppler shift
    double et2 = p_camera->Spice::cacheEndTime().Et();
    p_camera->Sensor::setTime(et2);
    double xv2 = ComputeXv(X);

    // Make sure we bound root (xv = 0.0)
    if((xv1 < 0.0) && (xv2 < 0.0)) return false;
    if((xv1 > 0.0) && (xv2 > 0.0)) return false;

    // Order the bounds
    double fl, fh, xl, xh;
    if(xv1 < xv2) {
      fl = xv1;
      fh = xv2;
      xl = et1;
      xh = et2;
    }
    else {
      fl = xv2;
      fh = xv1;
      xl = et2;
      xh = et1;
    }

    // Iterate a max of 30 times
    for(int j = 0; j < 30; j++) {
      // Use the secant method to guess the next et
      double etGuess = xl + (xh - xl) * fl / (fl - fh);

      // Compute the guessed Doppler shift.  Hopefully
      // this guess converges to zero at some point
      p_camera->Sensor::setTime(etGuess);
      double fGuess = ComputeXv(X);

      // Update the bounds
      double delTime;
      if(fGuess < 0.0) {
        delTime = xl - etGuess;
        xl = etGuess;
        fl = fGuess;
      }
      else {
        delTime = xh - etGuess;
        xh = etGuess;
        fh = fGuess;
      }

      // See if we are done
      if((fabs(delTime) <= p_timeTolerance) || (fGuess == 0.0)) {
        SpiceRotation *bodyFrame = p_camera->bodyRotation();
        SpicePosition *spaceCraft = p_camera->instrumentPosition();

        // Get body fixed spacecraft velocity and position
        std::vector<double> Ssc(6);

        // Load the state into Ssc and rotate to body-fixed
        vequ_c((SpiceDouble *) & (spaceCraft->Coordinate()[0]), &Ssc[0]);
        vequ_c((SpiceDouble *) & (spaceCraft->Velocity()[0]), &Ssc[3]);
        std::vector<double> bfSsc(6);
        bfSsc = bodyFrame->ReferenceVector(Ssc);

        // Extract the body-fixed position and velocity from the state
        std::vector<double> Vsc(3);
        std::vector<double> Xsc(3);
        vequ_c(&bfSsc[0], (SpiceDouble *) & (Xsc[0]));
        vequ_c(&bfSsc[3], (SpiceDouble *) & (Vsc[0]));

        // Determine if focal plane coordinate falls on the correct side of the
        // spacecraft. Radar has both left and right look directions. Make sure
        // the coordinate is on the same side as the look direction. This is done
        // by (X - S) . (V x S) where X=ground point vector, S=spacecraft position
        // vector, and V=velocity vector. If the dot product is greater than 0, then
        // the point is on the right side. If the dot product is less than 0, then
        // the point is on the left side. If the dot product is 0, then the point is
        // directly under the spacecraft (neither left or right) and is invalid.
        SpiceDouble vout1[3];
        SpiceDouble vout2[3];
        SpiceDouble dp;
        vsub_c(X, &Xsc[0], vout1);
        vcrss_c(&Vsc[0], &Xsc[0], vout2);
        dp = vdot_c(vout1, vout2);
        if(dp > 0.0 && p_lookDirection == Radar::Left) return false;
        if(dp < 0.0 && p_lookDirection == Radar::Right) return false;
        if(dp == 0.0) return false;



        // Compute body fixed look direction
        std::vector<double> lookB;
        lookB.resize(3);
        lookB[0] = X[0] - Xsc[0];
        lookB[1] = X[1] - Xsc[1];
        lookB[2] = X[2] - Xsc[2];

        std::vector<double> lookJ = bodyFrame->J2000Vector(lookB);
        SpiceRotation *cameraFrame = p_camera->instrumentRotation(); //this is the pointer to the camera's SpiceRotation/instrumentatrotation object
        std::vector<double> lookC = cameraFrame->ReferenceVector(lookJ);

        SpiceDouble unitLookC[3];
        vhat_c(&lookC[0], unitLookC);

        p_camera->SetFocalLength(p_slantRange * 1000.0); // p_slantRange is km so focal length is in m
        p_focalPlaneX = p_slantRange * 1000.0 / p_rangeSigma; // km to meters and scaled to focal plane
        p_focalPlaneY = 0.0;
        p_camera->target()->shape()->setSurfacePoint(surfacePoint); // Added 2-11-2013 by DAC

        // set the sensor's ground point and also makes it possible to calculate m_ra & m_dec

        return p_camera->Sensor::SetGround(surfacePoint, true);
      }
    }

    return false;
  }


  /** Compute undistorted focal plane coordinate from ground position using current Spice from SetImage call
   *
   * This method will compute the undistorted focal plane coordinate for
   * a ground position, using the current Spice settings (time and kernels)
   * without resetting the current point values for lat/lon/radius/x/y and
   * related radar parameter p_slantRange.
   *
   *  @history 2019-05-15 Debbie A. Cook - Added optional bool argument to match parent GetXY 
   *                          method to allow the bundle adjustment to skip the back of planet test during 
   *                          iterations. References #2591.
   *
   * @param spoint
   *
   * @return conversion was successful
   */
  bool RadarGroundMap::GetXY(const SurfacePoint &spoint, double *cudx,
                             double *cudy, bool test) {

    // Get the ground point in rectangular body-fixed coordinates (X)
    double X[3];
    X[0] = spoint.GetX().kilometers();
    X[1] = spoint.GetY().kilometers();
    X[2] = spoint.GetZ().kilometers();

    // Compute body-fixed look vector
    SpiceRotation *bodyFrame = p_camera->bodyRotation();
    SpicePosition *spaceCraft = p_camera->instrumentPosition();

    std::vector<double> sJ(6);   // Spacecraft state vector (position and velocity) in J2000 frame
    // Load the state into sJ
    vequ_c((SpiceDouble *) & (spaceCraft->Coordinate()[0]), &sJ[0]);
    vequ_c((SpiceDouble *) & (spaceCraft->Velocity()[0]), &sJ[3]);

    // Rotate the state to body-fixed
    p_sB.resize(6);
    p_sB = bodyFrame->ReferenceVector(sJ);

    // Extract the body-fixed position and velocity
    SpiceDouble VsB[3];
    SpiceDouble PsB[3];
    vequ_c(&p_sB[0], PsB);
    vequ_c(&p_sB[3], VsB);

    p_lookB.resize(3);
    vsub_c(X, PsB, &p_lookB[0]);

    p_groundSlantRange = vnorm_c(&p_lookB[0]);  // km
    p_groundDopplerFreq = 2. / p_waveLength / p_groundSlantRange * vdot_c(&p_lookB[0], &VsB[0]);
    *cudx = p_groundSlantRange * 1000.0 / p_rangeSigma;  // to meters, then to focal plane coord
    *cudy = p_groundDopplerFreq / p_dopplerSigma;   // htx to focal plane coord

    if (test == true) {
      std::string msg = "Back of planet test is not enabled for Radar images";
       throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return true;
  }


  double RadarGroundMap::ComputeXv(SpiceDouble X[3]) {
    // Get the spacecraft position (Xsc) and velocity (Vsc) in body fixed
    // coordinates
    SpiceRotation *bodyFrame = p_camera->bodyRotation();
    SpicePosition *spaceCraft = p_camera->instrumentPosition();

    // Load the state into Ssc
    std::vector<double> Ssc(6);
    vequ_c((SpiceDouble *) & (spaceCraft->Coordinate()[0]), &Ssc[0]);
    vequ_c((SpiceDouble *) & (spaceCraft->Velocity()[0]), &Ssc[3]);

    // Rotate the state to body-fixed
    std::vector<double> bfSsc(6);
    bfSsc = bodyFrame->ReferenceVector(Ssc);

    // Extract the body-fixed position and velocity
    std::vector<double> Vsc(3);
    std::vector<double> Xsc(3);
    vequ_c(&bfSsc[0], &Xsc[0]);
    vequ_c(&bfSsc[3], &Vsc[0]);

    // Compute the slant range
    SpiceDouble lookB[3];
    vsub_c(&Xsc[0], X, lookB);
    p_slantRange = vnorm_c(lookB);   // units are km

    // Compute and return xv = -2 * (point - observer) dot (point velocity - observer velocity) / (slantRange*wavelength)
    // In body-fixed coordinates, the point velocity = 0. so the equation becomes
    //    double xv = 2.0 * vdot_c(lookB,&Vsc[0]) / (vnorm_c(lookB) * WaveLength() );
    double xv = -2.0 * vdot_c(lookB, &Vsc[0]) / (vnorm_c(lookB) * WaveLength()); // - is applied to lookB above
    return xv;
  }


  /** Compute derivative w/r to position of focal plane coordinate from ground position using current Spice from SetImage call
   *
   * This method will compute the derivative of the undistorted focal plane coordinate for
   * a ground position with respect to a spacecraft position coordinate, using the current
   * Spice settings (time and kernels) without resetting the current point values for lat/lon/radius/x/y.
   *
   * @param varType enumerated partial type (definitions in SpicePosition)
   * @param coefIndex coefficient index of fit polynomial
   * @param *dx pointer to partial derivative of undistorted focal plane x
   * @param *dy pointer to partial derivative of undistorted focal plane y
   *
   * @return conversion was successful
   */
  // d_slantRange = (lookB dot d_lookB) / slantRange
  // d_dopplerFrequency = -dopplerFrequency/slantRange*d_slantRange -
  //                      2./wavelength/slantRange*(d_lookB dot vlookB) -
  //                      2./wavelength/slantRange*(lookB dot d_vlookB)

  // Add the partial for the x coordinate of the position (differentiating
  // point(x,y,z) - spacecraftPosition(x,y,z) in body-fixed and the velocity
  // Load the derivative of the state into d_lookJ
  bool RadarGroundMap::GetdXYdPosition(const SpicePosition::PartialType varType, int coefIndex,
                                       double *dx, double *dy) {
    SpicePosition *instPos = p_camera->instrumentPosition();
    SpiceRotation *bodyRot = p_camera->bodyRotation();

    std::vector <double> d_lookJ(6);
    vequ_c(&(instPos->CoordinatePartial(varType, coefIndex))[0], &d_lookJ[0]);
    vequ_c(&(instPos->VelocityPartial(varType, coefIndex))[0], &d_lookJ[3]);

    std::vector<double> d_lookB =  bodyRot->ReferenceVector(d_lookJ);

    double d_slantRange = (-1.) * vdot_c(&p_lookB[0], &d_lookB[0]) / p_groundSlantRange;
    double d_dopplerFreq = (-1.) * p_groundDopplerFreq * d_slantRange / p_groundSlantRange -
                           2. / p_waveLength / p_groundSlantRange * vdot_c(&d_lookB[0], &p_sB[3]) +
                           2. / p_waveLength / p_groundSlantRange * vdot_c(&p_lookB[0], &d_lookB[3]);

    *dx = d_slantRange * 1000.0 / p_rangeSigma;// km to meters, then to focal plane coord
    *dy = d_dopplerFreq / p_dopplerSigma;   // htz scaled to focal plane

    return true;
  }

  /** Compute derivative of focal plane coordinate w/r to ground point from ground position using current Spice from SetImage call
   *
   * This method will compute the derivative of the undistorted focal plane coordinate for
   * a ground position with respect to lat, lon, or radius, using the current Spice settings (time and kernels)
   * without resetting the current point values for lat/lon/radius/x/y.
   *
   * @param varType enumerated partial type (definitions in SpicePosition)
   * @param coefIndex coefficient index of fit polynomial
   * @param *dx pointer to partial derivative of undistorted focal plane x
   * @param *dy pointer to partial derivative of undistorted focal plane y
   *
   * @return conversion was successful
   */
  bool RadarGroundMap::GetdXYdPoint(std::vector<double> d_lookB,
                                    double *dx, double *dy) {

    //  TODO  add a check to make sure p_lookB has been set

    double d_slantRange = vdot_c(&p_lookB[0], &d_lookB[0]) / p_groundSlantRange;  // km
    // After switching to J2000, the last term will not be 0. as it is in body-fixed
//    double d_dopplerFreq = p_groundDopplerFreq*d_slantRange/p_groundSlantRange // Ken
//      + 2./p_waveLength/p_groundSlantRange*vdot_c(&d_lookB[0], &p_sB[3]);
    double d_dopplerFreq = (-1.) * p_groundDopplerFreq * d_slantRange / p_groundSlantRange
                           + 2. / p_waveLength / p_groundSlantRange * vdot_c(&d_lookB[0], &p_sB[3]);
//        + 2./p_wavelength/slantRange*vdot_c(&p_lookB[0], 0);

    *dx = d_slantRange * 1000.0 / p_rangeSigma;
    *dy = d_dopplerFreq / p_dopplerSigma;

    return true;
  }

}
