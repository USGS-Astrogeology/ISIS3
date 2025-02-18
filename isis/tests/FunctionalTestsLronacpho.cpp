#include <QTemporaryDir>
#include "LineManager.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "IEndian.h"
#include "PixelType.h"
#include "lronacpho.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace Isis;
using namespace testing;

/**
  *
  * @brief Photometric application for the LRO NAC cameras
  *
  * This application provides features that allow multiband cubes for LRO NAC cameras
  *   to be photometrically corrected
  *
  * @author 2016-09-16 Victor Silva
  *
  * @internal
  *   @history 2022-04-26 Victor Silva - Original Version - Functional test is against known value for input
  *                                      cub since fx is not yet callable
  */

static QString APP_XML = FileName("$ISISROOT/bin/xml/lronacpho.xml").expanded();

TEST(LronacphoNoDemAlgo3, FunctionalTestsLronacpho) {
  QTemporaryDir prefix;

  ASSERT_TRUE(prefix.isValid());

  QString inCubeFileName = "data/lronacpho/M143947267L.cal.echo.crop.cub";
  QString outCubeFileName = prefix.path() + "/output.cub";
  QString phoParameterFileName = "data/lronacpho/NAC_PHO_LROC_Empirical.0003.pvl";
  QVector<QString> args = {"from="+inCubeFileName, "to="+outCubeFileName, "PHOPAR="+phoParameterFileName, "usedem=no"};

  UserInterface options(APP_XML, args);
  try {
    lronacpho(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to apply photometry algorithm to LRO image: " <<e.toString().toStdString().c_str() << std::endl;
  }
  try{
    // open output cube to test al algorithm was applied correctly
    Cube outputCube;
    outputCube.open(outCubeFileName, "r");
    /* a better test would be to compare output using fx but since fx isn't callable yet, we're doing this via running fx outside of this program to get corrected DN value for pixel at (500,500)
     fx result = 0.271452963352
     fx equation:
     fx="\( f1 ) * ( ( cos(inal(f1) * pi / 180) + cos(emal(f1) * pi / 180) ) / cos(inal(f1) * pi / 180) / ( e ^ ( -1.479654495 -0.000083528 * phal(f1)^2 + 0.012964707 * phal(f1)  -0.237774774 * phal(f1) ^ (1/2) + 0.556075496 * ( cos(emal(f1)*pi/180) ) +   0.663671460 * ( cos(inal(f1) * pi / 180) ) -0.439918609 * ( cos(inal(f1) * pi / 180) )^2  ))) * 0.087598" f1=input/M143947267L.cal.echo.crop.cub to=output/output2.cub;
    */
    double algo3Result = 0.26584613;

    // dop! lineManager is not zero based
    LineManager oLine(outputCube);
    oLine.SetLine(300);
    outputCube.read(oLine);

    EXPECT_NEAR(oLine[300],algo3Result, 0.002);
    outputCube.close();
  }
  catch(IException &e){
    FAIL() << "Unable to open output cube: " <<e.toString().toStdString().c_str() << std::endl;
  }
}

TEST(LronacphoNoDemAlgo2, FunctionalTestsLronacpho) {
  QTemporaryDir prefix;

  ASSERT_TRUE(prefix.isValid());

  QString inCubeFileName = "data/lronacpho/M143947267L.cal.echo.crop.cub";
  QString outCubeFileName = prefix.path() + "/output.cub";
  QString phoParameterFileName = "data/lronacpho/NAC_PHO_LROC_Empirical.0002.pvl";
  QVector<QString> args = {"from="+inCubeFileName, "to="+outCubeFileName, "PHOPAR="+phoParameterFileName, "usedem=no"};

  UserInterface options(APP_XML, args);
  try {
    lronacpho(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to apply photometry algorithm to LRO image: " <<e.toString().toStdString().c_str() << std::endl;
  }
  try{
    // open output cube to test al algorithm was applied correctly
    Cube outputCube;
    outputCube.open(outCubeFileName, "r");

    double algo2Result = 0.28940132;

    // dop! lineManager is not zero based
    LineManager oLine(outputCube);
    oLine.SetLine(300);
    outputCube.read(oLine);

    EXPECT_NEAR(oLine[300],algo2Result, 0.002);

    outputCube.close();
  }
  catch(IException &e){
    FAIL() << "Unable to open output cube: " <<e.toString().toStdString().c_str() << std::endl;
  }
}

TEST(LronacphoNoDemAlgoDefault, FunctionalTestsLronacpho) {
  QTemporaryDir prefix;

  ASSERT_TRUE(prefix.isValid());

  QString inCubeFileName = "data/lronacpho/M143947267L.cal.echo.crop.cub";
  QString outCubeFileName = prefix.path() + "/output.cub";
  QString phoParameterFileName = "data/lronacpho/NAC_PHO_LROC_Empirical.0003.pvl";
  QVector<QString> args = {"from="+inCubeFileName, "to="+outCubeFileName, "usedem=no"};

  UserInterface options(APP_XML, args);
  try {
    lronacpho(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to apply photometry algorithm to LRO image: " <<e.toString().toStdString().c_str() << std::endl;
  }
  try{
    // open output cube to test al algorithm was applied correctly
    Cube outputCube;

    outputCube.open(outCubeFileName, "r");

    double algo3Result = 0.26584613;

    LineManager oLine(outputCube);
    oLine.SetLine(300);
    outputCube.read(oLine);

    EXPECT_NEAR(oLine[300],algo3Result, 0.002);
    outputCube.close();
  }
  catch(IException &e){
    FAIL() << "Unable to open output cube: " <<e.toString().toStdString().c_str() << std::endl;
  }
}