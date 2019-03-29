/**
 * @file
 *
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
#include "OsirisRexOcamsOpenCVDistortionMap.h"
#include "CameraFocalPlaneMap.h"

using namespace std;
namespace Isis {
  /**
   * Constructs a Distortion Map object for the OSIRIS-REx OCAMS Cameras. 
   *  
   * @param parent     Pointer to parent Camera object 
   * @param naifIkCode Integer code of the instrument (PolyCam, MapCam, SamCam) 
   * @param zdir       Direction of boresight out of detector 
   *  
   * Here are the parameters read from the IAK kernel for the OpenCV model:
   *  
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
   * @param td Temperature dependent focal length adjustment 
   *  
   * @param ct Camera head temperature (C)  (Manually set by camera model)
   * 
   * @internal 
   * @history 2019-01-22 Kris Becker  Renamed model parameter names for 
   *                       clarification of the OpenCV model
   *                       documentation/description. Updated to add the k4-k6
   *                       parameters.
   * @history 2019-01-25 Kris Becker Updated documentation some 
   * @history 2019-02-04 Kris Becker Properly initialize distortion/undistorted 
   *                       parameters in respective methods. 
   */
  OsirisRexOcamsOpenCVDistortionMap::OsirisRexOcamsOpenCVDistortionMap(Camera *parent,
                                                               int naifIkCode,
                                                               int functionIkCode,
                                                               const QString &filtername,
                                                               const double zdir)
                                                 : CameraDistortionMap(parent, zdir) {

    // Set some initial values
    p_camTemp   = 0.0;
    p_tolerance = 1.0E-10;
    p_debug     = false;

    bool success(false);
    if ( -64360 == naifIkCode ) {
        // PolyCam has unique model for each focal position
        success = load_polycam_model(parent, naifIkCode, functionIkCode);
    }
    else if ( -64361 == naifIkCode ) {
        // MapCam has a unique model per filter that is expressed in a NAIF ID
        QString fsfx("");
        if ( !filtername.isNull() ) fsfx = QString("_").append(filtername.toUpper());

        if ( !( success = load_model( parent, functionIkCode ) ) ) {
          success = load_model(parent, naifIkCode, fsfx);
        }
    }
    else { // if ( -64362 == naifIkCode ) // This is for the SamCam
        success = load_model(parent, naifIkCode);
    }

    // If all fail, use a default (none) model
    if ( !success ) success = load_model(parent, 0);
    

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

    // The OpenCV model's optical center is also the pointing boresight so 
    // ensure this is accounted for in the distortion model. Note the offset
    // should be added when resolving ground intersections and subtracted 
    // when back projecting ground coordinates to detector (see 
    // pointing_to_distortion_frame() and distortion_to_pointing_frame()
    // methods, respectively).
    m_focalMap->SetDetector(p_cx, p_cy);
    p_xoffset = 0.0 - m_focalMap->FocalPlaneX();
    p_yoffset = 0.0 - m_focalMap->FocalPlaneY();

    if ( p_debug ) {
      cout << "\n--> OREX OpenCV Distortion Model Initialization <--\n";
      cout << " NaifIkCode     = " << naifIkCode << "\n";
      cout << " FunctionIkCode = " << functionIkCode << "\n";
      cout << " Filtername     = " << filtername << "\n";
      cout << " K1             = " << p_k1 << "\n";
      cout << " K2             = " << p_k2 << "\n";
      cout << " K3             = " << p_k3 << "\n";
      cout << " K4             = " << p_k4 << "\n";
      cout << " K5             = " << p_k5 << "\n";
      cout << " K6             = " << p_k6 << "\n";
      cout << " Fx             = " << p_fx << "\n";
      cout << " Fy             = " << p_fy << "\n";
      cout << " Cx             = " << p_cx << "\n";
      cout << " Cy             = " << p_cy << "\n";
      cout << " P1             = " << p_p1 << "\n";
      cout << " P2             = " << p_p2 << "\n";
      cout << " A1             = " << p_a1 << "\n";
      cout << " Tolerance      = " << p_tolerance << "\n";

      cout << " XOffset        = " << p_xoffset << "\n";
      cout << " YOffset        = " << p_yoffset << "\n\n";

    }
    
    return;
  }

  /**
   * Set camera head temperature for the model 
   * 
   * @param temp Temperature of the camera (Celsius)
   */
  void OsirisRexOcamsOpenCVDistortionMap::SetCameraTemperature(const double temp) {
      p_camTemp = temp;
      if ( p_debug )  cout << "CameraHeadTemperature = " << p_camTemp << "\n";
      return;
  }

  /**
   * Compute undistorted focal plane x/y
   *
   * Compute undistorted focal plane x/y given a distorted focal plane dx/dy.
   * 
   * After calling this method, you can obtain the undistorted x/y via the
   * UndistortedFocalPlaneX and UndistortedFocalPlaneY methods
   *
   * @param dx Distorted focal plane x, in millimeters
   * @param dy Distorted focal plane y, in millimeters
   *
   * @return whether the conversion was successful
   */
  bool OsirisRexOcamsOpenCVDistortionMap::SetFocalPlane(const double dx, const double dy) {
    if ( p_debug ) cout << "\nUndistorting FP at " << dx << ", " << dy << "\n";
    // Handle degenerate case should convergence fail!! Adjust for center of
    // pointing boresight!
    p_focalPlaneX = dx;
    p_focalPlaneY = dy;
    p_undistortedFocalPlaneX = dx + p_xoffset;
    p_undistortedFocalPlaneY = dy + p_yoffset;

    double u, v, xpp, ypp;
    image_to_distortion_frame(dx, dy, &u, &v, &xpp, &ypp);
    if ( p_debug ) cout << "xpp=" << xpp << ", ypp=" << ypp << "\n";
    
    // With no distortion, xp == xpp and yp == ypp.    
    double xp = xpp;
    double yp = ypp;

    // Provided by the user/model
    double tolerance = p_tolerance;
    bool bConverged = false;

    // iterating to introduce undistortion...
    // we stop when the difference between undistorted coordinates
    // in successive iterations is at or below the given tolerance
    double xdistortion, ydistortion;
    double xdistorted, ydistorted;
    int nrevs(0);
    for(int i = 0; i < 50; i++) {

      // Apply the distortion...
      apply_distortion(xp, yp, &xdistorted, &ydistorted);

      // updated image coordinates
      xdistortion = xpp - xdistorted;
      ydistortion = ypp - ydistorted;

      // check for convergence
      if((fabs(xdistortion) <= tolerance) && (fabs(ydistortion) <= tolerance)) {
        bConverged = true;
        break;
      }

      nrevs++;
      if ( p_debug ) {
          cout << "i=" << i << ", xp=" << xp << ", yp=" << yp 
               << ", xdist=" << xdistortion << ", ydist=" << ydistortion << "\n";
      }

      // Update for next loop toward direction of convergence.
      // Ok this is dependent upon the direction of distortion. In this
      // context, the fisheye model, the distortion is toward the corners,
      // so add the difference in the distortion.
      xp = xp + xdistortion;
      yp = yp + ydistortion;
    }

    if ( p_debug ) {
        std::cout << "Loop terminated after " << nrevs << " iterations! - converged? "
                  << ( ( bConverged ) ? "Yes!" : "No-(") << "\n";
    }

    if( bConverged ) {
      double x, y;
      // Boresight alignment handled here
      distortion_to_pointing_frame(xp, yp, &x, &y, 
                                   &p_undistortedFocalPlaneX, 
                                   &p_undistortedFocalPlaneY);
    }

    return bConverged;
  }

  /**
   * @brief Compute distorted focal plane x/y
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
  bool OsirisRexOcamsOpenCVDistortionMap::SetUndistortedFocalPlane(const double ux,
                                                               const double uy) {
    if ( p_debug )  cout << "\nDistorting FP at " << ux << ", " << uy << "\n";
    // image coordinates prior to introducing distortion
    p_undistortedFocalPlaneX = ux;
    p_undistortedFocalPlaneY = uy;

    p_focalPlaneX = ux - p_xoffset;
    p_focalPlaneY = uy - p_yoffset;

    double x, y, xp, yp;
    // Boresight alignment handled here
    pointing_to_distortion_frame(ux, uy, &x, &y, &xp, &yp);
    if ( p_debug ) cout << "xp=" << xp << ", yp=" << yp << "\n";

    double xpp, ypp;
    apply_distortion(xp, yp, &xpp, &ypp);
    if ( p_debug ) cout << "xpp=" << xpp << ", ypp=" << ypp << "\n";

    double u,v;
    distortion_to_image_frame(xpp, ypp, &u, &v,
                              &p_focalPlaneX, 
                              &p_focalPlaneY);
    if ( p_debug ) cout << "Final FP ux=" << p_focalPlaneX << ", uy=" << p_focalPlaneY << "\n\n";

    return true;
  }


/**
 * Compute distortion model point from distorted focal plane coordinates 
 *  
 * This method computes (x'',y'') from ISIS3 distorted focal plane coordinates
 * (dx,dy). 
 *  
 * The (ux,uy) focal plane points are used to compute the image pixel 
 * coordinates that the OpenCV model is based upon. 
 * 
 * @author 2019-01-11 Kris Becker Original Version
 * 
 * @param dx  ISIS3 undistorted focal plane x coordinate
 * @param dy  ISIS3 undistorted focal plane y coordinate
 * @param u   Returns ISIS3 image sample cordinate
 * @param v   Returns ISIS3 image line cordinate
 * @param xpp Distortion model x'' coordinate
 * @param ypp Distortion model y'' coordinate
 */
  void OsirisRexOcamsOpenCVDistortionMap::image_to_distortion_frame(const double dx, 
                                                                const double dy,
                                                                double *u, 
                                                                double *v,
                                                                double *xpp, 
                                                                double *ypp) const {

    // Get the current undistorted pixel coordinate
    m_focalMap->SetFocalPlane(dx, dy);
    *u = m_focalMap->DetectorSample();
    *v = m_focalMap->DetectorLine();
    if ( p_debug ) cout << "Detector sample=" << *u << ", line=" << *v << "\n";

    // Normalize to get the xpp/ypp parameters. This is the target
    // of the undistorted pixel location and what our loop will
    // converge to.
    double deltaT = p_a1 * p_camTemp;
    *xpp = ( *u - p_cx ) / ( p_fx * (1.0 + deltaT) );
    *ypp = ( *v - p_cy ) / ( p_fy * (1.0 + deltaT) );

    return;
  }

/**
 * Compute ISIS3 undistorted focal plane coordinates from model 
 *  
 * This method computes the undistorted ISIS3 focal plane coordinates (dx,dy) 
 * resulting from the inverse of the OpenCV distortion model unitless (x',y') 
 * coordinates. 
 *  
 * Essentially, the conversion to focal plane coordinates need to be scaled to 
 * image coordinates by dividing by the pixel pitch and multiplying by the focal 
 * length (z). Finally the image coordinate is adjusted by decentering from the 
 * optical axis center providing (x,y). 
 *  
 * Then, (x,y) is used to compute the ISIS3 distorted focal plane coordinates 
 * (dx, dy). 
 * 
 * @author 2019-01-11 Kris Becker Original Version
 * 
 * @param xp Distortion model x' coordinate
 * @param yp Distortion model y' coordinate
 * @param x  Returns ISIS3 distorted image sample coordinate
 * @param y  Returns ISIS3 distorted image line coordinate
 * @param dx Returns ISIS3 x-axis distorted focal plane coordinate
 * @param dy Returns ISIS3 y-axis distorted focal plane coordinate
 */
  void OsirisRexOcamsOpenCVDistortionMap::distortion_to_pointing_frame(const double xp, 
                                                                   const double yp,
                                                                   double *x, 
                                                                   double *y,
                                                                   double *ux,
                                                                   double *uy) const {

    double pixel_pitch  = p_camera->PixelPitch();
    double focal_length = p_camera->FocalLength();

    *x = ( xp / pixel_pitch * focal_length ) + p_cx;
    *y = ( yp / pixel_pitch * focal_length ) + p_cy;

    // Center to detector pointing boresight relative to CCD center
    m_focalMap->SetDetector(*x, *y);
    *ux = m_focalMap->FocalPlaneX() + p_xoffset;
    *uy = m_focalMap->FocalPlaneY() + p_yoffset;

    if ( p_debug ) {
        std::cout << "ux=" << *ux<< ", uy=" << *uy << "\n";
        std::cout << "Detector sample=" << m_focalMap->DetectorSample() 
                  << ", line=" << m_focalMap->DetectorLine()<< "\n";
    }

    return;
  }


/**
 *  @brief Normalize ISIS3 focal plane coordinate to model point
 *  
 *  This method abtracts the conversion from the ISIS3 focal plane point (x,y)
 *  to the model point (x',y').
 *  
 *  The OpenCV model is derived from (sample,line) image coordinates whereas
 *  ISIS3 distortions are typically generated from focal plane (dx, dy)
 *  coordinaetes.
 *  
 *  The unitless (x',y') coordinates are computed from the focal plane (dx,dy)
 *  by converting to image coordinates, divide by focal length and multiply by
 *  the pixel pitch (z). Both these values are in millimeters.
 * 
 * @author 2019-01-11 Kris Becker Original Version
 * 
 * @param dx ISIS3 distorted focal plane x coordinate
 * @param dy ISIS3 distorted focal plane y coordinate
 * @param x  Returns ISIS3 image sample coordinate
 * @param y  Returns ISIS3 image line coordinate
 * @param xp Returns distortion model x' coordinate
 * @param yp Returns distortion model y' coordinate
 */
  void OsirisRexOcamsOpenCVDistortionMap::pointing_to_distortion_frame(const double ux, 
                                                                   const double uy,
                                                                   double *x,
                                                                   double *y,
                                                                   double *xp, 
                                                                   double *yp) const {

    // For normalizing the focal plane coordinates
    double pixel_pitch  = p_camera->PixelPitch();
    double focal_length = p_camera->FocalLength();

    // Get the current sample/line as this is the basis of the distortion 
    // model. We must get original sample/line for this so the z factor
    // is a bit different. Also, the offset from the CCD center to the
    // pointing boresight is applied here.
    m_focalMap->SetFocalPlane(ux - p_xoffset, uy - p_yoffset);
    *x = m_focalMap->DetectorSample();
    *y = m_focalMap->DetectorLine();
    if ( p_debug ) cout << "Detector sample=" << *x << ", line=" << *y << "\n";

    // Normalized input into OpenCV model
    *xp = (*x - p_cx) / focal_length * pixel_pitch;
    *yp = (*y - p_cy) / focal_length * pixel_pitch;

    return;
  }


/**
 * @brief Converts a distortion model coordinate to ISIS3 focal plane coordinate
 *  
 * This method computes ISIS3 (u,v) and (dx,dy) coordinates from the result of 
 * the distortion model that is applied to (x',y'). 
 *  
 * The (x'',y'') point is scaled by the axis focal length temperature dependent 
 * correction (fx, fy) and then decentered from the optics center (cx, cy) to 
 * produce (u,v). 
 *  
 * As a convenience, the ISIS3 focal plane coordinates are also computed. 
 * 
 * @author 2019-01-11 Kris Becker Original Version
 * 
 * @param xpp Distortion model x'' coordinate
 * @param ypp Distortion model y'' coordinate
 * @param u   Returns ISIS3 image sample cordinate
 * @param v   Returns ISIS3 image line cordinate
 * @param dx  Returns ISIS3 x-axis distorted focal plane coordinate
 * @param dy  Returns ISIS3 y-axis distorted focal plane coordinate
 */
  void OsirisRexOcamsOpenCVDistortionMap::distortion_to_image_frame(const double xpp, 
                                                                const double ypp,
                                                                double *u, 
                                                                double *v,
                                                                double *dx,
                                                                double *dy) const {
    double deltaT = p_a1 * p_camTemp;
    *u = p_fx * ( 1.0 + deltaT ) * xpp + p_cx;
    *v = p_fy * ( 1.0 + deltaT ) * ypp + p_cy;
    if ( p_debug ) cout << "u=" << *u << ", v=" << *v << "\n";

    // Use the new image pixel coordinate to compute the new focal plane coordinate
    m_focalMap->SetDetector(*u, *v);

    *dx = m_focalMap->FocalPlaneX();
    *dy = m_focalMap->FocalPlaneY();

    return;

  }

/**
 * @brief Apply OpenCV model equations to normalized parameters 
 *  
 * This method abstracts the OpenCV distortion model equations that are 
 * appled to the normalized focal plane coordinates (x',y'). The normalized 
 * focal plane coordinate system is centered at the optics axis origin and are 
 * unitless. To convert from ISIS3 focal plane coordinates, one only needs to 
 * multiply the coordinates by the pixel pitch and divide by the focal length. 
 *  
 * The result of the model is the distortion coordinates (x'',y''). 
 * 
 * @author 2019-01-15 Kris Becker
 * 
 * @param xp   Normalized x' coordinate
 * @param yp   Normalized y' coordinate
 * @param xpp  Returns the x'' parameter after applying distortion model 
 * @param ypp  Returns the y'' parameter after applying distortion model
 */
  void OsirisRexOcamsOpenCVDistortionMap::apply_distortion(const double xp, 
                                                       const double yp,
                                                       double *xpp, 
                                                       double *ypp) const {

    // Apply the OpenCV distortion model to the normalized focal plane
    // coordinates
    double r2 = xp * xp + yp * yp;
    double r4 = r2 * r2;
    double r6 = r2 * r4;

    //  dr is the radial distortion contribution
    double dr = ( 1.0 + (p_k1 * r2) + (p_k2 * r4) + (p_k3 * r6) ) /
                ( 1.0 + (p_k4 * r2) + (p_k5 * r4) + (p_k6 * r6) );

    // Tangential contributions
    double dt_x = ( 2.0 * p_p1 * xp * yp ) + ( ( p_p2 * r2 ) + ( p_p2 * 2.0 * xp * xp ) );
    double dt_y = ( ( ( p_p1 * r2 ) + ( p_p1 * 2.0 * yp * yp ) ) + ( 2.0 * p_p2 * xp * yp ) );

    // Image coordinates corrected for principal point, radial and decentering distortion
    *xpp = ( dr * xp ) + dt_x;
    *ypp = ( dr * yp ) + dt_y;

    return;
  }


/**
 * @brief Retrieve IAK parameters for PolyCam OpenCV model 
 *  
 * This method will attempt to read the appropriate parameters from the IAK 
 * for the PolyCam instrument. This is not so trival since there are 90 focus 
 * settings that affect the focal length as well as distortion characterization.
 * 
 * @author 2019-02-19 Kris Becker Original Version
 * 
 * @param camera       ISIS3 Camera instance for the OCAMS instrument
 * @param naifIkCode   Instrument/frame NAIF ID code
 * @param focusIkCode  PolyCam focus setting which provides unique focal length
 * 
 * @return bool        True if successful, false if fails
 */
  bool OsirisRexOcamsOpenCVDistortionMap::load_polycam_model(Camera *camera, 
                                                             const int naifIkCode, 
                                                             const int focusIkCode) {

    bool success(false);
    if ( !( success = load_model( camera, focusIkCode ) ) ) {
      if (  (success = load_model(camera, naifIkCode) ) ) {
          // Generic model for non-specific PolyCam distortion
          p_fx = p_fy = camera->FocalLength() / camera->PixelPitch();
          p_p1 = p_p2 = 0.0;
      }
    }
    return (success);
  }


  bool OsirisRexOcamsOpenCVDistortionMap::load_model(Camera *camera, 
                                                     const int naifIkCode,
                                                     const QString &suffix) {


    // Initialization of the model default parameters.
    // It sets the center of the boresight to be (513.0, 513.0) as
    // is used for all the instruments
    double focalLength = camera->FocalLength();
    double pixelPitch  = camera->PixelPitch();

    p_k1 = 0.0;
    p_k2 = 0.0;
    p_k3 = 0.0;
    p_k4 = 0.0;
    p_k5 = 0.0;
    p_k6 = 0.0;

    p_p1 = 0.0;
    p_p2 = 0.0;

    p_fx = focalLength / pixelPitch;
    p_fy = focalLength / pixelPitch;

    p_cx = 513.0;
    p_cy = 513.0;

    p_a1 = 0.0;

    p_tolerance = 1.0E-10;
    p_debug = false;

    // Retrieve model parameters if non-zero NAIF code
    if ( 0 != naifIkCode ) { 
         // Define kernel keywords to fetch
        QString ikCode = toString(naifIkCode);
        QString odkKey = "INS" + ikCode + "_OPENCV_OD_K" + suffix;
        QString ppKey  = "INS" + ikCode + "_OPENCV_OD_P" + suffix;
        QString flKey  = "INS" + ikCode + "_OPENCV_OD_F" + suffix;
        QString cbKey  = "INS" + ikCode + "_OPENCV_OD_C" + suffix;
        QString tdKey  = "INS" + ikCode + "_OPENCV_OD_A" + suffix;
        QString tolKey = "INS" + ikCode + "_TOLERANCE";
        QString dbKey  = "INS" + ikCode + "_DEBUG_MODEL";

        try {
            
          // Fetch the values for the distortion model
          p_k1 = camera->getDouble(odkKey, 0);
          p_k2 = camera->getDouble(odkKey, 1);
          p_k3 = camera->getDouble(odkKey, 2);
          p_k4 = camera->getDouble(odkKey, 3);
          p_k5 = camera->getDouble(odkKey, 4);
          p_k6 = camera->getDouble(odkKey, 5);
          p_p1 = camera->getDouble(ppKey, 0);
          p_p2 = camera->getDouble(ppKey, 1);
          p_fx = camera->getDouble(flKey, 0);
          p_fy = camera->getDouble(flKey, 1);
          p_cx = camera->getDouble(cbKey, 0);
          p_cy = camera->getDouble(cbKey, 1);
          p_a1 = camera->getDouble(tdKey, 0);

          p_tolerance = camera->getDouble(tolKey, 0);

          // Don't fail if we didn't find the debug option
          try {
            p_debug  = toBool(camera->getString(dbKey, 0));
          } catch (IException &ie) {
            p_debug = false;
          }

        } 
        catch (IException &ie) {
          // If any keyword retrieval fails
          return (false);
        }
    }
    return (true);
  }

}
