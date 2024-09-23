/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QDebug>

#include <iomanip>
#include <iostream>

#include "Camera.h"
#include "CameraFactory.h"
#include "FileName.h"
#include "IException.h"
#include "iTime.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TgoCassisCamera.h"

using namespace std;
using namespace Isis;

void TestLineSamp(Camera *cam, double samp, double line);

/**
 * Unit test for TGO CaSSIS camera.
 *
 * @internal
 *   @history 2018-08-15 Jeannie Backer - Updated lat/lon changes due to
 *                           changes in focal length.
 */
int main(void) {
  Preference::Preferences(true);

  qDebug() << "Unit Test for TgoCassisCamera...";
  try {
    // These should be lat/lon at center of image. To obtain these numbers for a new cube/camera,
    // set both the known lat and known lon to zero and copy the unit test output
    // "Latitude off by: " and "Longitude off by: " values directly into these variables.
    double knownLat = 4.14667346682538351; // 4.14700320539339717;
    double knownLon = 322.757314935797012; // 322.7582512383878;

    Cube c("$ISISTESTDATA/isis/src/tgo/unitTestData/CAS-MCO-2016-11-22T16.38.39.354-NIR-02036-00.cub", "r");
    TgoCassisCamera *cam = (TgoCassisCamera *) CameraFactory::Create(c);
    qDebug() << "FileName: " <<  QString::fromStdString(FileName(c.fileName().toStdString()).name());
    qDebug() << "Instrument Rotation Frame: " << cam->instrumentRotation()->Frame();
    qDebug() << "";

    // Test kernel IDs
    qDebug() << "Kernel IDs: ";
    qDebug() << "CK Frame ID = " << cam->CkFrameId();
    qDebug() << "CK Reference ID = " << cam->CkReferenceId();
    qDebug() << "SPK Target ID = " << cam->SpkTargetId();
    qDebug() << "SPK Reference ID = " << cam->SpkReferenceId();
    qDebug() << "";

    // Test Shutter Open/Close
    const PvlGroup &inst = c.label()->findGroup("Instrument", Pvl::Traverse);
    double exposureDuration = IString::ToDouble( inst["ExposureDuration"][0] );
    QString stime = QString::fromStdString(inst["StartTime"]);
    double et;
    str2et_c(stime.toLatin1().data(), &et);
    pair <iTime, iTime> shuttertimes = cam->ShutterOpenCloseTimes(et, exposureDuration);
    qDebug() << qSetRealNumberPrecision(18) << "Shutter open = " << shuttertimes.first.Et();
    qDebug() << qSetRealNumberPrecision(18) << "Shutter close = " << shuttertimes.second.Et();
    qDebug() << qSetRealNumberPrecision(18) << "Focal Length = " << cam->FocalLength();
    qDebug() << "";

    // Test all four corners to make sure the conversions are right
    qDebug() << "For upper left corner ...";
    TestLineSamp(cam, 1.0, 1.0);

    qDebug() << "For upper right corner ...";
    TestLineSamp(cam, cam->Samples(), 1.0);

    qDebug() << "For lower left corner ...";
    TestLineSamp(cam, 1.0, cam->Lines());

    qDebug() << "For lower right corner ...";
    TestLineSamp(cam, cam->Samples(), cam->Lines());

    double samp = cam->Samples() / 2;
    double line = cam->Lines() / 2;
    qDebug() << "For center pixel position ...";

    if(!cam->SetImage(samp, line)) {
      qDebug() << "ERROR";
      return 0;
    }

    if(abs(cam->UniversalLatitude() - knownLat) < 1E-13) {
      qDebug() << "Latitude OK";
    }
    else {
      qDebug() << qSetRealNumberPrecision(18)
               << "Latitude off by: " << cam->UniversalLatitude() - knownLat;
    }

    if(abs(cam->UniversalLongitude() - knownLon) < 1E-11) {
      qDebug() << "Longitude OK";
    }
    else {
      qDebug() << qSetRealNumberPrecision(18)
               << "Longitude off by: " << cam->UniversalLongitude() - knownLon;
    }
  }
  catch(IException &e) {
    e.print();
  }
}

void TestLineSamp(Camera *cam, double samp, double line) {
  bool success = cam->SetImage(samp, line);

  if(success) {
    success = cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude());
  }

  if(success) {
    double deltaSamp = samp - cam->Sample();
    double deltaLine = line - cam->Line();
    if(fabs(deltaSamp) < 1.1e-2) deltaSamp = 0.0;
    if(fabs(deltaLine) < 1.0e-2) deltaLine = 0.0;
    qDebug() << "DeltaSample = " << deltaSamp;
    qDebug() << "DeltaLine = " << deltaLine;
    qDebug() << "";
  }
  else {
    qDebug() << "DeltaSample = ERROR";
    qDebug() << "DeltaLine = ERROR";
    qDebug() << "";
  }
}
