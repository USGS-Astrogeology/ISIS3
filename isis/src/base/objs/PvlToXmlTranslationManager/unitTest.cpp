/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <sstream>

#include <QString>

#include "IException.h"
#include "IString.h"
#include "Preference.h"

#include "PvlToXmlTranslationManager.h"


using namespace Isis;
using namespace std;

int main(void) {
  Preference::Preferences(true);

  QDomDocument *QDomDoc;

  QDomDoc = new QDomDocument("XML_LABEL");
  QDomElement root = QDomDoc->createElement("Product_Observational");
  QDomDoc->appendChild(root);

  try {
    stringstream fStrm;
    fStrm << "Object = IsisCube" << endl;
    fStrm << "  Object = Core" << endl;
    fStrm << "    Group = Dimensions" << endl;
    fStrm << "     Samples = 2048" << endl;
    fStrm << "     Lines   = 255" << endl;
    fStrm << "     Bands   = 1" << endl;
    fStrm << "    EndGroup" << endl;
    fStrm << "    Group = Pixel" << endl;
    fStrm << "      Type       = SignedWord" << endl;
    fStrm << "      ByteOrder  = Lsb" << endl;
    fStrm << "      Base       = 0.0" << endl;
    fStrm << "      Multiplier = 1.0" << endl;
    fStrm << "    EndGroup" << endl;
    fStrm << "  EndObject" << endl;
    fStrm << "  Group = Instrument" << endl;
    fStrm << "    SpacecraftId              = TGO" << endl;
    fStrm << "    ExposureDuration          = 1.920e-003 <seconds>" << endl;
    fStrm << "    InstrumentId              = CaSSIS" << endl;
    fStrm << "    Filter                    = NIR" << endl;
    fStrm << "  EndGroup" << endl;
    fStrm << "EndObject" << endl;

    fStrm << "End" << endl;
    Pvl fLabel;
    fStrm >> fLabel;

    ofstream trnsStrm;
    trnsStrm.open ("temp.trn");
    trnsStrm << "Group = Samples" << endl;
    trnsStrm << " Auto" << endl;
    trnsStrm << " InputKey        = Samples" <<endl;
    trnsStrm << " InputPosition   = (IsisCube, Core, Dimensions)" <<endl;
    trnsStrm << " OutputName      = elements" <<endl;
    trnsStrm << " OutputSiblings  = axis_name|Sample" <<endl;
    trnsStrm << " OutputName      = elements" <<endl;
    trnsStrm << " OutputPosition  = (Product_Observational, File_Area_Observational, Array_2D_Image, new@Axis_Array)" <<endl;
    trnsStrm << " Translation = (*,*)" << endl;
    trnsStrm << "EndGroup" << endl;

    trnsStrm << "Group = Lines" << endl;
    trnsStrm << " Auto" << endl;
    trnsStrm << " InputKey        = Lines" <<endl;
    trnsStrm << " InputPosition   = (IsisCube, Core, Dimensions)" <<endl;
    trnsStrm << " OutputName      = elements" <<endl;
    trnsStrm << " OutputSiblings  = axis_name|Line" <<endl;
    trnsStrm << " OutputName      = elements" <<endl;
    trnsStrm << " OutputPosition  = (Product_Observational, File_Area_Observational, Array_2D_Image, new@Axis_Array)" <<endl;
    trnsStrm << " Translation = (*,*)" << endl;
    trnsStrm << "EndGroup" << endl;

    trnsStrm << "Group = ExposureDuration" << endl;
    trnsStrm << " Auto" << endl;
    trnsStrm << "  InputKey = ExposureDuration" << endl;
    trnsStrm << "  InputPosition = (IsisCube, Instrument)" << endl;
    trnsStrm << "  OutputName = att@Exposure_Time" << endl;
    trnsStrm << "  OutputPosition  = (Product_Observational, CaSSIS_Header, PEHK_HEADER)" << endl;
    trnsStrm << "  Translation = (*,*)" << endl;
    trnsStrm << "EndGroup" << endl;

    trnsStrm << "Group = AttributeOutput" << endl;
    trnsStrm << " Auto" << endl;
    trnsStrm << "  InputKey = Filter" << endl;
    trnsStrm << "  InputPosition = (IsisCube, Instrument)" << endl;
    trnsStrm << "  OutputName = Filter" << endl;
    trnsStrm << "  OutputAttributes = Form|Acronym" << endl;
    trnsStrm << "  OutputPosition  = (Product_Observational, CaSSIS_Header, DERIVED_HEADER_DATA)" << endl;
    trnsStrm << "  Translation = (*,*)" << endl;
    trnsStrm << "EndGroup" << endl;

    trnsStrm << "Group = DefaultOutput" << endl;
    trnsStrm << " Auto" << endl;
    trnsStrm << "  InputKey = NotInLabel" << endl;
    trnsStrm << "  InputPosition = (IsisCube, Instrument)" << endl;
    trnsStrm << "  OutputName = NotInLabel" << endl;
    trnsStrm << "  InputDefault = DefaultValue" << endl;
    trnsStrm << "  OutputPosition  = (Product_Observational, CaSSIS_Header, DERIVED_HEADER_DATA)" << endl;
    trnsStrm << "  Translation = (*,*)" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "End" << endl;

    trnsStrm << "Group = BadInputGroup" << endl;
    trnsStrm << " Auto" << endl;
    trnsStrm << "  InputKey = NotInLabel" << endl;
    trnsStrm << "  InputPosition = (IsisCube, NotAGroup)" << endl;
    trnsStrm << "  OutputName = BadInputGroup" << endl;
    trnsStrm << "  InputDefault = DefaultValue" << endl;
    trnsStrm << "  OutputPosition  = (Product_Observational, CaSSIS_Header, DERIVED_HEADER_DATA)" << endl;
    trnsStrm << "  Translation = (*,*)" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "End" << endl;

    trnsStrm.close();

    PvlToXmlTranslationManager transFileMgr("temp.trn");
    try {
      cout << "  Testing Setting Input Label" << endl;
      transFileMgr.SetLabel(fLabel);
    }
    catch (IException &e) {
      e.print();
    }
    try {
      cout << "  Testing Translating Something" << endl;
      cout << transFileMgr.InputKeyword("AttributeOutput") << endl;
    }
    catch (IException &e) {
      e.print();
    }

    PvlToXmlTranslationManager transMgr(fLabel, "temp.trn");
    try {
      cout << "  Testing InputKeyword" << endl;
      cout << transMgr.InputKeyword("ExposureDuration") << endl;
    }
    catch (IException &e) {
      e.print();
    }

    try {
      cout << "  Testing Bad InputKeyword" << endl;
      cout << transMgr.InputKeyword("BAD") << endl;
    }
    catch (IException &e) {
      e.print();
      cout << endl;
    }

    try {
      cout << "  Testing Translate" << endl;
      cout << transMgr.Translate("Samples").toStdString() << endl;
    }
    catch (IException &e) {
      e.print();
    }

    try {
      cout << "  Testing Bad Translate" << endl;
      cout << transMgr.Translate("BAD").toStdString() << endl;
    }
    catch (IException &e) {
      e.print();
      cout << endl;
    }

    try {
      cout << "  Testing InputHasKeyword True" << endl;
      cout << transMgr.InputHasKeyword("Lines") << endl;
    }
    catch (IException &e) {
      e.print();
    }

    try {
      cout << "  Testing InputHasKeyword False" << endl;
      cout << transMgr.InputHasKeyword("False") << endl;
    }
    catch (IException &e) {
      e.print();
      cout << endl;
    }

    try {
      cout << "  Testing bad InputKeyword, does not exist in group" << endl;
      cout << transMgr.InputKeyword("DefaultOutput") << endl;
    }
    catch (IException &e) {
      e.print();
      cout << endl;
    }

    try {
      cout << "  Testing bad InputKeyword, group does not exist" << endl;
      cout << transMgr.InputKeyword("BadInputGroup") << endl;
    }
    catch (IException &e) {
      e.print();
      cout << endl;
    }

    try {
      cout << "  Testing SetLabel" << endl;
      transMgr.SetLabel(fLabel);
      cout << "  SetLabel Succesful" << endl << endl;
    }
    catch (IException &e) {
      e.print();
    }

    try {
      cout << "  Testing Auto member" << endl;
      transMgr.Auto(*QDomDoc);
      cout << QDomDoc->toString().toStdString() << endl;
      cout << endl;
    }
    catch(IException &e) {
      e.print();
    }
  }
  catch(IException &e) {
    e.print();
  }
  remove("temp.trn");
}
