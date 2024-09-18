#include "Brick.h"
#include "CubeFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Statistics.h"

#include "apollocal.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/apollocal.xml").expanded());

TEST_F(ApolloCube, FunctionalTestApollocalDefault) {
  QTemporaryDir prefix;

  QString outCubeFileName = prefix.path()+"/outTEMP.cub";
  QVector<QString> args = {"to="+outCubeFileName};

  UserInterface options(APP_XML, args);

  try {
    apollocal(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Call failed, Unable to process cube: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString(), "r");

  // Check a region with both Null and non-Null data
  Brick brick(reseauSize + 10,reseauSize + 10,1,oCube.pixelType());
  int baseSamp = (int)(reseaus[0].first+0.5) - (reseauSize/2);
  int baseLine = (int)(reseaus[0].second+0.5) - (reseauSize/2);
  brick.SetBasePosition(baseSamp,baseLine,1);
  oCube.read(brick);
  Statistics reseauStats;
  reseauStats.AddData(&brick[0], brick.size());

  EXPECT_NEAR(reseauStats.Average(), -2864.497, 0.001);
  EXPECT_NEAR(reseauStats.Sum(), -30389453.463, 0.001);
  EXPECT_EQ(reseauStats.ValidPixels(), 10609);
  EXPECT_EQ(reseauStats.NullPixels(), 2160);
  EXPECT_NEAR(reseauStats.StandardDeviation(), 21.534, 0.001);
}
