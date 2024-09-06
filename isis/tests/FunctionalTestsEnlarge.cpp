#include "enlarge_app.h"

#include <QTemporaryFile>
#include <QTextStream>
#include <QStringList>

#include "CubeFixtures.h"
#include "PvlGroup.h"
#include "TestUtilities.h"

#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/enlarge.xml").expanded();

TEST_F(SmallCube, FunctionalTestEnlargeDefaultParameters) {
  QVector<QString> args = {"to=" + tempDir.path()+"/output.cub"};
  UserInterface options(APP_XML, args);
  Pvl appLog;
  enlarge(testCube, options, &appLog);

  PvlGroup groundPoint = appLog.findGroup("Results");

  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("InputLines"), 10);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("InputSamples"), 10);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("StartingLine"), 1);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("StartingSample"), 1);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("EndingLine"), 10);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("EndingSample"), 10);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("LineIncrement"), 1.0);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("SampleIncrement"), 1.0);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("OutputLines"), 10);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("OutputSamples"), 10);
}

TEST_F(SmallCube, FunctionalTestEnlargeScale) {
  QVector<QString> args = {"to=" + tempDir.path()+"/output.cub", "sscale=2", "lscale=4"};
  UserInterface options(APP_XML, args);
  Pvl appLog;
  enlarge(testCube, options, &appLog);

  PvlGroup groundPoint = appLog.findGroup("Results");

  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("LineIncrement"), .25);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("SampleIncrement"), .5);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("OutputLines"), 40);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("OutputSamples"), 20);
}

TEST_F(SmallCube, FunctionalTestEnlargeTotal) {
  QVector<QString> args = {"to=" + tempDir.path()+"/output.cub", "mode=total", "ons=20", "onl=40"};
  UserInterface options(APP_XML, args);
  Pvl appLog;
  enlarge(testCube, options, &appLog);

  PvlGroup groundPoint = appLog.findGroup("Results");

  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("LineIncrement"), .25);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("SampleIncrement"), .5);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("OutputLines"), 40);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("OutputSamples"), 20);
}

TEST_F(SmallCube, FunctionalTestEnlargeSmallDimensions) {
  QVector<QString> args = {"to=" + tempDir.path()+"/output.cub", "mode=total", "ons=10", "onl=1"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  std::string message = "Number of output samples/lines must be greater than or equal";
  try {
    enlarge(testCube, options, &appLog);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected error message: \"" << message.toStdString() << "\"";
  }
}

TEST_F(SmallCube, FunctionalTestEnlargeNearestNeighbor) {
  QVector<QString> args = {"to=" + tempDir.path()+"/output.cub", "interp=nearestneighbor"};
  UserInterface options(APP_XML, args);
  Pvl appLog;
  enlarge(testCube, options, &appLog);

  PvlGroup groundPoint = appLog.findGroup("Results");
  // Just make sure the applications runs without error. Tests the interpolation method in
  // the enlarge object.
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("LineIncrement"), 1.0);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("SampleIncrement"), 1.0);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("OutputLines"), 10);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("OutputSamples"), 10);
}

TEST_F(SmallCube, FunctionalTestEnlargeBilinear) {
  QVector<QString> args = {"to=" + tempDir.path()+"/output.cub", "interp=bilinear"};
  UserInterface options(APP_XML, args);
  Pvl appLog;
  enlarge(testCube, options, &appLog);

  PvlGroup groundPoint = appLog.findGroup("Results");
  // Just make sure the applications runs without error. Tests the interpolation method in
  // the enlarge object.
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("LineIncrement"), 1.0);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("SampleIncrement"), 1.0);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("OutputLines"), 10);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("OutputSamples"), 10);
}
