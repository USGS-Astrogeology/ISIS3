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
#include "JunoCamera.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlGroup.h"

using namespace std;
using namespace Isis;

void TestSampLine(Camera *cam, double samp, double line);

/**
 * Unit test for Juno's JunoCam instrument.
 *
 * @author 2017-09-11 Jesse Mapel
 * @internal
 *   @history 2017-09-11 Jesse Mapel - Original version.
 *   @history 2017-09-11 Jeannie Backer - Updated test data.
 */
int main(void) {
  Preference::Preferences(true);

  qDebug() << "Unit Test for JunoCamera...";
  try {
    // These should be lat/lon at center of image. To obtain these numbers for a new cube/camera,
    // set both the known lat and known lon to zero and copy the unit test output
    // "Latitude off by: " and "Longitude off by: " values directly into these variables.
    double knownLat = -45.4762320380959295;
    double knownLon = 278.270465938390657;

    Cube c("$ISISTESTDATA/isis/src/juno/unitTestData/JNCE_2013282_00M00099_V01_METHANE_0003.cub", "r");
    JunoCamera *cam = (JunoCamera *) CameraFactory::Create(c);
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

    // Test Shutter Open/Close
    const PvlGroup &inst = c.label()->findGroup("Instrument", Pvl::Traverse);
    double exposureDuration = Isis::toDouble( inst["ExposureDuration"][0] );
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
    TestSampLine(cam, 952.0, 1.0);

    qDebug() << "For upper right corner ...";
    TestSampLine(cam, 1630.0, 1.0);

    qDebug() << "For lower left corner ...";
    TestSampLine(cam, 1005.0, cam->Lines());

    qDebug() << "For lower right corner ...";
    TestSampLine(cam, 1630.0, cam->Lines());

    double samp = 1300;
    double line = 64;
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

void TestSampLine(Camera *cam, double samp, double line) {
  bool success = cam->SetImage(samp, line);

  if(success) {
    success = cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude());
  }

  if(success) {
    double deltaSamp = samp - cam->Sample();
    double deltaLine = line - cam->Line();
    if(fabs(deltaSamp) < 1.0e-3) deltaSamp = 0.0;
    if(fabs(deltaLine) < 1.0e-3) deltaLine = 0.0;
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
