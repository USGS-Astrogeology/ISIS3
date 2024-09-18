#include <QTemporaryDir>
#include <QFile>

#include "sumspice.h"
#include "TestUtilities.h"
#include "Table.h"
#include "UserInterface.h"
#include "Histogram.h"
#include "iTime.h"

#include "gmock/gmock.h"

using namespace Isis;
using ::testing::HasSubstr;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/sumspice.xml").expanded());

TEST(Sumspice, FunctionalTestSumspiceTimeUpdate) {
  QTemporaryDir prefix;
  QString tempDest = prefix.path() + "/sumspiceTEMP.cub";
  QString cubeFileName = "data/sumspice/st_2395699394_v.lev0.cub";
  QFile::copy(cubeFileName, tempDest);

  QVector<QString> args = {"from=" + tempDest,
                           "sumfile=data/sumspice/N2395699394.SUM",
                           "sumtime=start",
                           "update=times"
                          };
  UserInterface options(APP_XML, args);
  sumspice(options);

  Cube cube(tempDest.toStdString());
  Pvl *isisLabel = cube.label();

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "1/2395694869:238");
  EXPECT_EQ(inst["SpacecraftClockStopCount"][0], "1/2395694872:183");
  EXPECT_EQ(inst["StartTime"][0], "2005-09-21T10:44:07.352");
  EXPECT_EQ(inst["StopTime"][0], "2005-09-21T10:44:07.439");

  // SumTimeHistory Group
  PvlGroup &sumTime = isisLabel->findGroup("SumTimeHistory", Pvl::Traverse);
  EXPECT_EQ(sumTime["SUMFILE"][0], "N2395699394");
  EXPECT_EQ(std::stod(sumTime["SpacecraftClockStartCount"][0]), 2395694888);
  EXPECT_EQ(std::stod(sumTime["SpacecraftClockStopCount"][0]), 2395695365);
  EXPECT_EQ(sumTime["StartTime"][0], "2005-09-21T10:44:07");
  EXPECT_EQ(sumTime["StopTime"][0], "2005-09-21T10:44:07");

  std::unique_ptr<Histogram> hist (cube.histogram());

  EXPECT_NEAR(hist->Average(), 2.8611278533935547, 1e-11);
  EXPECT_EQ(hist->Sum(), 3000110);
  EXPECT_EQ(hist->ValidPixels(), 1048576);
  EXPECT_NEAR(hist->StandardDeviation(), 18.463625088626337, 1e-11);
}

TEST(Sumspice, FunctionalTestSumspicePointingUpdate) {
  QTemporaryDir prefix;
  QString tempDest = prefix.path() + "/sumspiceTEMP.cub";
  QString toLogPath = prefix.path() + "/log.txt";
  QString cubeFileName = "data/sumspice/st_2395699394_updatedtime.cub";
  QFile::copy(cubeFileName, tempDest);

  QVector<QString> args = {"from=" + tempDest,
                           "sumfile=data/sumspice/N2395699394.SUM",
                           "sumtime=start",
                           "update=pointing",
                           "tolog=" + toLogPath
                          };
  UserInterface options(APP_XML, args);
  sumspice(options);

  Cube cube(tempDest.toStdString());

  // InstrumentPointing Table
  Table ptTable = cube.readTable("InstrumentPointing");
  EXPECT_DOUBLE_EQ(ptTable.Label()["CkTableStartTime"], 180571511.57789);
  EXPECT_DOUBLE_EQ(ptTable.Label()["CkTableEndTime"], 180571511.57789);

  // InstrumentPosition Table
  Table posTable = cube.readTable("InstrumentPosition");
  EXPECT_DOUBLE_EQ(posTable.Label()["SpkTableStartTime"], 180571511.57789);
  EXPECT_DOUBLE_EQ(posTable.Label()["SpkTableEndTime"], 180571511.57789);

  // SunPosition Table
  Table sunTable = cube.readTable("SunPosition");
  EXPECT_DOUBLE_EQ(sunTable.Label()["SpkTableStartTime"], 180571511.57789);
  EXPECT_DOUBLE_EQ(sunTable.Label()["SpkTableEndTime"], 180571511.57789);

  // BodyRotation Table
  Table bodyTable = cube.readTable("BodyRotation");
  EXPECT_DOUBLE_EQ(bodyTable.Label().findKeyword("CkTableStartTime"), 180571511.57789);
  EXPECT_DOUBLE_EQ(bodyTable.Label().findKeyword("CkTableEndTime"), 180571511.57789);
}

TEST(Sumspice, FunctionalTestSumspicePositionUpdate) {
  QTemporaryDir prefix;
  QString tempDest = prefix.path() + "/sumspiceTEMP.cub";
  QString toLogPath = prefix.path() + "/log.txt";
  QString cubeFileName = "data/sumspice/st_2395699394_updatedtime.cub";
  QFile::copy(cubeFileName, tempDest);

  QVector<QString> args = {"from=" + tempDest,
                           "sumfile=data/sumspice/N2395699394.SUM",
                           "sumtime=start",
                           "update=position",
                           "tolog=" + toLogPath
                          };
  UserInterface options(APP_XML, args);
  sumspice(options);

  Cube cube(tempDest.toStdString());

  // InstrumentPointing Table
  Table ptTable = cube.readTable("InstrumentPointing");
  EXPECT_DOUBLE_EQ(ptTable.Label()["CkTableStartTime"], 180571511.57789);
  EXPECT_DOUBLE_EQ(ptTable.Label()["CkTableEndTime"], 180571511.57789);

  // InstrumentPosition Table
  Table posTable = cube.readTable("InstrumentPosition");
  EXPECT_DOUBLE_EQ(posTable.Label()["SpkTableStartTime"], 180571511.57789);
  EXPECT_DOUBLE_EQ(posTable.Label()["SpkTableEndTime"], 180571511.57789);

  // SunPosition Table
  Table sunTable = cube.readTable("SunPosition");
  EXPECT_DOUBLE_EQ(sunTable.Label()["SpkTableStartTime"], 180571511.57789);
  EXPECT_DOUBLE_EQ(sunTable.Label()["SpkTableEndTime"], 180571511.57789);

  // BodyRotation Table
  Table bodyTable = cube.readTable("BodyRotation");
  EXPECT_DOUBLE_EQ(bodyTable.Label().findKeyword("CkTableStartTime"), 180571511.57789);
  EXPECT_DOUBLE_EQ(bodyTable.Label().findKeyword("CkTableEndTime"), 180571511.57789);
}

TEST(Sumspice, FunctionalTestSumspiceSpiceUpdate) {
  QTemporaryDir prefix;
  QString tempDest = prefix.path() + "/sumspiceTEMP.cub";
  QString toLogPath = prefix.path() + "/log.txt";
  QString cubeFileName = "data/sumspice/st_2395699394_updatedtime.cub";
  QFile::copy(cubeFileName, tempDest);

  QVector<QString> args = {"from=" + tempDest,
                           "sumfile=data/sumspice/N2395699394.SUM",
                           "sumtime=start",
                           "update=spice",
                           "tolog=" + toLogPath
                          };
  UserInterface options(APP_XML, args);
  sumspice(options);

  Cube cube(tempDest.toStdString());

  // InstrumentPointing Table
  Table ptTable = cube.readTable("InstrumentPointing");
  EXPECT_DOUBLE_EQ(ptTable.Label()["CkTableStartTime"], 180571511.57789);
  EXPECT_DOUBLE_EQ(ptTable.Label()["CkTableEndTime"], 180571511.57789);

  // InstrumentPosition Table
  Table posTable = cube.readTable("InstrumentPosition");
  EXPECT_DOUBLE_EQ(posTable.Label()["SpkTableStartTime"], 180571511.57789);
  EXPECT_DOUBLE_EQ(posTable.Label()["SpkTableEndTime"], 180571511.57789);

  // SunPosition Table
  Table sunTable = cube.readTable("SunPosition");
  EXPECT_DOUBLE_EQ(sunTable.Label()["SpkTableStartTime"], 180571511.57789);
  EXPECT_DOUBLE_EQ(sunTable.Label()["SpkTableEndTime"], 180571511.57789);

  // BodyRotation Table
  Table bodyTable = cube.readTable("BodyRotation");
  EXPECT_DOUBLE_EQ(bodyTable.Label().findKeyword("CkTableStartTime"), 180571511.57789);
  EXPECT_DOUBLE_EQ(bodyTable.Label().findKeyword("CkTableEndTime"), 180571511.57789);

  CSVReader::CSVAxis csvLine;
  CSVReader header = CSVReader(toLogPath, false, 0, ',', false, true);
  csvLine = header.getRow(0);
  EXPECT_EQ(csvLine[0].toStdString(), "Filename");
  EXPECT_EQ(csvLine[1].toStdString(), "SUMFILE");
  EXPECT_EQ(csvLine[2].toStdString(), "SumTime");
  EXPECT_EQ(csvLine[3].toStdString(), "Update");
  EXPECT_EQ(csvLine[4].toStdString(), "CubeSumDeltaTime");
  EXPECT_EQ(csvLine[5].toStdString(), " ExposureTime");
  EXPECT_EQ(csvLine[6].toStdString(), "CubeStartTime");
  EXPECT_EQ(csvLine[7].toStdString(), "CubeCenterTime");
  EXPECT_EQ(csvLine[8].toStdString(), "CubeStopTime");
  EXPECT_EQ(csvLine[9].toStdString(), "SumStartTime");
  EXPECT_EQ(csvLine[10].toStdString(), "SumCenterTime");
  EXPECT_EQ(csvLine[11].toStdString(), "SumStopTime");

  csvLine = header.getRow(1);
  EXPECT_EQ(csvLine[1].toStdString(), "N2395699394");
  EXPECT_EQ(csvLine[2].toStdString(), "start");
  EXPECT_EQ(csvLine[3].toStdString(), "spice");
  EXPECT_EQ(csvLine[4].toDouble(), 2.3841858e-07);
  EXPECT_EQ(csvLine[5].toDouble(), 0.087);
  EXPECT_EQ(csvLine[6].toStdString(), "2005-09-21T10:44:07.3519998");
  EXPECT_EQ(csvLine[7].toStdString(), "2005-09-21T10:44:07.3954998");
  EXPECT_EQ(csvLine[8].toStdString(), "2005-09-21T10:44:07.4390337");
  EXPECT_EQ(csvLine[9].toStdString(), "2005-09-21T10:44:07.352");
  EXPECT_EQ(csvLine[10].toStdString(), "2005-09-21T10:44:07.3955");
  EXPECT_EQ(csvLine[11].toStdString(), "2005-09-21T10:44:07.439");

  std::unique_ptr<Histogram> hist (cube.histogram());

  EXPECT_NEAR(hist->Average(), 1.22291692076441, 1e-11);
  EXPECT_NEAR(hist->Sum(), 1282321.3331074715, 1e-11);
  EXPECT_EQ(hist->ValidPixels(), 1048576);
  EXPECT_NEAR(hist->StandardDeviation(), 11.842834365508679, 1e-11);
}

TEST(Sumspice, FunctionalTestSumspiceNoCubeError) {
  QVector<QString> args = {"sumfile=data/sumspice/N2395699394.SUM"};
  UserInterface options(APP_XML, args);

  try{
    sumspice(options);
    FAIL() << "Should throw an exception" << std::endl;
  }

  catch (IException &e){
    EXPECT_THAT(e.what(), HasSubstr("User must provide either an input cube file or an input cube file list"));
  }
}

TEST(Sumspice, FunctionalTestSumspiceNoSumError) {
  QTemporaryDir prefix;
  QVector<QString> args = {"from=" + prefix.path() + "/sumspiceTEMP.cub"};
  UserInterface options(APP_XML, args);

  try{
    sumspice(options);
    FAIL() << "Should throw an exception" << std::endl;
  }

  catch (IException &e){
    EXPECT_THAT(e.what(), HasSubstr("User must provide either a sum file or a sum file list"));
  }
}
