#include <iostream>

#include "cnetcheck.h"

#include "Cube.h"
#include "CubeAttribute.h"
#include "PixelType.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "TestUtilities.h"
#include "FileName.h"
#include "ControlNet.h"
#include "SerialNumber.h"
#include "ControlMeasure.h"
#include "ControlPoint.h"

#include "NetworkFixtures.h"
#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/cnetcheck.xml").expanded());

TEST_F(ThreeImageNetwork, FunctionalTestCnetcheckCamera) {
  QVector<QString> args = {"fromlist="+cubeListFile, "prefix="+tempDir.path()+"/", "nocube=false", "lowcoverage=false"};
  UserInterface options(APP_XML, args);

  QString cube1Serial = SerialNumber::Compose(*cube1->label());
  QString cube2Serial = SerialNumber::Compose(*cube2->label());

  // Add measure guarenteed to fail computing lat/lon
  ControlMeasure *m1 = new ControlMeasure();
  m1->SetAprioriLine(481);
  m1->SetAprioriSample(481);
  m1->SetCamera(cube1->camera());
  m1->SetCubeSerialNumber(cube1Serial);

  ControlMeasure *m2 = new ControlMeasure();
  m2->SetAprioriLine(999);
  m2->SetAprioriSample(999);
  m2->SetCamera(cube2->camera());
  m2->SetCubeSerialNumber(cube2Serial);

  ControlPoint *newPoint = new ControlPoint();
  newPoint->Add(m1);
  newPoint->Add(m2);

  network->AddPoint(newPoint);

  Pvl log;
  cnetcheck(*network, *cubeList, options, &log);

  std::ifstream f(tempDir.path().toStdString() + "/NoLatLon.txt");
  std::string ret((std::istreambuf_iterator<char>(f)),
                 std::istreambuf_iterator<char>());

  EXPECT_THAT(ret, testing::HasSubstr(cube1Serial.toStdString()));
  EXPECT_THAT(ret, testing::HasSubstr(cube2Serial.toStdString()));

  EXPECT_THAT(ret, testing::HasSubstr(cube1->fileName().toStdString()));
  EXPECT_THAT(ret, testing::HasSubstr(cube2->fileName().toStdString()));

  PvlGroup pvlResults = log.findGroup("Results");
  EXPECT_TRUE((int)pvlResults.findKeyword("Islands") == 1);
  EXPECT_TRUE((int)pvlResults.findKeyword("NoLatLonCubes") == 2);
  EXPECT_FALSE(pvlResults.hasKeyword("SingleCube"));
}


TEST_F(ThreeImageNetwork, FunctionalTestCnetcheckNoPoints) {
  // remove all controlpoints from test network
  QList<ControlPoint*> cpoints = network->GetPoints();

  for (int i = 0; i<cpoints.size(); i++) {
    network->DeletePoint(cpoints[i]);
  }

  QVector<QString> args = {"fromlist="+cubeListFile, "prefix="+tempDir.path()+"/", "delimit=comma", "lowcoverage=false", "cnet=test"};
  UserInterface options(APP_XML, args);

  Pvl log;
  cnetcheck(*network, *cubeList, options, &log);

  std::ifstream f(tempDir.path().toStdString() + "/NoControl.txt");
  std::string ret((std::istreambuf_iterator<char>(f)),
                  std::istreambuf_iterator<char>());

  EXPECT_THAT(ret, testing::HasSubstr(cube1->fileName().toStdString()));
  EXPECT_THAT(ret, testing::HasSubstr(cube2->fileName().toStdString()));
  EXPECT_THAT(ret, testing::HasSubstr(cube3->fileName().toStdString()));

  PvlGroup pvlResults = log.findGroup("Results");
  EXPECT_EQ((int)pvlResults.findKeyword("Islands"), 0);
  EXPECT_EQ((int)pvlResults.findKeyword("NoControl"), 3);
}


TEST_F(ThreeImageNetwork, FunctionalTestCnetcheckIslands) {
  // Add measures with fake serials not in the cubelist
  ControlMeasure *m1 = new ControlMeasure();
  m1->SetAprioriLine(481);
  m1->SetAprioriSample(481);
  m1->SetCubeSerialNumber("thisIsFakeLol");

  ControlMeasure *m2 = new ControlMeasure();
  m2->SetAprioriLine(481);
  m2->SetAprioriSample(481);
  m2->SetCubeSerialNumber("thisIsFakeLol2");

  ControlPoint *newPoint = new ControlPoint();
  newPoint->Add(m1);
  newPoint->Add(m2);

  network->AddPoint(newPoint);

  // append cube not in network
  FileName c("data/defaultImage/defaultCube.pvl");
  cubeList->append(c.expanded());

  QVector<QString> args = {"fromlist="+cubeListFile, "prefix="+tempDir.path()+"/", "tolerance=0.95"};
  UserInterface options(APP_XML, args);

  Pvl log;
  cnetcheck(*network, *cubeList, options, &log);

  PvlGroup pvlResults = log.findGroup("Results");

  std::ifstream f(tempDir.path().toStdString() + "/SingleCube.txt");
  std::string singlecube((std::istreambuf_iterator<char>(f)),
                 std::istreambuf_iterator<char>());
  EXPECT_THAT(singlecube, testing::HasSubstr("thisIsFakeLol"));
  EXPECT_THAT(singlecube, testing::HasSubstr("thisIsFakeLol2"));

  std::ifstream f2(tempDir.path().toStdString() + "/LowCoverage.txt");
  std::string lowcov((std::istreambuf_iterator<char>(f2)),
                 std::istreambuf_iterator<char>());

  EXPECT_THAT(lowcov, testing::HasSubstr(cube1->fileName().toStdString()));
  EXPECT_THAT(lowcov, testing::HasSubstr(cube2->fileName().toStdString()));
  EXPECT_THAT(lowcov, testing::HasSubstr(cube3->fileName().toStdString()));

  EXPECT_EQ((int)pvlResults.findKeyword("Islands"), 2);
  EXPECT_EQ((int)pvlResults.findKeyword("SingleCube"), 2);
  EXPECT_EQ((int)pvlResults.findKeyword("NoCube"), 2);
  EXPECT_EQ((int)pvlResults.findKeyword("NoControl"), 1);
  EXPECT_EQ((int)pvlResults.findKeyword("LowCoverage"), 3);
}

TEST_F(ThreeImageNetwork, FunctionaltestCnetcheckIgnoredMeasures){
  QVector<QString> args = {"fromlist="+cubeListFile, "prefix="+tempDir.path()+"/", "nocube=false", "lowcoverage=false"};
  UserInterface options(APP_XML, args);

  QString cube1Serial = SerialNumber::Compose(*cube1->label());
  QString cube2Serial = SerialNumber::Compose(*cube2->label());

  // Add measure guaranteed to fail computing lat/lon
  ControlMeasure *m1 = new ControlMeasure();
  m1->SetAprioriLine(481);
  m1->SetAprioriSample(481);
  m1->SetCamera(cube1->camera());
  m1->SetCubeSerialNumber(cube1Serial);
  m1->SetIgnored(true);

  ControlMeasure *m2 = new ControlMeasure();
  m2->SetAprioriLine(999);
  m2->SetAprioriSample(999);
  m2->SetCamera(cube2->camera());
  m2->SetCubeSerialNumber(cube2Serial);
  m2->SetIgnored(true);

  ControlPoint *newPoint = new ControlPoint();
  newPoint->Add(m1);
  newPoint->Add(m2);
  newPoint->SetIgnored(true);

  network->AddPoint(newPoint);

  Pvl log;
  cnetcheck(*network, *cubeList, options, &log);

  PvlGroup pvlResults = log.findGroup("Results");
  EXPECT_TRUE((int)pvlResults.findKeyword("Islands") == 1);
  EXPECT_FALSE(pvlResults.hasKeyword("NoLatLonCubes"));
  EXPECT_FALSE(pvlResults.hasKeyword("SingleCube"));
}


TEST_F(ThreeImageNetwork, FunctionalTestCnetcheckIslandsIgnoredPoint) {
  // Add measures with fake serials not in the cubelist
  ControlMeasure *m1 = new ControlMeasure();
  m1->SetAprioriLine(481);
  m1->SetAprioriSample(481);
  m1->SetCubeSerialNumber("thisIsFakeLol");

  ControlMeasure *m2 = new ControlMeasure();
  m2->SetAprioriLine(481);
  m2->SetAprioriSample(481);
  m2->SetCubeSerialNumber("thisIsFakeLol2");

  ControlPoint *newPoint = new ControlPoint();
  newPoint->Add(m1);
  newPoint->Add(m2);
  network->AddPoint(newPoint);

  ControlMeasure *m3 = new ControlMeasure();
  m3->SetAprioriLine(881);
  m3->SetAprioriSample(881);
  m3->SetCubeSerialNumber("thisIsFakeLol2");

  // Add an ignored point that would otherwise join the islands
  ControlPoint *existingPoint = network->GetPoint("test0001");
  existingPoint->Add(m3);
  existingPoint->SetIgnored(true);

  // append cube not in network
  FileName c("data/defaultImage/defaultCube.pvl");
  cubeList->append(c.expanded());

  QVector<QString> args = {"fromlist="+cubeListFile, "prefix="+tempDir.path()+"/", "tolerance=0.95"};
  UserInterface options(APP_XML, args);

  Pvl log;
  cnetcheck(*network, *cubeList, options, &log);

  PvlGroup pvlResults = log.findGroup("Results");

  EXPECT_EQ((int)pvlResults.findKeyword("Islands"), 2);
  EXPECT_EQ((int)pvlResults.findKeyword("SingleCube"), 2);
  EXPECT_EQ((int)pvlResults.findKeyword("NoCube"), 2);
  EXPECT_EQ((int)pvlResults.findKeyword("NoControl"), 1);
  EXPECT_EQ((int)pvlResults.findKeyword("LowCoverage"), 3);
}
