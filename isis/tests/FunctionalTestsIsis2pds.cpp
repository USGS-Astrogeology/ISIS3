#include "CameraFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "XmlToPvlTranslationManager.h"

#include "isis2pds.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/isis2pds.xml").expanded();

TEST_F(DefaultCube, FunctionalTestIsis2pdsDefault) {
  QTemporaryDir prefix;
  QString outputFile = prefix.path() + "/outTemp.img";
  QVector<QString> args = {"from=" + testCube->fileName(),
                          "to=" + outputFile,
                          "checksum=yes"
                          };

  UserInterface options(APP_XML, args);
  try {
    isis2pds(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Pvl outputLabel(options.GetFileName("TO").toStdString());
  PvlObject imageObject = outputLabel.findObject("IMAGE");

  EXPECT_EQ((std::string)outputLabel["PDS_VERSION_ID"], "PDS3");
  EXPECT_EQ((std::string)outputLabel["RECORD_TYPE"], "UNDEFINED");
  EXPECT_EQ(int(outputLabel["LABEL_RECORDS"]), 809);
  EXPECT_EQ(int(outputLabel["^IMAGE"]), 810);
  EXPECT_EQ((std::string)outputLabel["CHECKSUM"], "5656e616c61d8fa7ed568ff88632f044");

  EXPECT_EQ(int(imageObject["LINES"]), 1056);
  EXPECT_EQ(int(imageObject["LINE_SAMPLES"]), 1204);
  EXPECT_EQ(int(imageObject["BANDS"]), 1);
  EXPECT_EQ((std::string)imageObject["BAND_STORAGE_TYPE"], "BAND_SEQUENTIAL");
  EXPECT_DOUBLE_EQ(double(imageObject["OFFSET"]), 0.0);
  EXPECT_DOUBLE_EQ(double(imageObject["SCALING_FACTOR"]), 1.0);
  EXPECT_EQ(int(imageObject["SAMPLE_BITS"]), 32);
  EXPECT_EQ((std::string)imageObject["SAMPLE_BIT_MASK"], "2#11111111111111111111111111111111#");
  EXPECT_EQ((std::string)imageObject["SAMPLE_TYPE"], "PC_REAL");
  EXPECT_EQ((std::string)imageObject["CORE_NULL"], "16#FF7FFFFB#");
  EXPECT_EQ((std::string)imageObject["CORE_LOW_REPR_SATURATION"], "16#FF7FFFFC#");
  EXPECT_EQ((std::string)imageObject["CORE_LOW_INSTR_SATURATION"], "16#FF7FFFFD#");
  EXPECT_EQ((std::string)imageObject["CORE_HIGH_REPR_SATURATION"], "16#FF7FFFFF#");
  EXPECT_EQ((std::string)imageObject["CORE_HIGH_INSTR_SATURATION"], "16#FF7FFFFE#");

}

TEST_F(DefaultCube, FunctionalTestIsis2pdsDefaultMSB) {
  QTemporaryDir prefix;
  QString outputFile = prefix.path() + "/outTemp.img";
  QVector<QString> args = {"from=" + testCube->fileName(),
                          "to=" + outputFile,
                          "endian=msb",
                          "bittype=8bit"
                          };

  UserInterface options(APP_XML, args);
  try {
    isis2pds(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Pvl outputLabel(options.GetFileName("TO").toStdString());
  PvlObject imageObject = outputLabel.findObject("IMAGE");

  EXPECT_EQ(int(outputLabel["LABEL_RECORDS"]), 695);
  EXPECT_EQ(int(outputLabel["^IMAGE"]), 696);

  EXPECT_EQ(int(imageObject["SAMPLE_BITS"]), 8);
  EXPECT_EQ((std::string)imageObject["SAMPLE_BIT_MASK"], "2#11111111#");
  EXPECT_EQ((std::string)imageObject["SAMPLE_TYPE"], "MSB_UNSIGNED_INTEGER");
  EXPECT_EQ(int(imageObject["CORE_NULL"]), 0);
  EXPECT_EQ(int(imageObject["CORE_LOW_REPR_SATURATION"]), 1);
  EXPECT_EQ(int(imageObject["CORE_LOW_INSTR_SATURATION"]), 1);
  EXPECT_EQ(int(imageObject["CORE_HIGH_REPR_SATURATION"]), 255);
  EXPECT_EQ(int(imageObject["CORE_HIGH_INSTR_SATURATION"]), 255);

}

TEST_F(DefaultCube, FunctionalTestIsis2pdsLinear8bit) {
  QTemporaryDir prefix;
  QString outputFile = prefix.path() + "/outTemp.img";
  QVector<QString> args = {"from=" + testCube->fileName(),
                          "to=" + outputFile,
                          "stretch=linear",
                          "bittype=8bit",
                          "lrs=yes",
                          "lis=yes",
                          "his=yes",
                          "hrs=yes",
                          "minpercent=5.0",
                          "maxpercent=45.0"
                          };

  UserInterface options(APP_XML, args);
  try {
    isis2pds(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Pvl outputLabel(options.GetFileName("TO").toStdString());
  PvlObject imageObject = outputLabel.findObject("IMAGE");

  EXPECT_EQ(int(outputLabel["LABEL_RECORDS"]), 700);
  EXPECT_EQ(int(outputLabel["^IMAGE"]), 701);

  EXPECT_EQ(int(imageObject["SAMPLE_BITS"]), 8);
  EXPECT_DOUBLE_EQ(double(imageObject["OFFSET"]), 11.776);
  EXPECT_DOUBLE_EQ(double(imageObject["SCALING_FACTOR"]), 0.40799999999999997);
  EXPECT_EQ(int(imageObject["CORE_LOW_REPR_SATURATION"]), 1);
  EXPECT_EQ(int(imageObject["CORE_LOW_INSTR_SATURATION"]), 2);
  EXPECT_EQ(int(imageObject["CORE_HIGH_REPR_SATURATION"]), 255);
  EXPECT_EQ(int(imageObject["CORE_HIGH_INSTR_SATURATION"]), 254);

}

TEST_F(DefaultCube, FunctionalTestIsis2pdsLinears16bit) {
  QTemporaryDir prefix;
  QString outputFile = prefix.path() + "/outTemp.img";
  QVector<QString> args = {"from=" + testCube->fileName(),
                          "to=" + outputFile,
                          "stretch=linear",
                          "bittype=s16bit",
                          "lrs=yes",
                          "lis=yes",
                          "his=yes",
                          "hrs=yes",
                          "minpercent=45.0",
                          "maxpercent=55.0"
                          };

  UserInterface options(APP_XML, args);
  try {
    isis2pds(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Pvl outputLabel(options.GetFileName("TO").toStdString());
  PvlObject imageObject = outputLabel.findObject("IMAGE");

  EXPECT_EQ(int(outputLabel["LABEL_RECORDS"]), 745);
  EXPECT_EQ(int(outputLabel["^IMAGE"]), 746);

  EXPECT_DOUBLE_EQ(double(imageObject["OFFSET"]), 127.49923699068999);
  EXPECT_DOUBLE_EQ(double(imageObject["SCALING_FACTOR"]), 0.00038150465435678302);
  EXPECT_EQ(int(imageObject["SAMPLE_BITS"]), 16);
  EXPECT_EQ((std::string)imageObject["SAMPLE_BIT_MASK"], "2#1111111111111111#");
  EXPECT_EQ((std::string)imageObject["SAMPLE_TYPE"], "LSB_INTEGER");
  EXPECT_EQ(int(imageObject["CORE_LOW_REPR_SATURATION"]), -32767);
  EXPECT_EQ(int(imageObject["CORE_LOW_INSTR_SATURATION"]), -32766);
  EXPECT_EQ(int(imageObject["CORE_HIGH_REPR_SATURATION"]), -32764);
  EXPECT_EQ(int(imageObject["CORE_HIGH_INSTR_SATURATION"]), -32765);

}

TEST_F(DefaultCube, FunctionalTestIsis2pdsLinearu16bit) {
  QTemporaryDir prefix;
  QString outputFile = prefix.path() + "/outTemp.img";
  QVector<QString> args = {"from=" + testCube->fileName(),
                          "to=" + outputFile,
                          "stretch=linear",
                          "bittype=u16bit",
                          "lrs=yes",
                          "lis=yes",
                          "his=yes",
                          "hrs=yes",
                          "minpercent=1.0",
                          "maxpercent=99.0"
                          };

  UserInterface options(APP_XML, args);
  try {
    isis2pds(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Pvl outputLabel(options.GetFileName("TO").toStdString());
  PvlObject imageObject = outputLabel.findObject("IMAGE");

  EXPECT_EQ(int(outputLabel["LABEL_RECORDS"]), 735);
  EXPECT_EQ(int(outputLabel["^IMAGE"]), 736);

  EXPECT_DOUBLE_EQ(double(imageObject["OFFSET"]), 2.9886006409278001);
  EXPECT_DOUBLE_EQ(double(imageObject["SCALING_FACTOR"]), 0.0037997863573935998);
  EXPECT_EQ((std::string)imageObject["SAMPLE_TYPE"], "LSB_UNSIGNED_INTEGER");
  EXPECT_EQ(int(imageObject["CORE_LOW_REPR_SATURATION"]), 1);
  EXPECT_EQ(int(imageObject["CORE_LOW_INSTR_SATURATION"]), 2);
  EXPECT_EQ(int(imageObject["CORE_HIGH_REPR_SATURATION"]), 65535);
  EXPECT_EQ(int(imageObject["CORE_HIGH_INSTR_SATURATION"]), 65534);

}

TEST_F(DefaultCube, FunctionalTestIsis2pdsManual8bit) {
  QTemporaryDir prefix;
  QString outputFile = prefix.path() + "/outTemp.img";
  QVector<QString> args = {"from=" + testCube->fileName(),
                          "to=" + outputFile,
                          "stretch=manual",
                          "bittype=8bit",
                          "minimum=0",
                          "maximum=255"
                          };

  UserInterface options(APP_XML, args);
  try {
    isis2pds(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Pvl outputLabel(options.GetFileName("TO").toStdString());
  PvlObject imageObject = outputLabel.findObject("IMAGE");

  EXPECT_EQ(int(outputLabel["LABEL_RECORDS"]), 718);
  EXPECT_EQ(int(outputLabel["^IMAGE"]), 719);

  EXPECT_EQ(int(imageObject["SAMPLE_BITS"]), 8);
  EXPECT_DOUBLE_EQ(double(imageObject["OFFSET"]), -1.003937007874);
  EXPECT_DOUBLE_EQ(double(imageObject["SCALING_FACTOR"]), 1.003937007874);
  EXPECT_EQ(int(imageObject["CORE_LOW_REPR_SATURATION"]), 1);
  EXPECT_EQ(int(imageObject["CORE_LOW_INSTR_SATURATION"]), 1);
  EXPECT_EQ(int(imageObject["CORE_HIGH_REPR_SATURATION"]), 255);
  EXPECT_EQ(int(imageObject["CORE_HIGH_INSTR_SATURATION"]), 255);

}

TEST_F(DefaultCube, FunctionalTestIsis2pdsManuals16bit) {
  QTemporaryDir prefix;
  QString outputFile = prefix.path() + "/outTemp.img";
  QVector<QString> args = {"from=" + testCube->fileName(),
                          "to=" + outputFile,
                          "stretch=manual",
                          "bittype=s16bit",
                          "lrs=yes",
                          "lis=yes",
                          "his=yes",
                          "hrs=yes",
                          "minimum=-32768",
                          "maximum=32767"
                          };

  UserInterface options(APP_XML, args);
  try {
    isis2pds(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Pvl outputLabel(options.GetFileName("TO").toStdString());
  PvlObject imageObject = outputLabel.findObject("IMAGE");

  EXPECT_EQ(int(outputLabel["LABEL_RECORDS"]), 741);
  EXPECT_EQ(int(outputLabel["^IMAGE"]), 742);

  EXPECT_DOUBLE_EQ(double(imageObject["OFFSET"]), -2.5001526018641);
  EXPECT_DOUBLE_EQ(double(imageObject["SCALING_FACTOR"]), 1.0000763009309);
  EXPECT_EQ(int(imageObject["SAMPLE_BITS"]), 16);
  EXPECT_EQ((std::string)imageObject["SAMPLE_BIT_MASK"], "2#1111111111111111#");
  EXPECT_EQ((std::string)imageObject["SAMPLE_TYPE"], "LSB_INTEGER");
  EXPECT_EQ(int(imageObject["CORE_NULL"]), -32768);
  EXPECT_EQ(int(imageObject["CORE_LOW_REPR_SATURATION"]), -32767);
  EXPECT_EQ(int(imageObject["CORE_LOW_INSTR_SATURATION"]), -32766);
  EXPECT_EQ(int(imageObject["CORE_HIGH_REPR_SATURATION"]), -32764);
  EXPECT_EQ(int(imageObject["CORE_HIGH_INSTR_SATURATION"]), -32765);

}

TEST_F(DefaultCube, FunctionalTestIsis2pdsManualu16bit) {
  QTemporaryDir prefix;
  QString outputFile = prefix.path() + "/outTemp.img";
  QVector<QString> args = {"from=" + testCube->fileName(),
                          "to=" + outputFile,
                          "stretch=manual",
                          "bittype=u16bit",
                          "lrs=yes",
                          "lis=yes",
                          "his=yes",
                          "hrs=yes",
                          "minimum=0",
                          "maximum=65535"
                          };

  UserInterface options(APP_XML, args);
  try {
    isis2pds(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Pvl outputLabel(options.GetFileName("TO").toStdString());
  PvlObject imageObject = outputLabel.findObject("IMAGE");

  EXPECT_EQ(int(outputLabel["LABEL_RECORDS"]), 733);
  EXPECT_EQ(int(outputLabel["^IMAGE"]), 734);

  EXPECT_DOUBLE_EQ(double(imageObject["OFFSET"]), -3.0002289027926);
  EXPECT_DOUBLE_EQ(double(imageObject["SCALING_FACTOR"]), 1.0000763009309);
  EXPECT_EQ((std::string)imageObject["SAMPLE_TYPE"], "LSB_UNSIGNED_INTEGER");
  EXPECT_EQ(int(imageObject["CORE_NULL"]), 0);
  EXPECT_EQ(int(imageObject["CORE_LOW_REPR_SATURATION"]), 1);
  EXPECT_EQ(int(imageObject["CORE_LOW_INSTR_SATURATION"]), 2);
  EXPECT_EQ(int(imageObject["CORE_HIGH_REPR_SATURATION"]), 65535);
  EXPECT_EQ(int(imageObject["CORE_HIGH_INSTR_SATURATION"]), 65534);

}

TEST(isis2pdsTest, FunctionalTestIsis2pdsOffset) {
  QTemporaryDir prefix;
  QString outputFile = prefix.path() + "/outTemp.img";
  QVector<QString> args = {"from=data/isis2pds/ldem_4_reduced.cub",
                          "to=" + outputFile,
                          "checksum=yes"
                          };

  UserInterface options(APP_XML, args);
  try {
    isis2pds(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Pvl outputLabel(options.GetFileName("TO").toStdString());
  PvlObject imageObject = outputLabel.findObject("IMAGE");

  EXPECT_EQ((std::string)outputLabel["PDS_VERSION_ID"], "PDS3");
  EXPECT_EQ((std::string)outputLabel["RECORD_TYPE"], "UNDEFINED");
  EXPECT_EQ(int(outputLabel["LABEL_RECORDS"]), 2032);
  EXPECT_EQ(int(outputLabel["^IMAGE"]), 2033);
  EXPECT_EQ((std::string)outputLabel["CHECKSUM"], "315348326c41130fdefd0615c24a1ac9");
  EXPECT_EQ((std::string)outputLabel["TARGET_NAME"], "MOON");

  EXPECT_EQ(int(imageObject["LINES"]), 240);
  EXPECT_EQ(int(imageObject["LINE_SAMPLES"]), 480);
  EXPECT_EQ(int(imageObject["BANDS"]), 1);
  EXPECT_EQ((std::string)imageObject["BAND_STORAGE_TYPE"], "BAND_SEQUENTIAL");
  EXPECT_DOUBLE_EQ(double(imageObject["OFFSET"]), 0.0);
  EXPECT_DOUBLE_EQ(double(imageObject["SCALING_FACTOR"]), 1.0);
  EXPECT_EQ(int(imageObject["SAMPLE_BITS"]), 32);
  EXPECT_EQ((std::string)imageObject["SAMPLE_BIT_MASK"], "2#11111111111111111111111111111111#");
  EXPECT_EQ((std::string)imageObject["SAMPLE_TYPE"], "PC_REAL");
  EXPECT_EQ((std::string)imageObject["CORE_NULL"], "16#FF7FFFFB#");
  EXPECT_EQ((std::string)imageObject["CORE_LOW_REPR_SATURATION"], "16#FF7FFFFC#");
  EXPECT_EQ((std::string)imageObject["CORE_LOW_INSTR_SATURATION"], "16#FF7FFFFD#");
  EXPECT_EQ((std::string)imageObject["CORE_HIGH_REPR_SATURATION"], "16#FF7FFFFF#");
  EXPECT_EQ((std::string)imageObject["CORE_HIGH_INSTR_SATURATION"], "16#FF7FFFFE#");

  PvlObject projectionObject = outputLabel.findObject("IMAGE_MAP_PROJECTION");

  EXPECT_EQ((std::string)projectionObject["MAP_PROJECTION_TYPE"], "SIMPLE CYLINDRICAL");
  EXPECT_EQ((std::string)projectionObject["PROJECTION_LATITUDE_TYPE"], "PLANETOCENTRIC");
  EXPECT_DOUBLE_EQ(double(projectionObject["A_AXIS_RADIUS"]), 1737.4);
  EXPECT_DOUBLE_EQ(double(projectionObject["B_AXIS_RADIUS"]), 1737.4);
  EXPECT_DOUBLE_EQ(double(projectionObject["C_AXIS_RADIUS"]), 1737.4);
  EXPECT_EQ((std::string)projectionObject["FIRST_STANDARD_PARALLEL"], "N/A");
  EXPECT_EQ((std::string)projectionObject["SECOND_STANDARD_PARALLEL"], "N/A");
  EXPECT_EQ((std::string)projectionObject["COORDINATE_SYSTEM_NAME"], "PLANETOCENTRIC");
  EXPECT_EQ((std::string)projectionObject["POSITIVE_LONGITUDE_DIRECTION"], "EAST");
  EXPECT_EQ((std::string)projectionObject["KEYWORD_LATITUDE_TYPE"], "PLANETOCENTRIC");
  EXPECT_DOUBLE_EQ(double(projectionObject["CENTER_LATITUDE"]), 0.0);
  EXPECT_DOUBLE_EQ(double(projectionObject["CENTER_LONGITUDE"]), 180.0);
  EXPECT_EQ(int(projectionObject["LINE_FIRST_PIXEL"]), 1);
  EXPECT_EQ(int(projectionObject["LINE_LAST_PIXEL"]), 240);
  EXPECT_EQ(int(projectionObject["SAMPLE_FIRST_PIXEL"]), 1);
  EXPECT_EQ(int(projectionObject["SAMPLE_LAST_PIXEL"]), 480);
  EXPECT_DOUBLE_EQ(double(projectionObject["MAP_PROJECTION_ROTATION"]), 0.0);
  EXPECT_DOUBLE_EQ(double(projectionObject["MAP_RESOLUTION"]), 1.3333333333333);
  EXPECT_DOUBLE_EQ(double(projectionObject["MAP_SCALE"]), 22.74252);
  EXPECT_DOUBLE_EQ(double(projectionObject["MAXIMUM_LATITUDE"]), 90.0);
  EXPECT_DOUBLE_EQ(double(projectionObject["MINIMUM_LATITUDE"]), -90.0);
  EXPECT_DOUBLE_EQ(double(projectionObject["EASTERNMOST_LONGITUDE"]), 360.0);
  EXPECT_DOUBLE_EQ(double(projectionObject["WESTERNMOST_LONGITUDE"]), 0.0);
  EXPECT_DOUBLE_EQ(double(projectionObject["LINE_PROJECTION_OFFSET"]), 119.5);
  EXPECT_DOUBLE_EQ(double(projectionObject["SAMPLE_PROJECTION_OFFSET"]), 239.5);

}

TEST(isis2pdsTest, FunctionalTestIsis2pdsPds4) {
  QTemporaryDir prefix;
  QString outputFile = prefix.path() + "/outTemp.img";
  QVector<QString> args = {"from=data/isis2pds/dawnEqui1.cub",
                          "to=" + outputFile,
                          "pdsversion=pds4"
                          };

  UserInterface options(APP_XML, args);
  try {
    isis2pds(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  FileName xmlTransFile(prefix.path() + "/outTemp.xml");
  XmlToPvlTranslationManager outputLabel(xmlTransFile, "data/isis2pds/isis2pds4.trn");

  EXPECT_EQ(outputLabel.Translate("Target_Name"), "Vesta");
  EXPECT_EQ(outputLabel.Translate("Target_Type"), "Asteroid");
  EXPECT_EQ(outputLabel.Translate("Filter_Number"), "1");
  EXPECT_EQ(outputLabel.Translate("Bandwidth"), "0.0140");
  EXPECT_EQ(outputLabel.Translate("Wavelength"), "1.021");
  EXPECT_EQ(outputLabel.Translate("West_Bound_Coord"), "0.0048031106755032");
  EXPECT_EQ(outputLabel.Translate("East_Bound_Coord"), "359.97220491712");
  EXPECT_EQ(outputLabel.Translate("North_Bound_Coord"), "-1.1656354932701");
  EXPECT_EQ(outputLabel.Translate("South_Bound_Coord"), "-12.077278860826");
  EXPECT_EQ(outputLabel.Translate("Pixel_Resolution"), "162.67302420122");
  EXPECT_EQ(outputLabel.Translate("Pixel_Scale"), "30.884440468611");
  EXPECT_EQ(outputLabel.Translate("Upperleft_x"), "-14640.57217811");
  EXPECT_EQ(outputLabel.Translate("Upperleft_y"), "-5856.2288712439");
  EXPECT_EQ(outputLabel.Translate("Latitude_type"), "Planetocentric");
  EXPECT_EQ(outputLabel.Translate("Semi_major_radius"), "289000.0");
  EXPECT_EQ(outputLabel.Translate("Polar_radius"), "229000.0");
  EXPECT_EQ(outputLabel.Translate("Longitude_direction"), "Positive East");

}
