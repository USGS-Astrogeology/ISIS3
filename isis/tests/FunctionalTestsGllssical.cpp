#include "CameraFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "gllssical.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/gllssical.xml").expanded();

TEST_F(GalileoSsiCube, FunctionalTestGllssicalDefault) {
  // tempDir exists if the fixture subclasses TempTestingFiles, which most do
  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QVector<QString> args = {"from="+ testCube->fileName(),  "to="+outCubeFileName};

  UserInterface options(APP_XML, args);
  try {
    gllssical(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName, "r");

  PvlGroup radGroup = oCube.label()->findObject("IsisCube").findGroup("RadiometricCalibration");

  EXPECT_EQ(radGroup.findKeyword("DarkCurrentFile")[0], "$galileo/calibration/darkcurrent/2f8.dc04.cub");
  EXPECT_EQ(radGroup.findKeyword("GainFile")[0], "$galileo/calibration/gain/redf.cal04.cub");
  EXPECT_EQ(radGroup.findKeyword("ShutterFile")[0], "$galileo/calibration/shutter/calibration.so02F.cub");
  EXPECT_EQ(radGroup.findKeyword("OutputUnits")[0], "I/F");
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("ScaleFactor"), 1.0);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("S1"), 0.005155);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("RSUN"), 0.27217458506088);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("Scale"), 1.0);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("GC"), 9.771);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("GG"), 9.771);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("IOF-SCALE0"), 3.81877269502043e-04);

  Histogram *oCubeStats = oCube.histogram();

  EXPECT_DOUBLE_EQ(oCubeStats->Average(), 0.042984976415343983);
  EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 8691.5622311825537);
  EXPECT_DOUBLE_EQ(oCubeStats->ValidPixels(), 202200);
  EXPECT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 0.024836176853261116);
}


TEST_F(GalileoSsiCube, FunctionalTestGllssicalClear) {
  PvlGroup &bandBin = testCube->label()->findObject("IsisCube").findGroup("BandBin");
  std::istringstream bss(R"(
    Group = BandBin
      FilterName   = CLEAR
      FilterNumber = 0
      Center       = 0.611 <micrometers>
      Width        = .44 <micrometers>
    End_Group
  )");

  PvlGroup newBandBin;
  bss >> newBandBin;
  bandBin = newBandBin;
  testCube->reopen();

  // tempDir exists if the fixture subclasses TempTestingFiles, which most do
  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QVector<QString> args = {"from="+ testCube->fileName(),  "to="+outCubeFileName};

  UserInterface options(APP_XML, args);
  try {
    gllssical(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName, "r");

  PvlGroup radGroup = oCube.label()->findObject("IsisCube").findGroup("RadiometricCalibration");

  EXPECT_EQ(radGroup.findKeyword("DarkCurrentFile")[0], "$galileo/calibration/darkcurrent/2f8.dc04.cub");
  EXPECT_EQ(radGroup.findKeyword("GainFile")[0], "$galileo/calibration/gain/clrf.cal04.cub");
  EXPECT_EQ(radGroup.findKeyword("ShutterFile")[0], "$galileo/calibration/shutter/calibration.so02F.cub");
  EXPECT_EQ(radGroup.findKeyword("OutputUnits")[0], "I/F");
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("ScaleFactor"), 1.0);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("S1"), 0.0043579999999999999);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("RSUN"), 0.27217458506088);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("Scale"), 1.0);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("GC"), 9.771);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("GG"), 9.771);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("IOF-SCALE0"), 0.00032283630271385199);

  Histogram *oCubeStats = oCube.histogram();

  EXPECT_DOUBLE_EQ(oCubeStats->Average(), 0.0069728166790276117);
  EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 1409.4502994152463);
  EXPECT_DOUBLE_EQ(oCubeStats->ValidPixels(), 202135);
  EXPECT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 0.016192023893469475);
}


TEST_F(GalileoSsiCube, FunctionalTestGllssicalRadiance) {
  // tempDir exists if the fixture subclasses TempTestingFiles, which most do
  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QVector<QString> args = {"from="+ testCube->fileName(),  "to="+outCubeFileName,
                           "UNITS=RADIANCE", "SCALE=0.0001", "BITWEIGHTING=true"};

  UserInterface options(APP_XML, args);
  try {
    gllssical(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName, "r");

  PvlGroup radGroup = oCube.label()->findObject("IsisCube").findGroup("RadiometricCalibration");

  EXPECT_EQ(radGroup.findKeyword("DarkCurrentFile")[0], "$galileo/calibration/darkcurrent/2f8.dc04.cub");
  EXPECT_EQ(radGroup.findKeyword("GainFile")[0], "$galileo/calibration/gain/redf.cal04.cub");
  EXPECT_EQ(radGroup.findKeyword("ShutterFile")[0], "$galileo/calibration/shutter/calibration.so02F.cub");
  EXPECT_EQ(radGroup.findKeyword("OutputUnits")[0], "Radiance");
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("ScaleFactor"), 0.0001);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("Scale"), 0.0001);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("GC"), 9.771);
  EXPECT_DOUBLE_EQ((double)radGroup.findKeyword("GG"), 9.771);

  Histogram *oCubeStats = oCube.histogram();

  EXPECT_DOUBLE_EQ(oCubeStats->Average(), 10486394.731919922);
  EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 2130835409526.1282);
  EXPECT_DOUBLE_EQ(oCubeStats->ValidPixels(), 203200);
  EXPECT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 6119860.4426242532);
}

