/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QDebug>

#include "BulletClosestRayCallback.h"
#include "BulletTargetShape.h"
#include "IException.h"

#include "BulletWorldManager.h"

using namespace Isis;

/**
 * Unit test for the BulletWorldManager class
 */
int main(int argc, char *argv[]) {
  try {
    qDebug() << "Testing BulletWorldManager";
    qDebug() << Qt::endl;

    qDebug() << "Testing default constructor";
    qDebug() << "";
    BulletWorldManager defaultWorld;
    qDebug() << "World name: " << defaultWorld.name();
    qDebug() << "World size: " << defaultWorld.size();
    qDebug() << Qt::endl;

    qDebug() << "Testing with a name";
    BulletWorldManager namedWorld("TestWorld");
    qDebug() << "World name: " << namedWorld.name();
    qDebug() << "World size: " << namedWorld.size();
    qDebug() << Qt::endl;


    QString dskfile("$ISISTESTDATA/isis/src/base/unitTestData/hay_a_amica_5_itokawashape_v1_0_64q.bds");
    qDebug() << "Adding " << dskfile << "...";
    qDebug() << "";
    BulletTargetShape *itokawaShape = BulletTargetShape::load(dskfile);
    if (!itokawaShape) {
      std::string msg = "Failed loading shapefile.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    namedWorld.addTarget(itokawaShape);
    qDebug() << "World size: " << namedWorld.size();
    qDebug() << "";
    qDebug() << "Test intersection";
    btVector3 testFrom(-10.0, 0.0, 0.0);
    btVector3 testTo(10.0, 0.0, 0.0);
    BulletClosestRayCallback testHits(testFrom, testTo);
    qDebug() << "Ray start: ("<< testFrom.x() << ", "
                              << testFrom.y() << ", "
                              << testFrom.z() << ")";
    qDebug() << "Ray end: ("<< testTo.x() << ", "
                            << testTo.y() << ", "
                            << testTo.z() << ")";
    namedWorld.raycast(testFrom, testTo, testHits);
    qDebug() << "Closest intersection: (" << testHits.point().x() << ", "
                                          << testHits.point().y() << ", "
                                          << testHits.point().z() << ")";
    qDebug() << "";
    qDebug() << "Get the first target shape by index";
    BulletTargetShape *firstTarget = namedWorld.getTarget();
    if (firstTarget) {
      qDebug() << "Target name: " << firstTarget->name();
      qDebug() << "Get the first target shape by name";
      BulletTargetShape *namedTarget = namedWorld.getTarget(firstTarget->name());
      qDebug() << "Target name: " << namedTarget->name();
    }
    else {
      std::string msg = "Failed accessing the first target shape.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    qDebug() << "";

    qDebug() << "Get a non-existant target by name";
    QString badTargetName = "not_a_target";
    BulletTargetShape *badTarget = namedWorld.getTarget(badTargetName);
    qDebug() << "Target pointer is valid?" << (bool) badTarget;
    qDebug() << "";

    qDebug() << "Get the bullet world pointer";
    namedWorld.getWorld();
  }
  catch (IException &e) {
    qDebug() << "";
    qDebug() << "";
    std::string msg = "**************** UNIT TEST FAILED! **************** ";
    IException(e, IException::Unknown, msg, _FILEINFO_).print();
  }
}