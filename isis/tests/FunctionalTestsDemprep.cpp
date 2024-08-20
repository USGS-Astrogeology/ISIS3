#include <iostream>
#include <QTemporaryFile>
#include <QTemporaryDir>

#include "demprep.h"

#include "Cube.h"
#include "Pvl.h"
#include "TestUtilities.h"
#include "FileName.h"
#include "LineManager.h"
#include "Table.h"
#include "Histogram.h"

#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/demprep.xml").expanded();

TEST(Demprep, DemprepDefault){
  Pvl appLog;
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/padded.cub";
  QVector<QString> args = {"from=data/demprep/ulcn2005_lpo_downsampled.cub", "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
   demprep(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to prep DEM: " << e.toString().toStdString().c_str() << std::endl;
  }

  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();

  ASSERT_EQ(cube.sampleCount(), 439);
  ASSERT_EQ(cube.lineCount(), 221);
  ASSERT_EQ(cube.bandCount(), 1);

  // Pixels Group
  PvlGroup &pixels = isisLabel->findGroup("Pixels", Pvl::Traverse);
  ASSERT_EQ(pixels["Type"][0], "SignedWord");
  ASSERT_EQ(pixels["ByteOrder"][0], "Lsb");
  ASSERT_EQ(double(pixels["Base"]), 1737400.0);
  ASSERT_EQ(double(pixels["Multiplier"]), 1.0);

  // BandBin Group
  // Check size, first, 2 middle, and last values? Enough?
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["Center"].size(), 1);
  ASSERT_EQ(bandbin["OriginalBand"].size(), 1);

  // Mapping Group
  PvlGroup &mapping = isisLabel->findGroup("Mapping", Pvl::Traverse);
  ASSERT_EQ(mapping["ProjectionName"][0], "Equirectangular");
  ASSERT_DOUBLE_EQ(double(mapping["CenterLongitude"]), 180.0);
  ASSERT_EQ(mapping["TargetName"][0], "Moon");
  ASSERT_DOUBLE_EQ(double(mapping["EquatorialRadius"]), 1737400.0);
  ASSERT_DOUBLE_EQ(double(mapping["PolarRadius"]), 1737400.0);
  ASSERT_EQ(mapping["LatitudeType"][0], "Planetocentric");
  ASSERT_EQ(mapping["LongitudeDirection"][0], "PositiveEast");
  ASSERT_EQ(int(mapping["LongitudeDomain"]), 180);
  ASSERT_DOUBLE_EQ(double(mapping["MinimumLatitude"]), -90.0);
  ASSERT_DOUBLE_EQ(double(mapping["MaximumLatitude"]), 90.0);
  ASSERT_DOUBLE_EQ(double(mapping["MinimumLongitude"]), -180.0);
  ASSERT_DOUBLE_EQ(double(mapping["MaximumLongitude"]), 180.0);
  ASSERT_DOUBLE_EQ(double(mapping["UpperLeftCornerX"]), -10950000.0);
  ASSERT_DOUBLE_EQ(double(mapping["UpperLeftCornerY"]), 2775000.0);
  ASSERT_DOUBLE_EQ(double(mapping["PixelResolution"]), 25000.0);
  ASSERT_NEAR(double(mapping["Scale"]), 1.21293, .00001);
  ASSERT_DOUBLE_EQ(double(mapping["CenterLatitude"]), 0.0);

  Table shapeModel = cube.readTable("ShapeModelStatistics");
  // Assertion for minimum radius
  ASSERT_DOUBLE_EQ(double(shapeModel[0][0]), 1728.805);
  // Assertion for maximum radius
  ASSERT_DOUBLE_EQ(double(shapeModel[0][1]), 1745.313);


  std::unique_ptr<Histogram> hist (cube.histogram());

  ASSERT_NEAR(hist->Average(), 1736765.71744, .00001);
  ASSERT_DOUBLE_EQ(hist->Sum(), 166974392841);
  ASSERT_EQ(hist->ValidPixels(), 96141);
  ASSERT_NEAR(hist->StandardDeviation(), 2055.78, .01);
}


TEST(Demprep, DemprepInside){

  Pvl appLog;
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/padded.cub";
  QVector<QString> args = {"from=data/demprep/ulcn2005_lpo_inside.cub", "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
   demprep(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to prep DEM: " << e.toString().toStdString().c_str() << std::endl;
  }

  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();
  ASSERT_EQ(cube.sampleCount(), 250);
  ASSERT_EQ(cube.lineCount(), 250);
  ASSERT_EQ(cube.bandCount(), 1);

  // Mapping Group
  PvlGroup &mapping = isisLabel->findGroup("Mapping", Pvl::Traverse);
  ASSERT_EQ(mapping["ProjectionName"][0], "SimpleCylindrical");
  ASSERT_NEAR(double(mapping["UpperLeftCornerX"]), -5801235.97802, .00001);
  ASSERT_NEAR(double(mapping["UpperLeftCornerY"]), 77703.58546, .00001);
  ASSERT_NEAR(double(mapping["PixelResolution"]), 1895.20940, .00001);
  ASSERT_DOUBLE_EQ(double(mapping["Scale"]), 16.0);
}



TEST(Demprep, DemprepSpecialPixels){
  Pvl appLog;
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/padded.cub";
  QVector<QString> args = {"from=data/demprep/vest64_dtm_specialpixels_downsampled.cub", "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
   demprep(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to prep DEM: " << e.toString().toStdString().c_str() << std::endl;
  }

  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();
  ASSERT_EQ(cube.sampleCount(), 366);
  ASSERT_EQ(cube.lineCount(), 184);
  ASSERT_EQ(cube.bandCount(), 1);

  // Mapping Group
  PvlGroup &mapping = isisLabel->findGroup("Mapping", Pvl::Traverse);
  ASSERT_EQ(mapping["ProjectionName"][0], "Equirectangular");
  ASSERT_DOUBLE_EQ(double(mapping["UpperLeftCornerX"]), -915000.0);
  ASSERT_DOUBLE_EQ(double(mapping["UpperLeftCornerY"]), 460000.0);
  ASSERT_DOUBLE_EQ(double(mapping["PixelResolution"]), 5000);
  ASSERT_NEAR(double(mapping["Scale"]), 1.00880, .00001);


  std::unique_ptr<Histogram> hist (cube.histogram());

  ASSERT_NEAR(double(hist->Average()), 254239.25400, .00001);
  ASSERT_DOUBLE_EQ(hist->Sum(), 3869267206.65625);
  ASSERT_EQ(hist->ValidPixels(), 15219);
  ASSERT_EQ(hist->NullPixels(), 51665);
  ASSERT_EQ(hist->LisPixels(), 460);
  ASSERT_EQ(hist->LrsPixels(), 0);
  ASSERT_EQ(hist->HisPixels(), 0);
  ASSERT_EQ(hist->HrsPixels(), 0);
  ASSERT_NEAR(hist->StandardDeviation(), 22217.85549, .00001);
}


TEST(Demprep, DemprepSouthPole){
  Pvl appLog;
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/padded.cub";
  QVector<QString> args = {"from=data/demprep/ulcn2005_lpo_npole.cub", "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
   demprep(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to prep DEM: " << e.toString().toStdString().c_str() << std::endl;
  }

  Cube cube(cubeFileName);
  ASSERT_EQ(cube.sampleCount(), 250);
  ASSERT_EQ(cube.lineCount(), 251);
  ASSERT_EQ(cube.bandCount(), 1);

  std::unique_ptr<Histogram> hist (cube.histogram());

  ASSERT_NEAR(double(hist->Average()), 1737016.52267, .00001);
  ASSERT_EQ(hist->Sum(), 108997786798);
  ASSERT_EQ(hist->ValidPixels(), 62750);
  ASSERT_EQ(hist->NullPixels(), 0);
  ASSERT_NEAR(hist->StandardDeviation(), 449.297, .001);
}



TEST(Demprep, DemprepNorthPole){
  Pvl appLog;
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/padded.cub";
  QVector<QString> args = {"from=data/demprep/ulcn2005_lpo_npole.cub", "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
   demprep(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to prep DEM: " << e.toString().toStdString().c_str() << std::endl;
  }

  Cube cube(cubeFileName);
  ASSERT_EQ(cube.sampleCount(), 250);
  ASSERT_EQ(cube.lineCount(), 251);
  ASSERT_EQ(cube.bandCount(), 1);

  std::unique_ptr<Histogram> hist (cube.histogram());

  ASSERT_NEAR(double(hist->Average()), 1737016.52267, .00001);
  ASSERT_DOUBLE_EQ(hist->Sum(), 108997786798);
  ASSERT_EQ(hist->ValidPixels(), 62750);
  ASSERT_EQ(hist->NullPixels(), 0);
  ASSERT_NEAR(hist->StandardDeviation(), 449.297, .001);
}
