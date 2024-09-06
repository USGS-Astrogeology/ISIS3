#include "GaussianDistribution.h"
#include "IException.h"
#include "TestUtilities.h"

#include <gtest/gtest.h>
#include <QPair>
#include <QString>

class DoubleTest : public ::testing::TestWithParam<QPair <double, double> > {
  // Intentionally empty
};


TEST(GaussianDistribution, DefaultConstructor) {
  Isis::GaussianDistribution dist;
  EXPECT_EQ(dist.Mean(), 0);
  EXPECT_EQ(dist.StandardDeviation(), 1.0);
}


TEST(GaussianDistribution, Constructor) {
  Isis::GaussianDistribution dist(1.0, 2.0);
  EXPECT_EQ(dist.Mean(), 1.0);
  EXPECT_EQ(dist.StandardDeviation(), 2.0);
}


TEST(GaussianDistribution, InvalidPercentage) {
  Isis::GaussianDistribution dist;
  std::string message = "Argument percent outside of the range 0 to 100";
  try {
    dist.InverseCumulativeDistribution(110);
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected error message: \"" << message.toStdString() << "\"";
  }
}


TEST_P(DoubleTest, Distributions) {
  Isis::GaussianDistribution dist;
  EXPECT_NEAR(dist.CumulativeDistribution(GetParam().first), GetParam().second, .000000000000000001);
  EXPECT_NEAR(dist.InverseCumulativeDistribution(GetParam().second), GetParam().first, .000001);
}


QPair <double, double> neg3(-3.0, 0.13498980316295217);
QPair <double, double> neg2point5(-2.5, 0.62096653257757595);
QPair <double, double> neg2(-2.0, 2.2750131948179018);
QPair <double, double> neg1point5(-1.5, 6.6807201268857881);
QPair <double, double> neg1(-1.0, 15.865525393145695);
QPair <double, double> negpoint5(-0.5, 30.853753872598688);
QPair <double, double> zero(0.0, 50);
QPair <double, double> pospoint5(0.5, 69.146246127401312);
QPair <double, double> pos1(1.0, 84.134474606854297);
QPair <double, double> pos1point5(1.5, 93.319279873114212);
QPair <double, double> pos2(2.0, 97.724986805182098);
QPair <double, double> pos2point5(2.5, 99.379033467422431);
QPair <double, double> pos3(3.0, 99.865010196837048);


INSTANTIATE_TEST_SUITE_P(GaussianDistribution, DoubleTest, ::testing::Values(neg3, neg2point5, 
                         neg2, neg1point5, neg1, negpoint5, zero, pospoint5, pos1, pos1point5, 
                         pos2, pos2point5, pos3));