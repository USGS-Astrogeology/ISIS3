/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ControlMeasureLogData.h"

#include <iostream>

#include <QVariant>

#include "Preference.h"

using namespace Isis;
using namespace std;

int main() {
  Preference::Preferences(true);

  cout << "Testing default constructor..." << endl;
  {
    ControlMeasureLogData logData;

    cout << "Valid? " << logData.IsValid() << "\n"
         << "Type?  " << logData.GetDataType() << "\n"
         << "Value? " << logData.GetNumericalValue() << "\n"
         << "Keyword: '" << logData.ToKeyword() << "'\n\n";
  }

  cout << "Testing type-only constructor..." << endl;
  {
    ControlMeasureLogData logData(ControlMeasureLogData::GoodnessOfFit);

    cout << "Valid? " << logData.IsValid() << "\n"
         << "Type?  " << logData.GetDataType() << "\n"
         << "Value? " << logData.GetNumericalValue() << "\n"
         << "Keyword: '" << logData.ToKeyword() << "'\n\n";
  }

  cout << "Testing PvlKeyword constructor..." << endl;
  {
    ControlMeasureLogData logData(PvlKeyword("GoodnessOfFit", Isis::toString(3.14159)));

    cout << "Valid? " << logData.IsValid() << "\n"
         << "Type?  " << logData.GetDataType() << "\n"
         << "Value? " << logData.GetNumericalValue() << "\n"
         << "Keyword: '" << logData.ToKeyword() << "'\n\n";
  }

  cout << "Testing type/value constructor..." << endl;
  {
    ControlMeasureLogData logData(ControlMeasureLogData::GoodnessOfFit,
                                  3.14159);

    cout << "Valid? " << logData.IsValid() << "\n"
         << "Type?  " << logData.GetDataType() << "\n"
         << "Value? " << logData.GetValue().toDouble() << "\n"
         << "Keyword: '" << logData.ToKeyword() << "'\n\n";
  }

  cout << "Testing copy constructor 1..." << endl;
  {
    ControlMeasureLogData logDataTmp(ControlMeasureLogData::GoodnessOfFit,
                                  3.14159);
    ControlMeasureLogData logData(logDataTmp);

    cout << "Valid? " << logData.IsValid() << "\n"
         << "Type?  " << logData.GetDataType() << "\n"
         << "Value? " << logData.GetNumericalValue() << "\n"
         << "Keyword: '" << logData.ToKeyword() << "'\n\n";
  }

  cout << "Testing copy constructor 2..." << endl;
  {
    ControlMeasureLogData logDataTmp;
    ControlMeasureLogData logData(logDataTmp);

    cout << "Valid? " << logData.IsValid() << "\n"
         << "Type?  " << logData.GetDataType() << "\n"
         << "Value? " << logData.GetNumericalValue() << "\n"
         << "Keyword: '" << logData.ToKeyword() << "'\n\n";
  }

  cout << "Testing SetDataType/SetNumericalValue..." << endl;
  {
    ControlMeasureLogData logData;
    logData.SetDataType(ControlMeasureLogData::Obsolete_Eccentricity);
    logData.SetNumericalValue(3.14159);

    cout << "Valid? " << logData.IsValid() << "\n"
         << "Type?  " << logData.GetDataType() << "\n"
         << "Value? " << logData.GetNumericalValue() << "\n"
         << "Keyword: '" << logData.ToKeyword() << "'\n\n";
  }

  return 0;
}
