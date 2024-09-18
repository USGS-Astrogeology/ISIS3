#include "CameraFixtures.h"
#include "CubeFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "LineManager.h"
#include "Histogram.h"

#include "nocam2map.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/nocam2map.xml").expanded());

TEST_F(MroHiriseCube, FunctionalTestNocam2mapDefault) {

  QString outCubeFileName = tempDir.path() + "/outTEMP.cub";
  QString lonFile = tempDir.path() + "/lons.cub";
  QString latFile = tempDir.path() + "/lats.cub";

  Cube *latCube = new Cube();
  latCube->setDimensions(testCube->sampleCount(), testCube->lineCount(), 1);
  latCube->create(latFile);

  LineManager latline(*latCube);
  double latValue = -60;
  for(latline.begin(); !latline.end(); latline++) {
    for(int i = 0; i < latline.size(); i++) {
      latValue += 0.0001;
      latline[i] = (double) latValue ;
    }
    latCube->write(latline);
  }
  latCube->close();

  Cube *lonCube = new Cube();
  lonCube->setDimensions(testCube->sampleCount(), testCube->lineCount(), 1);
  lonCube->create(lonFile);

  LineManager lonline(*lonCube);
  double lonValue = 1;
  for(lonline.begin(); !lonline.end(); lonline++) {
    for(int i = 0; i < lonline.size(); i++) {
      lonValue += 0.0001;
      lonline[i] = (double) lonValue;
    }
    lonCube->write(lonline);
  }
  lonCube->close();

  QVector<QString> args = {"latcub="+latFile, "lonCub="+lonFile, "to="+outCubeFileName};
  UserInterface options(APP_XML, args);

  try {
    nocam2map(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to project image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString());

  PvlGroup &mapping = oCube.label()->findObject("IsisCube").findGroup("Mapping");
  EXPECT_NEAR((double)mapping.findKeyword("MinimumLatitude"), -60, 0.01);
  EXPECT_NEAR((double)mapping.findKeyword("MaximumLatitude"), 67.142402648925994, 0.01);
  EXPECT_NEAR((double)mapping.findKeyword("MinimumLongitude"), 1.0, 0.01);
  EXPECT_NEAR((double)mapping.findKeyword("MaximumLongitude"), 128.14239501953, 0.01);

  std::unique_ptr<Histogram> hist (oCube.histogram());

  ASSERT_DOUBLE_EQ(hist->Average(), 572.05232892280969);
  ASSERT_DOUBLE_EQ(hist->Sum(),     44620.081655979156);
  ASSERT_EQ(hist->ValidPixels(),    78);
  ASSERT_DOUBLE_EQ(hist->StandardDeviation(), 367.54352065771224);
}


TEST_F(SmallCube, FunctionalTestNocam2mapSmall) {

  QString outCubeFileName = tempDir.path() + "/outTEMP.cub";
  QString lonFile = tempDir.path() + "/lons.cub";
  QString latFile = tempDir.path() + "/lats.cub";

  Cube *latCube = new Cube();
  latCube->setDimensions(testCube->sampleCount(), testCube->lineCount(), 1);
  latCube->create(latFile);

  LineManager latline(*latCube);
  double latValue = -60;
  for(latline.begin(); !latline.end(); latline++) {
    for(int i = 0; i < latline.size(); i++) {
      latValue += 0.0001;
      latline[i] = (double) latValue ;
    }
    latCube->write(latline);
  }
  latCube->close();

  Cube *lonCube = new Cube();
  lonCube->setDimensions(testCube->sampleCount(), testCube->lineCount(), 1);
  lonCube->create(lonFile);

  LineManager lonline(*lonCube);
  double lonValue = 1;
  for(lonline.begin(); !lonline.end(); lonline++) {
    for(int i = 0; i < lonline.size(); i++) {
      lonValue += 0.0001;
      lonline[i] = (double) lonValue;
    }
    lonCube->write(lonline);
  }
  lonCube->close();

  QVector<QString> args = {"latcub="+latFile, "lonCub="+lonFile, "to="+outCubeFileName, "target=Mars"};
  UserInterface options(APP_XML, args);

  try {
    nocam2map(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to project image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString());

  PvlGroup &mapping = oCube.label()->findObject("IsisCube").findGroup("Mapping");
  EXPECT_NEAR((double)mapping.findKeyword("MinimumLatitude"), -60, 0.01);
  EXPECT_NEAR((double)mapping.findKeyword("MaximumLatitude"), -59.990001678467003, 0.01);
  EXPECT_NEAR((double)mapping.findKeyword("MinimumLongitude"), 1.0, 0.01);
  EXPECT_NEAR((double)mapping.findKeyword("MaximumLongitude"), 1.009999990, 0.01);
}
