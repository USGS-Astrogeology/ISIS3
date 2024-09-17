/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>
#include "IString.h"
#include "TaylorCameraDistortionMap.h"

using namespace std;
namespace Isis {

  /**
   *  @brief Taylor series camera distortion map constructor.
   *
   * Create a geometric distortion model which is approximated by a third-order
   * Taylor series expansion. This class maps between distorted and undistorted
   * focal plane x/y's. The default mapping is the identity, that is, the focal
   * plane x/y and undistorted focal plane x/y will be identical.
   *
   * @param parent        the parent camera that will use this distortion map
   * @param zDirection    the direction of the focal plane Z-axis
   *                      (either 1 or -1)
   *
   */
  TaylorCameraDistortionMap::TaylorCameraDistortionMap(Camera *parent, double zDirection):
    CameraDistortionMap(parent, zDirection) {
  }

  /**
   *  @brief Load distortion coefficients.
   *
   * This method loads the distortion coefficients from the instrument
   * kernel.  The coefficients in the NAIF instrument kernel are
   * expected to be in the form of:
   *
   * @code
   * INSxxxxx_OD_T_X = ( xa, xb, xc, xd, xe, xf, xg, xh, xi, xj )
   * INSxxxxx_OD_T_Y = ( ya, yb, yc, yd, ye, yf, yg, yh, yi, yj )
   *
   * where xxxxx is the instrument code (always a negative number)
   * @endcode
   *
   * These coefficient will be used to convert between focal plane
   * xp,yp to undistorted x,y as follows
   *
   *  [ xp ] = [ xa xb xc xd xe xf xg xh xi xj ] * [ 1     ]
   *  [ yp ]   [ ya yb yc yd ye yf yg yh yi yj ]   [ x     ]
   *                                               [ y     ]
   *                                               [ x*x   ]
   *                                               [ x*y   ]
   *                                               [ y*y   ]
   *                                               [ x*x*x ]
   *                                               [ x*x*y ]
   *                                               [ x*y*y ]
   *                                               [ y*y*y ]
   *
   * @param naifIkCode    Code to search for in instrument kernel
   */
  void TaylorCameraDistortionMap::SetDistortion(const int naifIkCode) {
    QString odtxkey = "INS" + QString::number(naifIkCode) + "_OD_T_X";
    QString odtykey = "INS" + QString::number(naifIkCode) + "_OD_T_Y";
    for(int i = 0; i < 10; ++i) {
      p_odtx.push_back(p_camera->getDouble(odtxkey, i));
      p_odty.push_back(p_camera->getDouble(odtykey, i));
    }
  }

  /**
   *  @brief Compute undistorted focal plane x/y.
   *
   * Compute undistorted focal plane x/y given a distorted focal plane x/y.
   * The undistorted coordinates are solved for using the Newton-Raphson
   * method for root-finding if the SetDistortion method is invoked.
   * After calling this method, you can obtain the undistorted x/y via
   * the UndistortedFocalPlaneX and UndistortedFocalPlaneY methods.
   *
   * @param dx distorted focal plane x in millimeters
   * @param dy distorted focal plane y in millimeters
   *
   * @return if the conversion was successful
   * @see SetDistortion
   * @todo Review the tolerance and maximum iterations of the root-
   *       finding algorithm.
   * @todo Review the handling of non-convergence of the root-finding
   *       algorithm.
   * @todo Add error handling for near-zero determinant.
   */
  bool TaylorCameraDistortionMap::SetFocalPlane(const double dx, const double dy) {
    p_focalPlaneX = dx;
    p_focalPlaneY = dy;

    // No coefficients == no distortion
    if(p_odtx.size() <= 0 && p_odty.size() <= 0) {
      p_undistortedFocalPlaneX = dx;
      p_undistortedFocalPlaneY = dy;
      return true;
    }

    // Solve the distortion equation using the Newton-Raphson method.
    // Set the error tolerance to about one millionth of a NAC pixel.
    const double tol = 1.4E-5;

    // The maximum number of iterations of the Newton-Raphson method.
    const int maxTries = 20;

    double x;
    double y;
    double fx;
    double fy;
    double Jxx;
    double Jxy;
    double Jyx;
    double Jyy;

    // Initial guess at the root
    x = dx;
    y = dy;

    this->DistortionFunction(x, y, &fx, &fy);

    for(int count = 1; ((fabs(fx) + fabs(fy)) > tol) && (count < maxTries); count++) {

      this->DistortionFunction(x, y, &fx, &fy);

      fx = dx - fx;
      fy = dy - fy;

      this->DistortionFunctionJacobian(x, y, &Jxx, &Jxy, &Jyx, &Jyy);

      double determinant = Jxx * Jyy - Jxy * Jyx;
      if(determinant < 1E-6) {
        //
        // Near-zero determinant. Add error handling here.
        //
        //-- Just break out and return with no convergence
        break;
      }

      x = x + (Jyy * fx - Jxy * fy) / determinant;
      y = y + (Jxx * fy - Jyx * fx) / determinant;
    }

    if((fabs(fx) + fabs(fy)) <= tol) {
      // The method converged to a root.
      p_undistortedFocalPlaneX = x;
      p_undistortedFocalPlaneY = y;
    }
    else {
      // The method did not converge to a root within the maximum
      // number of iterations. Return with no distortion.
      p_undistortedFocalPlaneX = dx;
      p_undistortedFocalPlaneY = dy;
    }

    return true;
  }

  /**
   *  @brief Compute distorted focal plane x/y.
   *
   * Compute distorted focal plane x/y given an undistorted focal plane x/y.
   * The distortion model is approximated by a third order Taylor series
   * expansion of a generic function if the SetDistortion method was invoked.
   * After calling this method, you can obtain the distorted x/y via the
   * FocalPlaneX and FocalPlaneY methods
   *
   * @param ux undistorted focal plane x in millimeters
   * @param uy undistorted focal plane y in millimeters
   *
   * @return if the conversion was successful
   * @see SetDistortion
   */
  bool TaylorCameraDistortionMap::SetUndistortedFocalPlane(const double ux,
      const double uy) {
    p_undistortedFocalPlaneX = ux;
    p_undistortedFocalPlaneY = uy;

    // No coefficients == nodistortion
    if(p_odtx.size() <= 0 && p_odty.size() <= 0) {
      p_focalPlaneX = ux;
      p_focalPlaneY = uy;
      return true;
    }

    this->DistortionFunction(ux, uy, &p_focalPlaneX, &p_focalPlaneY);

    return true;
  }

  /**
   * Compute distorted focal plane dx,dy given an undistorted focal plane ux,uy.
   * This describes the third order Taylor approximation to the distortion model.
   *
   * @param ux Undistored x
   * @param uy Undistored y
   * @param dx Result distorted x
   * @param dy Result distorted y
   */
  void TaylorCameraDistortionMap::DistortionFunction(double ux, double uy, double *dx, double *dy) {

    double f[10];
    f[0] = 1;
    f[1] = ux;
    f[2] = uy;
    f[3] = ux * ux;
    f[4] = ux * uy;
    f[5] = uy * uy;
    f[6] = ux * ux * ux;
    f[7] = ux * ux * uy;
    f[8] = ux * uy * uy;
    f[9] = uy * uy * uy;

    *dx = 0.0;
    *dy = 0.0;

    for(int i = 0; i < 10; i++) {
      *dx = *dx + f[i] * p_odtx[i];
      *dy = *dy + f[i] * p_odty[i];
    }

  }

  /**
   * Jacobian of the distortion function. The Jacobian was computed
   * algebraically from the function described in the DistortionFunction
   * method.
   *
   * @param x
   * @param y
   * @param Jxx
   * @param Jxy
   * @param Jyx
   * @param Jyy
   */
  void TaylorCameraDistortionMap::DistortionFunctionJacobian(double x, double y, double *Jxx, double *Jxy, double *Jyx, double *Jyy) {

    double d_dx[10];
    d_dx[0] = 0;
    d_dx[1] = 1;
    d_dx[2] = 0;
    d_dx[3] = 2 * x;
    d_dx[4] = y;
    d_dx[5] = 0;
    d_dx[6] = 3 * x * x;
    d_dx[7] = 2 * x * y;
    d_dx[8] = y * y;
    d_dx[9] = 0;
    double d_dy[10];
    d_dy[0] = 0;
    d_dy[1] = 0;
    d_dy[2] = 1;
    d_dy[3] = 0;
    d_dy[4] = x;
    d_dy[5] = 2 * y;
    d_dy[6] = 0;
    d_dy[7] = x * x;
    d_dy[8] = 2 * x * y;
    d_dy[9] = 3 * y * y;

    *Jxx = 0.0;
    *Jxy = 0.0;
    *Jyx = 0.0;
    *Jyy = 0.0;

    for(int i = 0; i < 10; i++) {
      *Jxx = *Jxx + d_dx[i] * p_odtx[i];
      *Jxy = *Jxy + d_dy[i] * p_odtx[i];
      *Jyx = *Jyx + d_dx[i] * p_odty[i];
      *Jyy = *Jyy + d_dy[i] * p_odty[i];
    }
  }

}
/**
 * Please direct questions to
 * Lillian Nguyen, JHUAPL, (443)778-5477, Lillian.Nguyen@jhuapl.edu
 */
