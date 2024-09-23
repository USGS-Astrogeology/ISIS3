/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "SessionLog.h"

#include <string.h>

#include "Application.h"
#include "Preference.h"
#include "IException.h"
#include "Pvl.h"

using namespace Isis;
using namespace std;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);
  PvlGroup &g = Preference::Preferences().findGroup("SessionLog");
  g["TerminalOutput"] = "On";
  try {
    PvlGroup results("Results");;
    results.addComment("// This is an example of the results group");
    results += PvlKeyword("Average", Isis::toString(13.5), "Meters");
    results[0].addComment("// Average size of a rock");

    Pvl error;
    PvlGroup temp("Error");
    temp += PvlKeyword("Program", "ratio");
    temp += PvlKeyword("Class", "I/O ERROR");
    temp += PvlKeyword("Status", Isis::toString(-1));
    temp += PvlKeyword("Message", "Unable to open file");
    temp += PvlKeyword("File", "unitTest.cpp");
    temp += PvlKeyword("Line", Isis::toString(501));
    error.addGroup(temp);
    char **s_argv;
    s_argv = new char*[10];
    s_argv[0] = new char[32];
    strncpy(s_argv[0], "unitTest", strlen("unitTest") + 1);
    s_argv[1] = new char[32];
    strncpy(s_argv[1], "num=a", strlen("num=a") + 1);
    s_argv[2] = new char[32];
    strncpy(s_argv[2], "den=b", strlen("den=b") + 1);
    s_argv[3] = new char[32];
    strncpy(s_argv[3], "to=bogus", strlen("to=bogus") + 1);
    s_argv[4] = 0;

    int s_argc = 4;
    try {
      Application app(s_argc, (char**)s_argv);
      SessionLog &log = SessionLog::TheLog(true);
      log.AddResults(results);
      std::cout << log << std::endl;
    }
    catch(IException &e) {
      e.print();
    }

    try {
      Application app(s_argc, (char**)s_argv);
      SessionLog &log = SessionLog::TheLog(true);
      log.AddResults(results);
      log.AddError(error);
      std::cout << log << std::endl;
    }
    catch(IException &e) {
      e.print();
    }

  }
  catch(IException &e) {
    e.print();
  }

  return 0;
}
