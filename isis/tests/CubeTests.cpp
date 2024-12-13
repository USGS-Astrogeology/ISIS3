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


void check_cube(Cube &cube,
                QString &file, 
                int sampleCount, 
                int lineCount, 
                int bandCount, 
                double base, 
                double multiplier, 
                int pixelType, 
                bool attached, 
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
  EXPECT_EQ(cube.labelsAttached(), attached);
  EXPECT_EQ(cube.format(), format);
  EXPECT_EQ(cube.isOpen(), isOpen);
  if (cube.isOpen()) {
    EXPECT_EQ(cube.isReadOnly(), isReadOnly);
    EXPECT_EQ(cube.isReadWrite(), isReadWrite);
  }
  EXPECT_EQ(cube.labelSize(), labelSize);
}

TEST(CubeTest, TestCubeAttachSpiceFromIsd) {
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
        NaifFrameCode             = -27002
        LeapSecond                = $base/kernels/lsk/naif0012.tls
        TargetAttitudeShape       = $base/kernels/pck/pck00009.tpc
        TargetPosition            = (Table, $base/kernels/spk/de430.bsp,
                                     $base/kernels/spk/mar097.bsp)
        InstrumentPointing        = (Table, $viking1/kernels/ck/vo1_sedr_ck2.bc,
                                     $viking1/kernels/fk/vo1_v10.tf)
        Instrument                = Null
        SpacecraftClock           = ($viking1/kernels/sclk/vo1_fict.tsc,
                                     $viking1/kernels/sclk/vo1_fsc.tsc)
        InstrumentPosition        = (Table, $viking1/kernels/spk/viking1a.bsp)
        InstrumentAddendum        = $viking1/kernels/iak/vikingAddendum003.ti
        ShapeModel                = $base/dems/molaMarsPlanetaryRadius0005.cub
        InstrumentPositionQuality = Reconstructed
        InstrumentPointingQuality = Reconstructed
        CameraVersion             = 1
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


  json isd = json::parse(R"(
    {"isis_camera_version": 2,
       "naif_keywords": {
            "BODY_CODE" : 499,
            "BODY499_RADII" : [3396.19, 3396.19, 3376.2],
            "BODY_FRAME_CODE" : 10014,
            "CLOCK_ET_-27999_33322515_COMPUTED" : "d5b9203a4d24c5c1",
            "INS-27002_TRANSX" : [0.0, 0.011764705882353, 0.0],
            "INS-27002_TRANSY" : [0.0, 0.0, 0.01176470588235],
            "INS-27002_ITRANSS" : [0.0, 85.0, 0.0],
            "INS-27002_ITRANSL" : [0.0, 0.0, 85.0]
      },
    "instrument_pointing": {
      "time_dependent_frames": [-85600, -85000, 1],
      "ck_table_start_time": 100,
      "ck_table_end_time": 100.1,
      "ck_table_original_size": 2,
      "ephemeris_times": [
        100,
        100.1
      ],
      "quaternions": [
        [0.0, -0.660435174378928, 0, 0.750883067090392],
        [0.0, -0.660435174378928, 0, 0.750883067090392]
      ],
      "angular_velocity": [
        [0, 0, 0],
        [0, 0, 0]
      ],
      "constant_frames": [-85600],
      "constant_rotation": [1, 0, 0, 0, 1, 0, 0, 0, 1]
    },
    "body_rotation": {
      "time_dependent_frames": [31006, 1],
      "ck_table_start_time": 100,
      "ck_table_end_time": 100.1,
      "ck_table_original_size": 2,
      "ephemeris_times": [
        100,
        100.1
      ],
      "quaternions": [
        [ 0, 0.8509035, 0, 0.525322 ],
        [ 0, 0.8509035, 0, 0.525322 ]
      ],
      "angular_velocity": [
        [0, 0, 0],
        [0, 0, 0]
      ],
      "constant_frames": [31001, 31007, 31006],
      "constant_rotation": [-0.4480736,  0,  0.8939967, 0,  1,  0, -0.8939967,  0, -0.4480736]
    },
    "instrument_position": {
      "spk_table_start_time": 100,
      "spk_table_end_time": 100.1,
      "spk_table_original_size": 2,
      "ephemeris_times": [
        100,
        100.1
      ],
      "positions": [
        [1000, 0, 0],
        [1000, 0, 0]
      ],
      "velocities": [
        [0, 0, 0],
        [0, 0, 0]
      ]
    },
    "sun_position": {
      "spk_table_start_time": 100,
      "spk_table_end_time": 100.1,
      "spk_table_original_size": 2,
      "ephemeris_times": [
        100
      ],
      "positions": [
        [0, 20, 0]
      ],
      "velocities": [
        [10,10,10]
      ]
    }
  })");

  Pvl label;
  labelStrm >> label;

  QTemporaryFile tempFile;
  Cube testCube;
  testCube.fromIsd(tempFile.fileName() + ".cub", label, isd, "rw");

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

  Camera *cam = testCube.camera();

  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameLong(), "Visual Imaging Subsystem Camera B");
}

TEST_F(SmallCube, TestCubeHasBlob) {
  Blob testBlob("TestBlob", "SomeBlob");
  testCube->write(testBlob);
  EXPECT_TRUE(testCube->hasBlob("TestBlob", "SomeBlob"));
  EXPECT_FALSE(testCube->hasBlob("SomeOtherTestBlob", "SomeBlob"));
}

TEST_F(TempTestingFiles, TestCubeCreateWriteCopy) {
  Cube out;
  QString file = "";
  check_cube(out, file, 0, 0, 0, 0, 1, 7, 1, 1, 0, 0, 0, 65536);
  out.setDimensions(150, 200, 2);
  file = QString(tempDir.path() + "/IsisCube_00.cub");
  out.create(file);
  check_cube(out, file, 150, 200, 2, 0, 1, 7, 1, 1, 1, 0, 1, 65536);

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
  QString file2 = tempDir.path() + "/IsisCube_01.cub";
  delete out.copy(file2, CubeAttributeOutput());
  out.close();

  // Test the open and read methods
  Cube in(file2);
  check_cube(in, file2, 150, 200, 2, 0, 1, 7, 1, 1, 1, 1, 0, 6563);

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

TEST_F(TempTestingFiles, TestCubeCreateWrite8bit) {
  Cube out;
  QString file = "";
  check_cube(out, file, 0, 0, 0, 0, 1, 7, 1, 1, 0, 0, 0, 65536);
  out.setDimensions(150, 200, 1);
  out.setLabelsAttached(0);
  out.setBaseMultiplier(200.0, -1.0);
  out.setByteOrder(ISIS_LITTLE_ENDIAN ? Msb : Lsb);
  out.setFormat(Cube::Bsq);
  out.setLabelSize(1000);
  out.setPixelType(UnsignedByte);
  check_cube(out, file, 150, 200, 1, 200, -1, 1, 0, 0, 0, 0, 0, 1000);
  file = QString(tempDir.path() + "/IsisCube_00.cub");
  out.create(file);

  long j = 0;
  LineManager oline(out);
  for(oline.begin(); !oline.end(); oline++) {
    for(int i = 0; i < oline.size(); i++) {
      oline[i] = (double) j;
    }
    out.clearIoCache();
    out.write(oline);
    j++;
  }
  out.close();

  Cube in;
  file = QString(tempDir.path() + "/IsisCube_00.lbl");
  try {
    in.open(file);
  }
  catch (IException &e) {
    e.print();
  }
  check_cube(in, file, 150, 200, 1, 200, -1, 1, 0, 0, 1, 1, 0, 419);
  j = 0;
  LineManager inLine(in);
  for(inLine.begin(); !inLine.end(); inLine++) {
    in.read(inLine );
    for(int i = 0; i < inLine.size(); i++) {
      EXPECT_NEAR(inLine[i], (double) j, 1e-15);
    }
    in.clearIoCache();
    j++;
  }
  in.close();
}

TEST_F(TempTestingFiles, TestCubeCreateWrite16bit) {
  Cube out;
  QString file = "";
  check_cube(out, file, 0, 0, 0, 0, 1, 7, 1, 1, 0, 0, 0, 65536);
  out.setDimensions(150, 200, 2);
  out.setBaseMultiplier(30000.0, -1.0);
  out.setByteOrder(ISIS_LITTLE_ENDIAN ? Msb : Lsb);
  out.setPixelType(SignedWord);
  check_cube(out, file, 150, 200, 2, 30000, -1, 4, 1, 1, 0, 0, 0, 65536);
  file = QString(tempDir.path() + "/IsisCube.cub");
  out.create(file);

  long j = 0;
  LineManager oline(out);
  for(oline.begin(); !oline.end(); oline++) {
    for(int i = 0; i < oline.size(); i++) {
      oline[i] = (double) j;
    }
    out.clearIoCache();
    out.write(oline);
    j++;
  }
  out.close();

  Cube in;
  try {
    in.open(file);
  }
  catch (IException &e) {
    e.print();
  }
  check_cube(in, file, 150, 200, 2, 30000, -1, 4, 1, 1, 1, 1, 0, 65536);
  j = 0;
  LineManager inLine(in);
  for(inLine.begin(); !inLine.end(); inLine++) {
    in.read(inLine );
    for(int i = 0; i < inLine.size(); i++) {
      EXPECT_NEAR(inLine[i], (double) j, 1e-15);
    }
    in.clearIoCache();
    j++;
  }
  in.close();
}

TEST_F(SmallCube, TestCubeHistorgramBand1) {
  Histogram *bandHist = testCube->histogram(1);
  EXPECT_DOUBLE_EQ(bandHist->Average(), 49.5);
  EXPECT_DOUBLE_EQ(bandHist->StandardDeviation(), 29.011491975882016);
  EXPECT_DOUBLE_EQ(bandHist->Mode(), 0);
  EXPECT_DOUBLE_EQ(bandHist->TotalPixels(), 100);
  EXPECT_DOUBLE_EQ(bandHist->NullPixels(), 0);
  delete bandHist;
}

TEST_F(SmallCube, TestCubeHistorgramAll) {
  Histogram *bandHist = testCube->histogram(0);
  EXPECT_DOUBLE_EQ(bandHist->Average(), 499.5);
  EXPECT_DOUBLE_EQ(bandHist->StandardDeviation(), 288.81943609574938);
  EXPECT_DOUBLE_EQ(bandHist->Mode(), 0);
  EXPECT_DOUBLE_EQ(bandHist->TotalPixels(), 1000);
  EXPECT_DOUBLE_EQ(bandHist->NullPixels(), 0);
  delete bandHist;
}

TEST_F(SmallCube, TestCubeStatisticsBand1) {
  Statistics *bandHist = testCube->statistics(1);
  EXPECT_DOUBLE_EQ(bandHist->Average(), 49.5);
  EXPECT_DOUBLE_EQ(bandHist->StandardDeviation(), 29.011491975882016);
  EXPECT_DOUBLE_EQ(bandHist->TotalPixels(), 100);
  EXPECT_DOUBLE_EQ(bandHist->NullPixels(), 0);
  delete bandHist;
}

TEST_F(SmallCube, TestCubeStatisticsAll) {
  Statistics *bandHist = testCube->statistics(0);
  EXPECT_DOUBLE_EQ(bandHist->Average(), 499.5);
  EXPECT_DOUBLE_EQ(bandHist->StandardDeviation(), 288.81943609574938);
  EXPECT_DOUBLE_EQ(bandHist->TotalPixels(), 1000);
  EXPECT_DOUBLE_EQ(bandHist->NullPixels(), 0);
  delete bandHist;
}

TEST_F(SmallCube, TestCubeNegativeHistStatsBand) {
  try {
    testCube->histogram(-1);
    FAIL() << "Expected an exception to be thrown";
  }
  catch (IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Invalid band in [CubeInfo::Histogram]"))
      << e.toString().toStdString();
  }

  try {
    testCube->statistics(-1);
    FAIL() << "Expected an exception to be thrown";
  }
  catch (IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Invalid band in [CubeInfo::Statistics]"))
      << e.toString().toStdString();
  }
}

TEST(CubeTest, TestCubeNoHistStats) {
  Cube cube;
  try {
    cube.histogram();
    FAIL() << "Expected an exception to be thrown";
  }
  catch (IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Cannot create histogram object for an unopened cube"))
      << e.toString().toStdString();
  }

  try {
    cube.statistics();
    FAIL() << "Expected an exception to be thrown";
  }
  catch (IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Cannot create statistics object for an unopened cube"))
      << e.toString().toStdString();
  }
}

TEST_F(SmallCube, TestCubePhyscialBands) {
  EXPECT_EQ(testCube->bandCount(), 10);
  for (int i = 0; i < testCube->bandCount(); i++) {
    EXPECT_EQ(testCube->physicalBand(i), i);
  }
}

TEST_F(SmallCube, TestCubeVirutalBands) {
  QString path = testCube->fileName();
  testCube->close();
  QList<QString> vbands = {"2", "3", "4", "5", "6", "7", "8", "9", "10"};
  testCube->setVirtualBands(vbands);
  testCube->open(path);
  EXPECT_EQ(testCube->bandCount(), 9);
  for (int i = 1; i < vbands.size() + 1; i++) {
    EXPECT_EQ(testCube->physicalBand(i), i + 1);
  }
}

TEST_F(SmallCube, TestCubeReopenRW) {
  QString path = testCube->fileName();
  check_cube(*testCube, path, 10, 10, 10, 0, 1, 7, 1, 1, 1, 0, 1, 65536);
  testCube->reopen("rw");
  check_cube(*testCube, path, 10, 10, 10, 0, 1, 7, 1, 1, 1, 0, 1, 65536);
}

TEST_F(SmallCube, TestCubeReopenR) {
  QString path = testCube->fileName();
  check_cube(*testCube, path, 10, 10, 10, 0, 1, 7, 1, 1, 1, 0, 1, 65536);
  testCube->reopen("r");
  check_cube(*testCube, path, 10, 10, 10, 0, 1, 7, 1, 1, 1, 1, 0, 65536);
}

TEST_F(SmallCube, TestCubeAlreadyOpenOpen) {
  try {
    testCube->open("blah");
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("You already have a cube opened."))
      << e.toString().toStdString();
  }
}

TEST_F(SmallCube, TestCubeAlreadyOpenCreate) {
  try {
    testCube->create("blah");
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("You already have a cube opened."))
      << e.toString().toStdString();
  }
}

TEST_F(TempTestingFiles, TestCubeWriteToReadOnly) {
  Cube testCube;
  QString file = QString(tempDir.path() + "/IsisCube_00.cub");
  testCube.setDimensions(10, 10, 1);
  testCube.create(file);
  testCube.close();
  testCube.open(file, "r");
  LineManager line(testCube);
  double pixelValue = 0.0;
  for(int i = 0; i < line.size(); i++) {
    line[i] = (double) pixelValue++;
  }

  try {
    testCube.write(line);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Cannot write to the cube [IsisCube_00.cub] because it is opened read-only."))
      << e.toString().toStdString();
  }
}

TEST(CubeTest, TestCubeOpenNonexistentCube) {
  try {
    Cube testCube;
    testCube.open("blah");
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Unable to open [blah]."))
      << e.toString().toStdString();
  }
}

TEST_F(SmallCube, TestCubePhyscialBandOutOfBounds) {
  QString path = testCube->fileName();
  testCube->close();
  QList<QString> vbands = {"1", "2", "3", "4", "5"};
  testCube->setVirtualBands(vbands);
  testCube->open(path);
  try {
    std::cout << testCube->bandCount() << std::endl;
    int band = testCube->physicalBand(6);
    std::cout << band << std::endl;
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Out of array bounds [6]."))
      << e.toString().toStdString();
  }

  try {
    testCube->physicalBand(0);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Out of array bounds [0]."))
      << e.toString().toStdString();
  }
}

TEST(CubeTest, TestCubeReadBlankCube) {
  Cube testCube;
  testCube.setDimensions(10, 10, 1);
  LineManager line(testCube);
  try {
    testCube.read(line);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Try opening a file before you read it."))
      << e.toString().toStdString();
  }
}

TEST(CubeTest, TestCubeWriteBlankCube) {
  Cube testCube;
  testCube.setDimensions(10, 10, 1);
  LineManager line(testCube);
  double pixelValue = 0.0;
  for(int i = 0; i < line.size(); i++) {
    line[i] = (double) pixelValue++;
  }
  try {
    testCube.write(line);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Tried to write to a cube before opening/creating it."))
      << e.toString().toStdString();
  }
}

TEST_F(TempTestingFiles, TestCubeCreateZeroDimCube) {
  Cube testCube;
  try {
    testCube.create(tempDir.path() + "/IsisCube_00.cub");
    testCube.close();
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Number of samples [0], lines [0], or bands [0] cannot be less than 1."))
      << e.toString().toStdString();
  }
}

TEST_F(TempTestingFiles, TestCubeCreateSmallLabel) {
  Cube testCube;
  try {
    testCube.setLabelSize(15);
    testCube.setDimensions(1, 1, 1);
    testCube.create(tempDir.path() + "/IsisCube_00.cub");
    testCube.close();
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Label space is full in [IsisCube_00.cub] unable to write labels."))
      << e.toString().toStdString();
  }
}

TEST_F(TempTestingFiles, TestCubeCreateTooBig) {
  Cube testCube;
  try {
    testCube.setDimensions(1000000, 1000000, 9);
    testCube.create("IsisCube_00.cub");
    testCube.close();
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("The cube you are attempting to create [IsisCube_00.cub] is [33527GB]. "
                                                  "This is larger than the current allowed size of [12GB]. The cube "
                                                  "dimensions were (S,L,B) [1000000, 1000000, 9] with [4] bytes per pixel. "
                                                  "If you still wish to create this cube, the maximum value can be changed "
                                                  "in your personal preference file located in [~/.Isis/IsisPreferences] "
                                                  "within the group CubeCustomization, keyword MaximumSize. If you do not have "
                                                  "an ISISPreference file, please refer to the documentation 'Environment and Preference Setup'. Error."))
      << e.toString().toStdString();
  }
}

TEST_F(SmallCube, TestCubeOpenBadAccessor) {
  try {
    Cube localTestCube;
    localTestCube.open(testCube->fileName(), "a");
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Unknown value for access [a]. Expected 'r'  or 'rw'."))
      << e.toString().toStdString();
  }
}

TEST(CubeTest, TestCubeInvalidDimSamples) {
  try {
    Cube testCube;
    testCube.setDimensions(0, 1, 1);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("SetDimensions:  Invalid number of sample, lines or bands."))
      << e.toString().toStdString();
  }
}

TEST(CubeTest, TestCubeInvalidDimLines) {
  try {
    Cube testCube;
    testCube.setDimensions(1, 0, 1);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("SetDimensions:  Invalid number of sample, lines or bands."))
      << e.toString().toStdString();
  }
}

TEST(CubeTest, TestCubeInvalidDimBands) {
  try {
    Cube testCube;
    testCube.setDimensions(1, 1, 0);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("SetDimensions:  Invalid number of sample, lines or bands."))
      << e.toString().toStdString();
  }
}

TEST(CubeTest, TestCubeNonePixelType) {
  try {
    Cube testCube;
    testCube.setPixelType(None);
    testCube.setDimensions(1, 1, 1);
    testCube.create("IsisCube_00.cub");
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Cannot create the cube [IsisCube_00.cub] with a pixel type set to None."))
      << e.toString().toStdString();
  }
}

TEST_F(TempTestingFiles, TestCubeAddGroupReadOnly) {
  try {
    Cube testCube;
    testCube.setDimensions(1024, 1024, 1);
    testCube.create(tempDir.path() + "/IsisCube_00.cub");
    testCube.reopen("r");
    testCube.putGroup(PvlGroup("TestGroup2"));
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Cannot add a group to the label of cube [IsisCube_00.cub] because it is opened read-only."))
      << e.toString().toStdString();
  }
}

TEST_F(SmallCube, TestCubeCreateECube) {
  Cube localTestCube;
  QString path = testCube->fileName();
  localTestCube.setExternalDnData(path);
  localTestCube.create(tempDir.path() + "/isisTruth_external.ecub");
  localTestCube.putGroup(PvlGroup("TestGroup"));
  EXPECT_EQ(localTestCube.realDataFileName().expanded(), path);
  EXPECT_TRUE(localTestCube.hasGroup("TestGroup"));

  path = localTestCube.fileName();
  check_cube(localTestCube, path, 10, 10, 10, 0, 1, 7, 2, 1, 1, 0, 1, 65536);
}

class ECube : public SmallCube {
    protected:
      Cube * testECube;

        void SetUp() override {
            SmallCube::SetUp();
            testECube = new Cube();
            testECube->setExternalDnData(testCube->fileName());
            QString path = tempDir.path() + "/external.ecub";
            testECube->create(path);

            testECube->close();
            testECube->open(path, "rw");
    }

      void TearDown() override {
        if (testECube->isOpen()) {
            testECube->close();
        }

        if (testECube) {
            delete testECube;
        }
        SmallCube::TearDown();
    }
};

TEST_F(ECube, TestCubeCreateECubeFromECube) {
  QString path1 = testECube->fileName();

  Cube localTestCube;
  localTestCube.setExternalDnData(path1);
  QString path2 = tempDir.path() + "/isisTruth_external2.ecub";
  localTestCube.create(path2);
  EXPECT_EQ(localTestCube.realDataFileName().expanded(), testCube->fileName());

  check_cube(localTestCube, path2, 10, 10, 10, 0, 1, 7, 2, 1, 1, 0, 1, 65536);
}

TEST_F(ECube, TestCubeECubeRead) {
    Brick readBrick(3, 3, 2, testECube->pixelType());
    readBrick.SetBasePosition(1, 1, 1);
    testECube->read(readBrick);
    std::vector<int> truth = {0, 1, 2, 10, 11, 12, 20, 21, 22, 100, 101, 102, 110, 111, 112, 120, 121, 122};
    for (int index = 0; index < readBrick.size(); index++) {
      EXPECT_EQ(readBrick[index], truth[index]);
    }
}

TEST_F(ECube, TestCubeECubeWrite) {
    Brick writeBrick(3, 3, 2, testECube->pixelType());
    writeBrick.SetBasePosition(1, 1, 1);
    for (int index = 0; index < writeBrick.size(); index++) {
      writeBrick[index] = 1.0;
    }
    try {
      testECube->write(writeBrick);
      FAIL();
    }
    catch(IException &e) {
      EXPECT_TRUE(e.toString().toLatin1().contains("The cube [external.ecub] does not support storing DN data because it is using an external file for DNs."))
        << e.toString().toStdString();
    }
}

TEST_F(ECube, TestCubeECubeFromECubeRead) {
    QString path1 = testECube->fileName();

    Cube localTestCube;
    localTestCube.setExternalDnData(path1);
    QString path2 = tempDir.path() + "/isisTruth_external2.ecub";
    localTestCube.create(path2);

    Brick readBrick(3, 3, 2, localTestCube.pixelType());
    readBrick.SetBasePosition(1, 1, 1);
    localTestCube.read(readBrick);
    std::vector<int> truth = {0, 1, 2, 10, 11, 12, 20, 21, 22, 100, 101, 102, 110, 111, 112, 120, 121, 122};
    for (int index = 0; index < readBrick.size(); index++) {
      EXPECT_EQ(readBrick[index], truth[index]);
    }
}
