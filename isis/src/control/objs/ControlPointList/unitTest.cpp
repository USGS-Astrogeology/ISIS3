/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ControlPointList.h"

#include <string>
#include <iostream>

#include "IException.h"
#include "FileName.h"
#include "Preference.h"

using namespace Isis;
using namespace std;

int main() {
  Isis::Preference::Preferences(true);
  Isis::ControlPointList cpl(Isis::FileName("points.lis")); //list of Control Point Ids in the file

  int size = cpl.Size();

  //print point ids in the list
  for(int i = 0; i < size; i++) {
    std::cerr << cpl.ControlPointId(i).toStdString() << "\n";
  }

  // index out of range
  try {
    std::cerr << cpl.ControlPointId(size).toStdString() << "\n";
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    std::cerr << cpl.ControlPointIndex("new0007") << "\n";
    std::cerr << cpl.ControlPointIndex("new0036") << "\n";
    std::cerr << cpl.ControlPointIndex("new0000") << "\n"; //not found - invalid point
  }
  catch(Isis::IException &e) {
    e.print();
  }
}
