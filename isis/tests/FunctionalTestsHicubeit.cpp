#include <QTemporaryDir>

#include "hicubeit.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/hicubeit.xml").expanded());

TEST(Hicubeit, Default) {
  QTemporaryDir prefix;
  QString outFileName = prefix.path() + "/out.cub";
  QVector<QString> args = { "BG= data/hicubeit/BG.cub",
                            "IR= data/hicubeit/IR.cub",
                            "RE= data/hicubeit/RE.cub",
                            "TO=" + outFileName };
  UserInterface options(APP_XML, args);

  try {
    hicubeit(options);
  }
  catch (IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  Cube outCube(outFileName.toStdString());
  Pvl *outLabel = outCube.label();

  PvlGroup dimensions = outLabel->findGroup("Dimensions", Pvl::Traverse);
  PvlGroup bandbin = outLabel->findGroup("BandBin", Pvl::Traverse);

  EXPECT_EQ(bandbin["Name"][0], "NearInfrared");
  EXPECT_EQ(bandbin["Name"][1], "Red");
  EXPECT_EQ(bandbin["Name"][2], "BlueGreen");

  EXPECT_EQ(int(dimensions["Samples"]), 2048);
  EXPECT_EQ(int(dimensions["Lines"]), 10);
  EXPECT_EQ(int(dimensions["Bands"]), 3);

  std::unique_ptr<Histogram> histA (outCube.histogram(0));
  EXPECT_NEAR(histA->Average(), 1.01418, .00001);
  EXPECT_NEAR(histA->Sum(), 51915.7, .1);
  EXPECT_EQ(histA->ValidPixels(), 51190);
  EXPECT_NEAR(histA->StandardDeviation(), 0.0966599, .00001);

  std::unique_ptr<Histogram> histB (outCube.histogram(1));
  EXPECT_NEAR(histB->Average(), 0.960096, .00001);
  EXPECT_NEAR(histB->Sum(), 9831.38, .1);
  EXPECT_EQ(histB->ValidPixels(), 10240);
  EXPECT_NEAR(histB->StandardDeviation(), 0.0626452, .00001);

  std::unique_ptr<Histogram> histC (outCube.histogram(2));
  EXPECT_NEAR(histC->Average(), 1.03663, .00001);
  EXPECT_NEAR(histC->Sum(), 21230.1, .1);
  EXPECT_EQ(histC->ValidPixels(), 20480);
  EXPECT_NEAR(histC->StandardDeviation(), 0.112095, .00001);
}

