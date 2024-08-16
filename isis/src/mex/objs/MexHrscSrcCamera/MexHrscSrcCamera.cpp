/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "MexHrscSrcCamera.h"

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
   * Constructs an Mex HRSC SRC Framing Camera object.
   *
   * @param lab Pvl label from a Mex HRSC SRC Framing Camera image.
   *
   * @author Stuart Sides
   *
   * @internal
   */

  MexHrscSrcCamera::MexHrscSrcCamera(Cube &cube) : FramingCamera(cube) {
    m_instrumentNameLong = "Super Resolution Channel";
    m_instrumentNameShort = "SRC";
    m_spacecraftNameLong = "Mars Express";
    m_spacecraftNameShort = "MEX";

    NaifStatus::CheckErrors();

    SetFocalLength(Spice::getDouble("INS" + toString(naifIkCode()) + "_FOCAL_LENGTH"));

    // For setting the pixel pitch, the Naif keyword PIXEL_SIZE is used instead of the ISIS
    // default of PIXEL_PITCH, so set the value directly.
    QString pp = "INS" + toString(naifIkCode()) + "_PIXEL_SIZE";
    double pixelPitch = Spice::getDouble(pp);
    pixelPitch /= 1000.0;
    SetPixelPitch(pixelPitch);

    // SRC doesn't appear to use any summing modes
    CameraDetectorMap *detectorMap = new CameraDetectorMap(this);
    detectorMap->SetDetectorSampleSumming(1);
    detectorMap->SetDetectorLineSumming(1);

    // Setup focal plane map. The class will read data from the instrument addendum kernel to pull
    // out the affine transforms from detector samp,line to focal plane x,y.
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(this, naifIkCode());

    // The boresight position recorded in the IK is zero-based and therefore needs to be adjusted
    // for ISIS
    double boresightSample = Spice::getDouble("INS" + toString(naifIkCode()) + "_CCD_CENTER",0) + 1.0;
    double boresightLine = Spice::getDouble("INS" + toString(naifIkCode()) + "_CCD_CENTER",1) + 1.0;
    focalMap->SetDetectorOrigin(boresightSample,boresightLine);

    // The distortion is documented as near 1 pixel at the corners. This is less than the
    // point spread, so zero distortion is used
    new CameraDistortionMap(this);

    // Setup the ground and sky map
    new CameraGroundMap(this);
    new CameraSkyMap(this);

    // The observation start time and clock count for SRC are based on the center of the exposure.
    Pvl &lab = *cube.label();
    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);

    iTime startTime;
    startTime.setUtc(QString::fromStdString(inst["StartTime"]));   
    setTime(startTime);

    // Internalize all the NAIF SPICE information into memory.
    LoadCache();
    NaifStatus::CheckErrors();
  }


  /**
   * Returns the shutter open and close times.
   *
   * @param time The SpacecraftClockStartCount converted to ephemeris time.
   * @param exposureDuration ExposureDuration keyword value from the labels, converted to
   *                         seconds.
   *
   * @return @b pair < @b iTime, @b iTime > The first value is the shutter
   *         open time and the second is the shutter close time.
   *
   */
  pair<iTime, iTime> MexHrscSrcCamera::ShutterOpenCloseTimes(double time,
                                                             double exposureDuration) {
    return FramingCamera::ShutterOpenCloseTimes(time - exposureDuration / 2.0, exposureDuration);
  }
}


/**
 * This is the function that is called in order to instantiate a MexHrscSrcCamera
 * object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* MexHrscSrcCamera
 */
extern "C" Isis::Camera *MexHrscSrcCameraPlugin(Isis::Cube &cube) {
  return new Isis::MexHrscSrcCamera(cube);
}
