#include "Constants.h"
#include <gtest/gtest.h>

TEST (ConstantsTests, TestPi){
  ASSERT_NEAR(Isis::PI, 3.141592653589793, .000000000000001);
}


TEST (ConstantsTests, TestHalfPi){
  ASSERT_NEAR(Isis::PI/2, 1.570796326794897, .000000000000001);
}

TEST (ConstantsTests, TestE){
  ASSERT_NEAR(Isis::E, 2.718281828459045, .000000000000001);
}

TEST (ConstantsTests, TestDeg2Rad){
  ASSERT_NEAR(Isis::DEG2RAD, 0.0174532925199433, .000000000000001);
  ASSERT_EQ(Isis::DEG2RAD * 180, Isis::PI);
}

TEST (ConstantsTests, TestRad2Deg){
  ASSERT_NEAR(Isis::RAD2DEG, 57.29577951308232, .000000000000001);
  ASSERT_EQ(Isis::RAD2DEG * Isis::PI/2, 90);
}

TEST (ConstantsTests, TestBigInt){
  ASSERT_EQ(sizeof(Isis::BigInt), 8);
}
