#include <QTextStream>
#include <QStringList>
#include <QFile>

#include "csm/csm.h"
#include "csm/Ellipsoid.h"

#include "campt.h"
#include "CameraFixtures.h"
#include "CsmFixtures.h"
#include "Mocks.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "SpecialPixel.h"

#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/campt.xml").expanded();

TEST_F(DefaultCube, FunctionalTestCamptBadColumnError) {
  // set up bad coordinates file
  std::ofstream of;
  of.open(tempDir.path().toStdString()+"/badList.lis");
  of << "1, 10,\n10,100,500\n100";
  of.close();

  // configure UserInterface arguments
  QVector<QString> args = {"to=" + tempDir.path()+"/output.pvl",
                           "coordlist=" + tempDir.path()+"/badList.lis",
                           "coordtype=image"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  try {
    campt(testCube, options, &appLog);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(Isis::IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Coordinate file formatted incorrectly."))
      << e.toString().toStdString();
  }
  catch(...) {
    FAIL() << "Expected an IException with message: \"Coordinate file formatted incorrectly.\n"
              "Each row must have two columns: a sample,line or a latitude,longitude pair.\"";
  }
}


TEST_F(DefaultCube, FunctionalTestCamptFlatFileError) {
  // configure UserInterface arguments for flat file error
  QVector<QString> args = {"format=flat"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  try {
    campt(testCube, options, &appLog);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(Isis::IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Flat file must have a name."))
      << e.toString().toStdString();
  }
  catch(...) {
    FAIL() << "Expected an IException with message: \"Flat file must have a name.\"";
  }
}

TEST_F(DefaultCube, FunctionalTestCamptDefaultParameters) {
  QVector<QString> args = {};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  campt(testCube, options, &appLog);
  PvlGroup groundPoint = appLog.findGroup("GroundPoint");

  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("Sample"), 602.0);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("Line"), 528.0);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, groundPoint.findKeyword("PixelValue"), "Null");
  EXPECT_NEAR( (double) groundPoint.findKeyword("RightAscension"), 310.2070335306, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("Declination"), -46.327246785573, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("PlanetocentricLatitude"), 10.181441241544, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("PlanetographicLatitude"), 10.299790241741, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("PositiveEast360Longitude"), 255.89292858176, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("PositiveEast180Longitude"), -104.10707141824, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("PositiveWest360Longitude"), 104.10707141824, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("PositiveWest180Longitude"), 104.10707141824, 1e-8);

  EXPECT_NEAR( toDouble(groundPoint.findKeyword("BodyFixedCoordinate")[0]), -818.59644751849999, 1e-8);
  EXPECT_NEAR( toDouble(groundPoint.findKeyword("BodyFixedCoordinate")[1]), -3257.2675597553998, 1e-8);
  EXPECT_NEAR( toDouble(groundPoint.findKeyword("BodyFixedCoordinate")[2]),  603.17640797124, 1e-8);

  EXPECT_NEAR( (double) groundPoint.findKeyword("LocalRadius"), 3412288.6570258001, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("SampleResolution"), 18.904248467739, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("LineResolution"), 18.904248467739, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("ObliqueDetectorResolution"), 19.589652452595999, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("ObliquePixelResolution"), 19.589652452595999, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("ObliqueLineResolution"), 19.589652452595999, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("ObliqueSampleResolution"), 19.589652452595999, 1e-8);

  EXPECT_NEAR( toDouble(groundPoint.findKeyword("SpacecraftPosition")[0]), -1152.8979327717, 1e-8);
  EXPECT_NEAR( toDouble(groundPoint.findKeyword("SpacecraftPosition")[1]), -3930.9421518203, 1e-8);
  EXPECT_NEAR( toDouble(groundPoint.findKeyword("SpacecraftPosition")[2]),  728.14118380775, 1e-8);

  EXPECT_NEAR( (double) groundPoint.findKeyword("SpacecraftAzimuth"), 240.08514246657, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("SlantDistance"), 762.37204449951003, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("TargetCenterDistance"), 4160.7294345949, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("SubSpacecraftLatitude"), 10.078847382918, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("SubSpacecraftLongitude"), 253.65422317887, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("SpacecraftAltitude"), 753.22374841704, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("OffNadirAngle"), 9.9273765143684, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("SubSpacecraftGroundAzimuth"), 267.5318718687, 1e-8);

  EXPECT_NEAR( toDouble(groundPoint.findKeyword("SunPosition")[0]),  147591102.63158, 1e-8);
  EXPECT_NEAR( toDouble(groundPoint.findKeyword("SunPosition")[1]), -127854342.1274, 1e-8);
  EXPECT_NEAR( toDouble(groundPoint.findKeyword("SunPosition")[2]), -81844199.02275, 1e-8);

  EXPECT_NEAR( (double) groundPoint.findKeyword("SubSolarAzimuth"), 92.033828156965, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("SolarDistance"), 1.4153000672557, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("SubSolarLatitude"), -22.740326163641, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("SubSolarLongitude"), 319.09846558533, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("SubSolarGroundAzimuth"), 118.87356333938, 1e-8);

  EXPECT_NEAR( (double) groundPoint.findKeyword("Phase"), 80.528381932125, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("Incidence"), 70.127983116628, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("Emission"), 12.133564327344, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("NorthAzimuth"), 332.65918493997, 2e-8);

  EXPECT_NEAR( (double) groundPoint.findKeyword("EphemerisTime"), -709401200.26114, 1e-8);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, groundPoint.findKeyword("UTC"), "1977-07-09T20:05:51.5549999");
  EXPECT_NEAR( (double) groundPoint.findKeyword("LocalSolarTime"), 7.7862975330952, 1e-8);
  EXPECT_NEAR( (double) groundPoint.findKeyword("SolarLongitude"), 294.73518830594998, 1e-8);

  EXPECT_NEAR( toDouble(groundPoint.findKeyword("LookDirectionBodyFixed")[0]),  0.43850176257802, 1e-8);
  EXPECT_NEAR( toDouble(groundPoint.findKeyword("LookDirectionBodyFixed")[1]),  0.88365594846443, 1e-8);
  EXPECT_NEAR( toDouble(groundPoint.findKeyword("LookDirectionBodyFixed")[2]), -0.16391573737569, 1e-8);

  EXPECT_NEAR( toDouble(groundPoint.findKeyword("LookDirectionJ2000")[0]),  0.44577814515745, 1e-8);
  EXPECT_NEAR( toDouble(groundPoint.findKeyword("LookDirectionJ2000")[1]), -0.52737586689974, 1e-8);
  EXPECT_NEAR( toDouble(groundPoint.findKeyword("LookDirectionJ2000")[2]), -0.72329561059897, 1e-8);

  EXPECT_NEAR( toDouble(groundPoint.findKeyword("LookDirectionCamera")[0]), -1.27447324380581e-04, 1e-8);
  EXPECT_NEAR( toDouble(groundPoint.findKeyword("LookDirectionCamera")[1]),  2.5816511718707e-05, 1e-8);
  EXPECT_NEAR( toDouble(groundPoint.findKeyword("LookDirectionCamera")[2]),  0.99999999154535, 1e-8);
}

TEST_F(DefaultCube, FunctionalTestCamptSetSL) {
  QVector<QString> args = {"sample=25.0", "line=25.0"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  campt(testCube, options, &appLog);
  PvlGroup groundPoint = appLog.findGroup("GroundPoint");

  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("Sample"), 25.0);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("Line"), 25.0);
}

TEST_F(DefaultCube, FunctionalTestCamptSetS) {
  QVector<QString> args = {"sample=25.0"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  campt(testCube, options, &appLog);
  PvlGroup groundPoint = appLog.findGroup("GroundPoint");

  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("Sample"), 25.0);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("Line"), 528.0);
}

TEST_F(DefaultCube, FunctionalTestCamptSetL) {
  QVector<QString> args = {"line=25.0"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  campt(testCube, options, &appLog);
  PvlGroup groundPoint = appLog.findGroup("GroundPoint");

  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("Sample"), 602.0);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("Line"), 25.0);
}

TEST_F(DefaultCube, FunctionalTestCamptSetGround) {
  QVector<QString> args = {"type=ground", "latitude=10.181441241544", "longitude=255.89292858176"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  campt(testCube, options, &appLog);
  PvlGroup groundPoint = appLog.findGroup("GroundPoint");

  EXPECT_NEAR( (double) groundPoint.findKeyword("Sample"), 602.0, 1e-4);
  EXPECT_NEAR( (double) groundPoint.findKeyword("Line"), 528.0, 1e-4);
}


TEST_F(DefaultCube, FunctionalTestCamptFlat) {
  QFile flatFile(tempDir.path()+"/testOut.txt");
  QVector<QString> args = {"format=flat",
                           "to="+flatFile.fileName(),
                           "append=false"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  campt(testCube, options, &appLog);

  int lineNumber = 0;
  QTextStream flatStream(&flatFile);
  while(!flatStream.atEnd()) {
    QString line = flatStream.readLine();
    QStringList fields = line.split(",");

    if(lineNumber == 0) {
      EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(1), "Sample");
      EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(2), "Line");
    }
    else if(lineNumber == 1) {
      EXPECT_DOUBLE_EQ(fields.value(1).toDouble(), 602.0);
      EXPECT_DOUBLE_EQ(fields.value(2).toDouble(), 528.0);
    }
    lineNumber++;
  }
}

TEST_F(DefaultCube, FunctionalTestCamptCoordList) {
  std::ofstream of;
  of.open(tempDir.path().toStdString()+"/coords.txt");
  of << "1, 10\n10, 100\n 100, 10000";
  of.close();

  QVector<QString> args = {"coordlist="+tempDir.path()+"/coords.txt",
                           "append=false",
                           "coordtype=image"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  campt(testCube, options, &appLog);
  PvlGroup groundPoint = appLog.group(0);

  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("Sample"), 1.0);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("Line"), 10.0);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, groundPoint.findKeyword("Error"), "NULL");

  groundPoint = appLog.group(1);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("Sample"), 10.0);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("Line"), 100.0);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, groundPoint.findKeyword("Error"), "NULL");

  groundPoint = appLog.group(2);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("Sample"), 100.0);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("Line"), 10000.0);
  QString ErrorMsg = "Requested position does not project in camera model; no surface intersection";
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, groundPoint.findKeyword("Error"), ErrorMsg);
}


TEST_F(DefaultCube, FunctionalTestCamptAllowOutside) {
  QVector<QString> args = {"sample=-1", "line=-1", "allowoutside=true"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  campt(testCube, options, &appLog);
  PvlGroup groundPoint = appLog.findGroup("GroundPoint");
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("Sample"), -1.0);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("Line"), -1.0);
}

TEST_F(DefaultCube, FunctionalTestCamptAllowError) {
  QVector<QString> args = {"sample=-100", "line=-100", "allowerror=true"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  campt(testCube, options, &appLog);
  PvlGroup groundPoint = appLog.findGroup("GroundPoint");
  QString ErrorMsg = "Requested position does not project in camera model; no surface intersection";
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, groundPoint.findKeyword("Error"), ErrorMsg);
}

TEST_F(CSMCubeFixture, FunctionalTestCamptCSMCamera) {

  double pointRadius = csm::Ellipsoid().getSemiMajorRadius();
  // We don't care exactly how the Mock is called so just setup return values
  EXPECT_CALL(mockModel, getSensorIdentifier)
      .WillRepeatedly(::testing::Return("MockSensorID"));
  EXPECT_CALL(mockModel, getPlatformIdentifier)
      .WillRepeatedly(::testing::Return("MockPlatformID"));
  EXPECT_CALL(mockModel, getReferenceDateAndTime)
      .WillRepeatedly(::testing::Return("20000101T115855.816"));
  EXPECT_CALL(mockModel, imageToRemoteImagingLocus)
      .WillRepeatedly(::testing::Return(csm::EcefLocus(pointRadius + 50000, 0, 0, -1, 0, 0)));
  EXPECT_CALL(mockModel, groundToImage(::testing::An<const csm::EcefCoord&>(), ::testing::_, ::testing::_, ::testing::_))
      .WillRepeatedly(::testing::Return(csm::ImageCoord(4.5, 4.5)));
  EXPECT_CALL(mockModel, computeGroundPartials)
      .WillRepeatedly(::testing::Return(std::vector<double>{1, 2, 3, 4, 5, 6}));
  EXPECT_CALL(mockModel, getImageTime)
      .WillRepeatedly(::testing::Return(10.0));
  EXPECT_CALL(mockModel, computeGroundPartials)
      .WillRepeatedly(::testing::Return(std::vector<double>{1, 2, 3, 4, 5, 6}));
  EXPECT_CALL(mockModel, getSensorPosition(::testing::An<const csm::ImageCoord&>()))
      .WillRepeatedly(::testing::Return(csm::EcefCoord(pointRadius + 50000, 0, 0)));
  EXPECT_CALL(mockModel, getIlluminationDirection)
      .WillRepeatedly(::testing::Return(csm::EcefVector(0.0, 0.0, -1.0)));

  QVector<QString> args = {"sample=5", "line=5"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  campt(testCube, options, &appLog);
  PvlGroup groundPoint = appLog.findGroup("GroundPoint");

  // Check that invalid values are all set to null
  EXPECT_TRUE(groundPoint.findKeyword("SunPosition").isNull(0));
  EXPECT_TRUE(groundPoint.findKeyword("SunPosition").isNull(1));
  EXPECT_TRUE(groundPoint.findKeyword("SunPosition").isNull(2));
  EXPECT_TRUE(groundPoint.findKeyword("SubSolarAzimuth").isNull());
  EXPECT_TRUE(groundPoint.findKeyword("SolarDistance").isNull());
  EXPECT_TRUE(groundPoint.findKeyword("SubSolarLatitude").isNull());
  EXPECT_TRUE(groundPoint.findKeyword("SubSolarLongitude").isNull());
  EXPECT_TRUE(groundPoint.findKeyword("SubSolarGroundAzimuth").isNull());
  EXPECT_TRUE(groundPoint.findKeyword("LocalSolarTime").isNull());
  EXPECT_TRUE(groundPoint.findKeyword("SolarLongitude").isNull());
  EXPECT_TRUE(groundPoint.findKeyword("LookDirectionJ2000").isNull(0));
  EXPECT_TRUE(groundPoint.findKeyword("LookDirectionJ2000").isNull(1));
  EXPECT_TRUE(groundPoint.findKeyword("LookDirectionJ2000").isNull(2));
  EXPECT_TRUE(groundPoint.findKeyword("LookDirectionCamera").isNull(0));
  EXPECT_TRUE(groundPoint.findKeyword("LookDirectionCamera").isNull(1));
  EXPECT_TRUE(groundPoint.findKeyword("LookDirectionCamera").isNull(2));
}
