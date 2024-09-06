/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "IString.h"
#include "CameraDistortionMap.h"

namespace Isis {
  /**
   * Camera distortion map constructor
   *
   * Create a radial camera distortion map.  This class maps between
   * distorted and undistorted focal plane x/y's.  The default mapping
   * is the identity, that is, the focal plane x/y and undistorted
   * focal plane x/y will be identical.
   *
   * @param parent        the parent camera that will use this distortion map
   * @param zDirection    the direction of the focal plane Z-axis
   *                      (either 1 or -1)
   *
   */
  CameraDistortionMap::CameraDistortionMap(Camera *parent, double zDirection) {
    p_camera = parent;
    p_camera->SetDistortionMap(this);
    p_zDirection = zDirection;
  }


  /**
   * Destructor for the ISIS default camera distortion map. 
   */
  CameraDistortionMap::~CameraDistortionMap() {
  }


  /**
   * Load distortion coefficients
   *
   * This method loads the distortion coefficients from the instrument
   * kernel.  The coefficients in the NAIF instrument kernel are
   * expected to be in the form of:
   *
   * @code
   * INSxxxxx_OD_K = ( coef1, coef2, ..., coefN)
   *
   * where xxxxx is the instrument code (always a negative number)
   * @endcode
   *
   * These coefficient will be used to convert from focal plane x,y
   * to undistorted x,y as follows (add equation here)
   *
   * @param naifIkCode    Code to search for in instrument kernel
   * @todo Generalize to read variable number of coefficients
   * @todo Add latex equation to the documentation
   */
  void CameraDistortionMap::SetDistortion(int naifIkCode) {
    p_odk.clear();
    QString odkkey = "INS" + QString::number(naifIkCode) + "_OD_K";
    for (int i = 0; i < 3; ++i) {
      p_odk.push_back(p_camera->Spice::getDouble(odkkey, i));
    }
  }


  /**
   * Compute undistorted focal plane x/y
   *
   * Compute undistorted focal plane x/y given a distorted focal plane x/y.
   * This virtual method can be used to apply various techniques for removing
   * optical distortion in the focal plane of a camera.  The default
   * implementation uses a polynomial distortion if the SetDistortion method
   * is invoked.  After calling this method, you can obtain the undistorted
   * x/y via the UndistortedFocalPlaneX and UndistortedFocalPlaneY methods
   *
   * @param dx distorted focal plane x in millimeters
   * @param dy distorted focal plane y in millimeters
   *
   * @return if the conversion was successful
   * @see SetDistortion
   * @todo Generalize polynomial equation
   */
  bool CameraDistortionMap::SetFocalPlane(double dx, double dy) {
    p_focalPlaneX = dx;
    p_focalPlaneY = dy;

    // No coefficients == no distortion
    if (p_odk.size() <= 0) {
      p_undistortedFocalPlaneX = dx;
      p_undistortedFocalPlaneY = dy;
      return true;
    }

    // Get the distance from the focal plane center and if we are close
    // then skip the distortion
    double r2 = (dx * dx) + (dy * dy);
    if (r2 <= 1.0E-6) {
      p_undistortedFocalPlaneX = dx;
      p_undistortedFocalPlaneY = dy;
      return true;
    }

    // Ok we need to apply distortion correction
    double drOverR = p_odk[0] + (r2 * (p_odk[1] + (r2 * p_odk[2])));
    p_undistortedFocalPlaneX = dx - (drOverR * dx);
    p_undistortedFocalPlaneY = dy - (drOverR * dy);
    return true;
  }


  /**
   * Compute distorted focal plane x/y
   *
   * Compute distorted focal plane x/y given an undistorted focal plane x/y.
   * This virtual method is used to apply various techniques for adding
   * optical distortion in the focal plane of a camera.  The default
   * implementation of this virtual method uses a polynomial distortion if
   * the SetDistortion method was invoked.
   * After calling this method, you can obtain the distorted x/y via the
   * FocalPlaneX and FocalPlaneY methods
   *
   * @param ux undistorted focal plane x in millimeters
   * @param uy undistorted focal plane y in millimeters
   *
   * @return if the conversion was successful
   * @see SetDistortion
   * @todo Generalize polynomial equation
   * @todo Figure out a better solution for divergence condition
   */
  bool CameraDistortionMap::SetUndistortedFocalPlane(const double ux,
      const double uy) {
    p_undistortedFocalPlaneX = ux;
    p_undistortedFocalPlaneY = uy;

    // No coefficients == nodistortion
    if (p_odk.size() <= 0) {
      p_focalPlaneX = ux;
      p_focalPlaneY = uy;
      return true;
    }

    // Compute the distance from the focal plane center and if we are
    // close to the center then no distortion is required
    double rp2 = (ux * ux) + (uy * uy);
    if (rp2 <= 1.0E-6) {
      p_focalPlaneX = ux;
      p_focalPlaneY = uy;
      return true;
    }

    // Ok make the correction, start by computing
    // fractional distortion at rp (r-prime)
    double rp = sqrt(rp2);
    double drOverR = p_odk[0] + (rp2 * (p_odk[1] + (rp2 * p_odk[2])));

    // Estimate r
    double r = rp + (drOverR * rp);
    double r_prev, r2_prev;
    double tolMilliMeters = p_camera->PixelPitch() / 100.0;
    int iteration = 0;
    do {
      // Don't get in an end-less loop.  This algorithm should
      // converge quickly.  If not then we are probably way outside
      // of the focal plane.  Just set the distorted position to the
      // undistorted position. Also, make sure the focal plane is less
      // than 1km, it is unreasonable for it to grow larger than that.
      if (iteration >= 15 || r > 1E9) {
        drOverR = 0.0;
        break;
      }

      r_prev = r;
      r2_prev = r * r;

      // Compute new fractional distortion:
      drOverR = p_odk[0] + (r2_prev * (p_odk[1] + (r2_prev * p_odk[2])));

      r = rp + (drOverR * r_prev);  // Compute new estimate of r
      iteration++;
    }
    while (fabs(r - r_prev) > tolMilliMeters);

    p_focalPlaneX = ux / (1.0 - drOverR);
    p_focalPlaneY = uy / (1.0 - drOverR);
    return true;
  }


  /**
   * Retrieve the distortion coefficients used for this model.
   *
   * @returns The optical distortion of polynomial coefficients.
   */
  std::vector<double> CameraDistortionMap::OpticalDistortionCoefficients() const {
    return p_odk;
  }


  /**
   * Gets the z-direction for this camera. The direction is represented as
   * +/- 1.
   * @returns The direction of the focal plane Z-axis
   */
  double CameraDistortionMap::ZDirection() const {
    return p_zDirection;
  }


  /**
   * Gets the x-value in the focal plane coordinate system. This is represented
   * in millimeters from the origin.
   *
   * @returns The distorted focal plane x.
   */
  double CameraDistortionMap::FocalPlaneX() const {
    return p_focalPlaneX;
  }


  /**
   * Gets the y-value in the focal plane coordinate system. This is represented
   * in millimeters from the origin.
   *
   * @returns The distorted focal plane y.
   */
  double CameraDistortionMap::FocalPlaneY() const {
    return p_focalPlaneY;
  }


  /**
   * Gets the x-value in the undistorted focal plane coordinate system. This is represented
   * in millimeters from the origin.
   *
   *@returns The undistorted focal plane x
   */
  double CameraDistortionMap::UndistortedFocalPlaneX() const {
    return p_undistortedFocalPlaneX;
  }


  /**
   * Gets the y-value in the undistorted focal plane coordinate system. This is represented
   * in millimeters from the origin.
   *
   * @returns The undistorted focal plane y
   */
  double CameraDistortionMap::UndistortedFocalPlaneY() const {
    return p_undistortedFocalPlaneY;
  }


  /**
   * Gets the z-value in the undistorted focal plane coordinate system. This is
   * represented as the focal length, in millimeters, with the sign indicating
   * z-direction.
   *
   * @returns The undistorted focal plane z
   */
  double CameraDistortionMap::UndistortedFocalPlaneZ() const {
    return p_zDirection *p_camera->FocalLength();
  }

}
