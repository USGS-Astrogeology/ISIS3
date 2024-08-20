#include <QTemporaryDir>

#include "tgocassis2isis.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "gtest/gtest.h"

using namespace Isis;
using ::testing::HasSubstr;

static QString APP_XML = FileName("$ISISROOT/bin/xml/tgocassis2isis.xml").expanded();

TEST(TgoCassis2Isis, TgoCassis2IsisTestBlu) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/blu_out.cub";
  QVector<QString> args = {"from=data/tgoCassis/CAS-MCO-2016-11-26T22.50.27.381-BLU-03005-B1.xml",
                           "to=" + cubeFileName };
  UserInterface options(APP_XML, args);

  try {
    tgocassis2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest image: " << e.toString().toStdString().c_str() << std::endl;
  }
  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  EXPECT_EQ(cube.sampleCount(), 64);
  EXPECT_EQ(cube.lineCount(), 218);
  EXPECT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(inst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(inst["TargetName"][0], "Mars" );
  EXPECT_EQ(inst["StartTime"][0], "2016-11-26T22:50:27.381");
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "2f015435767e275a");
  EXPECT_EQ(inst["ExposureDuration"][0], "1.440e-003");
  EXPECT_EQ(int(inst["SummingMode"]), 0);
  EXPECT_EQ(inst["Filter"][0], "BLU");

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(archive["DataSetId"][0], "TBD");
  EXPECT_EQ(archive["ProductVersionId"][0], "UNK");
  EXPECT_EQ(archive["ProductCreationTime"][0], "2017-10-03T10:50:12");
  EXPECT_DOUBLE_EQ(double(archive["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(archive["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(archive["PredictMaximumExposureTime"]), 1.5952);
  EXPECT_DOUBLE_EQ(double(archive["CassisOffNadirAngle"]), 10.032);
  EXPECT_DOUBLE_EQ(double(archive["PredictedRepetitionFrequency"]), 367.5);
  EXPECT_DOUBLE_EQ(double(archive["GroundTrackVelocity"]), 3.4686);
  EXPECT_DOUBLE_EQ(double(archive["ForwardRotationAngle"]), 52.703);
  EXPECT_DOUBLE_EQ(double(archive["SpiceMisalignment"]), 185.422);
  EXPECT_DOUBLE_EQ(double(archive["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(archive["FNumber"]), 6.50);
  EXPECT_EQ(int(archive["ExposureTimeCommand"]), 150);
  EXPECT_EQ(int(archive["FrameletNumber"]), 5);
  EXPECT_EQ(int(archive["NumberOfFramelets"]), 40);
  EXPECT_EQ(int(archive["ImageFrequency"]), 400000);
  EXPECT_EQ(int(archive["NumberOfWindows"]), 6);
  EXPECT_EQ(int(archive["UniqueIdentifier"]), 100799268);
  EXPECT_EQ(archive["ExposureTimestamp"][0], "2f015435767e275a");
  EXPECT_DOUBLE_EQ(double(archive["ExposureTimePEHK"]), 1.440e-003);
  EXPECT_DOUBLE_EQ(double(archive["PixelsPossiblySaturated"]), 0.00);
  EXPECT_EQ(int(archive["WindowCount"]), 3);
  EXPECT_EQ(int(archive["Window1Binning"]), 0);
  EXPECT_EQ(int(archive["Window1StartSample"]), 0);
  EXPECT_EQ(int(archive["Window1EndSample"]), 2047);
  EXPECT_EQ(int(archive["Window1StartLine"]), 354);
  EXPECT_EQ(int(archive["Window1EndLine"]), 632);
  EXPECT_EQ(int(archive["Window2Binning"]), 0);
  EXPECT_EQ(int(archive["Window2StartSample"]), 0);
  EXPECT_EQ(int(archive["Window2EndSample"]), 2047);
  EXPECT_EQ(int(archive["Window2StartLine"]), 712);
  EXPECT_EQ(int(archive["Window2EndLine"]), 966);
  EXPECT_EQ(int(archive["Window3Binning"]), 1);
  EXPECT_EQ(int(archive["Window3StartSample"]), 0);
  EXPECT_EQ(int(archive["Window3EndSample"]), 2047);
  EXPECT_EQ(int(archive["Window3StartLine"]), 1048);
  EXPECT_EQ(int(archive["Window3EndLine"]), 1302);
  EXPECT_EQ(int(archive["Window4Binning"]), 0);
  EXPECT_EQ(int(archive["Window4StartSample"]), 1024);
  EXPECT_EQ(int(archive["Window4EndSample"]), 1087);
  EXPECT_EQ(int(archive["Window4StartLine"]), 1409);
  EXPECT_EQ(int(archive["Window4EndLine"]), 1626);
  EXPECT_EQ(int(archive["Window5Binning"]), 0);
  EXPECT_EQ(int(archive["Window5StartSample"]), 640);
  EXPECT_EQ(int(archive["Window5EndSample"]), 767);
  EXPECT_EQ(int(archive["Window5StartLine"]), 200);
  EXPECT_EQ(int(archive["Window5EndLine"]), 208);
  EXPECT_EQ(int(archive["Window6Binning"]), 0);
  EXPECT_EQ(int(archive["Window6StartSample"]), 1280);
  EXPECT_EQ(int(archive["Window6EndSample"]), 1407);
  EXPECT_EQ(int(archive["Window6StartLine"]), 1850);
  EXPECT_EQ(int(archive["Window6EndLine"]), 1858);
  EXPECT_EQ(int(archive["YearDoy"]), 2016331);
  EXPECT_EQ(archive["ObservationId"][0], "CRUS_049218_201_0");

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["FilterName"][0], "BLU");
  EXPECT_DOUBLE_EQ(double(bandbin["Center"]), 494.8);
  EXPECT_DOUBLE_EQ(double(bandbin["Width"]), 133.6);
  EXPECT_EQ(bandbin["NaifIkCode"][0], "-143424");

  // Kernels Group
  PvlGroup &kernels = isisLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kernels["NaifFrameCode"]), -143400);

  Histogram *hist = cube.histogram();

  EXPECT_NEAR(hist->Average(), 0.10861519942703067, 0.0001);
  EXPECT_DOUBLE_EQ(hist->Sum(), 1515.3992624059319);
  EXPECT_EQ(hist->ValidPixels(), 13952);
  EXPECT_NEAR(hist->StandardDeviation(), 0.0013539864322174439, 0.0001);
}

TEST(TgoCassis2Isis, TgoCassis2IsisTestRed) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/red_out.cub";
  QVector<QString> args = {"from=data/tgoCassis/CAS-MCO-2016-11-26T22.50.27.381-RED-01005-B1.xml",
                           "to=" + cubeFileName };
  UserInterface options(APP_XML, args);

  try {
    tgocassis2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest image: " << e.toString().toStdString().c_str() << std::endl;
  }
  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  EXPECT_EQ(cube.sampleCount(), 2048);
  EXPECT_EQ(cube.lineCount(), 256);
  EXPECT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(inst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(inst["TargetName"][0], "Mars" );
  EXPECT_EQ(inst["StartTime"][0], "2016-11-26T22:50:27.381");
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "2f015435767e275a");
  EXPECT_EQ(inst["ExposureDuration"][0], "1.440e-003");
  EXPECT_EQ(int(inst["SummingMode"]), 0);
  EXPECT_EQ(inst["Filter"][0], "RED");

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(archive["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(archive["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(archive["PredictMaximumExposureTime"]), 1.5952);
  EXPECT_DOUBLE_EQ(double(archive["CassisOffNadirAngle"]), 10.032);
  EXPECT_DOUBLE_EQ(double(archive["PredictedRepetitionFrequency"]), 367.5);
  EXPECT_DOUBLE_EQ(double(archive["GroundTrackVelocity"]), 3.4686);
  EXPECT_DOUBLE_EQ(double(archive["ForwardRotationAngle"]), 52.703);
  EXPECT_DOUBLE_EQ(double(archive["SpiceMisalignment"]), 185.422);
  EXPECT_DOUBLE_EQ(double(archive["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(archive["FNumber"]), 6.50);
  EXPECT_EQ(int(archive["ExposureTimeCommand"]), 150);
  EXPECT_EQ(int(archive["FrameletNumber"]), 5);
  EXPECT_EQ(int(archive["NumberOfFramelets"]), 40);
  EXPECT_EQ(int(archive["ImageFrequency"]), 400000);
  EXPECT_EQ(int(archive["NumberOfWindows"]), 6);
  EXPECT_EQ(int(archive["UniqueIdentifier"]), 100799268);
  EXPECT_EQ(archive["ExposureTimestamp"][0], "2f015435767e275a");
  EXPECT_DOUBLE_EQ(double(archive["ExposureTimePEHK"]), 1.440e-003);
  EXPECT_DOUBLE_EQ(double(archive["PixelsPossiblySaturated"]), 0.16);
  EXPECT_EQ(archive["ObservationId"][0], "CRUS_049218_201_0");

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["FilterName"][0], "RED");
  EXPECT_DOUBLE_EQ(double(bandbin["Center"]), 836.0);
  EXPECT_DOUBLE_EQ(double(bandbin["Width"]), 98.5);
  EXPECT_EQ(bandbin["NaifIkCode"][0], "-143422");

  // Kernels Group
  PvlGroup &kernels = isisLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kernels["NaifFrameCode"]), -143400);

  Histogram *hist = cube.histogram();

  EXPECT_NEAR(hist->Average(), 0.29922493324255584, 0.0001);
  EXPECT_DOUBLE_EQ(hist->Sum(), 156880.04179987311);
  EXPECT_EQ(hist->ValidPixels(), 524288);
  EXPECT_NEAR(hist->StandardDeviation(), 0.0053377927102926321, 0.0001);
}

TEST(TgoCassis2Isis, TgoCassis2IsisTestNir) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/nir_out.cub";
  QVector<QString> args = {"from=data/tgoCassis/CAS-MCO-2016-11-26T22.50.27.381-NIR-02005-B1.xml",
                           "to=" + cubeFileName };
  UserInterface options(APP_XML, args);

  try {
    tgocassis2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest image: " << e.toString().toStdString().c_str() << std::endl;
  }
  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  EXPECT_EQ(cube.sampleCount(), 2048);
  EXPECT_EQ(cube.lineCount(), 256);
  EXPECT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(inst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(inst["TargetName"][0], "Mars" );
  EXPECT_EQ(inst["StartTime"][0], "2016-11-26T22:50:27.381");
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "2f015435767e275a");
  EXPECT_EQ(inst["ExposureDuration"][0], "1.440e-003");
  EXPECT_EQ(int(inst["SummingMode"]), 0);
  EXPECT_EQ(inst["Filter"][0], "NIR");

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(archive["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(archive["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(archive["PredictMaximumExposureTime"]), 1.5952);
  EXPECT_DOUBLE_EQ(double(archive["CassisOffNadirAngle"]), 10.032);
  EXPECT_DOUBLE_EQ(double(archive["PredictedRepetitionFrequency"]), 367.5);
  EXPECT_DOUBLE_EQ(double(archive["GroundTrackVelocity"]), 3.4686);
  EXPECT_DOUBLE_EQ(double(archive["ForwardRotationAngle"]), 52.703);
  EXPECT_DOUBLE_EQ(double(archive["SpiceMisalignment"]), 185.422);
  EXPECT_DOUBLE_EQ(double(archive["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(archive["FNumber"]), 6.50);
  EXPECT_EQ(int(archive["ExposureTimeCommand"]), 150);
  EXPECT_EQ(int(archive["FrameletNumber"]), 5);
  EXPECT_EQ(int(archive["NumberOfFramelets"]), 40);
  EXPECT_EQ(int(archive["ImageFrequency"]), 400000);
  EXPECT_EQ(int(archive["NumberOfWindows"]), 6);
  EXPECT_EQ(int(archive["UniqueIdentifier"]), 100799268);
  EXPECT_EQ(archive["ExposureTimestamp"][0], "2f015435767e275a");
  EXPECT_DOUBLE_EQ(double(archive["ExposureTimePEHK"]), 1.440e-003);
  EXPECT_DOUBLE_EQ(double(archive["PixelsPossiblySaturated"]), 0.00);
  EXPECT_EQ(archive["ObservationId"][0], "CRUS_049218_201_0");

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["FilterName"][0], "NIR");
  EXPECT_DOUBLE_EQ(double(bandbin["Center"]), 939.3);
  EXPECT_DOUBLE_EQ(double(bandbin["Width"]), 121.8);
  EXPECT_EQ(bandbin["NaifIkCode"][0], "-143423");

  // Kernels Group
  PvlGroup &kernels = isisLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kernels["NaifFrameCode"]), -143400);

  Histogram *hist = cube.histogram();

  EXPECT_NEAR(hist->Average(), 0.30084934431296517, 0.0001);
  EXPECT_DOUBLE_EQ(hist->Sum(), 157731.70103115588);
  EXPECT_EQ(hist->ValidPixels(), 524288);
  EXPECT_NEAR(hist->StandardDeviation(), 0.026628748188169373, 0.0001);
}

TEST(TgoCassis2Isis, TgoCassis2IsisTestPan) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/pan_out.cub";
  QVector<QString> args = {"from=data/tgoCassis/CAS-MCO-2016-11-26T22.50.27.381-PAN-00005-B1.xml",
                           "to=" + cubeFileName };
  UserInterface options(APP_XML, args);

  try {
    tgocassis2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest image: " << e.toString().toStdString().c_str() << std::endl;
  }
  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  EXPECT_EQ(cube.sampleCount(), 2048);
  EXPECT_EQ(cube.lineCount(), 280);
  EXPECT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(inst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(inst["TargetName"][0], "Mars" );
  EXPECT_EQ(inst["StartTime"][0], "2016-11-26T22:50:27.381");
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "2f015435767e275a");
  EXPECT_EQ(inst["ExposureDuration"][0], "1.440e-003");
  EXPECT_EQ(int(inst["SummingMode"]), 0);
  EXPECT_EQ(inst["Filter"][0], "PAN");

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(archive["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(archive["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(archive["PredictMaximumExposureTime"]), 1.5952);
  EXPECT_DOUBLE_EQ(double(archive["CassisOffNadirAngle"]), 10.032);
  EXPECT_DOUBLE_EQ(double(archive["PredictedRepetitionFrequency"]), 367.5);
  EXPECT_DOUBLE_EQ(double(archive["GroundTrackVelocity"]), 3.4686);
  EXPECT_DOUBLE_EQ(double(archive["ForwardRotationAngle"]), 52.703);
  EXPECT_DOUBLE_EQ(double(archive["SpiceMisalignment"]), 185.422);
  EXPECT_DOUBLE_EQ(double(archive["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(archive["FNumber"]), 6.50);
  EXPECT_EQ(int(archive["ExposureTimeCommand"]), 150);
  EXPECT_EQ(int(archive["FrameletNumber"]), 5);
  EXPECT_EQ(int(archive["NumberOfFramelets"]), 40);
  EXPECT_EQ(int(archive["ImageFrequency"]), 400000);
  EXPECT_EQ(int(archive["NumberOfWindows"]), 6);
  EXPECT_EQ(int(archive["UniqueIdentifier"]), 100799268);
  EXPECT_EQ(archive["ExposureTimestamp"][0], "2f015435767e275a");
  EXPECT_DOUBLE_EQ(double(archive["ExposureTimePEHK"]), 1.440e-003);
  EXPECT_DOUBLE_EQ(double(archive["PixelsPossiblySaturated"]), 29.17);
  EXPECT_EQ(archive["ObservationId"][0], "CRUS_049218_201_0");

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["FilterName"][0], "PAN");
  EXPECT_DOUBLE_EQ(double(bandbin["Center"]), 678.2);
  EXPECT_DOUBLE_EQ(double(bandbin["Width"]), 231.9);
  EXPECT_EQ(bandbin["NaifIkCode"][0], "-143421");

  // Kernels Group
  PvlGroup &kernels = isisLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kernels["NaifFrameCode"]), -143400);

  Histogram *hist = cube.histogram();

  EXPECT_NEAR(hist->Average(), 0.20770821990423169, 0.0001);
  EXPECT_DOUBLE_EQ(hist->Sum(), 119108.20162188262);
  EXPECT_EQ(hist->ValidPixels(), 573440);
  EXPECT_NEAR(hist->StandardDeviation(), 0.0022750386505265593, 0.0001);
}

TEST(TgoCassis2Isis, TgoCassis2IsisTestInstrumentError) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/error.cub";
  QVector<QString> args = {"from=data/tgoCassis/tgocassis2isis/CAS-MCO-2016-11-26T22.35.51.907-RED-01033-B1-InstrumentError.xml",
                           "to=" + cubeFileName };
  UserInterface options(APP_XML, args);

  try {
    tgocassis2isis(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("does not appear to be a valid TGO CaSSIS label."));
  }
}

TEST(TgoCassis2Isis, TgoCassis2IsisTestSpacecraftError) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/error.cub";
  QVector<QString> args = {"from=data/tgoCassis/tgocassis2isis/CAS-MCO-2016-11-26T22.35.51.907-RED-01033-B1-SpacecraftError.xml",
                           "to=" + cubeFileName };
  UserInterface options(APP_XML, args);

  try {
    tgocassis2isis(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("does not appear to be a valid TGO CaSSIS label."));
  }
}

TEST(TgoCassis2Isis, TgoCassis2IsisTestFilterError) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/error.cub";
  QVector<QString> args = {"from=data/tgoCassis/tgocassis2isis/CAS-MCO-2016-11-20T15.30.00.349-DMP-00000-00.xml",
                           "to=" + cubeFileName };
  UserInterface options(APP_XML, args);

  try {
    tgocassis2isis(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("does not appear to be a valid TGO CaSSIS label."));
  }
}

TEST(TgoCassis2Isis, TgoCassis2IsisTestReingestedUnproj) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/reingested_unproj.cub";
  QVector<QString> args = {"from=data/tgoCassis/tgocassis2isis/CAS-MCO-2016-11-26T22.50.30.181-RED-01012-B1_rdrgen.xml",
                           "to=" + cubeFileName };
  UserInterface options(APP_XML, args);

  try {
    tgocassis2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest image: " << e.toString().toStdString().c_str() << std::endl;
  }
  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  EXPECT_EQ(cube.sampleCount(), 1792);
  EXPECT_EQ(cube.lineCount(), 256);
  EXPECT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(inst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(inst["TargetName"][0], "Mars" );
  EXPECT_EQ(inst["StartTime"][0], "2018-05-05T23:11:48.767");
  EXPECT_EQ(inst["ExposureDuration"][0], "1.488e-003");
  EXPECT_EQ(int(inst["SummingMode"]), 0);
  EXPECT_EQ(inst["Filter"][0], "RED");

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(archive["ObservationId"][0], "MY34_002002_211_2");
  EXPECT_EQ(archive["ProductVersionId"][0], "1.0");
  EXPECT_EQ(archive["ScalingFactor"][0], "1.0");
  EXPECT_EQ(archive["YearDoy"][0], "2018125");

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["FilterName"][0], "RED");
  EXPECT_DOUBLE_EQ(double(bandbin["Center"]), 836.0);
  EXPECT_DOUBLE_EQ(double(bandbin["Width"]), 98.5);
  EXPECT_EQ(bandbin["NaifIkCode"][0], "-143422");

  // Kernels Group
  PvlGroup &kernels = isisLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kernels["NaifFrameCode"]), -143400);

  Histogram *hist = cube.histogram();

  EXPECT_NEAR(hist->Average(), 0.11603224072916873, 0.0001);
  EXPECT_DOUBLE_EQ(hist->Sum(), 53230.022498987615);
  EXPECT_EQ(hist->ValidPixels(), 458752);
  EXPECT_NEAR(hist->StandardDeviation(), 0.0031173288297140921, 0.0001);
}

TEST(TgoCassis2Isis, TgoCassis2IsisTestReingestedProj) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/reingested_unproj.cub";
  QVector<QString> args = {"from=data/tgoCassis/tgocassis2isis/CAS-MCO-2016-11-26T22.50.30.181-RED-01012-B1_proj_rdrgen.xml",
                           "to=" + cubeFileName };
  UserInterface options(APP_XML, args);

  try {
    tgocassis2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest image: " << e.toString().toStdString().c_str() << std::endl;
  }
  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  EXPECT_EQ(cube.sampleCount(), 40);
  EXPECT_EQ(cube.lineCount(), 16);
  EXPECT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(inst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(inst["TargetName"][0], "Mars" );
  EXPECT_EQ(inst["StartTime"][0], "2018-05-05T23:11:48.767");
  EXPECT_EQ(inst["ExposureDuration"][0], "1.488e-003");
  EXPECT_EQ(int(inst["SummingMode"]), 0);
  EXPECT_EQ(inst["Filter"][0], "RED");

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(archive["ObservationId"][0], "MY34_002002_211_2");
  EXPECT_EQ(archive["ProductVersionId"][0], "1.0");
  EXPECT_EQ(archive["ScalingFactor"][0], "1.0");
  EXPECT_EQ(archive["YearDoy"][0], "2018125");

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["FilterName"][0], "RED");
  EXPECT_DOUBLE_EQ(double(bandbin["Center"]), 836.0);
  EXPECT_DOUBLE_EQ(double(bandbin["Width"]), 98.5);
  EXPECT_EQ(bandbin["NaifIkCode"][0], "-143422");

  // Kernels Group
  PvlGroup &kernels = isisLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kernels["NaifFrameCode"]), -143400);

  Histogram *hist = cube.histogram();

  EXPECT_NEAR(hist->Average(), 0.11608710212517628, 0.0001);
  EXPECT_DOUBLE_EQ(hist->Sum(), 26.235685080289841);
  EXPECT_EQ(hist->ValidPixels(), 226);
  EXPECT_NEAR(hist->StandardDeviation(), 0.0031668801306310155, 0.0001);
}


TEST(TgoCassis2Isis, TgoCassis2IsisTestPSALabel) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/psa.cub";
  QVector<QString> args = {"from=data/tgoCassis/tgocassis2isis/MY36_015782_024_0_PAN_cropped.xml",
                           "to=" + cubeFileName };
  UserInterface options(APP_XML, args);

  try {
    tgocassis2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest image: " << e.toString().toStdString().c_str() << std::endl;
  }
  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  EXPECT_EQ(cube.sampleCount(), 500);
  EXPECT_EQ(cube.lineCount(), 3);
  EXPECT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(inst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(inst["TargetName"][0], "Mars" );
  EXPECT_EQ(inst["StartTime"][0], "2021-06-07T00:31:03.723");
  EXPECT_EQ(inst["ExposureDuration"][0], "1.018e-003");
  EXPECT_EQ(int(inst["SummingMode"]), 0);
  EXPECT_EQ(inst["Filter"][0], "PAN");

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(archive["ObservationId"][0], "MY36_015782_024_0");
  EXPECT_EQ(archive["ProductVersionId"][0], "1.0");
  EXPECT_EQ(archive["ScalingFactor"][0], "1.0");
  EXPECT_EQ(archive["YearDoy"][0], "2021158");

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["FilterName"][0], "PAN");
  EXPECT_DOUBLE_EQ(double(bandbin["Center"]), 678.2);
  EXPECT_DOUBLE_EQ(double(bandbin["Width"]), 231.9);
  EXPECT_EQ(bandbin["NaifIkCode"][0], "-143421");

  // Kernels Group
  PvlGroup &kernels = isisLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kernels["NaifFrameCode"]), -143400);

  Histogram *hist = cube.histogram();

  EXPECT_NEAR(hist->Average(), 0.2833722, 0.0000001);
  EXPECT_DOUBLE_EQ(hist->Sum(), 326.72824501991272);
  EXPECT_EQ(hist->ValidPixels(), 1153);
  EXPECT_NEAR(hist->StandardDeviation(), 0.001798, 0.000001);
}
