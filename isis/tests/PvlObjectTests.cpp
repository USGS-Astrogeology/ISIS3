#include "PvlObject.h"
#include "IException.h"

#include "TestUtilities.h"

#include <QString>

#include <iostream>
#include <sstream>

#include <fstream>
#include <stdlib.h>

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

using namespace Isis; 
using namespace std; 

TEST(PvlObject, HasKeyword){
  PvlObject o("Beasts");
  PvlKeyword kw1("CAT", "Meow");
  o+=kw1;
  
  PvlGroup g("Fish");
  PvlKeyword kw2("Trout", "Brown");
  PvlKeyword kw3("Bass", "Large mouth");
  g += kw2;
  g += kw3;
  o += g;

  EXPECT_TRUE(o.hasKeyword("CAT", PvlObject::Traverse));
  EXPECT_EQ(o.findKeyword("CAT", PvlObject::Traverse), kw1);

  EXPECT_TRUE(o.hasKeyword("Trout", PvlObject::Traverse));
  EXPECT_EQ(o.findKeyword("Trout", PvlObject::Traverse), kw2);

  EXPECT_TRUE(o.hasKeyword("Bass", PvlObject::Traverse));
  EXPECT_EQ(o.findKeyword("Bass", PvlObject::Traverse), kw3);
}

TEST(PvlObject, KeywordError) {
  PvlObject o;

  try {
    o.findKeyword("CAT", PvlObject::Traverse);
  }
  catch (IException &e) {
    EXPECT_TRUE(e.toString().find("Unable to find PVL keyword") != std::string::npos);
    return; 
  }
  
  FAIL() << "Expected error message: Unable to find PVL keyword";
}

TEST(PvlObject, streamParse) {
  PvlObject o;
  stringstream os;

  os << "Object = Hello\nKey=Value\nEndObject";
  os >> o;

  EXPECT_TRUE(o.hasKeyword("Key", PvlObject::Traverse));
}

TEST(PvlObject, invalidStream) {
  PvlObject o;
  stringstream os;
  os << "Object = Hello\nKey=Value\nEndGroup\n";
  try {  
    os >> o;
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().find("Unexpected [EndGroup] in PVL Object") != std::string::npos);
    return;
  }

  FAIL() << "Expected Error message: Unexpected [EndGroup] in PVL Object"; 
}


TEST(PvlObject, validateObject) {
  PvlObject pvlTmplObjectRoot("Object0");
  PvlObject pvlTmplObject1("Object1");
  PvlObject pvlTmplObject2("Object2");
  PvlGroup pvlTmplGrp("Point_ErrorMagnitude");
  PvlKeyword pvlTmplKwrd("Point_ErrorMagnitude__Required", "true");
  pvlTmplGrp += pvlTmplKwrd;
  pvlTmplKwrd.clear();

  pvlTmplKwrd = PvlKeyword("LessThan", "double");
  pvlTmplGrp += pvlTmplKwrd;
  pvlTmplKwrd.clear();

  pvlTmplKwrd = PvlKeyword("LessThan__Required", "false");
  pvlTmplGrp += pvlTmplKwrd;
  pvlTmplKwrd.clear();

  pvlTmplKwrd = PvlKeyword("LessThan__Repeated", "false");
  pvlTmplGrp += pvlTmplKwrd;
  pvlTmplKwrd.clear();

  pvlTmplKwrd = PvlKeyword("GreaterThan", "double");
  pvlTmplGrp += pvlTmplKwrd;
  pvlTmplKwrd.clear();

  pvlTmplKwrd = PvlKeyword("GreaterThan__Required", "true");
  pvlTmplGrp += pvlTmplKwrd;
  pvlTmplKwrd.clear();

  pvlTmplKwrd = PvlKeyword("GreaterThan__Repeated", "true");
  pvlTmplGrp += pvlTmplKwrd;

  pvlTmplObject1 += pvlTmplGrp;
  pvlTmplKwrd.clear();
  pvlTmplKwrd = PvlKeyword("Test_Required", "false");
  pvlTmplObject1 += pvlTmplKwrd;
  pvlTmplKwrd.clear();

  pvlTmplKwrd = PvlKeyword("Test_Repeated", "true");
  pvlTmplObject1 += pvlTmplKwrd;
  pvlTmplKwrd.clear();

  pvlTmplKwrd = PvlKeyword("Test", "string");
  pvlTmplObject1 += pvlTmplKwrd;

  pvlTmplObject2 += pvlTmplObject1;

  pvlTmplObjectRoot += pvlTmplObject2;

  // PvlGroup to be Validated
  PvlObject pvlObjectRoot("Object0");
  PvlObject pvlObject1("Object1");
  PvlObject pvlObject2("Object2");
  PvlGroup pvlGrp("Point_errormagnitude");
  PvlKeyword pvlKwrd("LessThan", "2");
  pvlGrp += pvlKwrd;

  pvlKwrd.clear();
  pvlKwrd = PvlKeyword("GreaterThan", "3.5");
  pvlGrp += pvlKwrd;

  pvlKwrd.clear();
  pvlKwrd = PvlKeyword("GreaterThan", "4.4545");
  pvlGrp += pvlKwrd;

  pvlObject1 += pvlGrp;

  pvlKwrd.clear();
  pvlKwrd = PvlKeyword("Test", "testing1");
  pvlObject1 += pvlKwrd;

  pvlKwrd.clear();
  pvlKwrd = PvlKeyword("Test", "testing2");
  pvlObject1 += pvlKwrd;

  pvlKwrd.clear();
  pvlKwrd = PvlKeyword("TestTest", "Not in Template");
  pvlObject1 += pvlKwrd;

  pvlObject2 += pvlObject1;

  pvlObjectRoot += pvlObject2;

  pvlTmplObjectRoot.validateObject(pvlObjectRoot);
  
  EXPECT_EQ(pvlObjectRoot.findKeyword("TestTest", PvlObject::Traverse), pvlKwrd);
}


TEST(PvlObject, constructFromPvl) {
  json j = {
    {"strkey", "fooval"},
    {"boolkey", false},
    {"numkey", 4.0}, 
    {"arrkey", {1,2,3,4}}
  };

  PvlObject p("CONVERTED", j);

  PvlKeyword strkey("strkey", "fooval");
  PvlKeyword boolkey("boolkey", "false");
  PvlKeyword numkey("numkey", "4.0"); 
  PvlKeyword arrkey("arrkey");
  arrkey += "1";
  arrkey += "2";
  arrkey += "3";
  arrkey += "4";

  EXPECT_EQ(p.findKeyword("strkey"), strkey);
  EXPECT_EQ(p.findKeyword("boolkey"), boolkey);
  EXPECT_EQ(p.findKeyword("arrkey"), arrkey);
  EXPECT_EQ(p.findKeyword("numkey"), numkey);
}

TEST(PvlObject, PvlGroupEqualTest){
  PvlGroup pvlTmplGrp("Point_ErrorMagnitude");
  PvlKeyword pvlTmplKwrd("Point_ErrorMagnitude__Required", "true");
  PvlKeyword pvlTmplKwrdOne("some_message", "true");
  PvlKeyword pvlTmplKwrdTwo("foo", "true");
  PvlKeyword pvlTmplKwrdThree("bar", "false");
  PvlKeyword pvlTmplKwrdFour("fooie", "true");
  pvlTmplGrp += pvlTmplKwrd;

  PvlGroup copy = PvlGroup(pvlTmplGrp);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, pvlTmplGrp, copy); // should pass


  copy.addKeyword(pvlTmplKwrdOne);

  EXPECT_FALSE(AssertPvlGroupEqual("Point_ErrorMagnitude", "Point_ErrorMagnitude", copy, pvlTmplGrp)); // should fail

  pvlTmplGrp.addKeyword(pvlTmplKwrdOne);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, pvlTmplGrp, copy); // should pass

  copy.addKeyword(pvlTmplKwrdTwo);
  pvlTmplGrp.addKeyword(pvlTmplKwrdThree);

  EXPECT_FALSE(AssertPvlGroupEqual("Point_ErrorMagnitude", "Point_ErrorMagnitude", copy, pvlTmplGrp)); // should fail

  copy.addKeyword(pvlTmplKwrdThree);
  pvlTmplGrp.addKeyword(pvlTmplKwrdTwo);
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, pvlTmplGrp, copy); // should pass

  copy.addKeyword(pvlTmplKwrdFour);
  EXPECT_FALSE(AssertPvlGroupEqual("Point_ErrorMagnitude", "Point_ErrorMagnitude", copy, pvlTmplGrp)); // should fail
}

