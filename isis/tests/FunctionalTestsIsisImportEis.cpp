#include <iostream>
#include <time.h>

#include <QRegExp>
#include <QString>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QFileInfo>
#include <QDataStream>
#include <QTextStream>
#include <QByteArray>
#include <QDataStream>

#include <nlohmann/json.hpp>
#include "TempFixtures.h"
#include "Histogram.h"
#include "md5wrapper.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "TestUtilities.h"
#include "isisimport.h"
#include "gmock/gmock.h"

using namespace Isis;
using namespace testing;
using json = nlohmann::json;

static QString APP_XML = FileName("$ISISROOT/bin/xml/isisimport.xml").expanded();

TEST_F(TempTestingFiles, FunctionalTestIsisImportEisNacFrame){
  std::istringstream PvlInput(R"(
    Object = IsisCube
      Object = Core
        StartByte   = 65537
        Format      = Tile
        TileSamples = 1024
        TileLines   = 108

        Group = Dimensions
          Samples = 4096
          Lines   = 108
          Bands   = 1
        End_Group

        Group = Pixels
          Type       = UnsignedWord
          ByteOrder  = Lsb
          Base       = 0.0
          Multiplier = 1.0
        End_Group
      End_Object

      Group = Instrument
        SpacecraftName     = Clipper
        InstrumentId       = NAC-FRAMING
        TargetName         = Europa
        StartTime          = 2032-04-25T15:19:02.191
        ExposureDuration   = 4.35 <ms>
        DetectorLineOffset = 803
      End_Group

      Group = BandBin
        FilterName = FRAMING_AREA
        Center     = UNK
        Width      = UNK
      End_Group

      Group = Kernels
        NaifFrameCode = -159103
      End_Group
    End_Object

    Object = Label
      Bytes = 65536
    End_Object

    Object = History
      Name      = IsisCube
      StartByte = 950273
      Bytes     = 843
    End_Object

    Object = OriginalXmlLabel
      Name      = IsisCube
      StartByte = 951116
      Bytes     = 8439
      ByteOrder = Lsb
    End_Object
    End
  )");
  QString dataFilePath= "data/isisimport/eispds/nacFrame/EIS000XXX_2032116T151902_0000C350-NAC-FRAME-IMG_RAW_V1.xml";
  QString dataFileName = "EIS000XXX_2032116T151902_0000C350-NAC-FRAME-IMG_RAW_V1.xml";
  QString imageFileName = "EIS000XXX_2032116T151902_0000C350-NAC-FRAME-IMG_RAW_V1.img";
  QString cubeFileName = tempDir.path() + "/nacFrame.cub";

  int samples = 1335;
  int lines = 3832;
  int bytes = 2;

  // create a temp img file and write data to it
  QFile tempImgFile(tempDir.path() + "/" + imageFileName);

  if(!tempImgFile.open(QFile::WriteOnly | QFile::Text)){
      FAIL() << " Could not open file for writing";
  }
  QDataStream out(&tempImgFile);

  // generate lines
  QByteArray writeToFile = QByteArray();
  short int fill = 0;
  for(int i=-1; i<(samples * bytes); i++){
    writeToFile.append(fill);
  }

  // write the lines to the temp file
  for(int i=0; i<lines; i++){
    QDataStream out(&tempImgFile);
    out << writeToFile;
  }
  tempImgFile.flush();
  tempImgFile.close();

  // create a temp data file and copy the contents of the xml in to it
  QFile tempDataFile(tempDir.path() + "/" + dataFileName);

  if(!tempDataFile.open(QFile::ReadWrite | QFile::Text)){
      FAIL() << " Could not open file for writing";
  }

  // open xml to get data
  QFile realXmlFile(dataFilePath);
  if (!realXmlFile.open(QIODevice::ReadOnly | QIODevice::Text))
  {
      FAIL() << "Failed to open file";
  }

  QTextStream xmlData(&tempDataFile);
  xmlData << realXmlFile.readAll();

  tempDataFile.close();
  realXmlFile.close();

  QFileInfo fileInfo(tempDataFile);

  // testing with template
  QVector<QString> args = {"from=" + fileInfo.absoluteFilePath(), "to=" + cubeFileName};
    UserInterface options(APP_XML, args);
  isisimport(options);

  Pvl truthLabel;
  PvlInput >> truthLabel;

  Cube outCube(cubeFileName);
  Pvl *outLabel = outCube.label();

  PvlGroup truthGroup = truthLabel.findGroup("Dimensions", Pvl::Traverse);
  PvlGroup &outGroup = outLabel->findGroup("Dimensions", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Pixels", Pvl::Traverse);
  outGroup = outLabel->findGroup("Pixels", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Instrument", Pvl::Traverse);
  outGroup = outLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("BandBin", Pvl::Traverse);
  outGroup = outLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Kernels", Pvl::Traverse);
  outGroup = outLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  // testing with out template
  QVector<QString> argsNoTemp = {"from=" + fileInfo.absoluteFilePath(), "to=" + cubeFileName};
  UserInterface optionsNoTemp(APP_XML, argsNoTemp);
  isisimport(optionsNoTemp);

  Cube outCubeNoTemp(cubeFileName);
  Pvl *outLabelNoTemp = outCubeNoTemp.label();

  truthGroup = truthLabel.findGroup("Dimensions", Pvl::Traverse);
  outGroup = outLabelNoTemp->findGroup("Dimensions", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Pixels", Pvl::Traverse);
  outGroup = outLabelNoTemp->findGroup("Pixels", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Instrument", Pvl::Traverse);
  outGroup = outLabelNoTemp->findGroup("Instrument", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("BandBin", Pvl::Traverse);
  outGroup = outLabelNoTemp->findGroup("BandBin", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Kernels", Pvl::Traverse);
  outGroup = outLabelNoTemp->findGroup("Kernels", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);
}


TEST_F(TempTestingFiles, FunctionalTestIsisImportEisWacFrame){
  std::istringstream PvlInput(R"(
  Object = IsisCube
    Object = Core
      StartByte   = 65537
      Format      = Tile
      TileSamples = 1024
      TileLines   = 20

      Group = Dimensions
        Samples = 4096
        Lines   = 20
        Bands   = 1
      End_Group

      Group = Pixels
        Type       = UnsignedWord
        ByteOrder  = Lsb
        Base       = 0.0
        Multiplier = 1.0
      End_Group
    End_Object

    Group = Instrument
      SpacecraftName     = Clipper
      InstrumentId       = WAC-FRAMING
      TargetName         = Europa
      StartTime          = 2032-04-25T23:49:37.002
      ExposureDuration   = 27.54 <ms>
      DetectorLineOffset = 1
    End_Group

    Group = BandBin
      FilterName = MASK
      Center     = UNK
      Width      = UNK
    End_Group

    Group = Kernels
      NaifFrameCode = -159104
    End_Group
  End_Object

  Object = Label
    Bytes = 65536
  End_Object

  Object = History
    Name      = IsisCube
    StartByte = 229377
    Bytes     = 745
  End_Object

  Object = OriginalXmlLabel
    Name      = IsisCube
    StartByte = 230122
    Bytes     = 8224
    ByteOrder = Lsb
  End_Object
  End
  )");
  QString dataFilePath = "data/isisimport/eispds/wacFrame/EIS000XXX_2032116T234937_0000C360-WAC-FRAME-IMG_RAW_V1.xml";
  QString dataFileName = "EIS000XXX_2032116T234937_0000C360-WAC-FRAME-IMG_RAW_V1.xml";
  QString imageFileName = "EIS000XXX_2032116T234937_0000C360-WAC-FRAME-IMG_RAW_V1.img";
  QString cubeFileName = tempDir.path() + "/wacFrame.cub";

  int samples = 1335;
  int lines = 3832;
  int bytes = 2;

  // create a temp img file and write data to it
  QFile tempImgFile(tempDir.path() + "/" + imageFileName);

  if(!tempImgFile.open(QFile::WriteOnly | QFile::Text)){
      FAIL() << " Could not open file for writing";
  }
  QDataStream out(&tempImgFile);

  // generate lines
  QByteArray writeToFile = QByteArray();
  short int fill = 0;
  for(int i=-1; i<(samples * bytes); i++){
    writeToFile.append(fill);
  }

  // write the lines to the temp file
  for(int i=0; i<lines; i++){
    QDataStream out(&tempImgFile);
    out << writeToFile;
  }
  tempImgFile.flush();
  tempImgFile.close();

  // create a temp data file and copy the contents of the xml in to it
  QFile tempDataFile(tempDir.path() + "/" + dataFileName);

  if(!tempDataFile.open(QFile::ReadWrite | QFile::Text)){
      FAIL() << " Could not open file for writing";
  }

  // open xml to get data
  QFile realXmlFile(dataFilePath);
  if (!realXmlFile.open(QIODevice::ReadOnly | QIODevice::Text))
  {
      FAIL() << "Failed to open file";
  }

  QTextStream xmlData(&tempDataFile);
  xmlData << realXmlFile.readAll();

  tempDataFile.close();
  realXmlFile.close();

  QFileInfo fileInfo(tempDataFile);

  // testing with template
  QVector<QString> args = {"from=" + fileInfo.absoluteFilePath(), "to=" + cubeFileName};
  UserInterface options(APP_XML, args);
  isisimport(options);

  Pvl truthLabel;
  PvlInput >> truthLabel;

  Cube outCube(cubeFileName);
  Pvl *outLabel = outCube.label();

  PvlGroup truthGroup = truthLabel.findGroup("Dimensions", Pvl::Traverse);
  PvlGroup &outGroup = outLabel->findGroup("Dimensions", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Pixels", Pvl::Traverse);
  outGroup = outLabel->findGroup("Pixels", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Instrument", Pvl::Traverse);
  outGroup = outLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("BandBin", Pvl::Traverse);
  outGroup = outLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Kernels", Pvl::Traverse);
  outGroup = outLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  // testing with out template
  QVector<QString> argsNoTemp = {"from=" + fileInfo.absoluteFilePath(), "to=" + cubeFileName};
  UserInterface optionsNoTemp(APP_XML, argsNoTemp);
  isisimport(optionsNoTemp);

  Cube outCubeNoTemp(cubeFileName);
  Pvl *outLabelNoTemp = outCubeNoTemp.label();

  truthGroup = truthLabel.findGroup("Dimensions", Pvl::Traverse);
  outGroup = outLabelNoTemp->findGroup("Dimensions", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Pixels", Pvl::Traverse);
  outGroup = outLabelNoTemp->findGroup("Pixels", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Instrument", Pvl::Traverse);
  outGroup = outLabelNoTemp->findGroup("Instrument", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("BandBin", Pvl::Traverse);
  outGroup = outLabelNoTemp->findGroup("BandBin", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Kernels", Pvl::Traverse);
  outGroup = outLabelNoTemp->findGroup("Kernels", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);
}

TEST_F(TempTestingFiles, FunctionalTestIsisImportEisWacPb){
  std::istringstream PvlInput(R"(
    Object = IsisCube
      Object = Core
        StartByte   = 65537
        Format      = Tile
        TileSamples = 1024
        TileLines   = 256

        Group = Dimensions
          Samples = 4096
          Lines   = 256
          Bands   = 1
        End_Group

        Group = Pixels
          Type       = UnsignedWord
          ByteOrder  = Lsb
          Base       = 0.0
          Multiplier = 1.0
        End_Group
      End_Object

      Group = Instrument
        SpacecraftName     = Clipper
        InstrumentId       = WAC-PUSHBROOM
        TargetName         = Europa
        StartTime          = 2032-04-25T23:49:19.002
        ExposureDuration   = 27.54 <ms>
        DetectorLineOffset = 1807
      End_Group

      Group = BandBin
        FilterName = RED
        Center     = 670 <nm>
        Width      = 60 <nm>
      End_Group

      Group = Kernels
        NaifFrameCode = -159104
      End_Group
    End_Object

    Object = Label
      Bytes = 65536
    End_Object

    Object = Table
      Name      = LineScanTimes
      StartByte = 2170986
      Bytes     = 20
      Records   = 1
      ByteOrder = Lsb

      Group = Field
        Name = EphemerisTime
        Type = Double
        Size = 1
      End_Group

      Group = Field
        Name = ExposureTime
        Type = Double
        Size = 1
      End_Group

      Group = Field
        Name = LineStart
        Type = Integer
        Size = 1
      End_Group
    End_Object

    Object = History
      Name      = IsisCube
      StartByte = 2162689
      Bytes     = 745
    End_Object

    Object = OriginalXmlLabel
      Name      = IsisCube
      StartByte = 2163434
      Bytes     = 7552
      ByteOrder = Lsb
    End_Object
    End
  )");

  QString dataFilePath = "data/isisimport/eispds/wacPushb/EIS000XXX_2032116T234928_0000C35F-WAC-PUSHB-IMG_RAW_V1.xml";
  QString dataFileName = "EIS000XXX_2032116T234928_0000C35F-WAC-PUSHB-IMG_RAW_V1.xml";
  QString imageFileName = "EIS000XXX_2032116T234928_0000C35F-WAC-PUSHB-IMG_RAW_V1.img";
  QString cubeFileName = tempDir.path() + "/WacPb.cub";


  int samples = 4096;
  int lines = 4096;
  int bytes = 2;

  // create a temp img file and write data to it
  QFile tempImgFile(tempDir.path() + "/" + imageFileName);

  if(!tempImgFile.open(QFile::WriteOnly | QFile::Text)){
      FAIL() << " Could not open file for writing";
  }
  QDataStream out(&tempImgFile);

  // generate lines
  QByteArray writeToFile = QByteArray();
  short int fill = 0;
  for(int i=-1; i<(samples * bytes); i++){
    writeToFile.append(fill);
  }

  // write the lines to the temp file
  for(int i=0; i<lines; i++){
    QDataStream out(&tempImgFile);
    out << writeToFile;
  }
  tempImgFile.flush();
  tempImgFile.close();

  // create a temp data file and copy the contents of the xml in to it
  QFile tempDataFile(tempDir.path() + "/" + dataFileName);

  if(!tempDataFile.open(QFile::ReadWrite | QFile::Text)){
      FAIL() << " Could not open file for writing";
  }

  // open xml to get data
  QFile realXmlFile(dataFilePath);
  if (!realXmlFile.open(QIODevice::ReadOnly | QIODevice::Text))
  {
      FAIL() << "Failed to open file";
  }

  QTextStream xmlData(&tempDataFile);
  xmlData << realXmlFile.readAll();

  tempDataFile.close();
  realXmlFile.close();

  QFileInfo fileInfo(tempDataFile);

  // testing with template
  QVector<QString> args = {"from=" + fileInfo.absoluteFilePath(), "to=" + cubeFileName};
  UserInterface options(APP_XML, args);
  isisimport(options);

  Pvl truthLabel;
  PvlInput >> truthLabel;

  Cube outCube(cubeFileName);
  Pvl *outLabel = outCube.label();

  PvlGroup truthGroup = truthLabel.findGroup("Dimensions", Pvl::Traverse);
  PvlGroup &outGroup = outLabel->findGroup("Dimensions", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Pixels", Pvl::Traverse);
  outGroup = outLabel->findGroup("Pixels", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Instrument", Pvl::Traverse);
  outGroup = outLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("BandBin", Pvl::Traverse);
  outGroup = outLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Kernels", Pvl::Traverse);
  outGroup = outLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  // testing with out template
  QVector<QString> argsNoTemp = {"from=" + fileInfo.absoluteFilePath(), "to=" + cubeFileName};
  UserInterface optionsNoTemp(APP_XML, argsNoTemp);
  isisimport(optionsNoTemp);

  Cube outCubeNoTemp(cubeFileName);
  Pvl *outLabelNoTemp = outCubeNoTemp.label();

  truthGroup = truthLabel.findGroup("Dimensions", Pvl::Traverse);
  outGroup = outLabelNoTemp->findGroup("Dimensions", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Pixels", Pvl::Traverse);
  outGroup = outLabelNoTemp->findGroup("Pixels", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Instrument", Pvl::Traverse);
  outGroup = outLabelNoTemp->findGroup("Instrument", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("BandBin", Pvl::Traverse);
  outGroup = outLabelNoTemp->findGroup("BandBin", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

  truthGroup = truthLabel.findGroup("Kernels", Pvl::Traverse);
  outGroup = outLabelNoTemp->findGroup("Kernels", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);
}
