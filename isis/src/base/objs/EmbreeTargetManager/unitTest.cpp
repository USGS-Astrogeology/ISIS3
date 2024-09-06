/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <QDebug>
#include <QVector>

#include "NaifDskPlateModel.h"
#include "NaifDskShape.h"
#include "Angle.h"
#include "Camera.h"
#include "Cube.h"
#include "Distance.h"
#include "EmbreeTargetManager.h"
#include "FileName.h"
#include "IException.h"
#include "Intercept.h"
#include "Latitude.h"
#include "Longitude.h"
#include "Preference.h"
#include "Pvl.h"
#include "Spice.h"
#include "SurfacePoint.h"
#include "Target.h"

using namespace Isis;

/** 
 * Unit test for Embree Ray Tracing Kernels
 *
 */
int main(int argc, char *argv[]) {
  try {
    Preference::Preferences(true);
    qDebug() << "Testing EmbreeTargetManager";
    qDebug() << Qt::endl;

    qDebug() << "Get an instance of the target manager";
    EmbreeTargetManager *manager = EmbreeTargetManager::getInstance();
    qDebug() << "Maximum cache size: " << manager->maxCacheSize();
    qDebug() << "Current cache size: " << manager->currentCacheSize();
    qDebug() << "";

    qDebug() << "Change the maximum cache size: ";
    manager->setMaxCacheSize(5);
    qDebug() << "New maximum cache size: " << manager->maxCacheSize();
    qDebug() << "";

    QString dskfile("$ISISTESTDATA/isis/src/base/unitTestData/hay_a_amica_5_itokawashape_v1_0_64q.bds");
    qDebug() << "Create a target shape for " << dskfile;
    EmbreeTargetShape *managedTargetShape = manager->create(dskfile);
    qDebug() << "Target shape status:";
    qDebug() << "  Number of polygons: " << managedTargetShape->numberOfPolygons();
    qDebug() << "  Number of vertices: " << managedTargetShape->numberOfVertices();
    qDebug() << "  Maximum distance:   " << managedTargetShape->maximumSceneDistance();
    qDebug() << "Current cache size: " << manager->currentCacheSize();
    qDebug() << "";

    qDebug() << "Create a new target shape for the same file";
    QString copyDSKFile = "$ISISTESTDATA/isis/src/base/unitTestData/hay_a_amica_5_itokawashape_v1_0_64q.bds";
    EmbreeTargetShape *copyTargetShape = manager->create(copyDSKFile);
    qDebug() << "Target shape status:";
    qDebug() << "  Number of polygons: " << copyTargetShape->numberOfPolygons();
    qDebug() << "  Number of vertices: " << copyTargetShape->numberOfVertices();
    qDebug() << "  Maximum distance:   " << copyTargetShape->maximumSceneDistance();
    qDebug() << "Current cache size: " << manager->currentCacheSize();
    qDebug() << "";
    qDebug() << "Shape file is in the cache? " << manager->inCache(copyDSKFile);
    qDebug() << "";
    qDebug() << "Both instances point to the same object? "
             << (copyTargetShape == managedTargetShape);
    qDebug() << "";

    qDebug() << "Free one of them";
    manager->free(copyDSKFile);
    qDebug() << "Current cache size: " << manager->currentCacheSize();
    qDebug() << "";

    qDebug() << "Free the other";
    manager->free(dskfile);
    qDebug() << "Current cache size: " << manager->currentCacheSize();
    qDebug() << "Shape file is in the cache? " << manager->inCache(copyDSKFile);
    qDebug() << "";

    qDebug() << "Set the maximum number of target shapes to 0";
    manager->setMaxCacheSize(0);
    qDebug() << "New maximum cache size: " << manager->maxCacheSize();
    qDebug() << "";

    qDebug() << "Attempt to create a new target shape";
    try {
      manager->create(dskfile);
    }
    catch (IException &e) {
      e.print();
    }
    qDebug() << "";

    qDebug() << "Attempt to free a shape that doesn't exist";
    try {
      manager->free("Not a DSK file");
    }
    catch (IException &e) {
      e.print();
    }
  }
  catch (IException &e) {
    qDebug() << "";
    qDebug() << "";
    std::string msg = "**************** UNIT TEST FAILED! **************** ";
    IException(e, IException::Unknown, msg, _FILEINFO_).print();
  }
}
