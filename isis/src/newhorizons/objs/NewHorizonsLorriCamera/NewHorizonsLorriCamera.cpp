/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "NewHorizonsLorriCamera.h"

#include <QString>

#include "CameraDetectorMap.h"
#include "CameraDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "CameraGroundMap.h"
#include "CameraSkyMap.h"
#include "IString.h"
#include "iTime.h"
#include "NaifStatus.h"
#include "NewHorizonsLorriDistortionMap.h"

using namespace std;

namespace Isis {
  /**
   * Constructs a New Horizons LORRI Framing Camera object. The LORRI camera has two summing modes,
   * 1x1 and 4x4. The handeling of these two modes is done through two different Naif codes,
   * -98301 and -98302 respectivly. This camera model code handles both cameras. The
   * IK and IAK kernels must supply keyword values for both codes. The cube labels show a summing
   * mode, but the value is not used
   *
   * @param lab Pvl label from a New Horizons LORRI Framing Camera image.
   *
   * @author Stuart Sides
   *
   * @internal
   */

  NewHorizonsLorriCamera::NewHorizonsLorriCamera(Cube &cube) : FramingCamera(cube) {
    m_instrumentNameLong = "Long Range Reconnaissance Imager";
    m_instrumentNameShort = "LORRI";
    m_spacecraftNameLong = "New Horizons";
    m_spacecraftNameShort = "NewHorizons";

    NaifStatus::CheckErrors();

    // The LORRI focal length is fixed and is designed not to change throught the operational
    // temperature. The NAIF code, set in the ISIS labels, will be used to read a single focal
    // length from the SPICE kernels. Version 100 of the Lorri IK uses meters for focal length,
    // units, version 200 uses mm, and has a units keyword.
    QString unitsKey = "INS" + QString::number(naifIkCode()) + "_FOCAL_LENGTH_UNITS";
    QString focalLengthUnits = Spice::getString(unitsKey);
    if (focalLengthUnits != "mm") {
      QString msg = QObject::tr("SPICE keyword [%1] is expected to be mm. [%2] was found instead").
                    arg(unitsKey).arg(focalLengthUnits);
       throw IException(IException::User, msg.toStdString(), _FILEINFO_);
    }
    SetFocalLength(Spice::getDouble("INS" + QString::number(naifIkCode()) + "_FOCAL_LENGTH"));

    // For setting the pixel pitch, the Naif keyword PIXEL_SIZE is used instead of the ISIS
    // default of PIXEL_PITCH, so set the value directly.
    QString pp = "INS" + QString::number(naifIkCode()) + "_PIXEL_SIZE";
    double pixelPitch = Spice::getDouble(pp);
    pixelPitch /= 1000.0;
    SetPixelPitch(pixelPitch);

    // Since the two summing modes are handeled via different Naif codes, force the summing modes
    // to 1 for both the 1x1 and 4x4 modes
    CameraDetectorMap *detectorMap = new CameraDetectorMap(this);
    detectorMap->SetDetectorSampleSumming(1);
    detectorMap->SetDetectorLineSumming(1);

    // Setup focal plane map. The class will read data from the instrument addendum kernel to pull
    // out the affine transforms from detector samp,line to focal plane x,y.
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(this, naifIkCode());

    // The boresight position recorded in the IK is zero-based and therefore needs to be adjusted
    // for ISIS
    double boresightSample = Spice::getDouble("INS" + QString::number(naifIkCode()) + "_CCD_CENTER",0) + 1.0;
    double boresightLine = Spice::getDouble("INS" + QString::number(naifIkCode()) + "_CCD_CENTER",1) + 1.0;
    focalMap->SetDetectorOrigin(boresightSample,boresightLine);

    // Setup distortion map. Start by reading the distortion coefficient from the instrument kernel.
    // Then construct the distortion model.
    // Uncomment the 3 lines below to use the default distortion map (i.e., no distortion)
    //    new CameraDistortionMap(this, -1);
    //    CameraDistortionMap *distortionMap = new CameraDistortionMap(this, -1);
    //    distortionMap->SetDistortion(naifIkCode());
    // Changed the SPICE keyword names to work with LORRI IK version 200
    QString e2("INS" + QString::number(naifIkCode()) + "_OOC_EM");
    QString e5("INS" + QString::number(naifIkCode()) + "_OOC_EM");
    QString e6("INS" + QString::number(naifIkCode()) + "_OOC_EM");
    new NewHorizonsLorriDistortionMap(this, getDouble(e2, 0), getDouble(e5, 1),
                                      getDouble(e6, 2), -1);

    // Setup the ground and sky map
    new CameraGroundMap(this);
    new CameraSkyMap(this);

    // The observation start time and clock count for LORRI are based on the center of the exposure.
    Pvl &lab = *cube.label();
    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);
    QString clockCount = QString::fromStdString(inst["SpacecraftClockStartCount"]);
    double et = getClockTime(clockCount).Et();
    double exposureDuration = (double)inst["ExposureDuration"] / 1000.0;

    pair<iTime, iTime> startStop = ShutterOpenCloseTimes(et, exposureDuration);
    setTime(et);

    // Internalize all the NAIF SPICE information into memory.
    LoadCache();
    NaifStatus::CheckErrors();
  }

  /**
   * Returns the shutter open and close times.  The LORRI camera doesn't use a shutter to start and
   * end an observation, but this function is being used to get the observation start and end times,
   * so we will simulate a shutter.
   *
   * @param exposureDuration ExposureDuration keyword value from the labels,
   *                         converted to seconds.
   * @param time The StartTime keyword value from the labels, converted to
   *             ephemeris time.
   *
   * @return @b pair < @b iTime, @b iTime > The first value is the shutter
   *         open time and the second is the shutter close time.
   *
   */
  pair<iTime, iTime> NewHorizonsLorriCamera::ShutterOpenCloseTimes(double time,
                                                         double exposureDuration) {
    return FramingCamera::ShutterOpenCloseTimes(time - exposureDuration / 2.0, exposureDuration);
  }
}

/**
 * This is the function that is called in order to instantiate a NewHorizonsLorriCamera
 * object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* NewHorizonsLorriCamera
 * @internal
 *   @history 2011-05-03 Jeannie Walldren - Added documentation.  Removed Dawn
 *            namespace.
 */
extern "C" Isis::Camera *NewHorizonsLorriCameraPlugin(Isis::Cube &cube) {
  return new Isis::NewHorizonsLorriCamera(cube);
}
