/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <string>
#include <vector>
#include <cmath>

#include "Pvl.h"
#include "PvlGroup.h"
#include "IException.h"
#include "PolygonTools.h"
#include "IString.h"

#include "GridPolygonSeeder.h"

namespace Isis {

  /**
   * @brief Construct a GridPolygonSeeder algorithm
   *
   *
   * @param pvl  A Pvl object that contains a valid polygon point
   * seeding definition
   */
  GridPolygonSeeder::GridPolygonSeeder(Pvl &pvl) : PolygonSeeder(pvl) {
    Parse(pvl);
  };


  /**
   * @brief Seed a polygon with points
   *
   * Seed the supplied polygon with points in a grid pattern. The spacing
   * is determined by the PVL group "PolygonSeederAlgorithm"
   *
   * @param lonLatPoly geos::MultiPolygon The polygon to be seeded with
   *                  points.
   * @param proj The Projection to seed the polygon into
   *
   * @return std::vector<geos::Point*> A vector of points which have been
   * seeded into the polygon. The caller assumes responsibility for deleteing
   *  these.
   *
   *  @internal
   *   @history 2007-05-09 Tracie Sucharski,  Changed a single spacing value
   *                            to a separate value for x and y.
   */
  std::vector<geos::geom::Point *> GridPolygonSeeder::Seed(const geos::geom::MultiPolygon *lonLatPoly) {
    //Projection *proj) {
    /*if (proj == NULL) {
      QString msg = "No Projection object available";
      throw iException::Message(iException::Programmer, msg, _FILEINFO_);
    }*/

    if(!p_subGrid)
      //return SeedGrid(lonLatPoly, proj);
      return SeedGrid(lonLatPoly);
    else
      //return SeedSubGrid(lonLatPoly, proj);
      return SeedSubGrid(lonLatPoly);

  }

  std::vector<geos::geom::Point *> GridPolygonSeeder::SeedGrid(const geos::geom::MultiPolygon *multiPoly) {

    // Storage for the points to be returned
    std::vector<geos::geom::Point *> points;

    // Create some things we will need shortly
    const geos::geom::Envelope *polyBoundBox = multiPoly->getEnvelopeInternal();

    // Call the parents standardTests member
    QString msg = StandardTests(multiPoly, polyBoundBox);
    if(!msg.isEmpty()) {
      return points;
    }

    // Do grid specific tests to make sure this poly should be seeded
    // (none for now)

    // Starting at the centroid of the xy polygon populate the polygon with a
    // grid of points with the requested spacing
    geos::geom::Point *centroid = multiPoly->getCentroid().release();
    double centerX = centroid->getX();
    double centerY = centroid->getY();
    delete centroid;

    int xStepsLeft = (int)((centerX - polyBoundBox->getMinX()) / p_Xspacing + 0.5);
    int yStepsLeft = (int)((centerY - polyBoundBox->getMinY()) / p_Yspacing + 0.5);
    double dRealMinX = centerX - (xStepsLeft * p_Xspacing);
    double dRealMinY = centerY - (yStepsLeft * p_Yspacing);

    for(double y = dRealMinY; y <= polyBoundBox->getMaxY(); y += p_Yspacing) {
      for(double x = dRealMinX; x <= polyBoundBox->getMaxX(); x += p_Xspacing) {
        geos::geom::Coordinate c(x, y);
        geos::geom::Point *p = Isis::globalFactory->createPoint(c).release();

        if(p->within(multiPoly)) {
          points.push_back(Isis::globalFactory->createPoint(c).release());
        }
        else {
          delete p;
        }
      }
    }

    return points;
  }

  /**
   * This method works a lot like SeedGrid, except around the edges of known polygons.
   * This method varies in that every grid square around the edge of a found polygon will
   * be searched in more depth than all other grid squares.
   *
   * @param lonLatPoly
   * @param proj
   *
   * @return std::vector<geos::Point*> List of found points inside the polygon
   */
  std::vector<geos::geom::Point *> GridPolygonSeeder::SeedSubGrid(const geos::geom::MultiPolygon *multiPoly) {
    //Projection *proj) {
    // Storage for the points to be returned
    std::vector<geos::geom::Point *> points;

    // Create some things we will need shortly
    //geos::geom::MultiPolygon *xymp = PolygonTools::LatLonToXY(*lonLatPoly, proj);
    const geos::geom::Envelope *polyBoundBox = multiPoly->getEnvelopeInternal();

    // Call the parents standardTests member
    QString msg = StandardTests(multiPoly, polyBoundBox);
    if(!msg.isEmpty()) {
      return points;
    }

    geos::geom::Point *centroid = multiPoly->getCentroid().release();
    double centerX = centroid->getX();
    double centerY = centroid->getY();
    delete centroid;

    // Do grid specific tests to make sure this poly should be seeded
    // (none for now)

    /**
     * Every square in the grid needs to be monitored, we'll need to know if:
     *   (a) center needs checked - pointShouldCheck
     *   (b) entire square needs checked using precision, next to found pt - pointShouldSubGridCheck
     *   (c) A point was found in the square - pointFound
     *   (d) The center of the square is not found, but the square hasnt been checked in depth - pointNotFound
     *   (e) The square has been checked in depth and no valid points found - pointCantFind
     */
    enum PointStatus {
      pointShouldCheck,
      pointShouldSubGridCheck,
      pointFound,
      pointNotFound,
      pointCantFind
    };

    // For maintaining an idea of what's going on in this polygon, we needs to know the dimensions
    //   of the grid.
    int xSteps = (int)((polyBoundBox->getMaxX() - polyBoundBox->getMinX()) / p_Xspacing + 1.5);
    int ySteps = (int)((polyBoundBox->getMaxY() - polyBoundBox->getMinY()) / p_Yspacing + 1.5);
    PointStatus pointCheck[xSteps][ySteps];

    // Initialize our grid of point status'
    for(int y = 0; y < ySteps; y++) {
      for(int x = 0; x < xSteps; x++) {
        pointCheck[x][y] = pointShouldCheck;
      }
    }

    /**
     * This is a pretty good equation for how much precision is to be used in the in-depth checks
     *   around the edges of polygons.
     *
     * Thickness * Depth^2 <= 0.5 (0.5 is a constant, the larger the more precision to be used)
     * Depth^2 <= 0.5/Thickness
     * Depth <= (0.5/Thickness)^0.5
     */
    int precision = (int)pow(0.5 / MinimumThickness(), 0.5) * 2;
    bool bGridCleared = true;
    int xStepsToCentroid = (int)((centerX - polyBoundBox->getMinX()) / p_Xspacing + 0.5);
    int yStepsToCentroid = (int)((centerY - polyBoundBox->getMinY()) / p_Yspacing + 0.5);
    double dRealMinX = centerX - (xStepsToCentroid * p_Xspacing);
    double dRealMinY = centerY - (yStepsToCentroid * p_Yspacing);

    do {
      // gridCleared is true if we did nothing, if we performed any actions on the grid
      //   it becomes false and another pass should be used.
      bGridCleared = true;

      for(int y = 0; y < ySteps; y++) {
        double centerY = dRealMinY + p_Yspacing * y;
        for(int x = 0; x < xSteps; x++) {
          double centerX = dRealMinX + p_Xspacing * x;
          geos::geom::Point *p = NULL;

          // pointShouldCheck tells us we need to check center. Calling
          //   CheckSubGrid with precision=0 will do this for us.
          if(pointCheck[x][y] == pointShouldCheck) {
            p = CheckSubGrid(*multiPoly, centerX, centerY, 0);
          }
          // pointShouldSubGridCheck tells us we're next to a grid
          //   square where a point was found, so check in depth
          else if(pointCheck[x][y] == pointShouldSubGridCheck) {
            p = CheckSubGrid(*multiPoly, centerX, centerY, precision);
          }

          // If we found a point, verify we can setCoordinate and save the point
          if(p != NULL) {
            // Convert the x/y point to a lon/lat point
            /*if (proj->SetCoordinate(p->getX(),p->getY())) {
              points.push_back(Isis::globalFactory->createPoint(
                  geos::geom::Coordinate(proj->UniversalLongitude(),
                  proj->UniversalLatitude())));
            }
            else {
              IString msg = "Unable to convert [(" + IString(x) + ",";
              msg += IString(y) + ")] to a (lon,lat)";
              throw iException::Message(iException::Programmer, msg, _FILEINFO_);
            }*/
            points.push_back(Isis::globalFactory->createPoint(
                               geos::geom::Coordinate(p->getX(), p->getY())).release());

            // We found something new and need a new pass
            bGridCleared = false;
            pointCheck[x][y] = pointFound;
          }
          else {
            if(pointCheck[x][y] == pointShouldCheck) {
              pointCheck[x][y] = pointNotFound;
            }
            else if(pointCheck[x][y] == pointShouldSubGridCheck) {
              pointCheck[x][y] = pointCantFind;
            }
          }
        }
      }

      // now that the grid has been updated with it's founds, we can look for subgrid checks
      for(int y = 0; y < ySteps; y++) {
        for(int x = 0; x < xSteps; x++) {
          if(pointCheck[x][y] == pointFound) {
            for(int yOff = -1; yOff <= 1; yOff++) {
              for(int xOff = -1; xOff <= 1; xOff++) {
                if(x + xOff >= 0 && x + xOff < xSteps &&
                    y + yOff >= 0 && y + yOff < ySteps &&
                    pointCheck[x+xOff][y+yOff] == pointNotFound) {

                  pointCheck[x+xOff][y+yOff] = pointShouldSubGridCheck;

                  // We need to do a searchso we need another pass
                  bGridCleared = false;
                }
              }
            }
          }
        }
      }

    }
    while(!bGridCleared);

    return points;
  }

  /**
   * This method is used to search for a valid point, on the polygon, within the square
   *    whose center is defined by the centerX, centerY arguments and size is given by p_Xspacing
   *    and p_Yspacing. The precision parameter determines how many points are checked. If precision
   *    is zero, then only the center is checked. In other cases, the pattern looks like this:
   * 0:                     ___________________________
   *                       |        2   1   2          |
   *                       |    2       2       2      |
   *                       | 2  1   2   0   2   1   2  |
   *                       |    2       2       2      |
   *                       |        2   1   2          |
   *                       |___________________________|
   *  Where the numbers represent the precision at which the point will be found.
   *
   * @param xymp The multipolygon we're testing for points
   * @param centerX The X position of the center of the polygon
   * @param centerY The Y position of the center of the polygon
   * @param precision See description
   *
   * @return geos::Point* Found point, NULL if nothing found
   */
  geos::geom::Point *GridPolygonSeeder::CheckSubGrid(const geos::geom::MultiPolygon &xymp, const double &centerX,
      const double &centerY, const int &precision) {
    // We'll make a 2D array detailing which points to check, and which not to, in this rectangle.
    // Figure out how many points across and vertically we need to check
    int gridSize = 1;
    for(int prec = 0; prec < precision  &&  prec < 6; prec ++) {
      // Maybe solve the recurrence relation for a single equation??
      gridSize = gridSize * 2 + 1;
    }

    // These are the possible values in which the 2D array can be. We need the transition value
    //   gridNewCheckPt to not count new points as old points.
    enum GridPoint {
      gridEmpty,
      gridNewCheckPt,
      gridCheckPt
    };

    GridPoint grid[gridSize][gridSize];

    for(int y = 0; y < gridSize; y++) {
      for(int x = 0; x < gridSize; x++) {
        grid[x][y] = gridEmpty;
      }
    }

    // Precision 0: Always center, this is always true
    grid[gridSize/2][gridSize/2] = gridCheckPt;

    // now populate the grid with what we wish to check for
    for(int prec = 0; prec < precision; prec ++) {
      // This tells us how far over in the 2D array to go at a precision from already found points
      int checkDist = (gridSize + 1) / (int)(4 * (pow(2.0, prec)) + 0.5);

      // Search the grid for already found points and set everything checkDist away to be checked too
      for(int y = 0; y < gridSize; y++) {
        for(int x = 0; x < gridSize; x++) {
          if(grid[x][y] == gridCheckPt) {
            // We should never overwrite found points, the checkDist should assure this wont happen
            if(x - checkDist > 0) grid[x-checkDist][y] = gridNewCheckPt;
            if(y - checkDist > 0) grid[x][y-checkDist] = gridNewCheckPt;
            if(x + checkDist < gridSize) grid[x+checkDist][y] = gridNewCheckPt;
            if(y + checkDist < gridSize) grid[x][y+checkDist] = gridNewCheckPt;
          }
        }
      }

      // Convert temporary check pt to final.
      // Do this separately to avoid changing the data we're processing
      for(int y = 0; y < gridSize; y++) {
        for(int x = 0; x < gridSize; x++) {
          if(grid[x][y] == gridNewCheckPt) grid[x][y] = gridCheckPt;
        }
      }
    }

    // We now have a grid of points to check inside this grid square. Figure out the
    //   distance each of these subsquare pixels are worth.
    double deltaXSize = p_Xspacing / (gridSize + 1);
    double deltaYSize = p_Yspacing / (gridSize + 1);

    geos::geom::Point *result = NULL;
    // Now loop through this grid, checking each point that needs checked, return the first valid pt
    for(int y = 0; !result && y < gridSize; y++) {
      for(int x = 0; !result && x < gridSize; x++) {
        if(grid[x][y] != gridCheckPt) continue;

        double xPos = centerX + (x - gridSize / 2) * deltaXSize;
        double yPos = centerY + (y - gridSize / 2) * deltaYSize;
        geos::geom::Coordinate c(xPos, yPos);
        geos::geom::Point *p = Isis::globalFactory->createPoint(c).release();
        if(p->within(&xymp)) {
          result = p->clone().release();
        }
        else {
          delete p;
        }
      }
    }

    return result;
  }

  /**
   * @brief Parse the GridPolygonSeeder spicific parameters from the PVL
   *
   * @param pvl The PVL object containing the control parameters for this
   * polygon seeder.
   */
  void GridPolygonSeeder::Parse(Pvl &pvl) {
    // Call the parents Parse method
    PolygonSeeder::Parse(pvl);

    // Pull parameters specific to this algorithm out
    try {
      // Get info from Algorithm group
      PvlGroup &algo = pvl.findGroup("PolygonSeederAlgorithm", Pvl::Traverse);
      PvlGroup &invalgo = invalidInput->findGroup("PolygonSeederAlgorithm",
                          Pvl::Traverse);

      // Set the spacing
      p_Xspacing = 0.0;
      if(algo.hasKeyword("XSpacing")) {
        p_Xspacing = (double) algo["XSpacing"];
        if(invalgo.hasKeyword("XSpacing")) {
          invalgo.deleteKeyword("XSpacing");
        }
      }
      else {
        std::string msg = "PVL for GridPolygonSeeder must contain [XSpacing] in [" + pvl.fileName() + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      p_Yspacing = 0.0;
      if(algo.hasKeyword("YSpacing")) {
        p_Yspacing = (double) algo["YSpacing"];
        if(invalgo.hasKeyword("YSpacing")) {
          invalgo.deleteKeyword("YSpacing");
        }
      }
      else {
        std::string msg = "PVL for GridPolygonSeeder must contain [YSpacing] in [" + pvl.fileName() + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      p_subGrid = false;
      if(algo.hasKeyword("SubGrid")) {
        p_subGrid = IString((std::string)algo["SubGrid"]).UpCase() != "FALSE";
        if(invalgo.hasKeyword("SubGrid")) {
          invalgo.deleteKeyword("SubGrid");
        }
      }
    }
    catch(IException &e) {
      std::string msg = "Improper format for PolygonSeeder PVL [" + pvl.fileName() + "]";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }

    if(p_Xspacing <= 0.0) {
      IString msg = "X Spacing must be greater that 0.0 [(" + IString(p_Xspacing) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    if(p_Yspacing <= 0.0) {
      IString msg = "Y Spacing must be greater that 0.0 [(" + IString(p_Yspacing) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  PvlGroup GridPolygonSeeder::PluginParameters(QString grpName) {
    PvlGroup pluginInfo(grpName.toStdString());

    PvlKeyword name("Name", Algorithm().toStdString());
    PvlKeyword minThickness("MinimumThickness", std::to_string(MinimumThickness()));
    PvlKeyword minArea("MinimumArea", std::to_string(MinimumArea()));
    PvlKeyword xSpac("XSpacing", std::to_string(p_Xspacing));
    PvlKeyword ySpac("YSpacing", std::to_string(p_Yspacing));
    PvlKeyword subGrid("SubGrid", std::to_string(p_subGrid));

    pluginInfo.addKeyword(name);
    pluginInfo.addKeyword(minThickness);
    pluginInfo.addKeyword(minArea);
    pluginInfo.addKeyword(xSpac);
    pluginInfo.addKeyword(ySpac);
    pluginInfo.addKeyword(subGrid);

    return pluginInfo;
  }

}; // End of namespace Isis


/**
 * @brief Create a GridPolygonSeeder object
 *
 * Used to create a GridPolygonSeeder object from a PolygonSeeder plugin PVL
 * file.
 *
 * @param pvl The Pvl object that describes how the new object should be
 *           initialized.
 *
 * @return A pointer to the new object
 */
extern "C" Isis::PolygonSeeder *GridPolygonSeederPlugin(Isis::Pvl &pvl) {
  return new Isis::GridPolygonSeeder(pvl);
}

