#ifndef OsirisRexOcamsCamera_h
#define OsirisRexOcamsCamera_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "FramingCamera.h"

#include <QString>

namespace Isis {
  /**
   * This class models the behavior and attributes of the OSIRIS-REx Cameras:  Mapping Camera,
   * PolyMath Camera, and Sample Camera.
   *
   * @ingroup SpiceInstrumentsAndCameras
   * @ingroup Osiris Rex
   *
   * @author  2014-04-02 Janet Barrett
   *
   * @internal
   *   @history 2014-04-02 Janet Barrett - Original version.
   *   @history 2015-09-28 Stuart C. Sides - Updated to work with OCams data delivery 2015-07
   *   @history 2016-11-17 Stuart C. Sides - Modified the camera detector origin, in the
   *                           constructor, to reflect the difference between the zero
   *                           based system in the IK and the one based system of
   *                           ISIS. That is, add one to the IK CCD_CENTER values.
   *   @history 2016-12-27 Jeannie Backer - Fixed coding standards. Improved docuementation.
   *                           Fixed CKFrameId() to return the spacecraft ID rather than the MapCam
   *                           ID. Added tests. Moved from branch into trunk. Fixes #4570.
   *   @history 2017-08-25 Jeannie Backer - Updated to handle multiple focus postition IK
   *                           codes for PolyCam frames. We read PolyCamFocusPositionNaifId from
   *                           the Instrument group for focus position specific values (such ase
   *                           focal length) and we read NaifFrameId from the Kernels group the
   *                           instrument frame code. Fixes #5127
   *   @history 2018-07-26 UA/OSIRIS-REx IPWG Team - Use
   *                           IrregularBodyCameraGroundMap to work with jigsaw
   *                           control
   *   @history 2019-03-13 Kris Becker (UA) - Added OpenCV distortion model as a
   *                           runtime option along with the original OCAMS
   *                           distortion model.
   *
   */
  class OsirisRexOcamsCamera : public FramingCamera {
    public:
      OsirisRexOcamsCamera(Cube &cube);
      ~OsirisRexOcamsCamera();

      virtual std::pair <iTime, iTime> ShutterOpenCloseTimes(double time,
                                                             double exposureDuration);

      virtual int CkFrameId() const;
      virtual int CkReferenceId() const;
      virtual int SpkReferenceId() const;

    protected:
      int getFunctionalIkCode(const int naifIkCode, const int polyfocusCode,
                             const QString &filter) const;


  };
};
#endif
