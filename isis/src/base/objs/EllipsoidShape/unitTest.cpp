/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <iomanip>

#include <QVector>

#include "Angle.h"
#include "Camera.h"
#include "CameraFactory.h"
#include "Cube.h"
#include "Distance.h"
#include "EllipsoidShape.h"
#include "IException.h"
#include "Latitude.h"
#include "Longitude.h"
#include "Preference.h"
#include "Pvl.h"
#include "ShapeModel.h"
#include "SurfacePoint.h"
#include "Target.h"

using namespace std;
using namespace Isis;


/**
  * Unit test for EllipsoidShape class
  *
  * @internal
  *   @history 2015-04-30 Jeannie Backer - Added test for calculateLocalNormal() with
  *                           no intersection and test for isDEM(). References #2243.
  *
  *   testcoverage 2015-04-30 - 92.857% scope, 100% line, 100% function
  */
int main() {
  try {

    Preference::Preferences(true);
    QString inputFile = "$ISISTESTDATA/isis/src/mgs/unitTestData/ab102401.cub";
    Cube cube;
    cube.open(inputFile);
    Camera *c = cube.camera();
    vector<Distance> radii = c->target()->radii();
    cout <<  setprecision(10)<< "Radii=["<<radii[0].kilometers() << ","
          <<radii[1].kilometers() << "," << radii[2].kilometers() << "]" << endl;
    Pvl &pvl = *cube.label();
    Spice spi(cube);
    Target targ(&spi, pvl);
    targ.setRadii(radii);
    
    cout << "Begin testing Ellipsoid Shape Model class...." << endl;

    cout << endl << "  Testing constructors..."  << endl;
    EllipsoidShape shape(&targ);
    EllipsoidShape shape2;

    cout << "    Shape  name is " << shape.name().toStdString() << endl;
    cout << "    Shape2  name is " << shape2.name().toStdString() << endl;
    cout << "    Shape is DEM type?" << toString(shape.isDEM()) << endl;

    std::vector<double> sB(3);
    sB[0] = -2399.54;
    sB[1] = -2374.03;
    sB[2] = 1277.68;
    std::vector<double> lookB(3, 1.);
    lookB[0] = -1.;
    cout << endl << "  Testing method intersectSurface with failure..." << endl;
    cout << "    Do we have an intersection? " << shape.hasIntersection() << endl;
    shape.intersectSurface(sB, lookB);
    if (!shape.hasIntersection()) cout << "    Intersection failed " << endl;
    cout << endl << "  Testing method calculateLocalNormal with intersection failure..." << endl;
    try {
      QVector<double *> emptyVector;
      shape.calculateLocalNormal(emptyVector);
    }
    catch (IException &e) {
      e.print();
    }


    cout << endl << "Testing method intersectSurface..." << endl;
    cout << "    Do we have an intersection? " << shape.hasIntersection() << endl;
    cout << "   Set a pixel in the image and check again." << endl;
    double line = 453.0;
    double sample = 534.0;
    c->SetImage(sample, line);
    c->instrumentPosition((double *) &sB[0]);
    std::vector<double> uB(3);
    c->sunPosition((double *) &uB[0]);
    c->SpacecraftSurfaceVector((double *) &lookB[0]);

    if (!shape.intersectSurface(sB, lookB)) {
        cout << "    ...  intersectSurface method failed" << endl;
        return -1;
    }
    cout << "    Do we have an intersection? " << shape.hasIntersection() << endl;
    SurfacePoint *sp = shape.surfaceIntersection();
    cout << "     surface point = (" << sp->GetX().kilometers() << ", " <<
      sp->GetY().kilometers() << ", " << sp->GetZ().kilometers() << ")" << endl;

    cout << endl << " Testing intersectSurface using surfacepoint from parent class..." << endl; 
    shape.intersectSurface(*sp, sB);
    cout << "    Do we have an intersection? " << shape.hasIntersection() << endl;

    cout << endl << " Testing intersectSurface using lat/lon from parent class..." << endl; 
    shape.intersectSurface(sp->GetLatitude(), sp->GetLongitude(), sB);
    cout << "    Do we have an intersection? " << shape.hasIntersection() << endl;

    cout << endl << "  Testing class method calculateLocalNormal..." << endl;
    QVector<double *>  notUsed(4);

    for (int i = 0; i < notUsed.size(); i ++)
        notUsed[i] = new double[3];

    shape.calculateLocalNormal(notUsed);
    vector<double> myNormal(3);
    myNormal = shape.localNormal();

    //Hand-calculated truth value:  [-0.6196003462957385, -0.7004971412244801, 0.3541174466282787]


    cout << "    local normal = (" << myNormal[0] << ", " << myNormal[1] << ", " << myNormal[2] << endl;

    cout << endl << "  Testing class method calculateSurfaceNormal..." << endl;
    shape.calculateSurfaceNormal();
    myNormal = shape.normal();



    cout << "    surface normal = (" << myNormal[0] << ", " << myNormal[1] << ", " << myNormal[2] << endl;

    cout << endl << "  Testing class method calculateDefaultNormal..." << endl;
    shape.calculateDefaultNormal();
    myNormal = shape.normal();
    cout << "    default normal = (" << myNormal[0] << ", " << myNormal[1] << ", " << myNormal[2] << endl;

    cout << endl << "  Testing localRadius method ..." << endl;
    cout  << "    Local radius = " << shape.localRadius(Latitude(20.532461495381, Angle::Degrees),
                                                    Longitude(228.26609149754, Angle::Degrees)).kilometers() << endl;


    cout << endl << "  Testing setHasIntersection method" << endl;
    shape.setHasIntersection(false);
    cout << "    Do we have an intersection? " << shape.hasIntersection() << endl;

    cout << endl << "  Testing setSurfacePoint method ..." << endl;
    shape.setSurfacePoint(*sp);
    cout << "     Do we have an intersection? " << shape.hasIntersection() << endl;
    cout << "     surface point = (" << sp->GetX().kilometers() << ", " <<
      sp->GetY().kilometers() << ", " << sp->GetZ().kilometers() << endl;

    cube.close();
  }
  catch (IException &e) {
    cout << endl << endl;
    std::string msg = "**************** UNIT TEST FAILED! **************** ";
    IException(e, IException::Unknown, msg, _FILEINFO_).print();
  }
}
