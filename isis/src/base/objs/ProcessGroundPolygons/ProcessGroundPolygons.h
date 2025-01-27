/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#ifndef ProcessGroundPolygons_h
#define ProcessGroundPolygons_h

#include "ProjectionFactory.h"
#include "Process.h"
#include "Brick.h"
#include "FileName.h"
#include "ProcessPolygons.h"
#include "UniversalGroundMap.h"

namespace Isis {
  /**
   * @brief Process cube polygons to map or camera projections
   *
   * This class allows a programmer to develop a program which
   * @ingroup HighLevelCubeIO
   *
   * @author  2008-12-14 Stacy Alley
   *
   * @internal
   *   @history 2008-05-12 Steven Lambright - Removed references to CubeInfo
   *   @history 2008-08-18 Steven Lambright - Updated to work with geos3.0.0
   *                           instead of geos2. Mostly namespace changes.
   *   @history 2012-02-24 Steven Lambright - Added Finalize()
   *   @history 2013-03-27 Jeannie Backer - Added programmer comments.
   *                           References #1248.
   *   @history 2013-09-10 Stuart Sides, Tracie Sucharski - Cleaned up and removed extraneous code.
   *                           References #1604.
   *   @history 2015-01-19 Sasha Brownsberger - Changed name of SetOutputCube function to SetStatCubes 
   *                                            function to better reflect the command functionality 
   *                                            and to avoid conflicts with Process::SetOutputCube 
   *                                            virtual function.  References #2215.
   *   @history 2016-11-30 Ian Humphrey - Modified SetStatCubes(5args) to attach the AlphaCube
   *                           group to the output cubes if it exists in the passed PVL.
   *                           References #4433.
   */
  class ProcessGroundPolygons : public ProcessPolygons {
    public:
      ProcessGroundPolygons();

      // SetOutputCube() is not virtual in the Process class nor in the
      // ProcessPolygons class, so the following definitions for this method
      // are the only ones that are allowed for ProcessGroundPolygon objects
      // and child objects

      //Cube is an existing camera cube or projection cube
      void SetStatCubes(const QString &parameter, QString &cube);

      //Determine cube size from the projection map
      void SetStatCubes(const QString &parameter, Isis::Pvl &map, int bands);

      void SetStatCubes(const QString &avgFileName, const QString
                         &countFileName, Isis::CubeAttributeOutput &atts,
                         QString &cube);

      void SetStatCubes(const QString &avgFileName, const QString
                         &countFileName, Isis::CubeAttributeOutput &atts,
                         Isis::Pvl &map, int bands);

      void AppendOutputCube(QString &cube, const QString &avgFileName,
                            const QString &countFileName = "");

      void Rasterize(std::vector<double> &lat,
                     std::vector<double> &lon,
                     std::vector<double> &values);

      void Rasterize(std::vector<double> &lat,
                     std::vector<double> &lon,
                     int &band, double &value);
 
      geos::geom::Geometry* Vectorize   (std::vector<double> &lat,
                                         std::vector<double> &lon);
                                         //std::vector<double> &values);

      void EndProcess();
      void Finalize();
      UniversalGroundMap *GetUniversalGroundMap() {
        return p_groundMap;
      };

    private:
      void Convert(std::vector<double> &lat, std::vector<double> &lon);
      UniversalGroundMap *p_groundMap;
      std::vector<double> p_samples, p_lines;

  };

};

#endif
