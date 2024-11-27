
#include <QString>
#include <QVector>

#include "Brick.h"
#include "CameraFixtures.h"
#include "Histogram.h"
#include "bandtrim.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/bandtrim.xml").expanded();

/**
   * BandtrimDefault Test
   * 
   * BandtrimDefault test given a single 5x5 input cube with 7 bands.
   * One pixel in each band is set to Isis::Null as below.
   * N implies a Null pixel, N1 is band 1, N2 is band 2, etc.
   * All Null pixels should be duplicated across each band in the
   * output Cube.
   * 
   * The output cube is verified by checking the histogram statistics
   * for each band.
   * 
   * |  |N1|  |N2|  | 
   * |  |  |  |  |  |
   * |N3|  |N4|  |N5|
   * |  |  |  |  |  |
   * |  |N6|  |N7|  |
   * 
   * INPUT: testCube from DefaultCube fixture modified as above.
   * 
   * OUTPUT: bandtrimDefaultOut.cub
   * 
   */
TEST_F(DefaultCube, FunctionalTestBandtrimDefault) {

  // reduce test cube size, create seven bands
  resizeCube(5, 5, 7);

  // set one pixel in each of the seven bands to Isis::Null
  // following the pattern in the comment block above

  Brick b(1, 1, 1, testCube->pixelType()); // create buffer of size 1 pixel

  b.SetBasePosition(2, 1, 1);
  b[0] = Isis::Null;
  testCube->write(b);

  b.SetBasePosition(4, 1, 2);
  b[0] = Isis::Null;
  testCube->write(b);

  b.SetBasePosition(1, 3, 3);
  b[0] = Isis::Null;
  testCube->write(b);

  b.SetBasePosition(3, 3, 4);
  b[0] = Isis::Null;
  testCube->write(b);

  b.SetBasePosition(5, 3, 5);
  b[0] = Isis::Null;
  testCube->write(b);

  b.SetBasePosition(2, 5, 6);
  b[0] = Isis::Null;
  testCube->write(b);

  b.SetBasePosition(4, 5, 7);
  b[0] = Isis::Null;
  testCube->write(b);
  
  // run bandtrim
  QVector<QString> args = {"to=" + tempDir.path() + "/bandtrimDefaultOut.cub"};
  UserInterface ui(APP_XML, args);

  try {
     bandtrim(testCube, ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  // Open output cube
  Cube outCube(tempDir.path() + "/bandtrimDefaultOut.cub");

  // validate histogram statistics for each band in output cube
  std::unique_ptr<Histogram> band1Hist (outCube.histogram(1));
  std::unique_ptr<Histogram> band2Hist (outCube.histogram(2));
  std::unique_ptr<Histogram> band3Hist (outCube.histogram(3));
  std::unique_ptr<Histogram> band4Hist (outCube.histogram(4));
  std::unique_ptr<Histogram> band5Hist (outCube.histogram(5));
  std::unique_ptr<Histogram> band6Hist (outCube.histogram(6));
  std::unique_ptr<Histogram> band7Hist (outCube.histogram(7));

  EXPECT_EQ(band1Hist->ValidPixels(), 18);
  EXPECT_EQ(band1Hist->Average(), 13);
  EXPECT_EQ(band1Hist->Sum(), 234);
  EXPECT_EQ(band2Hist->ValidPixels(), 18);
  EXPECT_EQ(band2Hist->Average(), 38);
  EXPECT_EQ(band2Hist->Sum(), 684);
  EXPECT_EQ(band3Hist->ValidPixels(), 18);
  EXPECT_EQ(band3Hist->Average(), 63);
  EXPECT_EQ(band3Hist->Sum(), 1134);
  EXPECT_EQ(band4Hist->ValidPixels(), 18);
  EXPECT_EQ(band4Hist->Average(), 88);
  EXPECT_EQ(band4Hist->Sum(), 1584);
  EXPECT_EQ(band5Hist->ValidPixels(), 18);
  EXPECT_EQ(band5Hist->Average(), 113);
  EXPECT_EQ(band5Hist->Sum(), 2034);
  EXPECT_EQ(band6Hist->ValidPixels(), 18);
  EXPECT_EQ(band6Hist->Average(), 138);
  EXPECT_EQ(band6Hist->Sum(), 2484);
  EXPECT_EQ(band7Hist->ValidPixels(), 18);
  EXPECT_EQ(band7Hist->Average(), 163);
  EXPECT_EQ(band7Hist->Sum(), 2934);

  outCube.close();
}


/**
   * BandtrimOneBand Test
   * 
   * BandtrimOneBand test given a single 5x5 input cube with 1 band.
   * The four pixels in the upper left corner are set to Isis::Null
   * as below. N implies a Null pixel.
   * 
   * The output cube is verified by checking histogram statistics.
   * 
   * |N|N| | | | 
   * |N|N| | | |
   * | | | | | |
   * | | | | | |
   * | | | | | |
   * 
   * INPUT: testCube from DefaultCube fixture
   * 
   * OUTPUT: bandtrimOneBandOut.cub
   */
TEST_F(DefaultCube, FunctionalTestBandtrimOneBand) {
  
  // reduce test cube size
  resizeCube(5, 5, 1);

  // set 4 pixel block in upper left corner to Isis::Null
  Brick b(1, 1, 1, testCube->pixelType()); // create buffer of size 1 pixels
  b.SetBasePosition(1, 1, 1);
  b[0] = Isis::Null;
  testCube->write(b);

  b.SetBasePosition(2, 1, 1);
  b[0] = Isis::Null;
  testCube->write(b);

  b.SetBasePosition(1, 2, 1);
  b[0] = Isis::Null;
  testCube->write(b);

  b.SetBasePosition(2, 2, 1);
  b[0] = Isis::Null;
  testCube->write(b);

  // run bandtrim
  QVector<QString> args = {"to=" + tempDir.path() + "/bandtrimOneBandOut.cub"};

  UserInterface ui(APP_XML, args);

  try {
    bandtrim(testCube, ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  // Open output cube
  Cube outCube(tempDir.path() + "/bandtrimOneBandOut.cub");

  // validate histogram statistics for each band in output cube
  std::unique_ptr<Histogram> band1Hist (outCube.histogram(1));

  EXPECT_EQ(band1Hist->ValidPixels(), 21);
  EXPECT_EQ(band1Hist->Average(), 14.714285714285714);
  EXPECT_EQ(band1Hist->Sum(), 309);

  outCube.close();
}

