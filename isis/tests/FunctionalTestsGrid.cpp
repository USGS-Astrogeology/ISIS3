#include "grid.h"

#include <QTemporaryFile>
#include <QTextStream>
#include <QStringList>

#include "CubeFixtures.h"
#include "CameraFixtures.h"
#include "CubeFixtures.h"
#include "LineManager.h"
#include "PvlGroup.h"
#include "SpecialPixel.h"
#include "TestUtilities.h"

#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/grid.xml").expanded());

TEST_F(DefaultCube, FunctionalTestGridGround) {
  QVector<QString> args = {"to=" + tempDir.path() + "/output.cub"};
  UserInterface options(APP_XML, args);
  grid(testCube, options);

  Cube outputCube;
  try {
    outputCube.open(tempDir.path() + "/output.cub", "r");
  }
  catch (IException &e) {
    FAIL() << "Unable to open output image: " << e.what() << std::endl;
  }

  // Check beginning and end of gridline
  LineManager line(outputCube);
  line.SetLine(579);
  outputCube.read(line);
  EXPECT_EQ(line[0], Isis::Hrs);

  line.SetLine(1056);
  outputCube.read(line);
  EXPECT_EQ(line[247], Isis::Hrs);

  outputCube.close();
}

TEST_F(SmallCube, FunctionalTestGridImage) {
  // The default linc and sinc are 10 and our image size is 10x10, so make linc and sinc smaller
  // than 10 to see the grid.
  QVector<QString> args = {"to=" + tempDir.path() + "/output.cub", "mode=image", "linc=5", "sinc=5"};
  UserInterface options(APP_XML, args);
  grid(testCube, options);

  Cube outputCube;
  try {
    outputCube.open(tempDir.path() + "/output.cub", "r");
  }
  catch (IException &e) {
    FAIL() << "Unable to open output image: " << e.what() << std::endl;
  }

  LineManager line(outputCube);
  double pixelValue = 0.0;
  for (int i = 1; i <= outputCube.lineCount(); i++) { // 1 based
    line.SetLine(i);
    outputCube.read(line);

    for (int j = 0; j < line.size(); j++) { // 0 based
      if (i % 5 == 1 || j % 5 == 0) {
        EXPECT_EQ(line[j], Isis::Hrs);
      }
      else {
        EXPECT_DOUBLE_EQ(line[j], pixelValue);
      }
      pixelValue++;
    }
  }
  outputCube.close();
}

TEST_F(SmallCube, FunctionalTestGridHrsLrs) {
  QVector<QString> args = {"to=" + tempDir.path() + "/output.cub", "mode=image", "linc=5", "sinc=5", "bkgndvalue=hrs", "linevalue=lrs"};
  UserInterface options(APP_XML, args);
  grid(testCube, options);

  Cube outputCube;
  try {
    outputCube.open(tempDir.path() + "/output.cub", "r");
  }
  catch (IException &e) {
    FAIL() << "Unable to open output image: " << e.what() << std::endl;
  }

  LineManager line(outputCube);
  for (int i = 1; i <= outputCube.lineCount(); i++) {
    line.SetLine(i);
    outputCube.read(line);

    for (int j = 0; j < line.size(); j++) {
      if (i % 5 == 1 || j % 5 == 0 ) {
        EXPECT_EQ(line[j], Isis::Lrs);
      }
      else {
        EXPECT_EQ(line[j], Isis::Hrs);
      }
    }
  }
  outputCube.close();
}

TEST_F(SmallCube, FunctionalTestGridLrsNull) {
  QVector<QString> args = {"to=" + tempDir.path() + "/output.cub", "mode=image", "linc=5", "sinc=5", "bkgndvalue=lrs", "linevalue=null"};
  UserInterface options(APP_XML, args);
  grid(testCube, options);

  Cube outputCube;
  try {
    outputCube.open(tempDir.path() + "/output.cub", "r");
  }
  catch (IException &e) {
    FAIL() << "Unable to open output image: " << e.what() << std::endl;
  }

  LineManager line(outputCube);
  for (int i = 1; i <= outputCube.lineCount(); i++) {
    line.SetLine(i);
    outputCube.read(line);

    for (int j = 0; j < line.size(); j++) {
      if (i % 5 == 1 || j % 5 == 0) {
        EXPECT_EQ(line[j], Isis::Null);
      }
      else {
        EXPECT_EQ(line[j], Isis::Lrs);
      }
    }
  }
  outputCube.close();
}

TEST_F(SmallCube, FunctionalTestGridNullDn) {
  QVector<QString> args = {"to=" + tempDir.path() + "/output.cub", "mode=image", "linc=5", "sinc=5", "bkgndvalue=null", "linevalue=dn", "dnvalue=0"};
  UserInterface options(APP_XML, args);
  grid(testCube, options);

  Cube outputCube;
  try {
    outputCube.open(tempDir.path() + "/output.cub", "r");
  }
  catch (IException &e) {
    FAIL() << "Unable to open output image: " << e.what() << std::endl;
  }

  LineManager line(outputCube);
  for (int i = 1; i <= outputCube.lineCount(); i++) {
    line.SetLine(i);
    outputCube.read(line);

    for (int j = 0; j < line.size(); j++) {
      if (i % 5 == 1 || j % 5 == 0) {
        EXPECT_DOUBLE_EQ(line[j], 0.0);
      }
      else {
        EXPECT_EQ(line[j], Isis::Null);
      }
    }
  }
  outputCube.close();
}

TEST_F(SmallCube, FunctionalTestGridDnHrs) {
  QVector<QString> args = {"to=" + tempDir.path() + "/output.cub", "mode=image", "linc=5", "sinc=5", "bkgndvalue=DN", "bkgnddnvalue=0"};
  UserInterface options(APP_XML, args);
  grid(testCube, options);

  Cube outputCube;
  try {
    outputCube.open(tempDir.path() + "/output.cub", "r");
  }
  catch (IException &e) {
    FAIL() << "Unable to open output image: " << e.what() << std::endl;
  }

  LineManager line(outputCube);
  for (int i = 1; i <= outputCube.lineCount(); i++) {
    line.SetLine(i);
    outputCube.read(line);

    for (int j = 0; j < line.size(); j++) {
      if (i % 5 == 1 || j % 5 == 0) {
        EXPECT_EQ(line[j], Isis::Hrs);
      }
      else {
        EXPECT_DOUBLE_EQ(line[j], 0.0);
      }
    }
  }
  outputCube.close();
}

TEST_F(DefaultCube, FunctionalTestGridMosaic) {
  QVector<QString> args = {"to=" + tempDir.path() + "/output.cub"};
  UserInterface options(APP_XML, args);
  grid(projTestCube, options);

  Cube outputCube;
  try {
    outputCube.open(tempDir.path() + "/output.cub", "r");
  }
  catch (IException &e) {
    FAIL() << "Unable to open output image: " << e.what() << std::endl;
  }

  LineManager line(outputCube);
  for (int i = 1; i <= outputCube.lineCount(); i++) {
    line.SetLine(i);
    outputCube.read(line);

    for (int j = 0; j < line.size(); j++) {
      // Check that the grid lines are HRS, the other pixels are not HRS
      if (i == 1 || j == 0 || j == 5) {
        EXPECT_EQ(line[j], Isis::Hrs);
      }
      else {
        EXPECT_NE(line[j], Isis::Hrs);
      }
    }
  }
  outputCube.close();
}

TEST_F(NewHorizonsCube, FunctionalTestGridBandDependent) {
  setInstrument("-98901", "LEISA", "NEW HORIZONS");

  QVector<QString> args = {"to=" + tempDir.path() + "/output.cub", "loninc=2", "latinc=1", "baselat=0", "baselon=353"};
  UserInterface options(APP_XML, args);
  grid(testCube, options);

  Cube outputCube;
  try {
    outputCube.open(tempDir.path() + "/output.cub", "r");
  }
  catch (IException &e) {
    FAIL() << "Unable to open output image: " << e.what() << std::endl;
  }

   // Check beginning and end of vertical and horizontal grid lines of band one
  LineManager line(outputCube);
  line.SetLine(1);
  outputCube.read(line);
  EXPECT_EQ(line[2], Isis::Hrs);
  line.SetLine(2);
  outputCube.read(line);
  EXPECT_EQ(line[0], Isis::Hrs);

  line.SetLine(1);
  outputCube.read(line);
  EXPECT_EQ(line[9], Isis::Hrs);
  line.SetLine(9);
  outputCube.read(line);
  EXPECT_EQ(line[2], Isis::Hrs);

  outputCube.close();
}

TEST_F(DefaultCube, FunctionalTestGridExtend) {
  QVector<QString> args = {"to=" + tempDir.path() + "/output.cub", "extendgrid=true"};
  UserInterface options(APP_XML, args);

  // change mapping group of cube to one that extends past longitude domain
  Pvl newMap;
  newMap.read("data/defaultImage/extendProj.map");
  PvlGroup &newMapGrp = newMap.findGroup("Mapping", Pvl::Traverse);

  projTestCube->putGroup(newMapGrp);

  grid(projTestCube, options);

  Cube outputCube;
  try {
    outputCube.open(tempDir.path() + "/output.cub", "r");
  }
  catch (IException &e) {
    FAIL() << "Unable to open output image: " << e.what() << std::endl;
  }

  // Check beginning and end of gridline
  LineManager line(outputCube);
  line.SetLine(1);
  outputCube.read(line);
  EXPECT_EQ(line[0], Isis::Hrs);

  line.SetLine(1);
  outputCube.read(line);
  EXPECT_EQ(line[2], Isis::Hrs);

  outputCube.close();
}

// Tests setting the dnvalue to the maximum of the pixel type.
TEST_F(DefaultCube, FunctionalTestGrid8bit) {
  QVector<QString> args = {"to=" + tempDir.path() + "/output.cub", "outline=yes", "linewidth=3", "linevalue=dn", "dnvalue=255"};
  UserInterface options(APP_XML, args);
  grid(testCube, options);

  Cube outputCube;
  try {
    outputCube.open(tempDir.path() + "/output.cub", "r");
  }
  catch (IException &e) {
    FAIL() << "Unable to open output image: " << e.what() << std::endl;
  }

  // Check beginning and end of gridline
  LineManager line(outputCube);
  line.SetLine(579);
  outputCube.read(line);
  EXPECT_EQ(line[0], Isis::Hrs);

  line.SetLine(1056);
  outputCube.read(line);
  EXPECT_EQ(line[247], Isis::Hrs);

  outputCube.close();
}

// Tests that we can set the lat/lon to the min/max.
TEST_F(DefaultCube, FunctionalTestGridWorld) {
  PvlGroup &mapping = projTestCube->label()->findObject("IsisCube").findGroup("Mapping");
  mapping.findKeyword("MinimumLatitude").setValue("-90.0");
  mapping.findKeyword("MaximumLatitude").setValue("90.0");
  mapping.findKeyword("MinimumLongitude").setValue("0.0");
  mapping.findKeyword("MaximumLongitude").setValue("360.0");
  mapping.findKeyword("UpperLeftCornerY").setValue("5400000.0");
  QString fileName = projTestCube->fileName();
  LineManager line(*projTestCube);
  for(line.begin(); !line.end(); line++) {
    for(int i = 0; i < line.size(); i++) {
      line[i] = (double)(i + 1);
    }
    projTestCube->write(line);
  }
  projTestCube->reopen("rw");

  // need to remove old camera pointer
  delete projTestCube;

  // Cube now has new mapping group
  projTestCube = new Cube(fileName.toStdString(), "rw");

  QVector<QString> args = {"to=" + tempDir.path() + "/output.cub", "ticks=true", "diagonal=true", "loninc=45"};
  UserInterface options(APP_XML, args);
  grid(projTestCube, options);

  Cube outputCube;
  try {
    outputCube.open(tempDir.path() + "/output.cub", "r");
  }
  catch (IException &e) {
    FAIL() << "Unable to open output image: " << e.what() << std::endl;
  }

  line = LineManager(outputCube);
  for (int i = 1; i <= outputCube.lineCount(); i++) {
    line.SetLine(i);
    outputCube.read(line);

    for (int j = 0; j < line.size(); j++) {
      EXPECT_DOUBLE_EQ(line[j], j + 1);
    }
  }
  outputCube.close();
}