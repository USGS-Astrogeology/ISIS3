/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <string>
#include <iostream>
#include <vector>

#include <QDebug>

#include <geos/geom/Geometry.h>
#include <geos/geom/Polygon.h>
#include <geos/geom/CoordinateSequence.h>
#include <geos/algorithm/LineIntersector.h>
#include <geos/util/IllegalArgumentException.h>
#include <geos/util/TopologyException.h>
#include <geos/util/GEOSException.h>
#include <geos/io/WKTReader.h>
#include <geos/io/WKTWriter.h>
#include <geos/operation/distance/DistanceOp.h>

#include "ImagePolygon.h"
#include "IString.h"
#include "SpecialPixel.h"
#include "PolygonTools.h"

using namespace std;

namespace Isis {

  /**
   *  Constructs a Polygon object, setting the polygon name
   *
   */
  ImagePolygon::ImagePolygon() {
    p_polygons = NULL;

    p_cube = NULL;

    m_leftCoord = NULL;
    m_rightCoord = NULL;
    m_topCoord = NULL;
    m_botCoord = NULL;

    p_cubeStartSamp = 1;
    p_cubeStartLine = 1;

    p_emission = 180.0;
    p_incidence = 180.0;

    p_subpixelAccuracy = 50; //An accuracte and quick number

    p_ellipsoid = false;
  }


  /**
   *  Constructs a Polygon object from a Blob
   *
   */
  ImagePolygon::ImagePolygon(Blob &blob) : ImagePolygon() {
    p_polyStr = string(blob.getBuffer(), blob.Size());

    geos::io::WKTReader *wkt = new geos::io::WKTReader(&(*globalFactory));
    p_polygons = PolygonTools::MakeMultiPolygon(wkt->read(p_polyStr).release());

    p_pts = new geos::geom::CoordinateSequence;

//    for (auto poly : *p_polygons) {
    for(unsigned int g = 0; g < p_polygons->getNumGeometries(); ++g) {
      const geos::geom::Polygon *poly =
          dynamic_cast<const geos::geom::Polygon *>(p_polygons->getGeometryN(g));
      geos::geom::CoordinateSequence coordArray = geos::geom::CoordinateSequence(*(poly->getCoordinates()));
      for (size_t i = 0; i < coordArray.getSize(); i++) {
        p_pts->add(geos::geom::Coordinate(coordArray.getAt(i)));
      }
    }
  }


  //! Destroys the Polygon object
  ImagePolygon::~ImagePolygon() {
    delete p_polygons;
    p_polygons = NULL;

    p_cube = NULL;

    delete m_leftCoord;
    m_leftCoord = NULL;

    delete m_rightCoord;
    m_rightCoord = NULL;

    delete m_topCoord;
    m_topCoord = NULL;

    delete m_botCoord;
    m_botCoord = NULL;
  }


  /**
   * Create a Polygon from given cube
   *
   * @param[in]  cube                 (Cube &)    Cube used to create polygon
   *
   * @param[in]  ss    (Default=1)    (in)       Starting sample number
   * @param[in]  sl    (Default=1)    (in)       Starting Line number
   * @param[in]  ns    (Default=0)    (in)       Number of samples used to create
   *       the polygon. Default of 0 will cause ns to be set to the number of
   *       samples in the cube.
   * @param[in]  nl    (Default=0)    (in)       Number of lines used to create
   *       the polygon. Default of 0 will cause nl to be set to the number of
   *       lines in the cube.
   * @param[in]  band  (Default=1)    (in)       Image band number
   */
  Camera * ImagePolygon::initCube(Cube &cube, int ss, int sl,
                                  int ns, int nl, int band) {
    p_gMap = new UniversalGroundMap(cube);
    p_gMap->SetBand(band);

    p_cube = &cube;

    Camera *cam = NULL;
    p_isProjected = false;

    try {
      cam = cube.camera();
    }
    catch(IException &camError) {
      try {
        cube.projection();
        p_isProjected = true;
      }
      catch(IException &projError) {
        std::string msg = "Can not create polygon, ";
        msg += "cube [" + cube.fileName().toStdString();
        msg += "] is not a camera or map projection";

        IException polyError(IException::User, msg, _FILEINFO_);

        polyError.append(camError);
        polyError.append(projError);

        throw polyError;
      }
    }
    if (cam != NULL) p_isProjected = cam->HasProjection();

    //  Create brick for use in SetImage
    p_brick = new Brick(1, 1, 1, cube.pixelType());

    //------------------------------------------------------------------------
    //  Save cube number of samples and lines for later use.
    //------------------------------------------------------------------------
    p_cubeSamps = cube.sampleCount();
    p_cubeLines = cube.lineCount();

    if (ns != 0) {
      p_cubeSamps = std::min(p_cubeSamps, ss + ns);
    }

    if (nl != 0) {
      p_cubeLines = std::min(p_cubeLines, sl + nl);
    }

    p_cubeStartSamp = ss;
    p_cubeStartLine = sl;

    if (p_ellipsoid && IsLimb() && p_gMap->Camera()) {
      try {
        p_gMap->Camera()->IgnoreElevationModel(true);
      }
      catch(IException &) {
        std::string msg = "Cannot use an ellipsoid shape model";
        msg += " on a limb image without a camera.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    return cam;
  }


  /**
   * Create a Polygon from given cube
   *
   * @param[in]  cube                 (Cube &)    Cube used to create polygon
   *
   * @param[in]  sinc (Default=1)  (in)       Pixel increment to define the
   *       granularity of the resulting polygon in the sample direction
   * @param[in]  linc (Default=1)  (in)       Pixel increment to define the
   *       granularity of the resulting polygon in the line direction
   *
   * @param[in]  ss    (Default=1)    (in)       Starting sample number
   * @param[in]  sl    (Default=1)    (in)       Starting Line number
   * @param[in]  ns    (Default=0)    (in)       Number of samples used to create
   *       the polygon. Default of 0 will cause ns to be set to the number of
   *       samples in the cube.
   * @param[in]  nl    (Default=0)    (in)       Number of lines used to create
   *       the polygon. Default of 0 will cause nl to be set to the number of
   *       lines in the cube.
   * @param[in]  band  (Default=1)    (in)       Image band number
   * @param increasePrecision Iteratively refine sinc and linc (defaults to
   *     false)
   *
   * @history 2008-04-28 Tracie Sucharski, When calculating p_pixInc, set
   *                             to 1 if values calculated is 0.
   * @history 2008-12-30 Tracie Sucharski - If ground map returns pole make
   *                        sure it is actually on the image.
   * @history 2009-05-28 Stuart Sides - Added the quality argument.
   */
  void ImagePolygon::Create(Cube &cube, int sinc, int linc,
                            int ss, int sl, int ns, int nl, int band,
                            bool increasePrecision) {

    Camera *cam = NULL;

    if (sinc < 1 || linc < 1) {
      std::string msg = "Sample and line increments must be 1 or greater";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    cam = initCube(cube, ss, sl, ns, nl, band);

    // Reduce the increment size to find a valid polygon
    bool polygonGenerated = false;
    while (!polygonGenerated) {
      try {
        p_sampinc = sinc;
        p_lineinc = linc;

        p_pts = NULL;
        p_pts = new geos::geom::CoordinateSequence();

        WalkPoly();

        polygonGenerated = true;
      }
      catch (IException &e) {
        delete p_pts;

        sinc = sinc * 2 / 3;
        linc = linc * 2 / 3;

        if (increasePrecision && (sinc > 1 || linc > 1)) {
        }
        else {
          e.print(); // This should be a NAIF error
          std::string msg = "Cannot find polygon for image "
              "[" + cube.fileName().toStdString() + "]: ";
          msg += increasePrecision ? "Cannot increase precision any further" :
              "The increment/step size might be too large";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }
    }

    /*------------------------------------------------------------------------
    /  If image contains 0/360 boundary, the polygon needs to be split up
    /  into multi polygons.
    /-----------------------------------------------------------------------*/
    if (cam) {
      Pvl defaultMap;
      cam->BasicMapping(defaultMap);
    }

    // Create the polygon, fixing if needed
    Fix360Poly();

    if (p_brick != 0) delete p_brick;

    if (p_gMap->Camera())
      p_gMap->Camera()->IgnoreElevationModel(false);
  }


  void ImagePolygon::Create(std::vector<std::vector<double>> polyCoordinates) {
    p_pts = new geos::geom::CoordinateSequence();

    for (std::vector<double> coord : polyCoordinates) {
      p_pts->add(geos::geom::Coordinate(coord[0], coord[1]));
    }

    std::vector<const geos::geom::Geometry *> *polys = new std::vector<const geos::geom::Geometry *>;
    const geos::geom::Polygon *poly = globalFactory->createPolygon(globalFactory->createLinearRing(*p_pts)).release();
    polys->push_back(poly->clone().release());
    // createMultiPolygon takes ownership of polys argument
    p_polygons = globalFactory->createMultiPolygon(*polys).release();

    Fix360Poly();
  }


  /**
  * Finds the next point on the image using a left hand rule walking algorithm. To
  * initiate the walk pass it the same point for both currentPoint and lastPoint.
  *
  * @param[in] currentPoint  (geos::geom::Coordinate *currentPoint)   This is the
  *       currentPoint in the path. You are looking for its successor.
  *
  * @param[in] lastPoint  (geos::geom::Coordinate lastPoint)   This is the
  *       lastPoint in the path, it allows the algorithm to calculate direction.
  *
  * @param[in] recursionDepth  (int)   This optional parameter keeps track
  *       of how far it has walked around a point. By default it is zero.
  *
  * throws Isis::iException::Programmer - Error walking the file
  *
  */
  geos::geom::Coordinate ImagePolygon::FindNextPoint(geos::geom::Coordinate *currentPoint,
      geos::geom::Coordinate lastPoint,
      int recursionDepth) {
    double x = lastPoint.x - currentPoint->x;
    double y = lastPoint.y - currentPoint->y;
    geos::geom::Coordinate result;

    // Check to see if depth is too deep (walked all the way around and found nothing)
    if (recursionDepth > 6) {
      return *currentPoint;
    }

    // Check and walk in appropriate direction
    if (x == 0.0 && y == 0.0) {  // Find the starting point on an image
      for (int line = -1 * p_lineinc; line <= 1 * p_lineinc; line += p_lineinc) {
        for (int samp = -1 * p_sampinc; samp <= 1 * p_sampinc; samp += p_sampinc) {
          double s = currentPoint->x + samp;
          double l = currentPoint->y + line;
          // Try the next left hand rule point if (s,l) does not produce a
          // lat/long or it is not on the image.
          if (!InsideImage(s, l) || !SetImage(s, l)) {
            geos::geom::Coordinate next(s, l);
            return FindNextPoint(currentPoint, next);
          }
        }
      }

      std::string msg = "Unable to create image footprint. Starting point is not on the edge of the image.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    else if (x < 0 && y < 0) {  // current is top left
      geos::geom::Coordinate next(currentPoint->x, currentPoint->y - 1 * p_lineinc);
      MoveBackInsideImage(next.x, next.y, 0, -p_lineinc);
      if (!recursionDepth || !InsideImage(next.x, next.y) || !SetImage(next.x , next.y)) {
        result = FindNextPoint(currentPoint, next, recursionDepth + 1);
      }
      else {
        result = FindBestPoint(currentPoint, next, lastPoint);
      }
    }
    else if (x == 0.0 && y < 0) {  // current is top
      geos::geom::Coordinate next(currentPoint->x + 1 * p_sampinc, currentPoint->y - 1 * p_lineinc);
      MoveBackInsideImage(next.x, next.y, p_sampinc, -p_lineinc);
      if (!recursionDepth || !InsideImage(next.x, next.y) || !SetImage(next.x , next.y)) {
        result = FindNextPoint(currentPoint, next, recursionDepth + 1);
      }
      else {
        result = FindBestPoint(currentPoint, next, lastPoint);
      }
    }
    else if (x > 0 && y < 0) {  // current is top right
      geos::geom::Coordinate next(currentPoint->x + 1 * p_sampinc, currentPoint->y);
      MoveBackInsideImage(next.x, next.y, p_sampinc, 0);
      if (!recursionDepth || !InsideImage(next.x, next.y) || !SetImage(next.x , next.y)) {
        result = FindNextPoint(currentPoint, next, recursionDepth + 1);
      }
      else {
        result = FindBestPoint(currentPoint, next, lastPoint);
      }
    }
    else if (x > 0 && y == 0.0) {  // current is right
      geos::geom::Coordinate next(currentPoint->x + 1 * p_sampinc, currentPoint->y + 1 * p_lineinc);
      MoveBackInsideImage(next.x, next.y, p_sampinc, p_lineinc);
      if (!recursionDepth || !InsideImage(next.x, next.y) || !SetImage(next.x , next.y)) {
        result = FindNextPoint(currentPoint, next, recursionDepth + 1);
      }
      else {
        result = FindBestPoint(currentPoint, next, lastPoint);
      }
    }
    else if (x > 0 && y > 0) {  // current is bottom right
      geos::geom::Coordinate next(currentPoint->x, currentPoint->y + 1 * p_lineinc);
      MoveBackInsideImage(next.x, next.y, 0, p_lineinc);
      if (!recursionDepth || !InsideImage(next.x, next.y) || !SetImage(next.x , next.y)) {
        result = FindNextPoint(currentPoint, next, recursionDepth + 1);
      }
      else {
        result = FindBestPoint(currentPoint, next, lastPoint);
      }
    }
    else if (x == 0.0 && y > 0) {  // current is bottom
      geos::geom::Coordinate next(currentPoint->x - 1 * p_sampinc, currentPoint->y + 1 * p_lineinc);
      MoveBackInsideImage(next.x, next.y, -p_sampinc, p_lineinc);
      if (!recursionDepth || !InsideImage(next.x, next.y) || !SetImage(next.x , next.y)) {
        result = FindNextPoint(currentPoint, next, recursionDepth + 1);
      }
      else {
        result = FindBestPoint(currentPoint, next, lastPoint);
      }
    }
    else if (x < 0 && y > 0) {   // current is bottom left
      geos::geom::Coordinate next(currentPoint->x - 1 * p_sampinc, currentPoint->y);
      MoveBackInsideImage(next.x, next.y, -p_sampinc, 0);
      if (!recursionDepth || !InsideImage(next.x, next.y) || !SetImage(next.x , next.y)) {
        result = FindNextPoint(currentPoint, next, recursionDepth + 1);
      }
      else {
        result = FindBestPoint(currentPoint, next, lastPoint);
      }
    }
    else if (x < 0 && y == 0.0) {  // current is left
      geos::geom::Coordinate next(currentPoint->x - 1 * p_sampinc, currentPoint->y - 1 * p_lineinc);
      MoveBackInsideImage(next.x, next.y, -p_sampinc, -p_lineinc);
      if (!recursionDepth || !InsideImage(next.x, next.y) || !SetImage(next.x , next.y)) {
        result = FindNextPoint(currentPoint, next, recursionDepth + 1);
      }
      else {
        result = FindBestPoint(currentPoint, next, lastPoint);
      }
    }
    else {
      std::string msg = "Unable to create image footprint. Error walking image.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }


    return result;
  }


  /**
   * This method ensures sample/line after sinc/linc have been applied is inside
   * the image. If not, it snaps to the edge of the image - given we didn't start
   * at the edge.
   *
   * @param sample Sample after sinc applied
   * @param line Line after linc applied
   * @param sinc Sample increment (we can back up at most this much)
   * @param linc Line increment (we can back up at most this much)
   */
  void ImagePolygon::MoveBackInsideImage(double &sample, double &line, double sinc, double linc) {
    // snap to centers of pixels!

    // Starting sample to snap to
    const double startSample = p_cubeStartSamp;
    // Ending sample to snap to
    const double endSample = p_cubeSamps;
    // Starting line to snap to
    const double startLine = p_cubeStartLine;
    // Ending line to snap to
    const double endLine = p_cubeLines;
    // Original sample for this point (before sinc)
    const double origSample = sample - sinc;
    // Original line for this point (before linc)
    const double origLine = line - linc;

    // We moved left off the image - snap to the edge
    if (sample < startSample && sinc < 0) {
      // don't snap if we started at the edge
      if (origSample == startSample) {
        return;
      }

      sample = startSample;
    }

    // We moved right off the image - snap to the edge
    if (sample > endSample && sinc > 0) {
      // don't snap if we started at the edge
      if (origSample == endSample) {
        return;
      }

      sample = endSample;
    }

    // We moved up off the image - snap to the edge
    if (line < startLine && linc < 0) {
      // don't snap if we started at the edge
      if (origLine == startLine) {
        return;
      }

      line = startLine;
    }

    // We moved down off the image - snap to the edge
    if (line > endLine && linc > 0) {
      // don't snap if we started at the edge
      if (fabs(origLine - endLine) < 0.5) {
        return;
      }

      line = endLine;
    }

    return;
  }


  /**
   * This returns true if sample/line are inside the cube
   *
   * @param sample
   * @param line
   *
   * @return bool
   */
  bool ImagePolygon::InsideImage(double sample, double line) {
    return (sample >= p_cubeStartSamp - 0.5 &&
            line > p_cubeStartLine - 0.5 &&
            sample <= p_cubeSamps + 0.5 &&
            line <= p_cubeLines + 0.5);
  }


  /**
  * Finds the first point that projects in an image
  *
  * @return geos::geom::Coordinate A starting point that is on the edge of the
  *         polygon.
  */
  geos::geom::Coordinate ImagePolygon::FindFirstPoint() {
    // @todo: Brute force method, should be improved
    for (int sample = p_cubeStartSamp; sample <= p_cubeSamps; sample++) {
      for (int line = p_cubeStartLine; line <= p_cubeLines; line++) {
        if (SetImage(sample, line)) {
          // An outlier check.  Make sure that the pixel we use to start
          // constructing a polygon is not surrounded by a bunch of invalid
          // positions.
          geos::geom::Coordinate firstPoint(sample, line);
          geos::geom::Coordinate lastPoint = firstPoint;
          if (!firstPoint.equals(FindNextPoint(&firstPoint, lastPoint))) {
            return firstPoint;
          }
        }
      }
    }

    // Check to make sure a point was found
    std::string msg = "No lat/lon data found for image";
    throw IException(IException::User, msg, _FILEINFO_);
  }


  /**
   * Retuns the maximum valid sample width of the cube set with either
   * initCube() or Create()
   */
  double ImagePolygon::validSampleDim() {
    double result = 0.0;

    calcImageBorderCoordinates();
    if (m_rightCoord && m_leftCoord)
      result = m_rightCoord->x - m_leftCoord->x + 1;

    return result;
  }


  /**
   * Retuns the maximum valid line width of the cube set with either
   * initCube() or Create()
   */
  double ImagePolygon::validLineDim() {
    double result = 0.0;

    calcImageBorderCoordinates();
    if (m_topCoord && m_botCoord)
      result = m_botCoord->y - m_topCoord->y + 1;

    return result;
  }


  /**
   * Calculates the four border points, particularly for validSampleDim() and
   * validLineDim()
   *
   * @internal
   * @history 2011-05-16 Tracie Sucharski - Fixed typo from p_cubeSamples to
   *                       p_cubeSamps in ASSERT.
   */
  void ImagePolygon::calcImageBorderCoordinates() {
    
    for (int line = p_cubeStartLine; !m_leftCoord && line <= p_cubeLines; line++) {
      for (int sample = p_cubeStartSamp; !m_leftCoord && sample <= p_cubeSamps; sample++) {
        if (SetImage(sample, line)) {
          m_leftCoord = new geos::geom::Coordinate(sample, line);
        }
      }
    }

    if (m_leftCoord) {
      for (int line = p_cubeStartLine; !m_rightCoord && line <= p_cubeLines; line++) {
        for (int sample = p_cubeSamps; !m_rightCoord && sample >= m_leftCoord->x; sample--) {
          if (SetImage(sample, line)) {
            m_rightCoord = new geos::geom::Coordinate(sample, line);
          }
        }
      }
    }

    if (m_leftCoord && m_rightCoord) {
      for (int sample = (int)m_leftCoord->x; !m_topCoord && sample <= m_rightCoord->x; sample++) {
        for (int line = 1; !m_topCoord && line <= p_cubeLines; line++) {
          if (SetImage(sample, line)) {
            m_topCoord = new geos::geom::Coordinate(sample, line);
          }
        }
      }
    }

    if (m_leftCoord && m_rightCoord && m_topCoord) {
      for (int sample = (int)m_leftCoord->x; !m_botCoord && sample <= m_rightCoord->x; sample++) {
        for (int line = p_cube->lineCount(); !m_botCoord && line >= m_topCoord->y; line--) {
          if (SetImage(sample, line)) {
            m_botCoord = new geos::geom::Coordinate(sample, line);
          }
        }
      }
    }
  }


  /**
   * Walks the image finding its lon lat polygon and stores it to p_pts.
   *
   * WARNING: Very large pixel increments for cubes that have cameras/projections
   * with no data at any of the 4 corners can still fail in this algorithm.
   */
  void ImagePolygon::WalkPoly() {
    vector<geos::geom::Coordinate> points;
    double lat, lon, prevLat, prevLon;

    // Find the edge of the polygon
    geos::geom::Coordinate firstPoint = FindFirstPoint();

    points.push_back(firstPoint);
    //************************
    // Start walking the edge
    //************************

    // set up for intialization
    geos::geom::Coordinate currentPoint = firstPoint;
    geos::geom::Coordinate lastPoint = firstPoint;
    geos::geom::Coordinate tempPoint;

    do {
      tempPoint = FindNextPoint(&currentPoint, lastPoint);

      // First check if the distance is within range of skipping
      bool snapToFirstPoint = true;

      // Never needs to find firstPoint on incements of 1
      snapToFirstPoint &= (p_sampinc != 1  &&  p_lineinc != 1);

      // Prevents catching the first point as the last
      snapToFirstPoint &= (points.size() > 2);

      // This method fails for steps larger than line/sample length
      snapToFirstPoint &= (p_sampinc < p_cubeSamps && p_lineinc < p_cubeLines);

      // Checks for appropriate distance without sqrt() call
      int minStepSize = std::min(p_sampinc, p_lineinc);
      snapToFirstPoint &= (DistanceSquared(&currentPoint, &firstPoint) < (minStepSize * minStepSize));

      // Searches for skipped firstPoint due to a large pixinc, by using a pixinc of 1
      if (snapToFirstPoint) {
        tempPoint = firstPoint;
      }
      // If pixinc is greater than line or sample dimention, then we could
      // skip firstPoint. This checks for that case.
      else if (p_sampinc > p_cubeSamps || p_lineinc > p_cubeLines) {
        // This is not expensive because incement must be large
        for (int pt = 0; pt < (int)points.size(); pt ++) {
          if (points[pt].equals(tempPoint)) {
            tempPoint = firstPoint;
            break;
          }
        }
      }
      // Failed to find the next point
      if (tempPoint.equals(currentPoint)) {
        geos::geom::Coordinate oldDuplicatePoint = tempPoint;

        // Init vars for first run through the loop
        tempPoint = lastPoint;
        lastPoint = currentPoint;
        currentPoint = tempPoint;

        // Must be 3 (not 2) to prevent the revisit of the starting point,
        // resulting in an infinite loop
        if (points.size() < 3) {
          std::string msg = "Failed to find next point in the image.";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }

        // remove last point from the list
        points.pop_back();

        tempPoint = FindNextPoint(&currentPoint, lastPoint, 1);

        if (tempPoint.equals(currentPoint) || tempPoint.equals(oldDuplicatePoint)) {
          std::string msg = "Failed to find next valid point in the image.";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }
      }


      // Check for triangle cycles and try to fix
      if ((p_sampinc > 1  ||  p_lineinc > 1) && points.size() >= 3) {
        if (points[points.size()-3].x == tempPoint.x &&
            points[points.size()-3].y == tempPoint.y) {
          // Remove the triangle from the list
          points.pop_back();
          points.pop_back();
          points.pop_back();
          // Reset the current (to be last) point
          currentPoint = points[points.size()-1];
          // change increment to prevent randomly bad pixels in the image
          if (p_sampinc > 1) p_sampinc --;
          if (p_lineinc > 1) p_lineinc --;
        }

        /**
         * If we have a very large polygon, look for the inability to find the
         * starting point by looking for the first cycle in the polygon
         *
         * "very large" is defined as 250 points
         */
        if (points.size() > 250) {
          int cycleStart = 0;
          int cycleEnd = 0;

          for (unsigned int pt = 1; pt < points.size() && cycleStart == 0; pt ++) {
            for (unsigned int check = pt + 1; check < points.size() && cycleStart == 0; check ++) {
              if (points[pt] == points[check]) {
                cycleStart = pt;
                cycleEnd = check;
              }
            }
          }

          // If a cycle was found, make it the polygon
          if (cycleStart != 0) {
            vector<geos::geom::Coordinate> cyclePoints;
            for (int pt = cycleStart; pt <= cycleEnd; pt ++) {
              cyclePoints.push_back(points[pt]);
            }

            points = cyclePoints;
            break;
          }
        }

      }

      lastPoint = currentPoint;
      currentPoint = tempPoint;
      points.push_back(currentPoint);

    }
    while (!currentPoint.equals(firstPoint));

    if (points.size() <= 3) {
      std::string msg = "Failed to find enough points on the image.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    FindSubpixel(points);

    prevLat = 0;
    prevLon = 0;
    // this vector stores crossing points, where the image crosses the
    // meridian. It stores the first coordinate of the pair in its vector
    vector<geos::geom::Coordinate> *crossingPoints = new vector<geos::geom::Coordinate>;
    for (unsigned int i = 0; i < points.size(); i++) {
      geos::geom::Coordinate *temp = &(points.at(i));
      SetImage(temp->x, temp->y);
      lon = p_gMap->UniversalLongitude();
      lat = p_gMap->UniversalLatitude();
      if (abs(lon - prevLon) >= 180 && i != 0) {
        crossingPoints->push_back(geos::geom::Coordinate(prevLon, prevLat));
      }
      p_pts->add(geos::geom::Coordinate(lon, lat));
      prevLon = lon;
      prevLat = lat;
    }


    // Checks for self-intersection and attempts to correct
    geos::geom::CoordinateSequence *tempPts = new geos::geom::CoordinateSequence();

    // Gets the starting, second, second to last, and last points to check for validity
    tempPts->add(geos::geom::Coordinate((*p_pts)[0].x, (*p_pts)[0].y));
    tempPts->add(geos::geom::Coordinate((*p_pts)[1].x, (*p_pts)[1].y));
    tempPts->add(geos::geom::Coordinate((*p_pts)[p_pts->size()-3].x, (*p_pts)[p_pts->size()-3].y));
    tempPts->add(geos::geom::Coordinate((*p_pts)[p_pts->size()-2].x, (*p_pts)[p_pts->size()-2].y));
    tempPts->add(geos::geom::Coordinate((*p_pts)[0].x, (*p_pts)[0].y));

    geos::geom::Polygon *tempPoly = globalFactory->createPolygon
                                    (globalFactory->createLinearRing(*tempPts)).release();

    // Remove the last point of the sequence if it produces invalid polygons
    if (!tempPoly->isValid()) {
      std::vector<geos::geom::Coordinate> coords;
      p_pts->toVector(coords);
      coords.erase(coords.begin() + p_pts->size() - 2);
      p_pts->setPoints(coords);
    }

    delete tempPts;
    tempPts = NULL;
    // end self-intersection check

    FixPolePoly(crossingPoints);
    delete crossingPoints;
    crossingPoints = NULL;
  }


  /**
  *  If the cube crosses the 0/360 boundary and contains a pole, Some  points are
  *  added to allow the polygon to unwrap properly. Throws an error if both poles
  *  are in the image. Returns if there is no pole in the image.
  *
  *  @param crossingPoints The coordinate sequence that crosses the 0/360 boundry
  */
  void ImagePolygon::FixPolePoly(std::vector<geos::geom::Coordinate> *crossingPoints) {
    // We currently do not support both poles in one image
    bool hasNorthPole = false;
    bool hasSouthPole = false;

    if (p_gMap->SetUniversalGround(90, 0)) {
      double nPoleSample = Null;
      double nPoleLine = Null;

      if (p_gMap->Projection()) {
        nPoleSample = p_gMap->Projection()->WorldX();
        nPoleLine = p_gMap->Projection()->WorldY();
      }
      else if (p_gMap->Camera()) {
        nPoleSample = p_gMap->Camera()->Sample();
        nPoleLine = p_gMap->Camera()->Line();
      }

      if (nPoleSample >= 0.5 && nPoleLine >= 0.5 &&
         nPoleSample <= p_cube->sampleCount() + 0.5 &&
         nPoleLine <= p_cube->lineCount() + 0.5 &&
         SetImage(nPoleSample, nPoleLine)) {
        hasNorthPole = true;
      }
    }

    if (p_gMap->SetUniversalGround(-90, 0)) {
      double sPoleSample = Null;
      double sPoleLine = Null;

      if (p_gMap->Projection()) {
        sPoleSample = p_gMap->Projection()->WorldX();
        sPoleLine = p_gMap->Projection()->WorldY();
      }
      else if (p_gMap->Camera()) {
        sPoleSample = p_gMap->Camera()->Sample();
        sPoleLine = p_gMap->Camera()->Line();
      }

      if (sPoleSample >= 0.5 && sPoleLine >= 0.5 &&
         sPoleSample <= p_cube->sampleCount() + 0.5 &&
         sPoleLine <= p_cube->lineCount() + 0.5 &&
         SetImage(sPoleSample, sPoleLine)) {
        hasSouthPole = true;
      }
    }

    if (hasNorthPole && hasSouthPole) {
      std::string msg = "Unable to create image footprint because image has both poles";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    else if (crossingPoints->size() == 0) {
      // No crossing points
      return;
    }

    if (hasNorthPole) {
      p_gMap->SetUniversalGround(90, 0);

      // If the (north) pole is settable but not within proper angles,
      //  then the polygon does not contain the (north) pole when the cube does
      if (p_gMap->Camera() &&
         p_gMap->Camera()->EmissionAngle() > p_emission) {
        return;
      }
      if (p_gMap->Camera() &&
         p_gMap->Camera()->IncidenceAngle() > p_incidence) {
        return;
      }
    }
    else if (hasSouthPole) {
      p_gMap->SetUniversalGround(-90, 0);

      // If the (south) pole is settable but not within proper angles,
      //  then the polygon does not contain the (south) pole when the cube does
      if (p_gMap->Camera() &&
         p_gMap->Camera()->EmissionAngle() > p_emission) {
        return;
      }
      if (p_gMap->Camera() &&
         p_gMap->Camera()->IncidenceAngle() > p_incidence) {
        return;
      }
    }

    geos::geom::Coordinate *closestPoint = &crossingPoints->at(0);
    geos::geom::Coordinate *pole = NULL;

    // Setup the right pole
    if (hasNorthPole) {
      pole = new geos::geom::Coordinate(0, 90);
    }
    else if (hasSouthPole) {
      pole = new geos::geom::Coordinate(0, -90);
    }
    else if (crossingPoints->size() % 2 == 1) {
      geos::geom::Coordinate nPole(0, 90);
      double nDist = DBL_MAX;
      geos::geom::Coordinate sPole(0, -90);
      double sDist = DBL_MAX;

      for (unsigned int index = 0; index < p_pts->size(); index ++) {
        double north = DistanceSquared(&nPole, &((*p_pts)[index]));
        if (north < nDist) {
          nDist = north;
        }
        double south = DistanceSquared(&sPole, &((*p_pts)[index]));
        if (south < sDist) {
          sDist = south;
        }
      }

      if (sDist < nDist) {
        pole = new geos::geom::Coordinate(0, -90);
      }
      else {
        pole = new geos::geom::Coordinate(0, 90);
      }
    }

    // Skip the fix if no pole was determined
    if (pole == NULL) {
      return;
    }

    // Find where the pole needs to be split
    double closestDistance = DBL_MAX;
    for (unsigned int i = 0; i < crossingPoints->size(); i++) {
      geos::geom::Coordinate *temp = &crossingPoints->at(i);
      double tempDistance;
      // Make sure the Lat is as close to 0 as possible for a correct distance
      if (temp->x > 180) {
        double mod = 0.0;
        while ((temp->x - mod) > 180) mod += 360.0;

        // Create the modified Point, create a pointer to it, and find the distance
        geos::geom::Coordinate modPointMem = geos::geom::Coordinate(temp->x - mod, temp->y);
        geos::geom::Coordinate *modPoint = &modPointMem;
        tempDistance = DistanceSquared(modPoint, pole);
      }
      else {
        tempDistance = DistanceSquared(temp, pole);
      }
      if (tempDistance < closestDistance) {
        closestDistance = tempDistance;
        closestPoint = temp;
      }
    }

    if (closestDistance == DBL_MAX) {
      std::string msg = "Image contains a pole but did not detect a meridian crossing!";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // split image at the pole
    geos::geom::CoordinateSequence *new_points = new geos::geom::CoordinateSequence();
    for (unsigned int i = 0; i < p_pts->size(); i++) {
      geos::geom::Coordinate temp = p_pts->getAt(i);
      new_points->add(temp);
      if (temp.equals(*closestPoint)) {
        // Setup direction
        if (i + 1 != p_pts->size()) { // Prevent overflow exception
          double fromLon, toLon;
          if ((p_pts->getAt(i + 1).x - closestPoint->x) > 0) {
            fromLon = 0.0;
            toLon   = 360.0;
          }
          else {
            fromLon = 360.0;
            toLon   = 0.0;
          }

          geos::algorithm::LineIntersector lineIntersector;
          geos::geom::Coordinate crossingPoint;
          geos::geom::Coordinate nPole(0.0, 90.0);
          geos::geom::Coordinate sPole(0.0, -90.0);
          double dist = DBL_MAX;

          for (int num = 0; num < 2 && dist > 180.0; num ++) {
            nPole = geos::geom::Coordinate(num * 360.0, 90.0);
            sPole = geos::geom::Coordinate(num * 360.0, -90.0);

            if (temp.x > 0.0  &&  p_pts->getAt(i + 1).x > 0.0) {
              crossingPoint = geos::geom::Coordinate(p_pts->getAt(i + 1).x - 360.0 + (num * 720.0), p_pts->getAt(i + 1).y);
            }
            else if (temp.x < 0.0  &&  p_pts->getAt(i + 1).x < 0.0) { // This should never happen
              crossingPoint = geos::geom::Coordinate(p_pts->getAt(i + 1).x + 360.0 - (num * 720.0), p_pts->getAt(i + 1).y);
            }

            dist = std::sqrt(DistanceSquared(&temp, &crossingPoint));
          }

          lineIntersector.computeIntersection(nPole, sPole, temp, crossingPoint);

          if (lineIntersector.hasIntersection()) {
            const geos::geom::Coordinate &intersection = lineIntersector.getIntersection(0);

            // Calculate the latituded of the points along the meridian
            if (pole->y < intersection.y) {
              dist = -dist;
            }
            vector<double> lats;
            double maxLat = std::max(intersection.y, pole->y);
            double minLat = std::min(intersection.y, pole->y);
            for (double lat = intersection.y + dist; lat < maxLat  &&  lat > minLat; lat += dist) {
              lats.push_back(lat);
            }

            // Add the new points
            new_points->add(geos::geom::Coordinate(fromLon, intersection.y));
            for (int lat = 0; lat < (int)lats.size(); lat ++) {
              new_points->add(geos::geom::Coordinate(fromLon, lats[lat]));
            }
            new_points->add(geos::geom::Coordinate(fromLon, pole->y));
            new_points->add(geos::geom::Coordinate(toLon, pole->y));
            for (int lat = lats.size() - 1; lat >= 0; lat --) {
              new_points->add(geos::geom::Coordinate(toLon, lats[lat]));
            }
            new_points->add(geos::geom::Coordinate(toLon, intersection.y));
          }
          else {
            std::string msg = "Image contains a pole but could not determine a meridian crossing!";
            throw IException(IException::Programmer, msg, _FILEINFO_);
          }

        }
      }
    }
    delete p_pts;
    p_pts = new_points;
    delete pole;
  }


  /**
   * Sets the sample/line values of the cube to get lat/lon values.  This
   * method checks whether the image pixel is Null for level 2 images and
   * if so, it is considered an invalid pixel.
   *
   * @param[in] sample   (const double)  Sample coordinate of the cube
   *
   * @param[in] line     (const double)  Line coordinate of the cube
   *
   * @return bool Returns true if the image was set successfully and false if it
   *              was not or if pixel of level 2 images is NULL.
   */
  bool ImagePolygon::SetImage(const double sample, const double line) {
    bool found = false;
    if (!p_isProjected) {
      found = p_gMap->SetImage(sample, line);
      if (!found) {
        return false;
      }
      else {
        // Check for valid emission and incidence
        try {
          if (p_gMap->Camera() &&
             p_gMap->Camera()->EmissionAngle() > p_emission) {
            return false;
          }
          if (p_gMap->Camera() &&
             p_gMap->Camera()->IncidenceAngle() > p_incidence) {
            return false;
          }
        }
        catch(IException &error) {
        }

        /**
         * This check has been removed because it causes push frame cameras
         * to fail inbetween the framelets, resulting in only the first
         * framlet to be walked, leaving out the rest of the image.
         *
         * This can cause autoseed/jigsaw issues, since they require conversion
         * from lat/lon to samp/line
         */
        //  Make sure we can go back to image coordinates
        //  This is done because some camera models due to distortion, get
        //  a lat/lon for samp/line=1:1, but entering that lat/lon does
        //  not return samp/line =1:1. Ie.  moc WA global images
        //double lat = p_gMap->UniversalLatitude();
        //double lon = p_gMap->UniversalLongitude();
        //return p_gMap->SetUniversalGround(lat,lon);

        return found;
      }
    }
    else {
      // If projected, make sure the pixel DN is valid before worrying about
      //  geometry.
      p_brick->SetBasePosition((int)sample, (int)line, 1);
      p_cube->read(*p_brick);
      if (Isis::IsNullPixel((*p_brick)[0])) {
        return false;
      }
      else {
        return p_gMap->SetImage(sample, line);
      }
    }
  }


  /**
   * If the cube crosses the 0/360 boundary and does not include a pole, the
   * polygon is separated into multiple polygons, usually one on each side of the
   * boundary. These polygons are put into a geos Multipolygon. If the cube does
   * not cross the 0/360 boundary then the Multipolygon will be a single Polygon.
   */
  void ImagePolygon::Fix360Poly() {
    bool convertLon = false;
    bool negAdjust = false;
    bool newCoords = false;  //  coordinates have been adjusted
    geos::geom::CoordinateSequence *newLonLatPts = new geos::geom::CoordinateSequence();
    double lon, lat;
    double lonOffset = 0;
    double prevLon = p_pts->getAt(0).x;
    double prevLat = p_pts->getAt(0).y;

    newLonLatPts->add(geos::geom::Coordinate(prevLon, prevLat));
    double dist = 0.0;
    for (unsigned int i = 1; i < p_pts->getSize(); i++) {
      lon = p_pts->getAt(i).x;
      lat = p_pts->getAt(i).y;
      
      // check to see if you just crossed the Meridian
      if (abs(lon - prevLon) > 180 && (prevLat != 90 && prevLat != -90)) {
        newCoords = true;
        // if you were already converting then stop (crossed Meridian even number of times)
        if (convertLon) {
          convertLon = false;
          lonOffset = 0;
        }
        else {   // Need to start converting again, deside how to adjust coordinates
          if ((lon - prevLon) > 0) {
            lonOffset = -360.;
            negAdjust = true;
          }
          else if ((lon - prevLon) < 0) {
            lonOffset = 360.;
            negAdjust = false;
          }
          convertLon = true;
        }


      }

      // Change to a minimum calculation
      if (newCoords  &&  dist == 0.0) {
        double longitude = (lon + lonOffset) - prevLon;
        double latitude = lat - prevLat;
        dist = std::sqrt((longitude * longitude) + (latitude * latitude));
      }

      // add coord
      newLonLatPts->add(geos::geom::Coordinate(lon + lonOffset, lat));

      // set current to old
      prevLon = lon;
      prevLat = lat;
    }

    // Nothing was done so return
    if (!newCoords) {
      geos::geom::Polygon *newPoly = globalFactory->createPolygon
                                     (globalFactory->createLinearRing(*newLonLatPts)).release();
      p_polygons = PolygonTools::MakeMultiPolygon(newPoly);
      delete newLonLatPts;
      return;
    }

    // bisect into seperate polygons
    try {
      geos::geom::Polygon *newPoly = globalFactory->createPolygon
                                     (globalFactory->createLinearRing(*newLonLatPts)).release();

      geos::geom::CoordinateSequence *pts = new geos::geom::CoordinateSequence();
      geos::geom::CoordinateSequence *pts2 = new geos::geom::CoordinateSequence();

      // Depending on direction of compensation bound accordingly
      //***************************************************

      // please verify correct if you change these values
      //***************************************************
      if (negAdjust) {
        pts->add(geos::geom::Coordinate(0., 90.));
        pts->add(geos::geom::Coordinate(-360., 90.));
        pts->add(geos::geom::Coordinate(-360., -90.));
        pts->add(geos::geom::Coordinate(0., -90.));
        for (double lat = -90.0 + dist; lat < 90.0; lat += dist) {
          pts->add(geos::geom::Coordinate(0.0, lat));
        }
        pts->add(geos::geom::Coordinate(0., 90.));
        pts2->add(geos::geom::Coordinate(0., 90.));
        pts2->add(geos::geom::Coordinate(360., 90.));
        pts2->add(geos::geom::Coordinate(360., -90.));
        pts2->add(geos::geom::Coordinate(0., -90.));
        for (double lat = -90.0 + dist; lat < 90.0; lat += dist) {
          pts2->add(geos::geom::Coordinate(0.0, lat));
        }
        pts2->add(geos::geom::Coordinate(0., 90.));
      }
      else {
        pts->add(geos::geom::Coordinate(360., 90.));
        pts->add(geos::geom::Coordinate(720., 90.));
        pts->add(geos::geom::Coordinate(720., -90.));
        pts->add(geos::geom::Coordinate(360., -90.));
        for (double lat = -90.0 + dist; lat < 90.0; lat += dist) {
          pts->add(geos::geom::Coordinate(360.0, lat));
        }
        pts->add(geos::geom::Coordinate(360., 90.));
        pts2->add(geos::geom::Coordinate(360., 90.));
        pts2->add(geos::geom::Coordinate(0., 90.));
        pts2->add(geos::geom::Coordinate(0., -90.));
        pts2->add(geos::geom::Coordinate(360., -90.));
        for (double lat = -90.0 + dist; lat < 90.0; lat += dist) {
          pts2->add(geos::geom::Coordinate(360.0, lat));
        }
        pts2->add(geos::geom::Coordinate(360., 90.));
      }

      geos::geom::Polygon *boundaryPoly = globalFactory->createPolygon
                                          (globalFactory->createLinearRing(*pts)).release();
      geos::geom::Polygon *boundaryPoly2 = globalFactory->createPolygon
                                           (globalFactory->createLinearRing(*pts2)).release();
      /*------------------------------------------------------------------------
      /  Intersecting the original polygon (converted coordinates) with the
      /  boundary polygons will create the multi polygons with the converted coordinates.
      /  These will need to be converted back to the original coordinates.
      /-----------------------------------------------------------------------*/
      geos::geom::Geometry *intersection = PolygonTools::Intersect(newPoly, boundaryPoly);
      geos::geom::MultiPolygon *convertPoly = PolygonTools::MakeMultiPolygon(intersection);
      delete intersection;

      intersection = PolygonTools::Intersect(newPoly, boundaryPoly2);
      geos::geom::MultiPolygon *convertPoly2 = PolygonTools::MakeMultiPolygon(intersection);
      delete intersection;

      /*------------------------------------------------------------------------
      / Adjust points created in the negative space or >360 space to be back in
      / the 0-360 world.  This will always only need to be done on convertPoly.
      / Then add geometries to finalpolys.
      /-----------------------------------------------------------------------*/
      std::vector<const geos::geom::Geometry *> *finalpolys = new std::vector<const geos::geom::Geometry *>;
      const geos::geom::Geometry *newGeom = NULL;

      for (size_t i = 0; i < convertPoly->getNumGeometries(); i++) {
        newGeom = (convertPoly->getGeometryN(i))->clone().release();
        geos::geom::CoordinateSequence *pts3 = convertPoly->getGeometryN(i)->getCoordinates().release();
        geos::geom::CoordinateSequence *newLonLatPts = new geos::geom::CoordinateSequence();
        // fix the points

        for (size_t k = 0; k < pts3->getSize() ; k++) {
          double lon = pts3->getAt(k).x;
          double lat = pts3->getAt(k).y;
          if (negAdjust) {
            lon = lon + 360;
          }
          else {
            lon = lon - 360;
          }
          newLonLatPts->add(geos::geom::Coordinate(lon, lat), k);

        }
        // Add the points to polys
        finalpolys->push_back(globalFactory->createPolygon
                              (globalFactory->createLinearRing(*newLonLatPts)).release());
      }

      // This loop is over polygons that will always be in 0-360 space no need to convert
      for (unsigned int i = 0; i < convertPoly2->getNumGeometries(); i++) {
        newGeom = (convertPoly2->getGeometryN(i))->clone().release();
        finalpolys->push_back(newGeom);
      }

      p_polygons = globalFactory->createMultiPolygon(*finalpolys).release();

      delete newLonLatPts;
      delete pts;
      delete pts2;
    }
    catch(geos::util::IllegalArgumentException *geosIll) {
      std::string msg = "Unable to create image footprint (Fix360Poly) due to ";
      msg += "geos illegal argument [" + IString(geosIll->what()) + "]";
      delete geosIll;
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    catch(geos::util::GEOSException *geosExc) {
      std::string msg = "Unable to create image footprint (Fix360Poly) due to ";
      msg += "geos exception [" + IString(geosExc->what()) + "]";
      delete geosExc;
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    catch(IException &e) {
      std::string msg = "Unable to create image footprint (Fix360Poly) due to ";
      msg += "isis operation exception [" + IString(e.what()) + "]";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
    catch(std::exception &e) {
      std::string msg = "Caught std::exception: ";
      msg += e.what();
      throw IException(IException::Unknown, msg, _FILEINFO_); 
    }
    catch(...) {
      std::string msg = "Unable to create image footprint (Fix360Poly) due to ";
      msg += "unknown exception";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
  }


  /**
   * Serialize the ImagePolygon to a Blob.
   *
   * The polygon will be serialized as a WKT srtring.
   *
   * @return @b Blob
   */
  Blob ImagePolygon::toBlob() const {
    // Check to see p_polygons is valid data
    if (!p_polygons) {
      string msg = "Cannot write a NULL polygon!";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    
    geos::io::WKTWriter *wkt = new geos::io::WKTWriter();
    wkt->setTrim(true);

    string polyStr = wkt->write(p_polygons);
    delete wkt;

    Blob newBlob("Footprint", "Polygon");
    newBlob.setData(polyStr.c_str(), polyStr.size());
    return newBlob;
  }


  /**
   * Calculates the distance squared between two coordinates.
   *
   * @param p1 The first Coordinate for the calculation
   * @param p2 The second Coordinate for the calculation
   *
   * return The distance squared between the Coordinates
   */
  double ImagePolygon::DistanceSquared(const geos::geom::Coordinate *p1, const geos::geom::Coordinate *p2) {
    return ((p2->x - p1->x) * (p2->x - p1->x)) + ((p2->y - p1->y) * (p2->y - p1->y));
  }


  /**
   * Returns True when the input image is a limb image
   *
   * @return bool True if any of the 4 corners of the input image cannot be set
   */
  bool ImagePolygon::IsLimb() {
    bool hasFourCorners = true;
    hasFourCorners &= SetImage(1, 1);
    hasFourCorners &= SetImage(p_cubeSamps, 1);
    hasFourCorners &= SetImage(p_cubeSamps, p_cubeLines);
    hasFourCorners &= SetImage(1, p_cubeLines);
    return !hasFourCorners;
  }



  /**
   * While walking the image in sample/line space, this function finds the best
   * valid point between the first valid point found and the last point which
   * failed its validity test using a linear search.
   *
   * @param currentPoint The last point added to the polygon
   * @param newPoint  The first valid point found for the next step.
   * @param lastPoint The last point that was found to be invalid which checking
   *                  for the next step.
   *
   * @return geos::geom::Coordinate The valid point found via searching between
   *         the provided points.
   */
  geos::geom::Coordinate ImagePolygon::FindBestPoint(geos::geom::Coordinate *currentPoint,
      geos::geom::Coordinate newPoint,
      geos::geom::Coordinate lastPoint) {
    geos::geom::Coordinate result = newPoint;
    // Use a binary search to snap to the limb when needed
    if (p_sampinc > 1  ||  p_lineinc > 1) {

      // Pull the invalid point back inside the image
      double x = lastPoint.x;
      double y = lastPoint.y;
      if (x < p_cubeStartSamp) {
        x = p_cubeStartSamp;
      }
      else if (x > p_cubeSamps) {
        x = p_cubeSamps;
      }
      if (y < p_cubeStartLine) {
        y = p_cubeStartLine;
      }
      else if (y > p_cubeLines) {
        y = p_cubeLines;
      }
      geos::geom::Coordinate invalid(x, y);
      geos::geom::Coordinate valid(result.x, result.y);

      // Find the best valid Coordinate
      while (!SetImage(invalid.x, invalid.y)) {
        int x, y;
        if (invalid.x > valid.x) {
          x = (int)invalid.x - 1;
        }
        else if (invalid.x < valid.x) {
          x = (int)invalid.x + 1;
        }
        else {
          x = (int)invalid.x;
        }
        if (invalid.y > valid.y) {
          y = (int)invalid.y - 1;
        }
        else if (invalid.y < valid.y) {
          y = (int)invalid.y + 1;
        }
        else {
          y = (int)invalid.y;
        }
        invalid = geos::geom::Coordinate(x, y);
      }

      result = FixCornerSkip(currentPoint, invalid);
    }

    return result;
  }


  /**
   * Looks at the next possible point relative to the lasts and attempts to adjust
   * the point outward to grab valid corner data.
   *
   * @param currentPoint The last valid point added to the polygon
   * @param newPoint The new point to be added to the polygon
   *
   * @return geos::geom::Coordinate The new point modified for a better result
   */
  geos::geom::Coordinate ImagePolygon::FixCornerSkip(geos::geom::Coordinate *currentPoint,
      geos::geom::Coordinate newPoint) {
    geos::geom::Coordinate originalPoint = newPoint;
    geos::geom::Coordinate modPoint = newPoint;

    // Step Too big
    if (p_sampinc > p_cubeSamps || p_lineinc > p_cubeLines) {
      return newPoint;
    }

    // An upper left corner
    else if (currentPoint->x < newPoint.x && currentPoint->y > newPoint.y) {
      while (newPoint.x >= currentPoint->x && SetImage(newPoint.x, newPoint.y)) {
        modPoint = newPoint;
        newPoint.x -= 1;
      }
    }

    // An upper right corner
    else if (currentPoint->y < newPoint.y && currentPoint->x < newPoint.x) {
      while (newPoint.y >= currentPoint->y && SetImage(newPoint.x, newPoint.y)) {
        modPoint = newPoint;
        newPoint.y -= 1;
      }
    }

    // An lower right corner
    else if (currentPoint->x > newPoint.x && currentPoint->y < newPoint.y) {
      while (newPoint.x <= currentPoint->x && SetImage(newPoint.x, newPoint.y)) {
        modPoint = newPoint;
        newPoint.x += 1;
      }
    }

    // An lower left corner
    else if (currentPoint->y > newPoint.y && currentPoint->x > newPoint.x) {
      while (newPoint.y <= currentPoint->y && SetImage(newPoint.x, newPoint.y)) {
        modPoint = newPoint;
        newPoint.y += 1;
      }
    }

    if (currentPoint->x == modPoint.x && currentPoint->y == modPoint.y) {
      return originalPoint;
    }
    return modPoint;
  }


  /**
   * Takes p_polygons in sample/line space and finds its subpixel accuracy. This
   * algorithm depends on a left-hand-turn algorithm and assumes that the vector
   * of Coordinates provided is not empty.
   *
   * @param points The vector of Coordinate to set to subpixel accuracy
   */
  void ImagePolygon::FindSubpixel(std::vector<geos::geom::Coordinate> & points) {
    if (p_subpixelAccuracy > 0) {

      // Fix the polygon with subpixel accuracy
      geos::geom::Coordinate old = points.at(0);
      bool didStartingPoint = false;
      for (unsigned int pt = 1; !didStartingPoint; pt ++) {
        if (pt >= points.size() - 1) {
          pt = 0;
          didStartingPoint = true;
        }

        // Binary Coordinate Search
        double maxStep = std::max(p_sampinc, p_lineinc);
        double stepY = (old.x - points.at(pt + 1).x) / maxStep;
        double stepX = (points.at(pt + 1).y - old.y) / maxStep;

        geos::geom::Coordinate valid = points.at(pt);
        geos::geom::Coordinate invalid(valid.x + stepX, valid.y + stepY);

        for (int itt = 0; itt < p_subpixelAccuracy; itt ++) {
          geos::geom::Coordinate half((valid.x + invalid.x) / 2.0, (valid.y + invalid.y) / 2.0);
          if (SetImage(half.x, half.y)  &&  InsideImage(half.x, half.y)) {
            valid = half;
          }
          else {
            invalid = half;
          }
        }

        old = points.at(pt);

        // Set new coordinate
        points[pt] = valid;

      }

      // Fix starting point
      points[points.size()-1] = geos::geom::Coordinate(points[0].x, points[0].y);

    }
  }


} // end namespace isis
