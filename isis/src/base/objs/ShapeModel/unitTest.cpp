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
 * This application tests the ShapeModel class
 *
 * @author 2010-10-11 Debbie A. Cook
 *
 * @internal
 *   @history 2015-05-04 Jeannie Backer - Added test for isDEM() and improved
 *                           overall test coverage.
 *   @history 2016-08-28 Kelvin Rodriguez - Changed print statements to properly flush
 *                           std out before exceptions to better suit differences in OSX 10.11
 *                           part of porting to 10.11.
 *
 *   testcoverage 2015-04-30 - 78.947% scope, 91.057% line, 96.154% function
 *   testcoverage 2015-05-04 - 94.737% scope, 100% line, 100% function
 */
class MyShape : public ShapeModel {
  public:
  MyShape(Target *target) : ShapeModel (target) {
    setName("Test");
  }

  using Isis::ShapeModel::intersectSurface;

  bool intersectSurface(std::vector<double> observerPos,
                        std::vector<double> lookDirection)  {
    cout << "    intersectSurface called with observer position = " <<
      observerPos[0] << ", " << observerPos[1] << ", " <<
      observerPos[2] << endl << "                                 lookDirection = " <<
      lookDirection[0] << ", " <<  lookDirection[1] << ", " << lookDirection[2] << endl;
    intersectEllipsoid(observerPos, lookDirection);
    SpiceDouble intersectionPoint[3] = {-2123.362258286, -2380.3717812236, 1194.6783966636};

    surfaceIntersection()->FromNaifArray(intersectionPoint);
    setHasIntersection(true);
    return true;
  }

  bool isDEM() const {
    return false;
  }

  bool ellipsoidIntersection() {
    return hasEllipsoidIntersection();
  }

  virtual void calculateDefaultNormal()  {
    calculateSurfaceNormal();
  }

  virtual void calculateLocalNormal(QVector<double *> cornerNeighborPoints) {
    std::vector<double> myNormal(3);
    myNormal[0] = -0.581842;
    myNormal[1] = -0.703663;
    myNormal[2] =  0.407823;
    setNormal(myNormal);
    setHasNormal(true);
  }

  virtual void calculateSurfaceNormal() {
    setNormal( -0.623384,
               -0.698838,
                0.350738);
    setHasNormal(true);
  }

  virtual void calculateEllipsoidNormal() {
    calculateEllipsoidalSurfaceNormal();
  }

  Distance localRadius(const Latitude &lat, const Longitude &lon) {
    double a = 6378.14;
    double b = 6378.14;
    double c = 6356.75;

    double rlat = lat.degrees();
    double rlon = lon.degrees();

    double xyradius = a * b / sqrt(pow(b * cos(rlon), 2) +
                      pow(a * sin(rlon), 2));

    const double &radius = xyradius * c / sqrt(pow(c * cos(rlat), 2) +
                           pow(xyradius * sin(rlat), 2));

    return Distance(radius, Distance::Kilometers);
  }

  bool normalStatus() {
    return hasNormal();
  }

  void setSmallNormal() {
    std::vector<double> normal(3, 10.);
    setNormal(normal);
    setHasNormal(true);
  }

  void setBigNormal() {
    std::vector<double> normal(3, -10.);
    setNormal(normal);
    setHasNormal(true);
  }

  double resolution() {
    return ShapeModel::resolution();
  }

  void setNoNormal() {
    setHasNormal(false);
  }
};



class MyEllipse : public ShapeModel {
  public:
  MyEllipse(Target *target) : ShapeModel (target) {
    setName("Ellipsoid");
  }

  MyEllipse() : ShapeModel() {
    setName("DefaultConstructor");
  }

  bool intersectSurface(std::vector<double> observerPos,
                                    std::vector<double> lookDirection)  {
    cout << "    intersectSurface called with observer position = " <<
      observerPos[0] << ", " << observerPos[1] << ", " <<
      observerPos[2] << endl << "                                 lookDirection = " <<
      lookDirection[0] << ", " <<  lookDirection[1] << ", " << lookDirection[2] << endl;
    return (intersectEllipsoid(observerPos, lookDirection));
  }

  bool isDEM() const {
    return false;
  }

  virtual void calculateLocalNormal(QVector<double *> cornerNeighborPoints) {

    std::vector<Distance> radii = targetRadii();
    std::vector<double> normal(3, 0.);
    SpiceDouble point[3];
    surfaceIntersection()->ToNaifArray(point);
    surfnm_c(radii[0].kilometers(), radii[1].kilometers(), radii[2].kilometers(), point, (SpiceDouble *) &normal[0]);
    setNormal(normal);
    setHasNormal(true);

  }

  virtual void calculateSurfaceNormal() {
    std::vector<Distance> radii = targetRadii();
    std::vector<double> normal(3, 0.);
    SpiceDouble point[3];
    surfaceIntersection()->ToNaifArray(point);
    surfnm_c(radii[0].kilometers(), radii[1].kilometers(), radii[2].kilometers(), point, (SpiceDouble *) &normal[0]);
    setNormal(normal);
    setHasNormal(true);
  }

  virtual void calculateDefaultNormal() {
    setNormal(1, 0, 0);
    setHasNormal(true);
  }

  Distance localRadius(const Latitude &lat, const Longitude &lon) {
    std::vector<Distance> radii = targetRadii();
    double a = radii[0].kilometers();
    double b = radii[1].kilometers();
    double c = radii[2].kilometers();

    double rlat = lat.degrees();
    double rlon = lon.degrees();

    double xyradius = a * b / sqrt(pow(b * cos(rlon), 2) +
                      pow(a * sin(rlon), 2));

    const double &radius = xyradius * c / sqrt(pow(c * cos(rlat), 2) +
                           pow(xyradius * sin(rlat), 2));

    return Distance(radius, Distance::Kilometers);
  }

  bool normalStatus() {
    return hasNormal();
  }

  double resolution() {
    return ShapeModel::resolution();
  }
};



int main() {
  try {
    Preference::Preferences(true);
    QString inputFile = "$ISISTESTDATA/isis/src/mgs/unitTestData/ab102401.cub";
    Cube cube;
    cube.open(inputFile);
    Camera *c = cube.camera();
    std::vector<Distance> radii = c->target()->radii();
    Pvl pvl = *cube.label();
    Spice spi(cube);
    Target targ(&spi, pvl);
    targ.setRadii(radii);

    cout << "Begin testing Shape Model base class...." << endl;

    MyShape shape(&targ);

    cout << endl << "  Shape name is " << shape.name() << endl;
    cout << "    Do we have an intersection? " << shape.hasIntersection() << endl;
    cout << "    Do we have an ellipsoid intersection? " << shape.ellipsoidIntersection() << endl;
    try {
      shape.resolution();
    }
    catch (IException &e) {
      cout << "    Test resolution() error message when there is no intersection:" << endl;
      e.print();
    }
    try {
      shape.calculateDefaultNormal();
    }
    catch (IException &e) {
      cout << "    Test setNormal(double,double,double) error message "
              "when there is no intersection:" << endl;
      e.print();
    }
    QVector<double *>  notUsed(4);
    for (int i = 0; i < notUsed.size(); i ++) notUsed[i] = new double[3];
    try {
      shape.calculateLocalNormal(notUsed);
    }
    catch (IException &e) {
      cout << "    Test setNormal(vector) error message when there is no intersection:" << endl;
      e.print();
    }
    cout << "    Set a pixel in the image and check again." << endl;
    double line = 453.0;
    double sample = 534.0;
    c->SetImage(sample, line);
    std::vector<double> sB(3);
    c->instrumentPosition((double *) &sB[0]);
    std::vector<double> uB(3);
    c->sunPosition((double *) &uB[0]);
    std::vector<double> lookB(3);
    c->SpacecraftSurfaceVector((double *) &lookB[0]);
    /*
    Sample/Line = 534/453
    surface normal = -0.623384, -0.698838, 0.350738
    Local normal = -0.581842, -0.703663, 0.407823
      Phase                      = 40.787328112158
      Incidence                  = 85.341094499768
      Emission                   = 46.966269013795
    */
    cout << endl << "    Testing pure virtual method intersectSurface..." << endl;
    if (!shape.intersectSurface(sB, lookB)) {
      cout << "    ...  intersectSurface method failed" << endl;
      return -1;
    }
    cout << "    Do we have an intersection? " << shape.hasIntersection() << endl;
    cout << "    Do we have an ellipsoid intersection? " << shape.ellipsoidIntersection() << endl;
    try {
      cout << "    Get the resolution:         ";
      cout << shape.resolution() << endl;
    }
    catch (IException &e) {
      cout << "    Test resolution() error message when there is no intersection:" << endl;
      e.print();
    }
    SurfacePoint *sp = shape.surfaceIntersection();
    cout << "    surface point = (" << sp->GetX().kilometers() << ", " <<
      sp->GetY().kilometers() << ", " << sp->GetZ().kilometers() << ")" << endl;

    try {
      cout << endl << "  Testing class method normal() when no normal exists..." << endl;
      cout << "    Do we have a normal? " << shape.normalStatus() << endl;
      std::vector<double> badnormal = shape.normal();
    }
    catch(Isis::IException &e) {
      e.print();
    }

    cout << endl << "  Testing photometric angle calculations before normal computation..." << endl;
    cout << "    Do we have a normal? " << shape.normalStatus() << endl;
    double emission  = shape.emissionAngle(sB);
    double incidence = shape.incidenceAngle(uB);
    cout << "    Emission angle = " << emission;
    cout << "    Incidence angle = " << incidence;
    cout << endl;

    cout << endl << "  Testing class method calculateLocalNormal..." << endl;
    shape.calculateLocalNormal(notUsed);
    cout << "    Do we have a normal? " << shape.normalStatus() << endl;
    vector<double> myNormal(3);
    myNormal = shape.normal();
    cout << "    local normal = (" << myNormal[0] << ", " << myNormal[1] << ", " << myNormal[2] << ")" << endl;

    cout << endl << "  Testing class method calculateSurfaceNormal..." << endl;
    shape.calculateSurfaceNormal();
    myNormal = shape.normal();
    cout << "    surface normal = (" << myNormal[0] << ", " << myNormal[1] << ", " << myNormal[2] << ")" << endl;

    cout << endl << "  Testing photometric angle calculations with undersize normal..." << endl;
    shape.setSmallNormal();
    emission  = shape.emissionAngle(sB);
    incidence = shape.incidenceAngle(uB);
    cout << "    Emission angle = " << emission;
    cout << "    Incidence angle = " << incidence;
    cout << endl;

    cout << endl << "  Testing photometric angle calculations with oversize normal..." << endl;
    shape.setBigNormal();
    emission  = shape.emissionAngle(sB);
    incidence = shape.incidenceAngle(uB);
    cout << "    Emission angle = " << emission;
    cout << "    Incidence angle = " << incidence;
    cout << endl;

    cout << "  Testing class method calculateDefaultNormal..." << endl;
    shape.calculateDefaultNormal();
    myNormal = shape.normal();
    cout << "    default normal = (" << myNormal[0] << ", " << myNormal[1] << ", " << myNormal[2] << ")" << endl;

    cout << endl << "  Testing photometric angle calculations..." << endl;
    // reset has normal to false to test (!hasNormal) scope for incidenceAngle()
    shape.setNoNormal();
    incidence = shape.incidenceAngle(uB);
    emission  = shape.emissionAngle(sB);
    cout << "    Emission angle = " << emission;
    cout << "    Incidence angle = " << incidence;
    cout << "    Phase angle = "  << shape.phaseAngle(sB, uB);
    cout << endl;

    cout << endl << "  Testing localRadius method ..." << endl;
    cout  << "    Local radius = " << shape.localRadius(Latitude(20.532461495381, Angle::Degrees),
                                                        Longitude(228.26609149754, Angle::Degrees)).kilometers() << endl;
    // Mars radii = 3397.      3397.         3375.

    cout << endl << "  Testing intersection with occlusion check..." << endl;
    if (!shape.intersectSurface(Latitude(20.532461495381, Angle::Degrees),
                                Longitude(228.26609149754, Angle::Degrees),
                                sB, true)) {
      cout << "    ...  intersectSurface method failed" << endl;
      return -1;
    }
    cout << "    Do we have an intersection? " << shape.hasIntersection() << endl;
    cout << "    Do we have an ellipsoid intersection? " << shape.ellipsoidIntersection() << endl;
    cout << "    Is the intersection visible? " << shape.isVisibleFrom(sB, lookB) << endl;
    SurfacePoint *occPoint = shape.surfaceIntersection();
    std::vector<double> occPosition(3, 0.0);
    occPosition[0] = occPoint->GetX().kilometers() * 1.1;
    occPosition[1] = occPoint->GetY().kilometers() * 1.1;
    occPosition[2] = occPoint->GetZ().kilometers() * 1.1;
    cout << "    Is the intersection visible from just above it? "
         << shape.isVisibleFrom(occPosition, lookB) << endl;
    
    cout << "    Calculate the ellipsoid normal" << endl;
    shape.calculateEllipsoidNormal();
    cout << "      Do we have a normal? " << shape.normalStatus() << endl;
    myNormal = shape.normal();
    cout << "      local normal = (" << myNormal[0] << ", " << myNormal[1] << ", " << myNormal[2] << ")" << endl;

    cout << endl << "  Testing setHasIntersection method" << endl;
    shape.setHasIntersection(false);
    cout << "    Do we have an intersection? " << shape.hasIntersection() << endl;
    try {
      cout << "    Get the resolution:         ";
      cout << shape.resolution() << endl;
    }
    catch (IException &e) {
      cout << "    Test resolution() error message when there is no intersection:" << endl;
      e.print();
    }
    try {
      cout << "    Attempt to calculate the ellipsoid normal without an intersection" << endl;
      shape.calculateEllipsoidNormal();
      cout << "    Calculation successful" << endl;
    }
    catch (IException &e) {
      e.print();
    }

    cout << endl << "  Testing setSurfacePoint method ..." << endl;
    shape.setSurfacePoint(*sp);
    cout << "     Do we have an intersection? " << shape.hasIntersection() << endl;
    try {
      cout << "    Get the resolution:         ";
      cout << shape.resolution() << endl;
    }
    catch (IException &e) {
      cout << "    Test resolution() error message when there is no intersection:" << endl;
      e.print();
    }
    cout << "     surface point = (" << sp->GetX().kilometers() << ", " <<
      sp->GetY().kilometers() << ", " << sp->GetZ().kilometers() << ")" << endl;

    // Test ellipse methods in base class
    MyEllipse eshape(&targ);
    try {
      cout << endl << "  Testing ellipsoid methods in base class" << endl;
      cout << "    Do we have an intersection? ";
      cout << eshape.hasIntersection() << endl;
      try {
        cout << "    Get the resolution:         ";
        cout << eshape.resolution() << endl;
      }
      catch (IException &e) {
        cout << "    Test resolution() error message when there is no intersection:" << endl;
        e.print();
      }
      cout << endl << "    Testing  failing of method intersectEllipsoid..." << endl;
      std::vector<double> badlook(3,1.);
      badlook[0] = -1.;

      if (!eshape.intersectSurface(sB, badlook)) {
        cout << "    ...  intersectSurface method failed -- no intersection" << endl;
      }

      cout << "    Do we have an intersection? " << eshape.hasIntersection() << endl;
      try {
        cout << "    Get the resolution:         ";
        cout << eshape.resolution() << endl;
      }
      catch (IException &e) {
        cout << "    Test resolution() error message when there is no intersection:" << endl;
        e.print();
      }
      cout << endl << "    Testing  method intersectEllipsoid..." << endl;

      if (eshape.intersectSurface(sB, lookB)) {
        SurfacePoint *sp = eshape.surfaceIntersection();
        cout << "    surface point = (" << sp->GetX().kilometers() << ", " <<
          sp->GetY().kilometers() << ", " << sp->GetZ().kilometers() << ")" << endl;
      }
      cout << "    Do we have an intersection? " << eshape.hasIntersection() << endl;
      cout << "    Get the resolution:         " << eshape.resolution() << endl;
      SurfacePoint *sp = eshape.surfaceIntersection();
      cout << "    surface point = (" << sp->GetX().kilometers() << ", " <<
        sp->GetY().kilometers() << ", " << sp->GetZ().kilometers() << ")" << endl;

      try {
        cout << endl << "    Testing  method calculateEllipsoidalSurfaceNormal with invalid intersection..." << endl;
        SurfacePoint badsp;
        eshape.setSurfacePoint(badsp);
        eshape.setHasIntersection(true);
        eshape.calculateLocalNormal(notUsed);
      }
      catch(Isis::IException &e) {
        e.print();
      }

      cout << endl << "    Testing  method setHasIntersection false..." << endl;
      eshape.setHasIntersection(false);
      cout << "    Do we have an intersection? " << eshape.hasIntersection() << endl;
      try {
        cout << "    Get the resolution:         ";
        cout << eshape.resolution() << endl;
      }
      catch (IException &e) {
        cout << "    Test resolution() error message when there is no intersection:" << endl;
        e.print();
      }
      try {
        cout << endl << "    Testing  method calculateEllipsoidalSurfaceNormal with no intersection..." << endl;
        eshape.calculateLocalNormal(notUsed);
      }
      catch(Isis::IException &e) {
        e.print();
      }
      cout << endl << "    Testing  method calculateEllipsoidalSurfaceNormal with valid intersection..." << endl;
      if (eshape.intersectSurface(sB, lookB)) cout << "    Intersection set" << endl;
      cout << "      Do we have a normal? " << eshape.normalStatus() << endl;
      eshape.calculateLocalNormal(notUsed);
      cout << "      Do we have a normal? " << eshape.normalStatus() << endl;
      myNormal = eshape.normal();
      cout << "      local normal = (" << myNormal[0] << ", " << myNormal[1] << ", " << myNormal[2] << ")" << endl;
      eshape.calculateSurfaceNormal();
      myNormal = eshape.normal();
      cout << endl << "    Testing  method targetRadii..." << endl;
      cout << "      true normal = (" << myNormal[0] << ", " << myNormal[1] << ", " << myNormal[2] << ")" << endl;


    }
    catch(Isis::IException &e) {
      IException(e, IException::Unknown, "Test ellipse methods failed.", _FILEINFO_).print();
    }

    // Test
    MyEllipse defaultShape;
    cout << endl << "  Testing default constructor..." << endl;
    cout << "    Shape is " << defaultShape.name() << endl;
    cout << "    Do we have an intersection? " << defaultShape.hasIntersection() << endl;
    cout << "    Is there a normal? " << defaultShape.normalStatus() << endl;
    try {
      defaultShape.resolution();
    }
    catch (IException &e) {
      cout << "    Test resolution() error message when there is no target:" << endl;
      e.print();
    }
    try {
      defaultShape.calculateSurfaceNormal();
    }
    catch (IException &e) {
      cout << "    Test targetRadii() error message when there is no target:" << endl;
      e.print();
    }
    defaultShape.setHasIntersection(true);
    defaultShape.calculateDefaultNormal();
    cout << "    Is there a normal? " << defaultShape.normalStatus() << endl;
    cout << "    Number of normal components = " << defaultShape.normal().size() << endl;

    cube.close();
  }
  catch (IException &e) {
    cout << endl << endl;
    std::string msg = "**************** UNIT TEST FAILED! **************** ";
    IException(e, IException::Unknown, msg, _FILEINFO_).print();
  }
}
