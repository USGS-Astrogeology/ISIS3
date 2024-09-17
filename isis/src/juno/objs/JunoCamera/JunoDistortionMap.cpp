/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "IString.h"
#include "JunoDistortionMap.h"

namespace Isis {
  /**
   * Juno JunoCam distortion map constructor
   *
   * Create a distortion map for Juno's JunoCam camera. This class maps between distorted
   * and undistorted focal plane x/y's.  The default mapping is the
   * identity, that is, the focal plane x/y and undistorted focal plane
   * x/y will be identical. The Z direction is set internally to positive for
   * JunoCam.
   *
   * @param parent        the parent camera that will use this distortion map
   *
   */
  JunoDistortionMap::JunoDistortionMap(Camera *parent)
      : CameraDistortionMap(parent, 1.0) {
  }


  /**
   * Destructor
   */
  JunoDistortionMap::~JunoDistortionMap() {
  }


  /**
   *  Load distortion coefficients for JunoCam
   *
   * This method loads the distortion coefficients from the instrument
   * kernel.  JunoCam's coefficients in the NAIF instrument kernel are
   * expected to be in the form of:
   *
   * @code
   * INS-61500_DISTORTION_K0 = coefficient, index 0
   * INS-61500_DISTORTION_K1 = coefficient, index 1
   * INS-61500_DISTORTION_K2 = coefficient, index 2
   * @endcode
   *
   * These coefficients are designed for use with pixel coordinates, so they
   * are scaled based on the pixel pitch to operate in focal plane millimeters.
   * These coefficient will be used to convert from undistorted focal plane x,y
   * to distorted focal plane x,y as follows
   *
   * @code
   * r2 = r2 = (ux * ux) + (uy * uy);
   * dr = 1 + INS-61500_DISTORTION_K0 + INS-61500_DISTORTION_K1*r2 +INS-61500_DISTORTION_K2*r2*r2;
   * dx = ux * dr;
   * dy = uy * dr;
   * @endcode
   *
   * @param naifIkCode Code to search for in instrument kernel
   */
  void JunoDistortionMap::SetDistortion(int naifIkCode) {

    // Use the pixel pitch to scale k1 and k2 coefficients to operate in focal
    // plane coordinates (millimeters). The coefficients found in the kernels
    // are based on detector coordinates (pixels).

    double pp = p_camera->PixelPitch();
    double p2 = pp * pp;

    // Currently k0 is non-existant in kernels (i.e equals zero). The try is
    // here in case this coefficient is needed for future distortion models.
    try {
      QString odk0 = "INS" + QString::number(naifIkCode) + "_DISTORTION_K0";
      p_odk.push_back(p_camera->Spice::getDouble(odk0));

    }
    catch (IException &e) {
      p_odk.push_back(0.0);
    }

    QString odk1 = "INS" + QString::number(naifIkCode) + "_DISTORTION_K1";
    p_odk.push_back(p_camera->Spice::getDouble(odk1) / p2);
    QString odk2 = "INS" + QString::number(naifIkCode) + "_DISTORTION_K2";
    p_odk.push_back(p_camera->Spice::getDouble(odk2) / (p2 * p2));
  }


  /**
   *  Compute distorted focal plane x/y
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
   * @return @b if the conversion was successful
   *
   * @see SetDistortion
   */
  bool JunoDistortionMap::SetUndistortedFocalPlane(const double ux,
                                                   const double uy) {

    p_undistortedFocalPlaneX = ux;
    p_undistortedFocalPlaneY = uy;

    // Compute the distance from the focal plane center and if we are
    // close to the center then assume no distortion
    double r2 = (ux * ux) + (uy * uy);
    if (r2 <= 1.0E-6) {
      p_focalPlaneX = ux;
      p_focalPlaneY = uy;
      return true;
    }

    // The equation given in the IK computes the undistorted focal plane
    // ux = dx * (1 + k1*r^2), r^2 = dx^2 + dy^2
    double dr = 1 + p_odk[0] + p_odk[1]*r2 + p_odk[2]*r2*r2;
    p_focalPlaneX = ux * dr;
    p_focalPlaneY = uy * dr;

    return true;

  }


  /**
   *  Compute undistorted focal plane x/y
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
  bool JunoDistortionMap::SetFocalPlane(double dx,
                                        double dy) {
    p_focalPlaneX = dx;
    p_focalPlaneY = dy;

    // Get the distance from the focal plane center and if we are close
    // then skip the distortion
    double r2 = (dx * dx) + (dy * dy);
    if (r2 <= 1.0E-6) {
      p_undistortedFocalPlaneX = dx;
      p_undistortedFocalPlaneY = dy;
      return true;
    }

    bool converged = false;
    int i = 0;
    int maximumIterations = 15;
    double tolerance = p_camera->PixelPitch() / 100.0;
    double uxEstimate = dx;
    double uyEstimate = dy;
    double uxPrev = dx;
    double uyPrev = dy;
    double xDistortion = 0.0;
    double yDistortion = 0.0;
    double dr = 0.0;
    while (!converged) {
      dr = p_odk[0] + p_odk[1]*r2 + p_odk[2]*r2*r2;
      xDistortion = uxEstimate * dr;
      yDistortion = uyEstimate * dr;
      uxEstimate = dx - xDistortion;
      uyEstimate = dy - yDistortion;
      i++;
      if (fabs(uxEstimate - uxPrev) < tolerance &&
          fabs(uyEstimate - uyPrev) < tolerance ) {
        converged = true;
      }
      // If doesn't converge, don't do correction
      if (i > maximumIterations) {
        p_undistortedFocalPlaneX = dx;
        p_undistortedFocalPlaneY = dy;
        break;
      }
      r2 = (uxEstimate * uxEstimate) + (uyEstimate * uyEstimate);
      uxPrev = uxEstimate;
      uyPrev = uyEstimate;
    }
    p_undistortedFocalPlaneX = uxEstimate;
    p_undistortedFocalPlaneY = uyEstimate;
    return true;
  }

}
