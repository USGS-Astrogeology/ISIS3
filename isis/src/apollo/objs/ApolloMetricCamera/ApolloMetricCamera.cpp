/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ApolloMetricCamera.h"

#include "ApolloMetricDistortionMap.h"

#include <QString>

#include "CameraDetectorMap.h"
#include "CameraFocalPlaneMap.h"
#include "CameraGroundMap.h"
#include "CameraSkyMap.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "NaifStatus.h"
#include "ReseauDistortionMap.h"

using namespace std;
namespace Isis {
  /**
   * Constructs an Apollo Metric Camera object using the image labels.
   *
   * @param cube An Apollo Metric image.
   *
   * @internal
   *   @history 2011-01-14 Travis Addair - Added new CK/SPK accessor methods.
   *   @history 2011-05-03 Jeannie Walldren - Added documentation.
   *   @history 2014-01-17 Kris Becker - Set CkReferenceID to J2000 to resolve
   *                         problem with ckwriter
   */
  ApolloMetricCamera::ApolloMetricCamera(Cube &cube) : FramingCamera(cube) {
    NaifStatus::CheckErrors();

    m_instrumentNameLong = "Metric Camera";
    m_instrumentNameShort = "Metric";

    Pvl &lab = *cube.label();
    const PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);

    // The Spacecraft Name should be either Apollo 15, 16, or 17.  The name
    // itself could be formatted any number of ways, but the number contained
    // in the name should be unique between the missions
    // We use the naifIkCode instead now
    //QString spacecraft = inst.findKeyword("SpacecraftName")[0];
    // Apollo 15 Pan naif code = -917240
    if (naifIkCode() == -915240) {
      p_ckFrameId = -915240;
      p_ckReferenceId = 1;
      p_spkTargetId = -915;
      m_spacecraftNameLong = "Apollo 15";
      m_spacecraftNameShort = "Apollo15";
    }
    // Apollo 16 Pan naif code = -917240
    else if (naifIkCode() == -916240) {
      p_ckFrameId = -916240;
      p_ckReferenceId = 1;
      p_spkTargetId = -916;
      m_spacecraftNameLong = "Apollo 16";
      m_spacecraftNameShort = "Apollo16";
    }
    // Apollo 17 Pan naif code = -917240
    else if (naifIkCode() == -917240) {
      p_ckFrameId = -917240;
      p_ckReferenceId = 1;
      p_spkTargetId = -917;
      m_spacecraftNameLong = "Apollo 17";
      m_spacecraftNameShort = "Apollo17";
    }
    else {
      std::string msg = "File does not appear to be an Apollo image";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Get the camera characteristics
    SetFocalLength();
    SetPixelPitch();

    // Setup detector map
    new CameraDetectorMap(this);

    // Setup focal plane map
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(this, naifIkCode());
    focalMap->SetDetectorOrigin(ParentSamples() / 2.0, ParentLines() / 2.0);

    QString ppKey("INS" + QString::number(naifIkCode()) + "_PP");
    QString odkKey("INS" + QString::number(naifIkCode()) + "_OD_K");
    QString decenterKey("INS" + QString::number(naifIkCode()) + "_DECENTER");

    new ApolloMetricDistortionMap(this, getDouble(ppKey, 0),
                                  getDouble(ppKey, 1), getDouble(odkKey, 0), getDouble(odkKey, 1),
                                  getDouble(odkKey, 2), getDouble(decenterKey, 0),
                                  getDouble(decenterKey, 1), getDouble(decenterKey, 2));

    // Setup the ground and sky map
    new CameraGroundMap(this);
    new CameraSkyMap(this);

    // Create a cache and grab spice info since it does not change for
    // a framing camera (fixed spacecraft position and pointing)
    // Convert the start time to et
    setTime(QString::fromStdString(inst["StartTime"]));
    LoadCache();
    NaifStatus::CheckErrors();
  }


  /**
   * Returns the shutter open and close times. The user should pass in the
   * exposure duration in seconds and the StartTime keyword value, converted to
   * ephemeris time. The StartTime keyword value from the labels represents the
   * shutter center time of the observation. To find the shutter open and close
   * times, half of the exposure duration is subtracted from and added to the
   * input time parameter, respectively.  This method overrides the
   * FramingCamera class method.
   * @b Note: Apollo did not provide exposure duration in the support data.
   *
   * @param exposureDuration Exposure duration, in seconds.
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
  pair<iTime, iTime> ApolloMetricCamera::ShutterOpenCloseTimes(double time,
                                                               double exposureDuration) {
    pair<iTime, iTime> shuttertimes;
    // To get shutter start (open) time, subtract half exposure duration
    shuttertimes.first = time - (exposureDuration / 2.0);
    // To get shutter end (close) time, add half exposure duration
    shuttertimes.second = time + (exposureDuration / 2.0);
    return shuttertimes;
  }

} // End Apollo namespace


/**
 * This is the function that is called in order to instantiate an
 * ApolloMetricCamera object.
 *
 * @param cube Cube
 *
 * @return Camera* ApolloMetricCamera
 * @internal
 *   @history 2011-05-03 Jeannie Walldren - Added documentation. Removed Apollo
 *            namespace.
 */
extern "C" Isis::Camera *ApolloMetricCameraPlugin(Isis::Cube &cube) {
  return new Isis::ApolloMetricCamera(cube);
}
