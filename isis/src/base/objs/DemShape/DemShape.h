#ifndef DemShape_h
#define DemShape_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ShapeModel.h"

template<class T> class QVector;

namespace Isis {
  class Cube;
  class Interpolator;
  class Portal;
  class Projection;

  /**
   * @brief Define shapes and provide utilities for targets stored as ISIS maps
   *
   * This class will define shapes of ISIS target bodies with the shape defined by an ISIS map
   * fille (level 2 image), as well as provide utilities to retrieve radii and photometric
   * information for the intersection point.
   *
   * @author 2010-07-30 Debbie A. Cook
   *
   * @internal
   *   @history 2010-07-30 - Debbie A. Cook - Original version.
   *   @history 2012-10-25 - Jeannie Backer - Changed call to Spice::resolution() method
   *                             to lower camel case. Added documentation. Ordered includes.
   *                             References #1181.
   *   @history 2015-04-30 Jeannie Backer - Added isDEM() method. References #2243.
   *   @history 2015-07-21 Kristin Berry - Added NaifStatus::CheckErrors() to see if any NAIF errors
   *                             were signaled. References #2248.
   *   @history 2015-10-01 Jeannie Backer - Made improvements to documentation and brought code
   *                           closer to ISIS coding standards. References #1438
   *   @history 2016-06-13 Kelvin Rodriguez - Removed redundant contructor PlaneShape(Target, Pvl).
   *                           References #2214
   *   @history 2017-05-19 Tyler Wilson - calculateDefaultNormal() and calculateSurfaceNormal()
   *                           now return the normal vector to an ellipsoid.  All references
   *                           to ShapeModel::calculateEllipsoidalSurfaceNormal have been
   *                           removed.  References #1028.
   *   @history 2017-05-19 Christopher Combs - Modified unitTest.cpp: Removed path of output in
   *                           testDemCube() to allow the test to pass when not using the standard
   *                           data areas. Fixes #4738.
   *   @history 2017-06-07 Kristin Berry - Added a using declaration so that the new
   *                            intersectSurface methods in ShapeModel are accessible by DemShape.
   *
   */
  class DemShape : public ShapeModel {
    public:
      // Constructor
      DemShape(Target *target, Pvl &pvl);

      // Constructor
      DemShape();

      // Destructor
      ~DemShape();

      // Make parent functions visible
      using Isis::ShapeModel::intersectSurface;

      // Intersect the shape model
      bool intersectSurface(std::vector<double> observerPos,
                            std::vector<double> lookDirection);

      Distance localRadius(const Latitude &lat, const Longitude &lon);

      // Return dem scale in pixels/degree
      double demScale();

      // Calculate the default normal of the current intersection point
      virtual void calculateDefaultNormal();

      // implement pure virtual method from ShapeModel class
      bool isDEM() const;

      // To compute the surface normal, you must call setLocalAreaPoint on top,
      // bottom, left, and right surrounding points in the image.  Then call
      // calculateSurfaceNormal and directSurfaceNormal to calculate the normal.
      // Use removeLOcalAreaPoints to clean up as needed.  See Camera for an
      // example, or use its GetLocalNormal method.

      // Calculate the surface normal of the current intersection point
      void calculateLocalNormal(QVector<double *> cornerNeighborPoints);
      void calculateSurfaceNormal();

    protected:
      Cube *demCube();         //!< Returns the cube defining the shape model.

    private:
    
      // Given a position along a ray, compute the difference between the 
      // radius at that position and the surface radius at that lon-lat location.
      // All lengths are in km.
      double demError(std::vector<double> const& observerPos,
                      std::vector<double> const& lookDirection, 
                      double t, 
                      double * intersectionPoint,
                      bool & success);
    
      // Find a value in the DEM. Used when intersecting a ray with the DEM.
      // Returned value is in km. 
      double findDemValue();
      
      Cube *m_demCube;        //!< The cube containing the model
      Projection *m_demProj;  //!< The projection of the model
      double m_pixPerDegree;  //!< Scale of DEM file in pixels per degree
      Portal *m_portal;       //!< Buffer used to read from the model
      Interpolator *m_interp; //!< Use bilinear interpolation from dem
      double m_demValue;      //!< A value picked from the dem
      bool m_demValueFound;   //!< True if it was attempted to find a value in the DEM
  };
}

#endif
