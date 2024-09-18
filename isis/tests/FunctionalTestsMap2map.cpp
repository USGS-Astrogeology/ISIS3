#include <QTemporaryDir>

#include "NetworkFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "map2map.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/map2map.xml").expanded());


TEST_F(ThreeImageNetwork, FunctionalTestMap2mapDefault) {

  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QVector<QString> args = {"from=" + cube1map->fileName(),
                           "to=" + outCubeFileName,
                           "map=" + cube2map->fileName(),
                           "defaultrange=map",
                           "INTERP=NEARESTNEIGHBOR"};

  UserInterface options(APP_XML, args);
  try {
    map2map(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube ocube(outCubeFileName.toStdString());

  PvlGroup &mapping = ocube.label()->findObject("IsisCube").findGroup("Mapping");

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("ProjectionName"), "Equirectangular");
  EXPECT_NEAR((double)mapping.findKeyword("CenterLongitude"), 0.25400668736684, 0.0001);

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("TargetName"), "Mars");
  EXPECT_NEAR((double)mapping.findKeyword("EquatorialRadius"), 3396190.0, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("PolarRadius"), 3376200.0, 0.0001);

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("LatitudeType"), "Planetocentric");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("LongitudeDirection"), "PositiveEast");
  EXPECT_EQ((int)mapping.findKeyword("LongitudeDomain"), 180);

  EXPECT_NEAR((double)mapping.findKeyword("MinimumLatitude"), 0.47920860194551, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("MaximumLatitude"), 3.3932951263901, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("MinimumLongitude"), -0.94830771139743, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("MaximumLongitude"), 1.4318179715731, 0.0001);

  EXPECT_NEAR((double)mapping.findKeyword("UpperLeftCornerX"), -71250.037709109, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("UpperLeftCornerY"), 201236.66564437, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("PixelResolution"), 255.37647924412, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("Scale"), 232.10614255659, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("CenterLatitude"), 1.5210901942398, 0.0001);

  std::unique_ptr<Histogram> hist (ocube.histogram());

  EXPECT_NEAR(hist->Average(), -1.7976931348623149e+308, .00001);
  EXPECT_EQ(hist->Sum(), 0);
  EXPECT_EQ(hist->ValidPixels(), 0);
  EXPECT_NEAR(hist->StandardDeviation(), -1.7976931348623149e+308, .0001);
}


TEST_F(ThreeImageNetwork, FunctionalTestMap2mapKeywords) {
  // testing when scale keyword is defined in input cube and map
  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QVector<QString> args = {"from="+ cube1map->fileName(),
                           "to="+ outCubeFileName,
                           "map=data/map2map/yesScale.pvl",
                           "MATCHMAP=yes"};

  UserInterface options(APP_XML, args);
  try {
    map2map(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube ocube(outCubeFileName.toStdString());

  PvlGroup &mapping = ocube.label()->findObject("IsisCube").findGroup("Mapping");

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("ProjectionName"), "Equirectangular");
  EXPECT_NEAR((double)mapping.findKeyword("CenterLongitude"), 0.25400668736684, 0.0001);

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("TargetName"), "Mars");
  EXPECT_NEAR((double)mapping.findKeyword("EquatorialRadius"), 3396190.0, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("PolarRadius"), 3376200.0, 0.0001);

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("LatitudeType"), "Planetocentric");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("LongitudeDirection"), "PositiveEast");
  EXPECT_EQ((int)mapping.findKeyword("LongitudeDomain"), 180);

  EXPECT_NEAR((double)mapping.findKeyword("MinimumLatitude"), 0.47920860194551, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("MaximumLatitude"), 3.3932951263901, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("MinimumLongitude"), -0.94830771139743, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("MaximumLongitude"), 1.4318179715731, 0.0001);

  EXPECT_NEAR((double)mapping.findKeyword("UpperLeftCornerX"), -71250.037709109005, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("UpperLeftCornerY"), 201236.66564436999, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("PixelResolution"), 255.37647924412, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("Scale"), 232.10614255659, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("CenterLatitude"), 1.5210901942398, 0.0001);

  std::unique_ptr<Histogram> hist (ocube.histogram());

  EXPECT_NEAR(hist->Average(), -1.7976931348623149e+308, .00001);
  EXPECT_EQ(hist->Sum(), 0);
  EXPECT_EQ(hist->ValidPixels(), 0);
  EXPECT_NEAR(hist->StandardDeviation(), -1.7976931348623149e+308, .0001);


  QString noScaleFile = tempDir.path() + "/outTempNoScale.cub";
  QVector<QString> argsNoScale = {"from=" + cube1map->fileName(),
                                  "to=" + noScaleFile,
                                  "map=data/map2map/noScale.pvl",
                                  "MATCHMAP=yes"};

  // testing for when the Scale keyword is missing from the input cube and the map.
  PvlGroup &mappingGroup = cube1map->label()->findObject("IsisCube").findGroup("Mapping");
  mappingGroup.deleteKeyword("Scale");

  UserInterface optionsNoScale(APP_XML, argsNoScale);
  try {
    map2map(optionsNoScale);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube ocubeNoScale(noScaleFile.toStdString());

  PvlGroup &mapNoScale = ocubeNoScale.label()->findObject("IsisCube").findGroup("Mapping");

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapNoScale.findKeyword("ProjectionName"), "Equirectangular");
  EXPECT_NEAR((double)mapNoScale.findKeyword("CenterLongitude"), 0.25400668736684, 0.0001);

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("TargetName"), "Mars");
  EXPECT_NEAR((double)mapNoScale.findKeyword("EquatorialRadius"), 3396190.0, 0.0001);
  EXPECT_NEAR((double)mapNoScale.findKeyword("PolarRadius"), 3376200.0, 0.0001);

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapNoScale.findKeyword("LatitudeType"), "Planetocentric");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapNoScale.findKeyword("LongitudeDirection"), "PositiveEast");
  EXPECT_EQ((int)mapNoScale.findKeyword("LongitudeDomain"), 180);

  EXPECT_NEAR((double)mapNoScale.findKeyword("MinimumLatitude"), 0.47920860194551, 0.0001);
  EXPECT_NEAR((double)mapNoScale.findKeyword("MaximumLatitude"), 3.3932951263901, 0.0001);
  EXPECT_NEAR((double)mapNoScale.findKeyword("MinimumLongitude"), -0.94830771139743, 0.0001);
  EXPECT_NEAR((double)mapNoScale.findKeyword("MaximumLongitude"), 1.4318179715731, 0.0001);

  EXPECT_NEAR((double)mapNoScale.findKeyword("UpperLeftCornerX"), -71250.037709109005, 0.0001);
  EXPECT_NEAR((double)mapNoScale.findKeyword("UpperLeftCornerY"), 201236.66564436999, 0.0001);
  EXPECT_NEAR((double)mapNoScale.findKeyword("PixelResolution"), 255.37647924412, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("Scale"), 232.10614255659, 0.0001);
  EXPECT_NEAR((double)mapNoScale.findKeyword("CenterLatitude"), 1.5210901942398, 0.0001);

  std::unique_ptr<Histogram> histNoScale (ocubeNoScale.histogram());

  EXPECT_NEAR(histNoScale->Average(), -1.7976931348623149e+308, .00001);
  EXPECT_EQ(histNoScale->Sum(), 0);
  EXPECT_EQ(histNoScale->ValidPixels(), 0);
  EXPECT_NEAR(histNoScale->StandardDeviation(), -1.7976931348623149e+308, .0001);
}


TEST_F(ThreeImageNetwork, FunctionalTestMap2mapUnits) {

  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QVector<QString> args = {"from="+ cube1map->fileName(),
                           "to="+outCubeFileName,
                           "map=data/map2map/mapfile.map",
                           "MAXLON=1.0"};

  UserInterface options(APP_XML, args);
  try {
    map2map(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube ocube(outCubeFileName.toStdString());

  PvlGroup &mapping = ocube.label()->findObject("IsisCube").findGroup("Mapping");

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("ProjectionName"), "Equirectangular");
  EXPECT_NEAR((double)mapping.findKeyword("CenterLongitude"),  -0.25400668736682003, 0.0001);

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("TargetName"), "Mars");
  EXPECT_NEAR((double)mapping.findKeyword("EquatorialRadius"), 3396190.0, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("PolarRadius"), 3376200.0, 0.0001);

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("LatitudeType"), "Planetocentric");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("LongitudeDirection"), "PositiveWest");
  EXPECT_EQ((int)mapping.findKeyword("LongitudeDomain"), 180);

  EXPECT_NEAR((double)mapping.findKeyword("MinimumLatitude"), 0.47920860194551, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("MaximumLatitude"), 3.3932951263901, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("MinimumLongitude"), -1.4318179715731001, 0.0001);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("MaximumLongitude"), 1.0);

  EXPECT_NEAR((double)mapping.findKeyword("UpperLeftCornerX"), -74314.555460039002, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("UpperLeftCornerY"), 201236.66564436999, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("PixelResolution"), 255.37647924412, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("Scale"), 232.10614255659, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("CenterLatitude"), 1.5210901942398, 0.0001);

  std::unique_ptr<Histogram> hist (ocube.histogram());

  EXPECT_NEAR(hist->Average(), -1.7976931348623149e+308, .00001);
  EXPECT_EQ(hist->Sum(), 0);
  EXPECT_EQ(hist->ValidPixels(), 0);
  EXPECT_NEAR(hist->StandardDeviation(), -1.7976931348623149e+308, .0001);
}


TEST(Map2mapTest, FunctionalTestMap2mapProjection) {
  QTemporaryDir prefix;
  QString outCubeFileName = prefix.path() + "/outTemp.cub";
  QVector<QString> args = {"from=data/map2map/WAC_GLD100_V1.0_GLOBAL_with_LOLA_30M_POLE.10km_cropped.cub",
                           "to="+outCubeFileName,
                           "map=data/map2map/orthographic.map",
                           "DEFAULTRANGE=MAP"};

  UserInterface options(APP_XML, args);
  try {
    map2map(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube ocube(outCubeFileName.toStdString());

  PvlGroup &mapping = ocube.label()->findObject("IsisCube").findGroup("Mapping");

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("ProjectionName"), "Orthographic");
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("CenterLongitude"), 0.0);

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("TargetName"), "Moon");
  EXPECT_NEAR((double)mapping.findKeyword("EquatorialRadius"), 1737400.0, 0.0001);
  EXPECT_NEAR((double)mapping.findKeyword("PolarRadius"), 1737400.0, 0.0001);

  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("LatitudeType"), "Planetocentric");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, mapping.findKeyword("LongitudeDirection"), "PositiveEast");
  EXPECT_EQ((int)mapping.findKeyword("LongitudeDomain"), 360);

  EXPECT_EQ((int)mapping.findKeyword("MinimumLatitude"), -90);
  EXPECT_EQ((int)mapping.findKeyword("MaximumLatitude"), 90);
  EXPECT_EQ((int)mapping.findKeyword("MinimumLongitude"), -180);
  EXPECT_EQ((int)mapping.findKeyword("MaximumLongitude"), 180);

  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("UpperLeftCornerX"), -1740000.0);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("UpperLeftCornerY"), 1740000.0);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("PixelResolution"), 10000.0);
  EXPECT_NEAR((double)mapping.findKeyword("Scale"), 3.0323350424149, 0.0001);
  EXPECT_DOUBLE_EQ((double)mapping.findKeyword("CenterLatitude"), 0.0);

  std::unique_ptr<Histogram> hist (ocube.histogram());

  EXPECT_NEAR(hist->Average(), -1.7976931348623149e+308, .00001);
  EXPECT_EQ(hist->Sum(), 0);
  EXPECT_EQ(hist->ValidPixels(), 0);
  EXPECT_NEAR(hist->StandardDeviation(), -1.7976931348623149e+308, .0001);
}
