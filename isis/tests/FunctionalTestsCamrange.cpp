#include <QTextStream>
#include <QStringList>
#include <QTemporaryFile>

#include "camrange.h"
#include "CameraFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/camrange.xml").expanded();

TEST_F(DefaultCube, FunctionalTestCamrangeMeta) {
  QVector<QString> args = { "FROM=" + testCube->fileName() };
  UserInterface options(APP_XML, args);
  Pvl appLog;

  camrange(options, &appLog);

  PvlGroup target = appLog.findGroup("Target");
  EXPECT_PRED_FORMAT2( AssertQStringsEqual, target.findKeyword("FROM"), testCube->fileName() );
  EXPECT_PRED_FORMAT2( AssertQStringsEqual, target.findKeyword("TargetName"), "MARS");
  EXPECT_EQ( (double) target.findKeyword("RadiusA"), 3396190.0 );
  EXPECT_EQ( (double) target.findKeyword("RadiusB"), 3396190.0 );
  EXPECT_EQ( (double) target.findKeyword("RadiusC"), 3376200.0 );

  PvlGroup pixelResolution = appLog.findGroup("PixelResolution");
  EXPECT_EQ( (double) pixelResolution.findKeyword("Lowest"), 18.986042659077999);
  EXPECT_EQ( (double) pixelResolution.findKeyword("Highest"), 18.840630706272002);
}

TEST_F(DefaultCube, FunctionalTestCamrangeUniversalGround) {
  QVector<QString> args = { "FROM=" + testCube->fileName() };
  UserInterface options(APP_XML, args);
  Pvl appLog;

  camrange(options, &appLog);

  PvlGroup universalGroundRange = appLog.findGroup("UniversalGroundRange");
  EXPECT_PRED_FORMAT2( AssertQStringsEqual, universalGroundRange.findKeyword("LatitudeType"), "Planetocentric" );
  EXPECT_PRED_FORMAT2( AssertQStringsEqual, universalGroundRange.findKeyword("LongitudeDirection"), "PositiveEast" );
  EXPECT_PRED_FORMAT2( AssertQStringsEqual, universalGroundRange.findKeyword("LongitudeDirection"), "PositiveEast" );
  EXPECT_EQ( (int) universalGroundRange.findKeyword("LongitudeDomain"), 360 );
  EXPECT_EQ( (double) universalGroundRange.findKeyword("MinimumLatitude"), 9.9284292709020008);
  EXPECT_EQ( (double) universalGroundRange.findKeyword("MaximumLatitude"), 10.434928979757);
  EXPECT_EQ( (double) universalGroundRange.findKeyword("MinimumLongitude"), 255.64532659879001);
  EXPECT_EQ( (double) universalGroundRange.findKeyword("MaximumLongitude"), 256.14630120215003);
}

TEST_F(DefaultCube, FunctionalTestCamrangeLatitude) {
  QVector<QString> args = { "FROM=" + testCube->fileName() };
  UserInterface options(APP_XML, args);
  Pvl appLog;

  camrange(options, &appLog);

  PvlGroup latitudeRange = appLog.findGroup("LatitudeRange");
  EXPECT_PRED_FORMAT2( AssertQStringsEqual, latitudeRange.findKeyword("LatitudeType"), "Planetographic" );
  EXPECT_EQ( (double) latitudeRange.findKeyword("MinimumLatitude"), 10.043959653390001);
  EXPECT_EQ( (double) latitudeRange.findKeyword("MaximumLatitude"), 10.556092485413);
}

TEST_F(DefaultCube, FunctionalTestCamrangeCardinals) {
  QVector<QString> args = { "FROM=" + testCube->fileName() };
  UserInterface options(APP_XML, args);
  Pvl appLog;

  camrange(options, &appLog);

  PvlGroup positiveWest360 = appLog.findGroup("PositiveWest360");
  EXPECT_PRED_FORMAT2( AssertQStringsEqual, positiveWest360.findKeyword("LongitudeDirection"), "PositiveWest" );
  EXPECT_EQ( (int) positiveWest360.findKeyword("LongitudeDomain"), 360 );
  EXPECT_EQ( (double) positiveWest360.findKeyword("MinimumLongitude"), 103.85369879785);
  EXPECT_EQ( (double) positiveWest360.findKeyword("MaximumLongitude"), 104.35467340120999);

  PvlGroup  positiveEast180 = appLog.findGroup("PositiveEast180");
  EXPECT_PRED_FORMAT2( AssertQStringsEqual, positiveEast180.findKeyword("LongitudeDirection"), "PositiveEast" );
  EXPECT_EQ( (int) positiveEast180.findKeyword("LongitudeDomain"), 180 );
  EXPECT_EQ( (double) positiveEast180.findKeyword("MinimumLongitude"), -104.35467340120999);
  EXPECT_EQ( (double) positiveEast180.findKeyword("MaximumLongitude"), -103.85369879785);

  PvlGroup positiveWest180 = appLog.findGroup("PositiveWest180");
  EXPECT_PRED_FORMAT2( AssertQStringsEqual, positiveWest180.findKeyword("LongitudeDirection"), "PositiveWest" );
  EXPECT_EQ( (int) positiveWest180.findKeyword("LongitudeDomain"), 180 );
  EXPECT_EQ( (double) positiveWest180.findKeyword("MinimumLongitude"), 103.85369879785);
  EXPECT_EQ( (double) positiveWest180.findKeyword("MaximumLongitude"), 104.35467340120999);
}

TEST_F(DefaultCube, FunctionalTestCamrangeWriteTo) {
  QFile outFile(tempDir.path()+"outFile.txt");
  QVector<QString> args = { "FROM=" + testCube->fileName(),
                            "TO="   + outFile.fileName() };
  UserInterface options(APP_XML, args);
  Pvl appLog;

  int sizeBefore = outFile.size();
  camrange(options, &appLog);
  int sizeAfter = outFile.size();

  EXPECT_TRUE( sizeBefore < sizeAfter );
}
