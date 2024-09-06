/**
 * @file
 * $Revision$
 * $Date$
 * $Id$
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are
 *   public domain. See individual third-party library and package descriptions
 *   for intellectual property information, user agreements, and related
 *   information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or
 *   implied, is made by the USGS as to the accuracy and functioning of such
 *   software and related material nor shall the fact of distribution
 *   constitute any such warranty, and no responsibility is assumed by the
 *   USGS in connection therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html
 *   in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */

#include "BulletShapeModel.h"

#include <numeric>

#include <QtGlobal>
#include <QVector>

#include "IException.h"
#include "Intercept.h"
#include "IString.h"
#include "Latitude.h"
#include "Longitude.h"
#include "NaifStatus.h"
#include "Pvl.h"
#include "ShapeModel.h"
#include "SpecialPixel.h"
#include "Statistics.h"
#include "SurfacePoint.h"
#include "Target.h"


using namespace std;

namespace Isis {

  /**
   * Default constructor that creates a shape model without any internal model.
   */
  BulletShapeModel::BulletShapeModel() : ShapeModel(), m_model(), m_tolerance(DBL_MAX),
                                         m_intercept(btVector3(0,0,0), btVector3(0,0,0)) {
    // defaults for ShapeModel parent class include:
    //     name = empty string
    //     surfacePoint = null sp
    //     hasIntersection = false
    //     hasNormal = false
    //     normal = (0,0,0)
    //     hasEllipsoidIntersection = false
    setName("Bullet");
  }

  /**
   * @brief Constructor provided for instantiation from an ISIS cube
   *
   * This constructor is typically used for and ISIS cube that has been
   * initialized by spiceinit.  The DEM name should be that of a NAIF DSK file.
   * This constructor will throw an exception if it fails to open the DSK file.
   *
   * @author 2014-02-12 Kris Becker
   *
   * @param target Target object describing the observed body
   * @param pvl    ISIS Cube label.  Extracts the name of the DEM from the Kernels
   *               group
   */
  BulletShapeModel::BulletShapeModel(Target *target, Pvl &pvl) :  
                                     ShapeModel(target), m_model(), m_tolerance(DBL_MAX),
                                     m_intercept(btVector3(0,0,0), btVector3(0,0,0)) {

    // defaults for ShapeModel parent class include:
    //     name = empty string
    //     surfacePoint = null sp
    //     hasIntersection = false
    //     hasNormal = false
    //     normal = (0,0,0)
    //     hasEllipsoidIntersection = false

    setName("Bullet");  // Really is used as type in the system at present!

    PvlGroup &kernels = pvl.findGroup("Kernels", Pvl::Traverse);

    QString shapefile;
    if (kernels.hasKeyword("ElevationModel")) {
      shapefile = QString::fromStdString(kernels["ElevationModel"]);
    }
    else { // if (kernels.hasKeyword("ShapeModel")) {
      shapefile = QString::fromStdString(kernels["ShapeModel"]);
    }

    QScopedPointer<BulletTargetShape> v_shape( BulletTargetShape::load(shapefile) );
    if (v_shape.isNull() ) {
      std::string mess = "Cannot create a BulletShape from " + shapefile.toStdString();
      throw IException(IException::User, mess, _FILEINFO_);
    }
    
    // Attempt to initialize the DSK file - exception ensues if errors occur
    // error thrown if ShapeModel=Null (i.e. Ellipsoid)
    m_model.reset(new BulletWorldManager(shapefile));
    m_model->addTarget( v_shape.take() );
  }


  /**
   * Constructor for creating new shape model from an existing target shape.
   * 
   * @param shape The existing target shape to use with this shape model
   * @param target Target object describing the observed body
   * @param pvl Unused
   * 
   * @author 2014-02-12 Kris Becker
   */
  BulletShapeModel::BulletShapeModel(BulletTargetShape *shape, Target *target, Pvl &pvl) : 
                                     ShapeModel(target), m_model(), m_tolerance(DBL_MAX), 
                                     m_intercept(btVector3(0,0,0), btVector3(0,0,0))  {
    // Attempt to initialize the DSK file - exception ensues if errors occur
    // error thrown if ShapeModel=Null (i.e. Ellipsoid)
    btAssert ( shape != 0 );
    m_model.reset( new BulletWorldManager( shape->name() ) );
    m_model->addTarget( shape );
    setName("Bullet");  // Really is used as type in the system at present!
  }

  /**
   * Constructor for creating a new shape model an existing Bullet World.
   * Assumes the world contains at least one active object
   * 
   * @param model The existing Bullet world model to use
   * @param target Target object describing the observed body
   * @param pvl Unused
   * 
   * @author 2014-02-12 Kris Becker
   */
  BulletShapeModel::BulletShapeModel(BulletWorldManager *model, Target *target, Pvl &pvl) : 
                                     ShapeModel(target), m_model(model), m_tolerance(DBL_MAX), 
                                     m_intercept(btVector3(0,0,0), btVector3(0,0,0))  {

    // TODO create valid Target
    // Using this constructor, ellipsoidNormal(),
    // calculateSurfaceNormal(), and setLocalNormalFromIntercept()
    // methods can not be called
  }


  /** 
   * Destructor. Cleanup is handled by Bullet routines and QPointers.
   */
  BulletShapeModel::~BulletShapeModel() { }


  /**
   * Returns the occlusion tolerance in kilometers.
   * 
   * @return @b double The occlusion tolerance in kilometers.
   */
  double BulletShapeModel::getTolerance() const {
    return ( m_tolerance );
  }


  /**
   * Sets the occlusion tolerance.
   * 
   * @param tolerance The new occlusion tolerance in kilometers.
   */
  void   BulletShapeModel::setTolerance(const double &tolerance) {
    m_tolerance = tolerance;
  }


  /**
   * This method computes a DEM intercept point given an observer location and
   * look direction using the Bullet model.
   *
   * @author 2014-02-13 Kris Becker
   *
   * @param observerPos    Position of observer in body fixed coordiates
   * @param lookDirection  Look direction (ray) from the observer
   *
   * @return @b bool If an intersection was found and saved.
   */
  bool BulletShapeModel::intersectSurface(std::vector<double> observerPos,
                                          std::vector<double> lookDirection) {

    clearSurfacePoint();
    btVector3 observer(observerPos[0], observerPos[1], observerPos[2]);
    btVector3 lookdir(lookDirection[0], lookDirection[1], lookDirection[2]);
    btVector3 rayEnd = castLookDir(observer, lookdir);
    BulletClosestRayCallback result(observer, rayEnd);
    bool success = m_model->raycast(observer, rayEnd, result);
    updateShapeModel(result);
    return ( success );
  }

/**
 * Compute the intersection at a specified latitude and longitude. The
 * intersection can also be checked for occlusion from an observer.
 *
 * If more than one intersection exist at the latitude and longitude, then the
 * intersection closest to the observer is saved. If occlusion is being
 * checked, then the closest non-occluded intersection is saved.
 * 
 * @author 2017-03-27 Kris Becker
 * 
 * @param lat The latitude to compute the intersection at.
 * @param lon The longitude to compute the intersection at.
 * @param observerPos The observer used for occlusion and deciding between
 *                    multiple intersections.
 * @param checkOcclusion If occlusion should be checked.
 * 
 * @return @b bool If an intersection was found and saved.
 */
  bool BulletShapeModel::intersectSurface(const Latitude &lat, const Longitude &lon,
                                          const std::vector<double> &observerPos,
                                          const bool &checkOcclusion) {

    // Set up for finding all rays along origin vector through lat/lon surface point
    clearSurfacePoint();
    btVector3 origin(0.0, 0.0, 0.0);
    btVector3 lookdir = latlonToVector(lat, lon);
    btVector3 rayEnd = castLookDir(origin, lookdir);
    BulletAllHitsRayCallback results( origin, rayEnd, false);

    // If no intersections (unlikely for this case), we are done!
    if ( !m_model->raycast(origin, rayEnd, results) ) {
      return ( false );
    }

    // Sort the intersections based on distance to the observer
    btVector3 observer(observerPos[0], observerPos[1], observerPos[2]);
    QVector<BulletClosestRayCallback> points = sortHits(results, observer);

    // If occlusion is being checked
    if ( checkOcclusion ) {
      for (int i = 0 ; i < points.size() ; i++) {

        // Check for occlusion
        //   If the hit is occluded then move on.
        //   Otherwise, it is the closest non-occluded point so take it.
        BulletClosestRayCallback &hit = points[i];
        if ( !isOccluded(hit, observer) ) {
          updateShapeModel( hit );
          break;
        }
      }
    }

    // If occlusion is not being checked, take the intersection closest to the observer
    else {
      updateShapeModel( points[0] );
    }

    // Is set by the update routine
    return ( hasIntersection() );
  }


/**
 * Compute the intersection at surface point. The intersection can also be
 * checked for occlusion from an observer.
 * 
 * The intersection is found by casting a ray from the center of the body
 * through the surface point. If multiple intersections are found, then the
 * intersection closest to the surface point is saved. If occlusion is being
 * checked, then the closest non-occluded point is saved.
 *  
 * @author 2017-03-27 Kris Becker
 * 
 * @param surfpt Surface point to find the intersection at
 * @param observerPos The observer used for occlusion and deciding between
 *                    multiple intersections.
 * @param checkOcclusion If occlusion should be checked.
 * 
 * @return @b bool If an intersection was found and saved.
 */
  bool BulletShapeModel::intersectSurface(const SurfacePoint &surfpt, 
                                          const std::vector<double> &observerPos,
                                          const bool &checkOcclusion) {

    // Set up for finding all rays along origin vector through lat/lon surface point
    clearSurfacePoint();
    btVector3 origin(0.0, 0.0, 0.0);
    btVector3 surfPointVec = pointToVector(surfpt);
    btVector3 rayEnd = castLookDir(origin, surfPointVec);
    BulletAllHitsRayCallback results(origin, rayEnd, false);

    // Cast the ray to find all intersections
    //   If no intersections (unlikely for this case), we are done!
    if ( !m_model->raycast(origin, rayEnd, results) ) {
      return ( false );
    }

    // Sort the intersections based on distance to the surface point
    QVector<BulletClosestRayCallback> points = sortHits(results, surfPointVec);

    // If occlusion is being checked
    if ( checkOcclusion ) {
      btVector3 observer(observerPos[0], observerPos[1], observerPos[2]);
      for (int i = 0 ; i < points.size() ; i++) {

        // Check for occlusion
        //   If the hit is occluded then move on.
        //   Otherwise, it is the closest non-occluded point so take it.
        BulletClosestRayCallback &hit = points[i];
        if ( !isOccluded(hit, observer) ) {
          updateShapeModel( hit );
          break;
        }
      }
    }

    // If occlusion is not being checked, take the intersection closest to the observer
    else {
      updateShapeModel( points[0] );
    }

    // Is set by the update routine
    return ( hasIntersection() );

  }


  /**
   * Check if an intersection is occluded from an observer.
   * 
   * @param hit The callback with the intersection to check
   * @param observer The observer position for checking occlusion
   * 
   * @return @b bool If the intersection is occluded from the observer.
   */
  bool BulletShapeModel::isOccluded(const BulletClosestRayCallback &hit,
                                    const btVector3 &observer) const {
    // If the callback does not have an intersection return true
    if ( !hit.isValid() ) {
      return true;
    }
    
    // Check if the emission angle is greater than 90 degrees
    // If true, then it is occluded, if false then unknown
    btVector3 psB = (observer - hit.point()).normalized();
    btScalar  angle = std::acos( hit.normal().dot( psB ) ) * RAD2DEG;
    if ( std::fabs( angle ) > 90.0 - DBL_MIN) {
      return true;
    }

    // Ray cast from the observer to the intersection
    //   If we have an intersection, test for occlusion
    BulletClosestRayCallback results( observer, hit.point() );
    if ( !m_model->raycast(observer, hit.point(), results) ) {
      return false;
    }

    // Is this intersection the same as the previous intersection?
    if ( results.isVisible( hit, getTolerance() ) ) {
      return false;
    }

    return true;
  }


  /**
   * Set the internal surface point. A new intersection based on the surface
   * point will be computed and saved.
   * 
   * @param surfacePoint The new surface point
   */
  void BulletShapeModel::setSurfacePoint(const SurfacePoint &surfacePoint) {
    std::vector<double> fakepos(3, 0.0);
    (void) intersectSurface(surfacePoint, fakepos, false);
  }


  /**
   * Clear the saved surface point and reset the saved intersection.
   */
  void BulletShapeModel::clearSurfacePoint() {
    updateShapeModel( BulletClosestRayCallback() );
  }


  /**
   * Compute the radius of the body at a lat/lon point. A ray is cast from
   * the center of the body through the lat/lon point. The intersection closest
   * to the center of the body is used to compute the radius.
   * 
   * NOTE this call does not update the internal state of the intercept point.
   * Use intersectSurface(lat, lon) for that.
   *
   * @author 2017-03-27 Kris Becker
   *
   * @param lat Latitude coordinate of grid point
   * @param lon Longitude coordinate of grid point
   *
   * @return @b Distance Radius value of the intercept grid point. If no
   *                     intersection is found, a default, invalid, Distance
   *                     object is returned.
   */
  Distance BulletShapeModel::localRadius(const Latitude &lat,
                                         const Longitude &lon) {

    // Cast a ray from the origin through the surface point at the input lat/lon
    btVector3 origin(0.0, 0.0, 0.0);
    btVector3 lookdir = latlonToVector(lat, lon);
    btVector3 rayEnd = castLookDir(origin, lookdir);

    BulletAllHitsRayCallback result(origin, rayEnd, false);
    if ( m_model->raycast(origin, rayEnd, result) ) {
      BulletClosestRayCallback hit = result.hit();
      return ( Distance(hit.point().length(), Distance::Kilometers) );
    }
    return ( Distance() );
  }


  /**
   * @brief Set the normal vector to the intercept point normal
   *
   * This method will reassign the ShapeModel normal to the current intercept 
   * point shape (which is a triangular plate) normal.  If an intercept point is 
   * not defined, an error will ensue. 
   *
   * @author 2017-03-27 Kris Becker
   */
  void BulletShapeModel::setLocalNormalFromIntercept()  {

    // Sanity check
    if ( !hasIntersection() ) { // hasIntersection()  <==>  !m_intercept.isNull()
      std::string mess = "Intercept point does not exist - cannot provide normal vector";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }

    btVector3 normal = m_intercept.normal();
    setLocalNormal(normal[0], normal[1], normal[2]);
  }


  /**
   * Indicates that this shape model is not from a DEM. Since this method
   * returns false for this class, the Camera class will not calculate the
   * local normal using neighbor points.
   *
   * @return @b bool Indicates that this is not a DEM shape model.
   */
  bool BulletShapeModel::isDEM() const {
    return false;
  }


  /**
   * Check if the saved intercept is visible from a observer with a given look
   * direction.
   * 
   * @param observerPos The position of the observer in body-fixed kilometers.
   * @param lookDirection The observer look direction inf body-fixed coordinates.
   * 
   * @return @b bool If the saved intercept is visible.
   */
  bool BulletShapeModel::isVisibleFrom(const std::vector<double> observerPos,
                                       const std::vector<double> lookDirection)  {

    if ( !m_intercept.isValid() ) return (false);

    // Check if the emission angle is greater than 90 degrees
    // If true, then it is occluded, if false then unknown
    btVector3 observer(observerPos[0], observerPos[1], observerPos[2]);
    btVector3 psB = (observer - m_intercept.point()).normalized();
    btScalar  angle = std::acos( m_intercept.normal().dot( psB ) ) * RAD2DEG;
    if ( std::fabs( angle ) > 90.0 - DBL_MIN) {
      return false;
    }

    btVector3 rayEnd = castLookDir( observer,  btVector3(lookDirection[0], lookDirection[1], lookDirection[2]) );
    BulletClosestRayCallback results(observer, rayEnd);
    (void) m_model->raycast(observer, rayEnd, results);
    return ( m_intercept.isVisible( results, getTolerance() ) );
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
  void BulletShapeModel::calculateLocalNormal(QVector<double *> neighborPoints) {

    // Sanity check
    if ( !hasIntersection() ) { // hasIntersection()  <==>  !m_intercept.isNull()
      std::string mess = "Intercept point does not exist - cannot provide normal vector";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }

    setLocalNormalFromIntercept();
  }


  /** 
   * Calculate the surface normal of the ellipsoid as the default
   */
  void BulletShapeModel::calculateDefaultNormal() {
    // ShapeModel (parent class) throws error if no intersection
     calculateSurfaceNormal();
  }

  /**
   * compute the ellipsoid surface normal of the target
   */
  void BulletShapeModel::calculateSurfaceNormal() {
    // ShapeModel (parent class) throws error if no intersection
    QVector<double> norm = ellipsoidNormal();
    setNormal(std::vector<double>(norm.begin(), norm.end()));// this takes care of setHasNormal(true);
  }


  /**
   * @brief Compute the true surface normal vector of an ellipsoid
   *
   * This routine is used instead of the one provided by the ShapeModel
   * implementation.  This is primarly because
   * ShapeModel::calculateEllipsoidalSurfaceNormal() is only suitable for a
   * spheroid. This implementation is intended for irregular bodies so we expect
   * triaxial ellipsoids.
   *
   * @author 2017-03-27 Kris Becker
   *
   * @return @b QVector<double> Normal vector at the intercept point relative
   *                            to the ellipsoid (not the plate model)
   */
  QVector<double> BulletShapeModel::ellipsoidNormal()  {

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
   * Returns a direct reference to the Bullet world that contains the target
   * shape and can perform ray casts.
   */
  const BulletWorldManager &BulletShapeModel::model() const {
    return (*m_model);
  }


  /**
   * Returns The maximum distance in the Bullet world. This can be used to
   * scale rays such that they will be guaranteed to intersect everything along
   * their path.
   * 
   * @return @b btScalar The maximum distance in the bullet world in kilometers.
   */
  btScalar BulletShapeModel::maxDistance() const {
    return m_model->getTarget()->maximumDistance();
  }


  /**
   * Compute the end point of a ray based on an observer and look direction.
   * The end point is computed as the observer position plus a scaled look
   * direction. Look direction is scaled to ensure that the ray intersects
   * every point along its path.
   * 
   * @param observer The observer position or origin of the ray.
   * @param lookDir The look direction or pointing of the ray.
   * 
   * @return @b btVector3 The end point of the ray for ray casting.
   */
  btVector3  BulletShapeModel::castLookDir(const btVector3 &observer, 
                                           const btVector3 &lookdir) const {
    btScalar lookScale = observer.length() + maxDistance();
    return ( observer + lookdir.normalized() * lookScale );
  }


  /**
   * Convert a pair of latitude and longitude values into a unit vector
   * pointing from the origin of the body towards the surface point(s) at the
   * latitude and longitude.
   * 
   * @param lat The latitude of the surface point
   * @param lon The longitude of the surface point
   * 
   * @return @b btVector3 A unit vector pointing from the origin to the surface
   *                      point at the latitude and longitude.
   */
  btVector3 BulletShapeModel::latlonToVector(const Latitude &lat, const Longitude &lon) const {
    double latAngle = lat.radians();
    double lonAngle = lon.radians();
    btVector3 lookDir( cos(latAngle) * cos(lonAngle),
                       cos(latAngle) * sin(lonAngle),
                       sin(latAngle) );
    return ( lookDir );
  }


  /**
   * Convert a surface point into a vector.
   * 
   * @param surfpt The surface point to convert.
   * 
   * @return @b btVector3 The surface point's location in body-fixed kilometers.
   */
  btVector3 BulletShapeModel::pointToVector(const SurfacePoint &surfpt) const {
    btVector3 point;
    surfpt.ToNaifArray( &point[0] );
    return ( point );
  }


  /**
   * Convert a vector into a surface point.
   * 
   * @param point The vector to convert
   * 
   * @return @b SurfacePoint The surface point on the body.
   */
  SurfacePoint BulletShapeModel::makeSurfacePoint(const btVector3 &point) const {
    SurfacePoint surfpt;
    surfpt.FromNaifArray( &point[0] );
    return ( surfpt );
  }


  /**
   * Sort the hits in an AllHitsRayCallback based on distance to a point.
   * 
   * @param hits The hits callback to sort hits from
   * @param sortPoint The point to sort hits based on. Hits will be sorted
   *                  in increasing distance from this point.
   * 
   * @return @b QVector<BulletClosestRayCallback> A vector of hits in order of
   *                                              increasing distance from the sortPoint.
   */
  QVector<BulletClosestRayCallback> BulletShapeModel::sortHits(const BulletAllHitsRayCallback &hits,
                                                               const btVector3 &sortPoint) const {
    QMap<btScalar, BulletClosestRayCallback> sortMap;
    for (int i = 0 ; i < hits.size() ; i++) {
      BulletClosestRayCallback hit = hits.hit(i);
      sortMap.insert( hit.distance(sortPoint), hit );
    }
    return ( QVector<BulletClosestRayCallback>::fromList( sortMap.values() ) );
  }


  /**
   * @brief Update shape model - carefully!! 
   *  
   * This is coded so that users can safely call the setSurfacePoint() and 
   * clearSurfacePoint() routines.
   * 
   * @author 2017-03-27 Kris Becker
   * 
   * @param result The ray callback to update the shape model with. If the ray
   *               callback does not have an intersection, then the shape model
   *               will be flagged as not having a surface point or normal.
   * 
   */
  void BulletShapeModel::updateShapeModel(const BulletClosestRayCallback &result) {
    m_intercept = result;
    if ( m_intercept.isValid() ) {
      ShapeModel::setSurfacePoint( makeSurfacePoint(m_intercept.point()) ); // sets ShapeModel::m_hasIntersection=t, ShapeModel::m_hasNormal=f

      btVector3 normal = m_intercept.normal();
      setLocalNormal(normal[0], normal[1], normal[2]);
    }
    else {
      ShapeModel::clearSurfacePoint();
      setHasLocalNormal(false);
    }

    return;
  }
}; // namespace Isis
