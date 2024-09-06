/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "TriangularPlate.h"

#include <string>
#include <vector>
#include <numeric>

#include <QtGlobal>

#include "AbstractPlate.h"
#include "Angle.h"
#include "Distance.h"
#include "Intercept.h"
#include "IException.h"
#include "IString.h"
#include "Latitude.h"
#include "Longitude.h"
#include "NaifDskApi.h"
#include "SurfacePoint.h"

using namespace std;

namespace Isis {

  /** Basic zero plate constructor */
  // no need to implement this method since it is private and never called
  // within this class
  // TriangularPlate::TriangularPlate() : AbstractPlate(), m_plate(3, 3, 0.0) { }
  
  /** Constructor creates a unique copy of an existing plate  */
  TriangularPlate::TriangularPlate(const NaifTriangle &plate, const int &plateId) : 
                                   AbstractPlate(), m_plate(plate.copy()), 
                                   m_plateId(plateId) { }
  
  TriangularPlate::~TriangularPlate() { }
  
  int TriangularPlate::id() const {
    return m_plateId;
  }
 
  QString TriangularPlate::name() const {
    return "TriangularPlate";
  }

  /**
   * @brief Determines the maximum radius from all the vertices of the plate
   *  
   * This method returns the maximum radius (or magnitude) of the vectors of the 
   * plate.  This routine is typically use to determine the maximum height of a 
   * plate so that a sufficient body fixed radius can be used to determine grid 
   * intercept points. 
   *  
   * @return Distance Returns the maximum magnitude of the vertex vectors 
   *  
   * @author 2013-12-05 Kris Becker
   * @internal 
   *   @history 2013-12-05 Kris Becker Original Version
   */
  Distance TriangularPlate::maxRadius() const {
    double radius = qMax(qMax(vnorm_c(m_plate[0]), vnorm_c(m_plate[1])), 
                         vnorm_c(m_plate[2]));
    return ( Distance(radius, Distance::Kilometers) );
  }
  
  Distance TriangularPlate::minRadius() const {
    double radius = qMin(qMin(vnorm_c(m_plate[0]), vnorm_c(m_plate[1])), 
                         vnorm_c(m_plate[2]));
    return ( Distance(radius, Distance::Kilometers) );
  }
  
  /**
   * @brief Returns the area of the plate in km
   * @return double Area of plate in km
   * 
   * @author 2014-02-05 Kris Becker
   * @internal 
   *   @history 2014-02-05 Kris Becker Original Version
   * 
   */
  double TriangularPlate::area() const { 
  
    //  Get the lengths of each side
    NaifVector edge(3);
    vsub_c(m_plate[1], m_plate[0], &edge[0]);
    double s1 = vnorm_c(&edge[0]);
  
    vsub_c(m_plate[2], m_plate[0], &edge[0]);
    double s2 = vnorm_c(&edge[0]);
  
    vsub_c(m_plate[2], m_plate[1], &edge[0]);
    double s3 = vnorm_c(&edge[0]);
  
    // Heron's formula for area
    double S = (s1 + s2 + s3) / 2.0;
    double p_area = std::sqrt(S * (S - s1) * (S - s2) * (S - s3));
  
    return (p_area);
  }
  
  /**
   * @brief Compute the surface normal of the plate 
   *  
   * @return NaifVector Returns the surface normal of the plate 
   *  
   * @author 2013-12-05 Kris Becker
   * @internal 
   *   @history 2013-12-05 Kris Becker Original Version
   *  
   */ 
  NaifVector TriangularPlate::normal() const {
  
    //  Get the lengths of each side
    NaifVector edge1(3);
    vsub_c(m_plate[1], m_plate[0], &edge1[0]);
  
    NaifVector edge2(3);
    vsub_c(m_plate[2], m_plate[0], &edge2[0]);
  
    NaifVector norm(3);
    ucrss_c(&edge1[0], &edge2[0], &norm[0]);
    
    return (norm);
  }
  
  NaifVector TriangularPlate::center() const {
    double third(0.33333333333333331);
    NaifVector midPt(3);
    vlcom3_c(third, m_plate[0], third, m_plate[1], third, m_plate[2], &midPt[0]);
    return (midPt);
  }
  
  /**
   * @brief Computes the separation angle from the plate normal of a given vector
   *  
   * This method will compute the separation angle between the plate normal and 
   * the given direction vector.  This can be use for determining the incidence 
   * and/or emission angle from an observer point given its look direction. 
   *  
   * @param raydir Given a direction vector, compute the angle of separation 
   *               between it and the plate normal vector
   * 
   * @return Returns the angle of separation
   *  
   * @author 2013-12-05 Kris Becker
   * @internal 
   *   @history 2013-12-05 Kris Becker Original Version
   */
  Angle TriangularPlate::separationAngle(const NaifVector &raydir) const {
    //  Get two sides
    NaifVector norm(normal());
    double sepang = vsep_c(&norm[0], &raydir[0]);
    return ( Angle(sepang, Angle::Radians) );
  }
  
  /**
   * @brief Determines if a look direction from a point intercepts the plate 
   *  
   * Given a point in space in body fixed coordinates and a look direction, this 
   * method determines the point of intercept on the plate. 
   * 
   * @param vertex An observer point in space in body fixed coordinates
   * @param raydir A look direction vector 
   * 
   * @return bool Returns true if the look direction from the observer intercepts 
   *              the plate, otherwise returns false
   *  
   * @author 2013-12-05 Kris Becker
   * @internal 
   *   @history 2013-12-05 Kris Becker Original Version
   */
  bool TriangularPlate::hasIntercept(const NaifVertex &vertex, 
                                     const NaifVector &raydir) const {
    NaifVertex point;
    return (findPlateIntercept(vertex, raydir, point));
  }
  
  /**
   * @brief Determines the give lat/lon point intercept the triangular plate 
   *  
   * Given a latitude/longitude point, this method determines if it intercepts the 
   * plate. 
   * 
   * @param lat  The latitude of the given grid point
   * @param lon  Longitude of the given point
   * 
   * @return bool Returns true if the lat/lon point intercepts the plate, false 
   *              otherwise
   *  
   * @author 2013-12-05 Kris Becker
   * @internal 
   *   @history 2013-12-05 Kris Becker Original Version
   */
  bool TriangularPlate::hasPoint(const Latitude &lat, 
                                 const Longitude &lon) const {
  
    //  Extend the maximum height of the plate to a resonable distance
    double maxrad = maxRadius().kilometers() * 1.5;
  
    // Create a surface point above the highest plate vertex
    SurfacePoint point(lat, lon, Distance(maxrad, Distance::Kilometers));
    NaifVertex obs(3);
    point.ToNaifArray(&obs[0]);
  
    // Set the ray direction back toward the center of the body
    NaifVector raydir(3);
    vminus_c(&obs[0], &raydir[0]);
  
    // Determine where the point/ray intercepts the plate
    NaifVertex xpt;
    return (findPlateIntercept(obs, raydir, xpt));
  }
  
  /**
   * @brief Determine the intercept point of a lat/lon location for the plate 
   *  
   * Determines if a lat/lon point intercepts a plate.  Given a latitude and 
   * longitude coordinate, this method converts the point to a body fixed X/Y/Z 
   * value and computes intercept point within the boundaries if the plate. If no 
   * intercept is found, a null pointer is returned. 
   * 
   * @param lat Latitude of the point
   * @param lon Longitude of the point
   * 
   * @return SurfacePoint* Pointer to the intersection of the point on the 
   *                       triangle. If an intersection does not exist a null
   *                       pointer is returned.
   *  
   * @author 2013-12-05 Kris Becker
   * @internal 
   *   @history 2013-12-05 Kris Becker Original Version
   */
  SurfacePoint *TriangularPlate::point(const Latitude &lat, 
                                       const Longitude &lon) const {
    //  Extend the maximum height of the plate
    double maxrad = maxRadius().kilometers() * 1.5;
  
    // Create a surface point 1.5 times above the highest plate vertex
    SurfacePoint point(lat, lon, Distance(maxrad, Distance::Kilometers));
    NaifVertex obs(3);
    point.ToNaifArray(&obs[0]);
  
    // Set the ray direction back toward the center of the body
    NaifVector raydir(3);
    vminus_c(&obs[0], &raydir[0]);
  
    // Determine if the point/ray intercepts the plate
    NaifVertex xpt;
    if ( !findPlateIntercept(obs, raydir, xpt) ) return (0);
  
    // Construct the intercept point and return it
    SurfacePoint *ipoint(new SurfacePoint());
    ipoint->FromNaifArray(&xpt[0]);
    return (ipoint);
  }
  
  
  /**
   * @brief Conpute the intercept point on a triangular plate 
   *  
   * Given a point in space and a look direction, compute the intercept point on a 
   * triangular plate. If the intercept point does not exist, return a null 
   * pointer. 
   * 
   * @param vertex Specifies a point in space of a body fixed coordinate
   * @param raydir Specifies a look direction from the vertex in body fixed 
   *               coordinates.  It can be of any magnitude.
   * 
   * @return Intercept* Returns the intercept point if it exists on the 
   *                    triangular plate, otherwise returns a null pointer.
   *  
   * @author 2013-12-05 Kris Becker
   * @internal 
   *   @history 2013-12-05 Kris Becker Original Version
   */
  Intercept *TriangularPlate::intercept(const NaifVertex &vertex, 
                                        const NaifVector &raydir) const {
    NaifVertex point;
    if ( !findPlateIntercept(vertex, raydir, point) ) return (0);
  
    // Got a valid intercept.  Construct it and return
    SurfacePoint *xpt = new SurfacePoint();
    xpt->FromNaifArray(&point[0]);
    return (new Intercept(vertex, raydir, xpt, clone()));
  }
  

  /**
   * @brief Returns the vth point of the triangle
   *  
   * Returns the X/Y/Z body fixed coordinate of the plate. 
   *  
   * @param v Specifies the point to return. Valid values of v is 0 to 2.
   * 
   * @return NaifVertex Returns the X/Y/Z body fixed coordinate of the requested 
   *                vertex in the plate
   *  
   * @author 2013-12-05 Kris Becker
   * @internal 
   *   @history 2013-12-05 Kris Becker Original Version
   */
  NaifVertex TriangularPlate::vertex(int v) const {
    NaifVertex vec(3);
    if ( (v < 0) || (v > 2) ) {
      std::string msg = "Unable to get TriangularPlate vertex for index ["
                    + toString(v) + "]. Valid index range is 0-2.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    for ( int i = 0 ; i < 3 ; i++ ) {
      vec[i] = m_plate[v][i];
    }
    return (vec);
  }
  
  
  /**
   * @brief Retrns a clone of the current plate 
   *  
   * Provides replication of the current triangular plate. Note this 
   * implementation returns a shared copy of the triangular plate as long as the 
   * plate type is shared by copy (TNT library is). 
   * 
   * @return AbstractPlate* Returns a copy of the plate 
   *  
   * @author 2013-12-05 Kris Becker
   * @internal 
   *   @history 2013-12-05 Kris Becker Original Version
   */
  AbstractPlate *TriangularPlate::clone() const {
    return (new TriangularPlate(m_plate, m_plateId));
  }
  
  /**
   * @brief Determines of if given a vertex and look direction intercepts the 
   *        plate
   *  
   * If from the location in space as specifed by obs and a look direction 
   * intercepts the current plate, the intercept point is returned. 
   *  
   * @param obs    An observer point in space
   * @param raydir A look direction eminating from the observer point
   * @param point  If the observer/look direction intercepts the plate, the 
   *               intercept point is computed an returned in this parameter. If
   *               the intercept point is not determined, this point is unchanged.
   * 
   * @return bool  Returns true in an intercept point is found, otherwise it 
   *               returned.
   *  
   * @author 2013-12-05 Kris Becker
   * @internal 
   *   @history 2013-12-05 Kris Becker Original Version
   */
  bool TriangularPlate::findPlateIntercept(const NaifVertex &obs,
                                           const NaifVector &raydir, 
                                           NaifVertex &point) const {
  
    // Construct three edges of the solid tehtrahderal between plate and observer
    NaifVector e1(3), e2(3), e3(3);
    vsub_c(m_plate[0], &obs[0], &e1[0]);
    vsub_c(m_plate[1], &obs[0], &e2[0]);
    vsub_c(m_plate[2], &obs[0], &e3[0]);
  
    // Test to see if the ray direction and plate normal are perpendicular
    NaifVector tnorm12(3);
    vcrss_c(&e1[0], &e2[0], &tnorm12[0]);
    double tdot12 = vdot_c(&raydir[0], &tnorm12[0]);
    double en   = vdot_c(&e3[0], &tnorm12[0]);
  
    //  Check for e3 perpendicular to plate normal.  If true, e3 is a linear
    // combination of e1 and e2.
    if ( qFuzzyCompare(en+1.0, 1.0) ) return (false);
  
    // Check if raydir and e3 are in same half space
    if ( (en > 0.0) && (tdot12 < 0.0) ) return (false);
    if ( (en < 0.0) && (tdot12 > 0.0) ) return (false);
  
    // Check that raydir and e1 are on the same side of the plane spanned by e2 
    // and e3.
    NaifVector tnorm23(3);
    vcrss_c(&e2[0], &e3[0], &tnorm23[0]);
    double tdot23 = vdot_c(&raydir[0], &tnorm23[0]);
  
    // Check if raydir and e3 are in same halfspace
    if ( (en > 0.0) && (tdot23 < 0.0) ) return (false);
    if ( (en < 0.0) && (tdot23 > 0.0) ) return (false);
  
    // Finally check that raydir and e2 are in the same half space bounded by e3
    // and e2.
    NaifVector tnorm31(3);
    vcrss_c(&e3[0], &e1[0], &tnorm31[0]);
    double tdot31 = vdot_c(&raydir[0], &tnorm31[0]);
  
    // Check if raydir and e2 are in same halfspace
    if ( (en > 0.0) && (tdot31 < 0.0) ) return (false);
    if ( (en < 0.0) && (tdot31 > 0.0) ) return (false);
  
    // Ok, we know raydir intersects the plate.  Compute the intercept point if
    // the denominator is not 0.
    double denom = tdot12 + tdot23 + tdot31;

    // NOTE: If we have gotten this far in the method,
    // we have checked that en != 0
    //     if en < 0, then tdot12 <= 0, tdot23 <= 0 and tdot31 <= 0
    //     if en > 0, then tdot12 >= 0, tdot23 >= 0 and tdot31 >= 0
    // So, in order for the denominator to be 0, then it must be that
    //     tdot12 == tdot23 == tdot31 == 0
    if ( qFuzzyCompare(denom+1.0, 1.0) ) return (false);
  
    double scale = en / denom;
    NaifVertex xpt(3);
    vlcom_c(1.0, &obs[0], scale, &raydir[0], &xpt[0]);
    point = xpt;
    return (true);
  }

} // namespace Isis
