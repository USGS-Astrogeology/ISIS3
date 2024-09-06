/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Hyb2OncCamera.h"

#include <QString>
#include <QtMath>

#include "CameraDetectorMap.h"
#include "CameraFocalPlaneMap.h"
#include "CameraGroundMap.h"
#include "CameraSkyMap.h"
#include "Hyb2OncDistortionMap.h"
#include "IString.h"
#include "iTime.h"
#include "NaifStatus.h"

using namespace std;

namespace Isis {
  /**
   * Constructs a Hayabusa2 Hyb2OncCamera object using the image labels.
   *
   * @param cube Cube Hayabusa2 ONC image.
   *
   * @internal
   */
  Hyb2OncCamera::Hyb2OncCamera(Cube &cube) : FramingCamera(cube) {
    m_spacecraftNameLong = "Hayabusa2";
    m_spacecraftNameShort = "Hayabusa2";

    // Set the correct instrument name based on NaifFrameCode in kernels group

    // ONC-T
    if (naifIkCode() == -37100) {
      m_instrumentNameLong = "Optical Navigation Camera - Telescopic Camera";
      m_instrumentNameShort = "ONC-T";
    }
    // ONC-W1
    else if (naifIkCode() == -37110) {
      m_instrumentNameLong = "Optical Navigation Camera - W1 Camera";
      m_instrumentNameShort = "ONC-W1";
    }
    // ONC-W2
    else if (naifIkCode() == -37120) {
      m_instrumentNameLong = "Optical Navigation Camera - W2 Camera";
      m_instrumentNameShort = "ONC-W2";
    }
    else {
      std::string msg = "File does not appear to be a Hayabusa2 image";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    NaifStatus::CheckErrors();

    SetFocalLength();  // Retrives from IK stored in units of mm
    SetPixelPitch();  // Get from IAK

    // Get the start time in et
    Pvl &lab = *cube.label();
    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);

    // set variables startTime and exposureDuration
    QString stime = QString::fromStdString(inst["SpacecraftClockStartCount"]);
    iTime etStart = getClockTime(stime);

    double exposureDuration = ((double) inst["ExposureDuration"]);
    iTime centerTime  = etStart + (exposureDuration / 2.0);

    // Setup focal plane map
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(this, naifIkCode());

    // BORESIGHT SAMPLE AND LINE still need to be added to the IAK
    double bLines = Spice::getDouble("INS" + toString(naifIkCode()) + "_BORESIGHT_LINE");
    double bSamples = Spice::getDouble("INS" + toString(naifIkCode()) + "_BORESIGHT_SAMPLE");

    focalMap->SetDetectorOrigin(bSamples, bLines);

    // Setup detector map (use default for now)
    CameraDetectorMap *detMap = new CameraDetectorMap(this);

    // Handle summing
    int binning = inst["Binning"];
    detMap->SetDetectorLineSumming(binning);
    detMap->SetDetectorSampleSumming(binning);

    // Setup distortion map (use default for now)
    CameraDistortionMap *distortionMap = new Hyb2OncDistortionMap(this);
    distortionMap->SetDistortion(naifIkCode());

    // Setup the ground and sky map
    new CameraGroundMap(this);
    new CameraSkyMap(this);

    setTime(centerTime);
    LoadCache();
    NaifStatus::CheckErrors();
  }


  /**
   * Default Destructor
   */
  Hyb2OncCamera::~Hyb2OncCamera() {
  }


  /**
   * Returns the shutter open and close times. The user should pass in the
   * ExposureDuration keyword value, converted from milliseconds to seconds, and
   * the StartTime keyword value, converted to ephemeris time. The StartTime
   * keyword value from the labels represents the time at the start of the
   * observation, as noted in the Clementine EDR image SIS. This method uses the
   * FramingCamera class implementation, returning the given time value as the
   * shutter open and the sum of the time value and exposure duration as the
   * shutter close.
   *
   * @param exposureDuration ExposureDuration keyword value from the labels,
   *                         converted to seconds.
   * @param time The StartTime keyword value from the labels, converted to
   *             ephemeris time.
   *
   * @return @b pair < @b iTime, @b iTime > The first value is the shutter
   *         open time and the second is the shutter close time.
   */
  pair<iTime, iTime> Hyb2OncCamera::ShutterOpenCloseTimes(double time,
                                                          double exposureDuration) {
    return FramingCamera::ShutterOpenCloseTimes(time, exposureDuration);
  }


  /**
   * CK frame ID -  - Instrument Code from spacit run on CK
   *
   * @return @b int The appropriate instrument code for the "Camera-matrix"
   *         Kernel Frame ID
   */
  int Hyb2OncCamera::CkFrameId() const {
    return (-37000);
  }


  /**
   * CK Reference ID - J2000
   *
   * @return @b int The appropriate instrument code for the "Camera-matrix"
   *         Kernel Reference ID
   */
  int Hyb2OncCamera::CkReferenceId() const {
    return (1);
  }


  /**
   * SPK Reference ID - J2000
   *
   * @return @b int The appropriate instrument code for the Spacecraft
   *         Kernel Reference ID
   */
  int Hyb2OncCamera::SpkReferenceId() const {
    return (1);
  }
}


/**
 * This is the function that is called in order to instantiate an Hyb2OncCamera
 * object.
 *
 * @param cube Cube Hayabusa2 ONC image.
 *
 * @return Isis::Camera* Hyb2OncCamera
 */
extern "C" Isis::Camera *Hyb2OncCameraPlugin(Isis::Cube &cube) {
  return new Isis::Hyb2OncCamera(cube);
}
