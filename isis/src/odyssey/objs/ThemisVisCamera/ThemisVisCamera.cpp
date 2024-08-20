/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ThemisVisCamera.h"

#include <iomanip>

#include <QDebug>
#include <QString>

#include "CameraFocalPlaneMap.h"
#include "CameraSkyMap.h"
#include "iTime.h"
#include "NaifStatus.h"
#include "PushFrameCameraDetectorMap.h"
#include "PushFrameCameraGroundMap.h"
#include "ThemisVisDistortionMap.h"


using namespace std;

namespace Isis {

  /**
   * Constructor for the Themis Vis Camera Model
   *
   * @param lab Pvl label from an Odyssey Themis VIS image.
   *
   * @throws IException::User - The image does not appear to be a Themis
   *                                  VIS image
   * @internal
   *   @history 2010-08-04 Jeannie Walldren - Added NAIF error check.
   */
  ThemisVisCamera::ThemisVisCamera(Cube &cube) : PushFrameCamera(cube) {
    m_instrumentNameLong = "Thermal Emission Imaging System Visual";
    m_instrumentNameShort = "Themis-VIS";
    m_spacecraftNameLong = "Mars Odyssey";
    m_spacecraftNameShort = "Odyssey";

    NaifStatus::CheckErrors();
    // Set up the camera characteristics
    // LoadFrameMounting("M01_SPACECRAFT","M01_THEMIS_VIS");
    // Changed Focal Length from 203.9 (millimeters????) to 202.059, per request from
    // Christopher Edwards (Christopher.Edwards@asu.edu) at ASU, on 2/18/11.
    SetFocalLength(202.059);
    SetPixelPitch(0.009);

    Pvl &lab = *cube.label();
    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);

    // make sure it is a themis vis image
    if(inst["InstrumentId"][0] != "THEMIS_VIS") {
      QString msg = "Unable to create Themis VIS camera model from an image with InstrumentId ["
                   +  QString::fromStdString(inst["InstrumentId"][0]) + "].";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Get necessary variables
    p_exposureDur = inst["ExposureDuration"];
    p_interframeDelay = inst["InterframeDelay"];
    int sumMode = inst["SpatialSumming"];

    // Get the start and end time
    double et;
    QString stime = QString::fromStdString(inst["SpacecraftClockCount"]);
    et = getClockTime(stime).Et();

    double offset = inst["SpacecraftClockOffset"];
    p_etStart = et + offset - ((p_exposureDur / 1000.0) / 2.0);
    p_nframes = inst["NumFramelets"];

    // Get the keywords from labels
    PvlGroup &bandBin = lab.findGroup("BandBin", Pvl::Traverse);

    PvlKeyword &filterNumbers = bandBin["FilterNumber"];
    for (int i = 0; i < filterNumbers.size(); i++) {
      p_filterNumber.append(std::stoi(filterNumbers[i]));
    }


    // Setup detector map
    double frameRate = p_interframeDelay;
    //int frameletHeight = 192;
    int frameletHeight = (int) (ParentLines() / ((double) p_nframes / (double) sumMode)); // = 192
    PushFrameCameraDetectorMap *dmap =
      new PushFrameCameraDetectorMap(this, p_etStart, frameRate, frameletHeight);
    dmap->SetDetectorSampleSumming(sumMode);
    dmap->SetDetectorLineSumming(sumMode);
    dmap->SetFrameletOrderReversed(false, p_nframes); // these framelets are in time ascending order
                                                      //(i.e. the order is not reversed)
    // dmap->SetFrameletsGeometricallyFlipped(true); this is not set... looks like it defaults to true???
    // We do not want to set the exposure duration in the detector map, let it default to 0.0...

    // Setup focal plane map
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(this, naifIkCode());
    focalMap->SetDetectorOrigin(512.5, 512.5);

    // Setup distortion map
    new ThemisVisDistortionMap(this);

    // Setup the ground and sky map
    bool evenFramelets = (inst["Framelets"][0] == "Even");
    new PushFrameCameraGroundMap(this, evenFramelets);
    new CameraSkyMap(this);

    LoadCache();
    NaifStatus::CheckErrors();
  }


  ThemisVisCamera::~ThemisVisCamera() {
  }



  /**
   * Sets the band in the camera model
   *
   * @param vband The band number to set
   */
  void ThemisVisCamera::SetBand(const int vband) {
    Camera::SetBand(vband);

    // Set the et
    double et = p_etStart + BandEphemerisTimeOffset(vband);
    setTime(et);
    PushFrameCameraDetectorMap *dmap = (PushFrameCameraDetectorMap *)this->DetectorMap();
    dmap->SetStartTime(et);
  }



  /**
   * Calculates time offset for the given cube band number.
   *
   * @param vband The band number.
   *
   * @return @b double The time offset value.
   *
   */
  double ThemisVisCamera::BandEphemerisTimeOffset(int vband) {
    // Lookup the time band corresponding to this ISIS cube band
    // number based on the FilterNumber keyword in the BandBin group.
    // Filter numbers indicate the physical location of the band in
    // the detector array.  They are numbered by ascending times.
    // (filter number = time band)
    int timeBand = p_filterNumber[vband - 1];

    if (HasReferenceBand()) {
    // If there is a reference band, the data has all been aligned in the band dimension

    // VIS BandNumbers (including the reference band) are numbered by ascending filter
    // wavelength. Convert the wavelength band to a time band (filter number).
      int wavelengthToTimeBand[] = { 2, 5, 3, 4, 1 };
      timeBand = wavelengthToTimeBand[ReferenceBand() - 1];
    }

    // Compute the time offset for this detector line.
    // Subtract 1 from the time band then multiply by the interframe delay then
    // subtract half the exposure duration, in seconds.
    //
    // Subtracting 1 from the time band number calculates the appropriate
    // number of interframe delay multiples for this filter number (recall this
    // corresponds to a location on the ccd)
    p_bandTimeOffset = ((timeBand - 1) * p_interframeDelay) - ((p_exposureDur / 1000.0) / 2.0);

    // Set the detector first line for this band on the ccd.
    // The VIS band first row values are 1-based detector row numbers
    // used for the beginning (bottom) of the 192-row framelet for the various bands.
    // These row values correspond directly to the filter numbers (time bands) {1, 2, 3, 4, 5}.
    // Obtained from the NAIF instrument kernel.
    // Note that row 1 is the first detector row to see an area of the ground.
    int visBandFirstRow[] = { 4, 203, 404, 612, 814 };
    PushFrameCameraDetectorMap *dmap = (PushFrameCameraDetectorMap *)this->DetectorMap();
    dmap->SetBandFirstDetectorLine(visBandFirstRow[timeBand - 1]);

    return p_bandTimeOffset;
  }



  /**
   * The camera model is band dependent (i.e. not band independent), so this
   * method returns false
   *
   * @return @b bool This will always return False.
   */
  bool ThemisVisCamera::IsBandIndependent() {
    return false;
  }



  /**
   * CK frame ID -  - Instrument Code from spacit run on CK
   *
   * @return @b int The appropriate instrument code for the "Camera-matrix"
   *         Kernel Frame ID
   */
  int ThemisVisCamera::CkFrameId() const {
    return -53000;
  }



  /**
   * CK Reference ID - MARSIAU
   *
   * @return @b int The appropriate instrument code for the "Camera-matrix"
   *         Kernel Reference ID
   */
  int ThemisVisCamera::CkReferenceId() const {
    return 16;
  }



  /**
   * SPK Reference ID - J2000
   *
   * @return @b int The appropriate instrument code for the Spacecraft
   *         Kernel Reference ID
   */
  int ThemisVisCamera::SpkReferenceId() const {
    return 1;
  }
}


// Plugin
/**
 * This is the function that is called in order to instantiate a
 * ThemisVisCamera object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* ThemisVisCamera
 * @internal
 *   @history 2011-05-03 Jeannie Walldren - Removed Odyssey namespace.
 */
extern "C" Isis::Camera *ThemisVisCameraPlugin(Isis::Cube &cube) {
  return new Isis::ThemisVisCamera(cube);
}
