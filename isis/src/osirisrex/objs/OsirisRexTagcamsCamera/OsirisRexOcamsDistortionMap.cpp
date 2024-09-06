/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QDebug>
#include <QtGlobal>
#include <QtMath>

#include "NaifStatus.h"
#include "OsirisRexOcamsDistortionMap.h"
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
  OsirisRexOcamsDistortionMap::OsirisRexOcamsDistortionMap(Camera *parent, 
                                                           double zDirection) 
                              : CameraDistortionMap(parent, zDirection) {

    m_detectorOriginSample = p_camera->FocalPlaneMap()->DetectorSampleOrigin();
    m_detectorOriginLine = p_camera->FocalPlaneMap()->DetectorLineOrigin();
    m_pixelPitch = p_camera->PixelPitch();
    p_tolerance = 1.0E-6;
    p_debug = false;

    // Set up our own focal plane map from the camera model. NOTE!!!
    // The CameraFocalPlaneMap must be set in the Camera object 
    // prior to calling distortion model!!!
    if ( !parent->FocalPlaneMap() ) {
        std::string mess = "FocalPlaneMap must be set in the Camera object prior to"
                       " initiating this distortion model!";
        throw IException(IException::Programmer, mess, _FILEINFO_);
    }

    m_focalMap.reset(new CameraFocalPlaneMap(*parent->FocalPlaneMap()));
  }


  /**
   * Default Destructor
   */
  OsirisRexOcamsDistortionMap::~OsirisRexOcamsDistortionMap() {
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
  void OsirisRexOcamsDistortionMap::SetDistortion(int naifIkCode) {
    // Load distortion coefficients, including filter if we have it.
    QString odkkey;


    odkkey = "INS" + toString(naifIkCode) + "_OD_K";
    try {
      for (int i = 0; i < 5; ++i) {
         p_odk.push_back(p_camera->Spice::getDouble(odkkey, i));
      }
    } 
    catch (IException &e) {
    }

    // Load center-of-distortion coordinates, including filter if we have it
    QString odcenterkey; 
    odcenterkey = "INS" + toString(naifIkCode) + "_OD_CENTER";
    m_distortionOriginSample = p_camera->Spice::getDouble(odcenterkey, 0);
    m_distortionOriginLine =   p_camera->Spice::getDouble(odcenterkey, 1);

    QString tolKey = "INS" + toString(naifIkCode) + "_TOLERANCE";
    p_tolerance = p_camera->getDouble(tolKey, 0);

    QString dbKey = "INS" + toString(naifIkCode) + "_DEBUG_MODEL";
    p_debug       = toBool(p_camera->getString(dbKey, 0));
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
  bool OsirisRexOcamsDistortionMap::SetFocalPlane(double dx, double dy) {
    if ( p_debug )std::cout << "\nUndistorting at " << dx << ", " << dy << "\n";
    p_focalPlaneX = dx; 
    p_focalPlaneY = dy;

    if ( p_debug ) {
        std::cout << "Detector sample=" << p_camera->FocalPlaneMap()->DetectorSample() 
                  << ", line=" << p_camera->FocalPlaneMap()->DetectorLine()<< "\n";
    }

    // Only apply the distortion if we have the correct number of coefficients
    if (p_odk.size() < 2) {
      p_undistortedFocalPlaneX = dx;
      p_undistortedFocalPlaneY = dy;
      return true; 
    }

    double x0 = (m_distortionOriginLine - m_detectorOriginSample) * m_pixelPitch; 
    double y0 = (m_distortionOriginSample - m_detectorOriginLine) * m_pixelPitch;
    if ( p_debug ) std::cout << "x0=" << x0 << ", y0=" << y0 << "\n";

    double xt = dx;
    double yt = dy;

    double xx, yy, r, rr, rrr, rrrr;
    double xdistortion, ydistortion;
    double xdistorted, ydistorted;
    double xprevious, yprevious;
    double drOverR;

    xprevious = 1000000.0;
    yprevious = 1000000.0;

    double tolerance = p_tolerance;

    bool bConverged = false;

    // Iterating to introduce distortion...
    // We stop when the difference between distorted coordinates
    // in successive iterations is at or below the given tolerance.
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

      xprevious = xt;
      yprevious = yt;
    }

    if(bConverged) {
      p_undistortedFocalPlaneX = xdistorted;
      p_undistortedFocalPlaneY = ydistorted;
      std::cout << "Converged ux=" << p_undistortedFocalPlaneX 
                << ", uy=" << p_undistortedFocalPlaneY << "\n";
      m_focalMap->SetFocalPlane(xdistorted, ydistorted);

      if ( p_debug ) {
          std::cout << "Detector sample=" << m_focalMap->DetectorSample() 
                    << ", line=" << m_focalMap->DetectorLine()<< "\n";
      }
    }
    if ( p_debug ) std::cout << "We out!\n";
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
  bool OsirisRexOcamsDistortionMap::SetUndistortedFocalPlane(const double ux,
      const double uy) {
    if ( p_debug ) std::cout << "\nDistorting at " << ux << ", " << uy << "\n";
    p_undistortedFocalPlaneX = ux;
    p_undistortedFocalPlaneY = uy;
    if ( p_debug ) {
        std::cout << "Detector sample=" << p_camera->FocalPlaneMap()->DetectorSample() 
                  << ", line=" << p_camera->FocalPlaneMap()->DetectorLine()<< "\n";
    }

    // Only apply the distortion if we have the correct number of coefficients.
    if (p_odk.size() < 2) {
      p_focalPlaneX = ux;
      p_focalPlaneY = uy;
      return true; 
    }
    double x0 = (m_distortionOriginLine - m_detectorOriginSample) * m_pixelPitch;
    double y0 = (m_distortionOriginSample - m_detectorOriginLine) * m_pixelPitch;

    // Compute the distance from the focal plane center. If we are
    // close to the center then no distortion is required
    double x = ux;
    double y = uy;
    double r = qSqrt(((x-x0) * (x-x0)) + ((y - y0) * (y-y0)));

    if (r <= 1.0E-6) {
      p_focalPlaneX = ux;
      p_focalPlaneY = uy;
      return true;
    }

    double r2 = r*r;
    double r3 = r2*r;
    double r4 = r2*r2;

    double drOverR = p_odk[0] + p_odk[1]*r + p_odk[2]*r2 + p_odk[3]*r3 + p_odk[4]*r4;

    p_focalPlaneX = x + drOverR * (x-x0);
    p_focalPlaneY = y + drOverR * (y-y0); 
    std::cout << "Final at " << p_focalPlaneX << ", " << p_focalPlaneY << "\n";
    m_focalMap->SetFocalPlane(p_focalPlaneX, p_focalPlaneY);
    if ( p_debug ) {
        std::cout << "Detector sample=" << m_focalMap->DetectorSample() 
                  << ", line=" << m_focalMap->DetectorLine()<< "\n";
    }
    return true;
  }
}

