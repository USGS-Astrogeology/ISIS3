/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "HayabusaNirsCamera.h"

#include <QString>

#include "CameraDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "CameraGroundMap.h"
#include "CameraSkyMap.h"
#include "IString.h"
#include "iTime.h"
#include "NaifStatus.h"
#include "NirsDetectorMap.h"

using namespace std;

namespace Isis {
  /**
   * Constructs a HayabusaNirsCamera object using the image labels.
   *
   * @param lab Pvl label from a Hayabusa NIRS image.
   *
   * @internal
   */
  HayabusaNirsCamera::HayabusaNirsCamera(Cube &cube) : FramingCamera(cube) {
    m_instrumentNameLong = "Near InfraRed Spectrometer";
    m_instrumentNameShort = "NIRS";
    m_spacecraftNameLong = "Hayabusa";
    m_spacecraftNameShort = "Hayabusa";

    NaifStatus::CheckErrors();
    Pvl &lab = *cube.label();

    // Get focal length and pixel pitch from IAK
    SetFocalLength();
    SetFocalLength(FocalLength() * 1000.0);  // Convert from meters to mm
    SetPixelPitch();

    // Get the start time in et
    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);

    // get the start and stop times
    QString startTime = QString::fromStdString(inst["SpacecraftClockStartCount"]);
    QString stopTime = QString::fromStdString(inst["SpacecraftClockStopCount"]);
    iTime etStart = getClockTime(startTime);
    iTime etStop = getClockTime(stopTime);

    double exposureDuration = etStop - etStart;
    iTime centerTime  = etStart + (exposureDuration / 2.0);

    // Setup focal plane map
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(this, naifIkCode());

    // lines and samples added to the pvl in the order you
    // call getDouble()
    double bLines = Spice::getDouble("INS" + QString::number(naifIkCode()) + "_BORESIGHT_LINE");
    double bSamples = Spice::getDouble("INS" + QString::number(naifIkCode()) + "_BORESIGHT_SAMPLE");

    focalMap->SetDetectorOrigin(bSamples, bLines);

    // Setup detector map
    CameraDetectorMap *detMap = new NirsDetectorMap(exposureDuration, this);
    detMap->SetStartingDetectorSample(0);
    detMap->SetStartingDetectorLine(0);

    // Setup distortion map
    new CameraDistortionMap(this);

    // Setup the ground and sky map
    new CameraGroundMap(this);
    new CameraSkyMap(this);

    setTime(centerTime);
    LoadCache();
    NaifStatus::CheckErrors();
  }


  /**
   * Destroys a HayabusaNirsCamera object
   */
  HayabusaNirsCamera::~HayabusaNirsCamera() {

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
  pair<iTime, iTime> HayabusaNirsCamera::ShutterOpenCloseTimes(double time,
                                                               double exposureDuration) {
    return FramingCamera::ShutterOpenCloseTimes(time, exposureDuration);
  }


  /**
   * @brief Returns the pixel ifov offsets from center of pixel.
   *
   * Returns the pixel ifov offset from the center of pixel for 8 points along
   * each edge of the fov. The Camera implementation only uses the four
   * corners.  Because the FOV of each pixel is so large, this uses 8 points on
   * each edge for a total of 28 points.
   *
   * @return @b QList<QPointF> A list of offset values to add to the look vector.
   */
   QList<QPointF> HayabusaNirsCamera::PixelIfovOffsets() {

     QList<QPointF> offsets;

     //  Create 8 pts on each edge of pixel
     int npts = 8;

     //  Top edge of pixel
     for (double x = -PixelPitch() / 2.0; x <= PixelPitch() / 2.0; x += PixelPitch() / (npts-1)) {
       offsets.append(QPointF(x, -PixelPitch() / 2.0));
     }
     //  Right edge of pixel
     for (double y = -PixelPitch() / 2.0; y <= PixelPitch() / 2.0; y += PixelPitch() / (npts-1)) {
       offsets.append(QPointF(PixelPitch() / 2.0, y));
     }
     //  Bottom edge of pixel
     for (double x = PixelPitch() / 2.0; x >= -PixelPitch() / 2.0; x -= PixelPitch() / (npts-1)) {
       offsets.append(QPointF(x, PixelPitch() / 2.0));
     }
     //  Left edge of pixel
     for (double y = PixelPitch() / 2.0; y >= -PixelPitch() / 2.0; y -= PixelPitch() / (npts-1)) {
       offsets.append(QPointF(-PixelPitch() / 2.0, y));
     }

     return offsets;
   }
}


/**
 * This is the function that is called in order to instantiate a HayabusaNirsCamera
 * object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* HayabusaNirsCamera
 * @internal
 *   @history 2013-11-27 Kris Becker - Original Version
 */
extern "C" Isis::Camera *HayabusaNirsCameraPlugin(Isis::Cube &cube) {
  return new Isis::HayabusaNirsCamera(cube);
}
