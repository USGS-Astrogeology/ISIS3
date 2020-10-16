/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <sstream>
#include <iostream>
#include "PvlGroup.h"
#include "PvlTokenizer.h"
#include "Preference.h"
#include "IException.h"

using namespace std;
using namespace Isis;

int main() {
  Isis::Preference::Preferences(true);

  Isis::PvlKeyword dog("DOG", toString(5.2), "meters");
  Isis::PvlKeyword cat("CATTLE");
  cat = "Meow";
  cat.addComment("Cats shed");

  Isis::PvlGroup ani("Animals");
  ani += dog;
  ani += cat;
  ani.addComment("/* Pets are cool");

  cout << ani << endl;

  cout << (double) ani["dog"] << endl << endl;

  ani -= "dog";
  cout << ani << endl << endl;

  ani -= ani[0];
  cout << ani << endl << endl;

  stringstream os;
  os << "# Testing" << endl
     << "/* 123 */" << endl
     << "Group=POODLE " << endl
     << "CAT=\"TABBY\" " << endl
     << "BIRD=(PARROT) \0" << endl
     << "REPTILE={SNAKE,LIZARD} \t" << endl
     << " "
     << "    BOVINE    =   (   COW  ,  CAMEL  ) \n  "
     << "TREE = {   \"MAPLE\"   ,\n \"ELM\" \n, \"PINE\"   }" << endl
     << "FLOWER = \"DAISY & \nTULIP \""
     << "# This is a comment\n"
     << "/* This is another comment */\n"
     << "BIG = (\"  NOT  \",\"REALLY LARGE\")" << endl
     << "EndGroup" << endl;

  PvlGroup g;
  os >> g;
  cout << g << endl;

  try {
    stringstream os2;
    os2 << "# Testing" << endl
        << "/* 123 */" << endl
        << "Group=POODLE " << endl
        << "CAT=\"TABBY\" " << endl
        << "BIRD=(PARROT) \0" << endl
        << "REPTILE={SNAKE,LIZARD} \t" << endl
        << " "
        << "    BOVINE    =   (   COW  ,  CAMEL  ) \n  "
        << "TREE = {   \"MAPLE\"   ,\n \"ELM\" \n, \"PINE\"   }" << endl
        << "FLOWER = \"DAISY & \nTULIP \""
        << "# This is a comment\n"
        << "/* This is another comment */\n"
        << "BIG = (\"  NOT  \",\"REALLY LARGE\")" << endl;

    PvlGroup g2;
    os2 >> g2;
    cout << g2 << endl;
  }
  catch(IException &e) {
    cout.flush();
    e.print();
  }
  
  // Validate Group

  // Template Group
  PvlGroup pvlTmplGrp("Point_ErrorMagnitude");
  PvlKeyword pvlTmplKwrd("Point_ErrorMagnitude__Required", "false");
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
  pvlTmplKwrd.clear();
  cout << "Template Group:\n" << pvlTmplGrp << endl << endl;
  
  // PvlGroup to be Validated
  PvlGroup pvlGrp("Point_errormagnitude");
  PvlKeyword pvlKwrd("LessThan", toString(2.5));
  
  try {
    pvlTmplGrp.validateGroup(pvlGrp);
  } 
  catch (IException &e) {
    cerr << "\n**Test1**RequiredKeyword\nResults Group:\n" << pvlGrp << endl;
    cerr << "**PVL ERROR** Required Keyword \"GreaterThan\" not found in the PvlGroup\n";
    cerr << "**********\n";
  }
  
  // Test Repeated values
  try {
    pvlKwrd.clear();
    PvlKeyword pvlKwrd("LessThan", toString(2.5));
    pvlGrp += pvlKwrd;
    
    pvlKwrd.clear();
    pvlKwrd = PvlKeyword("GreaterThan", toString(3.5));
    pvlGrp += pvlKwrd;
    
    pvlKwrd.clear();
    pvlKwrd = PvlKeyword("GreaterThan", toString(4.4545));
    pvlGrp += pvlKwrd;
    
    pvlKwrd.clear();
    pvlKwrd = PvlKeyword("GreaterThan", toString(100.8988095));
    pvlGrp += pvlKwrd;
    pvlTmplGrp.validateGroup(pvlGrp);
    
    cout << "\n**Test2**\nRepeated values are allowed if Repeat flag is set\n";
    cout << "Results Group:\n" << pvlGrp << endl;
    cerr << "**********\n";
  } 
  catch (IException &e) {
    e.print();
  } 
  
  // Test for unvalidated elements
  try {
    pvlKwrd.clear();
    PvlKeyword pvlKwrd("Less123Than", toString(2.5));
    pvlGrp += pvlKwrd;
    
    pvlKwrd.clear();
    pvlKwrd = PvlKeyword("GreaterThan", toString(3.5));
    pvlGrp += pvlKwrd;

    pvlTmplGrp.validateGroup(pvlGrp);
    
    cout << "\n**Test3**\nUnvalidated Keywords\n";
    cout << "Results Group:\n" << pvlGrp << endl;
    cerr << "**********\n";
  } 
  catch (IException &e) {
    e.print();
  } 
  
}
