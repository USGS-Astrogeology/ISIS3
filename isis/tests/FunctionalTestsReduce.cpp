#include "CubeFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "reduce_app.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace Isis;
using ::testing::HasSubstr;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/reduce.xml").expanded());

TEST_F(LargeCube, FunctionalTestReduceDefault) {
  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTemp.cub";
  QVector<QString> args = {"from=" + testCube->fileName(),
                            "to=" + outCubeFileName,
                            "algorithm=average",
                            "mode=total",
                            "ons=100",
                            "onl=100"
                          };

  UserInterface options(APP_XML, args);
  try {
    reduce(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube ocube(outCubeFileName.toStdString());

  ASSERT_EQ(ocube.sampleCount(), 100);
  ASSERT_EQ(ocube.lineCount(), 100);
  ASSERT_EQ(ocube.bandCount(), 10);

  std::unique_ptr<Histogram> hist (ocube.histogram());

  ASSERT_DOUBLE_EQ(hist->Average(), 499.5);
  ASSERT_DOUBLE_EQ(hist->Sum(), 4995000);
  ASSERT_EQ(hist->ValidPixels(), 10000);
  ASSERT_NEAR(hist->StandardDeviation(), 288.67513, 0.00001);

}

TEST_F(LargeCube, FunctionalTestReduceAverageScale1) {
  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTemp.cub";
  QVector<QString> args = {"from=" + testCube->fileName(),
                            "to=" + outCubeFileName,
                            "algorithm=average",
                            "sscale=1",
                            "lscale=1"
                          };

  UserInterface options(APP_XML, args);
  try {
    reduce(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube ocube(outCubeFileName.toStdString());

  ASSERT_EQ(ocube.sampleCount(), 1000);
  ASSERT_EQ(ocube.lineCount(), 1000);
  ASSERT_EQ(ocube.bandCount(), 10);

  std::unique_ptr<Histogram> hist (ocube.histogram());

  ASSERT_DOUBLE_EQ(hist->Average(), 499.5);
  ASSERT_DOUBLE_EQ(hist->Sum(), 499500000);
  ASSERT_EQ(hist->ValidPixels(), 1000000);
  ASSERT_NEAR(hist->StandardDeviation(), 288.67513, 0.00001);

}

TEST_F(LargeCube, FunctionalTestReduceAverageScale2) {
  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTemp.cub";
  QVector<QString> args = {"from=" + testCube->fileName(),
                            "to=" + outCubeFileName,
                            "algorithm=average",
                            "sscale=10",
                            "lscale=10"
                          };

  UserInterface options(APP_XML, args);
  try {
    reduce(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube ocube(outCubeFileName.toStdString());

  ASSERT_EQ(ocube.sampleCount(), 100);
  ASSERT_EQ(ocube.lineCount(), 100);
  ASSERT_EQ(ocube.bandCount(), 10);

  std::unique_ptr<Histogram> hist (ocube.histogram());

  ASSERT_DOUBLE_EQ(hist->Average(), 499.5);
  ASSERT_DOUBLE_EQ(hist->Sum(), 4995000);
  ASSERT_EQ(hist->ValidPixels(), 10000);
  ASSERT_NEAR(hist->StandardDeviation(), 288.67513, 0.00001);

}

TEST_F(LargeCube, FunctionalTestReduceNearestNeighbor) {
  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTemp.cub";
  QVector<QString> args = {"from=" + testCube->fileName(),
                            "to=" + outCubeFileName,
                            "algorithm=nearest",
                            "sscale=10",
                            "lscale=10"
                          };

  UserInterface options(APP_XML, args);
  try {
    reduce(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube ocube(outCubeFileName.toStdString());

  ASSERT_EQ(ocube.sampleCount(), 100);
  ASSERT_EQ(ocube.lineCount(), 100);
  ASSERT_EQ(ocube.bandCount(), 10);

  std::unique_ptr<Histogram> hist (ocube.histogram());

  ASSERT_DOUBLE_EQ(hist->Average(), 495);
  ASSERT_DOUBLE_EQ(hist->Sum(), 4950000);
  ASSERT_EQ(hist->ValidPixels(), 10000);
  ASSERT_NEAR(hist->StandardDeviation(), 288.67513, 0.00001);

}

 /* This particular test is testing the calulation of scale and output line and sample.  This is a
    result of a sporadic bug that was found (Mantis #1385) which only occurs at certain scales and
    user selected output number of lines.  For certain ONL values, the scale was calculated in
    the reduce application and passed into the Reduce object, which then calculated the ONL/ONS.  For
    certain scales there would be a round off error and the ONL/ONS would be 1 greater than what the
    user had entered.
 */
TEST_F(LargeCube, FunctionalTestReduceRoundOff) {
  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTemp.cub";
  QVector<QString> args = {"from=" + testCube->fileName(),
                            "to=" + outCubeFileName,
                            "algorithm=nearest",
                            "mode=total",
                            "ons=80",
                            "onl=483"
                          };

  UserInterface options(APP_XML, args);
  try {
    reduce(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube ocube(outCubeFileName.toStdString());

  ASSERT_EQ(ocube.sampleCount(), 80);
  ASSERT_EQ(ocube.lineCount(), 483);
  ASSERT_EQ(ocube.bandCount(), 10);

  std::unique_ptr<Histogram> hist (ocube.histogram());

  ASSERT_NEAR(hist->Average(), 498.96480, 0.00001);
  ASSERT_DOUBLE_EQ(hist->Sum(), 19280000);
  ASSERT_EQ(hist->ValidPixels(), 38640);
  ASSERT_NEAR(hist->StandardDeviation(), 288.68338, 0.00001);

}

TEST_F(LargeCube, FunctionalTestReduceError) {
  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTemp.cub";
  QVector<QString> args = {"from=" + testCube->fileName(),
                            "to=" + outCubeFileName,
                            "algorithm=average",
                            "mode=total",
                            "ons=1200",
                            "onl=100"
                          };

  UserInterface options(APP_XML, args);
  try{
    reduce(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e){
    EXPECT_THAT(e.what(), HasSubstr("Number of output samples/lines must be less than or equal"));
  }

}
