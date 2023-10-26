/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "IdealCamera.h"

#include <iomanip>
#include <string>

#include <QDebug>
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
#include "Pvl.h"

using namespace std;
namespace Isis {
  /**
   * Creates a generic camera model.  That is a camera without
   * optical distortion.  The following information from the label
   * must be available
   * \code
   *
   * Group = Instrument
   *   SpacecraftName = IdealSpacecraft
   *   InstrumentId   = IdealCamera
   *   TargetName     = Mars | Moon | etc
   *   StartTime      = YYYY-MM-DDTHH:MM:SS.SSS
   *   StopTime       = YYYY-MM-DDTHH:MM:SS.SSS
   *
   *   EphermisTime = nnnnnnnnnn.sss <second>
   *   ExposureDuration = nnn.nn <milliseconds>
   *
   *   InstrumentType = Framing | Linescan
   *   FocalLength    = nnn.n <millimeters> [ALTERNATIVE: IDEAL_FOCAL_LENGTH in NaifKeywords]
   *   PixelPitch      = nn.n <millimeters> [ALTERNATIVE: IDEAL_PIXEL_PITCH in NaifKeywords]
   *   SampleDetectors = nnnn
   *   LineDetectors   = nnnn
   * End_Group
   *
   * \endcode
   * Note boresight is assumed to be at the center of the detectors
   *
   * @param lab Pvl label from the image
   * @internal
   *   @history 2007-02-12 Debbie A. Cook - Added sign for all the trans
   *           parameters
   *   @history 2011-05-03 Jeannie Walldren - Added NAIF error check.
   */
  IdealCamera::IdealCamera(Cube &cube) : Camera(cube) {
    NaifStatus::CheckErrors();
    
    // Since this is an ideal camera, we'll call it Ideal Spacecraft
    m_spacecraftNameLong = "Ideal Spacecraft";
    m_spacecraftNameShort = "Ideal";
    m_instrumentNameLong = "Ideal Camera";
    m_instrumentNameShort = "Ideal";
    
    // Get required keywords from instrument group
    PvlGroup &inst = cube.label()->findGroup("Instrument", Pvl::Traverse);

    // Setup camera characteristics from instrument

    if (inst.hasKeyword("FocalLength")) {
      SetFocalLength(inst["FocalLength"]);
    }
    else {
      SetFocalLength(readValue("IDEAL_FOCAL_LENGTH", SpiceDoubleType).toDouble());
    }

    if (inst.hasKeyword("PixelPitch")) {
      SetPixelPitch(inst["PixelPitch"]);
    }
    else {
      SetPixelPitch(readValue("IDEAL_PIXEL_PITCH", SpiceDoubleType).toDouble());
    }

    double et = inst["EphemerisTime"];

    double exposureDuration = 0.0;
    if (inst.hasKeyword("ExposureDuration")) {
      exposureDuration = ((double) inst["ExposureDuration"]) / 1000.0;
    }

    double sampleDetectors = inst["SampleDetectors"];
    double lineDetectors   = inst["LineDetectors"];

    // These variables are used for maintaining compatibility with older versions of the
    //   ideal camera (noproj before it modified the naif keywords group) and for cubes without the
    //   naif keywords group at all.
    int xDependency = inst["FocalPlaneXDependency"];
    int yDependency = CameraFocalPlaneMap::Line;
    // Pixel direction
    double xdir = 1.0;
    double ydir = 1.0;
    double sdir = xdir;
    double ldir = ydir;

    if (inst.hasKeyword("TransX")) xdir = inst["TransX"];
    if (inst.hasKeyword("TransY")) ydir = inst["TransY"];

    if (xDependency == CameraFocalPlaneMap::Line) {
      yDependency = CameraFocalPlaneMap::Sample;
      sdir = ydir;
      ldir = xdir;
    }

    // Put the translation coefficients into the Naif kernel pool so the
    // CameraFocalPlaneClass can find them
    try {
      readValue("IDEAL_TRANSX", SpiceDoubleType);
    }
    catch (IException &) {
      double keyval[3];
      keyval[0] = 0.;
      if (inst.hasKeyword("TransX0")) {
        keyval[0] = inst["TransX0"];
      }

      keyval[xDependency] = PixelPitch() * xdir;
      keyval[yDependency] = 0.;

      storeValue("IDEAL_TRANSX", 0, SpiceDoubleType, keyval[0]);
      storeValue("IDEAL_TRANSX", 1, SpiceDoubleType, keyval[1]);
      storeValue("IDEAL_TRANSX", 2, SpiceDoubleType, keyval[2]);
      pdpool_c("IDEAL_TRANSX", 3, keyval);
    }

    try {
      readValue("IDEAL_TRANSY", SpiceDoubleType);
    }
    catch (IException &) {
      double keyval[3];
      keyval[0] = 0.;
      if (inst.hasKeyword("TransY0")) {
        keyval[0] = inst["TransY0"];
      }

      keyval[yDependency] = PixelPitch() * ydir;
      keyval[xDependency] = 0.;

      storeValue("IDEAL_TRANSY", 0, SpiceDoubleType, keyval[0]);
      storeValue("IDEAL_TRANSY", 1, SpiceDoubleType, keyval[1]);
      storeValue("IDEAL_TRANSY", 2, SpiceDoubleType, keyval[2]);
      pdpool_c("IDEAL_TRANSY", 3, keyval);
    }

    try {
      readValue("IDEAL_TRANSS", SpiceDoubleType);
    }
    catch (IException &) {
      double keyval[3];
      keyval[0] = 0.;
      if (inst.hasKeyword("TransS0")) {
        keyval[0] = inst["TransS0"];
      }

      keyval[xDependency] = 1 / PixelPitch() * sdir;
      keyval[yDependency] = 0.;

      storeValue("IDEAL_TRANSS", 0, SpiceDoubleType, keyval[0]);
      storeValue("IDEAL_TRANSS", 1, SpiceDoubleType, keyval[1]);
      storeValue("IDEAL_TRANSS", 2, SpiceDoubleType, keyval[2]);
      pdpool_c("IDEAL_TRANSS", 3, keyval);
    }

    try {
      readValue("IDEAL_TRANSL", SpiceDoubleType);
    }
    catch (IException &) {
      double keyval[3];
      keyval[0] = 0.;
      if (inst.hasKeyword("TransL0")) {
        keyval[0] = inst["TransL0"];
      }

      keyval[yDependency] = 1 / PixelPitch() * ldir;
      keyval[xDependency] = 0.0;

      storeValue("IDEAL_TRANSL", 0, SpiceDoubleType, keyval[0]);
      storeValue("IDEAL_TRANSL", 1, SpiceDoubleType, keyval[1]);
      storeValue("IDEAL_TRANSL", 2, SpiceDoubleType, keyval[2]);
      pdpool_c("IDEAL_TRANSL", 3, keyval);
    }

    // Create correct camera type
    QString type = QString::fromStdString(inst["InstrumentType"]);
    if (type.toUpper() == "FRAMING") {
      p_framing = true;
      new CameraDetectorMap(this);
      CameraFocalPlaneMap *fmap = new CameraFocalPlaneMap(this, 0);
      fmap->SetDetectorOrigin(sampleDetectors / 2.0 + 0.5,
                              lineDetectors / 2.0 + 0.5);
      new CameraDistortionMap(this);
      new CameraGroundMap(this);
      new CameraSkyMap(this);

      setTime(et);
      LoadCache();
    }
    else if (type.toUpper() == "LINESCAN") {
      p_framing = false;
      new LineScanCameraDetectorMap(this, et, exposureDuration);
      CameraFocalPlaneMap *fmap = new CameraFocalPlaneMap(this, 0);
      fmap->SetDetectorOrigin(sampleDetectors / 2.0 + 0.5,
                              0.0);
      new CameraDistortionMap(this);
      new LineScanCameraGroundMap(this);
      new LineScanCameraSkyMap(this);

      LoadCache();
      NaifStatus::CheckErrors();
    }
    else {
      std::string msg = "Unknown InstrumentType [" + std::to_string((int)inst["InstrumentType"]) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  
  //! Destroys the IdealCamera object
  IdealCamera::~IdealCamera() {}


  /**
   * CK frame ID.  This is an overridden method for the Camera class pure
   * virtual method. It will always throw an error for Ideal Camera models.
   * @throw IException - "No CK Frame ID for Ideal Camera class."
   * @return @b int
   */
  int IdealCamera::CkFrameId() const {
    string msg = "No CK Frame ID for Ideal Camera class";
    throw IException(IException::User, msg, _FILEINFO_);
  }


  /**
   * CK Reference ID.  This is an overridden method for the Camera class pure
   * virtual method. It will always throw an error for Ideal Camera models.
   * @throw IException - "No CK Reference ID for Ideal Camera class."
   * @return @b int
   */
  int IdealCamera::CkReferenceId() const {
    string msg = "No CK Reference ID for Ideal Camera class";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  
  /**
   * SPK Target ID.  This is an overridden method for the Camera class pure
   * virtual method. It will always throw an error for Ideal Camera models.
   * @throw IException - "No SPK Target ID for Ideal Camera class."
   * @return @b int
   */
  int IdealCamera::SpkTargetId() const {
    string msg = "No SPK Target ID for Ideal Camera class";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  
  /**
   * SPK Center ID.  This is an overridden method for the Camera class pure
   * virtual method. It will always throw an error for Ideal Camera models.
   * @throw IException - "No SPK Center ID for Ideal Camera class."
   * @return @b int
   */
  int IdealCamera::SpkCenterId() const {
    string msg = "No SPK Center ID for Ideal Camera class";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  
  /**
   * SPK Reference ID.  This is an overridden method for the Camera class pure
   * virtual method. It will always throw an error for Ideal Camera models.
   * @throw IException - "No SPK Reference ID for Ideal Camera class."
   * @return @b int
   */
  int IdealCamera::SpkReferenceId() const {
    string msg = "No SPK Reference ID for Ideal Camera class";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  
  
  /**
   * This method returns the full instrument name.
   *
   * @return QString
   */
  QString IdealCamera::instrumentNameLong() const {
    return m_instrumentNameLong;
  }
  
  
  /**
   * This method returns the shortened instrument name.
   *
   * @return QString
   */
  QString IdealCamera::instrumentNameShort() const {
    return m_instrumentNameShort;
  }
  
  
  /**
   * This method returns the full spacecraft name.
   * 
   * @return QString
   */
  QString IdealCamera::spacecraftNameLong() const {
    return m_spacecraftNameLong;
  }
  
  
  /**
   * This method returns the shortened spacecraft name.
   *
   * @return QString
   */
  QString IdealCamera::spacecraftNameShort() const {
    return m_spacecraftNameShort;
  }
}


/**
 * @brief External C function for createing the camera plugin
 *
 * This function is used by the CameraFactory to create an instance of the
 * IdealCamera object.
 *
 * @param lab The Isis::Pvl label object used for the information as how to
 *            create this object.
 */
extern "C" Isis::Camera *IdealCameraPlugin(Isis::Cube &cube) {
  return new Isis::IdealCamera(cube);
}
