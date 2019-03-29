/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QDebug>
#include <QtGlobal>
#include <QtMath>

#include "NaifStatus.h"
#include "OsirisRexDistortionMap.h"
#include "CameraFocalPlaneMap.h"

namespace Isis {
  /**
   * OSIRIS REx Camera distortion map constructor
   *
   * Create a camera distortion map for OSIRIS REx's.
   *
   * This class maps between distorted and undistorted
   * focal plane x/y's. The default mapping is the identity, that is,
   * the focal plane x/y and undistorted focal plane x/y will be
   * identical.
   *
   * @param parent        the parent camera that will use this distortion map
   * @param zDirection    the direction of the focal plane Z-axis
   *                      (either 1 or -1)
   *
   */
  OsirisRexDistortionMap::OsirisRexDistortionMap(Camera *parent, double zDirection)
      : CameraDistortionMap(parent, zDirection) {

    // Set up our own focal plane map from the camera model. NOTE!!!
    // The CameraFocalPlaneMap must be set in the Camera object 
    // prior to calling distortion model!!!
    if ( !parent->FocalPlaneMap() ) {
        QString mess = "FocalPlaneMap must be set in the Camera object prior to"
                       " initiating this distortion model!";
        throw IException(IException::Programmer, mess, _FILEINFO_);
    }

    // Replicate focal plane map for proper image coordinate conversions
    m_focalMap.reset(new CameraFocalPlaneMap(*parent->FocalPlaneMap()));

    m_detectorOriginSample = parent->FocalPlaneMap()->DetectorSampleOrigin();
    m_detectorOriginLine = parent->FocalPlaneMap()->DetectorLineOrigin();

    m_pixelPitch = p_camera->PixelPitch();
    m_debug = false;
  }


  /**
   * Default Destructor
   */
  OsirisRexDistortionMap::~OsirisRexDistortionMap() {
   }


  /**
   *  Load distortion coefficients and center-of-distortion for OCAMS
   *
   * This method loads the distortion coefficients from the instrument
   * kernel.  OCAMS's coefficients in the NAIF instrument kernel are
   * expected to be in one of the following forms:
   *
   * @code
   * INS-64361_OD_K_FILTER = (2.21E-05, 1.71E-04, 5.96E-05, 0.00E+00, 0.00E+00)
   * INS-64361_OD_CENTER_FILTER = (486.2, 450.3)
   * @endcode
   *
   * Or if the distortion is not filter-dependent:
   *
   * @code
   * INS-64361_OD_K = (2.21E-05, 1.71E-04, 5.96E-05, 0.00E+00, 0.00E+00)
   * INS-64361_OD_CENTER = (486.2, 450.3)
   * @endcode
   *
   *
   * @param naifIkCode    Code to search for in instrument kernel
   */
  bool OsirisRexDistortionMap::SetDistortion(int naifIkCode, QString filter) {
    // Load distortion coefficients, including filter if we have it.
    QString odkkey;

    if (filter.toUpper().compare("UNKNOWN") == 0) {
      odkkey = "INS" + toString(naifIkCode) + "_OD_K";
    }
    else {
      odkkey = "INS" + toString(naifIkCode) + "_OD_K_" + filter.trimmed().toUpper();
    }

    try {
      for (int i = 0; i < 5; ++i) {
         p_odk.push_back(p_camera->Spice::getDouble(odkkey, i));
      }
    }
    catch (IException &e) {
      // This means that this is an older image without a filter provided
      // don't update p_odk, we will not apply the distortion in this case
      m_distortionOriginSample = -1;
      m_distortionOriginLine = -1;
      if ( m_debug ) std::cout << "Bad Distortion Model - set to -1\n";
      return (false);
    }

    // Load center-of-distortion coordinates, including filter if we have it
    QString odcenterkey;
    if (filter.toUpper().compare("UNKNOWN") == 0) {
      odcenterkey = "INS" + toString(naifIkCode) + "_OD_CENTER";
    }
    else {
      odcenterkey = "INS" + toString(naifIkCode) + "_OD_CENTER_" + filter.trimmed().toUpper();
    }
    m_distortionOriginSample = p_camera->Spice::getDouble(odcenterkey, 0);
    m_distortionOriginLine =   p_camera->Spice::getDouble(odcenterkey, 1);

    try {
      QString dbKey = "INS" + toString(naifIkCode) + "_DEBUG_MODEL";
      m_debug     = toBool(p_camera->getString(dbKey, 0));        
    } catch (IException &ie) {
        m_debug = false;
    }


    if ( m_debug) {
      std::cout << "\nModel Initialized! - IKCode = " << naifIkCode 
                << ", Filter = " << filter << "\n";
      std::cout << "Detector   Center Samp = " << m_detectorOriginSample
                << ", Line = " << m_detectorOriginLine << "\n";
      std::cout << "Distortion Center Samp = " << m_distortionOriginSample
                << ", Line = " << m_distortionOriginLine << "\n";
    }

    return (true);
  }


  /**
   * Compute undistorted focal plane x/y
   *
   * Compute undistorted focal plane x/y given a distorted focal plane x/y.
   * After calling this method, you can obtain the undistorted
   * x/y via the UndistortedFocalPlaneX and UndistortedFocalPlaneY methods
   *
   * @param dx distorted focal plane x in millimeters
   * @param dy distorted focal plane y in millimeters
   *
   * @return if the conversion was successful
   */
  bool OsirisRexDistortionMap::SetFocalPlane(double dx, double dy) {
    p_focalPlaneX = dx;
    p_focalPlaneY = dy;

    if ( m_debug ) {
        m_focalMap->SetFocalPlane(dx, dy);
        std::cout << "\nCompute Undistorted at dx = " << dx 
             << ", dy = " << dy << "\n";
        std::cout << "DLine = " << m_focalMap->DetectorLine() 
             << ", DSamp = " << m_focalMap->DetectorSample() << "\n";
    }
    
    // Only apply the distortion if we have the correct number of coefficients
    if (p_odk.size() < 2) {
      p_undistortedFocalPlaneX = dx;
      p_undistortedFocalPlaneY = dy;
      return true;
    }

    double x0 = (m_distortionOriginLine - m_detectorOriginSample) * m_pixelPitch;
    double y0 = (m_distortionOriginSample - m_detectorOriginLine) * m_pixelPitch;
    if ( m_debug) {
      std::cout << "Detector   Center Samp = " << m_detectorOriginSample
                << ", Line = " << m_detectorOriginLine << "\n";
      std::cout << "Distortion Center Samp = " << m_distortionOriginSample
                << ", Line = " << m_distortionOriginLine << "\n";
      std::cout << "x0 = " << x0 << ", y0 = " << y0 << "\n";
      std::cout << "xP = " << x0 / m_pixelPitch << ", yP = " << y0 / m_pixelPitch << "\n";
    }
    double xt = dx;
    double yt = dy;

    double xx, yy, r, rr, rrr, rrrr;
    double xdistortion, ydistortion;
    double xdistorted, ydistorted;
    double xprevious, yprevious;
    double drOverR;

    xprevious = 1000000.0;
    yprevious = 1000000.0;

    double tolerance = 0.000001;

    bool bConverged = false;

    // Iterating to introduce distortion...
    // We stop when the difference between distorted coordinates
    // in successive iterations is at or below the given tolerance.
    int nrevs(0);
    for(int i = 0; i < 50; i++) {
      xx = (xt-x0) * (xt-x0);
      yy = (yt-y0) * (yt-y0);
      rr = xx + yy;
      r = qSqrt(rr);
      rrr = rr * r;
      rrrr = rr * rr;

      drOverR = p_odk[0] + p_odk[1]*r + p_odk[2]*rr + p_odk[3]*rrr + p_odk[4]*rrrr;

      // distortion at the current point location
      xdistortion = drOverR * (xt-x0);
      ydistortion = drOverR * (yt-y0);

      // updated image coordinates
      xt = dx - xdistortion;
      yt = dy - ydistortion;

      xdistorted = xt;
      ydistorted = yt;

      // check for convergence
      if((fabs(xt - xprevious) <= tolerance) && (fabs(yt - yprevious) <= tolerance)) {
        bConverged = true;
        break;
      }

      
      if ( m_debug ) {
          std::cout << "i=" << i << ", xt=" << xt << ", yt=" << yt 
               << ", xdist=" << xdistortion 
               << ", ydist=" << ydistortion
               << ", xtol=" << (xt - xprevious) 
               << ", ytol=" << (yt - yprevious) << "\n";
      }

      nrevs++;
      xprevious = xt;
      yprevious = yt;

    }

    if ( m_debug ) {
        std::cout << "Loop terminated after " << nrevs << " iterations! - converged? "
                  << ( ( bConverged ) ? "Yes!" : "No-(") << "\n";
    }

    if ( bConverged ) {
      p_undistortedFocalPlaneX = xdistorted;
      p_undistortedFocalPlaneY = ydistorted;
    }

    if ( m_debug ) {
        m_focalMap->SetFocalPlane(p_undistortedFocalPlaneX, p_undistortedFocalPlaneY);
        std::cout << "Undistorted ux = " << p_undistortedFocalPlaneX 
                  << ", uy = " << p_undistortedFocalPlaneY << "\n";
        std::cout << "ULine = " << m_focalMap->DetectorLine() 
                  << ", USamp = " << m_focalMap->DetectorSample() << "\n";
    }
    return bConverged;
  }


  /**
   * Compute distorted focal plane x/y
   *
   * Compute distorted focal plane x/y given an undistorted focal plane x/y.
   *
   * After calling this method, you can obtain the distorted x/y via the
   * FocalPlaneX and FocalPlaneY methods
   *
   * @param ux undistorted focal plane x in millimeters
   * @param uy undistorted focal plane y in millimeters
   *
   * @return if the conversion was successful
   * @see SetDistortion
   */
  bool OsirisRexDistortionMap::SetUndistortedFocalPlane(const double ux,
      const double uy) {
    p_undistortedFocalPlaneX = ux;
    p_undistortedFocalPlaneY = uy;

    if ( m_debug ) {
        m_focalMap->SetFocalPlane(ux, uy);
        std::cout << "\nCompute Distorted at ux = " << ux 
             << ", uy = " << uy << "\n";
        std::cout << "ULine = " << m_focalMap->DetectorLine() 
             << ", USamp = " << m_focalMap->DetectorSample() << "\n";
    }

    // Only apply the distortion if we have the correct number of coefficients.
    if (p_odk.size() < 2) {
      p_focalPlaneX = ux;
      p_focalPlaneY = uy;
      return true;
    }
    double x0 = (m_distortionOriginLine - m_detectorOriginSample) * m_pixelPitch;
    double y0 = (m_distortionOriginSample - m_detectorOriginLine) * m_pixelPitch;

    if ( m_debug ) {
      std::cout << "Detector   Center Samp = " << m_detectorOriginSample
                << ", Line = " << m_detectorOriginLine << "\n";
      std::cout << "Distortion Center Samp = " << m_distortionOriginSample
                << ", Line = " << m_distortionOriginLine << "\n";
      std::cout << "x0 = " << x0 << ", y0 = " << y0 << "\n";
      std::cout << "xP = " << x0 / m_pixelPitch << ", yP = " << y0 / m_pixelPitch << "\n";
    }

    // Compute the distance from the focal plane center. If we are
    // close to the center then no distortion is required
    double x = ux;
    double y = uy;
    double r = qSqrt(((x-x0) * (x-x0)) + ((y - y0) * (y-y0)));

    if (r <= 1.0E-6) {
      p_focalPlaneX = ux;
      p_focalPlaneY = uy;
      if ( m_debug ) {
        std::cout << "Degenerate case at center of distortion!\n";
      }
      return true;
    }

    double r2 = r*r;
    double r3 = r2*r;
    double r4 = r2*r2;

    double drOverR = p_odk[0] + p_odk[1]*r + p_odk[2]*r2 + p_odk[3]*r3 + p_odk[4]*r4;

    p_focalPlaneX = x + drOverR * (x-x0);
    p_focalPlaneY = y + drOverR * (y-y0); 

    if ( m_debug ) {
        m_focalMap->SetFocalPlane(p_focalPlaneX, p_focalPlaneY);
        std::cout << "Distorted dx = " << p_focalPlaneX 
                  << ", dy = " << p_focalPlaneY << "\n";
        std::cout << "DLine = " << m_focalMap->DetectorLine() 
                  << ", DSamp = " << m_focalMap->DetectorSample() << "\n";
    }
    return true;
  }
}
