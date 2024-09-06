#include "Column.h"
#include "TestUtilities.h"

#include <gtest/gtest.h>

using namespace Isis;

class Types : public testing::TestWithParam<Column::Type> {
};

class TypeError : public testing::TestWithParam<Column::Type> {
};

class Align : public testing::TestWithParam<Column::Align> {
};

class PrecisionError : public testing::TestWithParam<Column::Align> {
};

//Tests that the default constructor works as intended
TEST(Column, DefaultConstructor) {
  Column column;

  EXPECT_DOUBLE_EQ(column.Precision(), 4);
  EXPECT_DOUBLE_EQ(column.Width(), 0);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, column.Name(), QString(""));
  EXPECT_EQ(column.Alignment(), Column::NoAlign);
  EXPECT_EQ(column.DataType(), Column::NoType);
}

//Tests that the initialization constructor works as intended
TEST(Column, InitConstructor) {
  Column column("col1", 25, Column::Pixel, Column::Left);

  EXPECT_DOUBLE_EQ(column.Precision(), 4);
  EXPECT_DOUBLE_EQ(column.Width(), 25);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, column.Name(), QString("col1"));
  EXPECT_EQ(column.Alignment(), Column::Left);
  EXPECT_EQ(column.DataType(), Column::Pixel);
}

//Tests SetName & Name functions
TEST(Column, Name) {
  Column column;
  column.SetName("Test Column");

  EXPECT_PRED_FORMAT2(AssertQStringsEqual, column.Name(), QString("Test Column"));
}

//Tests SetWidth & Width functions
TEST(Column, Width) {
  Column column;
  column.SetWidth(100);

  EXPECT_DOUBLE_EQ(column.Width(), 100);
}

//Tests SetType & DataType functions with every member of the Type enum
TEST_P(Types, Type) {
  Column column;
  column.SetType(GetParam());

  EXPECT_EQ(column.DataType(), GetParam());
}

INSTANTIATE_TEST_SUITE_P(Column, Types, ::testing::Values(
  Column::NoType, Column::Integer, Column::Real, Column::String, Column::Pixel));

//Tests SetAlignment & Alignment functions with every member of the Align enum
TEST_P(Align, Alignment) {
  Column column;
  column.SetAlignment(GetParam());

  EXPECT_EQ(column.Alignment(), GetParam());
}

INSTANTIATE_TEST_SUITE_P(Column, Align, ::testing::Values(
  Column::NoAlign, Column::Right, Column::Left, Column::Decimal));

//Tests SetPrecision & Precision functions with Real type and Pixel type.
//These are the only two types expected to work with SetPrecision
TEST(Column, Precision) {
  Column column;
  column.SetType(Column::Real);
  column.SetPrecision(10);
  EXPECT_DOUBLE_EQ(column.Precision(), 10);

  column.SetType(Column::Pixel);
  column.SetPrecision(15);
  EXPECT_DOUBLE_EQ(column.Precision(), 15);
}

//Tests that SetName's exceptions are working correctly
//Should throw an error when SetName is called with a string
//whose length is greater than Width.
TEST(Column, SetNameError) {
  std::string message = "Name [Test Column] is wider than width";
  Column column;
  column.SetWidth(1);
  try {
    column.SetName("Test Column");
  }
  catch(IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected an IException with message \""
    << message.toStdString() <<"\"";
  }
}

//Tests that SetWidth's exceptions are working correctly
//Should throw an error when SetWidth is called with a value less than
//the length of Name
TEST(Column, SetWidthError) {
  std::string message = "Width is insufficient to contain name[Test Column]";
  Column column;
  column.SetName("Test Column");
  try {
    column.SetWidth(1);
  }
  catch(IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected an IException with message \""
    << message.toStdString() <<"\"";
  }
}

//Tests that SetTypes' exceptions are working correctly
//Should throw an error when Alignment is Decimal and
//SetType is called with String or Integer
TEST_P(TypeError, SetTypeError) {
  std::string message = "Integer or string type is not sensible if alignment is Decimal.";
  Column column;
  column.SetAlignment(Column::Decimal);
  try {
    column.SetType(GetParam());
  }
  catch(IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected an IException with message \""
    << message.toStdString() <<"\"";
  }
}

//Tests that SetAlignment's exceptions are working correctly
//Should throw an error when Type is String or Integer and
//SetAllignment is called with Decimal
TEST_P(TypeError, SetAlignmentError) {
  std::string message = "Decimal alignment does not make sense for integer or string values.";
  Column column;
  column.SetType(GetParam());
  try {
    column.SetAlignment(Column::Decimal);
  }
  catch(IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected an IException with message \""
    << message.toStdString() <<"\"";
  }
}

INSTANTIATE_TEST_SUITE_P(Column, TypeError, ::testing::Values(
  Column::Integer, Column::String));

//Tests that Precision's exceptions are working correctly
//Should throw an error when SetPrecision is called and Alignment is not Decimal
TEST_P(PrecisionError, SetPrecisionError) {
  std::string message = "Setting precision only makes sense for Decimal Alignment";
  Column column;
  column.SetAlignment(GetParam());

  try{
    column.SetPrecision(10);
  }
  catch(IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected an IException with message \""
    << message.toStdString() <<"\"";
  }
}
INSTANTIATE_TEST_SUITE_P(Column, PrecisionError, ::testing::Values(
  Column::NoAlign, Column::Right, Column::Left));
