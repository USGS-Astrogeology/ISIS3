/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "JunoCamera.h"

#include <QDebug>
#include <QString>

#include "CameraDetectorMap.h"
#include "CameraFocalPlaneMap.h"
#include "CameraGroundMap.h"
#include "CameraSkyMap.h"
#include "iTime.h"
#include "JunoDistortionMap.h"
#include "NaifStatus.h"

using namespace std;

namespace Isis {
  /**
   * @brief Initialize the Juno camera model
   *
   *
   * @param cube The image cube.
   */
  JunoCamera::JunoCamera(Cube &cube) : FramingCamera(cube) {

    m_instrumentNameLong = "Juno EPO Camera";
    m_instrumentNameShort = "JNC"; // or JunoCam?

    m_spacecraftNameLong = "Juno";
    m_spacecraftNameShort = "Juno";

    NaifStatus::CheckErrors();

    // Set up the camera characteristics
    instrumentRotation()->SetFrame( CkFrameId() );
    SetFocalLength();
    SetPixelPitch();

    // Get all the necessary stuff from the labels
    Pvl &lab = *cube.label();
    const PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);

    // Get summing mode
    // Summing modes are:
    //   1 = 1x1 (No summing)
    //   2 = 2x2
    int sumMode = (int) inst["SummingMode"];
    int summing = sumMode;

    //  Setup camera detector map
    CameraDetectorMap *detMap = new CameraDetectorMap(this);
    if ( summing > 0 ) {
      detMap->SetDetectorSampleSumming(summing);
      detMap->SetDetectorLineSumming(summing);
    }

    // Juno codes
    int junoCode = naifIkCode();
    QString juno = toString(junoCode);

    // Setup focal plane map and set Juno detector boresight
    new CameraFocalPlaneMap(this, junoCode);
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(this, junoCode);
    double bsSample = getDouble("INS" + juno + "_BORESIGHT_SAMPLE");
    double bsLine = getDouble("INS" + juno + "_BORESIGHT_LINE");
    focalMap->SetDetectorOrigin(bsSample, bsLine);

    // Set starting filter location on the detector
    const PvlGroup &bandBin = lab.findGroup("BandBin", Pvl::Traverse);
    QString filterIkCode = QString::fromStdString(bandBin.findKeyword("NaifIkCode")[0]);
    detMap->SetStartingDetectorLine(getDouble("INS" + filterIkCode + "_FILTER_OFFSET"));

    // Set up distortion map, keeping z-direction positive JunoDistortion map defaults to z+
    JunoDistortionMap *distortionMap = new JunoDistortionMap(this);
    distortionMap->SetDistortion(CkFrameId());

    // Setup the ground and sky map
    new CameraGroundMap(this);
    new CameraSkyMap(this);

    // Set time based on clock count, frame number, exposure duration, and interframe delay
    QString startClockCount   = QString::fromStdString(inst["SpacecraftClockStartCount"]);
    double observationStartEt = getClockTime(startClockCount).Et(); // in seconds
    double frameNumber     = (double) inst["FrameNumber"];
    double interFrameDelay    = (double) inst["InterFrameDelay"];  // in seconds
    double exposureDur        = ((double) inst["ExposureDuration"]) / 1000.0; // in seconds

    // Get the fixed time biases
    double startTimeBias       = getDouble("INS" + juno + "_START_TIME_BIAS");
    double interFrameDelayBias = getDouble("INS" + juno + "_INTERFRAME_DELTA");

    // get start et for this frame, in seconds
    double frameStartEt = observationStartEt + startTimeBias + (frameNumber - 1)
                             * (interFrameDelay + interFrameDelayBias);
    // Set start time to center of exposure time to ensure the proper SPICE data is cached.
    setTime(frameStartEt + exposureDur / 2.0);

    LoadCache();
    NaifStatus::CheckErrors();
  }


  /**
   * Destroys the JunoCamera object.
   */
  JunoCamera::~JunoCamera() {
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
   *
   * @return @b pair < @b iTime, @b iTime > The first value is the shutter
   *         open time and the second is the shutter close time.
   */
  pair <iTime, iTime> JunoCamera::ShutterOpenCloseTimes(double time,
                                                        double exposureDuration) {

    return FramingCamera::ShutterOpenCloseTimes(time, exposureDuration);
  }


  /**
   * Retrieves the CK frame ID for the JunoCam instrument.
   *
   * @return @b int The appropriate instrument code for the "Camera-matrix"
   *                Kernel Frame ID.
   */
  int JunoCamera::CkFrameId() const {
    return -61500;
  }


  /**
    * Retrieves the J2000 CK Reference ID for the JunoCam instrument.
    *
    * @return @b int The appropriate instrument code for the "Camera-matrix"
    *                Kernel Reference ID.
    */
  int JunoCamera::CkReferenceId() const {
    return 1;
  }


  /**
    * Retrieves the SPK Target Body ID for the JunoCam instrument.
    *
    * @return @b int The appropriate instrument code for the Spacecraft
    *                Kernel Target ID.
    */
  int JunoCamera::SpkTargetId() const {
    return -61;
  }


  /**
    * Retrieves the J2000 SPK Reference ID for the JunoCam instrument.
    *
    * @return @b int The appropriate instrument code for the Spacecraft
    *                Kernel Reference ID.
    */
  int JunoCamera::SpkReferenceId() const {
    return 1;
  }

}

/**
 * This is the function that is called in order to instantiate a JunoCamera
 * object.
 *
 * @param cube The image cube.
 *
 * @return Isis::Camera* JunoCamera
 */
extern "C" Isis::Camera *JunoCameraPlugin(Isis::Cube &cube) {
  return new Isis::JunoCamera(cube);
}
