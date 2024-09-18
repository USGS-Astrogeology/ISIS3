#include "CubeFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "LineManager.h"
#include "Histogram.h"

#include "bandnorm.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/bandnorm.xml").expanded());

TEST_F(SmallCube, FunctionalTestBandnormDefault) {
  QString outCubeFileName = tempDir.path() + "/outTEMP.cub";

  // force all bands to be normalized to 1's
  LineManager line(*testCube);
  double pixelValue = 10;
  for(line.begin(); !line.end(); line++) {
    for(int i = 0; i < line.size(); i++) {
      line[i] = (double) pixelValue;
    }
    testCube->write(line);
  }
  QVector<QString> args = {"to="+outCubeFileName};

  UserInterface options(APP_XML, args);
  try {
    bandnorm(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to process image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString());
  std::unique_ptr<Histogram> oCubeStats;

  for (int i = 1; i <= testCube->bandCount(); i++) {
    oCubeStats.reset(oCube.histogram(i));
    ASSERT_DOUBLE_EQ(oCubeStats->Average(), 1);
    ASSERT_DOUBLE_EQ(oCubeStats->Sum(), 100);
    ASSERT_EQ(oCubeStats->ValidPixels(), 100);
    ASSERT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 0);
  }
}


TEST_F(SmallCube, FunctionalTestBandnormPencil) {
  QString outCubeFileName = tempDir.path() + "/outTEMP.cub";
  QString pencilPath = tempDir.path() + "/pencil.txt";

  QVector<QString> args = {"to="+outCubeFileName, "SPECTRUM="+pencilPath, "AVERAGE=PENCIL"};

  QFile file(pencilPath);
  file.open(QIODevice::WriteOnly);
  QTextStream qout(&file);
  qout << "\" Average\", \n";

  for(int i = 1; i <=10; i++) {
    QString text(QString::number(i) + "\n");
    qout << text;
  }
  file.close();

  UserInterface options(APP_XML, args);
  try {
    bandnorm(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to process image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString());
  std::unique_ptr<Histogram> oCubeStats(oCube.histogram());
  ASSERT_DOUBLE_EQ(oCubeStats->Average(), 49.5);
  ASSERT_DOUBLE_EQ(oCubeStats->Sum(), 4950);
  ASSERT_EQ(oCubeStats->ValidPixels(), 100);
  ASSERT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 29.011491975882016);

  oCubeStats.reset(oCube.histogram(2));
  ASSERT_DOUBLE_EQ(oCubeStats->Average(), 74.75);
  ASSERT_DOUBLE_EQ(oCubeStats->Sum(), 7475);
  ASSERT_EQ(oCubeStats->ValidPixels(), 100);
  ASSERT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 14.505745987941008);

  oCubeStats.reset(oCube.histogram(3));
  ASSERT_DOUBLE_EQ(oCubeStats->Average(), 83.166666641235352);
  ASSERT_DOUBLE_EQ(oCubeStats->Sum(), 8316.6666641235352);
  ASSERT_EQ(oCubeStats->ValidPixels(), 100);
  ASSERT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 9.6704973399038625);

  oCubeStats.reset(oCube.histogram(4));
  EXPECT_NEAR(oCubeStats->Average(), 87.375, 0.0001);
  EXPECT_NEAR(oCubeStats->Sum(), 8737.5, 0.0001);
  ASSERT_DOUBLE_EQ(oCubeStats->ValidPixels(), 100);
  EXPECT_NEAR(oCubeStats->StandardDeviation(), 7.25287, 0.0001);

  oCubeStats.reset(oCube.histogram(5));
  EXPECT_NEAR(oCubeStats->Average(), 89.9, 0.0001);
  EXPECT_NEAR(oCubeStats->Sum(), 8990, 0.0001);
  ASSERT_DOUBLE_EQ(oCubeStats->ValidPixels(), 100);
  EXPECT_NEAR(oCubeStats->StandardDeviation(), 5.8023, 0.0001);

  oCubeStats.reset(oCube.histogram(6));
  EXPECT_NEAR(oCubeStats->Average(), 91.5833, 0.0001);
  EXPECT_NEAR(oCubeStats->Sum(), 9158.3333358764648, 0.0001);
  ASSERT_DOUBLE_EQ(oCubeStats->ValidPixels(), 100);
  EXPECT_NEAR(oCubeStats->StandardDeviation(), 4.83525, 0.0001);

  oCubeStats.reset(oCube.histogram(7));
  EXPECT_NEAR(oCubeStats->Average(), 92.7857, 0.0001);
  EXPECT_NEAR(oCubeStats->Sum(), 9278.5714263916016, 0.0001);
  ASSERT_DOUBLE_EQ(oCubeStats->ValidPixels(), 100);
  EXPECT_NEAR(oCubeStats->StandardDeviation(), 4.1445, 0.0001);

  oCubeStats.reset(oCube.histogram(8));
  EXPECT_NEAR(oCubeStats->Average(), 93.6875, 0.0001);
  EXPECT_NEAR(oCubeStats->Sum(), 9368.75, 0.0001);
  ASSERT_DOUBLE_EQ(oCubeStats->ValidPixels(), 100);
  EXPECT_NEAR(oCubeStats->StandardDeviation(), 3.62644, 0.0001);

  oCubeStats.reset(oCube.histogram(9));
  EXPECT_NEAR(oCubeStats->Average(), 94.3889, 0.0001);
  EXPECT_NEAR(oCubeStats->Sum(), 9438.8888854980469, 0.0001);
  ASSERT_DOUBLE_EQ(oCubeStats->ValidPixels(), 100);
  EXPECT_NEAR(oCubeStats->StandardDeviation(), 3.2235, 0.0001);

  oCubeStats.reset(oCube.histogram(10));
  EXPECT_NEAR(oCubeStats->Average(), 94.95, 0.0001);
  EXPECT_NEAR(oCubeStats->Sum(), 9495, 0.0001);
  ASSERT_DOUBLE_EQ(oCubeStats->ValidPixels(), 100);
  EXPECT_NEAR(oCubeStats->StandardDeviation(), 2.90115, 0.0001);
}


TEST_F(SmallCube, FunctionalTestBandnormByNumber) {
  QString outCubeFileName = tempDir.path() + "/outTEMP.cub";
  QString pencilPath = tempDir.path() + "/pencil.txt";

  QVector<QString> args = {"to="+outCubeFileName, "SPECTRUM="+pencilPath, "AVERAGE=PENCIL",
                           "method=number", "number=1"};

  QFile file(pencilPath);
  file.open(QIODevice::WriteOnly);
  QTextStream qout(&file);
  qout << "\" Average\", \" eh\", \n";

  for(int i = 1; i <=10; i++) {
    int n = 1;
    if (i == 7) n = 7;

    QString text = QString::number(i) + "," + QString::number(n) + "\n";
    qout << text;
  }

  file.close();

  UserInterface options(APP_XML, args);
  try {
    bandnorm(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to process image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString());
  std::unique_ptr<Histogram> oCubeStats(oCube.histogram(7));
  EXPECT_NEAR(oCubeStats->Average(), 92.7857, 0.0001);
  EXPECT_NEAR(oCubeStats->Sum(), 9278.5714263916016, 0.0001);
  ASSERT_DOUBLE_EQ(oCubeStats->ValidPixels(), 100);
  EXPECT_NEAR(oCubeStats->StandardDeviation(), 4.1445, 0.0001);

  // double check to see that other bands were not changed
  oCubeStats.reset(oCube.histogram(2));
  ASSERT_DOUBLE_EQ(oCubeStats->Average(), 149.5);
  ASSERT_DOUBLE_EQ(oCubeStats->Sum(), 14950);
  ASSERT_EQ(oCubeStats->ValidPixels(), 100);
  ASSERT_DOUBLE_EQ(oCubeStats->StandardDeviation(), 29.011491975882016);
}


TEST_F(SmallCube, FunctionalTestBandnormByBandAvg) {
  QString outCubeFileName = tempDir.path() + "/outTEMP.cub";
  QString pencilPath = tempDir.path() + "/pencil.txt";

  QVector<QString> args = {"to="+outCubeFileName, "AVERAGE=Band"};

  UserInterface options(APP_XML, args);
  try {
    bandnorm(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to process image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString());
  std::unique_ptr<Histogram> oCubeStats(oCube.histogram(1));
  EXPECT_NEAR(oCubeStats->Average(), 1, 0.0001);
  EXPECT_NEAR(oCubeStats->Sum(), 100.0, 0.0001);
  ASSERT_DOUBLE_EQ(oCubeStats->ValidPixels(), 100);
  EXPECT_NEAR(oCubeStats->StandardDeviation(), 0.586090, 0.0001);

  // double check to see that other bands were not changed
  oCubeStats.reset(oCube.histogram(2));
  EXPECT_NEAR(oCubeStats->Average(), 0.96763754, 0.0001);
  EXPECT_NEAR(oCubeStats->Sum(), 96.76375418, 0.0001);
  EXPECT_EQ(oCubeStats->ValidPixels(), 100);
  EXPECT_NEAR(oCubeStats->StandardDeviation(), 0.1877766488, 0.0001);
}


TEST_F(SmallCube, FunctionalTestBandnormByCubeAvg) {
  QString outCubeFileName = tempDir.path() + "/outTEMP.cub";
  QString pencilPath = tempDir.path() + "/pencil.txt";

  QVector<QString> args = {"to="+outCubeFileName, "AVERAGE=Cube"};

  UserInterface options(APP_XML, args);
  try {
    bandnorm(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to process image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString());
  std::unique_ptr<Histogram> oCubeStats(oCube.histogram(1));
  EXPECT_NEAR(oCubeStats->Average(), 0.099099099056329576, 0.0001);
  EXPECT_NEAR(oCubeStats->Sum(), 9.9099099056329578, 0.0001);
  ASSERT_DOUBLE_EQ(oCubeStats->ValidPixels(), 100);
  EXPECT_NEAR(oCubeStats->StandardDeviation(), 0.05808106515551998, 0.0001);

  // double check to see that other bands were not changed
  oCubeStats.reset(oCube.histogram(2));
  EXPECT_NEAR(oCubeStats->Average(), 0.29929929912090303, 0.0001);
  EXPECT_NEAR(oCubeStats->Sum(), 29.929929912090302, 0.0001);
  EXPECT_EQ(oCubeStats->ValidPixels(), 100);
  EXPECT_NEAR(oCubeStats->StandardDeviation(), 0.058081065874528971, 0.0001);
}
