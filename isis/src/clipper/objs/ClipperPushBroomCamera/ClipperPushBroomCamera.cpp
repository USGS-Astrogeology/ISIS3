/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ClipperPushBroomCamera.h"

#include "CameraDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "iTime.h"
#include "LineScanCameraGroundMap.h"
#include "LineScanCameraSkyMap.h"
#include "NaifStatus.h"

namespace Isis {
  /**
   * Constructs a ClipperPushBroomCamera object using the image labels.
   *
   * @param Cube &cube Clipper EIS image.
   */
  ClipperPushBroomCamera::ClipperPushBroomCamera(Cube &cube) : LineScanCamera(cube) {

    m_spacecraftNameLong = "Europa Clipper";
    m_spacecraftNameShort = "Clipper";

    int frameCode = naifIkCode();

    if (frameCode == -159103) {
      m_instrumentNameLong  = "Europa Imaging System Push Broom Narrow Angle Camera";
      m_instrumentNameShort = "EIS-PBNAC";
    }
    else if (frameCode == -159104) {
      m_instrumentNameLong  = "Europa Imaging System Push Broom Wide Angle Camera";
      m_instrumentNameShort = "EIS-PBWAC";
    }
    else {
      QString msg = "Unable to construct Clipper Push Broom camera model. "
                    "Unrecognized NaifFrameCode [" + toString(frameCode) + "].";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    NaifStatus::CheckErrors();

    Pvl &lab = *cube.label();

    PvlGroup &bandBin = lab.findGroup("BandBin", Pvl::Traverse);
    QString key = "INS" + toString(naifIkCode()) + "_" + bandBin["FilterName"][0] + "_FOCAL_LENGTH";
    SetFocalLength(Spice::getDouble(key));

    SetPixelPitch();

    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);
    QString startTime = inst["StartTime"];
    iTime etStart(startTime);

    ReadLineRates(lab.fileName());

    // set up detector map
    new VariableLineScanCameraDetectorMap(this, p_lineRates);

    // Turn off the aberration corrections for the instrument position object
    instrumentPosition()->SetAberrationCorrection("NONE");

    // Set up focal plane map
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(this, naifIkCode());
    // center of array (same for WAC and NAC based on XY origin in EIS_Sensor_summary.xlsx)
    focalMap->SetDetectorOrigin(2048.5, 0.5);
    focalMap->SetDetectorOffset(0.0, 0.0);

    // Set up distortion map
    CameraDistortionMap *distMap = new CameraDistortionMap(this);
    distMap->SetDistortion(naifIkCode());

    // Set up the ground and sky map
    new LineScanCameraGroundMap(this);
    new LineScanCameraSkyMap(this);

    setTime(etStart.Et());

    LoadCache();
    NaifStatus::CheckErrors();
  }


  /**
  * Destructor for a ClipperPushBroomCamera object.
  */
  ClipperPushBroomCamera::~ClipperPushBroomCamera() {
  }


   /**
    * CK frame ID - Instrument Code from spacit run on CK
    *
    * @return @b int The appropriate instrument code for the "Camera-matrix"
    *         Kernel Frame ID
    */
   int ClipperPushBroomCamera::CkFrameId() const {
     return (-159000);
   }


   /**
    * CK Reference ID - J2000
    *
    * @return @b int The appropriate instrument code for the "Camera-matrix"
    *         Kernel Reference ID
    */
   int ClipperPushBroomCamera::CkReferenceId() const {
     return (1);
   }


   /**
    * SPK Reference ID - J2000
    *
    * @return @b int The appropriate instrument code for the Spacecraft
    *         Kernel Reference ID
    */
   int ClipperPushBroomCamera::SpkReferenceId() const {
     return (1);
   }

   /**
    * @param filename
    */
   void ClipperPushBroomCamera::ReadLineRates(QString filename) {
     Table timesTable("LineScanTimes", filename);

     if(timesTable.Records() <= 0) {
       QString msg = "Table [LineScanTimes] in [";
       msg += filename + "] must not be empty";
       throw IException(IException::Unknown, msg, _FILEINFO_);
     }

     for(int i = 0; i < timesTable.Records(); i++) {
       p_lineRates.push_back(LineRateChange((int)timesTable[i][2],
                                            (double)timesTable[i][0],
                                            timesTable[i][1]));
     }

     if(p_lineRates.size() <= 0) {
       QString msg = "There is a problem with the data within the Table ";
       msg += "[LineScanTimes] in [" + filename + "]";
       throw IException(IException::Unknown, msg, _FILEINFO_);
     }
   }
}

/**
 * This is the function that is called in order to instantiate an ClipperPushBroomCamera
 * object.
 *
 * @param Isis::Cube &cube Clipper EIS image.
 *
 * @return Isis::Camera* ClipperPushBroomCamera
 */
extern "C" Isis::Camera *ClipperPushBroomCameraPlugin(Isis::Cube &cube) {
  return new Isis::ClipperPushBroomCamera(cube);
}
