#include <gtest/gtest.h>
#include "Displacement.h"
#include "IException.h"

TEST(Displacement, DefaultConstructor)
{
  Isis::Displacement disp;
  EXPECT_FALSE(disp.isValid());
}

TEST(Displacement, MeterConstructor)
{
  Isis::Displacement disp(1200.2, Isis::Displacement::Meters);
  EXPECT_TRUE(disp.isValid());
  EXPECT_DOUBLE_EQ(disp.meters(), 1200.2);
  EXPECT_DOUBLE_EQ(disp.kilometers(), 1.2002);
  EXPECT_DOUBLE_EQ(disp.pixels(100), 120020);
}

TEST(Displacement, KilometerConstructor)
{
  Isis::Displacement disp(12.805, Isis::Displacement::Kilometers);
  EXPECT_TRUE(disp.isValid());
  EXPECT_DOUBLE_EQ(disp.kilometers(), 12.805);
  EXPECT_DOUBLE_EQ(disp.meters(), 12805);
  EXPECT_DOUBLE_EQ(disp.pixels(15.5), 198477.5);
}

TEST(Displacement, PixelConstructor)
{
  Isis::Displacement disp(14, Isis::Displacement::Pixels);
  EXPECT_TRUE(disp.isValid());
  EXPECT_DOUBLE_EQ(disp.pixels(), 14);
  EXPECT_DOUBLE_EQ(disp.meters(), 14);
  EXPECT_DOUBLE_EQ(disp.kilometers(), 0.014);
}

TEST(Displacement, pixelsPerMeterConstructor)
{
  Isis::Displacement disp(100, 10);
  EXPECT_TRUE(disp.isValid());
  EXPECT_DOUBLE_EQ(disp.pixels(10), 100);
  EXPECT_DOUBLE_EQ(disp.meters(), 10);
  EXPECT_DOUBLE_EQ(disp.kilometers(), 0.01);
}

TEST(Displacement, CopyConstructor)
{
  Isis::Displacement disp1(1, Isis::Displacement::Meters);
  Isis::Displacement disp2(disp1);
  EXPECT_TRUE(disp2.isValid());
  EXPECT_DOUBLE_EQ(disp1.meters(), disp2.meters());
  EXPECT_DOUBLE_EQ(disp1.kilometers(), disp2.kilometers());
  EXPECT_DOUBLE_EQ(disp1.pixels(50), disp2.pixels(50));
}

TEST(Displacement, Setters)
{
  Isis::Displacement disp(100, Isis::Displacement::Meters);
  disp.setKilometers(100);
  EXPECT_DOUBLE_EQ(disp.kilometers(), 100);
  EXPECT_DOUBLE_EQ(disp.meters(), 100000);
  EXPECT_DOUBLE_EQ(disp.pixels(), 100000);
  disp.setPixels(100);
  EXPECT_DOUBLE_EQ(disp.kilometers(), 0.1);
  EXPECT_DOUBLE_EQ(disp.meters(), 100);
  EXPECT_DOUBLE_EQ(disp.pixels(), 100);
  disp.setMeters(100);
  EXPECT_DOUBLE_EQ(disp.kilometers(), 0.1);
  EXPECT_DOUBLE_EQ(disp.meters(), 100);
  EXPECT_DOUBLE_EQ(disp.pixels(), 100);
}

TEST(Displacement, ArithmeticOperators)
{
  Isis::Displacement disp1(150.3, Isis::Displacement::Meters);
  Isis::Displacement disp2(49.7, Isis::Displacement::Meters);
  Isis::Displacement result;
  result = disp1 + disp2;
  EXPECT_DOUBLE_EQ(result.meters(), 200);
  result = disp1 - disp2;
  EXPECT_DOUBLE_EQ(result.meters(), 100.6);
  result = disp2 - disp1;
  EXPECT_DOUBLE_EQ(result.meters(), -100.6);
  result = disp1 * 5;
  EXPECT_DOUBLE_EQ(result.meters(), 751.5);
  result = disp1 / 50.1;
  EXPECT_DOUBLE_EQ(result.meters(), 3);
  result += disp1;
  EXPECT_DOUBLE_EQ(result.meters(), 153.3);
  result -= disp2;
  EXPECT_DOUBLE_EQ(result.meters(), 103.6);
  result *= 2;
  EXPECT_DOUBLE_EQ(result.meters(), 207.2);
  result /= 2;
  EXPECT_DOUBLE_EQ(result.meters(), 103.6);
}


TEST(Displacement, UninitializedArithmetic)
{
  Isis::Displacement disp1;
  Isis::Displacement disp2;
  Isis::Displacement result = disp1 + disp2;
  EXPECT_FALSE(result.isValid());
  result = disp1 - disp2;
  EXPECT_FALSE(result.isValid());
  result = disp1 * 5;
  EXPECT_FALSE(result.isValid());
  result = disp1 / 3;
  EXPECT_FALSE(result.isValid());
  result += disp1;
  EXPECT_FALSE(result.isValid());
  result -= disp1;
  EXPECT_FALSE(result.isValid());
  result *= 2;
  EXPECT_FALSE(result.isValid());
  result /= 4;
  EXPECT_FALSE(result.isValid());
  disp2.setMeters(100);
  result = disp1 + disp2;
  EXPECT_FALSE(result.isValid());
  result = disp2 + disp1;
  EXPECT_FALSE(result.isValid());
  result = disp1 - disp2;
  EXPECT_FALSE(result.isValid());
  result += disp2;
  EXPECT_FALSE(result.isValid());
  result -= disp2;
  EXPECT_FALSE(result.isValid());
}

TEST(Displacement, EqualValuesComparisonOperators)
{
  Isis::Displacement disp1(100, Isis::Displacement::Meters);
  Isis::Displacement disp2(0.1, Isis::Displacement::Kilometers);
  EXPECT_TRUE(disp1 == disp2);
  EXPECT_FALSE(disp1 != disp2);
  EXPECT_TRUE(disp1 >= disp2);
  EXPECT_TRUE(disp1 <= disp2);
  EXPECT_FALSE(disp1 > disp2);
  EXPECT_FALSE(disp1 < disp2);
}

TEST(Displacement, InequalValuesComparisonOperators)
{
  Isis::Displacement disp1(10, Isis::Displacement::Meters);
  Isis::Displacement disp2(100, Isis::Displacement::Meters);
  EXPECT_FALSE(disp1 == disp2);
  EXPECT_TRUE(disp1 != disp2);
  EXPECT_FALSE(disp1 >= disp2);
  EXPECT_TRUE(disp1 <= disp2);
  EXPECT_FALSE(disp1 > disp2);
  EXPECT_TRUE(disp1 < disp2);
}

TEST(Displacement, InequalPixelsComparisonOperators)
{
  Isis::Displacement disp1(100, 10);
  Isis::Displacement disp2(10, Isis::Displacement::Meters);
  EXPECT_TRUE(disp1 == disp2);
  EXPECT_FALSE(disp1 != disp2);
  EXPECT_TRUE(disp1 >= disp2);
  EXPECT_TRUE(disp1 <= disp2);
  EXPECT_FALSE(disp1 > disp2);
  EXPECT_FALSE(disp1 < disp2);
}

TEST(Displacement, UninitializedComparison)
{
  try
  {
    if (Isis::Displacement() > Isis::Displacement()) {
      FAIL() << "Expected an error";
    }
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().find("Displacement has not been"
     " initialized") != std::string::npos) <<  e.toString();
  }
  catch(...)
  {
      FAIL() << "Expected an IExcpetion with message \""
      " Displacement has not been initialized, you must initialize it first"
      " before comparing with another displacement using [>].\"";
  }

  try
  {
    if (Isis::Displacement() < Isis::Displacement()) {
      FAIL() << "Expected an error";
    }
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().find("Displacement has not been"
     " initialized") != std::string::npos) <<  e.toString();
  }
  catch(...)
  {
      FAIL() << "Expected an IException with message \""
      " Displacement has not been initialized, you must initialize it first"
      " before comparing with another displacement using [<].\"";
  }
}
