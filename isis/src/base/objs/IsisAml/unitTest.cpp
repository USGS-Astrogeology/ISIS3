/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <QString>
#include <sstream>
#include "IsisAml.h"

#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Pvl.h"
#include "Preference.h"
#include "TextFile.h"


using namespace Isis;
using namespace std;

void ReportError(QString err);

class Inheritor : public IsisAml {
  public:
    // constructor
    Inheritor(const char *xmlfile);
    // destructor
    ~Inheritor();
};

Inheritor::Inheritor(const char *xmlfile): IsisAml(xmlfile) {

  try {
    cout << "---------- Tests for private data ----------" << endl;

    cout << "  App name = " << name.toStdString() << endl;
    cout << "  App brief = " << brief.toStdString() << endl;
    cout << "  App description = " << description.toStdString() << endl;

    // Print categories
    for(unsigned int l = 0; l < categorys.size(); l++) {
      cout << "  category[" << l << "] = " << categorys[l].toStdString() << endl;
    }

    // Loop for each group
    for(unsigned int g = 0; g < groups.size(); g++) {
      cout << "  group " << g << " name = " << groups[g].name.toStdString() << endl;

      // Loop for each parameter in the group
      for(unsigned int p = 0; p < groups[g].parameters.size(); p++) {
        cout << "    parameter " << p << ", name = " <<
             groups[g].parameters[p].name.toStdString() << endl;
        cout << "      type = " << groups[g].parameters[p].type.toStdString() << endl;
        cout << "      brief = " << groups[g].parameters[p].brief.toStdString() << endl;
        cout << "      description = " <<
             groups[g].parameters[p].description.toStdString() << endl;
        cout << "      internal def = " <<
             groups[g].parameters[p].internalDefault.toStdString() << endl;
        // Loop for each helper in the parameter
        cout << "      helpers = " << groups[g].parameters[p].helpers.size()
             << endl;
        for(unsigned int h = 0; h < groups[g].parameters[p].helpers.size(); h++) {
          cout << "        name = " << groups[g].parameters[p].helpers[h].name.toStdString() << endl;
          cout << "        brief = " << groups[g].parameters[p].helpers[h].brief.toStdString() << endl;
          cout << "        description = " << groups[g].parameters[p].helpers[h].description.toStdString() << endl;
          cout << "        function = " << groups[g].parameters[p].helpers[h].function.toStdString() << endl;
          cout << "        icon = " << groups[g].parameters[p].helpers[h].icon.toStdString() << endl;

        }
        cout << "      count = " << groups[g].parameters[p].count.toStdString() << endl;
        cout << "      minimum = " << groups[g].parameters[p].minimum.toStdString() << endl;
        cout << "      minimum inclusive = " <<
             groups[g].parameters[p].minimum_inclusive.toStdString() << endl;
        cout << "      maximum = " << groups[g].parameters[p].maximum.toStdString() << endl;
        cout << "      maximum inclusive = " <<
             groups[g].parameters[p].maximum_inclusive.toStdString() << endl;
        cout << "      filter = " << groups[g].parameters[p].filter.toStdString() << endl;
        cout << "      file mode = " << groups[g].parameters[p].fileMode.toStdString() << endl;
        cout << "      odd = " << groups[g].parameters[p].odd.toStdString() << endl;

        // Print the current values
        cout << "      Values:" << endl;
        for(unsigned int l = 0; l < groups[g].parameters[p].values.size(); l++) {
          cout << "        [" << l << "] = " <<
               groups[g].parameters[p].values[l].toStdString() << endl;
        }

        // Print the default values
        cout << "      Default Values:" << endl;
        for(unsigned int l = 0; l < groups[g].parameters[p].defaultValues.size(); l++) {
          cout << "        [" << l << "] = " <<
               groups[g].parameters[p].defaultValues[l].toStdString() << endl;
        }

        // Print the parameters which this one must be greater than
        cout << "      Greater Than:" << endl;
        for(unsigned int l = 0; l < groups[g].parameters[p].greaterThan.size(); l++) {
          cout << "        [" << l << "] = " <<
               groups[g].parameters[p].greaterThan[l].toStdString() << endl;
        }

        // Print the parameters which this one must be greater than or equal
        cout << "      Greater or Equal Than:" << endl;
        for(unsigned int l = 0; l < groups[g].parameters[p].greaterThanOrEqual.size(); l++) {
          cout << "        [" << l << "] = " <<
               groups[g].parameters[p].greaterThanOrEqual[l].toStdString() << endl;
        }

        // Print the parameters which this one must be less than
        cout << "      Less Than:" << endl;
        for(unsigned int l = 0; l < groups[g].parameters[p].lessThan.size(); l++) {
          cout << "        [" << l << "] = " <<
               groups[g].parameters[p].lessThan[l].toStdString() << endl;
        }

        // Print the parameters which this one must be less than or equal
        cout << "      Less Than or Equal:" << endl;
        for(unsigned int l = 0; l < groups[g].parameters[p].lessThanOrEqual.size(); l++) {
          cout << "        [" << l << "] = " <<
               groups[g].parameters[p].lessThanOrEqual[l].toStdString() << endl;
        }

        // Print the parameters which this one must not be equal to
        cout << "      Not equal to:" << endl;
        for(unsigned int l = 0; l < groups[g].parameters[p].notEqual.size(); l++) {
          cout << "        [" << l << "] = " <<
               groups[g].parameters[p].notEqual[l].toStdString() << endl;
        }

        // Print the parameters which must be included if this parameter is used
        cout << "      Include parameters:" << endl;
        for(unsigned int l = 0; l < groups[g].parameters[p].include.size(); l++) {
          cout << "        [" << l << "] = " <<
               groups[g].parameters[p].include[l].toStdString() << endl;
        }

        // Print the parameters which must NOT be included if this parameter is used
        cout << "      Exclude parameters:" << endl;
        for(unsigned int l = 0; l < groups[g].parameters[p].exclude.size(); l++) {
          cout << "        [" << l << "] = " <<
               groups[g].parameters[p].exclude[l].toStdString() << endl;
        }

        // Print the parameter's list options
        cout << "      List data:" << endl;
        for(unsigned int l = 0; l < groups[g].parameters[p].listOptions.size(); l++) {
          cout << "        value [" << l << "] = " <<
               groups[g].parameters[p].listOptions[l].value.toStdString() << endl;
          cout << "        brief [" << l << "] = " <<
               groups[g].parameters[p].listOptions[l].brief.toStdString() << endl;
          cout << "        description [" << l << "] = " <<
               groups[g].parameters[p].listOptions[l].description.toStdString() << endl;
          for(unsigned int e = 0; e < groups[g].parameters[p].listOptions[l].exclude.size(); e++) {
            cout << "          exclude = " <<
                 groups[g].parameters[p].listOptions[l].exclude[e].toStdString() << endl;
          }
        }
      }
    }
  }
  catch(IException &error) {
    error.print();
  }
}


int main(void) {
  Preference::Preferences(true);
  // Create the aml object
  cout << "Create the aml object" << endl;
  Inheritor *aml;
  std::string xmlFile = FileName("./unitTest.xml").expanded();
  try {
    aml = new Inheritor(xmlFile.c_str());
  }
  catch(IException &error) {
    error.print();
    exit(1);
  }


  // Test public members
  try {
    cout << "Application information" << endl;

    cout << "Program name : " << aml->ProgramName().toStdString() << endl;
    cout << "Brief description: " << aml->Brief().toStdString() << endl;
    cout << "Full description: " << aml->Description().toStdString() << endl;
    cout << "Version date: " << aml->Version().toStdString() << endl << endl;

    cout << "Number of parameter groups : " << aml->NumGroups() << endl << endl;
    cout << "Parameter information:" << endl;
    for(int g = 0; g < aml->NumGroups(); g++) {
      cout << "  Group number: " << g << endl;
      cout << "  Group name : " << aml->GroupName(g).toStdString() << endl;
      for(int p = 0; p < aml->NumParams(g); p++) {
        cout << "    Parameter number: " << p << endl;
        cout << "      Name: " << aml->ParamName(g, p).toStdString() << endl;
        cout << "      Type: " << aml->ParamType(g, p).toStdString() << endl;
        cout << "      Brief: " << aml->ParamBrief(g, p).toStdString() << endl;
        cout << "      Default: " << aml->ParamDefault(g, p).toStdString() << endl;
        cout << "      Internal default: " << aml->ParamInternalDefault(g, p).toStdString() << endl;
        cout << "      Filter: " << aml->ParamFilter(g, p).toStdString() << endl;
        cout << "      File Mode: " << aml->ParamFileMode(g, p).toStdString() << endl;
        cout << "      Helper Information:" << endl;
        for(int h = 0; h < aml->HelpersSize(g, p); h++) {
          cout << "        Name: " << aml->HelperButtonName(g, p, h).toStdString() << endl;
          cout << "        Brief: " << aml->HelperBrief(g, p, h).toStdString() << endl;
          cout << "        Description: " << aml->HelperDescription(g, p, h).toStdString() << endl;
          cout << "        Function: " << aml->HelperFunction(g, p, h).toStdString() << endl;
          cout << "        Icon: " << aml->HelperIcon(g, p, h).toStdString() << endl;
        }
        cout << "      List Information:" << endl;
        for(int o = 0; o < aml->ParamListSize(g, p); o++) {
          cout << "        Value: " << aml->ParamListValue(g, p, o).toStdString() << endl;
          cout << "        Brief: " << aml->ParamListBrief(g, p, o).toStdString() << endl;
          cout << "        Description: " << aml->ParamListDescription(g, p, o).toStdString() << endl;
          cout << "        List exclusions: " << endl;
          for(int e = 0; e < aml->ParamListExcludeSize(g, p, o); e++) {
            cout << "          Exclude parameter: " << aml->ParamListExclude(g, p, o, e).toStdString() << endl;
          }
          cout << "        List inclusions: " << endl;
          for(int i = 0; i < aml->ParamListIncludeSize(g, p, o); i++) {
            cout << "          Include parameter: " << aml->ParamListInclude(g, p, o, i).toStdString() << endl;
          }
        }
      }
    }

    cout << "Get/Put/Clear/WasEntered tests" << endl;
    cout << "Default value of G0P1 is " << aml->GetFileName("G0P1").toStdString() << endl;
    cout << "G0P1 WasEntered value " << aml->WasEntered("G0P1") << endl;
    aml->PutAsString("G0P1", "/home/user/file1.cub");
    cout << "G0P1 WasEntered value " << aml->WasEntered("G0P1") << endl;
    cout << "The value of G0P1 is " << aml->GetFileName("G0P1").toStdString() << endl;
    aml->Clear("G0P1");
    cout << "Default value of G0P1 is " << aml->GetFileName("G0P1").toStdString() << endl;
    aml->PutFileName("G0P1", "/home/user/file2.dat");
    cout << "The value of G0P1 is " << aml->GetFileName("G0P1").toStdString() << endl << endl;
    cout << "The value of G0P1 is " << aml->GetFileName("G0P1", "txt").toStdString() << endl << endl;
    aml->Clear("G0P1");

    cout << "Default value of G1P1 is " << aml->GetInteger("G1P1") << endl;
    aml->PutInteger("G1P1", 33);
    cout << "The value of G1P1 is " << aml->GetInteger("G1P1") << endl << endl;
    aml->Clear("G1P1");

    cout << "Default value of G1P2 is " << aml->GetDouble("G1P2") << endl;
    aml->PutDouble("G1P2", 0.000000001);
    cout << "The value of G1P2 is " << aml->GetDouble("G1P2") << endl << endl;
    aml->Clear("G1P2");

    cout << "Default value of G1P0 is " << aml->GetString("G1P0").toStdString() << endl;
    aml->PutString("G1P0", "G1p0L1");
    cout << "The value of G1P0 is " << aml->GetString("G1P0").toStdString() << endl << endl;
    aml->Clear("G1P0");

    aml->PutBoolean("G6P0", true);
    cout << "The value of G6P0 is " << aml->GetBoolean("G6P0") << endl << endl;
    aml->Clear("G6P0");
    aml->PutBoolean("G6P0", false);
    cout << "The value of G6P0 is " << aml->GetBoolean("G6P0") << endl << endl;
    aml->Clear("G6P0");

    aml->PutAsString("G6P0", "TRUE");
    cout << "The value of G6P0 is " << aml->GetBoolean("G6P0") << endl << endl;
    aml->Clear("G6P0");
    aml->PutAsString("G6P0", "FALSE");
    cout << "The value of G6P0 is " << aml->GetBoolean("G6P0") << endl << endl;
    aml->Clear("G6P0");
    aml->PutAsString("G6P0", "true");
    cout << "The value of G6P0 is " << aml->GetBoolean("G6P0") << endl << endl;
    aml->Clear("G6P0");
    aml->PutAsString("G6P0", "false");
    cout << "The value of G6P0 is " << aml->GetBoolean("G6P0") << endl << endl;
    aml->Clear("G6P0");

    aml->PutAsString("G6P0", "YES");
    cout << "The value of G6P0 is " << aml->GetBoolean("G6P0") << endl << endl;
    aml->Clear("G6P0");
    aml->PutAsString("G6P0", "NO");
    cout << "The value of G6P0 is " << aml->GetBoolean("G6P0") << endl << endl;
    aml->Clear("G6P0");
    aml->PutAsString("G6P0", "yes");
    cout << "The value of G6P0 is " << aml->GetBoolean("G6P0") << endl << endl;
    aml->Clear("G6P0");
    aml->PutAsString("G6P0", "no");
    cout << "The value of G6P0 is " << aml->GetBoolean("G6P0") << endl << endl;
    aml->Clear("G6P0");

    aml->PutAsString("G6P0", "T");
    cout << "The value of G6P0 is " << aml->GetBoolean("G6P0") << endl << endl;
    aml->Clear("G6P0");
    aml->PutAsString("G6P0", "F");
    cout << "The value of G6P0 is " << aml->GetBoolean("G6P0") << endl << endl;
    aml->Clear("G6P0");
    aml->PutAsString("G6P0", "t");
    cout << "The value of G6P0 is " << aml->GetBoolean("G6P0") << endl << endl;
    aml->Clear("G6P0");
    aml->PutAsString("G6P0", "f");
    cout << "The value of G6P0 is " << aml->GetBoolean("G6P0") << endl << endl;
    aml->Clear("G6P0");

    aml->PutAsString("G6P0", "Y");
    cout << "The value of G6P0 is " << aml->GetBoolean("G6P0") << endl << endl;
    aml->Clear("G6P0");
    aml->PutAsString("G6P0", "N");
    cout << "The value of G6P0 is " << aml->GetBoolean("G6P0") << endl << endl;
    aml->Clear("G6P0");
    aml->PutAsString("G6P0", "y");
    cout << "The value of G6P0 is " << aml->GetBoolean("G6P0") << endl << endl;
    aml->Clear("G6P0");
    aml->PutAsString("G6P0", "n");
    cout << "The value of G6P0 is " << aml->GetBoolean("G6P0") << endl << endl;
    aml->Clear("G6P0");

    cout << "Exact and partial name match tests:" << endl;
    cout << "  FROM's value = " << aml->GetString("from").toStdString() << endl;
    cout << "  FROM1's value = " << aml->GetString("from1").toStdString() << endl;
    cout << "  FR's value = " << aml->GetString("fr").toStdString() << endl;
    cout << endl;
  }
  catch(IException &error) {
    error.print();
  }
  cout << endl;

  // Test the error catching for public members

  cout << "---------- Test error throwing ----------" << endl;

  try {
    cout << "  PutAsString:" << endl;
    try { // PARAMETER VALUE ALREADY ENTERED
      aml->PutAsString("G1P0", "11111");
      aml->PutAsString("G1P0", "22222");
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }
    aml->Clear("G1P0");

    cout << "  PutString:" << endl;
    try { // PARAMETER VALUE ALREADY ENTERED
      aml->PutString("G1P0", "G1P0L0");
      aml->PutString("G1P0", "22222");
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }
    aml->Clear("G1P0");

    try { // PARAMETER NOT STRING
      aml->PutString("G2P4", "xxxxxx");
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }

    cout << "  PutFileName:" << endl;
    try { // PARAMETER VALUE ALREADY ENTERED
      aml->PutFileName("G0P0", "xxxxxxx");
      aml->PutFileName("G0P0", "yyyyyyy");
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }
    aml->Clear("G0P0");

    try { // PARAMETER NOT CUBENAME
      aml->PutCubeName("G2P4", "xxxxxx");
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }

    cout << "  Cube tests:" << endl;

    try { // UNABLE TO GET INPUT CUBE ATTRIBUTES
      aml->PutCubeName("CUBE2", "xxxxxxx.cub+1,2-4");
      CubeAttributeInput &att = aml->GetInputAttribute("CUBE2");
      cout << "    " << att.toString().toStdString() << endl;
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }
    aml->Clear("CUBE2");

    try { // UNABLE TO GET OUTPUT CUBE ATTRIBUTES
      aml->PutCubeName("CUBE1", "yyyyyyy.cub+8-bit+BSQ+detached");
      CubeAttributeOutput &att = aml->GetOutputAttribute("CUBE1");
      QString strng = att.toString();
      cout << "    Att QString  = " << strng.toStdString() << endl;
      cout << "    File format = " << att.fileFormatString().toStdString() << endl;
      cout << "    Pixel type  = " << PixelTypeName(att.pixelType()).toStdString() << endl;
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }
    aml->Clear("CUBE1");

    cout << "  PutInteger:" << endl;
    try { // PARAMETER VALUE ALREADY ENTERED
      aml->PutInteger("G6P2", 1);
      aml->PutInteger("G6P2", 1);
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }
    aml->Clear("G6P2");

    try { // PARAMETER NOT INTEGER
      aml->PutInteger("G6P0", 1);
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }

    cout << "  PutDouble:" << endl;
    try { // PARAMETER VALUE ALREADY ENTERED
      aml->PutDouble("G1P2", 1.0);
      aml->PutDouble("G1P2", 1.0);
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }
    aml->Clear("G1P2");

    try { // PARAMETER NOT DOUBLE
      aml->PutDouble("G0P0", 1.0);
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }


    cout << "  PutBoolean:" << endl;
    try { // PARAMETER VALUE ALREADY ENTERED
      aml->PutBoolean("G6P0", true);
      aml->PutBoolean("G6P0", false);
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }
    aml->Clear("G6P0");

    try { // PARAMETER NOT BOOLEAN
      aml->PutBoolean("G0P0", false);
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }

    cout << "  GetAsString:" << endl;
    try { // PARAMETER HAS NO VALUE
      QString s = aml->GetAsString("G2P0");
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }

    cout << "  GetFileName:" << endl;
    try { // PARAMETER HAS NO VALUE
      QString s = aml->GetFileName("G0P0");
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }

    try { // PARAMETER NOT FILENAME
      QString s = aml->GetFileName("G2P4");
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }

#if 0
    cout << "  GetCube:" << endl;
    try {
      QString s = aml->GetCube("CUBE1");
    }
    catch(IException &error) {
      ReportError(IString(error.Errors()));
      error.Clear();
    }

    cout << endl;
    try {
      QString s = aml->GetCube("G2P4");
    }
    catch(IException &error) {
      ReportError(IString(error.Errors()));
      error.Clear();
    }
#endif

    cout << "  GetString:" << endl;
    try { // PARAMETER HAS NO VALUE
      QString s = aml->GetString("G6P3");
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }

    try { // PARAMETER NOT STRING
      QString s = aml->GetString("G2P4");
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }

    cout << "  GetInteger:" << endl;
    try { // PARAMETER HAS NO VALUE
      aml->GetInteger("G2P0");
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }

    try { // PARAMETER NOT INTEGER
      aml->GetInteger("G0P0");
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }

    cout << "  GetDouble:" << endl;
    try { // PARAMETER HAS NO VALUE
      aml->GetDouble("G1P3");
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }

    try { // PARAMETER NOT DOUBLE
      aml->GetDouble("G0P1");
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }

    cout << "  GetBoolean:" << endl;
    try { // PARAMETER HAS NO VALUE
      aml->GetBoolean("G6P0");
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }

    try { // INVALID BOOLEAN VALUE
      aml->PutAsString("G6P0", "cccc");
      aml->GetBoolean("G6P0");
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }
    aml->Clear("G6P0");

    try { // PARAMETER NOT BOOLEAN
      aml->GetBoolean("G1P1");
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }

    try { // PARAMETER NOT UNIQUE
      QString s = aml->GetString("F");
    }
    catch(IException &error) {
      ReportError(QString::fromStdString(error.toString()));
    }

  }
  catch(IException &error) {
    error.print();
  }
  // Load up a valid set of parameters

  cout << "---------- Validate a correctly loaded set of parameters ----------" << endl;
  aml->Clear("G0P0");
  aml->PutAsString("G0P0", "FILE1");

  aml->Clear("G0P1");
  aml->PutAsString("G0P1", "FILE2");

  aml->Clear("G1P0");
  aml->PutAsString("G1P0", "G1P0L0");

  aml->Clear("G1P1");

  aml->Clear("G1P2");

  aml->Clear("G1P3");
  aml->PutAsString("G1P3", "1.1");

  aml->Clear("G2P0");

  aml->Clear("G2P1");
  aml->PutAsString("G2P1", "1");

  aml->Clear("G2P2");
  aml->PutAsString("G2P2", "3");

  aml->Clear("G2P3");
  aml->PutAsString("G2P3", "4");

  aml->Clear("G2P4");
  aml->PutAsString("G2P4", "5");

  aml->Clear("G3P0");

  aml->Clear("G3P1");
  aml->PutAsString("G3P1", "1.2");

  aml->Clear("G3P2");
  aml->PutAsString("G3P2", "1.3");

  aml->Clear("G3P3");
  aml->PutAsString("G3P3", "1.4");

  aml->Clear("G3P4");
  aml->PutAsString("G3P4", "1.5");

  aml->Clear("G4P0");
  aml->PutAsString("G4P0", "1.6");

  aml->Clear("G4P1");
  aml->PutAsString("G4P1", "6");

  aml->Clear("G4P2");
  aml->PutAsString("G4P2", "6.7");

  aml->Clear("G4P3");
  aml->PutAsString("G4P3", "7");

  aml->Clear("G4P4");
  aml->PutAsString("G4P4", "7.8");

  aml->Clear("G4P5");
  aml->PutAsString("G4P5", "8");

  aml->Clear("G5P0");
  aml->PutAsString("G5P0", "9");

  aml->Clear("G5P1");
  aml->PutAsString("G5P1", "10");

  aml->Clear("G5P2");
  aml->PutAsString("G5P2", "11");

  aml->Clear("G5P3");

  aml->Clear("G6P0");
  aml->PutAsString("G6P0", "yes");

  aml->Clear("G6P1");
  aml->PutAsString("G6P1", "13");

  aml->Clear("G6P2");

  aml->Clear("G6P3");
  aml->PutAsString("G6P3", "STRING2");

  aml->Clear("FROM");
  aml->PutAsString("FROM", "STRING3");

  aml->Clear("FROM1");
  aml->PutAsString("FROM1", "STRING4");

  aml->Clear("FR");
  aml->PutAsString("FR", "STRING4");

  aml->Clear("CUBE1");
  aml->PutAsString("CUBE1", "CUBE.DAT");

  aml->Clear("CUBE2");
  aml->PutAsString("CUBE2", "CUBE2.DAT");

  try {
    aml->VerifyAll();
  }
  catch(IException &error) {
    error.print();
  }
  cout << endl;

  // Test the command line member
  cout << "The current command line:" << endl;
  try {
    Pvl lab;
    aml->CommandLine(lab);
    cout << lab << endl << endl;
  }
  catch(IException &error) {
    error.print();
  }

  cout << endl;

  cout << "---------- Check for NO value in an option/list/included parameter ----------" << endl;
  aml->Clear("G1P0");
  aml->PutAsString("G1P0", "G1P0L1X");
  aml->Clear("G1P3");
  try { // PARAMETER MUST BE ENTERED IF OTHER PARAMETER HAS PARTICULAR VALUE
    aml->VerifyAll();
  }
  catch(IException &error) {
    ReportError(QString::fromStdString(error.toString()));
  }
  aml->Clear("G1P0");
  aml->PutAsString("G1P3", "1.1");
  aml->PutAsString("G1P0", "G1P0L0");

  cout << "---------- Check for value in an option/list/excluded parameter ----------" << endl;
  aml->PutAsString("G2P0", "0");
  try { // PARAMETER CAN NOT BE ENTERED IF OTHER PARAMETER HAS PARTICULAR VALUE
    aml->VerifyAll();
  }
  catch(IException &error) {
    ReportError(QString::fromStdString(error.toString()));
  }
  aml->Clear("G2P0");

  cout << "---------- Check error for unknown parameter ----------" << endl;
  try { // UNKNOWN PARAMETER
    aml->Clear("xyz");
  }
  catch(IException &error) {
    ReportError(QString::fromStdString(error.toString()));
  }

  cout << "---------- Check errors for user file overwrite preferences ----------" << endl;
  // Create a file
  QString testFile = "junk.txt";
  QString lines[3];
  lines[0] = "1";
  vector<QString> strvect;
  strvect.push_back(lines[0]);
  TextFile f1(testFile, "overwrite", strvect);
  aml->Clear("G0P1");
  // set output parameter to equal existing file
  aml->PutAsString("G0P1", "junk.txt");
  Preference &testpref = Preference::Preferences();

  try {
    // FILE OVERWRITE NOT ALLOWED
    testpref.findGroup("FileCustomization").findKeyword("Overwrite")[0] = "Error";
    cout << "  Overwrite not allowed:" << endl;
    aml->VerifyAll();
  }
  catch(IException &error) {
    ReportError(QString::fromStdString(error.toString()));
  }

  try { // INVALID OVERWRITE VALUE IN USER PREFERENCE FILE
    testpref.findGroup("FileCustomization").findKeyword("Overwrite")[0] = "Err";
    cout << "  Invalid Overwrite preference value:" << endl;
    aml->VerifyAll();
  }
  catch(IException &error) {
    ReportError(QString::fromStdString(error.toString()));
  }
  remove("junk.txt");
}

/**
 * Reports error messages from Isis:iException without full paths of filenames
 * @param err Error String containing iException messages
 * @author 2010-07-26 Jeannie Walldren
 * @internal
 *   @history 2010-07-26 Jeannie Walldren - Original version.
 */
void ReportError(QString err) {
  cout << err.replace(QRegExp("\\[/[^\\]]*\\]"), "[]").toStdString() << endl << endl;
}

