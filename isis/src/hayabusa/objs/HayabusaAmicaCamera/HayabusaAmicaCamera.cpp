/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "HayabusaAmicaCamera.h"

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
   * Constructs a Hayabusa HayabusaAmicaCamera object using the image labels.
   *
   * @param lab Pvl label from a Hayabusa AMICA image.
   *
   * @internal
   */
  HayabusaAmicaCamera::HayabusaAmicaCamera(Cube &cube) : FramingCamera(cube) {
    m_instrumentNameLong = "Amica";
    m_instrumentNameShort = "Amica";
    m_spacecraftNameLong = "Hayabusa";
    m_spacecraftNameShort = "Hayabusa";

    NaifStatus::CheckErrors();
    Pvl &lab = *cube.label();
    // Get the camera characteristics
    QString filter = QString::fromStdString((lab.findGroup("BandBin", Pvl::Traverse))["Name"]);
    filter = filter.toUpper();

    SetFocalLength();  // Retrives from IK stored in units of meters
    SetFocalLength(FocalLength() * 1000.0);  // Convert from meters to mm

    // Get from IAK
    SetPixelPitch();

    // Get the start time in et
    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);

    // set variables startTime and exposureDuration
    QString stime = QString::fromStdString(inst["SpacecraftClockStartCount"]);
    iTime etStart = getClockTime(stime);

    double exposureDuration = ((double) inst["ExposureDuration"]);
    iTime centerTime  = etStart + (exposureDuration / 2.0);

    // Setup focal plane map
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(this, naifIkCode());

    // lines and samples added to the pvl in the order you
    // call getDouble()
    double bLines = Spice::getDouble("INS" + QString::number(naifIkCode()) + "_BORESIGHT_LINE");
    double bSamples = Spice::getDouble("INS" + QString::number(naifIkCode()) + "_BORESIGHT_SAMPLE");

    focalMap->SetDetectorOrigin(bSamples, bLines);

    // Setup detector map. FirstSample is zero-based indexing, Detector is one-based.
    CameraDetectorMap *detMap =  new CameraDetectorMap(this);
    detMap->SetStartingDetectorSample((int) inst["FirstSample"] + 1);

    // We flip the image over the horizontal axis on ingestion to
    // match fits viewers. So for cubes that are for subframes, first/last line
    // values in the label are flipped about the detector's x-axis. We need to
    // compensate to set the detector's first line. FirstLine is zero-based
    // indexing and there are 1024 lines on the detector so the Detector last
    // line index is 1023.
    int actualFirstLine = 1023 - ((int) inst["LastLine"]);

    //The detector line indexing is one-based.
    detMap->SetStartingDetectorLine(actualFirstLine + 1);

    // Handle summing
    int binning = inst["Binning"];
    detMap->SetDetectorLineSumming(binning);
    detMap->SetDetectorSampleSumming(binning);

    // Setup distortion map
    CameraDistortionMap *dmap = new CameraDistortionMap(this);
    dmap->SetDistortion(-130102);

    // Setup the ground and sky map
    new CameraGroundMap(this);
    new CameraSkyMap(this);

    setTime(centerTime);
    LoadCache();
    NaifStatus::CheckErrors();
  }


  /**
   * Destructor
   */
  HayabusaAmicaCamera::~HayabusaAmicaCamera() {
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
   *
   * @author 2011-05-03 Jeannie Walldren
   * @internal
   *   @history 2011-05-03 Jeannie Walldren - Original version.
   */
  pair<iTime, iTime> HayabusaAmicaCamera::ShutterOpenCloseTimes(double time,
                                                        double exposureDuration) {
    return FramingCamera::ShutterOpenCloseTimes(time, exposureDuration);
  }


  /**
   * CK frame ID -  - Instrument Code from spacit run on CK
   *
   * @return @b int The appropriate instrument code for the "Camera-matrix"
   *         Kernel Frame ID
   */
  int HayabusaAmicaCamera::CkFrameId() const {
    return (-130000);
  }


  /**
   * CK Reference ID - J2000
   *
   * @return @b int The appropriate instrument code for the "Camera-matrix"
   *         Kernel Reference ID
   */
  int HayabusaAmicaCamera::CkReferenceId() const {
    return (1);
  }


  /**
   * SPK Reference ID - J2000
   *
   * @return @b int The appropriate instrument code for the Spacecraft
   *         Kernel Reference ID
   */
  int HayabusaAmicaCamera::SpkReferenceId() const {
    return (1);
  }
}


/**
 * This is the function that is called in order to instantiate an HayabusaAmicaCamera
 * object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* HayabusaAmicaCamera
 * @internal
 *   @history 2013-11-27 Kris Becker - Original Version
 */
extern "C" Isis::Camera *HayabusaAmicaCameraPlugin(Isis::Cube &cube) {
  return new Isis::HayabusaAmicaCamera(cube);
}
