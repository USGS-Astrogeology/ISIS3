/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>

#include <QString>
#include <QVariant>

#include "MdisCamera.h"
#include "TaylorCameraDistortionMap.h"

#include "CameraDetectorMap.h"
#include "CameraDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "CameraGroundMap.h"
#include "CameraSkyMap.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "NaifStatus.h"

using namespace std;

namespace Isis {
  /**
   * @brief Initialize the MDIS camera model for NAC and WAC
   *
   * This constructor reads the Messenger/MDIS instrument addendum for many of
   * its default parameters.
   *
   * This camera model does not support subframes of jailbar imaging modes.
   * An exception is thrown in those cases.
   *
   * @param lab Pvl label from a Messenger MDIS image.
   *
   * @throws iException::User - "Subframe imaging mode is not supported"
   * @throws iException::User - "Jail bar observations are not supported"
   * @throws iException::User - "New MDIS/NAC distortion model invalidates
   *                 previous SPICE - you must rerun spiceinit to get new
   *                 kernels"
   * @internal
   *   @history 2011-05-03 Jeannie Walldren - Added NAIF error check.
   *
   */
  MdisCamera::MdisCamera(Cube &cube) : FramingCamera(cube) {
    m_spacecraftNameLong = "Messenger";
    m_spacecraftNameShort = "Messenger";

    NaifStatus::CheckErrors();

    // Set up detector constants
    // Note that Wac has filters, -236800 through -236812
    // http://naif.jpl.nasa.gov/pub/naif/pds/data/mess-e_v_h-spice-6-v1.0/messsp_1000/data/ik/msgr_mdis_v160.ti
    const int MdisWac(-236800);
    const int MdisNac(-236820);

    if (naifIkCode() == MdisNac) {
      m_instrumentNameLong = "Mercury Dual Imaging System Narrow Angle Camera";
      m_instrumentNameShort = "MDIS-NAC";
    }
    else if (naifIkCode() <= MdisWac && naifIkCode() >= -236812) {
      m_instrumentNameLong = "Mercury Dual Imaging System Wide Angle Camera";
      m_instrumentNameShort = "MDIS-WAC";
    }
    else {
      std::string msg = QString::number(naifIkCode());
      msg += " is not a supported instrument kernel code for Messenger.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    Pvl &lab = *cube.label();
    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);

    // Clarification on MDIS subframe image mode provides us the ability to
    // support this mode now.  The entire MDIS frame is geometrically valid
    // but only portions of the full frame actually contain image data.  The
    // portions outside subframes should be NULL and not interfere in
    // downstream processing, such as mosaics.
#if defined(MDIS_SUBFRAMES_UNSUPPORTED)
    int subFrameMode = inst["SubFrameMode"];
    if(subFrameMode != 0) {
      string msg = "Subframe imaging mode is not supported!";
      throw iException::Message(iException::User, msg, _FILEINFO_);
    }
#endif

    //  According to the MDIS team, this is nothing to be concerned with and
    //  should be treated as other normal observations.  So the test to
    // disallow it has been effectively removed 2007-09-05 (KJB).
#if defined(MDIS_JAILBARS_UNSUPPORTED)
    int jailBars = inst["JailBars"];
    if(jailBars != 0) {
      string msg = "Jail bar observations are not currently supported!";
      throw iException::Message(iException::Programmer, msg, _FILEINFO_);
    }
#endif

    //  Determine filter number.  Only conditional code required for
    //  NAC and WAC support!
    int filterNumber(0);    //  Default appropriate for MDIS-NAC
    if(naifIkCode() == MdisWac) {
      PvlGroup &bandBin = lab.findGroup("BandBin", Pvl::Traverse);
      filterNumber = bandBin["Number"];
    }

    //  Set up instrument and filter code strings
    QString ikCode = toString(naifIkCode());
    int fnCode(naifIkCode() - filterNumber);
    QString filterCode = toString(fnCode);
    QString ikernKey;

    // Fetch the frame translations from the instrument kernels
    ikernKey = "INS" + ikCode + "_REFERENCE_FRAME";
    QString baseFrame = getString(ikernKey);

    ikernKey = "INS" + filterCode + "_FRAME";
    QString ikFrame = getString(ikernKey);

    // Set up the camera info from ik/iak kernels

    //  Turns out (2008-01-17) the WAC has different focal lengths for
    // each filter.  Added to the instrument kernel (IAK) on this date.
    //  Add temperature dependant focal length
    SetFocalLength(computeFocalLength(filterCode, lab));

    SetPixelPitch();

    // Removed by Jeff Anderson.  The refactor of the SPICE class
    // uses frames always so this is no longer needed
    //      LoadFrameMounting(baseFrame, ikFrame, false);

    // Get the start time from labels as the starting image time plus half
    // the exposure duration (in <MS>) to get pointing attitude.
    //  !!NOTE:  The ephemeris time MUST be set prior to creating the
    //           cache (CreateCache) because the kernels are all unloaded
    //           after the cache is done and this operation will fail!!
    QString stime = QString::fromStdString(inst["SpacecraftClockCount"]);
    double exposureDuration = ((double) inst["ExposureDuration"]) / 1000.0;// divide by 1000 to convert to seconds

    iTime etStart = getClockTime(stime);

    //  Setup camera detector map
    CameraDetectorMap *detMap = new CameraDetectorMap(this);

    // Setup focal plane map, and detector origin for the instrument that
    // may have a filter (WAC only!).
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(this, fnCode);

    //  Retrieve boresight location from instrument kernel (IK) (addendum?)
    ikernKey = "INS" + ikCode + "_BORESIGHT_SAMPLE";
    double sampleBoreSight = getDouble(ikernKey);

    ikernKey = "INS" + ikCode + "_BORESIGHT_LINE";
    double lineBoreSight = getDouble(ikernKey);

    //  Apply the boresight
    focalMap->SetDetectorOrigin(sampleBoreSight, lineBoreSight);

    // Determine summing.  MDIS has two sources of summing or binning.
    // One is performed in the FPU and the in the MP, post-observation,
    // on-board after coming out of the FPGAs, where the FPU binning is
    // performed.  The FPU binning was programmed incorrectly and the
    // actual pixels from the detector are peculiar.  Hence, I have
    // designed this camera model such that the offsets can be managed
    // external to the code.  See the MDIS instrument kernel addendum
    // in $ISISDATA/messenger/kernels/iak/mdisAddendum???.ti for the
    // offsets for *each* detector.  Note that an offset is only applied
    // when FPU binning is performed.
    int fpuBinMode   = inst["FpuBinningMode"];
    int pixelBinMode = inst["PixelBinningMode"];

    int summing = ((pixelBinMode == 0) ? 1 : pixelBinMode);
    //  FPU binning was performed, retrieve the FPU binning offsets and
    //  apply them to the focal plane mapping.
    if(fpuBinMode == 1) {
#if defined(USE_FPU_BINNING_OFFSETS)
      ikernKey = "INS" + ikCode + "_FPUBIN_START_SAMPLE";
      double fpuStartingSample = getDouble(ikernKey);
      detMap->SetStartingDetectorSample(fpuStartingSample);

      ikernKey = "INS" + ikCode + "_FPUBIN_START_LINE";
      double fpuStartingLine = getDouble(ikernKey);
      detMap->SetStartingDetectorLine(fpuStartingLine);
#endif
      summing *= 2;
    }

    //  Set summing/binning modes as an accumulation of FPU and MP binning.
    detMap->SetDetectorLineSumming(summing);
    detMap->SetDetectorSampleSumming(summing);

    // Setup distortion map.  As of 2007/12/06, we now have an actual model.
    // Note that this model supports distinct distortion for each WAC filter.
    // See $ISISDATA/messenger/kernels/iak/mdisAddendumXXX.ti or possibly
    // $ISISDATA/messenger/kernels/ik/msgr_mdis_vXXX.ti for the *_OD_K
    // parameters.
    // NAC has a new implementation of its distortion contributed by
    // Scott Turner and Lillian Nguyen at JHUAPL.
    // (2010/10/06) The WAC now uses the same disortion model implementation.
    // Valid Taylor Series parameters are in versions msgr_mdis_v120.ti IK
    // and above.   Note fnCode works for NAC as well as long as
    // filterNumber stays at 0 for the NAC only!
    try {
      TaylorCameraDistortionMap *distortionMap = new TaylorCameraDistortionMap(this);
      distortionMap->SetDistortion(fnCode);
    }
    catch(IException &ie) {
      string msg = "New MDIS NAC/WAC distortion models will invalidate previous "
                   "SPICE - you may need to rerun spiceinit to get new kernels";
      throw IException(ie, IException::User, msg, _FILEINFO_);
    }

    // Setup the ground and sky map
    new CameraGroundMap(this);
    new CameraSkyMap(this);

    // Create a cache and grab spice info since it does not change for
    // a framing camera (fixed spacecraft position and pointing) after,
    // of course applying the gimble offset which is handled in the SPICE
    // kernels (thank you!).  Note this was done automagically in the
    // SetEpheremisTime call above.  IMPORTANT that it be done prior to
    // creating the cache since all kernels are unloaded, essentially
    // clearing the pool and whacking the frames definitions, required to
    iTime centerTime = etStart + (exposureDuration / 2.0);
    setTime(centerTime);
    LoadCache();
    NaifStatus::CheckErrors();
  }


  /**
   * Returns the shutter open and close times.  The user should pass in the
   * ExposureDuration keyword value, converted from milliseconds to seconds, and
   * the SpacecraftClockCount keyword value, converted to ephemeris time. The
   * StartTime keyword value from the labels represents the shutter open time of
   * the observation. This method uses the FramingCamera class implementation,
   * returning the given time value as the shutter open and the sum of the time
   * value and exposure duration as the shutter close.
   *
   * @param exposureDuration Exposure duration value from the labels, converted
   *                         to seconds.
   * @param time The SpacecraftClockCount value from the labels, converted to
   *             ephemeris time
   * @return @b pair < @b iTime, @b iTime > The first value is the shutter
   *         open time and the second is the shutter close time.
   *
   * @see
   * @author 2011-05-03 Jeannie Walldren
   * @internal
   *   @history 2011-05-03 Jeannie Walldren - Original version.
   */
  pair <iTime, iTime> MdisCamera::ShutterOpenCloseTimes(double time,
                                                        double exposureDuration) {
    return FramingCamera::ShutterOpenCloseTimes(time, exposureDuration);
  }


/**
 * @brief Computes temperature-dependent focal length
 *
 * This method computes temperature dependent focal lengths based upon a 5th
 * order polynomial using the FocalPlaneTemperature keyword value stored in the
 * ISIS label (it is the FOCAL_PLANE_TEMPERATURE PDS keyword).  At the time of
 * this writing, only (the two) linear terms are used.
 *
 * In addition, this method is initially coded to be backword compatible but
 * this feature is likely to be removed when the kernels become fully adopted.
 *
 * IMPORTANT:  The computed temperature dependent focal length is stored in the
 * label of the cube during spiceinit.  This implementation uses the special
 * recording of keywords as retrieved from kernels and stores them as a string
 * value so (SOCET) folks can easily read and apply the focal lengths in their
 * environments.  String storage is preferred over storing as double since
 * these values are stored in hexidecimal format.
 *
 * @author Kris Becker (8/13/2012)
 * @internal
 *  @history 2012-10-22 Kris Becker - Store temperature dependant keyword in instrument
 *                                    neutral keyword called TempDependentFocalLength.
 *
 * @param filterCode Integer MDIS instrument/filter code
 * @param label      Pvl label from cube being initialized where temperature
 *                   keywords are extracted from.
 *
 * @return double    Computed temperature dependant focal length
 */
  double MdisCamera::computeFocalLength(const QString &filterCode,
                                        Pvl &label) {

    double focalLength(0.0);
    QString tdflKey("TempDependentFocalLength");

    //  Determine if the desired value is already computed.  We are interested
    //  in the temperature dependent value firstly.  Backward compatibility is
    //  considered below.
    QVariant my_tdfl = readStoredValue(tdflKey, SpiceStringType, 0);
    if (my_tdfl.isValid()) {
      focalLength = IString(my_tdfl.toString()).ToDouble();
    }
    else {
      // Hasn't been computed yet (in spiceinit now - maybe) or the proper
      //  IK containing polynomial parameters is not in use.

    // Original Code ensures backward compatibility
      focalLength = getDouble("INS" + filterCode + "_FOCAL_LENGTH");

      //  Check for disabling of temperature dependent focal length
      bool tdfl_disabled(false);
#ifndef DISABLE_TDFL_DISABLING
      try {
        IString tdfl_state = getString("DISABLE_MDIS_TD_FOCAL_LENGTH");
        tdfl_disabled = ( "TRUE" == tdfl_state.UpCase() );
      }
      catch (IException &ie) {
        tdfl_disabled = false;
      }
#endif

      // Attempt to retrieve parameters necessary for temperature-dependent focal
      // length and computed it
      if ( !tdfl_disabled ) {
        // Wrap a try clause all around this so that if it fails, will return
        // default
        try {
          PvlGroup &inst = label.findGroup("Instrument", Pvl::Traverse);
          double fpTemp = inst["FocalPlaneTemperature"];
          double fl(0.0);
          QString fptCoeffs = "INS" + filterCode + "_FL_TEMP_COEFFS";
          //  Compute 5th order polynomial
          for (int i = 0 ; i < 6 ;  i++) {
            fl += getDouble(fptCoeffs, i) * pow(fpTemp, (double) i);
          }

          // Store computed focal length
          focalLength = fl;
          storeValue(tdflKey, 0, SpiceStringType, QVariant(focalLength));
        }
        catch (IException &ie) {
          // Noop when supporting old IKs
          throw IException(ie, IException::Programmer,
                            "Failed to compute temperature-dependent focal length",
                             _FILEINFO_);
        }
      }
    }
     return (focalLength);
  }
}

/**
 * This is the function that is called in order to instantiate a MdisCamera
 * object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* MdisCamera
 */
extern "C" Isis::Camera *MdisCameraPlugin(Isis::Cube &cube) {
  return new Isis::MdisCamera(cube);
}
