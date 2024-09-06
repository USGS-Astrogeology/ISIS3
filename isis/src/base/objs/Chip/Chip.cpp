/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Chip.h"
#include "Camera.h"
#include "Cube.h"
#include "IException.h"
#include "Interpolator.h"
#include "IString.h"
#include "LineManager.h"
#include "PolygonTools.h"
#include "Portal.h"
#include "Projection.h"
#include "Statistics.h"
#include "TProjection.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <geos/geom/Point.h>
#include <tnt/tnt_array2d_utils.h>

using namespace std;
namespace Isis {

  /**
   * Constructs a Chip. The default size is 3x3.
   */
  Chip::Chip() {
    Init(3, 3);
  }

  /**
   * Constructs a copy of the passed in chip.
   *
   * @param other the chip to be copied
   */
  Chip::Chip(const Chip &other) {
    m_chipSamples = other.m_chipSamples;
    m_chipLines = other.m_chipLines;
    m_buf = other.m_buf;
    m_tackSample = other.m_tackSample;
    m_tackLine = other.m_tackLine;

    m_cubeTackSample = other.m_cubeTackSample;
    m_cubeTackLine = other.m_cubeTackLine;

    m_validMinimum = other.m_validMinimum;
    m_validMaximum = other.m_validMaximum;

    m_chipSample = other.m_chipSample;
    m_chipLine = other.m_chipLine;
    m_cubeSample = other.m_cubeSample;
    m_cubeLine = other.m_cubeLine;

    if (other.m_clipPolygon) {
      m_clipPolygon = PolygonTools::MakeMultiPolygon(
          other.m_clipPolygon->clone().release());
    }
    else {
      m_clipPolygon = NULL;
    }

    m_affine = other.m_affine;
    m_readInterpolator = other.m_readInterpolator;
    m_filename = other.m_filename;
  }


  /**
   * Construct a Chip with specified dimensions
   *
   * @param samples   number of samples in the chip
   * @param lines     number of lines in the chip
   */
  Chip::Chip(const int samples, const int lines) {
    Init(samples, lines);
  }


  //! Destroys the Chip object
  Chip::~Chip() {
    if (m_clipPolygon != NULL) delete m_clipPolygon;
  }


  /**
   * @brief Single value assignment operator
   *
   * Sets the entire chip to a constant
   *
   * @param d Value to set the chip to
   */
  void Chip::SetAllValues(const double &d) {
    for (unsigned int i = 0; i < m_buf.size(); i++) {
      fill(m_buf[i].begin(), m_buf[i].end(), d);
    }
  }


  /**
   * Common initialization used by constructors
   *
   * @param samples   number of samples in the chip
   * @param lines     number of lines in the chip
   * @internal
   *   @history 2010-06-15 Jeannie Walldren - Added call to set Read() method's interpolator
   *                           default to Cubic Convolution type
   */
  void Chip::Init(const int samples, const int lines) {
    SetReadInterpolator(Interpolator::CubicConvolutionType);
    SetSize(samples, lines);
    SetValidRange();
    m_clipPolygon = NULL;
  }


  /**
   * Change the size of the Chip
   *
   * @param samples   number of samples in the chip
   * @param lines     number of lines in the chip
   * @throws Isis::iException::User - Samples and lines must be greater than
   *                                        zero.
   * @internal
   *   @history 2010-06-10 Jeannie Walldren - Modified error message
   */
  void Chip::SetSize(const int samples, const int lines) {
    if ( samples <= 0.0 || lines <= 0.0 ) {
      std::string msg;
      msg += "Unable to set chip size to [";
      msg += toString(samples) + ", ";
      msg += toString(lines) + "]. Samples and lines must be greater than zero.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    m_chipSamples = samples;
    m_chipLines = lines;
    m_buf.clear();
    m_buf.resize(lines);
    for (int i = 0; i < lines; i++) {
      m_buf[i].resize(samples);
    }
    m_affine.Identity();
    m_tackSample = ((samples - 1) / 2) + 1;
    m_tackLine = ((lines - 1) / 2) + 1;
  }


  /**
  *
  * @param sample
  * @param line
  * @return @b bool True if the given sample, line is inside the
  *         chip
  */
  bool Chip::IsInsideChip(double sample, double line) {
    double minSamp = m_cubeTackSample - ((m_chipSamples - 1) / 2);
    double maxSamp = m_cubeTackSample + ((m_chipSamples - 1) / 2);

    double minLine = m_cubeTackLine - ((m_chipLines - 1) / 2);
    double maxLine = m_cubeTackLine + ((m_chipLines - 1) / 2);

    if ( sample < minSamp || sample > maxSamp ) return false;
    if ( line < minLine || line > maxLine ) return false;
    return true;
  }


  /**
   * This sets which cube position will be located at the chip
   * tack position
   *
   * @param cubeSample    the cube sample value to tack
   * @param cubeLine      the cube line value to tack
   */
  void Chip::TackCube(const double cubeSample, const double cubeLine) {
    m_cubeTackSample = cubeSample;
    m_cubeTackLine = cubeLine;
    m_affine.Identity();
    m_affine.Translate(m_cubeTackSample, m_cubeTackLine);
  }


  /**
   * Load cube data into the Chip. The data will be loaded such that the position set using
   * TackCube method will be put at the center of the chip. The data will be loaded to sub-pixel
   * accuracy using the interpolator indicated using SetReadInterpolator() method.
   *
   * @param cube      The cube used to put data into the chip
   * @param rotation  rotation in degrees of data about the cube tack point (default of 0)
   * @param scale     scale factor (default of 1)
   * @param band      Band number to use when loading (default of 1)
   * @see Read()
   * @see SetReadInterpolator()
   * @see GetReadInterpolator()
   */
  void Chip::Load(Cube &cube, const double rotation, const double scale, const int band) {
    // Initialize our affine transform
    m_affine.Identity();

    // We want an affine which translates from chip to cube. Note that we want to give adjusted
    // chip coordinates such that (0,0) is at the chip tack point and maps to the cube tack point.
    m_affine.Scale(scale);
    m_affine.Rotate(rotation);
    m_affine.Translate(m_cubeTackSample, m_cubeTackLine);

    // Now go read the data from the cube into the chip
    Read(cube, band);

    // Store off the cube address in case someone wants to match this chip
    m_filename = cube.fileName();
  }


  /**
   * @brief Load a chip using an Affine transform as provided by caller
   *
   * This method will load data from a cube using an established Affine transform as provided by
   * the caller. It is up to the caller to set up the affine appropriately.
   *
   * For example, the first thing this method will do is set the chip tack point to the transformed
   * cube location by replacing the existing affine transform with the one passed in and then
   * calling SetChipPosition providing the chip tack point as the argument. This establishes which
   * cube pixel is located at the chip tack point.
   *
   * The data will be loaded to sub-pixel accuracy using the interpolator indicated
   * using SetReadInterpolator() method.
   *
   * @param cube Cube to load the data from
   * @param affine Affine transform to set for chip load/operations
   * @param keepPoly Indicates whether clipping polygon should be kept or removed
   *                 (default of true)
   * @param band Band number to read data from (default of 1)
   * @see Read()
   * @see SetReadInterpolator()
   * @see GetReadInterpolator()
   */
  void Chip::Load(Cube &cube, const Affine &affine, const bool &keepPoly, const int band) {

    //  Set the tackpoint center to the cube location
    SetTransform(affine);
    SetChipPosition(TackSample(), TackLine());

    //  Remove the clipping polygon if requested
    if (!keepPoly) {
      delete m_clipPolygon;
      m_clipPolygon = 0;
    }

    // Now go read the data from the cube into the chip
    Read(cube, band);

    // Store off the cube address in case someone wants to match this chip
    m_filename = cube.fileName();
  }


  /**
   * Loads cube data into the Chip. The data will be loaded such that the position set using
   * TackCube method will be put at the center of the chip. The data will be loaded to sub-pixel
   * accuracy using the interpolator indicated using SetReadInterpolator() method.
   * Additionally, the data will be loaded such that it matches the camera and/or projective
   * geometry of a given Chip.
   *
   * @param cube      The cube used to put data into the chip
   * @param match     Match the geometry of this chip
   * @param matchChipCube The cube used to put data into the match chip
   * @param scale     scale factor (default of 1)
   * @param band      Band number to use when loading (default of 1)
   *
   * @throws Isis::iException::Programmer - Chip cube is not a camera or map
   *                                        projection
   * @throws Isis::iException::Programmer - Match chip cube is not a camera or
   *                                        map projection
   * @throws Isis::iException::Programmer - Cannot find enough points to perform
   *                                        Affine transformation.
   * @see Read()
   * @see SetReadInterpolator()
   * @see GetReadInterpolator()
   *
   * @internal
   *   @history 2010-01-28 Tracie Sucharski - When calculating control points away from corners,
   *                           added a linc to move into the center of the chip in a non-linear
   *                           fashion to prevent control points that fall in a line and cause
   *                           the matrix inversion to fail.
   *   @history 2010-05-24 Jeannie Walldren - Modified code when looking for control points for
   *                           affine to start at each corner and move inward rather than looping
   *                           around the corners. This lessens the likelyhood that the points will
   *                           be too linear for the transform to work properly. A check was also
   *                           added to ensure that we do not choose colinear points.
   *   @history 2010-06-10 Jeannie Walldren - Modified error message and added tolerance to
   *                           linearity check. Fixed error messages.
   */
  void Chip::Load(Cube &cube, Chip &match, Cube &matchChipCube, const double scale, const int band)
  {
    // See if the match cube has a camera or projection
    Camera *matchCam = NULL;
    TProjection *matchProj = NULL;
    try {
      matchCam = matchChipCube.camera();
    }
    catch (IException &error1) {
      try {
        matchProj = (TProjection *) matchChipCube.projection();
      }
      catch (IException &error2) {
        std::string msg = "Can not geom chip. ";
        msg += "Match chip cube [" + matchChipCube.fileName().toStdString();
        msg += "] is not a camera or map projection";

        IException fullError(IException::User, msg, _FILEINFO_);
        fullError.append(error1);
        fullError.append(error2);

        throw fullError;
      }
    }

    // See if the cube we are loading has a camera/projection
    Camera *cam = NULL;
    Projection *proj = NULL;
    try {
      cam = cube.camera();
    }
    catch (IException &error1) {
      try {
        proj = cube.projection();
      }
      catch (IException &error2) {
        std::string msg = "Can not geom chip. ";
        msg += "Chip cube [" + cube.fileName().toStdString();
        msg += "] is not a camera or map projection";

        IException fullError(IException::User, msg, _FILEINFO_);
        fullError.append(error1);
        fullError.append(error2);

        throw fullError;
      }
    }

    // Ok we can attempt to create an affine transformation that maps our chip to the match chip.
    // We will need a set of at least 3 control points so we can fit the affine transform.
    // We will try to find 4 points, one from each corner of the chip.
    vector<double> x(4), y(4);
    vector<double> xp(4), yp(4);

    // Choose these control points by beginning at each corner and moving inward in the chip
    // until an acceptable point is found
    // i = 0, start at upper left corner  (1, 1)
    // i = 1, start at lower left corner  (1, Lines()-1)
    // i = 2, start at upper right corner (Samples()-1, 1)
    // i = 3, start at lower right corner (Samples()-1, Lines()-1)
    for (int i = 0; i < (int) xp.size(); i++) {
      // define initial values for starting/ending sample/line for each index
      int startSamp = 1;
      int startLine = 1;
      int endSamp = Samples() - 1;
      int endLine = Lines() - 1;

      bool pointfound = false;
      while (!pointfound) {
        // start and end may cross (see MovePoints())
        // if we move outside chip, break out of loop
        if (startSamp < 1 || startSamp > Samples() - 1 ||
            endSamp   < 1 || endSamp   > Samples() - 1 ||
            startLine < 1 || startLine > Lines() - 1   ||
            endLine   < 1 || endLine   > Lines() - 1) {
          // unable to find acceptable control point from this corner
          // erase point and go to the next corner
          x.erase(x.begin() + i);
          y.erase(y.begin() + i);
          xp.erase(xp.begin() + i);
          yp.erase(yp.begin() + i);
          i--;
          break;
        }
        int chipSamp, chipLine;
        if (i < 2) {
          chipSamp = startSamp;
        }
        else {
          chipSamp = endSamp;
        }
        if (i % 2 == 0) {
          chipLine = startLine;
        }
        else {
          chipLine = endLine;
        }
        // Determine the offset from the tack point in our chip to one of the four corners.
        int sampOffset = chipSamp - TackSample();
        int lineOffset = chipLine - TackLine();

        // Use this offset to compute a chip position in the match chip.
        double matchChipSamp = match.TackSample() + sampOffset;
        double matchChipLine = match.TackLine() + lineOffset;

        // Now get the lat/lon at that chip position.
        match.SetChipPosition(matchChipSamp, matchChipLine);
        double lat, lon;
        if (matchCam != NULL) {
          matchCam->SetImage(match.CubeSample(), match.CubeLine());
          if (!matchCam->HasSurfaceIntersection() ) {
            vector<int> newlocation = MovePoints(startSamp, startLine, endSamp, endLine);
            startSamp = newlocation[0];
            startLine = newlocation[1];
            endSamp = newlocation[2];
            endLine = newlocation[3];
            continue;
          }
          lat = matchCam->UniversalLatitude();
          lon = matchCam->UniversalLongitude();
        }
        else {
          matchProj->SetWorld(match.CubeSample(), match.CubeLine() );
          if (!matchProj->IsGood() ) {
            vector<int> newlocation = MovePoints(startSamp, startLine, endSamp, endLine);
            startSamp = newlocation[0];
            startLine = newlocation[1];
            endSamp = newlocation[2];
            endLine = newlocation[3];
            continue;
          }
          lat = matchProj->UniversalLatitude();
          lon = matchProj->UniversalLongitude();
        }

        // Now use that lat/lon to find a line/sample in our chip
        double line, samp;
        if (cam != NULL) {
          cam->SetUniversalGround(lat, lon);
          if (!cam->HasSurfaceIntersection() ) {
            vector<int> newlocation = MovePoints(startSamp, startLine, endSamp, endLine);
            startSamp = newlocation[0];
            startLine = newlocation[1];
            endSamp = newlocation[2];
            endLine = newlocation[3];
            continue;
          }
          samp = cam->Sample();    // getting negative sample?!?!?!
          line = cam->Line();
        }
        else {
          proj->SetUniversalGround(lat, lon);
          if (!proj->IsGood()) {
            vector<int> newlocation = MovePoints(startSamp, startLine, endSamp, endLine);
            startSamp = newlocation[0];
            startLine = newlocation[1];
            endSamp = newlocation[2];
            endLine = newlocation[3];
            continue;
          }
          samp = proj->WorldX();
          line = proj->WorldY();
        }

        //     if (line < 1 || line > cube.lineCount()) continue;
        //     if (samp < 1 || samp > cube.sampleCount()) continue;

        // Ok save this control point
        pointfound = true;
        x[i] = sampOffset;
        y[i] = lineOffset;
        xp[i] = samp;
        yp[i] = line;

        // If we get 3 points on the same line, affine transform will fail.
        // Choose a one degree default tolerance for linearity check method.
        double tol = 1.0;
        // If we have already removed a point, use a stricter tolerance of 2 degrees.
        if (xp.size() == 3) {
          tol = 2.0;
        }
        if (i > 1) {
          if ( PointsColinear(xp[0], yp[0], xp[1], yp[1], xp[i], yp[i], tol) ) {
            // try to find a point further along that is not colinear
            pointfound = false;
            vector<int> newlocation = MovePoints(startSamp, startLine, endSamp, endLine);
            startSamp = newlocation[0];
            startLine = newlocation[1];
            endSamp = newlocation[2];
            endLine = newlocation[3];
            continue;
          }
        }
      }
    }

    if (xp.size() < 3) {
      std::string msg = "Cannot find enough points to perform Affine transformation. ";
      msg += "Unable to load chip from [" + cube.fileName().toStdString();
      msg += "] to match chip from [" + matchChipCube.fileName().toStdString() + "].";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Now take our control points and create the affine map
    m_affine.Solve(&x[0], &y[0], &xp[0], &yp[0], (int)x.size());

    //  TLS  8/3/06  Apply scale
    m_affine.Scale(scale);

    // Finally we need to make the affine map the tack point to the requested cube sample/line
    m_affine.Compute(0.0, 0.0);
    double cubeSampleOffset = m_cubeTackSample - m_affine.xp();
    double cubeLineOffset = m_cubeTackLine - m_affine.yp();
    m_affine.Translate(cubeSampleOffset, cubeLineOffset);

    // Now go read the data from the cube into the chip
    Read(cube, band);

    // Store off the cube address in case someone wants to match
    // this chip
    m_filename = cube.fileName();
  }


  /**
   * This method is called by Load() to determine whether the given 3 points are nearly colinear.
   * This is done by considering the triangle composed of these points. The method returns true if
   * all angles of the triangle are greater than the tolerance angle and less than 180 degrees,
   * minus the tolerance angle.
   *
   * @param x0     The x-value of the first point
   * @param y0     The y-value of the first point
   * @param x1     The x-value of the second point
   * @param y1     The y-value of the second point
   * @param x2     The x-value of the third point
   * @param y2     The y-value of the third point
   * @param tol    Minimum tolerance angle in degrees
   *
   * @return @b bool True if 3 given points are nearly colinear
   *
   * @internal
   *   @author Jeannie Walldren
   *   @history 2010-05-24 Jeannie Walldren - Original version.
   *   @history 2010-06-10 Jeannie Walldren - Modified to take in user defined tolerance as
   *                           parameter to allow registration of narrow search chip areas
   */
  bool Chip::PointsColinear(const double x0, const double y0, const double x1, const double y1,
                            const double x2, const double y2, const double tol) {
    // check angles at each point of the triangle composed of the 3 points
    //  if any angle is near 0 or 180 degrees, then the points are almost colinear

    // we have the following property:
    // sin(theta) = |v x w|/(|v|*|w|) where
    //     v=(vx,vy) and w=(wx,wy) are the vectors that define the angle theta
    //     |v| is the magnitude (norm) of the vector. In 2D, this is |v| = sqrt(vx^2 + vy^2)
    //     v x w is the cross product of the vectors. In 2D, this is v x w = vx*wy-vy*wx
    // See equations (5) and (6) at http://mathworld.wolfram.com/CrossProduct.html


    // first find the vectors that define the angles at each point
    // For example, if we shift the point P0 to the origin,
    //     the vectors defining the angle at P0
    //     are v01 = (x1-x0, y1-y0) and v02 = (x2-x0, y2-y0)
    // Note: v10 = -v01 and |v x w| = |w x v|
    // so we only need 3 vectors and the order we use these doesn't matter
    vector<double> v01, v12, v20;
    v01.push_back(x1 - x0);
    v01.push_back(y1 - y0);
    v12.push_back(x2 - x1);
    v12.push_back(y2 - y1);
    v20.push_back(x0 - x2);
    v20.push_back(y0 - y2);

    // sin(angle at P0) = |v01 x v02|/(|v01|*|v02|) =
    //      |v01x*v02y-v01y*v02x|/(sqrt(v01x^2+v01y^2)*sqrt(v01x^2+v02y^2))
    double sinP0 = fabs(v01[0] * v20[1] - v01[1] * v20[0]) /
            sqrt((pow(v01[0], 2) + pow(v01[1], 2)) * (pow(v12[0], 2) + pow(v12[1], 2)));
    // sin(angle at P1)
    double sinP1 = fabs(v12[0] * v20[1] - v12[1] * v20[0]) /
            sqrt((pow(v12[0], 2) + pow(v12[1], 2)) * (pow(v20[0], 2) + pow(v20[1], 2)));
    // sin(angle at P2)
    double sinP2 = fabs(v20[0] * v01[1] - v20[1] * v01[0]) /
            sqrt((pow(v20[0], 2) + pow(v20[1], 2)) * (pow(v01[0], 2) + pow(v01[1], 2)));

    // We will seek angles with sine near 0 (thus a multiple of 180 degrees or pi radians)
    // We will use a tolerance of tol degrees (tol*pi/180 radians)
    // Compare the smallest sine value to the sine of tol,
    // if it is less, then the angle is less than tol degrees or
    // greater than 180-tol degrees, so points are almost colinear
    double minSinValue = min(sinP0, min(sinP1, sinP2));
    if ( minSinValue < sin(tol * PI / 180) ) {
      return true;
    }
    else {
      return false;
    }
  }


  /**
   * This method is called by Load() to move a control point
   * across the chip.
   *
   * @param *startSamp   Sample value to be increased, or moved right.
   * @param *startLine   Line value to be increased, or moved downward.
   * @param *endSamp     Sample value to be decreased, or moved left.
   * @param *endLine     Line value to be decreased, or moved upward.
   *
   * @return @b vector < @b int > Vector containing the new sample and line values in the same
   *                     order as the parameters passed into the method.
   * @internal
   *   @author Tracie Sucharski
   *   @history 2010-05-24 Jeannie Walldren - Moved from Load() method to its own method since this
   *                           code needed to be repeated several times.
   */
  vector<int> Chip::MovePoints(const int startSamp, const int startLine,
                               const int endSamp, const int endLine) {
    vector<int> newlocations(4);
    int sinc = (endSamp - startSamp) / 4;
    // Ensures that the inc can cause start and end to cross
    if (sinc < 1) {
      sinc = 1;
    }
    int linc = (endLine - startLine) / 3;
    // Ensures that the inc can cause start and end to cross
    if (linc < 1) {
      linc = 1;
    }
    newlocations[0] = startSamp + sinc;
    newlocations[1] = startLine + linc;
    newlocations[2] = endSamp - sinc;
    newlocations[3] = endLine - linc;
    return newlocations;
  }

  /**
   * Compute the position of the cube given a chip coordinate. Any rotation or geometric matching
   * done during the Load process will be taken into account. Use CubeSample and CubeLine methods
   * to obtain results. Note the results could be outside of the cube
   *
   * @param sample    chip sample coordinate
   * @param line      chip line coordinate
   */
  void Chip::SetChipPosition(const double sample, const double line) {
    m_chipSample = sample;
    m_chipLine = line;
    m_affine.Compute(sample - TackSample(), line - TackLine());
    m_cubeSample = m_affine.xp();
    m_cubeLine = m_affine.yp();
  }


  /**
   * Compute the position of the chip given a cube coordinate. Any rotation or geometric matching
   * done during the Load process will be taken into account. Use  ChipSample and ChipLine methods
   * to obtain results. Note that the results could be outside of the chip.
   *
   * @param sample    chip sample coordinate
   * @param line      chip line coordinate
   */
  void Chip::SetCubePosition(const double sample, const double line) {
    m_cubeSample = sample;
    m_cubeLine = line;
    m_affine.ComputeInverse(sample, line);
    m_chipSample = m_affine.x() + TackSample();
    m_chipLine = m_affine.y() + TackLine();
  }


  /**
   * Set the valid range of data in the chip.
   * If never called all data in the chip is consider valid (other than special pixels).
   *
   * @param minimum   minimum valid pixel value (default of Isis::ValidMinimum)
   * @param maximum   maximum valid pixel value (default of Isis::ValidMaximum)
   *
   * @throws Isis::iException::Programmer - First parameter must be smaller than
   *             the second.
   * @internal
   *   @history 2010-06-10 Jeannie Walldren - Modified error message
   */
  void Chip::SetValidRange(const double minimum, const double maximum) {
    if (minimum >= maximum) {
      std::string msg = "Unable to set valid chip range to [" + toString(minimum);
      msg += ", " + toString(maximum) + "]. First parameter must be smaller than the second.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    m_validMinimum = minimum;
    m_validMaximum = maximum;
  }


  /**
   * Return if total number of valid pixels in chip meets a specified percentage of the entire chip.
   *
   * @param percentage The percentage that the valid pixels percentage must exceed
   *
   * @return bool Returns true if the percentage of valid pixels is greater than the
   *              specified percentage, and false if it is not
   */
  bool Chip::IsValid(double percentage) {
    int validCount = 0;
    for (int samp = 1; samp <= Samples(); samp++) {
      for (int line = 1; line <= Lines(); line++) {
        if (IsValid(samp, line)) validCount++;
      }
    }
    double validPercentage = 100.0 * (double) validCount /
                             (double)(Samples() * Lines());
    if (validPercentage < percentage) return false;
    return true;
  }


  /**
   * Extract a sub-chip from a chip.
   *
   * @param samples   Number of samples in the extracted chip (must be less than or equal to "this" chip)
   * @param lines     Number of lines in the extracted chip (must be less than or equal to "this" chip)
   * @param samp      Input chip sample to be placed at output chip tack
   * @param line      Input chip line to be placed at output chip tack
   * @return @b Chip Sub-chip extracted from the chip
   * @throws Isis::IException::Programmer - Chip extraction invalid
   * @internal
   *   @history 2010-06-10 Jeannie Walldren - Modified error message
   */
  Chip Chip::Extract(int samples, int lines, int samp, int line) {
    if ( samples > Samples() || lines > Lines() ) {
      std::string msg = "Cannot extract sub-chip of size [" + toString(samples);
      msg += ", " + toString(lines) + "] from chip of size [" + toString(Samples());
      msg += ", " + toString(Lines()) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    Chip chipped(samples, lines);
    for (int oline = 1; oline <= lines; oline++) {
      for (int osamp = 1; osamp <= samples; osamp++) {
        int thisSamp = samp + (osamp - chipped.TackSample());
        int thisLine = line + (oline - chipped.TackLine());
        if ((thisSamp < 1) ||
            (thisLine < 1) ||
            (thisSamp > Samples()) ||
            (thisLine > Lines()) ) {
          chipped.SetValue(osamp, oline, Isis::Null);
        }
        else {
          chipped.SetValue(osamp, oline, GetValue(thisSamp, thisLine));
        }
      }
    }

    chipped.m_affine = m_affine;
    chipped.m_validMinimum = m_validMinimum;
    chipped.m_validMaximum = m_validMaximum;
    chipped.m_tackSample = chipped.TackSample() + TackSample() - samp;
    chipped.m_tackLine = chipped.TackLine() + TackLine() - line;

    return chipped;
  }

  /**
   * @brief Extract a subchip centered at the designated coordinate
   *
   * This method extracts a subchip that is centered at the given sample and
   * line coordinate. All appropriate variables in the given chipped parameter
   * are set appropriately prior to return.
   *
   * @param samp Center (tack) sample chip coordinate to extract subchip
   * @param line Center (tack) line chip coordinate to extract subchip
   * @param chipped Chip to load the subchip in and return to caller
   */
  void Chip::Extract(int samp, int line, Chip &chipped) {
    int samples = chipped.Samples();
    int lines = chipped.Lines();
    //chipped.Init(samples, lines);
    chipped.m_tackSample = ((samples - 1) / 2) + 1;
    chipped.m_tackLine = ((lines - 1) / 2) + 1;

    for (int oline = 1; oline <= lines; oline++) {
      for (int osamp = 1; osamp <= samples; osamp++) {
        int thisSamp = samp + (osamp - chipped.TackSample());
        int thisLine = line + (oline - chipped.TackLine());
        if ((thisSamp < 1) ||
            (thisLine < 1) ||
            (thisSamp > Samples()) ||
            (thisLine > Lines()) ) {
          chipped.SetValue(osamp, oline, Isis::Null);
        }
        else {
          chipped.SetValue(osamp, oline, GetValue(thisSamp, thisLine));
        }
      }
    }

    chipped.m_affine = m_affine;
    chipped.m_validMinimum = m_validMinimum;
    chipped.m_validMaximum = m_validMaximum;
    chipped.m_tackSample = chipped.TackSample() + TackSample() - samp;
    chipped.m_tackLine = chipped.TackLine() + TackLine() - line;

    return;
  }


  /**
   * @brief Extract a subchip of this chip using an Affine transform
   *
   * This method will translate the data in this chip using an Affine transform to the output
   * chip as provided. Note that the Affine transformation is only applied within the confines
   * of this chip. No file I/O is performed.
   *
   * A proper Affine transform should not deviate too much from the identity as the mapping
   * operation may result in a NULL filled chip.
   * The operation of this affine is added to the existing affine so proper relationship to the
   * input cube (and any affine operations applied at load time) is preserved. This implies that
   * the resulting affine should yield nearly identical results when read directly from the cube.
   *
   * Bilinear interpolation is applied to surrounding transformed pixels
   * to provide each new output pixel.
   *
   * The chipped parameter will be updated to fully reflect the state of this original chip.
   * The state of the chipped parameter dictates the size and the tack sample and line coordinates.
   * Upon return, the corresponding cube sample and line coordinate is updated to the tack sample
   * and line chip coordinate.
   *
   * As such, note that an identity affine transform will yield identical results to the
   * Chip::Extract method specifying the tack sample and line as the location to extract.
   *
   * The following example demonstrates how to linearly shift a chip one pixel right and one down.
   * @code
   *   Chip mychip(35,35);
   *   Cube cube("mycube.cub");
   *   mychip.TackCube(200.0,200.0);
   *   mychip.Load(cube);
   *
   *   Affine shift;
   *   shift.Translate(-1.0,-1.0);
   *
   *   Chip ochip(15,15);
   *   mychip.Extract(ochip, shift);
   * @endcode
   *
   * @param chipped  Input/output chip containing the transformed subchip
   * @param affine   Affine transform to apply to extract subchip
   */
  void Chip::Extract(Chip &chipped, Affine &affine) {
    // Create an interpolator and portal for interpolation
    Interpolator interp(Interpolator::BiLinearType);
    Portal port(interp.Samples(), interp.Lines(), Isis::Double,
                interp.HotSample(), interp.HotLine());

    int samples = chipped.Samples();
    int lines = chipped.Lines();

    for (int oline = 1; oline <= lines; oline++) {
      int relativeLine = oline - chipped.TackLine();
      for (int osamp = 1; osamp <= samples; osamp++) {
        int relativeSamp = osamp - chipped.TackSample();
        affine.Compute(relativeSamp, relativeLine);
        double xp = affine.xp() + TackSample();
        double yp = affine.yp() + TackLine();
        port.SetPosition(xp, yp, 1);
        for (int i = 0; i < port.size(); i++) {
          int csamp = port.Sample(i);
          int cline = port.Line(i);
          if ((csamp < 1) ||
              (cline < 1) ||
              (csamp > Samples()) ||
              (cline > Lines()) ) {
            port[i] = Isis::Null;
          }
          else {
            port[i] = GetValue(csamp, cline);
          }
        }
        chipped.SetValue(osamp, oline, interp.Interpolate(xp, yp, port.DoubleBuffer()));
      }
    }

    chipped.m_validMinimum = m_validMinimum;
    chipped.m_validMaximum = m_validMaximum;
    chipped.m_filename = m_filename;

    // Make necessary adjustments to remaining chip elements. Note that this matrix multiply
    // acheives a completed transform of two Affine matrices.
    // No translations are required - only update tack points.
    chipped.m_affine = Affine(TNT::matmult(affine.Forward(), m_affine.Forward()));

    affine.Compute(0.0, 0.0);
    chipped.m_cubeTackSample = m_cubeTackSample + affine.xp();
    chipped.m_cubeTackLine = m_cubeTackLine + affine.yp();

    chipped.m_chipSample = chipped.TackSample();
    chipped.m_chipLine   = chipped.TackLine();
    chipped.m_cubeSample = chipped.m_cubeTackSample;
    chipped.m_cubeLine   = chipped.m_cubeTackLine;

    return;
  }


  /**
   * Returns a statistics object of the current data in the chip.
   *
   * The caller takes ownership of the returned instance.
   *
   * @return Isis::Statistics* Statistics of the data in the chip
   */
  Isis::Statistics *Chip::Statistics() {
    Isis::Statistics *stats = new Isis::Statistics();

    stats->SetValidRange(m_validMinimum, m_validMaximum);

    for (int i = 0; i < m_chipSamples; i++) {
      stats->AddData(&m_buf[i][0], m_chipLines);
    }

    return stats;
  }


  /**
   * This method reads data from a cube and puts it into the chip. The affine transform is used in
   * the SetChipPosition routine, so the geom of the chip is automatic. This method uses a default
   * interpolator type of Cubic Convolution. This can be changed using SetReadInterpolator().
   *
   * @param cube    Cube to read data from
   * @param band    Band number to read data from
   * @see SetReadInterpolator
   *
   * @todo We could modify the affine class to return the coefficients and then compute the
   *       derivative of the change in cube sample and line with respect to chip sample or line.
   *       The change might make the geom run a bit faster.
   * @see SetReadInterpolator()
   * @see GetReadInterpolator()
   * @internal
   *   @history 2010-06-15 Jeannie Walldren - Modified to allow any interpolator type except "None"
   */
  void Chip::Read(Cube &cube, const int band) {
    // Create an interpolator and portal for geoming
    Interpolator interp(m_readInterpolator);
    Portal port(interp.Samples(), interp.Lines(), cube.pixelType(),
                interp.HotSample(), interp.HotLine());
    // Loop through the pixels in the chip and geom them
    for (int line = 1; line <= Lines(); line++) {
      for (int samp = 1; samp <= Samples(); samp++) {
        SetChipPosition((double)samp, (double)line);
        if ((CubeSample() < 0.5) ||
            (CubeLine() < 0.5) ||
            (CubeSample() > cube.sampleCount() + 0.5) ||
            (CubeLine() > cube.lineCount() + 0.5)) {

          m_buf[line-1][samp-1] = Isis::NULL8;
        }
        else if (m_clipPolygon == NULL) {
          port.SetPosition(CubeSample(), CubeLine(), band);
          cube.read(port);
          m_buf[line-1][samp-1] =
            interp.Interpolate(CubeSample(), CubeLine(), port.DoubleBuffer());
        }
        else {
          std::unique_ptr<geos::geom::Point> pnt = globalFactory->createPoint(
              geos::geom::Coordinate(CubeSample(), CubeLine()));
          if (pnt->within(m_clipPolygon)) {
            port.SetPosition(CubeSample(), CubeLine(), band);
            cube.read(port);
            m_buf[line-1][samp-1] =
              interp.Interpolate(CubeSample(), CubeLine(), port.DoubleBuffer());
          }
          else {
            m_buf[line-1][samp-1] = Isis::NULL8;
          }
          pnt.reset();
        }
      }
    }
  }


  /**
   * Writes the contents of the Chip to a cube.
   *
   * @param filename  Name of the cube to create
   */
  void Chip::Write(const QString &filename) {
    Cube c;
    c.setDimensions(Samples(), Lines(), 1);
    c.create(filename);
    LineManager line(c);
    for (int i = 1; i <= Lines(); i++) {
      line.SetLine(i);
      for (int j = 1; j <= Samples(); j++) {
        line[j-1] = GetValue(j, i);
      }
      c.write(line);
    }
    c.close();
  }


  /**
   * Sets the clipping polygon for this chip. The coordinates must be in (sample,line) order.
   * All Pixel values outside this polygon will be set to Null8. The cubic convolution
   * interpolation is allowed to uses valid pixels outside the clipping area.
   *
   * @param clipPolygon  The polygons used to clip the chip
   */
  void Chip::SetClipPolygon(const geos::geom::MultiPolygon &clipPolygon) {
    if (m_clipPolygon != NULL) delete m_clipPolygon;
    m_clipPolygon = PolygonTools::CopyMultiPolygon(clipPolygon);
  }


  /**
   * Copy assignment operator.
   *
   * @param other chip to be copied to this
   *
   * @returns The chip that was copied from the other
   */
  Chip &Chip::operator=(const Chip &other) {
    m_chipSamples = other.m_chipSamples;
    m_chipLines = other.m_chipLines;
    m_buf = other.m_buf;
    m_tackSample = other.m_tackSample;
    m_tackLine = other.m_tackLine;

    m_cubeTackSample = other.m_cubeTackSample;
    m_cubeTackLine = other.m_cubeTackLine;

    m_validMinimum = other.m_validMinimum;
    m_validMaximum = other.m_validMaximum;

    m_chipSample = other.m_chipSample;
    m_chipLine = other.m_chipLine;
    m_cubeSample = other.m_cubeSample;
    m_cubeLine = other.m_cubeLine;

    // Free allocated memory.
    if (m_clipPolygon) {
      delete m_clipPolygon;
      m_clipPolygon = NULL;
    }

    if (other.m_clipPolygon) {
      m_clipPolygon = PolygonTools::MakeMultiPolygon(
          other.m_clipPolygon->clone().release());
    }

    m_affine = other.m_affine;
    m_readInterpolator = other.m_readInterpolator;
    m_filename = other.m_filename;

    return *this;
  }
} // end namespace isis
