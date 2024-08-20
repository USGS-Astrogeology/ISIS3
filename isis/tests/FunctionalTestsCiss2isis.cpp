#include <QTemporaryDir>

#include "ciss2isis.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/ciss2isis.xml").expanded();

TEST(Ciss2Isis, Ciss2isisTestNac) {
  Pvl appLog;
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/ciss2isis_out.cub";
  QVector<QString> args = { "from=data/ciss2isis/N1472853667_1.cropped.lbl",
                            "to=" + cubeFileName };
  UserInterface options(APP_XML, args);
  try {
    ciss2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest file: " << e.toString().toStdString().c_str() << std::endl;
  }

  Cube outCube(cubeFileName);
  Pvl *outLabel = outCube.label();

  PvlGroup dimensions = outLabel->findGroup("Dimensions", Pvl::Traverse);
  ASSERT_EQ((int)dimensions["Samples"], 1024);
  ASSERT_EQ((int)dimensions["Lines"], 10);
  ASSERT_EQ((int)dimensions["Bands"], 1);

  PvlGroup pixels = outLabel->findGroup("Pixels", Pvl::Traverse);
  ASSERT_EQ(pixels["Type"][0], "SignedWord");
  ASSERT_EQ(pixels["ByteOrder"][0], "Lsb");
  ASSERT_EQ((double)pixels["Base"], 0.0);
  ASSERT_EQ((double)pixels["Multiplier"], 1.0);

  std::istringstream iss(R"(
    Group = Instrument
      SpacecraftName          = Cassini-Huygens
      InstrumentId            = ISSNA
      TargetName              = Saturn
      StartTime               = 2004-09-02T21:32:36.410
      StopTime                = 2004-09-02T21:36:16.410
      ExposureDuration        = 220000.0 <Milliseconds>
      AntibloomingStateFlag   = On

      # BiasStripMean value converted back to 12 bit.
      BiasStripMean           = 50.00196
      CompressionRatio        = 1.845952
      CompressionType         = Lossless
      DataConversionType      = Table
      DelayedReadoutFlag      = No
      FlightSoftwareVersionId = 1.3
      GainModeId              = 12 <ElectronsPerDN>
      GainState               = 3
      ImageTime               = 2004-09-02T21:36:16.410
      InstrumentDataRate      = 182.783997 <KilobitsPerSecond>
      OpticsTemperature       = (0.712693, 1.905708 <DegreesCelcius>)
      ReadoutCycleIndex       = 10
      ShutterModeId           = NacOnly
      ShutterStateId          = Enabled
      SummingMode             = 1
      InstrumentModeId        = Full
      SpacecraftClockCount    = 1/1472853447.118
      ReadoutOrder            = 0
    End_Group
  )");

  PvlGroup truthInstGroup;
  iss >> truthInstGroup;
  PvlGroup &instGroup = outLabel->findGroup("Instrument", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, instGroup, truthInstGroup);

  std::istringstream arss(R"(
    Group = Archive
      DataSetId     = CO-S-ISSNA/ISSWA-2-EDR-V1.0
      ImageNumber   = 1472853667
      ObservationId = ISS_00ARI_DIFFUSRNG003_PRIME
      ProductId     = 1_N1472853667.118
    End_Group
  )");

  PvlGroup truthArchiveGroup;
  arss >> truthArchiveGroup;

  PvlGroup &archiveGroup = outLabel->findGroup("Archive", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, archiveGroup, truthArchiveGroup);

  std::istringstream bbss(R"(
    Group = BandBin
      FilterName   = CL1/CL2
      OriginalBand = 1
      Center       = 651.065
      Width        = 340.923
    End_Group
  )");

  PvlGroup truthBandBinGroup;
  bbss >> truthBandBinGroup;

  PvlGroup &bandBinGroup = outLabel->findGroup("BandBin", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, bandBinGroup, truthBandBinGroup);

  // Check for the ISS prefix pixel table
  ASSERT_TRUE(outLabel->hasObject("Table"));
  EXPECT_EQ(outLabel->findObject("Table", Pvl::Traverse)["Name"][0], "ISS Prefix Pixels");

  std::unique_ptr<Histogram> hist (outCube.histogram());
  EXPECT_NEAR(hist->Average(), 247.45226885705699, .00001);
  EXPECT_EQ(hist->Sum(), 2470316);
  EXPECT_EQ(hist->ValidPixels(), 9983);
  EXPECT_NEAR(hist->StandardDeviation(), 27.779542219945746, .0001);
}

TEST(Ciss2Isis, Ciss2isisTestWac) {
  Pvl appLog;
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/ciss2isis_out.cub";
  QVector<QString> args = { "from=data/ciss2isis/W1472855646_5.cropped.lbl",
                            "to=" + cubeFileName };
  UserInterface options(APP_XML, args);
  try {
    ciss2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest file: " << e.toString().toStdString().c_str() << std::endl;
  }

  Cube outCube(cubeFileName);
  Pvl *outLabel = outCube.label();

  PvlGroup dimensions = outLabel->findGroup("Dimensions", Pvl::Traverse);
  ASSERT_EQ((int)dimensions["Samples"], 1024);
  ASSERT_EQ((int)dimensions["Lines"], 10);
  ASSERT_EQ((int)dimensions["Bands"], 1);

  PvlGroup pixels = outLabel->findGroup("Pixels", Pvl::Traverse);
  ASSERT_EQ(pixels["Type"][0], "SignedWord");
  ASSERT_EQ(pixels["ByteOrder"][0], "Lsb");
  ASSERT_EQ((double)pixels["Base"], 0.0);
  ASSERT_EQ((double)pixels["Multiplier"], 1.0);

  std::istringstream iss(R"(
    Group = Instrument
      SpacecraftName          = Cassini-Huygens
      InstrumentId            = ISSWA
      TargetName              = Saturn
      StartTime               = 2004-09-02T22:09:15.409
      StopTime                = 2004-09-02T22:09:15.409
      ExposureDuration        = 5.0 <Milliseconds>
      AntibloomingStateFlag   = On
      BiasStripMean           = 72.644554
      CompressionRatio        = NotCompressed
      CompressionType         = NotCompressed
      DataConversionType      = 12Bit
      DelayedReadoutFlag      = Yes
      FlightSoftwareVersionId = 1.3
      GainModeId              = 29 <ElectronsPerDN>
      GainState               = 2
      ImageTime               = 2004-09-02T22:09:15.409
      InstrumentDataRate      = 182.783997 <KilobitsPerSecond>
      OpticsTemperature       = (7.024934, -999.0 <DegreesCelcius>)
      ReadoutCycleIndex       = 0
      ShutterModeId           = BothSim
      ShutterStateId          = Disabled
      SummingMode             = 1
      InstrumentModeId        = Full
      SpacecraftClockCount    = 1/1472855646.121
      ReadoutOrder            = 0
    End_Group
  )");

  PvlGroup truthInstGroup;
  iss >> truthInstGroup;
  PvlGroup &instGroup = outLabel->findGroup("Instrument", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, instGroup, truthInstGroup);

  std::istringstream arss(R"(
    Group = Archive
      DataSetId     = CO-S-ISSNA/ISSWA-2-EDR-V1.0
      ImageNumber   = 1472855646
      ObservationId = ISS_00ASA_MOS0ASWE001_UVIS
      ProductId     = 1_W1472855646.121
    End_Group
  )");

  PvlGroup truthArchiveGroup;
  arss >> truthArchiveGroup;

  PvlGroup &archiveGroup = outLabel->findGroup("Archive", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, archiveGroup, truthArchiveGroup);

  std::istringstream bbss(R"(
    Group = BandBin
      FilterName   = CL1/CL2
      OriginalBand = 1
      Center       = 633.837
      Width        = 285.938
    End_Group
  )");

  PvlGroup truthBandBinGroup;
  bbss >> truthBandBinGroup;

  PvlGroup &bandBinGroup = outLabel->findGroup("BandBin", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, bandBinGroup, truthBandBinGroup);

  // Check for the ISS prefix pixel table
  ASSERT_TRUE(outLabel->hasObject("Table"));
  EXPECT_EQ(outLabel->findObject("Table", Pvl::Traverse)["Name"][0], "ISS Prefix Pixels");

  std::unique_ptr<Histogram> hist (outCube.histogram());
  EXPECT_NEAR(hist->Average(), 70.914941406249994, .00001);
  EXPECT_EQ(hist->Sum(), 726169);
  EXPECT_EQ(hist->ValidPixels(), 10240);
  EXPECT_NEAR(hist->StandardDeviation(), 0.84419124016427105, .0001);
}

TEST(Ciss2Isis, Ciss2isisCustomMax) {
  Pvl appLog;
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/ciss2isis_out.cub";

  QString inputLabel = "data/ciss2isis/W1472855646_5.cropped.lbl";
  QString updatedPvlLabel = prefix.path() + "/W1472855646_5.cropped.lbl";
  Pvl inputPvl(inputLabel.toStdString());
  inputPvl["VALID_MAXIMUM"][1] = "70";
  inputPvl.write(updatedPvlLabel.toStdString());
  QFile::copy("data/ciss2isis/W1472855646_5.cropped.img", prefix.path() + "/W1472855646_5.cropped.img");

  QVector<QString> args = { "from=" + updatedPvlLabel,
                            "to=" + cubeFileName };
  UserInterface options(APP_XML, args);
  try {
    ciss2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest file: " << e.toString().toStdString().c_str() << std::endl;
  }

  Cube outCube(cubeFileName);
  Pvl *outLabel = outCube.label();

  PvlGroup dimensions = outLabel->findGroup("Dimensions", Pvl::Traverse);
  ASSERT_EQ((int)dimensions["Samples"], 1024);
  ASSERT_EQ((int)dimensions["Lines"], 10);
  ASSERT_EQ((int)dimensions["Bands"], 1);

  PvlGroup pixels = outLabel->findGroup("Pixels", Pvl::Traverse);
  ASSERT_EQ(pixels["Type"][0], "SignedWord");
  ASSERT_EQ(pixels["ByteOrder"][0], "Lsb");
  ASSERT_EQ((double)pixels["Base"], 0.0);
  ASSERT_EQ((double)pixels["Multiplier"], 1.0);

  std::unique_ptr<Histogram> hist (outCube.histogram());
  EXPECT_EQ(hist->Maximum(), 69);
  EXPECT_EQ(hist->ValidPixels(), 728);
  EXPECT_EQ(hist->HrsPixels(), (1024 * 10) - hist->ValidPixels());
}
