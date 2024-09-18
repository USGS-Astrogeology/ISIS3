#include "CameraFixtures.h"
#include "CubeFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "crop.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace Isis;
using ::testing::HasSubstr;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/crop.xml").expanded());

TEST_F(LargeCube, FunctionalTestCropDefault) {
  QTemporaryDir tempDir;
  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QVector<QString> args = {"from="+ testCube->fileName(),  "to="+outCubeFileName,
    "sample=818", "nsamples=100", "sinc=1", "line=700", "nlines=200", "linc=1"};

  UserInterface options(APP_XML, args);
  try {
    crop(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString(), "r");

  Histogram *oCubeStats = oCube.histogram();

  EXPECT_NEAR(oCubeStats->Average(), 798.5, 0.01);
  EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 15970000);
  EXPECT_DOUBLE_EQ(oCubeStats->ValidPixels(), 20000);
  EXPECT_NEAR(oCubeStats->StandardDeviation(), 57.735748638374318, 0.0000000001);
}

TEST_F(LargeCube, FunctionalTestCropSkip1) {
  QTemporaryDir tempDir;
  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QVector<QString> args = {"from="+ testCube->fileName(),  "to="+outCubeFileName,
    "sample=1", "nsamples=10", "sinc=1", "line=3", "nlines=1", "linc=1"};

  UserInterface options(APP_XML, args);
  try {
    crop(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
}

  Cube oCube(outCubeFileName.toStdString(), "r");

  Histogram *oCubeStats = oCube.histogram();

  EXPECT_NEAR(oCubeStats->Average(), 2, 0.01);
  EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 20);
  EXPECT_DOUBLE_EQ(oCubeStats->ValidPixels(), 10);
  EXPECT_NEAR(oCubeStats->StandardDeviation(), 0, 0.0000000001);
}

TEST_F(LargeCube, FunctionalTestCropSkip2) {
  QTemporaryDir tempDir;
  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QVector<QString> args = {"from="+ testCube->fileName(),  "to="+outCubeFileName,
    "sample=50", "nsamples=50", "sinc=2", "line=50", "nlines=50", "linc=3"};

  UserInterface options(APP_XML, args);
  try {
    crop(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString(), "r");

  Histogram *oCubeStats = oCube.histogram();

  EXPECT_NEAR(oCubeStats->Average(), 73, 0.01);
  EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 31025);
  EXPECT_DOUBLE_EQ(oCubeStats->ValidPixels(), 425);
  EXPECT_NEAR(oCubeStats->StandardDeviation(), 14.714259545157688, 0.0000000001);
}

TEST_F(LargeCube, FunctionalTestCropSkip5) {
    QTemporaryDir tempDir;
    QString outCubeFileName = tempDir.path() + "/outTemp.cub";
    QVector<QString> args = {"from="+ testCube->fileName(),  "to="+outCubeFileName,
      "sample=5", "nsamples=10", "sinc=5", "line=5", "nlines=10", "linc=5"};

    UserInterface options(APP_XML, args);
    try {
      crop(options);
    }
    catch (IException &e) {
      FAIL() << "Unable to open image: " << e.what() << std::endl;
    }

    Cube oCube(outCubeFileName.toStdString(), "r");

    Histogram *oCubeStats = oCube.histogram();

    EXPECT_NEAR(oCubeStats->Average(), 6.5, 0.01);
    EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 26);
    EXPECT_DOUBLE_EQ(oCubeStats->ValidPixels(), 4);
    EXPECT_NEAR(oCubeStats->StandardDeviation(), 2.8867513459481291, 0.0000000001);
}

TEST_F(DefaultCube, FunctionalTestCropNoSpice) {
    QTemporaryDir tempDir;
    QString outCubeFileName = tempDir.path() + "/outTemp.cub";
    QVector<QString> args = {"from="+ testCube->fileName(),  "to="+outCubeFileName,
      "sample=5", "nsamples=10", "sinc=5", "line=1", "nlines=2", "linc=2", "propspice=false"};

    testCube->close();

    UserInterface options(APP_XML, args);
    try {
      crop(options);
    }
    catch (IException &e) {
      FAIL() << "Unable to open image: " << e.what() << std::endl;
    }

    Cube oCube(outCubeFileName.toStdString(), "r");

    Histogram *oCubeStats = oCube.histogram();

    EXPECT_NEAR(oCubeStats->Average(), 7.5, 0.01);
    EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 15);
    EXPECT_DOUBLE_EQ(oCubeStats->ValidPixels(), 2);
    EXPECT_NEAR(oCubeStats->StandardDeviation(), 3.5355339059327378, 0.0000000001);
}

TEST_F(DefaultCube, FunctionalTestCropProj) {
    QTemporaryDir tempDir;
    QString outCubeFileName = tempDir.path() + "/outTemp.cub";
    QVector<QString> args = {"from="+ projTestCube->fileName(),  "to="+outCubeFileName,
      "sample=1", "nsamples=6", "sinc=2", "line=1", "nlines=2", "linc=2", "propspice=false"};

    projTestCube->close();

    UserInterface options(APP_XML, args);
    try {
      crop(options);
    }
    catch (IException &e) {
      FAIL() << "Unable to open image: " << e.what() << std::endl;
    }

    Cube oCube(outCubeFileName.toStdString(), "r");

    Histogram *oCubeStats = oCube.histogram();

    EXPECT_NEAR(oCubeStats->Average(), 3, 0.01);
    EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 9);
    EXPECT_DOUBLE_EQ(oCubeStats->ValidPixels(), 3);
    EXPECT_NEAR(oCubeStats->StandardDeviation(), 2, 0.0000000001);
}

TEST_F(DefaultCube, FunctionalTestCropError1) {
    QTemporaryDir tempDir;
    QString outCubeFileName = tempDir.path() + "/outTemp.cub";
    QVector<QString> args = {"from="+ testCube->fileName(),  "to="+outCubeFileName,
      "sample=2000", "nsamples=10", "sinc=5", "line=5", "nlines=10", "linc=5"};

    UserInterface options(APP_XML, args);

    try{
        crop(options);
        FAIL() << "Should throw an exception" << std::endl;
    }
    catch (IException &e){
        EXPECT_THAT(e.what(), HasSubstr("exceeds number of samples in"));
    }
}

TEST_F(DefaultCube, FunctionalTestCropError2) {
    QTemporaryDir tempDir;
    QString outCubeFileName = tempDir.path() + "/outTemp.cub";
    QVector<QString> args = {"from="+ testCube->fileName(),  "to="+outCubeFileName,
      "sample=50", "nsamples=10", "sinc=5", "line=2000", "nlines=10", "linc=5"};

    UserInterface options(APP_XML, args);

    try{
        crop(options);
        FAIL() << "Should throw an exception" << std::endl;
    }
    catch (IException &e){
        EXPECT_THAT(e.what(), HasSubstr("exceeds number of lines in"));
    }
}

TEST_F(DefaultCube, FunctionalTestCropError3) {
    QTemporaryDir tempDir;
    QString outCubeFileName = tempDir.path() + "/outTemp.cub";
    QVector<QString> args = {"from="+ testCube->fileName(),  "to="+outCubeFileName,
      "sample=1000", "nsamples=500", "sinc=5", "line=5", "nlines=10", "linc=5"};

    UserInterface options(APP_XML, args);

    try{
        crop(options);
        FAIL() << "Should throw an exception" << std::endl;
    }
    catch (IException &e){
        EXPECT_THAT(e.what(), HasSubstr("exceeds number of samples in"));
    }
}

TEST_F(DefaultCube, FunctionalTestCropError4) {
    QTemporaryDir tempDir;
    QString outCubeFileName = tempDir.path() + "/outTemp.cub";
    QVector<QString> args = {"from="+ testCube->fileName(),  "to="+outCubeFileName,
      "sample=50", "nsamples=10", "sinc=5", "line=1000", "nlines=500", "linc=5"};

    UserInterface options(APP_XML, args);

    try{
        crop(options);
        FAIL() << "Should throw an exception" << std::endl;
    }
    catch (IException &e){
        EXPECT_THAT(e.what(), HasSubstr("exceeds number of lines in"));
    }
}
