/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>

#include <QString>

#include "IException.h"
#include "IString.h"
#include "ID.h"
#include "Preference.h"

using namespace Isis;
using namespace std;

int main() {
  Preference::Preferences(true);
  cout << "Test One: core test and limit test" << endl;
  try {
    ID pid("ABCD??EFG");
    for(int i = 0; i < 100; i++) {
      QString test = pid.Next();
      if(i % 10 == 0) {
        cout << test.toStdString() << endl;
      }
    }
  }
  catch(IException &e) {
    e.print();
  }

  cout << endl << "Test 2: No '?' test" << endl;
  try {
    ID pid2("Serial");
    for(int i = 0; i < 5; i++) {
      cout << pid2.Next().toStdString() << endl;
    }
  }
  catch(IException &e) {
    e.print();
  }

  cout << endl << "Test 3: Broken replacement string" << endl;
  try {
    ID pid3("Serial??Number??");
    for(int i = 0; i < 5; i++) {
      cout << pid3.Next().toStdString() << endl;
    }
  }
  catch(IException &e) {
    e.print();
  }

  cout << endl << "Test 4: differing start numbers" << endl;
  try {
    ID pid4("Test??", 0);
    for(int i = 0; i < 5; i++) {
      cout << pid4.Next().toStdString() << endl;
    }
  }
  catch(IException &e) {
    e.print();
  }
  return 0;
}
