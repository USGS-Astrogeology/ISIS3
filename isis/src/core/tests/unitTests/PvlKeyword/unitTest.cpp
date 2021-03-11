#include "PvlKeyword.h"
#include "IException.h"
#include "PvlSequence.h"
#include "Preference.h"

#include <iostream>
#include <sstream>

using namespace std;
using namespace Isis;

int main() {
  Isis::Preference::Preferences(true);

  QString keywordsToTry[] = {
    "KEYWORD",
    "KEYWORD X",
    "KEYWORD =",
    "KEYWORD = SOME_VAL",
    "KEYWORD = \"  val  \"",
    "KEYWORD = \" 'val' \"",
    "KEYWORD = (VAL",
    "KEYWORD = (VAL1,VAL2",
    "KEYWORD = (A B,C,D)",
    "KEYWORD = ((A B),(C),(D",
    "KEYWORD = (SOME_VAL)",
    "KEYWORD = (SOME_VAL) <a>",
    "KEYWORD=(SOME_VAL)<a>",
    "KEYWORD = (A, )",
    "KEYWORD = ()",
    "KEYWORD = (A,B)",
    "KEYWORD = {A, B}",
    "KEYWORD = (A,B) #comment this",
    "KEYWORD = ( A , B )",
    "KEYWORD\t=\t( A\t,\tB )",
    "KEYWORD = (A, B,C,D,E))",
    "KEYWORD = ((1, 2), {3,  4}, (5), 6)",
    "KEYWORD = { \"VAL1\" ,   \"VAL2\", \"VAL3\"}",
    "KEYWORD = { \"VAL1\" , \"VAL2\", \"VAL3\")",
    "KEYWORD = { \"VAL1\" ,",
    "KEYWORD = \"(A,B,\"",
    "KEYWORD = ',E)'",
    "KEYWORD = ((1,2))",
    "KEYWORD = (\"(f1+f2)\",\"/(f1-f2)\")",
    "KEYWORD = \"(F1+F2)/(F1-F2)\"",
    "KEYWORD = ( (1,2)  , (A,B) )",
    "KEYWORD = \"(f1 + min(f2,f3))\"",
    "KEYWORD = \"(min(f2,f3) + f1)\"",
    "KEYWORD = \"min(f2,f3) + f1\"",
    "KEYWORD = \"f1 + min(f2,f3)\"",
    "KEYWORD = (A <a>, B <b>, C, D <d>)",
    "KEYWORD = (A <a>, B <b>, C, D <d>) <e>",
    "KEYWORD = ',E) <unit>",
    "KEYWORD = ,E) <unit>",
    "#SOMECOMMENT\nKEYWORD = SOME_VAL",
    "#SOMECOMMENT1\n#SOMECOMMENT2\nKEYWORD = SOME_VAL",
    "//SOMECOMMENT1\n#SOMECOMMENT2\nKEYWORD = SOME_VAL",
    "/*SOMECOMMENT1*/\nKEYWORD = SOME_VAL",
    "KEYWORD = '/*\n*/'",
    "/* SOMECOMMENT1\n  SOMECOMMENT2\nSOMECOMMENT3 */\nKEYWORD = SOME_VAL",
    "/*C1\n\nA\n/*\nC3*/\nKEYWORD = SOME_VAL",
    "/*C1\n/**/\nKEYWORD = SOME_VAL",
    "/*C1\nA/**/\nKEYWORD = SOME_VAL",
    "/*           A            */\n/* B *//*C*/\nKEYWORD = SOME_VAL",
    "/*C1/**/\nKEYWORD = SOME_VAL",
    "/*C1   \n\nA\n\nC3*//*Neato*//*Man*/KEYWORD = (A,B,C) /*Right?\nYes!*/"
    };

  cout << endl << endl;
  cout << "----- Testing Basic Read/Write -----" << endl;
  PvlKeyword keyword;
  for(unsigned int key = 0;
      key < sizeof(keywordsToTry) / sizeof(QString);
      key++) {
    QString keywordCpy = keywordsToTry[key];
    while(keywordCpy.contains("\n")) {
      cout << keywordCpy.mid(0, keywordCpy.indexOf("\n") + 1);
      keywordCpy = keywordCpy.mid(keywordCpy.indexOf("\n") + 1);
    }

    cout << "'" << keywordCpy << "' ";

    for(int pad = keywordCpy.size(); pad < 30; pad++) {
      cout << "-";
    }

    cout << "> ";

    vector< QString > keywordComments;
    QString keywordName;
    vector< pair<QString, QString> > keywordValues;

    bool result = false;

    try {
      result = keyword.readCleanKeyword(keywordsToTry[key],
                                        keywordComments,
                                        keywordName,
                                        keywordValues);

      if(result) cout << "VALID" << endl;
      else cout << "INCOMPLETE" << endl;
    }
    catch(IException &e) {
      cout << "INVALID" << endl;
      cout << "    ";
      cout.flush();
      e.print();
    }

    if(result) {
      for(unsigned int comment = 0; comment < keywordComments.size(); comment++)
        cout << "    COMMENT: " << keywordComments[comment] << endl;

      cout << "    NAME: " << keywordName << endl;

      for(unsigned int value = 0; value < keywordValues.size(); value++) {
        cout << "    VALUE: " << keywordValues[value].first;

        if(!keywordValues[value].second.isEmpty()) {
          cout << " <" << keywordValues[value].second << ">";
        }

        cout << endl;
      }
    }

  }

  cout << endl << endl;
  cout << "----- Testing Stream Read/Write -----" << endl;
  for(unsigned int key = 0;
      key < sizeof(keywordsToTry) / sizeof(QString);
      key ++) {
    stringstream stream;
    PvlKeyword someKey;

    cout << "Input:\n" << keywordsToTry[key] << endl;

    cout << endl << "Output: " << endl;
    try {
      stream.write(keywordsToTry[key].toLatin1().data(), keywordsToTry[key].size());
      stream >> someKey;
      cout << someKey << endl;
    }
    catch(IException &e) {
      e.print();
    }

    cout << endl;
  }

  cout << "----- Testing Difficult Cases Read/Write -----\n";


  try {

    const Isis::PvlKeyword keyL("FROM",
                                "/archive/projects/cassini/VIMS/UnivAZraw/tour/S60/cubes/GLO000OBMAP002//V1654449360_4.QUB");
    PvlKeyword keyLRead;
    stringstream streamL;
    streamL << keyL;
    streamL >> keyLRead;
    cout << keyLRead << endl;

    const Isis::PvlKeyword keyN("THE_INTERNET",
                                "Seven thousand eight hundred forty three million seventy four nine seventy six forty two eighty nine sixty seven thirty five million jillion bajillion google six nine four one two three four five six seven eight nine ten eleven twelve thirteen fourteen",
                                "terrabytes");
    PvlKeyword keyNRead;
    stringstream streamN;
    streamN << keyN;
    streamN >> keyNRead;
    cout << keyNRead << endl;

    const Isis::PvlKeyword keyZ("BIG_HUGE_LONG_NAME_THAT_SHOULD_TEST_OUT_PARSING",
                                "Seven thousand eight hundred forty three million seventy four",
                                "bubble baths");
    PvlKeyword keyZRead;
    stringstream streamZ;
    streamZ << keyZ;
    streamZ >> keyZRead;
    cout << keyZRead << endl;

    Isis::PvlKeyword keyU("ARRAY_TEST", toString(5.87), "lightyears");
    keyU.addValue("5465.6", "lightyears");
    keyU.addValue("574.6", "lightyears");

    PvlKeyword keyURead;
    stringstream streamU;
    streamU << keyU;
    streamU >> keyURead;
    cout << keyURead << endl;

    const Isis::PvlKeyword keyV("FIRST_100_DIGITS_OF_PI", "3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679");
    PvlKeyword keyVRead;
    stringstream streamV;
    streamV << keyV;
    streamV >> keyVRead;
    cout << keyVRead << endl;
    cout << "Raw Data -->" << endl;
    cout << keyVRead[0] << endl << endl;


    const Isis::PvlKeyword keyJ("A", "XXXXXXXXXXxxxxxxxxxxXXXXXXXXXXxxxxxxxxxxXXXXXXXXXXxxxxxxxxxxXXXXXXXXXXxxxx");
    PvlKeyword keyJRead;
    stringstream streamJ;
    streamJ << keyJ;
    streamJ >> keyJRead;
    cout << keyJRead << endl;

    string keyB = "TREE = {   \"MAPLE\"   ,\n \"ELM\" \n, \"PINE\"   }";
    PvlKeyword keyBRead;
    stringstream streamB;
    streamB << keyB;
    streamB >> keyBRead;
    cout << keyBRead << endl;

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
    cout << keyWRead << endl;

    const Isis::PvlKeyword key("NAME", "5.2", "meters");

    cout << key << endl;

    Isis::PvlKeyword key2("KEY");
    cout << key2 << endl;

    key2 += "5";
    key2 += QString("");
    key2.addValue("3.3", "feet");
    key2.addValue("Hello World!");
    QString str = "Hello World! This is a really really long comment that needs to"
                  " be wrapped onto several different lines to make the PVL file "
                  "look really pretty!";
    key2.addCommentWrapped(str);
    cout << key2 << endl;

    cout << key2[1] << endl;
    key2[1] = toString(88);
    cout << key2 << endl;

    Isis::PvlSequence seq;
    cout.flush();
    seq += "(a,b,c)";
    cout.flush();
    seq += "(\"Hubba Hubba\",\"Bubba\")";
    cout.flush();
    Isis::PvlKeyword k("key");
    k = seq;
    cout << k << endl;

    // Test SetUnits methods
    k = Isis::PvlKeyword("k", "radius", "meters");
    k.addValue("circumference", "meters");
    cout << "\n\n"
         << "Test SetUnits methods:\n\n"
         << "  original condition of Keyword k :\n"
         << "    " << k << "\n\n"
         << "  after k.SetUnits(\"circumference\", \"Fathoms\") :\n";
    k.setUnits("circumference", "Fathoms");
    cout << "    " << k << "\n\n"
         << "  after k.SetUnits(\"TeraFathoms\") :\n";
    k.setUnits("TeraFathoms");
    cout << "    " << k << "\n\n\n";

    //Test casting operators
    cout << "----------------------------------------" << endl;
    cout << "Testing cast operators" << endl;
    Isis::PvlKeyword cast01("cast1", "I'm being casted");
    Isis::PvlKeyword cast02("cast2", "465721");
    Isis::PvlKeyword cast03("cast3", "131.2435");
    cout << "string     = " << (QString)cast01 << endl;
    cout << "int     = " << (int)cast02 << endl;
    cout << "BigInt     = " << (Isis::BigInt)cast02 << endl;
    cout << "double     = " << (double)cast03 << endl;

  }
  catch(Isis::IException &e) {
    e.print();
  }
  catch(...) {
    cout << "Unknown error" << endl;
  }

  try {
    Isis::PvlKeyword key(" Test_key_2 ", "Might work");
    cout << key << endl;
    Isis::PvlKeyword key2("Bob is a name", "Yes it is");
  }
  catch(Isis::IException &e) {
    e.print();
  }


  try {
    Isis::PvlKeyword key(" Test_key_3 ", "Might'not work");
    cerr << key << endl;
  }
  catch(Isis::IException &e) {
    e.print();
  }
  
  // Test Validation of PvlKeywords
  // Validate type integer
  try {
    // Template Keyword
    PvlKeyword pvlTmplKwrd("KeyName", "integer");
    PvlKeyword pvlKwrd("KeyName", "3");
    pvlTmplKwrd.validateKeyword(pvlKwrd);
    pvlKwrd.clear();

    pvlKwrd=PvlKeyword("KeyName", "null");
    pvlTmplKwrd.validateKeyword(pvlKwrd);
    pvlKwrd.clear();
    
    pvlKwrd=PvlKeyword("KeyName", toString(3.5));
    pvlTmplKwrd.validateKeyword(pvlKwrd);
  } 
  catch(Isis::IException &e) {
   cerr << "Invalid Keyword Type: Integer Expected" << endl;
  }
  
  // Test keyword__Type
  try {
    PvlKeyword pvlTmplKwrd("KeyName", "integer");
    PvlKeyword pvlKwrd("KeyName", toString(-3));
    pvlTmplKwrd.validateKeyword(pvlKwrd, "positive");
  } catch(Isis::IException &e) {
    cerr <<"Positive number Expected" << endl;
  }
  
  // Test keyword__Type
  try {
    PvlKeyword pvlTmplKwrd("KeyName", "integer");
    PvlKeyword pvlTmplKwrdRange("KeyName__Range", toString(0-10));
    PvlKeyword pvlKwrd("KeyName", toString(11));
    pvlTmplKwrd.validateKeyword(pvlKwrd, "", &pvlTmplKwrdRange);
  } catch(Isis::IException &e) {
    cerr <<"Integer not in the Range. Expected (0-10)" << endl;
  }
  
  // Validate String
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
    cerr << "Invalid Keyword Value: Expected values \"value1\", \"value2\", \"value3\"" << endl;
  }
  
  // Validate Boolean
  try {
    PvlKeyword pvlTmplKwrd("KeyName", "boolean");
    PvlKeyword pvlKwrd("KeyName", "true");
    pvlTmplKwrd.validateKeyword(pvlKwrd);
    pvlKwrd.clear();

    pvlKwrd=PvlKeyword("KeyName", "null");
    pvlTmplKwrd.validateKeyword(pvlKwrd);
    pvlKwrd.clear();
    
    pvlKwrd=PvlKeyword("KeyName", "value");
    pvlTmplKwrd.validateKeyword(pvlKwrd);
  } 
  catch(Isis::IException &e) {
    cerr << "Invalid Keyword Type: Expected  Boolean values \"true\", \"false\", \"null\"" << endl;
  }
}
