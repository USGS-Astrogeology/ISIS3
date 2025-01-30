
#include <QString>
#include <QVector>

#include "CameraFixtures.h"
#include "Histogram.h"

#include "photrim.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/photrim.xml").expanded();

/**
   * FunctionalTestsPhotrimBase
   * 
   * PhotrimEmission test given a single 5x5 input cube with 1 band.
   * Default values for all parameters are used, resulting in no Null
   * pixels.
   *
   * | 1| 2| 3| 4| 5|    Valid Pixels:  25
   * | 6| 7| 8| 9|10|   Histogram Sum: 325
   * |11|12|13|14|15|   Histogram Avg:  13
   * |16|17|18|19|20|
   * |21|22|23|24|25|
   * 
   * The output cube is verified by checking histogram statistics.
   *
   *    INPUT: testCube from DefaultCube fixture resized to 5x5 pixels
   *           with one band.
               minemission=10.852
               maxemission=10.857
   * 
   * DEFAULTS: usedem=false
   *           minphase=0.0
   *           maxphase=180.0
   *           minemission=0.0
   *           maxemission=90.0
   *           minincidence=0.0
   *           maxincidence=90.0 
   * 
   *   OUTPUT: photrimEmission.cub
   */
  TEST_F(DefaultCube, FunctionalTestsPhotrimBase) {
  resizeCube(5,5,1);

  QVector<QString> args = {"to=" + tempDir.path() + "/PhotrimBase.cub"};

  UserInterface ui(APP_XML, args);

   try {
     photrim(testCube, ui);
   }
   catch(IException &e) {
     FAIL() << e.toString().toStdString().c_str() << std::endl;
   }

  // validate output cube
  Cube outCube(tempDir.path() + "/PhotrimBase.cub");

  // validate histogram statistics for output cube
  std::unique_ptr<Histogram> hist (outCube.histogram(1));
  EXPECT_EQ(hist->ValidPixels(), 25);
  EXPECT_EQ(hist->Average(), 13);
  EXPECT_EQ(hist->Sum(), 325);

  outCube.close();
}


/**
   * FunctionalTestsPhotrimEmission
   * 
   * PhotrimEmission test given a single 5x5 input cube with 1 band.
   * All pixels outside the range as defined by minemission and
   * maxemission are set to Null. Resulting pixel values are as
   * shown below.
   *
   * | N| N| 3| 4| 5|    Valid Pixels:  14
   * | 6| 7| 8| 9|10|   Histogram Sum: 135
   * |11|12|13|14| N|   Histogram Avg: 9.6428571428571423
   * |16|17| N| N| N|
   * | N| N| N| N| N|
   * 
   * The output cube is verified by checking histogram statistics.
   *
   *    INPUT: testCube from DefaultCube fixture resized to 5x5 pixels
   *           with one band.
               minemission=10.852
               maxemission=10.857
   * 
   * DEFAULTS: usedem=false
   *           minphase=0.0
   *           maxphase=180.0
   *           minemission=0.0
   *           maxemission=90.0
   *           minincidence=0.0
   *           maxincidence=90.0 
   * 
   *   OUTPUT: photrimEmission.cub
   */
TEST_F(DefaultCube, FunctionalTestsPhotrimEmission) {
  resizeCube(5,5,1);

  QVector<QString> args = {"to=" + tempDir.path() + "/photrimEmission.cub",
                          "minemission=10.852",
                          "maxemission=10.857"
                          };

  UserInterface ui(APP_XML, args);

   try {
     photrim(testCube, ui);
   }
   catch(IException &e) {
     FAIL() << e.toString().toStdString().c_str() << std::endl;
   }

  // validate output cube
  Cube outCube(tempDir.path() + "/photrimEmission.cub");

  // validate histogram statistics for output cube
  std::unique_ptr<Histogram> hist (outCube.histogram(1));
  EXPECT_EQ(hist->ValidPixels(), 14);
  EXPECT_NEAR(hist->Average(), 9.642857, 0.000001);
  EXPECT_EQ(hist->Sum(), 135);
  
  outCube.close();
}


/**
   * FunctionalTestsPhotrimPhase
   * 
   * PhotrimPhase test given a single 5x5 input cube with 1 band.
   * All pixels outside the range as defined by minphase and
   * maxphase are set to Null. Resulting pixel values are as
   * shown below.
   *
   * | N| N| N| N| N|    Valid Pixels:   8
   * | N| N| N| N| N|   Histogram Sum: 122
   * |11|12|13|14|15|   Histogram Avg:  15.25
   * | N| N|18|19|20|
   * | N| N| N| N| N|
   * 
   * The output cube is verified by checking histogram statistics.
   *
   *    INPUT: testCube from DefaultCube fixture resized to 5x5 pixels
   *           with one band.
   *           minphase=79.77
   *           maxphase=79.772
   * 
   * DEFAULTS: usedem=false
   *           minphase=0.0
   *           maxphase=180.0
   *           minemission=0.0
   *           maxemission=90.0
   *           minincidence=0.0
   *           maxincidence=90.0 
   * 
   *   OUTPUT: photrimPhase.cub
   */
TEST_F(DefaultCube, FunctionalTestsPhotrimPhase) {
  resizeCube(5,5,1);

  QVector<QString> args = {"to=" + tempDir.path() + "/photrimPhase.cub",
                          "minphase=79.77",
                          "maxphase=79.772"
                          };

  UserInterface ui(APP_XML, args);

   try {
     photrim(testCube, ui);
   }
   catch(IException &e) {
     FAIL() << e.toString().toStdString().c_str() << std::endl;
   }

  // validate output cube
  Cube outCube(tempDir.path() + "/photrimPhase.cub");

  // validate histogram statistics for output cube
  std::unique_ptr<Histogram> hist (outCube.histogram(1));
  EXPECT_EQ(hist->ValidPixels(), 8);
  EXPECT_EQ(hist->Average(), 15.25);
  EXPECT_EQ(hist->Sum(), 122);

  outCube.close();
}


/**
   * FunctionalTestsPhotrimIncidence
   * 
   * PhotrimIncidence test given a single 5x5 input cube with 1 band.
   * All pixels outside the range as defined by minincidence and
   * maxincidence are set to Null. Resulting pixel values are as
   * shown below.
   *
   * | N| N| N| N| N|    Valid Pixels:   5
   * | N| N| N| N| N|   Histogram Sum:  65
   * |11|12|13|14|15|   Histogram Avg:  13
   * | N| N| N| N| N|
   * | N| N| N| N| N|
   * 
   * The output cube is verified by checking histogram statistics.
   *
   *    INPUT: testCube from DefaultCube fixture resized to 5x5 pixels
   *           with one band.
   *           minincidence=70.248
   *           maxincidence=70.2485
   * 
   * DEFAULTS: usedem=false
   *           minphase=0.0
   *           maxphase=180.0
   *           minemission=0.0
   *           maxemission=90.0
   *           minincidence=0.0
   *           maxincidence=90.0 
   * 
   *   OUTPUT: photrimIncidence.cub
   */
TEST_F(DefaultCube, FunctionalTestsPhotrimIncidence) {
  resizeCube(5,5,1);

  QVector<QString> args = {"to=" + tempDir.path() + "/photrimIncidence.cub",
                          "minincidence=70.248",
                          "maxincidence=70.2485"
                          };

  UserInterface ui(APP_XML, args);

   try {
     photrim(testCube, ui);
   }
   catch(IException &e) {
     FAIL() << e.toString().toStdString().c_str() << std::endl;
   }

  // validate output cube
  Cube outCube(tempDir.path() + "/photrimIncidence.cub");

  // validate histogram statistics for output cube
  std::unique_ptr<Histogram> hist (outCube.histogram(1));
  EXPECT_EQ(hist->ValidPixels(), 5);
  EXPECT_EQ(hist->Average(), 13);
  EXPECT_EQ(hist->Sum(), 65);
}


/**
   * FunctionalTestsPhotrimUseDEM
   * 
   * PhotrimIncidence test given a single 5x5 input cube with 1 band.
   * The shapemodel set in cube is utilized. All pixels outside the
   * range as defined by minemission and maxemission are set to Null.
   * Resulting pixel values are as shown below.
   *
   * | N| N| 3| 4| 5|    Valid Pixels:  17
   * | 6| 7| 8| 9|10|   Histogram Sum: 189
   * |11|12|13|14|15|   Histogram Avg:  11.117647058823529
   * |16|17|18| N| N|
   * |21| N| N| N| N|
   * 
   * The output cube is verified by checking histogram statistics.
   *
   *    INPUT: testCube from DefaultCube fixture resized to 5x5 pixels
   *           with one band.
   *           usedem=true
   *           minemission=10.8
   *           maxemission=10.805
   * 
   * DEFAULTS: usedem=false
   *           minphase=0.0
   *           maxphase=180.0
   *           minemission=0.0
   *           maxemission=90.0
   *           minincidence=0.0
   *           maxincidence=90.0 
   * 
   *   OUTPUT: photrimIncidence.cub
   */
TEST_F(DefaultCube, FunctionalTestsPhotrimUseDEM) {
  resizeCube(5,5,1);

  QVector<QString> args = {"to=" + tempDir.path() + "/photrimUseDEM.cub",
                           "usedem=true",
                           "minemission=10.8",
                           "maxemission=10.805",
                          };

  UserInterface ui(APP_XML, args);

  try {
    photrim(testCube, ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  // validate output cube
  Cube outCube(tempDir.path() + "/photrimUseDEM.cub");

  // validate histogram statistics for output cube
  std::unique_ptr<Histogram> hist (outCube.histogram(1));
  EXPECT_EQ(hist->ValidPixels(),17);
  EXPECT_NEAR(hist->Average(), 11.117647, 0.000001);
  EXPECT_EQ(hist->Sum(), 189);

  outCube.close();
}
