#include "CameraFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "LineManager.h"
#include "Histogram.h"

#include "hicubenorm.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/hicubenorm.xml").expanded());

TEST_F(MroHiriseCube, FunctionalTestHicubenormSubtract) {
  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path()+"/outTEMP.cub";
  QString outStatsFile = prefix.path()+"/stats.csv";
  QVector<QString> args = {"to="+outCubeFileName, "stats="+outStatsFile, "format=PVL", "mode=subtract", "filter=5"};

  UserInterface options(APP_XML, args);
  try {
    hicubenorm(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to process HRISE image: " << e.what() << std::endl;
  }

  std::ifstream statsFile(outStatsFile.toStdString());
  Pvl stats;
  statsFile >> stats;

  PvlGroup res = stats.findGroup("Results");

  // check first column, middle column, and last column
  ASSERT_DOUBLE_EQ((double)res[3], -7.3641443519534);
  ASSERT_DOUBLE_EQ((double)res[4], -7.3641443519534);
  ASSERT_DOUBLE_EQ((double)res[5], 0);
  ASSERT_DOUBLE_EQ((double)res[6], 1);
  ASSERT_DOUBLE_EQ((double)res[7], 1);

  ASSERT_DOUBLE_EQ((double)res[315], 0);
  ASSERT_DOUBLE_EQ((double)res[316], 0);
  ASSERT_DOUBLE_EQ((double)res[317], 0);
  ASSERT_DOUBLE_EQ((double)res[318], 40);
  ASSERT_DOUBLE_EQ((double)res[319], 40);

  ASSERT_DOUBLE_EQ((double)res[9627], 5.1018076194189);
  ASSERT_DOUBLE_EQ((double)res[9628], 5.1018076194189);
  ASSERT_DOUBLE_EQ((double)res[9629], 0);
  ASSERT_DOUBLE_EQ((double)res[9630], 1204);
  ASSERT_DOUBLE_EQ((double)res[9631], 1204);

  Cube oCube(outCubeFileName.toStdString(), "r");

  Histogram *oCubeStats = oCube.histogram();

  ASSERT_DOUBLE_EQ(oCubeStats->Average(), 602.49999332903235);
  ASSERT_DOUBLE_EQ(oCubeStats->Sum(), 766032951.51837158);
  ASSERT_EQ(oCubeStats->ValidPixels(), 1271424);
  ASSERT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 347.47625973541767);

  delete oCubeStats;
}


TEST_F(MroHiriseCube, FunctionalTestHicubenormDivide) {

  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path()+"/outTEMP.cub";
  QString outStatsFile = prefix.path()+"/stats.csv";
  QVector<QString> args = {"to="+outCubeFileName, "mode=divide"};

  UserInterface options(APP_XML, args);
  try {
    hicubenorm(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to process HRISE image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString(), "r");

  Histogram *oCubeStats = oCube.histogram();

  ASSERT_DOUBLE_EQ(oCubeStats->Average(), -1462164.1207275416);
  ASSERT_DOUBLE_EQ(oCubeStats->Sum(), -78746310885.902481);
  ASSERT_EQ(oCubeStats->ValidPixels(), 53856);
  ASSERT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 9547737.4830061328);

  delete oCubeStats;
}


TEST_F(MroHiriseCube, FunctionalTestHicubenormAverage) {

  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTEMP.cub";
  QVector<QString> args = {"to="+outCubeFileName, "mode=subtract", "normalizer=average"};

  UserInterface options(APP_XML, args);
  try {
    hicubenorm(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to process HRISE image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString(), "r");

  Histogram *oCubeStats = oCube.histogram();

  ASSERT_DOUBLE_EQ(oCubeStats->Average(), 602.49999332903235);
  ASSERT_DOUBLE_EQ(oCubeStats->Sum(), 766032951.51837158);
  ASSERT_EQ(oCubeStats->ValidPixels(), 1271424);
  ASSERT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 347.47625973541767);

  delete oCubeStats;
}


TEST_F(MroHiriseCube, FunctionalTestHicubenormNewVersion) {

  QTemporaryDir prefix;
  QString tablePath = prefix.path() + "/stats.pvl"; // prefix.path() + "/stats.pvl";
  QString outCubeFileName = prefix.path() + "/outTEMP.cub";

  // Generate a Statistics file
  PvlGroup stats("Results");
  for(int i = 1; i <= 1204; i++) {
    stats += PvlKeyword("Band", "1");
    stats += PvlKeyword("RowCol", Isis::toString(i));
    stats += PvlKeyword("ValidPixels", Isis::toString(1056));
    stats += PvlKeyword("Mean", Isis::toString(i));
    stats += PvlKeyword("Median", Isis::toString(i/2));
    // the rest shouldn't matter
    stats += PvlKeyword("Std", "0.0");
    stats += PvlKeyword("Minimum", "0.0");
    stats += PvlKeyword("Maximum", "0.0");
  }

  Pvl table;
  table.addGroup(stats);
  table.write(tablePath.toStdString());

  QVector<QString> args = {"to="+outCubeFileName, "mode=subtract", "fromstats="+tablePath, "statsource=pvl", "normalizer=average", "new_version=yes"};

  UserInterface options(APP_XML, args);
  try {
    hicubenorm(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to process HRISE image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString(), "r");

  Histogram *oCubeStats = oCube.histogram();

  ASSERT_DOUBLE_EQ(oCubeStats->Average(), 602.50000120516233);
  ASSERT_DOUBLE_EQ(oCubeStats->Sum(), 766032961.53227234);
  ASSERT_EQ(oCubeStats->ValidPixels(), 1271424);
  ASSERT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 347.55180258334673);

  delete oCubeStats;
}


TEST_F(MroHiriseCube, FunctionalTestHicubenormPreserve) {

  // force a 2D gradiant vs the default 1D gradiant
  LineManager line(*testCube);
  double pixelValue = 1;
  for(line.begin(); !line.end(); line++) {
      for(int i = 0; i < line.size(); i++) {
        line[i] = (double)pixelValue;
        pixelValue++;
      }
      testCube->write(line);
  }
  testCube->reopen("rw");

  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTEMP.cub";
  QVector<QString> args = {"to="+outCubeFileName, "mode=divide", "normalizer=median", "preserve=true", "pausecrop=false"};

  UserInterface options(APP_XML, args);
  try {
    hicubenorm(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to process HRISE image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString(), "r");

  Histogram *oCubeStats = oCube.histogram();

  ASSERT_DOUBLE_EQ(oCubeStats->Average(), -1.0098553295767402e-05);
  ASSERT_DOUBLE_EQ(oCubeStats->Sum(), -0.51187546945585805);
  ASSERT_EQ(oCubeStats->ValidPixels(), 50688);
  ASSERT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 0.056835086507636828);

  delete oCubeStats;
}
