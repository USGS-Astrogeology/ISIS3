#include <QString>

#include "tagcams2isis.h"

#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlObject.h"
#include "TempFixtures.h"
#include "TestUtilities.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/tagcams2isis.xml").expanded();

/**
   * FunctionalTestTagcams2IsisNavCam
   * 
   * Tagcams2isis test of ingestion of OSIRIS-REx TagCams NAVCam fits file.
   * Input ...
   *   1) NAVCam fits file (data/tagcams2isis/20200303T213031S138_ncm_L0.fits)
   *   2) REMOVECALPIXELS=yes (default)
   *   3) TARGET=Label Value (default)
   *
   * Output ...
   *   NAVCam ISIS cube file
   */
TEST_F(TempTestingFiles, FunctionalTestTagcams2IsisNavCam) {
  QString cubeFileName = tempDir.path() + "/20200303T213031S138_ncm_L0.cub";
  QVector<QString> args = {"from=data/tagcams2isis/20200303T213031S138_ncm_L0.fits",
                           "to=" + cubeFileName };

  UserInterface ui(APP_XML, args);
  try {
   tagcams2isis(ui);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest OSIRIS-REx NAVCam image: "
          << e.toString().toStdString().c_str()
          << std::endl;
  }

  // open cube and read label
  std::unique_ptr<Cube> cube (new Cube(cubeFileName));
  Pvl *isisLabel = cube->label();

  // Core object
  PvlObject &core = isisLabel->findObject("Core", Pvl::Traverse);
  EXPECT_EQ(int(core["StartByte"]), 65537);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, core.findKeyword("Format"), "Tile");
  EXPECT_EQ(int(core["TileSamples"]), 864);
  EXPECT_EQ(int(core["TileLines"]), 972);

  // Dimensions Group in Core
  PvlGroup &dim = core.findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ(int(dim["Samples"]), 2592);
  EXPECT_EQ(int(dim["Lines"]), 1944);
  EXPECT_EQ(int(dim["Bands"]), 1);

  // Pixels Group in Core
  PvlGroup &pixels = core.findGroup("Pixels", Pvl::Traverse);
  EXPECT_EQ(pixels["Type"][0], "SignedWord");
  EXPECT_EQ(pixels["ByteOrder"][0], "Lsb");
  EXPECT_DOUBLE_EQ(double(pixels["Base"]), 32768.0);
  EXPECT_DOUBLE_EQ(double(pixels["Multiplier"]), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["MissionName"][0], "OSIRIS-REx");
  EXPECT_EQ(inst["SpacecraftName"][0], "OSIRIS-REX");
  EXPECT_EQ(inst["InstrumentId"][0], "NAVCam");
  EXPECT_EQ(inst["InstrumentName"][0], "TAGCAMS");
  EXPECT_EQ(inst["TargetName"][0], "Bennu");
  EXPECT_EQ(inst["StartTime"][0], "2020-03-03T21:30:31.138");
  EXPECT_EQ(inst["MidObservationTime"][0], "2020-03-03T21:30:31.140"); 
  EXPECT_DOUBLE_EQ(double(inst["ExposureDuration"]), 0.0042976);
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "3/0636542973.02560");
  EXPECT_DOUBLE_EQ(double(inst["EphemerisTime"]), 636543100.32543004);
  EXPECT_EQ(int(inst["Binning"]), 0);
  EXPECT_EQ(int(inst["Summing"]), 0);
  EXPECT_EQ(int(inst["BScale"]), 1);
  EXPECT_EQ(int(inst["DataCollectionMode"]), 80);
  EXPECT_EQ(int(inst["CompressionMode"]), 0);
  EXPECT_EQ(int(inst["SensorAnalogGain"]), 10);
  EXPECT_DOUBLE_EQ(double(inst["XReferenceCoordinate"]), 1413.083);
  EXPECT_DOUBLE_EQ(double(inst["YReferenceCoordinate"]), 1004.747);
  EXPECT_EQ(int(inst["StartXFactor"]), 0);
  EXPECT_EQ(int(inst["StartYFactor"]), 0);
  EXPECT_DOUBLE_EQ(double(inst["CameraHeadTemperature"]), -28.89233);

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(archive["SourceProductId"][0], "20200303T213031S138_ncm_L0");
  EXPECT_EQ(archive["ProducerId"][0], "SPOC");
  EXPECT_EQ(archive["MetaKernel"][0], "spoc-digestfits-2020-03-10T15_08_20.712Z.mk");
  EXPECT_EQ(int(archive["TagcamsCommandedSequenceId"]), 85);
  EXPECT_EQ(int(archive["TagcamsCommandedImageId"]), 6);
  EXPECT_EQ(int(archive["RawCameraHeadTemperature"]), 1613);
  EXPECT_DOUBLE_EQ(double(archive["RAAtReferencePixel"]), 332.5198707158909);
  EXPECT_DOUBLE_EQ(double(archive["DecAtReferencePixel"]), 68.711647070065496);
  EXPECT_EQ(int(archive["YearDoy"]), 2020063);

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["FilterName"][0], "Monochrome");
  EXPECT_EQ(bandbin["Name"][0], "NAVCam");
  ASSERT_EQ(int(bandbin["Number"]), 1);
  ASSERT_EQ(int(bandbin["Center"]), 550);

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifFrameCode"]), -64081);

  // Label object
  PvlObject &label = isisLabel->findObject("Label", Pvl::Traverse);
  EXPECT_EQ(int(label["Bytes"]), 65536);

  // OriginalLabel object
  PvlObject &origLabel = isisLabel->findObject("OriginalLabel", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, origLabel.findKeyword("Name"), "IsisCube");
  EXPECT_EQ(int(origLabel["StartByte"]), 10143233);
  EXPECT_EQ(int(origLabel["Bytes"]), 5638);
}


/**
   * FunctionalTestTagcams2IsisNavCamRemCalPixOff
   * 
   * Tagcams2isis test of ingestion of OSIRIS-REx TagCams NAVCam fits file with
   * REMOVECALPIXELS=no.
   * Input ...
   *   1) NAVCam fits file (data/tagcams2isis/20200303T213031S138_ncm_L0.fits)
   *   2) REMOVECALPIXELS=no
   *   3) TARGET=Label Value (default)
   *
   * Output ...
   *   NAVCam ISIS cube file
   */
TEST_F(TempTestingFiles, FunctionalTestTagcams2IsisNavCamRemCalPixOff) {
  QString cubeFileName = tempDir.path() + "/20200303T213031S138_ncm_L0.cub";
  QVector<QString> args = {"from=data/tagcams2isis/20200303T213031S138_ncm_L0.fits",
                           "to=" + cubeFileName,
                           "removecalpixels=no"};

  UserInterface ui(APP_XML, args);
  try {
   tagcams2isis(ui);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest OSIRIS-REx NAVCam image: "
          << e.toString().toStdString().c_str()
          << std::endl;
  }

  // open cube and read label
  std::unique_ptr<Cube> cube (new Cube(cubeFileName));
  Pvl *isisLabel = cube->label();

  // Core object
  PvlObject &core = isisLabel->findObject("Core", Pvl::Traverse);
  EXPECT_EQ(int(core["StartByte"]), 65537);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, core.findKeyword("Format"), "Tile");
  EXPECT_EQ(int(core["TileSamples"]), 688);
  EXPECT_EQ(int(core["TileLines"]), 1002);

  // Dimensions Group in Core
  PvlGroup &dim = core.findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ(int(dim["Samples"]), 2752);
  EXPECT_EQ(int(dim["Lines"]), 2004);
  EXPECT_EQ(int(dim["Bands"]), 1);

  // Pixels Group in Core
  PvlGroup &pixels = core.findGroup("Pixels", Pvl::Traverse);
  EXPECT_EQ(pixels["Type"][0], "SignedWord");
  EXPECT_EQ(pixels["ByteOrder"][0], "Lsb");
  EXPECT_DOUBLE_EQ(double(pixels["Base"]), 32768.0);
  EXPECT_DOUBLE_EQ(double(pixels["Multiplier"]), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["MissionName"][0], "OSIRIS-REx");
  EXPECT_EQ(inst["SpacecraftName"][0], "OSIRIS-REX");
  EXPECT_EQ(inst["InstrumentId"][0], "NAVCam");
  EXPECT_EQ(inst["InstrumentName"][0], "TAGCAMS");
  EXPECT_EQ(inst["TargetName"][0], "Bennu");
  EXPECT_EQ(inst["StartTime"][0], "2020-03-03T21:30:31.138");
  EXPECT_EQ(inst["MidObservationTime"][0], "2020-03-03T21:30:31.140"); 
  EXPECT_DOUBLE_EQ(double(inst["ExposureDuration"]), 0.0042976);
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "3/0636542973.02560");
  EXPECT_DOUBLE_EQ(double(inst["EphemerisTime"]), 636543100.32543004);
  EXPECT_EQ(int(inst["Binning"]), 0);
  EXPECT_EQ(int(inst["Summing"]), 0);
  EXPECT_EQ(int(inst["BScale"]), 1);
  EXPECT_EQ(int(inst["DataCollectionMode"]), 80);
  EXPECT_EQ(int(inst["CompressionMode"]), 0);
  EXPECT_EQ(int(inst["SensorAnalogGain"]), 10);
  EXPECT_DOUBLE_EQ(double(inst["XReferenceCoordinate"]), 1413.083);
  EXPECT_DOUBLE_EQ(double(inst["YReferenceCoordinate"]), 1004.747);
  EXPECT_EQ(int(inst["StartXFactor"]), 0);
  EXPECT_EQ(int(inst["StartYFactor"]), 0);
  EXPECT_DOUBLE_EQ(double(inst["CameraHeadTemperature"]), -28.89233);

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(archive["SourceProductId"][0], "20200303T213031S138_ncm_L0");
  EXPECT_EQ(archive["ProducerId"][0], "SPOC");
  EXPECT_EQ(archive["MetaKernel"][0], "spoc-digestfits-2020-03-10T15_08_20.712Z.mk");
  EXPECT_EQ(int(archive["TagcamsCommandedSequenceId"]), 85);
  EXPECT_EQ(int(archive["TagcamsCommandedImageId"]), 6);
  EXPECT_EQ(int(archive["RawCameraHeadTemperature"]), 1613);
  EXPECT_DOUBLE_EQ(double(archive["RAAtReferencePixel"]), 332.51987071589099);
  EXPECT_DOUBLE_EQ(double(archive["DecAtReferencePixel"]), 68.711647070065496);
  EXPECT_EQ(int(archive["YearDoy"]), 2020063);

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["FilterName"][0], "Monochrome");
  EXPECT_EQ(bandbin["Name"][0], "NAVCam");
  ASSERT_EQ(int(bandbin["Number"]), 1);
  ASSERT_EQ(int(bandbin["Center"]), 550);

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifFrameCode"]), -64081);

  // Label object
  PvlObject &label = isisLabel->findObject("Label", Pvl::Traverse);
  EXPECT_EQ(int(label["Bytes"]), 65536);

  // OriginalLabel object
  PvlObject &origLabel = isisLabel->findObject("OriginalLabel", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, origLabel.findKeyword("Name"), "IsisCube");
  EXPECT_EQ(int(origLabel["StartByte"]), 11095553);
  EXPECT_EQ(int(origLabel["Bytes"]), 5638);
}


/**
   * FunctionalTestTagcams2IsisNftCam
   * 
   * Tagcams2isis test of ingestion of OSIRIS-REx TagCams NFTCam fits file.
   * Input ...
   *   1) NAVCam fits file (data/tagcams2isis/20201020T214241S004_nft_L0.fits)
   *   2) REMOVECALPIXELS=yes (default)
   *   3) TARGET=Label Value (default)
   *
   * Output ...
   *   NFTCam ISIS cube file
   */
TEST_F(TempTestingFiles, FunctionalTestTagcams2IsisNftCam) {
  QString cubeFileName = tempDir.path() + "/20201020T214241S004_nft_L0";
  QVector<QString> args = {"from=data/tagcams2isis/20201020T214241S004_nft_L0.fits",
                           "to=" + cubeFileName };

  UserInterface ui(APP_XML, args);
  try {
   tagcams2isis(ui);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest OSIRIS-REx NFTCam image: "
          << e.toString().toStdString().c_str()
          << std::endl;
  }

  // open cube and read label
  std::unique_ptr<Cube> cube (new Cube(cubeFileName));
  Pvl *isisLabel = cube->label();

  // Core object
  PvlObject &core = isisLabel->findObject("Core", Pvl::Traverse);
  EXPECT_EQ(int(core["StartByte"]), 65537);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, core.findKeyword("Format"), "Tile");
  EXPECT_EQ(int(core["TileSamples"]), 864);
  EXPECT_EQ(int(core["TileLines"]), 972);

  // Dimensions Group in Core
  PvlGroup &dim = core.findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ(int(dim["Samples"]), 2592);
  EXPECT_EQ(int(dim["Lines"]), 1944);
  EXPECT_EQ(int(dim["Bands"]), 1);

  // Pixels Group in Core
  PvlGroup &pixels = core.findGroup("Pixels", Pvl::Traverse);
  EXPECT_EQ(pixels["Type"][0], "SignedWord");
  EXPECT_EQ(pixels["ByteOrder"][0], "Lsb");
  EXPECT_DOUBLE_EQ(double(pixels["Base"]), 32768.0);
  EXPECT_DOUBLE_EQ(double(pixels["Multiplier"]), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["MissionName"][0], "OSIRIS-REx");
  EXPECT_EQ(inst["SpacecraftName"][0], "OSIRIS-REX");
  EXPECT_EQ(inst["InstrumentId"][0], "NFTCam");
  EXPECT_EQ(inst["InstrumentName"][0], "TAGCAMS");
  EXPECT_EQ(inst["TargetName"][0], "Bennu");
  EXPECT_EQ(inst["StartTime"][0], "2020-10-20T21:42:40.974");
  EXPECT_EQ(inst["MidObservationTime"][0], "2020-10-20T21:42:40.976"); 
  EXPECT_DOUBLE_EQ(double(inst["ExposureDuration"]), 0.005894);
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "3/0656502090.40704");
  EXPECT_DOUBLE_EQ(double(inst["EphemerisTime"]), 656502230.15840399);
  EXPECT_EQ(int(inst["Binning"]), 0);
  EXPECT_EQ(int(inst["Summing"]), 0);
  EXPECT_EQ(int(inst["BScale"]), 1);
  EXPECT_EQ(int(inst["DataCollectionMode"]), 75);
  EXPECT_EQ(int(inst["CompressionMode"]), 0);
  EXPECT_EQ(int(inst["SensorAnalogGain"]), 10);
  EXPECT_DOUBLE_EQ(double(inst["XReferenceCoordinate"]), 1310.53);
  EXPECT_DOUBLE_EQ(double(inst["YReferenceCoordinate"]), 969.487);
  EXPECT_EQ(int(inst["StartXFactor"]), 0);
  EXPECT_EQ(int(inst["StartYFactor"]), 0);
  EXPECT_DOUBLE_EQ(double(inst["CameraHeadTemperature"]), -21.04614);

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(archive["SourceProductId"][0], "20201020T214241S004_nft_L0");
  EXPECT_EQ(archive["ProducerId"][0], "SPOC");
  EXPECT_EQ(archive["MetaKernel"][0], "spoc-digestfits-2021-02-13T22_56_01.274Z.mk");
  EXPECT_EQ(int(archive["TagcamsCommandedSequenceId"]), 200);
  EXPECT_EQ(int(archive["TagcamsCommandedImageId"]), 21);
  EXPECT_EQ(int(archive["RawCameraHeadTemperature"]), 1654);
  EXPECT_DOUBLE_EQ(double(archive["RAAtReferencePixel"]), 208.903477375357);
  EXPECT_DOUBLE_EQ(double(archive["DecAtReferencePixel"]), 16.561402529034101);
  EXPECT_EQ(int(archive["YearDoy"]), 2020294);

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["FilterName"][0], "Monochrome");
  EXPECT_EQ(bandbin["Name"][0], "NFTCam");
  ASSERT_EQ(int(bandbin["Number"]), 1);
  ASSERT_EQ(int(bandbin["Center"]), 550);

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifFrameCode"]), -64082);

  // Label object
  PvlObject &label = isisLabel->findObject("Label", Pvl::Traverse);
  EXPECT_EQ(int(label["Bytes"]), 65536);

  // OriginalLabel object
  PvlObject &origLabel = isisLabel->findObject("OriginalLabel", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, origLabel.findKeyword("Name"), "IsisCube");
  EXPECT_EQ(int(origLabel["StartByte"]), 10143233);
  EXPECT_EQ(int(origLabel["Bytes"]), 5635);
}


/**
   * FunctionalTestTagcams2IsisStowCam
   * 
   * Tagcams2isis test of ingestion of OSIRIS-REx TagCams StowCam fits file.
   * Input ...
   *   1) NAVCam fits file (data/tagcams2isis/20191211T191327S037_sto_L0.fits)
   *   2) REMOVECALPIXELS=yes (default)
   *   3) TARGET=Label Value (default)
   *
   * Output ...
   *   StowCam ISIS cube file
   */
TEST_F(TempTestingFiles, FunctionalTestTagcams2IsisStowCam) {
  QString cubeFileName = tempDir.path() + "/20191211T191327S037_sto_L0.cub";
  QVector<QString> args = {"from=data/tagcams2isis/20191211T191327S037_sto_L0.fits",
                           "to=" + cubeFileName };

  UserInterface ui(APP_XML, args);
  try {
   tagcams2isis(ui);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest OSIRIS-REx StowCam image: "
          << e.toString().toStdString().c_str()
          << std::endl;
  }

  // open cube and read label
  std::unique_ptr<Cube> cube (new Cube(cubeFileName));
  Pvl *isisLabel = cube->label();

  // Core object
  PvlObject &core = isisLabel->findObject("Core", Pvl::Traverse);
  EXPECT_EQ(int(core["StartByte"]), 65537);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, core.findKeyword("Format"), "Tile");
  EXPECT_EQ(int(core["TileSamples"]), 864);
  EXPECT_EQ(int(core["TileLines"]), 972);

  // Dimensions Group in Core
  PvlGroup &dim = core.findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ(int(dim["Samples"]), 2592);
  EXPECT_EQ(int(dim["Lines"]), 1944);
  EXPECT_EQ(int(dim["Bands"]), 1);

  // Pixels Group in Core
  PvlGroup &pixels = core.findGroup("Pixels", Pvl::Traverse);
  EXPECT_EQ(pixels["Type"][0], "SignedWord");
  EXPECT_EQ(pixels["ByteOrder"][0], "Lsb");
  EXPECT_DOUBLE_EQ(double(pixels["Base"]), 32768.0);
  EXPECT_DOUBLE_EQ(double(pixels["Multiplier"]), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["MissionName"][0], "OSIRIS-REx");
  EXPECT_EQ(inst["SpacecraftName"][0], "OSIRIS-REX");
  EXPECT_EQ(inst["InstrumentId"][0], "StowCam");
  EXPECT_EQ(inst["InstrumentName"][0], "TAGCAMS");
  EXPECT_EQ(inst["TargetName"][0], "Bennu");
  EXPECT_EQ(inst["StartTime"][0], "2019-12-11T19:13:26.938");
  EXPECT_EQ(inst["MidObservationTime"][0], "2019-12-11T19:13:26.952"); 
  EXPECT_DOUBLE_EQ(double(inst["ExposureDuration"]), 0.0299888);
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "3/0629363554.01024");
  EXPECT_DOUBLE_EQ(double(inst["EphemerisTime"]), 629363676.135341);
  EXPECT_EQ(int(inst["Binning"]), 0);
  EXPECT_EQ(int(inst["Summing"]), 0);
  EXPECT_EQ(int(inst["BScale"]), 1);
  EXPECT_EQ(int(inst["DataCollectionMode"]), 75);
  EXPECT_EQ(int(inst["CompressionMode"]), 0);
  EXPECT_EQ(int(inst["SensorAnalogGain"]), 8);
  EXPECT_DOUBLE_EQ(double(inst["XReferenceCoordinate"]), 1296.5);
  EXPECT_DOUBLE_EQ(double(inst["YReferenceCoordinate"]), 972.5);
  EXPECT_EQ(int(inst["StartXFactor"]), 0);
  EXPECT_EQ(int(inst["StartYFactor"]), 0);
  EXPECT_DOUBLE_EQ(double(inst["CameraHeadTemperature"]), -7.17563);

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(archive["SourceProductId"][0], "20191211T191327S037_sto_L0");
  EXPECT_EQ(archive["ProducerId"][0], "SPOC");
  EXPECT_EQ(archive["MetaKernel"][0], "spoc-digestfits-2020-02-05T20_27_37.424Z.mk");
  EXPECT_EQ(int(archive["TagcamsCommandedSequenceId"]), 190);
  EXPECT_EQ(int(archive["TagcamsCommandedImageId"]), 0);
  EXPECT_EQ(int(archive["RawCameraHeadTemperature"]), 1743);
  EXPECT_DOUBLE_EQ(double(archive["RAAtReferencePixel"]), 104.599330984022);
  EXPECT_DOUBLE_EQ(double(archive["DecAtReferencePixel"]), -55.6269193033577);
  EXPECT_EQ(int(archive["YearDoy"]), 2019345);

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["FilterName"][0], "Monochrome");
  EXPECT_EQ(bandbin["Name"][0], "StowCam");
  ASSERT_EQ(int(bandbin["Number"]), 1);
  ASSERT_EQ(int(bandbin["Center"]), 550);

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifFrameCode"]), -64071);

  // Label object
  PvlObject &label = isisLabel->findObject("Label", Pvl::Traverse);
  EXPECT_EQ(int(label["Bytes"]), 65536);

  // OriginalLabel object
  PvlObject &origLabel = isisLabel->findObject("OriginalLabel", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, origLabel.findKeyword("Name"), "IsisCube");
  EXPECT_EQ(int(origLabel["StartByte"]), 10143233);
  EXPECT_EQ(int(origLabel["Bytes"]), 5546);
}