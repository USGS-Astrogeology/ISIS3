#include <QTemporaryDir>
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Endian.h"
#include "PixelType.h"
#include "lronaccal.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "Histogram.h"
#include "crop.h"

#include "TempFixtures.h"

using namespace std;
using namespace Isis;
using namespace testing;

/**
  *
  * @brief Calibration application for the LRO NAC cameras
  *
  *  lronaccal performs radiometric corrections to images acquired by the Narrow Angle
  *  Camera aboard the Lunar Reconnaissance Orbiter spacecraft.
  *
  * @author 2016-09-16 Victor Silva
  *
  * @internal
  *   @history 2022-04-26 Victor Silva - Original Version - Functional test is against known value for input
  *                                      cub since fx is not yet callable
  */

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/lronaccal.xml").expanded());

TEST(LronaccalDefault, FunctionalTestsLronaccal) {
  QTemporaryDir outputDir;

  ASSERT_TRUE(outputDir.isValid());

  /*This application can not be run on any image that has been
    geometrically transformed (i.e. scaled, rotated, sheared, or
    reflected) or cropped
  */
  QString iCubeFile =  "$ISISTESTDATA/isis/src/lro/apps/lronaccal/M1333276014R.cub";
  QString oCubeFile = outputDir.path() + "/out.default.cub";
  QString oCubeCropFile = outputDir.path() + "/out.default.crop.cub";
  QString tCubeFile = "data/lronaccal/truth/M1333276014R.default.crop.cub";

  QVector<QString> args = {"from="+iCubeFile, "to="+oCubeFile };

  UserInterface options(APP_XML, args);
  try {
    lronaccal(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to calibrate the LRO image: " <<e.toString().c_str() << std::endl;
  }
  try{
    static QString CROP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/crop.xml").expanded());
    QVector<QString> argsCrop = {"from=" + oCubeFile, "to=" + oCubeCropFile, "sample=80", "nsamples=80", "line=80", "nlines=80"};
    UserInterface options2(CROP_XML, argsCrop);
    crop(options2);

    Cube oCube(oCubeCropFile.toStdString(), "r");
    Cube tCube(tCubeFile.toStdString(), "r");

    Histogram *oCubeStats = oCube.histogram();
    Histogram *tCubeStats = tCube.histogram();

    EXPECT_NEAR(oCubeStats->Average(), tCubeStats->Average(), 0.001);
    EXPECT_NEAR(oCubeStats->Sum(), tCubeStats->Sum(), 0.001);
    EXPECT_NEAR(oCubeStats->ValidPixels(), tCubeStats->ValidPixels(), 0.001);
    EXPECT_NEAR(oCubeStats->StandardDeviation(), tCubeStats->StandardDeviation(), 0.001);

    tCube.close();
    oCube.close();
  }
  catch(IException &e){
    FAIL() << "Unable to compare stats: " <<e.toString().c_str() << std::endl;
  }
}

TEST(LronaccalNear, FunctionalTestsLronaccal) {
  QTemporaryDir outputDir;

  ASSERT_TRUE(outputDir.isValid());

  /*This application can not be run on any image that has been
    geometrically transformed (i.e. scaled, rotated, sheared, or
    reflected) or cropped
  */
  QString iCubeFile =  "$ISISTESTDATA/isis/src/lro/apps/lronaccal/M1333276014R.cub";
  QString oCubeFile = outputDir.path() + "/out.near.cub";
  QString oCubeCropFile = outputDir.path() + "/out.near.crop.cub";
  QString tCubeFile = "data/lronaccal/truth/M1333276014R.near.crop.cub";

  QVector<QString> args = {"from="+iCubeFile, "to="+oCubeFile, "DarkFileType=NEAREST"};

  UserInterface options(APP_XML, args);
  try {
    lronaccal(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to calibrate the LRO image: " <<e.toString().c_str() << std::endl;
  }
  try{
    static QString CROP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/crop.xml").expanded());
    QVector<QString> argsCrop = {"from=" + oCubeFile, "to=" + oCubeCropFile, "sample=80", "nsamples=80", "line=80", "nlines=80"};
    UserInterface options2(CROP_XML, argsCrop);
    crop(options2);

    Cube oCube(oCubeCropFile.toStdString(), "r");
    Cube tCube(tCubeFile.toStdString(), "r");

    Histogram *oCubeStats = oCube.histogram();
    Histogram *tCubeStats = tCube.histogram();

    EXPECT_NEAR(oCubeStats->Average(), tCubeStats->Average(), 0.001);
    EXPECT_NEAR(oCubeStats->Sum(), tCubeStats->Sum(), 0.001);
    EXPECT_NEAR(oCubeStats->ValidPixels(), tCubeStats->ValidPixels(), 0.001);
    EXPECT_NEAR(oCubeStats->StandardDeviation(), tCubeStats->StandardDeviation(), 0.001);

    tCube.close();
    oCube.close();
  }
  catch(IException &e){
    FAIL() << "Unable to open output cube: " <<e.toString().c_str() << std::endl;
  }
}

TEST(LronaccalPair, FunctionalTestsLronaccal) {
  QTemporaryDir outputDir;

  ASSERT_TRUE(outputDir.isValid());

  QString iCubeFile =  "$ISISTESTDATA/isis/src/lro/apps/lronaccal/M1333276014R.cub";
  QString oCubeFile = outputDir.path() + "/out.pair.cub";
  QString oCubeCropFile = outputDir.path() + "/out.pair.crop.cub";
  QString tCubeFile = "data/lronaccal/truth/M1333276014R.pair.crop.cub";

  QVector<QString> args = {"from="+iCubeFile, "to="+oCubeFile, "DarkFileType=PAIR"};

  UserInterface options(APP_XML, args);
  try {
    lronaccal(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to calibrate the LRO image: " <<e.toString().c_str() << std::endl;
  }
  try{
    static QString CROP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/crop.xml").expanded());
    QVector<QString> argsCrop = {"from=" + oCubeFile, "to=" + oCubeCropFile, "sample=80", "nsamples=80", "line=80", "nlines=80"};
    UserInterface options2(CROP_XML, argsCrop);
    crop(options2);

    Cube oCube(oCubeCropFile.toStdString(), "r");
    Cube tCube(tCubeFile.toStdString(), "r");

    Histogram *oCubeStats = oCube.histogram();
    Histogram *tCubeStats = tCube.histogram();

    EXPECT_NEAR(oCubeStats->Average(), tCubeStats->Average(), 0.001);
    EXPECT_NEAR(oCubeStats->Sum(), tCubeStats->Sum(), 0.001);
    EXPECT_NEAR(oCubeStats->ValidPixels(), tCubeStats->ValidPixels(), 0.001);
    EXPECT_NEAR(oCubeStats->StandardDeviation(), tCubeStats->StandardDeviation(), 0.001);

    tCube.close();
    oCube.close();
  }
  catch(IException &e){
    FAIL() << "Unable to open output cube: " <<e.toString().c_str() << std::endl;
  }
}

TEST_F(TempTestingFiles, FunctionalTestsLronaccalNacLFull) {
  /*This application can not be run on any image that has been
    geometrically transformed (i.e. scaled, rotated, sheared, or
    reflected) or cropped
  */
  QString iCubeFile = "data/lronaccal/nacl00020d3a.cub";
  QString oCubeFile = tempDir.path() + "out.default.cub";

  QVector<QString> args = {"from="+iCubeFile, "to="+oCubeFile };

  UserInterface options(APP_XML, args);
  try {
    lronaccal(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to calibrate the LRO image: " <<e.toString().c_str() << std::endl;
  }

  Cube outCube(oCubeFile.toStdString());
  Pvl *outLabel = outCube.label();

  std::istringstream radIS(R"(
  Group = Radiometry
    DarkColumns               = (12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
                                24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
                                5046, 5047, 5048, 5049, 5050, 5051, 5052, 5053,
                                5054, 5055, 5056, 5057, 5058, 5059, 5060, 5061)
    DarkFileType              = NearestDarkFilePair
    DarkFiles                 = ($ISISDATA/lro/calibration/nac_darks/NACL_Avera-
                                geDarks_316828866T_exp0.0001.cub,
                                $ISISDATA/lro/calibration/nac_darks/NACL_Averag-
                                eDarks_319507266T_exp0.0001.cub)
    NonlinearOffset           = $ISISDATA/lro/calibration/NACL_LinearizationOff-
                                sets.0006.cub
    LinearizationCoefficients = $ISISDATA/lro/calibration/NACL_LinearizationCoe-
                                fficients.0007.txt
    FlatFile                  = $ISISDATA/lro/calibration/NACL_Flatfield.0006.c-
                                ub
    RadiometricType           = IOF
    ResponsivityValue         = 15869.0
    SolarDistance             = 0.98615168542222
  End_Group
  )");

  PvlGroup truthRadGroup;
  radIS >> truthRadGroup;
  PvlGroup &radGroup = outLabel->findGroup("Radiometry", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, radGroup, truthRadGroup);

  Histogram *oCubeStats = outCube.histogram();
  EXPECT_DOUBLE_EQ(oCubeStats->Average(), 0.026724545839011172);
  EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 136829.67469573719);
  EXPECT_EQ(oCubeStats->ValidPixels(), 5120000);
  EXPECT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 0.0020650268181325645);
}

TEST_F(TempTestingFiles, FunctionalTestsLronaccalNacLSummed) {
  /*This application can not be run on any image that has been
    geometrically transformed (i.e. scaled, rotated, sheared, or
    reflected) or cropped
  */
  QString iCubeFile = "data/lronaccal/nacl00007053.cub";
  QString oCubeFile = tempDir.path() + "/out.default.cub";

  QVector<QString> args = {"from="+iCubeFile, "to="+oCubeFile };

  UserInterface options(APP_XML, args);
  try {
    lronaccal(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to calibrate the LRO image: " <<e.toString().c_str() << std::endl;
  }

  Cube outCube(oCubeFile.toStdString());
  Pvl *outLabel = outCube.label();

  std::istringstream radIS(R"(
  Group = Radiometry
    DarkColumns               = (6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
                                2523, 2524, 2525, 2526, 2527, 2528, 2529, 2530)
    DarkFileType              = NearestDarkFilePair
    DarkFiles                 = ($ISISDATA/lro/calibration/nac_darks/NACL_Avera-
                                geDarks_303609666T_Summed.0001.cub,
                                $ISISDATA/lro/calibration/nac_darks/NACL_Averag-
                                eDarks_306288066T_Summed.0001.cub)
    NonlinearOffset           = $ISISDATA/lro/calibration/NACL_LinearizationOff-
                                sets_Summed.0006.cub
    LinearizationCoefficients = $ISISDATA/lro/calibration/NACL_LinearizationCoe-
                                fficients.0007.txt
    FlatFile                  = $ISISDATA/lro/calibration/NACL_Flatfield_Summed-
                                .0006.cub
    RadiometricType           = IOF
    ResponsivityValue         = 15869.0
    SolarDistance             = 1.0092946598536
  End_Group
  )");

  PvlGroup truthRadGroup;
  radIS >> truthRadGroup;
  PvlGroup &radGroup = outLabel->findGroup("Radiometry", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, radGroup, truthRadGroup);

  Histogram *oCubeStats = outCube.histogram();
  EXPECT_DOUBLE_EQ(oCubeStats->Average(), 0.0067645818969427939);
  EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 51951.988968520658);
  EXPECT_EQ(oCubeStats->ValidPixels(), 7680000);
  EXPECT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 0.0086012102391031867);
}

TEST_F(TempTestingFiles, FunctionalTestsLronaccalNacRFull) {
  /*This application can not be run on any image that has been
    geometrically transformed (i.e. scaled, rotated, sheared, or
    reflected) or cropped
  */
  QString iCubeFile = "data/lronaccal/nacr00020d3a.cub";
  QString oCubeFile = tempDir.path() + "/out.default.cub";

  QVector<QString> args = {"from="+iCubeFile, "to="+oCubeFile };

  UserInterface options(APP_XML, args);
  try {
    lronaccal(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to calibrate the LRO image: " <<e.toString().c_str() << std::endl;
  }

  Cube outCube(oCubeFile.toStdString());
  Pvl *outLabel = outCube.label();

  std::istringstream radIS(R"(
  Group = Radiometry
    DarkColumns               = (5051, 5050, 5049, 5048, 5047, 5046, 5045, 5044,
                                5043, 5042, 5041, 5040, 5039, 5038, 5037, 5036,
                                5035, 5034, 5033, 5032, 5031, 5030, 5029, 5028,
                                17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,
                                4, 3, 2)
    DarkFileType              = NearestDarkFilePair
    DarkFiles                 = ($ISISDATA/lro/calibration/nac_darks/NACR_Avera-
                                geDarks_316828866T_exp0.0001.cub,
                                $ISISDATA/lro/calibration/nac_darks/NACR_Averag-
                                eDarks_319507266T_exp0.0001.cub)
    NonlinearOffset           = $ISISDATA/lro/calibration/NACR_LinearizationOff-
                                sets.0006.cub
    LinearizationCoefficients = $ISISDATA/lro/calibration/NACR_LinearizationCoe-
                                fficients.0007.txt
    FlatFile                  = $ISISDATA/lro/calibration/NACR_Flatfield.0006.c-
                                ub
    RadiometricType           = IOF
    ResponsivityValue         = 15058.0
    SolarDistance             = 0.98615168542222
  End_Group
  )");

  PvlGroup truthRadGroup;
  radIS >> truthRadGroup;
  PvlGroup &radGroup = outLabel->findGroup("Radiometry", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, radGroup, truthRadGroup);

  Histogram *oCubeStats = outCube.histogram();
  EXPECT_DOUBLE_EQ(oCubeStats->Average(), 0.025868278779590172);
  EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 132445.58735150169);
  EXPECT_EQ(oCubeStats->ValidPixels(), 5120000);
  EXPECT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 0.0018962021917208359);
}

TEST_F(TempTestingFiles, FunctionalTestsLronaccalNacRSummed) {
  /*This application can not be run on any image that has been
    geometrically transformed (i.e. scaled, rotated, sheared, or
    reflected) or cropped
  */
  QString iCubeFile = "data/lronaccal/nacr00007053.cub";
  QString oCubeFile = tempDir.path() + "/out.default.cub";

  QVector<QString> args = {"from="+iCubeFile, "to="+oCubeFile };

  UserInterface options(APP_XML, args);
  try {
    lronaccal(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to calibrate the LRO image: " <<e.toString().c_str() << std::endl;
  }

  Cube outCube(oCubeFile.toStdString());
  Pvl *outLabel = outCube.label();

  std::istringstream radIS(R"(
  Group = Radiometry
    DarkColumns               = (2525, 2524, 2523, 2522, 2521, 2520, 2519, 2518,
                                2517, 2516, 2515, 2514, 8, 7, 6, 5, 4, 3, 2, 1)
    DarkFileType              = NearestDarkFilePair
    DarkFiles                 = ($ISISDATA/lro/calibration/nac_darks/NACR_Avera-
                                geDarks_303609666T_Summed.0001.cub,
                                $ISISDATA/lro/calibration/nac_darks/NACR_Averag-
                                eDarks_306288066T_Summed.0001.cub)
    NonlinearOffset           = $ISISDATA/lro/calibration/NACR_LinearizationOff-
                                sets_Summed.0006.cub
    LinearizationCoefficients = $ISISDATA/lro/calibration/NACR_LinearizationCoe-
                                fficients.0007.txt
    FlatFile                  = $ISISDATA/lro/calibration/NACR_Flatfield_Summed-
                                .0006.cub
    RadiometricType           = IOF
    ResponsivityValue         = 15058.0
    SolarDistance             = 1.0092946598536
  End_Group
  )");

  PvlGroup truthRadGroup;
  radIS >> truthRadGroup;
  PvlGroup &radGroup = outLabel->findGroup("Radiometry", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, radGroup, truthRadGroup);

  Histogram *oCubeStats = outCube.histogram();
  EXPECT_DOUBLE_EQ(oCubeStats->Average(), 0.0067305094629900421);
  EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 51690.312675763525);
  EXPECT_EQ(oCubeStats->ValidPixels(), 7680000);
  EXPECT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 0.0086439695700371976);
}