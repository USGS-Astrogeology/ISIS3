#include "CameraFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "ctxcal.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/ctxcal.xml").expanded());


TEST_F(MroCtxCube, DISABLED_FunctionalTestCtxcalDefault) {
  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QVector<QString> args = {"to="+outCubeFileName};

  UserInterface options(APP_XML, args);

  try {
    ctxcal(testCube.get(), options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString(), "r");

  PvlGroup radGroup = oCube.label()->findObject("IsisCube").findGroup("Radiometry");
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("iof"), 1.86764430855461e-04);

  Histogram *oCubeStats = oCube.histogram();

  EXPECT_DOUBLE_EQ(oCubeStats->Average(), 0.080529551990330225);
  EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 32.211820796132088);
  EXPECT_DOUBLE_EQ(oCubeStats->ValidPixels(), 400);
  EXPECT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 0.0012845090812918776);
}


TEST_F(MroCtxCube, FunctionalTestCtxcalFlatfile) {
  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QVector<QString> args = {"to="+outCubeFileName, "flatfile=$ISISDATA/mro/calibration/ctxFlat_0001.cub"};

  UserInterface options(APP_XML, args);

  try {
    ctxcal(testCube.get(), options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString(), "r");

  Histogram *oCubeStats = oCube.histogram();

  EXPECT_DOUBLE_EQ(oCubeStats->Average(), 0.10046864503994585);
  EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 40.187458015978336);
  EXPECT_DOUBLE_EQ(oCubeStats->ValidPixels(), 400);
  EXPECT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 0.0018248585597074806);
}

TEST_F(MroCtxCube, FunctionalTestCtxcalMonthlyFlatfile) {
  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QVector<QString> args = {"to="+outCubeFileName, "monthlyflat=True"};

  UserInterface options(APP_XML, args);

  try {
    ctxcal(testCube.get(), options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString(), "r");

  Histogram *oCubeStats = oCube.histogram();

  EXPECT_DOUBLE_EQ(oCubeStats->Average(), 0.080543650835752489);
  EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 32.217460334300995);
  EXPECT_DOUBLE_EQ(oCubeStats->ValidPixels(), 400);
  EXPECT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 0.0012787322597001109);
}


TEST_F(MroCtxCube, DISABLED_FunctionalTestCtxcalIofFalse) {
  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QVector<QString> args = {"to="+outCubeFileName, "iof=false"};

  UserInterface options(APP_XML, args);

  try {
    ctxcal(testCube.get(), options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString(), "r");

  PvlGroup radGroup = oCube.label()->findObject("IsisCube").findGroup("Radiometry");
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("iof"), 1);

  Histogram *oCubeStats = oCube.histogram();

  EXPECT_DOUBLE_EQ(oCubeStats->Average(), 229.35238750457765);
  EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 91740.955001831055);
  EXPECT_DOUBLE_EQ(oCubeStats->ValidPixels(), 400);
  EXPECT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 3.6583500046604196);
}


TEST_F(MroCtxCube, FunctionalTestCtxcalCameraComparison) {
  QString outCubeFileNameCam = tempDir.path() + "/outTemp.cub";
  QVector<QString> args = {"to="+outCubeFileNameCam};

  UserInterface options(APP_XML, args);

  try {
    ctxcal(testCube.get(), options);
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
    ctxcal(testCube.get(), options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube oNoCamCube(outCubeFileNameCam.toStdString(), "r");
  Cube oCamCube(outCubeFileNameCam.toStdString(), "r");

  Pvl *noCamLab = oNoCamCube.label();
  Pvl *camLab = oCamCube.label();

  EXPECT_DOUBLE_EQ((double)noCamLab->findObject("IsisCube").findGroup("Radiometry").findKeyword("iof"),
                   (double)camLab->findObject("IsisCube").findGroup("Radiometry").findKeyword("iof"));

  EXPECT_DOUBLE_EQ((double)noCamLab->findObject("IsisCube").findGroup("Radiometry").findKeyword("iof"), 1.86764430855461e-04);
}

