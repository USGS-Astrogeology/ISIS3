/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "HrscCamera.h"

#include <string>

#include <QString>

#include "CameraDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "iTime.h"
#include "LineScanCameraGroundMap.h"
#include "LineScanCameraSkyMap.h"
#include "NaifStatus.h"
#include "Statistics.h"
#include "VariableLineScanCameraDetectorMap.h"

using namespace std;
namespace Isis {
  /**
   * Creates a HrscCamera Camera Model
   *
   * @param lab Pvl label from the iamge
   * @internal
   *   @history 2011-05-03 Jeannie Walldren - Added NAIF error check.
   */
  HrscCamera::HrscCamera(Cube &cube) : LineScanCamera(cube) {
    m_instrumentNameLong = "High Resolution Stereo Camera";
    m_instrumentNameShort = "HRSC";
    m_spacecraftNameLong = "Mars Express";
    m_spacecraftNameShort = "MEX";

    NaifStatus::CheckErrors();
    // Setup camera characteristics from instrument and frame kernel
    SetFocalLength();
    SetPixelPitch(0.007);
    instrumentRotation()->SetFrame(-41210);

    ReadLineRates(cube);

    // Get required keywords from instrument group
    Pvl &lab = *cube.label();
    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);

    // Setup detector map for transform of image pixels to detector position
    new VariableLineScanCameraDetectorMap(this, p_lineRates);
    DetectorMap()->SetDetectorSampleSumming(inst["Summing"]);

    // Setup focal plane map for transform of detector position to
    // focal plane x/y.  This will read the appropriate CCD
    // transformation coefficients from the instrument kernel

    new CameraFocalPlaneMap(this, naifIkCode());

    QString ikernKey = "INS" + toString(naifIkCode())  + "_BORESIGHT_SAMPLE";
    double sampleBoresight = getDouble(ikernKey);

    ikernKey = "INS" + toString(naifIkCode())  + "_BORESIGHT_LINE";
    double lineBoresight = getDouble(ikernKey);

    FocalPlaneMap()->SetDetectorOrigin(sampleBoresight, lineBoresight);

    // Setup distortion map.  This will read the optical distortion
    // coefficients from the instrument kernel
    new CameraDistortionMap(this);

    // Setup the ground and sky map to transform undistorted focal
    // plane x/y to lat/lon or ra/dec respectively.
    new LineScanCameraGroundMap(this);
    new LineScanCameraSkyMap(this);

    LoadCache();
    NaifStatus::CheckErrors();
  }


  //! Destroys the HiriseCamera object
  HrscCamera::~HrscCamera() {
  }


  /**
   * CK frame ID -  - Instrument Code from spacit run on CK
   *
   * @return @b int The appropriate instrument code for the "Camera-matrix"
   *         Kernel Frame ID
   */
  int HrscCamera::CkFrameId() const {
    return (-41001);
  }


  /**
   * CK Reference ID - J2000
   *
   * @return @b int The appropriate instrument code for the "Camera-matrix"
   *         Kernel Reference ID
   */
  int HrscCamera::CkReferenceId() const {
    return (1);
  }


  /**
   * SPK Reference ID - J2000
   *
   * @return @b int The appropriate instrument code for the Spacecraft
   *         Kernel Reference ID
   */
  int HrscCamera::SpkReferenceId() const {
    return (1);
  }


  /**
   * @param cube
   */
  void HrscCamera::ReadLineRates(Cube &cube) {
    Table timesTable("LineScanTimes");

    // check if LineScanTimes was added from isd
    if (cube.hasTable("LineScanTimes")) {
      timesTable = cube.readTable("LineScanTimes");
    }
    else {
      timesTable = Table("LineScanTimes", cube.fileName());
    }

    if(timesTable.Records() <= 0) {
      QString msg = "Table [LineScanTimes] in [";
      msg += cube.fileName() + "] must not be empty";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    for(int i = 0; i < timesTable.Records(); i++) {
      p_lineRates.push_back(LineRateChange((int)timesTable[i][2],
                                           (double)timesTable[i][0],
                                           timesTable[i][1]));
    }

    if(p_lineRates.size() <= 0) {
      QString msg = "There is a problem with the data within the Table ";
      msg += "[LineScanTimes] in [" + cube.fileName() + "]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
  }
}


/**
 * This is the function that is called in order to instantiate a
 * HrscCamera object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* HrscCamera
 * @internal
 *   @history 2011-05-03 Jeannie Walldren - Removed Mex namespace.
 */
extern "C" Isis::Camera *HrscCameraPlugin(Isis::Cube &cube) {
  return new Isis::HrscCamera(cube);
}
