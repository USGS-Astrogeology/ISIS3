#include <QTextStream>
#include <QStringList>
#include <QTemporaryFile>

#include "findimageoverlaps.h"
#include "overlapstats.h"
#include "ImagePolygon.h"
#include "NetworkFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"

#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/overlapstats.xml").expanded());

TEST_F(ThreeImageNetwork, FunctionalTestOverlapstatsBadCubeList) {
  QString badCubeList = tempDir.path() + "/badcubes.lis";
  QString overlapsPath = tempDir.path() + "/overlaps.lis";
  FileList cubes;

  // badCubeList only contains cube1, overlaps has cube1 and cube2
  cubes.append(FileName(cube1->fileName().toStdString()));
  cubes.write(badCubeList.toStdString());
  cubes.append(FileName(cube2->fileName().toStdString()));
  cube1->close();
  cube2->close();

  QString FIO_APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/findimageoverlaps.xml").expanded());
  QVector<QString> fio_args = { "OVERLAPLIST=" + overlapsPath };
  UserInterface fioOptions(FIO_APP_XML, fio_args);
  findimageoverlaps(cubes, fioOptions, false, nullptr);

  QVector<QString> args = { "FROMLIST=" + badCubeList,
                            "OVERLAPLIST=" + overlapsPath };
  UserInterface options(APP_XML, args);
  Pvl appLog;

  try {
    overlapstats(options, &appLog);
    FAIL() << "Expected Exception for invalid overlap list";
  }
  catch(IException &e) {
    std::string errorMsg = "in overlap list that was not in the provided cube list";
    EXPECT_TRUE(e.toString().find(errorMsg.c_str()) != std::string::npos)
                                              <<  e.toString();
  }
}


TEST_F(ThreeImageNetwork, FunctionalTestOverlapstatsDefault) {
  // fill points so cube3 contributes to overlaps
  ImagePolygon poly;
  coords = {{30, 0},
            {30, 10},
            {35, 10},
            {35, 0},
            {30, 0}};
  poly.Create(coords);
  cube3->write(poly);
  cube3->reopen("rw");

  FileList cubes;
  cubes.append(FileName(cube1->fileName().toStdString()));
  cubes.append(FileName(cube2->fileName().toStdString()));
  cubes.append(FileName(cube3->fileName().toStdString()));
  cube1->close();
  cube2->close();
  cube3->close();

  QString cubeListPath = tempDir.path() + "/cubes.lis";
  QString overlapsPath = tempDir.path() + "/overlaps.lis";
  cubes.write(cubeListPath.toStdString());

  // get overlaps from findimageoverlaps
  QString FIO_APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/findimageoverlaps.xml").expanded());
  QVector<QString> fio_args = { "OVERLAPLIST=" + overlapsPath };
  UserInterface fioOptions(FIO_APP_XML, fio_args);
  findimageoverlaps(cubes, fioOptions);

  QVector<QString> args = { "FROMLIST=" + cubeListPath,
                            "OVERLAPLIST=" + overlapsPath };
  UserInterface options(APP_XML, args);
  Pvl appLog;
  overlapstats(options, &appLog);

  PvlGroup group = appLog.findGroup("Results");
  EXPECT_EQ( (double) group.findKeyword("ThicknessMinimum"), 0.14135606732925);
  EXPECT_EQ( (double) group.findKeyword("ThicknessMaximum"), 0.44103454403747);
  EXPECT_EQ( (double) group.findKeyword("ThicknessAverage"), 0.29119530568336);
  EXPECT_EQ( (double) group.findKeyword("ThicknessStandardDeviation"), 0.21190468305603);
  EXPECT_EQ( (double) group.findKeyword("ThicknessVariance"), 0.044903594701078);
  EXPECT_EQ( (double) group.findKeyword("AreaMinimum"), 49665309599.111);
  EXPECT_EQ( (double) group.findKeyword("AreaMaximum"), 125515198928.3);
  EXPECT_EQ( (double) group.findKeyword("AreaAverage"), 87590254263.708);
  EXPECT_EQ( (double) group.findKeyword("AreaStandardDeviation"), 53633971096.922);
  EXPECT_EQ( (double) group.findKeyword("AreaVariance"), 2.87660285562549e+21);
  EXPECT_EQ( (double) group.findKeyword("ImageStackMinimum"), 2.0);
  EXPECT_EQ( (double) group.findKeyword("ImageStackMaximum"), 3.0);
  EXPECT_EQ( (double) group.findKeyword("ImageStackAverage"), 2.5);
  EXPECT_EQ( (double) group.findKeyword("ImageStackStandardDeviation"), 0.70710678118655);
  EXPECT_EQ( (double) group.findKeyword("ImageStackVariance"), 0.5);
  EXPECT_EQ( (int) group.findKeyword("PolygonCount"), 3);
}


TEST_F(ThreeImageNetwork, FunctionalTestOverlapstatsFull) {
  // write points so that cube2 is a full overlap of cube1
  ImagePolygon poly;
  coords = {{31, 1},
            {31, 9},
            {34, 9},
            {34, 1},
            {31, 1}};
  poly.Create(coords);
  cube2->write(poly);
  cube2->reopen("rw");

  FileList cubes;
  cubes.append(FileName(cube1->fileName().toStdString()));
  cubes.append(FileName(cube2->fileName().toStdString()));
  cube1->close();
  cube2->close();

  QString cubeListPath = tempDir.path() + "/cubes.lis";
  QString overlapsPath = tempDir.path() + "/overlaps.lis";
  cubes.write(cubeListPath.toStdString());

  // get overlaps for overlapstats input
  QString FIO_APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/findimageoverlaps.xml").expanded());
  QVector<QString> fio_args = { "OVERLAPLIST=" + overlapsPath };
  UserInterface fioOptions(FIO_APP_XML, fio_args);
  findimageoverlaps(cubes, fioOptions);

  QVector<QString> args = { "FROMLIST=" + cubeListPath,
                            "OVERLAPLIST=" + overlapsPath };
  UserInterface options(APP_XML, args);
  Pvl appLog;
  overlapstats(options, &appLog);

  PvlGroup group = appLog.findGroup("Results");
  EXPECT_EQ( (double) group.findKeyword("ThicknessMinimum"), 0.37266300670341);
  EXPECT_EQ( (double) group.findKeyword("ThicknessMaximum"), 0.37266300670341);
  EXPECT_EQ( (double) group.findKeyword("ThicknessAverage"), 0.37266300670341);
  EXPECT_EQ( (double) group.findKeyword("ThicknessStandardDeviation"), -1.79769313486231e+308);
  EXPECT_EQ( (double) group.findKeyword("ThicknessVariance"), -1.79769313486231e+308);
  EXPECT_EQ( (double) group.findKeyword("AreaMinimum"), 83798250265.466);
  EXPECT_EQ( (double) group.findKeyword("AreaMaximum"), 83798250265.466);
  EXPECT_EQ( (double) group.findKeyword("AreaAverage"), 83798250265.466);
  EXPECT_EQ( (double) group.findKeyword("AreaStandardDeviation"), -1.79769313486231e+308);
  EXPECT_EQ( (double) group.findKeyword("AreaVariance"), -1.79769313486231e+308);
  EXPECT_EQ( (double) group.findKeyword("ImageStackMinimum"), 2.0);
  EXPECT_EQ( (double) group.findKeyword("ImageStackMaximum"), 2.0);
  EXPECT_EQ( (double) group.findKeyword("ImageStackAverage"), 2.0);
  EXPECT_EQ( (double) group.findKeyword("ImageStackStandardDeviation"), -1.79769313486231e+308);
  EXPECT_EQ( (double) group.findKeyword("ImageStackVariance"), -1.79769313486231e+308);
  EXPECT_EQ( (int) group.findKeyword("PolygonCount"), 2);
}


TEST_F(ThreeImageNetwork, FunctionalTestOverlapstatsNoOverlap) {
// create footprint for cube3 that has no overlap with cubes 1 and 2
  ImagePolygon poly;
  coords = {{50, 50},
            {50, 40},
            {40, 40},
            {40, 50},
            {50, 50}};
  poly.Create(coords);
  cube3->write(poly);
  cube3->reopen("rw");

  FileList cubes;
  cubes.append(FileName(cube1->fileName().toStdString()));
  cubes.append(FileName(cube2->fileName().toStdString()));
  cubes.append(FileName(cube3->fileName().toStdString()));
  cube1->close();
  cube2->close();
  cube3->close();

  QString cubeListPath = tempDir.path() + "/cubes.lis";
  QString overlapsPath = tempDir.path() + "/overlaps.lis";
  cubes.write(cubeListPath.toStdString());

  // get overlaps for overlapstats input
  QString FIO_APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/findimageoverlaps.xml").expanded());
  QVector<QString> fio_args = { "OVERLAPLIST=" + overlapsPath };
  UserInterface fioOptions(FIO_APP_XML, fio_args);
  findimageoverlaps(cubes, fioOptions);

  QVector<QString> args = { "FROMLIST=" + cubeListPath,
                            "OVERLAPLIST=" + overlapsPath };
  UserInterface options(APP_XML, args);
  Pvl appLog;
  overlapstats(options, &appLog);

  PvlGroup group = appLog.findGroup("Results");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, group.findKeyword("NoOverlap"), tempDir.path().toStdString() + "/cube3.cub");
}
