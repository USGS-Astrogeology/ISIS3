#include <iostream>
#include <QTemporaryFile>

#include "cam2cam.h"

#include "Cube.h"
#include "CubeAttribute.h"
#include "IException.h"
#include "PixelType.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "TestUtilities.h"
#include "FileName.h"
#include "ProjectionFactory.h"
#include "CameraFixtures.h"
#include "Mocks.h"

using namespace Isis;
using ::testing::Return;
using ::testing::AtLeast;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/cam2cam.xml").expanded());

TEST_F(DefaultCube, FunctionalTestCam2camNoChange) {

  QVector<QString> args = {"to="+tempDir.path()+"/Cam2CamNoChange.cub", "INTERP=BILINEAR"};
  UserInterface ui(APP_XML, args);

  testCube->reopen("r");
  QString inFile = testCube->fileName();
  Cube mcube(inFile.toStdString(),"r");

  cam2cam(testCube, &mcube, ui);

  Cube icube(inFile.toStdString());
  PvlGroup icubeInstrumentGroup = icube.label()->findGroup("Instrument", Pvl::Traverse);

  Cube ocube(tempDir.path().toStdString()+"/Cam2CamNoChange.cub");
  PvlGroup ocubeInstrumentGroup = ocube.label()->findGroup("Instrument", Pvl::Traverse);

  ASSERT_EQ(icubeInstrumentGroup.findKeyword("SpacecraftName"), ocubeInstrumentGroup.findKeyword("SpacecraftName"));
  ASSERT_EQ(icubeInstrumentGroup.findKeyword("InstrumentId"), ocubeInstrumentGroup.findKeyword("InstrumentID"));
  ASSERT_EQ(icubeInstrumentGroup.findKeyword("TargetName"), ocubeInstrumentGroup.findKeyword("TargetName"));
  ASSERT_EQ(icubeInstrumentGroup.findKeyword("StartTime"), ocubeInstrumentGroup.findKeyword("StartTime"));
  ASSERT_EQ(icubeInstrumentGroup.findKeyword("ExposureDuration"), ocubeInstrumentGroup.findKeyword("ExposureDuration"));
  ASSERT_EQ(icubeInstrumentGroup.findKeyword("SpacecraftClockCount"), ocubeInstrumentGroup.findKeyword("SpacecraftClockCount"));
  ASSERT_EQ(icubeInstrumentGroup.findKeyword("FloodModeId"), ocubeInstrumentGroup.findKeyword("FloodModeId"));
  ASSERT_EQ(icubeInstrumentGroup.findKeyword("GainModeId"), ocubeInstrumentGroup.findKeyword("GainModeId"));
  ASSERT_EQ(icubeInstrumentGroup.findKeyword("OffsetModeId"), ocubeInstrumentGroup.findKeyword("OffsetModeId"));
}
