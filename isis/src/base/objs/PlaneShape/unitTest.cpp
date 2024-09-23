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
#include "PlaneShape.h"
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
 * Unit test for PlaneShape class
 *
 *
   * @author 2012-07-30 Ken Edmundson
 *
 * @internal
  *   @history 2015-04-30 Jeannie Backer - Added test for isDEM(). References #2243.
 *  
 *  
 *  
 *   testcoverage 2015-04-30 - 59.375% scope, 92.308% line, 100% function
 */
int main() {
  try {
    Preference::Preferences(true);
    //string inputFile = "$ISISTESTDATA/isis/src/mgs/unitTestData/ab102401.cub";
    QString inputFile = "$ISISTESTDATA/isis/src/base/unitTestData/PlaneShape/W1591510834_1_cal.cub";
    Cube cube;
    cube.open(inputFile);
    Camera *c = cube.camera();
    std::vector<Distance> radii = c->target()->radii();
    Pvl pvl = *cube.label();
    Spice spi(cube);
    Target targ(&spi, pvl);
    targ.setRadii(radii);

    cout << "Begin testing Plane Shape Model class...." << endl;

    cout << endl << "  Testing constructors..."  << endl;
    PlaneShape shape(&targ, pvl);
    PlaneShape shape2(&targ);
    PlaneShape shape3;

    cout << "    Shape1  name is " << shape.name().toStdString() << endl;
    cout << "    Shape2  name is " << shape2.name().toStdString() << endl;
    cout << "    Shape3  name is " << shape3.name().toStdString() << endl;
    cout << "    Shape is DEM type? " << Isis::toString((bool)shape3.isDEM()) << endl;

    std::vector<double> sB(3);
    sB[0] = -19584.5;
    sB[1] = 920594.0;
    sB[2] = 516257.0;
    std::vector<double> lookB(3, 1.);
    lookB[0] = -1.;

    cout << endl << "  Testing method intersectSurface with failure..." << endl; 
    cout << "    Do we have an intersection? " << shape.hasIntersection() << endl;
    shape.intersectSurface(sB, lookB);
    if (!shape.hasIntersection())
      cout << "    Intersection failed " << endl;

    cout << endl << "Testing method intersectSurface..." << endl; 
    cout << "    Do we have an intersection? " << shape.hasIntersection() << endl;
    cout << "   Set a pixel in the image and check again." << endl;
    double line = 272.516;
    double sample = 189.935;
    c->SetImage(sample, line);
    c->instrumentPosition((double *) &sB[0]);
    std::vector<double> uB(3);
    c->sunPosition((double *) &uB[0]);
    c->SpacecraftSurfaceVector((double *) &lookB[0]);

  // Sample/Line = 189.935/272.516
  // surface normal = 0.0, 0.0, 1.0
  // Local normal = 0.0, 0.0, 1.0
  //  Phase = 34.4111
  //  Incidence = 83.3692
  //  Emission = 62.2289

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

    shape.intersectSurface(sB, lookB);

    cout << endl << "  Testing class method calculateLocalNormal..." << endl;
    QVector<double *>  notUsed(4);

    for (int i = 0; i < notUsed.size(); i ++)
        notUsed[i] = new double[3];

    shape.calculateLocalNormal(notUsed);
    vector<double> myNormal(3);
    myNormal = shape.normal();
    cout << "    local normal = (" << myNormal[0] << ", " << myNormal[1] << ", " << myNormal[2] << ")" << endl;

    cout << endl << "  Testing class method calculateSurfaceNormal..." << endl;
    shape.calculateSurfaceNormal(); 
    myNormal = shape.normal();
    cout << "    surface normal = (" << myNormal[0] << ", " << myNormal[1] << ", " << myNormal[2] << ")" << endl;

    cout << endl << "  Testing class method calculateDefaultNormal..." << endl;
    shape.calculateDefaultNormal();
    myNormal = shape.normal();
    cout << "    default normal = (" << myNormal[0] << ", " << myNormal[1] << ", " << myNormal[2] << ")" << endl;

    cout << endl << "  Testing localRadius method ..." << endl;
    double radius = shape.localRadius(Latitude(0.0, Angle::Degrees),
                                      Longitude(336.824286272771076, Angle::Degrees)).kilometers();
    printf("   Localradius = %lf km\n", radius);
    // radius at point on ring plane = 127509023.718129977583885

    cout << endl << "  Testing setHasIntersection method..." << endl;
    shape.setHasIntersection(false);
    cout << "    Do we have an intersection? " << shape.hasIntersection() << endl;

    cout << endl << "  Testing setSurfacePoint method..." << endl;
    shape.setSurfacePoint(*sp);
    cout << "     Do we have an intersection? " << shape.hasIntersection() << endl;
    cout << "     surface point = (" << sp->GetX().kilometers() << ", " << 
      sp->GetY().kilometers() << ", " << sp->GetZ().kilometers() << ")" << endl;

    cout << endl << "  Testing incidence angle method..." << endl;
    cout << "    incidence angle: " << shape.incidenceAngle(uB) << endl;

    cout << endl << "  Testing emission angle method..." << endl;
    cout << "     emission angle: " << shape.emissionAngle(sB) << endl;

    cout << endl << "  Testing phase angle method..." << endl;
    cout << "        phase angle: " << shape.phaseAngle(sB,uB) << endl;

    cube.close();
  } 
  catch (IException &e) {
    cout << endl << endl;
    std::string msg = "**************** UNIT TEST FAILED! **************** ";
    IException(e, IException::Unknown, msg, _FILEINFO_).print();
  }
}
