#include "stats.h"

#include <iostream>

#include "gmock/gmock.h"

#include "Cube.h"
#include "FileName.h"
#include "ImageHistogram.h"
#include "Pvl.h"
#include "SpecialPixel.h"

using namespace Isis;

class MockCube : public Cube {
  public:
    MOCK_CONST_METHOD0(bandCount, int());
    MOCK_CONST_METHOD0(fileName, QString());
    MOCK_CONST_METHOD1(physicalBand, int(const int &virtualBand));
    MOCK_METHOD4(histogram, Histogram*(
          const int &band, const double &validMin,
          const double &validMax,
          QString msg));
};

class stats_FlatFileTest : public ::testing::Test {
  protected:
    Pvl testPvl;

    void SetUp() override {
      PvlGroup firstGroup("FirstGroup");
      firstGroup += PvlKeyword("NumberKey", "0.0");
      firstGroup += PvlKeyword("StringKey", "Hello");
      testPvl += firstGroup;

      PvlGroup secondGroup("SecondGroup");
      PvlKeyword dupKey("DuplicateKey", "stats here");
      secondGroup += dupKey;
      secondGroup += dupKey;
      testPvl += secondGroup;
    }
};

class stats_MockHist : public ::testing::Test {
  protected:
    MockCube *mockCube;

    void SetUp() override {
      mockCube = nullptr;

      ImageHistogram *testBand1Stats = new ImageHistogram(-10, 10, 21);
      for (int val = -10; val <=10; val++) {
        testBand1Stats->AddData(val);
      }
      testBand1Stats->AddData(0.0);

      ImageHistogram *testBand2Stats = new ImageHistogram(-10, 10, 21);
      testBand2Stats->AddData(Null);
      testBand2Stats->AddData(Lrs);
      testBand2Stats->AddData(Lis);
      testBand2Stats->AddData(His);
      testBand2Stats->AddData(Hrs);

      mockCube = new MockCube();
      EXPECT_CALL(*mockCube, bandCount())
            .Times(1)
            .WillOnce(::testing::Return(2));
      EXPECT_CALL(*mockCube, histogram(1, ::testing::_, ::testing::_, ::testing::_))
            .Times(1)
            .WillOnce(::testing::Return(testBand1Stats));
      EXPECT_CALL(*mockCube, histogram(2, ::testing::_, ::testing::_, ::testing::_))
            .Times(1)
            .WillOnce(::testing::Return(testBand2Stats));
      EXPECT_CALL(*mockCube, fileName())
            .Times(2)
            .WillRepeatedly(::testing::Return("TestCube.cub"));
      EXPECT_CALL(*mockCube, physicalBand(1))
            .Times(1)
            .WillOnce(::testing::Return(1));
      EXPECT_CALL(*mockCube, physicalBand(2))
            .Times(1)
            .WillOnce(::testing::Return(2));
    }

    void TearDown() override {
      if (mockCube) {
        delete mockCube;
        mockCube = nullptr;
      }
      // The histograms will be cleaned up by the stats function
    }
};


TEST_F(stats_MockHist, TestStats) {
  Pvl statsPvl = stats(
        mockCube,
        Isis::ValidMinimum,
        Isis::ValidMaximum);

  ASSERT_EQ(statsPvl.groups(), 2);

  PvlGroup band1Stats = statsPvl.group(0);
  EXPECT_EQ("TestCube.cub", (std::string)(band1Stats.findKeyword("From")));
  EXPECT_EQ(1, (int) (band1Stats.findKeyword("Band")));
  EXPECT_EQ(22, (int) (band1Stats.findKeyword("ValidPixels")));
  EXPECT_EQ(22, (int) (band1Stats.findKeyword("TotalPixels")));
  EXPECT_EQ(0, (int) (band1Stats.findKeyword("OverValidMaximumPixels")));
  EXPECT_EQ(0, (int) (band1Stats.findKeyword("UnderValidMinimumPixels")));
  EXPECT_EQ(0, (int) (band1Stats.findKeyword("NullPixels")));
  EXPECT_EQ(0, (int) (band1Stats.findKeyword("LisPixels")));
  EXPECT_EQ(0, (int) (band1Stats.findKeyword("LrsPixels")));
  EXPECT_EQ(0, (int) (band1Stats.findKeyword("HisPixels")));
  EXPECT_EQ(0, (int) (band1Stats.findKeyword("HrsPixels")));
  EXPECT_EQ(0.0, (double) (band1Stats.findKeyword("Average")));
  EXPECT_NEAR(6.0553, (double) (band1Stats.findKeyword("StandardDeviation")), 0.0001);
  EXPECT_NEAR(36.6667, (double) (band1Stats.findKeyword("Variance")), 0.0001);
  EXPECT_EQ(0.0, (double) (band1Stats.findKeyword("Median")));
  EXPECT_EQ(0.0, (double) (band1Stats.findKeyword("Mode")));
  EXPECT_EQ(0.0, (double) (band1Stats.findKeyword("Skew")));
  EXPECT_EQ(-10, (double) (band1Stats.findKeyword("Minimum")));
  EXPECT_EQ(10.0, (double) (band1Stats.findKeyword("Maximum")));
  EXPECT_EQ(0.0, (double) (band1Stats.findKeyword("Sum")));

  PvlGroup band2Stats = statsPvl.group(1);
  EXPECT_EQ("TestCube.cub", (std::string)(band2Stats.findKeyword("From")));
  EXPECT_EQ(2, (int) (band2Stats.findKeyword("Band")));
  EXPECT_EQ(0, (int) (band2Stats.findKeyword("ValidPixels")));
  EXPECT_EQ(5, (int) (band2Stats.findKeyword("TotalPixels")));
  EXPECT_EQ(0, (int) (band2Stats.findKeyword("OverValidMaximumPixels")));
  EXPECT_EQ(0, (int) (band2Stats.findKeyword("UnderValidMinimumPixels")));
  EXPECT_EQ(1, (int) (band2Stats.findKeyword("NullPixels")));
  EXPECT_EQ(1, (int) (band2Stats.findKeyword("LisPixels")));
  EXPECT_EQ(1, (int) (band2Stats.findKeyword("LrsPixels")));
  EXPECT_EQ(1, (int) (band2Stats.findKeyword("HisPixels")));
  EXPECT_EQ(1, (int) (band2Stats.findKeyword("HrsPixels")));
  EXPECT_EQ("N/A", (std::string)band2Stats.findKeyword("Average"));
  EXPECT_EQ("N/A", (std::string)band2Stats.findKeyword("StandardDeviation"));
  EXPECT_EQ("N/A", (std::string)band2Stats.findKeyword("Variance"));
  EXPECT_EQ("N/A", (std::string)band2Stats.findKeyword("Median"));
  EXPECT_EQ("N/A", (std::string)band2Stats.findKeyword("Mode"));
  EXPECT_EQ("N/A", (std::string)band2Stats.findKeyword("Skew"));
  EXPECT_EQ("N/A", (std::string)band2Stats.findKeyword("Minimum"));
  EXPECT_EQ("N/A", (std::string)band2Stats.findKeyword("Maximum"));
  EXPECT_EQ("N/A", (std::string)band2Stats.findKeyword("Sum"));
}

TEST(stats, ValidMinimum) {
  ImageHistogram *testStats = new ImageHistogram(-1000,1000);

  MockCube *mockCube = new MockCube();
  EXPECT_CALL(*mockCube, bandCount())
        .Times(1)
        .WillOnce(::testing::Return(1));
  EXPECT_CALL(*mockCube, histogram(1, 0.0, ::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(testStats));
  EXPECT_CALL(*mockCube, fileName())
        .Times(1)
        .WillRepeatedly(::testing::Return("TestCube.cub"));
  EXPECT_CALL(*mockCube, physicalBand(1))
        .Times(1)
        .WillOnce(::testing::Return(1));

  Pvl statsPvl = stats(
        dynamic_cast<Cube*>(mockCube),
        0.0,
        Isis::ValidMaximum);

  delete mockCube;
  mockCube = nullptr;
  // The histogram will be cleaned up in the stats function
}

TEST(stats, ValidMaximum) {
  ImageHistogram *testStats = new ImageHistogram(-1000,1000);

  MockCube *mockCube = new MockCube();
  EXPECT_CALL(*mockCube, bandCount())
        .Times(1)
        .WillOnce(::testing::Return(1));
  EXPECT_CALL(*mockCube, histogram(1, ::testing::_, 0.0, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(testStats));
  EXPECT_CALL(*mockCube, fileName())
        .Times(1)
        .WillRepeatedly(::testing::Return("TestCube.cub"));
  EXPECT_CALL(*mockCube, physicalBand(1))
        .Times(1)
        .WillOnce(::testing::Return(1));

  Pvl statsPvl = stats(
        dynamic_cast<Cube*>(mockCube),
        Isis::ValidMinimum,
        0.0);

  delete mockCube;
  mockCube = nullptr;
  // The histogram will be cleaned up in the stats function
}

TEST_F(stats_FlatFileTest, FlatFile) {
  std::ostringstream *testStream = new std::ostringstream();
  writeStatsStream(testPvl, false, testStream);
  EXPECT_EQ(testStream->str(), "0.0,Hello\nstats here,stats here\n");

  delete testStream;
  testStream = nullptr;
}

TEST_F(stats_FlatFileTest, FlatFileHeader) {
  std::ostringstream *testStream = new std::ostringstream();
  writeStatsStream(testPvl, true, testStream);
  EXPECT_EQ(testStream->str(), "NumberKey,StringKey\n0.0,Hello\nstats here,stats here\n");

  delete testStream;
  testStream = nullptr;
}
