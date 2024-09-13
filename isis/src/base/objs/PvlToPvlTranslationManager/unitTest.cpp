/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <sstream>
#include "PvlToPvlTranslationManager.h"
#include "Preference.h"
#include "IException.h"
#include "IString.h"
#include "Preference.h"

using namespace Isis;
using namespace std;

int main(void) {
  Preference::Preferences(true);

  try {

    stringstream fStrm;
    fStrm << "^IMAGE = (SomeFile.dat, 32)" << endl;
    fStrm << "Object = Image" << endl;
    fStrm << "  Group = Size" << endl;
    fStrm << "    NL = 500" << endl;
    fStrm << "    NS = 400" << endl;
    fStrm << "    NB = (27,12,1)" << endl;
    fStrm << "  EndGroup" << endl;
    fStrm << "  Group = Pixel" << endl;
    fStrm << "    Bits = 16" << endl;
    fStrm << "    Signed = True" << endl;
    fStrm << "    Architecture = Sun" << endl;
    fStrm << "    Resolution = 100<meters>" << endl;
    fStrm << "  EndGroup" << endl;
    fStrm << "  Object = BandInfo" << endl;
    fStrm << "    Band = (r,g,b)" << endl;
    fStrm << "    Center = (2, 8, 1.9)" << endl;
    fStrm << "  EndObject" << endl;
    fStrm << "EndObject" << endl;

    fStrm << "OBJECT = QUBE" << endl;
    fStrm << "  GROUP = IMAGE_MAP_PROJECTION" << endl;
    fStrm << "    MAP_PROJECTION_TYPE = SINUSOIDAL" << endl;
    fStrm << "    A_AXIS_RADIUS = 1737.4000000" << endl;
    fStrm << "    CENTER_LONGITUDE = 44.4975624" << endl;
    fStrm << "  END_GROUP = IMAGE_MAP_PROJECTION" << endl;
    fStrm << "END_OBJECT = QUBE" << endl;
    fStrm << "End" << endl;
    Pvl fLabel;
    fStrm >> fLabel;

    stringstream trnsStrm;
    trnsStrm << "Group = DataFileName" << endl;
    trnsStrm << "  InputKey = ^IMAGE" << endl;
    trnsStrm << "  Translation = (*,*)" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = ImageStartByte" << endl;
    trnsStrm << "  InputKey = ^IMAGE" << endl;
    trnsStrm << "  InputDefault = 1" << endl;
    trnsStrm << "  Translation = (*,*)" << endl;
    trnsStrm << "EndGroup" << endl;
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
    trnsStrm << "  Auto" << endl;
    trnsStrm << "  Optional" << endl;
    trnsStrm << "  InputPosition = (Image,Bogus)" << endl;
    trnsStrm << "  InputKey = Extra" << endl;
    trnsStrm << "  Translation = (*,*)" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = BytesPerPixel" << endl;
    trnsStrm << "  Auto" << endl;
    trnsStrm << "  InputPosition = Size" << endl;
    trnsStrm << "  InputPosition = (Image,Size)" << endl;
    trnsStrm << "  InputPosition = (Image,Pixel)" << endl;
    trnsStrm << "  InputKey = Bits" << endl;
    trnsStrm << "  InputDefault = 8" << endl;
    trnsStrm << "  OutputName = PixelBytes" << endl;
    trnsStrm << "  OutputPosition = Root" << endl;
    trnsStrm << "  Translation = (1,8)" << endl;
    trnsStrm << "  Translation = (2,16)" << endl;
    trnsStrm << "  Translation = (4,32)" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = PixelResolution" << endl;
    trnsStrm << "  InputPosition = (Image,Pixel)" << endl;
    trnsStrm << "  InputKey = Resolution" << endl;
    trnsStrm << "  InputDefault = 1" << endl;
    trnsStrm << "  Translation = (*,*)" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = Sign" << endl;
    trnsStrm << "  InputPosition = (Image,Pixel)" << endl;
    trnsStrm << "  InputKey = Signed" << endl;
    trnsStrm << "  InputDefault = True" << endl;
    trnsStrm << "  Translation = (True,True)" << endl;
    trnsStrm << "  Translation = (False,False)" << endl;
    trnsStrm << "  Translation = (True,Yes)" << endl;
    trnsStrm << "  Translation = (False,No)" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = BandName" << endl;
    trnsStrm << "  Auto" << endl;
    trnsStrm << "  OutputName = Band" << endl;
    trnsStrm << "  OutputPosition = (\"Object\",\"IsisCube\",";
    trnsStrm <<                      "\"Object\",\"BandBin\")" << endl;
    trnsStrm << "  InputPosition = (Image,BandInfo)" << endl;
    trnsStrm << "  InputKey = Band" << endl;
    trnsStrm << "  Translation = (Red,r)" << endl;
    trnsStrm << "  Translation = (Green,g)" << endl;
    trnsStrm << "  Translation = (Blue,b)" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = BadGroup" << endl;
    trnsStrm << "  InputPosition = (Bad1,Bad2,Bad3)" << endl;
    trnsStrm << "  InputKey = BadKey" << endl;
    trnsStrm << "  InputDefault = 1" << endl;
    trnsStrm << "  Translation = (*,*)" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = GoodGroupBadKey" << endl;
    trnsStrm << "  InputPosition = (Image,Pixel)" << endl;
    trnsStrm << "  InputKey = BadKey" << endl;
    trnsStrm << "  InputDefault = 1" << endl;
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

    PvlToPvlTranslationManager transMgr(fLabel, trnsStrm);

    cout << "Testing PvlToPvlTranslationManager object" << endl;

    cout << "  Testing InputValue member" << endl;
    cout << "    DataFileName    = " << transMgr.InputKeyword("DataFileName")[0] << endl;
    cout << "    StartByte       = " << transMgr.InputKeyword("ImageStartByte")[1] << endl;
    cout << "    NumberOfBands   = " << transMgr.InputKeyword("NumberOfBands")[0] << endl;
    cout << "    PixelResolution = " << transMgr.InputKeyword("PixelResolution")[0] << endl;
    cout << "    Error messages:" << endl;
    try {
      transMgr.InputKeyword("BadGroup");
    }
    catch(IException &e) {
      cerr << "    ";
      e.print();
    }
    try {
      transMgr.InputKeyword("GoodGroupBadKey");
    }
    catch(IException &e) {
      cerr << "    ";
      e.print();
    }
    cout << endl;

    cout << "  Testing InputUnits member" << endl;
    cout << "    PixelResolution = " << transMgr.InputKeyword("PixelResolution").unit() << endl;
    cout << "    NumberOfBands   = " << transMgr.InputKeyword("NumberOfBands").unit() << endl;
    cout << "    Error messages:" << endl;
    try {
      transMgr.InputKeyword("BadGroup").unit();
    }
    catch(IException &e) {
      cerr << "    ";
      e.print();
    }
    try {
      transMgr.InputKeyword("GoodGroupBadKey").unit();
    }
    catch(IException &e) {
      cerr << "    ";
      e.print();
    }
    cout << endl;

    cout << "  Testing InputSize member" << endl;
    cout << "    BandName        = " << transMgr.InputKeyword("BandName").size() << endl;
    cout << "    PixelResolution = " << transMgr.InputKeyword("PixelResolution").size() << endl;
    cout << "    Error messages:" << endl;
    try {
      transMgr.InputKeyword("BadGroup").size();
    }
    catch(IException &e) {
      cerr << "    ";
      e.print();
    }
    try {
      transMgr.InputKeyword("GoodGroupBadKey").size();
    }
    catch(IException &e) {
      cerr << "    ";
      e.print();
    }
    cout << endl;

    cout << "  Testing InputHasKeyword member" << endl;
    cout << "    BandName        = " << transMgr.InputHasKeyword("BandName") << endl;
    cout << "    GoodGroupBadKey = " << transMgr.InputHasKeyword("GoodGroupBadKey") << endl;
    cout << endl;

//    cout << "  Testing InputHasGroup member" << endl;
//    cout << "    BandName = " << transMgr.InputHasGroup ("BandName") << endl;
//    cout << "    BadGroup = " << transMgr.InputHasGroup ("BadGroup") << endl;
//    cout << endl;

    cout << "  Testing Translate member" << endl;
    cout << "    DataFileName = " << transMgr.Translate("DataFileName").toStdString() << endl;
    cout << "    ImageStartByte   = " << transMgr.Translate("ImageStartByte", 1).toStdString() << endl;
    cout << "    Lines = " << transMgr.Translate("NumberOfLines").toStdString() << endl;
    cout << endl;

    cout << "  Testing Auto member" << endl;
    Pvl pvl;
    transMgr.Auto(pvl);
    cout << pvl << endl;


  }
  catch(IException &e) {
    e.print();
  }

  return 0;
}
