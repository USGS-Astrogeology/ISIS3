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
#include "EllipsoidShape.h"
#include "IException.h"
#include "Latitude.h"
#include "Longitude.h"
#include "Preference.h"
#include "Pvl.h"
#include "EquatorialCylindricalShape.h"
#include "SurfacePoint.h"
#include "Target.h"

using namespace std;
using namespace Isis;

/**
  * Unit test for EquatorialCylindricalShape class
  *
  * @internal
  *   @history 2015-04-30 Jeannie Backer - Added test for calculateLocalNormal() with
  *                           no intersection and test for isDEM(). References #2243.
  *  
  *   testcoverage 2015-04-30 - 10.127% scope, 9.040% line, 100% function
  *  
  *   @todo Construct EquatorialCylindricalShape from cube without
  *         ShapeModelStatistics table to test error throw.
  *   @todo Call intersectSurface() where DEM shape intersection fails and 
  *         ellipse intersection fails.
  */
int main() {
  try {
    Preference::Preferences(true);
    QString inputFile = "$ISISTESTDATA/isis/src/mgs/unitTestData/ab102401.cub";
    // string inputFile = "/work/projects/isis/latest/m00775/test/M123149061RE.lev1.cub";
    Cube cube;
    cube.open(inputFile);
    Camera *c = cube.camera();
    std::vector<Distance> radii = c->target()->radii();
    Pvl &pvl = *cube.label();
    Spice spi(cube);
    Target targ(&spi, pvl);
    targ.setRadii(radii);

    cout << "Begin testing Dem Shape Model class...." << endl;

    EquatorialCylindricalShape shape(&targ, pvl);

    cout << "Shape name is " << shape.name().toStdString() << endl;
    cout << "Shape is DEM type? " << Isis::toString((bool)shape.isDEM()) << endl;

    cout << endl << "Testing method intersectSurface..." << endl; 
    cout << "  Do we have an intersection? " << shape.hasIntersection() << endl;
    cout << endl << " Set a pixel in the image and check again." << endl;
    double line = 453.0;
    double sample = 534.0;
    // The next point goes with the LRO image /work/projects/isis/latest/m00775/test/M123149061RE.lev1.cub
    // double line = 1255.62;
    // double sample = 26112.5;
    c->SetImage(sample, line);
    std::vector<double> sB(3);
    c->instrumentPosition((double *) &sB[0]);
    std::vector<double> uB(3);
    c->sunPosition((double *) &uB[0]);
    std::vector<double> lookB(3);
    c->SpacecraftSurfaceVector((double *) &lookB[0]);
    /*
    Find a point that fails in the DemShape intersect method and use instead. for a better test
    Sample/Line = 534/453
    surface normal = -0.623384, -0.698838, 0.350738
    Local normal = -0.581842, -0.703663, 0.407823
      Phase                      = 40.787328112158
      Incidence                  = 85.341094499768
      Emission                   = 46.966269013795
    */
    if (!shape.intersectSurface(sB, lookB)) { 
        cout << "...  intersectSurface method failed" << endl;
        return -1;
    }
    cout << "  Do we have an intersection? " << shape.hasIntersection() << endl;
    SurfacePoint *sp = shape.surfaceIntersection();
    cout << "   surface point = (" << sp->GetX().kilometers() << ", " << 
      sp->GetY().kilometers() << ", " << sp->GetZ().kilometers() << ")" << endl;

     /*
    Set the look vector straight up to cause it to fail in the EllipsoidalShape intersection
    This tests a bug introduced when the ShapeModel classes were added that caused
    qview and spiceinit to hang if the look vector pointed off the target body.  DAC
    */
    cout << endl << "Testing a condition that previously caused qview and spiceinit to hang instead of failing." << endl;
    lookB[0] = 1.;
    lookB[1] = -0.9;
    lookB[2] = -0.01;
    if (!shape.intersectSurface(sB, lookB)) { 
        cout << "...  intersectSurface method failed" << endl;
    }

    // TODO 
    // Test alternate constructors DemShape() and DemShape(target)

  }
  catch (IException &e) {
    cout << endl << endl;
    std::string msg = "**************** UNIT TEST FAILED! **************** ";
    IException(e, IException::Unknown, msg, _FILEINFO_).print();
  }
}
