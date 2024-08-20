/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "OsirisRexOcamsCamera.h"

#include <QDebug>
#include <QString>

#include "CameraDetectorMap.h"
#include "OsirisRexDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "CameraGroundMap.h"
#include "CameraSkyMap.h"
#include "IString.h"
#include "iTime.h"
#include "NaifStatus.h"

using namespace std;

namespace Isis {

  /**
   * Constructs an OSIRIS-REx Camera Model using the image labels. This model supports MapCam,
   * PolyCam, and SamCam images.
   *
   * @param lab Pvl label from an Osiris Rex MapCam image.
   */
  OsirisRexOcamsCamera::OsirisRexOcamsCamera(Cube &cube) : FramingCamera(cube) {

    NaifStatus::CheckErrors();

    m_spacecraftNameLong = "OSIRIS-REx";
    m_spacecraftNameShort = "OSIRIS-REx";

    // The general IK code will be used to retrieve the transx,
    // transy, transs and transl from the iak. The focus position specific
    // IK code will be used to find pixel pitch and ccd center in the ik.
    int frameCode = naifIkCode();

    if (frameCode == -64361) {
      m_instrumentNameLong = "Mapping Camera";
      m_instrumentNameShort = "MapCam";
    }
    else if (frameCode == -64362) {
      m_instrumentNameLong = "Sampling Camera";
      m_instrumentNameShort = "SamCam";
    } // IK values for polycam: -64360 (general) and -64616 to -64500 (focus specific)
    else if ((frameCode == -64360) || (frameCode >= -64616 && frameCode <= -64500)) {
      m_instrumentNameLong = "PolyMath Camera";
      m_instrumentNameShort = "PolyCam";
    }
    else {
      QString msg = "Unable to construct OSIRIS-REx camera model. "
                    "Unrecognized NaifFrameCode [" + toString(frameCode) + "].";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    Pvl &lab = *cube.label();
    PvlGroup inst = lab.findGroup("Instrument", Pvl::Traverse);

    QString ikCode = toString(frameCode);
    if (inst.hasKeyword("PolyCamFocusPositionNaifId") && frameCode == -64360) {
      if (QString::compare("NONE", QString::fromStdString(inst["PolyCamFocusPositionNaifId"]),
                           Qt::CaseInsensitive) != 0) {
        ikCode = QString::fromStdString(inst["PolyCamFocusPositionNaifId"][0]);
      }
    }

    QString focalLength = "INS" + ikCode + "_FOCAL_LENGTH";
    SetFocalLength(getDouble(focalLength));

    // The instrument kernel contains pixel pitch in microns, so convert it to mm.
    QString pitch = "INS" + ikCode + "_PIXEL_SIZE";
    SetPixelPitch(getDouble(pitch) / 1000.0);

    // Get the start time in et
    // Set the observation time and exposure duration
    QString clockCount = QString::fromStdString(inst["SpacecraftClockStartCount"]);
    double startTime = getClockTime(clockCount).Et();
    double exposureDuration = ((double) inst["ExposureDuration"]) / 1000.0;
    pair<iTime, iTime> shuttertimes = ShutterOpenCloseTimes(startTime, exposureDuration);

    // Add half exposure duration to get time at center of image
    iTime centerTime = shuttertimes.first.Et() + exposureDuration / 2.0;

    // Setup detector map
    new CameraDetectorMap(this);

    // Setup focal plane map using the general IK code for the given camera
    // Note that this is not the specific naifIkCode() value for PolyCam
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(this, frameCode);

    // The instrument kernel contains a CCD_CENTER keyword instead of BORESIGHT_LINE
    // and BORESIGHT_SAMPLE keywords.
    focalMap->SetDetectorOrigin(
        Spice::getDouble("INS" + ikCode + "_CCD_CENTER", 0) + 1.0,
        Spice::getDouble("INS" + ikCode + "_CCD_CENTER", 1) + 1.0);

    // Setup distortion map
    OsirisRexDistortionMap *distortionMap = new OsirisRexDistortionMap(this);

    // Different distortion model for each instrument and filter
    PvlGroup bandBin = lab.findGroup("BandBin", Pvl::Traverse);
    QString filterName = QString::fromStdString(bandBin["FilterName"]);
    distortionMap->SetDistortion(ikCode.toInt(), filterName);

    // Setup the ground and sky map
    new CameraGroundMap(this);
    new CameraSkyMap(this);

    setTime(centerTime);
    LoadCache();
    NaifStatus::CheckErrors();
  }


  //! Destroys the MapCamera object.
  OsirisRexOcamsCamera::~OsirisRexOcamsCamera() {
  }


  /**
   * The frame ID for the spacecraft (or instrument) used by the Camera-matrix
   * Kernel. For this camera model, the spacecraft frame is used, represented
   * by the frame ID -64000.
   *
   * @return @b int The appropriate code for the Camera-matrix Kernel.
   */
  int OsirisRexOcamsCamera::CkFrameId() const {
    return -64000;
  }


  /**
   * The frame ID for the reference coordinate system used by the Camera-matrix
   * Kernel. For this mission, the reference frame J2000, represented by the
   * frame ID 1.
   *
   * @return @b int The appropriate reference frame ID code for the
   *         Camera-matrix Kernel.
   */
  int OsirisRexOcamsCamera::CkReferenceId() const {
    return 1;
  }


  /**
   * The reference frame ID for the Spacecraft Kernel is 1, representing J2000.
   *
   * @return @b int The appropriate frame ID code for the Spacecraft Kernel.
   */
  int OsirisRexOcamsCamera::SpkReferenceId() const {
    return 1;
  }


  /**
   * Returns the shutter open and close times. The user should pass in the
   * ExposureDuration keyword value, converted to seconds, and the StartTime
   * keyword value, converted to ephemeris time. The StartTime keyword value
   * from the labels represents the time at the start of the observation, as
   * noted in the Osiris Rex EDR image SIS. This method uses the FramingCamera
   * class implementation, returning the given time value as the shutter open
   * and the sum of the time value and exposure duration as the shutter close.
   *
   * @param exposureDuration ExposureDuration keyword value from the labels,
   *                         converted to seconds.
   * @param time The StartTime keyword value from the labels, converted to
   *             ephemeris time.
   *
   * @return @b pair < @b iTime, @b iTime > The first value is the shutter
   *         open time and the second is the shutter close time.
   *
   * @see http://pds-imaging.jpl.nasa.gov/documentation/clementine_edrsis.pdf
   * @author 2011-05-03 Jeannie Walldren
   * @internal
   *   @history 2011-05-03 Jeannie Walldren - Original version.
   */
  pair<iTime, iTime> OsirisRexOcamsCamera::ShutterOpenCloseTimes(double time,
                                                               double exposureDuration) {
    return FramingCamera::ShutterOpenCloseTimes(time, exposureDuration);
  }

}


/**
 * This is the function that is called in order to instantiate a MapCamera
 * object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* OsirisRexOcamsCamera
 */
extern "C" Isis::Camera *OsirisRexOcamsCameraPlugin(Isis::Cube &cube) {
  return new Isis::OsirisRexOcamsCamera(cube);
}
