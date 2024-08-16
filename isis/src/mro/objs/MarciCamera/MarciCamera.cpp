/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iomanip>

#include "MarciCamera.h"
#include "MarciDistortionMap.h"

#include <QString>

#include "CameraFocalPlaneMap.h"
#include "CameraSkyMap.h"
#include "IException.h"
#include "iTime.h"
#include "NaifStatus.h"
#include "PushFrameCameraDetectorMap.h"
#include "PushFrameCameraGroundMap.h"

using namespace std;
namespace Isis {

  /**
   * Constructor for the Marci Camera Model
   *
   * @param lab Pvl Label to create the camera model from
   *
   * @throws Isis::IException::User - The image does not appear to be a Marci
   *             image
   * @internal
   *   @history 2011-05-03 Jeannie Walldren - Added NAIF error check.
   */
  MarciCamera::MarciCamera(Cube &cube) : PushFrameCamera(cube) {
    m_instrumentNameLong = "Mars Color Imager";
    m_instrumentNameShort = "MARCI";
    m_spacecraftNameLong = "Mars Reconnaissance Orbiter";
    m_spacecraftNameShort = "MRO";

    NaifStatus::CheckErrors();
    Pvl &lab = *cube.label();
    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);
    // make sure it is a marci image
    if(inst["InstrumentId"][0] != "Marci") {
      string msg = "The image does not appear to be a Marci Image";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Set up the camera characteristics
    SetFocalLength();

    QString pixelPitchKey = "INS" + toString(naifIkCode()) + "_PIXEL_SIZE";
    SetPixelPitch(getDouble(pixelPitchKey));

    // Get necessary variables
    p_exposureDur = inst["ExposureDuration"];
    p_interframeDelay = inst["InterframeDelay"];
    int sumMode = inst["SummingMode"];

    // Get the start and end time
    double et;
    QString stime = QString::fromStdString(inst["SpacecraftClockCount"]);
    et = getClockTime(stime).Et();
    p_etStart = et - ((p_exposureDur / 1000.0) / 2.0);
    p_nframelets = (int) (ParentLines() / sumMode);

    // These numbers came from "MARCI_CTX_Cal_Report_v1.5.pdf" page 7 (Bandpasses & downlinked detector rows)
    map<QString, int> filterToDetectorOffset;
    filterToDetectorOffset.insert(pair<QString, int>("BLUE",     709));
    filterToDetectorOffset.insert(pair<QString, int>("GREEN",    734));
    filterToDetectorOffset.insert(pair<QString, int>("ORANGE",   760));
    filterToDetectorOffset.insert(pair<QString, int>("RED",      786));
    filterToDetectorOffset.insert(pair<QString, int>("NIR",      811));
    filterToDetectorOffset.insert(pair<QString, int>("LONG_UV",  266));
    filterToDetectorOffset.insert(pair<QString, int>("SHORT_UV", 293));

    map<QString, int> filterToFilterNumbers;
    filterToFilterNumbers.insert(pair<QString, int>("BLUE",     0));
    filterToFilterNumbers.insert(pair<QString, int>("GREEN",    1));
    filterToFilterNumbers.insert(pair<QString, int>("ORANGE",   2));
    filterToFilterNumbers.insert(pair<QString, int>("RED",      3));
    filterToFilterNumbers.insert(pair<QString, int>("NIR",      4));
    filterToFilterNumbers.insert(pair<QString, int>("LONG_UV",  5));
    filterToFilterNumbers.insert(pair<QString, int>("SHORT_UV", 6));

    int frameletOffsetFactor = inst["ColorOffset"];

    if((int)inst["DataFlipped"] != 0) frameletOffsetFactor *= -1;
    map<QString, int> filterToFrameletOffset;
    filterToFrameletOffset.insert(pair<QString, int>("NIR",      0 * frameletOffsetFactor));
    filterToFrameletOffset.insert(pair<QString, int>("RED",      1 * frameletOffsetFactor));
    filterToFrameletOffset.insert(pair<QString, int>("ORANGE",   2 * frameletOffsetFactor));
    filterToFrameletOffset.insert(pair<QString, int>("GREEN",    3 * frameletOffsetFactor));
    filterToFrameletOffset.insert(pair<QString, int>("BLUE",     4 * frameletOffsetFactor));
    filterToFrameletOffset.insert(pair<QString, int>("LONG_UV",  5 * frameletOffsetFactor));
    filterToFrameletOffset.insert(pair<QString, int>("SHORT_UV", 6 * frameletOffsetFactor));

    // Get the keywords from labels
    const PvlGroup &bandBin = lab.findGroup("BandBin", Pvl::Traverse);
    const PvlKeyword &filtNames = bandBin["FilterName"];

    for(int i = 0; i < filtNames.size(); i++) {
      if(filterToDetectorOffset.find(QString::fromStdString(filtNames[i])) == filterToDetectorOffset.end()) {
        QString msg = "Unrecognized filter name [" + QString::fromStdString(filtNames[i]) + "]";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      p_detectorStartLines.push_back(filterToDetectorOffset.find(QString::fromStdString(filtNames[i]))->second);
      p_filterNumbers.push_back(filterToFilterNumbers.find(QString::fromStdString(filtNames[i]))->second);
      p_frameletOffsets.push_back(filterToFrameletOffset.find(QString::fromStdString(filtNames[i]))->second);
    }

    // Setup detector map
    double frameletRate = p_interframeDelay;
    PushFrameCameraDetectorMap *dmap =
      new PushFrameCameraDetectorMap(this, p_etStart, frameletRate, 16);
    dmap->SetDetectorSampleSumming(sumMode);
    dmap->SetDetectorLineSumming(sumMode);
    dmap->SetFrameletsGeometricallyFlipped(false);

    int numFramelets = ParentLines() / (16 / sumMode);
    bool flippedFramelets = (int)inst["DataFlipped"] != 0;
    dmap->SetFrameletOrderReversed(flippedFramelets, numFramelets);

    // Setup focal plane map
    new CameraFocalPlaneMap(this, -74400);

    if ((int) naifIkCode() == -74410) {
      // The line detector origin is in the middle of the orange framelet
      FocalPlaneMap()->SetDetectorOrigin(512.5, 760.0 + 8.5);
    }
    else if ((int) naifIkCode() == -74420) {
      FocalPlaneMap()->SetDetectorOrigin(512.5, 288.5);
    }
    else {
      string msg = "Unrecognized NaifIkCode [" + IString((int) naifIkCode()) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Setup distortion map
    new MarciDistortionMap(this, naifIkCode());

    // Setup the ground and sky map
    bool evenFramelets = (inst["Framelets"][0] == "Even");
    new PushFrameCameraGroundMap(this, evenFramelets);
    new CameraSkyMap(this);
    LoadCache();
    NaifStatus::CheckErrors();

    if(sumMode == 1) {
      SetGeometricTilingHint(16, 4);
    }
    else if(sumMode == 2) {
      SetGeometricTilingHint(8, 4);
    }
    else if(sumMode == 4) {
      SetGeometricTilingHint(4, 4);
    }
    else {
      SetGeometricTilingHint(2, 2);
    }
  }



  //! Destroys the Themis Vis Camera object
  MarciCamera::~MarciCamera() {
  }



  /**
   * Sets the band in the camera model
   *
   * @param vband The band number to set
   */
  void MarciCamera::SetBand(const int vband) {
    // Sanity check on requested band
    int maxVirtualBands = min(p_detectorStartLines.size(), p_frameletOffsets.size());
    
    if (((vband <= 0) || (vband > maxVirtualBands)) && (vband > Bands())) {
      ostringstream mess;
      mess << "Requested virtual band (" << vband
           << ") outside valid (BandBin/Center) limits (1 - " << maxVirtualBands
           <<  ")";
      throw IException(IException::Programmer, mess.str(), _FILEINFO_);
    }

    Camera::SetBand(vband);
    
    if ((vband > maxVirtualBands) && (vband <= Bands())) {
      // probably switching to a band from phocube or similar
      // instead of a different filter band, so just re-use the 
      // properties from the current band. 
      return;
    }

    PushFrameCameraDetectorMap *dmap = (PushFrameCameraDetectorMap *)DetectorMap();
    dmap->SetBandFirstDetectorLine(p_detectorStartLines.at(vband-1));
    dmap->SetFrameletOffset(p_frameletOffsets.at(vband-1));

    MarciDistortionMap *distmap = (MarciDistortionMap *)DistortionMap();
    distmap->SetFilter(p_filterNumbers.at(vband-1));
    
  }



  /**
   * The camera model is band dependent, so this method returns false
   *  
   * @return bool false
   */
  bool MarciCamera::IsBandIndependent() {
    return false;
  }



  /**
   * CK frame ID -  - Instrument Code from spacit run on CK
   *
   * @return @b int The appropriate instrument code for the "Camera-matrix"
   *         Kernel Frame ID
   */
  int MarciCamera::CkFrameId() const {
    return (-74000);
  }



  /**
   * CK Reference ID - MRO_MME_OF_DATE
   *
   * @return @b int The appropriate instrument code for the "Camera-matrix"
   *         Kernel Reference ID
   */
  int MarciCamera::CkReferenceId() const {
    return (-74900);
  }



  /**
   *  SPK Reference ID - J2000
   *
   * @return @b int The appropriate instrument code for the Spacecraft
   *         Kernel Reference ID
   */
  int MarciCamera::SpkReferenceId() const {
    return (1);
  }
}


/**
 * This is the function that is called in order to instantiate a MarciCamera object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* MarciCamera
 * @internal
 *   @history 2011-05-03 Jeannie Walldren - Removed Mro namespace.
 */
extern "C" Isis::Camera *MarciCameraPlugin(Isis::Cube &cube) {
  return new Isis::MarciCamera(cube);
}
