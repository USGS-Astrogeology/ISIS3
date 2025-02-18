#ifndef OsirisRexOcamsOpenCVDistortionMap_h
#define OsirisRexOcamsOpenCVDistortionMap_h
/**
 * @file
 *   Unless noted otherwise, the portions of Isis written by the USGS are public
 *   domain. See individual third-party library and package descriptions for
 *   intellectual property information,user agreements, and related information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or implied,
 *   is made by the USGS as to the accuracy and functioning of such software
 *   and related material nor shall the fact of distribution constitute any such
 *   warranty, and no responsibility is assumed by the USGS in connection
 *   therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html in a browser or see
 *   the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */

#include <QSharedPointer>

#include "CameraDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "Camera.h"

namespace Isis {
  /**
   * OsirisRexOcamsOpenCVDistortionMap camera distortion model for TAGCAMS
   * 
   * This model implements a robust camera distortion model based upon the OpenCV 
   * stereo camera calibration model produced by the Kinetx NAV team for the 
   * TAGCAMS camera system. This model is described in the (white?) paper 
   * "IN-FLIGHT CALIBRATION OF THE OSIRIS-REX OPTICAL NAVIGATION IMAGERS". This 
   * model derives 10 parameters that make up the distortion model acquired during 
   * inflight and approach operations toward Bennu. 
   *  
   * This model is an image line/sample coordinate-based model that is computed 
   * from star positions in several images. The actual positions of the stars are 
   * determined where they should be seen in the undistorted image plane and the 
   * model produces the distorted location of the star inthe image. Therefore, the 
   * model is implemented such that the computation of distorted pixel location is 
   * a direct computation of the model and the undistorted location is iterative. 
   *  
   * All ten of the parameters (k1, k2, k3, p1, p2, fx, fy, cx, cy, td) are read 
   * from the instrument (addendum) kernel. The temperature of the camera head 
   * (ct) must come from the camera model (assumed to be provided in the cube 
   * header) in units of celcius. The temperature is initially set to 0 so if its 
   * not available, the variable focal length component (which is pretty small) 
   * will be excluded from the distortion. 
   *  
   * Other parameters that can be adjusted in the kernel are the convergence 
   * tolerance limit and invoking debugging output to help evaluate behavior. See 
   * $ISIS3DATA/osirisrex/kernels/iak/orex_tagcams_addendum_v05.ti. 
   *  
   * Reference Document 
   *   OIA_ORX_112_Distortion_Boresight_Memo.pdf (August 9, 2018) 
   *  
   * @author 2019-01-08 Kris Becker
   * 
   * @internal 
   *   @history 2019-01-24 Kris Becker - Removed unused variables to suppress
   *                         warnings
   *   @history 2019-01-28 Kris Becker - Cleaned up implementation: handle
   *                         option/pointing boresight offsets from center of
   *                         CCD; renamed conversion methods to more meaningful
   *                         names.
   *   @history 2019-02-04 Kris Becker Properly initialize distorted and
   *                         undistorted parameters in respective methods
   *   @history 2019-03-15 Kris Becker - Ensure init doesn't fail if debug mode
   *                         cannot be determined
   */
  class OsirisRexOcamsOpenCVDistortionMap : public CameraDistortionMap {
    public:
      /**
       *  
       * Constructs a Distortion Map object for the OSIRIS-REx TagCams Camera. 
       *  
       * @param parent Pointer to parent Camera object 
       * @param naifIkCode NAIF instrument code typically in Kernels object 
       * @param functionIkCode Special function code for an OCAMS instrument. 
       *                       the PolyCam this is the focus position code.
       *                       For the MapCam, it is the filter code. It
       *                       (currently) doesn't apply to the SamCam.
       * @param filtername Name of the filter. This is really only needed for 
       *                   the MapCam as it had no specific IK code and the
       *                   NAIF parameters had the filter name appended to
       *                   keywords.
       * @param zdir Pointing direction of the instrument boresight occasionally 
       *             required for some instrument configurations. 
       *  
       *  
       * --- NAIF distortion model parameters retained internally --- 
       * @param k1 First coefficient of radial distortion
       * @param k2 Second coefficient of radial distortion
       * @param k3 Third coefficient of radial distortio 
       * @param k4 Forth coefficient of radial distortion
       * @param k5 Fifth coefficient of radial distortion
       * @param k6 Sixth coefficient of radial distortion
       * @param p1 x tangential distortion component
       * @param p2 y tangential distortion compomen 
       * @param fx x focal length
       * @param fy y focal length
       * @param cx X distortion axis center pixel location (pixels)
       * @param cy Y distortion axis center pixel location (pixels) 
       * @param a1 Temperature dependent focal length factor 
       * @param ct Camera head temperature (C) 
       * 
       * 
       * @internal
       */
      OsirisRexOcamsOpenCVDistortionMap(Camera *parent, int naifIkCode,
                                        int functionIkCode, 
                                        const QString &filtername,
                                        const double zdir = 1.0);
 
      //! Destroys OsirisRexOcamsOpenCVDistortionMap object.
      ~OsirisRexOcamsOpenCVDistortionMap() {};
      
    /**
     * Set camera head temperature for the model 
     * 
     * 
     * @param temp Temperature of the camera (Celsius)
     */
      void SetCameraTemperature(const double temp);

      /**
       * Compute undistorted focal plane x/y
       *
       * Compute undistorted focal plane x/y given a distorted focal plane x/y.
       * fter calling this method, you can obtain the undistorted
       * x/y via the UndistortedFocalPlaneX and UndistortedFocalPlaneY methods
       *
       * @param dx Distorted focal plane x, in millimeters
       * @param dy Distorted focal plane y, in millimeters
       *
       * @return whether the conversion was successful
       */
      virtual bool SetFocalPlane(const double dx, const double dy);
      
      
      /**
       * Compute distorted focal plane x/y
       *
       * Compute distorted focal plane x/y given an undistorted focal plane x/y.
       * After calling this method, you can obtain the distorted x/y via the
       * FocalPlaneX and FocalPlaneY methods
       *
       * @param ux Undistorted focal plane x, in millimeters
       * @param uy Undistorted focal plane y, in millimeters
       *
       * @return whether the conversion was successful
       */
      virtual bool SetUndistortedFocalPlane(const double ux, const double uy);

    protected:
        void image_to_distortion_frame(const double dx, const double dy,
                                       double *u, double *v,
                                       double *xpp, double *ypp) const;
        void distortion_to_pointing_frame(const double xp, const double yp,
                                          double *x, double *y,
                                          double *ux, double *uy) const;

        void pointing_to_distortion_frame(const double ux, const double uy,
                                      double *x, double *y,
                                      double *xp, double *yp) const;
        void distortion_to_image_frame(const double xpp, const double ypp,
                                       double *u, double *v,
                                       double *dx, double *dy) const;

        void apply_distortion(const double xp, const double yp,
                              double *xpp, double *ypp) const;

      
    private:  
      // parameters below are from camera calibration report
      double p_k1;       //!< First coefficient of radial distortion.
      double p_k2;       //!< Second coefficient of radial distortion.
      double p_k3;       //!< Third coefficient of radial distortion.
      double p_k4;       //!< Fourth coefficient of radial distortion.
      double p_k5;       //!< Fifth coefficient of radial distortion.
      double p_k6;       //!< Sixth coefficient of radial distortion.
      double p_p1;       //!< Tangential x-coordinate.
      double p_p2;       //!< Tangential y-coordinate.
      double p_fx;       //!< X focal plane length
      double p_fy;       //!< Y focal plane length
      double p_cx;       //!< x optical axis center.
      double p_cy;       //!< y optical axis center.
      double p_a1 ;      //!< Temperature dependent parameter
                         //!
      double p_camTemp;  //!< Camera head temperature
      double p_tolerance; //!< Convergence tolerance
      bool   p_debug;     //!< Debug the model

      double p_xoffset;   //!< Difference in CCD center X coordinate and boresight
      double p_yoffset;   //!< Difference in CCD center Y coordinate and boresight

      QSharedPointer<CameraFocalPlaneMap> m_focalMap;  // Local focal plane map

      bool load_model(Camera *camera, const int naifIKcode = 0, const QString &suffix = "");
      bool load_polycam_model(Camera *camera, const int naifIKcode, const int focusIkCode);

  };
};
#endif
