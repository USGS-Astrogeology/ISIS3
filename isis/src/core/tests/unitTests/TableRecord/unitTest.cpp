/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>

#include "Buffer.h"
#include "IException.h"
#include "Preference.h"
#include "PvlGroup.h"
#include "TableField.h"
#include "TableRecord.h"

using namespace std;
using namespace Isis;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << "Testing Isis::TableRecord" << endl;
  TableField f1("One", TableField::Integer);
  TableField f2("Two", TableField::Double);
  TableField f3("Three", TableField::Text, 50);
  TableField f4("Four", TableField::Real);

  TableRecord rec;
  rec += f1;
  rec += f2;
  rec += f3;
  rec += f4;


  cout << "Fields      = " << rec.Fields() << endl;
  cout << "Record size = " << rec.RecordSize() << endl;

  cout << "-----" << endl;
  cout << "testing pack" << endl;

  char buf[66];
  cout << "Packing..." << endl;
  rec.Pack(buf);
  cout << "Unpacking..." << endl;
  rec.Unpack(buf);
  for(int i = 0; i <= 3; i++) {
    TableField &f = rec[i];
    PvlGroup g = f.pvlGroup();
    cout << g << endl;
  }

  cout << "-----" << endl;
  TableField &f = rec[2];
  PvlGroup g = f.pvlGroup();
  cout << g << endl;

  cout << "-----" << endl;
  f = rec["TWO"];
  g = f.pvlGroup();
  cout << g << endl;

  cout << "-----" << endl;
  try {
    f = rec["Five"];
  }
  catch(IException &e) {
    e.print();
  }
}

