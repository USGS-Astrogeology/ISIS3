#include <iostream>

#include "Message.h"
#include "Preference.h"
#include "IString.h"

using namespace Isis;
using namespace std;

int main(void) {
  Preference::Preferences(true);


//***************************************************************************
  cout << Message::ArraySubscriptNotInRange(100000)
       << endl;

  cout << endl;

//***************************************************************************

  cout << Message::KeywordAmbiguous("KEY")
       << endl;

  cout << Message::KeywordUnrecognized("KEY")
       << endl;

  cout << Message::KeywordDuplicated("KEY")
       << endl;

  cout << Message::KeywordNotArray("KEY")
       << endl;

  cout << Message::KeywordNotFound("KEY")
       << endl;

  cout << endl;

//***************************************************************************

  cout << Message::KeywordBlockInvalid("BLOCK")
       << endl;

  cout << Message::KeywordBlockStartMissing("BLOCK", "FOUND")
       << endl;

  cout << Message::KeywordBlockEndMissing("BLOCK", "FOUND")
       << endl;

  cout << endl;

//***************************************************************************

  cout << Message::KeywordValueBad("KEY")
       << endl;

  cout << Message::KeywordValueBad("KEY", "12345678901234567890")
       << endl;

  cout << Message::KeywordValueBad("KEY", "abcdefghijklmnopqrstuvwxyz")
       << endl;

  cout << Message::KeywordValueExpected("KEY")
       << endl;

  cout << Message::KeywordValueNotInRange("KEY", "0", "(0,20]")
       << endl;

  vector<QString> list;
  list.push_back("X");
  list.push_back("Y");
  list.push_back("Z");
  cout << Message::KeywordValueNotInList("KEY", "A", list)
       << endl;

  cout << endl;

//***************************************************************************

  cout << Message::MissingDelimiter(')')
       << endl;

  cout << Message::MissingDelimiter(')', "12345678901234567890")
       << endl;

  cout << Message::MissingDelimiter(')', "abcdefghijklmnopqrstuvwxyz")
       << endl;

  cout << endl;

//***************************************************************************

  cout << Message::FileOpen("test.dat") << endl;
  cout << Message::FileCreate("test.dat") << endl;
  cout << Message::FileRead("test.dat") << endl;
  cout << Message::FileWrite("test.dat") << endl;

  cout << endl;
}
