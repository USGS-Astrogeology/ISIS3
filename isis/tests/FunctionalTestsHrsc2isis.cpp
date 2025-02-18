#include <QTemporaryDir>
#include <QFile>

#include "hrsc2isis.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"
#include "OriginalLabel.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace Isis;
using ::testing::HasSubstr;

static QString APP_XML = FileName("$ISISROOT/bin/xml/hrsc2isis.xml").expanded();

TEST(Hrsc2isis, Hrsc2IsisTestDefault) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/hrsc2isisTEMP.cub";
  QVector<QString> args = {"from=data/hrsc2isis/default/h0279_0000_re2_cropped.img",
                           "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
    hrsc2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest HRSC image: " << e.toString().toStdString().c_str() << std::endl;
  }
  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  ASSERT_EQ(cube.sampleCount(), 1288);
  ASSERT_EQ(cube.lineCount(), 5);
  ASSERT_EQ(cube.bandCount(), 1);

  // Pixels Group
  ASSERT_EQ(PixelTypeName(cube.pixelType()), "Real");
  ASSERT_EQ(ByteOrderName(cube.byteOrder()), "Lsb");
  ASSERT_DOUBLE_EQ(cube.base(), 0.0);
  ASSERT_DOUBLE_EQ(cube.multiplier(), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SpacecraftName"][0].toStdString(), "MARS EXPRESS");
  ASSERT_EQ(inst["InstrumentId"][0].toStdString(), "HRSC" );
  ASSERT_EQ(inst["StartTime"][0].toStdString(), "2004-04-07T07:08:31.61500" );
  ASSERT_EQ(inst["StopTime"][0].toStdString(), "2004-04-07T07:10:08.32300" );
  ASSERT_EQ(inst["SpacecraftClockStartCount"][0].toStdString(), "1/0029401660.57967" );
  ASSERT_EQ(inst["SpacecraftClockStopCount"][0].toStdString(), "1/0029401808.32263" );
  ASSERT_EQ(inst["MissionPhaseName"][0].toStdString(), "MC_Phase_5" );
  ASSERT_EQ(inst["TargetName"][0].toStdString(), "Mars" );
  ASSERT_EQ(int(inst["Summing"]), 4);
  ASSERT_DOUBLE_EQ(inst["FocalPlaneTemperature"], 8.1755);
  ASSERT_DOUBLE_EQ(inst["LensTemperature"], 8.3794);
  ASSERT_DOUBLE_EQ(inst["InstrumentTemperature"], 11.234);

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  ASSERT_EQ(archive["DataSetId"][0].toStdString(), "MEX-M-HRSC-3-RDR-V2.0" );
  ASSERT_EQ(archive["DetectorId"][0].toStdString(), "MEX_HRSC_RED" );
  ASSERT_EQ(archive["EventType"][0].toStdString(), "MARS-REGIONAL-STEREO-Vo-Te-Im" );
  ASSERT_EQ(int(archive["OrbitNumber"]), 279);
  ASSERT_EQ(archive["ProductId"][0].toStdString(), "H0279_0000_RE2.IMG" );

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_DOUBLE_EQ(bandbin["Width"], 48);
  ASSERT_DOUBLE_EQ(bandbin["Center"], 748.0);

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifIkCode"]), -41212);

  std::unique_ptr<Histogram> hist (cube.histogram());

  ASSERT_NEAR(hist->Average(), 63.6635, .0001);
  ASSERT_DOUBLE_EQ(hist->Sum(), 409993);
  ASSERT_EQ(hist->ValidPixels(), 6440);
  ASSERT_NEAR(hist->StandardDeviation(), 6.36599, .00001);

  // check original label exists
  Pvl ogLab = cube.readOriginalLabel().ReturnLabels();
  ASSERT_EQ(archive["DETECTOR_ID"][0].toStdString(), "MEX_HRSC_RED" );

}


TEST(Hrsc2isis, Hrsc2IsisTestPhobos) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/hrsc2isisTEMP.cub";
  QVector<QString> args = {"from=data/hrsc2isis/phobos/h7926_0009_s22_cropped.img",
                           "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
    hrsc2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest HRSC image: " << e.toString().toStdString().c_str() << std::endl;
  }
  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  ASSERT_EQ(cube.sampleCount(), 5184);
  ASSERT_EQ(cube.lineCount(), 5);
  ASSERT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["MissionPhaseName"][0].toStdString(), "ME_Phase_20" );
  ASSERT_EQ(inst["TargetName"][0].toStdString(), "Phobos" );
  ASSERT_EQ(int(inst["Summing"]), 1);

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  ASSERT_EQ(archive["DataSetId"][0].toStdString(), "MEX-M-HRSC-3-RDR-V2.0" );
  ASSERT_EQ(archive["DetectorId"][0].toStdString(), "MEX_HRSC_S2" );
  ASSERT_EQ(archive["EventType"][0].toStdString(), "PHOBOS-LIMB-CARTOGRAPHY-Im" );
  ASSERT_EQ(int(archive["OrbitNumber"]), 7926);
  ASSERT_EQ(archive["ProductId"][0].toStdString(), "H7926_0009_S22.IMG" );

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_DOUBLE_EQ(bandbin["Width"], 178);
  ASSERT_DOUBLE_EQ(bandbin["Center"], 679.0);

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifIkCode"]), -41211);

  std::unique_ptr<Histogram> hist (cube.histogram());

  ASSERT_NEAR(hist->Average(), 0.0962, .0001);
  ASSERT_DOUBLE_EQ(hist->Sum(), 2496);
  ASSERT_EQ(hist->ValidPixels(), 25920);
  ASSERT_NEAR(hist->StandardDeviation(), 0.52835, .00001);

  // check original label exists
  Pvl ogLab = cube.readOriginalLabel().ReturnLabels();
  ASSERT_EQ(archive["DETECTOR_ID"][0].toStdString(), "MEX_HRSC_S2" );
}


TEST(Hrsc2isis, Hrsc2IsisTestProjection) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/hrsc2isisTEMP.cub";
  QVector<QString> args = {"from=data/hrsc2isis/projection/h6541_0000_ir4_cropped.img",
                           "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try{
    hrsc2isis(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e){
    EXPECT_THAT(e.what(), HasSubstr("has keyword [PROCESSING_LEVEL_ID = 4] and can not be read by this program"));
  }
}


TEST(Hrsc2isis, Hrsc2IsisTestSrcImage) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/hrsc2isisTEMP.cub";
  QVector<QString> args = {"from=data/hrsc2isis/srcImage/h2862_0006_sr2_cropped.img",
                           "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
    hrsc2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest HRSC image: " << e.toString().toStdString().c_str() << std::endl;
  }
  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  ASSERT_EQ(cube.sampleCount(), 1008);
  ASSERT_EQ(cube.lineCount(), 5);
  ASSERT_EQ(cube.bandCount(), 1);

  // Pixels Group
  ASSERT_EQ(PixelTypeName(cube.pixelType()), "SignedWord");
  ASSERT_EQ(ByteOrderName(cube.byteOrder()), "Lsb");
  ASSERT_DOUBLE_EQ(cube.base(), 0.0);
  ASSERT_DOUBLE_EQ(cube.multiplier(), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["InstrumentId"][0].toStdString(), "SRC" );
  ASSERT_EQ(inst["MissionPhaseName"][0].toStdString(), "ME_Phase_2" );
  ASSERT_EQ(inst["TargetName"][0].toStdString(), "Deimos" );
  ASSERT_DOUBLE_EQ(inst["ExposureDuration"], 27.216);

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  ASSERT_EQ(archive["DataSetId"][0].toStdString(), "MEX-M-HRSC-3-RDR-V2.0" );
  ASSERT_EQ(archive["DetectorId"][0].toStdString(), "MEX_HRSC_SRC" );
  ASSERT_EQ(archive["EventType"][0].toStdString(), "DEIMOS-LIMB-CARTOGRAPHY-Im" );
  ASSERT_EQ(int(archive["OrbitNumber"]), 2862);
  ASSERT_EQ(archive["ProductId"][0].toStdString(), "H2862_0006_SR2.IMG" );

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_DOUBLE_EQ(bandbin["Width"], 250);
  ASSERT_DOUBLE_EQ(bandbin["Center"], 600.0);

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifIkCode"]), -41220);

  std::unique_ptr<Histogram> hist (cube.histogram());

  ASSERT_NEAR(hist->Average(), 40.6, .0001);
  ASSERT_DOUBLE_EQ(hist->Sum(), 204624);
  ASSERT_EQ(hist->ValidPixels(), 5040);
  ASSERT_NEAR(hist->StandardDeviation(), 568.86015, .00001);
}
