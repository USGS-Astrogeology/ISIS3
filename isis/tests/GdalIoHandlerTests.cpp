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

TEST_F(ReadWriteTiff, GdalIoTestsDefaulWrite) {
  PixelType isisPixelType = Double;
  createTiff(isisPixelType, false);

  {
    const QList<int> *bandList = new QList<int>;

    GdalIoHandler handler(path, bandList);
    Brick localBrick(6, 1, 1, isisPixelType);

    localBrick.SetBasePosition(1, 1, 1);
    // init everything to 100
    for (int i = 0; i < localBrick.size(); i++) {
      localBrick[i] = 100;
    }

    handler.write(localBrick);
  } // file is closed  

  // read it back 
  dataset = GDALDataset::FromHandle(GDALOpen(path.toStdString().c_str(), GA_ReadOnly));
  dbuf = (double *) CPLMalloc(sizeof(double)*6*1);
  GDALRasterBand *poBand = dataset->GetRasterBand(1);
  CPLErr err = poBand->RasterIO(GF_Read, 0, 0, 
                                         6, 1,
                                         dbuf, 
                                         6, 1, 
                                         IsisPixelToGdal(isisPixelType),
                                         0, 0);

  
  for (int i = 0; i<6*1; i++) { 
    ASSERT_EQ(((double *) dbuf)[i], 100);
  }

}

TEST_F(ReadWriteTiff, GdalIoTestsReadFloat64) {
  PixelType isisPixelType = Double;
  createTiff(isisPixelType);

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
  createTiff(isisPixelType);

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
  createTiff(isisPixelType);

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
  createTiff(isisPixelType);

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
  createTiff(isisPixelType);

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
  createTiff(isisPixelType);
 
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
  createTiff(isisPixelType);

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

TEST_F(ReadWriteTiff, GdalIoTestsReadUInt8) {
  PixelType isisPixelType = UnsignedByte;
  createTiff(isisPixelType);

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

TEST_F(ReadWriteTiff, GdalIoTestsWriteFloat64) {
  PixelType isisPixelType = Double;
  createTiff(isisPixelType, false);
  // Create a context so the handler goes out of scope and closes the file
  {
    const QList<int> *bandList = new QList<int>;
    GdalIoHandler handler(path, bandList, IsisPixelToGdal(isisPixelType));

    localBrick = new Brick(6, 1, 1, isisPixelType);
    localBrick->SetBasePosition(1, 1, 1);

    double *brickDoubleBuff = localBrick->DoubleBuffer();

    brickDoubleBuff[0] = HIGH_INSTR_SAT8;
    brickDoubleBuff[1] = HIGH_REPR_SAT8;
    brickDoubleBuff[2] = LOW_INSTR_SAT8;
    brickDoubleBuff[3] = LOW_REPR_SAT8;
    brickDoubleBuff[4] = NULL8;
    brickDoubleBuff[5] = 1000;

    handler.write(*localBrick);
  }

  dataset = GDALDataset::FromHandle(GDALOpen(path.toStdString().c_str(), GA_ReadOnly));
  dbuf = (double *)CPLMalloc(sizeof(double) * 6);
  GDALRasterBand *poBand = dataset->GetRasterBand(1);
  CPLErr err = poBand->RasterIO(GF_Read, 0, 0,
                                         6, 1,
                                         dbuf,
                                         6, 1,
                                         IsisPixelToGdal(isisPixelType),
                                         0, 0);
  EXPECT_EQ(((double *)dbuf)[0], HIGH_INSTR_SAT8);
  EXPECT_EQ(((double *)dbuf)[1], HIGH_REPR_SAT8);
  EXPECT_EQ(((double *)dbuf)[2], LOW_INSTR_SAT8);
  EXPECT_EQ(((double *)dbuf)[3], LOW_REPR_SAT8);
  EXPECT_EQ(((double *)dbuf)[4], NULL8);
  EXPECT_EQ(((double *)dbuf)[5], 1000);
}

TEST_F(ReadWriteTiff, GdalIoTestsWriteFloat32) {
  PixelType isisPixelType = Real;
  createTiff(isisPixelType, false);
  // Create a context so the handler goes out of scope and closes the file
  {
    const QList<int> *bandList = new QList<int>;
    GdalIoHandler handler(path, bandList, IsisPixelToGdal(isisPixelType));

    localBrick = new Brick(6, 1, 1, isisPixelType);
    localBrick->SetBasePosition(1, 1, 1);

    double *brickDoubleBuff = localBrick->DoubleBuffer();

    brickDoubleBuff[0] = HIGH_INSTR_SAT8;
    brickDoubleBuff[1] = HIGH_REPR_SAT8;
    brickDoubleBuff[2] = LOW_INSTR_SAT8;
    brickDoubleBuff[3] = LOW_REPR_SAT8;
    brickDoubleBuff[4] = NULL8;
    brickDoubleBuff[5] = 1000;

    handler.write(*localBrick);
  }

  dataset = GDALDataset::FromHandle(GDALOpen(path.toStdString().c_str(), GA_ReadOnly));
  dbuf = (float *)CPLMalloc(sizeof(float) * 6);
  GDALRasterBand *poBand = dataset->GetRasterBand(1);
  CPLErr err = poBand->RasterIO(GF_Read, 0, 0,
                                         6, 1,
                                         dbuf,
                                         6, 1,
                                         IsisPixelToGdal(isisPixelType),
                                         0, 0);
  EXPECT_EQ(((float *)dbuf)[0], HIGH_INSTR_SAT4);
  EXPECT_EQ(((float *)dbuf)[1], HIGH_REPR_SAT4);
  EXPECT_EQ(((float *)dbuf)[2], LOW_INSTR_SAT4);
  EXPECT_EQ(((float *)dbuf)[3], LOW_REPR_SAT4);
  EXPECT_EQ(((float *)dbuf)[4], NULL4);
  EXPECT_EQ(((float *)dbuf)[5], 1000);
}

TEST_F(ReadWriteTiff, GdalIoTestsWriteInt32) {
  PixelType isisPixelType = SignedInteger;
  createTiff(isisPixelType, false);
  // Create a context so the handler goes out of scope and closes the file
  {
    const QList<int> *bandList = new QList<int>;
    GdalIoHandler handler(path, bandList, IsisPixelToGdal(isisPixelType));

    localBrick = new Brick(6, 1, 1, isisPixelType);
    localBrick->SetBasePosition(1, 1, 1);

    double *brickDoubleBuff = localBrick->DoubleBuffer();

    brickDoubleBuff[0] = HIGH_INSTR_SAT8;
    brickDoubleBuff[1] = HIGH_REPR_SAT8;
    brickDoubleBuff[2] = LOW_INSTR_SAT8;
    brickDoubleBuff[3] = LOW_REPR_SAT8;
    brickDoubleBuff[4] = NULL8;
    brickDoubleBuff[5] = 1000;

    handler.write(*localBrick);
  }

  dataset = GDALDataset::FromHandle(GDALOpen(path.toStdString().c_str(), GA_ReadOnly));
  dbuf = (unsigned int *)CPLMalloc(sizeof(unsigned int) * 6);
  GDALRasterBand *poBand = dataset->GetRasterBand(1);
  CPLErr err = poBand->RasterIO(GF_Read, 0, 0,
                                6, 1,
                                dbuf,
                                6, 1,
                                IsisPixelToGdal(isisPixelType),
                                0, 0);
  EXPECT_EQ(((unsigned int *)dbuf)[0], HIGH_INSTR_SATI4);
  EXPECT_EQ(((unsigned int *)dbuf)[1], HIGH_REPR_SATI4);
  EXPECT_EQ(((unsigned int *)dbuf)[2], LOW_INSTR_SATI4);
  EXPECT_EQ(((unsigned int *)dbuf)[3], LOW_REPR_SATI4);
  EXPECT_EQ(((unsigned int *)dbuf)[4], NULLI4);
  EXPECT_EQ(((unsigned int *)dbuf)[5], 1000);
}

TEST_F(ReadWriteTiff, GdalIoTestsWriteUInt32) {
  PixelType isisPixelType = UnsignedInteger;
  createTiff(isisPixelType, false);
  // Create a context so the handler goes out of scope and closes the file
  {
    const QList<int> *bandList = new QList<int>;
    GdalIoHandler handler(path, bandList, IsisPixelToGdal(isisPixelType));

    localBrick = new Brick(6, 1, 1, isisPixelType);
    localBrick->SetBasePosition(1, 1, 1);

    double *brickDoubleBuff = localBrick->DoubleBuffer();

    brickDoubleBuff[0] = HIGH_INSTR_SAT8;
    brickDoubleBuff[1] = HIGH_REPR_SAT8;
    brickDoubleBuff[2] = LOW_INSTR_SAT8;
    brickDoubleBuff[3] = LOW_REPR_SAT8;
    brickDoubleBuff[4] = NULL8;
    brickDoubleBuff[5] = 1000;

    handler.write(*localBrick);
  }

  dataset = GDALDataset::FromHandle(GDALOpen(path.toStdString().c_str(), GA_ReadOnly));
  dbuf = (unsigned int *)CPLMalloc(sizeof(unsigned int) * 6);
  GDALRasterBand *poBand = dataset->GetRasterBand(1);
  CPLErr err = poBand->RasterIO(GF_Read, 0, 0,
                                         6, 1,
                                         dbuf,
                                         6, 1,
                                         IsisPixelToGdal(isisPixelType),
                                         0, 0);
  EXPECT_EQ(((unsigned int *)dbuf)[0], HIGH_INSTR_SATUI4);
  EXPECT_EQ(((unsigned int *)dbuf)[1], HIGH_REPR_SATUI4);
  EXPECT_EQ(((unsigned int *)dbuf)[2], LOW_INSTR_SATUI4);
  EXPECT_EQ(((unsigned int *)dbuf)[3], LOW_REPR_SATUI4);
  EXPECT_EQ(((unsigned int *)dbuf)[4], NULLUI4);
  EXPECT_EQ(((unsigned int *)dbuf)[5], 1000);
}

TEST_F(ReadWriteTiff, GdalIoTestsWriteInt16) {
  PixelType isisPixelType = SignedWord;
  createTiff(isisPixelType, false);
  // Create a context so the handler goes out of scope and closes the file
  {
    const QList<int> *bandList = new QList<int>;
    GdalIoHandler handler(path, bandList, IsisPixelToGdal(isisPixelType));

    localBrick = new Brick(6, 1, 1, isisPixelType);
    localBrick->SetBasePosition(1, 1, 1);

    double *brickDoubleBuff = localBrick->DoubleBuffer();

    brickDoubleBuff[0] = HIGH_INSTR_SAT8;
    brickDoubleBuff[1] = HIGH_REPR_SAT8;
    brickDoubleBuff[2] = LOW_INSTR_SAT8;
    brickDoubleBuff[3] = LOW_REPR_SAT8;
    brickDoubleBuff[4] = NULL8;
    brickDoubleBuff[5] = 1000;

    handler.write(*localBrick);
  }

  dataset = GDALDataset::FromHandle(GDALOpen(path.toStdString().c_str(), GA_ReadOnly));
  dbuf = (short *)CPLMalloc(sizeof(short) * 6);
  GDALRasterBand *poBand = dataset->GetRasterBand(1);
  CPLErr err = poBand->RasterIO(GF_Read, 0, 0,
                                         6, 1,
                                         dbuf,
                                         6, 1,
                                         IsisPixelToGdal(isisPixelType),
                                         0, 0);
  EXPECT_EQ(((short *)dbuf)[0], HIGH_INSTR_SAT2);
  EXPECT_EQ(((short *)dbuf)[1], HIGH_REPR_SAT2);
  EXPECT_EQ(((short *)dbuf)[2], LOW_INSTR_SAT2);
  EXPECT_EQ(((short *)dbuf)[3], LOW_REPR_SAT2);
  EXPECT_EQ(((short *)dbuf)[4], NULL2);
  EXPECT_EQ(((short *)dbuf)[5], 1000);
}

TEST_F(ReadWriteTiff, GdalIoTestsWriteUInt16) {
  PixelType isisPixelType = UnsignedWord;
  createTiff(isisPixelType, false);
  // Create a context so the handler goes out of scope and closes the file
  {
    const QList<int> *bandList = new QList<int>;
    GdalIoHandler handler(path, bandList, IsisPixelToGdal(isisPixelType));

    localBrick = new Brick(6, 1, 1, isisPixelType);
    localBrick->SetBasePosition(1, 1, 1);

    double *brickDoubleBuff = localBrick->DoubleBuffer();

    brickDoubleBuff[0] = HIGH_INSTR_SAT8;
    brickDoubleBuff[1] = HIGH_REPR_SAT8;
    brickDoubleBuff[2] = LOW_INSTR_SAT8;
    brickDoubleBuff[3] = LOW_REPR_SAT8;
    brickDoubleBuff[4] = NULL8;
    brickDoubleBuff[5] = 1000;

    handler.write(*localBrick);
  }

  dataset = GDALDataset::FromHandle(GDALOpen(path.toStdString().c_str(), GA_ReadOnly));
  dbuf = (unsigned short *)CPLMalloc(sizeof(unsigned short) * 6);
  GDALRasterBand *poBand = dataset->GetRasterBand(1);
  CPLErr err = poBand->RasterIO(GF_Read, 0, 0,
                                         6, 1,
                                         dbuf,
                                         6, 1,
                                         IsisPixelToGdal(isisPixelType),
                                         0, 0);
  EXPECT_EQ(((unsigned short *)dbuf)[0], HIGH_INSTR_SATU2);
  EXPECT_EQ(((unsigned short *)dbuf)[1], HIGH_REPR_SATU2);
  EXPECT_EQ(((unsigned short *)dbuf)[2], LOW_INSTR_SATU2);
  EXPECT_EQ(((unsigned short *)dbuf)[3], LOW_REPR_SATU2);
  EXPECT_EQ(((unsigned short *)dbuf)[4], NULLU2);
  EXPECT_EQ(((unsigned short *)dbuf)[5], 1000);
}

TEST_F(ReadWriteTiff, GdalIoTestsWriteInt8) {
  PixelType isisPixelType = SignedByte;
  createTiff(isisPixelType, false);
  // Create a context so the handler goes out of scope and closes the file
  {
    const QList<int> *bandList = new QList<int>;
    GdalIoHandler handler(path, bandList, IsisPixelToGdal(isisPixelType));

    localBrick = new Brick(6, 1, 1, isisPixelType);
    localBrick->SetBasePosition(1, 1, 1);

    double *brickDoubleBuff = localBrick->DoubleBuffer();

    brickDoubleBuff[0] = HIGH_INSTR_SAT8;
    brickDoubleBuff[1] = HIGH_REPR_SAT8;
    brickDoubleBuff[2] = LOW_INSTR_SAT8;
    brickDoubleBuff[3] = LOW_REPR_SAT8;
    brickDoubleBuff[4] = NULL8;
    brickDoubleBuff[5] = 50;

    handler.write(*localBrick);
  }

  dataset = GDALDataset::FromHandle(GDALOpen(path.toStdString().c_str(), GA_ReadOnly));
  dbuf = (char *)CPLMalloc(sizeof(char) * 6);
  GDALRasterBand *poBand = dataset->GetRasterBand(1);
  CPLErr err = poBand->RasterIO(GF_Read, 0, 0,
                                         6, 1,
                                         dbuf,
                                         6, 1,
                                         IsisPixelToGdal(isisPixelType),
                                         0, 0);
  EXPECT_EQ(((char *)dbuf)[0], HIGH_INSTR_SATS1);
  EXPECT_EQ(((char *)dbuf)[1], HIGH_REPR_SATS1);
  EXPECT_EQ(((char *)dbuf)[2], LOW_INSTR_SATS1);
  EXPECT_EQ(((char *)dbuf)[3], LOW_REPR_SATS1);
  EXPECT_EQ(((char *)dbuf)[4], NULLS1);
  EXPECT_EQ(((char *)dbuf)[5], 50);
}

TEST_F(ReadWriteTiff, GdalIoTestsWriteUInt8) {
  PixelType isisPixelType = UnsignedByte;
  createTiff(isisPixelType, false);
  // Create a context so the handler goes out of scope and closes the file
  {
    const QList<int> *bandList = new QList<int>;
    GdalIoHandler handler(path, bandList, IsisPixelToGdal(isisPixelType));

    localBrick = new Brick(6, 1, 1, isisPixelType);
    localBrick->SetBasePosition(1, 1, 1);

    double *brickDoubleBuff = localBrick->DoubleBuffer();

    brickDoubleBuff[0] = HIGH_INSTR_SAT8;
    brickDoubleBuff[1] = HIGH_REPR_SAT8;
    brickDoubleBuff[2] = LOW_INSTR_SAT8;
    brickDoubleBuff[3] = LOW_REPR_SAT8;
    brickDoubleBuff[4] = NULL8;
    brickDoubleBuff[5] = 50;

    handler.write(*localBrick);
  }

  dataset = GDALDataset::FromHandle(GDALOpen(path.toStdString().c_str(), GA_ReadOnly));
  dbuf = (unsigned char *)CPLMalloc(sizeof(unsigned char) * 6);
  GDALRasterBand *poBand = dataset->GetRasterBand(1);
  CPLErr err = poBand->RasterIO(GF_Read, 0, 0,
                                         6, 1,
                                         dbuf,
                                         6, 1,
                                         IsisPixelToGdal(isisPixelType),
                                         0, 0);
  EXPECT_EQ(((unsigned char *)dbuf)[0], HIGH_INSTR_SAT1);
  EXPECT_EQ(((unsigned char *)dbuf)[1], HIGH_REPR_SAT1);
  EXPECT_EQ(((unsigned char *)dbuf)[2], LOW_INSTR_SAT1);
  EXPECT_EQ(((unsigned char *)dbuf)[3], LOW_REPR_SAT1);
  EXPECT_EQ(((unsigned char *)dbuf)[4], NULL1);
  EXPECT_EQ(((unsigned char *)dbuf)[5], 50);
}

