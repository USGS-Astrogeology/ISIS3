#include "PvlKeyword.h"
#include "PvlSequence.h"
#include "IException.h"

#include "TestUtilities.h"
#include "Fixtures.h"

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
using ::testing::HasSubstr;

void comparePvlKeywords(PvlKeyword pvlKeyword1, PvlKeyword pvlKeyword2);

TEST_F(RawPvlKeywords, ReadKeywords)
{
	int results_idx = 0;
	for (unsigned int key = 0; key < sizeof(keywordsToTry) / sizeof(QString); key++)
	{
		vector<QString> keywordComments;
		QString keywordName;
		vector<pair<QString, QString>> keywordValues;

		bool result = false;

		try
		{
			result = PvlKeyword::readCleanKeyword(keywordsToTry[key],
												 keywordComments,
												 keywordName,
												 keywordValues);
		}
		catch (IException &e)
		{
			result = false;
			continue;
		}

		EXPECT_EQ(result, valid[key]);

		if (result != valid[key]) {
			cout << "UNEXCEPTED PARSE RESULT FOR: \"" << keywordsToTry[key] << "\"" << endl;
			cout << "EXPECTED: " << valid[key] << endl;
			cout << "GOT: " << result << endl;
		}

		if (result)
		{
			PvlKeyword keyword;
			keyword.setName(keywordName);
			keyword.addComments(keywordComments);

			for (unsigned int value = 0; value < keywordValues.size(); value++)
			{
				keyword.addValue(keywordValues[value].first,
								 keywordValues[value].second);
			}
			comparePvlKeywords(results[results_idx], keyword);
			results_idx += 1;
		}
	}
}

TEST_F(RawPvlKeywords, StreamReadKeywords)
{
	int results_idx = 0;
	for (unsigned int key = 0; key < sizeof(keywordsToTry) / sizeof(QString); key++)
	{
		stringstream stream;
		PvlKeyword keyword;

		bool result = false;
		try {
			stream.write(keywordsToTry[key].toLatin1().data(), keywordsToTry[key].size());
			stream >> keyword;
			result = true;
		}
		catch (IException &e) {
			continue;
		}

		ASSERT_EQ(result, valid[key]);
		if (result) {
			comparePvlKeywords(results[results_idx], keyword);
			results_idx += 1;
		}
	}
}

TEST(PvlKeyword, CheckParsing) {
	const Isis::PvlKeyword keyL("FROM",
															"/archive/projects/cassini/VIMS/UnivAZraw/tour/S60/cubes/GLO000OBMAP002//V1654449360_4.QUB");
	PvlKeyword keyLRead;
	stringstream streamL;
	streamL << keyL;
	streamL >> keyLRead;

	comparePvlKeywords(keyL, keyLRead);

	const Isis::PvlKeyword keyN("THE_INTERNET",
															"Seven thousand eight hundred forty three million seventy four nine seventy six forty two eighty nine sixty seven thirty five million jillion bajillion google six nine four one two three four five six seven eight nine ten eleven twelve thirteen fourteen",
															"terrabytes");
	PvlKeyword keyNRead;
	stringstream streamN;
	streamN << keyN;
	streamN >> keyNRead;

	comparePvlKeywords(keyN, keyNRead);

	const Isis::PvlKeyword keyZ("BIG_HUGE_LONG_NAME_THAT_SHOULD_TEST_OUT_PARSING",
															"Seven thousand eight hundred forty three million seventy four",
															"bubble baths");
	PvlKeyword keyZRead;
	stringstream streamZ;
	streamZ << keyZ;
	streamZ >> keyZRead;

	comparePvlKeywords(keyZ, keyZRead);

	Isis::PvlKeyword keyU("ARRAY_TEST", toString(5.87), "lightyears");
	keyU.addValue("5465.6", "lightyears");
	keyU.addValue("574.6", "lightyears");

	PvlKeyword keyURead;
	stringstream streamU;
	streamU << keyU;
	streamU >> keyURead;

	comparePvlKeywords(keyU, keyURead);

	const Isis::PvlKeyword keyV("FIRST_100_DIGITS_OF_PI",
															"3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679");
	PvlKeyword keyVRead;
	stringstream streamV;
	streamV << keyV;
	streamV >> keyVRead;

	comparePvlKeywords(keyV, keyVRead);

	const Isis::PvlKeyword keyJ("A", 
															"XXXXXXXXXXxxxxxxxxxxXXXXXXXXXXxxxxxxxxxxXXXXXXXXXXxxxxxxxxxxXXXXXXXXXXxxxx");
	PvlKeyword keyJRead;
	stringstream streamJ;
	streamJ << keyJ;
	streamJ >> keyJRead;

	comparePvlKeywords(keyJ, keyJRead);

	string keyB = "TREE = {   \"MAPLE\"   ,\n \"ELM\" \n, \"PINE\"   }";
	PvlKeyword pvlKeyB("TREE");
	pvlKeyB.addValue("MAPLE");
	pvlKeyB.addValue("ELM");
	pvlKeyB.addValue("PINE");
	PvlKeyword keyBRead;
	stringstream streamB;
	streamB << keyB;
	streamB >> keyBRead;

	comparePvlKeywords(pvlKeyB, keyBRead);

	Isis::PvlKeyword keyW("UGHHHHHHHHHHHH");
	keyW += toString(59999.0);
	keyW += toString(59999.0);
	keyW += toString(59999.0);
	keyW += toString(59999.0);
	keyW += toString(59999.0);
	keyW += toString(59999.0);
	keyW += toString(59999.0);
	keyW += toString(59999.0);
	keyW += toString(59999.0);
	keyW += toString(59999.0);
	keyW += toString(59999.0);
	keyW += toString(59999.0);

	PvlKeyword keyWRead;
	stringstream streamW;
	streamW << keyW;
	streamW >> keyWRead;

	comparePvlKeywords(keyW, keyWRead);
}

TEST(PvlKeyword, UnitContructor) {
	const Isis::PvlKeyword key("NAME", "5.2", "meters");

	EXPECT_EQ(key.name(), "NAME");
	EXPECT_EQ(key[0], "5.2");
	EXPECT_EQ(key.unit(0), "meters");
}

TEST(PvlKeyword, WrappingComment) {
	Isis::PvlKeyword key("KEY");

	key += "5";
	key += QString("");
	key.addValue("3.3", "feet");
	key.addValue("Hello World!");
	QString str = "Hello World! This is a really really long comment that needs to"
								" be wrapped onto several different lines to make the PVL file "
								"look really pretty!";
	key.addCommentWrapped(str);

	EXPECT_EQ(key.name(), "KEY");
	EXPECT_EQ(key[0], "5");
	EXPECT_EQ(key[1], "");
	EXPECT_EQ(key[2], "3.3");
	EXPECT_EQ(key.unit(2), "feet");
	EXPECT_EQ(key[3], "Hello World!");
	EXPECT_EQ(key.comment(0), "# Hello World! This is a really really long comment that needs to be");
	EXPECT_EQ(key.comment(1), "# wrapped onto several different lines to make the PVL file look really");
	EXPECT_EQ(key.comment(2), "# pretty!");
}

TEST(PvlKeyword, IndexSetValue)
{
	Isis::PvlKeyword key("KEY");

	key += "5";
	key += QString("");
	key.addValue("3.3", "feet");
	key.addValue("Hello World!");

	key[1] = toString(88);
	EXPECT_EQ(key[1], "88");
}

TEST(PvlKeyword, PvlSequence) {
	PvlKeyword truthKeyword("key");
	truthKeyword.addValue("(a, b, c)");
	truthKeyword.addValue("(\"Hubba Hubba\", Bubba)");

	Isis::PvlSequence seq;
	seq += "(a,b,c)";
	seq += "(\"Hubba Hubba\",\"Bubba\")";
	Isis::PvlKeyword k("key");
	k = seq;
	cout << truthKeyword << endl;
	cout << k << endl;
	comparePvlKeywords(truthKeyword, k);
}

TEST(PvlKeyword, SetUnitsIndividual) {
	PvlKeyword k;
	k = Isis::PvlKeyword("k", "radius", "meters");
	k.addValue("circumference", "meters");
	
	k.setUnits("circumference", "Fathoms");
	EXPECT_EQ(k.unit(1), "Fathoms");
}

TEST(PvlKeyword, SetUnitsMultiple) {
	PvlKeyword k;
	k = Isis::PvlKeyword("k", "radius", "meters");
	k.addValue("circumference", "meters");
	k.setUnits("TeraFathoms");
	EXPECT_EQ(k.unit(0), "TeraFathoms");
	EXPECT_EQ(k.unit(1), "TeraFathoms");
}

TEST(PvlKeyword, QStringCast) {
	PvlKeyword cast("cast", "I'm being casted");

	EXPECT_EQ((QString)cast, "I'm being casted");
}

TEST(PvlKeyword, IntCast) {
	PvlKeyword cast("cast", "I'm being casted");

	EXPECT_EQ((QString)cast, "I'm being casted");
}

TEST(PvlKeyword, BigIntCast) {
	PvlKeyword cast("cast", "465721");

	EXPECT_EQ((int)cast, 465721);
	EXPECT_EQ((Isis::BigInt)cast, 465721);
}

TEST(PvlKeyword, DoubleCast) {
	PvlKeyword cast("cast", "131.2435");

	EXPECT_EQ((double)cast, 131.2435);
}

TEST(PvlKeyword, MiscTest1) {
	try {
		PvlKeyword key(" Test_key_2 ", "Might work");
		PvlKeyword key2("Bob is a name", "Yes it is");
	}
	catch(IException &e) {
		EXPECT_THAT(e.what(), HasSubstr("Keyword name cannot contain whitespace."));
	}
}

TEST(PvlKeyword, MiscTest2) {
	try {
    PvlKeyword key(" Test_key_3 ", "Might'not work");
	}
	catch(IException &e) {
		FAIL() << "Unable to create pvlKeyword: " << e.what() << std::endl;
	}
}

TEST(PvlKeyword, KeywordValidationPass)
{
	PvlKeyword pvlTmplKwrd("KeyName", "integer");
	PvlKeyword pvlKwrd("KeyName", "3");
	pvlTmplKwrd.validateKeyword(pvlKwrd);
}

TEST(PvlKeyword, KeywordValidationNull)
{
	PvlKeyword pvlTmplKwrd("KeyName", "integer");
	PvlKeyword pvlKwrd("KeyName", "null");
	pvlTmplKwrd.validateKeyword(pvlKwrd);
}

TEST(PvlKeyword, KeywordValidationFail) {
  try {
    PvlKeyword pvlTmplKwrd("KeyName", "integer");
		PvlKeyword pvlKwrd("KeyName", toString(3.5));
    pvlTmplKwrd.validateKeyword(pvlKwrd);
  } 
  catch(Isis::IException &e) {
		EXPECT_THAT(e.what(), HasSubstr("Failed to convert string [3.5] to an integer"));
  }
}

TEST(PvlKeyword, KeywordValidationPositive) {
  try {
		PvlKeyword pvlTmplKwrd("KeyName", "integer");
		PvlKeyword pvlKwrd("KeyName", toString(-3));
		pvlTmplKwrd.validateKeyword(pvlKwrd, "positive");
	} 
  catch(Isis::IException &e) {
		EXPECT_THAT(e.what(), HasSubstr("has invalid value"));
	}
}

TEST(PvlKeyword, KeywordValidationRange) {
  try {
		PvlKeyword pvlTmplKwrd("KeyName", "integer");
		PvlKeyword pvlTmplKwrdRange("KeyName__Range", toString(0));
		pvlTmplKwrdRange.addValue(toString(10));
		PvlKeyword pvlKwrd("KeyName", toString(11));
		pvlTmplKwrd.validateKeyword(pvlKwrd, "", &pvlTmplKwrdRange);
	} 
  catch(Isis::IException &e) {
		EXPECT_THAT(e.what(), HasSubstr("is not in the specified Range"));
	}
}

TEST(PvlKeyword, KeywordValidationString) {
  try {
    PvlKeyword pvlTmplKwrd("KeyName", "string");
    PvlKeyword pvlTmplKwrdValue("KeyName__Value", "value0");
    pvlTmplKwrdValue.addValue("value1");
    pvlTmplKwrdValue.addValue("value2");
    pvlTmplKwrdValue.addValue("value3");
    PvlKeyword pvlKwrd("KeyName", "VALUe3");
    pvlTmplKwrd.validateKeyword(pvlKwrd, "", &pvlTmplKwrdValue);
    pvlKwrd.clear();

    pvlKwrd=PvlKeyword("KeyName", "value");
    pvlTmplKwrd.validateKeyword(pvlKwrd, "", &pvlTmplKwrdValue);
	} 
  catch(Isis::IException &e) {
		EXPECT_THAT(e.what(), HasSubstr("Wrong Type of value in the Keyword"));
	}
}

TEST(PvlKeyword, testJsonAddInt) {
    PvlKeyword keyword("Key");
		json jsonobj = {{"Key1", 1}, {"Key2", 2}};
		keyword.addJsonValue(jsonobj["Key1"]);
		keyword.addJsonValue(jsonobj["Key2"]);

		EXPECT_EQ(keyword[0], "1");
		EXPECT_EQ(keyword[1], "2");
}

TEST(PvlKeyword, testJsonAddDouble) {
    PvlKeyword keyword("Key");
		json jsonobj = {{"Key", 1.000000000000001}};
		keyword.addJsonValue(jsonobj["Key"]);

		EXPECT_EQ(keyword[0], "1.000000000000001");
}

TEST(PvlKeyword, testJsonAddBool) {
    PvlKeyword keyword("Key");
		json jsonobj = {{"Key", true}};
		keyword.addJsonValue(jsonobj["Key"]);

		EXPECT_EQ(keyword[0], "true");
}

TEST(PvlKeyword, testJsonAddNull) {
    PvlKeyword keyword("Key");
		json jsonobj = {{"Key", nullptr}};
		keyword.addJsonValue(jsonobj["Key"]);

		EXPECT_EQ(keyword[0], "Null");
}

TEST(PvlKeyword, testJsonAddString) {
    PvlKeyword keyword("Key");
		json jsonobj = {{"Key", "Banana"}};
		keyword.addJsonValue(jsonobj["Key"]);

		EXPECT_EQ(keyword[0], "Banana");
}

TEST(PvlKeyword, testJsonSet) {
    PvlKeyword keyword("Key", "1");
		json jsonobj = {{"Key", 2}};

		keyword.setJsonValue(jsonobj["Key"]);

		EXPECT_EQ(keyword[0], "2");
}

void comparePvlKeywords(PvlKeyword pvlKeyword1, PvlKeyword pvlKeyword2)
{
	EXPECT_TRUE(PvlKeyword::stringEqual(pvlKeyword1.name(), pvlKeyword2.name()));

	ASSERT_EQ(pvlKeyword1.comments(), pvlKeyword2.comments());
	for (unsigned int comment = 0; comment < (unsigned int)pvlKeyword1.comments(); comment++)
	{
		EXPECT_TRUE(PvlKeyword::stringEqual(pvlKeyword1.comment(comment), pvlKeyword2.comment(comment)));
	}

	ASSERT_EQ(pvlKeyword1.size(), pvlKeyword2.size());
	for (unsigned int value = 0; value < (unsigned int)pvlKeyword1.size(); value++)
	{
		EXPECT_TRUE(PvlKeyword::stringEqual(pvlKeyword1[value], pvlKeyword2[value]));
		EXPECT_TRUE(PvlKeyword::stringEqual(pvlKeyword1.unit(value), pvlKeyword2.unit(value)));
	}
}
