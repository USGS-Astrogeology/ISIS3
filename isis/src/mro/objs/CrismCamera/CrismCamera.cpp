/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "CrismCamera.h"

#include <fstream>
#include <iostream>
#include <iomanip>

#include <QString>

//#include "CrismCameraGroundMap.h"
//#include "CrismDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "LineScanCameraDetectorMap.h"
#include "LineScanCameraGroundMap.h"
#include "LineScanCameraSkyMap.h"
#include "Constants.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "NaifStatus.h"
#include "SpecialPixel.h"

using namespace std;

namespace Isis {
  /**
   * Constructor for the MRO CRISM Camera Model
   *
   * @param [in] lab   (Pvl &)  Label used to create camera model
   *
   * @internal
   *   @history 2012-04-12  Kris Becker, Flagstaff Original Version
   *
   */
  CrismCamera::CrismCamera(Cube &cube) : LineScanCamera(cube), m_lineRates(),
                                       m_isBandDependent(true) {
    m_instrumentNameLong = "Compact Reconnaissance Imaging Spectrometer for Mars";
    m_instrumentNameShort = "CRISM";
    m_spacecraftNameLong = "Mars Reconnaissance Orbiter";
    m_spacecraftNameShort = "MRO";
    NaifStatus::CheckErrors();

    Pvl &lab = *cube.label();

    PvlGroup inst = lab.findGroup("Instrument", Pvl::Traverse);

    // SensorId = S (VNIR), = L (IR) = J (JOINT)
    QString sensor = QString::fromStdString(inst ["SensorId"]);

    // Prepare instrument code
    QString ikCode(toString(naifIkCode()));

    // Set Frame mounting.  Same for both (VNIR, IR) detectors
    SetFocalLength();
    SetPixelPitch();

    // Get the start and end time in et
    double etStart = getEtTime(QString::fromStdString(inst ["SpacecraftClockStartCount"]));
    double etStop  = getEtTime(QString::fromStdString(inst ["SpacecraftClockStopCount"]));


    //  Compute the exposure time of the first line and the line rate.  This
    //  algorithm is taken from the CRISM instrument kernel, mro_crism_v10.ti,
    //  at the time of development.
//    double framesPerSec = (double) inst["FrameRate"];
    int    exposure     = (int)    inst["ExposureParameter"];

    // calculate seconds for a full frame
//    double frame_time = 1.0 / framesPerSec;

    // calculate seconds per pixel clock
//    double pixel_clock_time = frame_time / 83333.0;

    // This is what John Hayes does in the DPU (Data Processing Unit), to write
    // register to FPU (Focal Plane Unit) specifying how long NOT to integrate,
    // in pixel clocks [0..83333]
    int reg = ((480 - exposure) * 83333) / 480;

    // Actual integration starts 3 line-times later, rounded up to next line
    // time
    int start_clocks = reg + (3 * 166);
    if (start_clocks % 166) start_clocks += 166 - (start_clocks % 166);

    // integration continues 4 line-times after de-assertion
//    int stop_clocks = 83333 + (4 * 166);

//    double start_time = start_clocks * pixel_clock_time;
//    double stop_time  = stop_clocks  * pixel_clock_time;

    // Start of first line exposure time.  This is the start time of the
    // frame time plus the itegration delay start time - constant for all
    // frames
//    double obsStartTime(etStart+start_time);
//    double obsStopTime(obsStartTime+(stop_time*ParentLines())-start_time);
//    double obsEndTime(obsStartTime+(frame_time*(ParentLines()))-start_time);

    double frameStartTime(etStart);
//    double frameStopTime(frameStartTime+(stop_time*(ParentLines())));
//    double frameEndTime(frameStartTime+(frame_time*(ParentLines())));

//    double lineTime((etStop-etStart+frame_time)/(ParentLines()));


    //  Compute the sclk and UTC of the specifed line for cropping purposes
#if 0
     iTime myLineStartTime(etStart  + ((25.0 - 1.0) * frame_time));
//     cout << "\nLine 25 Start Times...\n";
     SpiceChar sclk[80];
     (void) sce2s_c(naifSclkCode(), myLineStartTime.Et(), sizeof(sclk), sclk);
//     cout << "UTC@Line 25:  " << myLineStartTime.UTC() << "\n";
//     cout << "SCLK@Line 25: " << sclk << "\n";

#endif
    //  Setup detector map
#if 1
    double jaTime = (etStop-etStart)/ParentLines();
    new LineScanCameraDetectorMap(this, frameStartTime, jaTime);
//    new LineScanCameraDetectorMap(this, frameStartTime, frame_time);
#else
    // Have to use variable line scan detector mapping due to how line scans
    // are performed.  This is currently segfaulting...

    double stime(obsStartTime);
    double scanTime(stop_time-start_time);
    m_lineRates.clear();
    for (int i = 0 ; i < ParentLines() ; i++) {
      m_lineRates.push_back(LineRateChange(i+1, stime, scanTime));
      stime += frame_time;
    }
    m_lineRates.push_back(LineRateChange(ParentLines()+1,stime-start_time,start_time));
    double endTime(stime-frame_time+scanTime);
    new VariableLineScanCameraDetectorMap(this, m_lineRates);
#endif

    int binning = inst["PixelAveragingWidth"];
    DetectorMap()->SetDetectorSampleSumming(binning);
    DetectorMap()->SetDetectorLineSumming(1.0);  // Line dimension never binned

    // Setup focal plane map
    CameraFocalPlaneMap *fmap = new CameraFocalPlaneMap(this, naifIkCode());

    // lines and samples added to the pvl in the order you
    // call getDouble()
    double bLine = getDouble("INS"+ikCode+"_BORESIGHT_LINE");
    double bSample = getDouble("INS"+ikCode+"_BORESIGHT_SAMPLE");

    fmap->SetDetectorOrigin(bSample, bLine);
    fmap->SetDetectorOffset(0.0, 0.0);

    // Setup distortion map
    //new CrismDistortionMap(this);
    new CameraDistortionMap(this);


    // Setup the ground and sky map
    new LineScanCameraGroundMap(this);
//    new CrismCameraGroundMap(this);
    new LineScanCameraSkyMap(this);

    setTime(iTime(frameStartTime));
    double tol = 0.0; //PixelResolution();
    if(tol < 0.) {
      // Alternative calculation of .01*ground resolution of a pixel
      tol = PixelPitch() * SpacecraftAltitude() / FocalLength() / 1000. / 100.;
    }

//    cout << "\nCreateCache(" << frameStartTime << ", " << frameEndTime << ")...\n";
#if 0
//    cout << "CacheSize: " << CacheSize(obsStartTime, obsStopTime) << "\n";
    createCache(obsStartTime, obsStopTime, ParentLines(), tol);

#else
//    cout << "LoadCache()...\n";
    LoadCache();
#endif
//    cout << "Done.\n";
    NaifStatus::CheckErrors();
    return;
  }

  void CrismCamera::SetBand (const int physicalBand) {
    return;
  }

  bool CrismCamera::IsBandIndependent () {
    return (m_isBandDependent);
  }


  double CrismCamera::getEtTime(const QString &sclk) {
    return (getClockTime(sclk, -74999).Et());
  }
}


// Plugin
/**
 * This is the function that is called in order to instantiate a CrismCamera object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* CrismCamera
 * @internal
 */
extern "C" Isis::Camera *CrismCameraPlugin(Isis::Cube &cube) {
  return new Isis::CrismCamera(cube);
}
