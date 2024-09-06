/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "CameraDetectorMap.h"
#include "iTime.h"

namespace Isis {
  /** Default constructor assumes no summing and starting detector offsets
   *
   * @param parent Camera that will use this detector map
   */
  CameraDetectorMap::CameraDetectorMap(Camera *parent) {
    p_camera = parent;
    p_startingDetectorSample = 1.0;
    p_startingDetectorLine = 1.0;
    p_detectorSampleSumming = 1.0;
    p_detectorLineSumming = 1.0;
    Compute();
    if (parent != 0) {
      p_camera->SetDetectorMap(this);
    }
  }


  //! Destructor
  CameraDetectorMap::~CameraDetectorMap() {
  }


  /** Compute parent position from a detector coordinate
   *
   * This method will compute a parent sample/line given a
   * detector coordinate
   *
   * @param sample Sample number in the detector
   * @param line Line number in the detector
   *
   * @return conversion successful
   */
  bool CameraDetectorMap::SetDetector(const double sample, 
                                      const double line) {
    p_detectorSample = sample;
    p_detectorLine   = line;
    p_parentSample   = (p_detectorSample - p_ss) / p_detectorSampleSumming + 1.0;
    p_parentLine     = (p_detectorLine   - p_sl) / p_detectorLineSumming   + 1.0;
    return true;
  }


  /** Compute detector position from a parent image coordinate
   *
   * This method will compute the detector position from the parent
   * line/sample coordinate
   *
   * @param sample Sample number in the parent image
   * @param line Line number in the parent image
   *  
   * @return conversion successful
   */
  bool CameraDetectorMap::SetParent(const double sample, 
                                    const double line) {
    return CameraDetectorMap::SetParent(sample, line, 0.0);
  }


  /** Compute detector position from a parent image coordinate
   *
   * This method will compute the detector position from the parent
   * line/sample coordinate and an offset from the currently set time 
   * in seconds. If the time has not already been set, the input 
   * offset is not applied.  
   * 
   * @param sample Sample number in the parent image
   * @param line Line number in the parent image
   * @param deltaT option time offset from center of exposure in seconds
   *  
   * @return conversion successful
   */
  bool CameraDetectorMap::SetParent(const double sample, 
                                    const double line, 
                                    const double deltaT) {
    p_parentSample = sample;
    p_parentLine = line;
    p_detectorSample = (p_parentSample - 1.0) * p_detectorSampleSumming + p_ss;
    p_detectorLine   = (p_parentLine   - 1.0) * p_detectorLineSumming + p_sl;
    if (p_camera->isTimeSet()) { 
      p_camera->setTime(p_camera->time().Et() + deltaT); 
    }
    return true;
  }


  /** 
   * Compute new offsets whenenver summing or starting sample/lines change
   */
  void CameraDetectorMap::Compute() {
    p_ss = (p_detectorSampleSumming / 2.0) + 0.5 +
           (p_startingDetectorSample - 1.0);

    p_sl = (p_detectorLineSumming / 2.0) + 0.5 +
           (p_startingDetectorLine - 1.0);
  }


  /**
   * Return the starting detector sample adjusted for summation
   * 
   * @returns (double) The starting sample
   */
  double CameraDetectorMap::AdjustedStartingSample() const {
    return p_ss;
  }


  /**
   * Return the starting detector line adjusted for summation
   * 
   * @returns (double) The starting line
   */
  double CameraDetectorMap::AdjustedStartingLine() const {
    return p_sl;
  }


  /**
   * Return parent sample
   * 
   * @returns (double) The parent sample
   */
  double CameraDetectorMap::ParentSample() const {
    return p_parentSample;
  }


  /**
   * Return parent line
   * 
   * @returns (double) The parent line
   */
  double CameraDetectorMap::ParentLine() const {
    return p_parentLine;
  }


  /**
   * Return detector sample
   * 
   * @returns (double) The detector sample
   */
  double CameraDetectorMap::DetectorSample() const {
    return p_detectorSample;
  }


  /**
   * Return detector line
   * 
   * @returns (double) The detector line
   */
  double CameraDetectorMap::DetectorLine() const {
    return p_detectorLine;
  }


  /**
   * Return scaling factor for computing sample resolution
   * 
   * @returns (double) The scaling factor for sample resolution
   */
  double CameraDetectorMap::SampleScaleFactor() const {
    return p_detectorSampleSumming;
  }


  /**
   * Return scaling factor for computing line resolution
   * 
   * @returns (double) The scaling factor for line resolution
   */
  double CameraDetectorMap::LineScaleFactor() const {
    return p_detectorLineSumming;
  }


  /**
   * Return the line collection rate (0 for framing cameras)
   * 
   * @returns (double) The line collection rate
   */
  double CameraDetectorMap::LineRate() const {
    return 0.0;
  }


  /**
   * This virtual method is for returning the exposure duration of a given pixel.
   * 
   * For framing cameras, exposure duration is not available so it throws an error.
   * 
   * @param sample The sample of the desired pixel.
   * @param line The line of the desired pixel.
   * @param band The band of the desired pixel.
   * 
   * @return @b double The exposure duration for the desired pixel in seconds.
   * 
   * @throws IException::Programmer "Exposure duration is only available for LineScan, 
   *         VariableLineScan, and PushFrame Cameras."
   */
  double CameraDetectorMap::exposureDuration(const double sample,
                                             const double line,
                                             const int band) const {
    std::string msg = "Exposure duration is only available for LineScan, VariableLineScan, "
                  "and PushFrame Cameras."; 
    throw IException(IException::Programmer, msg, _FILEINFO_);

    // This should never return
    return -1;
  }
}

