/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "MocNarrowAngleCamera.h"

#include <QString>

#include "CameraDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "LineScanCameraDetectorMap.h"
#include "LineScanCameraGroundMap.h"
#include "LineScanCameraSkyMap.h"
#include "NaifStatus.h"

using namespace std;
namespace Isis {
  /**
   * Constructor for the Mgs MOC Narrow Angle Camera Model
   *
   * @param lab Pvl label from an MOC NAC image.
   *
   * @internal
   *   @history 2011-05-03 Jeannie Walldren - Added NAIF error check.
   */
  MocNarrowAngleCamera::MocNarrowAngleCamera(Cube &cube) : LineScanCamera(cube) {
    m_instrumentNameLong = "Mars Orbiter Camera Narrow Angle";
    m_instrumentNameShort = "MOC-NA";
    m_spacecraftNameLong = "Mars Global Surveyor";
    m_spacecraftNameShort = "MGS";

    NaifStatus::CheckErrors();
    // Set up the camera info from ik/iak kernels
    //      LoadEulerMounting();
    SetFocalLength();
    SetPixelPitch();
    instrumentRotation()->SetTimeBias(-1.15);

    // Get the start time from labels
    Pvl &lab = *cube.label();
    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);
    QString stime =  QString::fromStdString(inst["SpacecraftClockCount"]);
    double etStart = getClockTime(stime).Et();

    // Get other info from labels
    double csum = inst["CrosstrackSumming"];
    double dsum = inst["DowntrackSumming"];
    double lineRate = (double) inst["LineExposureDuration"] / 1000.0;
    lineRate *= dsum;
    double ss = inst["FirstLineSample"];

    // Setup detector map
    LineScanCameraDetectorMap *detectorMap =
      new LineScanCameraDetectorMap(this, etStart, lineRate);
    detectorMap->SetDetectorSampleSumming(csum);
    detectorMap->SetDetectorLineSumming(dsum);
    detectorMap->SetStartingDetectorSample(ss);

    // Setup focal plane map
    CameraFocalPlaneMap *focalMap =
      new CameraFocalPlaneMap(this, naifIkCode());
    focalMap->SetDetectorOrigin(1024.5, 0.0);
    focalMap->SetDetectorOffset(0.0, 0.0);

    // Setup distortion map
    new CameraDistortionMap(this);

    // Setup the ground and sky map
    new LineScanCameraGroundMap(this);
    new LineScanCameraSkyMap(this);

    LoadCache();
    NaifStatus::CheckErrors();
  }
}


/**
 * This is the function that is called in order to instantiate a MocNarrowAngleCamera object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* MocNarrowAngleCamera
 * @internal
 *   @history 2011-05-03 Jeannie Walldren - Removed Mgs namespace.
 */
extern "C" Isis::Camera *MocNarrowAngleCameraPlugin(Isis::Cube &cube) {
  return new Isis::MocNarrowAngleCamera(cube);
}
