#include "shadow.h"

#include <cmath>
#include <math.h>

#include "Cube.h"
#include "FileName.h"
#include "Histogram.h"
#include "LineManager.h"
#include "Pvl.h"
#include "Table.h"
#include "TestUtilities.h"

#include "CameraFixtures.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/shadow.xml").expanded());

TEST_F(DemCube, FunctionalTestShadowMatch) {
  QVector<QString> shadowArgs = {"to=" + tempDir.path() + "/shadow.cub",
                                 "match=" + testCube->fileName()};
  UserInterface shadowUi(APP_XML, shadowArgs);
  Pvl appLog;
  shadow(demCube, shadowUi, &appLog);

  PvlGroup shadowStats = appLog.findGroup("ShadowStatistics");
  EXPECT_EQ(int(shadowStats["NumComputedAzimuthElevations"]), 10000);
  EXPECT_DOUBLE_EQ(double(shadowStats["AverageAzimuth"]), 160.51969475898);
  EXPECT_DOUBLE_EQ(double(shadowStats["MinimumAzimuth"]), 160.28400847111);
  EXPECT_DOUBLE_EQ(double(shadowStats["MaximumAzimuth"]), 160.77111068527);

  EXPECT_DOUBLE_EQ(double(shadowStats["AverageElevation"]), 90.0);
  EXPECT_DOUBLE_EQ(double(shadowStats["MinimumElevation"]), 90.0);
  EXPECT_DOUBLE_EQ(double(shadowStats["MaximumElevation"]), 90.0);

  EXPECT_EQ(int(shadowStats["NumRays"]), 5551);
  EXPECT_EQ(int(shadowStats["NumRayDemIntersections"]), 5551);
  EXPECT_DOUBLE_EQ(double(shadowStats["AverageRayDemIntersectionsPerRay"]), 1.0);
  EXPECT_EQ(int(shadowStats["NumLightedPixels"]), 5551);
  EXPECT_EQ(int(shadowStats["NumShadowedPixels"]), 0);
  EXPECT_EQ(int(shadowStats["NumSpecialPixels"]), 2800);
  EXPECT_EQ(int(shadowStats["NumPixelsShadowedByRays"]), 0);

  Cube shadowCube(shadowUi.GetCubeName("TO").toStdString());

  std::unique_ptr<Histogram> hist (shadowCube.histogram());

  EXPECT_NEAR(hist->Average(), 0.00084506527240706553, 1e-11);
  EXPECT_NEAR(hist->Sum(), 4.6909573271316205, 1e-11);
  ASSERT_EQ(hist->ValidPixels(), 5551);
  EXPECT_NEAR(hist->StandardDeviation(), 0.0010084740620921499, 1e-11);
}

TEST_F(DemCube, FunctionalTestShadowTime) {
  QVector<QString> shadowArgs = {"to=" + tempDir.path() + "/shadow.cub",
                                 "sunpositionsource=time",
                                 "time=1977-07-09T15:05:53"};
  PvlGroup kernels = testCube->label()->findObject("IsisCube").findGroup("Kernels");
  shadowArgs.push_back("spk=" + QString::fromStdString(kernels["TargetPosition"][2]));

  UserInterface shadowUi(APP_XML, shadowArgs);
  Pvl appLog;
  shadow(demCube, shadowUi, &appLog);

  PvlGroup shadowStats = appLog.findGroup("ShadowStatistics");
  EXPECT_EQ(int(shadowStats["NumComputedAzimuthElevations"]), 10000);
  EXPECT_DOUBLE_EQ(double(shadowStats["AverageAzimuth"]), 141.60048536348);
  EXPECT_DOUBLE_EQ(double(shadowStats["MinimumAzimuth"]), 141.18641687989);
  EXPECT_DOUBLE_EQ(double(shadowStats["MaximumAzimuth"]), 142.02798316054);

  EXPECT_DOUBLE_EQ(double(shadowStats["AverageElevation"]), 54.723733952308997);
  EXPECT_DOUBLE_EQ(double(shadowStats["MinimumElevation"]), 54.185416336220001);
  EXPECT_DOUBLE_EQ(double(shadowStats["MaximumElevation"]), 55.260883777776002);

  EXPECT_EQ(int(shadowStats["NumRays"]), 9604);
  EXPECT_EQ(int(shadowStats["NumRayDemIntersections"]), 10177);
  EXPECT_DOUBLE_EQ(double(shadowStats["AverageRayDemIntersectionsPerRay"]), 1.0596626405664);
  EXPECT_EQ(int(shadowStats["NumLightedPixels"]), 9500);
  EXPECT_EQ(int(shadowStats["NumShadowedPixels"]), 104);
  EXPECT_EQ(int(shadowStats["NumSpecialPixels"]), 2800);
  EXPECT_EQ(int(shadowStats["NumPixelsShadowedByRays"]), 104);

  Cube shadowCube(shadowUi.GetCubeName("TO").toStdString());

  std::unique_ptr<Histogram> hist (shadowCube.histogram());

  EXPECT_NEAR(hist->Average(), 0.57755590112585775, 1e-11);
  EXPECT_NEAR(hist->Sum(), 5486.7810606956482, 1e-11);
  ASSERT_EQ(hist->ValidPixels(), 9500);
  EXPECT_NEAR(hist->StandardDeviation(), 0.0027122379225963896, 1e-11);
}

TEST_F(DemCube, FunctionalTestShadowNoShadow) {
  QVector<QString> shadowArgs = {"to=" + tempDir.path() + "/shadow.cub",
                                 "match=" + testCube->fileName(),
                                 "preset=noshadow"};
  UserInterface shadowUi(APP_XML, shadowArgs);
  Pvl appLog;
  shadow(demCube, shadowUi, &appLog);

  std::cout << appLog << '\n';

  PvlGroup shadowStats = appLog.findGroup("ShadowStatistics");
  EXPECT_EQ(int(shadowStats["NumComputedAzimuthElevations"]), 10000);
  EXPECT_DOUBLE_EQ(double(shadowStats["AverageAzimuth"]), 160.51969475898);
  EXPECT_DOUBLE_EQ(double(shadowStats["MinimumAzimuth"]), 160.28400847111);
  EXPECT_DOUBLE_EQ(double(shadowStats["MaximumAzimuth"]), 160.77111068527);

  EXPECT_DOUBLE_EQ(double(shadowStats["AverageElevation"]), 90.0);
  EXPECT_DOUBLE_EQ(double(shadowStats["MinimumElevation"]), 90.0);
  EXPECT_DOUBLE_EQ(double(shadowStats["MaximumElevation"]), 90.0);

  EXPECT_EQ(int(shadowStats["NumRays"]), 0);
  EXPECT_EQ(int(shadowStats["NumRayDemIntersections"]), 0);
  EXPECT_EQ(int(shadowStats["NumLightedPixels"]), 5551);
  EXPECT_EQ(int(shadowStats["NumShadowedPixels"]), 0);
  EXPECT_EQ(int(shadowStats["NumSpecialPixels"]), 2800);
  EXPECT_EQ(int(shadowStats["NumPixelsShadowedByRays"]), 0);

  Cube shadowCube(shadowUi.GetCubeName("TO").toStdString());

  std::unique_ptr<Histogram> hist (shadowCube.histogram());

  EXPECT_NEAR(hist->Average(), 0.00084506527240706553, 1e-11);
  EXPECT_NEAR(hist->Sum(), 4.6909573271316205, 1e-11);
  EXPECT_EQ(hist->ValidPixels(), 5551);
  EXPECT_NEAR(hist->StandardDeviation(), 0.0010084740620921499, 1e-11);
}

TEST_F(DemCube, FunctionalTestShadowAccurate) {
  QVector<QString> shadowArgs = {"to=" + tempDir.path() + "/shadow.cub",
                                 "match=" + testCube->fileName(),
                                 "preset=accurate"};
  UserInterface shadowUi(APP_XML, shadowArgs);
  Pvl appLog;
  shadow(demCube, shadowUi, &appLog);

  PvlGroup shadowStats = appLog.findGroup("ShadowStatistics");
  EXPECT_EQ(int(shadowStats["NumComputedAzimuthElevations"]), 10000);
  EXPECT_DOUBLE_EQ(double(shadowStats["AverageAzimuth"]), 160.51969475898);
  EXPECT_DOUBLE_EQ(double(shadowStats["MinimumAzimuth"]), 160.28400847111);
  EXPECT_DOUBLE_EQ(double(shadowStats["MaximumAzimuth"]), 160.77111068527);

  EXPECT_DOUBLE_EQ(double(shadowStats["AverageElevation"]), 90.0);
  EXPECT_DOUBLE_EQ(double(shadowStats["MinimumElevation"]), 90.0);
  EXPECT_DOUBLE_EQ(double(shadowStats["MaximumElevation"]), 90.0);

  EXPECT_EQ(int(shadowStats["NumRays"]), 5551);
  EXPECT_EQ(int(shadowStats["NumRayDemIntersections"]), 5551);
  EXPECT_DOUBLE_EQ(double(shadowStats["AverageRayDemIntersectionsPerRay"]), 1.0);
  EXPECT_EQ(int(shadowStats["NumLightedPixels"]), 5551);
  EXPECT_EQ(int(shadowStats["NumShadowedPixels"]), 0);
  EXPECT_EQ(int(shadowStats["NumSpecialPixels"]), 2800);
  EXPECT_EQ(int(shadowStats["NumPixelsShadowedByRays"]), 0);

  Cube shadowCube(shadowUi.GetCubeName("TO").toStdString());

  std::unique_ptr<Histogram> hist (shadowCube.histogram());

  EXPECT_NEAR(hist->Average(), 0.00084506527240706553, 1e-11);
  EXPECT_NEAR(hist->Sum(), 4.6909573271316205, 1e-11);
  ASSERT_EQ(hist->ValidPixels(), 5551);
  EXPECT_NEAR(hist->StandardDeviation(), 0.0010084740620921499, 1e-11);
}

TEST_F(DemCube, FunctionalTestShadowCustom) {
  QVector<QString> shadowArgs = {"to=" + tempDir.path() + "/shadow.cub",
                                 "match=" + testCube->fileName(),
                                 "preset=custom"};
  UserInterface shadowUi(APP_XML, shadowArgs);
  Pvl appLog;
  shadow(demCube, shadowUi, &appLog);

  PvlGroup shadowStats = appLog.findGroup("ShadowStatistics");
  EXPECT_EQ(int(shadowStats["NumComputedAzimuthElevations"]), 10000);
  EXPECT_DOUBLE_EQ(double(shadowStats["AverageAzimuth"]), 160.51969475898);
  EXPECT_DOUBLE_EQ(double(shadowStats["MinimumAzimuth"]), 160.28400847111);
  EXPECT_DOUBLE_EQ(double(shadowStats["MaximumAzimuth"]), 160.77111068527);

  EXPECT_DOUBLE_EQ(double(shadowStats["AverageElevation"]), 90.0);
  EXPECT_DOUBLE_EQ(double(shadowStats["MinimumElevation"]), 90.0);
  EXPECT_DOUBLE_EQ(double(shadowStats["MaximumElevation"]), 90.0);

  EXPECT_EQ(int(shadowStats["NumRays"]), 5551);
  EXPECT_EQ(int(shadowStats["NumRayDemIntersections"]), 5551);
  EXPECT_DOUBLE_EQ(double(shadowStats["AverageRayDemIntersectionsPerRay"]), 1.0);
  EXPECT_EQ(int(shadowStats["NumLightedPixels"]), 5551);
  EXPECT_EQ(int(shadowStats["NumShadowedPixels"]), 0);
  EXPECT_EQ(int(shadowStats["NumSpecialPixels"]), 2800);
  EXPECT_EQ(int(shadowStats["NumPixelsShadowedByRays"]), 0);

  Cube shadowCube(shadowUi.GetCubeName("TO").toStdString());

  std::unique_ptr<Histogram> hist (shadowCube.histogram());

  EXPECT_NEAR(hist->Average(), 0.00084506527240706553, 1e-11);
  EXPECT_NEAR(hist->Sum(), 4.6909573271316205, 1e-11);
  ASSERT_EQ(hist->ValidPixels(), 5551);
  EXPECT_NEAR(hist->StandardDeviation(), 0.0010084740620921499, 1e-11);
}

TEST_F(DemCube, FunctionalTestShadowErrors) {
  QVector<QString> shadowArgs = {"to=" + tempDir.path() + "/shadow.cub",
                                 "match=" + testCube->fileName()};
  UserInterface shadowUi(APP_XML, shadowArgs);
  try {
    shadow(testCube, shadowUi);
    FAIL() << "Shouldn't have been able to generate a shadow file" << std::endl;
  } catch(IException &e) {
    std::string expectedStr = "is not a proper DEM. All DEM files must now be padded at the poles and contain a ShapeModelStatistics table defining their minimum and maximum radii values";
    EXPECT_THAT(e.what(), testing::HasSubstr(expectedStr));
  }

  shadowArgs.pop_front();
  shadowArgs.pop_back();
  shadowArgs.push_back("match=" + demCube->fileName());
  shadowUi = UserInterface(APP_XML, shadowArgs);

  try {
    shadow(demCube, shadowUi);
    FAIL() << "Shouldn't have been able to generate a shadow file" << std::endl;
  } catch(IException &e) {
    std::string expectedStr = "Could not find the sun position from the match file";
    EXPECT_THAT(e.what(), testing::HasSubstr(expectedStr));
  }

  shadowArgs.pop_front();
  shadowArgs.pop_back();
  shadowArgs.push_back("match=" + testCube->fileName());
  shadowUi = UserInterface(APP_XML, shadowArgs);

  Table shapeModelStats = demCube->readTable("ShapeModelStatistics");

  TableRecord originalRecord = shapeModelStats[0];
  TableRecord badRecord = shapeModelStats[0];
  badRecord[0] = -1.0;

  shapeModelStats.Delete(0);
  shapeModelStats += badRecord;

  demCube->write(shapeModelStats);
  demCube->reopen("rw");

  try {
    shadow(demCube, shadowUi);
    FAIL() << "Shouldn't have been able to generate a shadow file" << std::endl;
  } catch(IException &e) {
    std::string expectedStr = "the shadowing algorithm must be a DEM which stores radii; The input DEM contains zero or negative radii";
    EXPECT_THAT(e.what(), testing::HasSubstr(expectedStr));
  }

  shapeModelStats.Delete(0);
  shapeModelStats += originalRecord;

  demCube->write(shapeModelStats);
  demCube->reopen("rw");
}
