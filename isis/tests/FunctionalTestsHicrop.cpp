#include "CameraFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"

#include "hicrop.h"

#include "TestUtilities.h"
#include "CameraFixtures.h"
#include "gmock/gmock.h"

using namespace Isis;
using namespace testing;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/hicrop.xml").expanded());

TEST_F(MroHiriseCube, FunctionalTestHicropCropByCk) {
  // make the image stretch outside of the CK time ranges
  PvlGroup &inst = testCube->label()->findObject("IsisCube").findGroup("Instrument");
  inst.findKeyword("DeltaLineTimerCount").setValue("99999");

  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTemp.cub";

  QVector<QString> args = {"from=eh", "to="+outCubeFileName, "source=CK", "CK="+ckPath,
  "SCLK="+sclkPath, "LSK="+lskPath};

  UserInterface options(APP_XML, args);
  Pvl *logs = new Pvl();
  try {
     hicrop(testCube, options, logs);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  PvlGroup results = logs->findGroup("Results");

  EXPECT_EQ((int)results.findKeyword("InputLines"), 1056);
  EXPECT_EQ((int)results.findKeyword("NumberOfLinesCropped"), 54);
  EXPECT_EQ((int)results.findKeyword("OututStartingLine"), 55);
  EXPECT_EQ((int)results.findKeyword("OututEndingLine"), 1056);
  EXPECT_EQ((int)results.findKeyword("OututLineCount"), 1002);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStartTime"), "2008-05-17T09:37:24.7300819");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStopTime"), "2008-05-17T09:37:31.0666673");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStartClock"), "4/0895484265.14186");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStopClock"), "4/0895484271.36245");

  Cube oCube(outCubeFileName.toStdString());
  PvlGroup oCubeInstrument = testCube->label()->findObject("IsisCube").findGroup("Instrument");

  EXPECT_EQ(oCube.lineCount(), 1002);
  EXPECT_EQ(oCube.sampleCount(), 1204);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("StartTime"), "2008-05-17T09:37:24.7300819");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("StopTime"), "2008-05-17T09:37:31.0666673");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("SpacecraftClockStartCount"), "895484264:57342");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("SpacecraftClockStopCount"), "895484272:12777");
}


TEST_F(MroHiriseCube, FunctionalTestHicropCropByLine) {
  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTemp.cub";

  QVector<QString> args = {"to="+outCubeFileName, "source=LINE", "LINE=55", "NLINES=1002", "CK="+ckPath,
  "SCLK="+sclkPath, "LSK="+lskPath};

  UserInterface options(APP_XML, args);
  Pvl *logs = new Pvl();
  try {
     hicrop(testCube, options, logs);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  PvlGroup results = logs->findGroup("Results");

  EXPECT_EQ((int)results.findKeyword("InputLines"), 1056);
  EXPECT_EQ((int)results.findKeyword("NumberOfLinesCropped"), 54);
  EXPECT_EQ((int)results.findKeyword("OututStartingLine"), 55);
  EXPECT_EQ((int)results.findKeyword("OututEndingLine"), 1056);
  EXPECT_EQ((int)results.findKeyword("OututLineCount"), 1002);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStartTime"), "2008-05-17T09:37:24.7892562");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStopTime"), "2008-05-17T09:37:24.8845088");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStartClock"), "4/0895484264.57678");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStopClock"), "4/0895484264.63921");

  Cube oCube(outCubeFileName.toStdString());
  PvlGroup oCubeInstrument = testCube->label()->findObject("IsisCube").findGroup("Instrument");

  EXPECT_EQ(oCube.lineCount(), 1002);
  EXPECT_EQ(oCube.sampleCount(), 1204);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("StartTime"), "2008-05-17T09:37:24.7300819");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("StopTime"), "2008-05-17T09:37:31.0666673");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("SpacecraftClockStartCount"), "895484264:57342");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("SpacecraftClockStopCount"), "895484272:12777");
}


TEST_F(MroHiriseCube, FunctionalTestHicropCropByTimes) {

  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTemp.cub";

  QVector<QString> args = {"to="+outCubeFileName, "source=TIME", "STARTTIME=264289109.96933", "STOPTIME=264289110.06", "CK="+ckPath,
  "SCLK="+sclkPath, "LSK="+lskPath};

  UserInterface options(APP_XML, args);
  Pvl *logs = new Pvl();
  try {
     hicrop(testCube, options, logs);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  PvlGroup results = logs->findGroup("Results");

  EXPECT_EQ((int)results.findKeyword("InputLines"), 1056);
  EXPECT_EQ((int)results.findKeyword("NumberOfLinesCropped"), 103);
  EXPECT_EQ((int)results.findKeyword("OututStartingLine"), 1);
  EXPECT_EQ((int)results.findKeyword("OututEndingLine"), 953);
  EXPECT_EQ((int)results.findKeyword("OututLineCount"), 953);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStartTime"), "2008-05-17T09:37:24.7841228");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStopTime"), "2008-05-17T09:37:24.8747174");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStartClock"), "4/0895484264.57342");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStopClock"), "4/0895484264.63279");

  Cube oCube(outCubeFileName.toStdString());
  PvlGroup oCubeInstrument = testCube->label()->findObject("IsisCube").findGroup("Instrument");

  EXPECT_EQ(oCube.lineCount(), 953);
  EXPECT_EQ(oCube.sampleCount(), 1204);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("StartTime"), "2008-05-17T09:37:24.7300819");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("StopTime"), "2008-05-17T09:37:31.0666673");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("SpacecraftClockStartCount"), "895484264:57342");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("SpacecraftClockStopCount"), "895484272:12777");
}


TEST_F(MroHiriseCube, FunctionalTestHicropCropByJitterDefault) {

  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTemp.cub";

  QVector<QString> args = {"to="+outCubeFileName, "source=Jitter", "jitter="+jitterPath, "CK="+ckPath,
  "SCLK="+sclkPath, "LSK="+lskPath};

  UserInterface options(APP_XML, args);
  Pvl *logs = new Pvl();
  try {
     hicrop(testCube, options, logs);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  PvlGroup results = logs->findGroup("Results");

  EXPECT_EQ((int)results.findKeyword("InputLines"), 1056);
  EXPECT_EQ((int)results.findKeyword("NumberOfLinesCropped"), 103);
  EXPECT_EQ((int)results.findKeyword("OututStartingLine"), 1);
  EXPECT_EQ((int)results.findKeyword("OututEndingLine"), 953);
  EXPECT_EQ((int)results.findKeyword("OututLineCount"), 953);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStartTime"), "2008-05-17T09:37:24.7841228");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStopTime"), "2008-05-17T09:37:24.8747174");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStartClock"), "4/0895484264.57342");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStopClock"), "4/0895484264.63279");

  Cube oCube(outCubeFileName.toStdString());
  PvlGroup oCubeInstrument = testCube->label()->findObject("IsisCube").findGroup("Instrument");

  EXPECT_EQ(oCube.lineCount(), 953);
  EXPECT_EQ(oCube.sampleCount(), 1204);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("StartTime"), "2008-05-17T09:37:24.7300819");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("StopTime"), "2008-05-17T09:37:31.0666673");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("SpacecraftClockStartCount"), "895484264:57342");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("SpacecraftClockStopCount"), "895484272:12777");
}


TEST_F(MroHiriseCube, FunctionalTestHicropCropByJitterZero) {

  QTemporaryDir prefix;

    // create a jitter file
    QString jitter = R"(# Sample                 Line                   ET
0     0     264289109.96933
-0.11     -0.04     264289109.98
-0.05     -0.02     264289109.99
1.5     0.6     264289110.06
    )";

  jitterPath = prefix.path() + "/jitter.txt";
  QFile jitterFile(jitterPath);

  if (jitterFile.open(QIODevice::WriteOnly)) {
    QTextStream out(&jitterFile);
    out << jitter;
    jitterFile.close();
  }
  else {
    FAIL() << "Failed to create Jitter file" << std::endl;
  }

  QString outCubeFileName = prefix.path() + "/outTemp.cub";

  QVector<QString> args = {"to="+outCubeFileName, "source=Jitter", "jitter="+jitterPath, "CK="+ckPath,
  "SCLK="+sclkPath, "LSK="+lskPath};

  UserInterface options(APP_XML, args);
  Pvl *logs = new Pvl();
  try {
     hicrop(testCube, options, logs);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  PvlGroup results = logs->findGroup("Results");

  EXPECT_EQ((int)results.findKeyword("InputLines"), 1056);
  EXPECT_EQ((int)results.findKeyword("NumberOfLinesCropped"), 216);
  EXPECT_EQ((int)results.findKeyword("OututStartingLine"), 114);
  EXPECT_EQ((int)results.findKeyword("OututEndingLine"), 953);
  EXPECT_EQ((int)results.findKeyword("OututLineCount"), 840);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStartTime"), "2008-05-17T09:37:24.7948649");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStopTime"), "2008-05-17T09:37:24.8747174");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStartClock"), "4/0895484264.58046");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStopClock"), "4/0895484264.63279");

  Cube oCube(outCubeFileName.toStdString());
  PvlGroup oCubeInstrument = testCube->label()->findObject("IsisCube").findGroup("Instrument");

  EXPECT_EQ(oCube.lineCount(), 840);
  EXPECT_EQ(oCube.sampleCount(), 1204);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("StartTime"), "2008-05-17T09:37:24.7300819");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("StopTime"), "2008-05-17T09:37:31.0666673");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("SpacecraftClockStartCount"), "895484264:57342");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("SpacecraftClockStopCount"), "895484272:12777");
}


TEST_F(MroHiriseCube, FunctionalTestHicropCropByJitterZeroSample) {

  QTemporaryDir prefix;

  // create a jitter file
  QString jitter = R"(# Sample                 Line                   ET
 0        -0.07     264289109.96933
-0.11     -0.04     264289109.98
-0.05     -0.02     264289109.99
1.5        0.6      264289110.06
    )";

  jitterPath = prefix.path() + "/jitter.txt";
  QFile jitterFile(jitterPath);

  if (jitterFile.open(QIODevice::WriteOnly)) {
    QTextStream out(&jitterFile);
    out << jitter;
    jitterFile.close();
  }
  else {
    FAIL() << "Failed to create Jitter file" << std::endl;
  }

  QString outCubeFileName = prefix.path() + "/outTemp.cub";

  QVector<QString> args = {"to="+outCubeFileName, "source=Jitter", "jitter="+jitterPath, "CK="+ckPath,
  "SCLK="+sclkPath, "LSK="+lskPath};

  UserInterface options(APP_XML, args);
  Pvl *logs = new Pvl();
  try {
     hicrop(testCube, options, logs);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  PvlGroup results = logs->findGroup("Results");

  EXPECT_EQ((int)results.findKeyword("InputLines"), 1056);
  EXPECT_EQ((int)results.findKeyword("NumberOfLinesCropped"), 103);
  EXPECT_EQ((int)results.findKeyword("OututStartingLine"), 1);
  EXPECT_EQ((int)results.findKeyword("OututEndingLine"), 953);
  EXPECT_EQ((int)results.findKeyword("OututLineCount"), 953);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStartTime"), "2008-05-17T09:37:24.7841228");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStopTime"), "2008-05-17T09:37:24.8747174");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStartClock"), "4/0895484264.57342");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStopClock"), "4/0895484264.63279");

  Cube oCube(outCubeFileName.toStdString());
  PvlGroup oCubeInstrument = testCube->label()->findObject("IsisCube").findGroup("Instrument");

  EXPECT_EQ(oCube.lineCount(), 953);
  EXPECT_EQ(oCube.sampleCount(), 1204);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("StartTime"), "2008-05-17T09:37:24.7300819");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("StopTime"), "2008-05-17T09:37:31.0666673");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("SpacecraftClockStartCount"), "895484264:57342");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("SpacecraftClockStopCount"), "895484272:12777");
}


TEST_F(MroHiriseCube, FunctionalTestHicropCropByJitterZeroLine) {

  QTemporaryDir prefix;

  // create a jitter file
  QString jitter = R"(# Sample                 Line                   ET
-0.18         0     264289109.96933
-0.11     -0.04     264289109.98
-0.05     -0.02     264289109.99
1.5        0.6      264289110.06
    )";

  jitterPath = prefix.path() + "/jitter.txt";
  QFile jitterFile(jitterPath);

  if (jitterFile.open(QIODevice::WriteOnly)) {
    QTextStream out(&jitterFile);
    out << jitter;
    jitterFile.close();
  }
  else {
    FAIL() << "Failed to create Jitter file" << std::endl;
  }

  QString outCubeFileName = prefix.path() + "/outTemp.cub";

  QVector<QString> args = {"to="+outCubeFileName, "source=Jitter", "jitter="+jitterPath, "CK="+ckPath,
  "SCLK="+sclkPath, "LSK="+lskPath};

  UserInterface options(APP_XML, args);
  Pvl *logs = new Pvl();
  try {
     hicrop(testCube, options, logs);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  PvlGroup results = logs->findGroup("Results");

  EXPECT_EQ((int)results.findKeyword("InputLines"), 1056);
  EXPECT_EQ((int)results.findKeyword("NumberOfLinesCropped"), 103);
  EXPECT_EQ((int)results.findKeyword("OututStartingLine"), 1);
  EXPECT_EQ((int)results.findKeyword("OututEndingLine"), 953);
  EXPECT_EQ((int)results.findKeyword("OututLineCount"), 953);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStartTime"), "2008-05-17T09:37:24.7841228");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStopTime"), "2008-05-17T09:37:24.8747174");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStartClock"), "4/0895484264.57342");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, results.findKeyword("OututStopClock"), "4/0895484264.63279");

  Cube oCube(outCubeFileName.toStdString());
  PvlGroup oCubeInstrument = testCube->label()->findObject("IsisCube").findGroup("Instrument");

  EXPECT_EQ(oCube.lineCount(), 953);
  EXPECT_EQ(oCube.sampleCount(), 1204);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("StartTime"), "2008-05-17T09:37:24.7300819");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("StopTime"), "2008-05-17T09:37:31.0666673");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("SpacecraftClockStartCount"), "895484264:57342");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, oCubeInstrument.findKeyword("SpacecraftClockStopCount"), "895484272:12777");
}


TEST_F(MroHiriseCube, FunctionalTestHicropInstrumentError) {
  PvlGroup &inst = testCube->label()->findObject("IsisCube").findGroup("Instrument");
  PvlKeyword &instrumentName = inst.findKeyword("InstrumentId");
  instrumentName.setValue("NoHiriseLmao");

  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTemp.cub";

  QVector<QString> args = {"to="+outCubeFileName, "source=Jitter", "jitter="+jitterPath, "CK="+ckPath,
  "SCLK="+sclkPath, "LSK="+lskPath};

  UserInterface options(APP_XML, args);
  Pvl *logs = new Pvl();

  try {
     hicrop(testCube, options, logs);
     FAIL() << "Exepected Error thrown";
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("Input cube has invalid InstrumentId"));;
  }
}


TEST_F(MroHiriseCube, FunctionalTestHicropStartStopTimeError) {

  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTemp.cub";

  QVector<QString> args = {"to="+outCubeFileName, "source=TIME", "STOPTIME=264289109.96933", "STARTTIME=264289110.06", "jitter="+jitterPath, "CK="+ckPath,
  "SCLK="+sclkPath, "LSK="+lskPath};

  UserInterface options(APP_XML, args);
  Pvl *logs = new Pvl();

  try {
     hicrop(testCube, options, logs);
     FAIL() << "Exepected Error thrown";
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("Invalid start/stop times"));;
  }
}


TEST_F(MroHiriseCube, FunctionalTestHicropCkRangeError) {

  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTemp.cub";

  QVector<QString> args = {"to="+outCubeFileName, "source=TIME", "STARTTIME=1", "STOPTIME=2", "jitter="+jitterPath, "CK="+ckPath,
  "SCLK="+sclkPath, "LSK="+lskPath};

  UserInterface options(APP_XML, args);
  Pvl *logs = new Pvl();

  try {
     hicrop(testCube, options, logs);
     FAIL() << "Exepected Error thrown";
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("These times fall outside of the given CK file's time coverage"));;
  }
}
