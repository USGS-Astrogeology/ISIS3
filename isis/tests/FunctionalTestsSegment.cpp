#include "CubeFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "segment.h"

#include "gtest/gtest.h"

using namespace Isis;
using ::testing::HasSubstr;

static QString APP_XML = FileName("$ISISROOT/bin/xml/segment.xml").expanded();

TEST_F(LargeCube, FunctionalTestSegmentDefault) {

  QVector<QString> args = {"from=" + testCube->fileName(),
                           "nl=250",
                           "overlap=64"
                          };

  UserInterface options(APP_XML, args);
  try {
    segment(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube segOne(tempDir.path() + "/large." + "segment1.cub");

  EXPECT_EQ(segOne.sampleCount(), 1000);
  EXPECT_EQ(segOne.lineCount(), 250);
  EXPECT_EQ(segOne.bandCount(), 10);

  std::unique_ptr<Histogram> histSegOne (segOne.histogram());

  EXPECT_DOUBLE_EQ(histSegOne->Average(), 124.5);
  EXPECT_DOUBLE_EQ(histSegOne->Sum(), 31125000);
  EXPECT_EQ(histSegOne->ValidPixels(), 250000);
  EXPECT_NEAR(histSegOne->StandardDeviation(), 72.16835, 0.00001);

  Cube segTwo(tempDir.path() + "/large." + "segment2.cub");

  EXPECT_EQ(segTwo.sampleCount(), 1000);
  EXPECT_EQ(segTwo.lineCount(), 250);
  EXPECT_EQ(segTwo.bandCount(), 10);

  std::unique_ptr<Histogram> histSegTwo (segTwo.histogram());

  EXPECT_DOUBLE_EQ(histSegTwo->Average(), 310.5);
  EXPECT_DOUBLE_EQ(histSegTwo->Sum(), 77625000);
  EXPECT_EQ(histSegTwo->ValidPixels(), 250000);
  EXPECT_NEAR(histSegTwo->StandardDeviation(), 72.1683506, 0.00001);

  Cube segThree(tempDir.path() + "/large." + "segment3.cub");

  EXPECT_EQ(segThree.sampleCount(), 1000);
  EXPECT_EQ(segThree.lineCount(), 250);
  EXPECT_EQ(segThree.bandCount(), 10);

  std::unique_ptr<Histogram> histSegThree (segThree.histogram());

  EXPECT_DOUBLE_EQ(histSegThree->Average(), 496.5);
  EXPECT_DOUBLE_EQ(histSegThree->Sum(), 124125000);
  EXPECT_EQ(histSegThree->ValidPixels(), 250000);
  EXPECT_NEAR(histSegThree->StandardDeviation(), 72.16835, 0.00001);

  Cube segFour(tempDir.path() + "/large." + "segment4.cub");

  EXPECT_EQ(segFour.sampleCount(), 1000);
  EXPECT_EQ(segFour.lineCount(), 250);
  EXPECT_EQ(segFour.bandCount(), 10);

  std::unique_ptr<Histogram> histSegFour (segFour.histogram());

  EXPECT_DOUBLE_EQ(histSegFour->Average(), 682.5);
  EXPECT_DOUBLE_EQ(histSegFour->Sum(), 170625000);
  EXPECT_EQ(histSegFour->ValidPixels(), 250000);
  EXPECT_NEAR(histSegFour->StandardDeviation(), 72.168350, 0.00001);

  Cube segFive(tempDir.path() + "/large." + "segment5.cub");

  EXPECT_EQ(segFive.sampleCount(), 1000);
  EXPECT_EQ(segFive.lineCount(), 250);
  EXPECT_EQ(segFive.bandCount(), 10);

  std::unique_ptr<Histogram> histSegFive (segFive.histogram());

  EXPECT_DOUBLE_EQ(histSegFive->Average(), 868.5);
  EXPECT_DOUBLE_EQ(histSegFive->Sum(), 217125000);
  EXPECT_EQ(histSegFive->ValidPixels(), 250000);
  EXPECT_NEAR(histSegFive->StandardDeviation(), 72.16835, 0.00001);
}

TEST_F(LargeCube, FunctionalTestSegmentBoundary) {

  QVector<QString> args = {"from=" + testCube->fileName(),
                           "nl=500",
                           "overlap=88"
                          };

  UserInterface options(APP_XML, args);
  try {
    segment(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube segOne(tempDir.path() + "/large." + "segment1.cub");

  EXPECT_EQ(segOne.sampleCount(), 1000);
  EXPECT_EQ(segOne.lineCount(), 500);
  EXPECT_EQ(segOne.bandCount(), 10);

  std::unique_ptr<Histogram> histSegOne (segOne.histogram());

  EXPECT_DOUBLE_EQ(histSegOne->Average(), 249.5);
  EXPECT_DOUBLE_EQ(histSegOne->Sum(), 124750000);
  EXPECT_EQ(histSegOne->ValidPixels(), 500000);
  EXPECT_NEAR(histSegOne->StandardDeviation(), 144.3374229, 0.00001);

  Cube segTwo(tempDir.path() + "/large." + "segment2.cub");

  EXPECT_EQ(segTwo.sampleCount(), 1000);
  EXPECT_EQ(segTwo.lineCount(), 500);
  EXPECT_EQ(segTwo.bandCount(), 10);

  std::unique_ptr<Histogram> histSegTwo (segTwo.histogram());

  EXPECT_DOUBLE_EQ(histSegTwo->Average(), 661.5);
  EXPECT_DOUBLE_EQ(histSegTwo->Sum(), 330750000);
  EXPECT_EQ(histSegTwo->ValidPixels(), 500000);
  EXPECT_NEAR(histSegTwo->StandardDeviation(), 144.33742295, 0.00001);

  Cube segThree(tempDir.path() + "/large." + "segment3.cub");

  EXPECT_EQ(segThree.sampleCount(), 1000);
  EXPECT_EQ(segThree.lineCount(), 176);
  EXPECT_EQ(segThree.bandCount(), 10);

  std::unique_ptr<Histogram> histSegThree (segThree.histogram());

  EXPECT_DOUBLE_EQ(histSegThree->Average(), 911.5);
  EXPECT_DOUBLE_EQ(histSegThree->Sum(), 160424000);
  EXPECT_EQ(histSegThree->ValidPixels(), 176000);
  EXPECT_NEAR(histSegThree->StandardDeviation(), 50.806147, 0.00001);
}

TEST_F(LargeCube, FunctionalTestSegmentNolError) {

  QVector<QString> args = {"from=" + testCube->fileName(),
                           "nl=500",
                           "overlap=600"
                          };

  UserInterface options(APP_XML, args);
  try {
    segment(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("The Line Overlap (OVERLAP) must be less than the Number of Lines (LN)"));
  }
}
