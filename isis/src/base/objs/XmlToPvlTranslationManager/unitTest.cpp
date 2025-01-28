/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <sstream>

#include <QDebug>

#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Preference.h"
#include "XmlToPvlTranslationManager.h"

using namespace Isis;
using namespace std;

/** 
 * Unit test for XmlToPvlTranslationManager class
 *  
 * @author ????-??-?? Unknown 
 *  
 *  @internal
 *   @history 2018-06-06 Jeannie Backer - Removed file paths from error message written to
 *                           test output.
 *  
 */
int main(void) {
  Preference::Preferences(true);

  try {
    FileName fLabel("$ISISTESTDATA/isis/src/base/unitTestData/xmlTestLabel.xml");

    stringstream trnsStrm;
    
    trnsStrm << "Group = Version" << endl;
    trnsStrm << "  Auto = 1" << endl;
    trnsStrm << "  Debug = 1" << endl;
    trnsStrm << "  InputPosition = (Identification_Area)" << endl;
    trnsStrm << "  InputKey = version_id" << endl;
    trnsStrm << "  OutputPosition = (group, instrument)" << endl;
    trnsStrm << "  OutputName = Version" << endl;
    trnsStrm << "  Translation = (*,*)" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = Host" << endl;
    trnsStrm << "  Auto = 1" << endl;
    trnsStrm << "  Debug = 1" << endl;
    trnsStrm << "  InputPosition = (Observation_Area, Investigation_Area)" << endl;
    trnsStrm << "  InputKey = Instrument_Host_Id" << endl;
    trnsStrm << "  OutputPosition = (group, instrument)" << endl;
    trnsStrm << "  OutputName = Host" << endl;
    trnsStrm << "  Translation = (*,*)" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = BandWidth" << endl;
    trnsStrm << "  Auto = 1" << endl;
    trnsStrm << "  Debug = 1" << endl;
    trnsStrm << "  InputPosition = (Observation_Area)" << endl;
    trnsStrm << "  InputKey = Science_Facets" << endl;
    trnsStrm << "  InputKeyAttribute = bandwidth" << endl;
    trnsStrm << "  OutputPosition = (group, instrument)" << endl;
    trnsStrm << "  OutputName = BandWidth" << endl;
    trnsStrm << "  Translation = (*,*)" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = SpacecraftName" << endl;
    trnsStrm << "  Auto = 1" << endl;
    trnsStrm << "  Debug = 1" << endl;
    trnsStrm << "  InputPosition = (Observation_Area, Observing_System, Observing_System_Component)" << endl;
    trnsStrm << "  InputKey = name" << endl;
    trnsStrm << "  InputKeyDependencies = tag@type|Spacecraft" << endl;
    trnsStrm << "  OutputPosition = (group, instrument)" << endl;
    trnsStrm << "  OutputName = SpacecraftName" << endl;
    trnsStrm << "  Translation = (*,*)" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = InstrumentId" << endl;
    trnsStrm << "  Auto = 1" << endl;
    trnsStrm << "  Debug = 1" << endl;
    trnsStrm << "  InputPosition = (Observation_Area, Observing_System, Observing_System_Component)" << endl;
    trnsStrm << "  InputKey = name" << endl;
    trnsStrm << "  InputKeyDependencies = tag@type|Instrument" << endl;
    trnsStrm << "  OutputPosition = (group, instrument)" << endl;
    trnsStrm << "  OutputName = InstrumentId" << endl;
    trnsStrm << "  Translation = (*,*)" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = InstrumentIfovWithUnits" << endl;
    trnsStrm << "  InputPosition = (CaSSIS_Header, CaSSIS_General)" << endl;
    trnsStrm << "  InputKey = INSTRUMENT_IFOV" << endl;
    trnsStrm << "  OutputPosition = (group, instrument)" << endl;
    trnsStrm << "  OutputName = InstrumentIfovWithUnits" << endl;
    trnsStrm << "  Translation = (*,*)" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = OnboardImageAcquisitionTimeUTC" << endl;
    trnsStrm << "  Auto = 1" << endl;
    trnsStrm << "  Debug = 1" << endl;
    trnsStrm << "  InputPosition = (CaSSIS_Header, DERIVED_HEADER_DATA)" << endl;
    trnsStrm << "  InputKey = OnboardImageAcquisitionTime" << endl;
    trnsStrm << "  InputKeyDependencies = att@Time_Base|UTC" << endl;
    trnsStrm << "  OutputPosition = (group, instrument)" << endl;
    trnsStrm << "  OutputName = OnboardImageAcquisitionTimeUTC" << endl;
    trnsStrm << "  Translation = (*,*)" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = OnboardImageAcquisitionTimeET" << endl;
    trnsStrm << "  Auto = 1" << endl;
    trnsStrm << "  Debug = 1" << endl;
    trnsStrm << "  InputPosition = (CaSSIS_Header, DERIVED_HEADER_DATA)" << endl;
    trnsStrm << "  InputKey = OnboardImageAcquisitionTime" << endl;
    trnsStrm << "  InputKeyDependencies = att@Time_Base|ET" << endl;
    trnsStrm << "  OutputPosition = (group, instrument)" << endl;
    trnsStrm << "  OutputName = OnboardImageAcquisitionTimeET" << endl;
    trnsStrm << "  Translation = (*,*)" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = CoreBands" << endl;
    trnsStrm << "  Auto = 1" << endl;
    trnsStrm << "  Debug = 1" << endl;
    trnsStrm << "  InputPosition = (Product_Observational, File_Area_Observational," << endl;
    trnsStrm << "                Array_2D_Image, Axis_Array)" << endl;
    trnsStrm << "  InputKeyDependencies = \"tag@axis_name|Band\"" << endl;
    trnsStrm << "  InputKey = elements" << endl;
    trnsStrm << "  InputDefault = 1" << endl;
    trnsStrm << "  OutputPosition = (group, CoreCube)" << endl;
    trnsStrm << "  OutputName = CoreBands" << endl;
    trnsStrm << "  Translation = (*, *)" << endl;
    trnsStrm << "End_Group" << endl;
    trnsStrm << "Group = CoreSamples" << endl;
    trnsStrm << "  Auto = 1" << endl;
    trnsStrm << "  Debug = 1" << endl;
    trnsStrm << "  InputPosition = (Product_Observational, File_Area_Observational," << endl;
    trnsStrm << "                Array_2D_Image, Axis_Array)" << endl;
    trnsStrm << "  InputKeyDependencies = \"tag@axis_name|Sample\"" << endl;
    trnsStrm << "  InputKey = elements" << endl;
    trnsStrm << "  InputKeyAttribute = Units" << endl;
    trnsStrm << "  InputDefault = 2" << endl;
    trnsStrm << "  OutputPosition = (group, CoreCube)" << endl;
    trnsStrm << "  OutputName = CoreSamples" << endl;
    trnsStrm << "  Translation = (*, *)" << endl;
    trnsStrm << "End_Group" << endl;
    trnsStrm << "Group = CoreLines" << endl;
    trnsStrm << "  Auto = 1" << endl;
    trnsStrm << "  Debug = 1" << endl;
    trnsStrm << "  InputPosition = (Product_Observational, Bad_Parent)" << endl;
    trnsStrm << "  InputKey = elements" << endl;
    trnsStrm << "  InputDefault = 10" << endl;
    trnsStrm << "  OutputPosition = (group, CoreCube)" << endl;
    trnsStrm << "  OutputName = CoreLines" << endl;
    trnsStrm << "  Translation = (*, *)" << endl;
    trnsStrm << "End_Group" << endl;

    trnsStrm << "End" << endl;
    
    // Copy this buffer to use for additional tests
    std::stringstream trnsStrm2, trnsStrm3;
    trnsStrm2 << trnsStrm.rdbuf()->str();
    trnsStrm3 << trnsStrm.rdbuf()->str();

    stringstream badTrnsStrm;

    badTrnsStrm << "Group = NoInputPosition" << endl;
    badTrnsStrm << "  InputKey = INSTRUMENT_IFOV" << endl;
    badTrnsStrm << "  OutputPosition = (group, instrument)" << endl;
    badTrnsStrm << "  OutputName = NoInputPosition" << endl;
    badTrnsStrm << "  Translation = (*,*)" << endl;
    badTrnsStrm << "EndGroup" << endl;
    badTrnsStrm << "Group = BadInputPosition" << endl;
    badTrnsStrm << "  InputPosition = (CaSSIS_Header, CaSSIS_General, Bad_Parent)" << endl;
    badTrnsStrm << "  InputKey = INSTRUMENT_IFOV" << endl;
    badTrnsStrm << "  OutputPosition = (group, instrument)" << endl;
    badTrnsStrm << "  OutputName = BadInputPosition" << endl;
    badTrnsStrm << "  Translation = (*,*)" << endl;
    badTrnsStrm << "EndGroup" << endl;
    badTrnsStrm << "Group = InputKeyDoesNotExist" << endl;
    badTrnsStrm << "  InputPosition = (CaSSIS_Header, CaSSIS_General)" << endl;
    badTrnsStrm << "  InputKey = Bad_Input_Element" << endl;
    badTrnsStrm << "  OutputPosition = (group, instrument)" << endl;
    badTrnsStrm << "  OutputName = InputKeyDoesNotExist" << endl;
    badTrnsStrm << "  Translation = (*,*)" << endl;
    badTrnsStrm << "EndGroup" << endl;
    badTrnsStrm << "Group = InputKeyAttributeDoesNotExist" << endl;
    badTrnsStrm << "  InputPosition = (CaSSIS_Header, CaSSIS_General)" << endl;
    badTrnsStrm << "  InputKey = INSTRUMENT_IFOV" << endl;
    badTrnsStrm << "  InputKeyAttribute = Bad_Input_Element_Attribute" << endl;
    badTrnsStrm << "  OutputPosition = (group, instrument)" << endl;
    badTrnsStrm << "  OutputName = InputKeyAttributeDoesNotExist" << endl;
    badTrnsStrm << "  Translation = (*,*)" << endl;
    badTrnsStrm << "EndGroup" << endl;
    badTrnsStrm << "Group = NoDependencyType" << endl;
    badTrnsStrm << "  InputPosition = (Observation_Area, Observing_System, Observing_System_Component)" << endl;
    badTrnsStrm << "  InputKey = name" << endl;
    badTrnsStrm << "  InputKeyDependencies = type|Spacecraft" << endl;
    badTrnsStrm << "  OutputPosition = (group, instrument)" << endl;
    badTrnsStrm << "  OutputName = NoDependencyType" << endl;
    badTrnsStrm << "  Translation = (*,*)" << endl;
    badTrnsStrm << "EndGroup" << endl;
    badTrnsStrm << "Group = BadDependencyType" << endl;
    badTrnsStrm << "  InputPosition = (Observation_Area, Observing_System, Observing_System_Component)" << endl;
    badTrnsStrm << "  InputKey = name" << endl;
    badTrnsStrm << "  InputKeyDependencies = bad@type|Spacecraft" << endl;
    badTrnsStrm << "  OutputPosition = (group, instrument)" << endl;
    badTrnsStrm << "  OutputName = BadDependencyType" << endl;
    badTrnsStrm << "  Translation = (*,*)" << endl;
    badTrnsStrm << "EndGroup" << endl;
    badTrnsStrm << "Group = NoDependencyValue" << endl;
    badTrnsStrm << "  InputPosition = (Observation_Area, Observing_System, Observing_System_Component)" << endl;
    badTrnsStrm << "  InputKey = name" << endl;
    badTrnsStrm << "  InputKeyDependencies = bad@type" << endl;
    badTrnsStrm << "  OutputPosition = (group, instrument)" << endl;
    badTrnsStrm << "  OutputName = NoDependencyValue" << endl;
    badTrnsStrm << "  Translation = (*,*)" << endl;
    badTrnsStrm << "EndGroup" << endl;

    badTrnsStrm << "End";

    stringstream invalidTrnsStrm;

    invalidTrnsStrm << "Group = InstrumentIfovWithUnits" << endl;
    invalidTrnsStrm << "  InputPosition = (CaSSIS_Header, CaSSIS_General)" << endl;
    invalidTrnsStrm << "  InputKey = INSTRUMENT_IFOV" << endl;
    invalidTrnsStrm << "  InputKeyAttribute = (Units, Attribute_2)" << endl;
    invalidTrnsStrm << "  OutputPosition = (group, instrument)" << endl;
    invalidTrnsStrm << "  OutputName = InstrumentIfovWithUnits" << endl;
    invalidTrnsStrm << "  Translation = (*,*)" << endl;
    invalidTrnsStrm << "EndGroup" << endl;
    invalidTrnsStrm << "Group = InstrumentIfovWithUnits" << endl;
    invalidTrnsStrm << "  InputPosition = (CaSSIS_Header, CaSSIS_General)" << endl;
    invalidTrnsStrm << "  InputKey = INSTRUMENT_IFOV" << endl;
    invalidTrnsStrm << "  InputKeyDependencies = \"tag@name|value\"" << endl;
    invalidTrnsStrm << "  OutputPosition = (group, instrument)" << endl;
    invalidTrnsStrm << "  OutputName = InstrumentIfovWithUnits" << endl;
    invalidTrnsStrm << "  Translation = (*,*)" << endl;
    invalidTrnsStrm << "EndGroup" << endl;
    invalidTrnsStrm << "Group = InstrumentIfovWithUnits" << endl;
    invalidTrnsStrm << "  Debug = Bad_Value" << endl;
    invalidTrnsStrm << "  InputPosition = (CaSSIS_Header, CaSSIS_General)" << endl;
    invalidTrnsStrm << "  InputKey = INSTRUMENT_IFOV" << endl;
    invalidTrnsStrm << "  OutputPosition = (group, instrument)" << endl;
    invalidTrnsStrm << "  OutputName = InstrumentIfovWithUnits" << endl;
    invalidTrnsStrm << "  Translation = (*,*)" << endl;
    invalidTrnsStrm << "EndGroup" << endl;

    invalidTrnsStrm << "End";

    cout << "Testing string stream translation specification" << endl << endl;

    XmlToPvlTranslationManager transMgr(fLabel, trnsStrm);

    cout << "Testing Translate method" << endl << endl;
    cout << "Translation of InstrumentIfovWithUnits: " <<
            transMgr.Translate("InstrumentIfovWithUnits") << endl << endl;

    cout << "Testing file-based constructor" << endl << endl;
    FileName XmltoPvlFile("$ISISTESTDATA/isis/src/base/unitTestData/XmlToPvlTestLabel.pvl");
    QString XmltoPvlFileString = XmltoPvlFile.toString(); 
    XmlToPvlTranslationManager transMgrFileConstructor(XmltoPvlFileString);

    cout << "Testing stream-only constructor" << endl << endl;
    XmlToPvlTranslationManager transMgrStreamConstructor(trnsStrm2);

    cout << "Testing constructor which uses an input label and translation file" << endl << endl;
    XmlToPvlTranslationManager transMgrFilesConstructor(fLabel, XmltoPvlFileString);

    cout << "Testing constructor which uses an input label and translation file" << endl << endl;
    XmlToPvlTranslationManager transMgrLabelStreamConstructor(fLabel, trnsStrm3);

    cout << "Testing Auto method" << endl << endl;
    Pvl outputLabel;
    transMgr.Auto(outputLabel);
    cout << endl << outputLabel << endl << endl;

    cout << "Testing Auto method with input and output labels" << endl << endl;
    transMgr.Auto(fLabel, outputLabel);
    cout << endl << outputLabel << endl << endl;

    cout << "Testing SetLabel method" << endl << endl;
    transMgr.SetLabel(fLabel);

    cout << "Testing error throws" << endl << endl;
    XmlToPvlTranslationManager badTransMgr(fLabel, badTrnsStrm);

    try {
      transMgr.Translate("InstrumentIfovWithUnits", 2);
    }
    catch(IException &e) {
      e.print();
      cout << endl;
    }

    try {
      badTransMgr.Translate("NoInputPosition");
    }
    catch(IException &e) {
      e.print();
      cout << endl;
    }

    try {
      badTransMgr.Translate("BadInputPosition");
    }
    catch(IException &e) {
      e.print();
      cout << endl;
    }

    try {
      badTransMgr.Translate("InputKeyDoesNotExist");
    }
    catch(IException &e) {
      e.print();
      cout << endl;
    }

    try {
      badTransMgr.Translate("InputKeyAttributeDoesNotExist");
    }
    catch(IException &e) {
      e.print();
      cout << endl;
    }

    try {
      badTransMgr.Translate("NoDependencyType");
    }
    catch(IException &e) {
      e.print();
      cout << endl;
    }

    try {
      badTransMgr.Translate("BadDependencyType");
    }
    catch(IException &e) {
      e.print();
      cout << endl;
    }

    try {
      badTransMgr.Translate("NoDependencyValue");
    }
    catch(IException &e) {
      e.print();
      cout << endl;
    }

    try {
      badTransMgr.Translate("NotInTranslationTable");
    }
    catch(IException &e) {
      e.print();
      cout << endl;
    }

    try {
      XmlToPvlTranslationManager invalidTransMgr(fLabel, invalidTrnsStrm);
    }
    catch(IException &e) {
      e.print();
      cout << endl;
    }

    try {
      FileName nonExistantFile("DoesNotExist.xml");
      stringstream simpleTrans;
      simpleTrans << "Group = Version" << endl;
      simpleTrans << "  Auto = 1" << endl;
      simpleTrans << "  Debug = 1" << endl;
      simpleTrans << "  InputPosition = (Identification_Area)" << endl;
      simpleTrans << "  InputKey = version_id" << endl;
      simpleTrans << "  OutputPosition = (group, instrument)" << endl;
      simpleTrans << "  OutputName = Version" << endl;
      simpleTrans << "  Translation = (*,*)" << endl;
      simpleTrans << "EndGroup" << endl;
      XmlToPvlTranslationManager nonExistantFileManager(nonExistantFile, simpleTrans);
    }
    catch(IException &e) {
      e.print();
      cout << endl;
    }

    try {
      FileName pvlFile("$ISISROOT/appdata/translations/pdsImage.trn");
      stringstream simpleTrans;
      simpleTrans << "Group = Version" << endl;
      simpleTrans << "  Auto = 1" << endl;
      simpleTrans << "  Debug = 1" << endl;
      simpleTrans << "  InputPosition = (Identification_Area)" << endl;
      simpleTrans << "  InputKey = version_id" << endl;
      simpleTrans << "  OutputPosition = (group, instrument)" << endl;
      simpleTrans << "  OutputName = Version" << endl;
      simpleTrans << "  Translation = (*,*)" << endl;
      simpleTrans << "EndGroup" << endl;
      XmlToPvlTranslationManager pvlTransFileManager(pvlFile, simpleTrans);
    }
    catch(IException &e) {
      QString message = e.toString();
      cout << message.replace(QRegExp("in file.*/translations"), "in file [/translations");
      cout << endl;
      cout << endl;
    }


  }
  catch(IException &e) {
    e.print();
  }

  return 0;
}
