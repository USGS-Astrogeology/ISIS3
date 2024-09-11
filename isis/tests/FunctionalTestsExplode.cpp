#include "explode.h"

#include <QFileInfo> 
#include <QString>

#include "CameraFixtures.h"
#include "Cube.h"
#include "Histogram.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/explode.xml").expanded();

/**
   * ExplodeDefault Test
   * 
   * ExplodeDefault test given a single 5x5 input cube with 2 bands.
   *
   * The output cube is verified by checking the histogram statistics
   * for each band.
   * 
   * INPUT: testCube from DefaultCube fixture resized to 5x5x1
   * 
   * OUTPUT: 1) explodeOut.band0001.cub
   *         2) explodeOut.band0002.cub
   */
TEST_F(DefaultCube, FunctionalTestExplodeDefault) {

  // reduce test cube size and create two bands
  resizeCube(5, 5, 2);

  // close and reopen test cube to ensure dn buffer is available
  testCube->reopen("r");

  // run explode  
  QVector<QString> args = {"to=" + tempDir.path() + "/explodeOut"};
  UserInterface ui(APP_XML, args);

  try {
    explode(testCube, ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  // validate band 1 output cube
  Cube outCube1(tempDir.path() + "/explodeOut.band0001.cub");

  ASSERT_EQ(outCube1.sampleCount(), 5);
  ASSERT_EQ(outCube1.lineCount(), 5);
  ASSERT_EQ(outCube1.bandCount(), 1);

  std::unique_ptr<Histogram> inCubeBand1Hist (testCube->histogram(1));
  std::unique_ptr<Histogram> outCube1Hist (outCube1.histogram(1));

  EXPECT_NEAR(outCube1Hist->Average(), inCubeBand1Hist->Average(), .000001);
  EXPECT_NEAR(outCube1Hist->Sum(), inCubeBand1Hist->Sum(), .000001);
  EXPECT_EQ(outCube1Hist->ValidPixels(), inCubeBand1Hist->ValidPixels());
  EXPECT_EQ(outCube1Hist->TotalPixels(), inCubeBand1Hist->TotalPixels());
  EXPECT_NEAR(outCube1Hist->StandardDeviation(), inCubeBand1Hist->StandardDeviation(), .000001);

  // validate band 2 output cube
  Cube outCube2(tempDir.path() + "/explodeOut.band0002.cub");

  ASSERT_EQ(outCube2.sampleCount(), 5);
  ASSERT_EQ(outCube2.lineCount(), 5);
  ASSERT_EQ(outCube2.bandCount(), 1);

  std::unique_ptr<Histogram> inCubeBand2Hist (testCube->histogram(2));
  std::unique_ptr<Histogram> outCube2Hist (outCube2.histogram(1));

  EXPECT_NEAR(outCube2Hist->Average(), inCubeBand2Hist->Average(), .000001);
  EXPECT_NEAR(outCube2Hist->Sum(), inCubeBand2Hist->Sum(), .000001);
  EXPECT_EQ(outCube2Hist->ValidPixels(), inCubeBand2Hist->ValidPixels());
  EXPECT_EQ(outCube2Hist->TotalPixels(), inCubeBand2Hist->TotalPixels());
  EXPECT_NEAR(outCube2Hist->StandardDeviation(), inCubeBand2Hist->StandardDeviation(), .000001);
}

