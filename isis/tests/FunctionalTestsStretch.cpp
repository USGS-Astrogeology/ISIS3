#include "CubeFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"
#include "UserInterface.h"

#include "stretch_app.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString STRETCH_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/stretch.xml").expanded());

// case 1
TEST_F(SpecialSmallCube, FunctionalTestStretchDefault) {
  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTemp.cub";
  QVector<QString> args = {"to="+outCubeFileName, "null=500", "lis=700", "lrs=800", "his=900", "hrs=1000"};
  QString pairs = "0:255 255:0";

  UserInterface options(STRETCH_XML, args);
  Pvl log;
  try {
    stretch(testCube, pairs, options, &log);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString(), "r");

  Histogram *oCubeStats = oCube.histogram();

  EXPECT_DOUBLE_EQ(oCubeStats->Average(), 505.25);
  EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 50525);
  EXPECT_DOUBLE_EQ(oCubeStats->ValidPixels(), 100);
  EXPECT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 302.16673352386897);
}

// case 2, changes other special pixels to other special pixels
TEST_F(SpecialSmallCube, FunctionalTestStretchSwitchSpecial) {
  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTemp.cub";
  QVector<QString> args = {"to="+outCubeFileName, "null=hrs", "lis=NULL", "lrs=Lis", "his=lRs", "hrs=HiS"};
  QString pairs = "0:255 255:0";

  UserInterface options(STRETCH_XML, args);
  Pvl log;
  try {
    stretch(testCube, pairs, options, &log);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString(), "r");

  Histogram *oCubeStats = oCube.histogram();

  EXPECT_DOUBLE_EQ(oCubeStats->Average(), 230.5);
  EXPECT_DOUBLE_EQ(oCubeStats->Sum(), 11525);
  EXPECT_DOUBLE_EQ(oCubeStats->ValidPixels(), 50);
  EXPECT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 14.577379737113251);
}
