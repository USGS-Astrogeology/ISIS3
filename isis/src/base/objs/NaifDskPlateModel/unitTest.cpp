/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <QDebug>

#include "Angle.h"
#include "FileName.h"
#include "IException.h"
#include "Intercept.h"
#include "Latitude.h"
#include "Longitude.h"
#include "NaifDskApi.h"
#include "NaifDskPlateModel.h"
#include "Preference.h"
#include "SurfacePoint.h"
#include "QRegularExpression"

using namespace Isis;

/**
 *
 * @internal
 *   @history 2015-02-25 Jeannie Backer - Original version.
 *                           Code coverage: 91.667% scope, 95.960% line, and 100% function.
 *                           Unable to create circumstances for missing coverage.
 *
 *   @todo - Test coverage - Constructor error throw - get dsk file openDSK returns NULL
 *   @todo - Test coverage - openDSK error throw - get dsk file with no segments
 *
 *
 *   @history 2017-05-19 Christopher Combs - Added ReportError method to remove paths that would
 *                           cause the test to fail when not using the standard data area.
 *                           Fixes #4738.
 *
 */

 void ReportError(QString err) {
   qDebug() << err.replace(QRegularExpression("(\\/[\\w\\-\\. ]*)+\\/base"), "base") << Qt::endl;
 }

int main(int argc, char *argv[]) {
  try {
    Preference::Preferences(true);
    qDebug() << "Unit test for NaifDskPlateModel.";
    qDebug() << "";

    qDebug() << "Default constructor.";
    NaifDskPlateModel naifPlateModel;
    qDebug() << "default object is valid?" << naifPlateModel.isValid();
    qDebug() << "File name:          " << naifPlateModel.filename();
    qDebug() << "Number of Plates:   " << naifPlateModel.numberPlates();
    qDebug() << "Number of Vertices: " << naifPlateModel.numberVertices();
    qDebug() << "";

    qDebug() << "Construct NaifDskPlateModel object from NAIF DSK file and retrieve basic info.";
    FileName dskFile("$hayabusa/kernels/dsk/hay_a_amica_5_itokawashape_v1_0_512q.bds");
    NaifDskPlateModel naifPlateModelFromDSK(QString::fromStdString(dskFile.expanded()));
    qDebug() << "object is valid? " << naifPlateModelFromDSK.isValid();
    FileName file(naifPlateModelFromDSK.filename().toStdString());
    qDebug() << "File name:          " << QString::fromStdString(file.baseName());
    qDebug() << "Size:               " << naifPlateModelFromDSK.size();
    qDebug() << "Number of plates:   " << naifPlateModelFromDSK.numberPlates();
    qDebug() << "Number of vertices: " << naifPlateModelFromDSK.numberVertices();
    qDebug() << "";

    qDebug() << "Look for surface point at equator, lon 0.0";
    Latitude lat(0.0, Angle::Degrees);
    Longitude lon(0.0, Angle::Degrees);
    SurfacePoint *sp = naifPlateModelFromDSK.point(lat, lon);
    qDebug() << "Surface point at pole is null?     " << QString::fromStdString(Isis::toString(sp == NULL));
    qDebug() << "Surface point: " << sp->GetX().meters()
                                  << sp->GetY().meters()
                                  << sp->GetZ().meters()
                                  << " meters";
    qDebug() << "";

    qDebug() << "Look for intercept with obsever position and look direction ray:";
    NaifVertex obsPos(3);
    NaifVector rayDir(3);
    NaifVertex xpoint(3);
    obsPos[0] = 0.0;   obsPos[1] = 0.0;   obsPos[2] = 0.0;
    rayDir[0] = 1.0;   rayDir[1] = 1.0;   rayDir[2] = 1.0;
    xpoint[0] = 2.0;   xpoint[1] = 2.0;   xpoint[2] = 2.0;
    Intercept *intercept = naifPlateModelFromDSK.intercept(obsPos, rayDir);
    qDebug() << "Ray Dir:            " << rayDir;
    qDebug() << "Observer:           " << obsPos;
    qDebug() << "Intercept is null?  " << QString::fromStdString(Isis::toString(intercept == NULL));
    qDebug() << "intercept plateID?  "
             << naifPlateModelFromDSK.plateIdOfIntercept(obsPos, rayDir, xpoint);

    // Find obs/rayDir with valid intercept
    obsPos[0]  = 1000.0;  obsPos[1]  = 0.0;   obsPos[2]  = 0.0;
    rayDir[0] = -1.0;  rayDir[1] = 0.0;  rayDir[2] = 0.0;
    intercept = naifPlateModelFromDSK.intercept(obsPos, rayDir);
    qDebug() << "Ray Dir:            " << rayDir;
    qDebug() << "Observer:           " << obsPos;
    qDebug() << "Intercept is null?  " << QString::fromStdString(Isis::toString(intercept == NULL));
    qDebug() << "intercept plate name                 = " << intercept->shape()->name();
    qDebug() << "intercept vertex (obsPos position) = " << intercept->observer();
    qDebug() << "intercept vector (look direction)    = " << intercept->lookDirectionRay();
    SurfacePoint xp = intercept->location();
    xpoint[0] = xp.GetX().meters();
    xpoint[1] = xp.GetY().meters();
    xpoint[2] = xp.GetZ().meters();
    qDebug() << "intercept surface point (location)   = " << xpoint << " meters";
    qDebug() << "intercept plateID                    =  "
             << naifPlateModelFromDSK.plateIdOfIntercept(obsPos, rayDir, xpoint);
    qDebug() << "";

    qDebug() << "Get plate info from id:";
    qDebug() << "Is plate ID = 0 valid? " << naifPlateModelFromDSK.isPlateIdValid(0);
    qDebug() << "Is plate ID = 1 valid? " << naifPlateModelFromDSK.isPlateIdValid(1);
    qDebug() << "Is plate ID = 1 valid for invalid NaifDskPlateModel? "
             << naifPlateModel.isPlateIdValid(1);
    qDebug() << "Triangular Plate for ID = 1:";
    qDebug() << naifPlateModelFromDSK.plate(1);
    qDebug() << "";

    // currently there is a clone() method prototype, but no implementation.
    // the following is a test if the method is ever implemented...
    // NaifDskPlateModel *clonePlateModel = naifPlateModelFromDSK.clone();
    // qDebug() << clonePlateModel->isValid();
    // qDebug() << clonePlateModel->filename();
    // qDebug() << clonePlateModel->size();
    // qDebug() << clonePlateModel->numberPlates();
    // qDebug() << clonePlateModel->numberVertices();
    // qDebug() << "";

    qDebug() << "================================= Error Throws ==================================";
//  qDebug() << "Thrown from Constructor: Create object where openDSK() returns NULL.";
//    try {
//      FileName junkFile("");
//      NaifDskPlateModel naifPlateModelFromDSK(junkFile.expanded());
//    }
//    catch (IException &e) {
//      e.print();
//    }
//    qDebug() << "";
    qDebug() << "Thrown from plateIdOfIntercept(): Get plate ID of intercept with invalid obsPos.";
    try {
      NaifVertex badObs(2);
      badObs[0] = 0.0; badObs[1] = 0.0;
      naifPlateModelFromDSK.plateIdOfIntercept(badObs, rayDir, xpoint);
    }
    catch (IException &e) {
      e.print();
    }
    qDebug() << "";
    qDebug() << "Thrown from plate(): Get plate from invalid plate ID.";
    try {
      naifPlateModelFromDSK.plate(0);
    }
    catch (IException &e) {
      e.print();
    }
    qDebug() << "";
    qDebug() << "Thrown from openDSK(): Open DSK file that doesn't exist.";
    try {
      FileName junkFile("./junk.bds");
      NaifDskPlateModel naifPlateModelFromDSK(QString::fromStdString(junkFile.expanded()));
    }
    catch (IException &e) {
      e.print();
    }
    qDebug() << "";
//    qDebug() << "Thrown from openDSK(): Open DSK file with no segments.";
//    try {
//      FileName noSegmentsFile("");
//      NaifDskPlateModel junkmodel(noSegmentsFile.expanded());
//    }
//    catch (IException &e) {
//      e.print();
//    }
//    qDebug() << "";
    qDebug() << "~NaifDskDescriptor(): Unknown NAIF error has occured.";
    try {
      FileName junkFile("$base/kernels/spk/de405.bsp");
      NaifDskPlateModel naifPlateModelFromDSK(QString::fromStdString(junkFile.toString()));
    }
    catch (IException &e) {
      ReportError(QString::fromStdString(e.toString()));
    }

  }
  catch (IException &e) {
    qDebug() << "";
    qDebug() << "";
    IException(e, IException::Programmer,
              "\n------------Unit Test Failed.------------",
              _FILEINFO_).print();
  }
}
