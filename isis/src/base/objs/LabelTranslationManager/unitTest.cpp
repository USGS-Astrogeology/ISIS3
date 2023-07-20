/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <sstream>
#include "LabelTranslationManager.h"
#include "Preference.h"
#include "IException.h"
#include "IString.h"
#include "Preference.h"

using namespace Isis;
using namespace std;
/**
 * Child class of LabelTranslationManager to implement pure virtual methods.
 *
 * @author 2017-01-11 Jeannie Backer
 *
 * @internal
 *  @history 2017-01-11 Jeannie Backer - Original Version. Fixes #4584.
 *  @history 2017-10-16 Kristin Berry - Updated for changes to dependency specification format and
 *                        the addition of xml namspaces. References #5202
 */
class TestTranslationManager : public LabelTranslationManager {
  public:

    TestTranslationManager(const QString &transFile) : LabelTranslationManager() {
    AddTable(transFile);
    }

    TestTranslationManager(std::istream &transStrm) : LabelTranslationManager() {
    AddTable(transStrm);
    }

    ~TestTranslationManager() {
    }

    virtual QString Translate(QString nName, int findex = 0) {
      QString inputValue = "Test Input, Index " + toString(findex);
      return PvlTranslationTable::Translate(nName, inputValue);
    }
};

int main(void) {

  Preference::Preferences(true);
  stringstream trnsStrm;
  trnsStrm << "Group = NumberOfLines" << endl;
  trnsStrm << "  Auto" << endl;
  trnsStrm << "  OutputName = Lines" << endl;
  trnsStrm << "  OutputPosition = (\"Object\",\"IsisCube\",";
  trnsStrm <<                      "\"Group\",\"Dimensions\")" << endl;
  trnsStrm << "  InputPosition = (Image,Size)" << endl;
  trnsStrm << "  InputKey = NL" << endl;
  trnsStrm << "  Translation = (*,*)" << endl;
  trnsStrm << "EndGroup" << endl;
  trnsStrm << "Group = NumberOfBands" << endl;
  trnsStrm << "  Auto" << endl;
  trnsStrm << "  Optional" << endl;
  trnsStrm << "  OutputName = Bands" << endl;
  trnsStrm << "  OutputPosition = (\"Object\",\"IsisCube\",";
  trnsStrm <<                      "\"Group\",\"Dimensions\")" << endl;
  trnsStrm << "  InputPosition = (Image,Size)" << endl;
  trnsStrm << "  InputKey = Nb" << endl;
  trnsStrm << "  InputDefault = 1" << endl;
  trnsStrm << "  Translation = (*,*)" << endl;
  trnsStrm << "EndGroup" << endl;
  trnsStrm << "Group = Bonus" << endl;
  trnsStrm << "  Auto" << endl;
  trnsStrm << "  Optional" << endl;
  trnsStrm << "  InputPosition = (Image,Pixel)" << endl;
  trnsStrm << "  InputKey = Bonus" << endl;
  trnsStrm << "  Translation = (*,*)" << endl;
  trnsStrm << "EndGroup" << endl;
  trnsStrm << "Group = Extra" << endl;
  trnsStrm << "  Optional" << endl;
  trnsStrm << "  InputPosition = (Image,Bogus)" << endl;
  trnsStrm << "  InputKey = Extra" << endl;
  trnsStrm << "  Translation = (*,*)" << endl;
  trnsStrm << "EndGroup" << endl;
  trnsStrm << "Group = PixelResolution" << endl;
  trnsStrm << "  InputPosition = (Image,Pixel)" << endl;
  trnsStrm << "  InputKey = Resolution" << endl;
  trnsStrm << "  InputDefault = 1" << endl;
  trnsStrm << "  Translation = (*,*)" << endl;
  trnsStrm << "EndGroup" << endl;
  trnsStrm << "Group = BandName" << endl;
  trnsStrm << "  Auto" << endl;
  trnsStrm << "  OutputName = Band" << endl;
  trnsStrm << "  OutputPosition = (\"Object\",\"IsisCube\",";
  trnsStrm <<                      "\"Object\",\"BandBin\")" << endl;
  trnsStrm << "  InputPosition = (Image,BandInfo)" << endl;
  trnsStrm << "  InputKey = Band" << endl;
  trnsStrm << "  Translation = (*,*)" << endl;
  trnsStrm << "EndGroup" << endl;
  trnsStrm << "Group = CenterLongitude" << endl;
  trnsStrm << "  Auto" << endl;
  trnsStrm << "  OutputPosition = (\"Group\",\"Mapping\")" << endl;
  trnsStrm << "  OutputName = CenterLongitude" << endl;
  trnsStrm << "  InputPosition = IMAGE_MAP_PROJECTION" << endl;
  trnsStrm << "  InputPosition = (QUBE,IMAGE_MAP_PROJECTION)" << endl;
  trnsStrm << "  InputPosition = (SPECTRAL_QUBE,IMAGE_MAP_PROJECTION)" << endl;
  trnsStrm << "  InputKey = CENTER_LONGITUDE" << endl;
  trnsStrm << "  Translation = (*,*)" << endl;
  trnsStrm << "EndGroup" << endl;

  trnsStrm << "End" << endl;

  TestTranslationManager transMgr(trnsStrm);

  try {
    cout << "Testing LabelTranslationManager object" << endl;

    cout << endl << "Testing Translate method:" << endl;
    cout << endl << "Translating Extra: ";
    cout << transMgr.Translate("Extra") << endl;

    cout << endl << "Testing Auto method:" << endl;
    Pvl translatedLabel;
    transMgr.Auto(translatedLabel);
    cout << endl << translatedLabel << endl;
  }
  catch(IException &e) {
    e.print();
  }
  try {
    cout << endl << "Testing parseSpecification method: att@name|value" << endl;
    transMgr.parseSpecification((QString)"att@name:value");

    cout << endl << "Testing parseSpecification method: tag@name|value" << endl;
    transMgr.parseSpecification((QString)"tag@name|value");

    cout << endl << "Testing parseSpecification method: att@name" << endl;
    transMgr.parseSpecification((QString)"att@name");

    cout << endl << "Testing parseSpecification method: new@name" << endl;
    transMgr.parseSpecification((QString)"new@name");

    cout << endl << "Testing parseSpecification method: name|value" << endl;
    transMgr.parseSpecification((QString)"name|value");

    cout << endl << "Testing parseSpecification method: value" << endl;
    transMgr.parseSpecification((QString)"value");

    cout << endl << "Testing parseSpecification method: namespace:name" << endl;
    transMgr.parseSpecification((QString)"namespace:name");

    cout << endl << "Testing parseSpecification method: namespace:name|value" << endl;
    transMgr.parseSpecification((QString)"namespace:name|value");

    cout << endl << "Testing parseSpecification method: att@namespace:name|value" << endl;
    transMgr.parseSpecification((QString)"att@namepsace:name|value");

    cout << endl << "Testing parseSpecification method: tag@name|value" << endl;
    transMgr.parseSpecification((QString)"tag@namespace:name|value");
  }
  catch(IException &e) {
    e.print();
  }


  try {
    cout << endl << "Testing parseSpecification method: att|name|value" << endl;
    transMgr.parseSpecification((QString)"att|name|value");
  }
  catch(IException &e) {
    e.print();
  }


  try {
    cout << endl << "Testing parseSpecification method: att@name@value" << endl;
    transMgr.parseSpecification((QString)"att@name@value");
  }
  catch(IException &e) {
    e.print();
  }

  try {
    cout << endl << "Testing parseSpecification method: not@name|value" << endl;
    transMgr.parseSpecification((QString)"not@name|value");
  }
  catch(IException &e) {
    e.print();
  }

  try {
    cout << endl << "Testing parseSpecification method: att@name|value1|value2" << endl;
    transMgr.parseSpecification((QString)"att@name|value1|value2");
  }
  catch(IException &e) {
    e.print();
  }
  return 0;
}
