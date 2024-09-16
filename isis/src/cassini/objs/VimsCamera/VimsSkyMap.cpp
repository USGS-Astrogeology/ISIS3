/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "VimsSkyMap.h"

#include <iostream>
#include <iomanip>

#include <QDebug>

#include "Camera.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "LeastSquares.h"
#include "PolynomialBivariate.h"
#include "Preference.h"
#include "SpecialPixel.h"

using namespace std;

namespace Isis {
  /**
   * Constructs the VimsSkyMap object
   *
   * @param parent A pointer to the parent Camera object
   * @param lab Pvl labels for the image
   *
   * @internal
   */
  VimsSkyMap::VimsSkyMap(Camera *parent, Pvl &lab) :
    CameraSkyMap(parent) {
  }


  /**
   * Initialize vims sky model
   *
   * @param [in] lab   (Pvl &)      Cube Pvl label
   *
   * @throw iException::Io - "Cannot process NYQUIST(undersampled) mode "
   * @throw iException::Io - "Can't open unit vector file"
   * @internal
   *   @history  2007-04-16 Tracie Sucharski - Look for unit vectors in
   *                              the proper directory.
   *   @history  2007-04-18 Tracie Sucharski, The inaccuracy of the 15 Mhz clock
   *                    (exposure , interline_delay) is already taken care of
   *                     in the labels values, so remove the adjustment from the
   *                     code (exp * 1.01725).
   *                     *Reference:  email from John Ivens 11/27/2006.
   */
  void VimsSkyMap::Init(Pvl &lab) {
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
    p_irExp = (std::stod(inst ["ExposureDuration"][0]) * 1.01725) / 1000.;
    p_visExp = std::stod(inst ["ExposureDuration"][1]) / 1000.;
    p_interlineDelay = (std::stod(inst ["InterlineDelayDuration"]) * 1.01725) / 1000.;

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

    //  Calculate ra/dec maps
    for(int line = 0; line < p_camera->ParentLines(); line++) {
      for(int samp = 0; samp < p_camera->ParentSamples(); samp++) {
        p_raMap[line][samp] = Isis::NULL8;
        p_decMap[line][samp] = Isis::NULL8;
      }
    }

    //---------------------------------------------------------------------
    //  Loop for each pixel in cube, get pointing information and calculate
    //  control point (line,sample,lat,lon) for later use in latlon_to_linesamp.
    //---------------------------------------------------------------------
    p_minRa = 99999.;
    p_minDec = 99999.;
    p_maxRa = -99999.;
    p_maxDec = -99999.;

    p_camera->IgnoreProjection(true);
    for(int line = 0; line < p_camera->ParentLines(); line++) {

      //  VIS exposure is for a single line.  According to SIS,
      //  NATIVE_START_TIME is for the first pixel of the IR exposure.
      //  "The offset from IR start to VIS start is calculated by
      //   IrExposMsec - VisExposMsec)/2".
      //  This needs to be moved to forward routine

      if(p_channel == "VIS") {
        double et = ((double)p_etStart + (((p_irExp * p_swathWidth) - p_visExp) / 2.)) +
                    ((line + 0.5) * p_visExp);
        p_camera->setTime(et);
      }

      for(int samp = 0; samp < p_camera->ParentSamples(); samp++) {
        if(p_channel == "IR") {
          double et = (double)p_etStart +
                      (line * p_camera->ParentSamples() * p_irExp) +
                      (line * p_interlineDelay) + ((samp + 0.5) * p_irExp);
          p_camera->setTime(et);
        }

        p_camera->SetImage((double) samp + 1, (double)line + 1);
        double ra = p_camera->RightAscension();
        double dec = p_camera->Declination();
        if(ra < p_minRa) p_minRa = ra;
        if(ra > p_maxRa) p_maxRa = ra;
        if(dec < p_minDec) p_minDec = dec;
        if(dec > p_maxDec) p_maxDec = dec;
        p_raMap[line][samp] = ra;
        p_decMap[line][samp] = dec;
      }
    }
    p_camera->IgnoreProjection(false);

  }


  /** Compute ground position from focal plane coordinate
   *
   * This method will compute the ground position given an
   * undistorted focal plane coordinate.  Note that the latitude/longitude
   * value can be obtained from the camera class passed into the constructor.
   *
   * @param ux distorted focal plane x in millimeters
   * @param uy distorted focal plane y in millimeters
   * @param uz distorted focal plane z in millimeters
   *
   * @return conversion was successful
   */
  bool VimsSkyMap::SetFocalPlane(const double ux, const double uy,
                                 const double uz) {
    p_ux = ux;
    p_uy = uy;
    p_uz = uz;

    double imgSamp = ux;
    double imgLine = uy;

    if((imgLine < 0.5) || (imgLine > p_camera->ParentLines() + 0.5) ||
       (imgSamp < 0.5) || (imgSamp > p_camera->ParentSamples() + 0.5)) {
      return false;
    }
    imgLine--;
    imgSamp--;

    // does interline_delay & exposure-duration account for summing modes?
    // if not, won't use p_parentLine/p_parentSample
    double et = 0.;
    if(p_channel == "VIS") {
      et = (p_etStart + ((p_irExp * p_swathWidth) - p_visExp) / 2.) +
           ((imgLine + 0.5) * p_visExp);
    }
    else if(p_channel == "IR") {
      et = (double)p_etStart +
           (imgLine * p_camera->ParentSamples() * p_irExp) +
           (imgLine * p_interlineDelay) + ((imgSamp + 0.5) * p_irExp);
    }
    p_camera->setTime(et);

    SpiceDouble lookC[3];
    LookDirection(lookC);

    SpiceDouble unitLookC[3];
    vhat_c(lookC, unitLookC);
    return p_camera->SetLookDirection(unitLookC);
  }

  /**
   *  Sets the sky position to the given ra and dec
   *
   * @param ra the right ascension
   * @param dec the declination
   *
   * @return set sky was successful
   */
  bool VimsSkyMap::SetSky(const double ra, const double dec) {
    if(ra < p_minRa || ra > p_maxRa ||
        dec < p_minDec || dec > p_maxDec) {
      return false;
    }
    //  Find closest points  ??? what tolerance ???
    double minDist = 9999.;
    int minSamp = -1;
    int minLine = -1;

    for(int line = 0; line < p_camera->ParentLines(); line++) {

      for(int samp = 0; samp < p_camera->ParentSamples(); samp++) {
        double mapRa = p_raMap[line][samp];
        if(mapRa == Isis::NULL8) continue;
        double mapDec = p_decMap[line][samp];
        if(mapDec == Isis::NULL8) continue;
        //  If on boundary convert lons.  If trying to find 360, convert
        //  lons on other side of meridian to values greater than 360.  If
        //  trying to find 1.0, convert lons on other side to negative numbers.
        if(abs(mapRa - ra) > 180) {
          if((ra - mapRa) > 0) {
            mapRa = 360. + mapRa;
          }
          else if((ra - mapRa) < 0) {
            mapRa = mapRa - 360.;
          }
        }
        double dist = ((ra - mapRa) * (ra - mapRa)) +
                      ((dec - mapDec) * (dec - mapDec));
        if(dist < minDist) {
          minDist = dist;
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
    if(minDist >= 9999.) return false;

    //-------------------------------------------------------------
    //  Set-up for LU decomposition (least2 fit).
    //  Assume we will have 9 control points, this may not be true
    //  and will need to be adjusted before the final solution.
    //-------------------------------------------------------------
    PolynomialBivariate sampBasis(1);
    PolynomialBivariate lineBasis(1);
    LeastSquares sampLsq(sampBasis);
    LeastSquares lineLsq(lineBasis);
    vector<double> known(2);

    for(int line = minLine - 1; line < minLine + 2; line++) {
      if(line < 0 || line > p_camera->ParentLines() - 1) continue;
      for(int samp = minSamp - 1; samp < minSamp + 2; samp++) {
        //  Check for edges
        if(samp < 0 || samp > p_camera->ParentSamples() - 1) continue;

        double mapRa = p_raMap[line][samp];
        double mapDec = p_decMap[line][samp];
        if((mapRa == Isis::NULL8) || (mapDec == Isis::NULL8)) continue;

        //  If on boundary convert lons.  If trying to find 360, convert
        //  lons on other side of meridian to values greater than 360.  If
        //  trying to find 1.0, convert lons on other side to negative numbers.
        if(abs(mapRa - ra) > 180) {
          if((ra - mapRa) > 0) {
            mapRa = 360. + mapRa;
          }
          else if((ra - mapRa) < 0) {
            mapRa = mapRa - 360.;
          }
        }

        known[0] = mapDec;
        known[1] = mapRa;
        sampLsq.AddKnown(known, samp + 1);
        lineLsq.AddKnown(known, line + 1);
      }
    }
    if(sampLsq.Knowns() < 3) return false;

    sampLsq.Solve();
    lineLsq.Solve();

    //  Solve for new sample position
    known[0] = dec;
    known[1] = ra;
    double inSamp = sampLsq.Evaluate(known);
    double inLine = lineLsq.Evaluate(known);

    if(inSamp < 0 || inSamp > p_camera->ParentSamples() + 0.5 ||
        inLine < 0 || inLine > p_camera->ParentLines() + 0.5) {
      return false;
    }

    p_camera->IgnoreProjection(true);
    p_camera->SetImage(inSamp, inLine);
    p_camera->IgnoreProjection(false);
    p_focalPlaneX = inSamp;
    p_focalPlaneY = inLine;

    return true;
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
   *   @history 2008-01-03  Tracie Sucharski - Converted Rick'scode rather than using the unitVector
   *                     files from Rick.
   *   @history 2013-11-18  Tracie Sucharski - Added this method to VimsSkyMap,so that old
   *                           unitVector files are no longer needed.
   */
  void VimsSkyMap::LookDirection(double v[3]) {
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
