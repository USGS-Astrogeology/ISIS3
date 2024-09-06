/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QTextStream>

#include "Cube.h"
#include "GisGeometry.h"
#include "GisTopology.h"
#include "IException.h"
#include "ImagePolygon.h"
#include "IString.h"
#include "Preference.h"

using namespace Isis;
using namespace std;

/**
 * Unit test for GisTopology class
 *
 *
 * @author 2016-02-23 Jeannie Backer
 *
 * @internal
 *   @history 2016-02-23 Jeannie Backer - Original version.
 *   @history 2016-03-01 Ian Humphrey - Added a few more tests for clone(), wkb() and wkt()'s
 *                           scope coverage. References #2398.
 *
 */
int main() {
  try {
    Preference::Preferences(true);
    qDebug() << "";
    qDebug() << "Testing GisTopology...";
    qDebug() << "";

    GisTopology *topo = GisTopology::instance();

    // get polygon string from cube's blob
    QString inputFile = "$ISISTESTDATA/isis/src/messenger/unitTestData/EW0211286081G.lev1.cub";
    Cube cube;
    cube.open(inputFile);
    ImagePolygon footprint = cube.readFootprint();
    QString wktFromCube = QString::fromStdString(footprint.polyStr());
    qDebug() << "wkt from cube: " << wktFromCube;

    // create geometry from wkt string and make sure the output string matches
    GEOSGeometry *fromWKT = topo->geomFromWKT(wktFromCube);
    QString wktFromGeom = topo->wkt(fromWKT, GisTopology::PreserveGeometry);
    qDebug() << "wkt from cube == wkt from geometry? " << (wktFromCube == wktFromGeom);

    // since we passed GisTopology::PreserveGeometry, check that GEOSGeometry * is still valid
    if (fromWKT)
      qDebug() << "GEOSGeometry from cube is preserved.";
    qDebug() << "";
    qDebug() << "==============================================================================";
    qDebug() << "";

    // get wkb string from previous geometry and use it to create a new geometry from wkb
    QString wkbFromGeom = topo->wkb(fromWKT, GisTopology::PreserveGeometry);
    qDebug() << "WKB: " << wkbFromGeom;
    qDebug() << "wkt from cube == wkb from geometry? " << (wktFromCube == wkbFromGeom);

    GEOSGeometry *fromWKB = topo->geomFromWKB(wkbFromGeom);
    QString wkbFromNewGeom = topo->wkb(fromWKB, GisTopology::PreserveGeometry);
    qDebug() << "wkb from original geometry == wkb from new geometry? "
             << (wkbFromGeom == wkbFromNewGeom);
    qDebug() << "";
    qDebug() << "==============================================================================";
    qDebug() << "";

    // create GEOSGeometry clone from GisGeometry (same blob)
    // and compare wkb/wkt to previous results
    GisGeometry geom(cube);
    const GEOSGeometry *g = geom.geometry();
    GEOSGeometry *clone = topo->clone(g);
    QString wktFromClone = topo->wkt(clone, GisTopology::PreserveGeometry);
    qDebug() << "wkt from clone == wkt from original geometry? " << (wktFromClone == wktFromGeom);
    QString wkbFromClone = topo->wkb(clone, GisTopology::PreserveGeometry);
    qDebug() << "wkb from clone == wkb from original geometry? "
             << (wkbFromClone == wkbFromGeom);
    qDebug() << "";
    qDebug() << "==============================================================================";
    qDebug() << "";

    // grab the wkt and destroy the geometry using fromWKT pointer
    QString wktFromGeomToDestroy = topo->wkt(fromWKT, GisTopology::DestroyGeometry);
    qDebug() << "wkt from cube == new wkt, set to destroy the geometry? "
             << (wktFromCube == wktFromGeomToDestroy);
    // NOTE - fromWKT is now a dangling pointer
    // QString crash = topo->wkt(fromWKT, GisTopology::PreserveGeometry);

    // grab the wkb and destroy the geometry using fromWKB pointer
    QString wkbFromGeomToDestroy = topo->wkb(fromWKB, GisTopology::DestroyGeometry);
    qDebug() << "wkb from original geometry == new wkb, set to destroy the geometry? "
             << (wkbFromGeom == wkbFromGeomToDestroy);
    // NOTE - fromWKB is now a dangling pointer
    // QString crash = topo->wkb(fromWKB, GisTopology::PreserveGeometry);

    qDebug() << "==============================================================================";
    qDebug() << "";

    // create a "clone" using NULL
    GEOSGeometry *nullClone = topo->clone(NULL);
    if (!nullClone)
      qDebug() << "clone(NULL) gave us a null pointer...";
    qDebug() << "";
    qDebug() << "==============================================================================";
    qDebug() << "";

    // Verify we can create prepared geometry
    qDebug() << "Prepared geometry created...";
    // TODO - test preparedGeometry()'s exception

    // Not sure how to verify through GisTopology API
    topo->preparedGeometry(g);
    //QString wktFromPrep = topo->wkt(prep, GisTopology::PreserveGeometry);
    //QString wkbFromPrep = topo->wkb(prep, GisTopology::PreserveGeometry);
    qDebug() << "";
    qDebug() << "==============================================================================";
// Once test coverage tool works again, we can see if we need this test for full coverage.
#if 0
    qDebug() << "";
    //inputFile = "/work/users/kbecker/EW0213634118G.lev1.cub";
    QFile file("unitTest.wkb");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      throw IException(IException::Io, "Unable to open wkt file, [unitTest.wkb]", _FILEINFO_);
    }
    QString wkbFromFile;
    QTextStream stream(&file);
    wkbFromFile.append(stream.readAll());
    GEOSGeometry *fromWKBFile = topo->geomFromWKB(wkbFromFile);
    QString wkb = topo->wkb(fromWKBFile, GisTopology::PreserveGeometry);
    qDebug() << "wkb from file == wkb from geometry? "
             << (wkbFromFile.trimmed() == wkb.trimmed());
    qDebug() << "";
    qDebug() << "==============================================================================";
#endif
    /* These tests produce ParseExceptions (from libgeos)...
    qDebug() << "Testing exceptions...";
    qDebug() << "Try to get a GEOSGeometry from bad wkb...";
    try {
      // geomFromWKB() "Unable to convert the given WKB string to a GEOSGeometry"
      QString badWKB = "invalid wkb data";
      topo->geomFromWKB(badWKB);
    }
    catch (IException &e) {
      e.print();
    }

    qDebug() << "";
    qDebug() << "Try to get a GEOSGeometry from bad wkt...";
    try {
      QString badWKT = "invalid wkt data";
      topo->geomFromWKT(badWKT);
    }
    catch (IException &e) {
      e.print();
    }
    */

  }
  catch (IException &e) {
    qDebug() << "";
    qDebug() << "";
    std::string msg = "**************** UNIT TEST FAILED! **************** ";
    IException(e, IException::Unknown, msg, _FILEINFO_).print();
  }

}
