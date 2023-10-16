#include <iostream>
#include <time.h>

#include <QString>
#include <nlohmann/json.hpp>

#include "TempFixtures.h"
#include "md5wrapper.h"
#include "Plugin.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"

#include "isisimport.h"
#include "TestUtilities.h"

#include "gmock/gmock.h"

using namespace Isis;
using json = nlohmann::json;

static QString APP_XML = FileName("$ISISROOT/bin/xml/isisimport.xml").expanded();


TEST_F(TempTestingFiles, FunctionalTestIsisImportMerMICaseOne){
    std::istringstream PvlInput(R"(
        Object = IsisCube
            Object = Core
                StartByte   = 65537
                Format      = Tile
                TileSamples = 128
                TileLines   = 128

                Group = Dimensions
                    Samples = 5
                    Lines   = 5
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
                RoverMotionCounter          = (64, 472, 321, 1798, 1331)
                RoverMotionCounterName      = (SITE, DRIVE, IDD, PMA, HGA)
                SpacecraftName              = MARS_EXPLORATION_ROVER_1
                InstrumentID                = MI
                InstrumentName              = "MICROSCOPIC IMAGER"
                InstrumentSerialNumber      = 110
                LocalTrueSolarTime          = 12:07:38
                PlanetDayNumber             = 691
                SolarLongitude              = 350.592
                SpacecraftClockCntPartition = 1
                SpacecraftClockStartCount   = 189529263.621
                SpacecraftClockStopCount    = 189529263.774
                StartTime                   = 2006-01-03T02:59:18.555
                StopTime                    = 2006-01-03T02:59:18.707
                ExposureDuration            = 153.6 <ms>
                ExposureDurationCount       = 30
                FilterName                  = MI_OPEN
                FilterNumber                = 2
                FlatFieldCorrectionFlag     = FALSE
                InstrumentModeID            = FULL_FRAME
                InstrumentTemperature       = (-13.9083, -3.79407, 0.272201, -6.62645,
                                            -7.49551, -8.20222, -3.87095, -11.4134,
                                            0.0) <degC>
                InstrumentTemperatureName   = ("FRONT HAZ ELECTRONICS",
                                            "REAR HAZ ELECTRONICS",
                                            "LEFT PAN ELECTRONICS", "LEFT PAN CCD",
                                            "RIGHT PAN CCD", "LEFT NAV CCD", "MI CCD",
                                            "MI ELECTRONICS", "EDL CCD")
                OffsetModeID                = 4080
                ShutterEffectCorrectionFlag = TRUE
                TemperatureMiCCD            = -3.87095
                TemperatureMiElectronics    = -11.4134
            End_Group

            Group = Archive
                DataSetID   = MER1-M-MI-2-EDR-SCI-V1.0
                DataSetName = "MER 1 MARS MICROSCOPIC IMAGER SCIENCE EDR VERSION 1.0"
                ProductID   = 1M189529263EFF64KCP2977M2F1
            End_Group

            Group = MerImageRequestParms
                PixelAveragingHeight = 1
                PixelAveragingWidth  = 1
            End_Group

            Group = MerSubframeRequestParms
                FirstLine        = 1
                FirstLineSamples = 1
            End_Group
        End_Object

        Object = Label
            Bytes = 65536
        End_Object

        Object = History
            Name      = IsisCube
            StartByte = 2162689
            Bytes     = 473
        End_Object

        Object = OriginalLabel
            Name      = IsisCube
            StartByte = 2163162
            Bytes     = 19191
        End_Object
        End
    )");
  QVector<QString> args = {"from=data/isisimport/1M189529263EFF64KCP2977M2F1_cropped.IMG", "to=" + tempDir.path() + "/MerMI1.cub" };
  UserInterface options(APP_XML, args);

  isisimport(options);

  Pvl output = Pvl(tempDir.path() + "/MerMI1.cub");
  Pvl truth;
  PvlInput >> truth;

  PvlGroup truthDimensions = truth.findGroup("Dimensions", Isis::Plugin::Traverse);
  PvlGroup truthPixels = truth.findGroup("Pixels", Isis::Plugin::Traverse);
  PvlGroup truthArchive = truth.findGroup("Archive", Isis::Plugin::Traverse);
  PvlGroup truthInstrument = truth.findGroup("Instrument", Isis::Plugin::Traverse);
  PvlGroup truthMerImageRequestParms = truth.findGroup("MerImageRequestParms", Isis::Plugin::Traverse);
  PvlGroup truthMerSubframeRequestParms = truth.findGroup("MerSubframeRequestParms", Isis::Plugin::Traverse);

  PvlGroup outputDimensions = output.findGroup("Dimensions", Isis::Plugin::Traverse);
  PvlGroup outputPixels = output.findGroup("Pixels", Isis::Plugin::Traverse);
  PvlGroup outputArchive = output.findGroup("Archive", Isis::Plugin::Traverse);
  PvlGroup outputInstrument = output.findGroup("Instrument", Isis::Plugin::Traverse);
  PvlGroup outputMerImageRequestParms = output.findGroup("MerImageRequestParms", Isis::Plugin::Traverse);
  PvlGroup outputMerSubframeRequestParms = output.findGroup("MerSubframeRequestParms", Isis::Plugin::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outputDimensions, truthDimensions);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outputPixels, truthPixels);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outputArchive, truthArchive);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outputInstrument, truthInstrument);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outputMerImageRequestParms, truthMerImageRequestParms);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outputMerSubframeRequestParms, truthMerSubframeRequestParms);

}

TEST_F(TempTestingFiles, FunctionalTestIsisImportMerMICaseTwo){
    std::istringstream PvlInput(R"(
        Object = IsisCube
          Object = Core
            StartByte   = 65537
            Format      = Tile
            TileSamples = 5
            TileLines   = 5

            Group = Dimensions
              Samples = 5
              Lines   = 5
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
            RoverMotionCounter          = (121, 0, 89, 304, 60)
            RoverMotionCounterName      = (SITE, DRIVE, IDD, PMA, HGA)
            SpacecraftName              = MARS_EXPLORATION_ROVER_2
            InstrumentID                = MI
            InstrumentName              = "MICROSCOPIC IMAGER"
            InstrumentSerialNumber      = 105
            LocalTrueSolarTime          = 11:14:38
            PlanetDayNumber             = 710
            SolarLongitude              = 349.777
            SpacecraftClockCntPartition = 1
            SpacecraftClockStartCount   = 189392700.246
            SpacecraftClockStopCount    = 189392700.886
            StartTime                   = 2006-01-01T13:01:41.933
            StopTime                    = 2006-01-01T13:01:42.573
            ExposureDuration            = 640.0 <ms>
            ExposureDurationCount       = 125
            FilterName                  = MI_OPEN
            FilterNumber                = 2
            FlatFieldCorrectionFlag     = FALSE
            InstrumentModeID            = FULL_FRAME
            InstrumentTemperature       = (-27.0727, -12.3804, -9.42408, -11.0184,
                                          -12.5536, 0.0, -29.7877, -31.0892,
                                          0.0) <degC>
            InstrumentTemperatureName   = ("FRONT HAZ ELECTRONICS",
                                          "REAR HAZ ELECTRONICS",
                                          "LEFT PAN ELECTRONICS", "LEFT PAN CCD",
                                          "RIGHT PAN CCD", "LEFT NAV CCD", "MI CCD",
                                          "MI ELECTRONICS", "EDL CCD")
            OffsetModeID                = 4090
            ShutterEffectCorrectionFlag = TRUE
            TemperatureMiCCD            = -29.7877
            TemperatureMiElectronics    = -31.0892
          End_Group

          Group = Archive
            DataSetID   = MER2-M-MI-2-EDR-SCI-V1.0
            DataSetName = "MER 2 MARS MICROSCOPIC IMAGER SCIENCE EDR VERSION 1.0"
            ProductID   = 2M189392700EFFAL00P2977M2F1
          End_Group

          Group = MerImageRequestParms
            PixelAveragingHeight = 1
            PixelAveragingWidth  = 1
          End_Group

          Group = MerSubframeRequestParms
            FirstLine        = 1
            FirstLineSamples = 1
          End_Group
        End_Object

        Object = Label
          Bytes = 65536
        End_Object

        Object = History
          Name      = IsisCube
          StartByte = 65587
          Bytes     = 542
        End_Object

        Object = OriginalLabel
          Name      = IsisCube
          StartByte = 66129
          Bytes     = 18124
        End_Object
        End
    )");
  QVector<QString> args = {"from=data/isisimport/2M189392700EFFAL00P2977M2F1_cropped.IMG", "to=" + tempDir.path() + "/MerMI2.cub" };
  UserInterface options(APP_XML, args);

  isisimport(options);

  Pvl output = Pvl(tempDir.path() + "/MerMI2.cub");
  Pvl truth;
  PvlInput >> truth;

  PvlGroup truthDimensions = truth.findGroup("Dimensions", Isis::Plugin::Traverse);
  PvlGroup truthPixels = truth.findGroup("Pixels", Isis::Plugin::Traverse);
  PvlGroup truthArchive = truth.findGroup("Archive", Isis::Plugin::Traverse);
  PvlGroup truthInstrument = truth.findGroup("Instrument", Isis::Plugin::Traverse);
  PvlGroup truthMerImageRequestParms = truth.findGroup("MerImageRequestParms", Isis::Plugin::Traverse);
  PvlGroup truthMerSubframeRequestParms = truth.findGroup("MerSubframeRequestParms", Isis::Plugin::Traverse);

  PvlGroup outputDimensions = output.findGroup("Dimensions", Isis::Plugin::Traverse);
  PvlGroup outputPixels = output.findGroup("Pixels", Isis::Plugin::Traverse);
  PvlGroup outputArchive = output.findGroup("Archive", Isis::Plugin::Traverse);
  PvlGroup outputInstrument = output.findGroup("Instrument", Isis::Plugin::Traverse);
  PvlGroup outputMerImageRequestParms = output.findGroup("MerImageRequestParms", Isis::Plugin::Traverse);
  PvlGroup outputMerSubframeRequestParms = output.findGroup("MerSubframeRequestParms", Isis::Plugin::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outputDimensions, truthDimensions);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outputPixels, truthPixels);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outputArchive, truthArchive);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outputInstrument, truthInstrument);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outputMerImageRequestParms, truthMerImageRequestParms);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outputMerSubframeRequestParms, truthMerSubframeRequestParms);

}