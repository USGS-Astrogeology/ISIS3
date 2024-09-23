/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ApolloPanoramicCamera.h"

#include "ApolloPanIO.h"
#include "ApolloPanoramicDetectorMap.h"

#include <QString>

#include "CameraDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "LineScanCameraDetectorMap.h"
#include "LineScanCameraGroundMap.h"
#include "LineScanCameraSkyMap.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"

using namespace std;
namespace Isis {
  /**
   * Constructs an Apollo Panoramic Camera object using the image labels.
   *
   * @param lab Pvl label from an Apollo Panoramic image.
   *
   */
  ApolloPanoramicCamera::ApolloPanoramicCamera(Isis::Cube &cube) : Isis::LineScanCamera(cube) {
    // Set up the camera info from ik/iak kernels
    SetFocalLength(610.0);  //nominal (uncalibrated) focal length in mm from "Apollo 15 SIM Bay
    // Photographic Equipment and Mission Summary" August, 1971
    SetPixelPitch(0.005);   //internally all images are modeled as if they have 5 micron pixels

    double  constantTimeOffset = 0.0,
    additionalPreroll = 0.0,
    additiveLineTimeError = 0.0,
    multiplicativeLineTimeError = 0.0;

    // Set up naming info
    m_instrumentNameLong = "Panoramic Camera";
    m_instrumentNameShort = "Pan";

    // Apollo15 Pan naif code = -915230
    if (naifIkCode() == -915230) {
      m_spacecraftNameLong = "Apollo 15";
      m_spacecraftNameShort = "Apollo15";
    }
    // Apollo16 Pan naif code = -916230
    else if (naifIkCode() == -916230) {
      m_spacecraftNameLong = "Apollo 16";
      m_spacecraftNameShort = "Apollo16";
    }
    // Apollo17 Pan naif code = -917230
    else if (naifIkCode() == -917230) {
      m_spacecraftNameLong = "Apollo 17";
      m_spacecraftNameShort = "Apollo17";
    }
    else {
      std::string msg = "File does not appear to be an Apollo image";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    //following keywords in InstrumentAddendum file
    QString ikernKey = "INS" + QString::number((int)naifIkCode()) + "_CONSTANT_TIME_OFFSET";
    constantTimeOffset = getDouble(ikernKey);

    ikernKey = "INS" + QString::number((int)naifIkCode()) + "_ADDITIONAL_PREROLL";
    additionalPreroll = getDouble(ikernKey);

    ikernKey = "INS" + QString::number((int)naifIkCode()) + "_ADDITIVE_LINE_ERROR";
    additiveLineTimeError = getDouble(ikernKey);

    ikernKey = "INS" + QString::number((int)naifIkCode()) + "_MULTIPLI_LINE_ERROR";
    multiplicativeLineTimeError = getDouble(ikernKey);

    Pvl &lab = *cube.label();
    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);
    QString stime = QString::fromStdString(inst["StartTime"]);
    SpiceDouble etStart;
    str2et_c(stime.toLatin1().data(), &etStart);
    stime = QString::fromStdString(inst["StopTime"]);
    SpiceDouble etStop;
    str2et_c(stime.toLatin1().data(), &etStop);
    iTime isisTime( QString::fromStdString(inst["StartTime"]));

    // Get other info from labels
    // line exposure duration, sec/mm
    double lineRate = ( (double) inst["LineExposureDuration"] )*0.005;

    lineRate *= 1.0 + multiplicativeLineTimeError;
    lineRate += additiveLineTimeError;
    etStart += additionalPreroll * lineRate;
    etStart += constantTimeOffset;

    setTime(isisTime);

    // Setup detector map
    //note (etStart+etStop)/2.0 is the time in the middle of image
    //  (line = 0 after interior orientation)
    ApolloPanoramicDetectorMap *detectorMap =
    new ApolloPanoramicDetectorMap((Camera *)this,
                                   (etStart+etStop)/2.0,
                                   (double)lineRate, &lab);
    //interior orientation residual stats
    m_residualMean = detectorMap->meanResidual();
    m_residualMax = detectorMap->maxResidual();
    m_residualStdev = detectorMap->stdevResidual();

    detectorMap->SetDetectorSampleSumming(1.0);
    detectorMap->SetStartingDetectorSample(0.0);
    // Setup focal plane map
    PvlGroup &kernel = lab.findGroup("Kernels", Pvl::Traverse);
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(this, (int) kernel["NaifFrameCode"]);

    //  Retrieve boresight location from instrument kernel (IK) (addendum?)
    double sampleBoreSight = 0.0;  //Presently no NAIF keywords for this sensor
    double lineBoreSight = 0.0;  //Presently no NAIF keywords for this sensor

    focalMap->SetDetectorOrigin(sampleBoreSight, lineBoreSight);
    focalMap->SetDetectorOffset(0.0, 0.0);

    // Setup distortion map
    new CameraDistortionMap(this, -1.0);
    //distMap->SetDistortion(naifIkCode());    Presently no NAIF keywords for this sensor

    //Setup the ground and sky map
    new LineScanCameraGroundMap(this);
    new LineScanCameraSkyMap(this);

    PvlGroup &instP = lab.findGroup("Kernels", Pvl::Traverse);
    m_CkFrameId = IString::ToInteger(instP["NaifFrameCode"][0]);
    m_CkFrameId = -int(-m_CkFrameId/1000)*1000;

    LoadCache();
  }
}// end Isis namespace


/**
 * This is the function that is called in order to instantiate an
 * ApolloPanoramicCamera object.
 *
 * @param lab Cube labels
 *
 */
extern "C" Isis::Camera *ApolloPanoramicCameraPlugin(Isis::Cube &cube) {
   return new Isis::ApolloPanoramicCamera(cube);
}
