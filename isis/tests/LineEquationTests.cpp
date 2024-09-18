#include <gtest/gtest.h>
#include "LineEquation.h"
#include "IException.h"
#include "TestUtilities.h"

TEST(LineEquation, DefaultConstructor)
{
  Isis::LineEquation testEquation;

  EXPECT_FALSE(testEquation.Defined());
  EXPECT_FALSE(testEquation.HaveSlope());
  EXPECT_FALSE(testEquation.HaveIntercept());
  EXPECT_EQ(testEquation.Points(), 0);
}

TEST(LineEquation, InitConstructor)
{
  Isis::LineEquation testEquation(1.0, 2.0, 3.0, 4.0);

  EXPECT_TRUE(testEquation.Defined());
  EXPECT_TRUE(testEquation.HaveSlope());
  EXPECT_TRUE(testEquation.HaveIntercept());

  EXPECT_DOUBLE_EQ(testEquation.Slope(), 1.0);
  EXPECT_DOUBLE_EQ(testEquation.Intercept(), 1.0);
  EXPECT_EQ(testEquation.Points(), 2);
}

TEST(LineEquation, AddingPoints)
{
  Isis::LineEquation testEquation;

  testEquation.AddPoint(1.0,2.0);

  EXPECT_EQ(testEquation.Points(), 1);
  EXPECT_FALSE(testEquation.HaveSlope());
  EXPECT_FALSE(testEquation.HaveIntercept());
  EXPECT_FALSE(testEquation.Defined());

  testEquation.AddPoint(3.0,4.0);

  EXPECT_EQ(testEquation.Points(), 2);
  EXPECT_FALSE(testEquation.HaveSlope());
  EXPECT_FALSE(testEquation.HaveIntercept());
  EXPECT_TRUE(testEquation.Defined());
  EXPECT_DOUBLE_EQ(testEquation.Slope(), 1.0);
  EXPECT_DOUBLE_EQ(testEquation.Intercept(), 1.0);
  EXPECT_TRUE(testEquation.HaveSlope());
  EXPECT_TRUE(testEquation.HaveIntercept());

  try
  {
    testEquation.AddPoint(5.0, 6.0);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().find("Line equation is already defined"))
      <<  e.toString();
  }
  catch(...)
  {
    FAIL() << "Expected an IException with message:"
      "\"Line equation is already defined with 2 points\"";
  }

  EXPECT_EQ(testEquation.Points(), 2);
  EXPECT_TRUE(testEquation.HaveSlope());
  EXPECT_TRUE(testEquation.HaveIntercept());
  EXPECT_TRUE(testEquation.Defined());
  EXPECT_DOUBLE_EQ(testEquation.Slope(), 1.0);
  EXPECT_DOUBLE_EQ(testEquation.Intercept(), 1.0);
}

TEST(LineEquation, UndefinedSlope)
{
  Isis::LineEquation testEquation;

  try
  {
    testEquation.Slope();
    FAIL() << "Expected an exception to be thrown";
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().find("Line equation undefined"))
      <<e.toString();
  }
  catch(...)
  {
    FAIL() << "Expected an IException with message:"
      "\"Line equation undefined:  2 points are required\"";
  }
}

TEST(LineEquation, UndefinedIntercept)
{
  Isis::LineEquation testEquation;

  try
  {
    testEquation.Intercept();
    FAIL() << "Expected an exception to be thrown";
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().find("Line equation undefined"))
      <<e.toString();
  }
  catch(...)
  {
    FAIL() << "Expected an IException with message:"
     " \"Line equation undefined:  2 points are required\"";
  }
}

TEST(LineEquation, AddSamePoints)
{
  Isis::LineEquation testEquation;
  testEquation.AddPoint(1.0,1.0);
  testEquation.AddPoint(1.0,1.0);

  try
  {
    testEquation.Intercept();
    FAIL() << "Expected an exception to be thrown";
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().find("Points have identical"))
      <<e.toString();
  }
  catch(...)
  {
    FAIL() << "Expected an IException with message:"
      "\"Points have identical independent variables -- no intercept\"";
  }

  try
  {
    testEquation.Slope();
    FAIL() << "Expected an exception to be thrown";
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().find("Points have identical"))
      <<e.toString();
  }
  catch(...)
  {
    FAIL() << "Expected an IException with message:"
      "\"Points have identical independent variables -- no slope\"";
  }
}

TEST(LineEquation, InitSamePoints)
{
  try
  {
    Isis::LineEquation testEquation(1.0,1.0,1.0,1.0);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().find("Points have identical"))
      <<e.toString();
  }
  catch(...)
  {
    FAIL() << "Expected an IException with message:"
      "\"Points have identical independent variables -- no intercept\"";
  }
}
