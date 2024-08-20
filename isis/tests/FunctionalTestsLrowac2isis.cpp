#include <QTemporaryDir>

#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "lrowac2isis.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/lrowac2isis.xml").expanded();

TEST(Lrowac2isisTests, FunctionalTestLrowac2isisDefault) {
  QTemporaryDir prefix;

  QString cubeFileName = prefix.path() + "/lrowac2isisTEMP.cub";
  QVector<QString> args = {"from=data/lrowac2isis/wac0000a1c4_cropped.img",
                           "to=" + cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    lrowac2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LROWAC image " << e.toString().toStdString().c_str() << std::endl;
  }

  // UV Even Cube
  QString cubeFileUvEven = prefix.path() + "/lrowac2isisTEMP.uv.even.cub";
  Cube uvEvenCube(cubeFileUvEven);
  Pvl *uvEvenLabel = uvEvenCube.label();

  // Dimensions group
  EXPECT_EQ(uvEvenCube.sampleCount(), 128);
  EXPECT_EQ(uvEvenCube.lineCount(), 5);
  EXPECT_EQ(uvEvenCube.bandCount(), 2);

  // Pixels group
  EXPECT_EQ(PixelTypeName(uvEvenCube.pixelType()), "Real");
  EXPECT_DOUBLE_EQ(uvEvenCube.base(), 0.0);
  EXPECT_DOUBLE_EQ(uvEvenCube.multiplier(), 1.0);

  // Instrument Group
  PvlGroup &uvEvenInst = uvEvenLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(uvEvenInst["SpacecraftName"][0], "LUNAR RECONNAISSANCE ORBITER");
  EXPECT_EQ(uvEvenInst["InstrumentId"][0], "WAC-UV");
  EXPECT_EQ(uvEvenInst["TargetName"][0], "Moon");
  EXPECT_EQ(uvEvenInst["StartTime"][0], "2009-09-15T07:27:49.230000");
  EXPECT_EQ(uvEvenInst["StopTime"][0], "2009-09-15T07:30:19.542000");
  EXPECT_EQ(uvEvenInst["MissionPhaseName"][0], "COMMISSIONING");
  EXPECT_EQ(std::stod(uvEvenInst["BeginTemperatureFpa"][0]), -1.66529297828674);
  EXPECT_EQ(std::stod(uvEvenInst["MiddleTemperatureFpa"][0]), -1.12489998340607);
  EXPECT_EQ(std::stod(uvEvenInst["EndTemperatureFpa"][0]), -0.669131994247437);
  EXPECT_EQ(std::stod(uvEvenInst["BeginTemperatureScs"][0]), 10.8307619094849);
  EXPECT_EQ(std::stod(uvEvenInst["MiddleTemperatureScs"][0]), 10.914568901062);
  EXPECT_EQ(std::stod(uvEvenInst["EndTemperatureScs"][0]), 10.9736194610596);
  EXPECT_EQ(uvEvenInst["Mode"][0], "0");
  EXPECT_EQ(uvEvenInst["DataFlipped"][0], "No");
  EXPECT_EQ(uvEvenInst["ColorOffset"][0], "0");
  EXPECT_EQ(uvEvenInst["Decompanded"][0], "Yes");
  EXPECT_EQ(uvEvenInst["Framelets"][0], "Even");
  EXPECT_EQ(uvEvenInst["NumFramelets"][0], "1");
  EXPECT_EQ(uvEvenInst["InstrumentModeId"][0], "COLOR");

  // Bandbin Group
  PvlGroup &uvEvenBandbin = uvEvenLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(uvEvenBandbin["FilterNumber"][0], "1");
  EXPECT_EQ(uvEvenBandbin["FilterNumber"][1], "2");
  EXPECT_EQ(uvEvenBandbin["Center"][0], "321");
  EXPECT_EQ(uvEvenBandbin["Center"][1], "360");
  EXPECT_EQ(uvEvenBandbin["Width"][0], "32");
  EXPECT_EQ(uvEvenBandbin["Width"][1], "15");


  std::unique_ptr<Histogram> uvEvenHist (uvEvenCube.histogram());

  EXPECT_NEAR(uvEvenHist->Average(), 86.1211, 0.0001);
  EXPECT_EQ(uvEvenHist->Sum(), 11023.5);
  EXPECT_EQ(uvEvenHist->ValidPixels(), 128);
  EXPECT_EQ(uvEvenHist->StandardDeviation(), 31.168941871307862);

  // UV Odd Cube
  QString cubeFileUvOdd = prefix.path() + "/lrowac2isisTEMP.uv.odd.cub";
  Cube uvOddCube(cubeFileUvOdd);
  Pvl *uvOddLabel = uvOddCube.label();

  // Dimensions group
  EXPECT_EQ(uvOddCube.sampleCount(), 128);
  EXPECT_EQ(uvOddCube.lineCount(), 5);
  EXPECT_EQ(uvOddCube.bandCount(), 2);

  // Pixels group
  EXPECT_EQ(PixelTypeName(uvOddCube.pixelType()), "Real");
  EXPECT_DOUBLE_EQ(uvOddCube.base(), 0.0);
  EXPECT_DOUBLE_EQ(uvOddCube.multiplier(), 1.0);

  // Instrument Group
  PvlGroup &uvOddInst = uvOddLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(uvOddInst["SpacecraftName"][0], "LUNAR RECONNAISSANCE ORBITER");
  EXPECT_EQ(uvOddInst["InstrumentId"][0], "WAC-UV");
  EXPECT_EQ(uvOddInst["TargetName"][0], "Moon");
  EXPECT_EQ(uvOddInst["StartTime"][0], "2009-09-15T07:27:49.230000");
  EXPECT_EQ(uvOddInst["StopTime"][0], "2009-09-15T07:30:19.542000");
  EXPECT_EQ(uvOddInst["MissionPhaseName"][0], "COMMISSIONING");
  EXPECT_EQ(std::stod(uvOddInst["BeginTemperatureFpa"][0]), -1.66529297828674);
  EXPECT_EQ(std::stod(uvOddInst["MiddleTemperatureFpa"][0]), -1.12489998340607);
  EXPECT_EQ(std::stod(uvOddInst["EndTemperatureFpa"][0]), -0.669131994247437);
  EXPECT_EQ(std::stod(uvOddInst["BeginTemperatureScs"][0]), 10.8307619094849);
  EXPECT_EQ(std::stod(uvOddInst["MiddleTemperatureScs"][0]), 10.914568901062);
  EXPECT_EQ(std::stod(uvOddInst["EndTemperatureScs"][0]), 10.9736194610596);
  EXPECT_EQ(uvOddInst["Mode"][0], "0");
  EXPECT_EQ(uvOddInst["DataFlipped"][0], "No");
  EXPECT_EQ(uvOddInst["ColorOffset"][0], "0");
  EXPECT_EQ(uvOddInst["Decompanded"][0], "Yes");
  EXPECT_EQ(uvOddInst["Framelets"][0], "Odd");
  EXPECT_EQ(uvOddInst["NumFramelets"][0], "1");
  EXPECT_EQ(uvOddInst["InstrumentModeId"][0], "COLOR");

  // Bandbin Group
  PvlGroup &uvOddBandbin = uvOddLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(uvOddBandbin["FilterNumber"][0], "1");
  EXPECT_EQ(uvOddBandbin["FilterNumber"][1], "2");
  EXPECT_EQ(uvOddBandbin["Center"][0], "321");
  EXPECT_EQ(uvOddBandbin["Center"][1], "360");
  EXPECT_EQ(uvOddBandbin["Width"][0], "32");
  EXPECT_EQ(uvOddBandbin["Width"][1], "15");


  std::unique_ptr<Histogram> uvOddHist (uvOddCube.histogram());

  EXPECT_NEAR(uvOddHist->Average(), 85.7861328125, 0.0001);
  EXPECT_EQ(uvOddHist->Sum(), 43922.5);
  EXPECT_EQ(uvOddHist->ValidPixels(), 512);
  EXPECT_NEAR(uvOddHist->StandardDeviation(), 30.5786, 0.0001);

  // VIS Even Cube
  QString cubeFileVisEven = prefix.path() + "/lrowac2isisTEMP.vis.even.cub";
  Cube visEvenCube(cubeFileVisEven);
  Pvl *visEvenLabel = visEvenCube.label();

  // Dimensions group
  EXPECT_EQ(visEvenCube.sampleCount(), 704);
  EXPECT_EQ(visEvenCube.lineCount(), 18);
  EXPECT_EQ(visEvenCube.bandCount(), 5);

  // Pixels group
  EXPECT_EQ(PixelTypeName(visEvenCube.pixelType()), "Real");
  EXPECT_DOUBLE_EQ(visEvenCube.base(), 0.0);
  EXPECT_DOUBLE_EQ(visEvenCube.multiplier(), 1.0);

  // Instrument Group
  PvlGroup &visEvenInst = visEvenLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(visEvenInst["SpacecraftName"][0], "LUNAR RECONNAISSANCE ORBITER");
  EXPECT_EQ(visEvenInst["InstrumentId"][0], "WAC-VIS");
  EXPECT_EQ(visEvenInst["TargetName"][0], "Moon");
  EXPECT_EQ(visEvenInst["StartTime"][0], "2009-09-15T07:27:49.230000");
  EXPECT_EQ(visEvenInst["StopTime"][0], "2009-09-15T07:30:19.542000");
  EXPECT_EQ(visEvenInst["MissionPhaseName"][0], "COMMISSIONING");
  EXPECT_EQ(std::stod(visEvenInst["BeginTemperatureFpa"][0]), -1.66529297828674);
  EXPECT_EQ(std::stod(visEvenInst["MiddleTemperatureFpa"][0]), -1.12489998340607);
  EXPECT_EQ(std::stod(visEvenInst["EndTemperatureFpa"][0]), -0.669131994247437);
  EXPECT_EQ(std::stod(visEvenInst["BeginTemperatureScs"][0]), 10.8307619094849);
  EXPECT_EQ(std::stod(visEvenInst["MiddleTemperatureScs"][0]), 10.914568901062);
  EXPECT_EQ(std::stod(visEvenInst["EndTemperatureScs"][0]), 10.9736194610596);
  EXPECT_EQ(visEvenInst["Mode"][0], "0");
  EXPECT_EQ(visEvenInst["DataFlipped"][0], "No");
  EXPECT_EQ(visEvenInst["ColorOffset"][0], "0");
  EXPECT_EQ(visEvenInst["Decompanded"][0], "Yes");
  EXPECT_EQ(visEvenInst["Framelets"][0], "Even");
  EXPECT_EQ(visEvenInst["NumFramelets"][0], "1");
  EXPECT_EQ(visEvenInst["InstrumentModeId"][0], "COLOR");

  // Bandbin Group
  PvlGroup &visEvenBandbin = visEvenLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(visEvenBandbin["FilterNumber"][0], "3");
  EXPECT_EQ(visEvenBandbin["FilterNumber"][1], "4");
  EXPECT_EQ(visEvenBandbin["Center"][0], "415");
  EXPECT_EQ(visEvenBandbin["Center"][1], "566");
  EXPECT_EQ(visEvenBandbin["Width"][0], "36");
  EXPECT_EQ(visEvenBandbin["Width"][1], "20");


  std::unique_ptr<Histogram> visEvenHist (visEvenCube.histogram());

  EXPECT_NEAR(visEvenHist->Average(), 140.49680397727272, 0.0001);
  EXPECT_EQ(visEvenHist->Sum(), 395639);
  EXPECT_EQ(visEvenHist->ValidPixels(), 2816);
  EXPECT_NEAR(visEvenHist->StandardDeviation(), 40.1957, 0.0001);

  // VIS Odd Cube
  QString cubeFileVisOdd = prefix.path() + "/lrowac2isisTEMP.vis.odd.cub";
  Cube visOddCube(cubeFileVisOdd);
  Pvl *visOddLabel = visOddCube.label();

  // Dimensions group
  EXPECT_EQ(visOddCube.sampleCount(), 704);
  EXPECT_EQ(visOddCube.lineCount(), 18);
  EXPECT_EQ(visOddCube.bandCount(), 5);

  // Pixels group
  EXPECT_EQ(PixelTypeName(visOddCube.pixelType()), "Real");
  EXPECT_DOUBLE_EQ(visOddCube.base(), 0.0);
  EXPECT_DOUBLE_EQ(visOddCube.multiplier(), 1.0);

  // Instrument Group
  PvlGroup &visOddInst = visOddLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(visOddInst["SpacecraftName"][0], "LUNAR RECONNAISSANCE ORBITER");
  EXPECT_EQ(visOddInst["InstrumentId"][0], "WAC-VIS");
  EXPECT_EQ(visOddInst["TargetName"][0], "Moon");
  EXPECT_EQ(visOddInst["StartTime"][0], "2009-09-15T07:27:49.230000");
  EXPECT_EQ(visOddInst["StopTime"][0], "2009-09-15T07:30:19.542000");
  EXPECT_EQ(visOddInst["MissionPhaseName"][0], "COMMISSIONING");
  EXPECT_EQ(std::stod(visOddInst["BeginTemperatureFpa"][0]), -1.66529297828674);
  EXPECT_EQ(std::stod(visOddInst["MiddleTemperatureFpa"][0]), -1.12489998340607);
  EXPECT_EQ(std::stod(visOddInst["EndTemperatureFpa"][0]), -0.669131994247437);
  EXPECT_EQ(std::stod(visOddInst["BeginTemperatureScs"][0]), 10.8307619094849);
  EXPECT_EQ(std::stod(visOddInst["MiddleTemperatureScs"][0]), 10.914568901062);
  EXPECT_EQ(std::stod(visOddInst["EndTemperatureScs"][0]), 10.9736194610596);
  EXPECT_EQ(visOddInst["Mode"][0], "0");
  EXPECT_EQ(visOddInst["DataFlipped"][0], "No");
  EXPECT_EQ(visOddInst["ColorOffset"][0], "0");
  EXPECT_EQ(visOddInst["Decompanded"][0], "Yes");
  EXPECT_EQ(visOddInst["Framelets"][0], "Odd");
  EXPECT_EQ(visOddInst["NumFramelets"][0], "1");
  EXPECT_EQ(visOddInst["InstrumentModeId"][0], "COLOR");

  // Bandbin Group
  PvlGroup &visOddBandbin = visOddLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(visOddBandbin["FilterNumber"][0], "3");
  EXPECT_EQ(visOddBandbin["FilterNumber"][1], "4");
  EXPECT_EQ(visOddBandbin["Center"][0], "415");
  EXPECT_EQ(visOddBandbin["Center"][1], "566");
  EXPECT_EQ(visOddBandbin["Width"][0], "36");
  EXPECT_EQ(visOddBandbin["Width"][1], "20");

  std::unique_ptr<Histogram> visOddHist (visOddCube.histogram());

  EXPECT_NEAR(visOddHist->Average(), 141.94663149350649, 0.0001);
  EXPECT_NEAR(visOddHist->Sum(), 1399026, 0.0001);
  EXPECT_EQ(visOddHist->ValidPixels(), 9856);
  EXPECT_NEAR(visOddHist->StandardDeviation(), 24.4899, 0.0001);
}

TEST(Lrowac2isisTests, FunctionalTestLrowac2isisColorOffset) {
  QTemporaryDir prefix;

  QString cubeFileName = prefix.path() + "/lrowac2isisTEMP.cub";
  QVector<QString> args = {"from=data/lrowac2isis/wac0000a1c4_cropped.img",
                           "to=" + cubeFileName,
                           "coloroffset=true"};

  UserInterface options(APP_XML, args);
  try {
    lrowac2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LROWAC image " << e.toString().toStdString().c_str() << std::endl;
  }

  // UV Even Cube
  QString cubeFileUvEven = prefix.path() + "/lrowac2isisTEMP.uv.even.cub";
  Cube uvEvenCube(cubeFileUvEven);
  Pvl *uvEvenLabel = uvEvenCube.label();

  // Dimensions group
  EXPECT_EQ(uvEvenCube.sampleCount(), 128);
  EXPECT_EQ(uvEvenCube.lineCount(), 13);
  EXPECT_EQ(uvEvenCube.bandCount(), 2);

  // Instrument Group
  PvlGroup &uvEvenInst = uvEvenLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(uvEvenInst["ColorOffset"][0], "2");
  EXPECT_EQ(uvEvenInst["Decompanded"][0], "Yes");
  EXPECT_EQ(uvEvenInst["Framelets"][0], "Even");
  EXPECT_EQ(uvEvenInst["NumFramelets"][0], "3");
  EXPECT_EQ(uvEvenInst["InstrumentModeId"][0], "COLOR");


  std::unique_ptr<Histogram> uvEvenHist (uvEvenCube.histogram());

  EXPECT_NEAR(uvEvenHist->Average(), 87.537109375, 0.0001);
  EXPECT_EQ(uvEvenHist->Sum(), 44819);
  EXPECT_EQ(uvEvenHist->ValidPixels(), 512);
  EXPECT_NEAR(uvEvenHist->StandardDeviation(), 32.4207, 0.0001);

  // UV Odd Cube
  QString cubeFileUvOdd = prefix.path() + "/lrowac2isisTEMP.uv.odd.cub";
  Cube uvOddCube(cubeFileUvOdd);
  Pvl *uvOddLabel = uvOddCube.label();

  // Dimensions group
  EXPECT_EQ(uvOddCube.sampleCount(), 128);
  EXPECT_EQ(uvOddCube.lineCount(), 13);
  EXPECT_EQ(uvOddCube.bandCount(), 2);

  // Instrument Group
  PvlGroup &uvOddInst = uvOddLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(uvOddInst["ColorOffset"][0], "2");
  EXPECT_EQ(uvOddInst["Decompanded"][0], "Yes");
  EXPECT_EQ(uvOddInst["Framelets"][0], "Odd");
  EXPECT_EQ(uvOddInst["NumFramelets"][0], "3");
  EXPECT_EQ(uvOddInst["InstrumentModeId"][0], "COLOR");


  std::unique_ptr<Histogram> uvOddHist (uvOddCube.histogram());

  EXPECT_NEAR(uvOddHist->Average(), 85.7861328125, 0.0001);
  EXPECT_EQ(uvOddHist->Sum(), 43922.5);
  EXPECT_EQ(uvOddHist->ValidPixels(), 512);
  EXPECT_NEAR(uvOddHist->StandardDeviation(), 30.5786, 0.0001);

  // VIS Even Cube
  QString cubeFileVisEven = prefix.path() + "/lrowac2isisTEMP.vis.even.cub";
  Cube visEvenCube(cubeFileVisEven);
  Pvl *visEvenLabel = visEvenCube.label();

  // Dimensions group
  EXPECT_EQ(visEvenCube.sampleCount(), 704);
  EXPECT_EQ(visEvenCube.lineCount(), 130);
  EXPECT_EQ(visEvenCube.bandCount(), 5);

  // Instrument Group
  PvlGroup &visEvenInst = visEvenLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(visEvenInst["ColorOffset"][0], "2");
  EXPECT_EQ(visEvenInst["Decompanded"][0], "Yes");
  EXPECT_EQ(visEvenInst["Framelets"][0], "Even");
  EXPECT_EQ(visEvenInst["NumFramelets"][0], "9");
  EXPECT_EQ(visEvenInst["InstrumentModeId"][0], "COLOR");

  // Bandbin Group
  PvlGroup &visEvenBandbin = visEvenLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(visEvenBandbin["FilterNumber"][0], "3");
  EXPECT_EQ(visEvenBandbin["FilterNumber"][1], "4");
  EXPECT_EQ(visEvenBandbin["FilterNumber"][2], "5");
  EXPECT_EQ(visEvenBandbin["FilterNumber"][3], "6");
  EXPECT_EQ(visEvenBandbin["FilterNumber"][4], "7");
  EXPECT_EQ(visEvenBandbin["Center"][0], "415");
  EXPECT_EQ(visEvenBandbin["Center"][1], "566");
  EXPECT_EQ(visEvenBandbin["Center"][2], "604");
  EXPECT_EQ(visEvenBandbin["Center"][3], "643");
  EXPECT_EQ(visEvenBandbin["Center"][4], "689");
  EXPECT_EQ(visEvenBandbin["Width"][0], "36");
  EXPECT_EQ(visEvenBandbin["Width"][1], "20");
  EXPECT_EQ(visEvenBandbin["Width"][2], "20");
  EXPECT_EQ(visEvenBandbin["Width"][3], "23");
  EXPECT_EQ(visEvenBandbin["Width"][4], "39");


  std::unique_ptr<Histogram> visEvenHist (visEvenCube.histogram());

  EXPECT_NEAR(visEvenHist->Average(), 144.54956371, 0.0001);
  EXPECT_NEAR(visEvenHist->Sum(), 1424680.5, 0.0001);
  EXPECT_EQ(visEvenHist->ValidPixels(), 9856);
  EXPECT_NEAR(visEvenHist->StandardDeviation(), 26.579817, 0.0001);

  // VIS Odd Cube
  QString cubeFileVisOdd = prefix.path() + "/lrowac2isisTEMP.vis.odd.cub";
  Cube visOddCube(cubeFileVisOdd);
  Pvl *visOddLabel = visOddCube.label();

  // Dimensions group
  EXPECT_EQ(visOddCube.sampleCount(), 704);
  EXPECT_EQ(visOddCube.lineCount(), 130);
  EXPECT_EQ(visOddCube.bandCount(), 5);

  // Instrument Group
  PvlGroup &visOddInst = visOddLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(visOddInst["ColorOffset"][0], "2");
  EXPECT_EQ(visOddInst["Decompanded"][0], "Yes");
  EXPECT_EQ(visOddInst["Framelets"][0], "Odd");
  EXPECT_EQ(visOddInst["NumFramelets"][0], "9");
  EXPECT_EQ(visOddInst["InstrumentModeId"][0], "COLOR");

  // Bandbin Group
  PvlGroup &visOddBandbin = visOddLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(visOddBandbin["FilterNumber"][0], "3");
  EXPECT_EQ(visOddBandbin["FilterNumber"][1], "4");
  EXPECT_EQ(visOddBandbin["FilterNumber"][2], "5");
  EXPECT_EQ(visOddBandbin["FilterNumber"][3], "6");
  EXPECT_EQ(visOddBandbin["FilterNumber"][4], "7");
  EXPECT_EQ(visOddBandbin["Center"][0], "415");
  EXPECT_EQ(visOddBandbin["Center"][1], "566");
  EXPECT_EQ(visOddBandbin["Center"][2], "604");
  EXPECT_EQ(visOddBandbin["Center"][3], "643");
  EXPECT_EQ(visOddBandbin["Center"][4], "689");
  EXPECT_EQ(visOddBandbin["Width"][0], "36");
  EXPECT_EQ(visOddBandbin["Width"][1], "20");
  EXPECT_EQ(visOddBandbin["Width"][2], "20");
  EXPECT_EQ(visOddBandbin["Width"][3], "23");
  EXPECT_EQ(visOddBandbin["Width"][4], "39");


  std::unique_ptr<Histogram> visOddHist (visOddCube.histogram());

  EXPECT_NEAR(visOddHist->Average(), 141.94663149350649, 0.0001);
  EXPECT_NEAR(visOddHist->Sum(), 1399026, 0.0001);
  EXPECT_EQ(visOddHist->ValidPixels(), 9856);
  EXPECT_NEAR(visOddHist->StandardDeviation(), 24.4899, 0.0001);

}

TEST(Lrowac2isisTests, FunctionalTestLrowac2isisNoUnlut) {
  QTemporaryDir prefix;

  QString cubeFileName = prefix.path() + "/lrowac2isisTEMP.cub";;
  QVector<QString> args = {"from=data/lrowac2isis/wac0000a1c4_cropped.img",
                           "to=" + cubeFileName,
                           "unlut=false"};

  UserInterface options(APP_XML, args);
  try {
    lrowac2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LROWAC image " << e.toString().toStdString().c_str() << std::endl;
  }

  // UV Even Cube
  QString cubeFileUvEven = prefix.path() + "/lrowac2isisTEMP.uv.even.cub";
  Cube uvEvenCube(cubeFileUvEven);
  Pvl *uvEvenLabel = uvEvenCube.label();

  // Dimensions group
  EXPECT_EQ(uvEvenCube.sampleCount(), 128);
  EXPECT_EQ(uvEvenCube.lineCount(), 5);
  EXPECT_EQ(uvEvenCube.bandCount(), 2);

  // Instrument Group
  PvlGroup &uvEvenInst = uvEvenLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(uvEvenInst["ColorOffset"][0], "0");
  EXPECT_EQ(uvEvenInst["Decompanded"][0], "No");
  EXPECT_EQ(uvEvenInst["Framelets"][0], "Even");
  EXPECT_EQ(uvEvenInst["NumFramelets"][0], "1");
  EXPECT_EQ(uvEvenInst["InstrumentModeId"][0], "COLOR");


  std::unique_ptr<Histogram> uvEvenHist (uvEvenCube.histogram());

  EXPECT_NEAR(uvEvenHist->Average(), 44.4609375, 0.0001);
  EXPECT_EQ(uvEvenHist->Sum(), 5691);
  EXPECT_EQ(uvEvenHist->ValidPixels(), 128);
  EXPECT_NEAR(uvEvenHist->StandardDeviation(), 15.39449863, 0.0001);

  // UV Odd Cube
  QString cubeFileUvOdd = prefix.path() + "/lrowac2isisTEMP.uv.odd.cub";
  Cube uvOddCube(cubeFileUvOdd);
  Pvl *uvOddLabel = uvOddCube.label();

  // Dimensions group
  EXPECT_EQ(uvOddCube.sampleCount(), 128);
  EXPECT_EQ(uvOddCube.lineCount(), 5);
  EXPECT_EQ(uvOddCube.bandCount(), 2);

  // Instrument Group
  PvlGroup &uvOddInst = uvOddLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(uvOddInst["ColorOffset"][0], "0");
  EXPECT_EQ(uvOddInst["Decompanded"][0], "No");
  EXPECT_EQ(uvOddInst["Framelets"][0], "Odd");
  EXPECT_EQ(uvOddInst["NumFramelets"][0], "1");
  EXPECT_EQ(uvOddInst["InstrumentModeId"][0], "COLOR");


  std::unique_ptr<Histogram> uvOddHist (uvOddCube.histogram());

  EXPECT_NEAR(uvOddHist->Average(), 44.380859375, 0.0001);
  EXPECT_EQ(uvOddHist->Sum(), 22723);
  EXPECT_EQ(uvOddHist->ValidPixels(), 512);
  EXPECT_NEAR(uvOddHist->StandardDeviation(), 15.25508, 0.0001);

  // VIS Even Cube
  QString cubeFileVisEven = prefix.path() + "/lrowac2isisTEMP.vis.even.cub";
  Cube visEvenCube(cubeFileVisEven);
  Pvl *visEvenLabel = visEvenCube.label();

  // Dimensions group
  EXPECT_EQ(visEvenCube.sampleCount(), 704);
  EXPECT_EQ(visEvenCube.lineCount(), 18);
  EXPECT_EQ(visEvenCube.bandCount(), 5);

  // Instrument Group
  PvlGroup &visEvenInst = visEvenLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(visEvenInst["ColorOffset"][0], "0");
  EXPECT_EQ(visEvenInst["Decompanded"][0], "No");
  EXPECT_EQ(visEvenInst["Framelets"][0], "Even");
  EXPECT_EQ(visEvenInst["NumFramelets"][0], "1");
  EXPECT_EQ(visEvenInst["InstrumentModeId"][0], "COLOR");


  std::unique_ptr<Histogram> visEvenHist (visEvenCube.histogram());

  EXPECT_NEAR(visEvenHist->Average(), 61.2080965, 0.0001);
  EXPECT_EQ(visEvenHist->Sum(), 172362);
  EXPECT_EQ(visEvenHist->ValidPixels(), 2816);
  EXPECT_NEAR(visEvenHist->StandardDeviation(), 6.821865, 0.0001);

  // VIS Odd Cube
  QString cubeFileVisOdd = prefix.path() + "/lrowac2isisTEMP.vis.odd.cub";
  Cube visOddCube(cubeFileVisOdd);
  Pvl *visOddLabel = visOddCube.label();

  // Dimensions group
  EXPECT_EQ(visOddCube.sampleCount(), 704);
  EXPECT_EQ(visOddCube.lineCount(), 18);
  EXPECT_EQ(visOddCube.bandCount(), 5);

  // Instrument Group
  PvlGroup &visOddInst = visOddLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(visOddInst["ColorOffset"][0], "0");
  EXPECT_EQ(visOddInst["Decompanded"][0], "No");
  EXPECT_EQ(visOddInst["Framelets"][0], "Odd");
  EXPECT_EQ(visOddInst["NumFramelets"][0], "1");
  EXPECT_EQ(visOddInst["InstrumentModeId"][0], "COLOR");


  std::unique_ptr<Histogram> visOddHist (visOddCube.histogram());

  EXPECT_NEAR(visOddHist->Average(), 61.74015, 0.0001);
  EXPECT_NEAR(visOddHist->Sum(), 608511, 0.0001);
  EXPECT_EQ(visOddHist->ValidPixels(), 9856);
  EXPECT_NEAR(visOddHist->StandardDeviation(), 4.67139, 0.0001);
}
