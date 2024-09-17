/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QDebug>
#include <QtGlobal>
#include <QtMath>

#include "KaguyaTcCameraDistortionMap.h"

namespace Isis {
  /**
   * Kaguya TC Camera distortion map constructor
   *
   * Create a camera distortion map for Kaguya's TC1 and TC2
   * This class maps between distorted and undistorted
   * focal plane x/y's. The default mapping is the identity, that is,
   * the focal plane x/y and undistorted focal plane x/y will be
   * identical.
   *
   * @param parent        the parent camera that will use this distortion map
   * @param zDirection    the direction of the focal plane Z-axis
   *                      (either 1 or -1)
   *
   */
  KaguyaTcCameraDistortionMap::KaguyaTcCameraDistortionMap(Camera *parent, int naifIkCode)
      : CameraDistortionMap(parent) {
    QString odtxkey = "INS" + QString::number(naifIkCode) + "_DISTORTION_COEF_X";
    QString odtykey = "INS" + QString::number(naifIkCode) + "_DISTORTION_COEF_Y";
    QString boresightkey = "INS" + QString::number(naifIkCode) + "_BORESIGHT";

    for(int i = 0; i < 4; ++i) {
      p_odkx.push_back(p_camera->getDouble(odtxkey, i));
      p_odky.push_back(p_camera->getDouble(odtykey, i));
    }

    // add boresight x and y to coefficients vector
    p_odkx[0] = p_odkx[0] + p_camera->getDouble(boresightkey, 0);
    p_odky[0] = p_odky[0] + p_camera->getDouble(boresightkey, 1);
  }



  /**
   * Destructor
   */
  KaguyaTcCameraDistortionMap::~KaguyaTcCameraDistortionMap() {
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
   * This implements the following distortion correction from the IK for the terrain camera,
   * see: SEL_TC_V01.TI
   *
   * r2 = x^2 + y^2
   *
   * Distortion coefficients information:
   *  INS<INSTID>_DISTORTION_COEF_X  = ( a0, a1, a2, a3)
   *  INS<INSTID>_DISTORTION_COEF_Y  = ( b0, b1, b2, b3),
   *
   * Distance r from the center:
   *   r = - (n - INS<INSTID>_CENTER) * INS<INSTID>_PIXEL_SIZE.
   *
   * Line-of-sight vector v is calculated as
   *  v[X] = INS<INSTID>BORESIGHT[X]
   *         +a0 +a1*r +a2*r^2 +a3*r^3 ,
   *  v[Y] = INS<INSTID>BORESIGHT[Y]
   *        +r +a0 +a1*r +a2*r^2 +a3*r^3 ,
   *  v[Z] = INS<INSTID>BORESIGHT[Z] .
   *
   * @param dx distorted focal plane x in millimeters
   * @param dy distorted focal plane y in millimeters
   *
   * @return if the conversion was successful
   * @see SetDistortion
   */
  bool KaguyaTcCameraDistortionMap::SetFocalPlane(double dx, double dy) {
    p_focalPlaneX = dx;
    p_focalPlaneY = dy;

    double x = dx;
    double y = dy;

    double r2 = x*x + y*y;
    double r = qSqrt(r2);
    double r3 = r2 * r;

    double dr_x = p_odkx[0] + p_odkx[1] * r + p_odkx[2] * r2 + p_odkx[3] * r3; //add boresight offset
    double dr_y = p_odky[0] + p_odky[1] * r + p_odky[2] * r2 + p_odky[3] * r3; //add boresight offset

    p_undistortedFocalPlaneX = x + dr_x;
    p_undistortedFocalPlaneY = y + dr_y;

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
  bool KaguyaTcCameraDistortionMap::SetUndistortedFocalPlane(const double ux,
      const double uy) {

    p_undistortedFocalPlaneX = ux;
    p_undistortedFocalPlaneY = uy;

    double xt = ux;
    double yt = uy;

    double xx, yy, r, rr, rrr, dr_x, dr_y;
    double xdistortion, ydistortion;
    double xdistorted, ydistorted;
    double xprevious, yprevious;

    xprevious = 1000000.0;
    yprevious = 1000000.0;

    double tolerance = 0.000001;
    bool bConverged = false;

    // Iterating to introduce distortion...
    // We stop when the difference between distorted coordinates
    // in successive iterations is below the given tolerance
    for (int i = 0; i < 50; i++) {
      xx = xt * xt;
      yy = yt * yt;
      rr = xx + yy;
      r = qSqrt(rr);
      rrr = rr * r;

      // Radial distortion
      // dr is the radial distortion contribution
      dr_x = p_odkx[0] + p_odkx[1] * r + p_odkx[2] * rr + p_odkx[3] * rrr; // why did hayabusa have a -1
      dr_y = p_odky[0] + p_odky[1] * r + p_odky[2] * rr + p_odky[3] * rrr; // why did hayabusa have a -1

      // Distortion at the current point location
      xdistortion = dr_x;
      ydistortion = dr_y;

      // updated image coordinates
      xt = ux - xdistortion;
      yt = uy - ydistortion;

      // distorted point corrected for principal point
      xdistorted = xt;
      ydistorted = yt;

      // check for convergence
      if ((fabs(xt - xprevious) < tolerance) && (fabs(yt - yprevious) < tolerance)) {
        bConverged = true;
        break;
      }

      xprevious = xt;
      yprevious = yt;
    }

    if (bConverged) {
      p_focalPlaneX = xdistorted;
      p_focalPlaneY = ydistorted;
    }

    return bConverged;
  }
}
