#include <nlohmann/json.hpp>

#include "PvlToJSON.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"

#include "gmock/gmock.h"

using json = nlohmann::json;
using namespace Isis;


TEST(PvlToJSONTest, KeywordConversion) {
  PvlKeyword testKey1("TestKey1", "A");
  PvlKeyword testKey2("TestKey2", "1");
  testKey2 += "2";

  json testJson1 = pvlKeywordToJSON(testKey1);
  json testJson2 = pvlKeywordToJSON(testKey2);

  EXPECT_EQ(testJson1["Value"], testKey1[0]);
  EXPECT_EQ(testJson2["Value"][0], testKey2[0]);
  EXPECT_EQ(testJson2["Value"][1], testKey2[1]);
}


TEST(PvlToJSONTest, KeywordCommentConversion) {
  PvlKeyword testKey1("TestKey1", "This keyword has 1 comment");
  testKey1.addComment("Test comment");
  PvlKeyword testKey2("TestKey2", "This keyword has multiple comments");
  testKey2.addComment("First comment");
  testKey2.addComment("Second comment");
  PvlKeyword testKey3("TestKey3", "This keywords has no comments");

  json testJson1 = pvlKeywordToJSON(testKey1);
  json testJson2 = pvlKeywordToJSON(testKey2);
  json testJson3 = pvlKeywordToJSON(testKey3);

  EXPECT_EQ(testJson1["Comment"], testKey1.comment(0));
  EXPECT_EQ(testJson2["Comment"][0], testKey2.comment(0));
  EXPECT_EQ(testJson2["Comment"][1], testKey2.comment(1));
  EXPECT_FALSE(testJson3.contains("Comment"));
}


TEST(PvlToJSONTest, KeywordUnitConversion) {
  PvlKeyword testKey1("TestKey1", "1", "m");
  PvlKeyword testKey2("TestKey2", "2", "m");
  testKey2.addValue("Hello World");
  testKey2.addValue("3.14", "r");
  PvlKeyword testKey3("TestKey3", "2");
  testKey3.addValue("Hello World");
  testKey3.addValue("3.14");

  json testJson1 = pvlKeywordToJSON(testKey1);
  json testJson2 = pvlKeywordToJSON(testKey2);
  json testJson3 = pvlKeywordToJSON(testKey3);

  EXPECT_EQ(testJson1["Units"], testKey1.unit(0));
  EXPECT_EQ(testJson2["Units"][0], testKey2.unit(0));
  EXPECT_EQ(testJson2["Units"][1], testKey2.unit(1));
  EXPECT_EQ(testJson2["Units"][2], testKey2.unit(2));
  EXPECT_FALSE(testJson3.contains("Units"));
}


TEST(PvlToJSONTest, GroupConversion) {
  PvlKeyword testKey1("TestKey1", "A");
  PvlKeyword testKey2("TestKey2", "1");
  PvlGroup testGroup("TestGroup");
  testGroup += testKey1;
  testGroup += testKey2;

  json testJson = pvlGroupToJSON(testGroup);

  EXPECT_TRUE(testJson.contains(testKey1.name()));
  EXPECT_TRUE(testJson.contains(testKey2.name()));
}


TEST(PvlToJSONTest, GroupRepeatedKeysConversion) {
  PvlKeyword testKey1("TestKey2", "1");
  PvlKeyword testKey2(testKey1.name(), "2");
  PvlGroup testGroup("TestGroup");
  testGroup += testKey1;
  testGroup += testKey2;

  json testJson = pvlGroupToJSON(testGroup);

  EXPECT_TRUE(testJson.contains(testKey2.name()));
  EXPECT_EQ(testJson[testKey1.name()].size(), 2);
  EXPECT_EQ(testJson[testKey1.name()][0]["Value"], testKey1[0]);
  EXPECT_EQ(testJson[testKey2.name()][1]["Value"], testKey2[0]);
}


TEST(PvlToJSONTest, GroupCommentConversion) {
  PvlGroup testGroup1("TestGroup1");
  testGroup1.addComment("Test comment");
  PvlGroup testGroup2("TestGroup2");
  testGroup2.addComment("First Comment");
  testGroup2.addComment("Second Comment");
  PvlGroup testGroup3("TestGroup3");

  json testJson1 = pvlGroupToJSON(testGroup1);
  json testJson2 = pvlGroupToJSON(testGroup2);
  json testJson3 = pvlGroupToJSON(testGroup3);

  EXPECT_EQ(testJson1["Comment"], testGroup1.comment(0));
  EXPECT_EQ(testJson2["Comment"][0], testGroup2.comment(0));
  EXPECT_EQ(testJson2["Comment"][1], testGroup2.comment(1));
  EXPECT_FALSE(testJson3.contains("Comment"));
}


TEST(PvlToJSONTest, ObjectConversion) {
  PvlKeyword testKey1("TestKey1", "A");
  PvlKeyword testKey2("TestKey2", "1");
  PvlKeyword testKey3("TestKey3", "hello world");
  PvlGroup testGroup("TestGroup");
  testGroup += testKey1;
  testGroup += testKey2;
  PvlObject testObject("TestObject");
  testObject += testGroup;
  testObject += testKey3;

  json testJson = pvlObjectToJSON(testObject);

  EXPECT_TRUE(testJson.contains(testGroup.name()));
  EXPECT_TRUE(testJson.contains(testKey3.name()));
}


TEST(PvlToJSONTest, ObjectNestedConversion) {
  PvlKeyword testKey1("TestKey1", "A");
  PvlKeyword testKey2("TestKey2", "1");
  PvlKeyword testKey3("TestKey3", "hello world");
  PvlObject testObject1("TestObject1");
  testObject1 += testKey1;
  testObject1 += testKey2;
  PvlObject testObject2("TestObject2");
  testObject2 += testObject1;
  testObject2 += testKey3;

  json testJson = pvlObjectToJSON(testObject2);

  EXPECT_TRUE(testJson.contains(testKey3.name()));
  EXPECT_TRUE(testJson.contains(testObject1.name()));
  EXPECT_TRUE(testJson[testObject1.name()].contains(testKey1.name()));
  EXPECT_TRUE(testJson[testObject1.name()].contains(testKey2.name()));
}


TEST(PvlToJSONTest, ObjectRepeatedConversion) {
  PvlKeyword testKey1("TestKey1", "A");
  PvlKeyword testKey2("TestKey2", "1");
  PvlGroup testGroup(testKey1.name());
  testGroup += testKey2;
  PvlObject testObject("TestObject");
  testObject += testGroup;
  testObject += testKey1;
  testObject += testKey2;

  json testJson = pvlObjectToJSON(testObject);

  EXPECT_TRUE(testJson.contains(testKey1.name()));
  EXPECT_TRUE(testJson.contains(testKey2.name()));
  EXPECT_EQ(testJson[testKey1.name()].size(), 2);
  EXPECT_TRUE(testJson[testKey1.name()][0].contains("Value"));
  EXPECT_TRUE(testJson[testKey1.name()][1].contains(testKey2.name()));
}


TEST(PvlToJSONTest, ObjectCommentConversion) {
  PvlObject testObject1("TestObject1");
  testObject1.addComment("Test comment");
  PvlObject testObject2("TestObject2");
  testObject2.addComment("First Comment");
  testObject2.addComment("Second Comment");
  PvlObject testObject3("TestObject3");

  json testJson1 = pvlObjectToJSON(testObject1);
  json testJson2 = pvlObjectToJSON(testObject2);
  json testJson3 = pvlObjectToJSON(testObject3);

  EXPECT_EQ(testJson1["Comment"], testObject1.comment(0));
  EXPECT_EQ(testJson2["Comment"][0], testObject2.comment(0));
  EXPECT_EQ(testJson2["Comment"][1], testObject2.comment(1));
  EXPECT_FALSE(testJson3.contains("Comment"));
}


TEST(PvlToJSONTest, PvlConversion) {
  PvlKeyword testKey1("TestKey1", "A");
  PvlKeyword testKey2("TestKey2", "1");
  PvlKeyword testKey3("TestKey3", "hello world");
  PvlKeyword testKey4("TestKey4", "3.14");
  PvlObject testObject("TestObject");
  testObject += testKey1;
  testObject += testKey2;
  PvlGroup testGroup("TestGroup");
  testGroup += testKey3;
  Pvl testPvl;
  testPvl += testObject;
  testPvl += testGroup;
  testPvl += testKey4;

  json testJson = pvlToJSON(testPvl);

  EXPECT_TRUE(testJson.contains(testGroup.name()));
  EXPECT_TRUE(testJson.contains(testObject.name()));
  EXPECT_TRUE(testJson.contains(testKey4.name()));
}
