#include <QTextStream>
#include <QStringList>
#include <QFile>
#include <QDebug>

#include "mappt.h"
#include "CameraFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "SpecialPixel.h"

#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/mappt.xml").expanded();

TEST_F(DefaultCube, FunctionalTestMapptImageTest) {
  QVector<QString> args = {"append=false", "type=image", "sample=1", "line=1"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  mappt(projTestCube, options, &appLog);
  PvlGroup mapPoint = appLog.findGroup("Results");

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapPoint.findKeyword("FileName"), projTestCube->fileName().toStdString());
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapPoint.findKeyword("FilterName"), "CLEAR");
  EXPECT_EQ( (double) mapPoint.findKeyword("Band"), 1);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PlanetographicLatitude"), 9.3870849567571);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PlanetocentricLatitude"), 9.2788326719634);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveWest360Longitude"), 359.14528612684);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveEast360Longitude"), 0.85471387315749);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveEast180Longitude"), 0.85471387315749);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveWest180Longitude"), -0.85471387315751);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("x"), 50000);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("y"), 550000);
}

TEST_F(DefaultCube, FunctionalTestMapptGroundTest) {
  QVector<QString> args = {"append=false", "type=ground", "latitude=9.2788326719634", "longitude=0.85471387315749"};
  UserInterface options(APP_XML, args);
  Pvl appLog;
  mappt(projTestCube, options, &appLog);

  PvlGroup mapPoint = appLog.findGroup("Results");

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapPoint.findKeyword("FileName"), projTestCube->fileName().toStdString());
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapPoint.findKeyword("FilterName"), "CLEAR");
  EXPECT_EQ( (double) mapPoint.findKeyword("Band"), 1);
  EXPECT_NEAR( (double) mapPoint.findKeyword("Sample"), 1, 1e-8);
  EXPECT_NEAR( (double) mapPoint.findKeyword("Line"), 1, 1e-8);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PlanetographicLatitude"), 9.3870849567571);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PlanetocentricLatitude"), 9.2788326719634);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveWest360Longitude"), 359.14528612684);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveEast360Longitude"), 0.85471387315749);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveEast180Longitude"), 0.85471387315749);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveWest180Longitude"), -0.85471387315751);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("x"), 50000);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("y"), 550000);
}

TEST_F(DefaultCube, FunctionalTestMapptProjectionTest) {
  QVector<QString> args = {"append=false", "type=projection", "x=50000.0", "y=550000.0"};
  UserInterface options(APP_XML, args);
  Pvl appLog;
  mappt(projTestCube, options, &appLog);

  PvlGroup mapPoint = appLog.findGroup("Results");

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapPoint.findKeyword("FileName"), projTestCube->fileName().toStdString());
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapPoint.findKeyword("FilterName"), "CLEAR");
  EXPECT_EQ( (double) mapPoint.findKeyword("Band"), 1);
  EXPECT_NEAR( (double) mapPoint.findKeyword("Sample"), 1, 1e-8);
  EXPECT_NEAR( (double) mapPoint.findKeyword("Line"), 1, 1e-8);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PlanetographicLatitude"), 9.3870849567571);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PlanetocentricLatitude"), 9.2788326719634);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveWest360Longitude"), 359.14528612684);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveEast360Longitude"), 0.85471387315749);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveEast180Longitude"), 0.85471387315749);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveWest180Longitude"), -0.85471387315751);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("x"), 50000);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("y"), 550000);
}

TEST_F(DefaultCube, FunctionalTestMapptCoordsysTest) {
  QVector<QString> args = {"append=false",
                           "coordsys=userdefined",
                           "type=ground",
                           "lattype=planetographic"
                           "londir=positivewest",
                           "londom=180",
                           "latitude=9.3870849567571",
                           "longitude=0.85471387315749"};

  UserInterface options(APP_XML, args);
  Pvl appLog;
  mappt(projTestCube, options, &appLog);

  PvlGroup mapPoint = appLog.findGroup("Results");

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapPoint.findKeyword("FileName"), projTestCube->fileName().toStdString());
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapPoint.findKeyword("FilterName"), "CLEAR");
  EXPECT_EQ( (double) mapPoint.findKeyword("Band"), 1);
  EXPECT_NEAR( (double) mapPoint.findKeyword("Sample"), 1, 1e-8);
  EXPECT_NEAR( (double) mapPoint.findKeyword("Line"), 1, 1e-8);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PlanetographicLatitude"), 9.3870849567571);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PlanetocentricLatitude"), 9.2788326719634);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveWest360Longitude"), 359.14528612684);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveEast360Longitude"), 0.85471387315749);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveEast180Longitude"), 0.85471387315749);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveWest180Longitude"), -0.85471387315751);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("x"), 50000);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("y"), 550000);

}

TEST_F(DefaultCube, FunctionalTestMapptFlatFileTest) {
  QFile flatFile(tempDir.path() + "/testOut.txt");
  QVector<QString> args = {"to="+flatFile.fileName(), "append=false", "type=projection", "x=50000.0", "y=550000.0", "format=flat"};
  UserInterface options(APP_XML, args);
  Pvl appLog;
  mappt(projTestCube, options, &appLog);

  PvlGroup mapPoint = appLog.findGroup("Results");

  int lineNumber = 0;
  QTextStream flatStream(&flatFile);

  if (flatFile.open(QIODevice::ReadOnly)) {
    while(!flatStream.atEnd()) {
      QString line = flatStream.readLine();
      QStringList fields = line.split(",");

      if(lineNumber == 0) {
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(0), "Filename");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(1), "Sample");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(2), "Line");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(3), "Band");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(4), "FilterName");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(5), "PixelValue");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(6), "X");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(7), "Y");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(8), "PlanetocentricLatitude");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(9), "PlanetographicLatitude");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(10), "PositiveEast360Longitude");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(11), "PositiveEast180Longitude");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(12), "PositiveWest360Longitude");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(13), "PositiveWest180Longitude");
      }
      else if(lineNumber == 1) {
        EXPECT_PRED_FORMAT2(AssertStringsEqual, fields.value(0).toStdString(), mapPoint.findKeyword("FileName"));
        EXPECT_DOUBLE_EQ(fields.value(1).toDouble(), mapPoint.findKeyword("Sample"));
        EXPECT_DOUBLE_EQ(fields.value(2).toDouble(), mapPoint.findKeyword("Line"));
        EXPECT_DOUBLE_EQ(fields.value(3).toDouble(), mapPoint.findKeyword("Band"));
        EXPECT_PRED_FORMAT2(AssertStringsEqual, fields.value(4).toStdString(), mapPoint.findKeyword("FilterName"));
        EXPECT_PRED_FORMAT2(AssertStringsEqual, fields.value(5).toStdString(), mapPoint.findKeyword("PixelValue"));
        EXPECT_DOUBLE_EQ(fields.value(6).toDouble(), mapPoint.findKeyword("X"));
        EXPECT_DOUBLE_EQ(fields.value(7).toDouble(), mapPoint.findKeyword("Y"));
        EXPECT_DOUBLE_EQ(fields.value(8).toDouble(), mapPoint.findKeyword("PlanetocentricLatitude"));
        EXPECT_DOUBLE_EQ(fields.value(9).toDouble(), mapPoint.findKeyword("PlanetographicLatitude") );
        EXPECT_DOUBLE_EQ(fields.value(10).toDouble(), mapPoint.findKeyword("PositiveEast360Longitude"));
        EXPECT_DOUBLE_EQ(fields.value(11).toDouble(), mapPoint.findKeyword("PositiveEast180Longitude"));
        EXPECT_DOUBLE_EQ(fields.value(12).toDouble(), mapPoint.findKeyword("PositiveWest360Longitude"));
        EXPECT_DOUBLE_EQ(fields.value(13).toDouble(), mapPoint.findKeyword("PositiveWest180Longitude"));
      }
      lineNumber++;
    }
  }
  else {
    FAIL() << "FAILED TO OPEN FLATFILE";
  }
}

TEST_F(DefaultCube, FunctionalTestMapptAllowOutside) {
  QVector<QString> args = {"type=image", "sample=-1", "line=-1", "allowoutside=true"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  mappt(projTestCube, options, &appLog);
  PvlGroup groundPoint = appLog.findGroup("Results");
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("Sample"), -1.0);
  EXPECT_DOUBLE_EQ( (double) groundPoint.findKeyword("Line"), -1.0);

  args = {"type=image", "sample=-1", "line=-1", "allowoutside=false"};
  mappt(projTestCube, options, &appLog);

  PvlGroup mapPoint = appLog.findGroup("Results");

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapPoint.findKeyword("OutsideOfImage"), "Requested point falls outside of image boundaries");
}

TEST_F(DefaultCube, FunctionalTestMapptBandTest) {
  QVector<QString> args = { "from="+projTestCube->fileName()+"+2", "append=false", "type=image", "sample=1", "line=1"};
  UserInterface options(APP_XML, args);
  Pvl appLog;
  mappt(options, &appLog);
  PvlGroup mapPoint = appLog.findGroup("Results");

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapPoint.findKeyword("FilterName"), "NIR");
  EXPECT_EQ( (double) mapPoint.findKeyword("Band"), 2);
}


TEST_F(DefaultCube, FunctionalTestMapptImageCoordList) {
  std::ofstream of;
  of.open(tempDir.path().toStdString()+"/coords.txt");
  of << "1, 1\n2, 2\n 3, 3";
  of.close();

  QVector<QString> args = {"coordlist="+tempDir.path()+"/coords.txt",
                           "UseCoordList=True",
                           "append=false",
                           "type=image"};
  UserInterface options(APP_XML, args);

  Pvl appLog;
  mappt(projTestCube, options, &appLog);

  PvlGroup mapPoint = appLog.group(0);

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapPoint.findKeyword("FileName"), projTestCube->fileName().toStdString());
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapPoint.findKeyword("FilterName"), "CLEAR");
  EXPECT_EQ( (double) mapPoint.findKeyword("Band"), 1);
  EXPECT_NEAR( (double) mapPoint.findKeyword("Sample"), 1, 1e-8);
  EXPECT_NEAR( (double) mapPoint.findKeyword("Line"), 1, 1e-8);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PlanetographicLatitude"), 9.3870849567571);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PlanetocentricLatitude"), 9.2788326719634);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveWest360Longitude"), 359.14528612684);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveEast360Longitude"), 0.85471387315749);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveEast180Longitude"), 0.85471387315749);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveWest180Longitude"), -0.85471387315751);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("x"), 50000);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("y"), 550000);

  mapPoint = appLog.group(1);

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapPoint.findKeyword("FileName"), projTestCube->fileName().toStdString());
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapPoint.findKeyword("FilterName"), "CLEAR");
  EXPECT_EQ( (double) mapPoint.findKeyword("Band"), 1);
  EXPECT_NEAR( (double) mapPoint.findKeyword("Sample"), 2, 1e-8);
  EXPECT_NEAR( (double) mapPoint.findKeyword("Line"), 2, 1e-8);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PlanetographicLatitude"), 7.6808677548562);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PlanetocentricLatitude"), 7.5917721861518);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveWest360Longitude"), 357.44703128109);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveEast360Longitude"), 2.5529687189083);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveEast180Longitude"), 2.5529687189083);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveWest180Longitude"), -2.5529687189083);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("x"), 150000);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("y"), 450000);

  mapPoint = appLog.group(2);

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapPoint.findKeyword("FileName"), projTestCube->fileName().toStdString());
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapPoint.findKeyword("FilterName"), "CLEAR");
  EXPECT_EQ( (double) mapPoint.findKeyword("Band"), 1);
  EXPECT_NEAR( (double) mapPoint.findKeyword("Sample"), 3, 1e-8);
  EXPECT_NEAR( (double) mapPoint.findKeyword("Line"), 3, 1e-8);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PlanetographicLatitude"), 5.9743363392284);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PlanetocentricLatitude"), 5.9047117003403);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveWest360Longitude"), 355.75985208984);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveEast360Longitude"), 4.2401479101647);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveEast180Longitude"), 4.2401479101647);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("PositiveWest180Longitude"), -4.2401479101646);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("x"), 250000);
  EXPECT_DOUBLE_EQ( (double) mapPoint.findKeyword("y"), 350000.0);

}

TEST_F(DefaultCube, FunctionalTestMapptCoordListFlatFile) {
  std::ofstream of;
  of.open(tempDir.path().toStdString()+"/coords.txt");
  of << "1, 1\n2, 2\n 3, 3";
  of.close();

  QFile flatFile(tempDir.path() + "/testOut.txt");
  QVector<QString> args = {"coordlist="+tempDir.path()+"/coords.txt","to=" + flatFile.fileName(),
                           "UseCoordList=True",
                           "append=false", "format=flat",
                           "type=image"};
  UserInterface options(APP_XML, args);

  Pvl appLog;
  mappt(projTestCube, options, &appLog);

  int lineNumber = 0;
  QTextStream flatStream(&flatFile);

  PvlGroup mapPoint = appLog.group(0);

  if (flatFile.open(QIODevice::ReadOnly)) {
    while(!flatStream.atEnd()) {
      QString line = flatStream.readLine();
      QStringList fields = line.split(",");

      if(lineNumber == 0) {
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(0), "Filename");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(1), "Sample");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(2), "Line");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(3), "Band");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(4), "FilterName");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(5), "PixelValue");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(6), "X");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(7), "Y");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(8), "PlanetocentricLatitude");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(9), "PlanetographicLatitude");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(10), "PositiveEast360Longitude");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(11), "PositiveEast180Longitude");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(12), "PositiveWest360Longitude");
        EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(13), "PositiveWest180Longitude");
      }
      else {
        mapPoint = appLog.group(lineNumber-1);

        EXPECT_PRED_FORMAT2(AssertStringsEqual, fields.value(0).toStdString(), mapPoint.findKeyword("FileName"));
        EXPECT_DOUBLE_EQ(fields.value(1).toDouble(), mapPoint.findKeyword("Sample"));
        EXPECT_DOUBLE_EQ(fields.value(2).toDouble(), mapPoint.findKeyword("Line"));
        EXPECT_DOUBLE_EQ(fields.value(3).toDouble(), mapPoint.findKeyword("Band"));
        EXPECT_PRED_FORMAT2(AssertStringsEqual, fields.value(4).toStdString(), mapPoint.findKeyword("FilterName"));
        EXPECT_PRED_FORMAT2(AssertStringsEqual, fields.value(5).toStdString(), mapPoint.findKeyword("PixelValue"));
        EXPECT_DOUBLE_EQ(fields.value(6).toDouble(), mapPoint.findKeyword("X"));
        EXPECT_DOUBLE_EQ(fields.value(7).toDouble(), mapPoint.findKeyword("Y"));
        EXPECT_DOUBLE_EQ(fields.value(8).toDouble(), mapPoint.findKeyword("PlanetocentricLatitude"));
        EXPECT_DOUBLE_EQ(fields.value(9).toDouble(), mapPoint.findKeyword("PlanetographicLatitude") );
        EXPECT_DOUBLE_EQ(fields.value(10).toDouble(), mapPoint.findKeyword("PositiveEast360Longitude"));
        EXPECT_DOUBLE_EQ(fields.value(11).toDouble(), mapPoint.findKeyword("PositiveEast180Longitude"));
        EXPECT_DOUBLE_EQ(fields.value(12).toDouble(), mapPoint.findKeyword("PositiveWest360Longitude"));
        EXPECT_DOUBLE_EQ(fields.value(13).toDouble(), mapPoint.findKeyword("PositiveWest180Longitude"));
      }

      lineNumber++;
    }
  }
  else {
    FAIL() << "FAILED TO OPEN FLATFILE";
  }
}

TEST_F(DefaultCube, FunctionalTestMapptBadColumnError) {
  std::ofstream of;
  of.open(tempDir.path().toStdString()+"/coords.txt");
  of << "1, 1\n2\n 3, 3";
  of.close();

  QVector<QString> args = {"coordlist="+tempDir.path()+"/coords.txt",
                           "UseCoordList=True",
                           "append=false",
                           "type=image"};

  UserInterface options(APP_XML, args);
  Pvl appLog;
  try {
    mappt(projTestCube, options, &appLog);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Coordinate file formatted incorrectly."))
      << e.toString().toStdString();
  }
  catch(...) {
    FAIL() << "Expected an IException with message: \"Coordinate file formatted incorrectly.\n"
              "Each row must have two columns: a sample,line or a latitude,longitude pair.\"";
  }

}
