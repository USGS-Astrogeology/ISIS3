/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include "IString.h"
#include "CollectorMap.h"
#include "Preference.h"


using namespace std;
using namespace Isis;

class ClassTest {
  public:
    ClassTest(int n = 0) : _n(n) {
#if defined(DEBUG)
      cout << "ClassTest " << _n << " being created!\n";
#endif
    }
    ~ClassTest() {
#if defined(DEBUG)
      cout << "ClassTest " << _n << " being destroyed!\n";
#endif
    }
    ClassTest(const ClassTest &rhs) : _n(rhs._n) {
#if defined(DEBUG)
      cout << "ClassTest " << _n << " being copy constructed!\n";
#endif
    }
    ClassTest &operator=(const ClassTest &rhs) {
      if(&rhs != this) {
#if defined(DEBUG)
        cout << "Copying ClassTest(" << rhs._n << ")\n";
#endif
        _n = rhs._n;
      }
      return (*this);
    }

    int Ident() const {
      return (_n);
    }

  private:
    int _n;
};


class TestCollector {
  public:
    TestCollector() { }
    ~TestCollector() { }

    void addClass(const ClassTest &t) {
      cout << "Adding " << t.Ident() << " to int list\n";
      _list.add(t.Ident(), t);
      cout << "Int list size = " << _list.size() << "\n";

      cout << "Adding " << t.Ident() << " to double list\n";
      _flist.add(t.Ident(), t);
      cout << "Double list size = " << _flist.size() << "\n";

      cout << "Adding " << t.Ident() << " to string list\n";
      _slist.add(IString(t.Ident()), t);
      cout << "IString list size = " << _slist.size() << "\n";

      return;
    }

    ClassTest &find(int n) {
      cout << "Exists in double list?: " << _flist.exists(n) << endl;
      return(_list.get(n));
    }


    ClassTest &find(double n) {
      cout << "Exists in int list?: " << _flist.exists(n) << endl;
      return(_flist.get(n));
    }

    ClassTest &findInString(double n) {
      string sN = IString(n);
      cout << "Exists in int list?: " << _slist.exists(sN) << endl;
      return(_slist.get(sN));
    }

    void report(int n) const {
      bool exists = _list.exists(n);
      const ClassTest &t = _list.get(n);
      exists = _flist.exists(n);
      const ClassTest &ft = _flist.get(n);
      exists = _slist.exists(IString(n));
      cout << "Got element " << t.Ident() << " and " << ft.Ident()
           << " - In string?: " << exists << endl;
      return;
    }

    void reportNth(int i) const {
      const ClassTest &t = _list.getNth(i);
      bool exists = _flist.exists(t.Ident());
      const ClassTest &ft = _flist.getNth(i);
      exists = _slist.exists(IString(t.Ident()));
      cout << "Got element " << t.Ident() << " and " << ft.Ident()
           << " - In string? " << exists << endl;
      return;
    }


  private:
    CollectorMap<int, ClassTest> _list;
    CollectorMap<double, ClassTest, RobustFloatCompare> _flist;
    CollectorMap<IString, ClassTest, NoCaseStringCompare> _slist;

};

int main(int argc, char *argv[]) {
  Isis::Preference::Preferences(true);

  typedef CollectorMap<IString, double, NoCaseStringCompare> NoCaseStrings;
  NoCaseStrings dmap;

  cout << "\nSize of double map = " << dmap.size() << endl;
  dmap.add("two", 2.0);
  dmap.add("one", 1.0);
  cout << "Size of double map = " << dmap.size() << endl;

  cout << "One = " << dmap.get("one") << endl;
  cout << "Two = " << dmap.get("Two") << endl;

  const double &one = dmap.get("one");
  cout << "\nTest Const one = " << one << endl;

  dmap.remove("one");


  cout << "\n\nCreate array of 10 doubles...\n";
  typedef CollectorMap < IString, double *,
          NoCaseStringCompare,
          NoopRemoval,
          DefaultCopy > ArrayMap;
  ArrayMap buffer;
  buffer.add("array", new double[10]);

  cout << "Size of array map = " << buffer.size() << endl;
  cout << "Initializing 0 to 9...\n";
  for(int i = 0 ; i < 10 ; i++) {
    buffer.get("array")[i] = (double) i;
  }


  cout << "\nChecking values...\n";
  double *d = buffer.get("array");
  for(int j = 0 ; j < 10 ; j++) {
    cout << "Array[" << j << "] = " << d[j] << endl;
  }

  cout << "\n\nTest Class copying activity and scoping...\n";
  {
    typedef CollectorMap<int, ClassTest> ClassMap;
    ClassMap ctest1;
    ctest1.add(1, ClassTest(1));
    cout << "Done with 1, doing 2...\n";
    ctest1.add(2, ClassTest(2));
    cout << "Done with 2, removing 1...\n";
    ctest1.remove(1);
    cout << "Removed 1, doing 3...\n";
    ctest1.add(3, ClassTest(3));

    cout << "Get ClassTest 2 via a copy\n";
    ClassTest two = ctest1.get(2);
    cout << "Got it: " << two.Ident() << endl;


    cout << "Get ClassTest 3 via a reference\n";
    ClassTest &three = ctest1.get(3);
    cout << "Got it: " << three.Ident() << endl;


    cout << "\n\nTest CollectorMap copy operation...\n";
    ClassMap cCopy = ctest1;
    cout << "Copy size: " << cCopy.size() << endl;
    cout << "ClassTest 2 copy Ident: " << cCopy.get(2).Ident() << endl;

    cout << "Going out of scope for ClassTest 1\n";
  }

  cout << "\n\nTest ClassTest Pointers...\n";
  typedef CollectorMap < int, ClassTest *,
          SimpleCompare,
          PointerRemoval,
          PointerCopy > PointerMap;
  PointerMap ctest2;
  ctest2.add(4, new ClassTest(4));
  ctest2.add(5, new ClassTest(5));
  ctest2.add(6, new ClassTest(6));
  ctest2.add(7, new ClassTest(7));

  cout << "Remove ClassTest 6\n";
  ctest2.remove(6);

//  Test const interators
  cout << "\n\nTesting Const Iterators...\n";
  PointerMap::CollectorConstIter cIter;
  for(cIter = ctest2.begin() ; cIter != ctest2.end() ; ++cIter) {
    cout << "Const Test Ident = " << cIter->second->Ident() << endl;
  }

//  Test non-const interators and fetch routine
  cout << "\n\nTesting Non-Const Iterators...\n";
  PointerMap::CollectorIter ncIter;
  int i = 0;
  for(ncIter = ctest2.begin() ; ncIter != ctest2.end() ; ++ncIter, i++) {
    cout << "Non-Const Test Ident = " << ncIter->second->Ident() << endl;
    cout << "Nth Test Ident       = " << ctest2.getNth(i)->Ident() << endl;
  }

//  Ok...let's test the pointer copy constructor
  cout << "\n\nTest copying of pointer object...\n";
  PointerMap map2(ctest2);
  cout << "Copied the pointer map, size: " << map2.size() << endl;

// Test internal class usage
  cout << "\n\nTesting internal class usage...\n";
  TestCollector t;
  cout << "Adding Class 10!\n";
  t.addClass(ClassTest(10));
  cout << "\n\nAdding Class 11!\n";
  t.addClass(ClassTest(11));
  t.report(10);
  t.find(11);
  t.reportNth(1);

  //  Test duplicate keys
  cout << "\n\nTesting Duplicate Keys...\n";
  typedef CollectorMap<int, QString> IntStr;
  IntStr dupstr(IntStr::DuplicateKeys);
  dupstr.add(1, "One");
  dupstr.add(1, "One #2");
  dupstr.add(1, "One #3");
  dupstr.add(2, "Two");
  dupstr.add(2, "Two #2");
  dupstr.add(3, "Three");

  cout << "Size of Dup object: " << dupstr.size() << endl;
  cout << "Number Ones:   " << dupstr.count(1) << endl;
  cout << "Number Twos:   " << dupstr.count(2) << endl;
  cout << "Number Threes: " << dupstr.count(3) << endl;
  cout << "Number Fours:  " << dupstr.count(4) << endl;
  IntStr::CollectorConstIter isIter;
  int j = 0;
  for(isIter = dupstr.begin() ; isIter != dupstr.end() ; ++isIter, j++) {
    cout << "IntStr[" << j << "] = {" << isIter->first << ", "
         << isIter->second.toStdString() << "}, Index: " << dupstr.index(isIter->first)
         << endl;
    cout << "Nth Test Ident       = " << dupstr.getNth(j).toStdString() << endl;
  }

  cout <<  "Non-existant Index: " << dupstr.index(4) << endl;


  cout << "\nTerminating...\n";

  return (0);
}
