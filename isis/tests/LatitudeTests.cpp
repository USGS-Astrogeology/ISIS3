#include "Angle.h"
#include "Distance.h"
#include "IException.h"
#include "Latitude.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "SpecialPixel.h"
#include "TestUtilities.h"

#include <gtest/gtest.h>
#include <QString>

class Latitude_MappingGroupTest : public ::testing::Test {
  protected:
    Isis::PvlGroup mappingGroup;

    void SetUp() override {
      mappingGroup = Isis::PvlGroup("Mapping");
      mappingGroup += Isis::PvlKeyword("LatitudeType", "Planetocentric");
      mappingGroup += Isis::PvlKeyword("EquatorialRadius", "1.0");
      mappingGroup += Isis::PvlKeyword("PolarRadius", "1.0");
   }
};


class Latitude_TargetNameGroupTest : public ::testing::Test {
  protected:
    Isis::PvlGroup mappingGroup;

    void SetUp() override {
      mappingGroup = Isis::PvlGroup("Mapping");
      mappingGroup += Isis::PvlKeyword("LatitudeType", "Planetocentric");
      mappingGroup += Isis::PvlKeyword("TargetName", "MARS");
   }
};


TEST(Latitude, DefaultConstrcutor) {
  Isis::Latitude latitude;
  EXPECT_FALSE(latitude.isValid());
  EXPECT_EQ(latitude.radians(), Isis::Null);
  EXPECT_EQ(latitude.degrees(), Isis::Null); 
  EXPECT_EQ(latitude.errorChecking(), Isis::Latitude::AllowPastPole);
}


TEST(Latitude, DegreeInputConstructor) {
  Isis::Latitude latitude(45.0, Isis::Angle::Degrees);
  EXPECT_EQ(latitude.degrees(), 45.0);
  EXPECT_EQ(latitude.errorChecking(), Isis::Latitude::AllowPastPole);
}


TEST(Latitude, RadianInputConstructor) {
  Isis::Latitude latitude(Isis::PI/4, Isis::Angle::Radians, Isis::Latitude::ThrowAllErrors);
  EXPECT_EQ(latitude.radians(), Isis::PI/4);
  EXPECT_EQ(latitude.errorChecking(), Isis::Latitude::ThrowAllErrors);
}


TEST(Latitude, AngleInputConstructor) {
  Isis::Angle angle(45.0, Isis::Angle::Degrees);
  Isis::Latitude latitude(angle);
  EXPECT_EQ(latitude.degrees(), 45.0);
  EXPECT_EQ(latitude.errorChecking(), Isis::Latitude::AllowPastPole);
}


TEST_F(Latitude_MappingGroupTest, AnglePlanetocentricConstructor) {
  Isis::Angle angle(45.0, Isis::Angle::Degrees);
  Isis::Latitude latitude(angle, mappingGroup);
  EXPECT_EQ(latitude.degrees(), 45.0);
  EXPECT_EQ(latitude.errorChecking(), Isis::Latitude::ThrowAllErrors);
}


TEST_F(Latitude_MappingGroupTest, AnglePlanetographicConstructor) {
  mappingGroup["LatitudeType"][0] = "Planetographic";
  Isis::Angle angle(45.0, Isis::Angle::Degrees);
  Isis::Latitude latitude(angle, mappingGroup);
  EXPECT_EQ(latitude.degrees(), 45.0);
  EXPECT_EQ(latitude.errorChecking(), Isis::Latitude::ThrowAllErrors);
}


TEST_F(Latitude_MappingGroupTest, DegreePlanetographicConstructor) {
  mappingGroup["LatitudeType"][0] = "Planetographic";
  Isis::Latitude latitude(45.0, mappingGroup, Isis::Angle::Degrees);
  EXPECT_EQ(latitude.degrees(), 45.0);
  EXPECT_EQ(latitude.errorChecking(), Isis::Latitude::ThrowAllErrors);
}


TEST_F(Latitude_MappingGroupTest, DegreePlanetocentricConstructor) {
  Isis::Latitude latitude(45.0, mappingGroup, Isis::Angle::Degrees);
  EXPECT_EQ(latitude.degrees(), 45.0);
  EXPECT_EQ(latitude.errorChecking(), Isis::Latitude::ThrowAllErrors);
}


TEST_F(Latitude_MappingGroupTest, AngleIncorrectLatitudeType) {
  std::string message = "is not recognized";
  try {
    mappingGroup["LatitudeType"][0] = "InvalidValue";
    Isis::Angle angle(45.0, Isis::Angle::Degrees);
    Isis::Latitude latitude(angle, mappingGroup);
    FAIL() << "Expected an IException";
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected error message: \"" << message.toStdString() << "\"";
  }
}


TEST_F(Latitude_MappingGroupTest, DegreeIncorrectLatitudeType) {
  std::string message = "is not recognized";
  try {
    mappingGroup["LatitudeType"][0] = "InvalidValue";
    Isis::Latitude latitude(45.0, mappingGroup, Isis::Angle::Degrees);
    FAIL() << "Expected an IException";
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected error message: \"" << message.toStdString() << "\"";
  }
}


TEST_F(Latitude_MappingGroupTest, AngleMissingRadii) {
  std::string message = "Unable to create Latitude object from given mapping group";
  try {
    mappingGroup.deleteKeyword("EquatorialRadius");
    Isis::Angle angle(45.0, Isis::Angle::Degrees);
    Isis::Latitude latitude(angle, mappingGroup);
    FAIL() << "Expected an IException";
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected error message: \"" << message.toStdString() << "\"";
  }
}


TEST_F(Latitude_MappingGroupTest, DegreeMissingRadii) {
  std::string message = "Unable to create Latitude object from given mapping group";
  try {
    mappingGroup.deleteKeyword("EquatorialRadius");
    Isis::Latitude latitude(45.0, mappingGroup, Isis::Angle::Degrees);
    FAIL() << "Expected an IException";
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected error message: \"" << message.toStdString() << "\"";
  }
}


TEST_F(Latitude_TargetNameGroupTest, DegreeTargetRadiiConstructor) {
  Isis::Latitude latitude(45.0, mappingGroup, Isis::Angle::Degrees);
  EXPECT_EQ(latitude.degrees(), 45.0);
  EXPECT_EQ(latitude.errorChecking(), Isis::Latitude::ThrowAllErrors);
}


TEST_F(Latitude_TargetNameGroupTest, AngleTargetRadiiConstructor) {
  Isis::Angle angle(45.0, Isis::Angle::Degrees);
  Isis::Latitude latitude(angle, mappingGroup);
  EXPECT_EQ(latitude.degrees(), 45.0);
  EXPECT_EQ(latitude.errorChecking(), Isis::Latitude::ThrowAllErrors);
}


TEST(Latitude, PlanetocentricConstructor) {
  Isis::Latitude latitude(45.0,
                          Isis::Distance(1.0, Isis::Distance::Units::Meters),
                          Isis::Distance(1.0, Isis::Distance::Units::Meters),
                          Isis::Latitude::Planetocentric,
                          Isis::Angle::Degrees);
  EXPECT_EQ(latitude.degrees(), 45.0);
  EXPECT_EQ(latitude.errorChecking(), Isis::Latitude::ThrowAllErrors);
}


TEST(Latitude, PlanetographicConstructor) {
  Isis::Latitude latitude(45.0,
                          Isis::Distance(1.0, Isis::Distance::Units::Meters),
                          Isis::Distance(1.0, Isis::Distance::Units::Meters),
                          Isis::Latitude::Planetographic,
                          Isis::Angle::Degrees);
  EXPECT_EQ(latitude.degrees(), 45.0);
  EXPECT_EQ(latitude.errorChecking(), Isis::Latitude::ThrowAllErrors);
}


TEST(Latitude, PlanetographicEllipsoidConstructor) {
  Isis::Latitude latitude(45.0,
                          Isis::Distance(1.0, Isis::Distance::Units::Meters),
                          Isis::Distance(2.0, Isis::Distance::Units::Meters),
                          Isis::Latitude::Planetographic,
                          Isis::Angle::Degrees);

  double radianTruth = atan( tan(45.0 * (Isis::PI / 180.0)) * (2.0 / 1.0) * (2.0 / 1.0) );

  EXPECT_EQ(latitude.radians(), radianTruth);
  EXPECT_EQ(latitude.errorChecking(), Isis::Latitude::ThrowAllErrors);
}


TEST(Latitude, CopyConstructor) {
  Isis::Latitude latitude1(45.0,
                           Isis::Distance(1.0, Isis::Distance::Units::Meters),
                           Isis::Distance(1.0, Isis::Distance::Units::Meters),
                           Isis::Latitude::Planetographic,
                           Isis::Angle::Degrees);
  Isis::Latitude latitude2(latitude1);
  EXPECT_EQ(latitude2.degrees(), 45.0);
  EXPECT_EQ(latitude2.errorChecking(), Isis::Latitude::ThrowAllErrors);
}


TEST(Latitude, SetPlanetocentric) {
  Isis::Latitude latitude;
  latitude.setPlanetocentric(45.0, Isis::Angle::Degrees);
  ASSERT_EQ(latitude.planetocentric(Isis::Angle::Degrees), 45.0);
}


TEST(Latitude, SetPlanetocentricPastPole) {
  Isis::Latitude latitude(45.0,
                          Isis::Distance(1.0, Isis::Distance::Units::Meters),
                          Isis::Distance(1.0, Isis::Distance::Units::Meters),
                          Isis::Latitude::Planetocentric,
                          Isis::Angle::Degrees,
                          Isis::Latitude::AllowPastPole);
  latitude.setPlanetocentric(95.0, Isis::Angle::Degrees);
  ASSERT_EQ(latitude.planetocentric(Isis::Angle::Degrees), 95.0);
}


TEST(Latitude, SetPlanetocentricOutOfRange) {
  std::string message = "Latitudes outside of the -90/90 range cannot be converted";
  try {
      Isis::Latitude latitude(30.0,
                              Isis::Distance(1.0, Isis::Distance::Units::Meters),
                              Isis::Distance(1.0, Isis::Distance::Units::Meters),
                              Isis::Latitude::Planetocentric,
                              Isis::Angle::Degrees);
    latitude.setPlanetographic(95.0, Isis::Angle::Degrees);
    FAIL() << "Expected an IException";
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected error message: \"" << message.toStdString() << "\"";
  }
}


TEST(Latitude, SetPlanetographic) {
  Isis::Latitude latitude(30.0,
                          Isis::Distance(1.0, Isis::Distance::Units::Meters),
                          Isis::Distance(1.0, Isis::Distance::Units::Meters),
                          Isis::Latitude::Planetographic,
                          Isis::Angle::Degrees);
  latitude.setPlanetographic(45.0, Isis::Angle::Degrees);
  ASSERT_EQ(latitude.planetographic(Isis::Angle::Degrees), 45.0);
}


TEST(Latitude, SetPlanetographicRadiiNotSet) {
  std::string message = "cannot be converted to Planetocentic without the planetary radii";
  try {
    Isis::Latitude latitude(30.0, Isis::Angle::Degrees);
    latitude.setPlanetographic(45.0, Isis::Angle::Degrees);
    FAIL() << "Expected an IException";
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected error message: \"" << message.toStdString() << "\"";
  }
}


TEST(Latitude, SetPlanetographicOutOfRange) {
  std::string message = "Latitudes outside of the -90/90 range cannot be converted";
  try {
      Isis::Latitude latitude(30.0,
                              Isis::Distance(1.0, Isis::Distance::Units::Meters),
                              Isis::Distance(1.0, Isis::Distance::Units::Meters),
                              Isis::Latitude::Planetographic,
                              Isis::Angle::Degrees);
    latitude.setPlanetographic(95.0, Isis::Angle::Degrees);
    FAIL() << "Expected an IException";
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected error message: \"" << message.toStdString() << "\"";
  }
}


TEST(Latitude, SetErrorChecking) {
  Isis::Latitude latitude(35.0, Isis::Angle::Degrees);
  latitude.setErrorChecking(Isis::Latitude::ThrowAllErrors);
  ASSERT_EQ(latitude.errorChecking(), Isis::Latitude::ThrowAllErrors);
}


TEST(Latitude, InRange) {
  Isis::Latitude latitudeMin(1.0, Isis::Angle::Degrees);
  Isis::Latitude latitudeMax(3.0, Isis::Angle::Degrees);
  Isis::Latitude latitude(2.0, Isis::Angle::Degrees);

  ASSERT_TRUE(latitude.inRange(latitudeMin, latitudeMax));
}


TEST(Latitude, OutOfRange) {
  Isis::Latitude latitudeMin(1.0, Isis::Angle::Degrees);
  Isis::Latitude latitudeMax(2.0, Isis::Angle::Degrees);
  Isis::Latitude latitude(6.0, Isis::Angle::Degrees);

  ASSERT_FALSE(latitude.inRange(latitudeMin, latitudeMax));
}


TEST(Latitude, MinGreaterThanMax) {
  std::string message = "is greater than maximum latitude";
  try{
    Isis::Latitude latitudeMin(3.0, Isis::Angle::Degrees);
    Isis::Latitude latitudeMax(1.0, Isis::Angle::Degrees);
    Isis::Latitude latitude(2.0, Isis::Angle::Degrees);
    latitude.inRange(latitudeMin, latitudeMax);
    FAIL() << "Expected an IException";
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected error message: \"" << message.toStdString() << "\"";
  }
}


TEST(Latitude, Assignment) {
  Isis::Latitude latitude1(1.0, Isis::Angle::Degrees);
  Isis::Latitude latitude2(2.0, Isis::Angle::Degrees);
  latitude1 = latitude2;
  EXPECT_EQ(latitude1.degrees(), latitude2.degrees());
  EXPECT_EQ(latitude1.errorChecking(), latitude2.errorChecking());
}


TEST_F(Latitude_MappingGroupTest, AddPlanetocentric) {
  Isis::Latitude latitude(1.0, mappingGroup, Isis::Angle::Degrees);
  Isis::Angle angleToAdd(2.0, Isis::Angle::Degrees);
  latitude = latitude.add(angleToAdd, mappingGroup);
  ASSERT_DOUBLE_EQ(latitude.degrees(), 1.0 + 2.0);
}


TEST_F(Latitude_MappingGroupTest, AddPlanetographic) {
  mappingGroup["LatitudeType"][0] = "Planetographic";
  Isis::Latitude latitude(1.0, mappingGroup, Isis::Angle::Degrees);
  Isis::Angle angleToAdd(2.0, Isis::Angle::Degrees);
  latitude = latitude.add(angleToAdd, mappingGroup);
  ASSERT_DOUBLE_EQ(latitude.degrees(), 1.0 + 2.0);
}


TEST_F(Latitude_MappingGroupTest, AddIncorrectLatitudeType) {
  std::string message = "is not recognized";
  try {
    mappingGroup["LatitudeType"][0] = "Planetographic";
    Isis::Angle angle(1.0, Isis::Angle::Degrees);
    Isis::Latitude latitude(angle, mappingGroup);
    mappingGroup["LatitudeType"][0] = "InvalidValue";
    latitude = latitude.add(angle, mappingGroup);
    FAIL() << "Expected an IException";
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected error message: \"" << message.toStdString() << "\"";
  }
}


TEST_F(Latitude_TargetNameGroupTest, Add) {
  Isis::Latitude latitude(1.0, mappingGroup, Isis::Angle::Degrees);
  Isis::Angle angleToAdd(2.0, Isis::Angle::Degrees);
  latitude = latitude.add(angleToAdd, mappingGroup);
  ASSERT_DOUBLE_EQ(latitude.degrees(), 1.0 + 2.0);
}


TEST(Latitude, AddPlanetocentric) {
  Isis::Latitude latitude(1.0, Isis::Angle::Degrees);
  Isis::Angle angleToAdd(2.0, Isis::Angle::Degrees);
  Isis::Distance equatorialRadius(1.0, Isis::Distance::Units::Meters);
  Isis::Distance polarRadius(1.0, Isis::Distance::Units::Meters);
  latitude = latitude.add(angleToAdd, 
                          equatorialRadius, 
                          polarRadius, 
                          Isis::Latitude::CoordinateType::Planetocentric);  
  ASSERT_DOUBLE_EQ(latitude.degrees(), 1.0 + 2.0);
}


TEST(Latitude, AddPlanetographic) {
  Isis::Distance equatorialRadius(1.0, Isis::Distance::Units::Meters);
  Isis::Distance polarRadius(1.0, Isis::Distance::Units::Meters);
  Isis::Latitude latitude(1.0,
                          equatorialRadius,
                          polarRadius,
                          Isis::Latitude::Planetographic,
                          Isis::Angle::Degrees);  
  Isis::Angle angleToAdd(2.0, Isis::Angle::Degrees);
  latitude = latitude.add(angleToAdd, 
                          equatorialRadius, 
                          polarRadius, 
                          Isis::Latitude::CoordinateType::Planetographic);
  ASSERT_DOUBLE_EQ(latitude.degrees(), 1.0 + 2.0);
}