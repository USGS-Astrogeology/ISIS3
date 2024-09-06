/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iostream>
#include <float.h>

#include <QList>
#include <QString>
#include <QStringList>

#include "ControlNetDiff.h"
#include "FileName.h"
#include "IException.h"
#include "Preference.h"

using namespace std;
using namespace Isis;


int main() {
  Preference::Preferences(true);

  FileName f1("cnet.pvl");
  FileName f2("cnet2.pvl");

  ControlNetDiff diff;

  cout << "Testing no differences...\n\n";
  Pvl results = diff.compare(f1, f1);
  cout << results << endl << endl;

  results = diff.compare(f2, f2);
  cout << results << endl;

  cout << "\n\nTesting differences...\n\n";
  results = diff.compare(f1, f2);
  cout << results << endl;

  cout << "\n\nTesting differences with tolerances...\n\n";
  Pvl diffFile(FileName("cnet.diff").expanded());
  diff.addTolerances(diffFile);
  results = diff.compare(f1, f2);
  cout << results << endl;

  return 0;
}
