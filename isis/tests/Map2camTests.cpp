#include <iostream>
#include <QTemporaryFile>

#include "map2cam.h"

#include "Cube.h"
#include "CubeAttribute.h"
#include "PixelType.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "TestUtilities.h"
#include "FileName.h"

#include "CameraFixtures.h"

#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/map2cam.xml").expanded());

TEST_F(DefaultCube, FunctionalTestMap2camTest) {
  QVector<QString> args = {"from="+projTestCube->fileName(), "match="+testCube->fileName(), "to="+tempDir.path()+"/level1.cub"};
  UserInterface ui(APP_XML, args);

  map2cam_f(ui);
  Cube ocube(tempDir.path().toStdString()+"/level1.cub");

  ASSERT_TRUE(ocube.label()->findObject("IsisCube").hasGroup("Kernels"));
  ASSERT_FALSE(ocube.label()->findObject("IsisCube").hasGroup("Mapping"));
}
