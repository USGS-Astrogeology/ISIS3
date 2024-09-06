/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <QDebug>
#include <QVector>

#include "Angle.h"
#include "BulletShapeModel.h"
#include "Camera.h"
#include "Cube.h"
#include "Distance.h"
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

void outputModelStatus(BulletShapeModel &bulletModel);
void testCameraToGround(std::vector<double> &observer,
                        std::vector<double> &lookDir,
                        BulletShapeModel &bulletModel);
void testGroundToCamera(Latitude &lat,
                        Longitude &lon,
                        std::vector<double> &observer,
                        BulletShapeModel &bulletModel);
void testGroundPointToCamera(Latitude &lat,
                             Longitude &lon,
                             std::vector<double> &observer,
                             BulletShapeModel &bulletModel);

/**
 * Unit test for Bullet Physics Ray Tracing library
 *
 * @internal
 *   @history 2017-03-19 Kris Becker
 *   @history 2018-08-07 Adam Goins - Removed qDebug() output of maxnumtriangles and maxnumparts
 *                           because they are values grabbed from Bullet so we don't need to be
 *                           testing them.
 *
 */
int main(int argc, char *argv[]) {
  try {
    Preference::Preferences(true);

    QString itokawaCubeFile("$ISISTESTDATA/isis/src/hayabusa/unitTestData/st_2391934788_v.cub");
    QString itokawaDskFile("$ISISTESTDATA/isis/src/base/unitTestData/hay_a_amica_5_itokawashape_v1_0_64q.bds");

    qDebug() << "Unit test for BulletShapeModel";
    qDebug() << "";
    qDebug() << "";

    Cube itokawaCube(itokawaCubeFile);
    Camera *itokawaCamera = itokawaCube.camera();
    Target *itokawaTarget = itokawaCamera->target();

    qDebug() << "----====        Construct default shape model        ====----";
    qDebug() << "";
    qDebug() << "";
    BulletShapeModel defaultModel;
    outputModelStatus(defaultModel);
    qDebug() << "";

    qDebug() << "----====       Construct shape model from cube       ====----";
    qDebug() << "";
    qDebug() << "";
    qDebug() << "Using: " << itokawaCubeFile;
    Pvl itokawaLabel(itokawaCubeFile.toStdString());
    PvlObject &itokawaCubeLabel = itokawaLabel.findObject("IsisCube");
    itokawaCubeLabel.findGroup("Kernels").findKeyword("ShapeModel").setValue(itokawaDskFile.toStdString());
    BulletShapeModel itokawaModel( itokawaTarget, itokawaLabel );
    itokawaModel.setTolerance(0.001);
    outputModelStatus(itokawaModel);
    qDebug() << "";

    qDebug() << "----====     Test camera to ground intersections     ====----";
    qDebug() << "";
    qDebug() << "";
    std::vector<double> observer(3, 0.0);
    std::vector<double> lookDir(3, 0.0);

    observer[0] = 20; observer[1] = 0; observer[2] = 0;
    lookDir[0] = -1; lookDir[1] = 0; lookDir[2] = 0;
    testCameraToGround(observer, lookDir, itokawaModel);
    qDebug() << "";

    observer[0] = 0; observer[1] = 20; observer[2] = 0;
    lookDir[0] = 0; lookDir[1] = -1; lookDir[2] = 0;
    testCameraToGround(observer, lookDir, itokawaModel);
    qDebug() << "";

    observer[0] = 0; observer[1] = 0; observer[2] = 20;
    lookDir[0] = 0; lookDir[1] = 0; lookDir[2] = -1;
    testCameraToGround(observer, lookDir, itokawaModel);
    qDebug() << "";

    observer[0] = 0.1; observer[1] = .03; observer[2] = 10;
    lookDir[0] = 0; lookDir[1] = 0; lookDir[2] = -1;
    testCameraToGround(observer, lookDir, itokawaModel);
    qDebug() << "";

    observer[0] = -5; observer[1] = -4; observer[2] = -2;
    lookDir[0] = 0.25; lookDir[1] = 0.2; lookDir[2] = 0.1;
    testCameraToGround(observer, lookDir, itokawaModel);
    qDebug() << "";

    observer[0] = -5; observer[1] = 5; observer[2] = 4;
    lookDir[0] = 1; lookDir[1] = -2; lookDir[2] = 1;
    testCameraToGround(observer, lookDir, itokawaModel);
    qDebug() << "";

    qDebug() << "----====     Test ground to camera intersections     ====----";
    qDebug() << "";
    qDebug() << "";
    itokawaCamera->SetImage( itokawaCube.sampleCount() / 2,
                             itokawaCube.lineCount()   / 2 );
    itokawaCamera->instrumentPosition(&observer[0]);
    Latitude testLat(0, Angle::Degrees);
    Longitude testLon(0, Angle::Degrees);
    qDebug() << "";

    testLat.setDegrees(-14.5);
    testLon.setDegrees(338);
    testGroundToCamera(testLat, testLon, observer, itokawaModel);
    qDebug() << "";

    testLat.setDegrees(41.6);
    testLon.setDegrees(328);
    testGroundToCamera(testLat, testLon, observer, itokawaModel);
    qDebug() << "";

    testLat.setDegrees(-4.67);
    testLon.setDegrees(207.6);
    testGroundToCamera(testLat, testLon, observer, itokawaModel);
    qDebug() << "";

    testLat.setDegrees(-3.33);
    testLon.setDegrees(165.2);
    testGroundToCamera(testLat, testLon, observer, itokawaModel);
    qDebug() << "";

    testLat.setDegrees(-18.6357);
    testLon.setDegrees(292);
    testGroundToCamera(testLat, testLon, observer, itokawaModel);
    qDebug() << "";

    testLat.setDegrees(0.0);
    testLon.setDegrees(350);
    testGroundToCamera(testLat, testLon, observer, itokawaModel);
    qDebug() << "";

    testLat.setDegrees(25);
    testLon.setDegrees(200);
    testGroundToCamera(testLat, testLon, observer, itokawaModel);
    qDebug() << "";

    qDebug() << "----====  Test ground point to camera intersections  ====----";
    qDebug() << "";
    qDebug() << "";

    testLat.setDegrees(-14);
    testLon.setDegrees(194);
    testGroundPointToCamera(testLat, testLon, observer, itokawaModel);
    qDebug() << "";

    testLat.setDegrees(42.782);
    testLon.setDegrees(328.573);
    testGroundPointToCamera(testLat, testLon, observer, itokawaModel);
    qDebug() << "";


    testLat.setDegrees(-26.1383);
    testLon.setDegrees(356.964);
    testGroundPointToCamera(testLat, testLon, observer, itokawaModel);
    qDebug() << "";


    testLat.setDegrees(12.8509);
    testLon.setDegrees(291.106);
    testGroundPointToCamera(testLat, testLon, observer, itokawaModel);
    qDebug() << "";


    testLat.setDegrees(-18.6357);
    testLon.setDegrees(60);
    testGroundPointToCamera(testLat, testLon, observer, itokawaModel);
    qDebug() << "";

    testLat.setDegrees(25);
    testLon.setDegrees(200);
    testGroundPointToCamera(testLat, testLon, observer, itokawaModel);
    qDebug() << "";
  }
  catch (IException &e) {
    qDebug() << "";
    qDebug() << "";
    std::string msg = "**************** UNIT TEST FAILED! **************** ";
    IException(e, IException::Unknown, msg, _FILEINFO_).print();
  }

}

void outputModelStatus(BulletShapeModel &bulletModel) {
  qDebug() << "Bullet shape model status:";
  qDebug() << "  Name: " << bulletModel.name();
  qDebug() << "  Tolerance: " << bulletModel.getTolerance();
  qDebug() << "Model has intersection? " << bulletModel.hasIntersection();
  SurfacePoint *intersection = NULL;
  if ( bulletModel.hasIntersection() ) {
    intersection = bulletModel.surfaceIntersection();
    qDebug() << "  Surface Point: ("
             << intersection->GetX().kilometers() << ", "
             << intersection->GetY().kilometers() << ", "
             << intersection->GetZ().kilometers() << ")";
  }
  qDebug() << "Model has normal? " << bulletModel.hasLocalNormal();
  std::vector<double> normal;
  if ( bulletModel.hasLocalNormal() ) {
    normal = bulletModel.localNormal();
    qDebug() << "  Local Normal: ("
             << normal[0] << ", "
             << normal[1] << ", "
             << normal[2] << ")";
    if ( bulletModel.hasIntersection() ) {
      bulletModel.calculateDefaultNormal();
      normal = bulletModel.normal();
      qDebug() << "  Ellipsoid Normal: ("
               << normal[0] << ", "
               << normal[1] << ", "
               << normal[2] << ")";
      bulletModel.setLocalNormalFromIntercept();
      normal = bulletModel.localNormal();
      qDebug() << "  Recalculated Local Normal: ("
               << normal[0] << ", "
               << normal[1] << ", "
               << normal[2] << ")";
    }
  }
  qDebug() << "";
}

void testCameraToGround(std::vector<double> &observer,
                        std::vector<double> &lookDir,
                        BulletShapeModel &bulletModel) {
  qDebug() << "Observer position: ("
           << observer[0] << ", "
           << observer[1] << ", "
           << observer[2] << ")";
  qDebug() << "Look direction:    ("
           << lookDir[0] << ", "
           << lookDir[1] << ", "
           << lookDir[2] << ")";
  qDebug() << "";
  qDebug() << "Intersected: " << bulletModel.intersectSurface(observer, lookDir);
  qDebug() << "";
  outputModelStatus(bulletModel);
}

void testGroundToCamera(Latitude &lat,
                        Longitude &lon,
                        std::vector<double> &observer,
                        BulletShapeModel &bulletModel) {
  qDebug() << "Latitude:  " << lat.degrees();
  qDebug() << "Longitude: " << lon.degrees();
  qDebug() << "";
  qDebug() << "Intersected without occlusion test: " << bulletModel.intersectSurface(lat, lon, observer, false);
  qDebug() << "";
  qDebug() << "Intersected with occlusion test: " << bulletModel.intersectSurface(lat, lon, observer, true);
  qDebug() << "";
  outputModelStatus(bulletModel);
}

void testGroundPointToCamera(Latitude &lat,
                             Longitude &lon,
                             std::vector<double> &observer,
                             BulletShapeModel &bulletModel) {
  qDebug() << "Latitude:     " << lat.degrees();
  qDebug() << "Longitude:    " << lon.degrees();
  Distance pointRadius = bulletModel.localRadius(lat, lon);
  qDebug() << "Local Radius: " << pointRadius.kilometers();
  SurfacePoint groundPoint(lat ,lon, pointRadius);
  qDebug() << "Ground Point: ("
           << groundPoint.GetX().kilometers() << ", "
           << groundPoint.GetY().kilometers() << ", "
           << groundPoint.GetZ().kilometers() << ")";
  qDebug() << "";
  qDebug() << "Intersected without occlusion test: " << bulletModel.intersectSurface(groundPoint, observer, false);
  qDebug() << "";
  qDebug() << "Intersected with occlusion test: " << bulletModel.intersectSurface(groundPoint, observer, true);
  qDebug() << "";
  outputModelStatus(bulletModel);
}
