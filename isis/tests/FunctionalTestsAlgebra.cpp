#include "algebra.h"
#include "Brick.h"
#include "CameraFixtures.h"
#include "Histogram.h"
#include "LineManager.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/algebra.xml").expanded();

/**
   * class AlgebraCube
   * 
   * Reimplements DefaultCube::resizeCube method to force the use
   * of Real dns to properly test propagation of special pixels
   * to the output cube.
   */
class AlgebraCube : public DefaultCube {
  protected:

    void resizeCube(int samples, int lines, int bands) {
      label = Pvl();
      PvlObject &isisCube = testCube->label()->findObject("IsisCube");
      label.addObject(isisCube);

      PvlGroup &dim = label.findObject("IsisCube").findObject("Core").findGroup("Dimensions");
      dim.findKeyword("Samples").setValue(QString::number(samples));
      dim.findKeyword("Lines").setValue(QString::number(lines));
      dim.findKeyword("Bands").setValue(QString::number(bands));

      // force Real dns
      PvlObject &core = label.findObject("IsisCube").findObject("Core");
      PvlGroup &pixels = core.findGroup("Pixels");
      pixels.findKeyword("Type").setValue("Real");

      delete testCube;
      testCube = new Cube();
      testCube->fromIsd(tempDir.path() + "/default.cub", label, isd, "rw");

      LineManager line(*testCube);
      int pixelValue = 1;
      for(int band = 1; band <= bands; band++) {
        for (int i = 1; i <= testCube->lineCount(); i++) {
          line.SetLine(i, band);
          for (int j = 0; j < line.size(); j++) {
            line[j] = (double) (pixelValue % 255);
            pixelValue++;
          }
          testCube->write(line);
        }
      }

      // set 1st three diagonal pixels of band 1 to special pixels Null, Lrs, and Lis
      // set last two diagonal pixels of band 2 to special pixels Hrs and His

      Brick b(1, 1, 1, testCube->pixelType()); // create buffer of size 1 pixel

      b.SetBasePosition(1, 1, 1);
      b[0] = Isis::Null;
      testCube->write(b);  

      b.SetBasePosition(2, 2, 1);
      b[0] = Isis::Lrs;
      testCube->write(b);  

      b.SetBasePosition(3, 3, 1);
      b[0] = Isis::Lis;
      testCube->write(b);  

      b.SetBasePosition(4, 4, 2);
      b[0] = Isis::Hrs;
      testCube->write(b);  

      b.SetBasePosition(5, 5, 2);
      b[0] = Isis::His;
      testCube->write(b); 
    }
};


/**
   * FunctionalTestAlgebraAdd
   * 
   * Pixel by pixel addition of bands 1 and 2 of input cube.
   * 
   * INPUT: testCube from DefaultCube fixture with 2 bands.
   *         a=1 (multiplicative constant for 1st input cube)
   *         b=1 (multiplicative constant for 2nd input cube)
   *         c=0 (additive constant for entire equation)
   *         d=0 (additive constant for 1st input cube)
   *         e=0 (additive constant for 2nd input cube)
   * 
   *        Band 1                        Band 2
   * 
   * | N | 2 | 3 | 4 | 5 |         | 26| 27| 28| 29| 30|
   * | 6 |Lrs| 8 | 9 | 10|         | 31| 32| 33| 34| 35|
   * | 11| 12|Lis| 14| 15|         | 36| 37| 38| 39| 40|
   * | 16| 17| 18| 19| 20|         | 41| 42| 43|Hrs| 45|
   * | 21| 22| 23| 24| 25|         | 46| 47| 48| 49|His|
   * 
   * OUTPUT: algebraAddOut.cub
   * 
   * | N | 29| 31| 33| 35|
   * | 37|Lrs| 41| 43| 45|
   * | 47| 49|Lis| 53| 55|
   * | 57| 59| 61| N | 65|
   * | 67| 69| 71| 73| N |
   */
TEST_F(AlgebraCube, FunctionalTestAlgebraAdd) {

  // reduce test cube size, create two bands
  resizeCube(5, 5, 2);

  // close and reopen test cube to ensure dn buffer is available
  testCube->reopen("r");

  // run algebra
  QVector<QString> args = {"from="  + testCube->fileName() + "+1",
                           "from2=" + testCube->fileName() + "+2",
                           "to=" + tempDir.path() + "/algebraAddOut.cub",
                           "operator=add",
                           "a=1",
                           "b=1",
                           "c=0"};
  UserInterface ui(APP_XML, args);

  try {
     algebra(testCube, ui, testCube);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  // Open output cube
  Cube outCube(tempDir.path() + "/algebraAddOut.cub");

  // validate histogram statistics in output cube
  std::unique_ptr<Histogram> hist (outCube.histogram(1));

  EXPECT_EQ(hist->ValidPixels(), 20);
  EXPECT_EQ(hist->Average(), 51);
  EXPECT_EQ(hist->Sum(), 1020);

  // validate propagation of special pixels
  Brick b(1, 1, 1, outCube.pixelType()); // create buffer of size 1 pixel

  b.SetBasePosition(1, 1, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Null");

  b.SetBasePosition(2, 2, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Lrs");

  b.SetBasePosition(3, 3, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Lis");

  b.SetBasePosition(4, 4, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Null");

  b.SetBasePosition(5, 5, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Null");

  outCube.close();  
}


/**
   * FunctionalTestAlgebraSubtract
   * 
   * Pixel by pixel subtraction of band 1 from band 2 of input cube.
   * 
   * INPUT: testCube from DefaultCube fixture with 2 bands.
   *         a=1 (multiplicative constant for 1st input cube)
   *         b=1 (multiplicative constant for 2nd input cube)
   *         c=0 (additive constant for entire equation)
   *         d=0 (additive constant for 1st input cube)
   *         e=0 (additive constant for 2nd input cube)
   * 
   *        Band 1                        Band 2
   * 
   * | N | 2 | 3 | 4 | 5 |         | 26| 27| 28| 29| 30|
   * | 6 |Lrs| 8 | 9 | 10|         | 31| 32| 33| 34| 35|
   * | 11| 12|Lis| 14| 15|         | 36| 37| 38| 39| 40|
   * | 16| 17| 18| 19| 20|         | 41| 42| 43|Hrs| 45|
   * | 21| 22| 23| 24| 25|         | 46| 47| 48| 49|His|
   * 
   * OUTPUT: algebraSubtractOut.cub
   * 
   * | N | 25| 25| 25| 25|
   * | 25|  N| 25| 25| 25|
   * | 25| 25|  N| 25| 25|
   * | 25| 25| 25|Hrs| 25|
   * | 25| 25| 25| 25|His|
   */
TEST_F(AlgebraCube, FunctionalTestAlgebraSubtract) {

  // reduce test cube size, create two bands
  resizeCube(5, 5, 2);

  // close and reopen test cube to ensure dn buffer is available
  testCube->reopen("r");

  // run algebra
  QVector<QString> args = {"from="  + testCube->fileName() + "+2",
                           "from2=" + testCube->fileName() + "+1",
                           "to=" + tempDir.path() + "/algebraSubtractOut.cub",
                           "operator=subtract",
                           "a=1",
                           "b=1",
                           "c=0"};
  UserInterface ui(APP_XML, args);

  try {
     algebra(testCube, ui, testCube);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  // Open output cube
  Cube outCube(tempDir.path() + "/algebraSubtractOut.cub");

  // validate histogram statistics in output cube
  std::unique_ptr<Histogram> hist (outCube.histogram(1));

  EXPECT_EQ(hist->ValidPixels(), 20);
  EXPECT_EQ(hist->Average(), 25);
  EXPECT_EQ(hist->Sum(), 500);

  // validate propagation of special pixels
  Brick b(1, 1, 1, outCube.pixelType()); // create buffer of size 1 pixel

  b.SetBasePosition(1, 1, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Null");

  b.SetBasePosition(2, 2, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Null");

  b.SetBasePosition(3, 3, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Null");

  b.SetBasePosition(4, 4, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Hrs");

  b.SetBasePosition(5, 5, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "His");

  outCube.close();  
}


/**
   * FunctionalTestAlgebraMultiply
   * 
   * Pixel by pixel multiplication of band 1 from band 2 of input cube.
   * 
   * INPUT: testCube from DefaultCube fixture with 2 bands.
   *         a=1 (multiplicative constant for 1st input cube)
   *         b=1 (multiplicative constant for 2nd input cube)
   *         c=0 (additive constant for entire equation)
   *         d=0 (additive constant for 1st input cube)
   *         e=0 (additive constant for 2nd input cube)
   * 
   *        Band 1                        Band 2
   * 
   * | N | 2 | 3 | 4 | 5 |         | 26| 27| 28| 29| 30|
   * | 6 |Lrs| 8 | 9 | 10|         | 31| 32| 33| 34| 35|
   * | 11| 12|Lis| 14| 15|         | 36| 37| 38| 39| 40|
   * | 16| 17| 18| 19| 20|         | 41| 42| 43|Hrs| 45|
   * | 21| 22| 23| 24| 25|         | 46| 47| 48| 49|His|
   * 
   * OUTPUT: algebraMultiplyOut.cub
   * 
   * |   N|  54|  84| 116| 150|
   * | 186| Lrs| 264| 306| 350|
   * | 396| 444| Lis| 546| 600|
   * | 656| 714| 774|   N| 900|
   * | 966|1034|1104|1176|   N|
   */
TEST_F(AlgebraCube, FunctionalTestAlgebraMultiply) {

  // reduce test cube size, create two bands
  resizeCube(5, 5, 2);

  // close and reopen test cube to ensure dn buffer is available
  testCube->reopen("r");

  // run algebra
  QVector<QString> args = {"from="  + testCube->fileName() + "+1",
                           "from2=" + testCube->fileName() + "+2",
                           "to=" + tempDir.path() + "/algebraMultiplyOut.cub",
                           "operator=multiply",
                           "a=1",
                           "b=1",
                           "c=0"};
  UserInterface ui(APP_XML, args);

  try {
     algebra(testCube, ui, testCube);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  // Open output cube
  Cube outCube(tempDir.path() + "/algebraMultiplyOut.cub");

  // validate histogram statistics in output cube
  std::unique_ptr<Histogram> hist (outCube.histogram(1));

  EXPECT_EQ(hist->ValidPixels(), 20);
  EXPECT_EQ(hist->Average(), 541);
  EXPECT_EQ(hist->Sum(), 10820);

  // validate propagation of special pixels
  Brick b(1, 1, 1, outCube.pixelType()); // create buffer of size 1 pixel

  b.SetBasePosition(1, 1, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Null");

  b.SetBasePosition(2, 2, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Lrs");

  b.SetBasePosition(3, 3, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Lis");

  b.SetBasePosition(4, 4, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Null");

  b.SetBasePosition(5, 5, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Null");

  outCube.close();  
}


/**
   * FunctionalTestAlgebraDivide
   * 
   * Pixel by pixel division band 1 / band 2 of input cube.
   * 
   * INPUT: testCube from DefaultCube fixture with 2 bands.
   *         a=1 (multiplicative constant for 1st input cube)
   *         b=1 (multiplicative constant for 2nd input cube)
   *         c=0 (additive constant for entire equation)
   *         d=0 (additive constant for 1st input cube)
   *         e=0 (additive constant for 2nd input cube)
   * 
   *        Band 1                        Band 2
   * 
   * | N | 2 | 3 | 4 | 5 |         | 26| 27| 28| 29| 30|
   * | 6 |Lrs| 8 | 9 | 10|         | 31| 32| 33| 34| 35|
   * | 11| 12|Lis| 14| 15|         | 36| 37| 38| 39| 40|
   * | 16| 17| 18| 19| 20|         | 41| 42| 43|Hrs| 45|
   * | 21| 22| 23| 24| 25|         | 46| 47| 48| 49|His|
   * 
   * OUTPUT: algebraDivideOut.cub
   * 
   * |   N|.074|.107|.137|.166|    Values shown truncated at 3 decimal places
   * |.193| Lrs|.242|.264|.285|    
   * |.305|.324| Lis|.358|.375|
   * |.390|.404|.418|   N|.444|
   * |.456|.468|.479|.489|   N|
   */
  TEST_F(AlgebraCube, FunctionalTestAlgebraDivide) {

  // reduce test cube size, create two bands
  resizeCube(5, 5, 2);

  // close and reopen test cube to ensure dn buffer is available
  testCube->reopen("r");

  // run algebra
  QVector<QString> args = {"from="  + testCube->fileName() + "+1",
                           "from2=" + testCube->fileName() + "+2",
                           "to=" + tempDir.path() + "/algebraDivideOut.cub",
                           "operator=divide",
                           "a=1",
                           "b=1",
                           "c=0"};
  UserInterface ui(APP_XML, args);

  try {
     algebra(testCube, ui, testCube);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  // Open output cube
  Cube outCube(tempDir.path() + "/algebraDivideOut.cub");

  // validate histogram statistics in output cube
  std::unique_ptr<Histogram> hist (outCube.histogram(1));

  EXPECT_EQ(hist->ValidPixels(), 20);
  EXPECT_NEAR(hist->Average(), 0.319384, 0.000001);
  EXPECT_NEAR(hist->Sum(), 6.387686, 0.000001);

  // validate propagation of special pixels
  Brick b(1, 1, 1, outCube.pixelType()); // create buffer of size 1 pixel

  b.SetBasePosition(1, 1, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Null");

  b.SetBasePosition(2, 2, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Lrs");

  b.SetBasePosition(3, 3, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Lis");

  b.SetBasePosition(4, 4, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Null");

  b.SetBasePosition(5, 5, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Null");

  outCube.close();  
}


/**
   * FunctionalTestAlgebraUnary
   * 
   * Unary processing of band 1 (a * from1) + c).
   * 
   * INPUT: testCube from DefaultCube fixture with 2 bands.
   *         a=1 (multiplicative constant for 1st input cube)
   *         b=1 (multiplicative constant for 2nd input cube)
   *         c=0 (additive constant for entire equation)
   *         d=0 (additive constant for 1st input cube)
   *         e=0 (additive constant for 2nd input cube)
   * 
   *        Band 1
   * 
   * | N | 2 | 3 | 4 | 5 |
   * | 6 |Lrs| 8 | 9 | 10|
   * | 11| 12|Lis| 14| 15|
   * | 16| 17| 18| 19| 20|
   * | 21| 22| 23| 24| 25|
   * 
   * OUTPUT: algebraUnaryOut.cub (identical to input band 1)
   * 
   * | N | 2 | 3 | 4 | 5 |
   * | 6 |Lrs| 8 | 9 | 10|
   * | 11| 12|Lis| 14| 15|
   * | 16| 17| 18| 19| 20|
   * | 21| 22| 23| 24| 25|
   */
  TEST_F(AlgebraCube, FunctionalTestAlgebraUnary) {

  // reduce test cube size, create two bands
  resizeCube(5, 5, 1);

  // close and reopen test cube to ensure dn buffer is available
  testCube->reopen("r");

  // run algebra
  QVector<QString> args = {"from="  + testCube->fileName() + "+1",
                           "to=" + tempDir.path() + "/algebraUnaryOut.cub",
                           "operator=unary",
                           "a=1",
                           "c=0"};
  UserInterface ui(APP_XML, args);

  try {
     algebra(testCube, ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  // Open output cube
  Cube outCube(tempDir.path() + "/algebraUnaryOut.cub");

  // validate histogram statistics in output cube
  std::unique_ptr<Histogram> hist (outCube.histogram(1));

  EXPECT_EQ(hist->ValidPixels(), 22);
  EXPECT_NEAR(hist->Average(), 13.818181, 0.000001);
  EXPECT_EQ(hist->Sum(), 304);

  // validate propagation of special pixels
  Brick b(1, 1, 1, outCube.pixelType()); // create buffer of size 1 pixel

  b.SetBasePosition(1, 1, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Null");

  b.SetBasePosition(2, 2, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Lrs");

  b.SetBasePosition(3, 3, 1);
  outCube.read(b);
  EXPECT_EQ(PixelToString(b[0]), "Lis");

  outCube.close();  
}


