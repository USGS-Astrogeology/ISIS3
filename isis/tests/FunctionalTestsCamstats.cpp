#include <QTextStream>
#include <QStringList>
#include <QTemporaryFile>

#include "camstats.h"
#include "CameraFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"


using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/camstats.xml").expanded();

TEST(CamStats, FunctionalTestCamstatsDefaultParameters) {
  QVector<QString> args = {"from=data/camstats/camstats-default.cub"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  camstats(options, &appLog);

  PvlGroup group = appLog.findGroup("UserParameters");
  EXPECT_DOUBLE_EQ((double) group.findKeyword("Linc"), 1.0);
  EXPECT_DOUBLE_EQ((double) group.findKeyword("Sinc"), 1.0);

  group = appLog.findGroup("Latitude");
  EXPECT_NEAR( (double) group.findKeyword("LatitudeMinimum"), 10.071886160496, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("LatitudeMaximum"), 10.115114798772, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("LatitudeAverage"), 10.093481303136, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("LatitudeStandardDeviation"), 0.0093294485296208998, 1e-8);

  group = appLog.findGroup("Longitude");
  EXPECT_NEAR( (double) group.findKeyword("LongitudeMinimum"), 255.64553280449999, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("LongitudeMaximum"), 255.68944338700999, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("LongitudeAverage"), 255.66752361025999, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("LongitudeStandardDeviation"), 0.0096378907096538995, 1e-8);

  group = appLog.findGroup("SampleResolution");
  EXPECT_NEAR( (double) group.findKeyword("SampleResolutionMinimum"), 18.840689155126, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("SampleResolutionMaximum"), 18.851286623334001, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("SampleResolutionAverage"), 18.846056043836999, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("SampleResolutionStandardDeviation"), 0.003012465806427, 1e-8);

  group = appLog.findGroup("LineResolution");
  EXPECT_NEAR( (double) group.findKeyword("LineResolutionMinimum"), 18.840689155126, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("LineResolutionMaximum"), 18.851286623334001, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("LineResolutionAverage"), 18.846056043836999, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("LineResolutionStandardDeviation"), 0.003012465806427, 1e-8);

  group = appLog.findGroup("Resolution");
  EXPECT_NEAR( (double) group.findKeyword("ResolutionMinimum"), 18.840689155126, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("ResolutionMaximum"), 18.851286623334001, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("ResolutionAverage"), 18.846056043836999, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("ResolutionStandardDeviation"), 0.003012465806427, 1e-8);

  group = appLog.findGroup("ObliqueSampleResolution");
  EXPECT_NEAR( (double) group.findKeyword("ObliqueSampleResolutionMinimum"), 19.155188929324002, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("ObliqueSampleResolutionMaximum"), 19.367224440177999, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("ObliqueSampleResolutionAverage"), 19.275477210727999, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("ObliqueSampleResolutionStandardDeviation"), 0.033844349179675999, 1e-8);

  group = appLog.findGroup("ObliqueLineResolution");
  EXPECT_NEAR( (double) group.findKeyword("ObliqueLineResolutionMinimum"), 19.155188929324002, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("ObliqueLineResolutionMaximum"), 19.367224440177999, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("ObliqueLineResolutionAverage"), 19.275477210727999, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("ObliqueLineResolutionStandardDeviation"), 0.033844349179675999, 1e-8);

  group = appLog.findGroup("ObliqueResolution");
  EXPECT_NEAR( (double) group.findKeyword("ObliqueResolutionMinimum"), 19.155188929324002, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("ObliqueResolutionMaximum"), 19.367224440177999, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("ObliqueResolutionAverage"), 19.275477210727999, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("ObliqueResolutionStandardDeviation"), 0.033844349179675999, 1e-8);

  group = appLog.findGroup("AspectRatio");
  EXPECT_DOUBLE_EQ( (double) group.findKeyword("AspectRatioMinimum"), 1.0);
  // Maximum spelled incorrectly to match misspelling in CameraStatistics.cpp
  EXPECT_DOUBLE_EQ( (double) group.findKeyword("AspectRatioMaximun"), 1.0);
  EXPECT_DOUBLE_EQ( (double) group.findKeyword("AspectRatioAverage"), 1.0);
  EXPECT_DOUBLE_EQ( (double) group.findKeyword("AspectRatioStandardDeviation"), 0.0);

  group = appLog.findGroup("PhaseAngle");
  EXPECT_NEAR( (double) group.findKeyword("PhaseMinimum"), 79.765542221500993, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("PhaseMaximum"), 79.913103465944999, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("PhaseAverage"), 79.839125602796003, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("PhaseStandardDeviation"), 0.042323277540651003, 1e-8);

  group = appLog.findGroup("EmissionAngle");
  EXPECT_NEAR( (double) group.findKeyword("EmissionMinimum"), 10.79844564701, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("EmissionMaximum"), 11.034510484207001, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("EmissionAverage"), 10.916733851338, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("EmissionStandardDeviation"), 0.051256401936749997, 1e-8);

  group = appLog.findGroup("IncidenceAngle");
  EXPECT_NEAR( (double) group.findKeyword("IncidenceMinimum"), 70.261909633600993, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("IncidenceMaximum"), 70.296704005951, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("IncidenceAverage"), 70.279320858073007, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("IncidenceStandardDeviation"), 0.0096632244052295996, 1e-8);

  group = appLog.findGroup("LocalSolarTime");
  EXPECT_NEAR( (double) group.findKeyword("LocalSolarTimeMinimum"), 7.7698044038522998, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("LocalSolarTimeMaximum"), 7.7727317760194001, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("LocalSolarTimeAverage"), 7.7712704575698996, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("LocalSolarTimeStandardDeviation"), 0.00064252695695377895, 1e-8);

  group = appLog.findGroup("LocalRadius");
  EXPECT_NEAR( (double) group.findKeyword("LocalRadiusMinimum"), 3411855.0366011998, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("LocalRadiusMaximum"), 3412097.9756964999, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("LocalRadiusAverage"), 3411965.1948032002, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("LocalRadiusStandardDeviation"), 54.219727730480997, 1e-5);

  group = appLog.findGroup("NorthAzimuth");
  EXPECT_NEAR( (double) group.findKeyword("NorthAzimuthMinimum"), 332.91681144925002, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("NorthAzimuthMaximum"), 338.21827790755998, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("NorthAzimuthAverage"), 333.98488155281001, 1e-8);
  EXPECT_NEAR( (double) group.findKeyword("NorthAzimuthStandardDeviation"), 0.48452163466794002, 1e-8);
}

TEST_F(DefaultCube, FunctionalTestCamstatsAttach) {
  QVector<QString> args = {"attach=true", "linc=100", "sinc=100"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  QString inPath = testCube->fileName();

  camstats(testCube, options, &appLog);

  testCube->open(inPath, "r");
  EXPECT_TRUE(testCube->hasTable("CameraStatistics"));
}

TEST_F(DefaultCube, FunctionalTestCamstatsFlat) {
  QTemporaryFile flatFile;
  flatFile.open();

  QVector<QString> args = {"to=" + flatFile.fileName(), "format=flat", "linc=100", "sinc=100"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  camstats(testCube, options, &appLog);

  int lineNumber = 0;
  QTextStream flatStream(&flatFile);
  while(!flatStream.atEnd()) {
    QString line = flatStream.readLine();
    QStringList fields = line.split(",");

    if(lineNumber == 0) {
      EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(1), "LatitudeMinimum");
      EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(2), "LatitudeMaximum");
    }
    else if(lineNumber == 1) {
      EXPECT_NEAR(fields.value(2).toDouble(), 10.434709879, 1e-8);
      EXPECT_NEAR(fields.value(1).toDouble(), 9.928647861, 1e-8);
    }
    lineNumber++;
  }
}
