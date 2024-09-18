#include <QTemporaryFile>
#include <QString>
#include <iostream>

#include "Cube.h"
#include "TempFixtures.h"
#include "TestUtilities.h"
#include "ImageImporter.h"
#include "Histogram.h"

#include "gmock/gmock.h"

using namespace Isis;

// Expected fail on MacOS
TEST_F(TempTestingFiles, DISABLED_UnitTestImageImporterTestJpeg) {
  FileName inputName("data/stdFormatImages/rgb.jpg");

  ImageImporter *importer = ImageImporter::fromFileName(inputName);
  FileName outputName(tempDir.path().toStdString() + "/out.cub");
  importer->import(outputName);
  delete importer;

  Cube outCube(outputName.expanded());
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


TEST_F(TempTestingFiles, UnitTestImageImporterStd2IsisTiffRgb) {
  FileName inputName("data/stdFormatImages/rgb.tif");

  ImageImporter *importer = ImageImporter::fromFileName(inputName);
  FileName outputName(tempDir.path().toStdString() + "/out.cub");
  importer->import(outputName);
  delete importer;

  Cube outCube(outputName.expanded());
  Pvl *outLabel = outCube.label();

  PvlGroup dimensions = outLabel->findGroup("Dimensions", Pvl::Traverse);
  ASSERT_EQ((int)dimensions["Samples"], 256);
  ASSERT_EQ((int)dimensions["Lines"], 192);
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
  ASSERT_NEAR(hist->Average(), 88.4844970703125, .00001);
  ASSERT_EQ(hist->Sum(), 4349190);
  ASSERT_EQ(hist->ValidPixels(), 49152);
  ASSERT_NEAR(hist->StandardDeviation(), 64.032045970490174, .0001);
}


TEST_F(TempTestingFiles, UnitTestImageImporterStd2IsisJp2) {
  FileName inputName("data/stdFormatImages/rgb.jp2");

  ImageImporter *importer = ImageImporter::fromFileName(inputName);
  FileName outputName(tempDir.path().toStdString() + "/out.cub");
  importer->import(outputName);
  delete importer;

  Cube outCube(outputName.expanded());
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

