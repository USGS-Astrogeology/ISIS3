#include <iostream>

#include "Preference.h"
#include "IException.h"

using namespace Isis;
using namespace std;

int main() {
  try {
    // Note: Normally a call to Preferences in a unitTest would be called
    // with a true. In this case however, we can't have the Preferences
    // call clearing the the Pvl everytime. We aren't using any keywords
    // from the preferences anyway, just the ones we add.
    Isis::Preference::Preferences(false);
    Isis::Pvl system;
    Isis::PvlGroup j1("Junk");
    j1 += Isis::PvlKeyword("Fruit", "Tomato");
    j1 += Isis::PvlKeyword("Vegetable", "Potato");
    system.addGroup(j1);
    system.write("tmpSystem");

    Isis::Pvl user;
    Isis::PvlGroup j2("Junk");
    j2 += Isis::PvlKeyword("Vegetable", "Potatoe");
    user.addGroup(j2);
    user.write("tmpUser");

    cout << "Testing normally" << endl;
#if 0
    Isis::Preference p2;
    p2.Load("tmpSystem");
    cout << p2 << endl;
    p2.Load("tmpUser");
    cout << p2 << endl;
#endif

    Isis::Preference::Preferences(false).Load("tmpSystem");
    Isis::PvlGroup j3 = Isis::Preference::Preferences(false).findGroup("Junk");
    cout << j3 << endl;
    Isis::Preference::Preferences(false).Load("tmpUser");
    Isis::PvlGroup j4 = Isis::Preference::Preferences(false).findGroup("Junk");
    cout << j4 << endl;

    cout << endl << endl;
    //  if (p2.hasGroup("Junk")) {
    if(Isis::Preference::Preferences(false).hasGroup("Junk")) {
      Isis::PvlGroup &g = Isis::Preference::Preferences(false).findGroup("Junk");
      cout << "Value of Vegetable is : " << (QString) g["Vegetable"] << endl;
    }

    remove("tmpSystem");
    remove("tmpUser");
  }
  catch(Isis::IException &error) {
    error.print();
  }

  // Now test without a preference file to see if the expected exception is thrown
  try {
    Isis::Preference::Preferences(true);
    Isis::Preference::Preferences(true).Load("dummyPrefs");
  }
  // Expect to see: "The preference file dummyPrefs was not found or does not exist."
  catch(Isis::IException &error) {
    error.print();
  }

  return 0;
}
