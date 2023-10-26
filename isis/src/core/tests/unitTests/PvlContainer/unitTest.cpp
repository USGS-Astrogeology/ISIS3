/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>

#include "IString.h"

#include "PvlContainer.h"
#include "Preference.h"

using namespace Isis;
using namespace std;

int main() {
  Preference::Preferences(true);

  PvlKeyword dog("DOG", std::to_string(5.2), "meters");
  PvlKeyword cat("CATTLE");
  cat = "Meow";
  cat.addComment("Cats shed");

  PvlContainer ani("Animals");
  ani += dog;
  ani += cat;
  ani.addComment("/* Pets are cool */");

//  cout << "1 ..." << endl;
  cout << ani << endl;

//  cout << "2 ..." << endl;
  cout << (double) ani["dog"] << endl;

  ani -= "dog";
//  cout << "3 ..." << endl;
  cout << ani << endl;

//  cout << "4 ..." << endl;
  ani -= ani[0];
  cout << ani << endl;

  cout << "Test inserter ..." << endl;
  PvlKeyword monkey("Orangutan", "gross");
  ani.addKeyword(dog, ani.begin());
  ani.addKeyword(monkey, ani.begin());
  cout << ani << endl;

  // This is testing reallocation caused by adds,
  // was a problem when using std::vector
  cout << endl << "Test reallocation ..." << endl;
  PvlKeyword * ptr1 = &ani["DOG"];
  for (int i = 0; i < 250; i++) 
    ani += PvlKeyword("Test_keyword", std::to_string(i));

  PvlKeyword * ptr2 = &ani["DOG"];
  if (ptr1 == ptr2) 
    cout << "Pointer to DOG is equivalent" << endl;
  else
    cout << "FAILURE: Pointer to DOG changed after multiple adds" << endl; 

}
