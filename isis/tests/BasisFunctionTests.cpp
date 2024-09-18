#include <gtest/gtest.h>
#include <QString>
#include "BasisFunction.h"
#include "IException.h"

TEST(BasisFunction, Initialization)
{
  std::string name("basis1");
  Isis::BasisFunction testBasis("basis1", 3, 3);

  EXPECT_EQ(testBasis.Coefficients(), 3);
  EXPECT_EQ(testBasis.Variables(), 3);
  EXPECT_STREQ(testBasis.Name().toStdString().c_str(), name.c_str());
}

TEST(BasisFunction, Evaluation)
{
  Isis::BasisFunction testBasis("basis", 3, 3);
  double expectedOutput = 12.7;
  double output;
  std::vector <double> vars;
  std::vector <double> coefs;

  coefs.push_back(2.5);
  coefs.push_back(3.2);
  coefs.push_back(1.0);
  testBasis.SetCoefficients(coefs);

  vars.push_back(3.0);
  vars.push_back(1.0);
  vars.push_back(2.0);

  output = testBasis.Evaluate(vars);
  EXPECT_DOUBLE_EQ(output, expectedOutput);

  vars[0] = 3.5;
  vars[1] = 1.2;
  vars[2] = 10.8;

  expectedOutput = 23.39;
  output = testBasis.Evaluate(vars);
  EXPECT_DOUBLE_EQ(output, expectedOutput);
}

TEST(BasisFunction, InequalCoefficientAmount)
{
  Isis:: BasisFunction testBasis("basis", 1, 1);
  std::vector <double> coefs;

  coefs.push_back(1.0);
  coefs.push_back(1.0);

  try
  {
    testBasis.SetCoefficients(coefs);
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().find("Unable to set coefficients vector") != std::string::npos)
      <<  e.toString();
  }
  catch(...)
  {
    FAIL() << "Expected an IException with message: "
      "\"Unable to set coefficients vector. The size of the given vector "
                  "does not match number of coefficients "
                  "in the basis equation\"";
  }

}

TEST(BasisFunction, InequalVariableAmount)
{
  Isis:: BasisFunction testBasis("basis", 1, 1);
  std::vector <double> coefs;
  std::vector <double> vars;

  coefs.push_back(1.0);
  testBasis.SetCoefficients(coefs);

  vars.push_back(1.0);
  vars.push_back(1.0);

  try
  {
    testBasis.Evaluate(vars);
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().find("Unable to evaluate function") != std::string::npos)
      <<  e.toString();
  }
  catch(...)
  {
    FAIL() << "Expected an IException with message: "
              "\"Unable to set coefficients vector. The size of the given vector"
              " does not match number of coefficients in the basis equation\"";
  }
}

TEST(BasisFunction, ExtraCoefficients)
{
  Isis::BasisFunction testBasis("basis", 1, 2);
  std::vector <double> coefs;
  std::vector <double> vars;

  coefs.push_back(1.0);
  coefs.push_back(1.0);
  testBasis.SetCoefficients(coefs);

  vars.push_back(1.0);
  try
  {
    testBasis.Evaluate(vars);
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().find("Unable to evaluate function") != std::string::npos)
      <<  e.toString();
  }
  catch(...)
  {
    FAIL() << "Expected an IException with message: "
               "\"Unable to evaluate function for the given vector of values. "
               "The number of terms in the expansion does not "
               "match number of coefficients in the basis equation\"";
  }
}

TEST(BasisFunction, NonVectorVariable)
{
  Isis::BasisFunction testBasis("basis", 1, 1);
  double var = 1.2;
  double expectedOutput = 3.0;
  double output;
  std::vector <double> coefs;

  coefs.push_back(2.5);
  testBasis.SetCoefficients(coefs);
  output = testBasis.Evaluate(var);
  EXPECT_DOUBLE_EQ(output, expectedOutput);
}
