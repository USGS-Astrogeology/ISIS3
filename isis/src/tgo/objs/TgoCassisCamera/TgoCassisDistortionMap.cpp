/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>

#include <QDebug>

#include "IString.h"
#include "TgoCassisDistortionMap.h"

namespace Isis {
  /**
   * Exomars TGO CaSSIS distortion map constructor.
   *
   * Create a camera distortion map.  This class maps between distorted
   * and undistorted focal plane x/y's. The default mapping is the
   * identity, that is, the focal plane x/y and undistorted focal plane
   * x/y will be identical.
   *
   * @param parent        the parent camera that will use this distortion map
   * @param naifIkCode    NAIF IK code.
   *
   */
  TgoCassisDistortionMap::TgoCassisDistortionMap(Camera *parent,
                                                 int naifIkCode)
      : CameraDistortionMap(parent) {

    QString od = "INS" + QString::number(naifIkCode) + "_OD_";

    for(int i = 0; i < 6; i++) {
      m_A1_corr.push_back(p_camera->getDouble(od + "A1_CORR", i));
      m_A2_corr.push_back(p_camera->getDouble(od + "A2_CORR", i));
      m_A3_corr.push_back(p_camera->getDouble(od + "A3_CORR", i));
      m_A1_dist.push_back(p_camera->getDouble(od + "A1_DIST", i));
      m_A2_dist.push_back(p_camera->getDouble(od + "A2_DIST", i));
      m_A3_dist.push_back(p_camera->getDouble(od + "A3_DIST", i));
    }
    m_pixelPitch = p_camera->getDouble("INS" + QString::number(naifIkCode) + "_PIXEL_PITCH");
    m_width  = p_camera->getDouble("INS" + QString::number(naifIkCode) + "_FILTER_SAMPLES");
    m_height = p_camera->getDouble("INS" + QString::number(naifIkCode) + "_FILTER_LINES");
  }


  /**
   * Exomars TGO CaSSIS distortion map destructor.
   */
  TgoCassisDistortionMap::~TgoCassisDistortionMap() {
  }


  /**
   * Compute undistorted focal plane (x,y) coordinate given the distorted
   * (x,y).
   *
   * Model derived by Stepan Tulyakov and Anoton Ivanov, EPFL (Ecole
   * Polytechnique Federale de Lausanne).
   *
   * Given distorted focal plane coordinates, in millimeters, and parameters of
   * rational CORRECTION model A1_corr, A2_corr, A3_corr, this function returns undistorted
   * focal plane coordinates, in millimeters.
   *
   * The rational optical distortion correction model is described by following
   * equations:
   *
   *  chi = [ dx^2, dx*dy, dy^2, dx, dy, 1]
   *
   *          A1_corr * chi'
   *  x =    ---------------
   *          A3_corr * chi'
   *
   *          A2_corr * chi'
   *  y =    ----------------
   *          A3_corr * chi'
   *
   * @param dx distorted focal plane x, in millimeters
   * @param dy distorted focal plane y, in millimeters
   *
   * @return @b bool Indicates whether the conversion was successful.
   */
  bool TgoCassisDistortionMap::SetFocalPlane(const double dx,
                                             const double dy) {

    p_focalPlaneX = dx;
    p_focalPlaneY = dy;

    // calculate divisor using A3_corr coeffiecients
    double divider = chiDotA(dx, dy, m_A3_corr);
    // This distortion model is only valid for values on the CCD:
    // -1/2 * pixel pitch * CCD width  = -10.24 < x < 10.24 = 1/2 * pixel pitch * CCD width
    // -1/2 * pixel pitch * CCD height = -10.24 < y < 10.24 = 1/2 * pixel pitch * CCD height
    //
    // Also, the zeros for the divider variable fall well outside the boundary
    // of the CCD. (See $ISISDATA/tgo/assets/distortion/DistortionModelA3CorrRoots.jpg).
    //
    // So, whenever x or y are too far from center or divider is near zero,
    // return the given inputs

    if ( dx < -0.5*m_pixelPitch*m_width  - 0.2 ||
         dx >  0.5*m_pixelPitch*m_width  + 0.2 ||
         dy < -0.5*m_pixelPitch*m_height - 0.2 ||
         dy >  0.5*m_pixelPitch*m_height + 0.2 ) {
      p_undistortedFocalPlaneX = dx;
      p_undistortedFocalPlaneY = dy;

      return true;
    }

    // get undistorted ideal (x,y) coordinates
    double ux = chiDotA(dx, dy, m_A1_corr) / divider;
    double uy = chiDotA(dx, dy, m_A2_corr) / divider;

    p_undistortedFocalPlaneX = ux;
    p_undistortedFocalPlaneY = uy;

    return true;
  }


  /**
   * Compute distorted focal plane (x,y) given an undistorted focal plane (x,y).
   *
   * Model derived by Stepan Tulyakov and Anoton Ivanov, EPFL (Ecole
   * Polytechnique Federale de Lausanne).
   *
   * Given ideal focal plane coordinates, in millimeters, and parameters
   * of rational CORRECTION model A1_dist, A2_dist, A3_dist, this function
   * returns distorted focal plane coordinates, in millimeters.
   *
   * The rational optical distortion correction model is described by following
   * equations:
   *
   *  chi = [ dx^2, dx*dy, dy^2, dx, dy, 1]
   *
   *          A1_dist * chi'
   *  x =    ---------------
   *          A3_dist * chi'
   *
   *          A2_dist * chi'
   *  y =    ----------------
   *          A3_dist * chi'
   *
   *
   * @param ux undistorted focal plane x, in millimeters
   * @param uy undistorted focal plane y, in millimeters
   *
   * @return @b bool Indicates whether the conversion was successful
   */
  bool TgoCassisDistortionMap::SetUndistortedFocalPlane(const double ux,
                                                        const double uy) {
    p_undistortedFocalPlaneX = ux;
    p_undistortedFocalPlaneY = uy;

    // calculate divisor using A3_dist coeffiecients
    double divider = chiDotA(ux, uy, m_A3_dist);
    // This distortion model is only valid for values on the CCD:
    // -1/2 * pixel pitch * CCD width  = -10.24 < x < 10.24 = 1/2 * pixel pitch * CCD width
    // -1/2 * pixel pitch * CCD height = -10.24 < y < 10.24 = 1/2 * pixel pitch * CCD height
    //
    // Also, the zeros for the divider variable fall well outside the boundary
    // of the CCD. (See $ISISDATA/tgo/assets/distortion/DistortionModelA3DistRoots.jpg).
    //
    // So, whenever x or y are too far from center or divider is near zero,
    // return the given inputs

    if ( ux < -0.5*m_pixelPitch*m_width  - 0.2 ||
         ux >  0.5*m_pixelPitch*m_width  + 0.2 ||
         uy < -0.5*m_pixelPitch*m_height - 0.2 ||
         uy >  0.5*m_pixelPitch*m_height + 0.2 ) {
      p_focalPlaneX = ux;
      p_focalPlaneY = uy;

      return true;
    }

    // get undistorted ideal (x,y) coordinates
    double dx = chiDotA(ux, uy, m_A1_dist) / divider;
    double dy = chiDotA(ux, uy, m_A2_dist) / divider;

    p_focalPlaneX = dx;
    p_focalPlaneY = dy;
    return true;
  }


  /**
   * Evaluate the value for the multi-variate polynomial, given the list of
   * 6 coefficients.
   *
   * We define
   *  @f[ chi = [x^2, xy, y^2, x, y, 1 @f]
   * and
   *  @f[ A = [A_0, A_1, A_2, A_3, A_4, A_5 @f]
   *
   * And we return
   *  @f[ \chi \cdot A = c_0 x^2 + c_1 xy + c_2 y^2 + c_3 x + c_4 y + c_5 @f]
   *
   * @param x The input x value.
   * @param y The input y value.
   * @param A The list of coeffients.
   *
   * @return @b double The value of chi dot A.
   */
  double TgoCassisDistortionMap::chiDotA(double x,
                                         double y,
                                         QList<double> A) {
    double x2 = x*x;
    double y2 = y*y;
    double xy = x*y;

    return A[0] * x2
         + A[1] * xy
         + A[2] * y2
         + A[3] * x
         + A[4] * y
         + A[5];
  }
}
