/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include "Camera.h"
#include "IString.h"
#include "LroWideAngleCameraFocalPlaneMap.h"

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
  LroWideAngleCameraFocalPlaneMap::LroWideAngleCameraFocalPlaneMap(Camera *parent,
                                                                   int naifIkCode) :
                                                                   CameraFocalPlaneMap(parent,
                                                                                       naifIkCode) {
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
  void LroWideAngleCameraFocalPlaneMap::addFilter(int naifIkCode) {

    QString xkey  = "INS" + toString(naifIkCode) + "_TRANSX";
    QString ykey  = "INS" + toString(naifIkCode) + "_TRANSY";
    QString ixkey = "INS" + toString(naifIkCode) + "_ITRANSS";
    QString iykey = "INS" + toString(naifIkCode) + "_ITRANSL";
    TranslationParameters trans_p;
    for (int i = 0; i < 3; ++i) {
      trans_p.m_transx[i]  = p_camera->getDouble(xkey, i);
      trans_p.m_transy[i]  = p_camera->getDouble(ykey, i);
      trans_p.m_itranss[i] = p_camera->getDouble(ixkey, i);
      trans_p.m_itransl[i] = p_camera->getDouble(iykey, i);
    }

    m_transparms.push_back(trans_p);
    return;
  }


/**
 * @brief Implements band-dependant focal plane parameters
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
  void LroWideAngleCameraFocalPlaneMap::setBand(int vband) {
    if ( (vband <= 0) || (vband > m_transparms.size()) ) {
      std::string mess = "Invalid band (" + QString::number(vband) + " requested " +
                     " Must be <= " + QString::number(m_transparms.size());
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }

    //  Install new parameters
    int iband = vband - 1;
    for ( int i = 0 ; i < 3 ; i++) {
      p_transx[i]  = m_transparms[iband].m_transx[i];
      p_transy[i]  = m_transparms[iband].m_transy[i];
      p_itranss[i] = m_transparms[iband].m_itranss[i];
      p_itransl[i] = m_transparms[iband].m_itransl[i];
    }

    return;
  }
} // namespace Isis
