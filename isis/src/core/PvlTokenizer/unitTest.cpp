/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <sstream>
#include "PvlToken.h"
#include "PvlTokenizer.h"
#include "IException.h"
#include "Preference.h"

using namespace Isis;
using namespace std;

int main(void) {
  Preference::Preferences(true);

//*****************************************************************************
// Create Instances of the Tokenizer
//*****************************************************************************

  PvlTokenizer tizer;

//*****************************************************************************
// Create a stream and load it
//*****************************************************************************

  stringstream os;
  os << "DOG=POODLE "
     << "CAT=\"TABBY\" "
     << "BIRD=(PARROT) \0"
     << "REPTILE={SNAKE,LIZARD} \t"
     << "-VEGGIE \n"
     << " "
     << "    BOVINE    =   (   COW  ,  CAMEL  ) \n  "
     << "TREE = {   \"MAPLE\"   ,\n \"ELM\" \n, \"PINE\"   }"
     << "FLOWER = \"DAISY & \nTULIP \""
     << "# This is a comment\n"
     << "/* This is another comment\n"
     << "BIG = (\"  NOT  \",\"REALLY LARGE\")\n"
     << "SEQUENCE = ((a,b,c), (d,e,f))"
     << "QUOTED_STRING=\"A QUOTED STRING\""
     << "QuotedNewLine=\"abcd\nefgh \nijk\n lmn\""
     << "ApostNewLine=\'abcd\nefgh \nijk\n lmn\'";

  try {
    tizer.Load(os);
  }
  catch(IException &e) {
    e.print();
  }

  vector<PvlToken> t = tizer.GetTokenList();

  cout << "TESTING TOKENIZER" << endl;
  int i, j;
  for(i = 0; i < (int)t.size(); i++) {
    cout << t[i].key() << " is ";
    for(j = 0; j < t[i].valueSize(); j++) {
      cout << t[i].value(j) << " ";
    }
    cout << endl;
  }
  cout << endl;

  cout << "TESTING TOKENIZER CLEAR" << endl;
  tizer.Clear();
  vector<PvlToken> t2 = tizer.GetTokenList();
  cout << t2.size() << endl << endl;

  stringstream os2;
  os2 << "PHRASE = \"The quick brown fox jumped over the lazy dog";
  cout << "TESTING TOKEN ERROR [" << os2.str() << "]" << endl;
  try {
    tizer.Load(os2);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  stringstream os3;
  os3 << "PHRASE = {To Be or Not To Be That is the Question";
  cout << "TESTING TOKEN ERROR [" << os3.str() << "]" << endl;
  try {
    tizer.Load(os3);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  stringstream os4;
  os4 << "PHRASE = (I came, I saw, I conquered";
  cout << "TESTING TOKEN ERROR [" << os4.str() << "]" << endl;
  try {
    tizer.Load(os4);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  stringstream os5;
  os5 << "FOOD = (\"french\",\"fries,\"good\") ";
  cout << "TESTING TOKEN ERROR [" << os5.str() << "]" << endl;
  try {
    tizer.Load(os5);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  stringstream os6;
  os6 << "FOOD = (\"burgers\",\"hotdogs,\"good\")";
  cout << "TESTING TOKEN ERROR [" << os6.str() << "]" << endl;
  try {
    tizer.Load(os6);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  stringstream os7;
  os7 << "FOOD = (\"pickels,pizza\")";
  cout << "TESTING TOKEN ERROR [" << os7.str() << "]" << endl;
  try {
    tizer.Load(os7);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  stringstream os8;
  os8 << "FISH = (\"trout\",\"pizz\"a)";
  cout << "TESTING TOKEN ERROR [" << os8.str() << "]" << endl;
  try {
    tizer.Load(os8);
  }
  catch(IException &e) {
    e.print();
  }
  cout << endl;

  return 0;
}
