/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>

#include "IString.h"
#include "MarciDistortionMap.h"

namespace Isis {
  /** Camera distortion map constructor
   *
   * Create a camera distortion map.  This class maps between distorted
   * and undistorted focal plane x/y's.  The default mapping is the
   * identity, that is, the focal plane x/y and undistorted focal plane
   * x/y will be identical.
   *
   * @param parent        the parent camera that will use this distortion map
   * @param zDirection    the direction of the focal plane Z-axis
   *                      (either 1 or -1)
   *
   */
  MarciDistortionMap::MarciDistortionMap(Camera *parent, int naifIkCode) : CameraDistortionMap(parent) {
    QString odkkey = "INS" + QString::number(naifIkCode) + "_DISTORTION_COEFFS";

    for(int i = 0; i < 4; i++) {
      p_odk.push_back(p_camera->getDouble(odkkey, i));
    }
  }

  /** Compute undistorted focal plane x/y
   *
   * Compute undistorted focal plane x/y given a distorted focal plane x/y.
   *
   * @param dx distorted focal plane x in millimeters
   * @param dy distorted focal plane y in millimeters
   *
   * @return if the conversion was successful
   * @see SetDistortion
   * @todo Generalize polynomial equation
   */
  bool MarciDistortionMap::SetFocalPlane(const double dx, const double dy) {
    p_focalPlaneX = dx;
    p_focalPlaneY = dy;

    double dxPix = p_focalPlaneX / p_camera->PixelPitch();
    double dyPix = p_focalPlaneY / p_camera->PixelPitch();

    // Get the distance from the focal plane center and if we are close
    // then skip the distortion
    double radialDist2 = (dxPix * dxPix) + (dyPix * dyPix);

    if(radialDist2 <= 1.0E-3) {
      p_undistortedFocalPlaneX = dx;
      p_undistortedFocalPlaneY = dy;
      return true;
    }

    // Ok we need to apply distortion correction
    double radialDist4 = radialDist2 * radialDist2;
    double radialDist6 = radialDist4 * radialDist2;

    double uRadialDist = p_odk[0] + radialDist2 * p_odk[1] +
                         radialDist4 * p_odk[2] +
                         radialDist6 * p_odk[3];

    // double radialDist = sqrt(radialDist2);
    double uxPix = dxPix * uRadialDist;
    double uyPix = dyPix * uRadialDist;


    p_undistortedFocalPlaneX = uxPix * p_camera->PixelPitch();
    p_undistortedFocalPlaneY = uyPix * p_camera->PixelPitch();

    return true;
  }

  /** Compute distorted focal plane x/y
   *
   * Compute distorted focal plane x/y given an undistorted focal plane x/y.
   *
   * @param ux undistorted focal plane x in millimeters
   * @param uy undistorted focal plane y in millimeters
   *
   * @return if the conversion was successful
   * @see SetDistortion
   * @todo Generalize polynomial equation
   * @todo Figure out a better solution for divergence condition
   */
  bool MarciDistortionMap::SetUndistortedFocalPlane(const double ux,
      const double uy) {
    p_undistortedFocalPlaneX = ux;
    p_undistortedFocalPlaneY = uy;

    double uxPix = ux / p_camera->PixelPitch();
    double uyPix = uy / p_camera->PixelPitch();

    double dxPix = GuessDx(uxPix);
    double dyPix = uyPix;

    // Get the distance from the focal plane center and if we are close
    // then skip the distortion
    double Ru = sqrt((uxPix * uxPix) + (uyPix * uyPix));

    if(Ru <= 1.0E-6) {
      p_focalPlaneX = ux;
      p_focalPlaneY = uy;
      return true;
    }

    double delta = 1.0;
    int iter = 0;

    double Rd = sqrt((dxPix * dxPix) + (dyPix * dyPix));

    while(fabs(delta) > 1E-9) {
      if(fabs(delta) > 1E30 || iter > 50) {
        return false;
      }

      double Rd2 = Rd * Rd;
      double Rd3 = Rd2 * Rd;
      double Rd4 = Rd3 * Rd;
      double Rd5 = Rd4 * Rd;
      double Rd6 = Rd5 * Rd;

      double fRd = p_odk[0] + Rd2 * p_odk[1] +
                   Rd4 * p_odk[2] +
                   Rd6 * p_odk[3] - Ru * (1.0 / Rd);

      double fRd2 = 2 * p_odk[1] * Rd +
                    4 * p_odk[2] * Rd3 +
                    6 * p_odk[3] * Rd5 +
                    Ru * (1.0 / Rd2);

      delta = fRd / fRd2;

      Rd = Rd - delta;

      iter ++;
    }

    dxPix = uxPix * (Rd / Ru);
    dyPix = uyPix * (Rd / Ru);

    p_focalPlaneX = dxPix * p_camera->PixelPitch();
    p_focalPlaneY = dyPix * p_camera->PixelPitch();

    return true;
  }

  double MarciDistortionMap::GuessDx(double uX) {
    // We're using natural log fits, but if uX < 1 the fit doesnt work
    if(fabs(uX) < 1) return uX;

    if(p_filter == 0) { // BLUE FILTER
      return (1.4101 * log(fabs(uX)));
    }

    else if(p_filter == 1) { // GREEN FILTER
      return (1.1039 * log(fabs(uX)));
    }

    else if(p_filter == 2) { // ORANGE FILTER
      return (0.8963 * log(fabs(uX)) + 2.1644);
    }

    else if(p_filter == 3) { // RED FILTER
      return (1.1039 * log(fabs(uX)));
    }

    else if(p_filter == 4) { // NIR FILTER
      return (1.4101 * log(fabs(uX)));
    }

    return uX;
  }
}
