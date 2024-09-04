/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include "Endian.h"
#include "IString.h"
#include "Preference.h"

using namespace Isis;
using namespace std;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << "Unit test for IsisEndian" << endl;

  if(IsLittleEndian() == true) {
    cout << IsLittleEndian() << endl;
    cout << IsBigEndian() << endl;
  }
  else {
    cout << IsBigEndian() << endl;
    cout << IsLittleEndian() << endl;
  }

  if(IsLsb() == true) {
    cout << IsLsb() << endl;
    cout << IsMsb() << endl;
  }
  else {
    cout << IsMsb() << endl;
    cout << IsLsb() << endl;
  }

  cout << ByteOrderName(ByteOrderEnumeration("msb")).toStdString() << endl;
  cout << ByteOrderName(ByteOrderEnumeration("lsb")).toStdString() << endl;
}
