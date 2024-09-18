#include "Angle.h" 
#include "SpecialPixel.h"
#include "Constants.h" 
#include "IException.h"

#include <QString>
#include <QDebug> 

#include <gtest/gtest.h>


TEST(AngleTest, DefaultConstructor) {
  Isis::Angle angle;
  EXPECT_FALSE(angle.isValid());
  EXPECT_EQ(angle.radians(), Isis::Null);
  EXPECT_EQ(angle.degrees(), Isis::Null); 
  EXPECT_STREQ(angle.toString().toLatin1(), ""); 
}


TEST(AngleTest, NullInput_DegreeOutput) {
  Isis::Angle angle;
  EXPECT_DOUBLE_EQ(angle.degrees(), Isis::Null);
  EXPECT_FALSE(angle.isValid());
  EXPECT_STREQ(angle.toString().toLatin1(), ""); 
}


TEST(AngleTest, DegreeInput_RadianOutput) {
  Isis::Angle angle(30., Isis::Angle::Degrees );
  EXPECT_DOUBLE_EQ(angle.radians(), 30 * Isis::PI/180.0);
  EXPECT_TRUE(angle.isValid()); 
  EXPECT_STREQ(angle.toString().toLatin1(), "30.0 degrees");  
}


TEST(AngleTest, RadianInput_DegreeOutput) {
  Isis::Angle angle(30. * Isis::PI / 180.0, Isis::Angle::Radians );
  EXPECT_DOUBLE_EQ(angle.degrees(), 30.0);
  EXPECT_STREQ(angle.toString(false).toLatin1(), "30.0");  
}


TEST(AngleTest, CopyConstructor) {
  Isis::Angle angle(30., Isis::Angle::Degrees );
  Isis::Angle angleCopy(angle);
  EXPECT_DOUBLE_EQ(angleCopy.degrees(), 30.0);
}


TEST(AngleTest, Mutators) {
  Isis::Angle angle(30.0, Isis::Angle::Degrees );
  angle.setDegrees(180);
  EXPECT_DOUBLE_EQ(angle.degrees(), 180.0);
  angle.setRadians(Isis::PI);
  EXPECT_DOUBLE_EQ(angle.radians(), Isis::PI);
}

TEST(AngleOperators, QDebug) {
  Isis::Angle angle(0.0, Isis::Angle::Degrees);
  QString qdebug_output;
  QDebug debug(&qdebug_output);
  debug << angle;
  EXPECT_STREQ(qdebug_output.toLatin1(), "0 <radians> (0 <degrees>) "); 
}


TEST(AngleOperators, Assignment) {
  Isis::Angle angle(30.0, Isis::Angle::Degrees );
  angle = Isis::Angle(45.0, Isis::Angle::Degrees);
  EXPECT_DOUBLE_EQ(angle.degrees(), 45.0);
}


TEST(AngleOperators, AddSubtract) {
  Isis::Angle angle1(30.0, Isis::Angle::Degrees );
  Isis::Angle angle2(60.0, Isis::Angle::Degrees );
  angle1 = angle1 + angle2;  
  EXPECT_DOUBLE_EQ(angle1.degrees(), 90.0);
  angle1 += angle2;
  EXPECT_DOUBLE_EQ(angle1.degrees(), 150.0);
  angle1 -= angle2;
  EXPECT_DOUBLE_EQ(angle1.degrees(), 90.0);
  angle1 = angle1 - angle2;
  EXPECT_DOUBLE_EQ(angle1.degrees(), 30.0);
}


TEST(AngleOperators, MultiplyDivide) {
  Isis::Angle angle(30., Isis::Angle::Degrees );
  angle = 2. * angle;
  EXPECT_DOUBLE_EQ(angle.degrees(), 60.0);
  angle *= 2;
  EXPECT_DOUBLE_EQ(angle.degrees(), 120.0);
  angle /= 2;
  EXPECT_DOUBLE_EQ(angle.degrees(), 60.0);
  angle = angle / 2;
  EXPECT_DOUBLE_EQ(angle.degrees(), 30.0);
}  


TEST(AngleOperators, Logical) {
  Isis::Angle angle1(30.0, Isis::Angle::Degrees );
  Isis::Angle angle2(45.0, Isis::Angle::Degrees );
  EXPECT_FALSE(angle1 == angle2);
  EXPECT_TRUE(Isis::Angle() == Isis::Angle()); 
  EXPECT_FALSE(Isis::Angle() == angle2); 
}


TEST(AngleOperators, LessThan) {
  Isis::Angle angle1(30.0, Isis::Angle::Degrees );
  Isis::Angle angle2(45.0, Isis::Angle::Degrees );
  EXPECT_TRUE(angle1 < angle2);
  EXPECT_FALSE(angle2 < angle1);
  EXPECT_FALSE(angle1 < angle1); 
}

// Added good and consistent test for "greater than" 
TEST(AngleOperators, GreaterThan) {
  Isis::Angle angle1(30.0, Isis::Angle::Degrees );
  Isis::Angle angle2(45.0, Isis::Angle::Degrees );
  EXPECT_TRUE(angle2 > angle1);
  EXPECT_FALSE(angle1 > angle2); 
  EXPECT_FALSE(angle2 > angle2); 
}
  

TEST(AngleOperators, LessThanOrEqualTo) {
  Isis::Angle angle1(30.0, Isis::Angle::Degrees );
  Isis::Angle angle2(45.0, Isis::Angle::Degrees );
  EXPECT_TRUE(angle1 <= angle1);
  EXPECT_TRUE(angle1 <= angle2);
  EXPECT_FALSE(angle2 <= angle1); 
}


TEST(AngleExceptions, LessThanNullAngle){
  Isis::Angle angle1(30., Isis::Angle::Degrees );
  
  try {
    if (angle1 < Isis::Angle()) {
      FAIL() << "Expected an error";
    }
   }
   catch(Isis::IException &e) {
    
     EXPECT_TRUE(e.toString().find("Cannot compare a invalid angles with the < operator") != std::string::npos)
       << e.toString();
   }
   catch(...) {
     FAIL() << "Expected IException";
   }
}
    
TEST(AngleExceptions, NullAngleLessThan){
  Isis::Angle angle1(30., Isis::Angle::Degrees );

  try {
    if (Isis::Angle() < angle1) {
      FAIL() << "Expected an error";
    }
   }
   catch(Isis::IException &e) {
     EXPECT_TRUE(e.toString().find("Cannot compare a invalid angles with the < operator") != std::string::npos)
          << e.toString();
   }
   catch(...) {
     FAIL() << "Expected IException";
   }
}

TEST(AngleExceptions, NullAngleLessThanOrEqual){
  Isis::Angle angle1(30., Isis::Angle::Degrees );

  try {
    if (Isis::Angle() <= angle1) {
      FAIL() << "Expected an error";
    }
   }
   catch(Isis::IException &e) {
     EXPECT_TRUE(e.toString().find("Cannot compare a invalid angles with the < operator") != std::string::npos)
       << e.toString();
   }
   catch(...) {
     FAIL() << "Expected IException";
   }
}


TEST(AngleExceptions, GreaterThanNullAngle){
  Isis::Angle angle1(30., Isis::Angle::Degrees );
  
  try {
    if (angle1 > Isis::Angle()) {
      FAIL() << "Expected an error";
    }
   }
   catch(Isis::IException &e) {
     EXPECT_TRUE(e.toString().find("Cannot compare a invalid angles with the > operator") != std::string::npos)
       <<  e.toString();
   }
   catch(...) {
     FAIL() << "Expected IException";
   }
}


TEST(AngleExceptions, GreaterThanOrEqualToNullAngle){
  Isis::Angle angle1(30., Isis::Angle::Degrees );
  
  try {
    if (angle1 >= Isis::Angle()) {
      FAIL() << "Expected an error";
    }
   }
   catch(Isis::IException &e) {
     EXPECT_TRUE(e.toString().find("Cannot compare a invalid angles with the > operator") != std::string::npos)
       <<  e.toString();
   }
   catch(...) {
     FAIL() << "Expected IException";
   }
}


TEST(AngleOperators, Multiplication) {
  Isis::Angle angle(30.0, Isis::Angle::Degrees );
  angle = 2.0 * angle; 
  EXPECT_DOUBLE_EQ(angle.degrees(), 60.0);
}


TEST(Angle, wrapValue){
  Isis::Angle angle(30., Isis::Angle::Degrees);
  EXPECT_EQ(angle.unitWrapValue(Isis::Angle::Degrees), 360);
  EXPECT_DOUBLE_EQ(angle.unitWrapValue(Isis::Angle::Radians), 2*Isis::PI);
}


TEST(Angle, setAngleDegrees){
  Isis::Angle angle(0.0, Isis::Angle::Degrees);
  angle.setAngle(60.0, Isis::Angle::Degrees);
  EXPECT_DOUBLE_EQ(angle.degrees(), 60.0);
}


TEST(Angle, setAngleRadians){
  Isis::Angle angle(0.0, Isis::Angle::Radians);
  angle.setAngle(0.5, Isis::Angle::Radians);
  EXPECT_DOUBLE_EQ(angle.radians(), 0.5);
}


TEST(Angle, QStringConstructor){
  Isis::Angle angle(QString("-70 15 30.125"));
  EXPECT_DOUBLE_EQ(angle.degrees(), -70.25836805555555);
  Isis::Angle angle2(QString("  +70  30 11     "));
  EXPECT_DOUBLE_EQ(angle2.degrees(), 70.503055555555562);
  Isis::Angle angle3(QString("100 00 00"));
  EXPECT_DOUBLE_EQ(angle3.degrees(), 100.0); 
}


TEST(AngleExceptions, InvalidInput){
  try {
    Isis::Angle angle(QString("100"));
    angle.toString(); 

    FAIL() << "Expected an error";
   }
   catch(Isis::IException &e) {
     EXPECT_TRUE(e.toString().find(
         "[100] is not a vaid input to Angle. It needs to be of the form: \"dd mm ss.ss\"") != std::string::npos)
         <<  e.toString();
   }
   catch(...) {
     FAIL() << "Expected IException."; 
   }
}


TEST(AngleExceptions, InvalidInput2){
  try {
    Isis::Angle angle(QString("70 11"));
    angle.toString(); 

    FAIL() << "Expected an error";
   }
   catch(Isis::IException &e) {
     EXPECT_TRUE(e.toString().find(
         "[70 11] is not a vaid input to Angle. It needs to be of the form: \"dd mm ss.ss\"") != std::string::npos)
         <<  e.toString();
   }
   catch(...) {
     FAIL() << "Expected IException."; 
   }
}


TEST(AngleExceptions, InvalidInput3){
  try {
    Isis::Angle angle(QString("this 79 should 00 fail 0.111"));
    angle.toString(); 

    FAIL() << "Expected an error";
   }
   catch(Isis::IException &e) {
     EXPECT_TRUE(e.toString().find(
         "[this 79 should 00 fail 0.111] is not a vaid input to Angle. It needs to be of the form: \"dd mm ss.ss\"") != std::string::npos)
         <<  e.toString();
   } 
   catch(...) {
     FAIL() << "Expected IException."; 
   }
}
