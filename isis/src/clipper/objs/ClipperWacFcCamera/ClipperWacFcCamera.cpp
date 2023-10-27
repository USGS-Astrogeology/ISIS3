/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ClipperWacFcCamera.h"

#include <QString>

#include "CameraDetectorMap.h"
#include "CameraDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "CameraGroundMap.h"
#include "CameraSkyMap.h"
#include "IString.h"
#include "iTime.h"
#include "NaifStatus.h"

using namespace std;

namespace Isis {

/**
   * Constructs a Clipper wide angle framing camera object.
   *
   * @param lab Pvl label from a Clipper wide angle framing camera image.
   *
   * @author Stuart Sides and Summer Stapleton-Greig
   *
   * @internal
   */
  ClipperWacFcCamera::ClipperWacFcCamera(Cube &cube) : FramingCamera(cube) {
    m_spacecraftNameLong = "Europa Clipper";
    m_spacecraftNameShort = "Clipper";

    m_instrumentNameLong  = "Europa Imaging System Framing Wide Angle Camera";
    m_instrumentNameShort = "EIS-FWAC";

    NaifStatus::CheckErrors();

    SetFocalLength();
    SetPixelPitch();

    // Set up detector map, focal plane map, and distortion map
    new CameraDetectorMap(this);
    new CameraFocalPlaneMap(this, naifIkCode());
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(this, naifIkCode());
    focalMap->SetDetectorOrigin(2048.5, 1024.5);
    new CameraDistortionMap(this);
    CameraDistortionMap *distMap = new CameraDistortionMap(this);
    distMap->SetDistortion(naifIkCode());

    // Setup the ground and sky map
    new CameraGroundMap(this);
    new CameraSkyMap(this);

    Pvl &lab = *cube.label();
    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);
    QString startTime = QString::fromStdString(inst["StartTime"]);
    iTime etStart(startTime);

    // double exposureDuration = (double)inst["ExposureDuration"] / 1000.0;
    // pair<iTime, iTime> startStop = ShutterOpenCloseTimes(et, exposureDuration);

     setTime(etStart.Et()); // Set the time explicitly for now to prevent segfault

    // Internalize all the NAIF SPICE information into memory.
    LoadCache();
    NaifStatus::CheckErrors();
  }

  /**
   * Returns the shutter open and close times.  The LORRI camera doesn't use a shutter to start and
   * end an observation, but this function is being used to get the observation start and end times,
   * so we will simulate a shutter.
   *
   * @param exposureDuration ExposureDuration keyword value from the labels,
   *                         converted to seconds.
   * @param time The StartTime keyword value from the labels, converted to
   *             ephemeris time.
   *
   * @return @b pair < @b iTime, @b iTime > The first value is the shutter
   *         open time and the second is the shutter close time.
   *
   */
  pair<iTime, iTime> ClipperWacFcCamera::ShutterOpenCloseTimes(double time,
                                                         double exposureDuration) {
    return FramingCamera::ShutterOpenCloseTimes(time - exposureDuration / 2.0, exposureDuration);
  }
}

/**
 * This is the function that is called in order to instantiate a ClipperWacFcCamera
 * object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* ClipperWacFcCamera
 * @internal
 */
extern "C" Isis::Camera *ClipperWacFcCameraPlugin(Isis::Cube &cube) {
  return new Isis::ClipperWacFcCamera(cube);
}
