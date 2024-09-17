/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "CTXCamera.h"

#include <QString>

#include "CameraDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "IException.h"
#include "iTime.h"
#include "IString.h"
#include "LineScanCameraDetectorMap.h"
#include "LineScanCameraGroundMap.h"
#include "LineScanCameraSkyMap.h"
#include "NaifStatus.h"

using namespace std;
namespace Isis {
  /**
   * Constructs an MRO CTX Camera object using the image labels.
   *
   * @param lab Pvl label from a CTX Camera image.
   *
   * @internal
   *   @history 2011-05-03 Jeannie Walldren - Added NAIF error check.
   */
  CTXCamera::CTXCamera(Cube &cube) : LineScanCamera(cube) {
    m_instrumentNameLong = "Context Camera";
    m_instrumentNameShort = "CTX";
    m_spacecraftNameLong = "Mars Reconnaissance Orbiter";
    m_spacecraftNameShort = "MRO";

    NaifStatus::CheckErrors();
    // Set up the camera info from ik/iak kernels
    SetFocalLength();
    SetPixelPitch();

    Pvl &lab = *cube.label();
    // Get the start time from labels
    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);
    QString stime = QString::fromStdString(inst["SpacecraftClockCount"]);
    double etStart = getClockTime(stime).Et();

    // Get other info from labels
    double csum = inst["SpatialSumming"];
    double lineRate = (double) inst["LineExposureDuration"] / 1000.0;
    lineRate *= csum;
    double ss = inst["SampleFirstPixel"];
    ss += 1.0;

    // Setup detector map
    LineScanCameraDetectorMap *detectorMap =
      new LineScanCameraDetectorMap(this, etStart, lineRate);
    detectorMap->SetDetectorSampleSumming(csum);
    detectorMap->SetStartingDetectorSample(ss);

    // Setup focal plane map
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(this, naifIkCode());

    //  Retrieve boresight location from instrument kernel (IK) (addendum?)
    QString ikernKey = "INS" + QString::number((int)naifIkCode()) + "_BORESIGHT_SAMPLE";
    double sampleBoreSight = getDouble(ikernKey);

    ikernKey = "INS" + QString::number((int)naifIkCode()) + "_BORESIGHT_LINE";
    double lineBoreSight = getDouble(ikernKey);

    focalMap->SetDetectorOrigin(sampleBoreSight, lineBoreSight);
    focalMap->SetDetectorOffset(0.0, 0.0);

    // Setup distortion map
    CameraDistortionMap *distMap = new CameraDistortionMap(this);
    distMap->SetDistortion(naifIkCode());

    // Setup the ground and sky map
    new LineScanCameraGroundMap(this);
    new LineScanCameraSkyMap(this);

    LoadCache();
    NaifStatus::CheckErrors();
  }
}

/**
 * This is the function that is called in order to instantiate a CTXCamera
 * object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* CTXCamera
 * @internal
 *   @history 2011-05-03 Jeannie Walldren - Added documentation.  Removed Mro
 *            namespace.
 */
extern "C" Isis::Camera *CTXCameraPlugin(Isis::Cube &cube) {
  return new Isis::CTXCamera(cube);
}
