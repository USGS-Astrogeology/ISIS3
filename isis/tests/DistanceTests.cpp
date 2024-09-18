#include "Distance.h"
#include "Displacement.h"
#include "IException.h" 
#include "SpecialPixel.h"

#include <gtest/gtest.h>

using namespace Isis;

TEST(DistanceTests, DefaultConstructor) {
	Distance dist;
	EXPECT_EQ(dist.meters(), Null);
	EXPECT_EQ(dist.kilometers(), Null);
	EXPECT_EQ(dist.pixels(), Null);
	EXPECT_EQ(dist.solarRadii(), Null);
}


TEST(DistanceTests, MetersConstructor) {
	Distance dist(1500500, Distance::Meters);
	EXPECT_EQ(dist.meters(), 1500500);
	EXPECT_EQ(dist.kilometers(), 1500.5);
	EXPECT_DOUBLE_EQ(dist.solarRadii(), 1500500 / 6.9599e8);
	EXPECT_EQ(dist.pixels(1), 1500500);
}


TEST(DistanceTests, KilometersConstructor) {
	Distance dist(1500.5, Distance::Kilometers);
	EXPECT_EQ(dist.kilometers(), 1500.5);
	EXPECT_EQ(dist.meters(), 1500500);
	EXPECT_DOUBLE_EQ(dist.solarRadii(), 1500500 / 6.9599e8);
	EXPECT_EQ(dist.pixels(1), 1500500);
}


TEST(DistanceTests, SolarRadiiConstructor) {
	Distance dist(1, Distance::SolarRadii);
	EXPECT_EQ(dist.solarRadii(), 1);
	EXPECT_DOUBLE_EQ(dist.meters(), 6.9599e8);
	EXPECT_DOUBLE_EQ(dist.kilometers(), 6.9599e5);
	EXPECT_DOUBLE_EQ(dist.pixels(1), 6.9599e8);
}


TEST(DistanceTests, PixelsConstructor) {
	Distance dist(1500500, Distance::Pixels);
	EXPECT_EQ(dist.pixels(1), 1500500);
	EXPECT_EQ(dist.meters(), 1500500);
	EXPECT_EQ(dist.kilometers(), 1500.5);
	EXPECT_DOUBLE_EQ(dist.solarRadii(), 1500500 / 6.9599e8);
}


TEST(DistanceTests, PixelsPerMeterConstructor) {
	Distance dist(1500500, 2);
	EXPECT_EQ(dist.pixels(2), 1500500);
	EXPECT_EQ(dist.meters(), 750250);
	EXPECT_EQ(dist.kilometers(), 750.25);
	EXPECT_DOUBLE_EQ(dist.solarRadii(), 750250 / 6.9599e8);
}


TEST(DistanceTests, CopyConstructor) {
	Distance origDist(1500.5, Distance::Meters);
	Distance copiedDist(origDist);
	ASSERT_EQ(copiedDist.meters(), 1500.5);
}


TEST(DistanceTests, SetMeters) {
	Distance dist;
	dist.setMeters(1500500);
	ASSERT_EQ(dist.meters(), 1500500);
}


TEST(DistanceTests, SetKilometers) {
	Distance dist;
	dist.setKilometers(1500500);
	ASSERT_EQ(dist.kilometers(), 1500500);
}


TEST(DistanceTests, SetSolarRadii) {
	Distance dist;
	dist.setSolarRadii(1);
	ASSERT_EQ(dist.solarRadii(), 1);
}


TEST(DistanceTests, SetPixels) {
	Distance dist;
	dist.setPixels(1500500, 2);
	ASSERT_EQ(dist.pixels(2), 1500500);
}


TEST(DistanceTests, SetNegativeDistance) {
	try {
		Distance dist;
		dist.setMeters(-1);
		FAIL() << "Expected error message: Negative distances are not supported";
	}
	catch(IException &e) {
	    EXPECT_TRUE(e.toString().find("Negative distances are not supported") != std::string::npos) 
	    									<<  e.toString();
	}
	catch(...) {
	   	FAIL() << "Expected error message: Negative distances are not supported";
	}
}


TEST(DistanceTests, ToString) {
	Distance dist(1500500, Distance::Meters);
	ASSERT_EQ(dist.toString(), "1500500.0 meters");
}


TEST(DistanceTests, IsValidTrue) {
	Distance dist(1500500, Distance::Meters);
	ASSERT_TRUE(dist.isValid());
}


TEST(DistanceTests, IsValidFalse) {
	Distance dist;
	ASSERT_FALSE(dist.isValid());
}


TEST(DistanceTests, GreaterThanDifferent) {
	Distance dist1(10, Distance::Meters);
	Distance dist2(20, Distance::Meters);
	EXPECT_TRUE(dist2 > dist1);
	EXPECT_FALSE(dist1 > dist2);
}


TEST(DistanceTests, GreaterThanEqual) {
	Distance dist1(10, Distance::Meters);
	Distance dist2(10, Distance::Meters);
	EXPECT_FALSE(dist2 > dist1);
	EXPECT_FALSE(dist1 > dist2);
}


TEST(DistanceTests, GreaterThanNull) {
	try {
		bool value = Distance() > Distance();
		ASSERT_TRUE(value);	// Added this line so we do not get an unused comparison build warning.
		FAIL() << "Expected error message: Distance has not been initialized";
	}
	catch(IException &e) {
	    EXPECT_TRUE(e.toString().find("Distance has not been initialized") != std::string::npos) 
	    									<<  e.toString();
	}
	catch(...) {
	   	FAIL() << "Expected error message: Distance has not been initialized";
	}
}


TEST(DistanceTests, LessThanDifferent) {
	Distance dist1(10, Distance::Meters);
	Distance dist2(20, Distance::Meters);
	EXPECT_TRUE(dist1 < dist2);
	EXPECT_FALSE(dist2 < dist1);
}


TEST(DistanceTests, LessThanEqual) {
	Distance dist1(10, Distance::Meters);
	Distance dist2(10, Distance::Meters);
	EXPECT_FALSE(dist1 < dist2);
	EXPECT_FALSE(dist2 < dist1);
}


TEST(DistanceTests, LessThanNull) {
	try {
		bool value = Distance() < Distance();
		ASSERT_TRUE(value);	// Added this line so we do not get an unused comparison build warning.
		FAIL() << "Expected error message: Distance has not been initialized";
	}
	catch(IException &e) {
	    EXPECT_TRUE(e.toString().find("Distance has not been initialized") != std::string::npos) 
	    									<<  e.toString();
	}
	catch(...) {
	   	FAIL() << "Expected error message: Distance has not been initialized";
	}
}


TEST(DistanceTests, AssignDistance) {
	Distance dist1(10, Distance::Meters);
	Distance dist2(20, Distance::Meters);
	dist1 = dist2;
	ASSERT_EQ(dist1.meters(), 20);
}


TEST(DistanceTests, Add) {
	Distance dist1(10, Distance::Meters);
	Distance dist2(20, Distance::Meters);
	Distance sum = dist1 + dist2;
	ASSERT_EQ(sum.meters(), 30);
}


TEST(DistanceTests, SubtractPositive) {
	Distance dist1(10, Distance::Meters);
	Distance dist2(20, Distance::Meters);
	Displacement difference = dist2 - dist1;
	ASSERT_EQ(difference.meters(), 10);
}


TEST(DistanceTests, SubtractNegative) {
	Distance dist1(10, Distance::Meters);
	Distance dist2(20, Distance::Meters);
	Displacement difference = dist1 - dist2;
	ASSERT_EQ(difference.meters(), -10);
}


TEST(DistanceTests, DivideDistance) {
	Distance dist1(10, Distance::Meters);
	Distance dist2(20, Distance::Meters);
	ASSERT_EQ(dist2 / dist1, 2);
}


TEST(DistanceTests, DivideDouble) {
	Distance dist1(10, Distance::Meters);
	Distance quotient = dist1 / 2;
	ASSERT_EQ(quotient.meters(), 5);
}


TEST(DistanceTests, Multiply) {
	Distance dist1(10, Distance::Meters);
	Distance product = dist1 * 2;
	ASSERT_EQ(product.meters(), 20);
}


TEST(DistanceTests, AddAssign) {
	Distance dist1(10, Distance::Meters);
	Distance dist2(20, Distance::Meters);
	dist1 += dist2;
	ASSERT_EQ(dist1.meters(), 30);
}


TEST(DistanceTests, SubtractAssignPositive) {
	Distance dist1(10, Distance::Meters);
	Distance dist2(30, Distance::Meters);
	dist2 -= dist1;
	ASSERT_EQ(dist2.meters(), 20);
}


TEST(DistanceTests, SubtractAssignNegative) {
	try {
		Distance dist1(10, Distance::Meters);
		Distance dist2(30, Distance::Meters);
		dist1 -= dist2;
		FAIL() << "Expected error message: Negative distances are not supported";
	}
	catch(IException &e) {
	    EXPECT_TRUE(e.toString().find("Negative distances are not supported") != std::string::npos) 
	    									<<  e.toString();
	}
	catch(...) {
	   	FAIL() << "Expected error message: Negative distances are not supported";
	}
}


TEST(DistanceTests, DivideAssign) {
	Distance dist(10, Distance::Meters);
	dist /= 2;
	ASSERT_EQ(dist.meters(), 5);
}


TEST(DistanceTests, MultiplyAssign) {
	Distance dist(10, Distance::Meters);
	dist *= 2;
	ASSERT_EQ(dist.meters(), 20);
}