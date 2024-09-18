#include <QTemporaryFile>
#include <QTemporaryDir>

#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "gmock/gmock.h"
#include "UserInterface.h"
#include "hidtmgen.h"
#include "ProcessImportPds.h"

using namespace Isis;
using ::testing::HasSubstr;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/hidtmgen.xml").expanded());

TEST(Hidtmgen, HidtmgenTestColor){
   //Serves as default test case -- test all keywords for all generated products.
   QTemporaryDir prefix;
   QVector<QString> args = {"defaultnames=TRUE",
                            "dtm=data/hidtmgen/dtm/Ares4_Marth_Crater_3557E_126N_ngate_03_lowres.cub",
                            "outputdir=" + prefix.path(),
                            "orthofromlist=data/hidtmgen/color/orthoInputList.txt",
                            "paramspvl=data/hidtmgen/color/params.pvl",
                            "orthosequencenumberlist=data/hidtmgen/color/sequenceNumbers.txt"
                           };

  UserInterface options(APP_XML, args);
  try {
    hidtmgen(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to HIRISE generate PDS products: " <<  e.toString().c_str() << std::endl;
  }

  Pvl dtmLabel(prefix.path().toStdString() +"/DTEEZ_042252_1930_042753_1930_A31.IMG");

  ASSERT_EQ(dtmLabel["RECORD_TYPE"][0], "FIXED_LENGTH");
  ASSERT_DOUBLE_EQ(dtmLabel["RECORD_BYTES"], 32);
  ASSERT_DOUBLE_EQ(dtmLabel["FILE_RECORDS"], 155);
  ASSERT_DOUBLE_EQ(dtmLabel["^IMAGE"], 133);

  ASSERT_EQ(dtmLabel["DATA_SET_ID"][0], "MRO-M-HIRISE-5-DTM-V1.0");
  ASSERT_EQ(dtmLabel["PRODUCER_INSTITUTION_NAME"][0], "UNIVERSITY OF ARIZONA");
  ASSERT_EQ(dtmLabel["PRODUCER_ID"][0], "UA");
  ASSERT_EQ(dtmLabel["PRODUCER_FULL_NAME"][0], "ALFRED MCEWEN");
  ASSERT_EQ(dtmLabel["PRODUCT_ID"][0], "DTEEZ_042252_1930_042753_1930_A31");
  ASSERT_DOUBLE_EQ(dtmLabel["PRODUCT_VERSION_ID"], 0.314);
  ASSERT_EQ(dtmLabel["INSTRUMENT_HOST_NAME"][0], "MARS RECONNAISSANCE ORBITER");
  ASSERT_EQ(dtmLabel["INSTRUMENT_NAME"][0], "HIGH RESOLUTION IMAGING SCIENCE EXPERIMENT");
  ASSERT_EQ(dtmLabel["INSTRUMENT_ID"][0], "HIRISE");
  ASSERT_EQ(dtmLabel["TARGET_NAME"][0], "MARS");
  ASSERT_EQ(dtmLabel["SOURCE_PRODUCT_ID"][0], "ESP_042252_1930");
  ASSERT_EQ(dtmLabel["SOURCE_PRODUCT_ID"][1], "ESP_042753_1930");
  ASSERT_EQ(dtmLabel["RATIONALE_DESC"][0], "NULL");


  PvlObject dtmImage = dtmLabel.findObject("IMAGE");
  ASSERT_DOUBLE_EQ(dtmImage["LINES"], 23);
  ASSERT_DOUBLE_EQ(dtmImage["LINE_SAMPLES"], 8);
  ASSERT_DOUBLE_EQ(dtmImage["BANDS"], 1);
  ASSERT_DOUBLE_EQ(dtmImage["OFFSET"], 0.0);
  ASSERT_DOUBLE_EQ(dtmImage["SCALING_FACTOR"], 1.0);
  ASSERT_DOUBLE_EQ(dtmImage["SAMPLE_BITS"], 32);
  ASSERT_EQ(dtmImage["SAMPLE_BIT_MASK"][0], "2#11111111111111111111111111111111#");
  ASSERT_EQ(dtmImage["SAMPLE_TYPE"][0], "PC_REAL");
  ASSERT_EQ(dtmImage["MISSING_CONSTANT"][0], "16#FF7FFFFB#");
  ASSERT_DOUBLE_EQ(dtmImage["VALID_MINIMUM"], -1884.17);
  ASSERT_DOUBLE_EQ(dtmImage["VALID_MAXIMUM"], -1324.12);

  PvlObject dtmProj = dtmLabel.findObject("IMAGE_MAP_PROJECTION");
  ASSERT_EQ(dtmProj["^DATA_SET_MAP_PROJECTION"][0], "DSMAP.CAT");
  ASSERT_EQ(dtmProj["MAP_PROJECTION_TYPE"][0], "EQUIRECTANGULAR");
  ASSERT_EQ(dtmProj["PROJECTION_LATITUDE_TYPE"][0], "PLANETOCENTRIC");
  ASSERT_DOUBLE_EQ(dtmProj["A_AXIS_RADIUS"], 3396.19);
  ASSERT_DOUBLE_EQ(dtmProj["B_AXIS_RADIUS"], 3396.19);
  ASSERT_DOUBLE_EQ(dtmProj["C_AXIS_RADIUS"], 3396.19);
  ASSERT_EQ(dtmProj["COORDINATE_SYSTEM_NAME"][0], "PLANETOCENTRIC");
  ASSERT_EQ(dtmProj["POSITIVE_LONGITUDE_DIRECTION"][0], "EAST");
  ASSERT_EQ(dtmProj["KEYWORD_LATITUDE_TYPE"][0], "PLANETOCENTRIC");
  ASSERT_DOUBLE_EQ(dtmProj["CENTER_LATITUDE"], 0.0);
  ASSERT_DOUBLE_EQ(dtmProj["CENTER_LONGITUDE"], 180.0);
  ASSERT_DOUBLE_EQ(dtmProj["LINE_FIRST_PIXEL"], 1);
  ASSERT_DOUBLE_EQ(dtmProj["LINE_LAST_PIXEL"], 23);
  ASSERT_DOUBLE_EQ(dtmProj["SAMPLE_FIRST_PIXEL"], 1);
  ASSERT_DOUBLE_EQ(dtmProj["SAMPLE_LAST_PIXEL"], 8);
  ASSERT_DOUBLE_EQ(dtmProj["MAP_PROJECTION_ROTATION"], 0.0);
  ASSERT_NEAR(dtmProj["MAP_RESOLUTION"], 59.27469, .00001);
  ASSERT_DOUBLE_EQ(dtmProj["MAP_SCALE"], 1000.0);
  ASSERT_NEAR(dtmProj["MAXIMUM_LATITUDE"], 12.82864, .00001);
  ASSERT_NEAR(dtmProj["MINIMUM_LATITUDE"], 12.45094, .00001);
  ASSERT_DOUBLE_EQ(dtmProj["LINE_PROJECTION_OFFSET"], 760.5);
  ASSERT_DOUBLE_EQ(dtmProj["SAMPLE_PROJECTION_OFFSET"], -10413.5);
  ASSERT_NEAR(dtmProj["EASTERNMOST_LONGITUDE"], 355.80733, .00001);
  ASSERT_NEAR(dtmProj["WESTERNMOST_LONGITUDE"], 355.68017, .00001);


  PvlObject dtmView = dtmLabel.findObject("VIEWING_PARAMETERS");
  ASSERT_DOUBLE_EQ(dtmView["NORTH_AZIMUTH"], 270.0);


  Pvl orthoLabel1(prefix.path().toStdString() +"/ESP_042252_1930_IRB_B_41_ORTHO.IMG");

  ASSERT_EQ(orthoLabel1["RECORD_TYPE"][0], "FIXED_LENGTH");
  ASSERT_DOUBLE_EQ(orthoLabel1["RECORD_BYTES"], 40);
  ASSERT_DOUBLE_EQ(orthoLabel1["FILE_RECORDS"], 252);
  ASSERT_DOUBLE_EQ(orthoLabel1["^IMAGE"], 103);

  ASSERT_EQ(orthoLabel1["DATA_SET_ID"][0], "MRO-M-HIRISE-5-DTM-V1.0");
  ASSERT_EQ(orthoLabel1["PRODUCER_INSTITUTION_NAME"][0], "UNIVERSITY OF ARIZONA");
  ASSERT_EQ(orthoLabel1["PRODUCER_ID"][0], "UA");
  ASSERT_EQ(orthoLabel1["PRODUCER_FULL_NAME"][0], "ALFRED MCEWEN");
  ASSERT_EQ(orthoLabel1["PRODUCT_ID"][0], "ESP_042252_1930_IRB_B_41_ORTHO");
  ASSERT_DOUBLE_EQ(orthoLabel1["PRODUCT_VERSION_ID"], 0.314);
  ASSERT_EQ(orthoLabel1["INSTRUMENT_HOST_NAME"][0], "MARS RECONNAISSANCE ORBITER");
  ASSERT_EQ(orthoLabel1["INSTRUMENT_HOST_ID"][0], "MRO");
  ASSERT_EQ(orthoLabel1["INSTRUMENT_NAME"][0], "HIGH RESOLUTION IMAGING SCIENCE EXPERIMENT");
  ASSERT_EQ(orthoLabel1["INSTRUMENT_ID"][0], "HIRISE");
  ASSERT_EQ(orthoLabel1["TARGET_NAME"][0], "MARS");
  ASSERT_EQ(orthoLabel1["SOURCE_PRODUCT_ID"][0], "DTEEZ_042252_1930_042753_1930_A31");
  ASSERT_EQ(orthoLabel1["SOURCE_PRODUCT_ID"][1], "ESP_042252_1930");
  ASSERT_EQ(orthoLabel1["RATIONALE_DESC"][0], "NULL");
  ASSERT_EQ(orthoLabel1["SOFTWARE_NAME"][0], "Socet_Set 5.4.1");
  ASSERT_EQ(orthoLabel1["RATIONALE_DESC"][0], "NULL");
  ASSERT_DOUBLE_EQ(orthoLabel1["LABEL_RECORDS"], 102);

  PvlObject orthoImage1 = orthoLabel1.findObject("IMAGE");
  ASSERT_DOUBLE_EQ(orthoImage1["LINES"], 50);
  ASSERT_DOUBLE_EQ(orthoImage1["LINE_SAMPLES"], 40);
  ASSERT_DOUBLE_EQ(orthoImage1["BANDS"], 3);
  ASSERT_DOUBLE_EQ(orthoImage1["OFFSET"], 0.0);
  ASSERT_DOUBLE_EQ(orthoImage1["SCALING_FACTOR"], 1.0);
  ASSERT_DOUBLE_EQ(orthoImage1["SAMPLE_BITS"], 8);
  ASSERT_EQ(orthoImage1["SAMPLE_TYPE"][0], "MSB_UNSIGNED_INTEGER");
  ASSERT_EQ(orthoImage1["BAND_STORAGE_TYPE"][0], "BAND_SEQUENTIAL");
  ASSERT_DOUBLE_EQ(orthoImage1["CORE_NULL"], 0);
  ASSERT_DOUBLE_EQ(orthoImage1["CORE_LOW_REPR_SATURATION"], 1);
  ASSERT_DOUBLE_EQ(orthoImage1["CORE_LOW_INSTR_SATURATION"], 1);
  ASSERT_DOUBLE_EQ(orthoImage1["CORE_HIGH_REPR_SATURATION"], 255);
  ASSERT_DOUBLE_EQ(orthoImage1["CORE_HIGH_INSTR_SATURATION"], 255);

  PvlObject orthoProj1 = orthoLabel1.findObject("IMAGE_MAP_PROJECTION");
  ASSERT_EQ(orthoProj1["^DATA_SET_MAP_PROJECTION"][0], "DSMAP.CAT");
  ASSERT_EQ(orthoProj1["MAP_PROJECTION_TYPE"][0], "EQUIRECTANGULAR");
  ASSERT_EQ(orthoProj1["PROJECTION_LATITUDE_TYPE"][0], "PLANETOCENTRIC");
  ASSERT_DOUBLE_EQ(orthoProj1["A_AXIS_RADIUS"], 3396.19);
  ASSERT_DOUBLE_EQ(orthoProj1["B_AXIS_RADIUS"], 3396.19);
  ASSERT_DOUBLE_EQ(orthoProj1["C_AXIS_RADIUS"], 3396.19);
  ASSERT_EQ(orthoProj1["COORDINATE_SYSTEM_NAME"][0], "PLANETOCENTRIC");
  ASSERT_EQ(orthoProj1["POSITIVE_LONGITUDE_DIRECTION"][0], "EAST");
  ASSERT_EQ(orthoProj1["KEYWORD_LATITUDE_TYPE"][0], "PLANETOCENTRIC");
  ASSERT_DOUBLE_EQ(orthoProj1["CENTER_LATITUDE"], 0.0);
  ASSERT_DOUBLE_EQ(orthoProj1["CENTER_LONGITUDE"], 180.0);
  ASSERT_DOUBLE_EQ(orthoProj1["LINE_FIRST_PIXEL"], 1);
  ASSERT_DOUBLE_EQ(orthoProj1["LINE_LAST_PIXEL"], 50);
  ASSERT_DOUBLE_EQ(orthoProj1["SAMPLE_FIRST_PIXEL"], 1);
  ASSERT_DOUBLE_EQ(orthoProj1["SAMPLE_LAST_PIXEL"], 40);
  ASSERT_DOUBLE_EQ(orthoProj1["MAP_PROJECTION_ROTATION"], 0.0);
  ASSERT_NEAR(orthoProj1["MAP_RESOLUTION"], 117259.25436, .00001);
  ASSERT_NEAR(orthoProj1["MAP_SCALE"], 0.50550, .00001);
  ASSERT_NEAR(orthoProj1["MAXIMUM_LATITUDE"], 12.82848, .00001);
  ASSERT_NEAR(orthoProj1["MINIMUM_LATITUDE"], 12.82806, .00001);
  ASSERT_DOUBLE_EQ(orthoProj1["LINE_PROJECTION_OFFSET"], 1504258.5);
  ASSERT_DOUBLE_EQ(orthoProj1["SAMPLE_PROJECTION_OFFSET"], -20600155.500001);
  ASSERT_NEAR(orthoProj1["EASTERNMOST_LONGITUDE"], 355.68076, .00001);
  ASSERT_NEAR(orthoProj1["WESTERNMOST_LONGITUDE"], 355.68041, .00001);
  ASSERT_EQ(orthoProj1["FIRST_STANDARD_PARALLEL"][0], "N/A");
  ASSERT_EQ(orthoProj1["SECOND_STANDARD_PARALLEL"][0], "N/A");


  PvlObject orthoView1 = orthoLabel1.findObject("VIEWING_PARAMETERS");
  ASSERT_DOUBLE_EQ(orthoView1["NORTH_AZIMUTH"], 270.0);


  Pvl orthoLabel2(prefix.path().toStdString() +"/ESP_042252_1930_IRB_D_31_ORTHO.IMG");

  ASSERT_EQ(orthoLabel2["RECORD_TYPE"][0], "FIXED_LENGTH");
  ASSERT_DOUBLE_EQ(orthoLabel2["RECORD_BYTES"], 40);
  ASSERT_DOUBLE_EQ(orthoLabel2["FILE_RECORDS"], 252);
  ASSERT_DOUBLE_EQ(orthoLabel2["^IMAGE"], 103);

  ASSERT_EQ(orthoLabel2["DATA_SET_ID"][0], "MRO-M-HIRISE-5-DTM-V1.0");
  ASSERT_EQ(orthoLabel2["PRODUCER_INSTITUTION_NAME"][0], "UNIVERSITY OF ARIZONA");
  ASSERT_EQ(orthoLabel2["PRODUCER_ID"][0], "UA");
  ASSERT_EQ(orthoLabel2["PRODUCER_FULL_NAME"][0], "ALFRED MCEWEN");
  ASSERT_EQ(orthoLabel2["PRODUCT_ID"][0], "ESP_042252_1930_IRB_D_31_ORTHO");
  ASSERT_DOUBLE_EQ(orthoLabel2["PRODUCT_VERSION_ID"], 0.314);
  ASSERT_EQ(orthoLabel2["INSTRUMENT_HOST_NAME"][0], "MARS RECONNAISSANCE ORBITER");
  ASSERT_EQ(orthoLabel2["INSTRUMENT_HOST_ID"][0], "MRO");
  ASSERT_EQ(orthoLabel2["INSTRUMENT_NAME"][0], "HIGH RESOLUTION IMAGING SCIENCE EXPERIMENT");
  ASSERT_EQ(orthoLabel2["INSTRUMENT_ID"][0], "HIRISE");
  ASSERT_EQ(orthoLabel2["TARGET_NAME"][0], "MARS");
  ASSERT_EQ(orthoLabel2["SOURCE_PRODUCT_ID"][0], "DTEEZ_042252_1930_042753_1930_A31");
  ASSERT_EQ(orthoLabel2["SOURCE_PRODUCT_ID"][1], "ESP_042252_1930");
  ASSERT_EQ(orthoLabel2["RATIONALE_DESC"][0], "NULL");
  ASSERT_EQ(orthoLabel2["SOFTWARE_NAME"][0], "Socet_Set 5.4.1");
  ASSERT_EQ(orthoLabel2["RATIONALE_DESC"][0], "NULL");
  ASSERT_DOUBLE_EQ(orthoLabel2["LABEL_RECORDS"], 102);

  PvlObject orthoImage2 = orthoLabel2.findObject("IMAGE");
  ASSERT_DOUBLE_EQ(orthoImage2["LINES"], 50);
  ASSERT_DOUBLE_EQ(orthoImage2["LINE_SAMPLES"], 40);
  ASSERT_DOUBLE_EQ(orthoImage2["BANDS"], 3);
  ASSERT_DOUBLE_EQ(orthoImage2["OFFSET"], 0.0);
  ASSERT_DOUBLE_EQ(orthoImage2["SCALING_FACTOR"], 1.0);
  ASSERT_DOUBLE_EQ(orthoImage2["SAMPLE_BITS"], 8);
  ASSERT_EQ(orthoImage2["SAMPLE_TYPE"][0], "MSB_UNSIGNED_INTEGER");
  ASSERT_EQ(orthoImage2["BAND_STORAGE_TYPE"][0], "BAND_SEQUENTIAL");
  ASSERT_DOUBLE_EQ(orthoImage2["CORE_NULL"], 0);
  ASSERT_DOUBLE_EQ(orthoImage2["CORE_LOW_REPR_SATURATION"], 1);
  ASSERT_DOUBLE_EQ(orthoImage2["CORE_LOW_INSTR_SATURATION"], 1);
  ASSERT_DOUBLE_EQ(orthoImage2["CORE_HIGH_REPR_SATURATION"], 255);
  ASSERT_DOUBLE_EQ(orthoImage2["CORE_HIGH_INSTR_SATURATION"], 255);

  PvlObject orthoProj2 = orthoLabel2.findObject("IMAGE_MAP_PROJECTION");
  ASSERT_EQ(orthoProj2["^DATA_SET_MAP_PROJECTION"][0], "DSMAP.CAT");
  ASSERT_EQ(orthoProj2["MAP_PROJECTION_TYPE"][0], "EQUIRECTANGULAR");
  ASSERT_EQ(orthoProj2["PROJECTION_LATITUDE_TYPE"][0], "PLANETOCENTRIC");
  ASSERT_DOUBLE_EQ(orthoProj2["A_AXIS_RADIUS"], 3396.19);
  ASSERT_DOUBLE_EQ(orthoProj2["B_AXIS_RADIUS"], 3396.19);
  ASSERT_DOUBLE_EQ(orthoProj2["C_AXIS_RADIUS"], 3396.19);
  ASSERT_EQ(orthoProj2["COORDINATE_SYSTEM_NAME"][0], "PLANETOCENTRIC");
  ASSERT_EQ(orthoProj2["POSITIVE_LONGITUDE_DIRECTION"][0], "EAST");
  ASSERT_EQ(orthoProj2["KEYWORD_LATITUDE_TYPE"][0], "PLANETOCENTRIC");
  ASSERT_DOUBLE_EQ(orthoProj2["CENTER_LATITUDE"], 0.0);
  ASSERT_DOUBLE_EQ(orthoProj2["CENTER_LONGITUDE"], 180.0);
  ASSERT_DOUBLE_EQ(orthoProj2["LINE_FIRST_PIXEL"], 1);
  ASSERT_DOUBLE_EQ(orthoProj2["LINE_LAST_PIXEL"], 50);
  ASSERT_DOUBLE_EQ(orthoProj2["SAMPLE_FIRST_PIXEL"], 1);
  ASSERT_DOUBLE_EQ(orthoProj2["SAMPLE_LAST_PIXEL"], 40);
  ASSERT_DOUBLE_EQ(orthoProj2["MAP_PROJECTION_ROTATION"], 0.0);
  ASSERT_NEAR(orthoProj2["MAP_RESOLUTION"], 29314.81359, .00001);
  ASSERT_NEAR(orthoProj2["MAP_SCALE"], 2.02200, .00001);
  ASSERT_NEAR(orthoProj2["MAXIMUM_LATITUDE"], 12.82801, .00001);
  ASSERT_NEAR(orthoProj2["MINIMUM_LATITUDE"], 12.82631, .00001);
  ASSERT_DOUBLE_EQ(orthoProj2["LINE_PROJECTION_OFFSET"], 376050.5);
  ASSERT_DOUBLE_EQ(orthoProj2["SAMPLE_PROJECTION_OFFSET"], -5150060.5);
  ASSERT_NEAR(orthoProj2["EASTERNMOST_LONGITUDE"], 355.68250, .00001);
  ASSERT_NEAR(orthoProj2["WESTERNMOST_LONGITUDE"], 355.68114, .00001);
  ASSERT_EQ(orthoProj2["FIRST_STANDARD_PARALLEL"][0], "N/A");
  ASSERT_EQ(orthoProj2["SECOND_STANDARD_PARALLEL"][0], "N/A");

  PvlObject orthoView2 = orthoLabel2.findObject("VIEWING_PARAMETERS");
  ASSERT_DOUBLE_EQ(orthoView2["NORTH_AZIMUTH"], 270.0);

}


TEST(Hidtmgen, HidtmgenTestDtmOnly){
   QTemporaryDir prefix;
   QVector<QString> args = {"defaultnames=TRUE",
                            "dtm=data/hidtmgen/dtm/Ares4_Marth_Crater_3557E_126N_ngate_03_lowres.cub",
                            "outputdir=" + prefix.path(),
                            "paramspvl=data/hidtmgen/dtmOnly/params.pvl",
                           };

  UserInterface options(APP_XML, args);
  try {
    hidtmgen(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to HIRISE generate PDS products: " <<  e.toString().c_str() << std::endl;
  }

  Pvl dtmLabel(prefix.path().toStdString() +"/DTEEZ_042252_1930_042753_1930_A15.IMG");

  ASSERT_EQ(dtmLabel["RECORD_TYPE"][0], "FIXED_LENGTH");
  ASSERT_DOUBLE_EQ(dtmLabel["RECORD_BYTES"], 32);
  ASSERT_DOUBLE_EQ(dtmLabel["FILE_RECORDS"], 155);
  ASSERT_DOUBLE_EQ(dtmLabel["^IMAGE"], 133);
}


TEST(Hidtmgen, HidtmgenTestEqui){
   QTemporaryDir prefix;
   QVector<QString> args = {"defaultnames=TRUE",
                            "dtm=data/hidtmgen/dtm/DTM_Zumba_1m_forPDS_lowres.cub",
                            "outputdir=" + prefix.path(),
                            "orthofromlist=data/hidtmgen/equi/orthoInputList.txt",
                            "paramspvl=data/hidtmgen/equi/params.pvl",
                            "orthosequencenumberlist=data/hidtmgen/equi/sequenceNumbers.txt"
                           };

  UserInterface options(APP_XML, args);
  try {
    hidtmgen(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to HIRISE generate PDS products: " <<  e.toString().c_str() << std::endl;
  }

  Pvl dtmLabel(prefix.path().toStdString() +"/DTEEZ_002118_1510_003608_1510_A02.IMG");
  ASSERT_EQ(dtmLabel["RECORD_TYPE"][0], "FIXED_LENGTH");
  ASSERT_DOUBLE_EQ(dtmLabel["RECORD_BYTES"], 28);
  ASSERT_DOUBLE_EQ(dtmLabel["FILE_RECORDS"], 203);
  ASSERT_DOUBLE_EQ(dtmLabel["^IMAGE"], 183);

  PvlObject dtmProj = dtmLabel.findObject("IMAGE_MAP_PROJECTION");
  ASSERT_EQ(dtmProj["MAP_PROJECTION_TYPE"][0], "EQUIRECTANGULAR");


  Pvl orthoLabel(prefix.path().toStdString() +"/PSP_002118_1510_RED_C_01_ORTHO.IMG");
  ASSERT_EQ(orthoLabel["RECORD_TYPE"][0], "FIXED_LENGTH");
  ASSERT_DOUBLE_EQ(orthoLabel["RECORD_BYTES"], 50);
  ASSERT_DOUBLE_EQ(orthoLabel["FILE_RECORDS"], 132);
  ASSERT_DOUBLE_EQ(orthoLabel["^IMAGE"], 83);

  PvlObject orthoProj = orthoLabel.findObject("IMAGE_MAP_PROJECTION");
  ASSERT_EQ(orthoProj["MAP_PROJECTION_TYPE"][0], "EQUIRECTANGULAR");
}


TEST(Hidtmgen, HidtmgenTestErrorEmptyOrthoFromList){
   QTemporaryDir prefix;
   QVector<QString> args = {"defaultnames=TRUE",
                            "dtm=data/hidtmgen/dtm/Ares4_Marth_Crater_3557E_126N_ngate_03_lowres.cub",
                            "outputdir=" + prefix.path(),
                            "orthofromlist=data/hidtmgen/error/orthoInputListEmpty.txt",
                            "paramspvl=data/hidtmgen/error/params.pvl",
                            "orthosequencenumberlist=data/hidtmgen/error/sequenceNumbers2item.txt"
                           };

  UserInterface options(APP_XML, args);
  try {
    hidtmgen(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("File [data/hidtmgen/error/orthoInputListEmpty.txt] contains no data"));
  }
}


TEST(Hidtmgen, HidtmgenTestErrorInvalidDtm){
   QTemporaryDir prefix;
   QVector<QString> args = {"defaultnames=TRUE",
                            "dtm=data/hidtmgen/ortho/ESP_042252_1930_3-BAND_COLOR_2m_o_cropped.cub",
                            "outputdir=" + prefix.path(),
                            "orthofromlist=data/hidtmgen/error/orthoInputList2Item.txt",
                            "paramspvl=data/hidtmgen/error/params.pvl",
                            "orthosequencenumberlist=data/hidtmgen/error/sequenceNumbers2item.txt"
                           };

  UserInterface options(APP_XML, args);
  try {
    hidtmgen(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("Input cube [data/hidtmgen/ortho/ESP_042252_1930_3-BAND_COLOR_2m_o_cropped.cub] does not appear to be a DTM"));
  }
}


TEST(Hidtmgen, HidtmgenTestErrorNoInput){
   QTemporaryDir prefix;
   QVector<QString> args = {"defaultnames=TRUE",
                            "outputdir=" + prefix.path(),
                            "paramspvl=data/hidtmgen/error/params.pvl",
                            "orthosequencenumberlist=data/hidtmgen/error/sequenceNumbers2item.txt"
                           };

  UserInterface options(APP_XML, args);
  try {
    hidtmgen(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("User must supply DTM or ORTHOFROMLIST or both"));
  }
}

TEST(Hidtmgen, HidtmgenTestErrorDtmInvalidProjection){
   QTemporaryDir prefix;
   QVector<QString> args = {"defaultnames=TRUE",
                            "dtm=data/hidtmgen/dtm/Ares4_Marth_Crater_3557E_126N_ngate_03_lowres_sinusoidal.cub",
                            "outputdir=" + prefix.path(),
                            "orthofromlist=data/hidtmgen/error/orthoInputList2Item.txt",
                            "paramspvl=data/hidtmgen/error/params.pvl",
                            "orthosequencenumberlist=data/hidtmgen/error/sequenceNumbers2item.txt"
                           };

  UserInterface options(APP_XML, args);
  try {
    hidtmgen(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("The projection type [SINUSOIDAL] is not supported"));
  }
}

TEST(Hidtmgen, HidtmgenTestErrorInputSeqMismatch){
   QTemporaryDir prefix;
   QVector<QString> args = {"defaultnames=TRUE",
                            "dtm=data/hidtmgen/dtm/Ares4_Marth_Crater_3557E_126N_ngate_03_lowres.cub",
                            "outputdir=" + prefix.path(),
                            "orthofromlist=data/hidtmgen/error/orthoInputList2Item.txt",
                            "paramspvl=data/hidtmgen/error/params.pvl",
                            "orthosequencenumberlist=data/hidtmgen/error/sequenceNumbers1item.txt"
                           };

  UserInterface options(APP_XML, args);
  try {
    hidtmgen(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("Output sequence number list must correspond to the input ortho list"));
  }
}


TEST(Hidtmgen, HidtmgenTestErrorInputOutputMismatch){
   QTemporaryDir prefix;
   QVector<QString> args = {"defaultnames=FALSE",
                            "dtm=data/hidtmgen/dtm/Ares4_Marth_Crater_3557E_126N_ngate_03_lowres.cub",
                            "dtm_product_id=xyz",
                            "dtmto=" + prefix.path() + "/xyz.IMG",
                            "outputdir=" + prefix.path(),
                            "orthofromlist=data/hidtmgen/error/orthoInputList2Item.txt",
                            "paramspvl=data/hidtmgen/error/params.pvl",
                            "orthotolist=data/hidtmgen/error/orthoToList1Item.txt",
                            "orthosequencenumberlist=data/hidtmgen/error/sequenceNumbers2item.txt",
                            "orthoproductidlist=data/hidtmgen/error/sequenceNumbers2item.txt"
                           };

  UserInterface options(APP_XML, args);
  try {
    hidtmgen(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("Output ortho list and product id list must correspond to the input ortho list"));
  }
}


TEST(Hidtmgen, HidtmgenTestErrorInvalidInstitution){
   QTemporaryDir prefix;
   QVector<QString> args = {"defaultnames=TRUE",
                            "dtm=data/hidtmgen/dtm/Ares4_Marth_Crater_3557E_126N_ngate_03_lowres.cub",
                            "outputdir=" + prefix.path(),
                            "orthofromlist=data/hidtmgen/error/orthoInputList2Item.txt",
                            "paramspvl=data/hidtmgen/error/invalidProducingInst.pvl",
                            "orthosequencenumberlist=data/hidtmgen/error/sequenceNumbers2item.txt"
                           };

  UserInterface options(APP_XML, args);
  try {
    hidtmgen(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("PRODUCING_INSTITUTION value [USGS] in the PARAMSPVL file must be a single character"));
  }
}

TEST(Hidtmgen, HidtmgenTestErrorInvalidVersionId){
   QTemporaryDir prefix;
   QVector<QString> args = {"defaultnames=TRUE",
                            "dtm=data/hidtmgen/dtm/Ares4_Marth_Crater_3557E_126N_ngate_03_lowres.cub",
                            "outputdir=" + prefix.path(),
                            "orthofromlist=data/hidtmgen/error/orthoInputList2Item.txt",
                            "paramspvl=data/hidtmgen/error/invalidVersionId.pvl",
                            "orthosequencenumberlist=data/hidtmgen/error/sequenceNumbers2item.txt"
                           };

  UserInterface options(APP_XML, args);
  try {
    hidtmgen(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("Version number [-4.0] is invalid"));
  }
}


TEST(Hidtmgen, HidtmgenTestErrorDtmInvalidBandSize){
   QTemporaryDir prefix;
   QVector<QString> args = {"defaultnames=TRUE",
                            "dtm=data/hidtmgen/dtm/DTM_2Bands_cropped.cub",
                            "outputdir=" + prefix.path(),
                            "orthofromlist=data/hidtmgen/error/orthoInputList2item.txt",
                            "paramspvl=data/hidtmgen/error/params.pvl",
                            "orthosequencenumberlist=data/hidtmgen/error/sequenceNumbers2item.txt"
                           };

  UserInterface options(APP_XML, args);
  try {
    hidtmgen(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("Input cube [data/hidtmgen/dtm/DTM_2Bands_cropped.cub] does not appear to be a DTM"));
  }
}


TEST(Hidtmgen, HidtmgenTestErrorOrthoInvalidBandSize){
   QTemporaryDir prefix;
   QVector<QString> args = {"defaultnames=TRUE",
                            "dtm=data/hidtmgen/dtm/Ares4_Marth_Crater_3557E_126N_ngate_03_lowres.cub",
                            "outputdir=" + prefix.path(),
                            "orthofromlist=data/hidtmgen/error/orthoInputList2Bands.txt",
                            "paramspvl=data/hidtmgen/error/params.pvl",
                            "orthosequencenumberlist=data/hidtmgen/error/sequenceNumbers1item.txt"
                           };

  UserInterface options(APP_XML, args);
  try {
    hidtmgen(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("The file [data/hidtmgen/ortho/2BandImage.cub] found in the ORTHOFROMLIST is not a valid orthorectified image. Band count must be 1 (RED) or 3 (color)"));
  }
}

TEST(Hidtmgen, HidtmgenTestNonDefaultNames){
   QTemporaryDir prefix;
   QVector<QString> args = {"defaultnames=FALSE",
                            "dtm=data/hidtmgen/dtm/DTM_Zumba_1m_forPDS_lowres.cub",
                            "outputdir=" + prefix.path(),
                            "dtmto="+ prefix.path() + "/dtm.img",
                            "orthofromlist=data/hidtmgen/nonDefaultNames/orthoInputList.txt",
                            "orthotolist=data/hidtmgen/nonDefaultNames/orthoOutputFiles.lis",
                            "orthoproductidlist=data/hidtmgen/nonDefaultNames/orthoOutputProductIds.lis",
                            "paramspvl=data/hidtmgen/nonDefaultNames/params.pvl",
                            "dtm_product_id=DtmProduct"
                           };

  UserInterface options(APP_XML, args);
  try {
    hidtmgen(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to HIRISE generate PDS products: " <<  e.toString().c_str() << std::endl;
  }

  Pvl dtmLabel(prefix.path().toStdString() +"/dtm.img");

  ASSERT_EQ(dtmLabel["RECORD_TYPE"][0], "FIXED_LENGTH");
  ASSERT_DOUBLE_EQ(dtmLabel["RECORD_BYTES"], 28);
  ASSERT_DOUBLE_EQ(dtmLabel["FILE_RECORDS"], 203);
  ASSERT_DOUBLE_EQ(dtmLabel["^IMAGE"], 183);

}


TEST(Hidtmgen, HidtmgenTestOrthoOnly){
   QTemporaryDir prefix;
   QVector<QString> args = {"defaultnames=TRUE",
                            "outputdir=" + prefix.path(),
                            "orthofromlist=data/hidtmgen/orthoOnly/orthoInputList.txt",
                            "paramspvl=data/hidtmgen/orthoOnly/params.pvl",
                            "orthosequencenumberlist=data/hidtmgen/orthoOnly/sequenceNumbers.txt"
                           };

  UserInterface options(APP_XML, args);
  try {
    hidtmgen(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to HIRISE generate PDS products: " <<  e.toString().c_str() << std::endl;
  }

  Pvl orthoLabel(prefix.path().toStdString() +"/ESP_042252_1930_IRB_D_31_ORTHO.IMG");
  ASSERT_EQ(orthoLabel["SOURCE_PRODUCT_ID"][0], "DTems_xxxxxx_xxxx_yyyyyy_yyyy_vnn");
  ASSERT_EQ(orthoLabel["SOURCE_PRODUCT_ID"][1], "ESP_042252_1930");
  ASSERT_EQ(orthoLabel["RECORD_TYPE"][0], "FIXED_LENGTH");
  ASSERT_DOUBLE_EQ(orthoLabel["RECORD_BYTES"], 40);
  ASSERT_DOUBLE_EQ(orthoLabel["FILE_RECORDS"], 254);
  ASSERT_DOUBLE_EQ(orthoLabel["^IMAGE"], 105);

}


TEST(Hidtmgen, HidtmgenTestOutputTypesAll832){
   QTemporaryDir prefix;
   QVector<QString> args = {"defaultnames=TRUE",
                            "dtm=data/hidtmgen/dtm/Ares4_Marth_Crater_3557E_126N_ngate_03_lowres.cub",
                            "outputdir=" + prefix.path(),
                            "orthofromlist=data/hidtmgen/outputTypes/orthoInputList.txt",
                            "paramspvl=data/hidtmgen/outputTypes/params.pvl",
                            "endian=msb",
                            "null=FALSE",
                            "LIS=TRUE",
                            "LRS=TRUE",
                            "HIS=TRUE",
                            "HRS=TRUE",
                            "dtmbittype=8BIT",
                            "orthobittype=32bit",
                            "orthosequencenumberlist=data/hidtmgen/outputTypes/sequenceNumbers.txt"
                           };

  UserInterface options(APP_XML, args);
  try {
    hidtmgen(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to HIRISE generate PDS products: " <<  e.toString().c_str() << std::endl;
  }

  Pvl dtmLabel(prefix.path().toStdString() +"/DTEEZ_042252_1930_042753_1930_A31.IMG");
  ASSERT_EQ(dtmLabel["RECORD_TYPE"][0], "FIXED_LENGTH");
  ASSERT_DOUBLE_EQ(dtmLabel["RECORD_BYTES"], 8);
  ASSERT_DOUBLE_EQ(dtmLabel["FILE_RECORDS"], 558);
  ASSERT_DOUBLE_EQ(dtmLabel["^IMAGE"], 536);


  PvlObject dtmImage = dtmLabel.findObject("IMAGE");
  ASSERT_DOUBLE_EQ(dtmImage["SAMPLE_BITS"], 8);
  ASSERT_EQ(dtmImage["SAMPLE_TYPE"][0], "MSB_UNSIGNED_INTEGER");

  Pvl orthoLabel(prefix.path().toStdString() +"/ESP_042252_1930_IRB_D_31_ORTHO.IMG");
  ASSERT_EQ(orthoLabel["RECORD_TYPE"][0], "FIXED_LENGTH");
  ASSERT_DOUBLE_EQ(orthoLabel["RECORD_BYTES"], 160);
  ASSERT_DOUBLE_EQ(orthoLabel["FILE_RECORDS"], 177);
  ASSERT_DOUBLE_EQ(orthoLabel["^IMAGE"], 28);

  PvlObject orthoImage = orthoLabel.findObject("IMAGE");
  ASSERT_DOUBLE_EQ(orthoImage["SAMPLE_BITS"], 32);
  ASSERT_EQ(orthoImage["SAMPLE_TYPE"][0], "IEEE_REAL");

}


TEST(Hidtmgen, HidtmgenTestOutputTypesAllU16S16){
   QTemporaryDir prefix;
   QVector<QString> args = {"defaultnames=TRUE",
                            "dtm=data/hidtmgen/dtm/Ares4_Marth_Crater_3557E_126N_ngate_03_lowres.cub",
                            "outputdir=" + prefix.path(),
                            "orthofromlist=data/hidtmgen/outputTypes/orthoInputList.txt",
                            "paramspvl=data/hidtmgen/outputTypes/params2.pvl",
                            "endian=msb",
                            "null=FALSE",
                            "LIS=TRUE",
                            "LRS=TRUE",
                            "HIS=TRUE",
                            "HRS=TRUE",
                            "dtmbittype=u16bit",
                            "orthobittype=s16bit",
                            "orthosequencenumberlist=data/hidtmgen/outputTypes/sequenceNumbers.txt"
                           };

  UserInterface options(APP_XML, args);
  try {
    hidtmgen(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to HIRISE generate PDS products: " <<  e.toString().c_str() << std::endl;
  }

  Pvl dtmLabel(prefix.path().toStdString() +"/DTEEZ_042252_1930_042753_1930_A07.IMG");
  ASSERT_EQ(dtmLabel["RECORD_TYPE"][0], "FIXED_LENGTH");
  ASSERT_DOUBLE_EQ(dtmLabel["RECORD_BYTES"], 16);
  ASSERT_DOUBLE_EQ(dtmLabel["FILE_RECORDS"], 288);
  ASSERT_DOUBLE_EQ(dtmLabel["^IMAGE"], 266);


  PvlObject dtmImage = dtmLabel.findObject("IMAGE");
  ASSERT_DOUBLE_EQ(dtmImage["SAMPLE_BITS"], 16);
  ASSERT_EQ(dtmImage["SAMPLE_TYPE"][0], "MSB_UNSIGNED_INTEGER");

  Pvl orthoLabel(prefix.path().toStdString() +"/ESP_042252_1930_IRB_D_31_ORTHO.IMG");
  ASSERT_EQ(orthoLabel["RECORD_TYPE"][0], "FIXED_LENGTH");
  ASSERT_DOUBLE_EQ(orthoLabel["RECORD_BYTES"], 80);
  ASSERT_DOUBLE_EQ(orthoLabel["FILE_RECORDS"], 202);
  ASSERT_DOUBLE_EQ(orthoLabel["^IMAGE"], 53);

  PvlObject orthoImage = orthoLabel.findObject("IMAGE");
  ASSERT_DOUBLE_EQ(orthoImage["SAMPLE_BITS"], 16);
  ASSERT_EQ(orthoImage["SAMPLE_TYPE"][0], "MSB_INTEGER");
}


TEST(Hidtmgen, HidtmgenTestOutputTypesNoneS16U16){
   QTemporaryDir prefix;
   QVector<QString> args = {"defaultnames=TRUE",
                            "dtm=data/hidtmgen/dtm/Ares4_Marth_Crater_3557E_126N_ngate_03_lowres.cub",
                            "outputdir=" + prefix.path(),
                            "orthofromlist=data/hidtmgen/outputTypes/orthoInputList.txt",
                            "paramspvl=data/hidtmgen/outputTypes/params2.pvl",
                            "endian=msb",
                            "null=FALSE",
                            "LIS=FALSE",
                            "LRS=FALSE",
                            "HIS=FALSE",
                            "HRS=FALSE",
                            "dtmbittype=S16BIT",
                            "orthobittype=U16BIT",
                            "orthosequencenumberlist=data/hidtmgen/outputTypes/sequenceNumbers.txt"
                           };

  UserInterface options(APP_XML, args);
  try {
    hidtmgen(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to HIRISE generate PDS products: " <<  e.toString().c_str() << std::endl;
  }

  Pvl dtmLabel(prefix.path().toStdString() +"/DTEEZ_042252_1930_042753_1930_A07.IMG");
  ASSERT_EQ(dtmLabel["RECORD_TYPE"][0], "FIXED_LENGTH");
  ASSERT_DOUBLE_EQ(dtmLabel["RECORD_BYTES"], 16);
  ASSERT_DOUBLE_EQ(dtmLabel["FILE_RECORDS"], 288);
  ASSERT_DOUBLE_EQ(dtmLabel["^IMAGE"], 266);


  PvlObject dtmImage = dtmLabel.findObject("IMAGE");
  ASSERT_DOUBLE_EQ(dtmImage["SAMPLE_BITS"], 16);
  ASSERT_EQ(dtmImage["SAMPLE_TYPE"][0], "MSB_INTEGER");

  Pvl orthoLabel(prefix.path().toStdString() +"/ESP_042252_1930_IRB_D_31_ORTHO.IMG");
  ASSERT_EQ(orthoLabel["RECORD_TYPE"][0], "FIXED_LENGTH");
  ASSERT_DOUBLE_EQ(orthoLabel["RECORD_BYTES"], 80);
  ASSERT_DOUBLE_EQ(orthoLabel["FILE_RECORDS"], 202);
  ASSERT_DOUBLE_EQ(orthoLabel["^IMAGE"], 53);

  PvlObject orthoImage = orthoLabel.findObject("IMAGE");
  ASSERT_DOUBLE_EQ(orthoImage["SAMPLE_BITS"], 16);
  ASSERT_EQ(orthoImage["SAMPLE_TYPE"][0], "MSB_UNSIGNED_INTEGER");
}


TEST(Hidtmgen, HidtmgenTestPolar){
   QTemporaryDir prefix;
   QVector<QString> args = {"defaultnames=TRUE",
                            "dtm=data/hidtmgen/dtm/Polar_Crater_1_1m_ngate_edited2_forPDS_lowres.cub",
                            "paramspvl=data/hidtmgen/polar/params.pvl",
                            "outputdir=" + prefix.path(),
                            "orthofromlist=data/hidtmgen/polar/orthoInputList.txt",
                            "orthosequence=data/hidtmgen/polar/orthosequencenumberlist.txt"
                           };

  UserInterface options(APP_XML, args);
  try {
    hidtmgen(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to HIRISE generate PDS products: " <<  e.toString().c_str() << std::endl;
  }

  Pvl dtmLabel(prefix.path().toStdString() +"/DTEPZ_009404_2635_010221_2635_Z12.IMG");
  ASSERT_EQ(dtmLabel["RECORD_TYPE"][0], "FIXED_LENGTH");
  ASSERT_DOUBLE_EQ(dtmLabel["RECORD_BYTES"], 52);
  ASSERT_DOUBLE_EQ(dtmLabel["FILE_RECORDS"], 96);
  ASSERT_DOUBLE_EQ(dtmLabel["^IMAGE"], 85);

  PvlObject dtmProj = dtmLabel.findObject("IMAGE_MAP_PROJECTION");
  ASSERT_EQ(dtmProj["MAP_PROJECTION_TYPE"][0], "POLAR STEREOGRAPHIC");

  Pvl orthoLabel(prefix.path().toStdString() +"/PSP_009404_2635_RED_C_1_ORTHO.IMG");
  ASSERT_EQ(orthoLabel["RECORD_TYPE"][0], "FIXED_LENGTH");
  ASSERT_DOUBLE_EQ(orthoLabel["RECORD_BYTES"], 50);
  ASSERT_DOUBLE_EQ(orthoLabel["FILE_RECORDS"], 115);
  ASSERT_DOUBLE_EQ(orthoLabel["^IMAGE"], 66);

  PvlObject orthoProj = orthoLabel.findObject("IMAGE_MAP_PROJECTION");
  ASSERT_EQ(orthoProj["MAP_PROJECTION_TYPE"][0], "POLAR STEREOGRAPHIC");
}
