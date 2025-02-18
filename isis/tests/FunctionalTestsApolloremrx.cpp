#include "Brick.h"
#include "CubeFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "apolloremrx.h"

#include "gtest/gtest.h"

using namespace Isis;
using ::testing::HasSubstr;

static QString APP_XML = FileName("$ISISROOT/bin/xml/apolloremrx.xml").expanded();

TEST_F(ApolloCube, FunctionalTestApolloremrxDefault) {

  testCube->group("RESEAUS")["STATUS"] = "Refined";

  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path()+"/outTEMP.cub";
  QVector<QString> args = {"to=" + outCubeFileName,
                           "action=null"};

  UserInterface options(APP_XML, args);
  try {
    apolloremrx(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Call failed, Unable to process cube: " << e.what() << std::endl;
  }

  Cube cube(outCubeFileName);
  Pvl *isisLabel = cube.label();

  PvlGroup reseausGroup = isisLabel->findObject("IsisCube").findGroup("Reseaus");
  PvlKeyword lineKey = reseausGroup.findKeyword("Line");
  EXPECT_NEAR(lineKey[0].toDouble(), 200, 0.0001);
  EXPECT_NEAR(lineKey[1].toDouble(), 400, 0.0001);
  EXPECT_NEAR(lineKey[2].toDouble(), 600, 0.0001);

  PvlKeyword sampleKey = reseausGroup.findKeyword("Sample");
  EXPECT_NEAR(sampleKey[0].toDouble(), 200, 0.0001);
  EXPECT_NEAR(sampleKey[1].toDouble(), 400, 0.0001);
  EXPECT_NEAR(sampleKey[2].toDouble(), 600, 0.0001);

  PvlKeyword validKey = reseausGroup.findKeyword("Valid");
  EXPECT_EQ(validKey[0].toInt(), 1);
  EXPECT_EQ(validKey[1].toInt(), 1);
  EXPECT_EQ(validKey[2].toInt(), 1);

  EXPECT_PRED_FORMAT2(AssertQStringsEqual, reseausGroup.findKeyword("Status"), "Removed");

  Brick brick(reseauSize,reseauSize,1,cube.pixelType());

  for (size_t i = 0; i < reseaus.size(); i++) {
    int baseSamp = (int)(reseaus[i].first+0.5) - (reseauSize/2);
    int baseLine = (int)(reseaus[i].second+0.5) - (reseauSize/2);
    brick.SetBasePosition(baseSamp,baseLine,1);
    cube.read(brick);
    Statistics reseauStats;
    reseauStats.AddData(&brick[0], brick.size());

    EXPECT_NEAR(reseauStats.Average(), i, 0.001) << "Reseau " << i;
    EXPECT_EQ(reseauStats.ValidPixels(), 9604) << "Reseau " << i;
    EXPECT_EQ(reseauStats.NullPixels(), 1005) << "Reseau " << i;
    EXPECT_NEAR(reseauStats.StandardDeviation(), 0.0, 0.001) << "Reseau " << i;
  }
}

TEST_F(ApolloCube, FunctionalTestApolloremrxPatch) {

  testCube->group("RESEAUS")["STATUS"] = "Refined";

  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path()+"/outTEMP.cub";
  QVector<QString> args = {"to=" + outCubeFileName,
                           "action=PATCH"};

  UserInterface options(APP_XML, args);
  try {
    apolloremrx(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Call failed, Unable to process cube: " << e.what() << std::endl;
  }

  Cube cube(outCubeFileName);

  Brick brick(reseauSize,reseauSize,1,cube.pixelType());

  for (size_t i = 0; i < reseaus.size(); i++) {
    int baseSamp = (int)(reseaus[i].first+0.5) - (reseauSize/2);
    int baseLine = (int)(reseaus[i].second+0.5) - (reseauSize/2);
    brick.SetBasePosition(baseSamp,baseLine,1);
    cube.read(brick);
    Statistics reseauStats;
    reseauStats.AddData(&brick[0], brick.size());

    EXPECT_EQ(reseauStats.ValidPixels(), 10609) << "Reseau " << i;
    EXPECT_EQ(reseauStats.NullPixels(), 0) << "Reseau " << i;
  }
}

TEST_F(ApolloCube, FunctionalTestApolloremrxRemovedError) {

  testCube->group("RESEAUS")["STATUS"] = "Removed";

  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path()+"/outTEMP.cub";
  QVector<QString> args = {"to=" + outCubeFileName,"action=null"};

  UserInterface options(APP_XML, args);
  try {
    apolloremrx(testCube, options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("appears to already have reseaus removed"));
  }
}

TEST_F(ApolloCube, FunctionalTestApolloremrxSpacecraftError) {

  testCube->group("Instrument")["SpacecraftName"] = "Galileo Orbiter";

  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path()+"/outTEMP.cub";
  QVector<QString> args = {"to=" + outCubeFileName,"action=null"};

  UserInterface options(APP_XML, args);
  try {
    apolloremrx(testCube, options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("This application is for use with Apollo spacecrafts only"));
  }
}

TEST_F(ApolloCube, FunctionalTestApolloremrxNominalError) {

  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path()+"/outTEMP.cub";
  QVector<QString> args = {"to=" + outCubeFileName,"action=null"};

  UserInterface options(APP_XML, args);
  try {
    apolloremrx(testCube, options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("appears to have nominal reseau status. You must run findrx first"));
  }
}
