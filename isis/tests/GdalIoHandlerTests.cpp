#include <QTemporaryFile>
#include <QString>
#include <iostream>
#include <iomanip>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "Brick.h"
#include "PixelType.h"
#include "GdalIoHandler.h"
#include "CubeTileHandler.h"
#include "SpecialPixel.h"
#include "TiffFixtures.h"

#include "TestUtilities.h"

#include "gmock/gmock.h"

using namespace Isis;

TEST(GdalIoTests, DefaulWrite) {
  QString filePath = "/Users/acpaquette/repos/ISIS3/build/gdal_files/B10_013341_1010_XN_79S172W.tiff";
  
  {
    const QList<int> *bandList = new QList<int>;

    GdalIoHandler handler(filePath, bandList);
    Brick localBrick(200, 200, 1, SignedWord);

    localBrick.SetBasePosition(1, 1, 1);
    // init everything to 1
    for (int i = 0; i < localBrick.size(); i++) {
      localBrick[i] = 100;
    }

    localBrick.SetBasePosition(1, 1, 1);
    handler.write(localBrick);
  } // file is closed  

  // read it back 
  GDALDatasetUniquePtr ds = GDALDatasetUniquePtr(GDALDataset::FromHandle(GDALOpen(filePath.toStdString().c_str(), GA_ReadOnly)));
  double* dbuf = (double *) CPLMalloc(sizeof(double)*200*200); 
  GDALRasterBand  *poBand = ds->GetRasterBand(1);
  CPLErr err = poBand->RasterIO(GF_Read, 0, 0, 
                                         200, 200,
                                         dbuf, 
                                         200, 200, 
                                         GDT_Float64,
                                         0, 0);

  
  for (int i = 0; i<200*200; i++) { 
    ASSERT_EQ(dbuf[i], 100);
  }

}

TEST_F(ReadWriteTiff, GdalIoTestsReadFloat64) {
  PixelType isisPixelType = Double;
  writeTiff(isisPixelType);

  const QList<int> *bandList = new QList<int>;
  GdalIoHandler handler(path, bandList, IsisPixelToGdal(isisPixelType));
  handler.read(*localBrick);

  double *brickDoubleBuff = localBrick->DoubleBuffer();
  EXPECT_EQ(brickDoubleBuff[0], HIGH_INSTR_SAT8);
  EXPECT_EQ(brickDoubleBuff[1], HIGH_REPR_SAT8);
  EXPECT_EQ(brickDoubleBuff[2], LOW_INSTR_SAT8);
  EXPECT_EQ(brickDoubleBuff[3], LOW_REPR_SAT8);
  EXPECT_EQ(brickDoubleBuff[4], NULL8);
  EXPECT_EQ(brickDoubleBuff[5], 1000.0);
}

TEST_F(ReadWriteTiff, GdalIoTestsReadFloat32) {
  PixelType isisPixelType = Real;
  writeTiff(isisPixelType);

  const QList<int> *bandList = new QList<int>;
  GdalIoHandler handler(path, bandList, IsisPixelToGdal(isisPixelType));
  handler.read(*localBrick);

  double *brickDoubleBuff = localBrick->DoubleBuffer();
  EXPECT_EQ(brickDoubleBuff[0], HIGH_INSTR_SAT8);
  EXPECT_EQ(brickDoubleBuff[1], HIGH_REPR_SAT8);
  EXPECT_EQ(brickDoubleBuff[2], LOW_INSTR_SAT8);
  EXPECT_EQ(brickDoubleBuff[3], LOW_REPR_SAT8);
  EXPECT_EQ(brickDoubleBuff[4], NULL8);
  EXPECT_EQ(brickDoubleBuff[5], 1000.0);
}

TEST_F(ReadWriteTiff, GdalIoTestsReadInt32) {
  PixelType isisPixelType = SignedInteger;
  writeTiff(isisPixelType);

  const QList<int> *bandList = new QList<int>;
  GdalIoHandler handler(path, bandList, IsisPixelToGdal(isisPixelType));
  handler.read(*localBrick);

  double *brickDoubleBuff = localBrick->DoubleBuffer();
  EXPECT_EQ(brickDoubleBuff[0], HIGH_INSTR_SAT8);
  EXPECT_EQ(brickDoubleBuff[1], HIGH_REPR_SAT8);
  EXPECT_EQ(brickDoubleBuff[2], LOW_INSTR_SAT8);
  EXPECT_EQ(brickDoubleBuff[3], LOW_REPR_SAT8);
  EXPECT_EQ(brickDoubleBuff[4], NULL8);
  EXPECT_EQ(brickDoubleBuff[5], 1000.0);
}

TEST_F(ReadWriteTiff, GdalIoTestsReadUInt32) {
  PixelType isisPixelType = UnsignedInteger;
  writeTiff(isisPixelType);

  const QList<int> *bandList = new QList<int>;
  GdalIoHandler handler(path, bandList, IsisPixelToGdal(isisPixelType));
  handler.read(*localBrick);

  double *brickDoubleBuff = localBrick->DoubleBuffer();
  EXPECT_EQ(brickDoubleBuff[0], HIGH_INSTR_SAT8);
  EXPECT_EQ(brickDoubleBuff[1], HIGH_REPR_SAT8);
  EXPECT_EQ(brickDoubleBuff[2], LOW_INSTR_SAT8);
  EXPECT_EQ(brickDoubleBuff[3], LOW_REPR_SAT8);
  EXPECT_EQ(brickDoubleBuff[4], NULL8);
  EXPECT_EQ(brickDoubleBuff[5], 1000.0);
}

TEST_F(ReadWriteTiff, GdalIoTestsReadInt16) {
  PixelType isisPixelType = SignedWord;
  writeTiff(isisPixelType);

  const QList<int> *bandList = new QList<int>;
  GdalIoHandler handler(path, bandList, IsisPixelToGdal(isisPixelType));
  handler.read(*localBrick);

  double *brickDoubleBuff = localBrick->DoubleBuffer();
  EXPECT_EQ(brickDoubleBuff[0], HIGH_INSTR_SAT8);
  EXPECT_EQ(brickDoubleBuff[1], HIGH_REPR_SAT8);
  EXPECT_EQ(brickDoubleBuff[2], LOW_INSTR_SAT8);
  EXPECT_EQ(brickDoubleBuff[3], LOW_REPR_SAT8);
  EXPECT_EQ(brickDoubleBuff[4], NULL8);
  EXPECT_EQ(brickDoubleBuff[5], 1000.0);
}

TEST_F(ReadWriteTiff, GdalIoTestsReadUInt16) {
  PixelType isisPixelType = UnsignedWord;
  writeTiff(isisPixelType);
 
  const QList<int> *bandList = new QList<int>;
  GdalIoHandler handler(path, bandList, IsisPixelToGdal(isisPixelType));
  handler.read(*localBrick);

  double *brickDoubleBuff = localBrick->DoubleBuffer();
  EXPECT_EQ(brickDoubleBuff[0], HIGH_INSTR_SAT8);
  EXPECT_EQ(brickDoubleBuff[1], HIGH_REPR_SAT8);
  EXPECT_EQ(brickDoubleBuff[2], LOW_INSTR_SAT8);
  EXPECT_EQ(brickDoubleBuff[3], LOW_REPR_SAT8);
  EXPECT_EQ(brickDoubleBuff[4], NULL8);
  EXPECT_EQ(brickDoubleBuff[5], 1000.0);
}

TEST_F(ReadWriteTiff, GdalIoTestsReadInt8) {
  PixelType isisPixelType = SignedByte;
  writeTiff(isisPixelType);

  const QList<int> *bandList = new QList<int>;
  GdalIoHandler handler(path, bandList, IsisPixelToGdal(isisPixelType));
  handler.read(*localBrick);

  double *brickDoubleBuff = localBrick->DoubleBuffer();
  EXPECT_EQ(brickDoubleBuff[0], HIGH_REPR_SAT8);
  EXPECT_EQ(brickDoubleBuff[1], HIGH_REPR_SAT8);
  EXPECT_EQ(brickDoubleBuff[2], NULL8);
  EXPECT_EQ(brickDoubleBuff[3], NULL8);
  EXPECT_EQ(brickDoubleBuff[4], NULL8);
  EXPECT_EQ(brickDoubleBuff[5], 50);
}

TEST_F(ReadWriteTiff, GdalIoTestsReadByte) {
  PixelType isisPixelType = UnsignedByte;
  writeTiff(isisPixelType);

  const QList<int> *bandList = new QList<int>;
  GdalIoHandler handler(path, bandList, IsisPixelToGdal(isisPixelType));
  handler.read(*localBrick);

  double *brickDoubleBuff = localBrick->DoubleBuffer();
  EXPECT_EQ(brickDoubleBuff[0], HIGH_REPR_SAT8);
  EXPECT_EQ(brickDoubleBuff[1], HIGH_REPR_SAT8);
  EXPECT_EQ(brickDoubleBuff[2], NULL8);
  EXPECT_EQ(brickDoubleBuff[3], NULL8);
  EXPECT_EQ(brickDoubleBuff[4], NULL8);
  EXPECT_EQ(brickDoubleBuff[5], 50);
}