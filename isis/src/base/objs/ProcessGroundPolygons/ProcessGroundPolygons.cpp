/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "ProcessGroundPolygons.h"

#include <vector>

#include "geos/geom/CoordinateSequence.h"
#include "geos/geom/LineString.h"
#include "geos/io/WKTWriter.h"

#include "Application.h"
#include "BoxcarCachingAlgorithm.h"
#include "IException.h"
#include "PolygonTools.h"
#include "Projection.h"

using namespace std;
namespace Isis {

  ProcessGroundPolygons::ProcessGroundPolygons() {
    p_groundMap = NULL;
  }


  /**
   * This method gets called from the application with the lat/lon
   * vertices of a polygon along with a vector of values.
   * The location of the values with in the verctor indicate which
   * band that value gets written to
   *
   * @param lat
   * @param lon
   * @param values
   */
  void ProcessGroundPolygons::Rasterize(std::vector<double> &lat,
                                        std::vector<double> &lon,
                                        std::vector<double> &values) {

    // Decide if we need to split the poly on the 360 boundry
    // Yes, we can do this better. The lat and lon vectors should be passed in as polygons, but
    // for now this is reusing older code.
    bool crosses = false;
    for (unsigned int i = 0; i < lon.size() - 1; i++) {
      if (fabs(lon[i] - lon[i+1]) > 180.0) {
        crosses = true;
        break;
      }
    }

    if (crosses) {
      // Make a polygon from the lat/lon vectors and split it on 360
      geos::geom::CoordinateSequence pts;
      for (unsigned int i = 0; i < lat.size(); i++) {
        pts.add(geos::geom::Coordinate(lon[i], lat[i]));
      }
      pts.add(geos::geom::Coordinate(lon[0], lat[0]));

      geos::geom::Polygon *crossingPoly = Isis::globalFactory->createPolygon(
          globalFactory->createLinearRing(pts)).release();

      geos::geom::MultiPolygon *splitPoly = NULL;
      try {
        splitPoly = PolygonTools::SplitPolygonOn360(crossingPoly);
      }
      // Ignore any pixel footprints that could not be split. This should only be pixels
      // that contain the pole.
      catch (IException &) {
        // See leading comment
      }

      delete crossingPoly;

      if (splitPoly != NULL) {
        // Process the polygons in the split multipolygon as if we were still using the lat/lon vectors
        for (unsigned int g = 0; g < splitPoly->getNumGeometries(); ++g) {
          const geos::geom::Polygon *poly =
              dynamic_cast<const geos::geom::Polygon *>(splitPoly->getGeometryN(g));

          geos::geom::CoordinateSequence *llcoords =
              poly->getExteriorRing()->getCoordinates().release();

          // Move each coordinate in the exterior ring of this lat/lon polygon to vectors
          // Ignore any holes in the polygon
          std::vector<double> tlat;
          std::vector<double> tlon;
          for (unsigned int cord = 0; cord < llcoords->getSize() - 1; ++cord) {
            tlon.push_back(llcoords->getAt(cord).x);
            tlat.push_back(llcoords->getAt(cord).y);
          }

          Convert(tlat, tlon);
          ProcessPolygons::Rasterize(p_samples, p_lines, values);
        }
        delete splitPoly;
      }
    }
    else {
      Convert(lat, lon);
      ProcessPolygons::Rasterize(p_samples, p_lines, values);
    }
  }




/**
 * This method gets called from the application with the lat/lon
 * vertices of a polygon.
 *
 * it returns a geos::geom::Geometry* representation of the 
 * geometry
 *
 * @param lat
 * @param lon
 * 
 */
geos::geom::Geometry* ProcessGroundPolygons::Vectorize(std::vector<double> &lat,
                                         std::vector<double> &lon)
                                         //std::vector<double> &values) 
										 {

											 
  
  // Decide if we need to split the poly on the 360 boundry
  // Yes, we can do this better. The lat and lon vectors should be passed in as polygons, but
  // for now this is reusing older code.
  bool crosses = false;
  for (unsigned int i = 0; i < lon.size() - 1; i++) {
    if (fabs(lon[i] - lon[i+1]) > 180.0) {
      crosses = true;
      break;
    }
  }


  if (crosses) {
    // Make a polygon from the lat/lon vectors and split it on 360
    geos::geom::CoordinateSequence pts;
    for (unsigned int i = 0; i < lat.size(); i++) {
      pts.add(geos::geom::Coordinate(lon[i], lat[i]));
    }
    pts.add(geos::geom::Coordinate(lon[0], lat[0]));

    geos::geom::Polygon *crossingPoly = Isis::globalFactory->createPolygon(
        globalFactory->createLinearRing(pts)).release();

    geos::geom::MultiPolygon *splitPoly = NULL;
    try {
      splitPoly = PolygonTools::SplitPolygonOn360(crossingPoly);
    }
    // Ignore any pixel footprints that could not be split. This should only be pixels
    // that contain the pole.
    catch (IException &) {
      // See leading comment
    }

    delete crossingPoly;

    if (splitPoly != NULL) {
      // Process the polygons in the split multipolygon as if we were still using the lat/lon vectors
      for (unsigned int g = 0; g < splitPoly->getNumGeometries(); ++g) {
        const geos::geom::Polygon *poly =
            dynamic_cast<const geos::geom::Polygon *>(splitPoly->getGeometryN(g));

        geos::geom::CoordinateSequence *llcoords =
            poly->getExteriorRing()->getCoordinates().release();

        // Move each coordinate in the exterior ring of this lat/lon polygon to vectors
        // Ignore any holes in the polygon
        std::vector<double> tlat;
        std::vector<double> tlon;
        for (unsigned int cord = 0; cord < llcoords->getSize() - 1; ++cord) {
          tlon.push_back(llcoords->getAt(cord).x);
          tlat.push_back(llcoords->getAt(cord).y);
        }

      }
      return splitPoly;
    }
  }
  else { // if does not crosses

    // Make a polygon from the lat/lon vectors and split it on 360
    geos::geom::CoordinateSequence pts;
    for (unsigned int i = 0; i < lat.size(); i++) {
      pts.add(geos::geom::Coordinate(lon[i], lat[i]));
    }
    pts.add(geos::geom::Coordinate(lon[0], lat[0]));

    geos::geom::Polygon *singlePoly = Isis::globalFactory->createPolygon(
        globalFactory->createLinearRing(pts)).release();

    return singlePoly;
  }
  return nullptr;
}

  /**
   * This method gets called from the application with the lat/lon
   * verticies of a polygon along with the band number and the
   * value for the polygon.
   *
   * @param lat
   * @param lon
   * @param band
   * @param value
   */
  void ProcessGroundPolygons::Rasterize(std::vector<double> &lat,
                                        std::vector<double> &lon,
                                        int &band, double &value) {

    Convert(lat, lon);
    ProcessPolygons::Rasterize(p_samples, p_lines, band, value);
  }


  /**
   * Converts lat/long to line/sample using the universal ground
   * map object.
   *
   * @param lat
   * @param lon
   */
  void ProcessGroundPolygons::Convert(std::vector<double> &lat,
                                      std::vector<double> &lon) {

    p_samples.clear();
    p_lines.clear();

    for (unsigned int i = 0; i < lat.size(); i++) {
      if (p_groundMap->SetUniversalGround(lat[i], lon[i])) {
        p_samples.push_back(p_groundMap->Sample());
        p_lines.push_back(p_groundMap->Line());
      }
    }
  }


  /**
   * This method cleans up any open outputcube files and deletes
   * the pointer to the universal ground map if there was one
   * created.
   *
   * @deprecated Please use Finalize()
   */
  void ProcessGroundPolygons::EndProcess() {

    if(p_groundMap != NULL) {
      delete p_groundMap;
    }

    ProcessPolygons::EndProcess();
  }


  /**
   * This method cleans up any open outputcube files and deletes
   * the pointer to the universal ground map if there was one
   * created.
   *
   */
  void ProcessGroundPolygons::Finalize() {

    if(p_groundMap != NULL) {
      delete p_groundMap;
    }

    ProcessPolygons::Finalize();
  }


  /**
   * This gives the option to append to the cube
   *
   * @param cube
   * @param avgFileName
   * @param countFileName
   */
  void ProcessGroundPolygons::AppendOutputCube(QString &cubeStr,
      const QString &avgFileName,
      const QString &countFileName) {
    /*We need a ground map for converting lat/long to line/sample  see Convert()*/
    Cube cube(cubeStr, "r");
    p_groundMap = new UniversalGroundMap(cube);
    ProcessPolygons::AppendOutputCube(avgFileName, countFileName);

  }


  /**
   * This method creates two cubes and creates a universal ground
   * map using the pvl information of the 'cube of interest'
   *
   * @param avgFileName
   * @param countFileName
   * @param outAtts
   * @param cube
   */
  void ProcessGroundPolygons::SetStatCubes(const QString &avgFileName,
      const QString &countFileName,
      Isis::CubeAttributeOutput &outAtts,
      QString &cubeStr) {
    /*We need a ground map for converting lat/long to line/sample  see Convert()*/
    Cube cube(cubeStr);
    p_groundMap = new UniversalGroundMap(cube);

    /*setup input cube to transfer projection or camera labels*/
    CubeAttributeInput inAtts;
    Isis::Process::SetInputCube(cubeStr, inAtts, 0);
    int nBands = this->InputCubes[0]->bandCount();
    int nLines = this->InputCubes[0]->lineCount();
    int nSamples = this->InputCubes[0]->sampleCount();

    this->Process::SetOutputCube(avgFileName, outAtts, nSamples, nLines, nBands);
    this->Process::SetOutputCube(countFileName, outAtts, nSamples, nLines, nBands);

    OutputCubes[0]->addCachingAlgorithm(new BoxcarCachingAlgorithm());
    OutputCubes[1]->addCachingAlgorithm(new BoxcarCachingAlgorithm());

    ClearInputCubes();

  }


  /**
   * This is a method that is called directly from the
   * application.  Using the "TO" parameter we also create a
   * count cube name.  Then we call the overloaded SetStatCubes
   * method above.
   *
   * @param parameter
   * @param cube
   */
  void ProcessGroundPolygons::SetStatCubes(const QString &parameter,
      QString &cube) {

    QString avgString =
      Application::GetUserInterface().GetCubeName(parameter);
    CubeAttributeOutput atts =
      Application::GetUserInterface().GetOutputAttribute(parameter);

    FileName file(avgString);
    QString path = file.path();
    QString filename = file.baseName();
    QString countString = path + "/" + filename + "-count-";

    SetStatCubes(avgString, countString, atts, cube);

  }


  /**
   *
   *
   * @param parameter
   * @param map
   * @param bands
   */
  void ProcessGroundPolygons::SetStatCubes(const QString &parameter,
      Isis::Pvl &map, int bands) {

    QString avgString =
      Application::GetUserInterface().GetCubeName(parameter);
    CubeAttributeOutput atts =
      Application::GetUserInterface().GetOutputAttribute(parameter);

    FileName file(avgString);
    QString path = file.path();
    QString filename = file.baseName();
    QString countString = path + "/" + filename + "-count-";

    SetStatCubes(avgString, countString, atts, map, bands);

  }


  /**
   *
   *
   * @param avgFileName
   * @param countFileName
   * @param atts
   * @param map
   * @param bands
   *
   * @internal
   *   @history 2016-11-30 Ian Humphrey - The passed PVL is now checked for the AlphaCube group.
   *                           If it exists, the AlphaCube group is attached to the output cubes.
   *                           References #4433.
   */
  void ProcessGroundPolygons::SetStatCubes(const QString &avgFileName,
      const QString &countFileName,
      Isis::CubeAttributeOutput &atts,
      Isis::Pvl &map, int bands) {
    int samples, lines;

    Projection *proj = ProjectionFactory::CreateForCube(map, samples, lines,
                       false);

    this->ProcessPolygons::SetStatCubes(avgFileName, countFileName, atts,
                                         samples, lines, bands);

    OutputCubes[0]->addCachingAlgorithm(new BoxcarCachingAlgorithm());
    OutputCubes[1]->addCachingAlgorithm(new BoxcarCachingAlgorithm());

    /*Write the pvl group to the cube files.*/

    PvlGroup group = map.findGroup("Mapping", Pvl::Traverse);

    OutputCubes[0]->putGroup(group);
    OutputCubes[1]->putGroup(group);

    // If there is an alpha cube in the label passed, attach to output cubes
    if (map.hasGroup("AlphaCube")) {
      PvlGroup alpha = map.findGroup("AlphaCube", Pvl::Traverse);
      OutputCubes[0]->putGroup(alpha);
      OutputCubes[1]->putGroup(alpha);
    }

    /*We need a ground map for converting lat/long to line/sample  see Convert()*/
    p_groundMap = new UniversalGroundMap(*OutputCubes[0]);

    delete proj;
  }

} /* end namespace isis*/

