#include <iostream>
#include <time.h>

#include <QString>
#include <QTemporaryDir>
#include <nlohmann/json.hpp>

#include "Histogram.h"
#include "md5wrapper.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "TestUtilities.h"

#include "isisimport.h"

#include "gmock/gmock.h"

using namespace Isis;
using namespace testing;
using json = nlohmann::json;

static QString APP_XML = FileName("$ISISROOT/bin/xml/isisimport.xml").expanded();

TEST(IsisImportTests, loTestDefault) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/lo2isisTEMP.cub";
  QVector<QString> args = {"from=data/lo2isis/case01/3133_h1_cropped.cub", "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    isisimport(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LO image: " <<e.toString().toStdString().c_str() << std::endl;
  }

  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions group
  ASSERT_EQ(cube.sampleCount(), 151);
  ASSERT_EQ(cube.lineCount(), 5);
  ASSERT_EQ(cube.bandCount(), 1);

  // Pixels group
  ASSERT_EQ(PixelTypeName(cube.pixelType()), "UnsignedByte");
  ASSERT_EQ(ByteOrderName(cube.byteOrder()), "Lsb");
  ASSERT_DOUBLE_EQ(cube.base(), 0.0);
  ASSERT_DOUBLE_EQ(cube.multiplier(), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SpacecraftName"][0].toStdString(), "Lunar Orbiter 3");
  ASSERT_EQ(inst["InstrumentId"][0].toStdString(), "High Resolution Camera");
  ASSERT_EQ(inst["TargetName"][0].toStdString(), "Moon");
  ASSERT_EQ(inst["StartTime"][0].toStdString(), "1967-02-20T08:14:28.610");
  ASSERT_DOUBLE_EQ(inst["FiducialCoordinateMicron"], 50);
  ASSERT_EQ(inst["FiducialCoordinateMicron"].unit(), "um");
  ASSERT_DOUBLE_EQ(inst["FrameNumber"], 3133);

  ASSERT_EQ(inst["FiducialID"][0].toStdString(), "1b");
  ASSERT_EQ(inst["FiducialID"][6].toStdString(), "73a");
  ASSERT_EQ(inst["FiducialID"][14].toStdString(), "144b");
  ASSERT_EQ(inst["FiducialID"][29].toStdString(), "283b");

  ASSERT_DOUBLE_EQ(inst["FiducialSamples"][0].toDouble(), 32162.0);
  ASSERT_DOUBLE_EQ(inst["FiducialSamples"][6].toDouble(), 24295.0);
  ASSERT_DOUBLE_EQ(inst["FiducialSamples"][14].toDouble(), 16593.0);
  ASSERT_DOUBLE_EQ(inst["FiducialSamples"][29].toDouble(), 1248.0);

  ASSERT_DOUBLE_EQ(inst["FiducialLines"][0].toDouble(), 8510.0);
  ASSERT_DOUBLE_EQ(inst["FiducialLines"][6].toDouble(), 8504.0);
  ASSERT_DOUBLE_EQ(inst["FiducialLines"][14].toDouble(), 584.0);
  ASSERT_DOUBLE_EQ(inst["FiducialLines"][29].toDouble(), 8496.0);

  ASSERT_DOUBLE_EQ(inst["FiducialXCoordinates"][0].toDouble(), -108.168);
  ASSERT_DOUBLE_EQ(inst["FiducialXCoordinates"][6].toDouble(), -53.474);
  ASSERT_DOUBLE_EQ(inst["FiducialXCoordinates"][14].toDouble(), 0.122);
  ASSERT_DOUBLE_EQ(inst["FiducialXCoordinates"][29].toDouble(), 106.844);

  ASSERT_DOUBLE_EQ(inst["FiducialYCoordinates"][0].toDouble(), 27.476);
  ASSERT_DOUBLE_EQ(inst["FiducialYCoordinates"][6].toDouble(), 27.5);
  ASSERT_DOUBLE_EQ(inst["FiducialYCoordinates"][14].toDouble(), -27.493);
  ASSERT_DOUBLE_EQ(inst["FiducialYCoordinates"][29].toDouble(), 27.479);

  // Bandbin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["Center"][0], "1.0");
  ASSERT_EQ(bandbin["OriginalBand"][0], "1");

  // Kernels Group
  PvlGroup &kern = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kern["NaifFrameCode"]), -533001);

  std::unique_ptr<Histogram> hist (cube.histogram());

  ASSERT_NEAR(hist->Average(), 75.43576, .00001);
  ASSERT_EQ(hist->Sum(), 56954);
  ASSERT_EQ(hist->ValidPixels(), 755);
  ASSERT_NEAR(hist->StandardDeviation(), 11.2905, .0001);
}

TEST(IsisImportTests, loMirrored) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/lo2isisTEMP.cub";
  QVector<QString> args = {"from=data/lo2isis/case02/4164H_Full_mirror_cropped.cub", "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    isisimport(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LO image: " <<e.toString().toStdString().c_str() << std::endl;
  }

  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions group
  ASSERT_EQ(cube.sampleCount(), 34530);
  ASSERT_EQ(cube.lineCount(), 5);
  ASSERT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SpacecraftName"][0].toStdString(), "Lunar Orbiter 4");
  ASSERT_EQ(inst["InstrumentId"][0].toStdString(), "High Resolution Camera");
  ASSERT_EQ(inst["TargetName"][0].toStdString(), "Moon");
  ASSERT_EQ(inst["StartTime"][0].toStdString(), "1967-05-23T07:12:45.810");
  ASSERT_DOUBLE_EQ(inst["FrameNumber"], 4164);

  // Bandbin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["Center"][0], "1.0");
  ASSERT_EQ(bandbin["OriginalBand"][0], "1");

  // Kernels Group
  PvlGroup &kern = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kern["NaifFrameCode"]), -534001);
}

TEST(IsisImportTests, loMedToHi) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/lo2isisTEMP.cub";
  QVector<QString> args = {"from=data/lo2isis/case03/3083_med_tohi_isis2_cropped.cub", "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    isisimport(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LO image: " <<e.toString().toStdString().c_str() << std::endl;
  }

  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions group
  ASSERT_EQ(cube.sampleCount(), 100);
  ASSERT_EQ(cube.lineCount(), 5);
  ASSERT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SpacecraftName"][0].toStdString(), "Lunar Orbiter 3");
  ASSERT_EQ(inst["InstrumentId"][0].toStdString(), "Medium Resolution Camera");
  ASSERT_EQ(inst["TargetName"][0].toStdString(), "Moon");
  ASSERT_EQ(inst["StartTime"][0].toStdString(), "1967-02-17T21:09:27.610");
  ASSERT_DOUBLE_EQ(inst["FrameNumber"], 3083);
  ASSERT_DOUBLE_EQ(inst["BoresightSample"], 5427.039);
  ASSERT_DOUBLE_EQ(inst["BoresightLine"], 4550.455);
  ASSERT_DOUBLE_EQ(inst["SubFrame"], 0);

  // Bandbin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["FilterName"][0].toStdString(), "none");
  ASSERT_EQ(bandbin["Center"][0], "1.0");
  ASSERT_EQ(bandbin["OriginalBand"][0], "1");

  // Kernels Group
  PvlGroup &kern = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kern["NaifFrameCode"]), -533002);
}

TEST(IsisImportTests, loMed) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/lo2isisTEMP.cub";
  QVector<QString> args = {"from=data/lo2isis/case04/3083_med_isis2_cropped.cub", "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    isisimport(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LO image: " <<e.toString().toStdString().c_str() << std::endl;
  }

  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions group
  ASSERT_EQ(cube.sampleCount(), 11800);
  ASSERT_EQ(cube.lineCount(), 5);
  ASSERT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SpacecraftName"][0].toStdString(), "Lunar Orbiter 3");
  ASSERT_EQ(inst["InstrumentId"][0].toStdString(), "Medium Resolution Camera");
  ASSERT_EQ(inst["TargetName"][0].toStdString(), "Moon");
  ASSERT_EQ(inst["StartTime"][0].toStdString(), "1967-02-17T21:09:27.610");
  ASSERT_DOUBLE_EQ(inst["FrameNumber"], 3083);
  ASSERT_DOUBLE_EQ(inst["SubFrame"], 0);

  // Bandbin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["FilterName"][0].toStdString(), "none");
  ASSERT_EQ(bandbin["Center"][0], "1.0");
  ASSERT_EQ(bandbin["OriginalBand"][0], "1");

  // Kernels Group
  PvlGroup &kern = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kern["NaifFrameCode"]), -533002);
}

TEST(IsisImportTests, loReingest) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/lo2isisTEMP.cub";
  QVector<QString> args = {"from=data/lo2isis/reimport/3133_h1.pds_cropped.img", "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    isisimport(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LO image: " <<e.toString().toStdString().c_str() << std::endl;
  }

  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions group
  ASSERT_EQ(cube.sampleCount(), 151);
  ASSERT_EQ(cube.lineCount(), 5);
  ASSERT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SpacecraftName"][0].toStdString(), "Lunar Orbiter 3");
  ASSERT_EQ(inst["InstrumentId"][0].toStdString(), "High Resolution Camera");
  ASSERT_EQ(inst["TargetName"][0].toStdString(), "Moon");
  ASSERT_EQ(inst["StartTime"][0].toStdString(), "1967-02-20T08:14:28.610000");
  ASSERT_DOUBLE_EQ(inst["FrameNumber"], 3133);
  ASSERT_DOUBLE_EQ(inst["SubFrame"], 2921);

  // Kernels Group
  PvlGroup &kern = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kern["NaifFrameCode"]), -533001);
}
