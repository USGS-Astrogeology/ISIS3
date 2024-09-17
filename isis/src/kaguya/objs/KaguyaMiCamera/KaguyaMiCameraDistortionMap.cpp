/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>
#include <iostream>

#include "IString.h"
#include "Constants.h"
#include "FunctionTools.h"
#include "KaguyaMiCameraDistortionMap.h"

using namespace std;
using namespace Isis;

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
  KaguyaMiCameraDistortionMap::KaguyaMiCameraDistortionMap(Camera *parent) : CameraDistortionMap(parent, 1) {
  }

  /**
   * @param naifIkCode
   */
  void KaguyaMiCameraDistortionMap::SetDistortion(const int naifIkCode) {
    //determine if this is the VIS or the NIR sensor, by loooking at the pixel pitch
    if (p_camera->PixelPitch() == 0.013) m_numDistCoef = 3;  //VIS camera has 3 distortion coefs
    else                       m_numDistCoef = 4;  //NIR camera has 4 distortion coefs

    //read the distortion coefs from the NAIF Kernels
    QString naifXKey = "INS" + QString::number(naifIkCode) + "_DISTORTION_COEF_X";
    QString naifYKey = "INS" + QString::number(naifIkCode) + "_DISTORTION_COEF_Y";
    for (int i=0; i < m_numDistCoef; i++) {
      m_distCoefX[i] = p_camera->getDouble(naifXKey,i);
      m_distCoefY[i] = p_camera->getDouble(naifYKey,i);
    }

    //now read the boresights, or what I would typicall call the principal point offsets
    naifXKey = "INS" + QString::number(naifIkCode) + "_BORESIGHT";
    m_boreX = p_camera->getDouble(naifXKey, 0);
    m_boreY = p_camera->getDouble(naifXKey, 1);
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
   */
  bool KaguyaMiCameraDistortionMap::SetFocalPlane(const double dx, const double dy) {
    p_focalPlaneX = dx;
    p_focalPlaneY = dy;
    //cout << "focal plane: " << dx << " " << dy << "\n";
    //NOTE: the IK/FK kernel does not include the " + dx" as I do below.  They also define the
    // radial distance only in terms of Y.  Erroneously (I believe) they use only the
    // DISTORTION_COEF_X's in their model definition.  Finally, they provide different distortion
    // coefficients for each line of the CCD--despite them going throught the same optical path.
    // From this I conclude that this distorion model is only valid if x is very near zero.  Which
    // is exactly the situation we are shooting for when modeling a line scanner (x is the along
    // path direction for this sensor).  However, we can not just arbitarily zero, or almost zero,
    // any along path offset calculated by the back projections.  Those offsets are exactly the cost
    // being zeroed in the iterative LineScanCameraGroundMap routines to find the time that a point
    // on the ground was imaged.  Therefore it must be maintained--with the knowledge that the
    // small adjustmenst being provided by the distortion model are only relevant as the offsets (x)
    // approach zero.
    if (m_numDistCoef == 3) {  //VIS camera
      p_undistortedFocalPlaneX =
        m_boreX + m_distCoefX[0] + dy*(m_distCoefX[1] + dy*m_distCoefX[2]) + dx;
      p_undistortedFocalPlaneY =
        m_boreY + m_distCoefY[0] + dy*(m_distCoefY[1] + dy*m_distCoefY[2]) + dy;
    }
    else {  //NIR camera
      p_undistortedFocalPlaneX =  m_boreX +
        m_distCoefX[0] + dy*(m_distCoefX[1] + dy*(m_distCoefX[2] + dy*m_distCoefX[3])) + dx;
      p_undistortedFocalPlaneY =  m_boreY +
        m_distCoefY[0] + dy*(m_distCoefY[1] + dy*(m_distCoefY[2] + dy*m_distCoefY[3])) + dy;
    }
    //cout << "undistorted: " << p_undistortedFocalPlaneX << " " << p_undistortedFocalPlaneY << "\n";
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
   */
  bool KaguyaMiCameraDistortionMap::SetUndistortedFocalPlane(const double ux, const double uy) {\
    // image coordinates prior to introducing distortion
    p_undistortedFocalPlaneX = ux;
    p_undistortedFocalPlaneY = uy;

    //std::cout << "Distortionless : " << p_undistortedFocalPlaneX << " " << p_undistortedFocalPlaneY << "\n";

    if (m_numDistCoef == 3) {  //quadratic distortion model
      //use the quadratic equation to find the distorted Y value
      double A,B,C;  //coefficients of the quadric equation

      A = m_distCoefY[2];
      B = 1.0 + m_distCoefY[1];
      C = m_distCoefY[0] +m_boreY - uy;

      QList<double> roots = FunctionTools::realQuadraticRoots(A,B,C);

      if (roots.size() == 0) return false;
      else if (roots.size() == 1)
        p_focalPlaneY = roots[0];
      else  //two roots to choose between--choose the one closest to uy
        p_focalPlaneY = fabs(uy - roots[0]) < fabs(uy - roots[1]) ? roots[0] : roots[1];

      //now that we know the distortedY we can directly calculate the X
      p_focalPlaneX = ux -
        (m_boreX + m_distCoefX[0] + p_focalPlaneY*(m_distCoefX[1] + p_focalPlaneY*m_distCoefX[2]));

      return true;
    }
    else { //cubic distortion model
      //use the cubic equation to find the distorted Y value
      double a,b,c; //coefficients of the cubic (coef x^3 ==1)

      a = m_distCoefY[2] / m_distCoefY[3];
      b = (1.0 + m_distCoefY[1]) / m_distCoefY[3];
      c = (m_distCoefY[0] + m_boreY - uy) / m_distCoefY[3];

      QList<double> roots = FunctionTools::realCubicRoots(1.0,a,b,c);

      if (roots.size() == 1) { //one root
        p_focalPlaneY = roots[0];
      }
      else {  //pick the root closest to the orginal value
        QList<double> delta;

        //store all the distance from undistored to focal plane
        for (int i=0;i<roots.size();i++)
          delta << fabs(roots[i]-uy);

        if ( roots.size() == 3) //three roots to choose among
          p_focalPlaneY = delta[0] < delta[1] ?
                         (delta[0] < delta[2] ? roots[0]:roots[2]) :
                         (delta[1] < delta[2] ? roots[1]:roots[2]) ;
        else  //two roots to choose between
          p_focalPlaneY = delta[0] < delta[1] ? roots[0]:roots[1]  ;
      }

      //now that we know the distortedY we can directly calculate the X
      p_focalPlaneX = ux - (m_boreX +m_distCoefX[0] +
                      p_focalPlaneY*(m_distCoefX[1] +
                      p_focalPlaneY*(m_distCoefX[2] +
                      p_focalPlaneY* m_distCoefX[3])));
      return true;
    }
  }
}
