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
using namespace testing;
using json = nlohmann::json;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/isisimport.xml").expanded());

TEST_F(TempTestingFiles, FunctionalTestIsisImportLroNacLFull) {
  QString cubeFileName = tempDir.path() + "/lo2isisTEMP.cub";
  QVector<QString> args = {"from=data/lronac/nacl.img", "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    isisimport(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LO image: " <<e.toString().c_str() << std::endl;
  }

  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Dimensions group
  EXPECT_EQ(cube.sampleCount(), 5064);
  EXPECT_EQ(cube.lineCount(), 10);
  EXPECT_EQ(cube.bandCount(), 1);

  // Pixels group
  EXPECT_EQ(PixelTypeName(cube.pixelType()), "Real");
  EXPECT_EQ(ByteOrderName(cube.byteOrder()), "Lsb");
  EXPECT_DOUBLE_EQ(cube.base(), 0.0);
  EXPECT_DOUBLE_EQ(cube.multiplier(), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "LUNAR RECONNAISSANCE ORBITER");
  EXPECT_EQ(inst["InstrumentId"][0], "NACL");
  EXPECT_EQ(inst["TargetName"][0], "MOON");
  EXPECT_EQ(inst["StartTime"][0], "2009-11-29T14:51:21.968000");

  EXPECT_EQ(inst["TemperatureSCS"][0], "3.88");
  EXPECT_EQ(inst["TemperatureFPA"][0], "17.73");
  EXPECT_EQ(inst["TemperatureFPGA"][0], "-12.94");
  EXPECT_EQ(inst["TemperatureTelescope"][0], "8.89");
  EXPECT_EQ(inst["SpatialSumming"][0], "1");

  EXPECT_EQ(inst["TemperatureSCSRaw"][0], "2770");
  EXPECT_EQ(inst["TemperatureFPARaw"][0], "2115");
  EXPECT_EQ(inst["TemperatureFPGARaw"][0], "3440");
  EXPECT_EQ(inst["TemperatureTelescopeRaw"][0], "2536");

  // Bandbin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["FilterName"][0], "BroadBand");
  EXPECT_EQ(bandbin["Center"][0], "600");
  EXPECT_EQ(bandbin["Width"][0], "300");

  // Kernels Group
  PvlGroup &kern = isisLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kern["NaifFrameCode"]), -85600);


  /* Need to handle processing functor
  std::unique_ptr<Histogram> hist (cube.histogram());

  EXPECT_NEAR(hist->Average(), 156.48748025276461, .00001);
  EXPECT_EQ(hist->Sum(), 7924526);
  EXPECT_EQ(hist->ValidPixels(), 50640);
  EXPECT_NEAR(hist->StandardDeviation(), 36.500101257155755, .0001);
  */
}


TEST_F(TempTestingFiles, FunctionalTestIsisImportLroNacRFull) {
  QTemporaryDir prefix;
  ASSERT_TRUE(prefix.isValid());

  QString cubeFileName = prefix.path() + "/lo2isisTEMP.cub";
  QVector<QString> args = {"from=data/lronac/nacr.img", "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    isisimport(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LO image: " <<e.toString().c_str() << std::endl;
  }

  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Dimensions group
  EXPECT_EQ(cube.sampleCount(), 5064);
  EXPECT_EQ(cube.lineCount(), 10);
  EXPECT_EQ(cube.bandCount(), 1);

  // Pixels group
  EXPECT_EQ(PixelTypeName(cube.pixelType()), "Real");
  EXPECT_EQ(ByteOrderName(cube.byteOrder()), "Lsb");
  EXPECT_DOUBLE_EQ(cube.base(), 0.0);
  EXPECT_DOUBLE_EQ(cube.multiplier(), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "LUNAR RECONNAISSANCE ORBITER");
  EXPECT_EQ(inst["InstrumentId"][0], "NACR");
  EXPECT_EQ(inst["TargetName"][0], "MOON");
  EXPECT_EQ(inst["StartTime"][0], "2009-11-29T14:51:21.968000");

  EXPECT_EQ(inst["TemperatureSCS"][0], "3.88");
  EXPECT_EQ(inst["TemperatureFPA"][0], "17.67");
  EXPECT_EQ(inst["TemperatureFPGA"][0], "-11.38");
  EXPECT_EQ(inst["TemperatureTelescope"][0], "11.14");
  EXPECT_EQ(inst["SpatialSumming"][0], "1");

  EXPECT_EQ(inst["TemperatureSCSRaw"][0], "2770");
  EXPECT_EQ(inst["TemperatureFPARaw"][0], "2118");
  EXPECT_EQ(inst["TemperatureFPGARaw"][0], "3388");
  EXPECT_EQ(inst["TemperatureTelescopeRaw"][0], "2429");

  // Bandbin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["FilterName"][0], "BroadBand");
  EXPECT_EQ(bandbin["Center"][0], "600");
  EXPECT_EQ(bandbin["Width"][0], "300");

  // Kernels Group
  PvlGroup &kern = isisLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kern["NaifFrameCode"]), -85610);

  /* Need to handle processing functor
  std::unique_ptr<Histogram> hist (cube.histogram());

  EXPECT_NEAR(hist->Average(), 148.00383096366508, .00001);
  EXPECT_EQ(hist->Sum(), 7494914);
  EXPECT_EQ(hist->ValidPixels(), 50640);
  EXPECT_NEAR(hist->StandardDeviation(), 24.745522995633699, .0001);
  */
}


TEST_F(TempTestingFiles, FunctionalTestIsisImportLroLabelFail) {
  QTemporaryDir prefix;
  ASSERT_TRUE(prefix.isValid());

  QString badLabelPath = prefix.path() + "/badLabel.img";
  Pvl lab("data/lronac/nacr.img");
  PvlKeyword &bterm = lab.findKeyword("LRO:BTERM");
  bterm.setValue("fake");
  lab.write(badLabelPath.toStdString());

  QString cubeFileName = prefix.path() + "/doesntMatter.cub";
  QVector<QString> args = {"from="+badLabelPath, "to="+cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    isisimport(options);
    FAIL() << "Expected to throw exception";
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("The decompanding terms do not have the same dimensions"));
     }

}
