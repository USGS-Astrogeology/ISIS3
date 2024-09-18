#include <memory>
#include <ostream>

#include <QString>
#include <QTemporaryDir>
#include <QVector>

#include "Cube.h"
#include "FileName.h"
#include "Histogram.h"
#include "IException.h"
#include "TestUtilities.h"
#include "UserInterface.h"

#include "gtest/gtest.h"

#include "lrowacphomap.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/lrowacphomap.xml").expanded());

TEST(Lrowacphomap, FunctionalTestLrowacphomapWithBack) {
  QTemporaryDir tempDir;
  ASSERT_TRUE(tempDir.isValid());

  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QString testCubeFileName = "data/lrowacphomap/M1181493219CE.vis.odd.reduced.cub";
  QString backplaneFileName = "data/lrowacphomap/back.reduced.cub";
  QString phoPvlFileName = "data/lrowacphomap/hapke_full_reformatted.pvl";
  QString paramMapFileName = "data/lrowacphomap/1x1_70NS_7b_wbhs_albflt_grid_geirist_tcorrect.reduced.cub";

  QVector<QString> args = {"from=" + testCubeFileName + "+1",
                           "to=" + outCubeFileName,
                           "backplane=" + backplaneFileName,
                           "phoa=" + phoPvlFileName,
                           "phop=" + paramMapFileName};
  UserInterface options(APP_XML, args);

  try {
    lrowacphomap(options);
  }
  catch(IException &e) {
    FAIL() << "Call to lrowacphomap failed, unable to apply photometric correction to input cube: " 
           << e.what() << std::endl;
  }

  double expectedAvg = 58.565850201775;
  double expectedStdDev = 19.336237864721;
  double expectedMedian = 56.231717465174;
  double expectedMin = 21.181716918945;
  double expectedMax = 160.17492675781;
  double expectedSum = 197132.65177917;

  Cube outCube(outCubeFileName.toStdString());

  std::unique_ptr<Histogram> hist(outCube.histogram());

  EXPECT_NEAR(hist->Average(), expectedAvg, 0.001);
  EXPECT_NEAR(hist->StandardDeviation(), expectedStdDev, 0.001);
  EXPECT_NEAR(hist->Median(), expectedMedian, 0.001);
  EXPECT_NEAR(hist->Minimum(), expectedMin, 0.001);
  EXPECT_NEAR(hist->Maximum(), expectedMax, 0.001);
  EXPECT_NEAR(hist->Sum(), expectedSum, 0.001);
}

TEST(Lrowacphomap, FunctionalTestLrowacphomapNoBack) {
  QTemporaryDir tempDir;
  ASSERT_TRUE(tempDir.isValid());

  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QString testCubeFileName = "data/lrowacphomap/M1181493219CE.vis.odd.reduced.cub";
  QString phoPvlFileName = "data/lrowacphomap/hapke_full_reformatted.pvl";
  QString paramMapFileName = "data/lrowacphomap/1x1_70NS_7b_wbhs_albflt_grid_geirist_tcorrect.reduced.cub";

  QVector<QString> args = {"from=" + testCubeFileName + "+1",
                           "to=" + outCubeFileName,
                           "phoa=" + phoPvlFileName,
                           "phop=" + paramMapFileName,
                           "usedem=true"};
  UserInterface options(APP_XML, args);

  try {
    lrowacphomap(options);
  }
  catch(IException &e) {
    FAIL() << "Call to lrowacphomap failed, unable to apply photometric correction to input cube: " 
           << e.what() << std::endl;
  }

  double expectedAvg = 58.159470616532;
  double expectedStdDev = 18.558190342074;
  double expectedMedian = 56.508963061387;
  double expectedMin = 23.405038833618;
  double expectedMax = 155.67340087891;
  double expectedSum = 195764.77809525;

  Cube outCube(outCubeFileName.toStdString());

  std::unique_ptr<Histogram> hist(outCube.histogram());

  EXPECT_NEAR(hist->Average(), expectedAvg, 0.001);
  EXPECT_NEAR(hist->StandardDeviation(), expectedStdDev, 0.001);
  EXPECT_NEAR(hist->Median(), expectedMedian, 0.001);
  EXPECT_NEAR(hist->Minimum(), expectedMin, 0.001);
  EXPECT_NEAR(hist->Maximum(), expectedMax, 0.001);
  EXPECT_NEAR(hist->Sum(), expectedSum, 0.001);
}

TEST(Lrowacphomap, FunctionalTestLrowacphomapDefaultAlgoAndParCubeWithBack) {
  QTemporaryDir tempDir;
  ASSERT_TRUE(tempDir.isValid());

  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QString testCubeFileName = "data/lrowacphomap/M1181493219CE.vis.odd.reduced.cub";
  QString backplaneFileName = "data/lrowacphomap/back.reduced.cub";

  QVector<QString> args = {"from=" + testCubeFileName + "+1",
                           "to=" + outCubeFileName,
                           "backplane=" + backplaneFileName};
  UserInterface options(APP_XML, args);

  try {
    lrowacphomap(options);
  }
  catch(IException &e) {
    FAIL() << "Call to lrowacphomap failed, unable to apply photometric correction to input cube: " 
           << e.what() << std::endl;
  }

  double expectedAvg = 57.150192172911;
  double expectedStdDev = 19.201699528246;
  double expectedMedian = 55.0376024164;
  double expectedMin = 20.151010513306;
  double expectedMax = 157.08757019043;
  double expectedSum = 192367.54685402;

  Cube outCube(outCubeFileName.toStdString());

  std::unique_ptr<Histogram> hist(outCube.histogram());

  EXPECT_NEAR(hist->Average(), expectedAvg, 0.001);
  EXPECT_NEAR(hist->StandardDeviation(), expectedStdDev, 0.001);
  EXPECT_NEAR(hist->Median(), expectedMedian, 0.001);
  EXPECT_NEAR(hist->Minimum(), expectedMin, 0.001);
  EXPECT_NEAR(hist->Maximum(), expectedMax, 0.001);
  EXPECT_NEAR(hist->Sum(), expectedSum, 0.001);
}

TEST(Lrowacphomap, FunctionalTestLrowacphomapDefaultAlgoAndParCubeNoBack) {
  QTemporaryDir tempDir;
  ASSERT_TRUE(tempDir.isValid());

  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QString testCubeFileName = "data/lrowacphomap/M1181493219CE.vis.odd.reduced.cub";

  QVector<QString> args = {"from=" + testCubeFileName + "+1",
                           "to=" + outCubeFileName,
                           "usedem=true"};
  UserInterface options(APP_XML, args);

  try {
    lrowacphomap(options);
  }
  catch(IException &e) {
    FAIL() << "Call to lrowacphomap failed, unable to apply photometric correction to input cube: " 
           << e.what() << std::endl;
  }

  double expectedAvg = 56.750011832815;
  double expectedStdDev = 18.44290433699;
  double expectedMedian = 55.263128187622;
  double expectedMin = 22.274614334106;
  double expectedMax = 152.65106201172;
  double expectedSum = 191020.53982925;

  Cube outCube(outCubeFileName.toStdString());

  std::unique_ptr<Histogram> hist(outCube.histogram());

  EXPECT_NEAR(hist->Average(), expectedAvg, 0.001);
  EXPECT_NEAR(hist->StandardDeviation(), expectedStdDev, 0.001);
  EXPECT_NEAR(hist->Median(), expectedMedian, 0.001);
  EXPECT_NEAR(hist->Minimum(), expectedMin, 0.001);
  EXPECT_NEAR(hist->Maximum(), expectedMax, 0.001);
  EXPECT_NEAR(hist->Sum(), expectedSum, 0.001);
}
