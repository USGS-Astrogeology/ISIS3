#include <iostream>
#include <QTemporaryFile>
#include <QTemporaryDir>

#include "maptrim.h"

#include "Cube.h"
#include "Pvl.h"
#include "TestUtilities.h"
#include "FileName.h"
#include "LineManager.h"
#include "Table.h"
#include "Histogram.h"

#include "CameraFixtures.h"

#include "gmock/gmock.h"

using namespace Isis;
using testing::HasSubstr;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/maptrim.xml").expanded());

TEST_F(DefaultCube, FunctionalTestMaptrimDefault){
  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/maptrim.cub";
  QVector<QString> args = { "from=" + projTestCube->fileName(),
                            "to=" + outCubeFileName,
                            "minlat=2",
                            "maxlat=6",
                            "minlon=2",
                            "maxlon=6"
                          };
  UserInterface options(APP_XML, args);
  Pvl appLog;
  try{
    maptrim(options, &appLog);
  }
  catch (IException &e){
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube cube(outCubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Dimensions group
  EXPECT_EQ(cube.sampleCount(), 6);
  EXPECT_EQ(cube.lineCount(), 6);
  EXPECT_EQ(cube.bandCount(), 2);

  // Pixels group
  EXPECT_EQ(PixelTypeName(cube.pixelType()), "Real");
  EXPECT_EQ(ByteOrderName(cube.byteOrder()), "Lsb");
  EXPECT_DOUBLE_EQ(cube.base(), 0.0);
  EXPECT_DOUBLE_EQ(cube.multiplier(), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "VIKING_ORBITER_1");
  EXPECT_EQ(inst["InstrumentId"][0], "VISUAL_IMAGING_SUBSYSTEM_CAMERA_B");
  EXPECT_EQ(inst["TargetName"][0], "MARS");
  EXPECT_EQ(inst["StartTime"][0], "1977-07-09T20:05:51");
  EXPECT_EQ(inst["ExposureDuration"][0], "0.008480");
  EXPECT_EQ(inst["SpacecraftClockCount"][0], "33322515");
  EXPECT_EQ(inst["FloodModeId"][0], "ON");
  EXPECT_EQ(inst["GainModeId"][0], "HIGH");
  EXPECT_EQ(inst["OffsetModeId"][0], "ON");

  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(archive["DataSetId"][0], "VO1/VO2-M-VIS-2-EDR-V2.0");
  EXPECT_EQ(archive["ProductId"][0], "387A06");
  EXPECT_EQ(archive["MissionPhaseName"][0], "EXTENDED_MISSION");
  EXPECT_EQ(int(archive["ImageNumber"]), 33322515);
  EXPECT_EQ(int(archive["OrbitNumber"]), 387);

  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["FilterName"][0], "CLEAR");
  EXPECT_EQ(bandbin["FilterName"][1], "NIR");
  EXPECT_EQ(bandbin["FilterId"][0], "4");
  EXPECT_EQ(bandbin["FilterId"][1], "5");

  PvlGroup &kern = isisLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kern["NaifFrameCode"]), -27002);
  EXPECT_EQ(kern["LeapSecond"][0], "$base/kernels/lsk/naif0012.tls");
  EXPECT_EQ(kern["TargetAttitudeShape"][0], "$base/kernels/pck/pck00009.tpc");
  EXPECT_EQ(kern["TargetPosition"][0], "Table");
  EXPECT_EQ(kern["TargetPosition"][1], "$base/kernels/spk/de430.bsp");
  EXPECT_EQ((kern["InstrumentPointing"][0]), "Table");
  EXPECT_EQ((kern["InstrumentPointing"][1]), "$viking1/kernels/ck/vo1_sedr_ck2.bc");
  EXPECT_EQ((kern["InstrumentPointing"][2]), "$viking1/kernels/fk/vo1_v10.tf");
  EXPECT_EQ(kern["Instrument"][0], "Null");
  EXPECT_EQ(kern["SpacecraftClock"][0], "$viking1/kernels/sclk/vo1_fict.tsc");
  EXPECT_EQ(kern["InstrumentPosition"][0], "Table");
  EXPECT_EQ(kern["InstrumentPosition"][1], "$viking1/kernels/spk/viking1a.bsp");
  EXPECT_EQ(kern["InstrumentAddendum"][0], "$viking1/kernels/iak/vikingAddendum003.ti");
  EXPECT_EQ(kern["ShapeModel"][0], "$base/dems/molaMarsPlanetaryRadius0005.cub");
  EXPECT_EQ(kern["InstrumentPositionQuality"][0], "Reconstructed");
  EXPECT_EQ(kern["InstrumentPointingQuality"][0], "Reconstructed");
  EXPECT_EQ(int(kern["CameraVersion"]), 1);

  PvlGroup &mapping = isisLabel->findGroup("Mapping", Pvl::Traverse);
  EXPECT_EQ(mapping["ProjectionName"][0], "Sinusoidal");
  EXPECT_DOUBLE_EQ(double(mapping["CenterLongitude"]), 0.0);
  EXPECT_EQ(mapping["TargetName"][0], "MARS");
  EXPECT_DOUBLE_EQ(double(mapping["EquatorialRadius"]), 3396190.0);
  EXPECT_DOUBLE_EQ(double(mapping["PolarRadius"]), 3376200.0);
  EXPECT_EQ(mapping["LatitudeType"][0], "Planetocentric");
  EXPECT_EQ(mapping["LongitudeDirection"][0], "PositiveEast");
  EXPECT_EQ(int(mapping["LongitudeDomain"]), 360);
  EXPECT_EQ(int(mapping["MinimumLatitude"]), 0);
  EXPECT_EQ(int(mapping["MaximumLatitude"]), 10);
  EXPECT_EQ(int(mapping["MinimumLongitude"]), 0);
  EXPECT_EQ(int(mapping["MaximumLongitude"]), 10);
  EXPECT_DOUBLE_EQ(double(mapping["UpperLeftCornerX"]), 0.0);
  EXPECT_DOUBLE_EQ(double(mapping["UpperLeftCornerY"]), 600000.0);
  EXPECT_DOUBLE_EQ(double(mapping["PixelResolution"]), 100000.0);
  EXPECT_DOUBLE_EQ(double(mapping["Scale"]), 0.59274697523306);

  PvlGroup &alphacube = isisLabel->findGroup("AlphaCube", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(alphacube["AlphaSamples"]), 1204);
  EXPECT_DOUBLE_EQ(double(alphacube["AlphaLines"]), 1056);
  EXPECT_DOUBLE_EQ(double(alphacube["AlphaStartingSample"]), 0.5);
  EXPECT_DOUBLE_EQ(double(alphacube["AlphaStartingLine"]), 0.5);
  EXPECT_DOUBLE_EQ(double(alphacube["AlphaEndingSample"]), 1204.5);
  EXPECT_DOUBLE_EQ(double(alphacube["AlphaEndingLine"]), 1056.5);
  EXPECT_EQ(int(alphacube["BetaSamples"]), 1204);
  EXPECT_EQ(int(alphacube["BetaLines"]), 1056);

  std::unique_ptr<Histogram> hist (cube.histogram());
  EXPECT_EQ(hist->Average(), 130);
  EXPECT_EQ(hist->Sum(), 1170);
  EXPECT_EQ(hist->ValidPixels(), 9);
  EXPECT_DOUBLE_EQ(hist->StandardDeviation(), 80.367904041352233);

  EXPECT_TRUE(appLog.hasGroup("Mapping"));
}



TEST_F(DefaultCube, FunctionalTestMaptrimBoth){
  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/maptrim.cub";
  QVector<QString> args = { "from=" + projTestCube->fileName(),
                            "to=" + outCubeFileName,
                            "mode=both",
                            "minlat=2",
                            "maxlat=6",
                            "minlon=2",
                            "maxlon=6"
                          };
  UserInterface options(APP_XML, args);
  try{
    maptrim(options);
  }
  catch (IException &e){
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube cube(outCubeFileName.toStdString());

  // Dimensions group
  EXPECT_EQ(cube.sampleCount(), 3);
  EXPECT_EQ(cube.lineCount(), 3);
  EXPECT_EQ(cube.bandCount(), 2);

  std::unique_ptr<Histogram> hist (cube.histogram());
  EXPECT_EQ(hist->Average(), 130);
  EXPECT_EQ(hist->Sum(), 1170);
  EXPECT_EQ(hist->ValidPixels(), 9);
  EXPECT_DOUBLE_EQ(hist->StandardDeviation(), 80.367904041352233);
}



TEST_F(DefaultCube, FunctionalTestMaptrimCrop){
  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/maptrim.cub";
  QVector<QString> args = { "from=" + projTestCube->fileName(),
                            "to=" + outCubeFileName,
                            "mode=crop",
                            "minlat=2",
                            "maxlat=6",
                            "minlon=2",
                            "maxlon=6"
                          };
  UserInterface options(APP_XML, args);
  try{
    maptrim(options);
  }
  catch (IException &e){
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube cube(outCubeFileName.toStdString());

  EXPECT_EQ(cube.sampleCount(), 3);
  EXPECT_EQ(cube.lineCount(), 3);
  EXPECT_EQ(cube.bandCount(), 2);

  std::unique_ptr<Histogram> hist (cube.histogram());

  EXPECT_DOUBLE_EQ(hist->Average(), 130);
  EXPECT_EQ(hist->Sum(), 1170);
  EXPECT_EQ(hist->ValidPixels(), 9);
  EXPECT_DOUBLE_EQ(hist->StandardDeviation(), 80.367904041352233);
}


TEST_F(DefaultCube, FunctionalTestMaptrimLabelRanges){
  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/maptrim.cub";
  QVector<QString> args = { "from=" + projTestCube->fileName(),
                            "to=" + outCubeFileName,
                          };
  UserInterface options(APP_XML, args);
  try{
    maptrim(options);
  }
  catch (IException &e){
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube cube(outCubeFileName.toStdString());

  EXPECT_EQ(cube.sampleCount(), 6);
  EXPECT_EQ(cube.lineCount(), 6);
  EXPECT_EQ(cube.bandCount(), 2);

  std::unique_ptr<Histogram> hist (cube.histogram());

  EXPECT_DOUBLE_EQ(hist->Average(), 123.5);
  EXPECT_EQ(hist->Sum(), 4446);
  EXPECT_EQ(hist->ValidPixels(), 36);
  EXPECT_DOUBLE_EQ(hist->StandardDeviation(), 79.757668686375951);
}


TEST_F(DefaultCube, FunctionalTestMaptrimLevel1){
  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/maptrim.cub";
  QVector<QString> args = { "from=" + testCube->fileName(),
                            "to=" + outCubeFileName,
                          };
  UserInterface options(APP_XML, args);
  try{
    maptrim(options);
    FAIL() << "Test should fail with level 1 image." << std::endl;
  }
  catch (IException &e){
    EXPECT_THAT(e.what(), HasSubstr("Unable to initialize cube projection from file"));;
    EXPECT_THAT(e.what(), HasSubstr("Unable to find PVL group [Mapping] in file"));;
  }
}
