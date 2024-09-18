#include <iostream>

#include "Cube.h"
#include "CameraFixtures.h"
#include "Histogram.h"
#include "ImageHistogram.h"

#include "gaussstretch.h"
#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/gaussstretch.xml").expanded());

TEST_F(DefaultCube, FunctionalTestGaussstretch) {
  QString outputCubePath = tempDir.path() + "/tempGaussStretchOut.cub";
  double gsigma = 3.0;
  QVector<QString> args = {"from=" + testCube->fileName(), "to=" + outputCubePath, "gsigma=" + QString::number(gsigma)};
  UserInterface options(APP_XML, args);
  try {
    gaussstretch(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }
  Cube outputCube(outputCubePath.toStdString());

  EXPECT_FLOAT_EQ(outputCube.histogram()->Median(), -1.79769e+308);
}
