/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "RollingShutterCameraDetectorMap.h"

#include "iTime.h"

#include <QtMath>
#include <utility>
#include <vector>


namespace Isis {
  /**
   * Constructs a RollingShutterCameraDetectorMap
   *
   * @param times A std::vector of normalized readout times
   * @param coeffs A std::vector of the coefficients of the polynomial that fit the jitter. The
   *                  coefficients are from highest degree to lowest degree.
   * @param parent The parent camera that uses the detector map.
   */
  RollingShutterCameraDetectorMap::RollingShutterCameraDetectorMap(
        Camera *parent,
        std::vector<double> times,
        std::vector<double> sampleCoeffs,
        std::vector<double> lineCoeffs) : CameraDetectorMap(parent) {
    m_times = times;
    m_sampleCoeffs = sampleCoeffs;
    m_lineCoeffs = lineCoeffs;
  }


  /**
   * Destructor
   */
  RollingShutterCameraDetectorMap::~RollingShutterCameraDetectorMap() {

  }


  /**
   * Compute detector position from a parent image coordinate
   *
   * This method will compute the detector position from the parent
   * line/sample coordinate
   *
   * @param sample Sample number in the parent image
   * @param line Line number in the parent image
   *
   * @return conversion successful
   */
  bool RollingShutterCameraDetectorMap::SetParent(const double sample,
                                                  const double line) {
    return RollingShutterCameraDetectorMap::SetParent(sample, line, 0.0);
  }


  /**
   * Compute detector position from a parent image coordinate
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
  bool RollingShutterCameraDetectorMap::SetParent(const double sample,
                                                  const double line,
                                                  const double deltaT) {
    std::pair<double, double> jittered = removeJitter(sample, line);
    p_parentSample = jittered.first;
    p_parentLine = jittered.second;
    p_detectorSample = (p_parentSample - 1.0) * p_detectorSampleSumming + p_ss;
    p_detectorLine   = (p_parentLine   - 1.0) * p_detectorLineSumming + p_sl;
    if (p_camera->isTimeSet()) {
      p_camera->setTime(p_camera->time().Et() + deltaT);
    }
    return true;
  }


  /**
   * Compute parent position from a detector coordinate
   *
   * This method will compute a parent sample/line given a
   * detector coordinate
   *
   * @param sample Sample number in the detector
   * @param line Line number in the detector
   *
   * @return conversion successful
   */
  bool RollingShutterCameraDetectorMap::SetDetector(const double sample, const double line) {
    p_detectorSample = sample;
    p_detectorLine   = line;
    p_parentSample   = (p_detectorSample - p_ss) / p_detectorSampleSumming + 1.0;
    p_parentLine     = (p_detectorLine   - p_sl) / p_detectorLineSumming   + 1.0;
    std::pair<double, double> jittered = applyJitter(p_parentSample, p_parentLine);
    p_parentSample = jittered.first;
    p_parentLine = jittered.second;
    return true;
  }


  /**
   * Iteratively finds a solution to "apply" jitter to an image coordinate.
   * Each iteration adds jitter to the original image coordinate until it finds
   * an image coordinate that maps back to to the original image coordinate when
   * jitter is removed. This is similar to how we handle applying radial distortion.
   *
   * Note: If the jitter varies significantly (>1 pixel difference) then it is possible
   * for there to be multiple solutions to the inverse problem and it is impossible to
   * know which one to choose.
   *
   * @param sample Image sample to apply jitter to.
   * @param line Image line to apply jitter to.
   *
   * @return std::pair<double,double> Returns the image coordinate with jitter applied to it.
   */
  std::pair<double, double> RollingShutterCameraDetectorMap::applyJitter(const double sample,
                                                                         const double line) {
    std::pair<double, double> jittered = removeJitter(sample, line);
    double currentSample = sample;
    double currentLine = line;

    int iterations = 0;
    int maxIterations = 50;

    while((qFabs(sample - jittered.first) > 1e-7) ||
          (qFabs(line - jittered.second) > 1e-7)) {

      currentSample = sample + (currentSample - jittered.first);
      currentLine = line + (currentLine - jittered.second);

      jittered = removeJitter(currentSample, currentLine);

      iterations++;
      if (iterations > maxIterations) {
        std::string message = "Max Iterations reached.";
        throw IException(IException::Unknown, message, _FILEINFO_);
      }

    }

    return std::pair<double, double>(currentSample, currentLine);
  }

  /**
   * Remove the distortion from the image (parent) coordinates.
   *
   * This is a helper function for the SetParent method.
   *
   * @param sample Input (parent) image sample coordinate.
   * @param line   Input (parent) image line coordinate.
   *
   * @return std::pair<double,double> Returns a pair of doubles corresponding to the de-jittered
   *         sample and line. This is in image coordinates.
   */
  std::pair<double, double> RollingShutterCameraDetectorMap::removeJitter(const double sample,
                                                                          const double line) {
    // De-jitter equation in form:
    //   c1(t^n) + c2(t^(n-2)) + ... + cn(t)
    double sampleDejitter = 0.0;
    double lineDejitter = 0.0;
    // Note that # sample coeffs == # line coeffs
    for (unsigned int n = 1; n <= m_lineCoeffs.size(); n++) {
      double sampleCoeff = m_sampleCoeffs[n - 1];
      double lineCoeff = m_lineCoeffs[n - 1];
      // Round to nearest line
      int timeEntry = 0;
      // Any lines that round past last time entry will use last time entry as index
      if (line > m_times.size()) {
        timeEntry = m_times.size();
      }
      else {
        timeEntry = (int)round(line);
      }
      double time = m_times[timeEntry - 1];
      int exponent = m_lineCoeffs.size() - (n - 1);
      time = pow(time, exponent);
      sampleDejitter += (sampleCoeff * time);
      lineDejitter += (lineCoeff * time);
    }
    return std::pair<double, double>(sample - sampleDejitter, line - lineDejitter);
  }
}
