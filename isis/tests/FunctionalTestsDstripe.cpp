#include <QTemporaryDir>

#include "Fixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "dstripe.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/dstripe.xml").expanded();

TEST(Dstripe, FunctionalTestDstripeName) {
  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTemp.cub";
  QVector<QString> args = {"from=data/dstripe/dstripe-default.cub",
                           "to=" + outCubeFileName};

  UserInterface options(APP_XML, args);
  try {
    dstripe(options);
  } catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube cube(outCubeFileName);

  std::unique_ptr<Histogram> hist (cube.histogram(0));
  EXPECT_NEAR(hist->Average(), 99.405224835973755, .000001);
  EXPECT_NEAR(hist->Sum(), 19877864, .000001);
  EXPECT_EQ(hist->ValidPixels(), 199968);
  EXPECT_NEAR(hist->StandardDeviation(), 18.246570191788862, .000001);
}

TEST(Dstripe, FunctionalTestDstripeParallel) {
  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTemp.cub";
  QVector<QString> args = {"from=data/dstripe/dstripe-parallel.cub",
                           "mode=vert", "vlnl=51", "vhns=51",
                           "to=" + outCubeFileName};

  UserInterface options(APP_XML, args);
  try {
    dstripe(options);
  } catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube cube(outCubeFileName);

  std::unique_ptr<Histogram> hist (cube.histogram(0));
  EXPECT_NEAR(hist->Average(), 2.2391462548711162e-05, .000001);
  EXPECT_NEAR(hist->Sum(), 0.71652680155875714, .000001);
  EXPECT_EQ(hist->ValidPixels(), 32000);
  EXPECT_NEAR(hist->StandardDeviation(), 3.2450333566072249e-06, .000001);
}