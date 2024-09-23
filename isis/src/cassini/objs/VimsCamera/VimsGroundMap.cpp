/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "VimsGroundMap.h"

#include <iostream>
#include <iomanip>

#include <QDebug>
#include <QVector>

#include "BasisFunction.h"
#include "Camera.h"
#include "Constants.h"
#include "Distance.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "Latitude.h"
#include "LeastSquares.h"
#include "Longitude.h"
#include "PolynomialBivariate.h"
#include "Preference.h"
#include "SpecialPixel.h"
#include "Target.h"

using namespace std;

namespace Isis {
  /**
   * Constructs the VimsGroundMap object
   *
   * @param parent A pointer to the parent Camera object
   * @param lab Pvl labels for the image
   */
  VimsGroundMap::VimsGroundMap(Camera *parent, Pvl &lab) :
    CameraGroundMap(parent) {

    p_minX = DBL_MAX;
    p_maxX = -DBL_MAX;
    p_minY = DBL_MAX;
    p_maxY = -DBL_MAX;
    p_minZ = DBL_MAX;
    p_maxZ = -DBL_MAX;

    if (parent->ParentSamples() > 64 || parent->ParentLines() > 64) {
      IString msg = "The Vims ground map does not understand cubes that "
                    "initially have more than 64 lines or 64 samples.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }



  /**
   * Destroys the VimsGroundMap object
   */
  VimsGroundMap::~VimsGroundMap() {

  }



  /**
   * Initialize vims camera model
   *
   * @param [in] lab   (Pvl &)      Cube Pvl label
   *
   * @throw iException::Io - "Cannot process NYQUIST(undersampled) mode "
   *
   * @internal
   *   @history  2007-04-16 Tracie Sucharski - Look for unit vectors in
   *                              the proper directory.
   *   @history  2007-04-18 Tracie Sucharski, The inaccuracy of the 15 Mhz clock
   *                    (exposure , interline_delay) is already taken care of
   *                     in the labels values, so remove the adjustment from the
   *                     code (exp * 1.01725).
   *                     *Reference:  email from John Ivens 11/27/2006.
   *   @history 2007-04-19 Tracie Sucharski,  Swap bytes on unit vectors. They
   *                     are currently LSB binary files.
   *                     ???Todo:???  Convert unit vectors to text files.
   *   @history 2008-02-05 Tracie Sucharski, Replaced unitVector files with
   *                           Rick McCloskey's code to calculate look
   *                           direction.
   *   @history 2009-08-06 Tracie Sucharski, Bug in unit vector change made
   *                           on 2008-02-05, had the incorrect boresight for
   *                           VIS Hires.
   *   @history 2013-09-09 Tracie Sucharski, Turns out the times in the labels
   *                          are NOT corrected, so the corrections NEED to happen in the
   *                          code.  There was discussion among UofA Vims and PDS about reproducing
   *                          PDS archive with labels corrected, but the decision was made to
   *                          perform the correction in the code and leave the archive alone.
   *                          Some history:  08-23-2004 Rick McCloskey gave me the correction to
   *                          put in the code since he said the label values were incorrect.
   *                          11-27-2006 John Ivens said the labels values were corrected, so I
   *                          removed the correction factor from the code.
   *                          For more info, See previous history entries: 2007-04-18.
   *  @history 2014-04-09 Tracie Sucharski - When converting the camera model from lat/lon to x/y/z
   *                          calculations, the range check in the SetGround method was removed.
   *                          When creating global projections,  the pixel data would be replicated
   *                          in incorrect locations due to the least squares fitting always finding
   *                          a fit, even if it's incorrect.  The range checking was added back in
   *                          using x/y/z min/max values.  However, this caused the pole not to be
   *                          found, so the min/max z values are adjusted to include the pole if
   *                          they are within 1 km of the pole.  This value was chosen through
   *                          trial and error, and I have concerns that this may need to be
   *                          adjusted for images with different resolutions.  I think it would
   *                          just cause a few extra pixels at the edge worse case scenario.
   */
  void VimsGroundMap::Init(Pvl &lab) {

    PvlGroup inst = lab.findGroup("Instrument", Pvl::Traverse);

    //  Vis or IR
    p_channel = QString::fromStdString(inst ["Channel"]);
    // Get the start time in et
    QString stime = QString::fromStdString(inst ["NativeStartTime"]);
    QString intTime = stime.split(".").first();
    stime = stime.split(".").last();

    p_etStart = p_camera->getClockTime(intTime).Et();
    p_etStart += stime.toDouble() / 15959.0;
    //----------------------------------------------------------------------
    //  Because of inaccuracy with the 15 Mhz clock, the IR exposure and
    //  interline delay need to be adjusted.
    //----------------------------------------------------------------------
    p_irExp = (IString::ToDouble(inst ["ExposureDuration"][0]) * 1.01725) / 1000.;
    p_visExp = (IString::ToDouble(inst ["ExposureDuration"][1])) / 1000.;
    p_interlineDelay = (IString::ToDouble(inst ["InterlineDelayDuration"]) * 1.01725) / 1000.;

    // Get summation mode
    QString sampMode = QString::fromStdString(inst ["SamplingMode"]).toUpper();

    //  Get sample/line offsets
    int sampOffset = inst ["XOffset"];
    int lineOffset = inst ["ZOffset"];

    //  Get swath width/length which will be image size unless occultation image
    p_swathWidth = inst ["SwathWidth"];
    p_swathLength = inst ["SwathLength"];

    if (p_channel == "VIS") {
      if (sampMode == "NORMAL") {
        p_xPixSize = p_yPixSize = 0.00051;
        p_xBore = p_yBore = 31;
        p_camSampOffset = sampOffset - 1;
        p_camLineOffset = lineOffset - 1;

      }
      else if (sampMode == "HI-RES") {
        p_xPixSize = p_yPixSize = 0.00051 / 3.0;
        p_xBore = p_yBore = 94;
        //New as of 2009-08-04 per Dyer Lytle's email
        // Old Values:p_camSampOffset = 3 * (sampOffset - 1) + p_swathWidth;
        //            p_camLineOffset = 3 * (lineOffset - 1) + p_swathLength;
        p_camSampOffset = (3 * (sampOffset + p_swathWidth / 2)) - p_swathWidth / 2;
        p_camLineOffset = (3 * (lineOffset + p_swathLength / 2)) - p_swathLength / 2;
      }
      else {
        string msg = "Unsupported SamplingMode [" + sampMode.toStdString() + "]";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }
    else if (p_channel == "IR") {
      if (sampMode == "NORMAL") {
        p_xPixSize = p_yPixSize = 0.000495;
        p_xBore = p_yBore = 31;
        p_camSampOffset = sampOffset - 1;
        p_camLineOffset = lineOffset - 1;
      }
      else if (sampMode == "HI-RES") {
        p_xPixSize = 0.000495 / 2.0;
        p_yPixSize = 0.000495;
        p_xBore = 62.5;
        p_yBore = 31;
        p_camSampOffset = 2 * ((sampOffset - 1) + ((p_swathWidth - 1) / 4));
        p_camLineOffset = lineOffset - 1;
      }
      else {
        string msg = "Unsupported SamplingMode [" + sampMode.toStdString() + "]";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }

    //---------------------------------------------------------------------
    //  Loop for each pixel in cube, get pointing information and calculate
    //  control point (line, sample, x, y, z) for later use in latlon_to_linesamp.
    //---------------------------------------------------------------------
    p_camera->IgnoreProjection(true);
    for (int line = 0; line < p_camera->ParentLines(); line++) {

      //  VIS exposure is for a single line.  According to SIS,
      //  NATIVE_START_TIME is for the first pixel of the IR exposure.
      //  "The offset from IR start to VIS start is calculated by
      //   IrExposMsec - VisExposMsec)/2".
      //  This needs to be moved to forward routine

      if (p_channel == "VIS") {
        double et = ((double)p_etStart + (((p_irExp * p_swathWidth) - p_visExp) / 2.)) +
                    ((line + 0.5) * p_visExp);
        p_camera->setTime(et);
      }

      for (int samp = 0; samp < p_camera->ParentSamples(); samp++) {
        if (p_channel == "IR") {
          double et = (double)p_etStart +
                      (line * p_camera->ParentSamples() * p_irExp) +
                      (line * p_interlineDelay) + ((samp + 0.5) * p_irExp);
          p_camera->setTime(et);
        }

        if (p_camera->SetImage((double) samp + 1, (double)line + 1)) {

          double xyz[3];
          p_camera->Coordinate(xyz);

          if (xyz[0] != Null && xyz[1] != Null && xyz[2] != Null) {
            p_xyzMap[line][samp].setX(xyz[0]);
            p_xyzMap[line][samp].setY(xyz[1]);
            p_xyzMap[line][samp].setZ(xyz[2]);

//          if (samp !=0 ) {
//            QVector3D deltaXyz = p_xyzMap[line][samp-1] - p_xyzMap[line][samp];
//            qDebug()<<"samp:line = "<<samp<<" : "<<line<<" xyzMap[line][samp-1] = "<<p_xyzMap[line][samp-1]<<"  xyzMap[line][samp] = "<<p_xyzMap[line][samp]<<"  delta = "<<deltaXyz.length();
//          }

            // Find min/max x,y,z for use in SetGround method to make sure incoming lat/lon falls
            // within image.
            if (xyz[0] < p_minX) p_minX = xyz[0];
            if (xyz[0] > p_maxX) p_maxX = xyz[0];
            if (xyz[1] < p_minY) p_minY = xyz[1];
            if (xyz[1] > p_maxY) p_maxY = xyz[1];
            if (xyz[2] < p_minZ) p_minZ = xyz[2];
            if (xyz[2] > p_maxZ) p_maxZ = xyz[2];
          }
        }
      }
    }

    //  Fine tune min/max Z so that if the pole is actually in the image, it will be found in
    //  the SetGround method.  If the min/max values found for pixels centers is used, the pole
    //  will not be found even if it is actually in the image.  if the min/max z values are within
    //  1 km of the radius, set to the radius.

    //  NOTE:  IF THERE ARE PROBLEMS FOUND IN THE CAMERA MODEL,  THIS WOULD BE THE FIRST PLACE
    //         I WOULD LOOK.
    Distance radii[3];
    p_camera->radii(radii);
    if (abs(abs(p_minZ) - radii[2].kilometers()) < 1.0) {
      p_minZ = radii[2].kilometers() * (int(abs(p_minZ) / p_minZ));
    }
    if (abs(abs(p_maxZ) - radii[2].kilometers()) < 1.0) {
      p_maxZ = radii[2].kilometers() * (int(abs(p_maxZ) / p_maxZ));
    }

    p_camera->IgnoreProjection(false);
  }


  /** Compute ground position from focal plane coordinate
   *
   * This method will compute the ground position given an
   * undistorted focal plane coordinate.  Note that the latitude/longitude
   * value can be obtained from the camera class passed into the constructor.
   *
   * @param ux Distorted focal plane x in millimeters
   * @param uy Distorted focal plane y in millimeters
   * @param uz Distorted focal plane z in millimeters
   *
   * @return @b bool Indicates whether the conversion was successful
   *
   * @internal
   * @history 2008-01-02 Tracie Sucharski - Check incoming pixel for validity
   *                                against edge of pixel not center.
   * @history 2008-02-05 Tracie Sucharski, Replaced unitVector files with
   *                         Rick McCloskey's code to calculate look
   *                         direction
   * @history 2008-02-14 Tracie Sucharski, Change imgSamp/imgLine to double
   *                         so that fractional pixels are used in calculations.
   */
  bool VimsGroundMap::SetFocalPlane(const double ux, const double uy,
                                    const double uz) {

    p_ux = ux;
    p_uy = uy;
    p_uz = uz;

    double imgSamp = ux;
    double imgLine = uy;

    if ((imgLine < 0.5) || (imgLine > p_camera->ParentLines() + 0.5) ||
        (imgSamp < 0.5) || (imgSamp > p_camera->ParentSamples() + 0.5)) {
      return false;
    }
    imgLine--;
    imgSamp--;

    // does interline_delay & exposure-duration account for summing modes?
    // if not, won't use p_parentLine/p_parentSample
    double et = 0.;
    if (p_channel == "VIS") {
      et = (p_etStart + ((p_irExp * p_swathWidth) - p_visExp) / 2.) +
           ((imgLine + 0.5) * p_visExp);
    }
    else if (p_channel == "IR") {
      et = (double)p_etStart +
           (imgLine * p_camera->ParentSamples() * p_irExp) +
           (imgLine * p_interlineDelay) + ((imgSamp + 0.5) * p_irExp);
    }
    p_camera->setTime(et);

    //  get Look Direction
    SpiceDouble lookC[3];
    LookDirection(lookC);

    SpiceDouble unitLookC[3];
    vhat_c(lookC, unitLookC);
    return p_camera->SetLookDirection(unitLookC);
  }


  /** Compute undistorted focal plane coordinate from ground position
   *
   * @param lat Planetocentric latitude in degrees
   * @param lon Planetocentric longitude in degrees
   *
   * @return @b bool Indicates whether the conversion was successful
   *
   * @internal
   * @history 2007-04-18  Tracie Sucharski - Added check for reasonable
   *                             match when attempting to find closest
   *                             lat/lon in map arrays.
   * @history 2007-09-14  Tracie Sucharski - Added check for longitude
   *                             outside min/max bounds.  Don't know why
   *                             this wasn't put in before (lat check was
   *                             in), was it oversight, or did I take it out
   *                             for some reason???
   * @history 2007-12-14  Tracie Sucharski - Remove resolution test, too
   *                            image dependent and the resolution for vims is
   *                            incorrect due to the instrument having
   *                            rectangular pixels.
   * @history 2008-01-02  Tracie Sucharski -  Check validity of resulting
   *                            sample and line against edge of starting
   *                            ending pixels (0.5/Parent+0.5) instead of
   *                            center of pixels.
   * @history 2012-12-03  Tracie Sucharski - Check for valid minLat/maxLat, minLon/maxLon.  If
   *                            none are valid, this means the latMap and lonMap have no valid
   *                            data, therefore we cannot back project, so return false.
   * @history 2014-04-08  Tracie Sucharski - Change the sanity check made on 2012-12-03 from lat/lon
   *                            to xyz.
   *
   */
  bool VimsGroundMap::SetGround(const Latitude &lat, const Longitude &lon) {


    //  Make sure at least 1 pixel in image has projected onto surface
    if (p_minX == DBL_MAX || p_maxX == -DBL_MAX ||
        p_minY == DBL_MAX || p_maxY == -DBL_MAX ||
        p_minZ == DBL_MAX || p_maxZ == -DBL_MAX) return false;

    QVector3D xyz;
    if (p_camera->target()->shape()->name() == "Plane") {
        double radius = lat.degrees();
        if(radius <= 0.0)
          return false;

        double xCheck = radius * 0.001 * cos(lon.radians());
        double yCheck = radius * 0.001 * sin(lon.radians());

        xyz.setX(xCheck);
        xyz.setY(yCheck);
        xyz.setZ(0.);
      }
    else {
        //  Convert lat/lon to x/y/z
        Distance radius = p_camera->LocalRadius(lat, lon);
        SpiceDouble pB[3];
        latrec_c(radius.kilometers(), lon.radians(), lat.radians(), pB);

        //  Make sure this point falls within range of image
        if (pB[0] < p_minX || pB[0] > p_maxX ||
            pB[1] < p_minY || pB[1] > p_maxY ||
            pB[2] < p_minZ || pB[2] > p_maxZ) return false;

        xyz.setX(pB[0]);
        xyz.setY(pB[1]);
        xyz.setZ(pB[2]);
    }

    double minDist = DBL_MAX;
    int minSamp = -1;
    int minLine = -1;

    //  Find closest points  ??? what tolerance ???
    for (int line = 0; line < p_camera->ParentLines(); line++) {
      for (int samp = 0; samp < p_camera->ParentSamples(); samp++) {

        if (p_xyzMap[line][samp].isNull()) continue;

        //  Subtract map from coordinate then get length
        QVector3D deltaXyz = xyz - p_xyzMap[line][samp];
        if (deltaXyz.length() < minDist) {
          minDist = deltaXyz.length();
          minSamp = samp;
          minLine = line;
        }
      }
    }

    //-----------------------------------------------------------------
    //  If dist is less than some ??? tolerance ??? this is the
    //  closest point.  Use this point and surrounding 8 pts as
    //  control pts.
    //----------------------------------------------------------------
    if (minDist >= DBL_MAX) return false;

    //-------------------------------------------------------------
    //  Set-up for LU decomposition (least2 fit).
    //  Assume we will have 9 control points, this may not be true
    //  and will need to be adjusted before the final solution.
    //-------------------------------------------------------------
    BasisFunction sampXyzBasis("Sample", 4, 4);
    BasisFunction lineXyzBasis("Line", 4, 4);
    LeastSquares sampXyzLsq(sampXyzBasis);
    LeastSquares lineXyzLsq(lineXyzBasis);
    vector<double> knownXyz(4);

    //  Solve using x/y/z
    for (int line = minLine - 1; line < minLine + 2; line++) {
      if (line < 0 || line > p_camera->ParentLines() - 1) continue;
      for (int samp = minSamp - 1; samp < minSamp + 2; samp++) {
        //  Check for edges
        if (samp < 0 || samp > p_camera->ParentSamples() - 1) continue;
        if (p_xyzMap[line][samp].isNull()) continue;

        knownXyz[0] = p_xyzMap[line][samp].x();
        knownXyz[1] = p_xyzMap[line][samp].y();
        knownXyz[2] = p_xyzMap[line][samp].z();
        knownXyz[3] = 1;
        sampXyzLsq.AddKnown(knownXyz, samp + 1);
        lineXyzLsq.AddKnown(knownXyz, line + 1);
      }
    }

    if (sampXyzLsq.Knowns() < 4) return false;

    sampXyzLsq.Solve();
    lineXyzLsq.Solve();

    //  Solve for sample, line position corresponding to input lat, lon
    knownXyz[0] = xyz.x();
    knownXyz[1] = xyz.y();
    knownXyz[2] = xyz.z();
    knownXyz[3] = 1;
    double inSamp = sampXyzLsq.Evaluate(knownXyz);
    double inLine = lineXyzLsq.Evaluate(knownXyz);


    if (inSamp < 0.5 || inSamp > p_camera->ParentSamples() + 0.5 ||
        inLine < 0.5 || inLine > p_camera->ParentLines() + 0.5) {
      return false;
    }

    // Sanity check
    p_camera->IgnoreProjection(true);
    p_camera->SetImage(inSamp, inLine);
    p_camera->IgnoreProjection(false);
    if (!p_camera->HasSurfaceIntersection()) return false;

    p_focalPlaneX = inSamp;
    p_focalPlaneY = inLine;

    return true;
  }


  /**
   *  Compute undistorted focal plane coordinate from ground position.
   *
   * @param surfacePoint Ground surface point
   *
   * @return @b bool Indicates whether the conversion was successful.
   *
   * @internal
   */
  bool VimsGroundMap::SetGround(const SurfacePoint &surfacePoint) {
    return SetGround(surfacePoint.GetLatitude(), surfacePoint.GetLongitude());
//    return SetGroundwithLatitudeLongitude(surfacePoint.GetLatitude(), surfacePoint.GetLongitude());
  }


  /**
   * Determines the look direction in the camera coordinate system
   *
   * @param  [out]  v   Look direction vector in camera coordinates
   *
   * This method will compute the look direction vector in the camera coordinate
   * system.  This code was converted from Rick McCloskey's point_tbl c
   * code.
   *
   * @internal
   *   @history 2008-01-031  Tracie Sucharski - Converted Rick's code rather than
   *                                 using the unitVector files from Rick.
   */
  void VimsGroundMap::LookDirection(double v[3]) {

    double x = p_ux - 1. + p_camSampOffset;
    double y = p_uy - 1. + p_camLineOffset;

    //  Compute pointing angles based on pixel size separation
    double theta = Isis::HALFPI - (y - p_yBore) * p_yPixSize;
    double phi = (-1. * Isis::HALFPI) + (x - p_xBore) * p_xPixSize;

    v[0] = sin(theta) * cos(phi);
    v[1] = cos(theta);
    v[2] = sin(theta) * sin(phi) / -1.;
  }
}
