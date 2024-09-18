#include <iostream>
#include <fstream>

#include "lrolola2isis.h"
#include "Fixtures.h"
#include "TestUtilities.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace Isis;
using namespace testing;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/lrolola2isis.xml").expanded());

TEST_F(LidarObservationPair, FunctionalTestLrolola2isisTwoImage) {
  QString testFilePath = tempDir.path() + "/LidarTest_TwoImage";

  QVector<QString> args = {"from=" + csvPath,
                           "cubes=" + cubeListFile,
                           "to=" + testFilePath,
                           "outputtype=test",
                           "threshold=10",
                           "point_range_sigma=10",
                           "point_latitude_sigma=10",
                           "point_longitude_sigma=10",
                           "point_radius_sigma=10",
                           "pointid=Lidar????"};
  UserInterface options(APP_XML, args);

  try {
    lrolola2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LRO images / point cloud: " <<  e.toString() << std::endl;
  }

  QString otestFilePath = testFilePath + ".json";
  nlohmann::json testJson;
  nlohmann::json truthJson;

  std::ifstream ifs;
  ifs.open(otestFilePath.toStdString());
  ifs >> testJson;
  ifs.close();

  ifs.open("data/lrolola2isis/Lrolola2isisTruth.json");
  ifs >> truthJson;
  ifs.close();

  // If the model changes slightly, then the back-projected image coordinate
  // can change slightly so use a 0.01 pixel tolerance
  EXPECT_PRED_FORMAT3(AssertJsonsNear, testJson, truthJson, 0.01);
}

TEST_F(LidarObservationPair, FunctionalTestLrolola2isisMultipleCsv) {
  QString testFilePath = tempDir.path() + "/LidarTest_MultipleCsv";

  QVector<QString> args = {"fromlist=data/lrolola2isis/multipleCsv.lis",
                           "cubes=" + cubeListFile,
                           "to=" + testFilePath,
                           "outputtype=test",
                           "threshold=10",
                           "point_range_sigma=10",
                           "point_latitude_sigma=10",
                           "point_longitude_sigma=10",
                           "point_radius_sigma=10",
                           "pointid=Lidar????"};
  UserInterface options(APP_XML, args);

  try {
    lrolola2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LRO images / point cloud: " <<  e.toString() << std::endl;
  }

  QString otestFilePath = testFilePath + ".json";
  nlohmann::json testJson;
  nlohmann::json truthJson;

  std::ifstream ifs;
  ifs.open(otestFilePath.toStdString());
  ifs >> testJson;
  ifs.close();

  ifs.open("data/lrolola2isis/Lrolola2isisTruth.json");
  ifs >> truthJson;
  ifs.close();

  // If the model changes slightly, then the back-projected image coordinate
  // can change slightly so use a 0.01 pixel tolerance
  EXPECT_PRED_FORMAT3(AssertJsonsNear, testJson, truthJson, 0.01);
}
