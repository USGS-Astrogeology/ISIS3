/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <cmath>

#include <QDebug>
#include <QVector>

#include "Angle.h"
#include "Camera.h"
#include "Cube.h"
#include "Distance.h"
#include "EmbreeTargetShape.h"
#include "FileName.h"
#include "IException.h"
#include "Intercept.h"
#include "Latitude.h"
#include "LinearAlgebra.h"
#include "Longitude.h"
#include "Preference.h"
#include "Pvl.h"
#include "Spice.h"
#include "SurfacePoint.h"
#include "Target.h"

using namespace Isis;

void outputMultiHitRay(RTCMultiHitRay &ray);

void outputOcclusionRay(RTCOcclusionRay &ray);

void outputRayHitInformation(RayHitInformation &hit);

void outputIntersection(EmbreeTargetShape &embreeShape,
                        LinearAlgebra::Vector &observer,
                        LinearAlgebra::Vector &lookDirection);

void outputOcclusion(EmbreeTargetShape &embreeShape,
                     LinearAlgebra::Vector &observer,
                     LinearAlgebra::Vector &lookDirection,
                     unsigned ignorePrimID = RTC_INVALID_GEOMETRY_ID);

double roundToPrecision(double value, double precision);

/** 
 * Unit test for Embree Ray Tracing Kernels
 *
 */
int main(int argc, char *argv[]) {
  try {
    Preference::Preferences(true);

    qDebug() << "RTCMultiHitRay";
    qDebug() << Qt::endl;

    qDebug() << "Creating default ray";
    RTCMultiHitRay defaultRay;
    outputMultiHitRay(defaultRay);
    qDebug() << "";

    qDebug() << "Creating ray from standard vectors";
    std::vector<double> rayOrigin(3,0.0);
    rayOrigin[0] = -3.1; rayOrigin[1] = 1.75; rayOrigin[2] = 10.3;
    std::vector<double> rayDirection(3,0.0);
    rayDirection[0] = 32.4; rayDirection[1] = 15; rayDirection[2] = -1.6;
    RTCMultiHitRay stdRay(rayOrigin, rayDirection);
    outputMultiHitRay(stdRay);
    qDebug() << "";

    qDebug() << "Creating ray from linear algebra vectors";
    LinearAlgebra::Vector linRayOrigin = LinearAlgebra::vector(-3.1, 1.75, 10.3);
    LinearAlgebra::Vector linRayDirection = LinearAlgebra::vector(32.4, 15, -1.6);
    RTCMultiHitRay linRay(linRayOrigin, linRayDirection);
    outputMultiHitRay(linRay);
    qDebug() << Qt::endl;

    qDebug() << "RTCOcclusionRay";
    qDebug() << Qt::endl;

    qDebug() << "Creating default ray";
    RTCOcclusionRay defaultOccRay;
    outputOcclusionRay(defaultOccRay);
    qDebug() << "";

    qDebug() << "Creating ray from standard vectors";
    RTCOcclusionRay stdOccRay(rayOrigin, rayDirection);
    outputOcclusionRay(stdOccRay);
    qDebug() << "";

    qDebug() << "Creating ray from linear algebra vectors";
    RTCOcclusionRay linOccRay(linRayOrigin, linRayDirection);
    outputOcclusionRay(linOccRay);
    qDebug() << Qt::endl;

    qDebug() << "RayHitInformation";
    qDebug() << Qt::endl;

    qDebug() << "Creating default ray hit information";
    RayHitInformation defaultHit;
    outputRayHitInformation(defaultHit);
    qDebug() << "";

    qDebug() << "Creating ray hit information from intersection, normal, and primitive ID";
    RayHitInformation testHit(linRayOrigin, linRayDirection, -10);
    outputRayHitInformation(testHit);
    qDebug() << Qt::endl;

    qDebug() << "EmbreeTargetShape";
    qDebug() << Qt::endl;

    qDebug() << "Creating default target shape";
    EmbreeTargetShape defaultShape;
    qDebug() << "Target shape name: " << defaultShape.name();
    qDebug() << "Target mesh status:";
    qDebug() << "  Number of vertices: " << defaultShape.numberOfVertices();
    qDebug() << "  Number of polygons: " << defaultShape.numberOfVertices();
    RTCBounds defaultBounds = defaultShape.sceneBounds();
    qDebug() << "Scene bounds:";
    qDebug() << "  X min: " << defaultBounds.lower_x;
    qDebug() << "  X max: " << defaultBounds.upper_x;
    qDebug() << "  Y min: " << defaultBounds.lower_y;
    qDebug() << "  Y max: " << defaultBounds.upper_y;
    qDebug() << "  Z min: " << defaultBounds.lower_z;
    qDebug() << "  Z max: " << defaultBounds.upper_z;
    qDebug() << "  Maximum distance: " << defaultShape.maximumSceneDistance();
    qDebug() << Qt::endl;

    QString dskfile("$ISISTESTDATA/isis/src/base/unitTestData/hay_a_amica_5_itokawashape_v1_0_64q.bds");
    qDebug() << "Testing with " << dskfile << "...";
    qDebug() << Qt::endl;

    qDebug() << "Loading shapefile";
    qDebug() << Qt::endl;

    EmbreeTargetShape itokawaShape(dskfile);
    qDebug() << "Target shape name: " << itokawaShape.name();
    qDebug() << "Target mesh status:";
    qDebug() << "  Number of vertices: " << itokawaShape.numberOfVertices();
    qDebug() << "  Number of polygons: " << itokawaShape.numberOfVertices();
    RTCBounds itokawaBounds = itokawaShape.sceneBounds();
    qDebug() << "Scene bounds:";
    qDebug() << "  X min: " << itokawaBounds.lower_x;
    qDebug() << "  X max: " << itokawaBounds.upper_x;
    qDebug() << "  Y min: " << itokawaBounds.lower_y;
    qDebug() << "  Y max: " << itokawaBounds.upper_y;
    qDebug() << "  Z min: " << itokawaBounds.lower_z;
    qDebug() << "  Z max: " << itokawaBounds.upper_z;
    qDebug() << "  Maximum distance: " << itokawaShape.maximumSceneDistance();
    qDebug() << Qt::endl;

    // Vectors for observer-look direction intersections
    LinearAlgebra::Vector observer(3);
    LinearAlgebra::Vector lookDirection(3);

    qDebug() << "Testing axes ray intersection:";
    qDebug() << Qt::endl;

    observer[0] = 1000.0; observer[1] = 0.0; observer[2] = 0.0;
    lookDirection[0] = -1.0; lookDirection[1] = 0.0; lookDirection[2] = 0.0;
    outputIntersection(itokawaShape, observer, lookDirection);
    qDebug() << Qt::endl;

    observer[0] = 0.0; observer[1] = 1000.0; observer[2] = 0.0;
    lookDirection[0] = 0.0; lookDirection[1] = -1.0; lookDirection[2] = 0.0;
    outputIntersection(itokawaShape, observer, lookDirection);
    qDebug() << Qt::endl;

    observer[0] = 0.0; observer[1] = 0.0; observer[2] = 1000.0;
    lookDirection[0] = 0.0; lookDirection[1] = 0.0; lookDirection[2] = -1.0;
    outputIntersection(itokawaShape, observer, lookDirection);
    qDebug() << Qt::endl;

    qDebug() << "Testing offset ray intersection:";
    qDebug() << Qt::endl;

    observer[0] = 100.0; observer[1] = 0.05; observer[2] = 0.0;
    lookDirection[0] = -1.0; lookDirection[1] = 0.0; lookDirection[2] = 0.0;
    outputIntersection(itokawaShape, observer, lookDirection);
    qDebug() << Qt::endl;

    observer[0] = 0.0; observer[1] = 100.0; observer[2] = 0.05;
    lookDirection[0] = 0.0; lookDirection[1] = -1.0; lookDirection[2] = 0.0;
    outputIntersection(itokawaShape, observer, lookDirection);
    qDebug() << Qt::endl;

    observer[0] = 0.05; observer[1] = 0.0; observer[2] = 100.0;
    lookDirection[0] = 0.0; lookDirection[1] = 0.0; lookDirection[2] = -1.0;
    outputIntersection(itokawaShape, observer, lookDirection);
    qDebug() << Qt::endl;

    qDebug() << "Testing diagonal ray intersection:";
    qDebug() << Qt::endl;

    observer[0] = 10.0; observer[1] = 10.05; observer[2] = 0.0;
    lookDirection[0] = -1.0; lookDirection[1] = -1.0; lookDirection[2] = 0.0;
    lookDirection = LinearAlgebra::normalize(lookDirection);
    outputIntersection(itokawaShape, observer, lookDirection);
    qDebug() << Qt::endl;

    observer[0] = 0.0; observer[1] = 10.0; observer[2] = 10.05;
    lookDirection[0] = 0.0; lookDirection[1] = -1.0; lookDirection[2] = -1.0;
    lookDirection = LinearAlgebra::normalize(lookDirection);
    outputIntersection(itokawaShape, observer, lookDirection);
    qDebug() << Qt::endl;

    observer[0] = 10.05; observer[1] = 0.0; observer[2] = 10.0;
    lookDirection[0] = -1.0; lookDirection[1] = 0.0; lookDirection[2] = -1.0;
    lookDirection = LinearAlgebra::normalize(lookDirection);
    outputIntersection(itokawaShape, observer, lookDirection);
    qDebug() << Qt::endl;

    observer[0] = 10.0; observer[1] = 10.0; observer[2] = 10.0;
    lookDirection[0] = -1.0; lookDirection[1] = -1.0; lookDirection[2] = -1.0;
    lookDirection = LinearAlgebra::normalize(lookDirection);
    outputIntersection(itokawaShape, observer, lookDirection);
    qDebug() << Qt::endl;

    qDebug() << "Testing ray occlusion:";
    qDebug() << Qt::endl;

    observer[0] = 3.0; observer[1] = 2.0; observer[2] = 1.0;
    lookDirection[0] = -3.0; lookDirection[1] = -2.0; lookDirection[2] = -1.0;
    lookDirection = LinearAlgebra::normalize(lookDirection);
    outputOcclusion(itokawaShape, observer, lookDirection);
    qDebug() << Qt::endl;

    observer[0] = 3.0; observer[1] = 2.0; observer[2] = 1.0;
    lookDirection[0] = -3.1; lookDirection[1] = -1.9; lookDirection[2] = -1.1;
    lookDirection = LinearAlgebra::normalize(lookDirection);
    outputOcclusion(itokawaShape, observer, lookDirection);
    qDebug() << Qt::endl;

    observer[0] = 1.0; observer[1] = 1.0; observer[2] = 1.0;
    lookDirection[0] = -0.9; lookDirection[1] = -1.1; lookDirection[2] = -1.0;
    lookDirection = LinearAlgebra::normalize(lookDirection);
    outputOcclusion(itokawaShape, observer, lookDirection, 36496);
    qDebug() << Qt::endl;

    qDebug() << "Testing error throws";
    qDebug() << Qt::endl;

    qDebug() << "Invalid shapefile";
    try {
      EmbreeTargetShape invalidShapefile("junkyshapefile.bds");
    }
    catch (IException &e) {
      e.print();
    }
    try {
      EmbreeTargetShape invalidShapefile("junkydem.cub");
    }
    catch (IException &e) {
      e.print();
    }
    qDebug() << "";

    qDebug() << "Out of bounds intersection access";
    try {
      RTCMultiHitRay unintersectedRay;
      RayHitInformation badInformation = itokawaShape.getHitInformation(unintersectedRay, 0);
    }
    catch (IException &e) {
      e.print();
    }
    qDebug() << "";
  }
  catch (IException &e) {
    qDebug() << "";
    qDebug() << "";
    std::string msg = "**************** UNIT TEST FAILED! **************** ";
    IException(e, IException::Unknown, msg, _FILEINFO_).print();
  }
}

void outputIntersection(EmbreeTargetShape &embreeShape,
                        LinearAlgebra::Vector &observer,
                        LinearAlgebra::Vector &lookDirection) {
  qDebug() << "Testing target shape intersection";
  qDebug() << "";
  qDebug() << "Ray information:";
  qDebug() << "  Observer position: ("
           << observer[0] << ","
           << observer[1] << ","
           << observer[2] << ")";
  qDebug() << "  Look direction: ("
           << lookDirection[0] << ","
           << lookDirection[1] << ","
           << lookDirection[2] << ")";
  qDebug() << "";
  RTCMultiHitRay ray(observer, lookDirection);
  embreeShape.intersectRay(ray);
  qDebug() << "Number of intersections: " << (ray.lastHit + 1);
  qDebug() << "";
  for (int i = 0; i <= ray.lastHit; i++) {
    RayHitInformation intersectionInfo = embreeShape.getHitInformation(ray, i);
    qDebug() << "Intersection" << (i + 1) << "information:";
    qDebug() << "  Primitive ID:  " << intersectionInfo.primID;
    qDebug() << "  Intersection:   ("
            << roundToPrecision(intersectionInfo.intersection[0], 0.0001) << ","
            << roundToPrecision(intersectionInfo.intersection[1], 0.0001) << ","
            << roundToPrecision(intersectionInfo.intersection[2], 0.0001) << ")";
    qDebug() << "  Surface normal: ("
            << intersectionInfo.surfaceNormal[0] << ","
            << intersectionInfo.surfaceNormal[1] << ","
            << intersectionInfo.surfaceNormal[2] << ")";
  }
}

void outputOcclusion(EmbreeTargetShape &embreeShape,
                     LinearAlgebra::Vector &observer,
                     LinearAlgebra::Vector &lookDirection,
                     unsigned ignorePrimID) {
  qDebug() << "Testing ray occlusion";
  qDebug() << "";
  qDebug() << "Ray information:";
  qDebug() << "  Observer position: ("
           << observer[0] << ","
           << observer[1] << ","
           << observer[2] << ")";
  qDebug() << "  Look direction: ("
           << lookDirection[0] << ","
           << lookDirection[1] << ","
           << lookDirection[2] << ")";
  qDebug() << "";
  RTCOcclusionRay ray(observer, lookDirection);
  ray.ignorePrimID = ignorePrimID;
  qDebug() << "Ray is occluded? " << embreeShape.isOccluded(ray);
}

void outputMultiHitRay(RTCMultiHitRay &ray) {
  qDebug() << "Multi Hit Ray Status";
  qDebug() << "  Origin: (" << ray.ray.org_x << "," << ray.ray.org_y << "," << ray.ray.org_z << ")";
  qDebug() << "  Direction: (" << ray.ray.dir_x << "," << ray.ray.dir_y << "," << ray.ray.dir_z << ")";
  qDebug() << "  Near distance: " << ray.ray.tnear;
  qDebug() << "  Far distance: " << ray.ray.tfar;
  qDebug() << "  Normal: (" << ray.hit.Ng_x << "," << ray.hit.Ng_y << "," << ray.hit.Ng_z << ")";
  qDebug() << "  Intersection U: " << ray.hit.u;
  qDebug() << "  Intersection V: " << ray.hit.v;
  qDebug() << "  Geometry ID: " << ray.hit.geomID;
  qDebug() << "  Primitive ID: " << ray.hit.primID;
  qDebug() << "  Instance ID: " << ray.hit.instID[0];
  qDebug() << "  Number of Intersections: " << ray.lastHit + 1;
  for (int i = 0; i <= ray.lastHit; i++) {
    qDebug() << "  Intersection" << i + 1;
    qDebug() << "    Geometry ID: " << ray.hitGeomIDs[i];
    qDebug() << "    Primitive ID: " << ray.hitPrimIDs[i];
    qDebug() << "    Intersection U: " << ray.hitUs[i];
    qDebug() << "    Intersection V: " << ray.hitVs[i];
  }
}

void outputOcclusionRay(RTCOcclusionRay &ray) {
  qDebug() << "Occlusion Ray Status";
  qDebug() << "  Origin: (" << ray.ray.org_x << "," << ray.ray.org_y << "," << ray.ray.org_z << ")";
  qDebug() << "  Direction: (" << ray.ray.dir_x << "," << ray.ray.dir_y << "," << ray.ray.dir_z << ")";
  qDebug() << "  Near distance: " << ray.ray.tnear;
  qDebug() << "  Far distance: " << ray.ray.tfar;
  qDebug() << "  Normal: (" << ray.hit.Ng_x << "," << ray.hit.Ng_y << "," << ray.hit.Ng_z << ")";
  qDebug() << "  Intersection U: " << ray.hit.u;
  qDebug() << "  Intersection V: " << ray.hit.v;
  qDebug() << "  Geometry ID: " << ray.hit.geomID;
  qDebug() << "  Instance ID: " << ray.hit.instID[0];
  qDebug() << "  Ignored Primitive ID: " << ray.ignorePrimID;
}

void outputRayHitInformation(RayHitInformation &hit) {
  qDebug() << "Ray Hit Information";
  qDebug() << "  Primitive ID: " << hit.primID;
  qDebug() << "  Intersection: (" << roundToPrecision(hit.intersection[0], 0.0001) << ","
                                  << roundToPrecision(hit.intersection[1], 0.0001) << "," 
                                  << roundToPrecision(hit.intersection[2], 0.0001) << ")";
  qDebug() << "  Surface Normal: (" << hit.intersection[0] << ","
                                    << hit.intersection[1] << "," 
                                    << hit.intersection[2] << ")";
}

double roundToPrecision(double value, double precision) {
  return value - fmod(value, precision);
}