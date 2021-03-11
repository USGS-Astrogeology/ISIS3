#include <iostream>
#include "PvlToken.h"
#include "IException.h"
#include "Preference.h"

using namespace Isis;
using namespace std;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  PvlToken dog("Dog");
  dog.addValue("drools");

  cout << "Info on dog" << endl;
  cout << "  key:        " << dog.key() << endl;
  cout << "  upperkey:   " << dog.keyUpper() << endl;
  cout << "  valuesize:  " << dog.valueSize() << endl;
  cout << "  value:      " << dog.value() << endl;
  cout << "  uppervalue: " << dog.valueUpper() << endl;
  cout << endl;

  cout << "Adding another value to dog" << endl;
  dog.addValue("wags tail");
  cout << "  valuesize:  " << dog.valueSize() << endl;
  cout << "  value:      " << dog.value(1) << endl;
  cout << "  uppervalue: " << dog.valueUpper(1) << endl;
  cout << endl;

  cout << "Clearing dog values" << endl;
  dog.valueClear();
  cout << "  valuesize:  " << dog.valueSize() << endl;
  cout << endl;

  cout << "Testing Throws in dog" << endl;

  try {
    dog.value(-1);
  }
  catch(IException &e) {
    e.print();
  }

  try {
    dog.value(1);
  }
  catch(IException &e) {
    e.print();
  }

  return 0;
}
