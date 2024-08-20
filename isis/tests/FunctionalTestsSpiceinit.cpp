#include <iostream>
#include <QTemporaryFile>
#include <QRegularExpression>

#include <nlohmann/json.hpp>

#include "spiceinit.h"
#include "csminit.h"

#include "Blob.h"
#include "Cube.h"
#include "CubeAttribute.h"
#include "PixelType.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "TestUtilities.h"
#include "FileName.h"

#include "CameraFixtures.h"
#include "CubeFixtures.h"

using json = nlohmann::json;

#include "gmock/gmock.h"

using namespace Isis;
using ::testing::HasSubstr;

static QString APP_XML = FileName("$ISISROOT/bin/xml/spiceinit.xml").expanded();

TEST(Spiceinit, TestSpiceinitPredictAndReconCk) {

  std::istringstream labelStrm(R"(
    Object = IsisCube
      Object = Core
        StartByte   = 65537
        Format      = Tile
        TileSamples = 384
        TileLines   = 288

        Group = Dimensions
          Samples = 384
          Lines   = 288
          Bands   = 1
        End_Group

        Group = Pixels
          Type       = UnsignedByte
          ByteOrder  = Lsb
          Base       = 0.0
          Multiplier = 1.0
        End_Group
      End_Object

      Group = Instrument
        SpacecraftName           = "CLEMENTINE 1"
        InstrumentId             = UVVIS
        TargetName               = MOON
        StartTime                = 1994-03-05T08:21:22.626
        OrbitNumber              = 063
        FocalPlaneTemperature    = 273.633 <K>
        ExposureDuration         = 20.3904 <ms>
        OffsetModeID             = 6
        GainModeID               = 1
        CryocoolerDuration       = N/A
        EncodingCompressionRatio = 3.55
        EncodingFormat           = CLEM-JPEG-1
      End_Group

      Group = Archive
        ProductID    = LUB5120P.063
        MissionPhase = "LUNAR MAPPING"
      End_Group

      Group = BandBin
        FilterName = B
        Center     = 0.75 <micrometers>
        Width      = 0.01 <micrometers>
      End_Group

      Group = Kernels
        NaifFrameCode = -40022
      End_Group
    End_Object
  End
  )");

  Pvl label;
  labelStrm >> label;

  QTemporaryFile tempFile;
  tempFile.open();
  Cube testCube;

  testCube.fromLabel(tempFile.fileName() + ".cub", label, "rw");

  QVector<QString> args = {"ckrecon=True", "cksmithed=True", "attach=false"};
  UserInterface options(APP_XML, args);
  spiceinit(&testCube, options);

  PvlGroup kernels = testCube.group("Kernels");
  ASSERT_TRUE(kernels.hasKeyword("InstrumentPointing"));
  PvlKeyword instrumentPointing = kernels["InstrumentPointing"];
  ASSERT_EQ(instrumentPointing.size(), 3);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, instrumentPointing[0], "$Clementine1/kernels/ck/clem_2mn.bck");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, instrumentPointing[1], "$Clementine1/kernels/ck/clem_5sc.bck");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, instrumentPointing[2], "$clementine1/kernels/fk/clem_v12.tf");
  ASSERT_TRUE(kernels.hasKeyword("InstrumentPointingQuality"));
  EXPECT_PRED_FORMAT2(AssertStringsEqual, kernels["InstrumentPointingQuality"][0], "Reconstructed");
}


TEST(Spiceinit, TestSpiceinitCkConfigFile) {

  std::istringstream labelStrm(R"(
    Object = IsisCube
      Object = Core
        StartByte   = 65537
        Format      = Tile
        TileSamples = 320
        TileLines   = 420

        Group = Dimensions
          Samples = 640
          Lines   = 420
          Bands   = 1
        End_Group

        Group = Pixels
          Type       = Real
          ByteOrder  = Lsb
          Base       = 0.0
          Multiplier = 1.0
        End_Group
      End_Object

      Group = Instrument
        SpacecraftName            = "MARS RECONNAISSANCE ORBITER"
        InstrumentId              = CRISM
        TargetName                = Mars
        StartTime                 = 2011-02-25T01:51:05.839
        StopTime                  = 2011-02-25T01:52:57.573
        SpacecraftClockStartCount = 10/0983065897.48805
        SpacecraftClockStopCount  = 10/0983066009.31381
        SensorId                  = S
        ShutterModeId             = OPEN
        FrameRate                 = 3.75 <HZ>
        ExposureParameter         = 184
        PixelAveragingWidth       = 1
        ScanModeId                = SHORT
        SamplingModeId            = HYPERSPEC
      End_Group

      Group = Archive
        DataSetId               = MRO-M-CRISM-3-RDR-TARGETED-V1.0
        ProductId               = FRT0001CFD8_07_IF124S_TRR3
        ProductType             = TARGETED_RDR
        ProductCreationTime     = 2011-03-02T10:59:47
        ObservationType         = FRT
        ObservationId           = 16#0001CFD8#
        ObservationNumber       = 16#07#
        ActivityId              = IF124
        DetectorTemperature     = -53.687
        OpticalBenchTemperature = -41.003
        SpectrometerHousingTemp = -64.976
        SphereTemperature       = -41.062
        FpeTemperature          = 6.847
        ProductVersionId        = 3
        SoftwareName            = crism_imagecal
      End_Group

      Group = Kernels
        NaifIkCode = -74017
      End_Group
    End_Object
  End
  )");

  Pvl label;
  labelStrm >> label;

  QTemporaryFile tempFile;
  tempFile.open();
  Cube testCube;

  testCube.fromLabel(tempFile.fileName() + ".cub", label, "rw");

  QVector<QString> args(0);
  UserInterface options(APP_XML, args);
  spiceinit(&testCube, options);

  PvlGroup kernels = testCube.group("Kernels");
  ASSERT_TRUE(kernels.hasKeyword("InstrumentPointing"));
  PvlKeyword instrumentPointing = kernels["InstrumentPointing"];
  ASSERT_EQ(instrumentPointing.size(), 4);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, instrumentPointing[0], "Table");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, instrumentPointing[1], "$mro/kernels/ck/mro_crm_psp_110223_101128.bc");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, instrumentPointing[2], "$mro/kernels/ck/mro_sc_psp_110222_110228.bc");
  // Use a regex to match the version # for the frame kernel because this sometimes updates
  // when new MRO spice is released.
  QRegularExpression fkRegex("mro_v\\d\\d\\.tf");
  EXPECT_TRUE(fkRegex.match(QString::fromStdString(instrumentPointing[3])).hasMatch()) << "Frame kernel ["
      << instrumentPointing[3] << "] doesn't match regex ["
      << fkRegex.pattern().toStdString() << "].";
}


TEST(Spiceinit, TestSpiceinitDefault) {

  std::istringstream labelStrm(R"(
    Object = IsisCube
      Object = Core
        StartByte   = 65537
        Format      = Tile
        TileSamples = 128
        TileLines   = 128

        Group = Dimensions
          Samples = 1204
          Lines   = 1056
          Bands   = 1
        End_Group

        Group = Pixels
          Type       = UnsignedByte
          ByteOrder  = Lsb
          Base       = 0.0
          Multiplier = 1.0
        End_Group
      End_Object

      Group = Instrument
        SpacecraftName       = VIKING_ORBITER_1
        InstrumentId         = VISUAL_IMAGING_SUBSYSTEM_CAMERA_B
        TargetName           = MARS
        StartTime            = 1977-07-09T20:05:51
        ExposureDuration     = 0.008480 <seconds>
        SpacecraftClockCount = 33322515
        FloodModeId          = ON
        GainModeId           = HIGH
        OffsetModeId         = ON
      End_Group

      Group = Archive
        DataSetId       = VO1/VO2-M-VIS-2-EDR-V2.0
        ProductId       = 387A06
        MissonPhaseName = EXTENDED_MISSION
        ImageNumber     = 33322515
        OrbitNumber     = 387
      End_Group

      Group = BandBin
        FilterName = CLEAR
        FilterId   = 4
      End_Group

      Group = Kernels
        NaifFrameCode = -27002
      End_Group

      Group = Reseaus
        Line     = (5, 6, 8, 9, 10, 11, 12, 13, 14, 14, 15, 133, 134, 135, 137,
                    138, 139, 140, 141, 141, 142, 143, 144, 263, 264, 266, 267,
                    268, 269, 269, 270, 271, 272, 273, 393, 393, 395, 396, 397,
                    398, 399, 399, 400, 401, 402, 403, 523, 524, 525, 526, 527,
                    527, 528, 529, 530, 530, 532, 652, 652, 654, 655, 656, 657,
                    657, 658, 659, 660, 661, 662, 781, 783, 784, 785, 786, 787,
                    788, 788, 789, 790, 791, 911, 912, 913, 914, 915, 916, 917,
                    918, 918, 919, 920, 921, 1040, 1041, 1043, 1044, 1045, 1045,
                    1046, 1047, 1047, 1048, 1050)
        Sample   = (24, 142, 259, 375, 491, 607, 723, 839, 954, 1070, 1185, 24,
                    84, 201, 317, 433, 549, 665, 780, 896, 1011, 1127, 1183, 25,
                    142, 259, 375, 492, 607, 722, 838, 953, 1068, 1183, 25, 84,
                    201, 317, 433, 549, 665, 779, 895, 1010, 1125, 1182, 25, 143,
                    259, 375, 491, 607, 722, 837, 952, 1067, 1182, 25, 84, 201,
                    317, 433, 548, 664, 779, 894, 1009, 1124, 1181, 25, 142, 258,
                    374, 490, 605, 720, 835, 951, 1066, 1180, 24, 83, 200, 316,
                    431, 547, 662, 776, 892, 1007, 1122, 1179, 23, 140, 257, 373,
                    488, 603, 718, 833, 948, 1063, 1179)
        Type     = (1, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5,
                    5, 6, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 4, 5, 5, 5, 5, 5, 5, 5,
                    5, 5, 5, 6, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 4, 5, 5, 5, 5, 5,
                    5, 5, 5, 5, 5, 6, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 4, 5, 5, 5,
                    5, 5, 5, 5, 5, 5, 5, 6, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6)
        Valid    = (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        Template = $viking1/reseaus/vo1.visb.template.cub
        Status   = Nominal
      End_Group
    End_Object
  End
  )");

  Pvl label;
  labelStrm >> label;

  QTemporaryFile tempFile;
  tempFile.open();
  Cube testCube;

  testCube.fromLabel(tempFile.fileName() + ".cub", label, "rw");

  QVector<QString> args(0);
  UserInterface options(APP_XML, args);
  spiceinit(&testCube, options);

  PvlGroup kernels = testCube.group("Kernels");

  EXPECT_TRUE(kernels.hasKeyword("InstrumentPointing"));
  EXPECT_TRUE(kernels.hasKeyword("LeapSecond"));
  EXPECT_TRUE(kernels.hasKeyword("TargetAttitudeShape"));
  EXPECT_TRUE(kernels.hasKeyword("TargetPosition"));
  EXPECT_TRUE(kernels.hasKeyword("InstrumentPointing"));
  EXPECT_TRUE(kernels.hasKeyword("Instrument"));
  EXPECT_TRUE(kernels.hasKeyword("SpacecraftClock"));
  EXPECT_TRUE(kernels.hasKeyword("InstrumentPosition"));
  EXPECT_TRUE(kernels.hasKeyword("InstrumentAddendum"));
  EXPECT_TRUE(kernels.hasKeyword("ShapeModel"));
  EXPECT_TRUE(kernels.hasKeyword("InstrumentPositionQuality"));
  EXPECT_TRUE(kernels.hasKeyword("InstrumentPointingQuality"));
  EXPECT_TRUE(kernels.hasKeyword("CameraVersion"));

  spiceinit(&testCube, options);

  PvlGroup secondKernels = testCube.group("Kernels");

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, secondKernels, kernels);
}


TEST(Spiceinit, TestSpiceinitNadir) {

  std::istringstream labelStrm(R"(
    Object = IsisCube
      Object = Core
        StartByte   = 65537
        Format      = Tile
        TileSamples = 128
        TileLines   = 128

        Group = Dimensions
          Samples = 1536
          Lines   = 2688
          Bands   = 1
        End_Group

        Group = Pixels
          Type       = UnsignedByte
          ByteOrder  = Lsb
          Base       = 0.0
          Multiplier = 1.0
        End_Group
      End_Object

      Group = Instrument
        SpacecraftName        = "MARS GLOBAL SURVEYOR"
        InstrumentId          = MOC-NA
        TargetName            = Mars
        StartTime             = 2000-03-19T04:51:46.63
        StopTime              = 2000-03-19T04:51:47.92
        CrosstrackSumming     = 1
        DowntrackSumming      = 1
        FocalPlaneTemperature = 270.3
        GainModeId            = 0A
        LineExposureDuration  = 0.482100 <milliseconds>
        MissionPhaseName      = MAPPING
        OffsetModeId          = 38
        SpacecraftClockCount  = 637908733:72
        RationaleDesc         = "Sample of smooth plains in highlands "
        OrbitNumber           = 4604
        FirstLineSample       = 1
      End_Group

      Group = Archive
        DataSetId           = MGS-M-MOC-NA/WA-2-DSDP-L0-V1.0
        ProductId           = M13/01260
        ProducerId          = MGS_MOC_TEAM
        ProductCreationTime = 2001-03-01T03:00:38
        SoftwareName        = "makepds 1.9"
        UploadId            = UNK
        DataQualityDesc     = OK
        ImageNumber         = 07901260
        ImageKeyId          = 6379001260
      End_Group

      Group = BandBin
        FilterName   = BROAD_BAND
        OriginalBand = 1
        Center       = 0.7 <micrometers>
        Width        = 0.4 <micrometers>
      End_Group

      Group = Kernels
        NaifFrameCode = -94031
      End_Group
    End_Object
  End
  )");

  Pvl label;
  labelStrm >> label;

  QTemporaryFile tempFile;
  tempFile.open();
  Cube testCube;

  testCube.fromLabel(tempFile.fileName() + ".cub", label, "rw");

  QVector<QString> args = {"ckrecon=False", "cknadir=True", "tspk=$base/kernels/spk/de405.bsp", "attach=false"};
  UserInterface options(APP_XML, args);

  spiceinit(&testCube, options);

  PvlGroup kernels = testCube.group("Kernels");

  ASSERT_TRUE(kernels.hasKeyword("InstrumentPointing"));
  ASSERT_EQ(kernels["InstrumentPointing"].size(), 1);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, kernels["InstrumentPointing"][0], "Nadir");
}


TEST(Spiceinit, TestSpiceinitPadding) {

  std::istringstream labelStrm(R"(
    Object = IsisCube
      Object = Core
        StartByte   = 65537
        Format      = Tile
        TileSamples = 128
        TileLines   = 128

        Group = Dimensions
          Samples = 64
          Lines   = 64
          Bands   = 96
        End_Group

        Group = Pixels
          Type       = Real
          ByteOrder  = Lsb
          Base       = 0.0
          Multiplier = 1.0
        End_Group
      End_Object

      Group = Instrument
        SpacecraftName            = Cassini-Huygens
        InstrumentId              = VIMS
        Channel                   = VIS
        TargetName                = TITAN
        SpacecraftClockStartCount = 1545949489.188
        SpacecraftClockStopCount  = 1545950183.157
        StartTime                 = 2006-361T21:51:58.279
        StopTime                  = 2006-361T22:03:31.576
        NativeStartTime           = 1545949478.13981
        NativeStopTime            = 1545950172.02769
        InterlineDelayDuration    = 415.000000
        ExposureDuration          = (160.000000 <IR>, 5000.000000 <VIS>)
        SamplingMode              = NORMAL
        XOffset                   = 1
        ZOffset                   = 1
        SwathWidth                = 64
        SwathLength               = 64
      End_Group

      Group = BandBin
        OriginalBand = (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
                        17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
                        31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
                        45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,
                        59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72,
                        73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86,
                        87, 88, 89, 90, 91, 92, 93, 94, 95, 96)
        Center       = (0.35054, 0.35895, 0.36629, 0.37322, 0.37949, 0.38790,
                        0.39518, 0.40252, 0.40955, 0.41731, 0.42436, 0.43184,
                        0.43919, 0.44652, 0.45372, 0.46163, 0.46841, 0.47622,
                        0.48629, 0.48967, 0.49777, 0.50628, 0.51222, 0.51963,
                        0.52766, 0.53416, 0.54156, 0.54954, 0.55614, 0.56353,
                        0.57131, 0.57810, 0.58548, 0.59312, 0.59938, 0.60757,
                        0.61505, 0.62207, 0.62940, 0.63704, 0.64408, 0.65142,
                        0.65910, 0.66609, 0.67342, 0.68102, 0.68803, 0.69535,
                        0.70288, 0.71000, 0.71733, 0.72484, 0.73198, 0.73930,
                        0.74676, 0.75396, 0.76128, 0.76874, 0.77595, 0.78328,
                        0.79072, 0.79793, 0.80522, 0.81262, 0.81989, 0.82721,
                        0.83463, 0.84190, 0.84922, 0.85663, 0.86391, 0.87122,
                        0.87863, 0.88589, 0.89386, 0.90032, 0.90787, 0.91518,
                        0.92254, 0.92983, 0.93713, 0.94445, 0.95177, 0.95907,
                        0.96638, 0.97382, 0.98100, 0.98883, 0.99588, 1.00295,
                        1.01005, 1.01695, 1.02471, 1.03195, 1.03865, 1.04598)
      End_Group

      Group = Kernels
        NaifFrameCode = -82370
      End_Group
    End_Object
  End
  )");

  Pvl label;
  labelStrm >> label;

  QTemporaryFile tempFile;
  tempFile.open();
  Cube testCube;

  testCube.fromLabel(tempFile.fileName() + ".cub", label, "rw");

  QVector<QString> args = {"startpad=1.1", "endpad=0.5", "fk=$cassini/kernels/fk/cas_v40_usgs.tf", "attach=false"};
  UserInterface options(APP_XML, args);

  spiceinit(&testCube, options);

  PvlGroup kernels = testCube.group("Kernels");

  ASSERT_TRUE(kernels.hasKeyword("StartPadding"));
  ASSERT_EQ(kernels["StartPadding"].size(), 1);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, kernels["StartPadding"][0], "1.1");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, kernels["StartPadding"].unit(0), "seconds");

  ASSERT_TRUE(kernels.hasKeyword("EndPadding"));
  ASSERT_EQ(kernels["EndPadding"].size(), 1);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, kernels["EndPadding"][0], "0.5");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, kernels["EndPadding"].unit(0), "seconds");
}

TEST_F(DefaultCube, TestSpiceinitCsmCleanup) {
  // Add stuff from csminit
  testCube->putGroup(PvlGroup("CsmInfo"));
  Blob testBlob("CSMState", "String");
  testCube->write(testBlob);

  QVector<QString> args(0);
  UserInterface options(APP_XML, args);
  spiceinit(testCube, options);

  EXPECT_FALSE(testCube->hasGroup("CsmInfo"));
  EXPECT_FALSE(testCube->hasBlob("CSMState", "String"));
}

TEST_F(DefaultCube, TestSpiceinitCsmNoCleanup) {
  // Add stuff from csminit
  testCube->putGroup(PvlGroup("CsmInfo"));
  Blob testBlob("CSMState", "String");
  testCube->write(testBlob);

  // Mangle the cube so that spiceinit failes
  testCube->deleteGroup("Instrument");

  QVector<QString> args(0);
  UserInterface options(APP_XML, args);
  ASSERT_ANY_THROW(spiceinit(testCube, options));

  EXPECT_TRUE(testCube->hasGroup("CsmInfo"));
  EXPECT_TRUE(testCube->hasBlob("CSMState", "String"));
}

TEST_F(DemCube, FunctionalTestSpiceinitWebAndShapeModel) {

  std::istringstream labelStrm(R"(
    Object = IsisCube
      Object = Core
        StartByte   = 65537
        Format      = Tile
        TileSamples = 128
        TileLines   = 128

        Group = Dimensions
          Samples = 1204
          Lines   = 1056
          Bands   = 1
        End_Group

        Group = Pixels
          Type       = UnsignedByte
          ByteOrder  = Lsb
          Base       = 0.0
          Multiplier = 1.0
        End_Group
      End_Object

      Group = Instrument
        SpacecraftName       = VIKING_ORBITER_1
        InstrumentId         = VISUAL_IMAGING_SUBSYSTEM_CAMERA_B
        TargetName           = MARS
        StartTime            = 1977-07-09T20:05:51
        ExposureDuration     = 0.008480 <seconds>
        SpacecraftClockCount = 33322515
        FloodModeId          = ON
        GainModeId           = HIGH
        OffsetModeId         = ON
      End_Group

      Group = Archive
        DataSetId       = VO1/VO2-M-VIS-2-EDR-V2.0
        ProductId       = 387A06
        MissonPhaseName = EXTENDED_MISSION
        ImageNumber     = 33322515
        OrbitNumber     = 387
      End_Group

      Group = BandBin
        FilterName = CLEAR
        FilterId   = 4
      End_Group

      Group = Kernels
        NaifFrameCode = -27002
      End_Group

      Group = Reseaus
        Line     = (5, 6, 8, 9, 10, 11, 12, 13, 14, 14, 15, 133, 134, 135, 137,
                    138, 139, 140, 141, 141, 142, 143, 144, 263, 264, 266, 267,
                    268, 269, 269, 270, 271, 272, 273, 393, 393, 395, 396, 397,
                    398, 399, 399, 400, 401, 402, 403, 523, 524, 525, 526, 527,
                    527, 528, 529, 530, 530, 532, 652, 652, 654, 655, 656, 657,
                    657, 658, 659, 660, 661, 662, 781, 783, 784, 785, 786, 787,
                    788, 788, 789, 790, 791, 911, 912, 913, 914, 915, 916, 917,
                    918, 918, 919, 920, 921, 1040, 1041, 1043, 1044, 1045, 1045,
                    1046, 1047, 1047, 1048, 1050)
        Sample   = (24, 142, 259, 375, 491, 607, 723, 839, 954, 1070, 1185, 24,
                    84, 201, 317, 433, 549, 665, 780, 896, 1011, 1127, 1183, 25,
                    142, 259, 375, 492, 607, 722, 838, 953, 1068, 1183, 25, 84,
                    201, 317, 433, 549, 665, 779, 895, 1010, 1125, 1182, 25, 143,
                    259, 375, 491, 607, 722, 837, 952, 1067, 1182, 25, 84, 201,
                    317, 433, 548, 664, 779, 894, 1009, 1124, 1181, 25, 142, 258,
                    374, 490, 605, 720, 835, 951, 1066, 1180, 24, 83, 200, 316,
                    431, 547, 662, 776, 892, 1007, 1122, 1179, 23, 140, 257, 373,
                    488, 603, 718, 833, 948, 1063, 1179)
        Type     = (1, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5,
                    5, 6, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 4, 5, 5, 5, 5, 5, 5, 5,
                    5, 5, 5, 6, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 4, 5, 5, 5, 5, 5,
                    5, 5, 5, 5, 5, 6, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 4, 5, 5, 5,
                    5, 5, 5, 5, 5, 5, 5, 6, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6)
        Valid    = (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        Template = $viking1/reseaus/vo1.visb.template.cub
        Status   = Nominal
      End_Group
    End_Object
  End
  )");

  Pvl label;
  labelStrm >> label;

  QTemporaryFile tempFile;
  tempFile.open();
  Cube testCube;

  testCube.fromLabel(tempFile.fileName() + ".cub", label, "rw");

  QVector<QString> args = {"web=true", "shape=user", "model=" + demCube->fileName()};
  UserInterface options(APP_XML, args);
  spiceinit(&testCube, options);

  PvlGroup kernels = testCube.label()->findGroup("Kernels", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, kernels.findKeyword("ShapeModel"), demCube->fileName().toStdString());
}


TEST_F(SmallCube, FunctionalTestSpiceinitCsminitRestorationOnFail) {
  // csminit the cube

  // Create an ISD
  json isd;
  isd["reference_time"] = 0;
  isd["center_latitude"] = 3.03125;
  isd["center_longitude"] = -2.9375;
  isd["scale"] = 240;
  isd["center_longitude_sigma"] = 0.0645181963189456;
  isd["center_latitude_sigma"] = 0.0645181963189456;
  isd["scale_sigma"] = 8.25832912882503;
  QString isdPath = tempDir.path() + "/default.json";
  std::ofstream file(isdPath.toStdString());
  file << isd;
  file.flush();

  QString cubeFile = testCube->fileName();

  QVector<QString> csmArgs = {
    "from="+cubeFile,
    "isd="+isdPath};

  QString CSMINIT_APP_XML = FileName("$ISISROOT/bin/xml/csminit.xml").expanded();
  UserInterface csmOptions(CSMINIT_APP_XML, csmArgs);
  testCube->close();

  ASSERT_NO_THROW(csminit(csmOptions));
  testCube->open(cubeFile);
  PvlGroup csmInfoGroup = testCube->group("CsmInfo");
  testCube->close();

  // spiceinit
  QVector<QString> spiceinitArgs = {"from="+cubeFile};

  UserInterface spiceinitOptions(APP_XML, spiceinitArgs);
  ASSERT_ANY_THROW(spiceinit(spiceinitOptions));

  Cube outputCube(cubeFile);

  ASSERT_NO_THROW(outputCube.camera());
  EXPECT_TRUE(outputCube.hasBlob("CSMState", "String"));
  ASSERT_TRUE(outputCube.hasGroup("CsmInfo"));
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, csmInfoGroup, outputCube.group("CsmInfo"));
}


TEST(Spiceinit, TestSpiceinitHrscWebError) {

  std::istringstream labelStrm(R"(
    Object = IsisCube
      Object = Core
        StartByte   = 65537
        Format      = Tile
        TileSamples = 323
        TileLines   = 409

        Group = Dimensions
          Samples = 2584
          Lines   = 19632
          Bands   = 1
        End_Group

        Group = Pixels
          Type       = Real
          ByteOrder  = Lsb
          Base       = 0.0
          Multiplier = 1.0
        End_Group
      End_Object

      Group = Instrument
        SpacecraftName            = "MARS EXPRESS"
        InstrumentId              = HRSC
        StartTime                 = 2019-01-16T13:11:27.926
        StopTime                  = 2019-01-16T13:13:37.796
        SpacecraftClockStartCount = 1/0495724211.25691
        SpacecraftClockStopCount  = 1/0495724397.63935
        MissionPhaseName          = ME_Phase_40
        TargetName                = Mars
        Summing                   = 2
        FocalPlaneTemperature     = 8.9911 <degC>
        LensTemperature           = 9.6028 <degC>
        InstrumentTemperature     = 11.8457 <degC>
      End_Group

      Group = Archive
        DataSetId   = MEX-M-HRSC-3-RDR-V3.0
        DetectorId  = MEX_HRSC_P1
        EventType   = MARS-REGIONAL-MAPPING-Im-Gl-Pf
        OrbitNumber = 19029
        ProductId   = HJ029_0000_P12.IMG
      End_Group

      Group = BandBin
        Width  = 174.0 <nm>
        Center = 675.0 <nm>
      End_Group

      Group = Kernels
        NaifIkCode = -41217
      End_Group
    End_Object
  End
  )");

  Pvl label;
  labelStrm >> label;

  QTemporaryFile tempFile;
  tempFile.open();
  Cube testCube;

  testCube.fromLabel(tempFile.fileName() + ".cub", label, "rw");

  QVector<QString> args = {"web=true"};
  UserInterface options(APP_XML, args);
  

  try {
    spiceinit(&testCube, options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("Spice Server does not support MEX HRSC images. Please rerun spiceinit with local MEX data."));
  }
}
