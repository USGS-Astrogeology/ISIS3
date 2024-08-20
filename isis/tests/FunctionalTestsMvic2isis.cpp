#include <QTemporaryDir>
#include <memory>

#include "mvic2isis.h"
#include "TempFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"
#include "Endian.h"
#include "PixelType.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/mvic2isis.xml").expanded();

TEST_F(TempTestingFiles, Mvic2IsisTestTdiMode) {
  Pvl appLog;
  QString cubeFileName = tempDir.path() + "/mvic2isisTEMP.cub";
  QString undistortedCubeName = tempDir.path() + "/mvic2isisUndistorted.cub";
  QString errorCubeName = tempDir.path() + "/mvic2isisError.cub";
  QString qualityCubeName = tempDir.path() + "/mvic2isisQuality.cub";
  QVector<QString> args = {"from=data/mvic2isis/mc3_0034948318_0x536_sci_1_cropped.fits",
                           "to=" + cubeFileName,
                           "undistorted="+undistortedCubeName,
                           "error="+errorCubeName,
                           "quality="+qualityCubeName};

  UserInterface options(APP_XML, args);
  try {
   mvic2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest MVIC image: " << e.toString().toStdString().c_str() << std::endl;
  }
  std::unique_ptr<Cube> cube (new Cube(cubeFileName));
  Pvl *isisLabel = cube->label();

  // Dimensions Group
  ASSERT_EQ(cube->sampleCount(), 1);
  ASSERT_EQ(cube->lineCount(), 3);
  ASSERT_EQ(cube->bandCount(), 1);

  // Pixels Group
  ASSERT_EQ(cube->pixelType(), PixelType::Real);
  ASSERT_EQ(cube->byteOrder(), ByteOrder::Lsb);
  ASSERT_EQ(cube->base(), 0.0);
  ASSERT_EQ(cube->multiplier(), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SpacecraftName"][0], "NEW HORIZONS");
  ASSERT_EQ(inst["InstrumentId"][0], "MVIC_TDI");
  ASSERT_EQ(inst["TargetName"][0], "Jupiter");
  ASSERT_EQ(inst["MidObservationTime"][0], "2007-02-28T06:00:23.454");
  ASSERT_EQ(inst["SpacecraftClockStartCount"][0], "1/0034948318:06600");
  ASSERT_DOUBLE_EQ(double(inst["ExposureDuration"]), 0.59168);
  ASSERT_EQ(inst["Detector"][0], "CH4");
  ASSERT_EQ(inst["HwSide"][0], "1");
  ASSERT_EQ(inst["ScanType"][0], "TDI");
  ASSERT_EQ(inst["InstrumentMode"][0], "2");
  ASSERT_EQ(double(inst["RalphExposureDuration"]), 0.59168);
  ASSERT_EQ(double(inst["TdiRate"]), 54.0833);
  ASSERT_EQ(inst["StartTime"][0], "2007-02-28T06:00:00.520");

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  ASSERT_EQ(archive["HighSpeedCompressionMode"][0], "LOSSLESS");
  ASSERT_EQ(archive["ObservationCompletionStatus"][0], "COMPLETE");
  ASSERT_EQ(archive["SequenceDescription"][0], "MVIC terminator flat");

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["Name"][0], "CH4");
  ASSERT_EQ(double(bandbin["Center"]), 895);
  ASSERT_EQ(double(bandbin["Width"]), 40);
  ASSERT_EQ(double(bandbin["OriginalBand"]), 1);

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifFrameCode"]), -98908);
  ASSERT_EQ(kernel["NaifFrameCode"].unit(), "SPICE ID");

  // RadiometricCalibration Group
  PvlGroup &radiometricCalibration = isisLabel->findGroup("RadiometricCalibration", Pvl::Traverse);
  ASSERT_EQ(double(radiometricCalibration["PixelSize"]), 13.0000);
  ASSERT_EQ(double(radiometricCalibration["PixelFov"]), 19.8065);
  ASSERT_EQ(double(radiometricCalibration["Gain"]), 58.6000);
  ASSERT_EQ(double(radiometricCalibration["ReadNoise"]), 30.0000);
  ASSERT_EQ(double(radiometricCalibration["TdiRate"]), 54.0833);
  ASSERT_EQ(double(radiometricCalibration["SolarSpectrumResolved"]), 0.0394);
  ASSERT_EQ(double(radiometricCalibration["SolarSpectrumUnresolved"]), 1.55E-11);
  ASSERT_EQ(double(radiometricCalibration["PholusSpectrumResolved"]), 0.0385);
  ASSERT_EQ(double(radiometricCalibration["PholusSpectrumUnresolved"]), 1.51E-11);
  ASSERT_EQ(double(radiometricCalibration["CharonSpectrumResolved"]), 0.0394);
  ASSERT_EQ(double(radiometricCalibration["CharonSpectrumUnresolved"]), 1.54E-11);
  ASSERT_EQ(double(radiometricCalibration["JupiterSpectrumResolved"]), 0.0474);
  ASSERT_EQ(double(radiometricCalibration["JupiterSpectrumUnresolved"]), 1.86E-11);
  ASSERT_EQ(double(radiometricCalibration["PlutoSpectrumResolved"]), 0.0398);
  ASSERT_EQ(double(radiometricCalibration["PlutoSpectrumUnresolved"]), 1.56E-11);
  ASSERT_EQ(double(radiometricCalibration["SolarPivotWavelength"]), 8.86E-5);
  ASSERT_EQ(double(radiometricCalibration["JupiterPivotWavelength"]), 8.84E-5);
  ASSERT_EQ(double(radiometricCalibration["PholusPivotWavelength"]), 8.87E-5);
  ASSERT_EQ(double(radiometricCalibration["PlutoPivotWavelength"]), 8.86E-5);
  ASSERT_EQ(double(radiometricCalibration["CharonPivotWavelength"]), 8.86E-5);

  std::unique_ptr<Histogram> hist (cube->histogram());

  ASSERT_DOUBLE_EQ(hist->Average(), 0.34910885492960614);
  ASSERT_DOUBLE_EQ(hist->Sum(), 1.0473265647888184);
  ASSERT_EQ(hist->ValidPixels(), 3);
  ASSERT_DOUBLE_EQ(hist->StandardDeviation(), 0.6046742741102703);

  cube->close();
  try {
    cube->open(undistortedCubeName);
  } catch(IException &e) {
    FAIL() << "Unable to open undistorted output MVIC cube: " << e.toString().toStdString().c_str() << std::endl;
  }

  cube->close();
  try {
    cube->open(errorCubeName);
  } catch(IException &e) {
    FAIL() << "Unable to open error output MVIC cube: " << e.toString().toStdString().c_str() << std::endl;
  }

  cube->close();
  try {
    cube->open(qualityCubeName);
  } catch(IException &e) {
    FAIL() << "Unable to open quality output MVIC cube: " << e.toString().toStdString().c_str() << std::endl;
  }
}

TEST_F(TempTestingFiles, Mvic2IsisTestPanMode) {
  Pvl appLog;
  QString cubeFileName = tempDir.path() + "/mvic2isisTEMP.cub";
  QVector<QString> args = {"from=data/mvic2isis/mp1_0042515645_0x530_sci_1_cropped.fits", "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
   mvic2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest MVIC image: " << e.toString().toStdString().c_str() << std::endl;
  }
  std::unique_ptr<Cube> cube (new Cube(cubeFileName));
  Pvl *isisLabel = cube->label();

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["Detector"][0], "PAN1");
  ASSERT_EQ(inst["InstrumentMode"][0], "3");

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["Name"][0], "CLEAR");
  ASSERT_EQ(double(bandbin["Center"]), 680);
  ASSERT_EQ(double(bandbin["Width"]), 560);
  ASSERT_EQ(double(bandbin["OriginalBand"]), 1);

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifFrameCode"]), -98905);
  ASSERT_EQ(kernel["NaifFrameCode"].unit(), "SPICE ID");

  // RadiometricCalibration Group
  PvlGroup &radiometricCalibration = isisLabel->findGroup("RadiometricCalibration", Pvl::Traverse);
  ASSERT_EQ(double(radiometricCalibration["TdiRate"]), 81.9672);
  ASSERT_EQ(double(radiometricCalibration["SolarSpectrumResolved"]), 0.0695);
  ASSERT_EQ(double(radiometricCalibration["SolarSpectrumUnresolved"]), 2.73E-11);
  ASSERT_EQ(double(radiometricCalibration["PholusSpectrumResolved"]), 0.0603);
  ASSERT_EQ(double(radiometricCalibration["PholusSpectrumUnresolved"]), 2.36E-11);
  ASSERT_EQ(double(radiometricCalibration["CharonSpectrumResolved"]), 0.0685);
  ASSERT_EQ(double(radiometricCalibration["CharonSpectrumUnresolved"]), 2.68E-11);
  ASSERT_EQ(double(radiometricCalibration["JupiterSpectrumResolved"]), 0.0692);
  ASSERT_EQ(double(radiometricCalibration["JupiterSpectrumUnresolved"]), 2.71E-11);
  ASSERT_EQ(double(radiometricCalibration["PlutoSpectrumResolved"]), 0.0639);
  ASSERT_EQ(double(radiometricCalibration["PlutoSpectrumUnresolved"]), 2.51E-11);
  ASSERT_EQ(double(radiometricCalibration["SolarPivotWavelength"]), 6.48E-5);
  ASSERT_EQ(double(radiometricCalibration["JupiterPivotWavelength"]), 6.36E-5);
  ASSERT_EQ(double(radiometricCalibration["PholusPivotWavelength"]), 7.01E-5);
  ASSERT_EQ(double(radiometricCalibration["PlutoPivotWavelength"]), 6.64E-5);
  ASSERT_EQ(double(radiometricCalibration["CharonPivotWavelength"]), 6.51E-5);

  std::unique_ptr<Histogram> hist (cube->histogram());

  ASSERT_DOUBLE_EQ(hist->Average(), 1.391881783803304);
  ASSERT_DOUBLE_EQ(hist->Sum(), 4.1756453514099121);
  ASSERT_EQ(hist->ValidPixels(), 3);
  ASSERT_DOUBLE_EQ(hist->StandardDeviation(), 0.60270249191923053);
}

TEST_F(TempTestingFiles, Mvic2IsisTestFrameMode) {
  Pvl appLog;
  QString cubeFileName = tempDir.path() + "/mvic2isisTEMP.cub";
  QVector<QString> args = {"from=data/mvic2isis/mpf_0035126517_0x539_sci_1_cropped.fits", "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
   mvic2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest MVIC image: " << e.toString().toStdString().c_str() << std::endl;
  }
  std::unique_ptr<Cube> cube (new Cube(cubeFileName));
  Pvl *isisLabel = cube->label();

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["Detector"][0], "FRAME");
  ASSERT_EQ(inst["InstrumentMode"][0], "1");

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["Name"][0], "CLEAR");
  ASSERT_EQ(double(bandbin["Center"]), 680);
  ASSERT_EQ(double(bandbin["Width"]), 560);
  ASSERT_EQ(double(bandbin["OriginalBand"]), 1);

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifFrameCode"]), -98903);
  ASSERT_EQ(kernel["NaifFrameCode"].unit(), "SPICE ID");

  // RadiometricCalibration Group
  PvlGroup &radiometricCalibration = isisLabel->findGroup("RadiometricCalibration", Pvl::Traverse);
  ASSERT_EQ(double(radiometricCalibration["SolarSpectrumResolved"]), 0.0695);
  ASSERT_EQ(double(radiometricCalibration["SolarSpectrumUnresolved"]), 2.73E-11);
  ASSERT_EQ(double(radiometricCalibration["PholusSpectrumResolved"]), 0.0603);
  ASSERT_EQ(double(radiometricCalibration["PholusSpectrumUnresolved"]), 2.36E-11);
  ASSERT_EQ(double(radiometricCalibration["CharonSpectrumResolved"]), 0.0685);
  ASSERT_EQ(double(radiometricCalibration["CharonSpectrumUnresolved"]), 2.68E-11);
  ASSERT_EQ(double(radiometricCalibration["JupiterSpectrumResolved"]), 0.0692);
  ASSERT_EQ(double(radiometricCalibration["JupiterSpectrumUnresolved"]), 2.71E-11);
  ASSERT_EQ(double(radiometricCalibration["PlutoSpectrumResolved"]), 0.0639);
  ASSERT_EQ(double(radiometricCalibration["PlutoSpectrumUnresolved"]), 2.51E-11);
  ASSERT_EQ(double(radiometricCalibration["SolarPivotWavelength"]), 6.48E-5);
  ASSERT_EQ(double(radiometricCalibration["JupiterPivotWavelength"]), 6.36E-5);
  ASSERT_EQ(double(radiometricCalibration["PholusPivotWavelength"]), 7.01E-5);
  ASSERT_EQ(double(radiometricCalibration["PlutoPivotWavelength"]), 6.64E-5);
  ASSERT_EQ(double(radiometricCalibration["CharonPivotWavelength"]), 6.51E-5);
  ASSERT_EQ(radiometricCalibration["FlatFile"][0], "mfr_flat_20070130.fits");

  std::unique_ptr<Histogram> hist (cube->histogram());

  ASSERT_DOUBLE_EQ(hist->Average(), 83.167997894287112);
  ASSERT_DOUBLE_EQ(hist->Sum(), 2079.1999473571777);
  ASSERT_EQ(hist->ValidPixels(), 25);
  ASSERT_DOUBLE_EQ(hist->StandardDeviation(), 284.39166235335574);
}

TEST_F(TempTestingFiles, Mvic2IsisTestUncalibrated) {
  Pvl appLog;
  QString cubeFileName = tempDir.path() + "/mvic2isisTEMP.cub";
  // QString cubeFileName = "/Users/acpaquette/Desktop/mvic2isisTEMP.cub";
  QVector<QString> args = {"from=data/mvic2isis/mc1_0034942918_0x536_eng_1_cropped.fits", "to=" + cubeFileName};

  UserInterface options(APP_XML, args);
  try {
   mvic2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest MVIC image: " << e.toString().toStdString().c_str() << std::endl;
  }
  std::unique_ptr<Cube> cube (new Cube(cubeFileName));
  Pvl *isisLabel = cube->label();

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["Detector"][0], "BLUE");
  ASSERT_EQ(inst["InstrumentMode"][0], "2");

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["Name"][0], "BLUE");
  ASSERT_EQ(double(bandbin["Center"]), 475);
  ASSERT_EQ(double(bandbin["Width"]), 150);
  ASSERT_EQ(double(bandbin["OriginalBand"]), 1);

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifFrameCode"]), -98907);
  ASSERT_EQ(kernel["NaifFrameCode"].unit(), "SPICE ID");

  std::unique_ptr<Histogram> hist (cube->histogram());

  ASSERT_DOUBLE_EQ(hist->Average(), 83);
  ASSERT_DOUBLE_EQ(hist->Sum(), 249);
  ASSERT_EQ(hist->ValidPixels(), 3);
  ASSERT_DOUBLE_EQ(hist->StandardDeviation(), 2);

  args.pop_front();
  args.push_back("undistorted="+tempDir.path()+"/undistorted.cub");

  options = UserInterface(APP_XML, args);
  try {
   mvic2isis(options, &appLog);
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("does not appear to contain an MVIC undistorted image in XTENSION"));
  }

  args.pop_front();
  args.pop_back();
  args.push_back("error="+tempDir.path()+"/error.cub");

  options = UserInterface(APP_XML, args);
  try {
   mvic2isis(options, &appLog);
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("is past the last image group found in this FITS file. Image label count is"));
  }

  args.pop_front();
  args.pop_back();
  args.push_back("quality="+tempDir.path()+"/quality.cub");

  options = UserInterface(APP_XML, args);
  try {
   mvic2isis(options, &appLog);
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("is past the last image group found in this FITS file. Image label count is"));
    // Unsure how necessary two of these tests are. Really just testing a part of ProcessImportFits
    // rather than mvic2isis
  }
}

TEST_F(TempTestingFiles, Mvic2IsisTestBadComment) {
  Pvl appLog;
  QString cubeFileName = tempDir.path() + "/mvic2isisTEMP.cub";
  QVector<QString> args = {"from=data/mvic2isis/bad_comment_no_image.fits", "to=" + cubeFileName};

  args.push_back("undistorted="+tempDir.path()+"/bad.cub");

  UserInterface options(APP_XML, args);
  try {
   mvic2isis(options, &appLog);
   FAIL() << "Should not have been able to ingest: " << args[0].toStdString().c_str() << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("does not appear to contain an MVIC undistorted image in XTENSION"));
  }

  args.pop_front();
  args.pop_back();
  args.push_back("error="+tempDir.path()+"/bad.cub");

  options = UserInterface(APP_XML, args);
  try {
   mvic2isis(options, &appLog);
   FAIL() << "Should not have been able to ingest: " << args[0].toStdString().c_str() << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("does not appear to contain an MVIC Error image in the XTENSION"));
  }

  args.pop_front();
  args.pop_back();
  args.push_back("quality="+tempDir.path()+"/bad.cub");

  options = UserInterface(APP_XML, args);
  try {
   mvic2isis(options, &appLog);
   FAIL() << "Should not have been able to ingest: " << args[0].toStdString().c_str() << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("does not appear to contain an MVIC Quality image in extension"));
  }
}

TEST_F(TempTestingFiles, Mvic2IsisTestBadCommentKey) {
  Pvl appLog;
  QString cubeFileName = tempDir.path() + "/mvic2isisTEMP.cub";
  QVector<QString> args = {"from=data/mvic2isis/bad_comment_key_no_image.fits", "to=" + cubeFileName};

  args.push_back("undistorted="+tempDir.path()+"/bad.cub");

  UserInterface options(APP_XML, args);
  try {
   mvic2isis(options, &appLog);
   FAIL() << "Should not have been able to ingest: " << args[0].toStdString().c_str() << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("does not appear to contain an MVIC undistorted image in XTENSION"));
  }

  args.pop_front();
  args.pop_back();
  args.push_back("error="+tempDir.path()+"/bad.cub");

  options = UserInterface(APP_XML, args);
  try {
   mvic2isis(options, &appLog);
   FAIL() << "Should not have been able to ingest: " << args[0].toStdString().c_str() << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("does not appear to contain an MVIC Error image in the XTENSION"));
  }

  args.pop_front();
  args.pop_back();
  args.push_back("quality="+tempDir.path()+"/bad.cub");

  options = UserInterface(APP_XML, args);
  try {
   mvic2isis(options, &appLog);
   FAIL() << "Should not have been able to ingest: " << args[0].toStdString().c_str() << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("does not appear to contain an MVIC Quality image in extension"));
  }
}

TEST_F(TempTestingFiles, Mvic2IsisTestBadInstru) {
  Pvl appLog;
  QString cubeFileName = tempDir.path() + "/mvic2isisTEMP.cub";
  QVector<QString> args = {"from=data/mvic2isis/bad_inst_no_image.fits", "to=" + cubeFileName};

  UserInterface options(APP_XML, args);
  try {
   mvic2isis(options, &appLog);
   FAIL() << "Should not have been able to ingest: " << args[0].toStdString().c_str() << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("does not appear to be in New Horizons/MVIC FITS format"));
  }

  args.pop_front();
  args.pop_front();
  args.push_front("from=data/mvic2isis/bad_inst_key_no_image.fits");

  options = UserInterface(APP_XML, args);
  try {
   mvic2isis(options, &appLog);
   FAIL() << "Should not have been able to ingest: " << args[0].toStdString().c_str() << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("does not appear to be in New Horizons/MVIC FITS format"));
  }
}

TEST_F(TempTestingFiles, Mvic2IsisTestBadMission) {
  Pvl appLog;
  QString cubeFileName = tempDir.path() + "/mvic2isisTEMP.cub";
  QVector<QString> args = {"from=data/mvic2isis/bad_mission_no_image.fits", "to=" + cubeFileName};

  UserInterface options(APP_XML, args);
  try {
   mvic2isis(options, &appLog);
   FAIL() << "Should not have been able to ingest: " << args[0].toStdString().c_str() << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("does not appear to be in New Horizons/MVIC FITS format"));
  }

  args.pop_front();
  args.pop_front();
  args.push_front("from=data/mvic2isis/bad_mission_key_no_image.fits");

  options = UserInterface(APP_XML, args);
  try {
   mvic2isis(options, &appLog);
   FAIL() << "Should not have been able to ingest: " << args[0].toStdString().c_str() << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("does not appear to be in New Horizons/MVIC FITS format"));
  }
}
