#include "CameraFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "vikcal.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/vikcal.xml").expanded());

TEST_F(DefaultCube, FunctionalTestVikcalDefault) {
  // Note: DefaultCube is a viking image

  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QVector<QString> args = {"to="+outCubeFileName};

  UserInterface options(APP_XML, args);
  try {
    vikcal(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString(), "r");

  PvlGroup radGroup = oCube.label()->findObject("IsisCube").findGroup("Radiometry");

  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("offc"), 0);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("exp"), 7.73);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("gain"), 1.0);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("w0"), 90.36);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("w1"), 119.84873964656);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("dist0"), 243840000.0);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("dist1"), 211727039.58284);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("1.0/exp*w1"), 0.0010794114853583);

  Histogram *oCubeStats = oCube.histogram();

  EXPECT_DOUBLE_EQ(oCubeStats->Average(), 0.14601107854966053);
  EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 184914.12430735352);
  EXPECT_DOUBLE_EQ(oCubeStats->ValidPixels(), 1266439);
  EXPECT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 0.07892281197170499);
}


TEST_F(DefaultCube, FunctionalTestCtxcalCameraComparison) {
  // Note: DefaultCube is a viking image

  QString outCubeFileNameCam = tempDir.path() + "/outTemp.cub";
  QVector<QString> args = {"to="+outCubeFileNameCam};

  UserInterface options(APP_XML, args);

  try {
    vikcal(testCube, options);
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
    vikcal(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube oNoCamCube(outCubeFileNameCam.toStdString(), "r");
  Cube oCamCube(outCubeFileNameCam.toStdString(), "r");

  Pvl *noCamLab = oNoCamCube.label();
  Pvl *camLab = oCamCube.label();

  EXPECT_DOUBLE_EQ((double)noCamLab->findObject("IsisCube").findGroup("Radiometry").findKeyword("dist1"),
                   (double)camLab->findObject("IsisCube").findGroup("Radiometry").findKeyword("dist1"));

  EXPECT_DOUBLE_EQ((double)noCamLab->findObject("IsisCube").findGroup("Radiometry").findKeyword("dist1"), 211727039.58284);
}