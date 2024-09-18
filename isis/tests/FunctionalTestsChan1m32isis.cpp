#include <QTemporaryDir>
#include <QFile>

#include "chan1m32isis.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace Isis;
using ::testing::HasSubstr;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/chan1m32isis.xml").expanded());

TEST(Chan1m32Isis, Chan1m32IsisTestFowardAscending) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/chan1m32isisTEMP.cub";
  QString locDest = prefix.path() + "loc.IMG";
  QString obsDest = prefix.path() + "obs.IMG";
  QFile::copy("data/chan1m32isis/forwardAscending/M3T20090630T083407_V03_LOC_cropped.IMG", locDest);
  QFile::copy("data/chan1m32isis/forwardAscending/M3T20090630T083407_V03_OBS_cropped.IMG", obsDest);

  QVector<QString> args = {"from=data/chan1m32isis/forwardAscending/M3T20090630T083407_V03_L1B_cropped.LBL",
                           "loc=" + locDest,
                           "obs=" + obsDest,
                           "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
    chan1m32isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest Chandrayaan image: " <<  e.toString().c_str() << std::endl;
  }
  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  ASSERT_EQ(cube.sampleCount(), 608);
  ASSERT_EQ(cube.lineCount(), 5);
  ASSERT_EQ(cube.bandCount(), 3);

  // Pixels Group
  ASSERT_EQ(PixelTypeName(cube.pixelType()), "Real");
  ASSERT_EQ(ByteOrderName(cube.byteOrder()), "Lsb");
  ASSERT_DOUBLE_EQ(cube.base(), 0.0);
  ASSERT_DOUBLE_EQ(cube.multiplier(), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SpacecraftName"][0], "CHANDRAYAAN-1");
  ASSERT_EQ(inst["InstrumentId"][0], "M3" );
  ASSERT_EQ(inst["TargetName"][0], "MOON" );
  ASSERT_EQ(inst["SpacecraftClockStartCount"][0], "12/1759056.764" );
  ASSERT_DOUBLE_EQ(double(inst["LineExposureDuration"]), 50.88);
  ASSERT_EQ(inst["StartTime"][0], "2009-06-30T08:34:35.424411" );
  ASSERT_EQ(inst["StopTime"][0], "2009-06-30T08:34:35.678811" );
  ASSERT_EQ(inst["SpacecraftYawDirection"][0], "FORWARD" );
  ASSERT_EQ(inst["OrbitLimbDirection"][0], "ASCENDING" );

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  ASSERT_EQ(archive["ProductId"][0], "M3T20090630T083407_V03_RDN" );
  ASSERT_EQ(archive["SourceProductId"][0], "M3T20090630T083407_V01_L0.IMG" );
  ASSERT_EQ(archive["ProductType"][0], "CALIBRATED_IMAGE" );

  // BandBin Group
  // Check size, first, 2 middle, and last values? Enough?
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["Center"].size(), 256);
  ASSERT_EQ(bandbin["Width"].size(), 256);
  ASSERT_EQ(bandbin["FilterNumber"].size(), 256);
  ASSERT_EQ(bandbin["OriginalBand"].size(), 256);

  ASSERT_DOUBLE_EQ(std::stod(bandbin["Center"][0]), 446.02);
  ASSERT_DOUBLE_EQ(std::stod(bandbin["Center"][64]), 1084.8);
  ASSERT_DOUBLE_EQ(std::stod(bandbin["Center"][128]), 1723.5899999999999);
  ASSERT_DOUBLE_EQ(std::stod(bandbin["Center"][255]), 2991.17);

  ASSERT_DOUBLE_EQ(std::stod(bandbin["Width"][0]), 12.31);
  ASSERT_DOUBLE_EQ(std::stod(bandbin["Width"][64]), 12.29);
  ASSERT_DOUBLE_EQ(std::stod(bandbin["Width"][128]), 12.61);
  ASSERT_DOUBLE_EQ(std::stod(bandbin["Width"][255]), 12.18);


  ASSERT_DOUBLE_EQ(std::stod(bandbin["FilterNumber"][0]), 5);
  ASSERT_DOUBLE_EQ(std::stod(bandbin["FilterNumber"][64]), 69);
  ASSERT_DOUBLE_EQ(std::stod(bandbin["FilterNumber"][128]), 133);
  ASSERT_DOUBLE_EQ(std::stod(bandbin["FilterNumber"][255]), 260);

  ASSERT_DOUBLE_EQ(std::stod(bandbin["OriginalBand"][0]), 1);
  ASSERT_DOUBLE_EQ(std::stod(bandbin["OriginalBand"][64]), 65);
  ASSERT_DOUBLE_EQ(std::stod(bandbin["OriginalBand"][128]), 129);
  ASSERT_DOUBLE_EQ(std::stod(bandbin["OriginalBand"][255]), 256);

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifFrameCode"]), -86520);

  std::unique_ptr<Histogram> hist (cube.histogram());

  ASSERT_DOUBLE_EQ(hist->Average(), 15.58169779027077);
  ASSERT_DOUBLE_EQ(hist->Sum(), 47368.361282423139);
  ASSERT_EQ(hist->ValidPixels(), 3040);
  ASSERT_DOUBLE_EQ(hist->StandardDeviation(), 2.2696592481066249);
}


TEST(Chan1m32Isis, Chan1m32IsisTestFowardDescending) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/chan1m32isisTEMP.cub";
  QString locDest = prefix.path() + "obs.IMG";
  QString obsDest = prefix.path() + "loc.IMG";
  QFile::copy("data/chan1m32isis/forwardDescending/M3G20081129T171431_V03_LOC_cropped.IMG", locDest);
  QFile::copy("data/chan1m32isis/forwardDescending/M3G20081129T171431_V03_OBS_cropped.IMG", obsDest);

  QVector<QString> args = {"from=data/chan1m32isis/forwardDescending/M3G20081129T171431_V03_L1B_cropped.LBL",
                           "loc=" + locDest,
                           "obs=" + obsDest,
                           "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
    chan1m32isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest Chandrayaan image: " <<  e.toString().c_str() << std::endl;
  }
  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SpacecraftClockStartCount"][0], "2/1531046.542" );
  ASSERT_EQ(inst["SpacecraftClockStopCount"][0], "2/1531047.050" );
  ASSERT_EQ(inst["StartTime"][0], "2008-11-29T17:14:29.729807" );
  ASSERT_EQ(inst["StopTime"][0], "2008-11-29T17:14:30.238607" );
  ASSERT_EQ(inst["SpacecraftYawDirection"][0], "FORWARD" );
  ASSERT_EQ(inst["OrbitLimbDirection"][0], "DESCENDING" );

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  ASSERT_EQ(archive["ProductId"][0], "M3G20081129T171431_V03_RDN" );
  ASSERT_EQ(archive["SourceProductId"][0], "M3G20081129T171431_V01_L0.IMG" );
  ASSERT_EQ(archive["ProductType"][0], "CALIBRATED_IMAGE" );

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifFrameCode"]), -86520);

  std::unique_ptr<Histogram> hist (cube.histogram());

  ASSERT_DOUBLE_EQ(hist->Average(), 28.730294761277342);
  ASSERT_DOUBLE_EQ(hist->Sum(), 43670.048037141562);
  ASSERT_EQ(hist->ValidPixels(), 1520);
  ASSERT_DOUBLE_EQ(hist->StandardDeviation(), 18.613867183571024);
}

TEST(Chan1m32Isis, Chan1m32IsisTestReverseDescending) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/chan1m32isisTEMP.cub";

  QString locDest = prefix.path() + "loc.IMG";
  QString obsDest = prefix.path() + "obs.IMG";
  QFile::copy("data/chan1m32isis/reverseDescending/M3G20090106T113423_V03_LOC_cropped.IMG", locDest);
  QFile::copy("data/chan1m32isis/reverseDescending/M3G20090106T113423_V03_OBS_cropped.IMG", obsDest);

  QVector<QString> args = {"from=data/chan1m32isis/reverseDescending/M3G20090106T113423_V03_L1B_cropped.LBL",
                           "loc=" + locDest,
                           "obs=" + obsDest,
                           "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
    chan1m32isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest Chandrayaan image: " <<  e.toString().c_str() << std::endl;
  }
  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SpacecraftClockStartCount"][0], "4/1165041.748" );
  ASSERT_EQ(inst["SpacecraftClockStopCount"][0], "4/1165042.256" );
  ASSERT_EQ(inst["StartTime"][0], "2009-01-06T11:34:24.380656" );
  ASSERT_EQ(inst["StopTime"][0], "2009-01-06T11:34:24.889456" );
  ASSERT_EQ(inst["SpacecraftYawDirection"][0], "REVERSE" );
  ASSERT_EQ(inst["OrbitLimbDirection"][0], "DESCENDING" );

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  ASSERT_EQ(archive["ProductId"][0], "M3G20090106T113423_V03_RDN" );
  ASSERT_EQ(archive["SourceProductId"][0], "M3G20090106T113423_V01_L0.IMG" );
  ASSERT_EQ(archive["ProductType"][0], "CALIBRATED_IMAGE" );

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifFrameCode"]), -86520);

  std::unique_ptr<Histogram> hist (cube.histogram());

  ASSERT_NEAR(hist->Average(), 25.7498, .0001);
  ASSERT_NEAR(hist->Sum(), 39139.74936, .00001);
  ASSERT_EQ(hist->ValidPixels(), 1520);
  ASSERT_NEAR(hist->StandardDeviation(), 5.64341, .00001);
}



TEST(Chan1m32Isis, Chan1m32IsisTestReverseAscending) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/chan1m32isisTEMP.cub";

  QString locDest = prefix.path() + "loc.IMG";
  QString obsDest = prefix.path() + "obs.IMG";
  QFile::copy("data/chan1m32isis/reverseAscending/M3G20090423T191900_V03_LOC_cropped.IMG", locDest);
  QFile::copy("data/chan1m32isis/reverseAscending/M3G20090423T191900_V03_OBS_cropped.IMG", obsDest);

  QVector<QString> args = {"from=data/chan1m32isis/reverseAscending/M3G20090423T191900_V03_L1B_cropped.LBL",
                           "loc=" + locDest,
                           "obs=" + obsDest,
                           "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
    chan1m32isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest Chandrayaan image: " <<  e.toString().c_str() << std::endl;
  }
  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SpacecraftClockStartCount"][0], "9/1365765.385" );
  ASSERT_EQ(inst["SpacecraftClockStopCount"][0], "9/1365765.893" );
  ASSERT_EQ(inst["StartTime"][0], "2009-04-23T19:19:44.679982" );
  ASSERT_EQ(inst["StopTime"][0], "2009-04-23T19:19:45.188782" );
  ASSERT_EQ(inst["SpacecraftYawDirection"][0], "REVERSE" );
  ASSERT_EQ(inst["OrbitLimbDirection"][0], "ASCENDING" );

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  ASSERT_EQ(archive["ProductId"][0], "M3G20090423T191900_V03_RDN" );
  ASSERT_EQ(archive["SourceProductId"][0], "M3G20090423T191900_V01_L0.IMG" );
  ASSERT_EQ(archive["ProductType"][0], "CALIBRATED_IMAGE" );

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifFrameCode"]), -86520);

  std::unique_ptr<Histogram> hist (cube.histogram());

  ASSERT_NEAR(hist->Average(), 12.4351, .0001);
  ASSERT_NEAR(hist->Sum(), 18901.42668, .00001);
  ASSERT_EQ(hist->ValidPixels(), 1520);
  ASSERT_NEAR(hist->StandardDeviation(), 2.14976, .00001);
}


TEST(Chan1m32Isis, Chan1m32IsisTestLinerateNotConstant) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/chan1m32isisTEMP.cub";
  QVector<QString> args = {"from=data/chan1m32isis/linerateNotConstant/M3G20081118T223204_V03_L1B_cropped.LBL",
                           "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
    chan1m32isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest Chandrayaan image: " <<  e.toString().c_str() << std::endl;
  }
  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SpacecraftYawDirection"][0], "FORWARD" );
  ASSERT_EQ(inst["OrbitLimbDirection"][0], "DESCENDING" );

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  ASSERT_EQ(archive["ProductId"][0], "M3G20081118T223204_V03_RDN" );
  ASSERT_EQ(archive["SourceProductId"][0], "M3G20081118T223204_V01_L0.IMG" );
  ASSERT_EQ(archive["ProductType"][0], "CALIBRATED_IMAGE" );
  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifFrameCode"]), -86520);

  std::unique_ptr<Histogram> hist (cube.histogram());

  ASSERT_NEAR(hist->Average(), 757.2527, .0001);
  ASSERT_NEAR(hist->Sum(), 1151024.22573, .00001);
  ASSERT_EQ(hist->ValidPixels(), 1520);
  ASSERT_NEAR(hist->StandardDeviation(), 152.55850, .00001);
}



TEST(Chan1m32Isis, Chan1m32IsisTestL0) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/chan1m32isisTEMP.cub";
  QVector<QString> args = {"from=data/chan1m32isis/l0/M3G20090106T113423_V01_L0_cropped.LBL",
                           "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
    chan1m32isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest Chandrayaan image: " <<  e.toString().c_str() << std::endl;
  }
  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  PvlGroup &dimensions = isisLabel->findGroup("Dimensions", Pvl::Traverse);
  ASSERT_EQ(int(dimensions["Samples"]), 320);
  ASSERT_EQ(int(dimensions["Lines"]), 5);
  ASSERT_EQ(int(dimensions["Bands"]), 3);

  // Pixels Group
  PvlGroup &pixels = isisLabel->findGroup("Pixels", Pvl::Traverse);
  ASSERT_EQ(pixels["Type"][0], "SignedWord");
  ASSERT_EQ(pixels["ByteOrder"][0], "Lsb");
  ASSERT_EQ(double(pixels["Base"]), 0.0);
  ASSERT_EQ(double(pixels["Multiplier"]), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SpacecraftName"][0], "CHANDRAYAAN-1");
  ASSERT_EQ(inst["InstrumentId"][0], "M3" );
  ASSERT_EQ(inst["TargetName"][0], "MOON" );
  ASSERT_EQ(inst["SpacecraftClockStartCount"][0], "4/1165041.799" );
  ASSERT_EQ(inst["SpacecraftClockStopCount"][0], "4/1165065" );
  ASSERT_DOUBLE_EQ(double(inst["LineExposureDuration"]), 101.76);
  ASSERT_DOUBLE_EQ(double(inst["SpatialSumming"]), 2);
  ASSERT_EQ(inst["StartTime"][0], "2009-01-06T11:34:23" );
  ASSERT_EQ(inst["StopTime"][0], "2009-01-06T11:34:47" );
  ASSERT_EQ(inst["SpacecraftYawDirection"][0], "UNKNOWN" );
  ASSERT_EQ(inst["OrbitLimbDirection"][0], "UNKNOWN" );

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  ASSERT_EQ(archive["ProductId"][0], "M3G20090106T113423_V01_L0" );
  ASSERT_EQ(archive["ProductType"][0], "RAW_IMAGE" );

  // BandBin Group
  // Check size, first, 2 middle, and last values? Enough?
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["Center"].size(), 85);
  ASSERT_EQ(bandbin["FilterNumber"].size(), 85);
  ASSERT_EQ(bandbin["OriginalBand"].size(), 85);

  ASSERT_DOUBLE_EQ(std::stod(bandbin["Center"][0]), 460.990);
  ASSERT_DOUBLE_EQ(std::stod(bandbin["Center"][21]), 1009.95);
  ASSERT_DOUBLE_EQ(std::stod(bandbin["Center"][42]), 1429.15);
  ASSERT_DOUBLE_EQ(std::stod(bandbin["Center"][84]), 2976.20);

  ASSERT_DOUBLE_EQ(std::stod(bandbin["FilterNumber"][0]), 5);
  ASSERT_DOUBLE_EQ(std::stod(bandbin["FilterNumber"][21]), 57);
  ASSERT_DOUBLE_EQ(std::stod(bandbin["FilterNumber"][42]), 99);
  ASSERT_DOUBLE_EQ(std::stod(bandbin["FilterNumber"][84]), 253);

  ASSERT_DOUBLE_EQ(std::stod(bandbin["OriginalBand"][0]), 1);
  ASSERT_DOUBLE_EQ(std::stod(bandbin["OriginalBand"][21]), 22);
  ASSERT_DOUBLE_EQ(std::stod(bandbin["OriginalBand"][42]), 43);
  ASSERT_DOUBLE_EQ(std::stod(bandbin["OriginalBand"][84]), 85);

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifFrameCode"]), -86520);

  std::unique_ptr<Histogram> hist (cube.histogram());

  ASSERT_NEAR(hist->Average(), 776.031, .001);
  ASSERT_EQ(hist->Sum(), 1241649);
  ASSERT_EQ(hist->ValidPixels(), 1600);
  ASSERT_NEAR(hist->StandardDeviation(), 449.337, .001);
}


TEST(Chan1m32Isis, Chan1m32IsisTestBadFile) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/chan1m32isisTEMP.cub";
  QVector<QString> args = {"from=data/kaguyatc2isis/TC1S2B0_01_05186N225E0040_mini.lbl",
                           "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try{
    chan1m32isis(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e){
    EXPECT_THAT(e.what(), HasSubstr("PVL Keyword [PRODUCT_TYPE] does not exist in [Object = Root] in file"));
  }
}
