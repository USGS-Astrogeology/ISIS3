#include <QTemporaryDir>

#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"

#include "lrowaccal.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/lrowaccal.xml").expanded();

TEST(Lrowaccal, FunctionalTestLrowaccalRadianceUnitsLabelExists) {
  QTemporaryDir tempDir;
  ASSERT_TRUE(tempDir.isValid());

  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QString testCubeFileName = "data/lrowaccal/M1388981421CE.tmp.vis.even.reduced.cub";

  QVector<QString> args = {"from=" + testCubeFileName,
                           "to=" + outCubeFileName,
                           "radiometrictype=Radiance",
                           "radiometricfile=Default"};
  UserInterface options(APP_XML, args);

  try {
    lrowaccal(options);
  }
  catch(IException &e) {
    FAIL() << "Call to lrowaccal failed, unable to calibrate cube: " << e.what() << std::endl;
  }

  Cube outCube(outCubeFileName);

  ASSERT_TRUE(outCube.hasGroup("Radiometry"));
  PvlGroup &radiometry = outCube.group("Radiometry");

  ASSERT_TRUE(radiometry.hasKeyword("RadiometricType"));
  PvlKeyword &radiometricType = radiometry["RadiometricType"];
  ASSERT_EQ(radiometricType.unit(), "W/m2/sr/um");
}

TEST(Lrowaccal, FunctionalTestLrowaccalRadianceUnitsLabelNotForIOF) {
  QTemporaryDir tempDir;
  ASSERT_TRUE(tempDir.isValid());

  QString outCubeFileName = tempDir.path() + "/outTemp.cub";
  QString testCubeFileName = "data/lrowaccal/M1388981421CE.tmp.vis.even.reduced.cub";

  QVector<QString> args = {"from=" + testCubeFileName,
                           "to=" + outCubeFileName,
                           "radiometrictype=IOF",
                           "radiometricfile=Default"};
  UserInterface options(APP_XML, args);

  try {
    lrowaccal(options);
  }
  catch(IException &e) {
    FAIL() << "Call to lrowaccal failed, unable to calibrate cube: " << e.what() << std::endl;
  }

  Cube outCube(outCubeFileName);

  ASSERT_TRUE(outCube.hasGroup("Radiometry"));
  PvlGroup &radiometry = outCube.group("Radiometry");

  ASSERT_TRUE(radiometry.hasKeyword("RadiometricType"));
  PvlKeyword &radiometricType = radiometry["RadiometricType"];
  ASSERT_NE(radiometricType.unit(), "W/m2/sr/um");
}
