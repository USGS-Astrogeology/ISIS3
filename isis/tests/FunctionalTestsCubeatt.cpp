#include <iostream>

#include "Cube.h"
#include "CubeFixtures.h"
#include "Statistics.h"

#include "cubeatt.h"
#include "gmock/gmock.h"


using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/cubeatt.xml").expanded();

// Tests setting output attributes: bit type and range
TEST_F(SmallCube, FunctionalTestCubeattBitttypeAndRange) {
  QString cubePath = tempDir.path() + "/bitTypeCubeatt.cub+8bit+0.0:1.0";

  QVector<QString> args = {"from=" + testCube->fileName(), "to=" + cubePath};
  UserInterface options(APP_XML, args);
  cubeatt(options);

  Cube outputCube(cubePath);

  // Check attributes: pixel type, storage format, label format, storage order, pixel range, bands
  EXPECT_EQ(outputCube.pixelType(), PixelType::UnsignedByte);
  // Setting the pixel range modifies the base/multiplier, so check those.
  EXPECT_NE(outputCube.base(), 0);
  EXPECT_NE(outputCube.multiplier(), 1);

  // Test the DNs
  Statistics *outputStats = outputCube.statistics();
  EXPECT_FLOAT_EQ(outputStats->Minimum(), 0.0);
  EXPECT_FLOAT_EQ(outputStats->Maximum(), 1.0);
}


TEST_F(SmallCube, FunctionalTestCubeattNoChange) {
  QString cubePath = tempDir.path() + "/NoChangeCubeatt.cub";
  QVector<QString> args = {"from=" + testCube->fileName(), "to=" + cubePath};
  UserInterface options(APP_XML, args);
  cubeatt(options);

  Cube outputCube(cubePath);

  // Check attributes: pixel type, storage format, label format, storage order, pixel range, bands
  EXPECT_EQ(outputCube.pixelType(), PixelType::Real);
  EXPECT_EQ(outputCube.format(), Cube::Format::Tile);
  EXPECT_EQ(outputCube.labelsAttached(), Cube::AttachedLabel);
  EXPECT_EQ(outputCube.byteOrder(), ByteOrder::Lsb);
  // Setting the pixel range modifies the base/multiplier, so check those.
  EXPECT_EQ(outputCube.base(), 0);
  EXPECT_EQ(outputCube.multiplier(), 1);
  EXPECT_EQ(outputCube.bandCount(), 10);

  // Test that DNs match in the input and output cubes
  Statistics *outputStats = outputCube.statistics();
  Statistics *inputStats = testCube->statistics();
  EXPECT_DOUBLE_EQ(outputStats->Minimum(), inputStats->Minimum());
  EXPECT_DOUBLE_EQ(outputStats->Maximum(), inputStats->Maximum());
  EXPECT_DOUBLE_EQ(outputStats->Average(), inputStats->Average());
}


TEST_F(SmallCube, FunctionalTestCubeattVirtualBands) {
  QString cubePath = tempDir.path() + "/VirtualBandsCubeatt.cub";
  QVector<QString> args = {"from=" + testCube->fileName() + "+3,2,4,2,1,5,7,6,4", "to=" + cubePath};
  UserInterface options(APP_XML, args);
  cubeatt(options);
  Cube outputCube(cubePath);
  EXPECT_EQ(outputCube.bandCount(), 9);

  // Do need to check the label for this one, since outputCube.physicalBand() will not work
  // in this context:
  Pvl *label = outputCube.label();
  PvlGroup bandBin = label->findObject("IsisCube").findGroup("BandBin");
  EXPECT_EQ(QString(bandBin["OriginalBand"][0]), "3");
  EXPECT_EQ(QString(bandBin["OriginalBand"][1]), "2");
  EXPECT_EQ(QString(bandBin["OriginalBand"][2]), "4");
  EXPECT_EQ(QString(bandBin["OriginalBand"][3]), "2");
  EXPECT_EQ(QString(bandBin["OriginalBand"][4]), "1");
  EXPECT_EQ(QString(bandBin["OriginalBand"][5]), "5");
  EXPECT_EQ(QString(bandBin["OriginalBand"][6]), "7");
  EXPECT_EQ(QString(bandBin["OriginalBand"][7]), "6");
  EXPECT_EQ(QString(bandBin["OriginalBand"][8]), "4");


  // Test that DNs for each band have moved appropriately
  EXPECT_DOUBLE_EQ(outputCube.statistics(1)->Average(), testCube->statistics(3)->Average());
  EXPECT_DOUBLE_EQ(outputCube.statistics(2)->Average(), testCube->statistics(2)->Average());
  EXPECT_DOUBLE_EQ(outputCube.statistics(3)->Average(), testCube->statistics(4)->Average());
  EXPECT_DOUBLE_EQ(outputCube.statistics(4)->Average(), testCube->statistics(2)->Average());
  EXPECT_DOUBLE_EQ(outputCube.statistics(5)->Average(), testCube->statistics(1)->Average());
  EXPECT_DOUBLE_EQ(outputCube.statistics(6)->Average(), testCube->statistics(5)->Average());
  EXPECT_DOUBLE_EQ(outputCube.statistics(7)->Average(), testCube->statistics(7)->Average());
  EXPECT_DOUBLE_EQ(outputCube.statistics(8)->Average(), testCube->statistics(6)->Average());
  EXPECT_DOUBLE_EQ(outputCube.statistics(9)->Average(), testCube->statistics(4)->Average());
}


// Test using an already open cube as input and ui
TEST_F(SmallCube, FunctionalTestCubeattInputCube) {
  QString outputCubePath = tempDir.path() + "/bitTypeCubeatt.cub+8bit+0.0:1.0";
  QVector<QString> args = {"from=" + testCube->fileName(), "to=" + outputCubePath};
  UserInterface options(APP_XML, args);

  cubeatt(testCube, options);
  Cube outputCube(outputCubePath);

  EXPECT_EQ(outputCube.pixelType(), PixelType::UnsignedByte);
  // Setting the pixel range modifies the base/multiplier, so check those.
  EXPECT_NE(outputCube.base(), 0);
  EXPECT_NE(outputCube.multiplier(), 1);

  // Test the DNs
  Statistics *outputStats = outputCube.statistics();
  EXPECT_FLOAT_EQ(outputStats->Minimum(), 0.0);
  EXPECT_FLOAT_EQ(outputStats->Maximum(), 1.0);
}


// Test using an already open cube as input and specifying an output path and output attributes
TEST_F(SmallCube, FunctionalTestCubeattInputCubeOutputPath) {
  QString outputCubePath = tempDir.path() + "/bitTypeCubeatt.cub";
  CubeAttributeOutput attributeOutput("+8bit+0.0:1.0");

  cubeatt(testCube, outputCubePath, attributeOutput);
  Cube outputCube(outputCubePath);

  // Setting the pixel range modifies the base/multiplier, so check those.
  EXPECT_NE(outputCube.base(), 0);
  EXPECT_NE(outputCube.multiplier(), 1);

  // Test the DNs
  Statistics *outputStats = outputCube.statistics();
  EXPECT_FLOAT_EQ(outputStats->Minimum(), 0.0);
  EXPECT_FLOAT_EQ(outputStats->Maximum(), 1.0);
}

// Test using the input/output paths and cube attribute input and output passed in directly
TEST_F(SmallCube, FunctionalTestCubeattInputAndOutputAttributes) {
  QString inputCubePath = testCube->fileName();
  CubeAttributeInput attributeInput("+3,2,4");
  QString outputCubePath = tempDir.path() + "/bitTypeAndVirtualBandsCubeatt.cub";
  CubeAttributeOutput attributeOutput("+200:300");

  cubeatt(inputCubePath, attributeInput, outputCubePath, attributeOutput);

  Cube outputCube(outputCubePath);

  Statistics *outputStats = outputCube.statistics();
  EXPECT_GE(outputStats->Minimum(), 200);
  EXPECT_LE(outputStats->Maximum(), 300);
  EXPECT_EQ(outputCube.bandCount(), 3);

  Pvl *label = outputCube.label();
  PvlGroup bandBin = label->findObject("IsisCube").findGroup("BandBin");
  EXPECT_EQ(QString(bandBin["OriginalBand"][0]), "3");
  EXPECT_EQ(QString(bandBin["OriginalBand"][1]), "2");
  EXPECT_EQ(QString(bandBin["OriginalBand"][2]), "4");

  // Test that DNs for each band have moved appropriately
  EXPECT_DOUBLE_EQ(outputCube.statistics(1)->Average(), testCube->statistics(3)->Average());
  EXPECT_DOUBLE_EQ(outputCube.statistics(2)->Average(), testCube->statistics(2)->Average());
  EXPECT_DOUBLE_EQ(outputCube.statistics(3)->Average(), testCube->statistics(4)->Average());
}
