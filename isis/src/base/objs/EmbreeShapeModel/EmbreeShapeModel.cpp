/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "EmbreeShapeModel.h"

#include <numeric>
#include <float.h>

#include <QtGlobal>
#include <QList>

#include "IException.h"
#include "IString.h"
#include "Latitude.h"
#include "Longitude.h"
#include "NaifStatus.h"
#include "Pvl.h"
#include "ShapeModel.h"
#include "Target.h"


using namespace std;

namespace Isis {

  /** 
   * Default constructor sets type to a TIN
   */
  EmbreeShapeModel::EmbreeShapeModel() 
      : ShapeModel(),
        m_targetShape(0),
        m_targetManager(0),
        m_tolerance(DBL_MAX),
        m_shapeFile("") {
    // defaults for ShapeModel parent class include:
    //     name = empty string
    //     surfacePoint = null sp
    //     hasIntersection = false
    //     hasNormal = false
    //     normal = (0,0,0)
    //     hasEllipsoidIntersection = false
    setName("Embree");
  }


  /**
   * @brief Constructor provided for instantiation from an ISIS cube
   *
   * This constructor is typically used for and ISIS cube that has been
   * initialized by spiceinit. The target shape file will be read from the Pvl
   * label.
   *
   * @param target Target object describing the observed body
   * @param pvl ISIS Cube label. The target shape file will be extracted from
   *            the Kernels group
   * @param targetManager The target shape manager that will manage the target shape
   */
  EmbreeShapeModel::EmbreeShapeModel(Target *target, Pvl &pvl,
                                     EmbreeTargetManager *targetManager) 
      : ShapeModel(target),
        m_targetShape(0),
        m_targetManager(targetManager),
        m_tolerance(DBL_MAX),
        m_shapeFile("") {

    // defaults for ShapeModel parent class include:
    //     name = empty string
    //     surfacePoint = null sp
    //     hasIntersection = false
    //     hasNormal = false
    //     normal = (0,0,0)
    //     hasEllipsoidIntersection = false

    setName("Embree");  // Really is used as type in the system at present!

    PvlGroup &kernels = pvl.findGroup("Kernels", Pvl::Traverse);

    if (kernels.hasKeyword("ElevationModel")) {
      m_shapeFile = QString::fromStdString(kernels["ElevationModel"]);
    }
    else { // if (kernels.hasKeyword("ShapeModel")) {
      m_shapeFile = QString::fromStdString(kernels["ShapeModel"]);
    }

    try {
      // Request the EmbreeTargetShape from the manager
      // If the shapefile is being used by something else this will get a pointer
      // to the same target shape, otherwise it creates a new one.
      m_targetShape = m_targetManager->create(m_shapeFile);
    }
    catch(IException &e) {
      std::string msg = "Cannot create a EmbreeShape from " + m_shapeFile.toStdString();
      throw IException(e, IException::User, msg, _FILEINFO_);
    }
  }


  /**
   * Constructor provided for instantiation from a file.
   *
   * @param target Target object describing the observed body
   * @param shapefile The targe shape file to use
   * @param targetManager The target shape manager that will manage the target shape
   */
  EmbreeShapeModel::EmbreeShapeModel(Target *target, const QString &shapefile,
                                     EmbreeTargetManager *targetManager) 
      : ShapeModel(target),
        m_targetShape(0),
        m_targetManager(targetManager),
        m_tolerance(DBL_MAX),
        m_shapeFile(shapefile) {

    // defaults for ShapeModel parent class include:
    //     name = empty string
    //     surfacePoint = null sp
    //     hasIntersection = false
    //     hasNormal = false
    //     normal = (0,0,0)
    //     hasEllipsoidIntersection = false

    setName("Embree");  // Really is used as type in the system at present!

    try {
      // Request the EmbreeTargetShape from the manager
      // If the shapefile is being used by something else this will get a pointer
      // to the same target shape, otherwise it creates a new one.
      m_targetShape = m_targetManager->create(m_shapeFile);
    }
    catch(IException &e) {
      std::string msg = "Cannot create a EmbreeShape from " + m_shapeFile.toStdString();
      throw IException(e, IException::User, msg, _FILEINFO_);
    }
  }


  /** 
   * Destructor that notifies the target shape manager that the target shape
   * is no longer in use.
   * 
   * @see EmbreeTargetManager::free
   */
  EmbreeShapeModel::~EmbreeShapeModel() {
    if ( m_targetManager && !m_shapeFile.isEmpty() ) {
      m_targetManager->free(m_shapeFile);
    }
  }


  /**
   * This method computes an intercept point given an observer location and
   * look direction using the Embree model. This also saves the surface normal.
   *
   * @param observerPos    Position of observer in body-fixed kilometers
   * @param lookDirection  Unit look direction from the observer
   *
   * @return @b bool If an intercept was found
   */
  bool EmbreeShapeModel::intersectSurface(std::vector<double> observerPos,
                                          std::vector<double> lookDirection) {
    // Remove any previous intersection
    clearSurfacePoint();

    // Create a ray from the observer in the look direction
    RTCMultiHitRay ray(observerPos, lookDirection);

    m_targetShape->intersectRay(ray);

    // If nothing was hit
    if (ray.lastHit < 0) {
      setHasIntersection(false);
    }
    else {
      // Get the intersection point and the surface normal
      RayHitInformation hitInfo = m_targetShape->getHitInformation(ray, 0);

      // Update the surface point and surface normal
      updateIntersection(hitInfo);
    }

    return hasIntersection();
  }


/**
 * @brief Compute intersection of surface vector direction from observer with 
 *        occulusion
 * 
 * This method computes a surface intersection point at a given latitude and
 * longitude. All surface points at that latitude and longitude are collected
 * and then the intersect that is non-occluded and closest to the observer is
 * saved. The unit surface normal is also saved.
 * 
 * If occlusion is not checked, then the closest intersection to the observer
 * is saved along with the unit surface normal.
 * 
 * @param lat The latitude of the surface intersection
 * @param lon The longitude of the surface intersection
 * @param observerPos The position of the observer for occlusion checks
 * @param backCheck If occlusion should be checked
 * 
 * @return @b bool If an intersection was found
 */
  bool EmbreeShapeModel::intersectSurface(const Latitude &lat, const Longitude &lon,
                                          const std::vector<double> &observerPos,
                                          const bool &backCheck) {
    // Remove any previous intersection
    clearSurfacePoint();

    // Create a ray from the origin through the surface point
    RTCMultiHitRay ray = latlonToRay(lat,lon);

    m_targetShape->intersectRay(ray);

    // If no intersections (unlikely for this case), we are done!
    if ( ray.lastHit < 0 ) {
      return ( false );
    }
    LinearAlgebra::Vector observer = LinearAlgebra::vector(observerPos[0],
                                                           observerPos[1],
                                                           observerPos[2]);

    // Sorts hits based on distance to the observer
    QVector<RayHitInformation> hits = sortHits(ray, observer);

    // If desired, check occlusion
    if ( backCheck ) {
      for (int i = 0 ; i < hits.size() ; i++) {
        LinearAlgebra::Vector obsToIntersection = hits[i].intersection - observer;
        LinearAlgebra::Vector lookVector = LinearAlgebra::normalize(obsToIntersection);

        // Cast a ray from the observer to the intersection and check if it is occluded
        RTCOcclusionRay obsRay;
        obsRay.ray.org_x = observerPos[0];
        obsRay.ray.org_y = observerPos[1];
        obsRay.ray.org_z = observerPos[2];
        obsRay.ray.dir_x = lookVector[0];
        obsRay.ray.dir_y = lookVector[1];
        obsRay.ray.dir_z = lookVector[2];
        obsRay.ray.tnear = 0.0;
        obsRay.ray.tfar = LinearAlgebra::magnitude(obsToIntersection) - 0.0005;
        obsRay.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
        obsRay.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        obsRay.hit.primID = RTC_INVALID_GEOMETRY_ID;
        obsRay.ray.mask = 0xFFFFFFFF;
        obsRay.ignorePrimID = hits[i].primID;

        // If the intersection point is not occluded,
        // then it is the closest intersection to the oberserver.
        if ( !m_targetShape->isOccluded(obsRay) ) {
          updateIntersection( hits[i] );
          break;
        }
      }
     }
    else {
      // If not testing for occlusion, take the hit closest to the observer
      updateIntersection( hits[0] );
    }

    return ( hasIntersection() );
  }


/**
 * @brief Compute intersection of surface point from observer with occulusion 
 *  
 * Compute the intersection point closest to a surface point. If occlusion is
 * checked, then the intersection that is both closest to the surface point and
 * non-occluded is saved. If occlusion is not checked, then the intersection
 * closest to the surface point is saved.
 * 
 * The unit surface normal is also saved when an intersection is saved.
 * 
 * 
 * @param surfpt Surface intercept point
 * @param observerPos Observer to check occlusion
 * @param backCheck If occlusion should be checked
 * 
 * @return @b bool If an intersection was found
 */
  bool EmbreeShapeModel::intersectSurface(const SurfacePoint &surfpt, 
                                          const std::vector<double> &observerPos,
                                          const bool &backCheck) {
    // Remove any previous intersection
    clearSurfacePoint();

    // Set up for finding all rays along origin vector through lat/lon surface point
    RTCMultiHitRay ray = pointToRay(surfpt);

    // Extend the ray to be 1.5 times the length of the SurfacePoint's radius
    ray.ray.tfar *= 1.5;

    m_targetShape->intersectRay(ray);

    // If no intersections (unlikely for this case), we are done!
    if ( ray.lastHit < 0 ) {
      return ( false );
    }
    // Convert the observer to a LinearAlgebra vector for occlusion testing
    LinearAlgebra::Vector observer = LinearAlgebra::vector(observerPos[0],
                                                           observerPos[1],
                                                           observerPos[2]);

    // Convert the surface point to a LinearAlgebra vector for sorting hits
    LinearAlgebra::Vector surfPoint(3);
    surfpt.ToNaifArray( &surfPoint[0] );

    // Sorts hits based on distance to the surface point
    QVector< RayHitInformation > hits = sortHits(ray, surfPoint);

    // If desired, check occlusion
    if ( backCheck ) {
      for (int i = 0 ; i < hits.size() ; i++) {
        LinearAlgebra::Vector obsToIntersection = hits[i].intersection - observer;
        LinearAlgebra::Vector lookVector = LinearAlgebra::normalize(obsToIntersection);

        // Cast a ray from the observer to the intersection and check if it is occluded
        RTCOcclusionRay obsRay;
        obsRay.ray.org_x = observerPos[0];
        obsRay.ray.org_y = observerPos[1];
        obsRay.ray.org_z = observerPos[2];
        obsRay.ray.dir_x = lookVector[0];
        obsRay.ray.dir_y = lookVector[1];
        obsRay.ray.dir_z = lookVector[2];
        obsRay.ray.tnear = 0.0;
        obsRay.ray.tfar = LinearAlgebra::magnitude(obsToIntersection);
        obsRay.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
        obsRay.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        obsRay.hit.primID = RTC_INVALID_GEOMETRY_ID;
        obsRay.ray.mask = 0xFFFFFFFF;
        obsRay.ignorePrimID = hits[i].primID; 

        // If the intersection point is no occluded,
        // then it is the closest intersection to the oberserver.
        if ( !m_targetShape->isOccluded(obsRay) ) {
          updateIntersection( hits[i] );
          break;
        }
      }
     }
    else {
      // If not testing for occlusion, take the hit closest to the surface point
      updateIntersection( hits[0] );
    }

    return ( hasIntersection() );
  }


  /**
   * Update the ShapeModel given an intersection and normal.
   * 
   * @param hitinfo The intersection and normal to internalize
   */
  void EmbreeShapeModel::updateIntersection(const RayHitInformation hitInfo) {
    // Flag that there is an intersection
    setHasIntersection(true);

    // Create the surfacepoint
    SurfacePoint intersectPoint;
    std::vector<double> intersectArray(3);
    std::copy( hitInfo.intersection.data().begin(),
               hitInfo.intersection.data().end(),
               intersectArray.begin() );
    intersectPoint.FromNaifArray( &intersectArray[0] );
    setSurfacePoint(intersectPoint);

    // Save the surface normal
    setNormal( hitInfo.surfaceNormal[0],
               hitInfo.surfaceNormal[1],
               hitInfo.surfaceNormal[2] );
  }


  /**
   * Flag that the ShapeModel does not have a surface point or normal.
   * 
   * @note This does not actually delete the surface point or normal stored by
   *       the parent ShapeModel class, it just sets the flags to false.
   */
  void EmbreeShapeModel::clearSurfacePoint() {
    ShapeModel::clearSurfacePoint();
    setHasNormal(false);
    setHasLocalNormal(false);
    return;
  }


  /**
   * Determine radius at a given lat/lon grid point.
   * 
   * @NOTE this call does not update the internal state of the intercept point.
   *       Use intersectSurface(lat, lon) for that.
   *
   * @param lat Latitude coordinate of grid point
   * @param lon Longitude coordinate of grid point
   *
   * @return @b Distance Radius value of the intercept grid point
   */
  Distance EmbreeShapeModel::localRadius(const Latitude &lat,
                                         const Longitude &lon) {

    // Create a ray from the origin to the surface point
    RTCMultiHitRay ray = latlonToRay(lat,lon);

    // Extend the ray to 2.5 times the maximum radius
    ray.ray.tfar *= 2.5;

    m_targetShape->intersectRay(ray);

    // If no intersections (unlikely for this case), we are done!
    if ( ray.lastHit < 0 ) {
      return ( Distance() );
    }
    // Otherwise, get the first intersection
    RayHitInformation hitInfo = m_targetShape->getHitInformation( ray, 0 );

    // Return the distance to the intersection
    return ( Distance( LinearAlgebra::magnitude(hitInfo.intersection),
                       Distance::Kilometers                            ) );
  }


  /**
   * Indicates that this shape model is not from a DEM. Since this method
   * returns false for this class, the Camera class will not calculate the
   * local normal using neighbor points.
   *
   * @return @b bool Indicates that this is not a DEM shape model.
   */
  bool EmbreeShapeModel::isDEM() const {
    return false;
  }


  /**
   * Check if the current internalized surface point is visible from an
   * observer position and look direction. A new intersection is calculated
   * using the observer and look direction. If, the distance between the
   * surface point and new intersection is less than the tolerance, then the
   * surface point is considered to be visible.
   * 
   * @param observerPos The position of the observer
   * @param lookDirection The look direction of the observer
   */
  bool EmbreeShapeModel::isVisibleFrom(const std::vector<double> observerPos,
                                       const std::vector<double> lookDirection)  {
    //TODO check if there is a saved intersection
    // Create a ray from the observer in the look direction
    RTCMultiHitRay ray(observerPos, lookDirection);

    m_targetShape->intersectRay(ray);

    // If nothing was hit, something went really wrong. Just return false.
    if (ray.lastHit < 0) {
      return false;
    }
    else {
      // Get the new intersection point
      RayHitInformation hitInfo = m_targetShape->getHitInformation(ray, 0);

      // Check the distance between the new intersection and the saved intersection
      std::vector<double> intersectVect(3);
      surfaceIntersection()->ToNaifArray( &intersectVect[0] );
      LinearAlgebra::Vector oldIntersection = LinearAlgebra::vector( intersectVect[0],
                                                                     intersectVect[1],
                                                                     intersectVect[2] );
      return ( LinearAlgebra::magnitude(oldIntersection - hitInfo.intersection) < getTolerance() );
    }
  }


  /**
   * @brief Compute the normal for a local region of surface points
   *
   * This method will calculate the surface normal of an assumed very local
   * region of points.  This method is provided to fullfil the specs of the
   * ShapeModel class but this approach is not the most efficent means to
   * accomplish this for a pre-exising intercept point.  See
   * setLocalNormalFromIntercept() for this.
   *
   * The ShapeModel class makes the assumption that the four pixel corners of the
   * center intercept point forms a plane from which a surface normal can be
   * computed.  For the Naif DSK plate model, we have already identified the plate
   * (see m_intercept) from the DSK plate model (m_model) of the intercept point
   * that provides it directly.  That is what setLocalNormalFromIntercept()
   * provides.
   *
   * So, this implementation will compute the centroid of the neighboring points
   * and make a determination if it intercepts the current intercept plate as
   * defined by m_intercept - if it is valid.  If it does not exist or does not
   * intercept the plate, a new intercept point is computed and returned here.
   *
   * @author 2017-03-27 Kris Becker
   *
   * @param neighborPoints Input body-fixed points to compute normal for
   */
  void EmbreeShapeModel::calculateLocalNormal(QVector<double *> neighborPoints) {
    // Sanity check
    if ( !hasIntersection() ) { // hasIntersection()  <==>  hasNormall()
      std::string mess = "Intercept point does not exist - cannot provide normal vector";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }

    return;
  }


  /** 
   * Return the surface normal of the ellipsoid as the default.
   */
  void EmbreeShapeModel::calculateDefaultNormal() {
    // ShapeModel (parent class) throws error if no intersection
     calculateSurfaceNormal();
  }


  /** 
   * Return the surface normal of the ellipsoid
   */
  void EmbreeShapeModel::calculateSurfaceNormal() {
    // ShapeModel (parent class) throws error if no intersection
    QVector<double> norm = ellipsoidNormal(); 
    setNormal(std::vector<double>(norm.begin(), norm.end()));// this takes care of setHasNormal(true);
    return;
  }


  /**
   * @brief Compute the true surface normal vector of an ellipsoid
   *
   * This routine is used instead of the one provided by the ShapeModel
   * implementation.  This is primarly because
   * ShapeModel::calculateEllipsoidalSurfaceNormal() it is only suitable for a
   * spheroid. This implementation is intended for irregular bodies so we expect
   * triaxial ellipsoids.
   *
   * @author 2017-03-27 Kris Becker
   *
   * @return QVector<double> Normal vector at the intercept point relative to
   *                             the ellipsoid (not the plate model)
   */
  QVector<double> EmbreeShapeModel::ellipsoidNormal()  {

    // Sanity check on state
    if ( !hasIntersection() ) {
       std::string msg = "An intersection must be defined before computing the surface normal.";
       throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if ( !surfaceIntersection()->Valid() ) {
       std::string msg = "The surface point intersection must be valid to compute the surface normal.";
       throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (!hasValidTarget()) {
       std::string msg = "A valid target must be defined before computing the surface normal.";
       throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Get the coordinates of the current surface point
    SpiceDouble pB[3];
    surfaceIntersection()->ToNaifArray(pB);

    // Get the body radii and compute the true normal of the ellipsoid
    QVector<double> norm(3);
    // need a case for target == NULL
    std::vector<Distance> stdRadii = targetRadii();
    QVector<Distance> radii = QVector<Distance>(stdRadii.begin(), stdRadii.end());
    NaifStatus::CheckErrors();
    surfnm_c(radii[0].kilometers(), radii[1].kilometers(), radii[2].kilometers(),
             pB, &norm[0]);
    NaifStatus::CheckErrors();

    return (norm);
  }


  /**
   * Computes and returns incidence angle, in degrees, given the illuminator position.
   * The surface normal vector is calculated using an ellipsoid, not the local
   * normal of the actual target shape.
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
  double EmbreeShapeModel::incidenceAngle(const std::vector<double> &illuminatorBodyFixedPosition) {

    // If there is already a normal save it, because it's probably the local normal
    std::vector<double> localNormal;
    bool hadNormal = hasNormal();
    if ( hadNormal ) {
      localNormal = normal();
    }

    // Calculate the ellipsoid surface normal
    calculateDefaultNormal();
    
    // Use ShapeModel to calculate the ellipsoid incidence angle
    double ellipsoidEmission = ShapeModel::incidenceAngle(illuminatorBodyFixedPosition);

    // If there's a saved normal, reset it
    if ( hadNormal ) {
      setNormal(localNormal);
    }

    // Return the ellipsoid incidence angle
    return ellipsoidEmission;
  }


  /**
   * Given a latitude and longitude, create a ray that goes from the origin of
   * the target through that latitude and longitude. The ray length is set to
   * the maximum distance in the scene to ensure that it intersects all points
   * at that latitude and longitude.
   * 
   * @param lat The latitude for the ray to pass through
   * @param lon The longitude for the ray to pass through
   * 
   * @return @b RTCMultiHitRay A ray from the origin of the target through the
   *                           given latitude and longitude.
   */
  RTCMultiHitRay EmbreeShapeModel::latlonToRay(const Latitude &lat, const Longitude &lon) const {
    // Initialize ray
    RTCMultiHitRay ray;
    ray.ray.org_x = 0.0;
    ray.ray.org_y = 0.0;
    ray.ray.org_z = 0.0;

    // Convert the lat, lon to a unit look direction
    double latAngle = lat.radians();
    double lonAngle = lon.radians();
    ray.ray.dir_x = cos(latAngle) * cos(lonAngle);
    ray.ray.dir_y = cos(latAngle) * sin(lonAngle);
    ray.ray.dir_z = sin(latAngle);

    // Set the ray's length to extend to the scene boundary
    ray.ray.tfar = m_targetShape->maximumSceneDistance();

    return (ray);
  }


  /**
   * Given a surface point, create a ray that goes from the origin of the
   * target to the surface point.
   * 
   * @param surfpt The surface point for the ray to pass through
   * 
   * @return @b RTCMultiHitRay A ray from the origin of the target to the
   *                           given surface point.
   */
  RTCMultiHitRay EmbreeShapeModel::pointToRay(const SurfacePoint &surfpt) const {
    // Setup everything but the direction component
    RTCMultiHitRay ray;
    ray.ray.org_x = 0.0;
    ray.ray.org_y = 0.0;
    ray.ray.org_z = 0.0;
    ray.ray.tnear = 0.0;
    ray.hit.instID[0] = RTC_INVALID_GEOMETRY_ID; // instance
    ray.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    ray.hit.primID = RTC_INVALID_GEOMETRY_ID; // primitive id (triangle id)
    ray.ray.mask = 0xFFFFFFFF;
    ray.lastHit = -1;

    // Get the vector from the origin to the surface point
    std::vector<double> surfVect(3);
    surfpt.ToNaifArray( &surfVect[0] );
    LinearAlgebra::Vector direction = LinearAlgebra::vector( surfVect[0],
                                                             surfVect[1],
                                                             surfVect[2] );

    // Normalize the vector and store it in the ray
    direction = LinearAlgebra::normalize(direction);
    ray.ray.dir_x = direction[0];
    ray.ray.dir_y = direction[1];
    ray.ray.dir_z = direction[2];

    // Extend the ray to the surface point
    ray.ray.tfar = surfpt.GetLocalRadius().kilometers();
    return (ray);
  }


  /**
   * Sort all intersections by a ray based on distance to a point. All
   * intersections are first stored in RayHitInformation objects and then
   * sorted.
   * 
   * @param ray The ray to sort intersections for. The ray must already be
   *            intersected with the target shape by EmbreeTargetShape::intersectRay.
   * @param observer The point to sort the intersections based on.
   * 
   * @return @b QVector<RayHitInformation> Vector containing the sorted
   *                                       intersections. The first
   *                                       intersection is the one closest to
   *                                       the given point.
   */
  QVector<RayHitInformation> EmbreeShapeModel::sortHits(RTCMultiHitRay &ray,
                                                        LinearAlgebra::Vector &observer) {
    int hitCount = ray.lastHit + 1;
    // Container that holds the hit (x, y, z) and distance from the origin
    // QMap sorts based upon the key, distance, so this handles sorting hits.
    QMap< double , RayHitInformation > hitsMap;
    for (int i = 0 ; i < hitCount ; i++) {
      RayHitInformation hitInfo = m_targetShape->getHitInformation( ray, i );
      hitsMap.insert( LinearAlgebra::magnitude(observer - hitInfo.intersection), hitInfo );
    }

    // Return sorted vector of hits
    return ( QVector< RayHitInformation >::fromList( hitsMap.values() ) );
  }


  /**
   * Get the tolerance used when checking if the stored surface point is
   * visible.
   * 
   * @return @b double The tolerance in kilometers
   * 
   * @see isVisibleFrom
   */
  double EmbreeShapeModel::getTolerance() const {
    return m_tolerance;
  }


  /**
   * Set the tolerance used when checking if the stored surface point is
   * visible.
   * 
   * @param tolerance The new tolerance in kilometers
   * 
   * @see isVisibleFrom
   */
  void EmbreeShapeModel::setTolerance(const double &tolerance) {
    m_tolerance = tolerance;
  }
}; // namespace Isis
