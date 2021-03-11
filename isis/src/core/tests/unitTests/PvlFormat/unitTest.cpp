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

