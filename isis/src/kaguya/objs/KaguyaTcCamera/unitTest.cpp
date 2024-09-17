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
#include "KaguyaTcCamera.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlGroup.h"

using namespace std;
using namespace Isis;

void TestLineSamp(Camera *cam, double samp, double line);

/**
 * Unit test for Kaguya TC Camera.
 *
 * @internal
 *   @history 2018-10-05 Jeannie Backer and Adam Goins - Original version.
 */
int main(void) {
  Preference::Preferences(true);

  qDebug() << "Unit Test for KaguyaTcCamera...";
  try {
    // These should be lat/lon at center of image. To obtain these numbers for a new cube/camera,
    // set both the known lat and known lon to zero and copy the unit test output
    // "Latitude off by: " and "Longitude off by: " values directly into these variables.
    double knownLat = 58.3524398749999;
    double knownLon = 311.457363494321;

    qDebug() << "Testing TC2 w L2B0 image...";
    Cube c("$ISISTESTDATA/isis/src/kaguya/unitTestData/TC2W2B0_01_02735N583E3115.cub", "r");
    KaguyaTcCamera *cam = (KaguyaTcCamera *) CameraFactory::Create(c);
    qDebug() << "FileName: " <<  QString::fromStdString(FileName(c.fileName().toStdString()).name());
    qDebug() << "CK Frame: " << cam->instrumentRotation()->Frame();
    qDebug() << "";

    // Test kernel IDs
    qDebug() << "Kernel IDs: ";
    qDebug() << "CK Frame ID = " << cam->CkFrameId();
    qDebug() << "CK Reference ID = " << cam->CkReferenceId();
    qDebug() << "SPK Target ID = " << cam->SpkTargetId();
    qDebug() << "SPK Reference ID = " << cam->SpkReferenceId();
    qDebug() << "";

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


    qDebug() << "";
    qDebug() << "";
    qDebug() << "Testing TC1 s L2B0 image...";
    knownLat = -82.0195024182112;
    knownLon = 46.4153943800818;

    Cube c2("$ISISTESTDATA/isis/src/kaguya/unitTestData/TC1S2B0_01_06691S820E0465.cub", "r");
    KaguyaTcCamera *cam2 = (KaguyaTcCamera *) CameraFactory::Create(c2);
    qDebug() << "FileName: " << QString::fromStdString(FileName(c2.fileName().toStdString()).name());
    qDebug() << "CK Frame: " << cam2->instrumentRotation()->Frame();
    qDebug() << "";

    // Test kernel IDs
    qDebug() << "Kernel IDs: ";
    qDebug() << "CK Frame ID = " << cam2->CkFrameId();
    qDebug() << "CK Reference ID = " << cam2->CkReferenceId();
    qDebug() << "SPK Target ID = " << cam2->SpkTargetId();
    qDebug() << "SPK Reference ID = " << cam2->SpkReferenceId();
    qDebug() << "";

    qDebug() << qSetRealNumberPrecision(18) << "Focal Length = " << cam2->FocalLength();
    qDebug() << "";

    // Test all four corners to make sure the conversions are right
    qDebug() << "For upper left corner ...";
    TestLineSamp(cam2, 1.0, 1.0);

    qDebug() << "For upper right corner ...";
    TestLineSamp(cam2, cam2->Samples(), 1.0);

    qDebug() << "For lower left corner ...";
    TestLineSamp(cam2, 1.0, cam2->Lines());

    qDebug() << "For lower right corner ...";
    TestLineSamp(cam2, cam2->Samples(), cam2->Lines());

    samp = cam2->Samples() / 2;
    line = cam2->Lines() / 2;
    qDebug() << "For center pixel position ...";

    if(!cam2->SetImage(samp, line)) {
      qDebug() << "ERROR";
      return 0;
    }

    if(abs(cam2->UniversalLatitude() - knownLat) < 1E-13) {
      qDebug() << "Latitude OK";
    }
    else {
      qDebug() << qSetRealNumberPrecision(18)
               << "Latitude off by: " << cam2->UniversalLatitude() - knownLat;
    }

    if(abs(cam2->UniversalLongitude() - knownLon) < 1E-11) {
      qDebug() << "Longitude OK";
    }
    else {
      qDebug() << qSetRealNumberPrecision(18)
               << "Longitude off by: " << cam2->UniversalLongitude() - knownLon;
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
