#include <QTemporaryDir>

#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "hi2isis.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/hi2isis.xml").expanded();

TEST(hi2isisTest, FunctionalTestHi2isisDefault) {
  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTemp.cub";
  QVector<QString> args = {"from=data/hi2isis/PSP_001446_1790_BG12_0.IMG",
                           "to=" + outCubeFileName};

  UserInterface options(APP_XML, args);
  try {
    hi2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube cube(outCubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions group
  EXPECT_EQ(cube.sampleCount(), 256);
  EXPECT_EQ(cube.lineCount(), 5000);
  EXPECT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "MARS RECONNAISSANCE ORBITER");
  EXPECT_EQ(inst["InstrumentId"][0], "HIRISE");
  EXPECT_EQ(inst["TargetName"][0], "Mars");
  EXPECT_EQ(inst["StartTime"][0], "2006-11-17T03:27:53.118");
  EXPECT_EQ(inst["StopTime"][0], "2006-11-17T03:27:54.792");
  EXPECT_EQ(inst["ObservationStartCount"][0], "848201291:54379");
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "848201291:62546");
  EXPECT_EQ(inst["SpacecraftClockStopCount"][0], "848201293:41165");
  EXPECT_EQ(inst["ReadoutStartCount"][0], "848201300:53057");
  EXPECT_EQ(inst["CalibrationStartTime"][0], "2006-11-17T03:27:53.104");
  EXPECT_EQ(inst["CalibrationStartCount"][0], "848201291:61647");
  EXPECT_EQ(inst["AnalogPowerStartTime"][0], "-9999");
  EXPECT_EQ(inst["AnalogPowerStartCount"][0], "-9999");
  EXPECT_EQ(inst["MissionPhaseName"][0], "PRIMARY SCIENCE PHASE");
  EXPECT_DOUBLE_EQ(inst["LineExposureDuration"], 334.7500);
  EXPECT_DOUBLE_EQ(inst["ScanExposureDuration"], 83.6875);
  EXPECT_DOUBLE_EQ(inst["DeltaLineTimerCount"], 155);
  EXPECT_DOUBLE_EQ(inst["Summing"], 4);
  EXPECT_DOUBLE_EQ(inst["Tdi"], 64);
  EXPECT_DOUBLE_EQ(inst["FocusPositionCount"], 2020);
  EXPECT_DOUBLE_EQ(inst["CpmmNumber"], 4);
  EXPECT_EQ(inst["CcdId"][0], "BG12");
  EXPECT_DOUBLE_EQ(inst["ChannelNumber"], 0);
  EXPECT_EQ(inst["LookupTableType"][0], "Stored");
  EXPECT_DOUBLE_EQ(inst["LookupTableMinimum"], -9998);
  EXPECT_DOUBLE_EQ(inst["LookupTableMaximum"], -9998);
  EXPECT_DOUBLE_EQ(inst["LookupTableMedian"], -9998);
  EXPECT_DOUBLE_EQ(inst["LookupTableKValue"], -9998);
  EXPECT_DOUBLE_EQ(inst["LookupTableNumber"], 10);
  EXPECT_DOUBLE_EQ(inst["OptBnchFlexureTemperature"], 20.455);
  EXPECT_DOUBLE_EQ(inst["OptBnchMirrorTemperature"], 20.1949);
  EXPECT_DOUBLE_EQ(inst["OptBnchFoldFlatTemperature "], 20.5417);
  EXPECT_DOUBLE_EQ(inst["OptBnchFpaTemperature"], 19.8482);
  EXPECT_DOUBLE_EQ(inst["OptBnchFpeTemperature"], 19.5881);
  EXPECT_DOUBLE_EQ(inst["OptBnchLivingRmTemperature"], 20.1949);
  EXPECT_DOUBLE_EQ(inst["OptBnchBoxBeamTemperature"], 20.455);
  EXPECT_DOUBLE_EQ(inst["OptBnchCoverTemperature"], 20.1082);
  EXPECT_DOUBLE_EQ(inst["FieldStopTemperature"], 18.375);
  EXPECT_DOUBLE_EQ(inst["FpaPositiveYTemperature"], 19.1548);
  EXPECT_DOUBLE_EQ(inst["FpaNegativeYTemperature"], 19.0681);
  EXPECT_DOUBLE_EQ(inst["FpeTemperature"], 17.9418);
  EXPECT_DOUBLE_EQ(inst["PrimaryMirrorMntTemperature"], 20.0215);
  EXPECT_DOUBLE_EQ(inst["PrimaryMirrorTemperature"], 20.3683);
  EXPECT_DOUBLE_EQ(inst["PrimaryMirrorBafTemperature"], 0.414005 );
  EXPECT_DOUBLE_EQ(inst["MsTrussLeg0ATemperature"], 20.3683);
  EXPECT_DOUBLE_EQ(inst["MsTrussLeg0BTemperature"], 20.5417);
  EXPECT_DOUBLE_EQ(inst["MsTrussLeg120ATemperature"], 19.5881);
  EXPECT_DOUBLE_EQ(inst["MsTrussLeg120BTemperature"],20.2816);
  EXPECT_DOUBLE_EQ(inst["MsTrussLeg240ATemperature"], 19.6748);
  EXPECT_DOUBLE_EQ(inst["MsTrussLeg240BTemperature"], 19.9348);
  EXPECT_DOUBLE_EQ(inst["BarrelBaffleTemperature"], -21.006);
  EXPECT_DOUBLE_EQ(inst["SunShadeTemperature"], -28.7562);
  EXPECT_DOUBLE_EQ(inst["SpiderLeg30Temperature"], 17.7686);
  EXPECT_DOUBLE_EQ(inst["SpiderLeg150Temperature"], 18.2883);
  EXPECT_DOUBLE_EQ(inst["SpiderLeg270Temperature"], 17.1623);
  EXPECT_DOUBLE_EQ(inst["SecMirrorMtrRngTemperature"], 19.5881);
  EXPECT_DOUBLE_EQ(inst["SecMirrorTemperature"], 20.7151);
  EXPECT_DOUBLE_EQ(inst["SecMirrorBaffleTemperature"], -18.7871);
  EXPECT_DOUBLE_EQ(inst["IeaTemperature"], 25.8353);
  EXPECT_DOUBLE_EQ(inst["FocusMotorTemperature"], 21.4088);
  EXPECT_DOUBLE_EQ(inst["IePwsBoardTemperature"], 17.7363);
  EXPECT_DOUBLE_EQ(inst["CpmmPwsBoardTemperature"], 18.078);
  EXPECT_DOUBLE_EQ(inst["MechTlmBoardTemperature"], 35.0546);
  EXPECT_DOUBLE_EQ(inst["InstContBoardTemperature"], 34.6875);
  EXPECT_EQ(inst["DllLockedFlag"][0], "YES");
  EXPECT_EQ(inst["DllLockedFlag"][1], "YES");
  EXPECT_DOUBLE_EQ(inst["DllResetCount"], 0);
  EXPECT_EQ(inst["DllLockedOnceFlag"][0], "YES");
  EXPECT_EQ(inst["DllLockedOnceFlag"][1], "YES");
  EXPECT_DOUBLE_EQ(inst["DllFrequenceCorrectCount"], 4);
  EXPECT_EQ(inst["ADCTimingSetting"][0], "5");
  EXPECT_EQ(inst["ADCTimingSetting"][1], "4");
  EXPECT_EQ(inst["Unlutted"][0], "TRUE");

  // Archive Group
  PvlGroup &arch = isisLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(arch["DataSetId"][0], "MRO-M-HIRISE-2-EDR-V1.0");
  EXPECT_EQ(arch["ProducerId"][0], "UA");
  EXPECT_EQ(arch["ObservationId"][0], "PSP_001446_1790");
  EXPECT_EQ(arch["ProductId"][0], "PSP_001446_1790_BG12_0");

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["Name"][0], "BlueGreen");
  EXPECT_DOUBLE_EQ(bandbin["Center"], 500);
  EXPECT_EQ(bandbin["Center"].unit(), "NANOMETERS");
  EXPECT_DOUBLE_EQ(bandbin["Width"], 200);
  EXPECT_EQ(bandbin["Width"].unit(), "NANOMETERS");

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kernel["NaifIkCode"]), -74699);

  std::unique_ptr<Histogram> hist (cube.histogram());

  EXPECT_NEAR(hist->Average(), 5369.265074, 0.0001);
  EXPECT_NEAR(hist->Sum(), 6872659295, .00001);
  EXPECT_EQ(hist->ValidPixels(), 1280000);
  EXPECT_NEAR(hist->StandardDeviation(), 130.71538, .00001);
}

TEST(hi2isisTest, FunctionalTestHi2isisDefaultWorstCase) {
  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTemp.cub";
  QVector<QString> args = {"from=data/hi2isis/PSP_001331_2260_IR10_1.IMG",
                           "to=" + outCubeFileName};

  UserInterface options(APP_XML, args);
  try {
    hi2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube cube(outCubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions group
  EXPECT_EQ(cube.sampleCount(), 256);
  EXPECT_EQ(cube.lineCount(), 10000);
  EXPECT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "MARS RECONNAISSANCE ORBITER");
  EXPECT_EQ(inst["InstrumentId"][0], "HIRISE");
  EXPECT_EQ(inst["TargetName"][0], "Mars");
  EXPECT_EQ(inst["StartTime"][0], "2006-11-08T04:49:14.187");
  EXPECT_EQ(inst["StopTime"][0], "2006-11-08T04:49:17.990");
  EXPECT_EQ(inst["ObservationStartCount"][0], "847428572:42722");
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "847428573:01190");
  EXPECT_EQ(inst["SpacecraftClockStopCount"][0], "847428576:53783");
  EXPECT_EQ(inst["ReadoutStartCount"][0], "847428724:55340");
  EXPECT_EQ(inst["CalibrationStartTime"][0], "2006-11-08T04:49:14.175");
  EXPECT_EQ(inst["CalibrationStartCount"][0], "847428573:00368");
  EXPECT_DOUBLE_EQ(inst["LineExposureDuration"], 380.25);
  EXPECT_DOUBLE_EQ(inst["ScanExposureDuration"], 95.0625);
  EXPECT_DOUBLE_EQ(inst["DeltaLineTimerCount"], 337);
  EXPECT_DOUBLE_EQ(inst["Tdi"], 32);
  EXPECT_DOUBLE_EQ(inst["CpmmNumber"], 6);
  EXPECT_EQ(inst["CcdId"][0], "IR10");
  EXPECT_DOUBLE_EQ(inst["ChannelNumber"], 1);
  EXPECT_DOUBLE_EQ(inst["LookupTableNumber"], 17);
  EXPECT_DOUBLE_EQ(inst["OptBnchFlexureTemperature"], 19.5881);
  EXPECT_DOUBLE_EQ(inst["OptBnchMirrorTemperature"], 19.67480);
  EXPECT_DOUBLE_EQ(inst["OptBnchFoldFlatTemperature "], 19.9348);
  EXPECT_DOUBLE_EQ(inst["OptBnchFpaTemperature"], 19.5015);
  EXPECT_DOUBLE_EQ(inst["OptBnchFpeTemperature"], 19.2415);
  EXPECT_DOUBLE_EQ(inst["OptBnchLivingRmTemperature"], 19.4148);
  EXPECT_DOUBLE_EQ(inst["OptBnchBoxBeamTemperature"], 19.5881);
  EXPECT_DOUBLE_EQ(inst["OptBnchCoverTemperature"], 19.6748);
  EXPECT_DOUBLE_EQ(inst["FieldStopTemperature"], 17.9418);
  EXPECT_DOUBLE_EQ(inst["FpaPositiveYTemperature"], 18.8082);
  EXPECT_DOUBLE_EQ(inst["FpaNegativeYTemperature"], 18.6349);
  EXPECT_DOUBLE_EQ(inst["FpeTemperature"], 18.0284);
  EXPECT_DOUBLE_EQ(inst["PrimaryMirrorMntTemperature"], 19.5015);
  EXPECT_DOUBLE_EQ(inst["PrimaryMirrorTemperature"], 19.6748);
  EXPECT_DOUBLE_EQ(inst["PrimaryMirrorBafTemperature"], 2.39402);
  EXPECT_DOUBLE_EQ(inst["MsTrussLeg0ATemperature"], 19.6748);
  EXPECT_DOUBLE_EQ(inst["MsTrussLeg0BTemperature"], 19.8482);
  EXPECT_DOUBLE_EQ(inst["MsTrussLeg120ATemperature"], 19.32810);
  EXPECT_DOUBLE_EQ(inst["MsTrussLeg120BTemperature"],20.1949);
  EXPECT_DOUBLE_EQ(inst["MsTrussLeg240ATemperature"], 20.2816);
  EXPECT_DOUBLE_EQ(inst["MsTrussLeg240BTemperature"], 20.7151);
  EXPECT_DOUBLE_EQ(inst["BarrelBaffleTemperature"], -13.8299);
  EXPECT_DOUBLE_EQ(inst["SunShadeTemperature"], -33.9377);
  EXPECT_DOUBLE_EQ(inst["SpiderLeg30Temperature"], 17.50870);
  EXPECT_DOUBLE_EQ(inst["SpiderLeg150Temperature"], 17.50870);
  EXPECT_DOUBLE_EQ(inst["SpiderLeg270Temperature"], 17.76860);
  EXPECT_DOUBLE_EQ(inst["SecMirrorMtrRngTemperature"], 20.628400);
  EXPECT_DOUBLE_EQ(inst["SecMirrorTemperature"], 20.4550);
  EXPECT_DOUBLE_EQ(inst["SecMirrorBaffleTemperature"], -11.1761);
  EXPECT_DOUBLE_EQ(inst["IeaTemperature"], 25.4878);
  EXPECT_DOUBLE_EQ(inst["FocusMotorTemperature"], 21.4088);
  EXPECT_DOUBLE_EQ(inst["IePwsBoardTemperature"], 16.3696);
  EXPECT_DOUBLE_EQ(inst["CpmmPwsBoardTemperature"], 17.6224);
  EXPECT_DOUBLE_EQ(inst["MechTlmBoardTemperature"], 34.7792);
  EXPECT_DOUBLE_EQ(inst["InstContBoardTemperature"], 34.4121);

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["Name"][0], "NearInfrared");
  EXPECT_DOUBLE_EQ(bandbin["Center"], 900);
  EXPECT_DOUBLE_EQ(bandbin["Width"], 200);

  std::unique_ptr<Histogram> hist (cube.histogram());

  EXPECT_NEAR(hist->Average(), 3139.59286, 0.0001);
  EXPECT_NEAR(hist->Sum(), 7778400972, .00001);
  EXPECT_EQ(hist->ValidPixels(), 2477519);
  EXPECT_NEAR(hist->StandardDeviation(), 250.92715, .00001);
}

TEST(hi2isisTest, FunctionalTestHi2isisUnlut) {
  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTemp.cub";
  QVector<QString> args = {"from=data/hi2isis/PSP_001446_1790_BG12_0.IMG",
                           "to=" + outCubeFileName,
                           "unlut=false"};

  UserInterface options(APP_XML, args);
  try {
    hi2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube cube(outCubeFileName);

  std::unique_ptr<Histogram> hist (cube.histogram());

  EXPECT_NEAR(hist->Average(), 171.30529, 0.0001);
  EXPECT_NEAR(hist->Sum(), 219270783, .00001);
  EXPECT_EQ(hist->ValidPixels(), 1280000);
  EXPECT_NEAR(hist->StandardDeviation(), 4.623223, .00001);
}
