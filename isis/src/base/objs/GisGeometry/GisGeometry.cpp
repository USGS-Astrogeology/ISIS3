/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "GisGeometry.h"

// Qt library
#include <QDebug>
#include <QScopedPointer>
#include <QString>

// geos library
#include <geos_c.h>

// other ISIS
#include "Cube.h"
#include "IException.h"
#include "ImagePolygon.h"
#include "GisTopology.h"
#include "SpecialPixel.h"

//#define DISABLE_PREPARED_GEOMETRY 1

using namespace std;

namespace Isis {

  /**
   * Fundamental constructor of an empty object
   */
  GisGeometry::GisGeometry() : m_type(None), m_geom(0), m_preparedGeom(0) {
    // Must ensure GEOS is initialized
    GisTopology::instance();
  }


  /**
   * @brief Construct a point geometry
   *
   * This constructor will create a point geometry. Note this can either be used
   * to create a geometric geometry with a longitude/latitude or a grid geometry
   * in any system with an X/Y value. It is up to the caller to maintain the
   * coordinate system.
   *
   * @param xlongitude X or longitude coordinate
   * @param ylatitude  Y or latitude coordinate
   */
  GisGeometry::GisGeometry(const double xlongitude, const double ylatitude) :
                           m_type(GeosGis), m_geom(0), m_preparedGeom(0){

    GisTopology::instance();
    m_geom = makePoint(xlongitude, ylatitude);
    m_preparedGeom = makePrepared(this->m_geom);
    return;
  }


  /**
   * @brief Create a geometry from a cube file.
   *
   * This constructor will read the contents of the Polygon blob of an ISIS cube
   * file and create a geometry from its contents.
   *
   * @param cube  Cube object to create the geometry from
   */
  GisGeometry::GisGeometry(Cube &cube) : m_type(IsisCube), m_geom(0), m_preparedGeom(0) {

    GisTopology::instance();
    m_geom = fromCube(cube);
    m_preparedGeom = makePrepared(this->m_geom);
    return;
  }


  /**
   * @brief Create a geometry from a character WKT/WKB source
   *
   * @param gisSource String containing the text representation of a GIS
   *                  geometry. This can be either a WKT or WKB formatted string
   * @param t Type of GisGeometry - WKT or WKB
   *
   */
  GisGeometry::GisGeometry(const QString &gisSource, const GisGeometry::Type t) :
                           m_type(t), m_geom(0), m_preparedGeom(0) {
    GisTopology *gis(GisTopology::instance());
    if (WKT == t) {
      m_geom = gis->geomFromWKT(gisSource);
    }
    else if (WKB == t) {
      m_geom = gis->geomFromWKB(gisSource);
    }
    else if (IsisCube == t) {
      Cube cube;
      cube.open(gisSource);
      m_geom = fromCube(cube);
    }
    else {
      throw IException(IException::Programmer,
                       QString("Unknown GIS type given [%1]").arg(typeToString(t)),
                       _FILEINFO_);
    }

    //  Get the prepared geometry
    m_preparedGeom = makePrepared(this->m_geom);
    m_type = t;
    return;
  }


  /**
   * @brief Create a geometry from another geometry by cloning
   *
   * This contructor uses the GISTopology clone method to generate a new
   * geometry.
   *
   * @param geom  GISGeometry to create new geometry from
   */
  GisGeometry::GisGeometry(const GisGeometry &geom) : m_type(geom.m_type),
                                                      m_geom(0),
                                                      m_preparedGeom(0) {

    GisTopology *gis(GisTopology::instance());
    if ( geom.isDefined() ) {
      m_geom = gis->clone(geom.m_geom);
      m_preparedGeom = makePrepared(this->m_geom);
    }
  }


  /**
   * @brief Create a GISGeometry directly from a GEOS-C GEOSGeometry
   *
   * This constructor will create a new GISGeometry object that takes ownership
   * from the caller and is managed for the life of this new object.
   *
   * @param geom GEOSGeometry instance to use
   */
  GisGeometry::GisGeometry(GEOSGeometry *geom) : m_type(GeosGis), m_geom(geom),
                                                 m_preparedGeom(0) {
    GisTopology::instance();
    m_preparedGeom = makePrepared(this->m_geom);
    return;
  }


  /**
   * Destructor
   */
  GisGeometry::~GisGeometry() {
    destroy();
  }


  /**
   * @brief Assignment operator for GISGeomtries
   *
   * This assignment operator essentially clones the right side geomemtry
   *
   * @param geom Geometry to assign to current object
   * @return GisGeometry New geometry
   */
  GisGeometry &GisGeometry::operator=(GisGeometry const &geom) {
    if ( this != &geom ) {
      destroy();
      m_type = geom.m_type;
      if ( geom.isDefined() ) {
        GisTopology *gis(GisTopology::instance());
        m_geom = gis->clone(geom.m_geom);
        m_preparedGeom = makePrepared(this->m_geom);
      }
    }
    return (*this);
  }


  /**
   * @brief Set the geometry directly taking ownership
   *
   * This method will replace the current geoemetry with the geom parameter
   * contents. The existing contents of this object is destroyed before taking
   * ownership of the geom paramter object.
   *
   * @param geom GEOSGeometry to incoporate into this object
   */
  void GisGeometry::setGeometry(GEOSGeometry *geom) {
    destroy();
    m_geom = geom;
    m_preparedGeom = makePrepared(this->m_geom);
    return;
  }


  /**
   * Determines if the current geometry is valid
   *
   * @return bool True if contents are valid, otherwise false
   */
  bool GisGeometry::isDefined() const {
    return (m_geom != 0);
  }


  /**
   * Determines validity of the geometry contained in this object
   *
   *  First determines if it contains a geometry and then validates with the
   *  GEOS toolkit.
   *  
   * @return bool True if valid, false if invalid or non-existant 
   *  
   * @history 2018-07-29 Kris Becker - If the geometry is invalid, it throws an 
   *                        exception. Catch all exceptions and return proper
   *                        status.
   */  
  bool GisGeometry::isValid() const {
    if (!isDefined()) {
      return (false);
    }
    
    int valid(0);
    try {
        valid = GEOSisValid(this->m_geom);
    } catch (...) {
        valid = 0;
    }
    return (1  == valid);
  }


  /**
   * Returns a string describing reason for invalid geometry
   *
   * @return QString Description of the reason the geometry is invalid
   */
  QString GisGeometry::isValidReason() const {
    QString result = "Not defined!";
    if ( isDefined() ) {
      GisTopology *gis(GisTopology::instance());
      char *reason = GEOSisValidReason(this->m_geom);
      result = reason;
      gis->destroy(reason);
    }
    return (result);
  }


  /**
   * Returns the type (origin) of the geometry
   *
   * @return GisGeometry::Type Enum type of geometry origin
   */
  GisGeometry::Type GisGeometry::type() const {
    return (m_type);
  }


  /**
   * Returns enum representation of the geometry origin from string type
   *
   * @see type(QString)
   * @param gstrType  Character representation of geometry origin
   * @return GisGeometry::Type Enum type of origin
   */
  GisGeometry::Type GisGeometry::type(const QString &gstrType) {
    QString gtype = gstrType.toLower();
    if ("wkt" == gtype) return (WKT);
    if ("wkb" == gtype) return (WKB);
    if ("cube" == gtype) return (IsisCube);
    if ("isiscube" == gtype) return (IsisCube);
    if ("geometry" == gtype) return (GeosGis);
    if ("geosgis" == gtype) return (GeosGis);
    if ("gis" == gtype) return (GeosGis);
    if ("geos" == gtype) return (GeosGis);
    return (None);
  }


  /**
   * Returns the type of the Geometry as a QString.
   *
   * @see type(QString)
   * @param gstrType Enum type of origin
   * @return GisGeometry::Type Character representation of geometry or origin
   */
  QString GisGeometry::typeToString(const GisGeometry::Type &type) {
    if (WKT == type) return "WKT";
    if (WKB == type) return "WKB";
    if (IsisCube == type) return "IsisCube";
    if (GeosGis == type) return "GeosGis";
    return "None";
  }


  /**
   * Returns the GEOSGeometry object to extend functionality
   *
   * @return GEOSGeometry Pointer to GEOSGeometry structure
   */
  const GEOSGeometry *GisGeometry::geometry() const {
    return (m_geom);
  }


  /**
   * @brief Returns special GEOS prepared geometry if it exists
   *
   * This method will return a pointer to the prepared version of the
   * GEOSGeometry data. Caller should test for a NULL pointer as it may be
   * disabled or non-existant.
   *
   * @return GEOSPreparedGeometry Pointer to prepared geometry
   */
  const GEOSPreparedGeometry *GisGeometry::preparedGeometry() const {
    return (m_preparedGeom);
  }


  /**
   * @brief Clones the contents of this geometry to a new instance
   *
   * This method will clone the contents of this geometry and return a new
   * instance to the caller.
   *
   * @return GisGeometry Geometry to clone
   */
  GisGeometry *GisGeometry::clone() const {
    if (!isDefined()) {
      return (new GisGeometry());
    }

    GisTopology *gis(GisTopology::instance());
    QScopedPointer<GisGeometry> geom(new GisGeometry());

    geom->m_type = m_type;
    geom->m_geom = gis->clone(m_geom);
    geom->m_preparedGeom = makePrepared(geom->m_geom);
    return (geom.take());
  }


  /**
   * Tests for a defined but empty geometry
   *
   * @return bool True if empty, false if has content
   */
  bool GisGeometry::isEmpty() const {
    if ( !isValid() ) {
      return (true);
    }
    return (1 == GEOSisEmpty(this->m_geom));
  }


  /**
   * @brief Computes the area of a geometry
   *
   * This method will compute the area of a geometry. Note the area is in the
   * units of the coordinates. For example, if the coordinates of the geometry
   * are in latitude/longitude, then the units are in degrees. Callers must
   * maintain the units of the coordinates.
   *
   * Point geometries will have 0 area.
   *
   * @return double Area of the geometry in units of data coordinates of the
   *                geometry
   */
  double GisGeometry::area( ) const {
    if ( !isValid() ) {
      return (0.0);
    }

    int result = 0;
    double gisArea = 0.0;
    result = GEOSArea(this->m_geom, &gisArea);
    if (1 != result) {
      gisArea = 0.0;
    }
    return (gisArea);
  }


  /**
   * @brief Computes the length of a geometry
   *
   * This method will compute the length of a geometry. This is suitable for
   * Linestring and spatial geometries.
   *
   * @return double Length of the geometry in units of the coordinates
   */
  double GisGeometry::length( ) const {
    if ( !isValid() ) {
      return (0.0);
    }

    int result = 0;
    double gisLength = 0.0;
    result = GEOSLength(this->m_geom, &gisLength);
    if (1 != result) {
      gisLength = 0.0;
    }
    return (gisLength);
  }


  /**
   * @brief Computes the distance between two geometries
   *
   * This method computes the distance between two geometries. Refer to the
   * GEOS documentation as to the details of this computation.
   *
   * @param target Target geometry to compute distance to
   * @return double Distance in units of geometry coordinates
   */
  double GisGeometry::distance(const GisGeometry &target) const {
    if ( !isValid() ) {
      return (false);
    }
    if ( !target.isValid() ) {
      return (false);
    }

    int result = 0;
    double dist = Null;
    result = GEOSDistance(this->m_geom, target.geometry(), &dist);
    if ( 1 != result ) {
      dist = Null;
    }
    return (dist);
  }


  /**
   * Get number of points in geometry
   *
   * @return int Number of points contained in the geometry
   */
  int GisGeometry::points() const {
    if (!isValid() ) { return (0); }

    int ngeoms =  GEOSGetNumGeometries(m_geom);
    int npoints = 0;
    for (int i = 0 ; i < ngeoms ; i++) {
      npoints += GEOSGetNumCoordinates(GEOSGetGeometryN(m_geom, i));
    }

    return ( npoints );
  }


  /**
   * @brief Computes a new geometry from the intersection of the two geomtries
   *
   * This method will compute the union of two geometries and return a new
   * geometry that represents the combination of them.
   *
   * @param target Other geometry to combine by union opertor
   * @return bool  True if operation is successful, false otherwise
   */
  bool GisGeometry::intersects(const GisGeometry &target) const {
    if ( !isValid() ) {
      return (false);
    }
    if ( !target.isValid() ) {
      return (false);
    }

    int result = 0;
    if ( 0 != this->m_preparedGeom) {
      result = GEOSPreparedIntersects(this->m_preparedGeom, target.geometry());
    }
    else {
      result = GEOSIntersects(this->m_geom, target.geometry());
    }

    return (1 == result);
  }


  /**
   * Test if the target geometry is contained within this geometry
   *
   * @param target Other geometry to test
   * @return bool  True if target is contained with this geometry, false
   *               otherwise
   */
  bool GisGeometry::contains(const GisGeometry &target) const {
    if ( !isValid() ) {
      return (false);
    }
    if ( !target.isValid() ) {
      return (false);
    }

    int result = 0;
    if ( 0 != this->m_preparedGeom) {
      result = GEOSPreparedContains(this->m_preparedGeom, target.geometry());
    }
    else {
      result = GEOSContains(this->m_geom, target.geometry());
    }

    return (1 == result);
  }


  /**
   * Tests for disjoint geometries
   *
   * @param target Other geometry to test
   * @return bool  True if geometries are disjoint, false otherwise
   */
  bool GisGeometry::disjoint(const GisGeometry &target) const {
    if ( !isValid() ) {
      return (false);
    }
    if ( !target.isValid() ) {
      return (false);
    }

    int result = 0;
    if ( 0 != m_preparedGeom) {
      result = GEOSPreparedDisjoint(m_preparedGeom, target.geometry());
    }
    else {
      result = GEOSDisjoint(m_geom, target.geometry());
    }

    return (1 == result);
  }


  /**
   * Test for overlapping geometries
   *
   * @param target Geometry to test for overlap with this geometry
   * @return bool  True if target geometry overlaps with this geometry, false
   *               otherwise
   */
  bool GisGeometry::overlaps(const GisGeometry &target) const {
    if ( !isValid() ) {
      return (false);
    }
    if ( !target.isValid() ) {
      return (false);
    }

    int result = 0;
    if ( 0 != m_preparedGeom) {
      result = GEOSPreparedOverlaps(m_preparedGeom, target.geometry());
    }
    else {
      result = GEOSOverlaps(m_geom, target.geometry());
    }

    return (1 == result);
  }


/**
 * Test if target and this geometry are equal
 *
 * @param target Geometry to test for equality
 * @return bool  True if geometries are equal, false if not
 */
  bool GisGeometry::equals(const GisGeometry &target) const {
    if ( !isValid() ) {
      return (false);
    }

    if ( !target.isValid() ) {
      return (false);
    }

    int result = GEOSEquals(this->m_geom, target.geometry());
    return ( 1 == result );
  }


  /**
   * @brief Computes intersect ratio between two geometries
   *
   * This method computes the intersection of two geometries and the returns
   * the ratio of the area of intersection with this geometry. Units must be
   * the same for both geometries or result will not be valid.
   *
   * @param target  Geometry to compute intersect ratio for
   * @return double  Area of common intersection of two geometries in units of
   *                 the two geometries.
   */
  double GisGeometry::intersectRatio(const GisGeometry &target) const {
    if ( !isValid() ) {
      return (0.0);
    }
    if ( !target.isValid() ) {
      return (0.0);
    }

    //  Check for any intersection at all
  //  if ( !intersects(target) ) {
  //   return (0.0);
  // }

    // Prevent dividing by 0
    if (this->area() == 0) {
       return (0.0);
    }

    QScopedPointer<GisGeometry> inCommon(intersection(target));
    double ratio = inCommon->area() / this->area();
    return (ratio);
  }
  

/**
 * @brief Compute a buffer around an existing geometry 
 *  
 * Add a buffer around a geometry with the defined enlargement factor. It is 
 * common to use this as buffer(0) to fix geometries with self-intersections. 
 * These cases are often identified by calling isValid() and getting a false 
 * value returned. 
 * 
 * @author 2018-07-29 Kris Becker
 * 
 * @param width    Width to enlarge or shrink polygon 
 * @param quadsegs Number of segments to define a circle on corners
 * 
 * @return GisGeometry* Pointer to new GisGeometry with buffer applied
 */
  GisGeometry *GisGeometry::buffer(const double width,const int quadsegs) const {
      // If there is no geometry, return a null geometry
      if ( !isDefined()) {
          return (new GisGeometry());
      }
  
    // Create the buffer around the geom
    GEOSGeometry *geom = GEOSBuffer(m_geom, width, quadsegs);
    return (new GisGeometry(geom));
  }
  
  /** 
   * @brief Computes the envelope or bounding box of this geometry
   *
   * This method computes the envelope or bounding box of the geometry in this
   * object. A new geometry is computed and a pointer is returned to the
   * caller. The caller assumes ownership of this geometry.
   *
   * A null geometry will be returned if an error is occured or the current
   * geometry is invalid.
   *
   * @return GisGeometry  Pointer to envelope/bounding box. A null geometry, not
   *                      NULL pointer, is returned in problems occur.
   */
  GisGeometry *GisGeometry::envelope() const {
    if ( !isValid() ) {
      return (new GisGeometry());
    }

    GEOSGeometry *geom = GEOSEnvelope(m_geom);
    return (new GisGeometry(geom));
  }


  /**
   * Computes the convex hull of the geometry
   *
   * @return GisGeometry Pointer to new geometry that represents the convex
   *                     hull of this geometry. A null geometry, not NULL
   *                     pointer is returned if problems occur.
   */
  GisGeometry *GisGeometry::convexHull() const {
    if ( !isValid() ) {
      return (new GisGeometry());
    }

    GEOSGeometry *geom = GEOSConvexHull(m_geom);
    return (new GisGeometry(geom));
  }


 /**
  * @brief Simplify complex or overdetermined geoemtry
  *
  * This method will simplify a geometry with a Douglas -Peucker algorithm using
  * a tolerance specifying the maximum distance from the original (multi)polygon.
  * The use of this algorithm is designed to prevent oversimplification
  *
  *
  * @param tolerance Maximum distance from the original geometry expressed in the
  *                  coordinate system of the geometry.
  *
  * @return GisGeometry* Simplified geometry
  */
  GisGeometry *GisGeometry::simplify(const double &tolerance) const {
    if ( !isValid() ) {
      return (0);
    }
    GEOSGeometry *geom = GEOSTopologyPreserveSimplify(m_geom, tolerance);
    if ( 0 == geom ) {
      return (0);
    }

    return (new GisGeometry(geom));
  }


  /**
   * @brief Computes the intersection of two geometries
   *
   * The area of common interesction of the target geometry and this geometry
   * are computed and returned to the caller.
   *
   * @param target       Other geometry to compute intersection
   * @return GisGeometry Intersection geometry
   */
  GisGeometry *GisGeometry::intersection(const GisGeometry &target) const {
    //  Non-valid geometries return empty geometries
    if ( !isValid() ) {
      return (new GisGeometry());
    }
    if ( !target.isValid() ) {
      return (new GisGeometry());
    }

    GEOSGeometry *geom = GEOSIntersection(m_geom, target.geometry());
    return (new GisGeometry(geom));
  }


  /**
   * Computes the union of two geometries
   *
   * @param target  Other geometry to union with this geometry
   * @return GisGeometry Result of unioning the two geometries
   */
  GisGeometry *GisGeometry::g_union(const GisGeometry &target) const {
    if ( !isValid() ) {
      return (new GisGeometry());
    }
    if ( !target.isValid() ) {
      return (new GisGeometry());
    }

    GEOSGeometry *geom = GEOSUnion(m_geom, target.geometry());
    return (new GisGeometry(geom));
  }


  /**
   * @brief Computes the centroid of a spatial geometry
   *
   * This method will compute the coordinate centroid of a spatial geometry.
   *
   * @param xlongitude X/longitude coordinate of centroid
   * @param ylatitude  Y/latitude coordinate of centroid
   * @return bool      True if successful, false if failed
   */
  bool GisGeometry::centroid(double &xlongitude, double &ylatitude) const {
    xlongitude = ylatitude = Null;
    if ( !isValid() ) {
      return (false);
    }

    GEOSGeometry *center = GEOSGetCentroid(m_geom);
    if ( 0 != center ) {
      GEOSGeomGetX(center, &xlongitude);
      GEOSGeomGetY(center, &ylatitude);
      GisTopology::instance()->destroy(center);
      return (true);
    }

    return (false);
  }


  /**
   * Computes the centroid of the geometry and returns it as a new geometry
   *
   * @return GisGeometry Pointer to point geometry of centroid of this geometry
   */
  GisGeometry *GisGeometry::centroid() const {
    if ( !isValid() ) {
      return (new GisGeometry());
    }

    GEOSGeometry *center = GEOSGetCentroid(m_geom);
    return (new GisGeometry(center));
  }


  /**
   * Creates a prepared geometry of current geometry
   *
   * @param geom
   * @return GEOSPreparedGeometry
   */
  GEOSPreparedGeometry const *GisGeometry::makePrepared(const GEOSGeometry *geom)
                                                        const {
  #if defined(DISABLE_PREPARED_GEOMETRY)
    return (0);
  #else
    GisTopology *gis(GisTopology::instance());
    return (gis->preparedGeometry(geom));
  #endif
  }


  /**
   * Create a point geometry
   *
   * @param y  Y or latitude coordinate
   * @return GEOSGeometry Pointer to point geometry. Caller assumes ownership
   */
  GEOSGeometry *GisGeometry::makePoint(const double x, const double y) const {

    GEOSCoordSequence *point = GEOSCoordSeq_create(1, 2);
    GEOSCoordSeq_setX(point, 0, x);
    GEOSCoordSeq_setY(point, 0, y);

    return (GEOSGeom_createPoint(point));
  }


  /**
   * Reads Polygon from ISIS Cube and returns geometry from contents
   *
   * @param cube ISIS Cube containing a Polygon geometry object
   * @return GEOSGeometry Pointer to GEOS-C type geometry from Polygon BLOB
   */
  GEOSGeometry *GisGeometry::fromCube(Cube &cube) const {
    ImagePolygon myGis = cube.readFootprint();
    GisTopology *gis(GisTopology::instance());
    QString polyStr = QString::fromStdString(myGis.polyStr());
    return (gis->geomFromWKT(polyStr));
  }


  /**
   * Destroys the GEOS elements of this geometry object
   */
  void GisGeometry::destroy() {
    GisTopology *gis(GisTopology::instance());
    gis->destroy(m_geom);
    gis->destroy(m_preparedGeom);
    m_geom = 0;
    m_preparedGeom = 0;
    return;
  }

}  //namespace Isis
