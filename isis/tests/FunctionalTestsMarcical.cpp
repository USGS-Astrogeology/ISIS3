#include <QTemporaryDir>

#include "marcical.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/marcical.xml").expanded();

TEST(Marcical, MarcicalTestDefault) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/marcical_out.cub";
  QVector<QString> args = {"from=data/marcical/P12_005901_3391_MA_00N096W_cropped.cub", "to=" + cubeFileName };
  UserInterface options(APP_XML, args);
  marcical(options);

  Cube outCube(cubeFileName);
  Pvl *label = outCube.label();

  PvlGroup &dims = label->findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ( (int)dims["Lines"], 2 );
  EXPECT_EQ( (int)dims["Samples"], 50 );
  EXPECT_EQ( (int)dims["Bands"], 5 );

  PvlGroup &inst = label->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ( inst["VariableExposureDuration"][0], "17.5" );
  EXPECT_EQ( inst["VariableExposureDuration"][1], "15.0" );
  EXPECT_EQ( inst["VariableExposureDuration"][2], "17.5" );

  std::unique_ptr<Histogram> outHist (outCube.histogram());
  EXPECT_NEAR( outHist->Average(), 0.046682, 1e-6 );
  EXPECT_NEAR( outHist->Sum(), 1.86728, 1e-5 );
  EXPECT_EQ( outHist->ValidPixels(), 40 );
  EXPECT_NEAR( outHist->StandardDeviation(), 0.0148127, 1e-7 );
}

TEST(Marcical, MarcicalTestDefaultNoIof) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/marcical_out.cub";
  QVector<QString> args = {"from=data/marcical/P12_005901_3391_MA_00N096W_cropped.cub", "to=" + cubeFileName, "iof=no" };
  UserInterface options(APP_XML, args);
  marcical(options);

  Cube outCube(cubeFileName);
  Pvl *label = outCube.label();

  PvlGroup &dims = label->findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ( (int)dims["Lines"], 2 );
  EXPECT_EQ( (int)dims["Samples"], 50 );
  EXPECT_EQ( (int)dims["Bands"], 5 );

  PvlGroup &inst = label->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ( inst["VariableExposureDuration"][0], "17.5" );
  EXPECT_EQ( inst["VariableExposureDuration"][1], "15.0" );
  EXPECT_EQ( inst["VariableExposureDuration"][2], "17.5" );

  std::unique_ptr<Histogram> outHist (outCube.histogram());
  EXPECT_NEAR( outHist->Average(), 11.78765,  1e-5);
  EXPECT_NEAR( outHist->Sum(), 471.5062, 1e-3 );
  EXPECT_EQ( outHist->ValidPixels(), 40 );
  EXPECT_NEAR( outHist->StandardDeviation(), 3.74036, 1e-5 );
}

TEST(Marcical, MarcicalTestSingleDuration) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/marcical_out.cub";
  QVector<QString> args = {"from=data/marcical/K14_059003_3475_MA_00N112W_cropped.cub", "to=" + cubeFileName };
  UserInterface options(APP_XML, args);
  marcical(options);

  Cube outCube(cubeFileName);
  Pvl *label = outCube.label();

  PvlGroup &dims = label->findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ( (int)dims["Lines"], 10 );
  EXPECT_EQ( (int)dims["Samples"], 10 );
  EXPECT_EQ( (int)dims["Bands"], 5 );

  PvlGroup &inst = label->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ( inst["VariableExposureDuration"][0], "8.8" );

  std::unique_ptr<Histogram> outHist (outCube.histogram());
  EXPECT_NEAR( outHist->Average(), 0.00879284, 1e-7 );
  EXPECT_NEAR( outHist->Sum(), 0.175856, 1e-6 );
  EXPECT_EQ( outHist->ValidPixels(), 20 );
  EXPECT_NEAR( outHist->StandardDeviation(), 0.000899121, 1e-8 );
}

TEST(Marcical, MarcicalTestSingleDurationNoIof) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/marcical_out.cub";
  QVector<QString> args = {"from=data/marcical/K14_059003_3475_MA_00N112W_cropped.cub", "to=" + cubeFileName, "iof=no" };
  UserInterface options(APP_XML, args);
  marcical(options);

  Cube outCube(cubeFileName);
  Pvl *label = outCube.label();

  PvlGroup &dims = label->findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ( (int)dims["Lines"], 10 );
  EXPECT_EQ( (int)dims["Samples"], 10 );
  EXPECT_EQ( (int)dims["Bands"], 5 );

  PvlGroup &inst = label->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ( inst["VariableExposureDuration"][0], "8.8" );

  std::unique_ptr<Histogram> outHist (outCube.histogram());
  EXPECT_NEAR( outHist->Average(), 2.16086, 1e-4 );
  EXPECT_NEAR( outHist->Sum(), 43.2172, 1e-4 );
  EXPECT_EQ( outHist->ValidPixels(), 20 );
  EXPECT_NEAR( outHist->StandardDeviation(), 0.22096, 1e-4 );
}
