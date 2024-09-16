/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "DbProfile.h"
#include "Preference.h"

using namespace std;
using namespace Isis;

int main(int argc, char *argv[]) {
  Isis::Preference::Preferences(true);

  DbProfile p("test profile");

  cerr << "Valid: " << p.isValid() << endl;
  cerr << "Size: " << p.size() << endl;
  p.setName("test profile");
  cerr << "Name: " << p.Name().toStdString() << endl;
  cerr << "Adding a key..." << endl;
  p.add("foo", "bar");
  cerr << "Valid: " << p.isValid() << endl;
  cerr << "Size: " << p.size() << endl;
  cerr << "Count: " << p.count("foo") << endl;
  cerr << "Exists: " << p.exists("boo") << endl;
  cerr << "Exists: " << p.exists("foo") << endl;
  cerr << "() operator: " << p("foo").toStdString() << endl;
  cerr << "Value: " << p.value("foo", 0).toStdString() << endl;

  cerr << "Test getting non-existing key BadKey\n";
  cerr.flush();
  try {
    cerr << "BadKey =";
    cerr << p("BadKey").toStdString() << endl;
  }
  catch(IException &ie) {
    ie.print();
  }

  return 0;
}
