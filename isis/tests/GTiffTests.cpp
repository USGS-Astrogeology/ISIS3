#include <QTemporaryFile>
#include <QString>
#include <iostream>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "Blob.h"
#include "Brick.h"
#include "Camera.h"
#include "Cube.h"
#include "CubeAttribute.h"
#include "Histogram.h"
#include "LineManager.h"
#include "Statistics.h"

#include "CubeFixtures.h"
#include "TestUtilities.h"

#include "gmock/gmock.h"

using namespace Isis;

void check_tiff(Cube &cube,
                QString &file, 
                int sampleCount, 
                int lineCount, 
                int bandCount, 
                double base, 
                double multiplier, 
                int pixelType, 
                int attachType, 
                int format,
                int isOpen,
                int isReadOnly,
                int isReadWrite,
                int labelSize) {
  EXPECT_EQ(cube.fileName().toStdString(), file.toStdString());
  EXPECT_EQ(cube.sampleCount(), sampleCount);
  EXPECT_EQ(cube.lineCount(), lineCount);
  EXPECT_EQ(cube.bandCount(), bandCount);
  EXPECT_EQ(cube.base(), base);
  EXPECT_EQ(cube.multiplier(), multiplier);
  EXPECT_EQ(cube.pixelType(), pixelType);
  EXPECT_EQ(cube.labelsAttached(), attachType);
  EXPECT_EQ(cube.format(), format);
  EXPECT_EQ(cube.isOpen(), isOpen);
  if (cube.isOpen()) {
    EXPECT_EQ(cube.isReadOnly(), isReadOnly);
    EXPECT_EQ(cube.isReadWrite(), isReadWrite);
  }
  EXPECT_EQ(cube.labelSize(), labelSize);
}


TEST_F(TempTestingFiles, TestGTiffCreateWriteCopy) {
  Cube out;
  QString file = "";
  check_tiff(out, file, 0, 0, 0, 0, 1, 7, 0, 1, 0, 0, 0, 65536);
  out.setDimensions(150, 200, 2);
  out.setFormat(Isis::Cube::GTiff);
  file = QString(tempDir.path() + "/IsisCube_00.tiff");
  out.create(file);
  check_tiff(out, file, 150, 200, 2, 0, 1, 7, 0, 2, 1, 0, 1, 65536);

  LineManager line(out);
  long j = 0;
  for(line.begin(); !line.end(); line++) {
    for(int i = 0; i < line.size(); i++) {
      line[i] = (double) j;
      j++;
    }
    j--;
    out.write(line);
  }

  // Copy returns the resulting Cube, we don't care about it (but we need it to flush) so delete
  QString file2 = tempDir.path() + "/IsisCube_01.tiff";
  CubeAttributeOutput outAtt;
  outAtt.setFileFormat(Isis::Cube::GTiff);
  delete out.copy(file2, outAtt);
  out.close();

  // Test the open and read methods
  Cube in(file2);
  check_tiff(in, file2, 150, 200, 2, 0, 1, 7, 0, 2, 1, 1, 0, 65536);

  LineManager inLine(in);
  j = 0;
  for(inLine.begin(); !inLine.end(); inLine++) {
    in.read(inLine);
    for(int i = 0; i < inLine.size(); i++) {
      EXPECT_NEAR(inLine[i], (double) j, 1e-15);
      j++;
    }
    j--;
  }
  in.close();
}

class GdalDnTypeGenerator: public TempTestingFiles, public ::testing::WithParamInterface<Isis::PixelType> {
    // Intentionally left empty
    void SetUp() {
        TempTestingFiles::SetUp();
    }
};

INSTANTIATE_TEST_SUITE_P (GdalDnPixelTypes,
                          GdalDnTypeGenerator,
                          ::testing::Values(Isis::UnsignedByte,
                                            Isis::SignedByte,
                                            Isis::UnsignedWord,
                                            Isis::SignedWord,
                                            Isis::UnsignedInteger,
                                            Isis::SignedInteger,
                                            Isis::Real,
                                            Isis::Double));

TEST_P(GdalDnTypeGenerator, TestGTiffCreateWrite) {
  Cube out;
  QString file = "";
  check_tiff(out, file, 0, 0, 0, 0, 1, 7, 0, 1, 0, 0, 0, 65536);
  int lines = 200;
  int samples = 150;
  int bands = 4;
  out.setDimensions(samples, lines, bands);
  out.setBaseMultiplier(0.0, 1.0);
  out.setFormat(Cube::GTiff);
  out.setPixelType(GetParam());
  check_tiff(out, file, samples, lines, bands, 0, 1, GetParam(), 0, 2, 0, 0, 0, 65536);
  file = QString(tempDir.path() + "/IsisCube_00.tiff");
  out.create(file);

  // Only write DNs between 3 and 127 as those are valid
  // DN values for all pixel types
  // All special pixel input and output tests are handled in the
  // GdalIoHandler tests
  int min = 3;
  int max = 127;

  long j = min;
  LineManager oline(out);
  for(oline.begin(); !oline.end(); oline++) {
    for(int i = 0; i < oline.size(); i++) {
      oline[i] = (double) j;
    }
    out.clearIoCache();
    out.write(oline);
    j++;
    if (j > max) {
      j = min;
    }
  }
  out.close();

  Cube in;
  try {
    in.open(file);
  }
  catch (IException &e) {
    e.print();
  }
  check_tiff(in, file, samples, lines, bands, 0, 1, GetParam(), 0, 2, 1, 1, 0, 65536);

  Statistics *cubeStats = in.statistics(0);
  EXPECT_DOUBLE_EQ(cubeStats->Average(), 62.65625);
  EXPECT_DOUBLE_EQ(cubeStats->StandardDeviation(), 36.277390383170371);
  EXPECT_DOUBLE_EQ(cubeStats->TotalPixels(), 120000);
  EXPECT_DOUBLE_EQ(cubeStats->NullPixels(), 0);
  delete cubeStats;
  in.close();
}

// Add Test for GTiff with no ISIS metadata

// Add Test for extracting blobs