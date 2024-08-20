#include <QTextStream>
#include <QTemporaryDir>
#include <QStringList>
#include <QFile>
#include <QDebug>

#include "lrowac2pds.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "SpecialPixel.h"

#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/lrowac2pds.xml").expanded();

TEST(Lrowac2pds, FunctionalTestLrowac2pdsUv) {
  QTemporaryDir prefix;
  QVector<QString> args = {"fromlist=data/lrowac2pds/uv/wac0001832b.lis",
                           "to=" + prefix.path() + "/lrowac2pdsTEMP.img"};

  UserInterface options(APP_XML, args);

  lrowac2pds(options);

  Pvl outputLabel(options.GetFileName("TO").toStdString());
  PvlObject imageObject = outputLabel.findObject("IMAGE");

  // File Characteristics
  EXPECT_EQ((std::string)outputLabel["PDS_VERSION_ID"], "PDS3");
  EXPECT_EQ((std::string)outputLabel["RECORD_TYPE"], "FIXED_LENGTH");
  EXPECT_EQ(int(outputLabel["RECORD_BYTES"]), 128);
  EXPECT_EQ(int(outputLabel["FILE_RECORDS"]), 8780);
  EXPECT_EQ(int(outputLabel["LABEL_RECORDS"]), 76);
  EXPECT_EQ(int(outputLabel["^IMAGE"]), 77);

  // Data Identification
  EXPECT_EQ((std::string)outputLabel["DATA_SET_ID"], "LRO-L-LROC-3-CDR-V1.0");
  EXPECT_EQ((std::string)outputLabel["PRODUCT_ID"], "M115631721UC");
  EXPECT_EQ((std::string)outputLabel["MISSION_NAME"], "LUNAR RECONNAISSANCE ORBITER");
  EXPECT_EQ((std::string)outputLabel["MISSION_PHASE_NAME"], "NOMINAL MISSION");
  EXPECT_EQ(int(outputLabel["ORBIT_NUMBER"]), 2174);

  // Data Description
  EXPECT_EQ((std::string)outputLabel["TARGET_NAME"], "MOON");
  EXPECT_EQ((std::string)outputLabel["RATIONALE_DESC"], "GLOBAL COVERAGE");
  EXPECT_EQ((std::string)outputLabel["DATA_QUALITY_ID"], "0");

  // Environment
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:BEGIN_TEMPERATURE_SCS"]), 2.10);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:MIDDLE_TEMPERATURE_SCS"]), 2.08);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:END_TEMPERATURE_SCS"]), 2.13);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:BEGIN_TEMPERATURE_FPA"]), -23.55);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:MIDDLE_TEMPERATURE_FPA"]), -23.33);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:END_TEMPERATURE_FPA"]), -23.06);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:BEGIN_TEMPERATURE_SCS_RAW"]), 2849);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:MIDDLE_TEMPERATURE_SCS_RAW"]), 2850);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:BEGIN_TEMPERATURE_FPA_RAW"]), 3728);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:MIDDLE_TEMPERATURE_FPA_RAW"]), 3723);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:END_TEMPERATURE_FPA_RAW"]), 3717);

  // Imaging Parameters
  EXPECT_DOUBLE_EQ(double(outputLabel["EXPOSURE_DURATION"]), 48.0);
  EXPECT_EQ(int(outputLabel["LRO:EXPOSURE_CODE"]), 480);
  EXPECT_DOUBLE_EQ(double(outputLabel["INTERFRAME_DELAY"]), 703.125);
  EXPECT_EQ((std::string)outputLabel["INSTRUMENT_MODE_ID"], "UV");
  EXPECT_EQ((std::string)outputLabel["FILTER_NUMBER"][0], "1");
  EXPECT_EQ((std::string)outputLabel["FILTER_NUMBER"][1], "2");
  EXPECT_EQ((std::string)outputLabel["CENTER_FILTER_WAVELENGTH"][0], "321");
  EXPECT_EQ((std::string)outputLabel["CENTER_FILTER_WAVELENGTH"][1], "360");
  EXPECT_EQ((std::string)outputLabel["BANDWIDTH"][0], "32");
  EXPECT_EQ((std::string)outputLabel["BANDWIDTH"][1], "15");

  // Data Object
  EXPECT_EQ(int(imageObject["SAMPLE_BITS"]), 32);
  EXPECT_EQ((std::string)imageObject["SAMPLE_TYPE"], "PC_REAL");
  EXPECT_EQ((std::string)imageObject["VALID_MINIMUM"], "16#FF7FFFFA#");
  EXPECT_EQ((std::string)imageObject["NULL"], "16#FF7FFFFB#");
  EXPECT_EQ((std::string)imageObject["LOW_REPR_SATURATION"], "16#FF7FFFFC#");
  EXPECT_EQ((std::string)imageObject["LOW_INSTR_SATURATION"], "16#FF7FFFFD#");
  EXPECT_EQ((std::string)imageObject["HIGH_INSTR_SATURATION"], "16#FF7FFFFE#");
  EXPECT_EQ((std::string)imageObject["HIGH_REPR_SATURATION"], "16#FF7FFFFF#");
  EXPECT_EQ((std::string)imageObject["UNIT"], "I/F");
  EXPECT_EQ((std::string)imageObject["MD5_CHECKSUM"], "72b64e9200aa6b03838b166da9b0159d");
}

TEST(Lrowac2pds, FunctionalTestLrowac2pdsVis) {
  QTemporaryDir prefix;
  QVector<QString> args = {"fromlist=data/lrowac2pds/vis/wac00002cf4.lis",
                           "to=" + prefix.path() + "/lrowac2pdsTEMP.img"};

  UserInterface options(APP_XML, args);

  lrowac2pds(options);

  Pvl outputLabel(options.GetFileName("TO").toStdString());
  PvlObject imageObject = outputLabel.findObject("IMAGE");

  // File Characteristics
  EXPECT_EQ((std::string)outputLabel["PDS_VERSION_ID"], "PDS3");
  EXPECT_EQ((std::string)outputLabel["RECORD_TYPE"], "FIXED_LENGTH");
  EXPECT_EQ(int(outputLabel["FILE_RECORDS"]), 27175);
  EXPECT_EQ(int(outputLabel["LABEL_RECORDS"]), 15);
  EXPECT_EQ(int(outputLabel["^IMAGE"]), 16);

  // Data Identification
  EXPECT_EQ((std::string)outputLabel["DATA_SET_ID"], "LRO-L-LROC-3-CDR-V1.0");
  EXPECT_EQ((std::string)outputLabel["PRODUCT_ID"], "M103709659VC");
  EXPECT_EQ((std::string)outputLabel["MISSION_NAME"], "LUNAR RECONNAISSANCE ORBITER");
  EXPECT_EQ((std::string)outputLabel["MISSION_PHASE_NAME"], "COMMISSIONING");
  EXPECT_EQ(int(outputLabel["ORBIT_NUMBER"]), 449);

  // Data Description
  EXPECT_EQ((std::string)outputLabel["TARGET_NAME"], "MOON");
  EXPECT_EQ((std::string)outputLabel["RATIONALE_DESC"], "GLOBAL COVERAGE");
  EXPECT_EQ((std::string)outputLabel["DATA_QUALITY_ID"], "32");

  // Environment
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:BEGIN_TEMPERATURE_SCS"]), 2.62);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:MIDDLE_TEMPERATURE_SCS"]), 2.51);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:END_TEMPERATURE_SCS"]), 2.56);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:BEGIN_TEMPERATURE_FPA"]), -24.85);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:MIDDLE_TEMPERATURE_FPA"]), -24.90);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:END_TEMPERATURE_FPA"]), -24.85);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:BEGIN_TEMPERATURE_SCS_RAW"]), 2827);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:MIDDLE_TEMPERATURE_SCS_RAW"]), 2832);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:BEGIN_TEMPERATURE_FPA_RAW"]), 3758);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:MIDDLE_TEMPERATURE_FPA_RAW"]), 3759);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:END_TEMPERATURE_FPA_RAW"]), 3758);

  // Imaging Parameters
  EXPECT_DOUBLE_EQ(double(outputLabel["EXPOSURE_DURATION"]), 50.0);
  EXPECT_EQ(int(outputLabel["LRO:EXPOSURE_CODE"]), 500);
  EXPECT_DOUBLE_EQ(double(outputLabel["INTERFRAME_DELAY"]), 2234.375);
  EXPECT_EQ((std::string)outputLabel["INSTRUMENT_MODE_ID"], "VIS");
  EXPECT_EQ((std::string)outputLabel["FILTER_NUMBER"][0], "3");
  EXPECT_EQ((std::string)outputLabel["FILTER_NUMBER"][1], "4");
  EXPECT_EQ((std::string)outputLabel["FILTER_NUMBER"][2], "5");
  EXPECT_EQ((std::string)outputLabel["FILTER_NUMBER"][3], "6");
  EXPECT_EQ((std::string)outputLabel["FILTER_NUMBER"][4], "7");
  EXPECT_EQ((std::string)outputLabel["CENTER_FILTER_WAVELENGTH"][0], "415");
  EXPECT_EQ((std::string)outputLabel["CENTER_FILTER_WAVELENGTH"][1], "566");
  EXPECT_EQ((std::string)outputLabel["CENTER_FILTER_WAVELENGTH"][2], "604");
  EXPECT_EQ((std::string)outputLabel["CENTER_FILTER_WAVELENGTH"][3], "643");
  EXPECT_EQ((std::string)outputLabel["CENTER_FILTER_WAVELENGTH"][4], "689");
  EXPECT_EQ((std::string)outputLabel["BANDWIDTH"][0], "36");
  EXPECT_EQ((std::string)outputLabel["BANDWIDTH"][1], "20");
  EXPECT_EQ((std::string)outputLabel["BANDWIDTH"][2], "20");
  EXPECT_EQ((std::string)outputLabel["BANDWIDTH"][3], "23");
  EXPECT_EQ((std::string)outputLabel["BANDWIDTH"][4], "39");
  EXPECT_EQ(int(outputLabel["LRO:COMPRESSION_FLAG"]), 0);
  EXPECT_EQ(int(outputLabel["LRO:MODE"]), 0);
  EXPECT_EQ(int(outputLabel["LRO:NFRAMES"]), 97);
  EXPECT_EQ(int(outputLabel["LRO:BAND_CODE"]), 31);
  EXPECT_EQ(int(outputLabel["LRO:INTERFRAME_GAP_CODE"]), 118);
  EXPECT_EQ(int(outputLabel["LRO:COMPAND_CODE"]), 0);
  EXPECT_EQ(int(outputLabel["LRO:BACKGROUND_OFFSET"]), 56);

  // Data Object
  EXPECT_EQ(int(imageObject["SAMPLE_BITS"]), 32);
  EXPECT_EQ((std::string)imageObject["SAMPLE_TYPE"], "PC_REAL");
  EXPECT_EQ((std::string)imageObject["VALID_MINIMUM"], "16#FF7FFFFA#");
  EXPECT_EQ((std::string)imageObject["NULL"], "16#FF7FFFFB#");
  EXPECT_EQ((std::string)imageObject["LOW_REPR_SATURATION"], "16#FF7FFFFC#");
  EXPECT_EQ((std::string)imageObject["LOW_INSTR_SATURATION"], "16#FF7FFFFD#");
  EXPECT_EQ((std::string)imageObject["HIGH_INSTR_SATURATION"], "16#FF7FFFFE#");
  EXPECT_EQ((std::string)imageObject["HIGH_REPR_SATURATION"], "16#FF7FFFFF#");
  EXPECT_EQ((std::string)imageObject["UNIT"], "I/F");
  EXPECT_EQ((std::string)imageObject["MD5_CHECKSUM"], "03ff4198df40b995b4bf759e0eab49b9");
}

TEST(Lrowac2pds, FunctionalTestLrowac2pdsColor) {
  QTemporaryDir prefix;
  QVector<QString> args = {"fromlist=data/lrowac2pds/color/wac0000983c.lis",
                           "to=" + prefix.path() + "/lrowac2pdsTEMP.img"};

  UserInterface options(APP_XML, args);

  lrowac2pds(options);

  Pvl outputLabel(options.GetFileName("TO").toStdString());
  PvlObject imageObject = outputLabel.findObject("IMAGE");

  // File Characteristics
  EXPECT_EQ((std::string)outputLabel["PDS_VERSION_ID"], "PDS3");
  EXPECT_EQ((std::string)outputLabel["RECORD_TYPE"], "FIXED_LENGTH");
  EXPECT_EQ(int(outputLabel["RECORD_BYTES"]), 704);
  EXPECT_EQ(int(outputLabel["FILE_RECORDS"]), 7503);
  EXPECT_EQ(int(outputLabel["LABEL_RECORDS"]), 15);
  EXPECT_EQ(int(outputLabel["^IMAGE"]), 16);

  // Environment
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:BEGIN_TEMPERATURE_SCS"]), 9.25);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:MIDDLE_TEMPERATURE_SCS"]), 9.36);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:END_TEMPERATURE_SCS"]), 9.37);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:BEGIN_TEMPERATURE_FPA"]), -2.04);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:MIDDLE_TEMPERATURE_FPA"]), -1.75);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:END_TEMPERATURE_FPA"]), -1.51);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:BEGIN_TEMPERATURE_SCS_RAW"]), 2519);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:MIDDLE_TEMPERATURE_SCS_RAW"]), 2514);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:BEGIN_TEMPERATURE_FPA_RAW"]), 3031);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:MIDDLE_TEMPERATURE_FPA_RAW"]), 3019);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:END_TEMPERATURE_FPA_RAW"]), 3007);

  // Imaging Parameters
  EXPECT_DOUBLE_EQ(double(outputLabel["EXPOSURE_DURATION"]), 45.0);
  EXPECT_EQ(int(outputLabel["LRO:EXPOSURE_CODE"]), 450);
  EXPECT_DOUBLE_EQ(double(outputLabel["INTERFRAME_DELAY"]), 4375.0);
  EXPECT_EQ((std::string)outputLabel["INSTRUMENT_MODE_ID"], "COLOR");
  EXPECT_EQ(int(outputLabel["LRO:COMPRESSION_FLAG"]), 0);
  EXPECT_EQ(int(outputLabel["LRO:MODE"]), 0);
  EXPECT_EQ(int(outputLabel["LRO:NFRAMES"]), 24);
  EXPECT_EQ(int(outputLabel["LRO:BAND_CODE"]), 127);
  EXPECT_EQ(int(outputLabel["LRO:INTERFRAME_GAP_CODE"]), 255);
  EXPECT_EQ(int(outputLabel["LRO:COMPAND_CODE"]), 0);
  EXPECT_EQ(int(outputLabel["LRO:BACKGROUND_OFFSET"]), 56);

  // Data Object
  EXPECT_EQ(int(imageObject["LINES"]), 1872);
  EXPECT_EQ(int(imageObject["LINE_SAMPLES"]), 704);
  EXPECT_EQ(int(imageObject["SAMPLE_BITS"]), 32);
}

TEST(Lrowac2pds, FunctionalTestLrowac2pdsMono) {
  QTemporaryDir prefix;
  QVector<QString> args = {"fromlist=data/lrowac2pds/mono/wac0002c120.lis",
                           "to=" + prefix.path() + "/lrowac2pdsTEMP.img"};

  UserInterface options(APP_XML, args);

  lrowac2pds(options);

  Pvl outputLabel(options.GetFileName("TO").toStdString());
  PvlObject imageObject = outputLabel.findObject("IMAGE");

  // File Characteristics
  EXPECT_EQ((std::string)outputLabel["PDS_VERSION_ID"], "PDS3");
  EXPECT_EQ((std::string)outputLabel["RECORD_TYPE"], "FIXED_LENGTH");
  EXPECT_EQ(int(outputLabel["RECORD_BYTES"]), 1024);
  EXPECT_EQ(int(outputLabel["FILE_RECORDS"]), 2250);
  EXPECT_EQ(int(outputLabel["LABEL_RECORDS"]), 10);
  EXPECT_EQ(int(outputLabel["^IMAGE"]), 11);

  // Environment
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:BEGIN_TEMPERATURE_SCS"]), 10.63);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:MIDDLE_TEMPERATURE_SCS"]), 10.65);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:END_TEMPERATURE_SCS"]), 10.66);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:BEGIN_TEMPERATURE_FPA"]), -11.08);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:MIDDLE_TEMPERATURE_FPA"]), -10.96);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:END_TEMPERATURE_FPA"]), -10.85);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:BEGIN_TEMPERATURE_SCS_RAW"]), 2452);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:MIDDLE_TEMPERATURE_SCS_RAW"]), 2451);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:BEGIN_TEMPERATURE_FPA_RAW"]), 3376);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:MIDDLE_TEMPERATURE_FPA_RAW"]), 3372);
  EXPECT_DOUBLE_EQ(double(outputLabel["LRO:END_TEMPERATURE_FPA_RAW"]), 3370);

  // Imaging Parameters
  EXPECT_DOUBLE_EQ(double(outputLabel["EXPOSURE_DURATION"]), 26.0);
  EXPECT_EQ(int(outputLabel["LRO:EXPOSURE_CODE"]), 260);
  EXPECT_DOUBLE_EQ(double(outputLabel["INTERFRAME_DELAY"]), 421.875);
  EXPECT_EQ((std::string)outputLabel["INSTRUMENT_MODE_ID"], "BW");
  EXPECT_EQ((std::string)outputLabel["FILTER_NUMBER"], "6");
  EXPECT_EQ(int(outputLabel["CENTER_FILTER_WAVELENGTH"]), 643);
  EXPECT_EQ(int(outputLabel["BANDWIDTH"]), 23);
  EXPECT_EQ(int(outputLabel["LRO:COMPRESSION_FLAG"]), 1);
  EXPECT_EQ(int(outputLabel["LRO:MODE"]), 3);
  EXPECT_EQ(int(outputLabel["LRO:NFRAMES"]), 40);
  EXPECT_EQ(int(outputLabel["LRO:BAND_CODE"]), 8);
  EXPECT_EQ(int(outputLabel["LRO:INTERFRAME_GAP_CODE"]), 2);
  EXPECT_EQ(int(outputLabel["LRO:COMPAND_CODE"]), 0);
  EXPECT_EQ(int(outputLabel["LRO:BACKGROUND_OFFSET"]), 64);

  // Data Object
  EXPECT_EQ(int(imageObject["LINES"]), 560);
  EXPECT_EQ(int(imageObject["LINE_SAMPLES"]), 1024);
  EXPECT_EQ(int(imageObject["SAMPLE_BITS"]), 32);
}
