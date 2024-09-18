#include "CameraFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "LineManager.h"
#include "Histogram.h"

#include "hicolormos.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/hicolormos.xml").expanded());

TEST_F(MroHiriseCube, FunctionalTestHicolormosDefault) {
  QString outCubeFileName = tempDir.path() + "/outTEMP.cub";
  QVector<QString> args = {"to="+outCubeFileName};

  UserInterface options(APP_XML, args);
  try {
    hicolormos(&dejitteredCube, nullptr, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to process HRISE image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString());
  Pvl *label = oCube.label();
  PvlGroup group = label->findObject("IsisCube").findGroup("Mosaic");

  EXPECT_NEAR((double)group.findKeyword("IncidenceAngle"),  59.687930340662, 0.0001);
  EXPECT_NEAR((double)group.findKeyword("EmissionAngle"),   0.0916725124399, 0.0001);
  EXPECT_NEAR((double)group.findKeyword("PhaseAngle"),      59.597812369363, 0.0001);
  EXPECT_NEAR((double)group.findKeyword("LocalTime"),       15.486088288555, 0.0001);
  EXPECT_NEAR((double)group.findKeyword("SolarLongitude"),  113.54746578654, 0.0001);
  EXPECT_NEAR((double)group.findKeyword("SubSolarAzimuth"), 212.41484032558, 0.0001);
  EXPECT_NEAR((double)group.findKeyword("NorthAzimuth"),    270.00024569624, 0.0001);

  QStringList cppmmTdiFlag = {"Null","Null","Null","Null","128", "128", "128","Null","Null","Null","Null","Null"};
  QStringList cpmmSummingFlag = {"Null","Null","Null","Null","2", "1", "2","Null","Null","Null","Null","Null"};
  QStringList specialProcessingFlag = {"Null","Null","Null","Null","NOMINAL", "NOMINAL", "CUBENORM","Null","Null","Null","Null","Null"};

  PvlKeyword kw = group.findKeyword("cpmmTdiFlag");
  for (int i = 0; i < cppmmTdiFlag.size(); i++) {
    EXPECT_PRED_FORMAT2(AssertStringsEqual, cppmmTdiFlag[i].toStdString(), kw[i]);
  }

  kw = group.findKeyword("cpmmSummingFlag");
  for (int i = 0; i < cppmmTdiFlag.size(); i++) {
    EXPECT_PRED_FORMAT2(AssertStringsEqual, cpmmSummingFlag[i].toStdString(), kw[i]);
  }

  kw = group.findKeyword("SpecialProcessingFlag");
  for (int i = 0; i < cppmmTdiFlag.size(); i++) {
    EXPECT_PRED_FORMAT2(AssertStringsEqual, specialProcessingFlag[i].toStdString(), kw[i]);
  }

  Histogram *oCubeStats = oCube.histogram();

  ASSERT_DOUBLE_EQ(oCubeStats->Average(), 0.99336582359379422);
  ASSERT_DOUBLE_EQ(oCubeStats->Sum(),     802.63958546378569);
  ASSERT_EQ(oCubeStats->ValidPixels(),    808);
  ASSERT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 0.079236816283481101);
}
