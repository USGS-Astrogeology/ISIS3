#include <QTemporaryDir>

#include "kaguyami2isis.h"
#include "Histogram.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Endian.h"
#include "PixelType.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace Isis;
using namespace testing;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/kaguyatc2isis.xml").expanded());

TEST(kaguyatc2isisTest, FunctionalTestKaguyami2isisVis) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/kaguyatc2isisTEMP.cub";
  QVector<QString> args = {"from=data/kaguyami2isis/vis_cropped.img", "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    kaguyami2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest Kaguya MI image: " <<e.toString().c_str() << std::endl;
  }

  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Dimensions group
  EXPECT_EQ(cube.sampleCount(), 962);
  EXPECT_EQ(cube.lineCount(), 20);
  EXPECT_EQ(cube.bandCount(), 2);

  // Pixels group
  EXPECT_EQ(PixelTypeName(cube.pixelType()), "SignedWord");
  EXPECT_EQ(ByteOrderName(cube.byteOrder()), "Lsb");
  EXPECT_DOUBLE_EQ(cube.base(), 0.0);
  EXPECT_DOUBLE_EQ(cube.multiplier(), 0.013);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["MissionName"][0], "SELENE");
  EXPECT_EQ(inst["SpacecraftName"][0], "KAGUYA");
  EXPECT_EQ(inst["InstrumentName"][0], "Multiband Imager Visible");
  EXPECT_EQ(inst["InstrumentId"][0], "MI-VIS");
  EXPECT_EQ(inst["TargetName"][0], "MOON");
  EXPECT_EQ(inst["StartTime"][0], "2008-09-16T20:11:04.162607");
  EXPECT_EQ(inst["StopTime"][0], "2008-09-16T20:11:16.629582");
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "905631054.826");
  EXPECT_EQ(inst["SpacecraftClockStopCount"][0], "905631067.294");
  EXPECT_DOUBLE_EQ(inst["LineExposureDuration"], 2.6623);
  EXPECT_EQ(inst["LineExposureDuration"].unit(), "msec");
  EXPECT_DOUBLE_EQ(inst["LineSamplingInterval"], 13);
  EXPECT_EQ(inst["LineSamplingInterval"].unit(), "msec");
  EXPECT_DOUBLE_EQ(inst["CorrectedSamplingInterval"], 12.999974);
  EXPECT_EQ(inst["CorrectedSamplingInterval"].unit(), "msec");

  // Archive Group
  PvlGroup &arch = isisLabel->findGroup("Archive", Pvl::Traverse);

  EXPECT_EQ(arch["DataSetId"][0], "MI-VIS_Level2B");
  EXPECT_EQ(arch["ProductSetId"][0], "MI-VIS_Level2B2");

  // Bandbin Group
  PvlGroup &bandBin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  std::istringstream bandBinStream(R"(
  Group = BandBin
    FilterName = (MV1, MV2, MV3, MV4, MV5)
    Center     = (414.0, 749.0, 901.0, 950.0, 1001.0) <nm>
    Width      = (20.0, 12.0, 21.0, 30.0, 42.0) <nm>
    BaseBand   = MV5
  End_Group
  )");
  PvlGroup bandBinTruth;
  bandBinStream >> bandBinTruth;
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, bandBin, bandBinTruth);

  // Kernels Group
  PvlGroup &kern = isisLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kern["NaifCkCode"]), -131330);
  EXPECT_EQ(int(kern["NaifFrameCode"]), -131335);

  std::unique_ptr<Histogram> hist (cube.histogram());

  EXPECT_DOUBLE_EQ(hist->Average(), 25.685768243243238);
  EXPECT_DOUBLE_EQ(hist->Sum(), 494194.18099999992);
  EXPECT_EQ(hist->ValidPixels(), 19240);
  EXPECT_DOUBLE_EQ(hist->StandardDeviation(), 26.830242572528928);
}

TEST(kaguyatc2isisTest, FunctionalTestKaguyami2isisNir) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/kaguyatc2isisTEMP.cub";
  QVector<QString> args = {"from=data/kaguyami2isis/nir_cropped.img", "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    kaguyami2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest Kaguya MI image: " <<e.toString().c_str() << std::endl;
  }

  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Dimensions group
  EXPECT_EQ(cube.sampleCount(), 320);
  EXPECT_EQ(cube.lineCount(), 20);
  EXPECT_EQ(cube.bandCount(), 2);

  // Pixels group
  EXPECT_EQ(PixelTypeName(cube.pixelType()), "SignedWord");
  EXPECT_EQ(ByteOrderName(cube.byteOrder()), "Lsb");
  EXPECT_DOUBLE_EQ(cube.base(), 0.0);
  EXPECT_DOUBLE_EQ(cube.multiplier(), 0.013);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["MissionName"][0], "SELENE");
  EXPECT_EQ(inst["SpacecraftName"][0], "KAGUYA");
  EXPECT_EQ(inst["InstrumentName"][0], "Multiband Imager Near Infrared");
  EXPECT_EQ(inst["InstrumentId"][0], "MI-NIR");
  EXPECT_EQ(inst["TargetName"][0], "MOON");
  EXPECT_EQ(inst["StartTime"][0], "2008-09-16T20:10:30.480257");
  EXPECT_EQ(inst["StopTime"][0], "2008-09-16T20:10:42.921232");
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "905631021.132");
  EXPECT_EQ(inst["SpacecraftClockStopCount"][0], "905631033.574");
  EXPECT_DOUBLE_EQ(inst["LineExposureDuration"], 13.2148);
  EXPECT_EQ(inst["LineExposureDuration"].unit(), "msec");
  EXPECT_DOUBLE_EQ(inst["LineSamplingInterval"], 39);
  EXPECT_EQ(inst["LineSamplingInterval"].unit(), "msec");
  EXPECT_DOUBLE_EQ(inst["CorrectedSamplingInterval"], 38.999922);
  EXPECT_EQ(inst["CorrectedSamplingInterval"].unit(), "msec");

  // Archive Group
  PvlGroup &arch = isisLabel->findGroup("Archive", Pvl::Traverse);

  EXPECT_EQ(arch["DataSetId"][0], "MI-NIR_Level2B");
  EXPECT_EQ(arch["ProductSetId"][0], "MI-NIR_Level2B2");

  // Bandbin Group
  PvlGroup &bandBin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  std::istringstream bandBinStream(R"(
  Group = BandBin
    FilterName = (MN1, MN2, MN3, MN4)
    Center     = (1000.0, 1049.0, 1248.0, 1548.0) <nm>
    Width      = (27.0, 28.0, 33.0, 48.0) <nm>
    BaseBand   = MN1
  End_Group
  )");
  PvlGroup bandBinTruth;
  bandBinStream >> bandBinTruth;
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, bandBin, bandBinTruth);

  // Kernels Group
  PvlGroup &kern = isisLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kern["NaifCkCode"]), -131340);
  EXPECT_EQ(int(kern["NaifFrameCode"]), -131341);

  std::unique_ptr<Histogram> hist (cube.histogram());

  EXPECT_DOUBLE_EQ(hist->Average(), 29.395262812500022);
  EXPECT_DOUBLE_EQ(hist->Sum(), 188129.68200000015);
  EXPECT_EQ(hist->ValidPixels(), 6400);
  EXPECT_DOUBLE_EQ(hist->StandardDeviation(), 2.8449125231835715);
}

TEST(kaguyatc2isisTest, FunctionalTestKaguyami2isisProj) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/kaguyatc2isisTEMP.cub";
  QVector<QString> args = {"from=data/kaguyami2isis/3C5_label.pvl", "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    kaguyami2isis(options);
    FAIL() << "Should not have been able to ingest: " << args[0].toStdString().c_str() << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("Unsupported projected file"));
  }
}

TEST(kaguyatc2isisTest, FunctionalTestKaguyami2isisNullRange) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "kaguyatc2isisTEMP.cub";
  QVector<QString> args = {"from=data/kaguyami2isis/vis_cropped.img",
                           "setnullrange=yes",
                           "nullmin=0",
                           "nullmax=17486",
                           "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    kaguyami2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest Kaguya MI image: " <<e.toString().c_str() << std::endl;
  }

  Cube cube(cubeFileName.toStdString());
  std::unique_ptr<Histogram> hist (cube.histogram());

  EXPECT_EQ(hist->ValidPixels(), 79);
  EXPECT_EQ(hist->NullPixels(), 19161);
}

TEST(kaguyatc2isisTest, FunctionalTestKaguyami2isisHrsRange) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "kaguyatc2isisTEMP.cub";
  QVector<QString> args = {"from=data/kaguyami2isis/vis_cropped.img",
                           "sethrsrange=yes",
                           "hrsmin=0",
                           "hrsmax=17486",
                           "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    kaguyami2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest Kaguya MI image: " <<e.toString().c_str() << std::endl;
  }

  Cube cube(cubeFileName.toStdString());
  std::unique_ptr<Histogram> hist (cube.histogram());

  EXPECT_EQ(hist->ValidPixels(), 79);
  EXPECT_EQ(hist->HrsPixels(), 19161);
}

TEST(kaguyatc2isisTest, FunctionalTestKaguyami2isisHisRange) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "kaguyatc2isisTEMP.cub";
  QVector<QString> args = {"from=data/kaguyami2isis/vis_cropped.img",
                           "sethisrange=yes",
                           "hismin=0",
                           "hismax=17486",
                           "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    kaguyami2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest Kaguya MI image: " <<e.toString().c_str() << std::endl;
  }

  Cube cube(cubeFileName.toStdString());
  std::unique_ptr<Histogram> hist (cube.histogram());

  EXPECT_EQ(hist->ValidPixels(), 79);
  EXPECT_EQ(hist->HisPixels(), 19161);
}

TEST(kaguyatc2isisTest, FunctionalTestKaguyami2isisLrsRange) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "kaguyatc2isisTEMP.cub";
  QVector<QString> args = {"from=data/kaguyami2isis/vis_cropped.img",
                           "setlrsrange=yes",
                           "lrsmin=0",
                           "lrsmax=17486",
                           "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    kaguyami2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest Kaguya MI image: " <<e.toString().c_str() << std::endl;
  }

  Cube cube(cubeFileName.toStdString());
  std::unique_ptr<Histogram> hist (cube.histogram());

  EXPECT_EQ(hist->ValidPixels(), 79);
  EXPECT_EQ(hist->LrsPixels(), 19161);
}

TEST(kaguyatc2isisTest, FunctionalTestKaguyami2isisLisRange) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "kaguyatc2isisTEMP.cub";
  QVector<QString> args = {"from=data/kaguyami2isis/vis_cropped.img",
                           "setlisrange=yes",
                           "lismin=0",
                           "lismax=17486",
                           "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    kaguyami2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest Kaguya MI image: " <<e.toString().c_str() << std::endl;
  }

  Cube cube(cubeFileName.toStdString());
  std::unique_ptr<Histogram> hist (cube.histogram());

  EXPECT_EQ(hist->ValidPixels(), 79);
  EXPECT_EQ(hist->LisPixels(), 19161);
}

TEST(kaguyatc2isisTest, FunctionalTestKaguyami2isisError) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "kaguyatc2isisTEMP.cub";
  QVector<QString> args = {"from=data/kaguyatc2isis/TC1S2B0_01_05186N225E0040_mini.lbl",
                           "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
   kaguyami2isis(options);
   FAIL() << "Should not have been able to ingest: " << args[0].toStdString().c_str() << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("**PROGRAMMER ERROR** No value or default value to translate for translation group"));
  }
}
