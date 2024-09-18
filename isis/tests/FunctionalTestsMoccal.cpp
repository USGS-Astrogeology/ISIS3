#include "CameraFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "moccal.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/moccal.xml").expanded());

TEST_F(MgsMocCube, FunctionalTestMoccalDefault) {
  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QVector<QString> args = {"to="+outCubeFileName};

  UserInterface options(APP_XML, args);

  try {
    moccal(testCube.get(), options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString(), "r");

  PvlGroup radGroup = oCube.label()->findObject("IsisCube").findGroup("Radiometry");

  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("a"),   16.03);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("off"), 25.0);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("ex"),  100.0);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("z"),   27.67658);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("dc"),  0.00133691);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("g"),   0.123262);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("w0"),  3.991);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("s"),   1.423426946984);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("iof"), 0.50767834462549);

  Histogram *oCubeStats = oCube.histogram();

  EXPECT_DOUBLE_EQ(oCubeStats->Average(), 0.056909484090283513);
  EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 22.763793636113405);
  EXPECT_DOUBLE_EQ(oCubeStats->ValidPixels(), 400);
  EXPECT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 0.0021719888294085255);
}


TEST_F(MgsMocCube, FunctionalTestMoccalIofFalse) {
  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QVector<QString> args = {"to="+outCubeFileName, "iof=False"};

  UserInterface options(APP_XML, args);

  try {
    moccal(testCube.get(), options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString(), "r");

  PvlGroup radGroup = oCube.label()->findObject("IsisCube").findGroup("Radiometry");

  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("a"),   16.03);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("off"), 25.0);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("ex"),  100.0);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("z"),   27.67658);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("dc"),  0.00133691);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("g"),   0.123262);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("w0"),  3.991);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("s"),   1.423426946984);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("iof"), 1);


  Histogram *oCubeStats = oCube.histogram();

  EXPECT_DOUBLE_EQ(oCubeStats->Average(), 0.11209752136841417);
  EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 44.839008547365665);
  EXPECT_DOUBLE_EQ(oCubeStats->ValidPixels(), 400);
  EXPECT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 0.004278277553211739);
}


TEST_F(MgsMocCube, FunctionalTestMoccalNullwagoTrue) {
  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QVector<QString> args = {"to="+outCubeFileName, "nullwag=True"};

  UserInterface options(APP_XML, args);

  try {
    moccal(testCube.get(), options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString(), "r");

  PvlGroup radGroup = oCube.label()->findObject("IsisCube").findGroup("Radiometry");

  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("a"),   16.03);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("off"), 25.0);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("ex"),  100.0);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("z"),   27.67658);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("dc"),  0.00133691);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("g"),   0.123262);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("w0"),  3.991);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("s"),   1.423426946984);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("iof"), 0.50767834462549);

  Histogram *oCubeStats = oCube.histogram();

  EXPECT_DOUBLE_EQ(oCubeStats->Average(), 0.056909484090283513);
  EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 22.763793636113405);
  EXPECT_DOUBLE_EQ(oCubeStats->ValidPixels(), 400);
  EXPECT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 0.0021719888294085255);
}


TEST_F(MgsMocCube, FunctionalTestMoccalCameraComparison) {
  QString outCubeFileNameCam = tempDir.path() + "/outTemp.cub";
  QVector<QString> args = {"to="+outCubeFileNameCam};

  UserInterface options(APP_XML, args);

  try {
    moccal(testCube.get(), options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  // force camera to not construct
  Pvl *lab = testCube->label();
  lab->deleteObject("NaifKeywords");

  QString outCubeFileNameNoCam = tempDir.path() + "/outTempNoCam.cub";
  args = {"to="+outCubeFileNameNoCam};

  try {
    moccal(testCube.get(), options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube oNoCamCube(outCubeFileNameCam.toStdString(), "r");
  Cube oCamCube(outCubeFileNameCam.toStdString(), "r");

  PvlGroup noCamLab = oNoCamCube.label()->findObject("IsisCube").findGroup("Radiometry");
  PvlGroup camLab = oCamCube.label()->findObject("IsisCube").findGroup("Radiometry");

  EXPECT_DOUBLE_EQ((double)noCamLab.findKeyword("iof"), 0.50767834462549);
  EXPECT_DOUBLE_EQ((double)noCamLab.findKeyword("a"),   16.03);

  EXPECT_DOUBLE_EQ((double)noCamLab.findKeyword("iof"),
                   (double)camLab.findKeyword("iof"));

  EXPECT_DOUBLE_EQ((double)noCamLab.findKeyword("s"),
                   (double)camLab.findKeyword("s"));
}
