#include "ringsautomos.h"

#include "CubeFixtures.h"
#include "TestUtilities.h"
#include "UserInterface.h"
#include "Histogram.h"

#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/ringsautomos.xml").expanded());

TEST_F(RingsCube, FunctionalTestRingsautomos) {
  QString outPath = tempDir.path() + "/mosaic.cub";

  QVector<QString> args = {"fromlist=" + cubeListPath, "mosaic=" + outPath};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  ringsautomos(options, &appLog);

  Cube mos(outPath.toStdString());
  Pvl label = *mos.label();

  PvlGroup mapping = label.findObject("IsisCube").findGroup("Mapping");

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("ProjectionName"), "Planar");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("TargetName"), "Saturn");
  EXPECT_NEAR((double)mapping.findKeyword("UpperLeftCornerX"), -141593057.92723, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("UpperLeftCornerY"), 141593057.92723, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("PixelResolution"), 5899710.746968, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("Scale"), 0.5, 0.0001);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("RingLongitudeDirection"), "CounterClockwise");
  EXPECT_EQ((int)mapping.findKeyword("RingLongitudeDomain"), 360);
  EXPECT_NEAR((double)mapping.findKeyword("MaximumRingRadius"), 198012526.14923, 0.0001);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MinimumRingLongitude"), 0);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MaximumRingLongitude"), 360);
  EXPECT_NEAR((double)mapping.findKeyword("CenterRingRadius"), 169014263.07462, 0.0001);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("CenterRingLongitude"), 180);

  std::unique_ptr<Histogram> oCubeStats(mos.histogram());

  EXPECT_NEAR(oCubeStats->Average(),           1336,    0.001);
  EXPECT_NEAR(oCubeStats->Sum(),               903136,  0.001);
  EXPECT_NEAR(oCubeStats->ValidPixels(),       676,     0.001);
  EXPECT_NEAR(oCubeStats->StandardDeviation(), 1152.5415021048376, 0.001);
}


TEST_F(RingsCube, FunctionalTestRingsautomosSetRanges) {
  QString oFileListPath = tempDir.path() + "/outFileList.txt";
  QString outPath = tempDir.path() + "/mosaic.cub";

  QVector<QString> args = {"fromlist=" + cubeListPath, "mosaic=" + outPath,
                           "tolist=" + oFileListPath, "priority=beneath", "grange=user",
                           "minringlon=0", "maxringlon=100", "minringrad=8000000", "maxringrad=100000000",
                           "track=true", "matchbandbin=false", "matchdem=true"};

  UserInterface options(APP_XML, args);
  Pvl appLog;

  ringsautomos(options, &appLog);

  Cube mos(outPath.toStdString());
  Pvl label = *mos.label();

  PvlGroup mapping = label.findObject("IsisCube").findGroup("Mapping");

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("ProjectionName"), "Planar");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("TargetName"), "Saturn");
  EXPECT_NEAR((double)mapping.findKeyword("UpperLeftCornerX"), -100295082.69846, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("UpperLeftCornerY"), -9.7971743931788306e-10, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("PixelResolution"), 5899710.746968, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("Scale"), 0.5, 0.0001);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("RingLongitudeDirection"), "CounterClockwise");
  EXPECT_EQ((int)mapping.findKeyword("RingLongitudeDomain"), 360);
  EXPECT_NEAR((double)mapping.findKeyword("MaximumRingRadius"), 100000000, 0.0001);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MinimumRingLongitude"), 0);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MaximumRingLongitude"), 100);
  EXPECT_NEAR((double)mapping.findKeyword("CenterRingRadius"), 169014263.07462, 0.0001);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("CenterRingLongitude"), 180);

  std::unique_ptr<Histogram> oCubeStats(mos.histogram());

  EXPECT_NEAR(oCubeStats->Average(),           1079.2967741935483,    0.001);
  EXPECT_NEAR(oCubeStats->Sum(),               167291,  0.001);
  EXPECT_NEAR(oCubeStats->ValidPixels(),       155,     0.001);
  EXPECT_NEAR(oCubeStats->StandardDeviation(), 753.01066871414162, 0.001);

  FileList lout;
  lout.read(oFileListPath.toStdString());

  EXPECT_EQ(lout.size(), cubeFileList.size());
  EXPECT_PRED_FORMAT2(AssertStringsEqual, lout.at(0).expanded(), cubeFileList.at(0).expanded());
  EXPECT_PRED_FORMAT2(AssertStringsEqual, lout.at(1).expanded(), cubeFileList.at(1).expanded());
}


TEST_F(RingsCube, FunctionalTestRingsautomosPriority) {
  QString outPath = tempDir.path() + "/mosaic.cub";

  QVector<QString> args = {"fromlist=" + cubeListPath, "mosaic=" + outPath,
                           "priority=average", "highsat=true", "lowsat=true", "null=true"};

  UserInterface options(APP_XML, args);
  Pvl appLog;

  ringsautomos(options, &appLog);

  Cube mos(outPath.toStdString());
  Pvl label = *mos.label();

  PvlGroup mapping = label.findObject("IsisCube").findGroup("Mapping");

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("ProjectionName"), "Planar");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("TargetName"), "Saturn");
  EXPECT_NEAR((double)mapping.findKeyword("UpperLeftCornerX"), -141593057.92723, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("UpperLeftCornerY"), 141593057.92723, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("PixelResolution"), 5899710.746968, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("Scale"), 0.5, 0.0001);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("RingLongitudeDirection"), "CounterClockwise");
  EXPECT_EQ((int)mapping.findKeyword("RingLongitudeDomain"), 360);
  EXPECT_NEAR((double)mapping.findKeyword("MaximumRingRadius"), 198012526.14923, 0.0001);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MinimumRingLongitude"), 0);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MaximumRingLongitude"), 360);
  EXPECT_NEAR((double)mapping.findKeyword("CenterRingRadius"), 169014263.07462, 0.0001);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("CenterRingLongitude"), 180);

  std::unique_ptr<Histogram> oCubeStats(mos.histogram());

  EXPECT_NEAR(oCubeStats->Average(),           1434.8758620689655,    0.001);
  EXPECT_NEAR(oCubeStats->Sum(),               624171,  0.001);
  EXPECT_NEAR(oCubeStats->ValidPixels(),       435,     0.001);
  EXPECT_NEAR(oCubeStats->StandardDeviation(), 1167.5695079877848, 0.001);
}


TEST_F(RingsCube, FunctionalTestRingsautomosBandSelect) {
  QString outPath = tempDir.path() + "/mosaic.cub";

  QVector<QString> args = {"fromlist=" + cubeListPath, "mosaic=" + outPath,
                           "priority=band", "number=1", "criteria=lesser"};

  UserInterface options(APP_XML, args);
  Pvl appLog;

  ringsautomos(options, &appLog);

  Cube mos(outPath.toStdString());
  Pvl label = *mos.label();

  PvlGroup bandBin = label.findObject("IsisCube").findGroup("BandBin");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, bandBin.findKeyword("FilterName"), "CL1/CL2");
  ASSERT_EQ((int)bandBin.findKeyword("OriginalBand"), 1);
  ASSERT_DOUBLE_EQ((double)bandBin.findKeyword("Center"), 633.837);
  ASSERT_DOUBLE_EQ((double)bandBin.findKeyword("Width"), 285.938);

  PvlGroup mapping = label.findObject("IsisCube").findGroup("Mapping");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("ProjectionName"), "Planar");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("TargetName"), "Saturn");
  EXPECT_NEAR((double)mapping.findKeyword("UpperLeftCornerX"), -141593057.92723, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("UpperLeftCornerY"), 141593057.92723, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("PixelResolution"), 5899710.746968, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("Scale"), 0.5, 0.0001);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("RingLongitudeDirection"), "CounterClockwise");
  EXPECT_EQ((int)mapping.findKeyword("RingLongitudeDomain"), 360);
  EXPECT_NEAR((double)mapping.findKeyword("MaximumRingRadius"), 198012526.14923, 0.0001);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MinimumRingLongitude"), 0);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MaximumRingLongitude"), 360);
  EXPECT_NEAR((double)mapping.findKeyword("CenterRingRadius"), 169014263.07462, 0.0001);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("CenterRingLongitude"), 180);

  std::unique_ptr<Histogram> oCubeStats(mos.histogram());
  EXPECT_NEAR(oCubeStats->Average(),           1152.2840236686391,    0.001);
  EXPECT_NEAR(oCubeStats->Sum(),               778944,  0.001);
  EXPECT_NEAR(oCubeStats->ValidPixels(),       676,     0.001);
  EXPECT_NEAR(oCubeStats->StandardDeviation(), 1054.3443835915498, 0.001);
}
