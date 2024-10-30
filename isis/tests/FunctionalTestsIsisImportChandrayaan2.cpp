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

TEST_F(TempTestingFiles, FunctionalTestIsisImportChandrayaan2MinimalLabel){
  std::istringstream PvlInput(R"(
    Object = IsisCube
      Object = Core
        StartByte   = 65537
        Format      = Tile
        TileSamples = 128
        TileLines   = 400

        Group = Dimensions
          Samples = 17891
          Lines   = 400
          Bands   = 1
        End_Group

        Group = Pixels
          Type       = UnsignedByte
          ByteOrder  = Lsb
          Base       = 0.0
          Multiplier = 1.0
        End_Group
      End_Object

      Group = Instrument
        SpacecraftName = Chandrayaan-2
        InstrumentId   = TMC-2
        TargetName     = Moon
        StartTime      = 2019-11-28T00:35:38.9755
        StopTime       = 2019-11-28T00:45:17.9161
        LineExposureDuration = 3.236 <ms>
      End_Group

      Group = BandBin
        Center = 675
        Width  = 175
      End_Group

      Group = Kernels
        NaifFrameCode = -152212
      End_Group
    End_Object

    Object = Label
      Bytes = 65536
    End_Object

    Object = History
      Name      = IsisCube
      StartByte = 7233537
      Bytes     = 703
    End_Object

    Object = OriginalXmlLabel
      Name      = IsisCube
      StartByte = 7234240
      Bytes     = 4223
      ByteOrder = Lsb
    End_Object
    End
  )");
  QString dataFilePath= "data/isisimport/chan2/ch2_tmc_nca_20191128T0035389755_b_brw_d18.xml";
  QString dataFileName = "ch2_tmc_nca_20191128T0035389755_b_brw_d18.xml";
  QString imageFileName = "ch2_tmc_nca_20191128T0035389755_b_brw_d18.img";
  // QString cubeFileName = tempDir.path() + "/output.cub";
  QString cubeFileName = "/Users/astamile/testData/chandrayaan2/images/ch2_tmc_nca_20191128T0035389755_b_brw_d18.cub";

  int samples = 400;
  int lines = 17891;
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

}

TEST_F(TempTestingFiles, FunctionalTestIsisImportChandrayaan2FullLabel){
  std::istringstream PvlInput(R"(
  Object = IsisCube
    Object = Core
      StartByte   = 65537
      Format      = Tile
      TileSamples = 519
      TileLines   = 1000

      Group = Dimensions
        Samples = 180093
        Lines   = 4000
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
      SpacecraftName       = Chandrayaan-2
      InstrumentId         = TMC-2
      TargetName           = Moon
      StartTime            = 2024-08-08T05:32:59.6974
      StopTime             = 2024-08-08T05:42:42.4783
      LineExposureDuration = 3.236 <ms>
    End_Group

    Group = Archive
      JobId                  = TMCXXD18CHO2210300NNNN24221055326097_V2_1
      OrbitNumber            = 22103
      GainType               = g1
      ExposureType           = e1
      DetectorPixelWidth     = 7 <micrometers>
      FocalLength            = 140 <mm>
      ReferenceData          = SELENE
      OrbitLimbDirection     = Descending
      SpacecraftYawDirection = False
      SpacecraftAltitude     = 89.53 <km>
      PixelResolution        = 4.48 <meters/pixel>
      Roll                   = 0.009365 <degrees>
      Pitch                  = 0.066417 <degrees>
      Yaw                    = -0.017284 <degrees>
      SunAzimuth             = 116.737463 <degrees>
      SunElevation           = 39.932493 <degrees>
      SolarIncidence         = 50.067507 <degrees>
      Projection             = Selenographic
      Area                   = Equatorial
    End_Group

    Group = BandBin
      Center = 675
      Width  = 175
    End_Group

    Group = Kernels
      NaifFrameCode = -152210
    End_Group
  End_Object

  Object = Label
    Bytes = 65536
  End_Object

  Object = OriginalXmlLabel
    Name      = IsisCube
    StartByte = 1440809537
    Bytes     = 7625
    ByteOrder = Lsb
  End_Object
  End
  )");
  QString dataFilePath= "data/isisimport/chan2/ch2_tmc_ncn_20240808T0532596974_d_img_d18.xml";
  QString dataFileName = "ch2_tmc_ncn_20240808T0532596974_d_img_d18.xml";
  QString imageFileName = "ch2_tmc_ncn_20240808T0532596974_d_img_d18.img";
  // QString cubeFileName = tempDir.path() + "/output.cub";
  QString cubeFileName = "/Users/astamile/testData/chandrayaan2/images/ch2_tmc_ncn_20240808T0532596974_d_img_d18.cub";

  int samples = 4000;
  int lines = 180093;
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

    truthGroup = truthLabel.findGroup("Archive", Pvl::Traverse);
  outGroup = outLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, outGroup, truthGroup);

}