/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "LoHighDistortionMap.h"
#include "IString.h"

using namespace std;

namespace Isis {
  /** Constructor for LunarOrbiterHighDistortionMap class
   *
   * Define the distortion model coefficients for a Lunar Orbiter
   * High Resolution camera.
   *
   * @param parent    A pointer to the parent camera object
   *
   * @internal
   *
   * @history 2007-06-29 Debbie A. Cook Original version
   *
   */
  LoHighDistortionMap::LoHighDistortionMap(Camera *parent) :
    CameraDistortionMap(parent, -1) {
  }


  /** Load LO High Resolution Camera perspective & distortion coefficients
   *
   * This method loads the perspective correction factors, distortion centers,
   * and coefficients from the instrument kernel.  The perspective correction
   * factors in the NAIF instrument kernel are expected to be in the form of:
   *
   * @code
   * INSxxxxxxx_PERSPECTIVE_FACTORS = ( xpers, ypers)
   *
   * where xxxxxxx is the instrument code (always a negative number) and
   * xpers and ypers are the X and Y perspective correction factors.
   * @endcode
   *
   * These factors will be used to convert from focal plane x,y to
   * perspective-corrected x,y as follows.
   *
   *   pcx = FocalPlaneX * ( 1. + xpers*FocalPlaneX + ypers*FocalPlaneY);
   *   pcy = FocalPlaneY * ( 1. + xpers*FocalPlaneX + ypers*FocalPlaneY);
   *
   * The distortion center coordinates (in mm) are expected to be in the form
   * of:
   *
   * @code
   * INSxxxxxxx_DISTORTION_CENTER = ( xcenter, ycenter)
   *
   * where xxxxxxx as is described above for the perspective factors.
   * @endcode
   *
   * The center will be used to calculate the radius of distortion in the
   * equations below.
   *
   *    distX = PersCorrectedX - x0
   *    distY = PersCorrectedY - y0
   *
   * The distortion coefficients in the NAIF instrument
   * kernel are expected to be in the form of:
   *
   * @code
   * INSxxxxxxx_OD_K = ( coef1, coef2, ..., coefN)
   *
   * where xxxxxxx is as is described about for the perspective factors.
   * @endcode
   *
   * These coefficient will be used to convert from focal plane x,y to
   * to undistorted x,y as follows (add equation here)
   *
   *  r^2 = DistX^2 + DistY^2
   *  dr/r = k0 + k1*r^2
   *  ux = PersCorrectedX - DistX*dr/r, similarly for uy
   *
   * @param naifIkCode    Code to search for in instrument kernel
   * */
  void LoHighDistortionMap::SetDistortion(const int naifIkCode) {
    // Get the perspective correction factors for x and y and the distortion
    // center (point of symmetry of distortion)
    QString perskey = "INS" + QString::number(naifIkCode) + "_PERSPECTIVE_FACTORS";
    QString centkey = "INS" + QString::number(naifIkCode) + "_POINT_OF_SYMMETRY";
    p_xPerspective = p_camera->Spice::getDouble(perskey, 0);
    p_yPerspective = p_camera->Spice::getDouble(perskey, 1);
    p_x0 = p_camera->Spice::getDouble(centkey, 0);
    p_y0 = p_camera->Spice::getDouble(centkey, 1);

    // Get the distortion coefficients
    CameraDistortionMap::SetDistortion(naifIkCode);
  }


  /** Compute undistorted focal plane x/y for Lo High Resolution Camera
   *
   * Compute undistorted focal plane x/y given a distorted focal plane x/y
   * for the Lunar Orbiter High Resolution Camera. The polynomial used is
   * described in the SetDistortion documentation. After calling this method,
   * the  undistorted x/y can be obtained via the UndistortedFocalPlaneX and
   * UndistortedFocalPlaneY methods of the parent class.
   *
   * @param dx distorted focal plane x in millimeters
   * @param dy distorted focal plane y in millimeters
   *
   * @return if the conversion was successful
   * @see SetDistortion
   */
  bool LoHighDistortionMap::SetFocalPlane(const double dx,
                                          const double dy) {
    p_focalPlaneX = dx;
    p_focalPlaneY = dy;

    // Apply perspective correction factors to get perspective corrected x/y
    double perspectiveFactor = 1. + (p_xPerspective * dx) + (p_yPerspective * dy);
    double pcx = dx * perspectiveFactor;
    double pcy = dy * perspectiveFactor;

    // Translate the perspective-corrected x/y coordinate to be relative to the
    // distortion point of symmetry
    double distx  =  pcx - p_x0;
    double disty  =  pcy - p_y0;

    // Get the distance from the focal plane center and if we are close
    // skip the distortion
    double r2 = distx * distx + disty * disty;
    if(r2 <= 1.0E-6) {
      p_undistortedFocalPlaneX = pcx;
      p_undistortedFocalPlaneY = pcy;
      return true;
    }

    // Otherwise remove distortion
    double drOverR  =  p_odk[0] + p_odk[1] * r2;
    p_undistortedFocalPlaneX = pcx - (drOverR * distx);
    p_undistortedFocalPlaneY = pcy - (drOverR * disty);
    return true;
  }


  /** Compute distorted focal plane x/y for Lo High Resolution Camera
   *
   * Compute distorted focal plane x/y given an undistorted focal plane x/y for
   * the Lunar Orbiter High Resolution Camera.  This method applies both a
   * perspective error and a distortion error based on a polynomial defined
   * in the SetDistortion method.  After calling this method the distorted x/y
   * can be obtained via the FocalPlaneX and FocalPlaneY methods.
   *
   * @param ux undistorted focal plane x in millimeters
   * @param uy undistorted focal plane y in millimeters
   *
   * @return if the conversion was successful
   * @see SetDistortion
   *
   * @internal
   *   @history 2013-02-22 Debbie A. Cook - Removed signFactor since the z direction is now
   *                         handled in CameraGroundMap.
   */
  bool LoHighDistortionMap::SetUndistortedFocalPlane(const double ux,
      const double uy) {

    p_undistortedFocalPlaneX = ux;
    p_undistortedFocalPlaneY = uy;

    // Translate the distorted x/y coordinate to be relative to the
    // distortion point of symmetry
    double distux  =  p_undistortedFocalPlaneX - p_x0;
    double distuy  =  p_undistortedFocalPlaneY - p_y0;

    // Compute the distance from the focal plane center and if we are
    // close to the center then no distortion is required
    double rp2 = distux * distux + distuy * distuy;

    double pcx, pcy;

    if(rp2 > 1.0E-6) {

      // Add distortion.  First compute fractional distortion at rp (r-prime)
      double drOverR   =   p_odk[0]  +  rp2 * p_odk[1];

      // Compute the perspective corrected x/y
      pcx  =  p_undistortedFocalPlaneX + (distux * drOverR);
      pcy  =  p_undistortedFocalPlaneY + (distuy * drOverR);
    }
    else {
      pcx = p_undistortedFocalPlaneX;
      pcy = p_undistortedFocalPlaneY;
    }

    // Add the perspective error
    double perspectiveCorrection = 1. - (p_xPerspective * pcx) - (p_yPerspective * pcy);
    p_focalPlaneX = pcx * perspectiveCorrection;
    p_focalPlaneY = pcy * perspectiveCorrection;
    return true;
  }
}
