/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "CameraFocalPlaneMap.h"

#include <cmath>

#include <QDebug>
#include <QVector>

#include "Affine.h"
#include "Camera.h"
#include "Spice.h"

namespace Isis {
  /** Construct mapping between detectors and focal plane x/y
   *
   * @param parent      parent camera that will use this map
   * @param naifIkCode  code of the naif instrument for reading coefficients
   *
   */
  CameraFocalPlaneMap::CameraFocalPlaneMap(Camera *parent, const int naifIkCode) {
    Init(parent, naifIkCode);
  }


  /** Construct mapping between detectors and focal plane x/y
   *
   * @param naifIkCode  code of the naif instrument for reading coefficients
   *
   */
  CameraFocalPlaneMap::CameraFocalPlaneMap(const int naifIkCode) {
    Init(0, naifIkCode);
  }


 /**
  * Added new method to allow programmer to pass in affine coefficients
  * 
  * @author janderson (3/25/2014)
  * 
  * @param parent Parent camera
  * @param affine Affine transform containing the coefficients for both 
  *               transforms to (samp,line) to (x,y) and reverse
  */
  CameraFocalPlaneMap::CameraFocalPlaneMap(Camera *parent, Affine &affine) {
    p_detectorSampleOrigin = 0.0;
    p_detectorLineOrigin = 0.0;
    p_detectorSampleOffset = 0.0;
    p_detectorLineOffset = 0.0;
    p_camera = parent;

    p_transx[0]  = affine.Coefficients(1)[2];
    p_transx[1]  = affine.Coefficients(1)[0];
    p_transx[2]  = affine.Coefficients(1)[1];

    p_transy[0]  = affine.Coefficients(2)[2];
    p_transy[1]  = affine.Coefficients(2)[0];
    p_transy[2]  = affine.Coefficients(2)[1];

    p_itranss[0]  = affine.InverseCoefficients(1)[2];
    p_itranss[1]  = affine.InverseCoefficients(1)[0];
    p_itranss[2]  = affine.InverseCoefficients(1)[1];

    p_itransl[0]  = affine.InverseCoefficients(2)[2];
    p_itransl[1]  = affine.InverseCoefficients(2)[0];
    p_itransl[2]  = affine.InverseCoefficients(2)[1];

    if (parent != 0) {
      p_camera->SetFocalPlaneMap(this);
    }
  }


  /**
   * Destructor
   * 
   */
  CameraFocalPlaneMap::~CameraFocalPlaneMap(){
  }


  /**
   * Initialize the focal plane map to its default state 
   *  
   * @param parent Parent camera
   * @param naifIkCode  code of the naif instrument for reading coefficients
   */
  void CameraFocalPlaneMap::Init(Camera *parent, const int naifIkCode) {
    p_detectorSampleOrigin = 0.0;
    p_detectorLineOrigin = 0.0;
    p_detectorSampleOffset = 0.0;
    p_detectorLineOffset = 0.0;
    p_camera = parent;

    if (naifIkCode != 0) {
      QString xkey  = "INS" + QString::fromStdString(toString(naifIkCode)) + "_TRANSX";
      QString ykey  = "INS" + QString::fromStdString(toString(naifIkCode)) + "_TRANSY";
      QString ixkey = "INS" + QString::fromStdString(toString(naifIkCode)) + "_ITRANSS";
      QString iykey = "INS" + QString::fromStdString(toString(naifIkCode)) + "_ITRANSL";
      for (int i = 0; i < 3; ++i) {
        p_transx[i]  = parent->getDouble(xkey, i);
        p_transy[i]  = parent->getDouble(ykey, i);
        p_itranss[i] = parent->getDouble(ixkey, i);
        p_itransl[i] = parent->getDouble(iykey, i);
      }
    }
    else {
      QString xkey  = "IDEAL_TRANSX";
      QString ykey  = "IDEAL_TRANSY";
      QString ixkey = "IDEAL_TRANSS";
      QString iykey = "IDEAL_TRANSL";
      for (int i = 0; i < 3; ++i) {
        p_transx[i]  = parent->getDouble(xkey, i);
        p_transy[i]  = parent->getDouble(ykey, i);
        p_itranss[i] = parent->getDouble(ixkey, i);
        p_itransl[i] = parent->getDouble(iykey, i);
      }
    }

    if (parent != 0) {
      p_camera->SetFocalPlaneMap(this);
    }
  }


  /** Compute detector position (sample,line) from focal plane coordinates. NOTE: This is detector
   *  (sample, line) not necessarily image (sample, line). If the image was reformatted from what
   *  was collected the two differ. See the New Horizons LEISA camera for an example (LEISA is a
   *  frame camera being treated as a line scan camera).
   *
   * This method will compute both the centered and normal detector position
   * given a distorted focal plane coordinate.
   *
   * @param dx distorted focal plane x in millimeters
   * @param dy distorted focal plane y in millimeters
   *
   * @return conversion was successful
   */
  bool CameraFocalPlaneMap::SetFocalPlane(const double dx, const double dy) {
    p_focalPlaneX = dx;
    p_focalPlaneY = dy;

    p_centeredDetectorSample = p_itranss[0] + (p_itranss[1] * dx) + (p_itranss[2] * dy);
    p_centeredDetectorLine   = p_itransl[0] + (p_itransl[1] * dx) + (p_itransl[2] * dy);
    ComputeUncentered();
    return true;
  }


  /** Compute distorted focal plane coordinate from detector position (sampel,line)
   *
   * This method will compute both the distorted focal plane x/y and centered
   * detector position given a detector position
   *
   * @param sample undistorted focal plane x in millimeters
   * @param line undistorted focal plane y in millimeters
   *
   * @return conversion was successful
   */
  bool CameraFocalPlaneMap::SetDetector(const double sample, const double line) {
    p_detectorSample = sample;
    p_detectorLine = line;
    ComputeCentered();
    p_focalPlaneX = p_transx[0] + (p_transx[1] * p_centeredDetectorSample) + (p_transx[2] * p_centeredDetectorLine) ;
    p_focalPlaneY = p_transy[0] + (p_transy[1] * p_centeredDetectorSample) + (p_transy[2] * p_centeredDetectorLine) ;
    return true;
  }


  /** Return the focal plane x dependency variable
   *
   * This method returns the image variable (sample or line) on
   * which the focal plane x depends.
   *
   * @return dependency variable
   */
//  CameraFocalPlaneMap::FocalPlaneXDependencyType CameraFocalPlaneMap::FocalPlaneXDependency() {
  int CameraFocalPlaneMap::FocalPlaneXDependency() {
    if (p_transx[1] > p_transx[2]) {
      return Sample;
    }
    else {
      return Line;
    }
  }


  /** Return the sign of the p_transx coefficient with the greatest magnitude
   *
   * This method returns a +1. or -1. based on the sign of the p_transx
   * coefficient with the greatest magnitude.  Only p_transx[1] and
   * p_transx[2] are compared since p_transx[0] is used as a constant in the
   * affine transformation.
   *
   * @return sign of most significant coefficient
   */
  double CameraFocalPlaneMap::SignMostSigX()  {
    double magCoef1 = fabs(p_transx[1]);
    double magCoef2 = fabs(p_transx[2]);

    if (magCoef1 > magCoef2) {
      return (magCoef1 / p_transx[1]);
    }
    else {
      return (magCoef2 / p_transx[2]);
    }
  }


  /** Return the sign of the p_transy coefficient with the greatest magnitude
   *
   * This method returns a +1 or -1 based on the sign of the p_transy
   * coefficient with the greatest magnitude.  Only p_transy[1] and
   * p_transy[2] are compared since p_transy[0] is used as a constant in the
   * affine transformation.
   *
   * @return sign of most significant coefficient
   */
  double CameraFocalPlaneMap::SignMostSigY()  {
    double magCoef1 = fabs(p_transy[1]);
    double magCoef2 = fabs(p_transy[2]);

    if (magCoef1 > magCoef2) {
      return (magCoef1 / p_transy[1]);
    }
    else {
      return (magCoef2 / p_transy[2]);
    }
  }


  /**
   * @returns The distorted focal plane x
   */
  double CameraFocalPlaneMap::FocalPlaneX() const {
    return p_focalPlaneX;
  }


  /**
   * @returns The distorted focal plane y
   */
  double CameraFocalPlaneMap::FocalPlaneY() const {
    return p_focalPlaneY;
  }


  /**
   * @returns The detector sample
   */
  double CameraFocalPlaneMap::DetectorSample() const {
    return p_detectorSample;
  }


  /**
   * @returns The detector line
   */
  double CameraFocalPlaneMap::DetectorLine() const {
    return p_detectorLine;
  }


  /**
   * @returns The centered detector sample
   */
  double CameraFocalPlaneMap::CenteredDetectorSample() const {
    return p_centeredDetectorSample;
  }


  /**
   * @returns The centered detector line
   */
  double CameraFocalPlaneMap::CenteredDetectorLine() const {
    return p_centeredDetectorLine;
  }


  /** Set the detector origin
   *
   * This is used to set the origin of the detector.  Typically the middle
   * of the detector.  For example, a 512x512 dectector would have the
   * origin at (256.5,256.5).  If not set both are 0.
   *
   * @param sample  detector sample at the origin
   * @param line    detector line at the origin
   */
  void CameraFocalPlaneMap::SetDetectorOrigin(const double sample, const double line) {
    p_detectorSampleOrigin = sample;
    p_detectorLineOrigin = line;
  }


  /**
   * @returns The detector line origin
   */
  double CameraFocalPlaneMap::DetectorLineOrigin() const {
    return p_detectorLineOrigin;
  }


  /**
   * @returns The detector sample origin
   */
  double CameraFocalPlaneMap::DetectorSampleOrigin() const {
    return p_detectorSampleOrigin;
  }


  /** Set the detector offset
   *
   * This is used to set the offset between the detector origin and
   * the average location in detector pixels where the image is being
   * viewed.  If not set the offset are both 0.0
   *
   * @param sampleOffset sample offset in pixels
   * @param lineOffset sample offset in lines
   */
  void CameraFocalPlaneMap::SetDetectorOffset(const double sampleOffset,
                                const double lineOffset) {
    p_detectorSampleOffset = sampleOffset;
    p_detectorLineOffset = lineOffset;
  }


  /**
   * @returns The detector line offset
   */
  double CameraFocalPlaneMap::DetectorLineOffset() const {
    return p_detectorLineOffset;
  }


  /**
   * @returns The detector sample offset
   */
  double CameraFocalPlaneMap::DetectorSampleOffset() const {
    return p_detectorSampleOffset;
  }


  /**
   * Set the affine coefficients for converting destorted (x,y) to a detector Line
   * 
   * @param transL Vector of the affine coefficients
   */
  void CameraFocalPlaneMap::SetTransL(const QVector<double> transL) {
   for (int i=0; i<3; ++i) {
     p_itransl[i] = transL[i];
   }
  }


  /**
   * Set the affine coefficients for converting destorted (x,y) to a detector Sample
   * 
   * @param transS Vector of the affine coefficients
   */
  void CameraFocalPlaneMap::SetTransS(const QVector<double> transS) {
   for (int i=0; i<3; ++i) {
     p_itranss[i] = transS[i];
   }
  }


  /**
   * Set the affine coefficients for converting detector (sample,line) to a distorted X
   * 
   * @param transX Vector of the affine coefficients
   */
  void CameraFocalPlaneMap::SetTransX(const QVector<double> transX) {
   for (int i=0; i<3; ++i) {
     p_transx[i] = transX[i];
   }
  }


  /**
   * Set the affine coefficients for converting detector (sample,line) to a distorted Y
   * 
   * @param transY Vector of the affine coefficients
   */
  void CameraFocalPlaneMap::SetTransY(const QVector<double> transY) {
   for (int i=0; i<3; ++i) {
     p_transy[i] = transY[i];
   }
  }


  /**
   * @return The affine coefficients for converting detector (sample,line) to a distorted X
   */
  const double *CameraFocalPlaneMap::TransX() const{
    return p_transx;
  }


  /**
   * @returns The affine coefficients for converting detector (sample,line) to distorted Y
   */
  const double *CameraFocalPlaneMap::TransY() const{
    return p_transy;
  }


  /**
   * @returns The affine coefficients for converting distorted (x,y) to a detector Sample
   */
  const double *CameraFocalPlaneMap::TransS() const{
    return p_itranss;
  }


  /**
   * @returns The affine coefficients for converting distorted (x,y) to a detector Line
   */
  const double *CameraFocalPlaneMap::TransL() const{
    return p_itransl;
  }


  //! Convenience method to center detector origin (use when inheriting)
  void CameraFocalPlaneMap::ComputeCentered() {
    p_centeredDetectorSample = p_detectorSample - p_detectorSampleOrigin;
    p_centeredDetectorLine   = p_detectorLine   - p_detectorLineOrigin;
  }


  //! Convenience method to center detector origin (use when inheriting)
  void CameraFocalPlaneMap::ComputeUncentered() {
    p_detectorSample = p_centeredDetectorSample + p_detectorSampleOrigin;
    p_detectorLine   = p_centeredDetectorLine   + p_detectorLineOrigin;
  }

}

