/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include "VikingCamera.h"

#include <SpiceUsr.h>
#include <SpiceZfc.h>
#include <SpiceZmc.h>

#include <QString>

#include "CameraDetectorMap.h"
#include "CameraFocalPlaneMap.h"
#include "CameraGroundMap.h"
#include "CameraSkyMap.h"
#include "FileName.h"
#include "IString.h"
#include "iTime.h"
#include "NaifStatus.h"
#include "ReseauDistortionMap.h"

using namespace std;

namespace Isis {
  /**
   * Constructs a Viking Camera Model.
   *
   * @param cube The image cube.
   *
   * @throw iException::User - "File does not appear to be a Viking image.
   *        Invalid InstrumentId."
   * @throw iException::User - "File does not appear to be a Viking image.
   *        Invalid SpacecraftName."
   * @internal
   *   @history 2011-05-03 Jeannie Walldren - Added NAIF error check. Moved
   *                          offset calculation to ShutterOpenCloseTimes()
   *                          method and added a call to the new method.
   */
  VikingCamera::VikingCamera(Cube &cube) : FramingCamera(cube) {
    NaifStatus::CheckErrors();
    // Set the pixel pitch
    SetPixelPitch(1.0 / 85.0);

    // Find out what camera is being used, and set the focal length, altinstcode,
    // raster orientation, cone, crosscone, and camera
    Pvl &lab = *cube.label();
    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);
    QString spacecraft = QString::fromStdString(inst["SPACECRAFTNAME"]);
    QString instId = QString::fromStdString(inst["INSTRUMENTID"]);
    QString cam;
    int spn;
    double raster, cone, crosscone;
    int altinstcode = 0;
    if(spacecraft == "VIKING_ORBITER_1") {
      p_ckFrameId = -27000;
      p_spkTargetId = -27;
      m_spacecraftNameLong = "Viking Orbiter 1";
      m_spacecraftNameShort = "Viking1";

      spn = 1;
      altinstcode = -27999;
      if(instId == "VISUAL_IMAGING_SUBSYSTEM_CAMERA_A") {
        cam = "1a";
        SetFocalLength(474.398);
        crosscone = -0.707350;
        cone = -0.007580;
        raster = 89.735690;
        m_instrumentNameLong = "Visual Imaging Subsystem Camera A";
        m_instrumentNameShort = "VISA";
      }
      else if(instId == "VISUAL_IMAGING_SUBSYSTEM_CAMERA_B") {
        cam = "1b";
        SetFocalLength(474.448);
        crosscone = 0.681000;
        cone = -0.032000;
        raster = 90.022800;
        m_instrumentNameLong = "Visual Imaging Subsystem Camera B";
        m_instrumentNameShort = "VISB";
      }
      else {
        std::string msg = "File does not appear to be a Viking image. InstrumentId ["
            + instId + "] is invalid Viking value.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    else if(spacecraft == "VIKING_ORBITER_2") {
      p_ckFrameId = -30000;
      p_spkTargetId = -30;
      m_spacecraftNameLong = "Viking Orbiter 2";
      m_spacecraftNameShort = "Viking2";

      spn = 2;
      altinstcode = -30999;
      if(instId == "VISUAL_IMAGING_SUBSYSTEM_CAMERA_A") {
        cam = "2a";
        SetFocalLength(474.610);
        crosscone = -0.679330;
        cone = -0.023270;
        raster = 89.880691;
        m_instrumentNameLong = "Visual Imaging Subsystem Camera A";
        m_instrumentNameShort = "VISA";
      }
      else if(instId == "VISUAL_IMAGING_SUBSYSTEM_CAMERA_B") {
        cam = "2b";
        SetFocalLength(474.101);
        crosscone = 0.663000;
        cone = -0.044000;
        raster = 89.663790;
        m_instrumentNameLong = "Visual Imaging Subsystem Camera B";
        m_instrumentNameShort = "VISB";
      }
      else {
        std::string msg = "File does not appear to be a Viking image. InstrumentId ["
            + instId + "] is invalid Viking value.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    else {
      std::string msg = "File does not appear to be a Viking image. SpacecraftName ["
          + spacecraft + "] is invalid Viking value.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // DOCUMENTATION FROM ISIS2 lev1u_vik_vis_routines.c:
    /*****************************************************************************
     * Calculate the START_TIME keyword (time at middle of exposure in this case)*
     * value from FSC to get fractional seconds (PDS START_TIME provided is only *
     * to the nearest whole second).  The algorithm below was extracted from the *
     * NAIF document Viking Orbiter Time Tag Analysis and Restoration by Boris   *
     * Semenov and Chuck Acton.                                                  *
     *       1.  Get exposure duration from labels to center the time            *
     *       2.  Get FSC from IMAGE_NUMBER on labels to use as spacecraftClock   *
     *       3.  Load the appropriate FSC spacecraft clock kernel based on       *
     *           the spacecraft (Viking Orbiter 1 or Viking Orbiter 2)           *
     *       4.  Load a leap second kernel                                       *
     *       5.  Convert FSC to et                                               *
     *       6.  Add the offsets to get to midexposure                           *
     *       7.  Convert et to UTC calendar format and write to labels as        *
     *           START_TIME                                                      *
     *****************************************************************************/

    // Get clock count and convert it to a time
    QString spacecraftClock = QString::fromStdString(inst["SpacecraftClockCount"]);
    double etClock = getClockTime(spacecraftClock, altinstcode).Et();

    // exposure duration keyword value is measured in seconds
    double exposureDuration = inst["ExposureDuration"];

    // Calculate and load the euler angles
    SpiceDouble CP[3][3];
    eul2m_c((SpiceDouble)raster * rpd_c(), (SpiceDouble)cone * rpd_c(),
            (SpiceDouble) - crosscone * rpd_c(), 3, 2, 1, CP);

    //    LoadEulerMounting(CP);

    pair<iTime, iTime> shuttertimes = ShutterOpenCloseTimes(etClock, exposureDuration);

    // find center shutter time
    double centerTime = shuttertimes.first.Et() + exposureDuration / 2.0;
    char timepds[25];
    et2utc_c(centerTime, "ISOC", 3, 25, timepds);
    utc2et_c(timepds, &centerTime);

    // Setup detector map
    new CameraDetectorMap(this);

    // Setup focal plane map, and detector origin
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(this, naifIkCode());
    focalMap->SetDetectorOrigin(602.0, 528.0);

    // Setup distortion map
    QString fname = FileName("$viking" + toString(spn) + "/reseaus/vik" + cam
                             + "MasterReseaus.pvl").expanded();
    new ReseauDistortionMap(this, lab, fname);

    // Setup the ground and sky map
    new CameraGroundMap(this);
    new CameraSkyMap(this);

    setTime(centerTime);
    LoadCache();
    NaifStatus::CheckErrors();
  }


  /**
   * Returns the shutter open and close times. The user should pass in the
   * ExposureDuration keyword value and the SpacecraftClockCount keyword value,
   * converted to ephemeris time. To find the shutter open time, 2 offset values
   * must be added to the SpacecraftClockCount keyword value. To find the
   * shutter close time, the exposure duration is added to the calculated
   * shutter open time. This method overrides the FramingCamera class method.
   *
   * @param exposureDuration ExposureDuration keyword value from the labels, in
   *                         seconds.
   * @param time The SpacecraftClockCount keyword value from the labels,
   *             converted to ephemeris time.
   *
   * @return @b pair < @b iTime, @b iTime > The first value is the shutter open
   *         time and the second is the shutter close time.
   *
   * @author 2011-05-03 Jeannie Walldren
   * @internal
   *   @history 2011-05-03 Jeannie Walldren - Original version.
   */
  pair<iTime, iTime> VikingCamera::ShutterOpenCloseTimes(double time,
                                                         double exposureDuration) {
    pair<iTime, iTime> shuttertimes;
    double offset1;
    if (exposureDuration <= .420) {
      offset1 = 7.0 / 8.0 * 4.48;    //4.48 seconds = nomtick
    }
    else {
      offset1 = 3.0 / 8.0 * 4.48;
    }
    double offset2 = 1.0 / 64.0 * 4.48;

    // set private variables inherited from Spice class
    shuttertimes.first = time + offset1 + offset2;
    shuttertimes.second = shuttertimes.first.Et() + exposureDuration;
    return shuttertimes;
  }
}


/**
 * This is the function that is called in order to instantiate a VikingCamera
 * object.
 *
 * @param cube The image cube
 *
 * @return Isis::Camera* VikingCamera
 */
extern "C" Isis::Camera *VikingCameraPlugin(Isis::Cube &cube) {
  return new Isis::VikingCamera(cube);
}
