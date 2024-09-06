#include "Affine.h"
#include "IException.h"
#include "TestUtilities.h"

#include <gtest/gtest.h>
#include <QString>

TEST(Affine, DefaultConstructor) {
  Isis::Affine affine;
  EXPECT_EQ(affine.xp(), 0);
  EXPECT_EQ(affine.yp(), 0);
  EXPECT_EQ(affine.x(), 0);
  EXPECT_EQ(affine.y(), 0);
}


TEST(Affine, MatrixConstructor) {
  Isis::Affine::AMatrix matrix(3, 3, 0.0);
  matrix[0][0] = 1.0;
  matrix[1][1] = 1.0;
  matrix[2][2] = 1.0;
  Isis::Affine affine(matrix);
  EXPECT_EQ(affine.xp(), 0);
  EXPECT_EQ(affine.yp(), 0);
  EXPECT_EQ(affine.x(), 0);
  EXPECT_EQ(affine.y(), 0);
}


TEST(Affine, IncorrectDimensions) {
  std::string message = "Affine matrices must be 3x3";
  try {
    Isis::Affine::AMatrix matrix(2, 2, 0.0);
    matrix[0][0] = 1.0;
    matrix[1][1] = 1.0;
    Isis::Affine affine(matrix);
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected error message: \"" << message.toStdString() << "\"";
  }
}


TEST(Affine, NotInvertible) {
  std::string message = "Affine transform not invertible";
  try {
    Isis::Affine::AMatrix matrix(3, 3, 1.0);
    Isis::Affine affine(matrix);
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected error message: \"" << message.toStdString() << "\"";
  }
}


TEST(Affine, Identity) {
  Isis::Affine affine;
  Isis::Affine::AMatrix identity = affine.getIdentity();
  EXPECT_EQ(identity[0][0], 1);
  EXPECT_EQ(identity[0][1], 0);
  EXPECT_EQ(identity[0][2], 0);
  EXPECT_EQ(identity[1][0], 0);
  EXPECT_EQ(identity[1][1], 1);
  EXPECT_EQ(identity[1][2], 0);
  EXPECT_EQ(identity[2][0], 0);
  EXPECT_EQ(identity[2][1], 0);
  EXPECT_EQ(identity[2][2], 1);
}


// Solve (1,1)->(3,3) (3,3)->(1,1), (1,3)->(3,1)
TEST(Affine, Solve) {
  Isis::Affine affine;
  double x[] = { 1.0, 3.0, 1.0 };
  double y[] = { 1.0, 3.0, 3.0 };
  double xp[] = { 3.0, 1.0, 3.0 };
  double yp[] = { 3.0, 1.0, 1.0 };
  affine.Solve(x, y, xp, yp, 3);
  
  affine.Compute(1.0, 1.0);
  EXPECT_NEAR(affine.xp(), 3.0, .00000000001);
  EXPECT_NEAR(affine.yp(), 3.0, .00000000001);

  affine.Compute(3.0, 3.0);
  EXPECT_NEAR(affine.xp(), 1.0, .00000000001);
  EXPECT_NEAR(affine.yp(), 1.0, .00000000001);

  affine.Compute(1.0, 3.0);
  EXPECT_NEAR(affine.xp(), 3.0, .00000000001);
  EXPECT_NEAR(affine.yp(), 1.0, .00000000001);

  affine.Compute(3.0, 1.0);
  EXPECT_NEAR(affine.xp(), 1.0, .00000000001);
  EXPECT_NEAR(affine.yp(), 3.0, .00000000001);
}


TEST(Affine, Compute) {
  Isis::Affine affine;
  affine.Compute(1.0, 1.0);
  EXPECT_DOUBLE_EQ(affine.xp(), 1.0);
  EXPECT_DOUBLE_EQ(affine.yp(), 1.0);
  affine.ComputeInverse(affine.xp(), affine.yp());
  EXPECT_DOUBLE_EQ(affine.x(), 1.0);
  EXPECT_DOUBLE_EQ(affine.y(), 1.0);
}


TEST(Affine, Translate) {
  Isis::Affine affine;
  affine.Translate(1.0, -1.0);
  affine.Compute(0.0, 0.0);
  EXPECT_DOUBLE_EQ(affine.xp(), 1.0);
  EXPECT_DOUBLE_EQ(affine.yp(), -1.0);
}


TEST(Affine, Rotate) {
  Isis::Affine affine;
  affine.Rotate(90.0);
  affine.Compute(1.0, 1.0);
  EXPECT_DOUBLE_EQ(affine.xp(), -1.0);
  EXPECT_DOUBLE_EQ(affine.yp(), 1.0);
}


TEST(Affine, Scale) {
  Isis::Affine affine;
  affine.Scale(2.0);
  affine.Compute(1.0, 1.0);
  EXPECT_DOUBLE_EQ(affine.xp(), 2.0);
  EXPECT_DOUBLE_EQ(affine.yp(), 2.0);
}


TEST(Affine, Coefficients) {
  Isis::Affine affine;
  std::vector<double> xcoef = affine.Coefficients(1);
  EXPECT_DOUBLE_EQ(xcoef[0], 1.0);
  EXPECT_DOUBLE_EQ(xcoef[1], 0);
  EXPECT_DOUBLE_EQ(xcoef[2], 0);
  std::vector<double> ycoef = affine.Coefficients(2);
  EXPECT_DOUBLE_EQ(ycoef[0], 0);
  EXPECT_DOUBLE_EQ(ycoef[1], 1.0);
  EXPECT_DOUBLE_EQ(ycoef[2], 0);
}


TEST(Affine, InverseCoefficients) {
  Isis::Affine::AMatrix matrix(3, 3, 0.0);
  matrix[0][0] = 2.0;
  matrix[1][1] = 1.0;
  matrix[2][2] = 1.0;
  Isis::Affine affine(matrix);  
  std::vector<double> xcoef = affine.InverseCoefficients(1);
  EXPECT_DOUBLE_EQ(xcoef[0], 1.0/2.0);
  EXPECT_DOUBLE_EQ(xcoef[1], 0);
  EXPECT_DOUBLE_EQ(xcoef[2], 0);
  std::vector<double> ycoef = affine.InverseCoefficients(2);
  EXPECT_DOUBLE_EQ(ycoef[0], 0);
  EXPECT_DOUBLE_EQ(ycoef[1], 1.0);
  EXPECT_DOUBLE_EQ(ycoef[2], 0);
}