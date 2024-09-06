/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "DawnFcCamera.h"
#include "DawnFcDistortionMap.h"

#include <QString>

#include "CameraDetectorMap.h"
#include "CameraFocalPlaneMap.h"
#include "CameraGroundMap.h"
#include "CameraSkyMap.h"
#include "IString.h"
#include "iTime.h"
#include "NaifStatus.h"

using namespace std;

namespace Isis {
  /**
   * Constructs a Dawn Framing Camera object.
   *
   * @param lab Pvl label from a Dawn Framing Camera image.
   *
   * @author Janet Barret
   * @internal
   *   @history 2011-05-03 Jeannie Walldren - Added NAIF error check.  Added
   *                          call to ShutterOpenCloseTimes() method.
   *   @history 2011-07-26 Jeff Anderson - Modified to support varying focal length
   *                                       and optical distortion based on filter
   */
  DawnFcCamera::DawnFcCamera(Cube &cube) : FramingCamera(cube) {
    NaifStatus::CheckErrors();

    m_spacecraftNameLong = "Dawn";
    m_spacecraftNameShort = "Dawn";

    int ikCode = naifIkCode();

    // http://naif.jpl.nasa.gov/pub/naif/DAWN/kernels/ik/dawn_fc_v10.ti
    // FC1 has instrument codes -203110 to -203119 (to reference filters and radiator as well
    if (ikCode <= -203110 && ikCode > -203120) {
      m_instrumentNameLong = "Framing Camera 1";
      m_instrumentNameShort = "FC1";
    }
    // Likewise, FC2 has instrument codes -203120 to -203129
    else if (ikCode <= -203120 && ikCode > -203130) {
      m_instrumentNameLong = "Framing Camera 2";
      m_instrumentNameShort = "FC2";
    }
    else {
      std::string msg = "File does not appear to be a Dawn Framing Camera image. ";
      msg += "(" + QString::number(ikCode) + " is not a Dawn FC instrument code)";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // The focal length is dependent on wave length.  The NAIF code set
    // in the ISIS labels will read the correct focal length from the
    // Instrument kernel (IK)
    SetFocalLength();

    // The pixel pitch is not square for the FC instrument.  It is only
    // slightly rectangular 14 vs 14.088 microns.  ISIS only supports
    // square CCD pixels. The impact by calling SetPixelPitch means the
    // computation of pixel resolution (on the ground) will be slightly off.
    // We will spread the error by setting the pixel pitch to the average of the
    // two. The important part is the translation from detector coordinates
    // to focal plane coordinates.  Fortunately the affine transform will
    // allow us to have different sized detector pixels.  Therefore the
    // only problem with ISIS is the pixel resolution computation.  This may
    // be something we want to refactor later in case future instrument have
    // non-square detectors.
    QString keyword = "INS" + toString(naifIkCode()) + "_PIXEL_SIZE";
    double pixelPitch = (Spice::getDouble(keyword, 0) + Spice::getDouble(keyword, 1)) / 2.0;
    pixelPitch /= 1000.0;
    SetPixelPitch(pixelPitch);

    // We have not seen images or tested images with summing mode or
    // starting sample/line coordinates. Because of this uncertainty we will
    // throw an error the image size is not 1024 x 1024.  If in the future we
    // encounter such an image then inputs to the detector map will need
    // to be given
    if ((ParentLines() != 1024) || (ParentSamples() != 1024)) {
      string msg = "The ISIS Dawn FC model expects the image size to be 1024x1024";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    CameraDetectorMap *detectorMap = new CameraDetectorMap(this);
    detectorMap->SetDetectorSampleSumming(1);
    detectorMap->SetDetectorLineSumming(1);

    // Setup focal plane map. The class will read the instrument addendum kernel to pull out the affine tronsforms
    // from detector samp,line to focal plane x,y.  This is where the non-square detector size are read and utilized.
    // The boresight position recorded in the IK is zero-based and therefore needs to be adjusted for ISIS
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(this, naifIkCode());
    double boresightSample = Spice::getDouble("INS" + toString(naifIkCode()) + "_CCD_CENTER",0) + 1.0;
    double boresightLine   = Spice::getDouble("INS" + toString(naifIkCode()) + "_CCD_CENTER",1) + 1.0;
    focalMap->SetDetectorOrigin(boresightSample,boresightLine);

    // Setup distortion map.  Start by reading the distortion coefficient from the instrument kernel.  Then
    // construct the distortion model.  Note the distortion model code is copied from the RadialDistortionMap
    // class and reversed.  TODO:  Check with Ken Edmundson to see if we can just read from IK and pass 1/K
    // to the original RadialDistortionMap which would allow us to delete the DawnFcDistortionMap
    double k = Spice::getDouble("INS" + toString(naifIkCode()) + "_RAD_DIST_COEFF");
    new DawnFcDistortionMap(this,k);

    // Setup the ground and sky map
    new CameraGroundMap(this);
    new CameraSkyMap(this);

    // Get the timing information of the observation.  Start by computing the
    // beginning time of the exposure.  This will be based off the
    // spacecraft clock start count. There is a delay of 193 ms while the
    // CCD is discharged or cleared.  Finally the exporsure information
    // needs to be obtained.
    Pvl &lab = *cube.label();
    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);
    QString stime = QString::fromStdString(inst["SpacecraftClockStartCount"]);
    double et = getClockTime(stime).Et();
    et += 193.0 / 1000.0;
    double exposureDuration = (double)inst["ExposureDuration"] / 1000.0;
    pair<iTime, iTime> shuttertimes = ShutterOpenCloseTimes(et, exposureDuration);
    iTime centerTime = et + exposureDuration / 2.0;
    setTime(centerTime);

    // Internalize all the NAIF SPICE information into memory.
    LoadCache();
    NaifStatus::CheckErrors();
  }


  /**
   * Returns the shutter open and close times.  The user should pass in the
   * ExposureDuration keyword value, converted from milliseconds to seconds, and
   * the StartTime keyword value, converted to ephemeris time. The StartTime
   * keyword value from the labels represents the shutter open time of the
   * observation. This method uses the FramingCamera class implementation,
   * returning the given time value as the shutter open and the sum of the time
   * value and exposure duration as the shutter close.
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
  pair<iTime, iTime> DawnFcCamera::ShutterOpenCloseTimes(double time,
                                                         double exposureDuration) {
    return FramingCamera::ShutterOpenCloseTimes(time, exposureDuration);
  }
}


/**
 * This is the function that is called in order to instantiate a DawnFcCamera
 * object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* DawnFcCamera
 * @internal
 *   @history 2011-05-03 Jeannie Walldren - Added documentation.  Removed Dawn
 *            namespace.
 */
extern "C" Isis::Camera *DawnFcCameraPlugin(Isis::Cube &cube) {
  return new Isis::DawnFcCamera(cube);
}
