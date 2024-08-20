#include <iostream>
#include <sstream>

#include <QIODevice>
#include <QTextStream>
#include <QStringList>
#include <QFile>

#include "TempFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Cube.h"
#include "Histogram.h"

#include "gmock/gmock.h"

#include "std2isis.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/std2isis.xml").expanded();

TEST_F(TempTestingFiles, FunctionalTestStd2isisDefault) {
  QString outCubeFileName = tempDir.path() + "/outTemp.cub";

  QVector<QString> args = {"from=data/stdFormatImages/rgb.png", "to="+outCubeFileName};

  UserInterface options(APP_XML, args);
  try {
    std2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  Cube outCube(outCubeFileName);
  Pvl *outLabel = outCube.label();

  PvlGroup dimensions = outLabel->findGroup("Dimensions", Pvl::Traverse);
  ASSERT_EQ((int)dimensions["Samples"], 126);
  ASSERT_EQ((int)dimensions["Lines"], 126);
  ASSERT_EQ((int)dimensions["Bands"], 3);

  PvlGroup pixels = outLabel->findGroup("Pixels", Pvl::Traverse);
  ASSERT_EQ(pixels["Type"][0], "Real");
  ASSERT_EQ(pixels["ByteOrder"][0], "Lsb");
  ASSERT_EQ((double)pixels["Base"], 0.0);
  ASSERT_EQ((double)pixels["Multiplier"], 1.0);

  PvlGroup bandbin = outLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["Name"][0], "Red");
  ASSERT_EQ(bandbin["Name"][1], "Green");
  ASSERT_EQ(bandbin["Name"][2], "Blue");

  std::unique_ptr<Histogram> hist (outCube.histogram());
  ASSERT_NEAR(hist->Average(), 113.11904761904762, .00001);
  ASSERT_EQ(hist->Sum(), 1795878);
  ASSERT_EQ(hist->ValidPixels(), 15876);
  ASSERT_NEAR(hist->StandardDeviation(), 97.569786532996, .0001);
}


TEST_F(TempTestingFiles, FunctionalTestStd2isisArgb) {
  QString outCubeFileName = tempDir.path() + "/outTemp.cub";

  QVector<QString> args = {"from=data/stdFormatImages/rgb.png", "to="+outCubeFileName, "mode=argb" };

  UserInterface options(APP_XML, args);
  try {
    std2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  Cube outCube(outCubeFileName);
  Pvl *outLabel = outCube.label();

  PvlGroup dimensions = outLabel->findGroup("Dimensions", Pvl::Traverse);
  ASSERT_EQ((int)dimensions["Samples"], 126);
  ASSERT_EQ((int)dimensions["Lines"], 126);
  ASSERT_EQ((int)dimensions["Bands"], 4);

  PvlGroup pixels = outLabel->findGroup("Pixels", Pvl::Traverse);
  ASSERT_EQ(pixels["Type"][0], "Real");
  ASSERT_EQ(pixels["ByteOrder"][0], "Lsb");
  ASSERT_EQ((double)pixels["Base"], 0.0);
  ASSERT_EQ((double)pixels["Multiplier"], 1.0);

  PvlGroup bandbin = outLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["Name"][0], "Red");
  ASSERT_EQ(bandbin["Name"][1], "Green");
  ASSERT_EQ(bandbin["Name"][2], "Blue");
  ASSERT_EQ(bandbin["Name"][3], "Alpha");

  std::unique_ptr<Histogram> hist (outCube.histogram());
  ASSERT_NEAR(hist->Average(), 113.11904761904762, .00001);
  ASSERT_EQ(hist->Sum(), 1795878);
  ASSERT_EQ(hist->ValidPixels(), 15876);
  ASSERT_NEAR(hist->StandardDeviation(), 97.569786532996, .0001);
}


TEST_F(TempTestingFiles, FunctionalTestStd2isisTiffGrayscale) {
  QString outCubeFileName = tempDir.path() + "/outTemp.cub";

  QVector<QString> args = {"from=data/stdFormatImages/rgb.tif", "to="+outCubeFileName, "mode=grayscale"};

  UserInterface options(APP_XML, args);
  try {
    std2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  Cube outCube(outCubeFileName);
  Pvl *outLabel = outCube.label();

  PvlGroup dimensions = outLabel->findGroup("Dimensions", Pvl::Traverse);
  ASSERT_EQ((int)dimensions["Samples"], 256);
  ASSERT_EQ((int)dimensions["Lines"], 192);
  ASSERT_EQ((int)dimensions["Bands"], 1);

  PvlGroup pixels = outLabel->findGroup("Pixels", Pvl::Traverse);
  ASSERT_EQ(pixels["Type"][0], "Real");
  ASSERT_EQ(pixels["ByteOrder"][0], "Lsb");
  ASSERT_EQ((double)pixels["Base"], 0.0);
  ASSERT_EQ((double)pixels["Multiplier"], 1.0);

  PvlGroup bandbin = outLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["Name"][0], "Gray");

  std::unique_ptr<Histogram> hist (outCube.histogram());
  ASSERT_NEAR(hist->Average(), 96.403951009114579, .00001);
  ASSERT_EQ(hist->Sum(), 4738447);
  ASSERT_EQ(hist->ValidPixels(), 49152);
  ASSERT_NEAR(hist->StandardDeviation(), 34.639987308489523, .0001);
}


TEST_F(TempTestingFiles, FunctionalTestStd2isisSpecial) {
  QString outCubeFileName = tempDir.path() + "/outTemp.cub";

  QVector<QString> args = {"from=data/stdFormatImages/rgb.png", "to="+outCubeFileName, "setnullrange=yes",
                           "nullmin=100", "nullmax=240", "sethrsrange=yes", "mode=grayscale",
	                       "hrsmin=200", "hrsmax=260", "setlrsrange=yes", "lrsmin=0",
                           "lrsmax=100" };

  UserInterface options(APP_XML, args);
  try {
    std2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  Cube outCube(outCubeFileName);
  Pvl *outLabel = outCube.label();

  PvlGroup dimensions = outLabel->findGroup("Dimensions", Pvl::Traverse);
  ASSERT_EQ((int)dimensions["Samples"], 126);
  ASSERT_EQ((int)dimensions["Lines"], 126);
  ASSERT_EQ((int)dimensions["Bands"], 1);

  PvlGroup pixels = outLabel->findGroup("Pixels", Pvl::Traverse);
  ASSERT_EQ(pixels["Type"][0], "Real");
  ASSERT_EQ(pixels["ByteOrder"][0], "Lsb");
  ASSERT_EQ((double)pixels["Base"], 0.0);
  ASSERT_EQ((double)pixels["Multiplier"], 1.0);

  PvlGroup bandbin = outLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["Name"][0], "Gray");

  std::unique_ptr<Histogram> hist (outCube.histogram());

  ASSERT_EQ(hist->LrsPixels(), 5983);
  ASSERT_EQ(hist->HrsPixels(), 957);
  ASSERT_EQ(hist->NullPixels(), 8936);
  ASSERT_NEAR(hist->Average(), -1.7976931348623149e+308, .00001);
  ASSERT_EQ(hist->Sum(), 0);
  ASSERT_EQ(hist->ValidPixels(), 0);
  ASSERT_NEAR(hist->StandardDeviation(), -1.7976931348623149e+308, .0001);
}


TEST_F(TempTestingFiles, FunctionalTestStd2isisJp2) {
  QString outCubeFileName = tempDir.path() + "/outTemp.cub";

  QVector<QString> args = {"from=data/stdFormatImages/rgb.jp2", "to="+outCubeFileName };

  UserInterface options(APP_XML, args);
  try {
    std2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  Cube outCube(outCubeFileName);
  Pvl *outLabel = outCube.label();

  PvlGroup dimensions = outLabel->findGroup("Dimensions", Pvl::Traverse);
  ASSERT_EQ((int)dimensions["Samples"], 126);
  ASSERT_EQ((int)dimensions["Lines"], 126);
  ASSERT_EQ((int)dimensions["Bands"], 3);

  PvlGroup pixels = outLabel->findGroup("Pixels", Pvl::Traverse);
  ASSERT_EQ(pixels["Type"][0], "Real");
  ASSERT_EQ(pixels["ByteOrder"][0], "Lsb");
  ASSERT_EQ((double)pixels["Base"], 0.0);
  ASSERT_EQ((double)pixels["Multiplier"], 1.0);

  PvlGroup bandbin = outLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["Name"][0], "Red");
  ASSERT_EQ(bandbin["Name"][1], "Green");
  ASSERT_EQ(bandbin["Name"][2], "Blue");

  std::unique_ptr<Histogram> hist (outCube.histogram());

  ASSERT_NEAR(hist->Average(), 113.12452758881331, .00001);
  ASSERT_EQ(hist->Sum(), 1795965);
  ASSERT_EQ(hist->ValidPixels(), 15876);
  ASSERT_NEAR(hist->StandardDeviation(), 97.354405991298336, .0001);
}
