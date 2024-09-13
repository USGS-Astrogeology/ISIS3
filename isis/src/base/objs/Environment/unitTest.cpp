/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Environment.h"

#include <cstring>

#include "IString.h"
#include "Preference.h"
#include "ProgramLauncher.h"


using namespace Isis;
using std::cerr;


int main(int argc, char *argv[]) {
  Isis::Preference::Preferences(true);
  
  char * str = new char[256];
  strncpy(str, "USER=eclipse", 256);
  putenv(str);
  str = new char[256];
  strncpy(str, "HOST=wang", 256);
  putenv(str);
  
  cerr << "userName: " << Environment::userName().toStdString() << "\n";
  cerr << "hostName: " << Environment::hostName().toStdString() << "\n";
  
  QString cmd = "echo 'version:  isis" + Environment::isisVersion() + "' | cut -d . -f1";
  ProgramLauncher::RunSystemCommand(cmd);

  return 0;
}
