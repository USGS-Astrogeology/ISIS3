#include <QTemporaryFile>
#include <QString>
#include <iostream>

#include "Buffer.h"

#include "TempFixtures.h"
#include "TestUtilities.h"

#include "gmock/gmock.h"

using namespace Isis;

TEST(BufferTest, TestBufferDefault) {
  Buffer b(4, 3, 2, Isis::SignedInteger);

  for(int i = 0; i < b.size(); i++) {
    b[i] = i;
  }

  EXPECT_EQ(b.SampleDimension(), 4);
  EXPECT_EQ(b.LineDimension(), 3);
  EXPECT_EQ(b.BandDimension(), 2);
  EXPECT_EQ(b.size(), 24);

  EXPECT_EQ(b.Sample(), 0);
  EXPECT_EQ(b.Line(), 0);
  EXPECT_EQ(b.Band(), 0);

  int samp, line, band;
  b.Position(0, samp, line, band);
  EXPECT_EQ(samp, 0);
  EXPECT_EQ(line, 0);
  EXPECT_EQ(band, 0);

  EXPECT_EQ(b.Index(samp, line, band), 0);

  EXPECT_EQ(b.at(0), 0);
  EXPECT_EQ(b.at(10), 10);
  EXPECT_EQ(b.at(23), 23);
  EXPECT_EQ(b[0], 0);
  EXPECT_EQ(b[10], 10);
  EXPECT_EQ(b[23], 23);
  
  // b.SetBasePosition(3, 2, 1);
  // EXPECT_EQ(b.Sample(), 3);
  // EXPECT_EQ(b.Line(), 2);
  // EXPECT_EQ(b.Band(), 1);

  // b.Position(0, samp, line, band);
  // EXPECT_EQ(samp, 3);
  // EXPECT_EQ(line, 2);
  // EXPECT_EQ(band, 1);

  // EXPECT_EQ(b.Index(samp, line, band), 0);
  
  // EXPECT_EQ(b.Sample(16), 3);
  // EXPECT_EQ(b.Line(16), 3);
  // EXPECT_EQ(b.Band(16), 2);

  // b.Position(16, samp, line, band);
  // EXPECT_EQ(samp, 3);
  // EXPECT_EQ(line, 3);
  // EXPECT_EQ(band, 2);

  // EXPECT_EQ(b.Index(samp, line, band), 16);
}

TEST(BufferTest, TestBufferCopy) {
  Buffer b(4, 3, 2, Isis::SignedInteger);

  for(int i = 0; i < b.size(); i++) {
    b[i] = i;
  }

  Buffer a = b;

  EXPECT_TRUE(a.DoubleBuffer() != b.DoubleBuffer());

  EXPECT_EQ(a.SampleDimension(), 4);
  EXPECT_EQ(a.LineDimension(), 3);
  EXPECT_EQ(a.BandDimension(), 2);
  EXPECT_EQ(a.size(), 24);

  EXPECT_EQ(a[0], 0);
  EXPECT_EQ(a[23], 23);

  EXPECT_EQ(Isis::PixelTypeName(a.PixelType()), "SignedInteger");
}

TEST(BufferTest, TestBufferDefaultConstructor) {
  Buffer nullbuf;
  EXPECT_EQ(nullbuf.size(), 0);
}

TEST(BufferTest, TestBufferAssignment) {
  Buffer b(2, 2, 2, Isis::Double);
  b = 999.0;

  EXPECT_EQ(b.size(), 8);
  EXPECT_EQ(b[0], 999.0);
  EXPECT_EQ(b[2], 999.0);
  EXPECT_EQ(b[b.size()-1], 999.0);
}

TEST(BufferTest, TestBufferOutOfBound) {
  Buffer b(4, 3, 2, Isis::SignedInteger);
  for(int i = 0; i < b.size(); i++) {
    b[i] = i;
  }

  try {
    b.at(-1);
  }
  catch(Isis::IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Array subscript [-1] is out of array bounds"))
      << e.toString().toStdString();
  }

  try {
    b.at(24);
  }
  catch(Isis::IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Array subscript [24] is out of array bounds"))
      << e.toString().toStdString();
  }
}

TEST(BufferTest, TestBufferScale) {
  Buffer b(4, 3, 2, Isis::SignedInteger, 0.5);

  EXPECT_EQ(b.SampleDimension(), 4);
  EXPECT_EQ(b.LineDimension(), 3);
  EXPECT_EQ(b.BandDimension(), 2);

  EXPECT_EQ(b.SampleDimensionScaled(), 2);
  EXPECT_EQ(b.LineDimensionScaled(), 1);

  EXPECT_EQ(b.size(), 4);

  for(int i = 0; i < b.size(); i++) {
    b[i] = i;
  }

  Buffer d(4, 3, 2, Isis::SignedInteger, 1);
  d.CopyOverlapFrom(b);
  std::vector<int> truthBuffer = {0, 0, 1, 1,
                                  0, 0, 1, 1,
                                  0, 0, 1, 1,
                                  // Second band
                                  2, 2, 3, 3,
                                  2, 2, 3, 3,
                                  2, 2, 3, 3};
  for (int i = 0; i < truthBuffer.size(); i++) {
    EXPECT_EQ(truthBuffer[i], d[i]);
  }
}