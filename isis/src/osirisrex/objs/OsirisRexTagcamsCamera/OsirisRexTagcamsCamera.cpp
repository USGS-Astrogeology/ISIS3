/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "OsirisRexTagcamsCamera.h"

#include <QDebug>
#include <QString>

#include "CameraDetectorMap.h"
#include "OsirisRexOcamsDistortionMap.h"
#include "OsirisRexTagcamsDistortionMap.h"
#include "CameraDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "IrregularBodyCameraGroundMap.h"
#include "CameraSkyMap.h"
#include "IString.h"
#include "iTime.h"
#include "NaifStatus.h"

using namespace std;

namespace Isis {

  /**
   * Constructs an OSIRIS-REx Camera Model using the image labels. This model 
   * supportsNavigation cameras: NavCam, NFTCam, and StowCam 
   *  
   * @param lab Pvl label from an Osiris Rex TAGCAMS image. 
   */
  OsirisRexTagcamsCamera::OsirisRexTagcamsCamera(Cube &cube) : FramingCamera(cube) {

    NaifStatus::CheckErrors();

    m_spacecraftNameLong = "OSIRIS-REx";
    m_spacecraftNameShort = "OSIRIS-REx";

    // The general IK code will be used to retrieve the transx,
    // transy, transs and transl from the iak. The focus position specific
    // IK code will be used to find pixel pitch and ccd center in the ik.
    int frameCode = naifIkCode();

    if (frameCode == -64081) {
      m_instrumentNameLong = "Primary Optical Navigation (NCM) Camera";
      m_instrumentNameShort = "NAVCam";
    }
    else if (frameCode == -64082) {
      m_instrumentNameLong = "Natural Feature Tracking (NFT) Camera";
      m_instrumentNameShort = "NFTCam";
    } 
    else if (frameCode == -64071) {
      m_instrumentNameLong = "Stow Camera";
      m_instrumentNameShort = "StowCam";
    }
    else {
      std::string msg = "Unable to construct OSIRIS-REx Navigation camera model. "
                    "Unrecognized NaifIkCode [" + toString(frameCode) + "].";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    Pvl &lab = *cube.label();
    PvlGroup inst = lab.findGroup("Instrument", Pvl::Traverse);
    QString ikCode = toString(frameCode);

    // Kernel values are in meters so convert to millimeters
    QString focalLength = "INS" + ikCode + "_FOCAL_LENGTH";
    double flen = getDouble(focalLength) * 1000.0;
    SetFocalLength(flen);

    // The instrument kernel contains pixel pitch in microns, so convert it to mm.
    QString pitch = "INS" + ikCode + "_PIXEL_SIZE";
    double pp = getDouble(pitch) / 1000.0;
    SetPixelPitch(pp);

    // Get the start time in et
    // Set the observation time and exposure duration
    QString clockCount = QString::fromStdString(inst["SpacecraftClockStartCount"]);
    double startTime = getClockTime(clockCount).Et();
    double exposureDuration = ((double) inst["ExposureDuration"]);
    pair<iTime, iTime> shuttertimes = ShutterOpenCloseTimes(startTime, exposureDuration);

    // Add half exposure duration to get time at center of image
    iTime centerTime = shuttertimes.first.Et() + exposureDuration / 2.0;

    // Setup detector map
    CameraDetectorMap *detmap = new CameraDetectorMap(this);

    // Determine and set binning 
    int binning =  (int) inst["Binning"];
    double summing = 1.0;  // If summing == 0
    if ( binning > 0 ) {
        if (binning == 2) {
            summing = 2.0;
        }
        else if ( binning == 4 ) {
            summing = 4.0;
        }
        else {
            double factor =  (binning - 14) * 1.0;
            summing = factor * factor;
        }
    }
    detmap->SetDetectorLineSumming(summing);
    detmap->SetDetectorSampleSumming(summing);


    // Setup focal plane map using the general IK code for the given camera
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(this, frameCode);

    // Set the center of the ccd (1-based pixel coordinates)
    double sampBsight = getDouble("INS" + ikCode + "_CCD_CENTER", 0) + 1;
    double lineBsight = getDouble("INS" + ikCode + "_CCD_CENTER", 1) + 1;
    focalMap->SetDetectorOrigin(sampBsight, lineBsight);

    // Setup distortion map
    // See what type of distortion model the IAK is configured to use
    QString dmodel = getString("INS" + ikCode + "_DISTORTION_MODEL").toUpper();

    // Set up tangential distortion model
    if ( "OPENCV" == dmodel) {
        OsirisRexTagcamsDistortionMap *dmap = new OsirisRexTagcamsDistortionMap(this, frameCode);
        double camHeadTemp(0.0);
        if ( inst.hasKeyword("CameraHeadTemperature") ) {
            camHeadTemp = (double) inst["CameraHeadTemperature"];
        }
        dmap->SetCameraTemperature(camHeadTemp);
    }
    else if ( "OCAMS" == dmodel ) {
        OsirisRexOcamsDistortionMap *dmap = new OsirisRexOcamsDistortionMap(this, frameCode);
        dmap->SetDistortion(frameCode);
    } 
    else {
        new CameraDistortionMap(this); 
    }

    // Setup the irregular body ground map and the sky map
    new IrregularBodyCameraGroundMap(this);
    new CameraSkyMap(this);

    setTime(centerTime);
    LoadCache();
    NaifStatus::CheckErrors();
  }


  //! Destroys the TAGCAMS object.
  OsirisRexTagcamsCamera::~OsirisRexTagcamsCamera() {
  }


  /**
   * The frame ID for the spacecraft (or instrument) used by the Camera-matrix 
   * Kernel. For this camera model, the spacecraft frame is used, represented 
   * by the frame ID -64000. 
   *  
   * @return @b int The appropriate code for the Camera-matrix Kernel. 
   */
  int OsirisRexTagcamsCamera::CkFrameId() const { 
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
  int OsirisRexTagcamsCamera::CkReferenceId() const { 
    return 1; 
  }


  /** 
   * The reference frame ID for the Spacecraft Kernel is 1, representing J2000.
   *
   * @return @b int The appropriate frame ID code for the Spacecraft Kernel.
   */
  int OsirisRexTagcamsCamera::SpkReferenceId() const { 
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
   * @author 2019-01-09 Kris Becker
   * @internal
   */
  pair<iTime, iTime> OsirisRexTagcamsCamera::ShutterOpenCloseTimes(double time,
                                                               double exposureDuration) {
    return FramingCamera::ShutterOpenCloseTimes(time, exposureDuration);
  }

}


/**
 * This method is used to instantiate an OsirisRexTagcamsCamera object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* OsirisRexTagcamsCamera
 */
extern "C" Isis::Camera *OsirisRexTagcamsCameraPlugin(Isis::Cube &cube) {
  return new Isis::OsirisRexTagcamsCamera(cube);
}
