#include "automos.h"

#include "CameraFixtures.h"
#include "FileList.h"
#include "TestUtilities.h"
#include "UserInterface.h"
#include "Histogram.h"

#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/automos.xml").expanded();

TEST_F(DefaultCube, FunctionalTestAutomosDefault) {
  QString singleCubeListPath = tempDir.path() + "/newCubeList.lis";
  FileList singleCubeList;
  singleCubeList.append(projTestCube->fileName());
  singleCubeList.write(singleCubeListPath);
  QString outPath = tempDir.path() + "/mosaic.cub";

  QVector<QString> args = {"fromlist=" + singleCubeListPath, "mosaic=" + outPath};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  automos(options, &appLog);

  EXPECT_TRUE(appLog.hasGroup("ImageLocation"));

  Cube mos(outPath);
  Pvl label = *mos.label();

  PvlGroup mapping = label.findObject("IsisCube").findGroup("Mapping");

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("ProjectionName"), "Sinusoidal");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("TargetName"), "MARS");
  EXPECT_EQ((double)mapping.findKeyword("UpperLeftCornerX"), 0);
  EXPECT_EQ((double)mapping.findKeyword("UpperLeftCornerY"), 600000);
  EXPECT_EQ((double)mapping.findKeyword("PixelResolution"), 100000);
  EXPECT_EQ((double)mapping.findKeyword("Scale"), 0.59274697523305997);
  EXPECT_EQ(mapping.findKeyword("LatitudeType")[0], "Planetocentric");
  EXPECT_EQ(mapping.findKeyword("LongitudeDirection")[0], "PositiveEast");
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("EquatorialRadius"), 3396190.0);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("PolarRadius"), 3376200.0);
  EXPECT_EQ((int)mapping.findKeyword("LongitudeDomain"), 360);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MinimumLongitude"), 0.0);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MaximumLongitude"), 10.0);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MinimumLatitude"), 0.0);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MaximumLatitude"), 10.0);

  std::unique_ptr<Histogram> oCubeStats(mos.histogram());

  EXPECT_DOUBLE_EQ(oCubeStats->Average(), 123.5);
  EXPECT_EQ(oCubeStats->Sum(), 4446);
  EXPECT_EQ(oCubeStats->ValidPixels(), 36);
  EXPECT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 79.757668686375951);
}


TEST_F(DefaultCube, FunctionalTestAutomosSetRanges) {
  QString oFileListPath = tempDir.path() + "/outFileList.txt";
  QString outPath = tempDir.path() + "/mosaic.cub";

  QString singleCubeListPath = tempDir.path() + "/newCubeList.lis";
  FileList singleCubeList;
  singleCubeList.append(projTestCube->fileName());
  singleCubeList.write(singleCubeListPath);

  QVector<QString> args = {"fromlist=" + singleCubeListPath, "mosaic=" + outPath,
                           "tolist=" + oFileListPath, "priority=beneath", "grange=user",
                           "minlat=2", "maxlat=8", "minlon=2", "maxlon=8",
                           "matchbandbin=false", "matchdem=true"};

  UserInterface options(APP_XML, args);

  automos(options);

  Cube mos(outPath);
  Pvl label = *mos.label();

  PvlGroup mapping = label.findObject("IsisCube").findGroup("Mapping");

  EXPECT_EQ((double)mapping.findKeyword("UpperLeftCornerX"), 100000);
  EXPECT_EQ((double)mapping.findKeyword("UpperLeftCornerY"), 500000);
  EXPECT_EQ((double)mapping.findKeyword("PixelResolution"), 100000);
  EXPECT_EQ((double)mapping.findKeyword("Scale"), 0.59274697523305997);
  EXPECT_EQ((int)mapping.findKeyword("LongitudeDomain"), 360);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MinimumLongitude"), 2.0);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MaximumLongitude"), 8.0);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MinimumLatitude"), 2.0);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MaximumLatitude"), 8.0);

  FileList lout;
  lout.read(oFileListPath);

  EXPECT_EQ(lout.size(), singleCubeList.size());
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, lout.at(0).expanded(), singleCubeList.at(0).expanded());
}


TEST_F(DefaultCube, FunctionalTestAutomosPriority) {
  QString outPath = tempDir.path() + "/mosaic.cub";
  QString singleCubeListPath = tempDir.path() + "/newCubeList.lis";

  FileList singleCubeList;
  singleCubeList.append(projTestCube->fileName());
  singleCubeList.write(singleCubeListPath);

  QVector<QString> args = {"fromlist=" + singleCubeListPath, "mosaic=" + outPath,
                           "priority=average", "highsat=true", "lowsat=true", "null=true"};

  UserInterface options(APP_XML, args);

  automos(options);

  Cube mos(outPath);
  Pvl label = *mos.label();

  PvlGroup mapping = label.findObject("IsisCube").findGroup("Mapping");

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("ProjectionName"), "Sinusoidal");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("TargetName"), "MARS");
  EXPECT_EQ((double)mapping.findKeyword("UpperLeftCornerX"), 0);
  EXPECT_EQ((double)mapping.findKeyword("UpperLeftCornerY"), 600000);
  EXPECT_EQ((double)mapping.findKeyword("PixelResolution"), 100000);
  EXPECT_EQ((double)mapping.findKeyword("Scale"), 0.59274697523305997);
  EXPECT_EQ(mapping.findKeyword("LatitudeType")[0], "Planetocentric");
  EXPECT_EQ(mapping.findKeyword("LongitudeDirection")[0], "PositiveEast");
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("EquatorialRadius"), 3396190.0);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("PolarRadius"), 3376200.0);
  EXPECT_EQ((int)mapping.findKeyword("LongitudeDomain"), 360);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MinimumLongitude"), 0.0);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MaximumLongitude"), 10.0);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MinimumLatitude"), 0.0);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MaximumLatitude"), 10.0);

  std::unique_ptr<Histogram> oCubeStats(mos.histogram());

  EXPECT_DOUBLE_EQ(oCubeStats->Average(), 123.5);
  EXPECT_EQ(oCubeStats->Sum(), 4446);
  EXPECT_EQ(oCubeStats->ValidPixels(), 36);
  EXPECT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 79.757668686375951);
}

TEST_F(DefaultCube, FunctionalTestAutomosTracking) {
  QString outPath = tempDir.path() + "/mosaic.cub";
  QString singleCubeListPath = tempDir.path() + "/newCubeList.lis";
  QString trackingPath = tempDir.path() + "/mosaic_tracking.cub";

  FileList singleCubeList;
  singleCubeList.append(projTestCube->fileName());
  singleCubeList.write(singleCubeListPath);

  QVector<QString> args = {"fromlist=" + singleCubeListPath, "mosaic=" + outPath,
                           "priority=band", "highsat=true", "lowsat=true", "null=true", "track=true"};

  UserInterface options(APP_XML, args);

  automos(options);

  Cube mos(outPath);
  Cube track(trackingPath);
  Pvl label = *mos.label();
  Pvl trackingLabel = *track.label();

  PvlGroup mapping = label.findObject("IsisCube").findGroup("Mapping");
  PvlGroup tMapping = trackingLabel.findObject("IsisCube").findGroup("Mapping");

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("ProjectionName"), tMapping.findKeyword("ProjectionName"));
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("TargetName"), tMapping.findKeyword("TargetName"));
  EXPECT_EQ((double)mapping.findKeyword("UpperLeftCornerX"), (double)mapping.findKeyword("UpperLeftCornerX"));
  EXPECT_EQ((double)mapping.findKeyword("UpperLeftCornerY"), (double)tMapping.findKeyword("UpperLeftCornerY"));
  EXPECT_EQ((double)mapping.findKeyword("PixelResolution"), (double)tMapping.findKeyword("PixelResolution"));
  EXPECT_EQ((double)mapping.findKeyword("Scale"), (double)tMapping.findKeyword("Scale"));
  EXPECT_EQ(mapping.findKeyword("LatitudeType")[0], tMapping.findKeyword("LatitudeType")[0]);
  EXPECT_EQ(mapping.findKeyword("LongitudeDirection")[0], tMapping.findKeyword("LongitudeDirection")[0]);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("EquatorialRadius"), (double)tMapping.findKeyword("EquatorialRadius"));
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("PolarRadius"), (double)tMapping.findKeyword("PolarRadius"));
  EXPECT_EQ((int)mapping.findKeyword("LongitudeDomain"), (int)tMapping.findKeyword("LongitudeDomain"));
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MinimumLongitude"), (double)tMapping.findKeyword("MinimumLongitude"));
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MaximumLongitude"), (double)tMapping.findKeyword("MaximumLongitude"));
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MinimumLatitude"), (double)tMapping.findKeyword("MinimumLatitude"));
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MaximumLatitude"), (double)tMapping.findKeyword("MaximumLatitude"));
}


TEST_F(DefaultCube, FunctionalTestAutomosBandSelect) {
  QString outPath = tempDir.path() + "/mosaic.cub";
  QString singleCubeListPath = tempDir.path() + "/newCubeList.lis";

  FileList singleCubeList;
  singleCubeList.append(projTestCube->fileName());
  singleCubeList.write(singleCubeListPath);

  QVector<QString> args = {"fromlist=" + singleCubeListPath, "mosaic=" + outPath,
                           "priority=band", "number=1", "criteria=lesser"};

  UserInterface options(APP_XML, args);

  automos(options);

  Cube mos(outPath);
  Pvl label = *mos.label();

  PvlGroup mapping = label.findObject("IsisCube").findGroup("Mapping");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("ProjectionName"), "Sinusoidal");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("TargetName"), "MARS");
  EXPECT_EQ((double)mapping.findKeyword("UpperLeftCornerX"), 0);
  EXPECT_EQ((double)mapping.findKeyword("UpperLeftCornerY"), 600000);
  EXPECT_EQ((double)mapping.findKeyword("PixelResolution"), 100000);
  EXPECT_EQ((double)mapping.findKeyword("Scale"), 0.59274697523305997);
  EXPECT_EQ(mapping.findKeyword("LatitudeType")[0], "Planetocentric");
  EXPECT_EQ(mapping.findKeyword("LongitudeDirection")[0], "PositiveEast");
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("EquatorialRadius"), 3396190.0);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("PolarRadius"), 3376200.0);
  EXPECT_EQ((int)mapping.findKeyword("LongitudeDomain"), 360);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MinimumLongitude"), 0.0);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MaximumLongitude"), 10.0);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MinimumLatitude"), 0.0);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MaximumLatitude"), 10.0);

  std::unique_ptr<Histogram> oCubeStats(mos.histogram());

  EXPECT_DOUBLE_EQ(oCubeStats->Average(), 123.5);
  EXPECT_EQ(oCubeStats->Sum(), 4446);
  EXPECT_EQ(oCubeStats->ValidPixels(), 36);
  EXPECT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 79.757668686375951);
}
