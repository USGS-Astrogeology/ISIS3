
#include <QTemporaryDir>
#include <QFile>

#include "crism2isis.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace Isis;
using ::testing::HasSubstr;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/crism2isis.xml").expanded());

TEST(Crism2isis, Crism2IsisTestDdr) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/crism2isisTEMP.cub";

  QVector<QString> args = {"from=data/crism2isis/frt00003e25_01_de156l_ddr1.lbl",
                           "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
    crism2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest CRISM image: " <<  e.toString().c_str() << std::endl;
  }
  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  ASSERT_EQ(cube.sampleCount(), 64);
  ASSERT_EQ(cube.lineCount(), 15);
  ASSERT_EQ(cube.bandCount(), 14);

  // Pixels Group
  ASSERT_EQ(PixelTypeName(cube.pixelType()), "Real");
  ASSERT_EQ(ByteOrderName(cube.byteOrder()), "Lsb");
  ASSERT_DOUBLE_EQ(cube.base(), 0.0);
  ASSERT_DOUBLE_EQ(cube.multiplier(), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SpacecraftName"][0], "MARS RECONNAISSANCE ORBITER");
  ASSERT_EQ(inst["InstrumentId"][0], "CRISM" );
  ASSERT_EQ(inst["TargetName"][0], "Mars" );
  ASSERT_EQ(inst["SpacecraftClockStartCount"][0], "2/853135167:38571" );
  ASSERT_EQ(inst["SpacecraftClockStopCount"][0], "2/853135171:21163" );
  ASSERT_EQ(inst["StartTime"][0], "2007-01-13T05:59:08.707" );
  ASSERT_EQ(inst["StopTime"][0], "2007-01-13T05:59:12.442" );
  ASSERT_EQ(inst["SensorId"][0], "L" );
  ASSERT_EQ(inst["ShutterModeId"][0], "UNKNOWN" );
  ASSERT_DOUBLE_EQ(inst["FrameRate"], 3.75);
  ASSERT_EQ(inst["ExposureParameter"][0], "Null" );
  ASSERT_EQ(int(inst["PixelAveragingWidth"]), 10);
  ASSERT_EQ(inst["ScanModeId"][0], "LONG" );
  ASSERT_EQ(inst["SamplingModeId"][0], "UNKNOWN" );

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  ASSERT_EQ(archive["DataSetId"][0], "MRO-M-CRISM-6-DDR-V1.0" );
  ASSERT_EQ(archive["ProductId"][0], "FRT00003E25_01_DE156L_DDR1" );
  ASSERT_EQ(archive["ProductType"][0], "DDR" );
  ASSERT_EQ(archive["ProductCreationTime"][0], "2007-04-04T22:49:30" );
  ASSERT_EQ(int(archive["ProductVersionId"]), 1);

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["OriginalBand"].size(), 14);

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifIkCode"]), -74018);

  std::unique_ptr<Histogram> hist (cube.histogram());

  ASSERT_NEAR(hist->Average(), 64.5602, 0.0001);
  ASSERT_NEAR(hist->Sum(), 61977.86806, .00001);
  ASSERT_EQ(hist->ValidPixels(), 960);
  ASSERT_NEAR(hist->StandardDeviation(), 0.13314, .00001);
}

TEST(Crism2isis, Crism2IsisTestMrdr) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/crism2isisTEMP.cub";

  QVector<QString> args = {"from=data/crism2isis/t1865_mrrde_70n185_0256_1_cropped.lbl",
                           "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
    crism2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest CRISM image: " <<  e.toString().c_str() << std::endl;
  }
  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  ASSERT_EQ(cube.sampleCount(), 980);
  ASSERT_EQ(cube.lineCount(), 10);
  ASSERT_EQ(cube.bandCount(), 1);

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  ASSERT_EQ(archive["DataSetId"][0], "MRO-M-CRISM-5-RDR-MULTISPECTRAL-V1.0" );
  ASSERT_EQ(archive["ProductId"][0], "T1865_MRRDE_70N185_0256_1" );
  ASSERT_EQ(archive["ProductType"][0], "MAP_PROJECTED_MULTISPECTRAL_RDR" );
  ASSERT_EQ(archive["ProductCreationTime"][0], "2007-12-22T16:50:47.432000" );
  ASSERT_EQ(int(archive["ProductVersionId"]), 1);

  // Mapping Group
  PvlGroup &mapping = isisLabel->findGroup("Mapping", Pvl::Traverse);
  ASSERT_EQ(mapping["ProjectionName"][0], "Equirectangular");
  ASSERT_DOUBLE_EQ(double(mapping["CenterLongitude"]), -175.0);
  ASSERT_EQ(mapping["TargetName"][0], "Mars");
  ASSERT_DOUBLE_EQ(double(mapping["EquatorialRadius"]), 3396000.0);
  ASSERT_DOUBLE_EQ(double(mapping["PolarRadius"]), 3396000.0);
  ASSERT_EQ(mapping["LatitudeType"][0], "Planetocentric");
  ASSERT_EQ(mapping["LongitudeDirection"][0], "PositiveEast");
  ASSERT_EQ(int(mapping["LongitudeDomain"]), 180);
  ASSERT_DOUBLE_EQ(double(mapping["MinimumLatitude"]), 67.5000001);
  ASSERT_DOUBLE_EQ(double(mapping["MaximumLatitude"]), 72.5);
  ASSERT_DOUBLE_EQ(double(mapping["MinimumLongitude"]), 0.0);
  ASSERT_NEAR(double(mapping["MaximumLongitude"]), 9.99999, .00001);
  ASSERT_NEAR(double(mapping["UpperLeftCornerX"]), -10965321.32300, .00001);
  ASSERT_NEAR(double(mapping["UpperLeftCornerY"]), 4297290.91575, .00001);
  ASSERT_NEAR(double(mapping["PixelResolution"]), 231.52883, .00001);
  ASSERT_DOUBLE_EQ(double(mapping["Scale"]), 256.0);
  ASSERT_NEAR(double(mapping["CenterLatitude"]), 67.50000, .00001);

  std::unique_ptr<Histogram> hist (cube.histogram());

  ASSERT_NEAR(hist->Average(), 165.029, .001);
  ASSERT_NEAR(hist->Sum(), 1258015.64463, .00001);
  ASSERT_EQ(hist->ValidPixels(), 7623);
  ASSERT_NEAR(hist->StandardDeviation(), 49.9226, .0001);
}


TEST(Crism2isis, Crism2IsisTestTrdr) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/crism2isisTEMP.cub";

  QVector<QString> args = {"from=data/crism2isis/frt0001e5c3_07_if124s_trr3_cropped.lbl",
                           "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
    crism2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest CRISM image: " <<  e.toString().c_str() << std::endl;
  }
  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  ASSERT_EQ(cube.sampleCount(), 640);
  ASSERT_EQ(cube.lineCount(), 1);
  ASSERT_EQ(cube.bandCount(), 107);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SensorId"][0], "S" );
  ASSERT_EQ(inst["ShutterModeId"][0], "OPEN" );
  ASSERT_EQ(inst["ScanModeId"][0], "SHORT" );
  ASSERT_EQ(inst["SamplingModeId"][0], "HYPERSPEC" );

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  ASSERT_EQ(archive["DataSetId"][0], "MRO-M-CRISM-3-RDR-TARGETED-V1.0" );
  ASSERT_EQ(archive["ProductId"][0], "FRT0001E5C3_07_IF124S_TRR3");
  ASSERT_EQ(archive["ProductType"][0], "TARGETED_RDR" );
  ASSERT_EQ(archive["ProductCreationTime"][0], "2011-06-08T10:52:30" );
  ASSERT_EQ(int(archive["ProductVersionId"]), 3);
}


TEST(Crism2isis, Crism2IsisTestMrral) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/crism2isisTEMP.cub";

  QVector<QString> args = {"from=data/crism2isis/T0897_MRRAL_05S113_0256_1_cropped.LBL",
                           "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
    crism2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest CRISM image: " <<  e.toString().c_str() << std::endl;
  }
  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  ASSERT_EQ(cube.sampleCount(), 1279);
  ASSERT_EQ(cube.lineCount(), 10);
  ASSERT_EQ(cube.bandCount(), 1);

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  ASSERT_EQ(archive["DataSetId"][0], "MRO-M-CRISM-5-RDR-MULTISPECTRAL-V1.0" );
  ASSERT_EQ(archive["ProductId"][0], "T0897_MRRAL_05S113_0256_1");
  ASSERT_EQ(archive["ProductType"][0], "MAP_PROJECTED_MULTISPECTRAL_RDR" );
  ASSERT_EQ(archive["ProductCreationTime"][0], "2008-03-25T23:01:30.319000" );
  ASSERT_EQ(int(archive["ProductVersionId"]), 1);

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["OriginalBand"].size(), 72);
  ASSERT_EQ(bandbin["Width"].size(), 72);
  ASSERT_EQ(bandbin["Width"][0], " 410.12");
  ASSERT_EQ(bandbin["Width"][35], " 1625.00");
  ASSERT_EQ(bandbin["Width"][71], " 3923.47");
}


TEST(Crism2isis, Crism2IsisTestErrorNoWavelength) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/crism2isisTEMP.cub";

  QVector<QString> args = {"from=data/crism2isis/T0897_MRRAL_05S113_0256_1_cropped_badwv.lbl",
                           "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
    crism2isis(options);
    FAIL() << "Should throw an exception." <<std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("**I/O ERROR** Cannot find wavelength table"));
  }

}


TEST(Crism2isis, Crism2IsisTestErrorUnsupported) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/crism2isisTEMP.cub";

  QVector<QString> args = {"from=data/crism2isis/CDR410000000000_AT0300020L_2.LBL",
                           "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
    crism2isis(options);
    FAIL() << "Should throw an exception." <<std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("Unsupported CRISM file type, supported "
                                    "types are: DDR, MRDR, and TRDR"));
  }

}


TEST(Crism2isis, Crism2IsisTestErrorNoPid) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/crism2isisTEMP.cub";

  QVector<QString> args = {"from=data/crism2isis/t1865_mrrde_70n185_0256_1_cropped_no_pid.lbl",
                           "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
    crism2isis(options);
    FAIL() << "Should throw an exception." <<std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("Could not find label PRODUCT_ID, invalid MRDR"));
  }

}


TEST(Crism2isis, Crism2IsisTestErrorNoProdType) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/crism2isisTEMP.cub";

  QVector<QString> args = {"from=data/crism2isis/t1865_mrrde_70n185_0256_1_cropped_no_prod_type.lbl",
                           "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
    crism2isis(options);
    FAIL() << "Should throw an exception." <<std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("Unsupported CRISM file type, supported types are: DDR, MRDR, and TRDR"));
  }

}
