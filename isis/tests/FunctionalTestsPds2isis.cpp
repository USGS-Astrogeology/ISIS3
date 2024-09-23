#include <QTemporaryDir>

#include "pds2isis.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/pds2isis.xml").expanded());

TEST(Pds2Isis, Pds2isisTestDefault) {
  Pvl appLog;
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/pds2isis_out.cub";
  QVector<QString> args = { "from=data/pds2isis/ab102401_cropped.img",
                            "to=" + cubeFileName };
  UserInterface options(APP_XML, args);
  try {
    pds2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest file: " <<  e.toString().c_str() << std::endl;
  }

  Cube outCube(cubeFileName.toStdString());
  Pvl *outLabel = outCube.label();

  PvlGroup dimensions = outLabel->findGroup("Dimensions", Pvl::Traverse);
  ASSERT_EQ((int)dimensions["Samples"], 640);
  ASSERT_EQ((int)dimensions["Lines"], 5);
  ASSERT_EQ((int)dimensions["Bands"], 1);

  PvlGroup pixels = outLabel->findGroup("Pixels", Pvl::Traverse);
  ASSERT_EQ(pixels["Type"][0], "UnsignedByte");
  ASSERT_EQ(pixels["ByteOrder"][0], "Lsb");
  ASSERT_EQ(Isis::toDouble(pixels["Base"]), 0.0);
  ASSERT_EQ(Isis::toDouble(pixels["Multiplier"]), 1.0);

  PvlGroup archive = outLabel->findGroup("Archive", Pvl::Traverse);
  ASSERT_EQ(archive["DataSetId"][0], "MGS-M-MOC-NA/WA-2-DSDP-L0-V1.0");
  ASSERT_EQ(archive["ProducerId"][0], "MGS_MOC_TEAM");
  ASSERT_EQ(archive["ProductId"][0], "AB-1-024/01");
  ASSERT_EQ(archive["InstrumentId"][0], "MOC-WA");
  ASSERT_EQ(archive["TargetName"][0], "MARS");
  ASSERT_EQ(archive["MissionPhaseName"][0], "AB-1");
  ASSERT_EQ(archive["RationaleDescription"][0], "OLYMPUS MONS SPECIAL RED WIDE ANGLE");


  std::unique_ptr<Histogram> hist (outCube.histogram());
  ASSERT_NEAR(hist->Average(), 81.5828125, .00001);
  ASSERT_EQ(hist->Sum(), 261065);
  ASSERT_EQ(hist->ValidPixels(), 3200);
  ASSERT_NEAR(hist->StandardDeviation(), 30.5674, .0001);
}


TEST(Pds2Isis, Pds2isisTestBandBin) {
  Pvl appLog;
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/pds2isis_out.cub";
  QVector<QString> args = { "from=data/pds2isis/gaspra_nims_hires_radiance_cropped.lbl",
                            "to=" + cubeFileName };
  UserInterface options(APP_XML, args);

  try {
    pds2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest file: " <<  e.toString().c_str() << std::endl;
  }

  Cube outCube(cubeFileName.toStdString());
  Pvl *outLabel = outCube.label();

  PvlGroup dimensions = outLabel->findGroup("Dimensions", Pvl::Traverse);
  ASSERT_EQ((int)dimensions["Samples"], 100);
  ASSERT_EQ((int)dimensions["Lines"], 3);
  ASSERT_EQ((int)dimensions["Bands"], 3);

  bool ok = false;
  PvlGroup bandbin = outLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(Isis::toDouble(bandbin["Center"][0]),  0.7360);
  ASSERT_EQ(Isis::toDouble(bandbin["Center"][1]),  0.8750);
  ASSERT_EQ(Isis::toDouble(bandbin["Center"][2]),  1.0546);
  ASSERT_EQ(Isis::toDouble(bandbin["Center"][3]),  1.3326);
  ASSERT_EQ(Isis::toDouble(bandbin["Center"][4]),  1.6110);
  ASSERT_EQ(Isis::toDouble(bandbin["Center"][5]),  1.8914);
  ASSERT_EQ(Isis::toDouble(bandbin["Center"][6]),  2.1728);
  ASSERT_EQ(Isis::toDouble(bandbin["Center"][7]),  2.4529);
  ASSERT_EQ(Isis::toDouble(bandbin["Center"][8]),  2.7382);
  ASSERT_EQ(Isis::toDouble(bandbin["Center"][9]),  3.0178);
  ASSERT_EQ(Isis::toDouble(bandbin["Center"][10]), 3.3012);
  ASSERT_EQ(Isis::toDouble(bandbin["Center"][11]), 3.5839);
  ASSERT_EQ(Isis::toDouble(bandbin["Center"][12]), 3.8670);
  ASSERT_EQ(Isis::toDouble(bandbin["Center"][13]), 4.1503);
  ASSERT_EQ(Isis::toDouble(bandbin["Center"][14]), 4.4343);
  ASSERT_EQ(Isis::toDouble(bandbin["Center"][15]), 4.7164);
  ASSERT_EQ(Isis::toDouble(bandbin["Center"][16]), 4.9988);

  ASSERT_EQ(Isis::toDouble(bandbin["Width"][0]),  0.0125);
  ASSERT_EQ(Isis::toDouble(bandbin["Width"][1]),  0.0125);
  ASSERT_EQ(Isis::toDouble(bandbin["Width"][2]),  0.0250);
  ASSERT_EQ(Isis::toDouble(bandbin["Width"][3]),  0.0250);
  ASSERT_EQ(Isis::toDouble(bandbin["Width"][4]),  0.0250);
  ASSERT_EQ(Isis::toDouble(bandbin["Width"][5]),  0.0250);
  ASSERT_EQ(Isis::toDouble(bandbin["Width"][6]),  0.0250);
  ASSERT_EQ(Isis::toDouble(bandbin["Width"][7]),  0.0250);
  ASSERT_EQ(Isis::toDouble(bandbin["Width"][8]),  0.0250);
  ASSERT_EQ(Isis::toDouble(bandbin["Width"][9]),  0.0250);
  ASSERT_EQ(Isis::toDouble(bandbin["Width"][10]), 0.0250);
  ASSERT_EQ(Isis::toDouble(bandbin["Width"][11]), 0.0250);
  ASSERT_EQ(Isis::toDouble(bandbin["Width"][12]), 0.0250);
  ASSERT_EQ(Isis::toDouble(bandbin["Width"][13]), 0.0250);
  ASSERT_EQ(Isis::toDouble(bandbin["Width"][14]), 0.0250);
  ASSERT_EQ(Isis::toDouble(bandbin["Width"][15]), 0.0250);
  ASSERT_EQ(Isis::toDouble(bandbin["Width"][16]), 0.0250);

  ASSERT_EQ(Isis::toInt(bandbin["FilterNumber"][0]),  1);
  ASSERT_EQ(Isis::toInt(bandbin["FilterNumber"][1]),  2);
  ASSERT_EQ(Isis::toInt(bandbin["FilterNumber"][2]),  3);
  ASSERT_EQ(Isis::toInt(bandbin["FilterNumber"][3]),  4);
  ASSERT_EQ(Isis::toInt(bandbin["FilterNumber"][4]),  5);
  ASSERT_EQ(Isis::toInt(bandbin["FilterNumber"][5]),  6);
  ASSERT_EQ(Isis::toInt(bandbin["FilterNumber"][6]),  7);
  ASSERT_EQ(Isis::toInt(bandbin["FilterNumber"][7]),  8);
  ASSERT_EQ(Isis::toInt(bandbin["FilterNumber"][8]),  9);
  ASSERT_EQ(Isis::toInt(bandbin["FilterNumber"][9]),  10);
  ASSERT_EQ(Isis::toInt(bandbin["FilterNumber"][10]), 11);
  ASSERT_EQ(Isis::toInt(bandbin["FilterNumber"][11]), 12);
  ASSERT_EQ(Isis::toInt(bandbin["FilterNumber"][12]), 13);
  ASSERT_EQ(Isis::toInt(bandbin["FilterNumber"][13]), 14);
  ASSERT_EQ(Isis::toInt(bandbin["FilterNumber"][14]), 15);
  ASSERT_EQ(Isis::toInt(bandbin["FilterNumber"][15]), 16);
  ASSERT_EQ(Isis::toInt(bandbin["FilterNumber"][16]), 17);

  std::unique_ptr<Histogram> hist (outCube.histogram(0));
  ASSERT_NEAR(hist->Average(), 0.205984, 1e-3);
  ASSERT_NEAR(hist->Sum(), 185.386, 1e-3);
  ASSERT_EQ(hist->ValidPixels(), 900);
  ASSERT_NEAR(hist->StandardDeviation(), 0.606295, 1e-3);
}


TEST(Pds2Isis, Pds2isisTestOffset) {
  Pvl appLog;
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/pds2isis_out.cub";
  QVector<QString> args = { "from=data/pds2isis/ldem_4_cropped.img",
                            "to=" + cubeFileName };
  UserInterface options(APP_XML, args);

  try {
    pds2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest file: " <<  e.toString().c_str() << std::endl;
  }

  Cube outCube(cubeFileName.toStdString());
  Pvl *outLabel = outCube.label();

  PvlGroup mapping = outLabel->findGroup("Mapping", Pvl::Traverse);
  ASSERT_EQ(mapping["ProjectionName"][0], "SimpleCylindrical");
  ASSERT_EQ(Isis::toDouble(mapping["CenterLongitude"]), 180.0);
  ASSERT_EQ(mapping["TargetName"][0], "Moon");
  ASSERT_EQ(Isis::toDouble(mapping["EquatorialRadius"]), 1737400.0);
  ASSERT_EQ(Isis::toDouble(mapping["PolarRadius"]), 1737400.0);
  ASSERT_EQ(mapping["LatitudeType"][0], "Planetocentric");
  ASSERT_EQ(mapping["LongitudeDirection"][0], "PositiveEast");
  ASSERT_EQ((int)mapping["LongitudeDomain"], 360);
  ASSERT_EQ(Isis::toDouble(mapping["MinimumLatitude"]), -90.0);
  ASSERT_EQ(Isis::toDouble(mapping["MaximumLatitude"]), 90.0);
  ASSERT_EQ(Isis::toDouble(mapping["MinimumLongitude"]), 0.0);
  ASSERT_EQ(Isis::toDouble(mapping["MaximumLongitude"]), 360.0);
  ASSERT_EQ(Isis::toDouble(mapping["UpperLeftCornerX"]), -5458204.8);
  ASSERT_EQ(Isis::toDouble(mapping["UpperLeftCornerY"]), 2729102.4);
  ASSERT_EQ(Isis::toDouble(mapping["PixelResolution"]), 7580.84);
  ASSERT_EQ(Isis::toDouble(mapping["Scale"]), 4.0);

  std::unique_ptr<Histogram> hist (outCube.histogram());
  ASSERT_NEAR(hist->Average(), 1.7375e+06, 10);
  ASSERT_NEAR(hist->Sum(), 2.50026e+09, 1e3);
  ASSERT_EQ(hist->ValidPixels(), 1439);
  ASSERT_NEAR(hist->StandardDeviation(), 9187.96, .0001);
}

TEST(Pds2Isis, Pds2isisTestProjection) {
  Pvl appLog;
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/pds2isis_out.cub";
  QVector<QString> args = { "from=data/pds2isis/ff17_cropped.lbl",
                            "to=" + cubeFileName };
  UserInterface options(APP_XML, args);

  try {
    pds2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest file: " <<  e.toString().c_str() << std::endl;
  }

  Cube outCube(cubeFileName.toStdString());
  Pvl *outLabel = outCube.label();

  PvlGroup mapping = outLabel->findGroup("Mapping", Pvl::Traverse);
  ASSERT_EQ(mapping["ProjectionName"][0], "Sinusoidal");
  ASSERT_EQ(Isis::toDouble(mapping["CenterLongitude"]), 325.3829);
  ASSERT_EQ(mapping["TargetName"][0], "Venus");
  ASSERT_EQ(Isis::toDouble(mapping["EquatorialRadius"]), 6051920.0);
  ASSERT_EQ(Isis::toDouble(mapping["PolarRadius"]), 6051920.0);
  ASSERT_EQ(mapping["LatitudeType"][0], "Planetocentric");
  ASSERT_EQ(mapping["LongitudeDirection"][0], "PositiveEast");
  ASSERT_EQ((int)mapping["LongitudeDomain"], 360);
  ASSERT_EQ(Isis::toDouble(mapping["MinimumLatitude"]), -29.6357);
  ASSERT_EQ(Isis::toDouble(mapping["MaximumLatitude"]), -28.9092);
  ASSERT_EQ(Isis::toDouble(mapping["MinimumLongitude"]), 322.0367);
  ASSERT_EQ(Isis::toDouble(mapping["MaximumLongitude"]), 322.8903);
  ASSERT_EQ(Isis::toDouble(mapping["UpperLeftCornerX"]), -307162.5);
  ASSERT_EQ(Isis::toDouble(mapping["UpperLeftCornerY"]), -3053025.0);
  ASSERT_EQ(Isis::toDouble(mapping["PixelResolution"]), 75.0);
  ASSERT_EQ(Isis::toDouble(mapping["Scale"]), 1407.4);

  std::unique_ptr<Histogram> hist (outCube.histogram());
  ASSERT_NEAR(hist->Average(), 67.7978515625, .00001);
  ASSERT_EQ(hist->Sum(), 69425);
  ASSERT_EQ(hist->ValidPixels(), 1024);
  ASSERT_NEAR(hist->StandardDeviation(), 26.0079, .0001);
}

TEST(Pds2Isis, Pds2isisTestSpecialPixels) {
  Pvl appLog;
  QTemporaryDir prefix;
  QVector<QString> args;
  QString cubeFileName = prefix.path() + "/pds2isis_out.cub";

  // test setnullrange
  args = { "from=data/pds2isis/ab102401_cropped.img",
           "to=" + cubeFileName,
           "setnullrange=yes", "nullmin=15.0", "nullmax=45.0" };
  UserInterface options_nullrange(APP_XML, args);
  try {
    pds2isis(options_nullrange, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Failed for setnullrange=yes on file: " <<  e.toString().c_str() << std::endl;
  }

  // test sethrsrange
  args = { "from=data/pds2isis/ab102401_cropped.img",
           "to=" + cubeFileName,
           "sethrsrange=yes", "hrsmin=220.0", "hrsmax=250.0"};
  UserInterface options_hrsrange(APP_XML, args);
  try {
    pds2isis(options_hrsrange, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Failed for sethrsrange=yes on file: " <<  e.toString().c_str() << std::endl;
  }

  // test sethisrange
  args = { "from=data/pds2isis/ab102401_cropped.img",
           "to=" + cubeFileName,
           "sethisrange=yes", "hismin=190.0", "hismax=219.0"};
  UserInterface options_hisrange(APP_XML, args);
  try {
    pds2isis(options_hisrange, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Failed for sethisrange=yes on file: " <<  e.toString().c_str() << std::endl;
  }

  // test setlrsrange
  args = { "from=data/pds2isis/ab102401_cropped.img",
           "to=" + cubeFileName,
           "setlrsrange=yes", "lrsmin=96.0", "lrsmax=125.0"};
  UserInterface options_lrsrange(APP_XML, args);
  try {
    pds2isis(options_lrsrange, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Failed for setlrsrange=yes on file: " <<  e.toString().c_str() << std::endl;
  }

  // test setlisrange
  args = { "from=data/pds2isis/ab102401_cropped.img",
           "to=" + cubeFileName,
           "setlisrange=yes", "lismin=65.0", "lismax=95.0"};
  UserInterface options_lisrange(APP_XML, args);
  try {
    pds2isis(options_lisrange, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Failed for setlisrange=yes on file: " <<  e.toString().c_str() << std::endl;
  }
}

TEST(Pds2Isis, Pds2isisTestBIL) {
  Pvl appLog;
  QTemporaryDir prefix;
  QVector<QString> args;
  QString cubeFileName = prefix.path() + "/pds2isis_BIL_out.cub";

  args = { "from=data/pds2isis/BILtestData_cropped.LBL",
           "to=" + cubeFileName};
  UserInterface options(APP_XML, args);
  try {
    pds2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Failed to ingest file: " <<  e.toString().c_str() << std::endl;
  }

  Cube outCube(cubeFileName.toStdString());
  Pvl *outLabel = outCube.label();

  PvlGroup dimensions = outLabel->findGroup("Dimensions", Pvl::Traverse);
  ASSERT_EQ((int)dimensions["Lines"], 1);
  ASSERT_EQ((int)dimensions["Samples"], 304);
  ASSERT_EQ((int)dimensions["Bands"], 1);

  std::unique_ptr<Histogram> hist (outCube.histogram());
  ASSERT_NEAR(hist->Average(), 6.35692e+31, 1e25);
  ASSERT_NEAR(hist->Sum(), 1.9325e+34, 1e29);
  ASSERT_EQ(hist->ValidPixels(), 304);
  ASSERT_NEAR(hist->StandardDeviation(), 1.08618e+33, 1e28);
}

TEST(Pds2Isis, Pds2isisTestBIP) {
  Pvl appLog;
  QTemporaryDir prefix;
  QVector<QString> args;
  QString cubeFileName = prefix.path() + "/pds2isis_BIP_out.cub";

  args = { "from=data/pds2isis/BIPtestData_cropped.LBL",
           "to=" + cubeFileName};
  UserInterface options(APP_XML, args);
  try {
    pds2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Failed to ingest file: " <<  e.toString().c_str() << std::endl;
  }

  Cube outCube(cubeFileName.toStdString());
  Pvl *outLabel = outCube.label();

  PvlGroup dimensions = outLabel->findGroup("Dimensions", Pvl::Traverse);
  ASSERT_EQ((int)dimensions["Lines"], 1);
  ASSERT_EQ((int)dimensions["Samples"], 304);
  ASSERT_EQ((int)dimensions["Bands"], 1);

  std::unique_ptr<Histogram> hist (outCube.histogram());
  ASSERT_NEAR(hist->Average(), 6.35692e+31, 1e25);
  ASSERT_NEAR(hist->Sum(), 1.9325e+34, 1e29);
  ASSERT_EQ(hist->ValidPixels(), 304);
  ASSERT_NEAR(hist->StandardDeviation(), 1.08618e+33, 1e28);
}

TEST(Pds2Isis, Pds2isisTestNIMSQub) {
  Pvl appLog;
  QTemporaryDir prefix;
  QVector<QString> args;
  QString cubeFileName = prefix.path() + "/pds2isis_QUB_out.cub";

  args = { "from=data/pds2isis/30i001ci_cropped.qub",
           "to=" + cubeFileName};
  UserInterface options(APP_XML, args);
  try {
    pds2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Failed to ingest file: " <<  e.toString().c_str() << std::endl;
  }

  Cube outCube(cubeFileName.toStdString());
  Pvl *outLabel = outCube.label();

  PvlGroup dimensions = outLabel->findGroup("Dimensions", Pvl::Traverse);
  ASSERT_EQ((int)dimensions["Lines"], 46);
  ASSERT_EQ((int)dimensions["Samples"], 1);
  ASSERT_EQ((int)dimensions["Bands"], 12);

  std::unique_ptr<Histogram> hist (outCube.histogram());
  ASSERT_NEAR(hist->Average(), 1.64693e+30, 1e25);
  ASSERT_NEAR(hist->Sum(), 7.57588e+31, 1e26);
  ASSERT_EQ(hist->ValidPixels(), 46);
  ASSERT_NEAR(hist->StandardDeviation(), 1.117e+31, 1e26);
}
