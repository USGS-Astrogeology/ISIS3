#include <QTemporaryDir>

#include "kaguyatc2isis.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Endian.h"
#include "PixelType.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/kaguyatc2isis.xml").expanded());

TEST(kaguyatc2isisTest, kaguyatc2isisTestDefault) {
  Pvl appLog;
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/kaguyatc2isisTEMP.cub";
  QVector<QString> args = {"from=data/kaguyatc2isis/TC1S2B0_01_05186N225E0040_mini.lbl", "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    kaguyatc2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest Kaguya TC image: " <<e.toString().c_str() << std::endl;
  }

  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Dimensions group
  ASSERT_EQ(cube.sampleCount(), 3208);
  ASSERT_EQ(cube.lineCount(), 3);
  ASSERT_EQ(cube.bandCount(), 1);

  // Pixels group
  ASSERT_EQ(PixelTypeName(cube.pixelType()), "SignedWord");
  ASSERT_EQ(ByteOrderName(cube.byteOrder()), "Lsb");
  ASSERT_DOUBLE_EQ(cube.base(), 0.0);
  ASSERT_DOUBLE_EQ(cube.multiplier(), 0.013);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["MissionName"][0], "SELENE");
  ASSERT_EQ(inst["SpacecraftName"][0], "KAGUYA");
  ASSERT_EQ(inst["InstrumentName"][0], "TERRAIN CAMERA 1");
  ASSERT_EQ(inst["InstrumentId"][0], "TC1");
  ASSERT_EQ(inst["TargetName"][0], "MOON");
  ASSERT_EQ(inst["StartTime"][0], "2008-12-07T05:04:34.458542");
  ASSERT_EQ(inst["StopTime"][0], "2008-12-07T05:05:04.715727");
  ASSERT_EQ(inst["SpacecraftClockStartCount"][0], "912661463.551562");
  ASSERT_EQ(inst["SpacecraftClockStopCount"][0], "912661493.808747");
  ASSERT_DOUBLE_EQ(inst["ExposureDuration"], 3.25);
  ASSERT_EQ(inst["ExposureDuration"].unit(), "ms");
  ASSERT_DOUBLE_EQ(inst["LineSamplingInterval"], 6.499932);
  ASSERT_EQ(inst["LineSamplingInterval"].unit(), "ms");
  ASSERT_EQ(inst["IlluminationCondition"][0], "MORNING");

  // Archive Group
  PvlGroup &arch = isisLabel->findGroup("Archive", Pvl::Traverse);

  ASSERT_EQ(arch["DataSetId"][0], "SLN-L-TC-3-S-LEVEL2B0-V1.0");
  ASSERT_EQ(arch["ImageValueType"][0], "RADIANCE");
  ASSERT_EQ(int(arch["SceneMaximumDn"]), 3913);
  ASSERT_EQ(int(arch["SceneMinimumDn"]), 30);
  ASSERT_DOUBLE_EQ(double(arch["SceneAverageDn"]), 868.1);
  ASSERT_DOUBLE_EQ(double(arch["UpperLeftLatitude"]), 21.694101);
  ASSERT_DOUBLE_EQ(double(arch["UpperLeftLongitude"]), 3.476042);
  ASSERT_DOUBLE_EQ(double(arch["UpperRightLatitude"]), 21.711476);
  ASSERT_DOUBLE_EQ(double(arch["UpperRightLongitude"]), 4.636101);
  ASSERT_DOUBLE_EQ(double(arch["LowerLeftLatitude"]), 23.230896);
  ASSERT_DOUBLE_EQ(double(arch["LowerLeftLongitude"]), 3.440187);
  ASSERT_DOUBLE_EQ(double(arch["LowerRightLatitude"]), 23.248459);
  ASSERT_DOUBLE_EQ(double(arch["LowerRightLongitude"]), 4.613281);

  // Bandbin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["Center"][0], "640");
  ASSERT_EQ(bandbin["Width"][0], "420");

  // Kernels Group
  PvlGroup &kern = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kern["NaifCkCode"]), -131350);
  ASSERT_EQ(int(kern["NaifFrameCode"]), -131351);
}

TEST(kaguyatc2isisTest, kaguyatc2isisTestSpSupport) {
  Pvl appLog;
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/kaguyatc2isisTEMP.cub";
  QVector<QString> args = {"from=data/kaguyatc2isis/TC1S2B0_01_00811N526E0443_mini.lbl", "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    kaguyatc2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest Kaguya TC image: " <<e.toString().c_str() << std::endl;
  }

  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Dimensions group
  ASSERT_EQ(cube.sampleCount(), 1744);
  ASSERT_EQ(cube.lineCount(), 3);
  ASSERT_EQ(cube.bandCount(), 1);

  // Pixels group
  ASSERT_EQ(PixelTypeName(cube.pixelType()), "SignedWord");
  ASSERT_EQ(ByteOrderName(cube.byteOrder()), "Lsb");
  ASSERT_DOUBLE_EQ(cube.base(), 0.0);
  ASSERT_DOUBLE_EQ(cube.multiplier(), 0.013);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["MissionName"][0], "SELENE");
  ASSERT_EQ(inst["SpacecraftName"][0], "KAGUYA");
  ASSERT_EQ(inst["InstrumentName"][0], "TERRAIN CAMERA 1");
  ASSERT_EQ(inst["InstrumentId"][0], "TC1");
  ASSERT_EQ(inst["TargetName"][0], "MOON");
  ASSERT_EQ(inst["StartTime"][0], "2007-12-15T00:00:10.156275");
  ASSERT_EQ(inst["StopTime"][0], "2007-12-15T00:00:40.413540");
  ASSERT_EQ(inst["SpacecraftClockStartCount"][0], "881712007.432675");
  ASSERT_EQ(inst["SpacecraftClockStopCount"][0], "881712037.68994");
  ASSERT_DOUBLE_EQ(inst["ExposureDuration"], 3.25);
  ASSERT_EQ(inst["ExposureDuration"].unit(), "ms");
  ASSERT_DOUBLE_EQ(inst["LineSamplingInterval"], 6.499949);
  ASSERT_EQ(inst["LineSamplingInterval"].unit(), "ms");
  ASSERT_EQ(inst["IlluminationCondition"][0], "MORNING");

  // Archive Group
  PvlGroup &arch = isisLabel->findGroup("Archive", Pvl::Traverse);

  ASSERT_EQ(arch["DataSetId"][0], "SLN-L-TC-3-SP-SUPPORT-LEVEL2B0-V1.0");
  ASSERT_EQ(arch["ImageValueType"][0], "RADIANCE");
  ASSERT_EQ(int(arch["SceneMaximumDn"]), 2534);
  ASSERT_EQ(int(arch["SceneMinimumDn"]), 0);
  ASSERT_DOUBLE_EQ(double(arch["SceneAverageDn"]), 405.4);
  ASSERT_DOUBLE_EQ(double(arch["UpperLeftLatitude"]), 51.860902);
  ASSERT_DOUBLE_EQ(double(arch["UpperLeftLongitude"]), 43.80093);
  ASSERT_DOUBLE_EQ(double(arch["UpperRightLatitude"]), 51.857042);
  ASSERT_DOUBLE_EQ(double(arch["UpperRightLongitude"]), 44.875756);
  ASSERT_DOUBLE_EQ(double(arch["LowerLeftLatitude"]), 53.380049);
  ASSERT_DOUBLE_EQ(double(arch["LowerLeftLongitude"]), 43.797102);
  ASSERT_DOUBLE_EQ(double(arch["LowerRightLatitude"]), 53.375972);
  ASSERT_DOUBLE_EQ(double(arch["LowerRightLongitude"]),44.907878);

  // Bandbin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["Center"][0], "640");
  ASSERT_EQ(bandbin["Width"][0], "420");

  // Kernels Group
  PvlGroup &kern = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kern["NaifCkCode"]), -131350);
  ASSERT_EQ(int(kern["NaifFrameCode"]), -131351);
}