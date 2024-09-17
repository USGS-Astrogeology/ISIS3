/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "LoHighCamera.h"
#include "LoHighDistortionMap.h"
#include "LoCameraFiducialMap.h"

#include <QString>

#include "Affine.h"
#include "CameraDetectorMap.h"
#include "CameraFocalPlaneMap.h"
#include "CameraGroundMap.h"
#include "CameraSkyMap.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "NaifStatus.h"

using namespace std;
namespace Isis {
  /**
   * Constructs the LoHigh camera model object from the labels.
   *
   * This constructor uses the Pvl labels for Lunar Orbiter High (2 m)
   * resolution images.
   *
   * @param lab Pvl label from a Lunar Orbiter High image.
   * @internal
   *   @history 2011-05-03 Jeannie Walldren - Added NAIF error check.
   *
   */
  LoHighCamera::LoHighCamera(Cube &cube) : FramingCamera(cube) {
    NaifStatus::CheckErrors();

    m_instrumentNameLong = "High Resolution Camera";
    m_instrumentNameShort = "High";

    // LO3 High instrument kernel code = -533001
    if (naifIkCode() == -533001) {
      m_spacecraftNameLong = "Lunar Orbiter 3";
      m_spacecraftNameShort = "LO3";
    }
    // L04 High instrument kernel code = -534001
    else if (naifIkCode() == -534001) {
      m_spacecraftNameLong = "Lunar Orbiter 4";
      m_spacecraftNameShort = "LO4";
    }
    // LO5 High instrument kernel code = -535001
    else if (naifIkCode() == -535001) {
      m_spacecraftNameLong = "Lunar Orbiter 5";
      m_spacecraftNameShort = "LO5";
    }
    else {
      std::string msg = "File does not appear to be a Lunar Orbiter image: ";
      msg += toString(naifIkCode());
      msg += " is not a supported instrument kernel code for Lunar Orbiter.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Get the Instrument label information needed to define the camera for this frame
    Pvl &lab = *cube.label();
    PvlGroup inst = lab.findGroup("Instrument", Pvl::Traverse);
    QString spacecraft = QString::fromStdString(inst["SpacecraftName"]);
    QString instId = QString::fromStdString(inst["InstrumentId"]);

    // Turn off the aberration corrections for the instrument position object
    instrumentPosition()->SetAberrationCorrection("NONE");

    // Get the camera characteristics
    SetFocalLength();
    SetPixelPitch();

    // Get the start time in et
    double time = iTime(QString::fromStdString(inst["StartTime"])).Et();

    // Setup focal plane map
    LoCameraFiducialMap fid(inst, naifIkCode());

    // Setup detector map
    new CameraDetectorMap(this);

    // Setup focalplane map
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(this, naifIkCode());
    // Try (0.,0.)
    focalMap->SetDetectorOrigin(0.0, 0.0);

    // Setup distortion map
    LoHighDistortionMap *distortionMap = new LoHighDistortionMap(this);
    distortionMap->SetDistortion(naifIkCode());
    // Setup the ground and sky map
    new CameraGroundMap(this);
    new CameraSkyMap(this);

    //  Determine the NAIF ID for the CK frame reference.
    if ( spacecraft.contains("3") ) {
      m_ckFrameId = -533000;
    }
    else if ( spacecraft.contains("4") ) {
      m_ckFrameId = -534000;
    }
    else if ( spacecraft.contains("5") ) {
      m_ckFrameId = -535000;
    }
    else {
      std::string msg = "File does not appear to be an LunarOrbiter 3,4,5 image";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    setTime(time);
    LoadCache();
    NaifStatus::CheckErrors();
  }


  /**
   * Returns the shutter open and close times. The user should pass in the
   * exposure duration in seconds and the StartTime keyword value, converted to
   * ephemeris time. The StartTime keyword value from the labels represents the
   * shutter center time of the observation. To find the shutter open and close
   * times, half of the exposure duration is subtracted from and added to the
   * input time parameter, respectively. This method overrides the FramingCamera
   * class method.
   * @b Note: Lunar Orbitar did not provide exposure duration in the support
   * data.
   *
   * @param exposureDuration Exposure duration, in seconds
   * @param time The StartTime keyword value from the labels, converted to
   *             ephemeris time.
   *
   * @return @b pair < @b iTime, @b iTime > The first value is the shutter
   *         open time and the second is the shutter close time.
   *
   * @author 2011-05-03 Jeannie Walldren
   * @internal
   *   @history 2011-05-03 Jeannie Walldren - Original version.
   */
  pair<iTime, iTime> LoHighCamera::ShutterOpenCloseTimes(double time,
                                                         double exposureDuration) {
    pair<iTime, iTime> shuttertimes;
    // To get shutter start (open) time, subtract half the exposure duration from the center time
    shuttertimes.first = time - (exposureDuration / 2);
    // To get shutter end (close) time, add half the exposure duration to the center time
    shuttertimes.second = time + (exposureDuration / 2);
    return shuttertimes;
  }
}


/**
 * This is the function that is called in order to instantiate a LoHighCamera
 * object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* LoHighCamera
 * @internal
 *   @history 2011-05-03 Jeannie Walldren - Added documentation.  Removed Lo
 *            namespace.
 */
extern "C" Isis::Camera *LoHighCameraPlugin(Isis::Cube &cube) {
  return new Isis::LoHighCamera(cube);
}
