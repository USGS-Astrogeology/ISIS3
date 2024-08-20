/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ThemisIrCamera.h"
#include "ThemisIrDistortionMap.h"

#include <QString>

#include "CameraFocalPlaneMap.h"
#include "IException.h"
#include "iTime.h"
#include "LineScanCameraDetectorMap.h"
#include "LineScanCameraGroundMap.h"
#include "LineScanCameraSkyMap.h"
#include "NaifStatus.h"

using namespace std;
namespace Isis {
  /**
   * Constructor for the Themis Ir Camera Model
   *
   * @param lab Pvl label from an Odyssey Themis Ir image.
   *
   * @internal
   *   @history 2011-05-03 Jeannie Walldren - Added NAIF error check.
   */
  ThemisIrCamera::ThemisIrCamera(Cube &cube) : LineScanCamera(cube) {
    m_instrumentNameLong = "Thermal Emission Imaging System Infrared";
    m_instrumentNameShort = "Themis-IR";
    m_spacecraftNameLong = "Mars Odyssey";
    m_spacecraftNameShort = "Odyssey";

    NaifStatus::CheckErrors();
    // Set the detector size
    SetPixelPitch(0.05);
    SetFocalLength(203.9213);

    // Get the start time.  This includes adding a time offset that could
    // have been put in the labels during ingestion (thm2isis).  This is meant
    // to handle a random timing errors which can be up to four pixels
    Pvl &lab = *cube.label();
    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);
    QString stime = QString::fromStdString(inst["SpacecraftClockCount"]);
    p_etStart = getClockTime(stime).Et();

    double offset = inst["SpacecraftClockOffset"];
    p_etStart += offset;

    // If bands have been extracted from the original image then we
    // need to read the band bin group so we can map from the cube band
    // number to the instrument band number
    PvlGroup &bandBin = lab.findGroup("BandBin", Pvl::Traverse);
    PvlKeyword &orgBand = bandBin["FilterNumber"];
    for(int i = 0; i < orgBand.size(); i++) {
      p_originalBand.push_back(std::stoi(orgBand[i]));
    }

    // Themis IR had a summing mode added.  This directly affects the line
    // rate.  That is, the seconds per line.  In the Kieffer-Torson model
    // the line rates was 33.2804 ms per line in SumMode = 1.  In the
    // Duxbury model it is 33.2871 based on 1/22/2009 email with a readout
    // rate of 30.0417 lines/second
    int sumMode = 1;
    if(inst.hasKeyword("SpatialSumming")) {
      sumMode = inst["SpatialSumming"];
    }
    p_lineRate = 33.2871 / 1000.0 * sumMode;

    // If the TDI mode is enabled then 16 line in the detector are summed
    // to improve the SNR.  In the SetBand method we will the TDI mode to
    // determine line offset for the band.
    p_tdiMode = QString::fromStdString(inst["TimeDelayIntegration"]);

    // The detector map tells us how to convert from image coordinates to
    // detector coordinates.  In our case, a (sample,line) to a (sample,time)
    // This is band dependent so it will change in SetBand
    LineScanCameraDetectorMap *detectorMap = new LineScanCameraDetectorMap(this, p_etStart, p_lineRate);
    detectorMap->SetDetectorSampleSumming(sumMode);
    detectorMap->SetDetectorLineSumming(sumMode);

    // The focal plane map tells us how to go from detector position
    // to focal plane x/y (distorted).  That is, (sample,time) to (x,y).
    // This is band dependent so it will change in SetBand
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(this, naifIkCode());

    // The boresight sample in the K-T model was 164.25.  In Duxbury's it is
    // 160.5 or half the detector width.  The detector offset varies by band
    // and is set to the proper value for band 1 for now
    focalMap->SetDetectorOrigin(160.5, 0.0);
    focalMap->SetDetectorOffset(0.0, 120.5 - 8.5);

    // The camera has a distortion map which scales in the X direction,
    // effectively a variable focal length, and an independent Y direction.
    // Both are based on the band number
    new ThemisIrDistortionMap(this);

    // Setup the ground and sky map
    new LineScanCameraGroundMap(this);
    new LineScanCameraSkyMap(this);

    LoadCache();
    NaifStatus::CheckErrors();
  }


  /**
   * Change the THEMIS IR camera parameters based on the band
   * number
   *
   * @param vband The band number to set
   *
   * @author 2009-02-26 Jeff Anderson
   */
  void ThemisIrCamera::SetBand(const int vband) {
    // Lookup the original band from the band bin group.  Unless there is
    // a reference band which means the data has all been aligned in the
    // band dimension
    int band;
    if(HasReferenceBand()) {
      band = ReferenceBand();
      if((band < 1) || (band > 10)) {
        string msg = "Invalid Reference Band [" + IString(band) + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    else {
      if(vband > (int) p_originalBand.size()) {
        string msg = "Band number out of array bounds in ThemisIRCamera";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
      band = p_originalBand[vband-1];
    }

    // Get the detector line in the CCD.  If TDI mode is enabled then
    // we used the middle of the 16 lines which were summed.  Otherwise
    // an individual line was read out.  It is not clear where/how the
    // noTDI offsets were derived.  Hopefully from some THEMIS document.
    // They were copied from ISIS2 code and the only suspicious value is
    // at 52.  Also, Duxbury's 1/22/09 email used 128.5 vs 129.5 for the
    // center of the TDI enable detector line positions.
    double detectorLine;
    if(p_tdiMode == "ENABLED") {
      double bandDetector_TDI[] = {8.5, 24.5, 50.5, 76.5, 102.5,
                                   128.5, 154.5, 180.5, 205.5, 231.5
                                  };
      detectorLine = bandDetector_TDI[band-1];
    }
    else {
      int bandDetector_noTDI[] = {9, 24, 52, 77, 102, 129, 155, 181, 206, 232};
      detectorLine = (double) bandDetector_noTDI[band-1];
    }

    // Compute the time offset for this band (using detector line)
    // Subtracting 1.0 as in Duxbury's example would be the offset to
    // the center of the pixel but we want to the top edge so we
    // subtract 0.5
    p_bandTimeOffset = (detectorLine - 0.5) * p_lineRate;
    p_bandTimeOffset /= this->DetectorMap()->LineScaleFactor();

    // Adjust the starting time in the detector map for this band
    LineScanCameraDetectorMap *detectorMap =
      (LineScanCameraDetectorMap *) this->DetectorMap();
    detectorMap->SetStartTime(p_etStart + p_bandTimeOffset);

    // Compute the along track offset at this detector line.  That is,
    // the number of pixels away from the boresight line.  In the K-T model
    // the boresight line was 109.5.  In Duxbury's it is half of the
    // detector height or 120.5
    double alongtrackOffset = 120.5 - detectorLine;

    // Adjust alongtrackOffset using Kirk's empirically fitted numbers from
    // Apr 2009
    double empiricalOffset[] = { -0.076, -0.098, -0.089, -0.022, 0.0,
                                 -0.020, -0.005, -0.069, 0.025, 0.0
                               };
    alongtrackOffset += empiricalOffset[band-1];
    this->FocalPlaneMap()->SetDetectorOffset(0.0, alongtrackOffset);

    // Adjust the sample boresight using Kirk's empirically fitted numbers
    // from Apr 2009
    double sampleBoresight = 160.5;
    double empiricalBoresightOffset[] = { 0.021, 0.027, 0.005, 0.005, 0.0,
                                          -0.007, -0.012, -0.039, -0.045, 0.0
                                        };
    sampleBoresight -= empiricalBoresightOffset[band-1];
    this->FocalPlaneMap()->SetDetectorOrigin(sampleBoresight, 0.0);

    // Finally, adjust the optical distortion model based on the band
    ThemisIrDistortionMap *distMap =
      (ThemisIrDistortionMap *) DistortionMap();
    distMap->SetBand(band);
  }
}


// Plugin
/**
 * This is the function that is called in order to instantiate a
 * ThemisIrCamera object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* ThemisIrCamera
 * @internal
 *   @history 2011-05-03 Jeannie Walldren - Removed Odyssey namespace.
 */
extern "C" Isis::Camera *ThemisIrCameraPlugin(Isis::Cube &cube) {
  return new Isis::ThemisIrCamera(cube);
}
