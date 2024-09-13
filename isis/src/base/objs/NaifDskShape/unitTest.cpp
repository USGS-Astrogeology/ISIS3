/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <QDebug>
#include <QVector>

#include "Angle.h"
#include "Cube.h"
#include "Displacement.h"
#include "Distance.h"
#include "FileName.h"
#include "IException.h"
#include "Intercept.h"
#include "Latitude.h"
#include "Longitude.h"
#include "NaifDskApi.h"
#include "NaifDskPlateModel.h"
#include "NaifDskShape.h"
#include "Preference.h"
#include "Pvl.h"
#include "Spice.h"
#include "SurfacePoint.h"
#include "Target.h"

using namespace Isis;

/** 
 * Unit test for NAIF DSK shape model 
 *  
 * @internal 
 *   @history 2015-02-25 Jeannie Backer - Original version.
 *                           Code coverage: 88.889% scope, 93.671% line, and 100% function.
 *                           Unable to create circumstances for missing coverage.
 *   @history 2015-03-14 Jeannie Backer - Added checks for has local normal. Updated truth data.
 *                           Removing lines of code in calcualteLocalNormal() method resulted in
 *                           coverage improvement: 94.595% scope, 98.077% line, and 100% function.
  *   @history 2015-04-30 Jeannie Backer - Added a test for calculateLocalNormal() with
  *                           empty neighbor points and a test for isDEM(). References #2243.
 *  
 *   @todo - Test coverage - localRadius(lat,lon) creates null surface point
 *  
 *   testcoverage 2015-04-30 - 94.737% scope, 98.148% line, 100% function
 *
 */
int main(int argc, char *argv[]) {
  try {
    Preference::Preferences(true);
    qDebug() << "Unit test for NaifDskShape.";
    qDebug() << "";

    qDebug() << "Default constructor.";
    NaifDskShape shapeModel;
    qDebug() << "Shape name is " << shapeModel.name();
    qDebug() << "Shape is DEM type? " << QString::number(shapeModel.isDEM());

    qDebug() << "";

    qDebug() << "Construct NaifDskShape object from a plate model.";    
    FileName dskFile("$hayabusa/kernels/dsk/hay_a_amica_5_itokawashape_v1_0_512q.bds");
    NaifDskPlateModel plateModel(QString::fromStdString(dskFile.expanded()));
    NaifDskShape shapeModelFromPlate(plateModel);
    qDebug() << "Try to intersect surface at obsPos (0,0,0) and lookDir (1,1,1)";
    std::vector<double> obsPos; 
    std::vector<double> lookDir; 
    obsPos.push_back(0.0);   obsPos.push_back(0.0);   obsPos.push_back(0.0);
    lookDir.push_back(1.0);  lookDir.push_back(1.0);  lookDir.push_back(1.0);
    bool success = shapeModelFromPlate.intersectSurface(obsPos, lookDir);
    qDebug() << "Intersection successful? " << QString::number(success);
    qDebug() << "";
     
    qDebug() << "Construct NaifDskShape object from cube labels with ShapeModel=DSK file.";
    Pvl pvlWithShape("./st_2530292409_v_DskShapeModel.lbl");
    Target targetSh(NULL, pvlWithShape);
    NaifDskShape shapeModelFromPvlShape(&targetSh, pvlWithShape);
    qDebug() << "Try to intersect surface at obsPos (0,0,1000) and lookDir (0,-1,-1)";
    obsPos[0]  = 0.0;  obsPos[1]  = 0.0;   obsPos[2]  = 1000.0;
    lookDir[0] = 0.0;  lookDir[1] = -1.0;  lookDir[2] = -1.0;
    success = shapeModelFromPvlShape.intersectSurface(obsPos, lookDir);
    qDebug() << "Intersection successful?     " << QString::number(success);

    qDebug() << "Try to intersect surface at obsPos (1000,0,0) and ground point (1,0,0)";
    Displacement x(1.0, Displacement::Meters);
    Displacement y(0.0, Displacement::Meters);
    Displacement z(0.0, Displacement::Meters);
    SurfacePoint SurfPt(x, y, z);
    obsPos[0]  = 1000.0;  obsPos[1]  = 0.0;   obsPos[2]  = 0.0;
    success = shapeModelFromPvlShape.intersectSurface(SurfPt, obsPos);
    if (success) {
      SurfacePoint *point = shapeModelFromPvlShape.surfaceIntersection();
      qDebug() << "Intercept X = " << point->DisplacementToDouble(point->GetX(), SurfacePoint::Kilometers);
      qDebug() << "Intercept Y = " << point->DisplacementToDouble(point->GetY(), SurfacePoint::Kilometers);
      qDebug() << "Intercept Z = " << point->DisplacementToDouble(point->GetZ(), SurfacePoint::Kilometers);
    }
    qDebug() << "Intersection successful?     " << QString::number(success);

    qDebug() << "";
    qDebug() << "Construct NaifDskShape object from cube labels with ElevationModel=DSK file.";    
    Pvl pvlWithElevation("./st_2530292409_v_DskElevationModel.lbl");
    Target targetEl(NULL, pvlWithElevation);
    Distance meter(1, Distance::Meters);
    std::vector<Distance> radii;
    radii.push_back(meter);
    radii.push_back(meter);
    radii.push_back(meter);
    targetEl.setRadii(radii);
    NaifDskShape shapeModelFromPvlElevation(&targetEl, pvlWithElevation);
    qDebug() << "Try to intersect surface at obsPos (1000,0,0) and lookDir (-1,0,0)";
    obsPos[0]  = 1000.0;  obsPos[1]  = 0.0;   obsPos[2]  = 0.0;
    lookDir[0] = -1.0;  lookDir[1] = 0.0;  lookDir[2] = 0.0;
    success = shapeModelFromPvlElevation.intersectSurface(obsPos, lookDir);
    qDebug() << "Intersection successful?     " << QString::number(success);
    qDebug() << "Intersection valid? " << shapeModelFromPvlElevation.surfaceIntersection()->Valid();

    const Intercept *intercept = shapeModelFromPvlElevation.intercept();
    qDebug() << "Intercept is null?  " << QString::number(intercept == NULL);
    SurfacePoint xp = intercept->location();
    NaifVertex xpoint(3);
    xpoint[0] = xp.GetX().meters();
    xpoint[1] = xp.GetY().meters();
    xpoint[2] = xp.GetZ().meters();
    qDebug() << "intercept surface point (location)   = " << xpoint << " meters";
    qDebug() << "";

    qDebug() << "";
    qDebug() << "Now that we have a surface point, testing intersectSurface using surfacepoint"
                " from parent class..."; 
    shapeModelFromPvlElevation.intersectSurface(xp, obsPos);
    qDebug() << "Do we have an intersection? " << shapeModelFromPvlElevation.hasIntersection();

    qDebug() << "Testing intersectSurface using lat/lon from parent class..."; 
    shapeModelFromPvlElevation.intersectSurface(xp.GetLatitude(), xp.GetLongitude(), obsPos);
    qDebug() << "Do we have an intersection? " << shapeModelFromPvlElevation.hasIntersection();
    qDebug() << "";

    qDebug() << "Find local radius given lat/lon";
    Latitude lat(0, Angle::Degrees);
    Longitude lon(0, Angle::Degrees);
    Distance rad = shapeModelFromPvlElevation.localRadius(lat, lon);
    qDebug() << "Local radius at (" << lat.degrees() << ", " 
                                    << lon.degrees() << ") is valid? " << rad.isValid();
    qDebug() << "Local radius:                " << rad.meters() << "meters";
    qDebug() << "";

    qDebug() << "Access the associated NaifDskPlateModel.";
    NaifDskPlateModel plateModelFromShape = shapeModelFromPvlElevation.model();
    qDebug() << "Plate is valid?              " << plateModelFromShape.isValid();
    FileName file(plateModelFromShape.filename().toStdString());
    qDebug() << "Plate file name:             " << QString::fromStdString(file.baseName());
    qDebug() << "Plate size:                  " << plateModelFromShape.size();
    qDebug() << "Number of plates:            " << plateModelFromShape.numberPlates();
    qDebug() << "Number of vertices:          " << plateModelFromShape.numberVertices();
    qDebug() << "";


    qDebug() << "Try to calculate norms using valid shape model...";
    shapeModelFromPvlElevation.setLocalNormalFromIntercept();
    qDebug() << "Has intercept normal?                " << shapeModelFromPvlElevation.hasLocalNormal();
    std::vector<double> normal = shapeModelFromPvlElevation.localNormal();
    qDebug() << "Normal set from Intercept:           "
             << QVector<double>(normal.begin(), normal.end());
    // no need to call calculateSurfaceNormal() or ellipsoidNormal()
    // directly. these methods are called by calculateDefaultNormal()
    shapeModelFromPvlElevation.calculateDefaultNormal(); 
    qDebug() << "Has default normal?                  " << shapeModelFromPvlElevation.hasNormal();
    normal = shapeModelFromPvlElevation.normal();
    qDebug() << "Default normal:                      "
             << QVector<double>(normal.begin(), normal.end());

    QVector <double *> cornerNeighborPoints;
    double point[3];
    point[0] = 1.0;    point[1] = 0.0;    point[2] = 0.0;
    cornerNeighborPoints.push_back(point);
    shapeModelFromPvlElevation.calculateLocalNormal(cornerNeighborPoints); 
    qDebug() << "Has local normal?                    " << shapeModelFromPvlElevation.hasLocalNormal();
    normal = shapeModelFromPvlElevation.localNormal();
    qDebug() << "Local normal from neighbor points:   "
             << QVector<double>(normal.begin(), normal.end());
    qDebug() << "";

    qDebug() << "================================= Error Throws ==================================";
    qDebug() << "Construct NaifDskShape object from cube labels with ShapeModel=Null.";    
    try {
      Pvl pvlNullShape("./st_2530292409_v_NullShapeModel.lbl");
      NaifDskShape shapeModelFromPvlNull(&targetSh, pvlNullShape);
    } 
    catch (IException &e) {
      e.print();
    }
    qDebug() << "";
    qDebug() << "Thrown by setLocalNormalFromIntercept() - Failed to find intercept. ";
    try {
      shapeModelFromPvlShape.setLocalNormalFromIntercept();
    } 
    catch (IException &e) {
      e.print();
    }
    qDebug() << "";
    qDebug() << "Thrown by calculateLocalNormal() - Failed to find intercept for normal vector. ";
    try {
      shapeModel.calculateLocalNormal(cornerNeighborPoints);
    } 
    catch (IException &e) {
      e.print();
    }
    qDebug() << "";
    qDebug() << "Thrown by ellipsoidNormal() - No intersection. ";
    try {
      shapeModel.ellipsoidNormal();
    } 
    catch (IException &e) {
      e.print();
    }
    qDebug() << "";
    qDebug() << "Thrown by ellipsoidNormal() - Invalid intersection. ";
    try {
      // if the surface point is accidentally reset to an invalid point, but hasIntercept still 
      // set to true.
      shapeModelFromPvlElevation.setSurfacePoint(SurfacePoint());
      shapeModelFromPvlElevation.ellipsoidNormal();
    } 
    catch (IException &e) {
      e.print();
    }
    qDebug() << "";
    qDebug() << "Thrown by ellipsoidNormal() - Invalid target. ";
    try {
      // get valid intersection
      shapeModelFromPlate.intersectSurface(obsPos, lookDir);
      // model with invalid target
      shapeModelFromPlate.ellipsoidNormal();
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
