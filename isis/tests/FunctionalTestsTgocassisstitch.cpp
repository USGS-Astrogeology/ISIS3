#include <QTemporaryDir>

#include "tgocassisstitch.h"
#include "FileList.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "gtest/gtest.h"

using namespace Isis;
using ::testing::HasSubstr;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/tgocassisstitch.xml").expanded());

TEST(TgoCassisstitch, TgoCassisstitchMultiframeTest) {
  QTemporaryDir prefix;

  FileList *cubeList = new FileList();
  cubeList->append("data/tgoCassis/tgocassisstitch/CAS-MCO-2016-11-22T16.16.16.833-BLU-03006-B1.cub");
  cubeList->append("data/tgoCassis/tgocassisstitch/CAS-MCO-2016-11-22T16.16.16.833-RED-01006-B1_crop.cub");
  cubeList->append("data/tgoCassis/tgocassisstitch/CAS-MCO-2016-11-22T16.16.16.833-NIR-02006-B1_crop.cub");
  cubeList->append("data/tgoCassis/tgocassisstitch/CAS-MCO-2016-11-22T16.16.16.833-PAN-00006-B1_crop.cub");
  
  QString cubeListFile = prefix.path() + "/cubelist.lis";
  cubeList->write(cubeListFile.toStdString());

  QString cubeName = "default";

  QVector<QString> args = {"fromlist=" + cubeListFile,
                           "outputprefix=" + prefix.path() + "/CAS-MCO"};
  UserInterface options(APP_XML, args);

  try {
    tgocassisstitch(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassisstitch with cube list: " << e.what() << std::endl;
  }

  Cube cube(prefix.path().toStdString() + "/CAS-MCO-2016-11-22T16:16:16.833.cub");
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  EXPECT_EQ(cube.sampleCount(), 2048);
  EXPECT_EQ(cube.lineCount(), 2048);
  EXPECT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(inst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(inst["TargetName"][0], "Mars" );
  EXPECT_EQ(inst["StartTime"][0], "2016-11-22T16:16:16.833");
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "2f014e933c4a631f");
  EXPECT_EQ(inst["ExposureDuration"][0], "1.152e-003");
  EXPECT_EQ(int(inst["SummingMode"]), 0);
  EXPECT_EQ(inst["Filter"][0], "FULLCCD");

  // Red Archive Group
  PvlGroup &arcRed = isisLabel->findGroup("archiveRED", Pvl::Traverse);
  EXPECT_EQ(arcRed["DataSetId"][0], "TBD");
  EXPECT_EQ(arcRed["ProductVersionId"][0], "UNK");
  EXPECT_EQ(arcRed["ProductCreationTime"][0], "2017-10-03T09:38:29");
  EXPECT_DOUBLE_EQ(double(arcRed["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(arcRed["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(arcRed["PredictMaximumExposureTime"]), 5.2866);
  EXPECT_DOUBLE_EQ(double(arcRed["CassisOffNadirAngle"]), 9.923);
  EXPECT_DOUBLE_EQ(double(arcRed["PredictedRepetitionFrequency"]), 1218.0);
  EXPECT_DOUBLE_EQ(double(arcRed["GroundTrackVelocity"]), 2.6946);
  EXPECT_DOUBLE_EQ(double(arcRed["ForwardRotationAngle"]), 146.943);
  EXPECT_DOUBLE_EQ(double(arcRed["SpiceMisalignment"]), 351.195);
  EXPECT_DOUBLE_EQ(double(arcRed["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(arcRed["FNumber"]), 6.50);
  EXPECT_EQ(int(arcRed["ExposureTimeCommand"]), 120);
  EXPECT_EQ(int(arcRed["FrameletNumber"]), 6);
  EXPECT_EQ(int(arcRed["NumberOfFramelets"]), 30);
  EXPECT_EQ(int(arcRed["ImageFrequency"]), 1000000);
  EXPECT_EQ(int(arcRed["NumberOfWindows"]), 6);
  EXPECT_EQ(int(arcRed["UniqueIdentifier"]), 100732832);
  EXPECT_EQ(arcRed["ExposureTimestamp"][0], "2f014e931416226d");
  EXPECT_DOUBLE_EQ(double(arcRed["ExposureTimePEHK"]), 1.152e-003);
  EXPECT_DOUBLE_EQ(double(arcRed["PixelsPossiblySaturated"]), 0.00);
  EXPECT_EQ(int(arcRed["YearDoy"]), 2016327);

  // Pan Archive Group
  PvlGroup &arcPan = isisLabel->findGroup("archivePAN", Pvl::Traverse);
  EXPECT_EQ(arcPan["DataSetId"][0], "TBD");
  EXPECT_EQ(arcPan["ProductVersionId"][0], "UNK");
  EXPECT_EQ(arcPan["ProductCreationTime"][0], "2017-10-03T09:38:29");
  EXPECT_DOUBLE_EQ(double(arcPan["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(arcPan["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(arcPan["PredictMaximumExposureTime"]), 5.2866);
  EXPECT_DOUBLE_EQ(double(arcPan["CassisOffNadirAngle"]), 9.923);
  EXPECT_DOUBLE_EQ(double(arcPan["PredictedRepetitionFrequency"]), 1218.0);
  EXPECT_DOUBLE_EQ(double(arcPan["GroundTrackVelocity"]), 2.6946);
  EXPECT_DOUBLE_EQ(double(arcPan["ForwardRotationAngle"]), 146.943);
  EXPECT_DOUBLE_EQ(double(arcPan["SpiceMisalignment"]), 351.195);
  EXPECT_DOUBLE_EQ(double(arcPan["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(arcPan["FNumber"]), 6.50);
  EXPECT_EQ(int(arcPan["ExposureTimeCommand"]), 120);
  EXPECT_EQ(int(arcPan["FrameletNumber"]), 6);
  EXPECT_EQ(int(arcPan["NumberOfFramelets"]), 30);
  EXPECT_EQ(int(arcPan["ImageFrequency"]), 1000000);
  EXPECT_EQ(int(arcPan["NumberOfWindows"]), 6);
  EXPECT_EQ(int(arcPan["UniqueIdentifier"]), 100732832);
  EXPECT_EQ(arcPan["ExposureTimestamp"][0], "2f014e931416226d");
  EXPECT_DOUBLE_EQ(double(arcPan["ExposureTimePEHK"]), 1.152e-003);
  EXPECT_DOUBLE_EQ(double(arcPan["PixelsPossiblySaturated"]), 0.00);
  EXPECT_EQ(int(arcPan["YearDoy"]), 2016327);

  // Nir Archive Group
  PvlGroup &arcNir = isisLabel->findGroup("archiveNIR", Pvl::Traverse);
  EXPECT_EQ(arcNir["DataSetId"][0], "TBD");
  EXPECT_EQ(arcNir["ProductVersionId"][0], "UNK");
  EXPECT_EQ(arcNir["ProductCreationTime"][0], "2017-10-03T09:38:29");
  EXPECT_DOUBLE_EQ(double(arcNir["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(arcNir["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(arcNir["PredictMaximumExposureTime"]), 5.2866);
  EXPECT_DOUBLE_EQ(double(arcNir["CassisOffNadirAngle"]), 9.923);
  EXPECT_DOUBLE_EQ(double(arcNir["PredictedRepetitionFrequency"]), 1218.0);
  EXPECT_DOUBLE_EQ(double(arcNir["GroundTrackVelocity"]), 2.6946);
  EXPECT_DOUBLE_EQ(double(arcNir["ForwardRotationAngle"]), 146.943);
  EXPECT_DOUBLE_EQ(double(arcNir["SpiceMisalignment"]), 351.195);
  EXPECT_DOUBLE_EQ(double(arcNir["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(arcNir["FNumber"]), 6.50);
  EXPECT_EQ(int(arcNir["ExposureTimeCommand"]), 120);
  EXPECT_EQ(int(arcNir["FrameletNumber"]), 6);
  EXPECT_EQ(int(arcNir["NumberOfFramelets"]), 30);
  EXPECT_EQ(int(arcNir["ImageFrequency"]), 1000000);
  EXPECT_EQ(int(arcNir["NumberOfWindows"]), 6);
  EXPECT_EQ(int(arcNir["UniqueIdentifier"]), 100732832);
  EXPECT_EQ(arcNir["ExposureTimestamp"][0], "2f014e931416226d");
  EXPECT_DOUBLE_EQ(double(arcNir["ExposureTimePEHK"]), 1.152e-003);
  EXPECT_DOUBLE_EQ(double(arcNir["PixelsPossiblySaturated"]), 0.00);
  EXPECT_EQ(int(arcNir["YearDoy"]), 2016327);

  // Blue Archive Group
  PvlGroup &arcBlu = isisLabel->findGroup("archiveBLU", Pvl::Traverse);
  EXPECT_EQ(arcBlu["DataSetId"][0], "TBD");
  EXPECT_EQ(arcBlu["ProductVersionId"][0], "UNK");
  EXPECT_EQ(arcBlu["ProductCreationTime"][0], "2017-10-03T09:38:29");
  EXPECT_DOUBLE_EQ(double(arcBlu["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(arcBlu["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(arcBlu["PredictMaximumExposureTime"]), 5.2866);
  EXPECT_DOUBLE_EQ(double(arcBlu["CassisOffNadirAngle"]), 9.923);
  EXPECT_DOUBLE_EQ(double(arcBlu["PredictedRepetitionFrequency"]), 1218.0);
  EXPECT_DOUBLE_EQ(double(arcBlu["GroundTrackVelocity"]), 2.6946);
  EXPECT_DOUBLE_EQ(double(arcBlu["ForwardRotationAngle"]), 146.943);
  EXPECT_DOUBLE_EQ(double(arcBlu["SpiceMisalignment"]), 351.195);
  EXPECT_DOUBLE_EQ(double(arcBlu["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(arcBlu["FNumber"]), 6.50);
  EXPECT_EQ(int(arcBlu["ExposureTimeCommand"]), 120);
  EXPECT_EQ(int(arcBlu["FrameletNumber"]), 6);
  EXPECT_EQ(int(arcBlu["NumberOfFramelets"]), 30);
  EXPECT_EQ(int(arcBlu["ImageFrequency"]), 1000000);
  EXPECT_EQ(int(arcBlu["NumberOfWindows"]), 6);
  EXPECT_EQ(int(arcBlu["UniqueIdentifier"]), 100732832);
  EXPECT_EQ(arcBlu["ExposureTimestamp"][0], "2f014e931416226d");
  EXPECT_DOUBLE_EQ(double(arcBlu["ExposureTimePEHK"]), 1.152e-003);
  EXPECT_DOUBLE_EQ(double(arcBlu["PixelsPossiblySaturated"]), 0.00);
  EXPECT_EQ(int(arcBlu["YearDoy"]), 2016327);

  // Stitch Group
  PvlGroup &stitch = isisLabel->findGroup("Stitch", Pvl::Traverse);
  EXPECT_EQ(stitch["OriginalFilters"][0], "PAN");
  EXPECT_EQ(stitch["OriginalFilters"][1], "NIR");
  EXPECT_EQ(stitch["OriginalFilters"][2], "RED");
  EXPECT_EQ(stitch["OriginalFilters"][3], "BLU");
  EXPECT_EQ(stitch["FilterCenters"][0], "675");
  EXPECT_EQ(stitch["FilterCenters"][1], "985");
  EXPECT_EQ(stitch["FilterCenters"][2], "840");
  EXPECT_EQ(stitch["FilterCenters"][3], "485");
  EXPECT_EQ(stitch["FilterWidths"][0], "250");
  EXPECT_EQ(stitch["FilterWidths"][1], "220");
  EXPECT_EQ(stitch["FilterWidths"][2], "100");
  EXPECT_EQ(stitch["FilterWidths"][3], "165");
  EXPECT_EQ(stitch["FilterIkCodes"][0], "-143421");
  EXPECT_EQ(stitch["FilterIkCodes"][1], "-143423");
  EXPECT_EQ(stitch["FilterIkCodes"][2], "-143422");
  EXPECT_EQ(stitch["FilterIkCodes"][3], "-143424");
  EXPECT_EQ(stitch["FilterStartSamples"][0], "0.0");
  EXPECT_EQ(stitch["FilterStartSamples"][1], "0.0");
  EXPECT_EQ(stitch["FilterStartSamples"][2], "0.0");
  EXPECT_EQ(stitch["FilterStartSamples"][3], "1024.0");
  EXPECT_EQ(stitch["FilterStartLines"][0], "354.0");
  EXPECT_EQ(stitch["FilterStartLines"][1], "1048.0");
  EXPECT_EQ(stitch["FilterStartLines"][2], "712.0");
  EXPECT_EQ(stitch["FilterStartLines"][3], "1409.0");
  EXPECT_EQ(stitch["FilterLines"][0], "5");
  EXPECT_EQ(stitch["FilterLines"][1], "5");
  EXPECT_EQ(stitch["FilterLines"][2], "5");
  EXPECT_EQ(stitch["FilterLines"][3], "218");

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["FilterName"][0], "FULLCCD");


  // Kernels Group
  PvlGroup &kernels = isisLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kernels["NaifFrameCode"]), -143400);

  Histogram *hist = cube.histogram();

  EXPECT_NEAR(hist->Average(), 0.09578960688848305, 0.0001);
  EXPECT_DOUBLE_EQ(hist->Sum(), 1341.2460756525397);
  EXPECT_EQ(hist->ValidPixels(), 14002);
  EXPECT_NEAR(hist->StandardDeviation(), 0.0069463124411809603, 0.0001);
}

TEST(TgoCassisstitch, TgoCassisstitchSingleframeTest) {
  QTemporaryDir prefix;

  FileList *cubeList = new FileList();
  cubeList->append("data/tgoCassis/tgocassisstitch/CAS-MCO-2016-11-22T16.16.10.833-BLU-03000-B1_crop.cub");
  cubeList->append("data/tgoCassis/tgocassisstitch/CAS-MCO-2016-11-22T16.16.10.833-NIR-02000-B1_crop.cub");
  cubeList->append("data/tgoCassis/tgocassisstitch/CAS-MCO-2016-11-22T16.16.10.833-PAN-00000-B1_crop.cub");
  cubeList->append("data/tgoCassis/tgocassisstitch/CAS-MCO-2016-11-22T16.16.10.833-RED-01000-B1_crop.cub");

  QString cubeListFile = prefix.path() + "/cubelist.lis";
  cubeList->write(cubeListFile.toStdString());

  QString cubeName = "default";

  QVector<QString> args = {"fromlist=" + cubeListFile,
                           "cubename=" + cubeName,
                           "outputprefix=" + prefix.path() + "/CAS-MCO"};
  UserInterface options(APP_XML, args);

  try {
    tgocassisstitch(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassisstitch with cube list: " << e.what() << std::endl;
  }

  Cube cube(prefix.path().toStdString() + "/CAS-MCO-default.cub");
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  EXPECT_EQ(cube.sampleCount(), 2048);
  EXPECT_EQ(cube.lineCount(), 2048);
  EXPECT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(inst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(inst["TargetName"][0], "Mars" );
  EXPECT_EQ(inst["StartTime"][0], "2016-11-22T16:16:10.833");
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "2f014e932e2620aa");
  EXPECT_EQ(inst["ExposureDuration"][0], "1.152e-003");
  EXPECT_EQ(int(inst["SummingMode"]), 0);
  EXPECT_EQ(inst["Filter"][0], "FULLCCD");

  // Red Archive Group
  PvlGroup &arcRed = isisLabel->findGroup("archiveRED", Pvl::Traverse);
  EXPECT_EQ(arcRed["DataSetId"][0], "TBD");
  EXPECT_EQ(arcRed["ProductVersionId"][0], "UNK");
  EXPECT_EQ(arcRed["ProductCreationTime"][0], "2017-10-03T09:38:28");
  EXPECT_DOUBLE_EQ(double(arcRed["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(arcRed["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(arcRed["PredictMaximumExposureTime"]), 5.3718000000000004);
  EXPECT_DOUBLE_EQ(double(arcRed["CassisOffNadirAngle"]), 9.923);
  EXPECT_DOUBLE_EQ(double(arcRed["PredictedRepetitionFrequency"]), 1237.7);
  EXPECT_DOUBLE_EQ(double(arcRed["GroundTrackVelocity"]), 2.6803);
  EXPECT_DOUBLE_EQ(double(arcRed["ForwardRotationAngle"]), 147.264);
  EXPECT_DOUBLE_EQ(double(arcRed["SpiceMisalignment"]), 350.749);
  EXPECT_DOUBLE_EQ(double(arcRed["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(arcRed["FNumber"]), 6.50);
  EXPECT_EQ(int(arcRed["ExposureTimeCommand"]), 120);
  EXPECT_EQ(int(arcRed["FrameletNumber"]), 0);
  EXPECT_EQ(int(arcRed["NumberOfFramelets"]), 30);
  EXPECT_EQ(int(arcRed["ImageFrequency"]), 1000000);
  EXPECT_EQ(int(arcRed["NumberOfWindows"]), 6);
  EXPECT_EQ(int(arcRed["UniqueIdentifier"]), 100732832);
  EXPECT_EQ(arcRed["ExposureTimestamp"][0], "2f014e930e16226d");
  EXPECT_DOUBLE_EQ(double(arcRed["ExposureTimePEHK"]), 1.152e-003);
  EXPECT_DOUBLE_EQ(double(arcRed["PixelsPossiblySaturated"]), 0.00);
  EXPECT_EQ(int(arcRed["YearDoy"]), 2016327);

  // Pan Archive Group
  PvlGroup &arcPan = isisLabel->findGroup("archivePAN", Pvl::Traverse);
  EXPECT_EQ(arcPan["DataSetId"][0], "TBD");
  EXPECT_EQ(arcPan["ProductVersionId"][0], "UNK");
  EXPECT_EQ(arcPan["ProductCreationTime"][0], "2017-10-03T09:38:28");
  EXPECT_DOUBLE_EQ(double(arcPan["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(arcPan["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(arcPan["PredictMaximumExposureTime"]), 5.3718);
  EXPECT_DOUBLE_EQ(double(arcPan["CassisOffNadirAngle"]), 9.923);
  EXPECT_DOUBLE_EQ(double(arcPan["PredictedRepetitionFrequency"]), 1237.7);
  EXPECT_DOUBLE_EQ(double(arcPan["GroundTrackVelocity"]), 2.6803);
  EXPECT_DOUBLE_EQ(double(arcPan["ForwardRotationAngle"]), 147.264);
  EXPECT_DOUBLE_EQ(double(arcPan["SpiceMisalignment"]), 350.749);
  EXPECT_DOUBLE_EQ(double(arcPan["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(arcPan["FNumber"]), 6.50);
  EXPECT_EQ(int(arcPan["ExposureTimeCommand"]), 120);
  EXPECT_EQ(int(arcPan["FrameletNumber"]), 0);
  EXPECT_EQ(int(arcPan["NumberOfFramelets"]), 30);
  EXPECT_EQ(int(arcPan["ImageFrequency"]), 1000000);
  EXPECT_EQ(int(arcPan["NumberOfWindows"]), 6);
  EXPECT_EQ(int(arcPan["UniqueIdentifier"]), 100732832);
  EXPECT_EQ(arcPan["ExposureTimestamp"][0], "2f014e930e16226d");
  EXPECT_DOUBLE_EQ(double(arcPan["ExposureTimePEHK"]), 1.152e-003);
  EXPECT_DOUBLE_EQ(double(arcPan["PixelsPossiblySaturated"]), 0.00);
  EXPECT_EQ(int(arcPan["YearDoy"]), 2016327);

  // Nir Archive Group
  PvlGroup &arcNir = isisLabel->findGroup("archiveNIR", Pvl::Traverse);
  EXPECT_EQ(arcNir["DataSetId"][0], "TBD");
  EXPECT_EQ(arcNir["ProductVersionId"][0], "UNK");
  EXPECT_EQ(arcNir["ProductCreationTime"][0], "2017-10-03T09:38:28");
  EXPECT_DOUBLE_EQ(double(arcNir["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(arcNir["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(arcNir["PredictMaximumExposureTime"]), 5.3718);
  EXPECT_DOUBLE_EQ(double(arcNir["CassisOffNadirAngle"]), 9.923);
  EXPECT_DOUBLE_EQ(double(arcNir["PredictedRepetitionFrequency"]), 1237.7);
  EXPECT_DOUBLE_EQ(double(arcNir["GroundTrackVelocity"]), 2.6803);
  EXPECT_DOUBLE_EQ(double(arcNir["ForwardRotationAngle"]), 147.264);
  EXPECT_DOUBLE_EQ(double(arcNir["SpiceMisalignment"]), 350.749);
  EXPECT_DOUBLE_EQ(double(arcNir["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(arcNir["FNumber"]), 6.50);
  EXPECT_EQ(int(arcNir["ExposureTimeCommand"]), 120);
  EXPECT_EQ(int(arcNir["FrameletNumber"]), 0);
  EXPECT_EQ(int(arcNir["NumberOfFramelets"]), 30);
  EXPECT_EQ(int(arcNir["ImageFrequency"]), 1000000);
  EXPECT_EQ(int(arcNir["NumberOfWindows"]), 6);
  EXPECT_EQ(int(arcNir["UniqueIdentifier"]), 100732832);
  EXPECT_EQ(arcNir["ExposureTimestamp"][0], "2f014e930e16226d");
  EXPECT_DOUBLE_EQ(double(arcNir["ExposureTimePEHK"]), 1.152e-003);
  EXPECT_DOUBLE_EQ(double(arcNir["PixelsPossiblySaturated"]), 0.00);
  EXPECT_EQ(int(arcNir["YearDoy"]), 2016327);

  // Blue Archive Group
  PvlGroup &arcBlu = isisLabel->findGroup("archiveBLU", Pvl::Traverse);
  EXPECT_EQ(arcBlu["DataSetId"][0], "TBD");
  EXPECT_EQ(arcBlu["ProductVersionId"][0], "UNK");
  EXPECT_EQ(arcBlu["ProductCreationTime"][0], "2017-10-03T09:38:28");
  EXPECT_DOUBLE_EQ(double(arcBlu["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(arcBlu["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(arcBlu["PredictMaximumExposureTime"]), 5.3718);
  EXPECT_DOUBLE_EQ(double(arcBlu["CassisOffNadirAngle"]), 9.923);
  EXPECT_DOUBLE_EQ(double(arcBlu["PredictedRepetitionFrequency"]), 1237.7);
  EXPECT_DOUBLE_EQ(double(arcBlu["GroundTrackVelocity"]), 2.6803);
  EXPECT_DOUBLE_EQ(double(arcBlu["ForwardRotationAngle"]), 147.264);
  EXPECT_DOUBLE_EQ(double(arcBlu["SpiceMisalignment"]), 350.749);
  EXPECT_DOUBLE_EQ(double(arcBlu["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(arcBlu["FNumber"]), 6.50);
  EXPECT_EQ(int(arcBlu["ExposureTimeCommand"]), 120);
  EXPECT_EQ(int(arcBlu["FrameletNumber"]), 0);
  EXPECT_EQ(int(arcBlu["NumberOfFramelets"]), 30);
  EXPECT_EQ(int(arcBlu["ImageFrequency"]), 1000000);
  EXPECT_EQ(int(arcBlu["NumberOfWindows"]), 6);
  EXPECT_EQ(int(arcBlu["UniqueIdentifier"]), 100732832);
  EXPECT_EQ(arcBlu["ExposureTimestamp"][0], "2f014e930e16226d");
  EXPECT_DOUBLE_EQ(double(arcBlu["ExposureTimePEHK"]), 1.152e-003);
  EXPECT_DOUBLE_EQ(double(arcBlu["PixelsPossiblySaturated"]), 0.00);
  EXPECT_EQ(int(arcBlu["YearDoy"]), 2016327);

  // Stitch Group
  PvlGroup &stitch = isisLabel->findGroup("Stitch", Pvl::Traverse);
  EXPECT_EQ(stitch["OriginalFilters"][0], "RED");
  EXPECT_EQ(stitch["OriginalFilters"][1], "PAN");
  EXPECT_EQ(stitch["OriginalFilters"][2], "NIR");
  EXPECT_EQ(stitch["OriginalFilters"][3], "BLU");
  EXPECT_EQ(stitch["FilterCenters"][0], "840");
  EXPECT_EQ(stitch["FilterCenters"][1], "675");
  EXPECT_EQ(stitch["FilterCenters"][2], "985");
  EXPECT_EQ(stitch["FilterCenters"][3], "485");
  EXPECT_EQ(stitch["FilterWidths"][0], "100");
  EXPECT_EQ(stitch["FilterWidths"][1], "250");
  EXPECT_EQ(stitch["FilterWidths"][2], "220");
  EXPECT_EQ(stitch["FilterWidths"][3], "165");
  EXPECT_EQ(stitch["FilterIkCodes"][0], "-143422");
  EXPECT_EQ(stitch["FilterIkCodes"][1], "-143421");
  EXPECT_EQ(stitch["FilterIkCodes"][2], "-143423");
  EXPECT_EQ(stitch["FilterIkCodes"][3], "-143424");
  EXPECT_EQ(stitch["FilterStartSamples"][0], "0.0");
  EXPECT_EQ(stitch["FilterStartSamples"][1], "0.0");
  EXPECT_EQ(stitch["FilterStartSamples"][2], "0.0");
  EXPECT_EQ(stitch["FilterStartSamples"][3], "1024.0");
  EXPECT_EQ(stitch["FilterStartLines"][0], "712.0");
  EXPECT_EQ(stitch["FilterStartLines"][1], "354.0");
  EXPECT_EQ(stitch["FilterStartLines"][2], "1048.0");
  EXPECT_EQ(stitch["FilterStartLines"][3], "1409.0");
  EXPECT_EQ(stitch["FilterLines"][0], "5");
  EXPECT_EQ(stitch["FilterLines"][1], "5");
  EXPECT_EQ(stitch["FilterLines"][2], "5");
  EXPECT_EQ(stitch["FilterLines"][3], "5");

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["FilterName"][0], "FULLCCD");


  // Kernels Group
  PvlGroup &kernels = isisLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kernels["NaifFrameCode"]), -143400);

  Histogram *hist = cube.histogram();

  EXPECT_NEAR(hist->Average(), 0.19647446379065514, 0.0001);
  EXPECT_DOUBLE_EQ(hist->Sum(), 19.647446379065514);
  EXPECT_EQ(hist->ValidPixels(), 100);
  EXPECT_NEAR(hist->StandardDeviation(), 0.063902362199265747, 0.0001);

}
