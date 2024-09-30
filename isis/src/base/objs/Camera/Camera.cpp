/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Camera.h"


#include <algorithm>
#include <cfloat>
#include <cmath>
#include <iomanip>
#include <stdint.h>

#include <QDebug>
#include <QList>
#include <QPair>
#include <QString>
#include <QTime>
#include <QVector>

#include "MathUtils.h"

#include "Angle.h"
#include "Constants.h"
#include "CameraDetectorMap.h"
#include "CameraFocalPlaneMap.h"
#include "CameraDistortionMap.h"
#include "CameraGroundMap.h"
#include "CameraSkyMap.h"
#include "DemShape.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "Latitude.h"
#include "Longitude.h"
#include "NaifStatus.h"
#include "Projection.h"
#include "ProjectionFactory.h"
#include "RingPlaneProjection.h"
#include "ShapeModel.h"
#include "SpecialPixel.h"
#include "SurfacePoint.h"
#include "Target.h"
#include "TProjection.h"

using namespace std;

namespace Isis {

  /**
   * @brief Constructs the Camera object.
   * @param cube The Pvl label from the cube is used to create the Camera object.
   */
  Camera::Camera(Cube &cube) : Sensor(cube) {

    m_instrumentId = QString::fromStdString(cube.label()->findGroup("Instrument",
                        PvlObject::FindOptions::Traverse).findKeyword("InstrumentId")[0]);

    m_instrumentNameLong = "Unknown";
    m_instrumentNameShort = "Unknown";
    m_spacecraftNameLong = "Unknown";
    m_spacecraftNameShort = "Unknown";
    // Get the image size which can be different than the alpha cube size
    p_lines = cube.lineCount();
    p_samples = cube.sampleCount();
    p_bands = cube.bandCount();

    SetGeometricTilingHint();

    // Get the AlphaCube information
    p_alphaCube = new AlphaCube(cube);

    // Get the projection group if it exists
    Pvl &lab = *cube.label();
    if (lab.findObject("IsisCube").hasGroup("Mapping")) {
      p_projection = ProjectionFactory::CreateFromCube(lab);
    }
    else {
      p_projection = NULL;
    }
    p_ignoreProjection = false;

    // Initialize stuff
    p_focalLength = 0.0;
    p_pixelPitch = 1.0;
    p_referenceBand = 0;
    p_childBand = 1;

    p_distortionMap = NULL;
    p_focalPlaneMap = NULL;
    p_detectorMap = NULL;
    p_groundMap = NULL;
    p_skyMap = NULL;

    // See if we have a reference band
    PvlGroup &inst = lab.findObject("IsisCube").findGroup("Instrument");
    if (inst.hasKeyword("ReferenceBand")) {
      p_referenceBand = inst["ReferenceBand"];
    }

    p_groundRangeComputed = false;
    p_raDecRangeComputed = false;
    p_ringRangeComputed = false;
    p_pointComputed = false;
  }

  //! Destroys the Camera Object
  Camera::~Camera() {
    if (p_projection) {
      delete p_projection;
      p_projection = NULL;
    }

    if (p_alphaCube) {
      delete p_alphaCube;
      p_alphaCube = NULL;
    }

    if (p_distortionMap) {
      delete p_distortionMap;
      p_distortionMap = NULL;
    }

    if (p_focalPlaneMap) {
      delete p_focalPlaneMap;
      p_focalPlaneMap = NULL;
    }

    if (p_detectorMap) {
      delete p_detectorMap;
      p_detectorMap = NULL;
    }

    if (p_groundMap) {
      delete p_groundMap;
      p_groundMap = NULL;
    }

    if (p_skyMap) {
      delete p_skyMap;
      p_skyMap = NULL;
    }
  }


  /**
   * @brief Sets the sample/line values of the image to get the lat/lon values.
   *
   * @param sample Sample coordinate of the cube.
   * @param line Line coordinate of the cube.
   * @return @b bool Returns True if the image was set successfully and False if it
   *              was not.
   */
  bool Camera::SetImage(const double sample, const double line) {
    p_childSample = sample;
    p_childLine = line;
    p_pointComputed = true;

    // get shape
    // TODO: we need to validate this pointer (somewhere)
    ShapeModel *shape = target()->shape();
    shape->clearSurfacePoint();  // Set initial condition for ShapeModel

    // Case of no map projection
    if (p_projection == NULL || p_ignoreProjection) {
      // Convert to parent coordinate (remove crop, pad, shrink, enlarge)
      double parentSample = p_alphaCube->AlphaSample(sample);
      double parentLine = p_alphaCube->AlphaLine(line);
      bool success = false;
      success = p_detectorMap->SetParent(parentSample, parentLine);
      // Convert from parent to detector
      if (success) {
        double detectorSample = p_detectorMap->DetectorSample();
        double detectorLine   = p_detectorMap->DetectorLine();
        success = p_focalPlaneMap->SetDetector(detectorSample, detectorLine);
        // Now Convert from detector to distorted focal plane
        if (success) {
          double focalPlaneX = p_focalPlaneMap->FocalPlaneX();
          double focalPlaneY = p_focalPlaneMap->FocalPlaneY();
          success = p_distortionMap->SetFocalPlane(focalPlaneX, focalPlaneY);
          // Remove optical distortion
          if (success) {
            // Map to the ground
            double x = p_distortionMap->UndistortedFocalPlaneX();
            double y = p_distortionMap->UndistortedFocalPlaneY();
            double z = p_distortionMap->UndistortedFocalPlaneZ();
            return p_groundMap->SetFocalPlane(x, y, z);
          }
        }
      }
    }

    // The projection is a sky map
    else if (p_projection->IsSky()) {
      return SetImageSkyMapProjection(sample, line, shape);
    }

    // We have map projected camera model
    else {
      return SetImageMapProjection(sample, line, shape);
    }

    // failure
    shape->clearSurfacePoint();
    return false;
  }


  /**
   * @brief Sets the sample/line values of the image to get the lat/lon values with
   *        a time offset of deltaT.
   *
   * Warning: The deltaT parameter was added specifically for pixel2map to use for
   * the Dawn VIR camera. It is used to adjust the pointing to its location at specific
   * times like the times at the beginning, middle, and end of exposure for a specific pixel,
   * when the correct deltaT can be determined to achieve these results.
   *
   * Do not use this verstion of SetImage with a deltaT unless you understand exactly what this
   * does.
   *
   * @param sample Sample coordinate of the cube.
   * @param line Line coordinate of the cube.
   * @param deltaT seconds from the center exposure time
   *
   * @return @b bool Returns True if the image was set successfully and False if it
   *              was not.
   */
  bool Camera::SetImage(const double sample, const double line, const double deltaT) {
    p_childSample = sample;
    p_childLine = line;
    p_pointComputed = true;

    // get shape
    // TODO: we need to validate this pointer (somewhere)
    ShapeModel *shape = target()->shape();
    shape->clearSurfacePoint();  // Set initial condition for ShapeModel

    // Case of no map projection
    if (p_projection == NULL || p_ignoreProjection) {
      // Convert to parent coordinate (remove crop, pad, shrink, enlarge)
      double parentSample = p_alphaCube->AlphaSample(sample);
      double parentLine = p_alphaCube->AlphaLine(line);
      bool success = false;
      success = p_detectorMap->SetParent(parentSample, parentLine, deltaT);
      // Convert from parent to detector
      if (success) {
        double detectorSample = p_detectorMap->DetectorSample();
        double detectorLine   = p_detectorMap->DetectorLine();
        success = p_focalPlaneMap->SetDetector(detectorSample, detectorLine);
        // Now Convert from detector to distorted focal plane
        if (success) {
          double focalPlaneX = p_focalPlaneMap->FocalPlaneX();
          double focalPlaneY = p_focalPlaneMap->FocalPlaneY();
          success = p_distortionMap->SetFocalPlane(focalPlaneX, focalPlaneY);
          // Remove optical distortion
          if (success) {
            // Map to the ground
            double x = p_distortionMap->UndistortedFocalPlaneX();
            double y = p_distortionMap->UndistortedFocalPlaneY();
            double z = p_distortionMap->UndistortedFocalPlaneZ();
            return p_groundMap->SetFocalPlane(x, y, z);
          }
        }
      }
    }

    // The projection is a sky map
    else if (p_projection->IsSky()) {
      return SetImageSkyMapProjection(sample, line, shape);
    }

    // We have map projected camera model
    else {
      return SetImageMapProjection(sample, line, shape);
    }

    // failure
    shape->clearSurfacePoint();
    return false;
  }


/**
 * @brief Sets the sample/line values of the image to get the lat/lon values for a Map Projected
 * image.
 *
 * @param sample Sample coordinate of the cube
 * @param line Line coordinate of the cube
 * @param shape shape of the target
 *
 * @return bool Returns True if the image was set successfully and False if it
 *              was not.
 */
  bool Camera::SetImageMapProjection(const double sample, const double line, ShapeModel *shape) {
    Latitude lat;
    Longitude lon;
    Distance rad;
    if (shape->name() != "Plane") { // this is the normal behavior
      if (p_projection->SetWorld(sample, line)) {
        TProjection *tproj = (TProjection *) p_projection;
        lat = Latitude(tproj->UniversalLatitude(), Angle::Degrees);
        lon = Longitude(tproj->UniversalLongitude(), Angle::Degrees);
        rad = Distance(LocalRadius(lat, lon));
        if (!rad.isValid()) {
          shape->setHasIntersection(false);
          return false;
        }
        SurfacePoint surfPt(lat, lon, rad);
        if (SetGround(surfPt)) {
          p_childSample = sample;
          p_childLine = line;

          shape->setHasIntersection(true);
          return true;
        }
      }
    }
    else { // shape is ring plane
      if (p_projection->SetWorld(sample, line)) {
        RingPlaneProjection *rproj = (RingPlaneProjection *) p_projection;
        lat = Latitude(0.0, Angle::Degrees);
        lon = Longitude(rproj->UniversalRingLongitude(), Angle::Degrees);
        rad = Distance(rproj->UniversalRingRadius(),Distance::Meters);

        if (!rad.isValid()) {
          shape->setHasIntersection(false);
          return false;
        }
        SurfacePoint surfPt(lat, lon, rad);
        if (SetGround(surfPt)) {
          p_childSample = sample;
          p_childLine = line;

          shape->setHasIntersection(true);
          return true;
        }
      }
    }
    shape->clearSurfacePoint();
    return false;
  }


/**
 * @brief Sets the sample/line values of the image to get the lat/lon values for a Skymap Projected
 * image.
 *
 * @param sample Sample coordinate of the cube
 * @param line Line coordinate of the cube
 * @param shape shape of the target
 *
 * @return bool Returns True if the image was set successfully and False if it
 *              was not.
 */
  bool Camera::SetImageSkyMapProjection(const double sample, const double line, ShapeModel *shape) {
    TProjection *tproj = (TProjection *) p_projection;
    if (tproj->SetWorld(sample, line)) {
      if (SetRightAscensionDeclination(tproj->Longitude(),
                                      tproj->UniversalLatitude())) {
        p_childSample = sample;
        p_childLine = line;

        return HasSurfaceIntersection();
      }
    }
    shape->clearSurfacePoint();
    return false;
  }


 /**
   * Sets the lat/lon values to get the sample/line values
   *
   * @param latitude Latitude coordinate of the point
   * @param longitude Longitude coordinate of the point
   *
   * @return @b bool Returns true if the Universal Ground was set successfully and
   *              false if it was not
   */
  bool Camera::SetUniversalGround(const double latitude, const double longitude) {
    // Convert lat/lon or rad/az (i.e. ring rad / ring lon) to undistorted focal plane x/y
    if (p_groundMap->SetGround(Latitude(latitude, Angle::Degrees),
                              Longitude(longitude, Angle::Degrees))) {
      return RawFocalPlanetoImage();
    }

    target()->shape()->clearSurfacePoint();
    return false;
  }


  /**
   * Sets the lat/lon values to get the sample/line values
   *
   * @param latitude Latitude coordinate of the point
   * @param longitude Longitude coordinate of the point
   *
   * @return bool Returns true if the Universal Ground was set successfully and
   *              false if it was not
   */
  bool Camera::SetGround(Latitude latitude, Longitude longitude) {
    ShapeModel *shape = target()->shape();
    Distance localRadius;

    if (shape->name() != "Plane") { // this is the normal behavior
      localRadius = LocalRadius(latitude, longitude);
    }
    else {
      localRadius = Distance(latitude.degrees(),Distance::Kilometers);
      latitude = Latitude(0.,Angle::Degrees);
    }

      if (!localRadius.isValid()) {
      target()->shape()->clearSurfacePoint();
      return false;
    }

    return SetGround(SurfacePoint(latitude, longitude, localRadius));
  }



  /**
   * Sets the lat/lon/radius values to get the sample/line values
   *
   * @param surfacePt The point used for calculation
   *
   * @return bool Returns true if the Universal Ground was set successfully and
   *              false if it was not
   */
  bool Camera::SetGround(const SurfacePoint & surfacePt) {
    ShapeModel *shape = target()->shape();
    if (!surfacePt.Valid()) {
      shape->clearSurfacePoint();
      return false;
    }

    // Convert lat/lon to undistorted focal plane x/y
    if (p_groundMap->SetGround(surfacePt)) {
      return RawFocalPlanetoImage();
    }

    shape->clearSurfacePoint();
    return false;
  }


  /**
   * Computes the image coordinate for the current universal ground point
   *
   *
   * @return @b bool Returns true if image coordinate was computed successfully and
   *              false if it was not
   */
  bool Camera::RawFocalPlanetoImage() {
    double ux = p_groundMap->FocalPlaneX();
    double uy = p_groundMap->FocalPlaneY();

    // get shape
    // TODO: we need to validate this pointer (somewhere)
    ShapeModel *shape = target()->shape();

    // Convert undistorted x/y to distorted x/y
    bool success = p_distortionMap->SetUndistortedFocalPlane(ux, uy);
    if (success) {
      double focalPlaneX = p_distortionMap->FocalPlaneX();
      double focalPlaneY = p_distortionMap->FocalPlaneY();
      // Convert distorted x/y to detector position
      success = p_focalPlaneMap->SetFocalPlane(focalPlaneX, focalPlaneY);
      if (success) {
        double detectorSample = p_focalPlaneMap->DetectorSample();
        double detectorLine = p_focalPlaneMap->DetectorLine();
        // Convert detector to parent position
        success = p_detectorMap->SetDetector(detectorSample, detectorLine);
        if (success) {
          double parentSample = p_detectorMap->ParentSample();
          double parentLine = p_detectorMap->ParentLine();
          if (p_projection == NULL || p_ignoreProjection) {
            p_childSample = p_alphaCube->BetaSample(parentSample);
            p_childLine = p_alphaCube->BetaLine(parentLine);
            p_pointComputed = true;
            shape->setHasIntersection(true);
            return true;
          }
          else if (p_projection->IsSky()) {
            if (p_projection->SetGround(Declination(), RightAscension())) {
              p_childSample = p_projection->WorldX();
              p_childLine = p_projection->WorldY();
              p_pointComputed = true;
              shape->setHasIntersection(true);
              return true;
            }
          }
          else  if (p_projection->projectionType() == Projection::Triaxial) {
            if (p_projection->SetUniversalGround(UniversalLatitude(), UniversalLongitude())) {
              p_childSample = p_projection->WorldX();
              p_childLine = p_projection->WorldY();
              p_pointComputed = true;
              shape->setHasIntersection(true);
              return true;
            }
          }
          else { // ring plane
            // UniversalLongitude should return azimuth (ring longitude) in this case
            // TODO:
            //  when we make the change to real azimuths this value may need to be adjusted or
            //  code changed in the shapemodel or surfacepoint class.
            if (p_projection->SetUniversalGround(LocalRadius().meters(), UniversalLongitude())) {
              p_childSample = p_projection->WorldX();
              p_childLine = p_projection->WorldY();
              p_pointComputed = true;
              shape->setHasIntersection(true);
              return true;
            }
          }
        }
      }
    }

   shape->clearSurfacePoint();
   return false;
  }



  /**
  * @brief Sets the lat/lon/radius values to get the sample/line values
  *
  * @param latitude Latitude coordinate of the cube
  * @param longitude Longitude coordinate of the cube
  * @param radius Radius coordinate of the cube
  *
  * @return @b bool Returns True if the Universal Ground was set successfully
  *              and False if it was not
  */
  bool Camera::SetUniversalGround(const double latitude, const double longitude,
                                  const double radius) {
    // Convert lat/lon to undistorted focal plane x/y
    if (p_groundMap->SetGround(SurfacePoint(Latitude(latitude, Angle::Degrees),
                                            Longitude(longitude, Angle::Degrees),
                                            Distance(radius, Distance::Meters)))) {
      return RawFocalPlanetoImage();  // sets p_hasIntersection
    }

    target()->shape()->clearSurfacePoint();
   return false;
  }



  /**
   * @brief This method returns the Oblique Detector Resolution
   * if the Look Vector intersects the target and if the emission angle is greater than or equal
   * to 0, and less than 90 degrees.   Otherwise, it returns Isis::Null.  This formula provides an
   * improved estimate to the detector resolution for images near the limb:
   *
   *
   * @f[ \text{Oblique\;\;Detector\;\; Resolution} = \frac{n}{cos(\theta)} @f]
   *
   *
   * The equation is derived two separate ways.  A geometric argument is presented in
   * Reference 2, while a matrix algebra based argument is presented in Theorem 2.1 of
   * Reference 1.
   *
   *
   *   <b>Reference 1:</b>  J-M Morel and G. Yu, "Asift:  A new framework for fully affine
   *                 invariant image comparison," SIAM Journal on Imaging Sciences
   *                 2(2), pp. 438-469, 2009
   *
   *
   *   <b>Reference 2:</b>  Handwritten notes by Orrin Thomas which can be found in the
   *                 Glossary under the entry for Oblique Detector Resolution.
   *
   * @param useLocal If true, emission is fetched from LocalPhotometricAngles.
   *                 Otherwise, emission is fetched from EmissionAngle().
   *                 This is an optional parameter that defaults to true,
   *                 because local emission will give more accurate results.
   *
   * @return @b double
   */
  double Camera::ObliqueDetectorResolution(bool useLocal) {


    if(!HasSurfaceIntersection()) {
      return Isis::Null;
    }

    double thetaRad;
    double emissionDeg;

    if(useLocal) {
      Angle phase, emission, incidence;
      bool success;

      LocalPhotometricAngles(phase, incidence, emission, success);
      emissionDeg = (success) ? emission.degrees() : Isis::Null;
    }
    else {
      emissionDeg = EmissionAngle();
    }

    thetaRad = emissionDeg*DEG2RAD;

    if (thetaRad < HALFPI) {
      return DetectorResolution()/cos(thetaRad);

    }

    return Isis::Null;


  }


  /**
   * @brief Returns the detector resolution at the current position in meters.
   *
   * @return @b double The detector resolution
   */
  double Camera::DetectorResolution() {
    if (HasSurfaceIntersection()) {
      double sB[3];
      instrumentPosition(sB);
      double pB[3];
      Coordinate(pB);
      double dist = SensorUtilities::distance(sB, pB) * 1000.0;
      return dist / (p_focalLength / p_pixelPitch);
    }
    return Isis::Null;
  }


  /**
   * @brief Returns the sample resolution at the current position in meters.
   *
   * @return @b double The sample resolution
   */
  double Camera::SampleResolution() {
    return DetectorResolution() * p_detectorMap->SampleScaleFactor();
  }

  /**
   * @brief Returns the  oblique sample resolution at the current position in m.  This gives
   * a more accurate estimate of the sample resolution at oblique angles.
   *
   * @return @b double The sample resolution
   */
  double Camera::ObliqueSampleResolution(bool useLocal) {
    return ObliqueDetectorResolution(useLocal) * p_detectorMap->SampleScaleFactor();
  }


  /**
   * @brief Returns the line resolution at the current position in meters.
   *
   * @return @b double The line resolution
   */
  double Camera::LineResolution() {
    return DetectorResolution() * p_detectorMap->LineScaleFactor();
  }


  /**
   * @brief Returns the oblique line resolution at the current position in meters.  This
   * provides a more accurate estimate of the line resolution at oblique
   * angles.
   *
   * @return @b double The line resolution
   */
  double Camera::ObliqueLineResolution(bool useLocal) {
    return ObliqueDetectorResolution(useLocal) * p_detectorMap->LineScaleFactor();
  }


  /**
   * @brief Returns the pixel resolution at the current position in meters/pixel.
   * @return @b double The pixel resolution
   */
  double Camera::PixelResolution() {
    double lineRes = LineResolution();
    double sampRes = SampleResolution();
    if (lineRes < 0.0) return Isis::Null;
    if (sampRes < 0.0) return Isis::Null;
    return (lineRes + sampRes) / 2.0;
  }


  /**
   * @brief Returns the oblique pixel resolution at the current position in meters/pixel.  This
   * provides a more accurate estimate of the pixel resolution at oblique angles.
   *
   * @return @b double The pixel resolution
   */
  double Camera::ObliquePixelResolution(bool useLocal) {
    double lineRes = ObliqueLineResolution(useLocal);
    double sampRes = ObliqueSampleResolution(useLocal);
    if (lineRes < 0.0) return Isis::Null;
    if (sampRes < 0.0) return Isis::Null;
    return (lineRes + sampRes) / 2.0;
  }


  /**
   * @brief Returns the lowest/worst resolution in the entire image
   *
   * @return @b double The lowest/worst resolution in the image
   */
  double Camera::LowestImageResolution() {
    GroundRangeResolution();
    return p_maxres;
  }


  /**
   * @brief Returns the highest/best resolution in the entire image
   *
   * @return @b double The highest/best resolution in the entire image
   */
  double Camera::HighestImageResolution() {
    GroundRangeResolution();
    return p_minres;
  }


  /**
   * @brief Returns the lowest/worst oblique resolution in the entire image
   *
   * @return @b double The lowest/worst oblique resolution in the image
   */
  double Camera::LowestObliqueImageResolution() {
    GroundRangeResolution();
    return p_minobliqueres;
  }


  /**
   *  @brief Returns the highest/best oblique resolution in the entire image
   *
   * @return @b double The highest/best oblique resolution in the entire image
   */
  double Camera::HighestObliqueImageResolution() {
    GroundRangeResolution();
    return p_maxobliqueres;
  }


  /**
   *  @brief Computes the ground range and min/max resolution
   */
  void Camera::GroundRangeResolution() {
    // Software adjustment is needed if we get here -- call RingRangeResolution instead
    if (target()->shape()->name() == "Plane") {
      IString msg = "Images with plane targets should use Camera method RingRangeResolution ";
      msg += "instead of GroundRangeResolution";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Have we already done this
    if (p_groundRangeComputed) return;
    p_groundRangeComputed = true;

    bool computed = p_pointComputed;
    double originalSample = Sample();
    double originalLine = Line();
    int originalBand = Band();

    // Initializations
    p_minlat    = DBL_MAX;
    p_minlon    = DBL_MAX;
    p_minlon180 = DBL_MAX;
    p_maxlat    = -DBL_MAX;
    p_maxlon    = -DBL_MAX;
    p_maxlon180 = -DBL_MAX;
    p_minres    = DBL_MAX;
    p_maxres    = -DBL_MAX;
    p_minobliqueres = DBL_MAX;
    p_maxobliqueres = -DBL_MAX;

    // See if we have band dependence and loop for the appropriate number of bands
    int eband = p_bands;
    if (IsBandIndependent()) eband = 1;
    for (int band = 1; band <= eband; band++) {
      SetBand(band);

      // Loop for each line testing the left and right sides of the image
      for (int line = 1; line <= p_lines + 1; line++) {
        // Look for the first good lat/lon on the left edge of the image
        // If it is the first or last line then test the whole line
        int samp;
        for (samp = 1; samp <= p_samples + 1; samp++) {

          if (SetImage((double)samp - 0.5, (double)line - 0.5)) {
            double lat = UniversalLatitude();
            double lon = UniversalLongitude();
            if (lat < p_minlat) p_minlat = lat;
            if (lat > p_maxlat) p_maxlat = lat;
            if (lon < p_minlon) p_minlon = lon;
            if (lon > p_maxlon) p_maxlon = lon;

            if (lon > 180.0) lon -= 360.0;
            if (lon < p_minlon180) p_minlon180 = lon;
            if (lon > p_maxlon180) p_maxlon180 = lon;

            double res = PixelResolution();
            if (res > 0.0) {
              if (res < p_minres) p_minres = res;
              if (res > p_maxres) p_maxres = res;
            }
            //  Determine min/max oblique resolution
            double obliqueres = ObliquePixelResolution();
            if (obliqueres > 0.0) {
                if (obliqueres < p_minobliqueres) p_minobliqueres = obliqueres;
                if (obliqueres > p_maxobliqueres) p_maxobliqueres = obliqueres;

            }
            if ((line != 1) && (line != p_lines + 1)) break;
          }
        } // end loop through samples

        //We've already checked the first and last lines.
        if (line == 1) continue;
        if (line == p_lines + 1) continue;

        // Look for the first good lat/lon on the right edge of the image
        if (samp < p_samples + 1) {
          for (samp = p_samples + 1; samp >= 1; samp--) {
            if (SetImage((double)samp - 0.5, (double)line - 0.5)) {
              double lat = UniversalLatitude();
              double lon = UniversalLongitude();
              if (lat < p_minlat) p_minlat = lat;
              if (lat > p_maxlat) p_maxlat = lat;
              if (lon < p_minlon) p_minlon = lon;
              if (lon > p_maxlon) p_maxlon = lon;

              if (lon > 180.0) lon -= 360.0;
              if (lon < p_minlon180) p_minlon180 = lon;
              if (lon > p_maxlon180) p_maxlon180 = lon;

              double res = PixelResolution();
              if (res > 0.0) {
                if (res < p_minres) p_minres = res;
                if (res > p_maxres) p_maxres = res;
              }

              //  Determine min/max oblique resolution
              double obliqueres = ObliquePixelResolution();
              if (obliqueres > 0.0) {
                  if (obliqueres < p_minobliqueres) p_minobliqueres = obliqueres;
                  if (obliqueres > p_maxobliqueres) p_maxobliqueres = obliqueres;

              }
              break;
            }
          }
        }
      } // end loop through lines

      // Test at the sub-spacecraft point to see if we have a
      // better resolution
      double lat, lon;

      subSpacecraftPoint(lat, lon);
      Latitude latitude(lat, Angle::Degrees);
      Longitude longitude(lon, Angle::Degrees);
      // get the local radius for the subspacecraft point
      Distance radius(LocalRadius(latitude, longitude));
      SurfacePoint testPoint;

      if (radius.isValid()) {

        testPoint = SurfacePoint(latitude, longitude, radius);

        if (SetGround(testPoint)) {
          if (Sample() >= 0.5 && Line() >= 0.5 &&
              Sample() <= p_samples + 0.5 && Line() <= p_lines + 0.5) {
            double res = PixelResolution();
            if (res > 0.0) {
              if (res < p_minres) p_minres = res;
              if (res > p_maxres) p_maxres = res;
            }

            double obliqueres = ObliquePixelResolution();
            if (obliqueres > 0.0) {
                if (obliqueres < p_minobliqueres) p_minobliqueres = obliqueres;
                if (obliqueres > p_maxobliqueres) p_maxobliqueres = obliqueres;

            }
          }
        }
      } // end valid local (subspacecraft) radius

      // Special test for ground range to see if either pole is in the image
      latitude = Latitude(90, Angle::Degrees);
      longitude = Longitude(0.0, Angle::Degrees);
      // get radius for north pole
      radius = LocalRadius(latitude, longitude);

      if (radius.isValid()) {

        testPoint = SurfacePoint(latitude, longitude, radius);

        if (SetGround(testPoint)) {
          if (Sample() >= 0.5 && Line() >= 0.5 &&
              Sample() <= p_samples + 0.5 && Line() <= p_lines + 0.5) {
            p_maxlat = 90.0;
            p_minlon = 0.0;
            p_maxlon = 360.0;
            p_minlon180 = -180.0;
            p_maxlon180 = 180.0;
          }
        }
      } // end valid north polar radius

      latitude = Latitude(-90, Angle::Degrees);
      // get radius for south pole
      radius = LocalRadius(latitude, longitude);

      if (radius.isValid()) {

        testPoint = SurfacePoint(latitude, longitude, radius);
        if (SetGround(testPoint)) {
          if (Sample() >= 0.5 && Line() >= 0.5 &&
              Sample() <= p_samples + 0.5 && Line() <= p_lines + 0.5) {
            p_minlat = -90.0;
            p_minlon = 0.0;
            p_maxlon = 360.0;
            p_minlon180 = -180.0;
            p_maxlon180 = 180.0;
          }
        }
      } // end valid south polar radius

      // Another special test for ground range as we could have the
      // 0-360 seam running right through the image so
      // test it as well (the increment may not be fine enough !!!)
      for (Latitude lat = Latitude(p_minlat, Angle::Degrees);
                    lat <= Latitude(p_maxlat, Angle::Degrees);
                    lat += Angle((p_maxlat - p_minlat) / 10.0, Angle::Degrees)) {
        if (SetGround(lat, Longitude(0.0, Angle::Degrees))) {
          if (Sample() >= 0.5 && Line() >= 0.5 &&
              Sample() <= p_samples + 0.5 && Line() <= p_lines + 0.5) {
            p_minlon = 0.0;
            p_maxlon = 360.0;
            break;
          }
        } // end if set ground (lat, 0)

        // Another special test for ground range as we could have the
        // -180-180 seam running right through the image so
        // test it as well (the increment may not be fine enough !!!)
        if (SetGround(lat, Longitude(180.0, Angle::Degrees))) {
          if (Sample() >= 0.5 && Line() >= 0.5 &&
              Sample() <= p_samples + 0.5 && Line() <= p_lines + 0.5) {
            p_minlon180 = -180.0;
            p_maxlon180 = 180.0;
            break;
          }
        } // end if set ground (lat, 180)
      } // end for loop (latitudes from min to max)
    } // end for loop through bands

    SetBand(originalBand);

    if(computed) {
      SetImage(originalSample, originalLine);
    }
    else {
      p_pointComputed = false;
    }

    if (p_minlon == DBL_MAX  ||  p_minlat == DBL_MAX  ||  p_maxlon == -DBL_MAX  ||  p_maxlat == -DBL_MAX) {
      string message = "Camera missed planet or SPICE data off.";
      throw IException(IException::Unknown, message, _FILEINFO_);
    }


    // Checks for invalid lat/lon ranges
//    if(p_minlon == DBL_MAX  ||  p_minlat == DBL_MAX  ||  p_maxlon == -DBL_MAX
//    ||  p_maxlat == -DBL_MAX)
//    {
//      string message = "Camera missed planet or SPICE data off.";
//      throw IException(IException::Unknown, message, _FILEINFO_);
//    }
  }


  /**
   * @brief Analogous to above GroundRangeResolution method. Computes the ring range
   * and min/max resolution
   */
  void Camera::ringRangeResolution() {
    // TODO Add test to make sure we have a ring plane image **

    // Have we already done this
    if (p_ringRangeComputed) return;

    p_ringRangeComputed = true;

    bool computed = p_pointComputed;
    double originalSample = Sample();
    double originalLine = Line();
    int originalBand = Band();

    // Initializations
    p_minRingRadius       =  DBL_MAX;
    p_minRingLongitude    =  DBL_MAX;
    p_minRingLongitude180 =  DBL_MAX;
    p_maxRingRadius       = -DBL_MAX;
    p_maxRingLongitude    = -DBL_MAX;
    p_maxRingLongitude180 = -DBL_MAX;
    p_minres              =  DBL_MAX;
    p_maxres              = -DBL_MAX;

    // See if we have band dependence and loop for the appropriate number of bands
    int eband = p_bands;
    if (IsBandIndependent())
      eband = 1;

    for (int band = 1; band <= eband; band++) {
      SetBand(band);

      // Loop for each line testing the left and right sides of the image
      for (int line = 1; line <= p_lines + 1; line++) {

        // Look for the first good radius/azimuth on the left edge of the image
        // If it is the first or last line then test the whole line
        int samp;
        for (samp = 1; samp <= p_samples + 1; samp++) {

          if (SetImage((double)samp - 0.5, (double)line - 0.5)) {
            double radius = LocalRadius().meters();
            double azimuth = UniversalLongitude();
            if (radius < p_minRingRadius) p_minRingRadius = radius;
            if (radius > p_maxRingRadius) p_maxRingRadius = radius;
            if (azimuth < p_minRingLongitude) p_minRingLongitude = azimuth;
            if (azimuth > p_maxRingLongitude) p_maxRingLongitude = azimuth;

            if (azimuth > 180.0) azimuth -= 360.0;
            if (azimuth < p_minRingLongitude180) p_minRingLongitude180 = azimuth;
            if (azimuth > p_maxRingLongitude180) p_maxRingLongitude180 = azimuth;

            double res = PixelResolution();
            if (res > 0.0) {
              if (res < p_minres) p_minres = res;
              if (res > p_maxres) p_maxres = res;
            }
            if ((line != 1) && (line != p_lines + 1)) break;
          }
        }
        //We've already checked the first and last lines.
        if (line == 1) continue;
        if (line == p_lines + 1) continue;

        // Look for the first good rad/azimuth on the right edge of the image
        if (samp < p_samples + 1) {
          for(samp = p_samples + 1; samp >= 1; samp--) {
            if (SetImage((double)samp - 0.5, (double)line - 0.5)) {
              double radius = LocalRadius().meters();
              double azimuth = UniversalLongitude();
              if (radius < p_minRingRadius) p_minRingRadius = radius;
              if (radius > p_maxRingRadius) p_maxRingRadius = radius;
              if (azimuth < p_minRingLongitude) p_minRingLongitude = azimuth;
              if (azimuth > p_maxRingLongitude) p_maxRingLongitude = azimuth;

              if (azimuth > 180.0) azimuth -= 360.0;
              if (azimuth < p_minRingLongitude180) p_minRingLongitude180 = azimuth;
              if (azimuth > p_maxRingLongitude180) p_maxRingLongitude180 = azimuth;

              double res = PixelResolution();
              if (res > 0.0) {
                if (res < p_minres) p_minres = res;
                if (res > p_maxres) p_maxres = res;
              }
              break;
            }
          }
        }
      }

      // Test at the sub-spacecraft point to see if we have a
      // better resolution
      // TODO: is there something to this analogous for ring images?
//      double rad, lon;

//      subSpacecraftPoint(lat, lon);
//      Latitude latitude(lat, Angle::Degrees);
//      Longitude longitude(lon, Angle::Degrees);
//      Distance radius(LocalRadius(latitude, longitude));
//      SurfacePoint testPoint;

//      if (radius.isValid()) {

//        testPoint = SurfacePoint(latitude, longitude, radius);

//        if(SetGround(testPoint)) {
//          if(Sample() >= 0.5 && Line() >= 0.5 &&
//              Sample() <= p_samples + 0.5 && Line() <= p_lines + 0.5) {
//            double res = PixelResolution();
//            if(res > 0.0) {
//              if(res < p_minres) p_minres = res;
//              if(res > p_maxres) p_maxres = res;
//            }
//          }
//        }
//      }

      // Another special test for ring range as we could have the
      // 0-360 seam running right through the image so
      // test it as well (the increment may not be fine enough !!!)
      for (Distance radius = Distance(p_minRingRadius, Distance::Meters);
                   radius <= Distance(p_maxRingRadius, Distance::Meters);
                   radius += Distance((p_maxRingRadius - p_minRingRadius) / 10.0, Distance::Meters)) {
        if (SetGround(SurfacePoint(Latitude(0.0, Angle::Degrees), Longitude(0.0, Angle::Degrees), radius))) {
          if (Sample() >= 0.5 && Line() >= 0.5 &&
              Sample() <= p_samples + 0.5 && Line() <= p_lines + 0.5) {
            p_minRingLongitude = 0.0;
            p_maxRingLongitude = 360.0;
            break;
          }
        }

      // for (Latitude lat = Latitude(p_minlat, Angle::Degrees);
      //              lat <= Latitude(p_maxlat, Angle::Degrees);
      //              lat += Angle((p_maxlat - p_minlat) / 10.0, Angle::Degrees)) {
      //   if (SetGround(lat, Longitude(0.0, Angle::Degrees))) {
      //     if (Sample() >= 0.5 && Line() >= 0.5 &&
      //         Sample() <= p_samples + 0.5 && Line() <= p_lines + 0.5) {
      //       p_minlon = 0.0;
      //       p_maxlon = 360.0;
      //       break;
      //     }
      //   }

        // Another special test for ring range as we could have the
        // -180-180 seam running right through the image so
        // test it as well (the increment may not be fine enough !!!)
        if (SetGround(Latitude(0.0, Angle::Degrees), Longitude(180.0, Angle::Degrees))) {
          if (Sample() >= 0.5 && Line() >= 0.5 &&
              Sample() <= p_samples + 0.5 && Line() <= p_lines + 0.5) {
            p_minRingLongitude180 = -180.0;
            p_maxRingLongitude180 = 180.0;
            break;
          }
        }
      }
    } // end loop over bands

    SetBand(originalBand);

    if (computed) {
      SetImage(originalSample, originalLine);
    }
    else {
      p_pointComputed = false;
    }

    // Checks for invalid radius/lon ranges
    if (p_minRingRadius == DBL_MAX  ||  p_minRingRadius == DBL_MAX
        || p_minRingLongitude == DBL_MAX  ||  p_maxRingLongitude == -DBL_MAX) {
      string message = "RingPlane ShapeModel - Camera missed plane or SPICE data off.";
      throw IException(IException::Unknown, message, _FILEINFO_);
    }
  }


  /**
   * Checks whether the ground range intersects the longitude domain or not
   *
   * @param pvl The pvl file used to set the ground range
   *
   * @return @b bool Returns true if the range intersects the longitude domain, and
   *              false if it does not
   */
  bool Camera::IntersectsLongitudeDomain(Pvl &pvl) {
    double minlat, minlon, maxlat, maxlon;
    return GroundRange(minlat, maxlat, minlon, maxlon, pvl);
  }

  /**
   * Computes the Ground Range
   *
   * @param minlat The minimum latitude
   * @param maxlat The maximum latitude
   * @param minlon The minimum longitude
   * @param maxlon The maximum longitude
   * @param pvl The pvl file used for ground range calculations
   *
   * @return @b bool Returns true if it crosses the longitude domain boundary and
   *              false if it does not
   */
  bool Camera::GroundRange(double &minlat, double &maxlat,
                           double &minlon, double &maxlon,
                           Pvl &pvl) {
    // Compute the ground range and resolution
    GroundRangeResolution();

    // Get the default radii
    Distance localRadii[3];
    radii(localRadii);
    Distance &a = localRadii[0];
    Distance &b = localRadii[2];

    // See if the PVL overrides the radii
    PvlGroup map = pvl.findGroup("Mapping", Pvl::Traverse);

    if(map.hasKeyword("EquatorialRadius"))
      a = Distance(Isis::toDouble(map["EquatorialRadius"][0]), Distance::Meters);

    if(map.hasKeyword("PolarRadius"))
      b = Distance(Isis::toDouble(map["PolarRadius"][0]), Distance::Meters);

    // Convert to planetographic if necessary
    minlat = p_minlat;
    maxlat = p_maxlat;
    if(map.hasKeyword("LatitudeType")) {
      QString latType = QString::fromStdString(map["LatitudeType"]);
      if (latType.toUpper() == "PLANETOGRAPHIC") {
        if (abs(minlat) < 90.0) {  // So tan doesn't fail
          minlat *= PI / 180.0;
          minlat = atan(tan(minlat) * (a / b) * (a / b));
          minlat *= 180.0 / PI;
        }

        if(abs(maxlat) < 90.0) {  // So tan doesn't fail
          maxlat *= PI / 180.0;
          maxlat = atan(tan(maxlat) * (a / b) * (a / b));
          maxlat *= 180.0 / PI;
        }
      }
    }

    // Assume 0 to 360 domain but change it if necessary
    minlon = p_minlon;
    maxlon = p_maxlon;
    bool domain360 = true;
    if(map.hasKeyword("LongitudeDomain")) {
      QString lonDomain = QString::fromStdString(map["LongitudeDomain"]);
      if(lonDomain.toUpper() == "180") {
        minlon = p_minlon180;
        maxlon = p_maxlon180;
        domain360 = false;
      }
    }

    // Convert to the proper longitude direction
    if(map.hasKeyword("LongitudeDirection")) {
      QString lonDirection = QString::fromStdString(map["LongitudeDirection"]);
      if(lonDirection.toUpper() == "POSITIVEWEST") {
        double swap = minlon;
        minlon = -maxlon;
        maxlon = -swap;
      }
    }

    // Convert to the proper longitude domain
    if(domain360) {
      while(minlon < 0.0) {
        minlon += 360.0;
        maxlon += 360.0;
      }
      while(minlon > 360.0) {
        minlon -= 360.0;
        maxlon -= 360.0;
      }
    }
    else {
      while(minlon < -180.0) {
        minlon += 360.0;
        maxlon += 360.0;
      }
      while(minlon > 180.0) {
        minlon -= 360.0;
        maxlon -= 360.0;
      }
    }

    // Now return if it crosses the longitude domain boundary
    if((maxlon - minlon) > 359.0) return true;
    return false;
  }

  /**
   * Analogous to the above Ground Range method. Computes Range on the ring
   * plane
   *
   * @param minRingRadius The minimum ring radius
   * @param maxRingRadius The maximum ring radius
   * @param minRingLongitude The minimum ring longitude
   * @param maxRingLongitude The maximum ring longitude
   * @param pvl The pvl file used for ring range calculations
   *
   * @return @b bool Returns true if it crosses the longitude domain boundary and
   *              false if it does not
   */
  bool Camera::ringRange(double &minRingRadius, double &maxRingRadius,
                         double &minRingLongitude, double &maxRingLongitude, Pvl &pvl) {
    // Compute the ring range and resolution
    ringRangeResolution();

    // Get the mapping group
    PvlGroup map = pvl.findGroup("Mapping", Pvl::Traverse);

    // Get the ring radius range
    minRingRadius = p_minRingRadius;
    maxRingRadius = p_maxRingRadius;

    // Assume 0 to 360 domain but change it if necessary
    minRingLongitude = p_minRingLongitude;
    maxRingLongitude = p_maxRingLongitude;
    bool domain360 = true;
    if (map.hasKeyword("RingLongitudeDomain")) {
      QString ringLongitudeDomain = QString::fromStdString(map["RingLongitudeDomain"]);
      if (ringLongitudeDomain == "180") {
        minRingLongitude = p_minRingLongitude180;
        maxRingLongitude = p_maxRingLongitude180;
        domain360 = false;
      }
    }

    // Convert to the proper azimuth direction
    if (map.hasKeyword("RingLongitudeDirection")) {
      QString ringLongitudeDirection = QString::fromStdString(map["RingLongitudeDirection"]);
      if (ringLongitudeDirection.toUpper() == "Clockwise") {
        double swap = minRingLongitude;
        minRingLongitude = -maxRingLongitude;
        maxRingLongitude = -swap;
      }
    }

    // Convert to the proper azimuth domain
    if (domain360) {
      while (minRingLongitude < 0.0) {
        minRingLongitude += 360.0;
        maxRingLongitude += 360.0;
      }
      while (minRingLongitude > 360.0) {
        minRingLongitude -= 360.0;
        maxRingLongitude -= 360.0;
      }
    }
    else {
      while (minRingLongitude < -180.0) {
        minRingLongitude += 360.0;
        maxRingLongitude += 360.0;
      }
      while (minRingLongitude > 180.0) {
        minRingLongitude -= 360.0;
        maxRingLongitude -= 360.0;
      }
    }

    // Now return if it crosses the azimuth domain boundary
    if ((maxRingLongitude - minRingLongitude) > 359.0) {
      return true;
    }
    return false;
  }


  /**
   * Writes the basic mapping group to the specified Pvl.
   *
   * @param pvl Pvl to write mapping group to
   */
  void Camera::BasicMapping(Pvl &pvl) {
    PvlGroup map("Mapping");
    map += PvlKeyword("TargetName", target()->name().toStdString());

    std::vector<Distance> radii = target()->radii();
    map += PvlKeyword("EquatorialRadius", Isis::toString(radii[0].meters()), "meters");
    map += PvlKeyword("PolarRadius", Isis::toString(radii[2].meters()), "meters");

    map += PvlKeyword("LatitudeType", "Planetocentric");
    map += PvlKeyword("LongitudeDirection", "PositiveEast");
    map += PvlKeyword("LongitudeDomain", "360");

    GroundRangeResolution();
    map += PvlKeyword("MinimumLatitude", Isis::toString(p_minlat));
    map += PvlKeyword("MaximumLatitude", Isis::toString(p_maxlat));
    map += PvlKeyword("MinimumLongitude", Isis::toString(p_minlon));
    map += PvlKeyword("MaximumLongitude", Isis::toString(p_maxlon));
    map += PvlKeyword("PixelResolution", Isis::toString(p_minres));

    map += PvlKeyword("ProjectionName", "Sinusoidal");
    pvl.addGroup(map);
  }


  /**
   * Writes the basic mapping group for ring plane to the specified Pvl.
   *
   * @param pvl Pvl to write mapping group to
   */
  void Camera::basicRingMapping(Pvl &pvl) {
    if (target()->shape()->name() != "Plane") {
      // If we get here and we don't have a plane, throw an error
      IString msg = "A ring plane projection has been requested on an image whose shape is not a ring plane.  ";
      msg += "Rerun spiceinit with shape=RINGPLANE.  ";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    PvlGroup map("Mapping");
    map += PvlKeyword("TargetName", target()->name().toStdString());

    map += PvlKeyword("RingLongitudeDirection", "CounterClockwise");
    map += PvlKeyword("RingLongitudeDomain", "360");

    ringRangeResolution();
    map += PvlKeyword("MinimumRingRadius", Isis::toString(p_minRingRadius));
    map += PvlKeyword("MaximumRingRadius", Isis::toString(p_maxRingRadius));
    map += PvlKeyword("MinimumRingLongitude", Isis::toString(p_minRingLongitude));
    map += PvlKeyword("MaximumRingLongitude", Isis::toString(p_maxRingLongitude));
    map += PvlKeyword("PixelResolution", Isis::toString(p_minres));

    map += PvlKeyword("ProjectionName", "Planar");
    pvl.addGroup(map);
  }

  //! Reads the focal length from the instrument kernel
  void Camera::SetFocalLength() {
    int code = naifIkCode();
    QString key = "INS" + QString::fromStdString(toString(code)) + "_FOCAL_LENGTH";
    SetFocalLength(Spice::getDouble(key));
  }

  //! Reads the Pixel Pitch from the instrument kernel
  void Camera::SetPixelPitch() {
    int code = naifIkCode();
    QString key = "INS" + QString::fromStdString(toString(code)) + "_PIXEL_PITCH";
    SetPixelPitch(Spice::getDouble(key));
  }



  /**
   * Sets the right ascension declination
   *
   * @param ra Right ascension value
   * @param dec Declination value
   *
   * @return @b bool Returns true if the declination was set successfully and false
   *              if it was not
   */
  bool Camera::SetRightAscensionDeclination(const double ra, const double dec) {
    if (p_skyMap->SetSky(ra, dec)) {
      double ux = p_skyMap->FocalPlaneX();
      double uy = p_skyMap->FocalPlaneY();
      if (p_distortionMap->SetUndistortedFocalPlane(ux, uy)) {
        double dx = p_distortionMap->FocalPlaneX();
        double dy = p_distortionMap->FocalPlaneY();
        if (p_focalPlaneMap->SetFocalPlane(dx, dy)) {
          double detectorSamp = p_focalPlaneMap->DetectorSample();
          double detectorLine = p_focalPlaneMap->DetectorLine();
          if (p_detectorMap->SetDetector(detectorSamp, detectorLine)) {
            double parentSample = p_detectorMap->ParentSample();
            double parentLine = p_detectorMap->ParentLine();

            if (p_projection == NULL || p_ignoreProjection) {
              p_childSample = p_alphaCube->BetaSample(parentSample);
              p_childLine = p_alphaCube->BetaLine(parentLine);
              p_pointComputed = true;
              return true;
            }
            else if (p_projection->IsSky()) {
              if (p_projection->SetGround(dec, ra)) {
                p_childSample = p_projection->WorldX();
                p_childLine = p_projection->WorldY();
                p_pointComputed = true;
                return true;
              }
            }
            else if (target()->shape()->hasIntersection()) {
              if (p_projection->SetUniversalGround(UniversalLatitude(),
                                                  UniversalLongitude())) {
                p_childSample = p_projection->WorldX();
                p_childLine = p_projection->WorldY();
                p_pointComputed = true;
                return true;
              }
            }
          }
        }
      }
    }

    return false;
  }


  /**
   * This method will find the local normal at the current (sample, line) and
   * set it to the passed in array.
   *
   * @param [out] normal The local normal vector to be calculated.
   *
   */
  void Camera::GetLocalNormal(double normal[3]) {

    ShapeModel *shapeModel = target()->shape();
    if ( !shapeModel->hasIntersection()) {
      // if the shape is not intersected, then clearly there is no normal
      normal[0] = normal[1] = normal[2] = 0.0;
      return;
    }

    // The DEM shape model (and it's child classes) will use 4 surrounding neighbor
    // points to find the local normal. The SetImage() calls used to find the
    // neighbors is potentially expensive, so we will not calculate the neighbors
    // for shape models whose calculateLocalNormal() method won't use them.
    bool computed = p_pointComputed;
    if (!shapeModel->isDEM()) {
      // Non-DEM case: Ellipsoid, NAIF DSK, or Plane --
      // Pass in a vector where all of the "neighbors" are the origin (shape model center).
      // We do this so that if the implementation of the calculateLocalNormal() method in
      // any of the non-DEM shape model classes is modified to use this vector, then an error
      // should be thrown instead of a segmentation fault.
      QVector<double *> unusedNeighborPoints(4);
      double origin[3] = {0, 0, 0};
      unusedNeighborPoints.fill(origin);
      shapeModel->calculateLocalNormal(unusedNeighborPoints);
    }
    else { // attempt to find local normal for DEM shapes using 4 surrounding points on the image
      QVector<double *> cornerNeighborPoints(4);

      // As documented in the doxygen above, the goal of this method is to
      // calculate a normal vector to the surface using the 4 corner surrounding points.
      double samp = Sample();
      double line = Line();

      // order of points in vector is top, bottom, left, right
      QList< QPair< double, double > > surroundingPoints;
      surroundingPoints.append(qMakePair(samp, std::nexttoward(line - 0.5, line)));
      surroundingPoints.append(qMakePair(samp, std::nexttoward(line + 0.5, line)));
      surroundingPoints.append(qMakePair(std::nexttoward(samp - 0.5, samp), line));
      surroundingPoints.append(qMakePair(std::nexttoward(samp + 0.5, samp), line));

      // save input state to be restored on return
      double originalSample = samp;
      double originalLine = line;

      // now we have all four points in the image, so find the same points on the surface
      for (int i = 0; i < cornerNeighborPoints.size(); i++) {
        cornerNeighborPoints[i] = new double[3];
      }

      Latitude lat;
      Longitude lon;
      Distance radius;

      // if this is a dsk, we only need to use the existing intercept point (plate) normal then return
      for (int i = 0; i < cornerNeighborPoints.size(); i++) {
        // If a surrounding point fails, set it to the original point
        if (!(SetImage(surroundingPoints[i].first, surroundingPoints[i].second))) {
          surroundingPoints[i].first = samp;
          surroundingPoints[i].second = line;

          // If the original point fails too, we can't get a normal.  Clean up and return.
          if (!(SetImage(surroundingPoints[i].first, surroundingPoints[i].second))) {

            normal[0] = normal[1] = normal[2] = 0.0;

            // restore input state
            if (computed) {
              SetImage(originalSample, originalLine);
            }
            else {
              p_pointComputed = false;
            }

            // free memory
            for (int i = 0; i < cornerNeighborPoints.size(); i++) {
              delete [] cornerNeighborPoints[i];
            }

            return;
          }
        }

        SurfacePoint surfacePoint = GetSurfacePoint();
        lat = surfacePoint.GetLatitude();
        lon = surfacePoint.GetLongitude();
        radius = LocalRadius(lat, lon);

        latrec_c(radius.kilometers(), lon.radians(), lat.radians(), cornerNeighborPoints[i]);
      }

      // if the first 2 surrounding points match or the last 2 surrounding points match,
      // we can't get a normal.  Clean up and return.
      if ((surroundingPoints[0].first == surroundingPoints[1].first &&
          surroundingPoints[0].second == surroundingPoints[1].second) ||
         (surroundingPoints[2].first == surroundingPoints[3].first &&
          surroundingPoints[2].second == surroundingPoints[3].second)) {

        normal[0] = normal[1] = normal[2] = 0.0;

        // restore input state
        if (!computed) {
          SetImage(originalSample, originalLine);
        }
        else {
          p_pointComputed = false;
        }

        // free memory
        for (int i = 0; i < cornerNeighborPoints.size(); i++)
          delete [] cornerNeighborPoints[i];

        return;
      }

      // Restore input state to original point before calculating normal
      SetImage(originalSample, originalLine);
      shapeModel->calculateLocalNormal(cornerNeighborPoints);

      // free memory
      for (int i = 0; i < cornerNeighborPoints.size(); i++) {
        delete [] cornerNeighborPoints[i];
      }

    }

    // restore input state if calculation failed and clean up.
    if (!shapeModel->hasLocalNormal()) {
      p_pointComputed = false;
      return;
    }

    // restore failed computed state
    if (!computed) {
      p_pointComputed = false;
    }

    // Set the method normal values
    std::vector<double> localNormal(3);
    localNormal = shapeModel->localNormal();
    memcpy(normal, (double *) &localNormal[0], sizeof(double) * 3);
  }


  /**
   * Calculates LOCAL photometric angles using the DEM (not ellipsoid).  These
   * calculations are more expensive computationally than Sensor's angle getter
   * methods.  Furthermore, this cost is mostly in calculating the local normal
   * vector, which can be done only once for all angles using this method.
   *
   * @param phase The local phase angle to be calculated
   * @param incidence The local incidence angle to be calculated
   * @param emission The local emission angle to be calculated
   * @param success A boolean to keep track of whether normal is valid
   */
  void Camera::LocalPhotometricAngles(Angle & phase, Angle & incidence,
      Angle & emission, bool &success) {

    // get local normal vector
    double normal[3];
    GetLocalNormal(normal);
    success = true;

    // Check to make sure normal is valid
    if (SensorUtilities::magnitude(normal) == 0.) {
      success = false;
      return;
    }

    // get the vector from the surface to the sensor
    double sensorPosBf[3];
    instrumentBodyFixedPosition(sensorPosBf);
    SensorUtilities::Vec sensorPos(sensorPosBf);

    SurfacePoint surfacePoint = GetSurfacePoint();
    SensorUtilities::Vec groundPt = {
      surfacePoint.GetX().kilometers(),
      surfacePoint.GetY().kilometers(),
      surfacePoint.GetZ().kilometers()};

    SensorUtilities::Vec groundToSensor = sensorPos - groundPt;

    // get the vector from the surface to the sun
    SensorUtilities::Vec sunPos(m_uB);
    SensorUtilities::Vec groundToSun = sunPos - groundPt;

    phase = Angle(SensorUtilities::sepAngle(groundToSensor, groundToSun),
        Angle::Radians);

    emission = Angle(SensorUtilities::sepAngle(groundToSensor, normal),
        Angle::Radians);

    incidence = Angle(SensorUtilities::sepAngle(groundToSun, normal),
        Angle::Radians);

  }


  /**
   * Calculates the slope at the current point
   * by computing the angle between the local surface normal and the ellipsoid
   * surface normal. If there is a failure during the process, such as there
   * not being an intersection, then success will be false and slope will not
   * be modified.
   *
   * @param[out] slope The slope angle in degrees
   * @param[out] success If the slope was successfully calculated
   */
  void Camera::Slope(double &slope, bool &success) {
    ShapeModel *shapeModel = target()->shape();
    if ( !shapeModel->hasIntersection()) {
      success = false;
      return;
    }
    shapeModel->calculateSurfaceNormal();
    if (!shapeModel->hasNormal()) {
      success = false;
      return;
    }
    std::vector<double> ellipsoidNormal = shapeModel->normal();

    double localNormal[3];
    GetLocalNormal(localNormal);
    if (localNormal[0] == 0.0 && localNormal[1] == 0.0 && localNormal[2] == 0.0) {
      success = false;
      return;
    }

    slope = SensorUtilities::sepAngle(localNormal, &ellipsoidNormal[0]) * RAD2DEG;
    success = true;
  }


  /**
   * Computes the RaDec range
   *
   * @param minra Minimum right ascension value
   * @param maxra Maximum right ascension value
   * @param mindec Minimum declination value
   * @param maxdec Maximum declination value
   * @return @b bool Returns true if the range computation was successful and false
   *              if it was not
   */
  bool Camera::RaDecRange(double &minra, double &maxra,
                          double &mindec, double &maxdec) {


    bool computed = p_pointComputed;
    double originalSample = Sample();
    double originalLine = Line();
    int originalBand = Band();

    // Have we already done this
    if (!p_raDecRangeComputed) {
      p_raDecRangeComputed = true;

      // Initializations
      p_mindec    = DBL_MAX;
      p_minra     = DBL_MAX;
      p_minra180  = DBL_MAX;
      p_maxdec    = -DBL_MAX;
      p_maxra     = -DBL_MAX;
      p_maxra180  = -DBL_MAX;

      // See if we have band dependence and loop for the appropriate number of bands
      int eband = p_bands;
      if (IsBandIndependent()) eband = 1;
      for (int band = 1; band <= eband; band++) {
        this->SetBand(band);

        for (int line = 1; line <= p_lines; line++) {
          // Test left, top, and bottom sides
          int samp;
          for (samp = 1; samp <= p_samples; samp++) {
            SetImage((double)samp, (double)line);
            double ra = RightAscension();
            double dec = Declination();
            if (ra < p_minra) p_minra = ra;
            if (ra > p_maxra) p_maxra = ra;
            if (dec < p_mindec) p_mindec = dec;
            if (dec > p_maxdec) p_maxdec = dec;

            if (ra > 180.0) ra -= 360.0;
            if (ra < p_minra180) p_minra180 = ra;
            if (ra > p_maxra180) p_maxra180 = ra;

            if ((line != 1) && (line != p_lines)) break;
          }

          // Test right side
          if (samp < p_samples) {
            for (samp = p_samples; samp >= 1; samp--) {
              SetImage((double)samp, (double)line);
              double ra = RightAscension();
              double dec = Declination();
              if (ra < p_minra) p_minra = ra;
              if (ra > p_maxra) p_maxra = ra;
              if (dec < p_mindec) p_mindec = dec;
              if (dec > p_maxdec) p_maxdec = dec;

              if (ra > 180.0) ra -= 360.0;
              if (ra < p_minra180) p_minra180 = ra;
              if (ra > p_maxra180) p_maxra180 = ra;

              break;
            }
          }
        }

        // Special test for ground range to see if either pole is in the image
        if (SetRightAscensionDeclination(0.0, 90.0)) {
          if ((Line() >= 0.5) && (Line() <= p_lines) &&
              (Sample() >= 0.5) && (Sample() <= p_samples)) {
            p_maxdec = 90.0;
            p_minra = 0.0;
            p_maxra = 360.0;
            p_minra180 = -180.0;
            p_maxra180 = 180.0;
          }
        }

        if (SetRightAscensionDeclination(0.0, -90.0)) {
          if ((Line() >= 0.5) && (Line() <= p_lines) &&
              (Sample() >= 0.5) && (Sample() <= p_samples)) {
            p_mindec = -90.0;
            p_minra = 0.0;
            p_maxra = 360.0;
            p_minra180 = -180.0;
            p_maxra180 = 180.0;
          }
        }

        // Another special test for ground range as we could have the
        // 0-360 seam running right through the image so
        // test it as well (the increment may not be fine enough !!!)
        for (double dec = p_mindec; dec <= p_maxdec; dec += (p_maxdec - p_mindec) / 10.0) {
          if (SetRightAscensionDeclination(0.0, dec)) {
            if ((Line() >= 0.5) && (Line() <= p_lines) &&
                (Sample() >= 0.5) && (Sample() <= p_samples)) {
              p_minra = 0.0;
              p_maxra = 360.0;
              break;
            }
          }
        }

        // Another special test for ground range as we could have the
        // 0-360 seam running right through the image so
        // test it as well (the increment may not be fine enough !!!)
        for (double dec = p_mindec; dec <= p_maxdec; dec += (p_maxdec - p_mindec) / 10.0) {
          if (SetRightAscensionDeclination(180.0, dec)) {
            if ((Line() >= 0.5) && (Line() <= p_lines) &&
                (Sample() >= 0.5) && (Sample() <= p_samples)) {
              p_minra180 = -180.0;
              p_maxra180 = 180.0;
              break;
            }
          }
        }
      }
    }

    minra = p_minra;
    maxra = p_maxra;
    mindec = p_mindec;
    maxdec = p_maxdec;

    SetBand(originalBand);

    if (computed) {
      SetImage(originalSample, originalLine);
    }
    else {
      p_pointComputed = false;
    }

    return true;
  }


  /**
   * Returns the RaDec resolution
   *
   * @return @b double The resutant RaDec resolution
   */
  double Camera::RaDecResolution() {

    bool computed = p_pointComputed;
    double originalSample = Sample();
    double originalLine = Line();
    int originalBand = Band();

    SetImage(1.0, 1.0);
    double ra1 = RightAscension();
    double dec1 = Declination();

    SetImage(1.0, (double)p_lines);
    double ra2 = RightAscension();
    double dec2 = Declination();

    double dist = (ra1 - ra2) * (ra1 - ra2) + (dec1 - dec2) * (dec1 - dec2);
    dist = sqrt(dist);
    double lineRes = dist / (p_lines - 1);

    SetImage((double)p_samples, 1.0);
    ra2 = RightAscension();
    dec2 = Declination();

    dist = (ra1 - ra2) * (ra1 - ra2) + (dec1 - dec2) * (dec1 - dec2);
    dist = sqrt(dist);
    double sampRes = dist / (p_samples - 1);

    SetBand(originalBand);

    if (computed) {
      SetImage(originalSample, originalLine);
    }
    else {
      p_pointComputed = false;
    }

    return (sampRes < lineRes) ? sampRes : lineRes;
  }

  /**
   * Returns the North Azimuth
   *
   * @return @b double North Azimuth
   */
  double Camera::NorthAzimuth() {
    if (target()->shape()->name() == "Plane") {
      std::string msg = "North Azimuth is not available for plane target shapes.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    // Get the latitude of your current location using the shape model
    // specified in the image Kernels
    double lat = UniversalLatitude();
    // We are in northern hemisphere
    if (lat >= 0.0) {
      return ComputeAzimuth(90.0, 0.0);
    }
    // We are in southern hemisphere
    else {
      double azimuth = ComputeAzimuth(-90.0, 0.0) + 180.0;
      if (azimuth > 360.0) azimuth = azimuth - 360.0;
      return azimuth;
    }
  }

  /**
   * Returns the Sun Azimuth
   *
   * @return @b double Sun Azimuth
   *
   * @todo Get appropriate radius at the subsolar point
   */
  double Camera::SunAzimuth() {
    double lat, lon;
    subSolarPoint(lat, lon);
    return ComputeAzimuth(lat, lon);
  }


  /**
   * Return the Spacecraft Azimuth
   *
   * @return @b double Spacecraft Azimuth
   *
   * @todo Get appropriate radius at the subscraft point
   */
  double Camera::SpacecraftAzimuth() {
    double lat, lon;
    subSpacecraftPoint(lat, lon);
    return ComputeAzimuth(lat, lon);
  }


  /**
   * Computes the image azimuth value from your current position (origin) to a point of interest
   * specified by the lat/lon input to this method. (NOTE: This azimuth is different from a Ground
   * Azimuth)
   *
   * All azimuths are measured the same way regardless of the image level (level1 or level2) and
   * the shape model being used.
   *
   * The azimuth is an angle formed by a reference vector and a point of interest vector. The
   * current position is the vertex of the angle, i.e. the origin of the coordinate system on which
   * this angle will be measured. The azimuth is measured in a positive clockwise direction from the
   * reference vector (i.e. the initial ray of the angle) to the point of interest vector (i.e. the
   * terminal ray of the angle).
   *
   * The azimuth is measured in a positive clockwise direction because images have lines that
   * increase downward. If lines increased upward, then the azimuth would be measure in a positive
   * counterclockwise direction.
   *
   *
   * The reference vector is the vector from the origin to the right side of the image. This is
   * usually called the 3 o'clock reference vector because the image can be viewed as a clock face
   * and the origin point as the center of the clock face with the hand of the clock pointing at 3
   * o'clock.
   *
   * The point of interest vector is the vector along the surface of the body from the origin to
   * the point of interest. In order to calculate the azimuth, this vector is projected into the
   * reference plane (the plane containing the reference vector that is tangent to the surface). The
   * point of interest vector is also unitzed to 1 km in length and then scaled even further to
   * within a pixel of the azimuth's origin.
   *
   * The algorithm works by
   *   1.  Getting the body-fixed (x,y,z) of the azimuth's origin point and the body-fixed (x,y,z)
   *       of the point of interest. These coordinate also represent vectors from the center of the
   *       body-fixed system to the coordinate.
   *   2.  The vector from the origin point to the point of interest is then determined by vector
   *       subtraction.
   *   3.  The perpendicular component of this new vector to the origin vector is determined by
   *       vector subtraction. This gives a vector that is tangent to the planet surface at the
   *       origin point and that is in the direction of the point of interest.
   *   4.  The tangent vector is then unitized so that we may use pixel resolution to scale the
   *       point of interest vector to be within a pixel in size. This is done so that when we
   *       determine the new sample/line values, the original local radius can be used and the image
   *       coordinates are not thrown off by the curvature of the body
   *   5.  A new body-fixed vector from the center of the planet to the head of the tangent vector
   *       is determined.
   *   6.  The body-fixed (x,y,z) of the new vector is used to get ground coordinates (lat,lon) of
   *       the tail of the adjusted point of interest vector.
   *   7.  The (lat, lon) is used to get a  (line,sample) for the tail of the adjusted point of
   *       interest vector. So, we now have the line,sample of the origin point and the line,sample
   *       of a point in the direction of the point of interest and within a pixel's distance from
   *       the origin point.
   *   8.  The arctangent of (newline-originline)/ (newsample-originsample) is used to acquire the
   *       azimuth value.
   *
   * NOTE: All vectors in this method are body-fixed and use the radius of the shape model at the
   * origin point for doing calculations. By using the radius of the shape model at the origin, we
   * avoid problems where the DEM does not completely cover the planet.
   *
   * Note: This image azimuth algorithm is different from the ground azimuth algorithm used in
   * GroundAzimuth(). For ground azimuths, the initial ray of the angle is the vector from
   * the selected ground point to the north pole. For image azimuths, the initial ray is the vector
   * from the selected image location to the right, horizontally.    *
   *
   * @param lat The Latitude
   * @param lon The Longitude
   *
   * @return @b double Azimuth value
   * @internal
   *   @history 2009-09-23 Tracie Sucharski - Convert negative longitudes coming out of reclat.
   *   @history 2010-09-28 Janet Barrett - Added Randy's updated method for calculating the azimuth.
   *   @history 2011-02-11 Janet Barrett - Added documentation.
   *   @history 2011_02-11 Janet Barrett - There were some problems with calculating azimuths when a
   *                           DEM shape model was specified. One problem occurred when the DEM did
   *                           not cover the poles. The LocalRadius was returning a NULL for the
   *                           radius value in places that the DEM did not cover (such as the
   *                           poles). This was fixed by using the radius of the origin point when
   *                           determining the x,y,z location of the point of interest. The radius
   *                           is not important because we just need to know the direction of the
   *                           point of interest from the origin point. Another problem was also
   *                           found with the call to SetUniversalGround when the new point
   *                           (new point = point within a pixel of the origin point and in the
   *                           direction of the point of interest) was being determined. The new
   *                           point should be at the same radius as the origin point, but this was
   *                           not happening. The call to SetUniversalGround was changed to use the
   *                           radius of the origin point when determining the line,sample of the
   *                           new point. Another problem was that the vector pointing from
   *                           the origin point to the point of interest was being unitized before
   *                           its perpendicular component was being calculated. This has been
   *                           fixed.
   *   @history 2012-06-04 Janet Barrett - Removed redundant calls to Sample(), Line(),
   *                           and SetImage().
   *   @history 2014-04-17 Jeannie Backer - Modified ComputeAzimuth() to return an Isis::Null if the
   *                           method fails (instead of -1.0). Add a check in ComputeAzimuth() to
   *                           make sure the "SetUniversalGround()" call succeeds, if not, reset to
   *                           the original sample/line and return Null.
   *
   *   @todo Write PushState and PopState method to ensure the internals of the class are set based
   *         on SetImage or SetGround
   */
  double Camera::ComputeAzimuth(const double lat, const double lon) {
    // Make sure we are on the planet, if not, north azimuth is meaningless
    if (!HasSurfaceIntersection()) return Isis::Null;

    // Need to save the "state" of the camera so we can restore it when the
    // method is done
    bool computed = p_pointComputed;

    NaifStatus::CheckErrors();

    // Get the azimuth's origin point (the current position) and its radius
    SpiceDouble azimuthOrigin[3];
    Coordinate(azimuthOrigin);
    Distance originRadius = LocalRadius();
    if (!originRadius.isValid()) {
      return Isis::Null;
    }

    // Convert the point of interest to rectangular coordinates (x,y,z) in the
    // body-fixed coordinate system and use the azimuth's origin radius to avoid the
    // situation where the DEM does not cover the entire planet
    SpiceDouble pointOfInterestFromBodyCenter[3];
    latrec_c(originRadius.kilometers(), lon * PI / 180.0,
             lat * PI / 180.0, pointOfInterestFromBodyCenter);

    // Get the difference vector with its tail at the azimuth origin and its
    // head at the point of interest by subtracting vectors the vectors that
    // originate at the body center
    //
    // pointOfInterest = pointOfInterestFromBodyCenter - azimuthOriginFromBodyCenter
    //
    SpiceDouble pointOfInterest[3];
    vsub_c(pointOfInterestFromBodyCenter, azimuthOrigin, pointOfInterest);

    // Get the component of the difference vector pointOfInterestFromAzimuthOrigin that is
    // perpendicular to the origin point (i.e. project perpendicularly onto the reference plane).
    // This will result in a point of interest vector that is in the plane tangent to the surface at
    // the origin point
    SpiceDouble pointOfInterestProj[3];
    vperp_c(pointOfInterest, azimuthOrigin, pointOfInterestProj);

    // Unitize the tangent vector to a 1 km length vector
    SpiceDouble pointOfInterestProjUnit[3];
    vhat_c(pointOfInterestProj, pointOfInterestProjUnit);

    // Scale the vector to within a pixel of the azimuth's origin point.
    // Get pixel scale in km/pixel and divide by 2 to insure that we stay within
    // a pixel of the origin point
    double scale = (PixelResolution() / 1000.0) / 2.0;
    SpiceDouble pointOfInterestProjUnitScaled[3];
    vscl_c(scale, pointOfInterestProjUnit, pointOfInterestProjUnitScaled);

    // Compute the adjusted point of interest vector from the body center. This point
    // will be within a pixel of the origin and in the same direction as the requested
    // raw point of interest vector
    SpiceDouble adjustedPointOfInterestFromBodyCenter[3];
    vadd_c(azimuthOrigin, pointOfInterestProjUnitScaled, adjustedPointOfInterestFromBodyCenter);

    // Get the origin image coordinate
    double azimuthOriginSample = Sample();
    double azimuthOriginLine = Line();

    // Convert the point to a lat/lon and find out its image coordinate
    double adjustedPointOfInterestRad, adjustedPointOfInterestLon, adjustedPointOfInterestLat;
    reclat_c(adjustedPointOfInterestFromBodyCenter,
             &adjustedPointOfInterestRad,
             &adjustedPointOfInterestLon,
             &adjustedPointOfInterestLat);
    adjustedPointOfInterestLat = adjustedPointOfInterestLat * 180.0 / PI;
    adjustedPointOfInterestLon = adjustedPointOfInterestLon * 180.0 / PI;
    if (adjustedPointOfInterestLon < 0) adjustedPointOfInterestLon += 360.0;

    // Use the radius of the azimuth's origin point
    // (rather than that of the adjusted point of interest location)
    // to avoid the effects of topography on the calculation
    bool success = SetUniversalGround(adjustedPointOfInterestLat,
                                      adjustedPointOfInterestLon,
                                      originRadius.meters());
    if (!success) {
      // if the adjusted lat/lon values for the point of origin fail to be set, we can not compute
      // an azimuth. reset to the original sample/line and return null.
      SetImage(azimuthOriginSample, azimuthOriginLine);
      return Isis::Null;
    }

    double adjustedPointOfInterestSample = Sample();
    double adjustedPointOfInterestLine   = Line();

    // TODO:  Write PushState and PopState method to ensure the
    // internals of the class are set based on SetImage or SetGround

    // We now have the information needed to calculate an arctangent
    //
    //
    //         point of interest
    //                  |\       |
    //                  | \      |
    //                  |  \     |                      tan(A) = (delta line) / (delta sample)
    //    delta line    |   \    |                      A  = arctan( (delta line) / (delta sample) )
    //                  |    \   |
    //                  |     \  |
    //                  |      \ |
    //       ___________|_____A_\|_______________
    //              delta sample |origin point
    //                           |
    //                           |
    //                           |
    //                           |
    //                           |
    //
    // in this example, the azimuth is the angle indicated by the A plus 180 degrees, since we begin
    // the angle rotation at the positive x-axis
    // This quadrant issue (i.e.need to add 180 degrees) is handled by the atan2 program
    //
    double deltaSample = adjustedPointOfInterestSample - azimuthOriginSample;
    double deltaLine = adjustedPointOfInterestLine - azimuthOriginLine;

    // Compute the angle; the azimuth is the arctangent of the line difference divided by the
    // sample difference; the atan2 function is used because it determines which quadrant we
    // are in based on the sign of the 2 arguments; the arctangent is measured in a positive
    // clockwise direction because the lines in the image increase downward; the arctangent
    // uses the 3 o'clock axis (positive sample direction) as its reference line (line of
    // zero degrees); a good place to read about the atan2 function is at
    // http://en.wikipedia.org/wiki/Atan2
    double azimuth = 0.0;
    if (deltaSample != 0.0 || deltaLine != 0.0) {
      azimuth = atan2(deltaLine, deltaSample);
      azimuth *= 180.0 / PI;
    }

    // Azimuth is limited to the range of 0 to 360
    if (azimuth < 0.0) azimuth += 360.0;
    if (azimuth > 360.0) azimuth -= 360.0;

    NaifStatus::CheckErrors();

    // computed is true if the sample/line or lat/lon were reset in this method
    // to find the location of the point of interest
    // If so, reset "state" of camera to the original sample/line
    if (computed) {
      SetImage(azimuthOriginSample, azimuthOriginLine);
    }
    else {
      p_pointComputed = false;
    }

    return azimuth;
  }


  /**
   * Return the off nadir angle in degrees.
   *
   * @return @b double Off Nadir Angle
   */
  double Camera::OffNadirAngle() {
    // Get the xyz coordinates for the spacecraft and point we are interested in
    double coord[3], spCoord[3];
    Coordinate(coord);
    instrumentPosition(spCoord);

    // Get the angle between the 2 points and convert to degrees
    double a = SensorUtilities::sepAngle(coord, spCoord) * RAD2DEG;
    double b = 180.0 - EmissionAngle();

    // The three angles in a triangle must add up to 180 degrees
    double c = 180.0 - (a + b);

    return c;
  }


  /**
   * Computes and returns the ground azimuth between the ground point and
   * another point of interest, such as the subspacecraft point or the
   * subsolar point. The ground azimuth is the clockwise angle on the
   * ground between a line drawn from the ground point to the North pole
   * of the body and a line drawn from the selected point on the surface
   * to some point of interest on the surface (such as the subsolar point
   * or the subspacecraft point).
   *
   * Note: This is different from the image azimuth algorithm used in ComputeAzimuth().
   * For ground azimuths, the initial ray of the angle is the vector from the selected ground point
   * to the north pole. For image azimuths, the initial ray is the vector from the selected image
   * location to the right, horizontally.
   *
   * @param glat The latitude of the ground point
   * @param glon The longitude of the ground point
   * @param slat The latitude of the subspacecraft or subsolar point
   * @param slon The longitude of the subspacecraft or subsolar point
   *
   * @return @b double The azimuth in degrees
   */
  double Camera::GroundAzimuth(double glat, double glon,
                               double slat, double slon) {
    double a;
    double b;
    if (glat >= 0.0) {
      a = (90.0 - slat) * PI / 180.0;
      b = (90.0 - glat) * PI / 180.0;
    }
    else {
      a = (90.0 + slat) * PI / 180.0;
      b = (90.0 + glat) * PI / 180.0;
    }

    double cslon = slon;
    double cglon = glon;
    if (cslon > cglon) {
      if ((cslon-cglon) > 180.0) {
        while ((cslon-cglon) > 180.0) cslon = cslon - 360.0;
      }
    }
    if (cglon > cslon) {
      if ((cglon-cslon) > 180.0) {
        while ((cglon-cslon) > 180.0) cglon = cglon - 360.0;
      }
    }

    // Which quadrant are we in?
    int quad;
    if (slat > glat) {
      if (cslon > cglon) {
        quad = 1;
      }
      else if (cslon < cglon) {
        quad = 2;
      }
      else {
        quad = 1;
      }
    }
    else if (slat < glat) {
      if (cslon > cglon) {
        quad = 4;
      }
      else if (cslon < cglon) {
        quad = 3;
      }
      else {
        quad = 4;
      }
    }
    else {
      if (cslon > cglon) {
        quad = 1;
      }
      else if (cslon < cglon) {
        quad = 2;
      }
      else {
        return 0.0;
      }
    }

    double C = (cglon - cslon) * PI / 180.0;
    if (C < 0) C = -C;
    double c = acos(cos(a)*cos(b) + sin(a)*sin(b)*cos(C));
    double azimuth = 0.0;
    if (sin(b) == 0.0 || sin(c) == 0.0) {
      return azimuth;
    }
    double intermediate = (cos(a) - cos(b)*cos(c))/(sin(b)*sin(c));
    if (intermediate < -1.0) {
      intermediate = -1.0;
    }
    else if (intermediate > 1.0) {
      intermediate = 1.0;
    }
    double A = acos(intermediate) * 180.0 / PI;
    //double B = acos((cos(b) - cos(c)*cos(a))/(sin(c)*sin(a))) * 180.0 / PI;
    if (glat >= 0.0) {
      if (quad == 1 || quad == 4) {
        azimuth = A;
      }
      else if (quad == 2 || quad == 3) {
        azimuth = 360.0 - A;
      }
    }
    else {
      if (quad == 1 || quad == 4) {
        azimuth = 180.0 - A;
      }
      else if (quad == 2 || quad == 3) {
        azimuth = 180.0 + A;
      }
    }

    return azimuth;
  }


  /**
   * Sets the Distortion Map. This object will take ownership of the distortion
   * map pointer.
   *
   * @param *map Pointer to a CameraDistortionMap object
   */
 void Camera::SetDistortionMap(CameraDistortionMap *map, bool deleteExisting) {
   if (deleteExisting && p_distortionMap) {
     delete p_distortionMap;
   }

   p_distortionMap = map;
 }


  /**
   * Sets the Focal Plane Map. This object will take ownership of the focal plane
   * map pointer.
   *
   * @param *map Pointer to a CameraFocalPlaneMap object
   */
  void Camera::SetFocalPlaneMap(CameraFocalPlaneMap *map) {
    if (p_focalPlaneMap) {
      delete p_focalPlaneMap;
    }

    p_focalPlaneMap = map;
  }


  /**
   * Sets the Detector Map. This object will take ownership of the detector map
   * pointer.
   *
   * @param *map Pointer to a CameraDetectorMap object
   */
  void Camera::SetDetectorMap(CameraDetectorMap *map) {
    if (p_detectorMap) {
      delete p_detectorMap;
    }

    p_detectorMap = map;
  }


  /**
   * Sets the Ground Map. This object will take ownership of the ground map
   * pointer.
   *
   * @param *map Pointer to a CameraGroundMap object
   */
  void Camera::SetGroundMap(CameraGroundMap *map) {
    if (p_groundMap) {
      delete p_groundMap;
    }

    p_groundMap = map;
  }


  /**
   * Sets the Sky Map. This object will take ownership of the sky map pointer.
   *
   * @param *map Pointer to a CameraSkyMap object
   */
  void Camera::SetSkyMap(CameraSkyMap *map) {
    if (p_skyMap) {
      delete p_skyMap;
    }

    p_skyMap = map;
  }


  /**
   * This loads the spice cache big enough for this image. The default cache size
   *   is the number of lines in the cube if the ephemeris time changes in the
   *   image, one otherwise.
   *
   * @internal
   *   @history 2011-02-08 Jeannie Walldren - Removed unused input parameter.
   *                          Moved calculations of cache size and start/end
   *                          ephemeris times to their own methods.
   */
  void Camera::LoadCache() {
    // We want to stay in unprojected space for this process
    bool projIgnored = p_ignoreProjection;
    p_ignoreProjection = true;

    // get the cache variables
    pair<double,double> ephemerisTimes = StartEndEphemerisTimes();
    int cacheSize = CacheSize(ephemerisTimes.first, ephemerisTimes.second);

    // Set a position in the image so that the PixelResolution can be calculated
    SetImage(p_alphaCube->BetaSamples() / 2, p_alphaCube->BetaLines() / 2);
    double tol = PixelResolution() / 100.; //meters/pix/100.

    if (tol < 0.0) {
      // Alternative calculation of ground resolution of a pixel/100
      double altitudeMeters;
      if (target()->isSky()) {   // Use the unit sphere as the target
        altitudeMeters = 1.0;
      }
      else {
        altitudeMeters = SpacecraftAltitude() * 1000.;
      }
      tol = PixelPitch() * altitudeMeters / FocalLength() / 100.;
    }

    p_ignoreProjection = projIgnored;

    Spice::createCache(ephemerisTimes.first, ephemerisTimes.second,
                       cacheSize, tol);

    setTime(ephemerisTimes.first);

    // Reset to band 1
    SetBand(1);

    return;
  }


  /**
   * Calculates the start and end ephemeris times. These times are found by
   * looping through the bands and finding the ephemeris times for the upper
   * left and bottom right pixels in the image. The start time (shutter open
   * time) is the minimum value of those ephemeris times. The end time (shutter
   * close time) is the maximum value of those ephemeris times. This method must
   * be called before a call to the Spice::createCache() method.  It is called
   * in the LoadCache() method.
   *
   * @returns pair<double, double> A pair containing the start and end ephemeris times
   *
   * @throw iException::Programmer - "Unable to find time range for the
   *             spice kernels."
   * @see createCache()
   * @see LoadCache()
   *
   * @author 2011-02-02 Jeannie Walldren
   * @internal
   *   @history 2011-02-02 Jeannie Walldren - Original version.
   */
  pair<double, double> Camera::StartEndEphemerisTimes() {
    pair<double,double> ephemerisTimes;
    double startTime = -DBL_MAX;
    double endTime = -DBL_MAX;

    for (int band = 1; band <= Bands(); band++) {
      SetBand(band);
      SetImage(0.5, 0.5);
      double etStart = time().Et();
      SetImage(p_alphaCube->BetaSamples() + 0.5,
               p_alphaCube->BetaLines() + 0.5); // need to do something if SetImage returns false???
      double etEnd = time().Et();
      if (band == 1) {
        startTime = min(etStart, etEnd);
        endTime = max(etStart, etEnd);
      }
      startTime = min(startTime, min(etStart, etEnd));
      endTime = max(endTime, max(etStart, etEnd));
    }
    if (startTime == -DBL_MAX || endTime == -DBL_MAX) {
      string msg = "Unable to find time range for the spice kernels";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    ephemerisTimes.first = startTime;
    ephemerisTimes.second = endTime;
    return ephemerisTimes;
  }


  /**
   * This method calculates the spice cache size. This method finds the number
   * of lines in the beta cube and adds 1, since we need at least 2 points for
   * interpolation. This method must be called before a call to the
   * Spice::createCache() method.  It is called in the LoadCache() method.
   *
   * @param startTime Starting ephemeris time to cache
   * @param endTime Ending ephemeris time to cache
   *
   * @returns int The  calculated spice cache size
   *
   * @throw iException::Programmer - "A cache has already been created."
   * @see createCache()
   * @see LoadCache()
   *
   * @author 2011-02-02 Jeannie Walldren
   * @internal
   *   @history 2011-02-02 Jeannie Walldren - Original version.
   */
  int Camera::CacheSize(double startTime, double endTime) {
    int cacheSize;
    // BetaLines() + 1 so we get at least 2 points for interpolation
    cacheSize = p_alphaCube->BetaLines() + 1;
    if (startTime == endTime) {
      cacheSize = 1;
    }
    return cacheSize;
  }


  /**
   * This method sets the best geometric tiling size for projecting from this
   * camera model. This is used by cam2map/ProcessRubberSheet. When cubes are
   * projected, an attempt is made to use linear equations to take large, square
   * chunks of data at a time to cull the amount of SetUniversalGround(...) calls
   * necessary to project a cube. If the chunk of data fails to be linear, then it
   * will be split up into 4 corners and each of the new chunks (corners) are
   * reconsidered up until endSize is reached - the endsize size will be
   * considered, it is inclusive. The startSize must be a power of 2 greater
   * than 2, and the endSize must be a power of 2 equal to or less than the
   * start size but greater than 2. If both the startSize and endSize are set to 2
   * then no geometric tiling will be enabled.
   *
   * @param startSize The tile size to start with; default 128
   * @param endSize The tile size to give up at; default 8
   */
  void Camera::SetGeometricTilingHint(int startSize, int endSize) {
    // verify the start size is a multiple of 2 greater than 2
    int powerOf2 = 2;

    // No hint if 2's are passed in
    if (startSize == 2 && endSize == 2) {
      p_geometricTilingStartSize = 2;
      p_geometricTilingEndSize = 2;
      return;
    }

    if (endSize > startSize) {
      IString message = "Camera::SetGeometricTilingHint End size must be smaller than the start size";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    if (startSize < 4) {
      IString message = "Camera::SetGeometricTilingHint Start size must be at least 4";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    bool foundEnd = false;
    while (powerOf2 > 0 && startSize != powerOf2) {
      if (powerOf2 == endSize) foundEnd = true;
      powerOf2 *= 2;
    }

    // Didnt find a solution, the integer became negative first, must not be
    //   a power of 2
    if (powerOf2 < 0) {
      IString message = "Camera::SetGeometricTilingHint Start size must be a power of 2";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    if (!foundEnd) {
      IString message = "Camera::SetGeometricTilingHint End size must be a power of 2 less than the start size, but greater than 2";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    p_geometricTilingStartSize = startSize;
    p_geometricTilingEndSize = endSize;
  }


  /**
   * This will get the geometric tiling hint; these values are typically used for
   * ProcessRubberSheet::SetTiling(...).
   *
   * @param startSize Tiling start size
   * @param endSize Tiling end size
   */
  void Camera::GetGeometricTilingHint(int &startSize, int &endSize) {
    startSize = p_geometricTilingStartSize;
    endSize = p_geometricTilingEndSize;
  }


  /**
   * This returns true if the current Sample() or Line() value
   * is outside of the cube (meaning the point must have been
   * extrapolated).
   *
   *
   * @return @b bool Point was extrapolated
   */
  bool Camera::InCube() {
    if (Sample() < 0.5 || Line() < 0.5) {
      return false;
    }

    if (Sample() > Samples() + 0.5 || Line() > Lines() + 0.5) {
      return false;
    }

    return true;
  }


  /**
   * Checks to see if the camera object has a projection
   *
   * @return @b bool Returns true if it has a projection and false if it
   *              does not
   */
  bool Camera::HasProjection() {
    return p_projection != 0;
  }


  /**
   * Virtual method that checks if the band is independent
   *
   * @return @b bool Returns true if the band is independent, and false if it is
   *              not
   */
  bool Camera::IsBandIndependent() {
    return true;
  }


  /**
   * Returns the reference band
   *
   * @return @b int Reference Band
   */
  int Camera::ReferenceBand() const {
    return p_referenceBand;
  }


  /**
   * Checks to see if the Camera object has a reference band
   *
   * @return @b bool Returns true if it has a reference band, and false if it
   *              does not
   */
  bool Camera::HasReferenceBand() const {
    return p_referenceBand != 0;
  }


  /**
   * Virtual method that sets the band number
   *
   * @param band Band Number
   */
  void Camera::SetBand(const int band) {
    p_childBand = band;
  }


  /**
   * Returns the current sample number
   *
   * @return @b double Sample Number
   */
   double Camera::Sample() const {
    return p_childSample;
  }


  /**
   * Returns the current band
   *
   * @return @b int Band
   */
   int Camera::Band() const {
    return p_childBand;
  }


  /**
   * Returns the current line number
   *
   * @return @b double Line Number
   */
   double Camera::Line() const {
    return p_childLine;
  }


  /**
   * Returns the resolution of the camera
   *
   * @return @b double pixel resolution
   */
  double Camera::resolution() {
    return PixelResolution();
  }




  /**
   * Returns the focal length
   *
   * @return @b double Focal Length
   */
   double Camera::FocalLength() const {
    return p_focalLength;
  }


  /**
   * Returns the pixel pitch
   *
   * @return @b double Pixel Pitch
   */
   double Camera::PixelPitch() const {
    return p_pixelPitch;
  }


  /**
   * Returns the pixel ifov offsets from center of pixel, which defaults to the
   * (pixel pitch * summing mode ) / 2.  If an instrument has a non-square ifov, it must implement
   * this method to return the offsets from the center of the pixel.
   *
   * @returns QList<QPointF> A list of offsets
   *
   */
   QList<QPointF> Camera::PixelIfovOffsets() {

     QList<QPointF> offsets;
     offsets.append(QPointF(-PixelPitch() * DetectorMap()->SampleScaleFactor() / 2.0,
                            -PixelPitch() * DetectorMap()->LineScaleFactor() / 2.0));
     offsets.append(QPointF(PixelPitch() * DetectorMap()->SampleScaleFactor() / 2.0,
                            -PixelPitch() * DetectorMap()->LineScaleFactor() / 2.0));
     offsets.append(QPointF(PixelPitch() * DetectorMap()->SampleScaleFactor() / 2.0,
                            PixelPitch() * DetectorMap()->LineScaleFactor() / 2.0));
     offsets.append(QPointF(-PixelPitch() * DetectorMap()->SampleScaleFactor() / 2.0,
                            PixelPitch() * DetectorMap()->LineScaleFactor() / 2.0));

     return offsets;
   }


  /**
   * Returns the number of samples in the image
   *
   * @return @b int Number of Samples
   */
   int Camera::Samples() const {
    return p_samples;
  }


  /**
   * Returns the number of lines in the image
   *
   * @return @b int Number of Lines
   */
   int Camera::Lines() const {
    return p_lines;
  }


  /**
   * Returns the number of bands in the image
   *
   * @return @b int Number of Bands
   */
   int Camera::Bands() const {
    return p_bands;
  }


  /**
   * Returns the number of lines in the parent alphacube
   *
   * @return @b int Number of Lines in parent alphacube
   */
   int Camera::ParentLines() const {
    return p_alphaCube->AlphaLines();
  }


  /**
   * Returns the number of samples in the parent alphacube
   *
   * @return @b int Number of Samples in the parent alphacube
   */
   int Camera::ParentSamples() const {
    return p_alphaCube->AlphaSamples();
  }


  /**
   * Returns a pointer to the CameraDistortionMap object
   *
   * @return @b CameraDistortionMap*
   */
  CameraDistortionMap *Camera::DistortionMap() {
    return p_distortionMap;
  }


  /**
   * Returns a pointer to the CameraFocalPlaneMap object
   *
   * @return @b CameraFocalPlaneMap*
   */
  CameraFocalPlaneMap *Camera::FocalPlaneMap() {
    return p_focalPlaneMap;
  }


  /**
   * Returns a pointer to the CameraDetectorMap object
   *
   * @return @b CameraDetectorMap*
   */
  CameraDetectorMap *Camera::DetectorMap() {
    return p_detectorMap;
  }


  /**
   * Returns a pointer to the CameraGroundMap object
   *
   * @return @b CameraCGroundMap*
   */
  CameraGroundMap *Camera::GroundMap() {
    return p_groundMap;
  }


  /**
   * Returns a pointer to the CameraSkyMap object
   *
   * @return @b CameraSkyMap*
   */
  CameraSkyMap *Camera::SkyMap() {
    return p_skyMap;
  }


  /**
   * This method returns the InstrumentId as it appears in the cube.
   *
   * @return QString Returns m_instrumentId
   */
  QString Camera::instrumentId() {
    return m_instrumentId;
  }


  /**
   * This method returns the full instrument name.
   *
   * @return QString
   */
  QString Camera::instrumentNameLong() const {
    return m_instrumentNameLong;
  }


  /**
   * This method returns the shortened instrument name.
   *
   * @return QString
   */
  QString Camera::instrumentNameShort() const {
    return m_instrumentNameShort;
  }


  /**
   * This method returns the full spacecraft name.
   *
   * @return QString
   */
  QString Camera::spacecraftNameLong() const {
    return m_spacecraftNameLong;
  }


  /**
   * This method returns the shortened spacecraft name.
   *
   * @return QString
   */
  QString Camera::spacecraftNameShort() const {
    return m_spacecraftNameShort;
  }

  /**
   * Set whether or not the camera should ignore the Projection
   *
   * @param ignore
   */
  void Camera::IgnoreProjection(bool ignore) {
    p_ignoreProjection = ignore;
  }
  /**
   * @brief Provides target code for instruments SPK NAIF kernel
   *
   * This virtual method may need to be implemented in each camera model
   * providing the target NAIF ID code found in the mission SPK kernel. This
   * is typically the spacecraft ID code.
   *
   * This value can be easily determined by using the NAIF @b spacit
   * application that sumarizes binary kernels on the SPK kernel used for a
   * particular instrument on a spacecraft.  @b spacit will additionally
   * require a leap seconds kernel (LSK).  For example, the output of the
   * MESSENGER SPK camera supporting the MDIS camera below indicates it is
   * indeed the MESSENGER spacecraft:
   *
   * @code
   *     Segment ID     : msgr_20050903_20061125_recon002.nio
   *     Target Body    : Body -236, MESSENGER
   *     Center Body    : Body 2, VENUS BARYCENTER
   *     Reference frame: Frame 1, J2000
   *     SPK Data Type  : Type 1
   *     Description : Modified Difference Array
   *     UTC Start Time : 2006 OCT 16 19:25:41.111
   *     UTC Stop Time  : 2006 OCT 31 22:14:24.040
   *     ET Start Time  : 2006 OCT 16 19:26:46.293
   *     ET Stop time   : 2006 OCT 31 22:15:29.222
   * @endcode
   *
   * The SpkTargetId value is found in the "Target Body" entry (-236).
   *
   * For most cases, this is the NAIF SPK code returned by the naifSpkCode()
   * method (in the Spice class).  Some instrument camera models may need to
   * override this method if this is not case.
   *
   * @return @b int NAIF code for the SPK target for an instrument
   */
  int Camera::SpkTargetId() const {
    return (naifSpkCode());
  }

  /**
   * @brief Provides the center of motion body for SPK NAIF kernel
   *
   * This virtual method may need to be implemented in each camera model
   * providing the NAIF integer code for the center of motion of the object
   * identified by the SpkTargetId() code.  This is typically the targeted
   * body for a particular image observation, but may be unique depending
   * upon the design of the SPK mission kernels.
   *
   * This value can be easily determined by using the NAIF @b spacit
   * application that sumarizes binary kernels on the SPK kernel used for a
   * particular instrument on a spacecraft.  @b spacit will additionally
   * require a leap seconds kernel (LSK).  For example, the output of the
   * MESSENGER SPK camera supporting the MDIS camera below indicates it is
   * Venus.
   *
   * @code
   *     Segment ID     : msgr_20050903_20061125_recon002.nio
   *     Target Body    : Body -236, MESSENGER
   *     Center Body    : Body 2, VENUS BARYCENTER
   *     Reference frame: Frame 1, J2000
   *     SPK Data Type  : Type 1
   *     Description : Modified Difference Array
   *     UTC Start Time : 2006 OCT 16 19:25:41.111
   *     UTC Stop Time  : 2006 OCT 31 22:14:24.040
   *     ET Start Time  : 2006 OCT 16 19:26:46.293
   *     ET Stop time   : 2006 OCT 31 22:15:29.222
   * @endcode
   *
   * The SpkCenterId value is found in the "Center Body" entry (2). The
   * center of motion is most likely the targeted body for the image and
   * this is provided by the naifBodyCode() method (in the Spice class).  If
   * this is not consistently the case for a particular mission, then camera
   * models will need to reimplement this method.
   *
   * @return @b int NAIF code for SPK center of motion body for an
   *         instrument
   */
  int Camera::SpkCenterId() const {
    return (naifBodyCode());
  }

  /**
   * Sets the focal length
   *
   * @param v Focal Length
   */
  void Camera::SetFocalLength(double v) {
    p_focalLength = v;
  }

  /**
   * Sets the pixel pitch
   *
   * @param v Pixel Pitch
   */
  void Camera::SetPixelPitch(double v) {
    p_pixelPitch = v;
  }

  /**
   * Computes the celestial north clock angle at the current
   * line/sample or ra/dec. The reference vector is a vecor from the
   * current pixel pointed directly "upward". Celetial North
   * is a vector from the current pixel poiting towards celetial north.
   * The Celestial North Clock Angle is the angle between these two vectors
   * on the image.
   *
   * @return @b double The resultant Celestial North Clock Angle
   */
  double Camera::CelestialNorthClockAngle() {
    double orgLine = Line();
    double orgSample = Sample();
    double orgDec = Declination();
    double orgRa = RightAscension();

    SetRightAscensionDeclination(orgRa, orgDec + (2 * RaDecResolution()));
    double y = Line() - orgLine;
    double x = Sample() - orgSample;
    double celestialNorthClockAngle = atan2(-y, x) * 180.0 / Isis::PI;
    celestialNorthClockAngle = 90.0 - celestialNorthClockAngle;

    if (celestialNorthClockAngle < 0.0) {
       celestialNorthClockAngle += 360.0;
    }

    SetImage(orgSample, orgLine);
    return celestialNorthClockAngle;
  }


  /**
   * Return the exposure duration for the pixel that the camera is set to.
   *
   * @return @b double The exposure duration in seconds for the pixel that the camera is set to.
   */
  double Camera::exposureDuration() const {
    return p_detectorMap->exposureDuration(p_childSample, p_childLine, p_childBand);
  }


  /**
   * Return the exposure duration for the pixel at the given line, sample and band.
   *
   * @param sample The sample of the desired pixel.
   * @param line The line of the desired pixel.
   * @param band The band of the desired pixel. Defaults to 1.
   *
   * @return @b double The exposure duration for the desired pixel in seconds.
   */
  double Camera::exposureDuration(const double sample, const double line, const int band) const {

    //If band defaults to -1, use the camera's stored band.
    if (band < 0) {
      return p_detectorMap->exposureDuration(sample, line, p_childBand);
    }
    return p_detectorMap->exposureDuration(sample, line, band);
  }

// end namespace isis
}
