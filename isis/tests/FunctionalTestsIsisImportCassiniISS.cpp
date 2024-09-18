#include <iostream>
#include <time.h>

#include <QString>
#include <nlohmann/json.hpp>

#include "TempFixtures.h"
#include "Histogram.h"
#include "md5wrapper.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "TestUtilities.h"

#include "isisimport.h"

#include "gmock/gmock.h"

using namespace Isis;
using json = nlohmann::json;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/isisimport.xml").expanded());

TEST_F(TempTestingFiles, FunctionalTestIsisImportCassiniIssNac) {
  Pvl appLog;

  std::istringstream PvlInput(R"(
    Object = IsisCube
      Object = Core
        StartByte   = 65537
        Format      = Tile
        TileSamples = 1024
        TileLines   = 10

        Group = Dimensions
          Samples = 1024
          Lines   = 10
          Bands   = 1
        End_Group

        Group = Pixels
          Type       = SignedWord
          ByteOrder  = Lsb
          Base       = 0.0
          Multiplier = 1.0
        End_Group
      End_Object

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

      Group = Archive
        DataSetId     = CO-S-ISSNA/ISSWA-2-EDR-V1.0
        ImageNumber   = 1472853667
        ObservationId = ISS_00ARI_DIFFUSRNG003_PRIME
        ProductId     = 1_N1472853667.118
      End_Group

      Group = BandBin
        FilterName   = CL1/CL2
        OriginalBand = 1
        Center       = 651.065
        Width        = 340.923
      End_Group

      Group = Kernels
        NaifFrameCode = -82361
      End_Group
    End_Object

    Object = Label
      Bytes = 65536
    End_Object

    Object = Table
      Name        = "ISS Prefix Pixels"
      StartByte   = 90519
      Bytes       = 240
      Records     = 10
      ByteOrder   = Lsb
      Association = Lines

      Group = Field
        Name = OverclockPixels
        Type = Double
        Size = 3
      End_Group
    End_Object

    Object = OriginalLabel
      Name      = IsisCube
      StartByte = 86017
      Bytes     = 4502
    End_Object
    End
  )");

  QString cubeFileName = tempDir.path() + "/cissNac.cub";
  QVector<QString> args = {"from=data/ciss2isis/N1472853667_1.cropped.lbl", "to=" + cubeFileName};
  UserInterface options(APP_XML, args);

  try {
    isisimport(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest file: " <<  e.toString().c_str() << std::endl;
  }

  Pvl truthLabel;
  PvlInput >> truthLabel;

  Cube outCube(cubeFileName.toStdString());
  Pvl *outLabel = outCube.label();

  PvlGroup truthGroup = truthLabel.findGroup("Dimensions", Pvl::Traverse);
  PvlGroup &outGroup = outLabel->findGroup("Dimensions", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Pixels", Pvl::Traverse);
  outGroup = outLabel->findGroup("Pixels", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Instrument", Pvl::Traverse);
  outGroup = outLabel->findGroup("Instrument", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Archive", Pvl::Traverse);
  outGroup = outLabel->findGroup("Archive", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("BandBin", Pvl::Traverse);
  outGroup = outLabel->findGroup("BandBin", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  // Check for the ISS prefix pixel table
  ASSERT_TRUE(outLabel->hasObject("Table"));
  EXPECT_EQ(outLabel->findObject("Table", Pvl::Traverse)["Name"][0], "ISS Prefix Pixels");

  std::unique_ptr<Histogram> hist (outCube.histogram());
  EXPECT_NEAR(hist->Average(), 247.45226885705699, .00001);
  EXPECT_EQ(hist->Sum(), 2470316);
  EXPECT_EQ(hist->ValidPixels(), 9983);
  EXPECT_NEAR(hist->StandardDeviation(), 27.779542219945746, .0001);
}

TEST_F(TempTestingFiles, FunctionalTestIsisImportCassiniIssWac) {
  Pvl appLog;

  std::istringstream PvlInput(R"(
    Object = IsisCube
      Object = Core
        StartByte   = 65537
        Format      = Tile
        TileSamples = 1024
        TileLines   = 10

        Group = Dimensions
          Samples = 1024
          Lines   = 10
          Bands   = 1
        End_Group

        Group = Pixels
          Type       = SignedWord
          ByteOrder  = Lsb
          Base       = 0.0
          Multiplier = 1.0
        End_Group
      End_Object

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

      Group = Archive
        DataSetId     = CO-S-ISSNA/ISSWA-2-EDR-V1.0
        ImageNumber   = 1472855646
        ObservationId = ISS_00ASA_MOS0ASWE001_UVIS
        ProductId     = 1_W1472855646.121
      End_Group

      Group = BandBin
        FilterName   = CL1/CL2
        OriginalBand = 1
        Center       = 633.837
        Width        = 285.938
      End_Group

      Group = Kernels
        NaifFrameCode = -82361
      End_Group
    End_Object

    Object = Label
      Bytes = 65536
    End_Object

    Object = Table
      Name        = "ISS Prefix Pixels"
      StartByte   = 90469
      Bytes       = 240
      Records     = 10
      ByteOrder   = Lsb
      Association = Lines

      Group = Field
        Name = OverclockPixels
        Type = Double
        Size = 3
      End_Group
    End_Object

    Object = OriginalLabel
      Name      = IsisCube
      StartByte = 86017
      Bytes     = 4452
    End_Object
    End
  )");

  QString cubeFileName = tempDir.path() + "/cissWac.cub";
  QVector<QString> args = { "from=data/ciss2isis/W1472855646_5.cropped.lbl",
                            "to=" + cubeFileName };
  UserInterface options(APP_XML, args);
  try {
    isisimport(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest file: " <<  e.toString().c_str() << std::endl;
  }

  Pvl truthLabel;
  PvlInput >> truthLabel;

  Cube outCube(cubeFileName.toStdString());
  Pvl *outLabel = outCube.label();

  PvlGroup truthGroup = truthLabel.findGroup("Dimensions", Pvl::Traverse);
  PvlGroup &outGroup = outLabel->findGroup("Dimensions", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Pixels", Pvl::Traverse);
  outGroup = outLabel->findGroup("Pixels", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Instrument", Pvl::Traverse);
  outGroup = outLabel->findGroup("Instrument", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Archive", Pvl::Traverse);
  outGroup = outLabel->findGroup("Archive", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("BandBin", Pvl::Traverse);
  outGroup = outLabel->findGroup("BandBin", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  // Check for the ISS prefix pixel table
  ASSERT_TRUE(outLabel->hasObject("Table"));
  EXPECT_EQ(outLabel->findObject("Table", Pvl::Traverse)["Name"][0], "ISS Prefix Pixels");

  std::unique_ptr<Histogram> hist (outCube.histogram());
  EXPECT_NEAR(hist->Average(), 70.914941406249994, .00001);
  EXPECT_EQ(hist->Sum(), 726169);
  EXPECT_EQ(hist->ValidPixels(), 10240);
  EXPECT_NEAR(hist->StandardDeviation(), 0.84419124016427105, .0001);
}

TEST_F(TempTestingFiles, FunctionalTestIsisImportCassiniIssCustomMax) {
  Pvl appLog;

  std::istringstream PvlInput(R"(
    Object = IsisCube
      Object = Core
        StartByte   = 65537
        Format      = Tile
        TileSamples = 1024
        TileLines   = 10

        Group = Dimensions
          Samples = 1024
          Lines   = 10
          Bands   = 1
        End_Group

        Group = Pixels
          Type       = SignedWord
          ByteOrder  = Lsb
          Base       = 0.0
          Multiplier = 1.0
        End_Group
      End_Object

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

      Group = Archive
        DataSetId     = CO-S-ISSNA/ISSWA-2-EDR-V1.0
        ImageNumber   = 1472855646
        ObservationId = ISS_00ASA_MOS0ASWE001_UVIS
        ProductId     = 1_W1472855646.121
      End_Group

      Group = BandBin
        FilterName   = CL1/CL2
        OriginalBand = 1
        Center       = 633.837
        Width        = 285.938
      End_Group

      Group = Kernels
        NaifFrameCode = -82361
      End_Group
    End_Object

    Object = Label
      Bytes = 65536
    End_Object

    Object = Table
      Name        = "ISS Prefix Pixels"
      StartByte   = 90467
      Bytes       = 240
      Records     = 10
      ByteOrder   = Lsb
      Association = Lines

      Group = Field
        Name = OverclockPixels
        Type = Double
        Size = 3
      End_Group
    End_Object

    Object = OriginalLabel
      Name      = IsisCube
      StartByte = 86017
      Bytes     = 4450
    End_Object
    End
  )");
  QString cubeFileName = tempDir.path() + "/ciss2isis_out.cub";

  QString inputLabel = "data/ciss2isis/W1472855646_5.cropped.lbl";
  QString updatedPvlLabel = tempDir.path() + "/W1472855646_5.cropped.lbl";
  Pvl inputPvl(inputLabel.toStdString());
  inputPvl["VALID_MAXIMUM"][1] = "70";
  inputPvl.write(updatedPvlLabel.toStdString());
  QFile::copy("data/ciss2isis/W1472855646_5.cropped.img", tempDir.path() + "/W1472855646_5.cropped.img");

  QVector<QString> args = { "from=" + updatedPvlLabel,
                            "to=" + cubeFileName };
  UserInterface options(APP_XML, args);
  try {
    isisimport(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest file: " <<  e.toString().c_str() << std::endl;
  }

  Pvl truthLabel;
  PvlInput >> truthLabel;

  Cube outCube(cubeFileName.toStdString());
  Pvl *outLabel = outCube.label();

  PvlGroup truthGroup = truthLabel.findGroup("Dimensions", Pvl::Traverse);
  PvlGroup &outGroup = outLabel->findGroup("Dimensions", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Pixels", Pvl::Traverse);
  outGroup = outLabel->findGroup("Pixels", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  std::unique_ptr<Histogram> hist (outCube.histogram());
  EXPECT_EQ(hist->Maximum(), 69);
  EXPECT_EQ(hist->ValidPixels(), 728);
  EXPECT_EQ(hist->HrsPixels(), (1024 * 10) - hist->ValidPixels());
}
