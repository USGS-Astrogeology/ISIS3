/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "SsiCamera.h"

#include <QString>

#include "CameraDetectorMap.h"
#include "CameraDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "CameraGroundMap.h"
#include "CameraSkyMap.h"
#include "IString.h"
#include "iTime.h"
#include "NaifStatus.h"
#include "Pvl.h"
#include "PvlObject.h"
#include "RadialDistortionMap.h"
#include "Spice.h"

using namespace std;

namespace Isis {
  /**
   * Constructs a SsiCamera object using the image labels.
   *
   * @param lab Pvl label from a Galileo SSI image.
   *
   * @internal
   *   @history 2011-05-03 Jeannie Walldren - Added NAIF error check. Added call
   *                          to ShutterOpenCloseTimes() method.
   */
  SsiCamera::SsiCamera(Cube &cube) : FramingCamera(cube) {
    m_instrumentNameLong = "Solid State Imaging System";
    m_instrumentNameShort = "SSI";
    m_spacecraftNameLong = "Galileo Orbiter";
    m_spacecraftNameShort = "Galileo";

    NaifStatus::CheckErrors();
    // Get the camera characteristics
    double k1;

    Pvl &lab = *cube.label();
    iTime removeCoverDate("1994/04/01 00:00:00");
    iTime imageDate(QString::fromStdString(lab.findKeyword("StartTime", PvlObject::Traverse)[0]));
    /*
    * Change the Focal Length and K1 constant based on whether or not the protective cover is on
    * See "The Direction of the North Pole and the Control Network of Asteroid 951 Gaspra"  Icarus 107, 18-22 (1994)
    */
    if(imageDate < removeCoverDate) {
      int code = naifIkCode();
      QString key = "INS" + toString(code) + "_FOCAL_LENGTH_COVER";
      SetFocalLength(Spice::getDouble(key));
      k1 = Spice::getDouble("INS" + toString(naifIkCode()) + "_K1_COVER");
    }
    else {
      SetFocalLength();
      k1 = Spice::getDouble("INS" + toString(naifIkCode()) + "_K1");
    }

    SetPixelPitch();

    // Get the start time in et
    PvlGroup inst = lab.findGroup("Instrument", Pvl::Traverse);

    double et = iTime(QString::fromStdString(inst["StartTime"])).Et();

    //?????????? NEED THESE??????
    // exposure duration keyword value is measured in seconds
    double exposureDuration = ((double) inst["ExposureDuration"]);
    pair<iTime, iTime> shuttertimes = ShutterOpenCloseTimes(et, exposureDuration);

    // Get summation mode
    double sumMode = inst["Summing"];

    // Setup detector map
    CameraDetectorMap *detectorMap = new CameraDetectorMap(this);
    detectorMap->SetDetectorSampleSumming(sumMode);
    detectorMap->SetDetectorLineSumming(sumMode);

    // Setup focal plane map
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(this, naifIkCode());

    focalMap->SetDetectorOrigin(
      Spice::getDouble("INS" + toString(naifIkCode()) +
                       "_BORESIGHT_SAMPLE"),
      Spice::getDouble("INS" + toString(naifIkCode()) +
                       "_BORESIGHT_LINE"));

    // Setup distortion map
    new RadialDistortionMap(this, k1);

    // Setup the ground and sky map
    new CameraGroundMap(this);
    new CameraSkyMap(this);

    setTime(et);
    LoadCache();
    NaifStatus::CheckErrors();
  }

  /**
   * Returns the shutter open and close times. The user should pass in the
   * ExposureDuration keyword value and the StartTime keyword value, converted
   * to ephemeris time. The  StartTime keyword value from the labels represents
   * the shutter center time of the observation. To find the shutter open and
   * close times, half of the exposure duration is subtracted from and added to
   * the input time parameter, respectively. This method overrides the
   * FramingCamera class method.
   *
   * @param exposureDuration ExposureDuration keyword value from the labels, in
   *                         seconds.
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
  pair<iTime, iTime> SsiCamera::ShutterOpenCloseTimes(double time,
                                                      double exposureDuration) {
    pair <iTime, iTime> shuttertimes;
    // To get shutter start (open) time, subtract half exposure duration
    shuttertimes.first = time - (exposureDuration / 2.0);
    // To get shutter end (close) time, add half exposure duration
    shuttertimes.second = time + (exposureDuration / 2.0);
    return shuttertimes;
  }
}


/**
 * This is the function that is called in order to instantiate a SsiCamera
 * object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* SsiCamera
 * @internal
 *   @history 2011-05-03 Jeannie Walldren - Added documentation.  Removed
 *            Galileo namespace.
 */
extern "C" Isis::Camera *SsiCameraPlugin(Isis::Cube &cube) {
  return new Isis::SsiCamera(cube);
}
