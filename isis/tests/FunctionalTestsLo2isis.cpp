#include <QTemporaryDir>

#include "lo2isis.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Endian.h"
#include "PixelType.h"
#include "Histogram.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/lo2isis.xml").expanded());

TEST(lo2isisTest, lo2isisTestDefault) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/lo2isisTEMP.cub";
  QVector<QString> args = {"from=data/lo2isis/case01/3133_h1_cropped.cub", "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    lo2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LO image: " <<e.toString().c_str() << std::endl;
  }

  Cube cube(cubeFileName.toStdString());
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
  ASSERT_EQ(inst["SpacecraftName"][0], "Lunar Orbiter 3");
  ASSERT_EQ(inst["InstrumentId"][0], "High Resolution Camera");
  ASSERT_EQ(inst["TargetName"][0], "Moon");
  ASSERT_EQ(inst["StartTime"][0], "1967-02-20T08:14:28.610");
  ASSERT_DOUBLE_EQ(inst["FiducialCoordinateMicron"], 50);
  ASSERT_EQ(inst["FiducialCoordinateMicron"].unit(), "um");
  ASSERT_DOUBLE_EQ(inst["FrameNumber"], 3133);

  ASSERT_EQ(inst["FiducialID"][0], "1b");
  ASSERT_EQ(inst["FiducialID"][6], "73a");
  ASSERT_EQ(inst["FiducialID"][14], "144b");
  ASSERT_EQ(inst["FiducialID"][29], "283b");

  ASSERT_DOUBLE_EQ(IString::ToDouble(inst["FiducialSamples"][0]), 32162.0);
  ASSERT_DOUBLE_EQ(IString::ToDouble(inst["FiducialSamples"][6]), 24295.0);
  ASSERT_DOUBLE_EQ(IString::ToDouble(inst["FiducialSamples"][14]), 16593.0);
  ASSERT_DOUBLE_EQ(IString::ToDouble(inst["FiducialSamples"][29]), 1248.0);

  ASSERT_DOUBLE_EQ(IString::ToDouble(inst["FiducialLines"][0]), 8510.0);
  ASSERT_DOUBLE_EQ(IString::ToDouble(inst["FiducialLines"][6]), 8504.0);
  ASSERT_DOUBLE_EQ(IString::ToDouble(inst["FiducialLines"][14]), 584.0);
  ASSERT_DOUBLE_EQ(IString::ToDouble(inst["FiducialLines"][29]), 8496.0);

  ASSERT_DOUBLE_EQ(IString::ToDouble(inst["FiducialXCoordinates"][0]), -108.168);
  ASSERT_DOUBLE_EQ(IString::ToDouble(inst["FiducialXCoordinates"][6]), -53.474);
  ASSERT_DOUBLE_EQ(IString::ToDouble(inst["FiducialXCoordinates"][14]), 0.122);
  ASSERT_DOUBLE_EQ(IString::ToDouble(inst["FiducialXCoordinates"][29]), 106.844);

  ASSERT_DOUBLE_EQ(IString::ToDouble(inst["FiducialYCoordinates"][0]), 27.476);
  ASSERT_DOUBLE_EQ(IString::ToDouble(inst["FiducialYCoordinates"][6]), 27.5);
  ASSERT_DOUBLE_EQ(IString::ToDouble(inst["FiducialYCoordinates"][14]), -27.493);
  ASSERT_DOUBLE_EQ(IString::ToDouble(inst["FiducialYCoordinates"][29]), 27.479);

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


TEST(lo2isisTest, lo2isisMirrored) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/lo2isisTEMP.cub";
  QVector<QString> args = {"from=data/lo2isis/case02/4164H_Full_mirror_cropped.cub", "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    lo2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LO image: " <<e.toString().c_str() << std::endl;
  }

  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Dimensions group
  ASSERT_EQ(cube.sampleCount(), 34530);
  ASSERT_EQ(cube.lineCount(), 5);
  ASSERT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SpacecraftName"][0], "Lunar Orbiter 4");
  ASSERT_EQ(inst["InstrumentId"][0], "High Resolution Camera");
  ASSERT_EQ(inst["TargetName"][0], "Moon");
  ASSERT_EQ(inst["StartTime"][0], "1967-05-23T07:12:45.810");
  ASSERT_DOUBLE_EQ(inst["FrameNumber"], 4164);

  // Bandbin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["Center"][0], "1.0");
  ASSERT_EQ(bandbin["OriginalBand"][0], "1");

  // Kernels Group
  PvlGroup &kern = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kern["NaifFrameCode"]), -534001);
}



TEST(lo2isisTest, lo2isisMedToHi) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/lo2isisTEMP.cub";
  QVector<QString> args = {"from=data/lo2isis/case03/3083_med_tohi_isis2_cropped.cub", "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    lo2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LO image: " <<e.toString().c_str() << std::endl;
  }

  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Dimensions group
  ASSERT_EQ(cube.sampleCount(), 100);
  ASSERT_EQ(cube.lineCount(), 5);
  ASSERT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SpacecraftName"][0], "Lunar Orbiter 3");
  ASSERT_EQ(inst["InstrumentId"][0], "Medium Resolution Camera");
  ASSERT_EQ(inst["TargetName"][0], "Moon");
  ASSERT_EQ(inst["StartTime"][0], "1967-02-17T21:09:27.610");
  ASSERT_DOUBLE_EQ(inst["FrameNumber"], 3083);
  ASSERT_DOUBLE_EQ(inst["BoresightSample"], 5427.039);
  ASSERT_DOUBLE_EQ(inst["BoresightLine"], 4550.455);
  ASSERT_DOUBLE_EQ(inst["SubFrame"], 0);

  // Bandbin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["FilterName"][0], "none");
  ASSERT_EQ(bandbin["Center"][0], "1.0");
  ASSERT_EQ(bandbin["OriginalBand"][0], "1");

  // Kernels Group
  PvlGroup &kern = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kern["NaifFrameCode"]), -533002);
}


TEST(lo2isisTest, lo2isisMed) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/lo2isisTEMP.cub";
  QVector<QString> args = {"from=data/lo2isis/case04/3083_med_isis2_cropped.cub", "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    lo2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LO image: " <<e.toString().c_str() << std::endl;
  }

  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Dimensions group
  ASSERT_EQ(cube.sampleCount(), 11800);
  ASSERT_EQ(cube.lineCount(), 5);
  ASSERT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SpacecraftName"][0], "Lunar Orbiter 3");
  ASSERT_EQ(inst["InstrumentId"][0], "Medium Resolution Camera");
  ASSERT_EQ(inst["TargetName"][0], "Moon");
  ASSERT_EQ(inst["StartTime"][0], "1967-02-17T21:09:27.610");
  ASSERT_DOUBLE_EQ(inst["FrameNumber"], 3083);
  ASSERT_DOUBLE_EQ(inst["SubFrame"], 0);

  // Bandbin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["FilterName"][0], "none");
  ASSERT_EQ(bandbin["Center"][0], "1.0");
  ASSERT_EQ(bandbin["OriginalBand"][0], "1");

  // Kernels Group
  PvlGroup &kern = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kern["NaifFrameCode"]), -533002);
}


TEST(lo2isisTest, lo2isisReingest) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/lo2isisTEMP.cub";
  QVector<QString> args = {"from=data/lo2isis/reimport/3133_h1.pds_cropped.img", "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    lo2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LO image: " <<e.toString().c_str() << std::endl;
  }

  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Dimensions group
  ASSERT_EQ(cube.sampleCount(), 151);
  ASSERT_EQ(cube.lineCount(), 5);
  ASSERT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SpacecraftName"][0], "Lunar Orbiter 3");
  ASSERT_EQ(inst["InstrumentId"][0], "High Resolution Camera");
  ASSERT_EQ(inst["TargetName"][0], "Moon");
  ASSERT_EQ(inst["StartTime"][0], "1967-02-20T08:14:28.610000");
  ASSERT_DOUBLE_EQ(inst["FrameNumber"], 3133);
  ASSERT_DOUBLE_EQ(inst["SubFrame"], 2921);

  // Kernels Group
  PvlGroup &kern = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kern["NaifFrameCode"]), -533001);
}
