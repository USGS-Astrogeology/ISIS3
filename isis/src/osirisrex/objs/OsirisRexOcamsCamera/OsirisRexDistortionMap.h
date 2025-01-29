#ifndef OsirisRexDistortionMap_h
#define OsirisRexDistortionMap_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QSharedPointer>

#include "CameraDistortionMap.h"

namespace Isis {
  /** Distort/undistort focal plane coordinates for OSIRIS REx's cameras.
   *
   * Creates a map for adding/removing optical distortions
   * from the focal plane of a camera.
   *
   * @ingroup Camera
   *
   * @see Camera
   *
   * @author 2017-08-21 Kristin Berry and Jeannie Backer
   *
   * @internal
   *   @history 2017-08-21 Kristin Berry and Jeannie Backer - Original Version
   *   @history 2019-03-15 Kris Becker - Added debug options
   *
   */
  class OsirisRexDistortionMap : public CameraDistortionMap {
    public:
      OsirisRexDistortionMap(Camera *parent, double zDirection = 1.0);

      virtual ~OsirisRexDistortionMap();

      virtual bool SetDistortion(int naifIkCode, QString filter="UNKNOWN");

      virtual bool SetFocalPlane(double dx, double dy);

      virtual bool SetUndistortedFocalPlane(double ux, double uy);

    protected:
      double m_pixelPitch; //!< The pixel pitch for OCAMS.
      double m_detectorOriginSample; //!< The origin of the detector's sample coordinate.
      double m_detectorOriginLine; //!< The origin of the detector's line coordinate.
      double m_distortionOriginSample; //!< The distortion's origin sample coordinate.
      double m_distortionOriginLine; //!< The distortion's origin line coordinate. 
      bool   m_debug; //!< Debug option

      QSharedPointer<CameraFocalPlaneMap> m_focalMap;  // Local focal plane map
  };
};
#endif
