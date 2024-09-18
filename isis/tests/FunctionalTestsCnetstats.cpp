#include "cnetstats.h"

#include <QTemporaryFile>
#include <QTextStream>
#include <QStringList>

#include "NetworkFixtures.h"
#include "PvlGroup.h"
#include "TestUtilities.h"

#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/cnetstats.xml").expanded());

TEST_F(ThreeImageNetwork, FunctionalTestCnetstatsDefault) {
  QVector<QString> args = {};
  UserInterface options(APP_XML, args);
  Pvl log;

  QString serialNumList = tempDir.path()+"/cubes.lis";

  cnetstats(*network, serialNumList, options, &log);

  PvlGroup cnetSummary = log.findGroup("ControlNetSummary");
  EXPECT_DOUBLE_EQ( (double) cnetSummary.findKeyword("TotalImages"), 3);
  EXPECT_DOUBLE_EQ( (double) cnetSummary.findKeyword("ImagesInControlNet"), 3);

  EXPECT_DOUBLE_EQ( (double) cnetSummary.findKeyword("TotalPoints"), 16);
  EXPECT_DOUBLE_EQ( (double) cnetSummary.findKeyword("ValidPoints"), 16);
  EXPECT_DOUBLE_EQ( (double) cnetSummary.findKeyword("IgnoredPoints"), 0);
  EXPECT_DOUBLE_EQ( (double) cnetSummary.findKeyword("FixedPoints"), 0);
  EXPECT_DOUBLE_EQ( (double) cnetSummary.findKeyword("ConstrainedPoints"), 0);
  EXPECT_DOUBLE_EQ( (double) cnetSummary.findKeyword("FreePoints"), 16);
  EXPECT_DOUBLE_EQ( (double) cnetSummary.findKeyword("EditLockPoints"), 0);

  EXPECT_DOUBLE_EQ( (double) cnetSummary.findKeyword("TotalMeasures"), 41);
  EXPECT_DOUBLE_EQ( (double) cnetSummary.findKeyword("ValidMeasures"), 41);
  EXPECT_DOUBLE_EQ( (double) cnetSummary.findKeyword("IgnoredMeasures"), 0);
  EXPECT_DOUBLE_EQ( (double) cnetSummary.findKeyword("EditLockMeasures"), 0);

  EXPECT_PRED_FORMAT2(AssertStringsEqual, cnetSummary.findKeyword("AvgResidual"), "Null");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, cnetSummary.findKeyword("MinResidual"), "Null");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, cnetSummary.findKeyword("MaxResidual"), "Null");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, cnetSummary.findKeyword("MinLineResidual"), "Null");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, cnetSummary.findKeyword("MaxLineResidual"), "Null");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, cnetSummary.findKeyword("MinSampleResidual"), "Null");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, cnetSummary.findKeyword("MaxSampleResidual"), "Null");

  EXPECT_DOUBLE_EQ( (double) cnetSummary.findKeyword("MinLineShift"), 0);
  EXPECT_DOUBLE_EQ( (double) cnetSummary.findKeyword("MaxLineShift"), 0);
  EXPECT_DOUBLE_EQ( (double) cnetSummary.findKeyword("MinSampleShift"), 0);
  EXPECT_DOUBLE_EQ( (double) cnetSummary.findKeyword("MaxSampleShift"), 0);
  EXPECT_DOUBLE_EQ( (double) cnetSummary.findKeyword("AvgPixelShift"), 0);
  EXPECT_DOUBLE_EQ( (double) cnetSummary.findKeyword("MinPixelShift"), 0);
  EXPECT_DOUBLE_EQ( (double) cnetSummary.findKeyword("MaxPixelShift"), 0);

  EXPECT_PRED_FORMAT2(AssertStringsEqual, cnetSummary.findKeyword("MinGoodnessOfFit"), "NA");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, cnetSummary.findKeyword("MaxGoodnessOfFit"), "NA");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, cnetSummary.findKeyword("MinEccentricity"), "NA");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, cnetSummary.findKeyword("MaxEccentricity"), "NA");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, cnetSummary.findKeyword("MinPixelZScore"), "NA");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, cnetSummary.findKeyword("MaxPixelZScore"), "NA");

  EXPECT_NEAR( (double) cnetSummary.findKeyword("MinConvexHullRatio"), 0.40388096067313, 1e-10);
  EXPECT_NEAR( (double) cnetSummary.findKeyword("MaxConvexHullRatio"), 0.60732301576372, 1e-10);
  EXPECT_NEAR( (double) cnetSummary.findKeyword("AvgConvexHullRatio"), 0.47889221036267, 1e-10);
}

TEST_F(ThreeImageNetwork, FunctionalTestCnetstatsImageStats) {
  // Setup output file
  QTemporaryFile statsFile;
  ASSERT_TRUE(statsFile.open());

  QVector<QString> args = {"create_image_stats=yes", "image_stats_file=" + statsFile.fileName()};
  UserInterface options(APP_XML, args);
  Pvl log;
  QString serialNumList = tempDir.path()+"/cubes.lis";

  cnetstats(*network, serialNumList, options, &log);

  // No need to check specific values because the ControlNetStatistics unit test should check them.
  QTextStream stream(&statsFile);
  while(!stream.atEnd()) {
    QString line = stream.readLine();
    QStringList values = line.split(",");
    ASSERT_DOUBLE_EQ(values.size(), 9);
  }
}

TEST_F(ThreeImageNetwork, FunctionalTestCnetstatsPointStats) {
  // Setup output file
  QTemporaryFile statsFile;
  ASSERT_TRUE(statsFile.open());

  QVector<QString> args = {"create_point_stats=yes", "point_stats_file=" + statsFile.fileName()};
  UserInterface options(APP_XML, args);
  Pvl log;
  QString serialNumList = tempDir.path() + "/cubes.lis";

  cnetstats(*network, serialNumList, options, &log);

  // No need to check specific values because the ControlNetStatistics unit test should check them.
  int lineNumber = 0;
  QTextStream stream(&statsFile);
  while(!stream.atEnd()) {
    QString line = stream.readLine();
    QStringList values = line.split(",");

    if(lineNumber == 0) {
      ASSERT_DOUBLE_EQ(values.size() - 1, 7);
    }
    else {
      ASSERT_DOUBLE_EQ(values.size(), 8);
    }
    lineNumber++;
  }
}

TEST_F(ThreeImageNetwork, FunctionalTestCnetstatsCubeFilter) {
  // Setup def file
  QTemporaryFile defFile("XXXXXX.def");
  ASSERT_TRUE(defFile.open());

  std::ofstream ofstream;
  ofstream.open(defFile.fileName().toStdString());
  ofstream << "Object = Filters\n\tGroup = Cube_NumPoints\n\t\tGreaterThan = 15\n\tEndGroup\nEndObject";
  ofstream.close();

  // Setup output flat file
  QTemporaryFile flatFile;
  ASSERT_TRUE(flatFile.open());

  QVector<QString> args = {"filter=yes",
                           "deffile=" + defFile.fileName(),
                           "flatfile=" + flatFile.fileName()};
  UserInterface options(APP_XML, args);
  Pvl log;
  QString serialNumList = tempDir.path()+"/cubes.lis";

  cnetstats(*network, serialNumList, options, &log);

  // No need to check specific values because the ControlNetFilter unit test should check them.
  int lineNumber = 0;
  QTextStream stream(&flatFile);
  while(!stream.atEnd()) {
    QString line = stream.readLine();
    QStringList values = line.split(",");

    if(lineNumber == 0) {
      ASSERT_DOUBLE_EQ(values.size() - 1, 8);
    }
    else {
      ASSERT_DOUBLE_EQ(values.size(), 9);
    }
    lineNumber++;
  }
  // Make sure the filter was applied
  ASSERT_DOUBLE_EQ(lineNumber, 2);
}

TEST_F(ThreeImageNetwork, FunctionalTestCnetstatsPointFilter) {
  // Setup def file
  QTemporaryFile defFile("XXXXXX.def");
  ASSERT_TRUE(defFile.open());

  std::ofstream ofstream;
  ofstream.open(defFile.fileName().toStdString());
  ofstream << "Object = Filters\n\tGroup = Point_NumMeasures\n\t\tLessThan = 2\n\tEndGroup\nEndObject";
  ofstream.close();

  // Setup output flat file
  QTemporaryFile flatFile;
  ASSERT_TRUE(flatFile.open());

  QVector<QString> args = {"filter=yes",
                           "deffile=" + defFile.fileName(),
                           "flatfile=" + flatFile.fileName()};
  UserInterface options(APP_XML, args);
  Pvl log;
  QString serialNumList = tempDir.path()+"/cubes.lis";

  cnetstats(*network, serialNumList, options, &log);

  // No need to check specific values because the ControlNetFilter unit test should check them.
  int lineNumber = 0;
  QTextStream stream(&flatFile);
  while(!stream.atEnd()) {
    QString line = stream.readLine();
    QStringList values = line.split(",");

    ASSERT_DOUBLE_EQ(values.size(), 13);
    lineNumber++;
  }
  // Make sure the filter was applied
  ASSERT_DOUBLE_EQ(lineNumber, 15);
}

TEST_F(ThreeImageNetwork, FunctionalTestCnetstatsInvalidDefFile) {
  // Setup def file
  QTemporaryFile defFile("XXXXXX.def");
  ASSERT_TRUE(defFile.open());

  std::ofstream ofstream;
  ofstream.open(defFile.fileName().toStdString());
  ofstream << "Object = Filters\n\tGroup = BadGroupName\n\tEndGroup\nEndObject";
  ofstream.close();

  // Setup output flat file
  QTemporaryFile flatFile;
  ASSERT_TRUE(flatFile.open());

  QVector<QString> args = {"filter=yes",
                           "deffile=" + defFile.fileName(),
                           "flatfile=" + flatFile.fileName()};
  UserInterface options(APP_XML, args);
  Pvl log;
  QString serialNumList = tempDir.path() + "/cubes.lis";

  QString message = "Invalid Deffile";
  try {
    cnetstats(*network, serialNumList, options, &log);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected error message: \"" << message.toStdString() << "\"";
  }
}
