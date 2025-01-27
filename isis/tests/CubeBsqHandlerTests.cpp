#include <QFile>
#include <QString>
#include <iostream>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "Brick.h"
#include "CubeIoHandler.h"
#include "CubeBsqHandler.h"
#include "CubeTileHandler.h"
#include "LineManager.h"
#include "SpecialPixel.h"

#include "CubeFixtures.h"
#include "TestUtilities.h"

#include "gmock/gmock.h"

using namespace Isis;

TEST_F(SmallCube, TestCubeBsqHandlerRead) {
    QString path = testCube->fileName();
    QFile dataFile(path);
    if (!dataFile.open(QIODevice::ReadWrite)) {
        FAIL() << "Unable to open dataFile in ReadWrite";
    }
    CubeBsqHandler ioHandler(&dataFile, nullptr, *testCube->label(), true);
    Brick readBrick(1, 1, 2, testCube->pixelType());
    readBrick.SetBasePosition(1, 1, 1);
    ioHandler.read(readBrick);

    EXPECT_EQ(readBrick.size(), 2);
    EXPECT_EQ(readBrick[0], 0);
    EXPECT_EQ(readBrick[1], 100);
}

TEST_F(SmallCube, TestCubeBsqHandlerReadPastBoundary) {
    QString path = testCube->fileName();
    QFile dataFile(path);
    if (!dataFile.open(QIODevice::ReadWrite)) {
        FAIL() << "Unable to open dataFile in ReadWrite";
    }
    CubeBsqHandler ioHandler(&dataFile, nullptr, *testCube->label(), true);
    Brick readBrick(1, 1, 2, testCube->pixelType());
    readBrick.SetBasePosition(1, 1, -1);
    ioHandler.read(readBrick);

    EXPECT_EQ(readBrick.size(), 2);
    EXPECT_EQ(readBrick[0], NULL8);
    EXPECT_EQ(readBrick[1], NULL8);
}

TEST_F(SmallCube, TestCubeBsqHandlerReadBeforeStart) {
    QString path = testCube->fileName();
    QFile dataFile(path);
    if (!dataFile.open(QIODevice::ReadWrite)) {
        FAIL() << "Unable to open dataFile in ReadWrite";
    }
    CubeBsqHandler ioHandler(&dataFile, nullptr, *testCube->label(), true);
    Brick readBrick(1, 1, 2, testCube->pixelType());
    readBrick.SetBasePosition(1, 1, 0);
    ioHandler.read(readBrick);

    EXPECT_EQ(readBrick.size(), 2);
    EXPECT_EQ(readBrick[0], NULL8);
    EXPECT_EQ(readBrick[1], 0);
}

TEST_F(SmallCube, TestCubeBsqHandlerReadAfterEnd) {
    QString path = testCube->fileName();
    QFile dataFile(path);
    if (!dataFile.open(QIODevice::ReadWrite)) {
        FAIL() << "Unable to open dataFile in ReadWrite";
    }
    CubeBsqHandler ioHandler(&dataFile, nullptr, *testCube->label(), true);
    Brick readBrick(1, 1, 2, testCube->pixelType());
    readBrick.SetBasePosition(1, 1, 10);
    ioHandler.read(readBrick);

    EXPECT_EQ(readBrick.size(), 2);
    EXPECT_EQ(readBrick[0], 900);
    EXPECT_EQ(readBrick[1], NULL8);
}

TEST_F(SmallCube, TestCubeBsqHandlerReadOutsideVirtualBands) {
    QString path = testCube->fileName();
    QFile dataFile(path);
    if (!dataFile.open(QIODevice::ReadWrite)) {
        FAIL() << "Unable to open dataFile in ReadWrite";
    }
    QList<int> virtualBands;
    virtualBands.push_back(2);
    virtualBands.push_back(1);
    virtualBands.push_back(3);
    virtualBands.push_back(4);
    virtualBands.push_back(2);
    CubeBsqHandler ioHandler(&dataFile, &virtualBands, *testCube->label(), true);
    Brick readBrick(1, 1, 2, testCube->pixelType());
    readBrick.SetBasePosition(1, 1, 6);
    ioHandler.read(readBrick);

    EXPECT_EQ(readBrick.size(), 2);
    EXPECT_EQ(readBrick[0], NULL8);
    EXPECT_EQ(readBrick[1], NULL8);

    readBrick.SetBasePosition(1, 1, 1000);
    ioHandler.read(readBrick);

    EXPECT_EQ(readBrick[0], NULL8);
    EXPECT_EQ(readBrick[1], NULL8);


    readBrick.SetBasePosition(1, 1, -1);
    ioHandler.read(readBrick);

    EXPECT_EQ(readBrick[0], NULL8);
    EXPECT_EQ(readBrick[1], NULL8);
}

TEST_F(SmallCube, TestCubeBsqHandlerReadEdgeVirtualBands) {
    QString path = testCube->fileName();
    QFile dataFile(path);
    if (!dataFile.open(QIODevice::ReadWrite)) {
        FAIL() << "Unable to open dataFile in ReadWrite";
    }
    QList<int> virtualBands;
    virtualBands.push_back(2);
    virtualBands.push_back(1);
    virtualBands.push_back(3);
    virtualBands.push_back(4);
    virtualBands.push_back(2);
    CubeBsqHandler ioHandler(&dataFile, &virtualBands, *testCube->label(), true);
    Brick readBrick(1, 1, 2, testCube->pixelType());
    readBrick.SetBasePosition(1, 1, 0);
    ioHandler.read(readBrick);

    EXPECT_EQ(readBrick.size(), 2);
    EXPECT_EQ(readBrick[0], NULL8);
    EXPECT_EQ(readBrick[1], 100);

    readBrick.SetBasePosition(1, 1, 5);
    ioHandler.read(readBrick);

    EXPECT_EQ(readBrick.size(), 2);
    EXPECT_EQ(readBrick[0], 100);
    EXPECT_EQ(readBrick[1], NULL8);
}

TEST_F(SmallCube, TestCubeBsqHandlerReadVirtualBands) {
    QString path = testCube->fileName();
    QFile dataFile(path);
    if (!dataFile.open(QIODevice::ReadWrite)) {
        FAIL() << "Unable to open dataFile in ReadWrite";
    }
    QList<int> virtualBands;
    virtualBands.push_back(2);
    virtualBands.push_back(1);
    virtualBands.push_back(3);
    virtualBands.push_back(4);
    virtualBands.push_back(2);
    virtualBands.push_back(2);
    virtualBands.push_back(1);
    virtualBands.push_back(3);
    virtualBands.push_back(4);
    virtualBands.push_back(2);
    CubeBsqHandler ioHandler(&dataFile, &virtualBands, *testCube->label(), true);
    Brick readBrick(1, 1, 20, testCube->pixelType());
    readBrick.SetBasePosition(1, 1, -4);
    ioHandler.read(readBrick);

    EXPECT_EQ(readBrick.size(), 20);
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(readBrick[i], NULL8);
    }
    EXPECT_EQ(readBrick[5], 100);
    EXPECT_EQ(readBrick[6], 0);
    EXPECT_EQ(readBrick[7], 200);
    EXPECT_EQ(readBrick[8], 300);
    EXPECT_EQ(readBrick[9], 100);
    EXPECT_EQ(readBrick[10], 100);
    EXPECT_EQ(readBrick[11], 0);
    EXPECT_EQ(readBrick[12], 200);
    EXPECT_EQ(readBrick[13], 300);
    EXPECT_EQ(readBrick[14], 100);
    for (int i = 15; i < 20; i++) {
        EXPECT_EQ(readBrick[i], NULL8);
    }
}

class BsqCube : public TempTestingFiles {
    protected:
      Cube *testCube;

        void SetUp() override {
            TempTestingFiles::SetUp();

            testCube = new Cube();
            testCube->setDimensions(3, 1, 3);
            testCube->setFormat(Cube::Bsq);
            QString path = tempDir.path() + "/small.cub";
            testCube->create(path);

            LineManager line(*testCube);
            // our cube will be 1, 2, 3
            //                  2, 3, 4
            //                  3, 4, 5
            for (line.begin(); !line.end(); line++) {
                for (int i = 0; i < line.size(); i++) {
                    line[i] = 1 * i + line.Band();
                }
                testCube->write(line);
            }

            // Add a BandBin group to the cube label
            Pvl *label = testCube->label();
            PvlObject& cubeLabel = label->findObject("IsisCube");
            PvlGroup bandBin("BandBin");
            PvlKeyword originalBand("OriginalBand", "1");
            originalBand += "2";
            originalBand += "3";
            bandBin += originalBand;
            cubeLabel.addGroup(bandBin);
            testCube->close();
            testCube->open(path, "rw");
    }

      void TearDown() override {
        if (testCube->isOpen()) {
            testCube->close();
        }

        if (testCube) {
            delete testCube;
        }
    }
};

TEST_F(BsqCube, TestCubeBsqHandlerReadRepeatedAscendingVBands) {
    QString path = testCube->fileName();
    QFile dataFile(path);
    if (!dataFile.open(QIODevice::ReadWrite)) {
        FAIL() << "Unable to open dataFile in ReadWrite";
    }
    QList<int> virtualBands;
    virtualBands.push_back(1);
    virtualBands.push_back(2);
    virtualBands.push_back(2);
    virtualBands.push_back(3);
    CubeBsqHandler ioHandler(&dataFile, &virtualBands, *testCube->label(), true);
    Brick readBrick(3, 1, 1, testCube->pixelType());

    for (int sb = 1; sb <= virtualBands.size(); sb++) {
      readBrick.SetBasePosition(1, 1, sb);
      ioHandler.read(readBrick);
      for (int i = 0; i < readBrick.size(); i++) {
        EXPECT_EQ(readBrick[i], (i + virtualBands[readBrick.Band()-1]));
      }
    }
}

TEST_F(BsqCube, TestCubeBsqHandlerReadSkippedAscendingVBands) {
    QString path = testCube->fileName();
    QFile dataFile(path);
    if (!dataFile.open(QIODevice::ReadWrite)) {
        FAIL() << "Unable to open dataFile in ReadWrite";
    }
    QList<int> virtualBands;
    virtualBands.push_back(1);
    virtualBands.push_back(3);
    virtualBands.push_back(3);
    CubeBsqHandler ioHandler(&dataFile, &virtualBands, *testCube->label(), true);
    Brick readBrick(3, 1, 1, testCube->pixelType());

    for (int sb = 1; sb <= virtualBands.size(); sb++) {
        readBrick.SetBasePosition(1, 1, sb);
        ioHandler.read(readBrick);
        for (int i = 0; i < readBrick.size(); i++) {
            EXPECT_EQ(readBrick[i], (i + virtualBands[readBrick.Band()-1]));
        }
    }
}

TEST_F(BsqCube, TestCubeBsqHandlerReadOutOfBoundsVBands) {
    QString path = testCube->fileName();
    QFile dataFile(path);
    if (!dataFile.open(QIODevice::ReadWrite)) {
        FAIL() << "Unable to open dataFile in ReadWrite";
    }
    QList<int> virtualBands;
    virtualBands.push_back(1);
    virtualBands.push_back(5);
    CubeBsqHandler ioHandler(&dataFile, &virtualBands, *testCube->label(), true);
    Brick readBrick(3, 1, 1, testCube->pixelType());
    for (int sb = 1; sb <= virtualBands.size(); sb++) {
        for (int i = 0; i < readBrick.size(); i++) {
            readBrick.SetBasePosition(1, 1, sb);
            ioHandler.read(readBrick);
            if (readBrick.Band() == 1) {
                EXPECT_EQ(readBrick[i], (i + virtualBands[readBrick.Band()-1]));
            }
            else {
                EXPECT_EQ(readBrick[i], Null);
            }
        }
    }
}

TEST_F(BsqCube, TestCubeBsqHandlerReadDescendingVBands) {
    QString path = testCube->fileName();
    QFile dataFile(path);
    if (!dataFile.open(QIODevice::ReadWrite)) {
        FAIL() << "Unable to open dataFile in ReadWrite";
    }
    QList<int> virtualBands;
    virtualBands.push_back(3);
    virtualBands.push_back(1);
    virtualBands.push_back(3);
    CubeBsqHandler ioHandler(&dataFile, &virtualBands, *testCube->label(), true);
    Brick readBrick(3, 1, 1, testCube->pixelType());

    for (int sb = 1; sb <= virtualBands.size(); sb++) {
        readBrick.SetBasePosition(1, 1, sb);
        ioHandler.read(readBrick);
        for (int i = 0; i < readBrick.size(); i++) {
            EXPECT_EQ(readBrick[i], (i + virtualBands[readBrick.Band()-1]));
        }
    }
}

TEST_F(TempTestingFiles, TestCubeBsqHandlerExceedSampleSizeLimit) {
    std::istringstream labelStrm(R"(
    Object = IsisCube
        Object = Core
            StartByte   = 65537
            Format      = Bsq

            Group = Dimensions
                Samples = 268435457
                Lines   = 2
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

    Pvl label;
    labelStrm >> label;

    QString path = tempDir.path() + "/test_cube.cub";
    QFile dataFile(path);
    if (!dataFile.open(QIODevice::Truncate | QIODevice::ReadWrite)) {
        FAIL() << "Unable to open dataFile in ReadWrite";
    }

    CubeBsqHandler ioHandler(&dataFile, nullptr, label, false);
    EXPECT_EQ(ioHandler.getBytesPerChunk(), 1073741828);
    EXPECT_EQ(ioHandler.getChunkCountInSampleDimension(), 1);
    EXPECT_EQ(ioHandler.getSampleCountInChunk(), 268435457);
    EXPECT_EQ(ioHandler.getLineCountInChunk(), 1);
}

TEST_F(TempTestingFiles, TestCubeBsqHandlerExceedMaximumChunkLineSize) {
    std::istringstream labelStrm(R"(
    Object = IsisCube
        Object = Core
            StartByte   = 65537
            Format      = Bsq

            Group = Dimensions
                Samples = 15000
                Lines   = 18000
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

    Pvl label;
    labelStrm >> label;

    QString path = tempDir.path() + "/test_cube.cub";
    QFile dataFile(path);
    if (!dataFile.open(QIODevice::Truncate | QIODevice::ReadWrite)) {
        FAIL() << "Unable to open dataFile in ReadWrite";
    }

    CubeBsqHandler ioHandler(&dataFile, nullptr, label, false);
    EXPECT_EQ(ioHandler.getBytesPerChunk(), 540000000);
    EXPECT_EQ(ioHandler.getChunkCountInSampleDimension(), 1);
    EXPECT_EQ(ioHandler.getSampleCountInChunk(), 15000);
    EXPECT_EQ(ioHandler.getLineCountInChunk(), 9000);
}