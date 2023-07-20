/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "PvlKeyword.h"
#include "PvlGroup.h"
#include "PvlObject.h"
#include "PvlFormat.h"
#include "Preference.h"

using namespace std;
int main() {
  Isis::Preference::Preferences(true);

  // Test keywords
  {
    Isis::PvlKeyword key("mystring", "stringval");
    cout << key << endl;
  }

  {
    Isis::PvlKeyword key("mystring", "string val");
    cout << key << endl;
  }

  {
    Isis::PvlKeyword key("myint", "12345");
    cout << key << endl;
  }

  {
    Isis::PvlKeyword key("myfloat", "12345.67e+89");
    cout << key << endl;
  }

  {
    Isis::PvlKeyword key("myarray", "(12345, \"a short string\", 1.234)");
    cout << key << endl;
  }


  {
    Isis::PvlKeyword key("myarray", "{12345, \"a short string\", 1.234}");
    cout << key << endl;
  }

  // Test Groups
  {
    Isis::PvlGroup grp("Group1");
    grp += Isis::PvlKeyword("mystring", "stringval");
    grp += Isis::PvlKeyword("mystring", "string val");
    grp += Isis::PvlKeyword("myint", "12345");
    grp += Isis::PvlKeyword("myfloat", "12345.67e+89");
    grp += Isis::PvlKeyword("myarray", "(12345,\"a short string\",1.234)");
    cout << grp << endl;
  }


  // Test Objects
  {
    Isis::PvlGroup grp("Group1");
    grp += Isis::PvlKeyword("mystring", "stringval");
    grp += Isis::PvlKeyword("mystring", "string val");
    grp += Isis::PvlKeyword("myint", "12345");
    grp += Isis::PvlKeyword("myfloat", "12345.67e+89");
    grp += Isis::PvlKeyword("myarray", "(12345,\"a short string\",1.234)");
    Isis::PvlObject obj("Object1");
    obj.addGroup(grp);

    Isis::PvlObject obj2("Object2");
    obj2 += Isis::PvlKeyword("mystring", "stringval");
    obj2 += Isis::PvlKeyword("mystring", "string val");
    obj2 += Isis::PvlKeyword("myint", "12345");
    obj2 += Isis::PvlKeyword("myfloat", "12345.67e+89");
    obj2 += Isis::PvlKeyword("myarray", "(12345,\"a short string\",1.234)");
    obj.addObject(obj2);
    cout << obj << endl;
  }

  {
    Isis::PvlKeyword key("myequation", "(f1-f2) * 5");
    cout << key << endl;
  }

  {
    Isis::PvlKeyword key("myequation", "(f1-f2)*5");
    cout << key << endl;
  }


  {
    Isis::PvlKeyword key("mysequence");
    key += "(a,b)";
    key += "(c,d)";
    cout << key << endl;
  }

  {
    Isis::PvlKeyword key("mycommas");
    key += ",,,";
    cout << key << endl;
  }
}

