/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <string>
#include <iostream>
#include "Application.h"
#include "IException.h"
#include "Preference.h"
#include "PvlGroup.h"
#include "PvlObject.h"
#include "Pvl.h"

void myFunct() {
  std::cout << "In myFunct" << std::endl;
}

void myError() {
  std::string msg = "testing an error";
  throw Isis::IException(Isis::IException::Programmer, msg, _FILEINFO_);
}

void errorFormatting() {
  std::string msg = "local test error";
  Isis::IException exception = Isis::IException(Isis::IException::Programmer, msg, _FILEINFO_);

  // Add test for formatError
  Isis::Pvl &prefs = Isis::Preference::Preferences();
  Isis::PvlGroup &errorPrefs = prefs.findGroup("ErrorFacility");

  std::string errorString = Isis::Application::formatError(exception);
  std::cout << errorString.toStdString() << std::endl;

  std::string &formatValue = errorPrefs["Format"][0];
  formatValue = "Pvl";

  errorString = Isis::Application::formatError(exception);
  std::cout << errorString.toStdString() << std::endl;
}

using namespace std;
int main(int argc, char *argv[]) {
  Isis::Preference::Preferences(true);

  std::cout << "Testing Isis::Application Class ... " << std::endl;
  std::cout << std::endl;
  Isis::Application a(argc, argv);

  Isis::PvlGroup acct = a.Accounting();
  a.Log(acct);
  std::cout << std::endl;

  Isis::PvlObject hist = a.History();
  std::cout << hist << std::endl;
  std::cout << std::endl;

  std::cout << "Can't test Gui methods" << std::endl;
  std::cout << "  GuiLog(Pvl &results)" << std::endl;
  std::cout << "  GuiLog(PvlGroup &results)" << std::endl;
  std::cout << "  GuiLog(QString &results)" << std::endl;
  std::cout << "  GuiReportErrorLog(iException &e)" << std::endl;
  std::cout << std::endl;

  std::map<QString, void *> helpers;
  helpers["helper"] = (void *) myFunct;
  a.RegisterGuiHelpers(helpers);
  if (a.GetGuiHelper("helper") == (void *) myFunct) {
    std::cout << "GuiHelpers ok" << std::endl;
  }
  else {
    std::cout << "GuiHelpers bad" << std::endl;
  }
  std::cout << std::endl;

  a.Run(myFunct);
  std::cout << std::endl;

  a.Run(myError);
  std::cout << std::endl;

  a.Run(errorFormatting);
  std::cout << std::endl;

  Isis::Pvl p("print.prt");
  std::cout << p << std::endl;

  remove("print.prt");
  return 0;
}
