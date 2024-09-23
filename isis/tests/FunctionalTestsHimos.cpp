#include <QTemporaryDir>

#include "himos.h"
#include "CameraFixtures.h"
#include "FileList.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/himos.xml").expanded());

TEST_F(MroHiriseCube, FunctionalTestHimosDefault) {

  FileName mosFileList(tempDir.path().toStdString() + "/himosFileList.lis");

  FileList * cubeList = new FileList();
  cubeList->append(dejitteredCube.fileName().toStdString());
  cubeList->write(mosFileList);

  QVector<QString> args = {"from=" + QString::fromStdString(mosFileList.toString()),
                           "to=" + tempDir.path() + "/outputMos.cub"};

  UserInterface options(APP_XML, args);
  try {
   himos(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to create mosaic image: " <<  e.toString().c_str() << std::endl;
  }
  Cube outputMos(options.GetCubeName("TO").toStdString());
  PvlObject inputCubeLabel = dejitteredCube.label()->findObject("IsisCube");
  PvlObject outputCubeLabel = outputMos.label()->findObject("IsisCube");
  PvlGroup dimensions = outputCubeLabel.findObject("Core").findGroup("Dimensions");
  PvlGroup pixels = outputCubeLabel.findObject("Core").findGroup("Pixels");

  EXPECT_EQ(int(dimensions["Samples"]), 21);
  EXPECT_EQ(int(dimensions["Lines"]), 91);
  EXPECT_EQ(int(dimensions["Bands"]), 3);

  EXPECT_EQ(pixels["Type"][0], "SignedWord");
  EXPECT_EQ(pixels["ByteOrder"][0], "Lsb");
  EXPECT_EQ(double(pixels["Base"]), 1.4996565881653);
  EXPECT_EQ(double(pixels["Multiplier"]), 4.57882446313283e-05);

  PvlGroup inputMappingGroup = inputCubeLabel.findGroup("Mapping");
  PvlGroup outputMappingGroup = outputCubeLabel.findGroup("Mapping");
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, inputMappingGroup, outputMappingGroup);

  PvlGroup mos = outputCubeLabel.findGroup("Mosaic");
  EXPECT_EQ(mos["ProductId"][0], "TRA_000823_1720_BLUEGREEN");
  EXPECT_EQ(mos["SourceProductId"][0], "TRA_000823_1720_RED4_0");
  EXPECT_EQ(mos["SourceProductId"][1], "TRA_000823_1720_RED4_1");
  EXPECT_EQ(mos["StartTime"][0], "2006-09-29T15:16:33.385");
  EXPECT_EQ(mos["SpacecraftClockStartCount"][0], "844010212:12516");
  EXPECT_EQ(mos["StopTime"][0], "2006-09-29T15:16:35.036");
  EXPECT_EQ(mos["SpacecraftClockStopCount"][0], "844010213:55196");

  EXPECT_NEAR(Isis::toDouble(mos["IncidenceAngle"][0]), 59.687930340662, 1e-6);
  EXPECT_NEAR(Isis::toDouble(mos["EmissionAngle"][0]), 0.091672512439956, 1e-6);
  EXPECT_NEAR(Isis::toDouble(mos["PhaseAngle"][0]), 59.597812369363, 1e-6);
  EXPECT_NEAR(Isis::toDouble(mos["LocalTime"][0]), 15.486088288555, 1e-6);
  EXPECT_NEAR(Isis::toDouble(mos["SolarLongitude"][0]), 113.54746578654, 1e-6);
  EXPECT_NEAR(Isis::toDouble(mos["SubSolarAzimuth"][0]), 212.41484032558 , 1e-6);
  EXPECT_NEAR(Isis::toDouble(mos["NorthAzimuth"][0]), 270.00024569628, 1e-6);

  EXPECT_EQ(mos["cpmmTdiFlag"][5], "128");
  EXPECT_EQ(mos["cpmmSummingFlag"][5], "1");
  EXPECT_EQ(mos["SpecialProcessingFlag"][5], "NOMINAL");
}

TEST_F(MroHiriseCube, FunctionalTestHimosError) {

  FileName mosFileList(tempDir.path().toStdString() + "/himosFileList.lis");

  FileList * cubeList = new FileList();
  cubeList->append(testCube->fileName().toStdString());
  cubeList->write(mosFileList);

  QVector<QString> args = {"from=" + QString::fromStdString(mosFileList.toString()),
                           "to=" + tempDir.path() + "/outputMos.cub"};

  UserInterface options(APP_XML, args);
  try {
   himos(options);
   FAIL() << "Should not have been able to create mosaic: " << options.GetCubeName("TO").toStdString().c_str() << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("was NOT created"));
  }
}

TEST_F(MroHiriseCube, FunctionalTestHimosMismatchObs) {
  CubeAttributeOutput cubeAtts(FileName(dejitteredCube.fileName().toStdString()));
  Cube *copyDejitteredCube = dejitteredCube.copy(tempDir.path().toStdString() + "/copyDejitteredCube.cub", cubeAtts);
  copyDejitteredCube->label()->findObject("IsisCube").findGroup("Archive")["ObservationId"] = "Banana";
  copyDejitteredCube->reopen("rw");

  FileName mosFileList(tempDir.path().toStdString() + "/himosFileList.lis");

  FileList * cubeList = new FileList();
  cubeList->append(dejitteredCube.fileName().toStdString());
  cubeList->append(copyDejitteredCube->fileName().toStdString());
  cubeList->write(mosFileList);

  QVector<QString> args = {"from=" + QString::fromStdString(mosFileList.toString()),
                           "to=" + tempDir.path() + "/outputMos.cub"};

  UserInterface options(APP_XML, args);
  try {
   himos(options);
   FAIL() << "Should not have been able to create mosaic: " << options.GetCubeName("TO").toStdString().c_str() << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("was NOT created"));
  }
}

TEST_F(MroHiriseCube, FunctionalTestHimosMismatchFilter) {
  CubeAttributeOutput cubeAtts(FileName(dejitteredCube.fileName().toStdString()));
  Cube *copyDejitteredCube = dejitteredCube.copy(tempDir.path().toStdString() + "/copyDejitteredCube.cub", cubeAtts);
  copyDejitteredCube->label()->findObject("IsisCube").findGroup("BandBin")["Name"] = "Red";
  copyDejitteredCube->reopen("rw");

  FileName mosFileList(tempDir.path().toStdString() + "/himosFileList.lis");

  FileList * cubeList = new FileList();
  cubeList->append(dejitteredCube.fileName().toStdString());
  cubeList->append(copyDejitteredCube->fileName().toStdString());
  cubeList->write(mosFileList);

  QVector<QString> args = {"from=" + QString::fromStdString(mosFileList.toString()),
                           "to=" + tempDir.path() + "/outputMos.cub"};

  UserInterface options(APP_XML, args);
  try {
   himos(options);
   FAIL() << "Should not have been able to create mosaic: " << options.GetCubeName("TO").toStdString().c_str() << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("was NOT created"));
  }
}
