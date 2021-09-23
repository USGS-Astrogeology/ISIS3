#include "noproj.h"

#include <QTextStream>

#include "Fixtures.h"
#include "Histogram.h"
#include "LineManager.h"
#include "PvlGroup.h"
#include "TestUtilities.h"

#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/noproj.xml").expanded();

TEST_F(DefaultCube, FunctionalTestNoprojDefault) {
  QString cubeFileName = tempDir.path() + "/output.cub";
  QVector<QString> args = {"to=" + cubeFileName};
  UserInterface options(APP_XML, args);

  // Empty match cube
  noproj(testCube, NULL, options);

  Cube oCube(cubeFileName);
  Pvl *isisLabel = oCube.label();
  PvlGroup instGroup = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("SpacecraftName"), "IdealSpacecraft");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("InstrumentId"), "IdealCamera");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("TargetName"), "MARS");
  EXPECT_EQ((int) instGroup.findKeyword("SampleDetectors"), 1250);
  EXPECT_EQ((int) instGroup.findKeyword("LineDetectors"), 1150);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("InstrumentType"), "FRAMING");
  EXPECT_DOUBLE_EQ((double) instGroup.findKeyword("EphemerisTime"), -709401200.26114);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("StartTime"), "1977-07-09T20:05:51");
  EXPECT_EQ((int) instGroup.findKeyword("FocalPlaneXDependency"), 1);
  EXPECT_DOUBLE_EQ((double) instGroup.findKeyword("TransX"), 1.0);
  EXPECT_DOUBLE_EQ((double) instGroup.findKeyword("TransY"), 1.0);

  FileName matchedCubeName(instGroup.findKeyword("matchedCube"));
  FileName defaultCubeName(testCube->fileName());
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, matchedCubeName.name(), defaultCubeName.name());

  PvlGroup origInst = isisLabel->findGroup("OriginalInstrument", Pvl::Traverse);
  PvlGroup testCubeInst = testCube->label()->findGroup("Instrument", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupKeywordsEqual, origInst, testCubeInst); //REFACTOR

  std::unique_ptr<Histogram> hist (oCube.histogram(1));
  EXPECT_NEAR(hist->Average(), 127.4782522807407, .000001);
  EXPECT_NEAR(hist->Sum(), 166492334, .0001);
  EXPECT_EQ(hist->ValidPixels(), 1306045);
  EXPECT_NEAR(hist->StandardDeviation(), 68.405508539707895, .0001);
}

TEST_F(DefaultCube, FunctionalTestNoprojExpand) {
  QString cubeFileName = tempDir.path() + "/output.cub";
  QVector<QString> args = {"to=" + cubeFileName, "sampexp=10", "lineexp=5"};
  UserInterface options(APP_XML, args);

  noproj(testCube, NULL, options);

  Cube oCube(cubeFileName);
  Pvl *isisLabel = oCube.label();
  PvlGroup instGroup = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("SpacecraftName"), "IdealSpacecraft");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("InstrumentId"), "IdealCamera");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("TargetName"), "MARS");
  EXPECT_EQ((int) instGroup.findKeyword("SampleDetectors"), 1375);
  EXPECT_EQ((int) instGroup.findKeyword("LineDetectors"), 1208);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("InstrumentType"), "FRAMING");
  EXPECT_DOUBLE_EQ((double) instGroup.findKeyword("EphemerisTime"), -709401200.26114);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("StartTime"), "1977-07-09T20:05:51");
  EXPECT_EQ((int) instGroup.findKeyword("FocalPlaneXDependency"), 1);
  EXPECT_DOUBLE_EQ((double) instGroup.findKeyword("TransX"), 1.0);
  EXPECT_DOUBLE_EQ((double) instGroup.findKeyword("TransY"), 1.0);

  FileName matchedCubeName(instGroup.findKeyword("matchedCube"));
  FileName defaultCubeName(testCube->fileName());
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, matchedCubeName.name(), defaultCubeName.name());

  PvlGroup origInst = isisLabel->findGroup("OriginalInstrument", Pvl::Traverse);
  PvlGroup testCubeInst = testCube->label()->findGroup("Instrument", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupKeywordsEqual, origInst, testCubeInst); //REFACTOR

  std::unique_ptr<Histogram> hist (oCube.histogram(1));
  EXPECT_NEAR(hist->Average(), 127.50009071999523, .000001);
  EXPECT_NEAR(hist->Sum(), 166542786, 1);
  EXPECT_EQ(hist->ValidPixels(), 1306217);
  EXPECT_NEAR(hist->StandardDeviation(), 68.416277416274923, .0001);
}

// Test source parameter
// Since match cube = input cube, frommatch and frominput will
// give the same output. Default test already tests frommatch.
TEST_F(DefaultCube, FunctionalTestNoprojFromInput) {
  QString cubeFileName = tempDir.path() + "/output.cub";
  QVector<QString> args = {"to=" + cubeFileName, "source=frominput"};
  UserInterface options(APP_XML, args);

  noproj(testCube, NULL, options);

  Cube oCube(cubeFileName);
  Pvl *isisLabel = oCube.label();
  PvlGroup instGroup = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("SpacecraftName"), "IdealSpacecraft");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("InstrumentId"), "IdealCamera");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("TargetName"), "MARS");
  EXPECT_EQ((int) instGroup.findKeyword("SampleDetectors"), 1250);
  EXPECT_EQ((int) instGroup.findKeyword("LineDetectors"), 1150);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("InstrumentType"), "FRAMING");
  EXPECT_DOUBLE_EQ((double) instGroup.findKeyword("EphemerisTime"), -709401200.26114);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("StartTime"), "1977-07-09T20:05:51");
  EXPECT_EQ((int) instGroup.findKeyword("FocalPlaneXDependency"), 1);
  EXPECT_DOUBLE_EQ((double) instGroup.findKeyword("TransX"), 1.0);
  EXPECT_DOUBLE_EQ((double) instGroup.findKeyword("TransY"), 1.0);

  FileName matchedCubeName(instGroup.findKeyword("matchedCube"));
  FileName defaultCubeName(testCube->fileName());
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, matchedCubeName.name(), defaultCubeName.name());

  PvlGroup origInst = isisLabel->findGroup("OriginalInstrument", Pvl::Traverse);
  PvlGroup testCubeInst = testCube->label()->findGroup("Instrument", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupKeywordsEqual, origInst, testCubeInst); //REFACTOR

  std::unique_ptr<Histogram> hist (oCube.histogram(1));
  EXPECT_NEAR(hist->Average(), 127.4782522807407, .000001);
  EXPECT_NEAR(hist->Sum(), 166492334, .0001);
  EXPECT_EQ(hist->ValidPixels(), 1306045);
  EXPECT_NEAR(hist->StandardDeviation(), 68.405508539707895, .0001);
}

TEST_F(DefaultCube, FunctionalTestNoprojFromUser) {
  QString cubeFileName = tempDir.path() + "/output.cub";
  QVector<QString> args = {"to=" + cubeFileName, "source=fromuser", "sum=2"};
  UserInterface options(APP_XML, args);

  noproj(testCube, NULL, options);

  Cube oCube(cubeFileName);
  Pvl *isisLabel = oCube.label();
  PvlGroup instGroup = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("SpacecraftName"), "IdealSpacecraft");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("InstrumentId"), "IdealCamera");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("TargetName"), "MARS");
  EXPECT_EQ((int) instGroup.findKeyword("SampleDetectors"), 625);
  EXPECT_EQ((int) instGroup.findKeyword("LineDetectors"), 1150);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("InstrumentType"), "FRAMING");
  EXPECT_DOUBLE_EQ((double) instGroup.findKeyword("EphemerisTime"), -709401200.26114);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("StartTime"), "1977-07-09T20:05:51");
  EXPECT_EQ((int) instGroup.findKeyword("FocalPlaneXDependency"), 1);
  EXPECT_DOUBLE_EQ((double) instGroup.findKeyword("TransX"), 1.0);
  EXPECT_DOUBLE_EQ((double) instGroup.findKeyword("TransY"), 1.0);

  FileName matchedCubeName(instGroup.findKeyword("matchedCube"));
  FileName defaultCubeName(testCube->fileName());
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, matchedCubeName.name(), defaultCubeName.name());

  PvlGroup origInst = isisLabel->findGroup("OriginalInstrument", Pvl::Traverse);
  PvlGroup testCubeInst = testCube->label()->findGroup("Instrument", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupKeywordsEqual, origInst, testCubeInst); //REFACTOR

  std::unique_ptr<Histogram> hist (oCube.histogram(1));
  EXPECT_NEAR(hist->Average(), 127.46759871644132, .000001);
  EXPECT_NEAR(hist->Sum(), 41629898, .0001);
  EXPECT_EQ(hist->ValidPixels(), 326592);
  EXPECT_NEAR(hist->StandardDeviation(), 68.444806666131768, .0001);
}

TEST_F(DefaultCube, FunctionalTestNoprojSpecs) {
  std::ofstream specsStream;
  QString specsFileName = tempDir.path() + "/specs.pvl";
  specsStream.open(specsFileName.toStdString());
  specsStream << R"(
    Object = IdealInstrumentsSpecifications
      UserName     = ssides/sgstapleton
      Created      = 2006-12-14T10:10:49
      LastModified = 2019-06-27
      # 2019-11-05 Modified by ladoramkershner: Added Mariner 10
      
      # Group name and values will change once stabilized
      Group = "Clipper EIS 2025/EIS-NAC-RS"
        DetectorSamples = 4000
        DetectorLines = 2000
      End_Group

      Group = "Clipper EIS 2025/EIS-WAC-FC"
        DetectorSamples = 4000
        DetectorLines = 2000
      End_Group

      # Max offset from undistorted to distorted < 1
      Group = "HAYABUSA-2/ONC-T"
        DetectorSamples = 1025
        DetectorLines   = 1025
      End_Group

      # TBD (W1 images currently have very small data coverage)
      Group = "HAYABUSA-2/ONC-W1"
        DetectorSamples = 1025
        DetectorLines   = 1025
      End_Group

      # Max offset from undistorted to distorted: (2.15, 5.67)
      Group = "HAYABUSA-2/ONC-W2"
        DetectorSamples = 1027
        DetectorLines   = 1030
      End_Group

      Group = "MARS GLOBAL SURVEYOR/MOC-WA"
        DetectorSamples = 6000
      End_Group

      Group = "MARS RECONNAISSANCE ORBITER/CTX"
        DetectorSamples = 4991
        TransY = -0.39031635
        ItransS = 55.759479
      End_Group
      Group = "MARS GLOBAL SURVEYOR/MOC-WA"
        DetectorSamples = 6000
      End_Group

      Group = "MARS RECONNAISSANCE ORBITER/CTX"
        DetectorSamples = 4991
        TransY = -0.39031635
        ItransS = 55.759479
      End_Group

      Group = "MARS GLOBAL SURVEYOR/MOC-WA"
        DetectorSamples = 6000
      End_Group

      Group = "MARS RECONNAISSANCE ORBITER/CTX"
        DetectorSamples = 4991
        TransY = -0.39031635
        ItransS = 55.759479
      End_Group
      
      Group = "MARS RECONNAISSANCE ORBITER/HIRISE"
        DetectorSamples = 20000
    #    Use the average of red ccd's 4 & 5 for the offsets
        TransX = -92.9979
        ItransL = 7749.8250
      End_Group

      Group = "Messenger/MDIS-NAC"
        DetectorSamples = 1034
        DetectorLines = 1034
      End_Group

      Group = "Messenger/MDIS-WAC"
        DetectorSamples = 1034
        DetectorLines = 1034
      End_Group

      Group = "NEW HORIZONS/LEISA"
        DetectorSamples = 256
      End_Group

      Group = "TRACE GAS ORBITER/CaSSIS"
        DetectorSamples = 2048
        DetectorLines = 2048
      End_Group

      Group = "VIKING_ORBITER_1/VISUAL_IMAGING_SUBSYSTEM_CAMERA_B"
        DetectorSamples = 2000
        DetectorLines = 1000
      End_Group

      Group = "VIKING_ORBITER_1/VISUAL_IMAGING_SUBSYSTEM_CAMERA_A"
        DetectorSamples = 1250
        DetectorLines = 1150
      End_Group

      Group = "VIKING_ORBITER_2/VISUAL_IMAGING_SUBSYSTEM_CAMERA_B"
        DetectorSamples = 1250
        DetectorLines = 1150
      End_Group

      Group = "VIKING_ORBITER_2/VISUAL_IMAGING_SUBSYSTEM_CAMERA_A"
        DetectorSamples = 1250
        DetectorLines = 1150
      End_Group

      Group = "VOYAGER_1/NARROW_ANGLE_CAMERA"
        DetectorSamples = 1000
        DetectorLines = 1000
      End_Group

      Group = "VOYAGER_1/WIDE_ANGLE_CAMERA"
        DetectorSamples = 1000
        DetectorLines = 1000
      End_Group

      Group = "VOYAGER_2/NARROW_ANGLE_CAMERA"
        DetectorSamples = 1000
        DetectorLines = 1000
      End_Group

      Group = "VOYAGER_2/WIDE_ANGLE_CAMERA"
        DetectorSamples = 1000
        DetectorLines = 1000
      End_Group

      Group = "MARINER_10/M10_VIDICON_A"
        DetectorSamples = 832
        DetectorLines = 700
      End_Group

      Group = "MARINER_10/M10_VIDICON_B"
        DetectorSamples = 832
        DetectorLines = 700
      End_Group
    End_Object
    End
  )";
  specsStream.close();

  QString cubeFileName = tempDir.path() + "/output.cub";
  QVector<QString> args = {"to=" + cubeFileName, "specs=" + specsFileName};
  UserInterface options(APP_XML, args);

  noproj(testCube, NULL, options);

  Cube oCube(cubeFileName);
  Pvl *isisLabel = oCube.label();
  PvlGroup instGroup = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("SpacecraftName"), "IdealSpacecraft");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("InstrumentId"), "IdealCamera");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("TargetName"), "MARS");
  EXPECT_EQ((int) instGroup.findKeyword("SampleDetectors"), 2000);
  EXPECT_EQ((int) instGroup.findKeyword("LineDetectors"), 1000);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("InstrumentType"), "FRAMING");
  EXPECT_DOUBLE_EQ((double) instGroup.findKeyword("EphemerisTime"), -709401200.26114);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("StartTime"), "1977-07-09T20:05:51");
  EXPECT_EQ((int) instGroup.findKeyword("FocalPlaneXDependency"), 1);
  EXPECT_DOUBLE_EQ((double) instGroup.findKeyword("TransX"), 1.0);
  EXPECT_DOUBLE_EQ((double) instGroup.findKeyword("TransY"), 1.0);

  FileName matchedCubeName(instGroup.findKeyword("matchedCube"));
  FileName defaultCubeName(testCube->fileName());
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, matchedCubeName.name(), defaultCubeName.name());

  PvlGroup origInst = isisLabel->findGroup("OriginalInstrument", Pvl::Traverse);
  PvlGroup testCubeInst = testCube->label()->findGroup("Instrument", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupKeywordsEqual, origInst, testCubeInst); //REFACTOR

  std::unique_ptr<Histogram> hist (oCube.histogram(1));
  EXPECT_NEAR(hist->Average(), 127.53053767760592, .000001);
  EXPECT_NEAR(hist->Sum(), 153645476, .0001);
  EXPECT_EQ(hist->ValidPixels(), 1204774);
  EXPECT_NEAR(hist->StandardDeviation(), 68.420632943519294, .0001);
}

TEST_F(LineScannerCube, FunctionalTestNoprojLineScanner) {
  // Create KAGUYA/TC2 specs entry so that we can use the LineScannerCube fixture.
  std::ofstream specsStream;
  QString specsFileName = tempDir.path() + "/specs.pvl";
  specsStream.open(specsFileName.toStdString());
  specsStream << R"(
    Object = IdealInstrumentsSpecifications
      Group = "KAGUYA/TC2"
        DetectorSamples = 2000
        DetectorLines = 1000
      End_Group
    End_Object
    End
  )";
  specsStream.close();

  QString cubeFileName = tempDir.path() + "/output.cub";
  QVector<QString> args = {"to=" + cubeFileName, "specs=" + specsFileName};
  UserInterface options(APP_XML, args);

  noproj(testCube, NULL, options);

  Cube oCube(cubeFileName);
  Pvl *isisLabel = oCube.label();
  PvlGroup instGroup = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("SpacecraftName"), "IdealSpacecraft");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("InstrumentId"), "IdealCamera");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("TargetName"), "MOON");
  EXPECT_EQ((int) instGroup.findKeyword("SampleDetectors"), 2000);
  EXPECT_EQ((int) instGroup.findKeyword("LineDetectors"), 1);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("InstrumentType"), "LINESCAN");
  EXPECT_DOUBLE_EQ((double) instGroup.findKeyword("EphemerisTime"), 266722396.06431001);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instGroup.findKeyword("StartTime"), "2008-06-14T13:32:10.933207");
  EXPECT_EQ((int) instGroup.findKeyword("FocalPlaneXDependency"), 1);
  EXPECT_DOUBLE_EQ((double) instGroup.findKeyword("TransX"), -1.0);
  EXPECT_DOUBLE_EQ((double) instGroup.findKeyword("TransY"), -1.0);

  FileName matchedCubeName(instGroup.findKeyword("matchedCube"));
  FileName defaultCubeName(testCube->fileName());
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, matchedCubeName.name(), defaultCubeName.name());

  PvlGroup origInst = isisLabel->findGroup("OriginalInstrument", Pvl::Traverse);
  PvlGroup testCubeInst = testCube->label()->findGroup("Instrument", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupKeywordsEqual, origInst, testCubeInst); //REFACTOR

  std::unique_ptr<Histogram> hist (oCube.histogram(1));
  EXPECT_NEAR(hist->Average(), 26.259947527749951, .000001);
  EXPECT_NEAR(hist->Sum(), 78070.824000000604, .0001);
  EXPECT_EQ(hist->ValidPixels(), 2973);
  EXPECT_NEAR(hist->StandardDeviation(), 11.938337629048096, .0001);
}