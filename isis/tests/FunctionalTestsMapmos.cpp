#include <QTextStream>
#include <QStringList>
#include <QTemporaryFile>

#include <typeinfo>

#include "mapmos.h"
#include "NetworkFixtures.h"
#include "Pvl.h"
#include "Histogram.h"
#include "TestUtilities.h"
#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/mapmos.xml").expanded();

TEST_F(ThreeImageNetwork, FunctionalTestMapmosOntop) {
  QTemporaryDir prefix;
  QString mosPath = prefix.path() + "/mosaic.cub";
  QVector<QString> args = { "MOSAIC=" + mosPath,
                            "track=false",
                            "priority=ONTOP" };
  QVector<QString> *cube1args = new QVector<QString>(args);
  QVector<QString> *cube2args = new QVector<QString>(args);
  QVector<QString> *cube3args = new QVector<QString>(args);

  cube1args->append("FROM=" + cube1map->fileName());
  cube1args->append("create=true");
  cube2args->append("FROM=" + cube2map->fileName());
  cube2args->append("create=false");
  cube3args->append("FROM=" + cube3map->fileName());
  cube3args->append("create=false");

  UserInterface cube1options(APP_XML, *cube1args);
  UserInterface cube2options(APP_XML, *cube2args);
  UserInterface cube3options(APP_XML, *cube3args);
  Pvl appLog;

  try {
    mapmos(cube1options, &appLog);
    mapmos(cube2options, &appLog);
    mapmos(cube3options, &appLog);
  }
  catch (IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  Cube mosaic(mosPath);

  PvlObject mosaicLabel = mosaic.label()->findObject("IsisCube");
  PvlGroup dimensions = mosaicLabel.findObject("Core").findGroup("Dimensions");
  PvlGroup pixels = mosaicLabel.findObject("Core").findGroup("Pixels");
  PvlGroup mapping = mosaicLabel.findGroup("Mapping");

  EXPECT_EQ(int(dimensions["Samples"]), 552);
  EXPECT_EQ(int(dimensions["Lines"]), 677);
  EXPECT_EQ(int(dimensions["Bands"]), 1);

  EXPECT_EQ(pixels["Type"][0], "Real");
  EXPECT_EQ(pixels["ByteOrder"][0], "Lsb");
  EXPECT_EQ(double(pixels["Base"]), 0.0);
  EXPECT_EQ(double(pixels["Multiplier"]), 1.0);

  EXPECT_EQ(double(mapping["MinimumLatitude"]), 0.47920860194551);
  EXPECT_EQ(double(mapping["MaximumLatitude"]), 3.3932951263901);
  EXPECT_EQ(double(mapping["MinimumLongitude"]), -0.94830771139743);
  EXPECT_EQ(double(mapping["MaximumLongitude"]), 1.4318179715731);
}

TEST_F(ThreeImageNetwork, FunctionalTestMapmosBeneath) {
  QTemporaryDir prefix;
  QString mosPath = prefix.path() + "/mosaic.cub";
  QVector<QString> args = { "MOSAIC=" + mosPath,
                            "track=false",
                            "priority=BENEATH" };
  QVector<QString> *cube1args = new QVector<QString>(args);
  QVector<QString> *cube2args = new QVector<QString>(args);
  QVector<QString> *cube3args = new QVector<QString>(args);
  cube1args->append("FROM=" + cube1map->fileName());
  cube1args->append("create=true");
  cube2args->append("FROM=" + cube2map->fileName());
  cube2args->append("create=false");
  cube3args->append("FROM=" + cube3map->fileName());
  cube3args->append("create=false");

  UserInterface cube1options(APP_XML, *cube1args);
  UserInterface cube2options(APP_XML, *cube2args);
  UserInterface cube3options(APP_XML, *cube3args);
  Pvl appLog;

  try {
    mapmos(cube1options, &appLog);
    mapmos(cube2options, &appLog);
    mapmos(cube3options, &appLog);
  }
  catch (IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  Cube mosaic(mosPath);
  PvlObject mosaicLabel = mosaic.label()->findObject("IsisCube");
  PvlGroup dimensions = mosaicLabel.findObject("Core").findGroup("Dimensions");

  EXPECT_EQ(int(dimensions["Samples"]), 552);
  EXPECT_EQ(int(dimensions["Lines"]), 677);
  EXPECT_EQ(int(dimensions["Bands"]), 1);
}

TEST_F(ThreeImageNetwork, FunctionalTestMapmosAverage) {
  QTemporaryDir prefix;
  QString mosPath = prefix.path() + "/mosaic.cub";
  QVector<QString> args = { "MOSAIC=" + mosPath,
                            "track=false",
                            "priority=AVERAGE" };
  QVector<QString> *cube1args = new QVector<QString>(args);
  QVector<QString> *cube2args = new QVector<QString>(args);
  QVector<QString> *cube3args = new QVector<QString>(args);
  cube1args->append("FROM=" + cube1map->fileName());
  cube1args->append("create=true");
  cube2args->append("FROM=" + cube2map->fileName());
  cube2args->append("create=false");
  cube3args->append("FROM=" + cube3map->fileName());
  cube3args->append("create=false");

  UserInterface cube1options(APP_XML, *cube1args);
  UserInterface cube2options(APP_XML, *cube2args);
  UserInterface cube3options(APP_XML, *cube3args);
  Pvl appLog;

  try {
    mapmos(cube1options, &appLog);
    mapmos(cube2options, &appLog);
    mapmos(cube3options, &appLog);
  }
  catch (IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  Cube mosaic(mosPath);
  PvlObject mosaicLabel = mosaic.label()->findObject("IsisCube");
  PvlGroup dimensions = mosaicLabel.findObject("Core").findGroup("Dimensions");

  EXPECT_EQ(int(dimensions["Samples"]), 552);
  EXPECT_EQ(int(dimensions["Lines"]), 677);
  EXPECT_EQ(int(dimensions["Bands"]), 2);
}



TEST_F(ThreeImageNetwork, FunctionalTestMapmos720deg) {
  QTemporaryDir prefix;
  QString mosPath = prefix.path() + "/mosaic.cub";
  QVector<QString> args = { "MOSAIC=" + mosPath,
                            "create=true",
                            "track=false",
                            "priority=ONTOP",
                            "minlat=0",
                            "maxlat=5",
                            "minlon=0",
                            "maxlon=720" };
  UserInterface options(APP_XML, args);
  Pvl appLog;

  try {
    mapmos(cube1map, options, &appLog);
  }
  catch (IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  Cube mosaic(mosPath);
  PvlObject mosaicLabel = mosaic.label()->findObject("IsisCube");
  PvlGroup mapping = mosaicLabel.findGroup("Mapping");

  EXPECT_EQ(double(mapping["MinimumLatitude"]), 0);
  EXPECT_EQ(double(mapping["MaximumLatitude"]), 5);
  EXPECT_EQ(double(mapping["MinimumLongitude"]), 0);
  EXPECT_EQ(double(mapping["MaximumLongitude"]), 720);
}

TEST_F(ThreeImageNetwork, FunctionalTestMapmosExtents) {
  QTemporaryDir prefix;
  QString mosPath = prefix.path() + "/mosaic.cub";
  QVector<QString> args = { "MOSAIC=" + mosPath,
                            "create=true",
                            "track=false",
                            "priority=ONTOP",
                            "lowsaturation=true",
                            "highsaturation=true",
                            "null=true" };
  UserInterface options(APP_XML, args);
  Pvl appLog;

  try {
    mapmos(cube1map, options, &appLog);
  }
  catch (IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  Cube mosaic(mosPath);
  PvlObject mosaicLabel = mosaic.label()->findObject("IsisCube");
  PvlGroup dimensions = mosaicLabel.findObject("Core").findGroup("Dimensions");

  EXPECT_EQ(int(dimensions["Samples"]), 552);
  EXPECT_EQ(int(dimensions["Lines"]), 677);
  EXPECT_EQ(int(dimensions["Bands"]), 1);
}

TEST_F(ThreeImageNetwork, FunctionalTestMapmosTracking) {
  QTemporaryDir prefix;
  QString mosPath = prefix.path() + "/mosaic.cub";
  QVector<QString> args = { "MOSAIC=" + mosPath,
                            "create=true",
                            "track=true",
                            "priority=ONTOP" };
  UserInterface options(APP_XML, args);
  Pvl appLog;

  try {
    mapmos(cube1map, options, &appLog);
  }
  catch (IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  Cube mosaic(mosPath);
  PvlObject mosaicLabel = mosaic.label()->findObject("IsisCube");
  PvlGroup bandbin = mosaicLabel.findGroup("BandBin");
  PvlGroup tracking = mosaicLabel.findGroup("Tracking");
  QString trackPath = prefix.path() + "/" + QString::fromStdString(tracking["FileName"][0]);

  try {
    Cube trackCube(trackPath);
    PvlObject trackLabel = trackCube.label()->findObject("IsisCube");
    PvlGroup dimensions = trackLabel.findObject("Core").findGroup("Dimensions");

    EXPECT_EQ(int(dimensions["Samples"]), 552);
    EXPECT_EQ(int(dimensions["Lines"]), 677);
    EXPECT_EQ(int(dimensions["Bands"]), 1);
  }
  catch (IException &e) {
    FAIL() << "Invalid FileName for tracking cube: " << e.toString().toStdString().c_str() << std::endl;
  }
}

TEST_F(ThreeImageNetwork, FunctionalTestMapmosMatchBandBin) {
  QTemporaryDir prefix;
  QString mosPath = prefix.path() + "/mosaic.cub";
  QVector<QString> args = { "MOSAIC=" + mosPath,
                            "create=true",
                            "track=false",
                            "priority=ontop",
                            "matchbandbin=true" };
  UserInterface options(APP_XML, args);
  Pvl appLog;

  try {
    mapmos(cube1map, options, &appLog);
  }
  catch (IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  Cube mosaic(mosPath);
  PvlObject mosaicLabel = mosaic.label()->findObject("IsisCube");
  PvlGroup dimensions = mosaicLabel.findObject("Core").findGroup("Dimensions");

  EXPECT_EQ(int(dimensions["Samples"]), 552);
  EXPECT_EQ(int(dimensions["Lines"]), 677);
  EXPECT_EQ(int(dimensions["Bands"]), 1);
}

TEST_F(ThreeImageNetwork, FunctionalTestMapmosBandNumber) {
  QTemporaryDir prefix;
  QString mosPath = prefix.path() + "/mosaic.cub";
  QVector<QString> args = { "MOSAIC=" + mosPath,
                            "create=true",
                            "track=false",
                            "priority=band",
                            "type=bandnumber",
                            "number=1" };
  UserInterface options(APP_XML, args);
  Pvl appLog;

  try {
    mapmos(cube1map, options, &appLog);
  }
  catch (IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  Cube mosaic(mosPath);
  PvlObject mosaicLabel = mosaic.label()->findObject("IsisCube");
  PvlGroup dimensions = mosaicLabel.findObject("Core").findGroup("Dimensions");

  EXPECT_EQ(int(dimensions["Samples"]), 552);
  EXPECT_EQ(int(dimensions["Lines"]), 677);
  EXPECT_EQ(int(dimensions["Bands"]), 1);
}

TEST_F(ThreeImageNetwork, FunctionalTestMapmosKeyword) {
  QTemporaryDir prefix;
  QString mosPath = prefix.path() + "/mosaic.cub";
  QVector<QString> args = { "MOSAIC=" + mosPath,
                            "create=true",
                            "track=false",
                            "priority=band",
                            "type=keyword",
                            "keyname=OriginalBand",
                            "keyvalue=1" };
  UserInterface options(APP_XML, args);
  Pvl appLog;

  try {
    mapmos(cube1map, options, &appLog);
  }
  catch (IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  Cube mosaic(mosPath);
  PvlObject mosaicLabel = mosaic.label()->findObject("IsisCube");
  PvlGroup dimensions = mosaicLabel.findObject("Core").findGroup("Dimensions");

  EXPECT_EQ(int(dimensions["Samples"]), 552);
  EXPECT_EQ(int(dimensions["Lines"]), 677);
  EXPECT_EQ(int(dimensions["Bands"]), 1);
}

TEST_F(ThreeImageNetwork, FunctionalTestMapmosMatchDEM) {
  QTemporaryDir prefix;
  QString mosPath = prefix.path() + "/mosaic.cub";
  QVector<QString> args = { "MOSAIC=" + mosPath,
                            "create=true",
                            "track=false",
                            "priority=ONTOP",
                            "matchdem=true" };
  UserInterface options(APP_XML, args);
  Pvl appLog;

  try {
    mapmos(cube1map, options, &appLog);
  }
  catch (IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  Cube mosaic(mosPath);
  PvlObject mosaicLabel = mosaic.label()->findObject("IsisCube");
  PvlGroup dimensions = mosaicLabel.findObject("Core").findGroup("Dimensions");

  EXPECT_EQ(int(dimensions["Samples"]), 552);
  EXPECT_EQ(int(dimensions["Lines"]), 677);
  EXPECT_EQ(int(dimensions["Bands"]), 1);
}

TEST_F(ThreeImageNetwork, FunctionalTestMapmosAppLog) {
  QTemporaryDir prefix;
  QString mosPath = prefix.path() + "/mosaic.cub";
  QVector<QString> args = { "MOSAIC=" + mosPath,
                            "create=true",
                            "track=false",
                            "priority=ONTOP" };
  UserInterface options(APP_XML, args);
  Pvl appLog;
  QString fromPath = cube1map->fileName();

  try {
    mapmos(cube1map, options, &appLog);
  }
  catch (IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  PvlGroup location = appLog.findGroup("ImageLocation");
  EXPECT_EQ(location["File"][0], fromPath.toStdString());
  EXPECT_EQ(int(location["StartSample"]), 6);
  EXPECT_EQ(int(location["StartLine"]), 194);
}
