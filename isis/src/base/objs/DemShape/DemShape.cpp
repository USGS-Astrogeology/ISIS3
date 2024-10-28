/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "DemShape.h"

// Qt third party includes
#include <QDebug>
#include <QVector>

// c standard library third party includes
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <iomanip>
#include <string>
#include <vector>

// naif third party includes
#include <SpiceUsr.h>
#include <SpiceZfc.h>
#include <SpiceZmc.h>

#include "Cube.h"
#include "CubeManager.h"
#include "Distance.h"
#include "EllipsoidShape.h"
//#include "Geometry3D.h"
#include "IException.h"
#include "Interpolator.h"
#include "Latitude.h"
//#include "LinearAlgebra.h"
#include "Longitude.h"
#include "NaifStatus.h"
#include "Portal.h"
#include "Projection.h"
#include "Pvl.h"
#include "Spice.h"
#include "SurfacePoint.h"
#include "Table.h"
#include "Target.h"
#include "UniqueIOCachingAlgorithm.h"

using namespace std;

namespace Isis {
  /**
   * Construct a DemShape object. This method creates a ShapeModel object named
   * "DemShape". The member variables are set to Null.
   *
   */
  DemShape::DemShape() : ShapeModel() {
    setName("DemShape");
    m_demProj = NULL;
    m_demCube = NULL;
    m_interp = NULL;
    m_portal = NULL;
    m_demValueFound = false;
    m_demValue = -std::numeric_limits<double>::max();
  }


  /**
   * Construct a DemShape object. This method creates a ShapeModel object
   * named "DemShape" and initializes member variables from the projection
   * shape model using the given Target and Pvl.
   *
   * @param target Pointer to a valid target.
   * @param pvl Valid ISIS cube label.
   */
  DemShape::DemShape(Target *target, Pvl &pvl) : ShapeModel(target) {
    setName("DemShape");
    m_demProj = NULL;
    m_demCube = NULL;
    m_interp = NULL;
    m_portal = NULL;
    m_demValueFound = false;
    m_demValue = -std::numeric_limits<double>::max();

    PvlGroup &kernels = pvl.findGroup("Kernels", Pvl::Traverse);

    QString demCubeFile;
    if (kernels.hasKeyword("ElevationModel")) {
      demCubeFile = (QString) kernels["ElevationModel"];
    }
    else if(kernels.hasKeyword("ShapeModel")) {
      demCubeFile = (QString) kernels["ShapeModel"];
    }

    m_demCube = CubeManager::Open(demCubeFile);

    // This caching algorithm works much better for DEMs than the default,
    //   regional. This is because the uniqueIOCachingAlgorithm keeps track
    //   of a history, which for something that isn't linearly processing a
    //   cube is worth it. The regional caching algorithm tosses out results
    //   from iteration 1 of setlookdirection (first algorithm) at iteration
    //   4 and the next setimage has to re-read the data.
    m_demCube->addCachingAlgorithm(new UniqueIOCachingAlgorithm(5));
    m_demProj = m_demCube->projection();
    m_interp = new Interpolator(Interpolator::BiLinearType);
    m_portal = new Portal(m_interp->Samples(), m_interp->Lines(),
                          m_demCube->pixelType(),
                          m_interp->HotSample(), m_interp->HotLine());

    // Read in the Scale of the DEM file in pixels/degree
    const PvlGroup &mapgrp = m_demCube->label()->findGroup("Mapping", Pvl::Traverse);

    // Save map scale in pixels per degree
    m_pixPerDegree = (double) mapgrp["Scale"];
  }


  //! Destroys the DemShape
  DemShape::~DemShape() {
    m_demProj = NULL;

    // We do not have ownership of p_demCube
    m_demCube = NULL;

    delete m_interp;
    m_interp = NULL;

    delete m_portal;
    m_portal = NULL;
  }

  /**
     Given a position along a ray, compute the difference between the 
     radius at that position and the surface radius at that lon-lat location.
     All lengths are in km.
   * @param observerPos Observer position
   * @param lookDirection Look direction
   * @param t parameter measuring location on the ray
   * @param intersectionPoint location along the ray, eventual intersection location
   * @param success True if the calculation was successful
   * @return @d double Signed error, if the calculation was successful
   **/
  double DemShape::demError(vector<double> const& observerPos,
                            vector<double> const& lookDirection, 
                            double t, 
                            double * intersectionPoint,
                            bool & success) {
  
    // Initialize the return value
    success = false;
    
    // Compute the position along the ray
    for (size_t i = 0; i < 3; i++)
      intersectionPoint[i] = observerPos[i] + t * lookDirection[i];

    double pointRadiusKm = sqrt(intersectionPoint[0]*intersectionPoint[0] +
                                intersectionPoint[1]*intersectionPoint[1] +
                                intersectionPoint[2]*intersectionPoint[2]);
          
    // The lat/lon calculations are done here by hand for speed & efficiency.
    // With doing it in the SurfacePoint class using p_surfacePoint, there
    // is a 24% slowdown (which is significant in this very tightly looped call).
    double norm2 = intersectionPoint[0] * intersectionPoint[0] +
        intersectionPoint[1] * intersectionPoint[1];
    double latDD = atan2(intersectionPoint[2], sqrt(norm2)) * RAD2DEG;
    double lonDD = atan2(intersectionPoint[1], intersectionPoint[0]) * RAD2DEG;
    if (lonDD < 0) {
      lonDD += 360;
    }
    
    // Previous Sensor version used local version of this method with lat and lon doubles.
    // Steven made the change to improve speed.  He said the difference was negligible.
    Distance surfaceRadiusKm = localRadius(Latitude(latDD, Angle::Degrees),
                                           Longitude(lonDD, Angle::Degrees));
    
    if (Isis::IsSpecial(surfaceRadiusKm.kilometers())) {
      setHasIntersection(false);
      success = false;
      return -1; // return something
    }
    
    // Must set these to be able to compute resolution later
    surfaceIntersection()->FromNaifArray(intersectionPoint);
    setHasIntersection(true);
    
    success = true;
    return pointRadiusKm - surfaceRadiusKm.kilometers();
  } 
  
  /**
   * Find the intersection point with the DEM. Start by intersecting
   * with a nearby horizontal surface, then refine using the secant method.
   * This was validated to work with ground-level sensors. Likely can
     do well with images containing a limb.
   *
   * @param observerPos
   * @param lookDirection
   *
   * @return @b bool Indicates whether the intersection was found.
   */
  bool DemShape::intersectSurface(vector<double> observerPos,
                                  vector<double> lookDirection) {
  
    
    // Find norm of observerPos
    double positionNormKm = 0.0;
    for (size_t i = 0; i < observerPos.size(); i++)
      positionNormKm += observerPos[i]*observerPos[i];
    positionNormKm = sqrt(positionNormKm);

    // in each iteration, the current surface intersect point is saved for
    // comparison with the new, updated surface intersect point
    SpiceDouble newIntersectPt[3];

    // An estimate for the radius of points in the DEM. Ensure the radius is
    // strictly below the position, so that surfpt_c does not fail.
    double r = findDemValue();
    r = std::min(r, positionNormKm - 0.0001);
    
    // Try to intersect the target body ellipsoid at given radius as a first
    // approximation.
    bool status;
    surfpt_c((SpiceDouble *) &observerPos[0], &lookDirection[0], r, r, r, newIntersectPt,
               (SpiceBoolean*) &status);
  
    if (!status) { 
        // If no luck, start at the observer, and will try points along the ray.
        for (size_t i = 0; i < 3; i++)
          newIntersectPt[i] = observerPos[i];           
    }
    
    // Ensure the intersection point is set 
    surfaceIntersection()->FromNaifArray(newIntersectPt);
    setHasIntersection(true);
    
    // Find the current position along the ray, relative to the observer
    // Equation: newIntersectPt = observerPos + t * lookDirection
    double t0 = ((newIntersectPt[0] - observerPos[0]) * lookDirection[0] +
                 (newIntersectPt[1] - observerPos[1]) * lookDirection[1] +
                 (newIntersectPt[2] - observerPos[2]) * lookDirection[2]) 
                 / (lookDirection[0] * lookDirection[0] +
                    lookDirection[1] * lookDirection[1] +
                    lookDirection[2] * lookDirection[2]);
                 
    bool success = false;
    double intersectionPoint[3];
    
    // Initial guess. If no luck, wiggle it around.
    double f0 = demError(observerPos, lookDirection, t0, intersectionPoint, success); 
    if (!success) {
      std::vector<double> delta = {1.0, 0.1, 10.0, 100.0, 1000.0, 5000.0, 10000.0};
      for (size_t i = 0; i < delta.size(); i++) {
        double try_t = t0 + delta[i] / 1000.0; // convert to km
        f0 = demError(observerPos, lookDirection, try_t, intersectionPoint, success);
        if (success) {
          t0 = try_t;
          break;
        }
      }
    }
    if (!success) {
      setHasIntersection(false);
      return false;
    }
    
    // Form the next guess (secant method needs two guesses). Try to add this
    // many meters to the current guess.
    std::vector<double> delta = {1.0, 0.1, 10.0, 0.01, 100.0};
    double t1 = 0, f1 = 0;
    success = false;
    for (size_t i = 0; i < delta.size(); i++) {
      t1 = t0 + delta[i] / 1000.0; // convert to km
      f1 = demError(observerPos, lookDirection, t1, intersectionPoint, success);
      if (f1 == f0)
        continue; // equal values are not a good thing for the secant method
      if (success) 
        break;
    }
    if (!success) {
      setHasIntersection(false);
      return false;
    }

    // Secant method with at most 15 iterations. This method converges fast. 
    // If it does not converge in this many iterations, it never will. 
    bool converged = false;
    // Use 1/1000 of a pixel as tolerance. Otherwise the results may be not 
    // accurate enough for ground-level sensors with oblique views.
    double tol = resolution()/1000;  
    for (int i = 1; i <= 15; i++) {
      
      if (std::abs(f1) * 1000.0 < tol) {
        
        // Recompute tolerance at updated surface point and recheck
        surfaceIntersection()->FromNaifArray(intersectionPoint);
        tol = resolution() / 100.0;

        if (std::abs(f1) * 1000.0 < tol) {
          converged = true;
          setHasIntersection(true);
          break;
        }
      }
      
      // If the function values are large but are equal, there is nothing we can
      // do
      if (f1 == f0 && std::abs(f1) * 1000.0 >= tol) {
        converged = false;
        break;
      }
      
      // Secant method iteration
      double t2 = t1 - f1 * (t1 - t0) / (f1 - f0);
      double f2 = demError(observerPos, lookDirection, t2, intersectionPoint, success);
      
      if (!success) {
        converged = false;
        setHasIntersection(false);
        break;
      }
      
      // Update
      t0 = t1; f0 = f1;
      t1 = t2; f1 = f2;
    }

    NaifStatus::CheckErrors();
    
    return converged;
  }


  /**
   * Find a value in the DEM. Used when intersecting a ray with the DEM. 
   * Returned value is in km. A more robust method would return a couple
     of values, and later try one if the other one fails.
   */
  double DemShape::findDemValue() {
    
    if (m_demValueFound) 
      return m_demValue;
    
    int numSamples = m_demCube->sampleCount();
    int numLines = m_demCube->lineCount();
    
    // Try to pick about 25 samples not too close to the boundary. Stop at the
    // first successful one.
    int num = 5;
    int sampleSpacing = std::max(numSamples / (num + 1), 1);
    int lineSpacing = std::max(numLines / (num + 1), 1);

    // iterate as sample from sampleSpacing to numSamples-sampleSpacing
    for (int s = sampleSpacing; s <= numSamples - sampleSpacing; s += sampleSpacing) {
      for (int l = lineSpacing; l <= numLines - lineSpacing; l += lineSpacing) {

        m_portal->SetPosition(s, l, 1);
        m_demCube->read(*m_portal);
        if (!Isis::IsSpecial(m_portal->DoubleBuffer()[0])) {
          m_demValue = m_portal->DoubleBuffer()[0] / 1000.0;
          m_demValueFound = true;
          return m_demValue;
        }
      }
    }
    
    // If no luck, return the mean radius of the target
    vector<Distance> radii = targetRadii();
    double a = radii[0].kilometers();
    double b = radii[1].kilometers();
    double c = radii[2].kilometers();
    
    m_demValue = (a + b + c)/3.0;
  
    m_demValueFound = true;  
    return m_demValue;
  }
  
  /**
   * Gets the radius from the DEM, if we have one.
   *
   * @param lat Latitude
   * @param lon Longitude
   *
   * @return @b Distance Local radius from the DEM
   */
  Distance DemShape::localRadius(const Latitude &lat, const Longitude &lon) {

    Distance distance=Distance();

    if (lat.isValid() && lon.isValid()) {
      m_demProj->SetUniversalGround(lat.degrees(), lon.degrees());

      // The next if statement attempts to do the same as the previous one, but not as well so
      // it was replaced.
      // if (!m_demProj->IsGood())
      //   return Distance();

      m_portal->SetPosition(m_demProj->WorldX(), m_demProj->WorldY(), 1);

      m_demCube->read(*m_portal);
      distance = Distance(m_interp->Interpolate(m_demProj->WorldX(),
                                                m_demProj->WorldY(),
                                                m_portal->DoubleBuffer()),
                                                Distance::Meters);
    }

    return distance;
  }


  /**
   * Return the scale of the DEM shape, in pixels per degree.
   *
   * @return @b double The scale of the DEM.
   */
  double DemShape::demScale() {
    return m_pixPerDegree;
  }


  /**
   * This method calculates the default normal (Ellipsoid for backwards
   * compatibility) for the DemShape.
   */

  void DemShape::calculateDefaultNormal() {

    if (!surfaceIntersection()->Valid() || !hasIntersection() ) {
      IString msg = "A valid intersection must be defined before computing the surface normal";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Get the coordinates of the current surface point
    SpiceDouble pB[3];
    pB[0] = surfaceIntersection()->GetX().kilometers();
    pB[1] = surfaceIntersection()->GetY().kilometers();
    pB[2] = surfaceIntersection()->GetZ().kilometers();

    // Get the radii of the ellipsoid
    vector<Distance> radii = targetRadii();
    double a = radii[0].kilometers();
    double b = radii[1].kilometers();
    double c = radii[2].kilometers();

    vector<double> normal(3,0.);

    NaifStatus::CheckErrors();
    surfnm_c(a, b, c, pB, (SpiceDouble *) &normal[0]);
    NaifStatus::CheckErrors();

    setNormal(normal);
    setHasNormal(true);

  }



  /**
   * Returns the DEM Cube object.
   *
   * @return @b Cube* A pointer to the DEM cube associated with this shape model.
   */
  Cube *DemShape::demCube() {
    return m_demCube;
  }


  /**
   * Indicates that this shape model is from a DEM. Since this method returns
   * true for this class, the Camera class will calculate the local normal
   * using neighbor points. This method is pure virtual and must be
   * implemented by all DemShape classes. This parent implementation returns
   * true.
   *
   * @return @b bool Indicates that this is a DEM shape model.
   */
  bool DemShape::isDEM() const {
    return true;
  }


  /**
   * This method calculates the local surface normal of the current intersection
   * point.
   *
   * @param neighborPoints
   */
  void DemShape::calculateLocalNormal(QVector<double *> neighborPoints) {

    std::vector<SpiceDouble> normal(3);
    if (neighborPoints.isEmpty()) {
      normal[0] = normal[1] = normal[2] = 0.0;
      setLocalNormal(normal);
      setHasLocalNormal(false);
      return;
    }

    // subtract bottom from top and left from right and store results
    double topMinusBottom[3];
    vsub_c(neighborPoints[0], neighborPoints[1], topMinusBottom);
    double rightMinusLeft[3];
    vsub_c(neighborPoints[3], neighborPoints [2], rightMinusLeft);

    // take cross product of subtraction results to get normal
    ucrss_c(topMinusBottom, rightMinusLeft, (SpiceDouble *) &normal[0]);

    // unitize normal (and do sanity check for magnitude)
    double mag;
    unorm_c((SpiceDouble *) &normal[0], (SpiceDouble *) &normal[0], &mag);

    if (mag == 0.0) {
      normal[0] = normal[1] = normal[2] = 0.0;
      setLocalNormal(normal);
      setHasLocalNormal(false);
      return;
   }
    else {
      setHasLocalNormal(true);
    }

    // Check to make sure that the normal is pointing outward from the planet
    // surface. This is done by taking the dot product of the normal and
    // any one of the unitized xyz vectors. If the normal is pointing inward,
    // then negate it.
    double centerLookVect[3];
    SpiceDouble pB[3];
    surfaceIntersection()->ToNaifArray(pB);
    unorm_c(pB, centerLookVect, &mag);
    double dotprod = vdot_c((SpiceDouble *) &normal[0], centerLookVect);
    if (dotprod < 0.0) {
      vminus_c((SpiceDouble *) &normal[0], (SpiceDouble *) &normal[0]);
    }

    setLocalNormal(normal);
  }


  /**
   * This method calculates the surface normal of the current intersection
   * point.
   */
  void DemShape::calculateSurfaceNormal() {
    calculateDefaultNormal();
  }


}
