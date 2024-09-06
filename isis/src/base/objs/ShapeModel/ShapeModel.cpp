/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "ShapeModel.h"

#include <QDebug>

#include <algorithm>
#include <cfloat>
#include <iostream>
#include <iomanip>
#include <vector>

#include <cmath>

#include <SpiceUsr.h>
#include <SpiceZfc.h>
#include <SpiceZmc.h>

#include "Distance.h"
#include "SurfacePoint.h"
#include "IException.h"
#include "IString.h"
#include "NaifStatus.h"
#include "Spice.h"
#include "Target.h"

using namespace std;

namespace Isis {
  /**
   * Default constructor creates ShapeModel object, initializing name to an
   * empty string, surface point to an empty surface point, has intersection to
   * FALSE, has normal to FALSE, has ellipsoid intersection to FALSE, normal
   * vector size to 3, and target to NULL.
   */
  ShapeModel::ShapeModel() {
    Initialize();
    m_target = NULL;
  }


  /**
   * Constructs and loads a shape model from a target only
   *
   * This constructor creates ShapeModel object, initializing name to an
   * empty string, surface point to an empty surface point, has intersection to
   * FALSE, has normal to FALSE, has ellipsoid intersection to FALSE, normal
   * vector size to 3, and  target to the given target.
   *
   * @param target A pointer to a valid ISIS target.
   */
  ShapeModel::ShapeModel(Target *target) {
    Initialize();
    m_target = target;
  }


  /**
   * Initializes the ShapeModel private variables.
   */
  void ShapeModel::Initialize() {
    m_name = new QString();
    m_surfacePoint = new SurfacePoint();
    m_hasIntersection = false;
    m_hasNormal = false;
    m_hasLocalNormal = false;
    m_normal.resize(3,0.);
    m_localNormal.resize(3, 0.);
    m_hasEllipsoidIntersection = false;
  }


  //! Virtual destructor to destroy the ShapeModel object.
  ShapeModel::~ShapeModel() {

    delete m_name;
    m_name = NULL;

    delete m_surfacePoint;
    m_surfacePoint = NULL;
  }


/**
 * @brief Compute surface intersection with optional occlusion check
 *
 * This method sets the surface point at the given latitude, longitude. The
 * derived model is called to get the radius at that location to complete the
 * accuracy of the surface point, them the derived method is called to complete
 * the intersection.
 *
 * @author 2017-03-23 Kris Becker
 *
 * @param lat          Latitude of the surface point
 * @param lon          Longitude of the surface point
 * @param observerPos  Position of the observer
 * @param backCheck    Flag to indicate occlusion check
 *
 * @return bool        True if the intersection point is valid (visable)
 */
  bool ShapeModel::intersectSurface(const Latitude &lat, const Longitude &lon,
                                    const std::vector<double> &observerPos,
                                    const bool &backCheck) {
    // Distance radius = localRadius(lat, lon);
    return (intersectSurface(SurfacePoint(lat, lon, localRadius(lat, lon)), observerPos, backCheck));
  }


/**
 * @brief Compute surface intersection with optional occlusion check
 *
 * This method sets the surface point at the given latitude, longitude. The
 * derived model is called to get the radius at that location to complete the
 * accuracy of the surface point, them the derived method is called to complete
 * the intersection.
 *
 * @author 2017-03-23 Kris Becker
 *
 * @param surfpt        Absolute point on the surface to check
 * @param observerPos  Position of the observer
 * @param backCheck    Flag to indicate occlusion check
 *
 * @return bool        True if the intersection point is valid (visable)
 */
  bool ShapeModel::intersectSurface(const SurfacePoint &surfpt,
                                    const std::vector<double> &observerPos,
                                    const bool &backCheck) {

    // The default behavior is to set the point in the model without
    //  intersection tests at all
    setSurfacePoint(surfpt);
    return (true);
  }

  /**
   *  Calculates the ellipsoidal surface normal.
   */
  void ShapeModel::calculateEllipsoidalSurfaceNormal()  {
    // The below code is not truly normal unless the ellipsoid is a sphere.  TODO Should this be
    // fixed? Send an email asking Jeff and Stuart.  See Naif routine surfnm.c to get the true
    // for an ellipsoid.  For testing purposes to match old results do as Isis currently does until
    // Jeff and Stuart respond.

    if (!m_hasIntersection || !surfaceIntersection()->Valid()) {
     std::string msg = "A valid intersection must be defined before computing the surface normal";
      throw IException(IException::Programmer, msg, _FILEINFO_);
   }

   // Get the coordinates of the current surface point
    SpiceDouble pB[3];
    pB[0] = surfaceIntersection()->GetX().kilometers();
    pB[1] = surfaceIntersection()->GetY().kilometers();
    pB[2] = surfaceIntersection()->GetZ().kilometers();

    // Unitize the vector
    SpiceDouble upB[3];
    SpiceDouble dist;
    unorm_c(pB, upB, &dist);
    memcpy(&m_normal[0], upB, sizeof(double) * 3);

    m_hasNormal = true;
  }


  /**
   * Computes and returns emission angle, in degrees, given the observer
   * position.
   *
   * Emission Angle: The angle between the surface normal vector at the
   * intersection point and the vector from the intersection point to the
   * observer (usually the spacecraft). The emission angle varies from 0 degrees
   * when the observer is viewing the sub-spacecraft point (nadir viewing) to 90
   * degrees when the intercept is tangent to the surface of the target body.
   * Thus, higher values of emission angle indicate more oblique viewing of the
   * target.
   *
   * @param observerBodyFixedPosition  Three dimensional position of the observer,
   *                                   in the coordinate system of the target body.
   *
   * @return The emission angle, in decimal degrees.
   *
   */
  double ShapeModel::emissionAngle(const std::vector<double> &observerBodyFixedPosition) {

    // Calculate the surface normal if we haven't yet
    if (!hasNormal()) calculateDefaultNormal();

    // Get vector from center of body to surface point
    SpiceDouble pB[3];
    pB[0] = surfaceIntersection()->GetX().kilometers();
    pB[1] = surfaceIntersection()->GetY().kilometers();
    pB[2] = surfaceIntersection()->GetZ().kilometers();

    // Get vector from surface point to observer and normalize it
    SpiceDouble psB[3], upsB[3], dist;
    vsub_c((ConstSpiceDouble *) &observerBodyFixedPosition[0], pB, psB);
    unorm_c(psB, upsB, &dist);

    double angle = vdot_c((SpiceDouble *) &m_normal[0], upsB);
    if(angle > 1.0) return 0.0;
    if(angle < -1.0) return 180.0;
    return acos(angle) * RAD2DEG;
  }


  /**
   * Returns the status of the ellipsoid model intersection.
   *
   * @return @b bool Indicates whether this shape model has a valid ellipsoid intersection.
   */
  bool ShapeModel::hasEllipsoidIntersection() {
    return m_hasEllipsoidIntersection;
  }


  /**
   * Computes and returns incidence angle, in degrees, given the illuminator position.
   *
   * Incidence Angle: The angle between the surface normal vector at the intersection
   * point and the vector from the intersection point to the illuminator (usually the
   * sun).
   *
   * Note: this method does not use the surface model.
   *
   * @param illuminatorBodyFixedPosition Three dimensional position for the illuminator,
   *                                     in the body-fixed coordinate system.
   *
   * @return @b double Incidence angle, in degrees.
   */
  double ShapeModel::incidenceAngle(const std::vector<double> &illuminatorBodyFixedPosition) {

    // Calculate the surface normal if we haven't yet.
    if (!hasNormal()) calculateDefaultNormal();

    // Get vector from center of body to surface point
    SpiceDouble pB[3];
    pB[0] = surfaceIntersection()->GetX().kilometers();
    pB[1] = surfaceIntersection()->GetY().kilometers();
    pB[2] = surfaceIntersection()->GetZ().kilometers();

    // Get vector from surface point to sun and normalize it
    SpiceDouble puB[3], upuB[3], dist;
    vsub_c((SpiceDouble *) &illuminatorBodyFixedPosition[0], pB, puB);
    unorm_c(puB, upuB, &dist);

    double angle = vdot_c((SpiceDouble *) &m_normal[0], upuB);
    if(angle > 1.0) return 0.0;
    if(angle < -1.0) return 180.0;
    return acos(angle) * RAD2DEG;
  }


  /**
   * Finds the intersection point on the ellipsoid model using the given
   * position of the observer (spacecraft) and direction vector from the
   * observer to the target (body).
   *
   * @param observerBodyFixedPosition  Three dimensional position of the observer,
   *                                   in the coordinate system of the target body.
   * @param observerLookVectorToTarget Three dimensional direction vector from
   *                                   the observer to the target.
   *
   * @return @b bool Indicates whether this shape model found a valid ellipsoid intersection.
   */
  bool ShapeModel::intersectEllipsoid(const std::vector<double> observerBodyFixedPosition,
                                      const std::vector<double> &observerLookVectorToTarget) {

    // Clear out previous surface point and normal
    clearSurfacePoint();

    SpiceDouble lookB[3];

    // This memcpy does:
    // lookB[0] = observerLookVectorToTarget[0];
    // lookB[1] = observerLookVectorToTarget[1];
    // lookB[2] = observerLookVectorToTarget[2];
    memcpy(lookB,&observerLookVectorToTarget[0], 3*sizeof(double));

    // get target radii
    std::vector<Distance> radii = targetRadii();
    SpiceDouble a = radii[0].kilometers();
    SpiceDouble b = radii[1].kilometers();
    SpiceDouble c = radii[2].kilometers();

    // check if observer look vector intersects the target
    SpiceDouble intersectionPoint[3];
    SpiceBoolean intersected = false;

    NaifStatus::CheckErrors();
    surfpt_c((SpiceDouble *) &observerBodyFixedPosition[0], lookB, a, b, c,
             intersectionPoint, &intersected);
    NaifStatus::CheckErrors();

    if (intersected) {
      m_surfacePoint->FromNaifArray(intersectionPoint);
      m_hasIntersection = true;
    }
    else {
      m_hasIntersection = false;
    }

    m_hasEllipsoidIntersection = m_hasIntersection;
    return m_hasIntersection;
  }


  /**
   * Computes and returns phase angle, in degrees, given the positions of the
   * observer and illuminator.
   *
   * Phase Angle: The angle between the vector from the intersection point to
   * the observer (usually the spacecraft) and the vector from the intersection
   * point to the illuminator (usually the sun).
   *
   * @param observerBodyFixedPosition  Three dimensional position of the observer,
   *                                   in the coordinate system of the target body.
   * @param illuminatorBodyFixedPosition Three dimensional position for the illuminator,
   *                                     in the body-fixed coordinate system.
   *
   * @return @b double Phase angle, in degrees.
   */
  double ShapeModel::phaseAngle(const std::vector<double> &observerBodyFixedPosition,
                                const std::vector<double> &illuminatorBodyFixedPosition) {

    // Get vector from center of body to surface point
    SpiceDouble pB[3];
    pB[0] = surfaceIntersection()->GetX().kilometers();
    pB[1] = surfaceIntersection()->GetY().kilometers();
    pB[2] = surfaceIntersection()->GetZ().kilometers();

    // Get vector from surface point to observer and normalize it
    SpiceDouble psB[3], upsB[3], dist;
    vsub_c((SpiceDouble *) &observerBodyFixedPosition[0], pB, psB);
    unorm_c(psB, upsB, &dist);

    // Get vector from surface point to sun and normalize it
    SpiceDouble puB[3], upuB[3];
    vsub_c((SpiceDouble *) &illuminatorBodyFixedPosition[0], pB, puB);
    unorm_c(puB, upuB, &dist);

    double angle = vdot_c(upsB, upuB);

    // How can these lines be tested???
    if(angle > 1.0) return 0.0;
    if(angle < -1.0) return 180.0;
    return acos(angle) * RAD2DEG;
  }


  /**
   * Returns the surface intersection for this ShapeModel.
   *
   * @return @b SurfacePoint Three dimensional position for the surface
   *         intersection, in body-fixed coordinate system.
   */
  SurfacePoint *ShapeModel::surfaceIntersection() const {
    return m_surfacePoint;
  }


  /**
   * Returns intersection status.
   *
   * @return @b bool Indicates whether this ShapeModel has an intersection.
   */
  bool ShapeModel::hasIntersection() {
    return m_hasIntersection;
  }


  /**
   * Returns surface point normal status.
   *
   * @return @b Indicates whether this ShapeModel has a surface normal.
   */
  bool ShapeModel::hasNormal() const {
    return m_hasNormal;
  }


  /**
   * Returns surface point local normal status.
   *
   * @return @b Indicates whether this ShapeModel has a surface normal.
   */
  bool ShapeModel::hasLocalNormal() const {
    return m_hasLocalNormal;
  }


  /**
   * Clears or resets the current surface point.
   */
  void ShapeModel::clearSurfacePoint() {
    setHasIntersection(false);
    m_hasEllipsoidIntersection = false;
  }


  /**
   * Returns the surface normal at the current intersection point.
   * Note: This method will throw an error if the normal doesn't exist. Use the
   * hasNormal() method to verify before calling this method.
   *
   * @see hasNormal()
   *
   * @return A surface normal vector, if it exists.
   */
  std::vector<double> ShapeModel::normal() {
    if (m_hasNormal ) {
      return m_normal;
    }
    else {
      std::string message = "The normal has not been computed.";
      throw IException(IException::Unknown, message, _FILEINFO_);
    }
  }

    /**
   * Returns the local surface normal at the current intersection point.
   * Note: This method will throw an error if the normal doesn't exist. Use the
   * hasLocalNormal() method to verify before calling this method.
   *
   * @see hasLocalNormal()
   *
   * @return A local surface normal vector, if it exists.
   */
  std::vector<double> ShapeModel::localNormal() {
    if (m_hasLocalNormal ) {
      return m_localNormal;
    }
    else {
      std::string message = "The local normal has not been computed.";
      throw IException(IException::Unknown, message, _FILEINFO_);
    }
  }

/**
 * @brief Default occulsion implementation
 *
 *  This method is originally copied from Sensor::SetLocalGround(bool
 *  backCheck). This version checks for the emission angle from the observer to
 *  be less than or equal to 90 degrees.
 *
 *  It is recommended that models derived from this base class reimplement this
 *  method if a more robust, efficent test can be made.
 *
 *  Note this implementation does not handle occlusion!
 *
 * @author 2017-03-17 Kris Becker
 *
 * @param observerPos   Position of the observer in body fixed coordinates
 * @param lookDirection Look direction from the observer
 *
 * @return bool True if the point is not visable, false if it can be seen
 */
  bool ShapeModel::isVisibleFrom(const std::vector<double> observerPos,
                                 const std::vector<double> lookDirection)  {
    if ( hasIntersection() ) {
      if ( fabs(emissionAngle(observerPos)) <= 90.0 ) {
        return (true);
      }
    }

    // All other conditions indicate the point is not visable from the observer
    return (false);
  }

  /**
   * Returns the status of the target. If it is NULL, this method
   * returns false.
   *
   * @return @b Indicates whether the target is valid.
   */
  bool ShapeModel::hasValidTarget() const {
    return (m_target != NULL);
  }


  /**
   * Returns the radii of the body in km. The radii are obtained from the
   * target.
   * Note: This method will throw an error if the ShapeModel does not
   * have a valid target. Use the hasValidTarget() method to verify before
   * calling this method.
   *
   * @see hasValidTarget()
   *
   * @return Three dimensional vector containing the ellipsoid radii values.
   */
  std::vector<Distance> ShapeModel::targetRadii() const {
    if (hasValidTarget()) {
      return m_target->radii();
    }
    else {
      std::string message = "Unable to find target radii for ShapeModel. Target is NULL. ";
      throw IException(IException::Programmer, message, _FILEINFO_);
     }
  }


  /**
   * Sets the surface normal for the currect intersection point.
   * Note: This method will throw an error if this ShapeModel doesn't have
   * an intersection. Use the hasIntersection() method to verify before
   * calling this method.
   *
   * @see hasIntersection()
   *
   * @param normal Three dimensional surface normal vector.
   *
   */
  void ShapeModel::setNormal(const std::vector<double> normal) {
    if (m_hasIntersection) {
      m_normal = normal;
      m_hasNormal = true;
    }
    else {
      std::string message = "No intersection point is known.  A normal cannot be set.";
      throw IException(IException::Unknown, message, _FILEINFO_);
    }
  }

    /**
   * Sets the local normal for the currect intersection point.
   * Note: This method will throw an error if this ShapeModel doesn't have
   * an intersection. Use the hasIntersection() method to verify before
   * calling this method.
   *
   * @see hasIntersection()
   *
   * @param normal Three dimensional local normal vector.
   *
   */
  void ShapeModel::setLocalNormal(const std::vector<double> normal) {
    if (m_hasIntersection) {
      m_localNormal = normal;
      m_hasLocalNormal = true;
    }
    else {
      std::string message = "No intersection point is known.  A local normal cannot be set.";
      throw IException(IException::Unknown, message, _FILEINFO_);
    }
  }


  /**
   * Sets the surface normal for the currect intersection point.
   * Note: This method will throw an error if this ShapeModel doesn't have and
   * intersection. Use the hasIntersection() method to verify before calling
   * this method.
   *
   * @see hasIntersection()
   *
   * @param a First coordinate value for the three dimensional surface normal.
   * @param b Second coordinate value for the three dimensional surface normal.
   * @param c Third coordinate value for the three dimensional surface normal.
   *
   */
  void ShapeModel::setNormal(const double a, const double b, const double c) {
    if (m_hasIntersection) {
      m_normal[0] = a;
      m_normal[1] = b;
      m_normal[2] = c;
      m_hasNormal = true;
    }
    else {
      std::string message = "No intersection point is known.  A normal cannot be set.";
      throw IException(IException::Unknown, message, _FILEINFO_);
    }
  }

  /**
   * Sets the local normal for the currect intersection point.
   * Note: This method will throw an error if this ShapeModel doesn't have and
   * intersection. Use the hasIntersection() method to verify before calling
   * this method.
   *
   * @see hasIntersection()
   *
   * @param a First coordinate value for the three dimensional local normal.
   * @param b Second coordinate value for the three dimensional local normal.
   * @param c Third coordinate value for the three dimensional local normal.
   *
   */
  void ShapeModel::setLocalNormal(const double a, const double b, const double c) {
    if (m_hasIntersection) {
      m_localNormal[0] = a;
      m_localNormal[1] = b;
      m_localNormal[2] = c;
      m_hasLocalNormal = true;
    }
    else {
      std::string message = "No intersection point is known.  A local normal cannot be set.";
      throw IException(IException::Unknown, message, _FILEINFO_);
    }
  }

  /**
   * Sets the shape name.
   *
   * @param name The name of the ShapeModel.
   *
   */
  void ShapeModel::setName(QString name) {
     *m_name = name;
  }


  /**
   * Gets the shape name.
   *
   * @return @b QString The name of the ShapeModel.
   *
   */
  QString ShapeModel::name() const{
    return *m_name;
  }


  /**
   * Sets the flag to indicate whether this ShapeModel has an intersection.
   *
   * @param b Indicates whether there is an intersection.
   *
   */
  void ShapeModel::setHasIntersection(bool b) {
    m_hasIntersection  = b;
    setHasNormal(false);
    setHasLocalNormal(false);
  }


  /**
   * Set surface intersection point.
   * @param surfacePoint Position coordinate for the surface point.
   *
   */
  void ShapeModel::setSurfacePoint(const SurfacePoint &surfacePoint) {
    *m_surfacePoint  = surfacePoint;

    // Update status of intersection and normal
    m_hasIntersection  = true;
    // Set normal as not calculated
    setHasNormal(false);
    setHasLocalNormal(false);
  }


  /**
   * Sets the flag to indicate whether this ShapeModel has a surface normal.
   *
   * @param status Indicates whether there is a normal.
   *
   */
  void ShapeModel::setHasNormal(bool status) {
    m_hasNormal = status;
  }

    /**
   * Sets the flag to indicate whether this ShapeModel has a local normal.
   *
   * @param status Indicates whether there is a normal.
   *
   */
  void ShapeModel::setHasLocalNormal(bool status) {
    m_hasLocalNormal = status;
  }


  /**
   * Convenience method to get pixel resolution (m/pix) at current intersection
   * point.
   *
   * @return @double The pixel resolution at the surface intersection.
   */
  double ShapeModel::resolution() {
    if (hasValidTarget() && m_hasIntersection) {
      return m_target->spice()->resolution();
    }
    else {
      std::string message = "No valid intersection point for computing resolution.";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }
  }

}
