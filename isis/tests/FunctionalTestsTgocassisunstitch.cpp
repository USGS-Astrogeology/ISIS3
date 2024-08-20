#include <QTemporaryDir>

#include "tgocassisunstitch.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "gtest/gtest.h"

using namespace Isis;
using ::testing::HasSubstr;

static QString APP_XML = FileName("$ISISROOT/bin/xml/tgocassisunstitch.xml").expanded();

TEST(TgoCassisunstitch, TgoCassisunstitchDefaultTest) {
  QTemporaryDir prefix;

  QVector<QString> args = {"from=data/tgoCassis/tgocassisunstitch/stitched-2016-11-26T22-50-27.381_crop.cub",
                           "outputprefix=" + prefix.path() + "/unstitched"};
  UserInterface options(APP_XML, args);

  try {
    tgocassisunstitch(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassisunstitch with stitched cube: " << e.what() << std::endl;
  }

  // Unstitched Pan Cube
  QString panFile = prefix.path() + "/unstitched_PAN.cub";
  Cube panCube(panFile);
  Pvl *panLabel = panCube.label();

  // Dimensions group
  EXPECT_EQ(panCube.sampleCount(), 2048);
  EXPECT_EQ(panCube.lineCount(), 280);
  EXPECT_EQ(panCube.bandCount(), 1);

  // Instrument Group
  PvlGroup &panInst = panLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(panInst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(panInst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(panInst["TargetName"][0], "Mars");
  EXPECT_EQ(panInst["StartTime"][0], "2016-11-26T22:50:27.381");
  EXPECT_EQ(panInst["SpaceCraftClockStartCount"][0], "2f01543594abe199");
  EXPECT_EQ(panInst["ExposureDuration"][0], "1.440e-003");
  EXPECT_EQ(int(panInst["SummingMode"]), 0);
  EXPECT_EQ(panInst["Filter"][0], "PAN");

  // Archive Group
  PvlGroup &panArch = panLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(panArch["DataSetId"][0], "TBD");
  EXPECT_EQ(panArch["ProductVersionId"][0], "UNK");
  EXPECT_EQ(panArch["ProductCreationTime"][0], "2017-10-03T10:50:12");
  EXPECT_DOUBLE_EQ(double(panArch["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(panArch["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(panArch["PredictMaximumExposureTime"]), 1.5952);
  EXPECT_DOUBLE_EQ(double(panArch["CassisOffNadirAngle"]), 10.032);
  EXPECT_DOUBLE_EQ(double(panArch["PredictedRepetitionFrequency"]), 367.5);
  EXPECT_DOUBLE_EQ(double(panArch["GroundTrackVelocity"]), 3.4686);
  EXPECT_DOUBLE_EQ(double(panArch["ForwardRotationAngle"]), 52.703);
  EXPECT_DOUBLE_EQ(double(panArch["SpiceMisalignment"]), 185.422);
  EXPECT_DOUBLE_EQ(double(panArch["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(panArch["FNumber"]), 6.50);
  EXPECT_EQ(int(panArch["ExposureTimeCommand"]), 150);
  EXPECT_EQ(int(panArch["FrameletNumber"]), 5);
  EXPECT_EQ(int(panArch["NumberOfFramelets"]), 40);
  EXPECT_EQ(int(panArch["ImageFrequency"]), 400000);
  EXPECT_EQ(int(panArch["NumberOfWindows"]), 6);
  EXPECT_EQ(int(panArch["UniqueIdentifier"]), 100799268);
  EXPECT_EQ(panArch["ExposureTimestamp"][0], "2f015435767e275a");
  EXPECT_DOUBLE_EQ(double(panArch["ExposureTimePEHK"]), 1.440e-003);
  EXPECT_DOUBLE_EQ(double(panArch["PixelsPossiblySaturated"]), 29.17);
  EXPECT_EQ(int(panArch["WindowCount"]), 0);
  EXPECT_EQ(int(panArch["Window1Binning"]), 0);
  EXPECT_EQ(int(panArch["Window1StartSample"]), 0);
  EXPECT_EQ(int(panArch["Window1EndSample"]), 2047);
  EXPECT_EQ(int(panArch["Window1StartLine"]), 354);
  EXPECT_EQ(int(panArch["Window1EndLine"]), 633);
  EXPECT_EQ(int(panArch["Window2Binning"]), 0);
  EXPECT_EQ(int(panArch["Window2StartSample"]), 0);
  EXPECT_EQ(int(panArch["Window2EndSample"]), 2047);
  EXPECT_EQ(int(panArch["Window2StartLine"]), 712);
  EXPECT_EQ(int(panArch["Window2EndLine"]), 966);
  EXPECT_EQ(int(panArch["Window3Binning"]), 1);
  EXPECT_EQ(int(panArch["Window3StartSample"]), 0);
  EXPECT_EQ(int(panArch["Window3EndSample"]), 2047);
  EXPECT_EQ(int(panArch["Window3StartLine"]), 1048);
  EXPECT_EQ(int(panArch["Window3EndLine"]), 1302);
  EXPECT_EQ(int(panArch["Window4Binning"]), 0);
  EXPECT_EQ(int(panArch["Window4StartSample"]), 1024);
  EXPECT_EQ(int(panArch["Window4EndSample"]), 1087);
  EXPECT_EQ(int(panArch["Window4StartLine"]), 1409);
  EXPECT_EQ(int(panArch["Window4EndLine"]), 1662);
  EXPECT_EQ(int(panArch["Window5Binning"]), 0);
  EXPECT_EQ(int(panArch["Window5StartSample"]), 640);
  EXPECT_EQ(int(panArch["Window5EndSample"]), 767);
  EXPECT_EQ(int(panArch["Window5StartLine"]), 200);
  EXPECT_EQ(int(panArch["Window5EndLine"]), 208);
  EXPECT_EQ(int(panArch["Window6Binning"]), 0);
  EXPECT_EQ(int(panArch["Window6StartSample"]), 1280);
  EXPECT_EQ(int(panArch["Window6EndSample"]), 1407);
  EXPECT_EQ(int(panArch["Window6StartLine"]), 1850);
  EXPECT_EQ(int(panArch["Window6EndLine"]), 1858);
  EXPECT_EQ(int(panArch["YearDoy"]), 2016331);

  // Bandbin Group
  PvlGroup &panBand = panLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(panBand["FilterName"][0], "PAN");
  EXPECT_DOUBLE_EQ(double(panBand["Center"]), 675.0);
  EXPECT_DOUBLE_EQ(double(panBand["Width"]), 250.0);
  EXPECT_EQ(panBand["NaifIkCode"][0], "-143421");

  // Kernels Group
  PvlGroup &panKern = panLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(panKern["NaifFrameCode"]), -143400);

  // Unstitched Red Cube
  QString redFile = prefix.path() + "/unstitched_RED.cub";
  Cube redCube(redFile);
  Pvl *redLabel = redCube.label();

  // Dimensions group
  EXPECT_EQ(redCube.sampleCount(), 2048);
  EXPECT_EQ(redCube.lineCount(), 256);
  EXPECT_EQ(redCube.bandCount(), 1);

  // Instrument Group
  PvlGroup &redInst = redLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(redInst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(redInst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(redInst["TargetName"][0], "Mars");
  EXPECT_EQ(redInst["StartTime"][0], "2016-11-26T22:50:27.381");
  EXPECT_EQ(redInst["SpaceCraftClockStartCount"][0], "2f01543594abe199");
  EXPECT_EQ(redInst["ExposureDuration"][0], "1.440e-003");
  EXPECT_EQ(int(redInst["SummingMode"]), 0);
  EXPECT_EQ(redInst["Filter"][0], "RED");

  // Archive Group
  PvlGroup &redArch = redLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(redArch["DataSetId"][0], "TBD");
  EXPECT_EQ(redArch["ProductVersionId"][0], "UNK");
  EXPECT_EQ(redArch["ProductCreationTime"][0], "2017-10-03T10:50:12");
  EXPECT_DOUBLE_EQ(double(redArch["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(redArch["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(redArch["PredictMaximumExposureTime"]), 1.5952);
  EXPECT_DOUBLE_EQ(double(redArch["CassisOffNadirAngle"]), 10.032);
  EXPECT_DOUBLE_EQ(double(redArch["PredictedRepetitionFrequency"]), 367.5);
  EXPECT_DOUBLE_EQ(double(redArch["GroundTrackVelocity"]), 3.4686);
  EXPECT_DOUBLE_EQ(double(redArch["ForwardRotationAngle"]), 52.703);
  EXPECT_DOUBLE_EQ(double(redArch["SpiceMisalignment"]), 185.422);
  EXPECT_DOUBLE_EQ(double(redArch["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(redArch["FNumber"]), 6.50);
  EXPECT_EQ(int(redArch["ExposureTimeCommand"]), 150);
  EXPECT_EQ(int(redArch["FrameletNumber"]), 5);
  EXPECT_EQ(int(redArch["NumberOfFramelets"]), 40);
  EXPECT_EQ(int(redArch["ImageFrequency"]), 400000);
  EXPECT_EQ(int(redArch["NumberOfWindows"]), 6);
  EXPECT_EQ(int(redArch["UniqueIdentifier"]), 100799268);
  EXPECT_EQ(redArch["ExposureTimestamp"][0], "2f015435767e275a");
  EXPECT_DOUBLE_EQ(double(redArch["ExposureTimePEHK"]), 1.440e-003);
  EXPECT_DOUBLE_EQ(double(redArch["PixelsPossiblySaturated"]), 0.16);
  EXPECT_EQ(int(redArch["WindowCount"]), 1);
  EXPECT_EQ(int(redArch["Window1Binning"]), 0);
  EXPECT_EQ(int(redArch["Window1StartSample"]), 0);
  EXPECT_EQ(int(redArch["Window1EndSample"]), 2047);
  EXPECT_EQ(int(redArch["Window1StartLine"]), 354);
  EXPECT_EQ(int(redArch["Window1EndLine"]), 632);
  EXPECT_EQ(int(redArch["Window2Binning"]), 0);
  EXPECT_EQ(int(redArch["Window2StartSample"]), 0);
  EXPECT_EQ(int(redArch["Window2EndSample"]), 2047);
  EXPECT_EQ(int(redArch["Window2StartLine"]), 712);
  EXPECT_EQ(int(redArch["Window2EndLine"]), 967);
  EXPECT_EQ(int(redArch["Window3Binning"]), 1);
  EXPECT_EQ(int(redArch["Window3StartSample"]), 0);
  EXPECT_EQ(int(redArch["Window3EndSample"]), 2047);
  EXPECT_EQ(int(redArch["Window3StartLine"]), 1048);
  EXPECT_EQ(int(redArch["Window3EndLine"]), 1302);
  EXPECT_EQ(int(redArch["Window4Binning"]), 0);
  EXPECT_EQ(int(redArch["Window4StartSample"]), 1024);
  EXPECT_EQ(int(redArch["Window4EndSample"]), 1087);
  EXPECT_EQ(int(redArch["Window4StartLine"]), 1409);
  EXPECT_EQ(int(redArch["Window4EndLine"]), 1662);
  EXPECT_EQ(int(redArch["Window5Binning"]), 0);
  EXPECT_EQ(int(redArch["Window5StartSample"]), 640);
  EXPECT_EQ(int(redArch["Window5EndSample"]), 767);
  EXPECT_EQ(int(redArch["Window5StartLine"]), 200);
  EXPECT_EQ(int(redArch["Window5EndLine"]), 208);
  EXPECT_EQ(int(redArch["Window6Binning"]), 0);
  EXPECT_EQ(int(redArch["Window6StartSample"]), 1280);
  EXPECT_EQ(int(redArch["Window6EndSample"]), 1407);
  EXPECT_EQ(int(redArch["Window6StartLine"]), 1850);
  EXPECT_EQ(int(redArch["Window6EndLine"]), 1858);
  EXPECT_EQ(int(redArch["YearDoy"]), 2016331);

  // Bandbin Group
  PvlGroup &redBand = redLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(redBand["FilterName"][0], "RED");
  EXPECT_DOUBLE_EQ(double(redBand["Center"]), 840.0);
  EXPECT_DOUBLE_EQ(double(redBand["Width"]), 100.0);
  EXPECT_EQ(redBand["NaifIkCode"][0], "-143422");

  // Kernels Group
  PvlGroup &redKern = redLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(redKern["NaifFrameCode"]), -143400);


  // Unstitched Blu Cube
  QString bluFile = prefix.path() + "/unstitched_BLU.cub";
  Cube bluCube(bluFile);
  Pvl *bluLabel = bluCube.label();

  // Dimensions group
  EXPECT_EQ(bluCube.sampleCount(), 64);
  EXPECT_EQ(bluCube.lineCount(), 218);
  EXPECT_EQ(bluCube.bandCount(), 1);

  // Instrument Group
  PvlGroup &bluInst = bluLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(bluInst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(bluInst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(bluInst["TargetName"][0], "Mars");
  EXPECT_EQ(bluInst["StartTime"][0], "2016-11-26T22:50:27.381");
  EXPECT_EQ(bluInst["SpaceCraftClockStartCount"][0], "2f01543594abe199");
  EXPECT_EQ(bluInst["ExposureDuration"][0], "1.440e-003");
  EXPECT_EQ(int(bluInst["SummingMode"]), 0);
  EXPECT_EQ(bluInst["Filter"][0], "BLU");

  // Archive Group
  PvlGroup &bluArch = bluLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(bluArch["DataSetId"][0], "TBD");
  EXPECT_EQ(bluArch["ProductVersionId"][0], "UNK");
  EXPECT_EQ(bluArch["ProductCreationTime"][0], "2017-10-03T10:50:12");
  EXPECT_DOUBLE_EQ(double(bluArch["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(bluArch["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(bluArch["PredictMaximumExposureTime"]), 1.5952);
  EXPECT_DOUBLE_EQ(double(bluArch["CassisOffNadirAngle"]), 10.032);
  EXPECT_DOUBLE_EQ(double(bluArch["PredictedRepetitionFrequency"]), 367.5);
  EXPECT_DOUBLE_EQ(double(bluArch["GroundTrackVelocity"]), 3.4686);
  EXPECT_DOUBLE_EQ(double(bluArch["ForwardRotationAngle"]), 52.703);
  EXPECT_DOUBLE_EQ(double(bluArch["SpiceMisalignment"]), 185.422);
  EXPECT_DOUBLE_EQ(double(bluArch["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(bluArch["FNumber"]), 6.50);
  EXPECT_EQ(int(bluArch["ExposureTimeCommand"]), 150);
  EXPECT_EQ(int(bluArch["FrameletNumber"]), 5);
  EXPECT_EQ(int(bluArch["NumberOfFramelets"]), 40);
  EXPECT_EQ(int(bluArch["ImageFrequency"]), 400000);
  EXPECT_EQ(int(bluArch["NumberOfWindows"]), 6);
  EXPECT_EQ(int(bluArch["UniqueIdentifier"]), 100799268);
  EXPECT_EQ(bluArch["ExposureTimestamp"][0], "2f015435767e275a");
  EXPECT_DOUBLE_EQ(double(bluArch["ExposureTimePEHK"]), 1.440e-003);
  EXPECT_DOUBLE_EQ(double(bluArch["PixelsPossiblySaturated"]), 0);
  EXPECT_EQ(int(bluArch["WindowCount"]), 3);
  EXPECT_EQ(int(bluArch["Window1Binning"]), 0);
  EXPECT_EQ(int(bluArch["Window1StartSample"]), 0);
  EXPECT_EQ(int(bluArch["Window1EndSample"]), 2047);
  EXPECT_EQ(int(bluArch["Window1StartLine"]), 354);
  EXPECT_EQ(int(bluArch["Window1EndLine"]), 632);
  EXPECT_EQ(int(bluArch["Window2Binning"]), 0);
  EXPECT_EQ(int(bluArch["Window2StartSample"]), 0);
  EXPECT_EQ(int(bluArch["Window2EndSample"]), 2047);
  EXPECT_EQ(int(bluArch["Window2StartLine"]), 712);
  EXPECT_EQ(int(bluArch["Window2EndLine"]), 966);
  EXPECT_EQ(int(bluArch["Window3Binning"]), 1);
  EXPECT_EQ(int(bluArch["Window3StartSample"]), 0);
  EXPECT_EQ(int(bluArch["Window3EndSample"]), 2047);
  EXPECT_EQ(int(bluArch["Window3StartLine"]), 1048);
  EXPECT_EQ(int(bluArch["Window3EndLine"]), 1302);
  EXPECT_EQ(int(bluArch["Window4Binning"]), 0);
  EXPECT_EQ(int(bluArch["Window4StartSample"]), 1024);
  EXPECT_EQ(int(bluArch["Window4EndSample"]), 1087);
  EXPECT_EQ(int(bluArch["Window4StartLine"]), 1409);
  EXPECT_EQ(int(bluArch["Window4EndLine"]), 1626);
  EXPECT_EQ(int(bluArch["Window5Binning"]), 0);
  EXPECT_EQ(int(bluArch["Window5StartSample"]), 640);
  EXPECT_EQ(int(bluArch["Window5EndSample"]), 767);
  EXPECT_EQ(int(bluArch["Window5StartLine"]), 200);
  EXPECT_EQ(int(bluArch["Window5EndLine"]), 208);
  EXPECT_EQ(int(bluArch["Window6Binning"]), 0);
  EXPECT_EQ(int(bluArch["Window6StartSample"]), 1280);
  EXPECT_EQ(int(bluArch["Window6EndSample"]), 1407);
  EXPECT_EQ(int(bluArch["Window6StartLine"]), 1850);
  EXPECT_EQ(int(bluArch["Window6EndLine"]), 1858);
  EXPECT_EQ(int(bluArch["YearDoy"]), 2016331);

  // Bandbin Group
  PvlGroup &bluBand = bluLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bluBand["FilterName"][0], "BLU");
  EXPECT_DOUBLE_EQ(double(bluBand["Center"]), 485.0);
  EXPECT_DOUBLE_EQ(double(bluBand["Width"]), 165.0);
  EXPECT_EQ(bluBand["NaifIkCode"][0], "-143424");

  // Kernels Group
  PvlGroup &bluKern = bluLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(bluKern["NaifFrameCode"]), -143400);


  // Unstitched Nir Cube
  QString nirFile = prefix.path() + "/unstitched_NIR.cub";
  Cube nirCube(nirFile);
  Pvl *nirLabel = nirCube.label();

  // Dimensions group
  EXPECT_EQ(nirCube.sampleCount(), 2048);
  EXPECT_EQ(nirCube.lineCount(), 256);
  EXPECT_EQ(nirCube.bandCount(), 1);

  // Instrument Group
  PvlGroup &nirInst = nirLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(nirInst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(nirInst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(nirInst["TargetName"][0], "Mars");
  EXPECT_EQ(nirInst["StartTime"][0], "2016-11-26T22:50:27.381");
  EXPECT_EQ(nirInst["SpaceCraftClockStartCount"][0], "2f01543594abe199");
  EXPECT_EQ(nirInst["ExposureDuration"][0], "1.440e-003");
  EXPECT_EQ(int(nirInst["SummingMode"]), 0);
  EXPECT_EQ(nirInst["Filter"][0], "NIR");

  // Archive Group
  PvlGroup &nirArch = nirLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(nirArch["DataSetId"][0], "TBD");
  EXPECT_EQ(nirArch["ProductVersionId"][0], "UNK");
  EXPECT_EQ(nirArch["ProductCreationTime"][0], "2017-10-03T10:50:12");
  EXPECT_DOUBLE_EQ(double(nirArch["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(nirArch["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(nirArch["PredictMaximumExposureTime"]), 1.5952);
  EXPECT_DOUBLE_EQ(double(nirArch["CassisOffNadirAngle"]), 10.032);
  EXPECT_DOUBLE_EQ(double(nirArch["PredictedRepetitionFrequency"]), 367.5);
  EXPECT_DOUBLE_EQ(double(nirArch["GroundTrackVelocity"]), 3.4686);
  EXPECT_DOUBLE_EQ(double(nirArch["ForwardRotationAngle"]), 52.703);
  EXPECT_DOUBLE_EQ(double(nirArch["SpiceMisalignment"]), 185.422);
  EXPECT_DOUBLE_EQ(double(nirArch["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(nirArch["FNumber"]), 6.50);
  EXPECT_EQ(int(nirArch["ExposureTimeCommand"]), 150);
  EXPECT_EQ(int(nirArch["FrameletNumber"]), 5);
  EXPECT_EQ(int(nirArch["NumberOfFramelets"]), 40);
  EXPECT_EQ(int(nirArch["ImageFrequency"]), 400000);
  EXPECT_EQ(int(nirArch["NumberOfWindows"]), 6);
  EXPECT_EQ(int(nirArch["UniqueIdentifier"]), 100799268);
  EXPECT_EQ(nirArch["ExposureTimestamp"][0], "2f015435767e275a");
  EXPECT_DOUBLE_EQ(double(nirArch["ExposureTimePEHK"]), 1.440e-003);
  EXPECT_DOUBLE_EQ(double(nirArch["PixelsPossiblySaturated"]), 0);
  EXPECT_EQ(int(nirArch["WindowCount"]), 2);
  EXPECT_EQ(int(nirArch["Window1Binning"]), 0);
  EXPECT_EQ(int(nirArch["Window1StartSample"]), 0);
  EXPECT_EQ(int(nirArch["Window1EndSample"]), 2047);
  EXPECT_EQ(int(nirArch["Window1StartLine"]), 354);
  EXPECT_EQ(int(nirArch["Window1EndLine"]), 632);
  EXPECT_EQ(int(nirArch["Window2Binning"]), 0);
  EXPECT_EQ(int(nirArch["Window2StartSample"]), 0);
  EXPECT_EQ(int(nirArch["Window2EndSample"]), 2047);
  EXPECT_EQ(int(nirArch["Window2StartLine"]), 712);
  EXPECT_EQ(int(nirArch["Window2EndLine"]), 966);
  EXPECT_EQ(int(nirArch["Window3Binning"]), 1);
  EXPECT_EQ(int(nirArch["Window3StartSample"]), 0);
  EXPECT_EQ(int(nirArch["Window3EndSample"]), 2047);
  EXPECT_EQ(int(nirArch["Window3StartLine"]), 1048);
  EXPECT_EQ(int(nirArch["Window3EndLine"]), 1303);
  EXPECT_EQ(int(nirArch["Window4Binning"]), 0);
  EXPECT_EQ(int(nirArch["Window4StartSample"]), 1024);
  EXPECT_EQ(int(nirArch["Window4EndSample"]), 1087);
  EXPECT_EQ(int(nirArch["Window4StartLine"]), 1409);
  EXPECT_EQ(int(nirArch["Window4EndLine"]), 1662);
  EXPECT_EQ(int(nirArch["Window5Binning"]), 0);
  EXPECT_EQ(int(nirArch["Window5StartSample"]), 640);
  EXPECT_EQ(int(nirArch["Window5EndSample"]), 767);
  EXPECT_EQ(int(nirArch["Window5StartLine"]), 200);
  EXPECT_EQ(int(nirArch["Window5EndLine"]), 208);
  EXPECT_EQ(int(nirArch["Window6Binning"]), 0);
  EXPECT_EQ(int(nirArch["Window6StartSample"]), 1280);
  EXPECT_EQ(int(nirArch["Window6EndSample"]), 1407);
  EXPECT_EQ(int(nirArch["Window6StartLine"]), 1850);
  EXPECT_EQ(int(nirArch["Window6EndLine"]), 1858);
  EXPECT_EQ(int(nirArch["YearDoy"]), 2016331);

  // Bandbin Group
  PvlGroup &nirBand = nirLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(nirBand["FilterName"][0], "NIR");
  EXPECT_DOUBLE_EQ(double(nirBand["Center"]), 985.0);
  EXPECT_DOUBLE_EQ(double(nirBand["Width"]), 220.0);
  EXPECT_EQ(nirBand["NaifIkCode"][0], "-143423");

  // Kernels Group
  PvlGroup &nirKern = nirLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(nirKern["NaifFrameCode"]), -143400);
}


TEST(TgoCassisunstitch, TgoCassisunstitchSpiceTest) {
  QTemporaryDir prefix;

  QVector<QString> args = {"from=data/tgoCassis/tgocassisunstitch/stitched-spice-2016-11-26T22-50-27.381_crop.cub",
                           "outputprefix=" + prefix.path() + "/unstitched"};
  UserInterface options(APP_XML, args);

  try {
    tgocassisunstitch(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassisunstitch with stitched cube: " << e.what() << std::endl;
  }

  // Unstitched Pan Cube
  QString panFile = prefix.path() + "/unstitched_PAN.cub";
  Cube panCube(panFile);
  Pvl *panLabel = panCube.label();

  // Dimensions group
  EXPECT_EQ(panCube.sampleCount(), 2048);
  EXPECT_EQ(panCube.lineCount(), 280);
  EXPECT_EQ(panCube.bandCount(), 1);

  // Instrument Group
  PvlGroup &panInst = panLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(panInst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(panInst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(panInst["TargetName"][0], "Mars");
  EXPECT_EQ(panInst["StartTime"][0], "2016-11-26T22:50:27.381");
  EXPECT_EQ(panInst["SpaceCraftClockStartCount"][0], "2f01543594abe199");
  EXPECT_EQ(panInst["ExposureDuration"][0], "1.440e-003");
  EXPECT_EQ(int(panInst["SummingMode"]), 0);
  EXPECT_EQ(panInst["Filter"][0], "PAN");

  // Archive Group
  PvlGroup &panArch = panLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(panArch["DataSetId"][0], "TBD");
  EXPECT_EQ(panArch["ProductVersionId"][0], "UNK");
  EXPECT_EQ(panArch["ProductCreationTime"][0], "2017-10-03T10:50:12");
  EXPECT_DOUBLE_EQ(double(panArch["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(panArch["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(panArch["PredictMaximumExposureTime"]), 1.5952);
  EXPECT_DOUBLE_EQ(double(panArch["CassisOffNadirAngle"]), 10.032);
  EXPECT_DOUBLE_EQ(double(panArch["PredictedRepetitionFrequency"]), 367.5);
  EXPECT_DOUBLE_EQ(double(panArch["GroundTrackVelocity"]), 3.4686);
  EXPECT_DOUBLE_EQ(double(panArch["ForwardRotationAngle"]), 52.703);
  EXPECT_DOUBLE_EQ(double(panArch["SpiceMisalignment"]), 185.422);
  EXPECT_DOUBLE_EQ(double(panArch["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(panArch["FNumber"]), 6.50);
  EXPECT_EQ(int(panArch["ExposureTimeCommand"]), 150);
  EXPECT_EQ(int(panArch["FrameletNumber"]), 5);
  EXPECT_EQ(int(panArch["NumberOfFramelets"]), 40);
  EXPECT_EQ(int(panArch["ImageFrequency"]), 400000);
  EXPECT_EQ(int(panArch["NumberOfWindows"]), 6);
  EXPECT_EQ(int(panArch["UniqueIdentifier"]), 100799268);
  EXPECT_EQ(panArch["ExposureTimestamp"][0], "2f015435767e275a");
  EXPECT_DOUBLE_EQ(double(panArch["ExposureTimePEHK"]), 1.440e-003);
  EXPECT_DOUBLE_EQ(double(panArch["PixelsPossiblySaturated"]), 29.17);
  EXPECT_EQ(int(panArch["WindowCount"]), 0);
  EXPECT_EQ(int(panArch["Window1Binning"]), 0);
  EXPECT_EQ(int(panArch["Window1StartSample"]), 0);
  EXPECT_EQ(int(panArch["Window1EndSample"]), 2047);
  EXPECT_EQ(int(panArch["Window1StartLine"]), 354);
  EXPECT_EQ(int(panArch["Window1EndLine"]), 633);
  EXPECT_EQ(int(panArch["Window2Binning"]), 0);
  EXPECT_EQ(int(panArch["Window2StartSample"]), 0);
  EXPECT_EQ(int(panArch["Window2EndSample"]), 2047);
  EXPECT_EQ(int(panArch["Window2StartLine"]), 712);
  EXPECT_EQ(int(panArch["Window2EndLine"]), 966);
  EXPECT_EQ(int(panArch["Window3Binning"]), 1);
  EXPECT_EQ(int(panArch["Window3StartSample"]), 0);
  EXPECT_EQ(int(panArch["Window3EndSample"]), 2047);
  EXPECT_EQ(int(panArch["Window3StartLine"]), 1048);
  EXPECT_EQ(int(panArch["Window3EndLine"]), 1302);
  EXPECT_EQ(int(panArch["Window4Binning"]), 0);
  EXPECT_EQ(int(panArch["Window4StartSample"]), 1024);
  EXPECT_EQ(int(panArch["Window4EndSample"]), 1087);
  EXPECT_EQ(int(panArch["Window4StartLine"]), 1409);
  EXPECT_EQ(int(panArch["Window4EndLine"]), 1662);
  EXPECT_EQ(int(panArch["Window5Binning"]), 0);
  EXPECT_EQ(int(panArch["Window5StartSample"]), 640);
  EXPECT_EQ(int(panArch["Window5EndSample"]), 767);
  EXPECT_EQ(int(panArch["Window5StartLine"]), 200);
  EXPECT_EQ(int(panArch["Window5EndLine"]), 208);
  EXPECT_EQ(int(panArch["Window6Binning"]), 0);
  EXPECT_EQ(int(panArch["Window6StartSample"]), 1280);
  EXPECT_EQ(int(panArch["Window6EndSample"]), 1407);
  EXPECT_EQ(int(panArch["Window6StartLine"]), 1850);
  EXPECT_EQ(int(panArch["Window6EndLine"]), 1858);
  EXPECT_EQ(int(panArch["YearDoy"]), 2016331);

  // Bandbin Group
  PvlGroup &panBand = panLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(panBand["FilterName"][0], "PAN");
  EXPECT_DOUBLE_EQ(double(panBand["Center"]), 675.0);
  EXPECT_DOUBLE_EQ(double(panBand["Width"]), 250.0);
  EXPECT_EQ(panBand["NaifIkCode"][0], "-143421");

  // Kernels Group
  PvlGroup &panKern = panLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(panKern["NaifFrameCode"]), -143400);
  EXPECT_TRUE(panKern.hasKeyword("LeapSecond"));
  EXPECT_TRUE(panKern.hasKeyword("TargetAttitudeShape"));
  EXPECT_TRUE(panKern.hasKeyword("TargetPosition"));
  EXPECT_TRUE(panKern.hasKeyword("InstrumentPointing"));
  EXPECT_TRUE(panKern.hasKeyword("Instrument"));
  EXPECT_TRUE(panKern.hasKeyword("SpacecraftClock"));
  EXPECT_TRUE(panKern.hasKeyword("InstrumentPosition"));
  EXPECT_TRUE(panKern.hasKeyword("InstrumentAddendum"));
  EXPECT_TRUE(panKern.hasKeyword("ShapeModel"));

  // Unstitched Red Cube
  QString redFile = prefix.path() + "/unstitched_RED.cub";
  Cube redCube(redFile);
  Pvl *redLabel = redCube.label();

  // Dimensions group
  EXPECT_EQ(redCube.sampleCount(), 2048);
  EXPECT_EQ(redCube.lineCount(), 256);
  EXPECT_EQ(redCube.bandCount(), 1);

  // Instrument Group
  PvlGroup &redInst = redLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(redInst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(redInst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(redInst["TargetName"][0], "Mars");
  EXPECT_EQ(redInst["StartTime"][0], "2016-11-26T22:50:27.381");
  EXPECT_EQ(redInst["SpaceCraftClockStartCount"][0], "2f01543594abe199");
  EXPECT_EQ(redInst["ExposureDuration"][0], "1.440e-003");
  EXPECT_EQ(int(redInst["SummingMode"]), 0);
  EXPECT_EQ(redInst["Filter"][0], "RED");

  // Archive Group
  PvlGroup &redArch = redLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(redArch["DataSetId"][0], "TBD");
  EXPECT_EQ(redArch["ProductVersionId"][0], "UNK");
  EXPECT_EQ(redArch["ProductCreationTime"][0], "2017-10-03T10:50:12");
  EXPECT_DOUBLE_EQ(double(redArch["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(redArch["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(redArch["PredictMaximumExposureTime"]), 1.5952);
  EXPECT_DOUBLE_EQ(double(redArch["CassisOffNadirAngle"]), 10.032);
  EXPECT_DOUBLE_EQ(double(redArch["PredictedRepetitionFrequency"]), 367.5);
  EXPECT_DOUBLE_EQ(double(redArch["GroundTrackVelocity"]), 3.4686);
  EXPECT_DOUBLE_EQ(double(redArch["ForwardRotationAngle"]), 52.703);
  EXPECT_DOUBLE_EQ(double(redArch["SpiceMisalignment"]), 185.422);
  EXPECT_DOUBLE_EQ(double(redArch["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(redArch["FNumber"]), 6.50);
  EXPECT_EQ(int(redArch["ExposureTimeCommand"]), 150);
  EXPECT_EQ(int(redArch["FrameletNumber"]), 5);
  EXPECT_EQ(int(redArch["NumberOfFramelets"]), 40);
  EXPECT_EQ(int(redArch["ImageFrequency"]), 400000);
  EXPECT_EQ(int(redArch["NumberOfWindows"]), 6);
  EXPECT_EQ(int(redArch["UniqueIdentifier"]), 100799268);
  EXPECT_EQ(redArch["ExposureTimestamp"][0], "2f015435767e275a");
  EXPECT_DOUBLE_EQ(double(redArch["ExposureTimePEHK"]), 1.440e-003);
  EXPECT_DOUBLE_EQ(double(redArch["PixelsPossiblySaturated"]), 0.16);
  EXPECT_EQ(int(redArch["WindowCount"]), 1);
  EXPECT_EQ(int(redArch["Window1Binning"]), 0);
  EXPECT_EQ(int(redArch["Window1StartSample"]), 0);
  EXPECT_EQ(int(redArch["Window1EndSample"]), 2047);
  EXPECT_EQ(int(redArch["Window1StartLine"]), 354);
  EXPECT_EQ(int(redArch["Window1EndLine"]), 632);
  EXPECT_EQ(int(redArch["Window2Binning"]), 0);
  EXPECT_EQ(int(redArch["Window2StartSample"]), 0);
  EXPECT_EQ(int(redArch["Window2EndSample"]), 2047);
  EXPECT_EQ(int(redArch["Window2StartLine"]), 712);
  EXPECT_EQ(int(redArch["Window2EndLine"]), 967);
  EXPECT_EQ(int(redArch["Window3Binning"]), 1);
  EXPECT_EQ(int(redArch["Window3StartSample"]), 0);
  EXPECT_EQ(int(redArch["Window3EndSample"]), 2047);
  EXPECT_EQ(int(redArch["Window3StartLine"]), 1048);
  EXPECT_EQ(int(redArch["Window3EndLine"]), 1302);
  EXPECT_EQ(int(redArch["Window4Binning"]), 0);
  EXPECT_EQ(int(redArch["Window4StartSample"]), 1024);
  EXPECT_EQ(int(redArch["Window4EndSample"]), 1087);
  EXPECT_EQ(int(redArch["Window4StartLine"]), 1409);
  EXPECT_EQ(int(redArch["Window4EndLine"]), 1662);
  EXPECT_EQ(int(redArch["Window5Binning"]), 0);
  EXPECT_EQ(int(redArch["Window5StartSample"]), 640);
  EXPECT_EQ(int(redArch["Window5EndSample"]), 767);
  EXPECT_EQ(int(redArch["Window5StartLine"]), 200);
  EXPECT_EQ(int(redArch["Window5EndLine"]), 208);
  EXPECT_EQ(int(redArch["Window6Binning"]), 0);
  EXPECT_EQ(int(redArch["Window6StartSample"]), 1280);
  EXPECT_EQ(int(redArch["Window6EndSample"]), 1407);
  EXPECT_EQ(int(redArch["Window6StartLine"]), 1850);
  EXPECT_EQ(int(redArch["Window6EndLine"]), 1858);
  EXPECT_EQ(int(redArch["YearDoy"]), 2016331);

  // Bandbin Group
  PvlGroup &redBand = redLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(redBand["FilterName"][0], "RED");
  EXPECT_DOUBLE_EQ(double(redBand["Center"]), 840.0);
  EXPECT_DOUBLE_EQ(double(redBand["Width"]), 100.0);
  EXPECT_EQ(redBand["NaifIkCode"][0], "-143422");

  // Kernels Group
  PvlGroup &redKern = redLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(redKern["NaifFrameCode"]), -143400);
  EXPECT_TRUE(redKern.hasKeyword("LeapSecond"));
  EXPECT_TRUE(redKern.hasKeyword("TargetAttitudeShape"));
  EXPECT_TRUE(redKern.hasKeyword("TargetPosition"));
  EXPECT_TRUE(redKern.hasKeyword("InstrumentPointing"));
  EXPECT_TRUE(redKern.hasKeyword("Instrument"));
  EXPECT_TRUE(redKern.hasKeyword("SpacecraftClock"));
  EXPECT_TRUE(redKern.hasKeyword("InstrumentPosition"));
  EXPECT_TRUE(redKern.hasKeyword("InstrumentAddendum"));
  EXPECT_TRUE(redKern.hasKeyword("ShapeModel"));


  // Unstitched Blu Cube
  QString bluFile = prefix.path() + "/unstitched_BLU.cub";
  Cube bluCube(bluFile);
  Pvl *bluLabel = bluCube.label();

  // Dimensions group
  EXPECT_EQ(bluCube.sampleCount(), 64);
  EXPECT_EQ(bluCube.lineCount(), 218);
  EXPECT_EQ(bluCube.bandCount(), 1);

  // Instrument Group
  PvlGroup &bluInst = bluLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(bluInst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(bluInst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(bluInst["TargetName"][0], "Mars");
  EXPECT_EQ(bluInst["StartTime"][0], "2016-11-26T22:50:27.381");
  EXPECT_EQ(bluInst["SpaceCraftClockStartCount"][0], "2f01543594abe199");
  EXPECT_EQ(bluInst["ExposureDuration"][0], "1.440e-003");
  EXPECT_EQ(int(bluInst["SummingMode"]), 0);
  EXPECT_EQ(bluInst["Filter"][0], "BLU");

  // Archive Group
  PvlGroup &bluArch = bluLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(bluArch["DataSetId"][0], "TBD");
  EXPECT_EQ(bluArch["ProductVersionId"][0], "UNK");
  EXPECT_EQ(bluArch["ProductCreationTime"][0], "2017-10-03T10:50:12");
  EXPECT_DOUBLE_EQ(double(bluArch["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(bluArch["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(bluArch["PredictMaximumExposureTime"]), 1.5952);
  EXPECT_DOUBLE_EQ(double(bluArch["CassisOffNadirAngle"]), 10.032);
  EXPECT_DOUBLE_EQ(double(bluArch["PredictedRepetitionFrequency"]), 367.5);
  EXPECT_DOUBLE_EQ(double(bluArch["GroundTrackVelocity"]), 3.4686);
  EXPECT_DOUBLE_EQ(double(bluArch["ForwardRotationAngle"]), 52.703);
  EXPECT_DOUBLE_EQ(double(bluArch["SpiceMisalignment"]), 185.422);
  EXPECT_DOUBLE_EQ(double(bluArch["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(bluArch["FNumber"]), 6.50);
  EXPECT_EQ(int(bluArch["ExposureTimeCommand"]), 150);
  EXPECT_EQ(int(bluArch["FrameletNumber"]), 5);
  EXPECT_EQ(int(bluArch["NumberOfFramelets"]), 40);
  EXPECT_EQ(int(bluArch["ImageFrequency"]), 400000);
  EXPECT_EQ(int(bluArch["NumberOfWindows"]), 6);
  EXPECT_EQ(int(bluArch["UniqueIdentifier"]), 100799268);
  EXPECT_EQ(bluArch["ExposureTimestamp"][0], "2f015435767e275a");
  EXPECT_DOUBLE_EQ(double(bluArch["ExposureTimePEHK"]), 1.440e-003);
  EXPECT_DOUBLE_EQ(double(bluArch["PixelsPossiblySaturated"]), 0);
  EXPECT_EQ(int(bluArch["WindowCount"]), 3);
  EXPECT_EQ(int(bluArch["Window1Binning"]), 0);
  EXPECT_EQ(int(bluArch["Window1StartSample"]), 0);
  EXPECT_EQ(int(bluArch["Window1EndSample"]), 2047);
  EXPECT_EQ(int(bluArch["Window1StartLine"]), 354);
  EXPECT_EQ(int(bluArch["Window1EndLine"]), 632);
  EXPECT_EQ(int(bluArch["Window2Binning"]), 0);
  EXPECT_EQ(int(bluArch["Window2StartSample"]), 0);
  EXPECT_EQ(int(bluArch["Window2EndSample"]), 2047);
  EXPECT_EQ(int(bluArch["Window2StartLine"]), 712);
  EXPECT_EQ(int(bluArch["Window2EndLine"]), 966);
  EXPECT_EQ(int(bluArch["Window3Binning"]), 1);
  EXPECT_EQ(int(bluArch["Window3StartSample"]), 0);
  EXPECT_EQ(int(bluArch["Window3EndSample"]), 2047);
  EXPECT_EQ(int(bluArch["Window3StartLine"]), 1048);
  EXPECT_EQ(int(bluArch["Window3EndLine"]), 1302);
  EXPECT_EQ(int(bluArch["Window4Binning"]), 0);
  EXPECT_EQ(int(bluArch["Window4StartSample"]), 1024);
  EXPECT_EQ(int(bluArch["Window4EndSample"]), 1087);
  EXPECT_EQ(int(bluArch["Window4StartLine"]), 1409);
  EXPECT_EQ(int(bluArch["Window4EndLine"]), 1626);
  EXPECT_EQ(int(bluArch["Window5Binning"]), 0);
  EXPECT_EQ(int(bluArch["Window5StartSample"]), 640);
  EXPECT_EQ(int(bluArch["Window5EndSample"]), 767);
  EXPECT_EQ(int(bluArch["Window5StartLine"]), 200);
  EXPECT_EQ(int(bluArch["Window5EndLine"]), 208);
  EXPECT_EQ(int(bluArch["Window6Binning"]), 0);
  EXPECT_EQ(int(bluArch["Window6StartSample"]), 1280);
  EXPECT_EQ(int(bluArch["Window6EndSample"]), 1407);
  EXPECT_EQ(int(bluArch["Window6StartLine"]), 1850);
  EXPECT_EQ(int(bluArch["Window6EndLine"]), 1858);
  EXPECT_EQ(int(bluArch["YearDoy"]), 2016331);

  // Bandbin Group
  PvlGroup &bluBand = bluLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bluBand["FilterName"][0], "BLU");
  EXPECT_DOUBLE_EQ(double(bluBand["Center"]), 485.0);
  EXPECT_DOUBLE_EQ(double(bluBand["Width"]), 165.0);
  EXPECT_EQ(bluBand["NaifIkCode"][0], "-143424");

  // Kernels Group
  PvlGroup &bluKern = bluLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(bluKern["NaifFrameCode"]), -143400);
  EXPECT_TRUE(bluKern.hasKeyword("LeapSecond"));
  EXPECT_TRUE(bluKern.hasKeyword("TargetAttitudeShape"));
  EXPECT_TRUE(bluKern.hasKeyword("TargetPosition"));
  EXPECT_TRUE(bluKern.hasKeyword("InstrumentPointing"));
  EXPECT_TRUE(bluKern.hasKeyword("Instrument"));
  EXPECT_TRUE(bluKern.hasKeyword("SpacecraftClock"));
  EXPECT_TRUE(bluKern.hasKeyword("InstrumentPosition"));
  EXPECT_TRUE(bluKern.hasKeyword("InstrumentAddendum"));
  EXPECT_TRUE(bluKern.hasKeyword("ShapeModel"));


  // Unstitched Nir Cube
  QString nirFile = prefix.path() + "/unstitched_NIR.cub";
  Cube nirCube(nirFile);
  Pvl *nirLabel = nirCube.label();

  // Dimensions group
  EXPECT_EQ(nirCube.sampleCount(), 2048);
  EXPECT_EQ(nirCube.lineCount(), 256);
  EXPECT_EQ(nirCube.bandCount(), 1);

  // Instrument Group
  PvlGroup &nirInst = nirLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(nirInst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(nirInst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(nirInst["TargetName"][0], "Mars");
  EXPECT_EQ(nirInst["StartTime"][0], "2016-11-26T22:50:27.381");
  EXPECT_EQ(nirInst["SpaceCraftClockStartCount"][0], "2f01543594abe199");
  EXPECT_EQ(nirInst["ExposureDuration"][0], "1.440e-003");
  EXPECT_EQ(int(nirInst["SummingMode"]), 0);
  EXPECT_EQ(nirInst["Filter"][0], "NIR");

  // Archive Group
  PvlGroup &nirArch = nirLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(nirArch["DataSetId"][0], "TBD");
  EXPECT_EQ(nirArch["ProductVersionId"][0], "UNK");
  EXPECT_EQ(nirArch["ProductCreationTime"][0], "2017-10-03T10:50:12");
  EXPECT_DOUBLE_EQ(double(nirArch["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(nirArch["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(nirArch["PredictMaximumExposureTime"]), 1.5952);
  EXPECT_DOUBLE_EQ(double(nirArch["CassisOffNadirAngle"]), 10.032);
  EXPECT_DOUBLE_EQ(double(nirArch["PredictedRepetitionFrequency"]), 367.5);
  EXPECT_DOUBLE_EQ(double(nirArch["GroundTrackVelocity"]), 3.4686);
  EXPECT_DOUBLE_EQ(double(nirArch["ForwardRotationAngle"]), 52.703);
  EXPECT_DOUBLE_EQ(double(nirArch["SpiceMisalignment"]), 185.422);
  EXPECT_DOUBLE_EQ(double(nirArch["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(nirArch["FNumber"]), 6.50);
  EXPECT_EQ(int(nirArch["ExposureTimeCommand"]), 150);
  EXPECT_EQ(int(nirArch["FrameletNumber"]), 5);
  EXPECT_EQ(int(nirArch["NumberOfFramelets"]), 40);
  EXPECT_EQ(int(nirArch["ImageFrequency"]), 400000);
  EXPECT_EQ(int(nirArch["NumberOfWindows"]), 6);
  EXPECT_EQ(int(nirArch["UniqueIdentifier"]), 100799268);
  EXPECT_EQ(nirArch["ExposureTimestamp"][0], "2f015435767e275a");
  EXPECT_DOUBLE_EQ(double(nirArch["ExposureTimePEHK"]), 1.440e-003);
  EXPECT_DOUBLE_EQ(double(nirArch["PixelsPossiblySaturated"]), 0);
  EXPECT_EQ(int(nirArch["WindowCount"]), 2);
  EXPECT_EQ(int(nirArch["Window1Binning"]), 0);
  EXPECT_EQ(int(nirArch["Window1StartSample"]), 0);
  EXPECT_EQ(int(nirArch["Window1EndSample"]), 2047);
  EXPECT_EQ(int(nirArch["Window1StartLine"]), 354);
  EXPECT_EQ(int(nirArch["Window1EndLine"]), 632);
  EXPECT_EQ(int(nirArch["Window2Binning"]), 0);
  EXPECT_EQ(int(nirArch["Window2StartSample"]), 0);
  EXPECT_EQ(int(nirArch["Window2EndSample"]), 2047);
  EXPECT_EQ(int(nirArch["Window2StartLine"]), 712);
  EXPECT_EQ(int(nirArch["Window2EndLine"]), 966);
  EXPECT_EQ(int(nirArch["Window3Binning"]), 1);
  EXPECT_EQ(int(nirArch["Window3StartSample"]), 0);
  EXPECT_EQ(int(nirArch["Window3EndSample"]), 2047);
  EXPECT_EQ(int(nirArch["Window3StartLine"]), 1048);
  EXPECT_EQ(int(nirArch["Window3EndLine"]), 1303);
  EXPECT_EQ(int(nirArch["Window4Binning"]), 0);
  EXPECT_EQ(int(nirArch["Window4StartSample"]), 1024);
  EXPECT_EQ(int(nirArch["Window4EndSample"]), 1087);
  EXPECT_EQ(int(nirArch["Window4StartLine"]), 1409);
  EXPECT_EQ(int(nirArch["Window4EndLine"]), 1662);
  EXPECT_EQ(int(nirArch["Window5Binning"]), 0);
  EXPECT_EQ(int(nirArch["Window5StartSample"]), 640);
  EXPECT_EQ(int(nirArch["Window5EndSample"]), 767);
  EXPECT_EQ(int(nirArch["Window5StartLine"]), 200);
  EXPECT_EQ(int(nirArch["Window5EndLine"]), 208);
  EXPECT_EQ(int(nirArch["Window6Binning"]), 0);
  EXPECT_EQ(int(nirArch["Window6StartSample"]), 1280);
  EXPECT_EQ(int(nirArch["Window6EndSample"]), 1407);
  EXPECT_EQ(int(nirArch["Window6StartLine"]), 1850);
  EXPECT_EQ(int(nirArch["Window6EndLine"]), 1858);
  EXPECT_EQ(int(nirArch["YearDoy"]), 2016331);

  // Bandbin Group
  PvlGroup &nirBand = nirLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(nirBand["FilterName"][0], "NIR");
  EXPECT_DOUBLE_EQ(double(nirBand["Center"]), 985.0);
  EXPECT_DOUBLE_EQ(double(nirBand["Width"]), 220.0);
  EXPECT_EQ(nirBand["NaifIkCode"][0], "-143423");

  // Kernels Group
  PvlGroup &nirKern = nirLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(nirKern["NaifFrameCode"]), -143400);
  EXPECT_TRUE(nirKern.hasKeyword("LeapSecond"));
  EXPECT_TRUE(nirKern.hasKeyword("TargetAttitudeShape"));
  EXPECT_TRUE(nirKern.hasKeyword("TargetPosition"));
  EXPECT_TRUE(nirKern.hasKeyword("InstrumentPointing"));
  EXPECT_TRUE(nirKern.hasKeyword("Instrument"));
  EXPECT_TRUE(nirKern.hasKeyword("SpacecraftClock"));
  EXPECT_TRUE(nirKern.hasKeyword("InstrumentPosition"));
  EXPECT_TRUE(nirKern.hasKeyword("InstrumentAddendum"));
  EXPECT_TRUE(nirKern.hasKeyword("ShapeModel"));
}