/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QTextStream>

// geos library
#include <geos_c.h>

#include "Cube.h"
#include "GisGeometry.h"
#include "GisTopology.h"
#include "IException.h"
#include "ImagePolygon.h"
#include "IString.h"
#include "Preference.h"

using namespace Isis;
using namespace std;

void printBasicInfo(GisGeometry geom, QString description);
void printTargetInfo(GisGeometry geom, GisGeometry target, QString description);
void printTypes();

/**
 * Unit test for GisGeometry class
 *
 *
 * @author 2016-02-23 Jeannie Backer
 *
 * @internal
 *   @history 2016-02-23 Jeannie Backer - Original version.
 *   @history 2016-03-02 Ian Humphrey - Added tests for remaining untested methods.
 *                           References #2398.
 *   @history 2016-03-04 Ian Humphrey - Updated test and truthdata for equals() method.
 *                           References #2398.
 *   @history 2024-09-23 Ken Edmundson - Updated test and truthdata for 1) detecting
 *                           self-intersecting geometries; 2) repairing such geometries
 *                           with a buffer of size 0; and 3) overlap and intersection
 *                           of repaired geometry with another.
 *                           References #5612.
 *
 *
 * NOTE - distance(), intersects(), contains(), disjoin(), overlaps() methods
 *          are not completely tested.
 *        distance()'s if (result != 1), the exception case, is untested.
 *        For the other methods, if (0 != this->m_preparedGeom)'s else clause
 *          is untested. Only the default constructor will create a GisGeometry
 *          without a prepared geometry being instantiated. All the other constructors
 *          as well as setGeometry() instantiate prepared geometries.
 *          So, to test the else clause, the prepared geometry would have to be NULL after
 *          instantiation.
 *          The prepared geometry is created with makePrepared(), which calls
 *          GisTopology::preparedGeometry(). This method throws an exception
 *          if the prepared geometry is NULL.
 *
 * NOTE - setGeometry() is also untested.
 */
int main() {
  try {
    Preference::Preferences(true);
    qDebug() << "";
    qDebug() << "Testing GisGeometry...";
    qDebug() << "";
    printTypes();
    qDebug() << "";
    GisTopology *topo = GisTopology::instance();

    double xlongitude = 0;
    double ylatitude = 0;
    GisGeometry geomLatLon(xlongitude, ylatitude);
    printBasicInfo(geomLatLon,
                   "Construct Geometry from Lat/Lon");

    QString inputFile = "$ISISTESTDATA/isis/src/messenger/unitTestData/EW0211286081G.lev1.cub";
    Cube cube;
    cube.open(inputFile);
    GisGeometry geomCube(cube);
    printBasicInfo(geomCube,
                   "Construct Geometry from Cube");

    ImagePolygon footprint = cube.readFootprint();
    QString wkt = QString::fromStdString(footprint.polyStr());
    GisGeometry geomGisWKT(wkt, GisGeometry::WKT);
    printBasicInfo(geomGisWKT,
                   "Construct Geometry from WKT GIS source");

    const GEOSGeometry *g = geomCube.geometry();
    GEOSGeometry *clone = topo->clone(g);
    //QString wkb = topo->wkb(clone, GisTopology::DestroyGeometry);
    QString wkb = topo->wkb(clone);
    GisGeometry geomGisWKB(wkb, GisGeometry::WKB);
    printBasicInfo(geomGisWKB,
                   "Construct Geometry from WKB GIS source");

    GisGeometry geomGisIsisCube("$ISISTESTDATA/isis/src/messenger/unitTestData/EW0213634118G.lev1.cub",
                                GisGeometry::IsisCube);
    printBasicInfo(geomGisIsisCube,
                   "Construct Geometry from IsisCube GIS source");

    GEOSGeometry *geos = topo->geomFromWKT(wkt);
    GisGeometry geomGEOS(geos);
    printBasicInfo(geomGEOS,
                   "Construct Geometry from GEOSGeometry");
//???
    GisGeometry geomDefault;
    printBasicInfo(geomDefault, "Construct Empty Default Geometry");
//???    geomDefault.setGeometry(geos); // SEGFAULT
//???    printBasicInfo(geomDefault, "Set Default Geometry from GEOSGeometry");

    // polygon with self-intersecting geometry that lies
    // within the boundaries of EW0211286081G.lev1.cub
    QString wktSelfIntersect
        = QString::fromStdString("POLYGON ((286.0 51.0, 291.5 53.0, 295.0 49.8, 289.5 47.0, 286.6 51.5, 286.0 51.0))");
    GisGeometry geomGisWKTSelfIntersect(wktSelfIntersect, GisGeometry::WKT);
    printBasicInfo(geomGisWKTSelfIntersect,
                   "Construct Self-Intersecting Geometry from WKT GIS source");

    // repair the self-intersecting geometry with buffer(0)
    GisGeometry *repairedSelfIntersect = geomGisWKTSelfIntersect.buffer(0);
    printBasicInfo(*repairedSelfIntersect,
                   "Repaired Self-Intersecting Geometry from WKT GIS source");

    GisGeometry geomCopy(geomCube);
    printBasicInfo(geomCopy, "Construct Copy Geometry from GisGeometry from Cube");

    GisGeometry geomNotDefinedCopy(geomDefault); // geomDefault is an undefined geometry
    printBasicInfo(geomNotDefinedCopy, "Construct Copy Geometry from Undefined Geometry");

    GisGeometry geomOperatorEqual = geomCube;
    printBasicInfo(geomOperatorEqual, "Construct Equal Geometry from GisGeometry from Cube");

    GisGeometry geomOperatorEqual2 = geomOperatorEqual;
    printBasicInfo(geomOperatorEqual2, "Construct Equal Geometry from Equal Geometry");

    GisGeometry geomOperatorEqual3 = GisGeometry();
    printBasicInfo(geomOperatorEqual3, "Construct Equal Geometry from Undefined Geometry");

    GisGeometry *cloneGeom = geomCube.clone();
    printBasicInfo(*cloneGeom, "Clone Geometry from GisGeometry from Cube");

    GisGeometry *cloneGeomFromUndefined = GisGeometry().clone();
    printBasicInfo(*cloneGeomFromUndefined, "Clone Geometry from Undefined Geometry");

    printTargetInfo(geomDefault, geomGisWKT,
                    "Source: Invalid Geometry, Target: WKT Geometry");

    printTargetInfo(geomGisWKT, geomDefault,
                    "Source: WKT Geometry, Target: Invalid Geometry");

    // overlapping geometries
    printTargetInfo(*repairedSelfIntersect, geomGisIsisCube,
                    "Source: Repaired Self-Intersecting WKT Geometry, Target: GeomGisIsisCube Geometry");

    printTargetInfo(geomGisIsisCube, geomGisWKT,
                    "Source: GisIsisCube Geometry, Target: WKT Geometry");

    // equal geometries
    printTargetInfo(geomGisWKT, geomGisWKB,
                    "Source: WKT Geometry, Target: WKB Geometry (equal geometries)");

    // disjoint geometries
    printTargetInfo(geomGisWKT, geomLatLon,
                    "Source: WKT Geometry, Target: Lat/Lon Geometry");

    // TODO - test result != 1 (exception case) for distance() method

    // Test intersectRatio where target has area of 0
    qDebug() << "Intersect Ratio of WKT Geometry with Lat/Lon (single point) Geometry: "
             << geomGisWKT.intersectRatio(geomLatLon);
    qDebug() << "";

    GisGeometry *envelopeGeom = geomCube.envelope();
    printBasicInfo(*envelopeGeom, "Envelope Geometry from GisGeometry from Cube");

    GisGeometry *envelopeInvalidGeom = GisGeometry().envelope();
    printBasicInfo(*envelopeInvalidGeom, "Envelope Geometry from Invalid Geometry");

    GisGeometry *convexHullGeom = geomCube.convexHull();
    printBasicInfo(*convexHullGeom, "Convex Hull Geometry from Geometry from Cube");

    GisGeometry *convexHullInvalidGeom = geomDefault.convexHull();
    printBasicInfo(*convexHullInvalidGeom, "Convex Hull Geometry from Invalid Geometry");

    double tolerance = 3.14;
    GisGeometry *simpleGeom = geomCube.simplify(tolerance);
    printBasicInfo(*simpleGeom, "Simplified Geometry from Geometry from Cube");

    GisGeometry *simpleInvalidGeom = geomDefault.simplify(tolerance);
    if (!simpleInvalidGeom) {
      qDebug() << "Simplified Geometry from Invalid Geometry is NULL.";
      qDebug() << "";
    }

    // These two tests below should output empty geometries
    GisGeometry *intersectionInvalidSourceGeometry = geomDefault.intersection(geomGisWKT);
    printBasicInfo(*intersectionInvalidSourceGeometry,
                   "Intersection Geometry of Invalid Geometry with WKT Geometry as target");

    GisGeometry *intersectionInvalidTargetGeometry = geomGisWKT.intersection(geomDefault);
    printBasicInfo(*intersectionInvalidTargetGeometry,
                   "Intersection Geometry of WKT Geometry with Invalid Geometry as target");

    GisGeometry *intersectionGeom = geomGisIsisCube.intersection(geomGisWKT);
    printBasicInfo(*intersectionGeom,
                   "Intersection Geometry of GisIsisCube Geometry with WKT Geometry");

    GisGeometry *intersectCubeAndRepairedGeom = geomCube.intersection(*repairedSelfIntersect);
    printBasicInfo(*intersectCubeAndRepairedGeom,
                   "Intersection Geometry of GeomCube and Repaired Self-Intersecting WKT Geometries");

    // These two tests below should output empty geometries (union w/ invalid -> invalid)
    GisGeometry *unionInvalidSourceGeom = geomDefault.g_union(geomGisWKT);
    printBasicInfo(*unionInvalidSourceGeom,
                   "Union Geometry of Invalid Geometry with WKT Geometry as target");

    GisGeometry *unionInvalidTargetGeom = geomGisWKT.g_union(geomDefault);
    printBasicInfo(*unionInvalidTargetGeom,
                   "Union Geometry of WKT Geometry with Invalid Geometry as target");

    GisGeometry *unionGeom = geomGisIsisCube.g_union(geomGisWKT);
    printBasicInfo(*unionGeom,
                   "Union Geometry of GisIsisCube Geometry with WKT Geometry");

    GisGeometry *centroidInvalidGeom = geomDefault.centroid();
    printBasicInfo(*centroidInvalidGeom,
                   "Centroid Geometry of Invalid Geometry");

    GisGeometry *centroidGeom = geomGisWKT.centroid();
    printBasicInfo(*centroidGeom,
                   "Centroid Geometry of WKT Geometry");

    bool centroidFound = geomDefault.centroid(xlongitude, ylatitude);
    qDebug() << "Centroid found for Invalid Geometry? " << centroidFound;
    qDebug() << "";

    // TODO - test centroid(dbl,dbl) where center == 0 (exception case, would return false)

    centroidFound = geomGisWKT.centroid(xlongitude, ylatitude);
    qDebug() << "Centroid found for WKT Geometry? " << centroidFound;
    qDebug() << "\tLongitude: " << xlongitude;
    qDebug() << "\tLatitude:  " << ylatitude;
    qDebug() << "";

    // TODO - Not sure how to validate this result
    geomDefault.preparedGeometry();

    qDebug() << "Testing Errors...";
    try {
      GisGeometry geomGisNone("", GisGeometry::None);
    }
    catch (IException &e) {
      e.print();
    }

  }
  catch (IException &e) {
    qDebug() << "";
    qDebug() << "";
    QString msg = "**************** UNIT TEST FAILED! **************** ";
    IException(e, IException::Unknown, msg, _FILEINFO_).print();
  }
}


/**
 * Method to print basic information about the given geometry
 */
void printBasicInfo(GisGeometry geom, QString description) {
  qDebug() << description;
  qDebug() << "    isDefined?       " << geom.isDefined();
  qDebug() << "    isValid?         " << geom.isValid();
  qDebug() << "    isValidReason?   " << geom.isValidReason();
  qDebug() << "    isEmpty?         " << geom.isEmpty();
  qDebug() << "    type?            " << GisGeometry::typeToString(geom.type());
  qDebug() << "    area?            " << geom.area();
  qDebug() << "    length?          " << geom.length();
  qDebug() << "    points?          " << geom.points();
  qDebug() << "";
}


/**
 * Method to print information about this GisGeometry with relation to another
 */
void printTargetInfo(GisGeometry geom,  GisGeometry target, QString description) {
  qDebug() << description;
  qDebug() << "    distance?        " << toString(geom.distance(target));
  qDebug() << "    intersects?      " << toString(geom.intersects(target));
  qDebug() << "    contains?        " << toString(geom.contains(target));
  qDebug() << "    disjoint?        " << toString(geom.disjoint(target));
  qDebug() << "    overlaps?        " << toString(geom.overlaps(target));
  qDebug() << "    equals?          " << toString(geom.equals(target));
  qDebug() << "    intersect ratio? " << toString(geom.intersectRatio(target));
  qDebug() << "";
}


/**
 * Method to test the static type() and typeToString() methods.
 */
void printTypes() {
  qDebug() << "GisGeometry::Types:";
  qDebug() << "    wkt      = " << GisGeometry::typeToString(GisGeometry::type("wkt"));
  qDebug() << "    wkb      = " << GisGeometry::typeToString(GisGeometry::type("wkb"));
  qDebug() << "    cube     = " << GisGeometry::typeToString(GisGeometry::type("cube"));
  qDebug() << "    isiscube = " << GisGeometry::typeToString(GisGeometry::type("isiscube"));
  qDebug() << "    geometry = " << GisGeometry::typeToString(GisGeometry::type("geometry"));
  qDebug() << "    geosgis  = " << GisGeometry::typeToString(GisGeometry::type("geosgis"));
  qDebug() << "    gis      = " << GisGeometry::typeToString(GisGeometry::type("gis"));
  qDebug() << "    geos     = " << GisGeometry::typeToString(GisGeometry::type("geos"));
  qDebug() << "    other    = " << GisGeometry::typeToString(GisGeometry::type("other"));
  qDebug() << "";

}
