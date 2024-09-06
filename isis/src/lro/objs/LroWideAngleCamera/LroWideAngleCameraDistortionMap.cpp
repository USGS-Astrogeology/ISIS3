/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>

#include <sstream>
#include <iomanip>
#include <boost/foreach.hpp>

#include "IString.h"
#include "LroWideAngleCameraDistortionMap.h"

using namespace std;

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
  LroWideAngleCameraDistortionMap::LroWideAngleCameraDistortionMap(Camera *parent,
                                                                   int naifIkCode) :
                                                                   CameraDistortionMap(parent) {
    // Initialize distortion coefficients to 0 to avoid segfaults
    QString odkkey = "INS" + toString(naifIkCode) + "_OD_K";
    for (int i = 0; i < 3; ++i) {
      p_odk.push_back(0);
    }
  }


/**
 * @brief Add an additional set of parameters for a given LROC/WAC filter
 *
 * This method will read the parameters for LROC/WAC filter as indicated by the
 * IK code provided. It will create a vector of these parameters and append them
 * to the band list.
 *
 * The filters added should correspond directly to the order in which the
 * filters are physically stored in the ISIS cube (or the virtually selected
 * bands).
 *
 * @author 2013-03-07 Kris Becker
 *
 * @param naifIkCode NAIF IK code for the desired filter to add.
 */
  void LroWideAngleCameraDistortionMap::addFilter(int naifIkCode) {
    QString odkkey = "INS" + QString::number(naifIkCode) + "_OD_K";

    std::vector<double> v_odk;
    for(int i = 0; i < 3; i++) {
      v_odk.push_back(p_camera->getDouble(odkkey, i));
    }

    m_odkFilters.push_back(v_odk);
  }


/**
 * @brief Implements band-dependant distortion parameters
 *
 * This method should be used to switch to another band's set of distortion
 * parameters.  See the addFilter() method to add additional band parameters to
 * this object. Note that the band number should correspond with the same order
 * as they were added in the addFilter() method.
 *
 * @author 2013-03-07 Kris Becker
 *
 * @param vband  Band number to select.  Range is 1 to Bands.
 */
  void LroWideAngleCameraDistortionMap::setBand(int vband) {
    if ( (vband <= 0) || (vband > m_odkFilters.size()) ) {
      std::string mess = "Invalid band (" + QString::number(vband) + " requested " +
                     " Must be <= " + QString::number(m_odkFilters.size());
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }

    //  Install new parameters
    p_odk = m_odkFilters[vband-1];

    return;

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

  bool LroWideAngleCameraDistortionMap::SetFocalPlane(const double dx, const double dy) {
    p_focalPlaneX = dx;
    p_focalPlaneY = dy;

    double dk1 = p_odk[0];
    double dk2 = p_odk[1];
    double dk3 = p_odk[2];

    double rr = dx * dx + dy * dy;

    double dr = 1.0 + dk1 * rr + dk2 * rr * rr + dk3 * rr * rr * rr;

    // This was in here when the model used to be dx/dr so that the model
    // would never divide by zero.
    if ( dr == 0.0 ) return false;

    // Compute the undistorted positions
    p_undistortedFocalPlaneX = dx * dr;
    p_undistortedFocalPlaneY = dy * dr;

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

  bool LroWideAngleCameraDistortionMap::SetUndistortedFocalPlane(const double ux, const double uy) {
    // image coordinates prior to introducing distortion
    p_undistortedFocalPlaneX = ux;
    p_undistortedFocalPlaneY = uy;

    double xt = ux;
    double yt = uy;

    double rr, dr;
    double xdistorted, ydistorted;
    double xprevious, yprevious;

    xprevious = 1000000.0;
    yprevious = 1000000.0;
    // Changed this line to 10^-6... it allows the outer pixels to be found
    // when mapping back to the sensor
    double tolerance = 1.0e-6;

    bool bConverged = false;

    double dk1 = p_odk[0];
    double dk2 = p_odk[1];
    double dk3 = p_odk[2];

    // iterating to introduce distortion...
    // we stop when the difference between distorted coordinates
    // in successive iterations is at or below the given tolerance
    for(int i = 0; i < 50; i++) {
      rr = xt * xt + yt * yt;
      // dr is the radial distortion contribution
      dr = 1.0 + dk1 * rr + dk2 * rr * rr + dk3 * rr * rr * rr;

      // introducing distortion
      xt = ux / dr;
      yt = uy / dr;

      // distorted point
      xdistorted = xt;
      ydistorted = yt;

      // check for convergence
      if((fabs(xt - xprevious) <= tolerance) && (fabs(yt - yprevious) <= tolerance)) {
        bConverged = true;
        break;
      }

      xprevious = xt;
      yprevious = yt;
    }


    if(bConverged) {
      p_focalPlaneX = xdistorted;
      p_focalPlaneY = ydistorted;
    }

    return bConverged;
  }
}
