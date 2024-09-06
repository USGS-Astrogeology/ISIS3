/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QDebug>

#include "IException.h"

#include "BulletTargetShape.h"

using namespace Isis;

/**
 * Unit test for the BulletTargetShape class
 */
int main(int argc, char *argv[]) {
  try {
    qDebug() << "Testing BulletTargetShape";
    qDebug() << Qt::endl;

    qDebug() << "Testing default constructor";
    qDebug() << "";
    BulletTargetShape defaultTargetShape;
    qDebug() << "Target name: " << defaultTargetShape.name();
    qDebug() << "Maximum distance in kilometers: " << defaultTargetShape.maximumDistance();
    qDebug() << "btCollisionBody pointer is valid? " << (bool) defaultTargetShape.body();
    qDebug() << Qt::endl;

    qDebug() << "Testing load constructor";
    QString dskfile("$ISISTESTDATA/isis/src/base/unitTestData/hay_a_amica_5_itokawashape_v1_0_64q.bds");
    qDebug() << "Testing with " << dskfile << "...";
    qDebug() << "";
    BulletTargetShape *itokawaTargetShape = BulletTargetShape::load(dskfile);
    qDebug() << "Target name: " << itokawaTargetShape->name();
    qDebug() << "Maximum distance in kilometers: " << itokawaTargetShape->maximumDistance();
    qDebug() << "btCollisionBody pointer is valid? " << (bool) itokawaTargetShape->body();
    qDebug() << Qt::endl;

    qDebug() << "Testing load constructor with cube";
    QString itokawaCube = "$ISISTESTDATA/isis/src/hayabusa/unitTestData/st_2391934788_v.cub";
    qDebug() << "Testing with " << itokawaCube << "...";
    qDebug() << "";
    BulletTargetShape *cubeTargetShape = BulletTargetShape::load(itokawaCube);
    qDebug() << "Target shape pointer is valid?" << (bool) cubeTargetShape;
    qDebug() << Qt::endl;

    qDebug() << "Testing load constructor with other extension";
    QString otherFile = "$ISISTESTDATA/isis/src/base/unitTestData/xmlTestLabel.xml";
    qDebug() << "Testing with " << otherFile << "...";
    qDebug() << "";
    BulletTargetShape *otherTargetShape = BulletTargetShape::load(otherFile);
    qDebug() << "Target shape pointer is valid?" << (bool) otherTargetShape;
    qDebug() << Qt::endl;

    qDebug() << "Testing collision body constructor";
    BulletTargetShape collisionTargetShape(itokawaTargetShape->body(), "Itokawa");
    qDebug() << "Target name: " << collisionTargetShape.name();
    qDebug() << "Maximum distance in kilometers: " << collisionTargetShape.maximumDistance();
    qDebug() << "btCollisionBody pointer is valid? " << (bool) collisionTargetShape.body();
    qDebug() << Qt::endl;

    qDebug() << "Testing null collision body constructor";
    BulletTargetShape nullTargetShape(0, "Null");
    qDebug() << "Target name: " << nullTargetShape.name();
    qDebug() << "Maximum distance in kilometers: " << nullTargetShape.maximumDistance();
    qDebug() << "btCollisionBody pointer is valid? " << (bool) nullTargetShape.body();
  }
  catch (IException &e) {
    qDebug() << "";
    qDebug() << "";
    std::string msg = "**************** UNIT TEST FAILED! **************** ";
    IException(e, IException::Unknown, msg, _FILEINFO_).print();
  }
}
