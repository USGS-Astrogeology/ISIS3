#include <QTextStream>
#include <QStringList>
#include <QFile>
#include <QDebug>

#include "lronac2pds.h"
#include "NetworkFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "SpecialPixel.h"

#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/lronac2pds.xml").expanded());

TEST_F(ObservationPair, FunctionalTestLronac2pdsIof) {
  QVector<QString> args = {"from=" + cubeLPath,
                           "to=" + tempDir.path() + "/LroNacL.img"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  lronac2pds(options);

  Pvl outputLabel(options.GetFileName("TO").toStdString());
  PvlObject imageObject = outputLabel.findObject("IMAGE");

  EXPECT_EQ((std::string)outputLabel["FILE_RECORDS"], "104449");
  EXPECT_EQ((std::string)outputLabel["DATA_SET_ID"], "LRO-L-LROC-3-CDR-V1.0");
  EXPECT_EQ((std::string)outputLabel["PRODUCT_ID"], "M102128467LC");
  EXPECT_EQ((std::string)outputLabel["PRODUCT_TYPE"], "CDR");
  EXPECT_EQ((std::string)outputLabel["PRODUCT_VERSION_ID"], "N/A");

  EXPECT_EQ(int(imageObject["SAMPLE_BITS"]), 16);
  EXPECT_EQ((std::string)imageObject["SAMPLE_TYPE"], "LSB_INTEGER");
  EXPECT_DOUBLE_EQ(double(imageObject["SCALING_FACTOR"]), 3.0518509475997199e-05);
  EXPECT_EQ((std::string)imageObject["VALID_MINIMUM"], "-32752");
  EXPECT_EQ((std::string)imageObject["NULL"], "-32768");
  EXPECT_EQ((std::string)imageObject["LOW_REPR_SATURATION"], "-32767");
  EXPECT_EQ((std::string)imageObject["LOW_INSTR_SATURATION"], "-32766");
  EXPECT_EQ((std::string)imageObject["HIGH_INSTR_SATURATION"], "-32765");
  EXPECT_EQ((std::string)imageObject["HIGH_REPR_SATURATION"], "-32764");
  EXPECT_EQ((std::string)imageObject["UNIT"], "Scaled I/F");
  EXPECT_EQ((std::string)imageObject["MD5_CHECKSUM"], "5f5d7bc236f794ca651cebdde529f8a4");
}

TEST_F(ObservationPair, FunctionalTestLronac2pdsRadiance) {
  QVector<QString> args = {"from=" + cubeLPath,
                           "to=" + tempDir.path() + "/LroNacL.img"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  cubeL->label()->findObject("IsisCube").findGroup("Radiometry")["RadiometricType"] = "AbsoluteRadiance";
  cubeL->reopen("rw");

  lronac2pds(options);

  Pvl outputLabel(options.GetFileName("TO").toStdString());
  PvlObject imageObject = outputLabel.findObject("IMAGE");

  EXPECT_EQ((std::string)outputLabel["FILE_RECORDS"], "208897");
  EXPECT_EQ((std::string)outputLabel["DATA_SET_ID"], "LRO-L-LROC-3-CDR-V1.0");
  EXPECT_EQ((std::string)outputLabel["PRODUCT_ID"], "M102128467LC");
  EXPECT_EQ((std::string)outputLabel["PRODUCT_TYPE"], "CDR");
  EXPECT_EQ((std::string)outputLabel["PRODUCT_VERSION_ID"], "N/A");

  EXPECT_EQ(int(imageObject["SAMPLE_BITS"]), 32);
  EXPECT_EQ((std::string)imageObject["SAMPLE_TYPE"], "PC_REAL");
  EXPECT_EQ((std::string)imageObject["VALID_MINIMUM"], "16#FF7FFFFA#");
  EXPECT_EQ((std::string)imageObject["NULL"], "16#FF7FFFFB#");
  EXPECT_EQ((std::string)imageObject["LOW_REPR_SATURATION"], "16#FF7FFFFC#");
  EXPECT_EQ((std::string)imageObject["LOW_INSTR_SATURATION"], "16#FF7FFFFD#");
  EXPECT_EQ((std::string)imageObject["HIGH_INSTR_SATURATION"], "16#FF7FFFFE#");
  EXPECT_EQ((std::string)imageObject["HIGH_REPR_SATURATION"], "16#FF7FFFFF#");
  EXPECT_EQ((std::string)imageObject["UNIT"], "W / (m**2 micrometer sr)");
  EXPECT_EQ((std::string)imageObject["MD5_CHECKSUM"], "b51ea10347da242b4a5f8a25c21026f6");
}
