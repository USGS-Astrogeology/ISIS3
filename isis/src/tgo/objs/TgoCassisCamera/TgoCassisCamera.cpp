/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "TgoCassisCamera.h"

#include <cmath>

#include <QByteArray>
#include <QString>
#include <QVariant>

#include "CameraDetectorMap.h"
#include "CameraFocalPlaneMap.h"
#include "CameraGroundMap.h"
#include "CameraSkyMap.h"
#include "EndianSwapper.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "NaifStatus.h"
#include "TgoCassisDistortionMap.h"

using namespace std;

namespace Isis {
  /**
   * @brief Initialize the CaSSIS camera model
   *
   *
   * @param cube The image cube.
   */
  TgoCassisCamera::TgoCassisCamera(Cube &cube) : FramingCamera(cube) {

    m_instrumentNameLong = "Colour and Stereo Surface Imaging System";
    m_instrumentNameShort = "CaSSIS";

    m_spacecraftNameLong = "Trace Gas Orbiter";
    m_spacecraftNameShort = "TGO";

    NaifStatus::CheckErrors();

    // CaSSIS codes
    int cassisCode = naifIkCode();
    QString cassis = QString::number(cassisCode);

    // Get all the necessary stuff from the labels
    Pvl &lab = *cube.label();
    const PvlGroup &inst    = lab.findGroup("Instrument", Pvl::Traverse);

    // Set up the camera characteristics
    instrumentRotation()->SetFrame(-143420);
    SetFocalLength();
    SetPixelPitch();

    // Get the Start time from the labels
    // TODO: This is currently using UTC time. Once the timestamp is figured out,
    //       this will change to use SCLK. JAM 2017-02-06
    QString stime = QString::fromStdString(inst["SpacecraftClockStartCount"]);
    QString startT = QString::fromStdString(inst["StartTime"]);
    iTime et(startT);

    // Get summing mode
    // Summing modes are:
    //   0 = 1x1 (No summing)
    //   1 = 2x2
    //   2 = 4x4
    int sumMode = Isis::toInt(inst["SummingMode"][0]);
    int summing = sumMode * 2;

    //  Setup camera detector map
    CameraDetectorMap *detMap = new CameraDetectorMap(this);
    if ( summing > 0 ) {
      detMap->SetDetectorSampleSumming(summing);
      detMap->SetDetectorLineSumming(summing);
    }

    // Setup focal plane map
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(this, cassisCode);

    // Get CASSIS detector boresight
    double bsSample = getDouble("INS" + cassis + "_BORESIGHT_SAMPLE");
    double bsLine = getDouble("INS" + cassis + "_BORESIGHT_LINE");
    focalMap->SetDetectorOrigin(bsSample, bsLine);

    // Setup distortion map
    try {
      new TgoCassisDistortionMap(this, naifIkCode());
    }
    catch (IException &e) {
      // Set NULL so that cameras destructor wont seg fault trying to delete
      SetDistortionMap(NULL, false);
      std::string msg = "Unable to Create TgoCassisDistortionMap";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
    // Setup the ground and sky map
    new CameraGroundMap(this);
    new CameraSkyMap(this);

    // Set start time to center of exposure time to ensure
    // the proper SPICE data is cached.
    double p_exposureDur = Isis::toDouble(inst["ExposureDuration"]);
    iTime p_etStart = et + ( p_exposureDur / 2.0);

    setTime(p_etStart);
    LoadCache();
    NaifStatus::CheckErrors();
  }


  /**
   * Destroys the TgoCassisCamera object.
   */
  TgoCassisCamera::~TgoCassisCamera() {
  }


  /**
   * Returns the shutter open and close times.  The user should pass in the
   * ExposureDuration keyword value, converted from milliseconds to seconds, and
   * the SpacecraftClockCount keyword value, converted to ephemeris time. The
   * StartTime keyword value from the labels represents the shutter open time of
   * the observation. This method uses the FramingCamera class implementation,
   * returning the given time value as the shutter open and the sum of the time
   * value and exposure duration as the shutter close.
   *
   * @param exposureDuration Exposure duration value from the labels, converted
   *                         to seconds.
   * @param time The SpacecraftClockCount value from the labels, converted to
   *             ephemeris time
   * @return @b pair < @b iTime, @b iTime > The first value is the shutter
   *         open time and the second is the shutter close time.
   *
   * @see
   * @author 2017-02-01 Kris Becker
   * @internal
   */
  pair <iTime, iTime> TgoCassisCamera::ShutterOpenCloseTimes(double time,
                                                             double exposureDuration) {
    return FramingCamera::ShutterOpenCloseTimes(time, exposureDuration);
  }


  /**
   * CK frame ID -  TGO CaSSIS instrument code (TGO_CASSIS_FSA)
   *
   * @return @b int The appropriate instrument code for the "Camera-matrix"
   *                Kernel Frame ID.
   */
  int TgoCassisCamera::CkFrameId() const {
    return (-143000);
  }


  /**
    * CK Reference ID - J2000
    *
    * @return @b int The appropriate instrument code for the "Camera-matrix"
    *                Kernel Reference ID.
    */
  int TgoCassisCamera::CkReferenceId() const {
    return (1);
  }


  /**
    * SPK Target Body ID - TGO spacecraft -143
    *
    * @return @b int The appropriate instrument code for the Spacecraft
    *                Kernel Target ID.
    */
  int TgoCassisCamera::SpkTargetId() const {
    return (-143);
  }


  /**
    *  SPK Reference ID - J2000
    *
    * @return @b int The appropriate instrument code for the Spacecraft
    *                Kernel Reference ID.
    */
  int TgoCassisCamera::SpkReferenceId() const {
    return (1);
  }

}

/**
 * This is the function that is called in order to instantiate a TgoCassisCamera
 * object.
 *
 * @param cube The image cube.
 *
 * @return Isis::Camera* TgoCassisCamera
 */
extern "C" Isis::Camera *TgoCassisCameraPlugin(Isis::Cube &cube) {
  return new Isis::TgoCassisCamera(cube);
}
