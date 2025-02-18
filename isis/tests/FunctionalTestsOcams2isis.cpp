#include <QString>

#include "ocams2isis.h"

#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlObject.h"
#include "TempFixtures.h"
#include "TestUtilities.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/ocams2isis.xml").expanded();

/**
   * FunctionalTestOcams2IsisPolyCam
   * 
   * Ocams2isis test for ingestion of OSIRIS-REx OCams PolyCam fits file.
   * 
   * Input ...
   *   PolyCam fits file (data/osirisRexImages/ocams/20190328T200344S309_pol_iofL2pan.fits)
   *
   * Output ...
   *   PolyCam ISIS cube file
   */
TEST_F(TempTestingFiles, FunctionalTestOcams2IsisPolyCam) {

  QString cubeFileName = tempDir.path() + "/20190328T200344S309_pol_iofL2pan.cub";
  QVector<QString> args = {"from=data/osirisRexImages/ocams/20190328T200344S309_pol_iofL2pan.fits",
                           "to=" + cubeFileName };

  UserInterface ui(APP_XML, args);
  try {
   ocams2isis(ui);
 }
 catch (IException &e) {
   FAIL() << "Unable to ingest OSIRIS-REx PolyCam image: "
         << e.toString().toStdString().c_str()
         << std::endl;
 }

  // open cube and read label
  std::unique_ptr<Cube> cube (new Cube(cubeFileName));
  Pvl *isisLabel = cube->label();

  // Core object
  PvlObject &core = isisLabel->findObject("Core", Pvl::Traverse);
  EXPECT_EQ(int(core["StartByte"]), 65537);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, core.findKeyword("Format"), "Tile");
  EXPECT_EQ(int(core["TileSamples"]), 512);
  EXPECT_EQ(int(core["TileLines"]), 512);

  // Dimensions Group in Core
  PvlGroup &dim = core.findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ(int(dim["Samples"]), 1024);
  EXPECT_EQ(int(dim["Lines"]), 1024);
  EXPECT_EQ(int(dim["Bands"]), 1);

  // Pixels Group in Core
  PvlGroup &pixels = core.findGroup("Pixels", Pvl::Traverse);
  EXPECT_EQ(pixels["Type"][0].toStdString(), "Real");
  EXPECT_EQ(pixels["ByteOrder"][0].toStdString(), "Lsb");
  EXPECT_DOUBLE_EQ(double(pixels["Base"]), 0.0);
  EXPECT_DOUBLE_EQ(double(pixels["Multiplier"]), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["MissionName"][0].toStdString(), "OSIRIS-REx");
  EXPECT_EQ(inst["SpacecraftName"][0].toStdString(), "OSIRIS-REX");
  EXPECT_EQ(inst["InstrumentId"][0].toStdString(), "PolyCam");
  EXPECT_EQ(inst["TargetName"][0].toStdString(), "Bennu");
  EXPECT_EQ(inst["StartTime"][0].toStdString(), "2019-03-28T20:03:44.309");
  EXPECT_DOUBLE_EQ(double(inst["ExposureDuration"]), 4.285275);
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0].toStdString(), "3/0607075384.53799");
  EXPECT_EQ(int(inst["FocusPosition"]), 16650);
  EXPECT_EQ(int(inst["PolyCamFocusPositionNaifId"]), -64589);

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["FilterName"][0].toStdString(), "PAN");
  ASSERT_EQ(int(bandbin["Center"]), 650);

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifFrameCode"]), -64360);

  // Label object
  PvlObject &label = isisLabel->findObject("Label", Pvl::Traverse);
  EXPECT_EQ(int(label["Bytes"]), 65536);
}


/**
   * FunctionalTestOcams2IsisMapCam
   * 
   * Ocams2isis test for ingestion of OSIRIS-REx OCams MapCam fits file.
   * 
   * Input ...
   *   MapCam fits file (data/osirisRexImages/ocams/20190223T023051S790_map_iofL2pan.fits)
   *
   * Output ...
   *   MapCam ISIS cube file
   */
TEST_F(TempTestingFiles, FunctionalTestOcams2IsisMapCam) {

  QString cubeFileName = tempDir.path() + "/20190223T023051S790_map_iofL2pan.cub";
  QVector<QString> args = {"from=data/osirisRexImages/ocams/20190223T023051S790_map_iofL2pan.fits",
                           "to=" + cubeFileName };

  UserInterface ui(APP_XML, args);
  try {
   ocams2isis(ui);
 }
 catch (IException &e) {
   FAIL() << "Unable to ingest OSIRIS-REx MapCam image: "
         << e.toString().toStdString().c_str()
         << std::endl;
 }

  // open cube and read label
  std::unique_ptr<Cube> cube (new Cube(cubeFileName));
  Pvl *isisLabel = cube->label();

  // Core object
  PvlObject &core = isisLabel->findObject("Core", Pvl::Traverse);
  EXPECT_EQ(int(core["StartByte"]), 65537);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, core.findKeyword("Format"), "Tile");
  EXPECT_EQ(int(core["TileSamples"]), 512);
  EXPECT_EQ(int(core["TileLines"]), 512);

  // Dimensions Group in Core
  PvlGroup &dim = core.findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ(int(dim["Samples"]), 1024);
  EXPECT_EQ(int(dim["Lines"]), 1024);
  EXPECT_EQ(int(dim["Bands"]), 1);

  // Pixels Group in Core
  PvlGroup &pixels = core.findGroup("Pixels", Pvl::Traverse);
  EXPECT_EQ(pixels["Type"][0].toStdString(), "Real");
  EXPECT_EQ(pixels["ByteOrder"][0].toStdString(), "Lsb");
  EXPECT_DOUBLE_EQ(double(pixels["Base"]), 0.0);
  EXPECT_DOUBLE_EQ(double(pixels["Multiplier"]), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["MissionName"][0].toStdString(), "OSIRIS-REx");
  EXPECT_EQ(inst["SpacecraftName"][0].toStdString(), "OSIRIS-REX");
  EXPECT_EQ(inst["InstrumentId"][0].toStdString(), "MapCam");
  EXPECT_EQ(inst["TargetName"][0].toStdString(), "Bennu");
  EXPECT_EQ(inst["StartTime"][0].toStdString(), "2019-02-23T02:30:51.790");
  EXPECT_DOUBLE_EQ(double(inst["ExposureDuration"]), 8.285275);
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0].toStdString(), "3/0604161014.25874");
  EXPECT_EQ(int(inst["FocusPosition"]), 270);
  EXPECT_EQ(inst["PolyCamFocusPositionNaifId"][0].toStdString(), "None");

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["FilterName"][0].toStdString(), "PAN");
  ASSERT_EQ(int(bandbin["Center"]), 650);

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifFrameCode"]), -64361);

  // Label object
  PvlObject &label = isisLabel->findObject("Label", Pvl::Traverse);
  EXPECT_EQ(int(label["Bytes"]), 65536);  
}


/**
   * FunctionalTestOcams2IsisSamCam
   * 
   * Ocams2isis test for ingestion of OSIRIS-REx OCams SamCam fits file.
   * 
   * Input ...
   *   SamCam fits file (data/osirisRexImages/ocams/20200811T221418S029_sam_iofL2pan5.fits)
   *
   * Output ...
   *   SamCam ISIS cube file
   */
TEST_F(TempTestingFiles, FunctionalTestOcams2IsisSamCam) {

  QString cubeFileName = tempDir.path() + "/20200811T221418S029_sam_iofL2pan5.cub";
  QVector<QString> args = {"from=data/osirisRexImages/ocams/20200811T221418S029_sam_iofL2pan5.fits",
                           "to=" + cubeFileName };

  UserInterface ui(APP_XML, args);
  try {
   ocams2isis(ui);
 }
 catch (IException &e) {
   FAIL() << "Unable to ingest OSIRIS-REx SamCam image: "
         << e.toString().toStdString().c_str()
         << std::endl;
 }

  // open cube and read label
  std::unique_ptr<Cube> cube (new Cube(cubeFileName));
  Pvl *isisLabel = cube->label();

  // Core object
  PvlObject &core = isisLabel->findObject("Core", Pvl::Traverse);
  EXPECT_EQ(int(core["StartByte"]), 65537);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, core.findKeyword("Format"), "Tile");
  EXPECT_EQ(int(core["TileSamples"]), 512);
  EXPECT_EQ(int(core["TileLines"]), 512);

  // Dimensions Group in Core
  PvlGroup &dim = core.findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ(int(dim["Samples"]), 1024);
  EXPECT_EQ(int(dim["Lines"]), 1024);
  EXPECT_EQ(int(dim["Bands"]), 1);

  // Pixels Group in Core
  PvlGroup &pixels = core.findGroup("Pixels", Pvl::Traverse);
  EXPECT_EQ(pixels["Type"][0].toStdString(), "Real");
  EXPECT_EQ(pixels["ByteOrder"][0].toStdString(), "Lsb");
  EXPECT_DOUBLE_EQ(double(pixels["Base"]), 0);
  EXPECT_DOUBLE_EQ(double(pixels["Multiplier"]), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["MissionName"][0].toStdString(), "OSIRIS-REx");
  EXPECT_EQ(inst["SpacecraftName"][0].toStdString(), "OSIRIS-REX");
  EXPECT_EQ(inst["InstrumentId"][0].toStdString(), "SamCam");
  EXPECT_EQ(inst["TargetName"][0].toStdString(), "Bennu");
  EXPECT_EQ(inst["StartTime"][0].toStdString(), "2020-08-11T22:14:18.010");
  EXPECT_DOUBLE_EQ(double(inst["ExposureDuration"]), 12.285275);
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0].toStdString(), "3/0650455990.36753");
  EXPECT_EQ(int(inst["FocusPosition"]), 120);
  EXPECT_EQ(inst["PolyCamFocusPositionNaifId"][0].toStdString(), "None");

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["FilterName"][0].toStdString(), "PAN5");
  ASSERT_EQ(bandbin["Center"][0].toStdString(), "NULL");

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifFrameCode"]), -64362);

  // Label object
  PvlObject &label = isisLabel->findObject("Label", Pvl::Traverse);
  EXPECT_EQ(int(label["Bytes"]), 65536);
}
