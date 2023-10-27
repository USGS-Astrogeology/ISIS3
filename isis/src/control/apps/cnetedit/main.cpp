/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#define GUIHELPERS

#include "Isis.h"

#include "Application.h"
#include "Pvl.h"
#include "UserInterface.h"
#include "cnetedit.h"

#include "GuiEditFile.h"


using namespace std;
using namespace Isis;

void PrintTemp();
void EditDefFile();

map<QString, void *> GuiHelpers() {
    map<QString, void *> helper;
    helper["PrintTemp"]   = (void *) PrintTemp;
    helper["EditDefFile"] = (void *) EditDefFile;
    return helper;
}


void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  Pvl results = cnetedit(ui);
}

/**
   * Helper function to print out template to session log.
   */
void PrintTemp() {
  UserInterface &ui = Application::GetUserInterface();

  // Get template PVL
  Pvl userTemp;
  userTemp.read(ui.GetFileName("DEFFILE").toStdString());

  // Write template file out to the log
  Isis::Application::GuiLog(userTemp);
}


/**
   * Helper function to be able to edit the Deffile.
   * Opens an editor to edit the file.
   *
   * @author Sharmila Prasad (5/23/2011)
   */
void EditDefFile(void) {
  UserInterface &ui = Application::GetUserInterface();
  QString sDefFile = ui.GetAsString("DEFFILE");

  GuiEditFile::EditFile(ui, sDefFile);
}
