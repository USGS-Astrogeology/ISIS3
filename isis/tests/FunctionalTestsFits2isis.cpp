#include <QTemporaryDir>

#include "fits2isis.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/fits2isis.xml").expanded();

TEST(Fits2Isis, Fits2IsisTestDefault) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/fits2isisTEMP.cub";
  QVector<QString> args = {"from=data/fits2isis/default.fits", "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
   fits2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest fits file: " << e.toString().toStdString().c_str() << std::endl;
  }
  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  ASSERT_EQ(cube.sampleCount(), 25);
  ASSERT_EQ(cube.lineCount(), 10);
  ASSERT_EQ(cube.bandCount(), 3);

    // Pixels group
  ASSERT_EQ(PixelTypeName(cube.pixelType()), "Real");
  ASSERT_EQ(ByteOrderName(cube.byteOrder()), "Lsb");
  ASSERT_DOUBLE_EQ(cube.base(), 0.0);
  ASSERT_DOUBLE_EQ(cube.multiplier(), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["InstrumentId"][0], "WFPC2" );
  ASSERT_EQ(inst["StartTime"][0], "1999-02-20" );

  std::unique_ptr<Histogram> hist (cube.histogram());

  ASSERT_NEAR(hist->Average(), 0.07489, .000001);
  ASSERT_NEAR(hist->Sum(), 18.7225, .0001);
  ASSERT_EQ(hist->ValidPixels(), 250);
  ASSERT_NEAR(hist->StandardDeviation(), 0.8402, .0001);
}


TEST(Fits2Isis, Fits2IsisOrganizationBsq) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/fits2isisTEMP.cub";
  QVector<QString> args = {"from=data/fits2isis/organization.fits",
                           "to=" + cubeFileName,
                           "organization=bsq",
                           "imagenumber=1"};

  UserInterface options(APP_XML, args);
  try {
   fits2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest fits file: " << e.toString().toStdString().c_str() << std::endl;
  }

  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();
  // Dimensions Group
  ASSERT_EQ(cube.sampleCount(), 25);
  ASSERT_EQ(cube.lineCount(), 10);
  ASSERT_EQ(cube.bandCount(), 2);

    // Pixels group
  ASSERT_EQ(PixelTypeName(cube.pixelType()), "Real");
  ASSERT_EQ(ByteOrderName(cube.byteOrder()), "Lsb");
  ASSERT_DOUBLE_EQ(cube.base(), 0.0);
  ASSERT_DOUBLE_EQ(cube.multiplier(), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["Target"][0], "JUPITER" );

  std::unique_ptr<Histogram> hist (cube.histogram());

  ASSERT_NEAR(hist->Average(), 2.45598, .00001);
  ASSERT_NEAR(hist->Sum(), 613.9962, .0001);
  ASSERT_EQ(hist->ValidPixels(), 250);
  ASSERT_NEAR(hist->StandardDeviation(), 0.0242603, .0000001);
}

TEST(Fits2Isis, Fits2IsisOrganizationBil) {
  QTemporaryDir prefix;
  QString APP_XML = FileName("$ISISROOT/bin/xml/fits2isis.xml").expanded();
  QString cubeFileName = prefix.path() + "/fits2isisTEMP.cub";
  QVector<QString> args = {"from=data/fits2isis/organization.fits",
                           "to=" + cubeFileName,
                           "organization=bil",
                           "imagenumber=1"};

  UserInterface options(APP_XML, args);
  try {
   fits2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest fits file: " << e.toString().toStdString().c_str() << std::endl;
  }

  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  ASSERT_EQ(cube.sampleCount(), 25);
  ASSERT_EQ(cube.lineCount(), 2);
  ASSERT_EQ(cube.bandCount(), 10);

    // Pixels group
  ASSERT_EQ(PixelTypeName(cube.pixelType()), "Real");
  ASSERT_EQ(ByteOrderName(cube.byteOrder()), "Lsb");
  ASSERT_DOUBLE_EQ(cube.base(), 0.0);
  ASSERT_DOUBLE_EQ(cube.multiplier(), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["Target"][0], "JUPITER" );

  std::unique_ptr<Histogram> hist (cube.histogram());

  ASSERT_NEAR(hist->Average(), 1.25258, .00001);
  ASSERT_NEAR(hist->Sum(), 62.6292, .0001);
  ASSERT_EQ(hist->ValidPixels(), 50);
  ASSERT_NEAR(hist->StandardDeviation(), 1.253938, .000001);
}
