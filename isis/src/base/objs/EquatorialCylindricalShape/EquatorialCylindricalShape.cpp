/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "EquatorialCylindricalShape.h"

#include <QDebug>

#include <algorithm>
#include <cfloat>
#include <string>
#include <iomanip>
#include <cmath>
#include <vector>

#include <SpiceUsr.h>
#include <SpiceZfc.h>
#include <SpiceZmc.h>

#include "Cube.h"
#include "IException.h"
// #include "Geometry3D.h"
#include "Latitude.h"
// #include "LinearAlgebra.h"
#include "Longitude.h"
#include "NaifStatus.h"
#include "SpecialPixel.h"
#include "SurfacePoint.h"
#include "Table.h"

using namespace std;

#define MAX(x,y) (((x) > (y)) ? (x) : (y))

namespace Isis {
  /**
   * Initialize the ISIS Equatorial Cylindrical shape model.
   *
   * @param pvl Valid ISIS cube label.
   */
  EquatorialCylindricalShape::EquatorialCylindricalShape(Target *target, Pvl &pvl) :
      DemShape(target, pvl) {
    setName("EquatorialCylindricalShape");

    m_minRadius = NULL;
    m_maxRadius = NULL;

     // Read in the min/max radius of the DEM file and the Scale of the DEM
     // file in pixels/degree
    if (!demCube()->hasTable("ShapeModelStatistics")) {
      std::string msg = "The input cube references a ShapeModel that has "
        "not been updated for the new ray tracing algorithm. All DEM "
        "files must now be padded at the poles and contain a "
        "ShapeModelStatistics table defining their minimum and maximum "
        "radii values. The demprep program should be used to prepare the "
        "DEM before you can run this program. There is more information "
        "available in the documentation of the demprep program.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Table table("ShapeModelStatistics", demCubeFile(), *demCube()->label()));
    Table table("ShapeModelStatistics", demCube()->fileName().toStdString(), *demCube()->label());

    // Find minimum and maximum radius
    m_minRadius = new Distance(table[0]["MinimumRadius"], Distance::Kilometers);
    m_maxRadius = new Distance(table[0]["MaximumRadius"], Distance::Kilometers);
  }


  /**
   * Destructor for ISIS Equatorial Cylindrical shape model
   */
  EquatorialCylindricalShape::~EquatorialCylindricalShape() {

    delete m_minRadius;
    m_minRadius = NULL;

    delete m_minRadius;
    m_maxRadius = NULL;
   }


  /**
   * Finds the surface intersection point.
   *
   * @param observerBodyFixedPosition  Three dimensional position of the observer,
   *                                   in the coordinate system of the target body.
   * @param observerLookVectorToTarget Three dimensional direction vector from
   *                                   the observer to the target.
   *
   * @return @b bool Indicates whether this shape model found a valid surface intersection.
   */
  bool EquatorialCylindricalShape::intersectSurface(vector<double> observerBodyFixedPos,
                                                    vector<double> observerLookVectorToTarget) {

    // try to intersect the surface using the DemShape method
    // if this is successful, return
    //
    // otherwise, (i.e. DemShape::intersectSurface() fails) we will attempt to
    // intersect using the following iterative method...
    if (!DemShape::intersectSurface(observerBodyFixedPos, observerLookVectorToTarget)) {

      SpiceDouble a = targetRadii()[0].kilometers();

      double plen = 0.0;
      SpiceDouble plat, plon, pradius;
      SpiceDouble pB[3];
      double maxRadiusMetersSquared = m_maxRadius->kilometers() * m_maxRadius->kilometers();

      // Separate iteration algorithms are used for different projections -
      // use this iteration for equatorial cylindrical type projections that
      // failed to find an intersection with the DemShape method.
      int maxit = 100;
      int it = 0;
      bool done = false;

      // Normalize the look vector
      SpiceDouble ulookB[3];
      ulookB[0] = observerLookVectorToTarget[0];
      ulookB[1] = observerLookVectorToTarget[1];
      ulookB[2] = observerLookVectorToTarget[2];
      vhat_c(ulookB, ulookB);

      // Calculate the limb viewing angle to see if the line of sight is
      // pointing away from the planet
      SpiceDouble observer[3];
      observer[0] = observerBodyFixedPos[0];
      observer[1] = observerBodyFixedPos[1];
      observer[2] = observerBodyFixedPos[2];

      SpiceDouble negobserver[3];
      vminus_c(observer, negobserver);

      double psi0 = vsep_c(negobserver, ulookB); // find separation angle between -obsPos and look

      // If psi0 is greater than 90 degrees, then reject data as looking
      // away from the planet and no proper tangent point exists in the
      // direction that the spacecraft is looking
      if (psi0 > PI / 2.0) {
        setHasIntersection(false);
        return hasIntersection();
      }


      #if 0
      // JWB - The commented code here is an alternate way of doing this...
      //       which method has less expensive computing???
      //       I believe the alternate one may be (no need for cosine after naif routines)
      //
      //
      // find the vector to the point found by orthogonally projecting the observer position vector
      // onto the line containing the look direction vector
      SpiceDouble perp[3];
      vperp_c(&observer, &ulookB, perp);
      SpiceDouble v[3];
      vsub_c(&observer, &perp, v);
      double dot = vdot_c(ulookB, v);
      qDebug() << "dot = " << dot;
      if (dot > 0.0) {
        setHasIntersection(false);
        return hasIntersection();
      }

      // TODO: When added to ISIS library, see LinearAlgebra/Geometry3D equivalent methods:
      //           vperp=perpendicular{find max and normalize vectors, 2 dot products}
      //           vsep=Geometry3D::separationAngle{find max and normalize vectors, dot product}

      // replace calculation of psi0 and intersection check using psi0
      // replace calculation of tvec, observerdist, cospsi0
      // tvec below is equivalent to perp found above
      // observerdist*cospsi0 is equivalent to vnorm_c(v) [pretty sure about this equivalence... need to verify]

      #endif

      // Calculate the vector to the surface point
      SpiceDouble tvec[3];
      double observerdist = vnorm_c(observer);
      double cospsi0 = cos(psi0);
      tvec[0] = observer[0] + observerdist * cospsi0 * ulookB[0];
      tvec[1] = observer[1] + observerdist * cospsi0 * ulookB[1];
      tvec[2] = observer[2] + observerdist * cospsi0 * ulookB[2];

      double tlen = vnorm_c(tvec);

      // Calculate distance along look vector to first and last test point
      double radiusDiff = maxRadiusMetersSquared - tlen * tlen;

      // Make sure radiusDiff makes sense
      if (radiusDiff >= 0.0) {
        radiusDiff = sqrt(radiusDiff);
      }
      else {
        setHasIntersection(false);
        return hasIntersection();
      }

      double d0 = observerdist * cospsi0 - radiusDiff;
      double dm = observerdist * cospsi0 + radiusDiff;

      // Set the properties at the first test observation point
      double d = d0;
      SpiceDouble g1[3];
      g1[0] = observer[0] + d0 * ulookB[0];
      g1[1] = observer[1] + d0 * ulookB[1];
      g1[2] = observer[2] + d0 * ulookB[2];

      double g1len = vnorm_c(g1);
      SpiceDouble g1lat, g1lon, g1radius;
      reclat_c(g1, &g1radius, &g1lon, &g1lat);
      g1lat *= RAD2DEG;
      g1lon *= RAD2DEG;

      if (g1lon < 0.0) g1lon += 360.0;

      SpiceDouble negg1[3];
      vminus_c(g1, negg1);

      double psi1 = vsep_c(negg1, ulookB);

      // Set dalpha to be half the grid spacing for nyquist sampling
      //double dalpha = (PI/180.0)/(2.0*p_demScale);
      double cmin = cos((90.0 - 1.0 / (2.0 * demScale())) * DEG2RAD);
      double dalpha = MAX(cos(g1lat * DEG2RAD), cmin) / (2.0 * demScale() * RAD2DEG);

      // Previous Sensor version used local version of this method with lat and lon doubles.  Steven said
      // it didn't make a significant difference in speed.
      double r1 = (localRadius(Latitude(g1lat, Angle::Degrees),
                               Longitude(g1lon, Angle::Degrees))).kilometers();

      if (Isis::IsSpecial(r1)) {
        setHasIntersection(false);
        return hasIntersection();
      }

      // Set the tolerance to a fraction of the equatorial radius, a
      double tolerance = 3E-8 * a;

      // Main iteration loop
      // Loop from g1 to gm stepping by angles of dalpha until intersection is found
      while (!done) {

        if (d > dm) {
          setHasIntersection(false);
          return hasIntersection();
        }

        it = 0;

        // Calculate the angle between the look vector and the planet radius at the current
        // test point
        double psi2 = psi1 + dalpha;

        // Calculate the step size
        double dd = g1len * sin(dalpha) / sin(PI - psi2);

        // JAA:  If we are moving along the vector at a smaller increment than the pixel
        // tolerance we will be in an infinite loop.  The infinite loop is elimnated by
        // this test.  Now the algorithm produces a jagged limb in phocube.  This may
        // be a function of the very low resolution of the Vesta DEM and could
        // improve in the future
        if (dd < tolerance) {
          setHasIntersection(false);
          return hasIntersection();
        }

        // Calculate the vector to the current test point from the planet center
        d = d + dd;

        SpiceDouble g2[3];
        g2[0] = observer[0] + d * ulookB[0];
        g2[1] = observer[1] + d * ulookB[1];
        g2[2] = observer[2] + d * ulookB[2];

        double g2len = vnorm_c(g2);

        // Determine lat,lon,radius at this point
        SpiceDouble g2lat, g2lon, g2radius;
        reclat_c(g2, &g2radius, &g2lon, &g2lat);

        g2lat *= RAD2DEG;
        g2lon *= RAD2DEG;

        if (g2lon < 0.0) g2lon += 360.0;

        // Previous Sensor version used local version of this method with lat and lon doubles to save
        // According to Steven the savings was negligible.
        double r2 = (localRadius(Latitude(g2lat, Angle::Degrees),
                                 Longitude(g2lon, Angle::Degrees))).kilometers();


        if (Isis::IsSpecial(r2)) {
          setHasIntersection(false);
          return hasIntersection();
        }

        // Test for intersection
        if (r2 > g2len) {
          // An intersection has occurred. Interpolate between g1 and g2 to get the
          // lat,lon of the intersect point.

          // If g1 and g2 straddle a hill, then we may need to iterate a few times
          // until we are on the linear segment.
          while (it < maxit && !done) {
            // Calculate the fractional distance "v" to move along the look vector
            // to the intersection point. Check to see if there was a convergence
            // of the solution and the tolerance was too small to detect it.
            double palt;
            if ((g2len * r1 / r2 - g1len) == 0.0) {
              setHasIntersection(true);
              plen = pradius;
              palt = 0.0;
              done = true;
            }
            else {
              double v = (r1 - g1len) / (g2len * r1 / r2 - g1len);
              pB[0] = g1[0] + v * dd * ulookB[0];
              pB[1] = g1[1] + v * dd * ulookB[1];
              pB[2] = g1[2] + v * dd * ulookB[2];
              plen = vnorm_c(pB);
              reclat_c(pB, &pradius, &plon, &plat);
              plat *= RAD2DEG;
              plon *= RAD2DEG;
              if (plon < 0.0) plon += 360.0;

              if (plon > 360.0) plon -= 360.0;

              // Previous Sensor version used local version of this method with lat and lon doubles. ..Why Jeff???
              pradius = (localRadius(Latitude(plat, Angle::Degrees),
                                     Longitude(plon, Angle::Degrees))).kilometers();

              if (Isis::IsSpecial(pradius)) {
                setHasIntersection(false);
                return hasIntersection();
              }
              palt = plen - pradius;

              // The altitude relative to surface is +ve at the observation point,
              // so reset g1=p and r1=pradius
              if (palt > tolerance) {
                it = it + 1;
                g1[0] = pB[0];
                g1[1] = pB[1];
                g1[2] = pB[2];
                g1len = plen;
                r1 = pradius;
                dd = dd * (1.0 - v);

                // The altitude relative to surface -ve at the observation point,
                // so reset g2=p and r2=pradius
              }
              else if (palt < -tolerance) {
                it = it + 1;
                g2[0] = pB[0];
                g2[1] = pB[1];
                g2[2] = pB[2];
                g2len = plen;
                r2 = pradius;
                dd = dd * v;

                // We are within the tolerance, so the solution has converged
              }
              else {
                setHasIntersection(true);
                plen = pradius;
                palt = 0.0;
                done = true;
              }
            }
            if (!done && it >= maxit) {
              setHasIntersection(false);
              return hasIntersection();
            }
          }
        }

        g1[0] = g2[0];
        g1[1] = g2[1];
        g1[2] = g2[2];
        g1len = g2len;
        r1 = r2;
        psi1 = psi2;

        // TODO:  Examine how dalpha is computed at the limb for Vesta.  It
        // appears that eventually it gets really small and causes the loop
        // to be neverending.  Of course this could be happening for other
        // limb images and we just have never tested the code.  For example,
        // Messenger with a DEM could cause the problem.  As a result JAA
        // put in a test (above) for dd being smaller than the pixel
        // convergence tolerance.  If so the loop exits without an
        // intersection
        dalpha = MAX(cos(g2lat * DEG2RAD), cmin) / (2.0 * demScale() * RAD2DEG);
      } // end while

      SpiceDouble intersectionPoint[3];
      SpiceBoolean found;

      NaifStatus::CheckErrors();
      surfpt_c(&observerBodyFixedPos[0], &observerLookVectorToTarget[0], plen, plen, plen,
               intersectionPoint, &found);
      NaifStatus::CheckErrors();

      surfaceIntersection()->FromNaifArray(intersectionPoint);

      if (!found) {
        setHasIntersection(false);
        return hasIntersection();
      }
      else {
        return hasIntersection();
      }

    }

    setHasIntersection(true);
    return hasIntersection();
  }
  // Do nothing since the DEM intersection was already successful
}
