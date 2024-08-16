/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "HiriseCamera.h"

#include <string>
#include <iomanip>

#include <QString>

#include "CameraDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "iTime.h"
#include "LineScanCameraDetectorMap.h"
#include "LineScanCameraGroundMap.h"
#include "LineScanCameraSkyMap.h"
#include "NaifStatus.h"

using namespace std;
namespace Isis {
  /**
   * Creates a Hirise Camera Model
   *
   * @param lab Pvl label from the iamge
   * @internal
   *   @history 2011-05-03 Jeannie Walldren - Added NAIF error check.
   */
  HiriseCamera::HiriseCamera(Cube &cube) : LineScanCamera(cube) {
    m_instrumentNameLong = "High Resolution Imaging Science Experiment";
    m_instrumentNameShort = "HiRISE";
    m_spacecraftNameLong = "Mars Reconnaissance Orbiter";
    m_spacecraftNameShort = "MRO";

    NaifStatus::CheckErrors();
    // Setup camera characteristics from instrument and frame kernel
    SetFocalLength();
    SetPixelPitch();
    //LoadFrameMounting("MRO_SPACECRAFT", "MRO_HIRISE_OPTICAL_AXIS");
    instrumentRotation()->SetFrame(-74690);

    // Get required keywords from instrument group
    Pvl &lab = *cube.label();
    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);
    int tdiMode = inst["Tdi"];
    double binMode = inst["Summing"];
    int chan = inst["ChannelNumber"];
    int cpmm = inst["CpmmNumber"];
    double deltaLineTimerCount = inst["DeltaLineTimerCount"];
    QString stime = QString::fromStdString(inst["SpacecraftClockStartCount"]);

    // Convert CPMM number to CCD number
    static int cpmm2ccd[] = {0, 1, 2, 3, 12, 4, 10, 11, 5, 13, 6, 7, 8, 9};
    int ccd = cpmm2ccd[cpmm];

    // Compute the line rate, convert to seconds, and multiply by the
    // downtrack summing
    double unBinnedRate = (74.0 + (deltaLineTimerCount / 16.0)) / 1000000.0;
    double lineRate = unBinnedRate * binMode;

    // Convert the spacecraft clock count to ephemeris time
    SpiceDouble et;
    // The -74999 is the code to select the transformation from
    // high-precision MRO SCLK to ET
    et = getClockTime(stime, -74999).Et();

    // Adjust the start time so that it is the effective time for
    // the first line in the image file.  Note that on 2006-03-29, this
    // time is now subtracted as opposed to adding it.  The computed start
    // time in the EDR is at the first serial line.
    et -= unBinnedRate * (((double) tdiMode / 2.0) - 0.5);
    // Effective observation
    // time for all the TDI lines used for the
    // first line before doing binning
    et += unBinnedRate * (((double) binMode / 2.0) - 0.5);
    // Effective observation time of the first line
    // in the image file, which is possibly binned

    // Compute effective line number within the CCD (in pixels) for the
    // given TDI mode.
    //   This is the "centered" 0-based line number, where line 0 is the
    //   center of the detector array and line numbers decrease going
    //   towards the serial readout.  Line number +64 sees a spot
    //   on the ground before line number 0 or -64.
    double ccdLine_c = -64.0 + ((double) tdiMode / 2.0);

    // Setup detector map for transform of image pixels to detector position
    //      CameraDetectorMap *detectorMap =
    //        new LineScanCameraDetectorMap(this,et,lineRate);
    LineScanCameraDetectorMap *detectorMap =
      new LineScanCameraDetectorMap(this, et, lineRate);
    detectorMap->SetDetectorSampleSumming(binMode);
    detectorMap->SetDetectorLineSumming(binMode);
    if(chan == 0) {
      detectorMap->SetStartingDetectorSample(1025.0);
    }

    // Setup focal plane map for transform of detector position to
    // focal plane x/y.  This will read the appropriate CCD
    // transformation coefficients from the instrument kernel
    CameraFocalPlaneMap *focalMap =
      new CameraFocalPlaneMap(this, -74600 - ccd);
    focalMap->SetDetectorOrigin(1024.5, 0.0);
    focalMap->SetDetectorOffset(0.0, ccdLine_c);

    // Setup distortion map.  This will read the optical distortion
    // coefficients from the instrument kernel
    CameraDistortionMap *distortionMap = new CameraDistortionMap(this);
    distortionMap->SetDistortion(naifIkCode());

    // Setup the ground and sky map to transform undistorted focal
    // plane x/y to lat/lon or ra/dec respectively.
    new LineScanCameraGroundMap(this);
    new LineScanCameraSkyMap(this);

    LoadCache();
    NaifStatus::CheckErrors();
  }

  //! Destroys the HiriseCamera object
  HiriseCamera::~HiriseCamera() {}
}


/**
 * This is the function that is called in order to instantiate a
 * HiriseCamera object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* HiriseCamera
 * @internal
 *   @history 2011-05-03 Jeannie Walldren - Removed Mro namespace.
 */
extern "C" Isis::Camera *HiriseCameraPlugin(Isis::Cube &cube) {
  return new Isis::HiriseCamera(cube);
}
