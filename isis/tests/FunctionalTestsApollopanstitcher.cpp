#include <QTemporaryDir>
#include <memory>

#include "apollopanstitcher.h"
#include "Fixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"
#include "Endian.h"
#include "PixelType.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "TempFixtures.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/apollopanstitcher.xml").expanded());

TEST_F(TempTestingFiles, FunctionalTestApollopanstitcherDefault) {
  QVector<QString> args = {"file_base=$ISISTESTDATA/isis/src/apollo/apps/apollopanstitcher/tsts/default/input/AS15_P_0177R10",
                           "to=" + tempDir.path() + "/reduced8.cub",
                           "microns=50"};

  UserInterface options(APP_XML, args);
  try {
   apolloPanStitcher(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to stitcher apollo images: " <<  e.toString().c_str() << std::endl;
  }
  Cube outputCube(options.GetCubeName("TO").toStdString());

  std::unique_ptr<Histogram> hist (outputCube.histogram());

  EXPECT_DOUBLE_EQ(hist->Average(), 53214.457630315941);
  EXPECT_DOUBLE_EQ(hist->Sum(), 3243279908182.748);
  EXPECT_EQ(hist->ValidPixels(), 60947345);
  EXPECT_DOUBLE_EQ(hist->StandardDeviation(), 20175.877734537076);
}
