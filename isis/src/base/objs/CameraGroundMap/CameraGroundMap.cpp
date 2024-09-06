/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "CameraGroundMap.h"

#include <iostream>

#include <QDebug>

#include <SpiceUsr.h>

#include "IException.h"
#include "Latitude.h"
#include "Longitude.h"
#include "NaifStatus.h"
#include "SurfacePoint.h"
#include "Target.h"

using namespace std;

namespace Isis {

  /** 
   * Constructor
   *
   * @param parent Pointer to camera to be used for mapping with ground
   */
  CameraGroundMap::CameraGroundMap(Camera *parent) {
    p_camera = parent;
    p_camera->SetGroundMap(this);
  }


  /** 
   * Compute ground position from focal plane coordinate
   *
   * This method will compute the ground position given an
   * undistorted focal plane coordinate.  Note that the latitude/longitude
   * value can be obtained from the camera pointer passed into the constructor.
   *
   * @param ux undistorted focal plane x in millimeters
   * @param uy undistorted focal plane y in millimeters
   * @param uz undistorted focal plane z in millimeters
   *
   * @return @b bool If conversion was successful
   */
  bool CameraGroundMap::SetFocalPlane(const double ux, const double uy, const double uz) {
    NaifStatus::CheckErrors();

    SpiceDouble lookC[3];
    lookC[0] = ux;
    lookC[1] = uy;
    lookC[2] = uz;

    SpiceDouble unitLookC[3];
    vhat_c(lookC, unitLookC);

    NaifStatus::CheckErrors();

    bool result = p_camera->SetLookDirection(unitLookC);
    return result;
  }


  /** 
   * Compute undistorted focal plane coordinate from ground position
   *
   * @param lat planetocentric latitude in degrees
   * @param lon planetocentric longitude in degrees
   *
   * @return @b bool If conversion was successful
   */
  bool CameraGroundMap::SetGround(const Latitude &lat, const Longitude &lon) {
    if (p_camera->target()->shape()->name() == "Plane") {
      double radius = lat.degrees(); 
      // double azimuth = lon.degrees();
      Latitude lat(0., Angle::Degrees);
      if (radius < 0.0) radius = 0.0; // TODO: massive, temporary kluge to get around testing
                                                     // latitude at -90 in caminfo app (are there
                                                     // more issues like this? Probably)KE
      if (p_camera->Sensor::SetGround(SurfacePoint(lat, lon, Distance(radius, Distance::Meters)))) {
         LookCtoFocalPlaneXY();
         return true;
      }
    }
    else {
      Distance radius(p_camera->LocalRadius(lat, lon));
      if (radius.isValid()) {
        if (p_camera->Sensor::SetGround(SurfacePoint(lat, lon, radius))) {
          LookCtoFocalPlaneXY();
          return true;
        }
      }
    }

    return false;
  }


  /**
   * Compute undistorted focal plane coordinate from camera look vector
   */
  void CameraGroundMap::LookCtoFocalPlaneXY() {
    double lookC[3];
    p_camera->Sensor::LookDirection(lookC);

    //Get the fl as the z coordinate to handle instruments looking down the -z axis 2013-02-22.
    double focalLength = p_camera->DistortionMap()->UndistortedFocalPlaneZ();
    double scale = focalLength / lookC[2];

    p_focalPlaneX = lookC[0] * scale;
    p_focalPlaneY = lookC[1] * scale;
  }


  /** 
   * Compute undistorted focal plane coordinate from ground position that includes a local radius
   *
   * @param surfacePoint Surface point (ground position) 
   *
   * @return @b bool If conversion was successful
   */
  bool CameraGroundMap::SetGround(const SurfacePoint &surfacePoint) {
    if (p_camera->Sensor::SetGround(surfacePoint)) {
      LookCtoFocalPlaneXY();
      return true;
    }

    return false;
  }


  /** 
   * Compute undistorted focal plane coordinate from ground position using current Spice 
   * from SetImage call
   *
   * This method will compute the undistorted focal plane coordinate for
   * a ground position, using the current Spice settings (time and kernels)
   * without resetting the current point values for lat/lon/radius/m_pB/x/y.  The
   * class value for m_lookJ is set by this method.
   *
   * @param point Surface point (ground position) 
   * @param cudx [out] Pointer to computed undistorted x focal plane coordinate
   * @param cudy [out] Pointer to computed undistorted y focal plane coordinate
   * @param test Optional parameter to indicate whether to do the back-of-planet test.
   *
   * @return @b bool If conversion was successful
   */
  bool CameraGroundMap::GetXY(const SurfacePoint &point, double *cudx, 
                              double *cudy, bool test) {

    vector<double> pB(3);
    pB[0] = point.GetX().kilometers();
    pB[1] = point.GetY().kilometers();
    pB[2] = point.GetZ().kilometers();

    // Check for Sky images
    if (p_camera->target()->isSky()) {
      return false;
    }

    // Should a check be added to make sure SetImage has been called???

    // Get spacecraft vector in j2000 coordinates
    SpiceRotation *bodyRot = p_camera->bodyRotation();
    SpiceRotation *instRot = p_camera->instrumentRotation();
    vector<double> pJ = bodyRot->J2000Vector(pB);
    vector<double> sJ = p_camera->instrumentPosition()->Coordinate();

    // Calculate lookJ
    vector<double> lookJ(3);
    for (int ic = 0; ic < 3; ic++) {
      lookJ[ic] = pJ[ic] - sJ[ic];
    }

    // Save pB for target body partial derivative calculations NEW *** DAC 8-14-2015
    m_pB = pB;
    
    // During iterations in the bundle adjustment do not do the back-of-planet test.
    // Failures are expected to happen during the bundle adjustment due to bad camera
    // pointing or position, poor a priori points, or inaccurate target body information.  For
    // instance, control points near the limb of an image often fail the test.  The hope is
    // that during the bundle adjustment, any variables causing points to fail the test will
    // be corrected.  If not, the point residuals will likely be large on a point that fails the
    // test.  The back-of-planet test is still a valid check for a control net diagnostic
    // program, but not for the bundle adjustment.
    // 
    // TODO It might be useful to have a separate diagnostic program test all points in a 
    //            control net to see if any of the control points fail the back-of-planet test on 
    //            any of the images.

    // Check for point on back of planet by checking to see if surface point is viewable 
    //   (test emission angle)
    if (test) {
      vector<double> lookB = bodyRot->ReferenceVector(lookJ);
      double upsB[3], upB[3], dist;
      vminus_c((SpiceDouble *) &lookB[0], upsB);
      unorm_c(upsB, upsB, &dist);
      unorm_c((SpiceDouble *) &pB[0], upB, &dist);
      double cosangle = vdot_c(upB, upsB);
      double emission;
      if (cosangle > 1) {
        emission = 0;
      }
      else if (cosangle < -1) {
        emission = 180.;
      }
      else {
        emission = acos(cosangle) * 180.0 / Isis::PI;
      }

      if (fabs(emission) > 90.) {
        return false;
      }
    }

    // Get the look vector in the camera frame and the instrument rotation
    m_lookJ.resize(3);
    m_lookJ = lookJ;
    vector<double> lookC(3);
    lookC = instRot->ReferenceVector(m_lookJ);

    // Get focal length with direction for scaling coordinates
    double fl = p_camera->DistortionMap()->UndistortedFocalPlaneZ();

    *cudx = lookC[0] * fl / lookC[2];
    *cudy = lookC[1] * fl / lookC[2];
    return true;
  }


  /** 
   * Compute undistorted focal plane coordinate from ground position using current Spice
   * from SetImage call
   *
   * This method will compute the undistorted focal plane coordinate for
   * a ground position, using the current Spice settings (time and kernels)
   * without resetting the current point values for lat/lon/radius/m_pB/x/y.  The
   * class value for m_lookJ is set by this method.
   *
   * @param lat Latitude in degrees 
   * @param lon Longitude in degrees
   * @param radius Radius in meters
   * @param cudx [out] Pointer to computed undistored x focal plane coordinate
   * @param cudy [out] Pointer to computed undistored y focal plane coordinate
   *
   * @return @b bool If conversion was successful
   *
   * @see the application socetlinescankeywords
   */
  bool CameraGroundMap::GetXY(const double lat, const double lon,
                              const double radius, double *cudx, double *cudy) {
    SurfacePoint spoint(Latitude(lat, Angle::Degrees),
                        Longitude(lon, Angle::Degrees),
                        Distance(radius, Distance::Meters));
    return GetXY(spoint, cudx, cudy);
  }


  /** 
   * Compute derivative w/r to position of focal plane coordinate from ground position
   * using current Spice from SetImage call
   *
   * This method will compute the derivative of the undistorted focal plane coordinate for
   * a ground position with respect to a spacecraft position coordinate, using the current
   * Spice settings (time and kernels) without resetting the current point values for 
   * lat/lon/radius/x/y.
   *
   * @param varType enumerated partial type (definitions in SpicePosition)
   * @param coefIndex coefficient index of fit polynomial
   * @param *dx [out] pointer to partial derivative of undistorted focal plane x
   * @param *dy [out] pointer to partial derivative of undistorted focal plane y
   *
   * @return @b bool If conversion was successful
   */
  bool CameraGroundMap::GetdXYdPosition(const SpicePosition::PartialType varType, int coefIndex,
                                        double *dx, double *dy) {

    //TODO add a check to make sure m_lookJ has been set

    // Get directional fl for scaling coordinates
    double fl = p_camera->DistortionMap()->UndistortedFocalPlaneZ();

    // Rotate look vector into camera frame
    SpiceRotation *instRot = p_camera->instrumentRotation();
    vector<double> lookC(3);
    lookC = instRot->ReferenceVector(m_lookJ);

    SpicePosition *instPos = p_camera->instrumentPosition();

    vector<double> d_lookJ = instPos->CoordinatePartial(varType, coefIndex);
    for (int j = 0; j < 3; j++) d_lookJ[j] *= -1.0;
    vector<double> d_lookC =  instRot->ReferenceVector(d_lookJ);
    *dx = fl * DQuotient(lookC, d_lookC, 0);
    *dy = fl * DQuotient(lookC, d_lookC, 1);
    return true;
  }


  /** 
   * Compute derivative of focal plane coordinate w/r to instrument using current state from 
   * SetImage call
   *
   * This method will compute the derivative of the undistorted focal plane coordinate for
   * a ground position with respect to the instrument orientation, using the current Spice
   * settings (time and kernels) without resetting the current point values for lat/lon/radius/x/y.
   *
   * @param varType enumerated partial type (definitions in SpicePosition)
   * @param coefIndex coefficient index of fit polynomial
   * @param *dx out] pointer to partial derivative of undistorted focal plane x
   * @param *dy [out] pointer to partial derivative of undistorted focal plane y
   *
   * @return @b bool If conversion was successful
   */
  bool CameraGroundMap::GetdXYdOrientation(const SpiceRotation::PartialType varType, int coefIndex,
                                           double *dx, double *dy) {

    //TODO add a check to make sure m_lookJ has been set

    // Get directional fl for scaling coordinates
    double fl = p_camera->DistortionMap()->UndistortedFocalPlaneZ();

    // Rotate J2000 look vector into camera frame
    SpiceRotation *instRot = p_camera->instrumentRotation();
    vector<double> lookC(3);
    lookC = instRot->ReferenceVector(m_lookJ);

    // Rotate J2000 look vector into camera frame through the derivative rotation
    vector<double> d_lookC = instRot->ToReferencePartial(m_lookJ, varType, coefIndex);

    *dx = fl * DQuotient(lookC, d_lookC, 0);
    *dy = fl * DQuotient(lookC, d_lookC, 1);
    return true;
  }


  /**
   * Compute derivative of focal plane coordinate w/r to target body using current state
   *
   * This method will compute the derivative of the undistorted focal plane coordinate for
   * a ground position with respect to the target body orientation, using the current Spice
   * settings (time and kernels) without resetting the current point values for lat/lon/radius/x/y.
   *
   * @param varType enumerated partial type (definitions in SpicePosition)
   * @param coefIndex coefficient index of fit polynomial
   * @param *dx [out] pointer to partial derivative of undistorted focal plane x
   * @param *dy [out] pointer to partial derivative of undistorted focal plane y
   *
   * @return @b bool If conversion was successful
   */
  bool CameraGroundMap::GetdXYdTOrientation(const SpiceRotation::PartialType varType, int coefIndex,
                                            double *dx, double *dy) {

    //TODO add a check to make sure m_pB and m_lookJ have been set. 
    // 0.  calculate or save from previous GetXY call lookB.  We need toJ2000Partial that is 
    //     like a derivative form of J2000Vector  
    // 1.  we will call d_lookJ = bodyrot->toJ2000Partial (Make sure the partials are correct for 
    //     the target body orientation matrix.
    // 2.  we will then call d_lookC = instRot->ReferenceVector(d_lookJ)
    // 3.  the rest should be the same.

    // Get directional fl for scaling coordinates
    double fl = p_camera->DistortionMap()->UndistortedFocalPlaneZ();

    // Rotate body-fixed look vector into J2000 through the derivative rotation
    SpiceRotation *bodyRot = p_camera->bodyRotation();
    SpiceRotation *instRot = p_camera->instrumentRotation();
    vector<double> dlookJ = bodyRot->toJ2000Partial(m_pB, varType, coefIndex);
    vector<double> lookC(3);
    vector<double> dlookC(3);

    // Rotate both the J2000 look vector and the derivative J2000 look vector into the camera
    lookC = instRot->ReferenceVector(m_lookJ);
    dlookC = instRot->ReferenceVector(dlookJ);

    *dx = fl * DQuotient(lookC, dlookC, 0);
    *dy = fl * DQuotient(lookC, dlookC, 1);
    return true;
  }


  /** 
   * Compute derivative of focal plane coordinate w/r to ground point using current state
   *
   * This method will compute the derivative of the undistorted focal plane coordinate for
   * a ground position with respect to lat, lon, or radius, using the current Spice settings (time
   * and kernels) without resetting the current point values for lat/lon/radius/x/y.
   *
   * @param d_pB Point vector
   * @param *dx [out] pointer to partial derivative of undistorted focal plane x
   * @param *dy [out] pointer to partial derivative of undistorted focal plane y
   *
   * @return conversion was successful 
   */
  bool CameraGroundMap::GetdXYdPoint(vector<double> d_pB, double *dx, double *dy) {

    //  TODO  add a check to make sure m_lookJ has been set

    // Get directional fl for scaling coordinates
    double fl = p_camera->DistortionMap()->UndistortedFocalPlaneZ();

    // Rotate look vector into camera frame
    SpiceRotation *instRot = p_camera->instrumentRotation();
    vector<double> lookC(3);
    lookC = instRot->ReferenceVector(m_lookJ);

    SpiceRotation *bodyRot = p_camera->bodyRotation();
    vector<double> d_lookJ = bodyRot->J2000Vector(d_pB);
    vector<double> d_lookC = instRot->ReferenceVector(d_lookJ);

    *dx = fl * DQuotient(lookC, d_lookC, 0);
    *dy = fl * DQuotient(lookC, d_lookC, 1);
    return true;
  }


  /** 
   * Compute derivative of focal plane coordinate w/r to one of the ellipsoidal radii (a, b, or c)
   *
   * This method will compute the derivative of the undistorted focal plane coordinate for
   * a ground position with respect to the a (major axis), b (minor axis), or c (polar axis) radius, 
   * using the current Spice settings (time and kernels) without resetting the current point 
   * values for lat/lon/radius/x/y.
   *
   * @param spoint Surface point whose derivative is to be evalutated
   * @param raxis Radius axis enumerated partial type (definitions in this header)
   *
   * @throws IException::Programmer "Invalid partial type for this method"
   *
   * @return @b vector<double> partialDerivative of body-fixed point with respect to selected 
   *                           ellipsoid axis
   */
  vector<double> CameraGroundMap::EllipsoidPartial(SurfacePoint spoint, PartialType raxis) {
    double rlat = spoint.GetLatitude().radians();
    double rlon = spoint.GetLongitude().radians();
    double sinLon = sin(rlon);
    double cosLon = cos(rlon);
    double sinLat = sin(rlat);
    double cosLat = cos(rlat);

    vector<double> v(3);

    switch (raxis) {
      case WRT_MajorAxis:   
         v[0] = cosLat * cosLon;
         v[1] = 0.0;
         v[2] =  0.0;
         break;
      case WRT_MinorAxis:  
         v[0] = 0.0;
         v[1] =  cosLat * sinLon;
         v[2] =  0.0;
         break;
      case WRT_PolarAxis: 
         v[0] = 0.0;
         v[1] = 0.0;
         v[2] = sinLat;
         break;
      default:
        std::string msg = "Invalid partial type for this method";
        throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return v;
  }


  /** 
   * Compute derivative of focal plane coordinate w/r to mean of the ellipsoidal radii (a, b, c)
   *
   * This method will compute the derivative of the undistorted focal plane coordinate for
   * a ground position with respect to the mean of the a (major axis), b (minor axis), and  c 
   * (polar axis) radius, using the current Spice settings (time and kernels) without resetting the
   * current point values for lat/lon/radius/x/y.
   *
   * @param spoint Surface point whose derivative is to be evalutated
   * @param meanRadius Computed mean of radii
   *
   * @return @b vector<double> partialDerivative of body-fixed point with respect to mean radius
   * TODO This method assumes the radii of all points in the adjustment have been set identically
   *            to the  ???
   */
  vector<double> CameraGroundMap::MeanRadiusPartial(SurfacePoint spoint, Distance meanRadius) {
    double radkm = meanRadius.kilometers();

    vector<double> v(3);

    v[0] = spoint.GetX().kilometers() / radkm;
    v[1] = spoint.GetY().kilometers() / radkm;
    v[2] = spoint.GetZ().kilometers() / radkm;

    return v;
  }


  /** 
   * Compute derivative with respect to indicated variable of conversion function from lat/lon/rad
   * to rectangular coord
   * 
   * @param spoint Surface point (ground position)
   * @param wrt take derivative with respect to this value
   *
   * @return @b vector<double> partialDerivative Computed derivative
   */
  vector<double> CameraGroundMap::PointPartial(SurfacePoint spoint, PartialType wrt) {
    double rlat = spoint.GetLatitude().radians();
    double rlon = spoint.GetLongitude().radians();
    double sinLon = sin(rlon);
    double cosLon = cos(rlon);
    double sinLat = sin(rlat);
    double cosLat = cos(rlat);
    double radkm = spoint.GetLocalRadius().kilometers();

    vector<double> v(3);
    if (wrt == WRT_Latitude) {
      v[0] = -radkm * sinLat * cosLon;
      v[1] = -radkm * sinLon * sinLat;
      v[2] =  radkm * cosLat;
    }
    else if (wrt == WRT_Longitude) {
      v[0] = -radkm * cosLat * sinLon;
      v[1] =  radkm * cosLat * cosLon;
      v[2] =  0.0;
    }
    else {
      v[0] = cosLon * cosLat;
      v[1] = sinLon * cosLat;
      v[2] = sinLat;
    }

    return v;
  }


  /**
   * Convenience method for quotient rule applied to look vector
   *
   * This method will compute the derivative of the following function
   * (coordinate x or y) / (coordinate z)
   *
   * @param look  look vector in camera frame
   * @param dlook derivative of look vector in camera frame
   * @param index vector value to differentiate
   *
   * @return @b double derivative Computed derivative
   */
  double CameraGroundMap::DQuotient(vector<double> &look,
                                    vector<double> &dlook,
                                    int index) {
    return (look[2] * dlook[index] - look[index] * dlook[2]) /
           (look[2] * look[2]);
  }
}
