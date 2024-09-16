/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "DbAccess.h"
#include "Preference.h"

using namespace std;
using namespace Isis;

int main(int argc, char *argv[]) {
  Isis::Preference::Preferences(true);

  DbAccess d;
  DbProfile p("test profile");

  cout << "Profile count: " << d.profileCount() << endl;
  cout << "Adding a profile..." << endl;
  d.addProfile(p);
  cout << "Profile exist: " << d.profileExists("test profile") << endl;
  cout << "Profile count: " << d.profileCount() << endl;
  cout << "Default profile name: " << d.getDefaultProfileName().toStdString() << endl;
  DbProfile dup = d.getProfile(0);
  cout << "Duplicate profile name: " << dup.Name().toStdString() << endl;

  cout << "DbProfile valid: " << p.isValid() << endl;
  cout << "Size: " << d.size() << endl;
  cout << "Setting name: ";
  d.setName("new name");
  cout << d.Name().toStdString() << endl;
  cout << "Adding a key test_key..." << endl;
  cout << "Exists (before): " << d.exists("test_key") << endl;
  d.add("test_key", "test value");
  cout << "Exists (after): " << d.exists("test_key") << endl;
  cout << "Size: " << d.size() << endl;
  cout << "Count: " << d.count("test_key") << endl;
  cout << "Key: " << d.key(0).toStdString() << endl;
  cout << "Value: " << d.value("test_key").toStdString() << endl;
  cout << "() operator: " << d("test_key").toStdString() << endl;

  try {
    d.getProfile(99);
  }
  catch(IException &e) {
    e.print();
  }

  try {
    d.load("/tmp/not_a_file");
  }
  catch(IException &e) {
    e.print();
  }

  return 0;
}
