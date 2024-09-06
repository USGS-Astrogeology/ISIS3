#include "ID.h"
#include "IException.h"
#include "TestUtilities.h"
#include "gtest/gtest.h"

#include <QString>

TEST(ID, ConstructorDefaultBaseNum) {
  Isis::ID pid("ABC?");
  EXPECT_PRED_FORMAT2(Isis::AssertQStringsEqual, pid.Next(), "ABC1");
  EXPECT_PRED_FORMAT2(Isis::AssertQStringsEqual, pid.Next(), "ABC2");
}


TEST(ID, ConstructorSetBaseNum) {
  Isis::ID pid("ABC?", 2);
  EXPECT_PRED_FORMAT2(Isis::AssertQStringsEqual, pid.Next(), "ABC2");
  EXPECT_PRED_FORMAT2(Isis::AssertQStringsEqual, pid.Next(), "ABC3");
}


TEST(ID, ConstructorNoReplacement) {
  std::string message = "No replacement set in string";
  try {
    Isis::ID pid("ABC");
      FAIL() << "Expected an IException";
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected error message: \"" << message.toStdString() << "\"";
  }
}


TEST(ID, ConstructorMultipleReplacements) {
  std::string message = "contains more than one replacement set";
  try {
    Isis::ID pid("A?B?C");
    FAIL() << "Expected an IException";
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected error message: \"" << message.toStdString() << "\"";
  }
}


TEST(ID, Next) {
  Isis::ID pid("ABC??");
  QString idString;

  for (int i = 1; i < 20; i++) {
    if (i < 10) {
      idString = "ABC0" + QString::number(i);
    }
    else {
      idString = "ABC" + QString::number(i);
    }
    EXPECT_PRED_FORMAT2(Isis::AssertQStringsEqual, pid.Next(), idString);
  }
}

TEST(ID, NextMaximumReached) {
  std::string message = "Maximum number reached for string";
  try {
    Isis::ID pid("ABC?");
    for (int i = 0; i < 11; i++) {
      pid.Next();
    } 
    FAIL() << "Expected an IException";
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected error message: \"" << message.toStdString() << "\"";
  }
}

