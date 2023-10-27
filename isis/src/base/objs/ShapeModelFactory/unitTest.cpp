/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Distance.h"
#include "FileName.h"
#include "IException.h"
#include "ShapeModelFactory.h"
#include "ShapeModel.h"
#include "Camera.h"
#include "Preference.h"
#include "CameraFactory.h"
#include "Target.h"
#include "QRegularExpression"

  /**
   * This application tests the ShapeModelFactory class.
   *
   * @author 2010-10-11 Debbie A. Cook
   *
   * @internal
   *   @history
   */

using namespace std;
using namespace Isis;

/**
 *
 * @internal
 *   @history 2015-02-25 Jeannie Backer - Added test for Null ElevationModel. Added test for DSK Shape Model.
 *                           Code coverage: 81.818% scope, 84.058% line and 100% function.
 *
 *   @todo code coverage - need RingPlane shape that passes
 *   @todo code coverage - need RingPlane shape that throws error on construction
 *   @todo code coverage - need Null shape that throws error on EllipsoidShape construction
 *   @todo code coverage - need DEM file (not Equatorial Cylindrical) that throws error on
 *                           construction
 *   @todo code coverage - need constructor (EllipsoidShape, PlaneShape, EquatorialCylindricalShape,
 *                           or DemShape to return null shape.
 */

 void ReportError(QString err) {
   cout << err.replace(QRegularExpression("(\\/[\\w\\-\\. ]*)+\\/mgs"), "mgs") << endl;
 }

int main() {
  try {
    Isis::Preference::Preferences(true);

    cout << "Unit test for Isis::ShapeModel" << endl;

    // Test sky target
    // Build label for sky target test
    PvlGroup inst1("Instrument");
    inst1 += PvlKeyword("TargetName", "Sky");
    PvlGroup inst2("Instrument");
    inst2 += PvlKeyword("TargetName", "Mars");
    PvlGroup kern1("Kernels");
    FileName f("$ISISTESTDATA/isis/src/base/unitTestData/kernels");
    FileName f2("$base/dems");
    FileName f3("$ISISTESTDATA/isis/src/mgs/unitTestData");
    QString dir = f.expanded() + "/";
    QString dir2 = f2.expanded() + "/";
    QString dir3 = f3.expanded() + "/";
    kern1 += PvlKeyword("NaifFrameCode", std::to_string(-94031));
    kern1 += PvlKeyword("LeapSecond", dir.toStdString() + "naif0007.tls");
    kern1 += PvlKeyword("SpacecraftClock", dir.toStdString() + "MGS_SCLKSCET.00045.tsc");
    kern1 += PvlKeyword("TargetPosition", dir.toStdString() + "de405.bsp");
    kern1 += PvlKeyword("TargetAttitudeShape", dir.toStdString() + "pck00006.tpc");
    kern1 += PvlKeyword("Instrument", dir.toStdString() + "mocSpiceUnitTest.ti");
    kern1 += PvlKeyword("InstrumentAddendum", dir.toStdString() + "mocAddendum.ti");
    kern1 += PvlKeyword("InstrumentPosition", dir.toStdString() + "moc.bsp");
    kern1 += PvlKeyword("InstrumentPointing", dir.toStdString() + "moc.bc");
    kern1 += PvlKeyword("Frame", "");
    kern1 += PvlKeyword("NaifBodyCode", std::to_string(499));
    // Time Setup
    double startTime = -69382819.0;
    double endTime = -69382512.0;
    double slope = (endTime - startTime) / (10 - 1);

    kern1 += PvlKeyword("StartPadding", std::to_string(slope));
    kern1 += PvlKeyword("EndPadding", std::to_string(slope));

    Pvl lab1;
    lab1.addGroup(inst1);
    lab1.addGroup(kern1);

    // Test ShapeModel keyword
    cout << endl << "  Testing ShapeModel keyword (EquatorialCylindrical DEM)..." << endl;
    PvlGroup kern2 = kern1;
    kern2 += PvlKeyword("ShapeModel", dir2.toStdString() + "molaMarsPlanetaryRadius0005.cub");
    Pvl lab2;
    lab2.addGroup(inst2);
    lab2.addGroup(kern2);
    Target targSh(NULL, lab2);
    ShapeModel *smSh = ShapeModelFactory::create(&targSh, lab2);
    cout << "    Successfully created shape " << smSh->name() << endl;
    delete smSh;

    // Test ElevationModel keyword with value
    cout << endl << "  Testing ElevationModel keyword (EquatorialCylindrical DEM)..." << endl;
    PvlGroup kern3 = kern1;
    kern3 += PvlKeyword("ElevationModel", dir2.toStdString() + "molaMarsPlanetaryRadius0005.cub");
    Pvl lab3;
    lab3.addGroup(inst2);
    lab3.addGroup(kern3);
    Target targEl(NULL, lab3);
    ShapeModel *smEl = ShapeModelFactory::create(&targEl, lab3);
    cout << "    Successfully created shape " << smEl->name() << endl;
    delete smEl;

    // Test ShapeModel keyword with Null value
    cout << endl << "  Testing ShapeModel keyword (Null)..." << endl;
    PvlGroup kern4 = kern1;
    kern4 += PvlKeyword("ShapeModel", "Null");
    Pvl lab4;
    lab4.addGroup(inst2);
    lab4.addGroup(kern4);
    Target targShNull(NULL, lab4);
    ShapeModel *smShNull = ShapeModelFactory::create(&targShNull, lab4);
    cout << "    Successfully created shape " << smShNull->name() << endl;
    delete smShNull;

    // Test ElevationModel keyword with Null value
    cout << endl << "  Testing ElevationModel keyword (Null)..." << endl;
    PvlGroup kern5 = kern1;
    kern5 += PvlKeyword("ElevationModel", "Null");
    Pvl lab5;
    lab5.addGroup(inst2);
    lab5.addGroup(kern5);
    Target targElNull(NULL, lab5);
    ShapeModel *smElNull = ShapeModelFactory::create(&targElNull, lab5);
    cout << "    Successfully created shape " << smElNull->name() << endl;
    delete smElNull;

    // Test ShapeModel dem that's not Equatorial Cylindrical
    cout << endl << "  Testing DEM not equatorial cylindrical" << endl;
    PvlGroup kern6 = kern1;
    kern6 += PvlKeyword("ShapeModel", dir3.toStdString() + "ab102402.lev2.cub");
    Pvl lab6;
    lab6.addGroup(inst2);
    lab6.addGroup(kern6);
    Target targDem(NULL, lab6);
    ShapeModel *smDem = ShapeModelFactory::create(&targDem, lab6);
    cout << "    Successfully created shape " << smDem->name() << endl;
    delete smDem;

    // Test ShapeModel keyword with DSK
    cout << endl << "  Testing DSK file..." << endl;
    PvlGroup kern7 = kern1;
    FileName f7("$ISISTESTDATA/isis/src/base/unitTestData");
    QString dir7 = f7.expanded() + "/";
    kern7 += PvlKeyword("ShapeModel", dir7.toStdString() + "hay_a_amica_5_itokawashape_v1_0_64q.bds");
    Pvl lab7;
    lab7.addGroup(inst2);
    lab7.addGroup(kern7);
    Target targShDsk(NULL, lab7);
    ShapeModel *smShDsk = ShapeModelFactory::create(&targShDsk, lab7);
    cout << "    Successfully created shape " << smShDsk->name() << endl;
    delete smShDsk;

    // Test ShapeModel keyword with DSK and Embree ray tracing
    cout << endl << "  Testing DSK file with Embree ray tracing engine..." << endl;
    PvlGroup kern8 = kern7;
    kern8 += PvlKeyword("RayTraceEngine", "embree");
    kern8 += PvlKeyword("OnError", "fail");
    kern8 += PvlKeyword("BulletCubeSupported", "No");
    Pvl lab8;
    lab8.addGroup(inst2);
    lab8.addGroup(kern8);
    Target targEmbree(NULL, lab8);
    ShapeModel *smEmbree = ShapeModelFactory::create(&targEmbree, lab8);
    cout << "    Successfully created shape " << smEmbree->name() << endl;
    delete smEmbree;

    // Test ShapeModel keyword with DSK and Bullet ray tracing
    cout << endl << "  Testing DSK file with Bullet ray tracing engine..." << endl;
    PvlGroup kern9 = kern7;
    kern9 += PvlKeyword("RayTraceEngine", "bullet");
    kern9 += PvlKeyword("OnError", "fail");
    kern9 += PvlKeyword("BulletCubeSupported", "No");
    Pvl lab9;
    lab9.addGroup(inst2);
    lab9.addGroup(kern9);
    Target targBullet(NULL, lab9);
    ShapeModel *smBullet= ShapeModelFactory::create(&targBullet, lab9);
    cout << "    Successfully created shape " << smBullet->name() << endl;
    delete smBullet;

    // Create Spice and Target objects for sky test
    Target skyTarget(NULL, lab1);
    ShapeModel *skyShape = ShapeModelFactory::create(&skyTarget,  lab1);
    cout << endl << "  Testing Sky target..." << endl;
    cout << "    Shape model is " << skyShape->name() << endl;

    // Test demshape with ShapeModel keyword
    cout << endl << "  Testing DEM shape..." << endl;
    QString inputFile = "$ISISTESTDATA/isis/src/mgs/unitTestData/ab102401.cub";
    Cube cube;
    cube.open(inputFile);
    Camera *c = cube.camera();
    vector<Distance> radii(3,Distance());
    radii = c->target()->radii();
    Pvl pvl = *cube.label();
    Target targ(NULL, pvl);
    targ.setRadii(radii);
    ShapeModel *sm = ShapeModelFactory::create(&targ, pvl);
    cout << "    Successfully created shape " << sm->name() << endl;
    delete sm;
    cube.close();

    // Test ellipsoid shape (ShapeModel = Null)
    cout << endl << "  Testing Ellipsoid shape..." << endl;
    inputFile = "$ISISTESTDATA/isis/src/galileo/unitTestData/1213r.cub";
    cube.open(inputFile);
    c = cube.camera();
    radii = c->target()->radii();
    pvl = *cube.label();
    Target targ2(NULL, pvl);
    targ2.setRadii(radii);
    sm = ShapeModelFactory::create(&targ2, pvl);
    cout << "    Successfully created shape " << sm->name() << endl;
    delete sm;
    cube.close();

    // Test plane shape  TBD
    // inputFile = "$ISISDATA/;
    // cube.open(inputFile);
    // c = cube.camera();
    // radii = c->target()->radii();
    // pvl = *cube.label();
    // Target targ2(pvl);
    // targ3.setRadii(radii);
    // sm = ShapeModelFactory::Create(&targ3, pvl);
    // cout << "Successfully created shape " << sm->name() << endl;
    // delete sm;
    // cube.close();

//    cout << endl << "  Testing Bullet..." << endl;

    // Load IsisPreferences file that specifies Bullet
//    Isis::Preference::Preferences().Load("./IsisPreferences_bullet"); //TODO: move to test data area);
    
//    cout << endl << "  Testing Embree..." << endl;
    // Load IsisPreferences file that specifies Embree
//    Isis::Preference::Preferences().Load("./IsisPreferences_embree"); //TODO: move to test data area

    cout << endl << "=========================== Testing Errors ===========================" << endl;
    try {
      // Test ShapeModel file that does not exist
      cout << endl << "  Testing nonexistent file for shape model dem" << endl;
      PvlGroup kernError = kern1;
      kernError += PvlKeyword("ShapeModel", "NotAFile");
      Pvl labError;
      labError.addGroup(inst2);
      labError.addGroup(kernError);
      Target targBadFile(NULL, labError);
      ShapeModel *smBadFile = ShapeModelFactory::create(&targBadFile, lab4);
      cout << "    Successfully created shape " << smBadFile->name() << endl;
      delete smBadFile;
    }
    catch(Isis::IException &e) {
      e.print();
    }

    try {
      // Test ShapeModel that's not a valid Isis map projection
      cout << endl << "  Testing Isis cube file for dem that is not map projected" << endl;
      PvlGroup kernError = kern1;
      kernError += PvlKeyword("ShapeModel", dir3.toStdString() + "ab102401.cub");
      Pvl labError;
      labError.addGroup(inst2);
      labError.addGroup(kernError);
      Target targBadFile(NULL, labError);
      ShapeModel *smBadFile = ShapeModelFactory::create(&targBadFile, lab4);
      cout << "    Successfully created shape " << smBadFile->name() << endl;
      delete smBadFile;
    }
    catch(Isis::IException &e) {
      ReportError(e.toString());
    }

    try {
      // Test ShapeModel without shape model statistics
      cout << endl << "  Testing Isis cube file for dem that is missing shape model statistics" << endl;
      PvlGroup kernError = kern1;
      kernError += PvlKeyword("ShapeModel", "unitTestDemNoShapeModelStats.pvl");
      Pvl labError;
      labError.addGroup(inst2);
      labError.addGroup(kernError);
      Target targBadFile(NULL, labError);
      ShapeModel *smBadFile = ShapeModelFactory::create(&targBadFile, lab4);
      cout << "    Successfully created shape " << smBadFile->name() << endl;
      delete smBadFile;
    }
    catch (Isis::IException &e) {
      e.print();
    }
  }
  catch (IException &e) {
    IException(e, IException::Programmer,
              "\n\n\n------------Unit Test Failed.------------",
              _FILEINFO_).print();
  }
}
