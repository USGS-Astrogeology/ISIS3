
#include <QFileInfo> 
#include <QString>
#include <QTemporaryDir>
#include <QTextStream>

#include "Pvl.h"
#include "PvlGroup.h"
#include "mosrange.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/mosrange.xml").expanded();

/**
   * MosrangeDefault
   * 
   * Mosrange test given a list of four cubes and a map file.
   */
TEST(Mosrange, MosrangeDefault) {
 
  QVector<QString> args = {"fromlist=data/mosrange/mosrangeCubes.lis",
                           "map=data/mosrange/equi.map",
                           "precision=4"
                           };

  UserInterface options(APP_XML, args);

  Pvl results;

  try {
    results = mosrange(options);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  PvlGroup &mapping = results.findGroup("Mapping", Pvl::Traverse);

  EXPECT_EQ(mapping["ProjectionName"][0], "Equirectangular");
  EXPECT_EQ(mapping["TargetName"][0], "Mercury");
  EXPECT_DOUBLE_EQ(double(mapping["EquatorialRadius"]), 2440000.0);
  EXPECT_DOUBLE_EQ(double(mapping["PolarRadius"]), 2440000.0);
  EXPECT_EQ(mapping["LatitudeType"][0], "Planetocentric");
  EXPECT_EQ(mapping["LongitudeDirection"][0], "PositiveEast");
  EXPECT_EQ(int(mapping["LongitudeDomain"]), 360);
  EXPECT_DOUBLE_EQ(double(mapping["PixelResolution"]), 506.7143);
  EXPECT_DOUBLE_EQ(double(mapping["Scale"]), 84.0435);
  EXPECT_DOUBLE_EQ(double(mapping["MinObliquePixelResolution"]), 490.32027782048);
  EXPECT_DOUBLE_EQ(double(mapping["MaxObliquePixelResolution"]), 2265.4589309332);
  EXPECT_DOUBLE_EQ(double(mapping["CenterLongitude"]), 167.2285);
  EXPECT_DOUBLE_EQ(double(mapping["CenterLatitude"]), -13.6504);
  EXPECT_DOUBLE_EQ(double(mapping["MinimumLatitude"]), -21.5392);
  EXPECT_DOUBLE_EQ(double(mapping["MaximumLatitude"]), -5.7616);
  EXPECT_DOUBLE_EQ(double(mapping["MinimumLongitude"]), 134.2321);
  EXPECT_DOUBLE_EQ(double(mapping["MaximumLongitude"]), 200.2249);
}

/**
   * MosrangeOnErrorContinue
   * 
   * Mosrange test given a list of four cubes and a map file. Three have been
   * processed with spiceinit and one has not. Mosrange should continue to
   * produce a result with the three good cubes.
   */
TEST(Mosrange, MosrangeOnErrorContinue) {
  
  QVector<QString> args = {"fromlist=data/mosrange/mosrangeBadCube.lis",
                           "map=data/mosrange/equi.map",
                           "precision=4",
                           "onerror=continue"
                           };

  UserInterface options(APP_XML, args);

  Pvl results;

  try {
    results = mosrange(options);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  PvlGroup &mapping = results.findGroup("Mapping", Pvl::Traverse);

  EXPECT_EQ(mapping["ProjectionName"][0], "Equirectangular");
  EXPECT_EQ(mapping["TargetName"][0], "Mercury");
  EXPECT_DOUBLE_EQ(double(mapping["EquatorialRadius"]), 2440000.0);
  EXPECT_DOUBLE_EQ(double(mapping["PolarRadius"]), 2440000.0);
  EXPECT_EQ(mapping["LatitudeType"][0], "Planetocentric");
  EXPECT_EQ(mapping["LongitudeDirection"][0], "PositiveEast");
  EXPECT_EQ(int(mapping["LongitudeDomain"]), 360);
  EXPECT_DOUBLE_EQ(double(mapping["PixelResolution"]), 495.0249);
  EXPECT_DOUBLE_EQ(double(mapping["Scale"]), 86.0281);
  EXPECT_DOUBLE_EQ(double(mapping["MinObliquePixelResolution"]), 490.32027782048);
  EXPECT_DOUBLE_EQ(double(mapping["MaxObliquePixelResolution"]), 821.88316879416);
  EXPECT_DOUBLE_EQ(double(mapping["CenterLongitude"]), 154.5774);
  EXPECT_DOUBLE_EQ(double(mapping["CenterLatitude"]), -14.3546);
  EXPECT_DOUBLE_EQ(double(mapping["MinimumLatitude"]), -21.5392);
  EXPECT_DOUBLE_EQ(double(mapping["MaximumLatitude"]), -7.17);
  EXPECT_DOUBLE_EQ(double(mapping["MinimumLongitude"]), 134.2321);
  EXPECT_DOUBLE_EQ(double(mapping["MaximumLongitude"]), 174.9228);
}

/**
   * MosrangeOnErrorFail
   * 
   * Mosrange test given a list of four cubes and a map file. Three have been
   * processed with spiceinit and one has not. The unspiced cube should throw
   * the Exception "Unable to initialize camera model". Tests contents of output
   * errorlog and errorlist files.
   */
TEST(Mosrange, MosrangeOnErrorFail) {
  QTemporaryDir tempDir;
  
  QVector<QString> args = {"fromlist=data/mosrange/mosrangeBadCube.lis",
                           "map=data/mosrange/equi.map",
                           "precision=4",
                           "onerror=fail",
                           "errorlog=" + tempDir.path() + "/errorLog",
                           "errorlist=" + tempDir.path() + "/errorList.txt"
                           };

  UserInterface options(APP_XML, args);

  Pvl results;

  try {
    results = mosrange(options);
    FAIL() << "Expected Exception for a cube that hasn't been spiceinited";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Unable to initialize camera model"))
      << e.toString().toStdString();
  }

  // try to read back error log pvl output file
  Pvl errorLogPvl;
  try {
    errorLogPvl.read(tempDir.path().toStdString() + "/errorLog.log");
  }
  catch (IException &e) {
    FAIL() << "Unable to open error log pvl file: " << e.what() << std::endl;
  }

  ASSERT_TRUE(errorLogPvl.hasObject("ErrorSet"));
  PvlObject errorSet = errorLogPvl.findObject("ErrorSet");
  ASSERT_TRUE(errorSet.hasObject("File"));
  PvlObject errorFile = errorSet.findObject("File");
  ASSERT_TRUE(errorFile.hasKeyword("Name"));
  ASSERT_TRUE(errorFile.hasKeyword("Error"));

  // confirm name of cube with no spice error
  QFileInfo errorFileFullPath = (QFileInfo)(QString::fromStdString(errorFile.findKeyword("Name")));
  ASSERT_EQ(errorFileFullPath.fileName(), "EN0108828337M_noSPICE.cub");

  // confirm bad cube needs to be re-spiceinited
  QString errorType = QString::fromStdString(errorFile.findKeyword("Error"));
  ASSERT_TRUE(errorType.contains("re-run spiceinit"));

  // try to read back errorList output file
  QFile errorListFile(tempDir.path() + "/errorList.txt");
  try {
    errorListFile.open(QIODevice::ReadOnly | QIODevice::Text);
  }
  catch (IException &e) {
    FAIL() << "Unable to open error list text file: " << e.what() << std::endl;
  }

  // confirm name of cube with no spice error is in this file as well
  QTextStream in (&errorListFile);
  const QString content = in.readAll();
  ASSERT_TRUE(content.contains("EN0108828337M_noSPICE.cub"));
}
