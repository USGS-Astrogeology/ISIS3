/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Application.h"
#include "UserInterface.h"
#include "FileList.h"
#include "IString.h"

using namespace std;
using namespace Isis;

void MakeCompletion(const QString &appName);
QString GetParamCompletion(int grp, int param);
QString BuildStaticCompletes(QString paramList);
void PrintHelp();

int main(int argc, char *argv[]) {
  if(argc < 2) {
    PrintHelp();
    return 0;
  }

  for (int i = 1; i < argc; i++) {
    MakeCompletion(argv[i]);
  }
  return 0;
}

void PrintHelp() {
  cout << "This program is used to set up the isis tab-completion for the tcsh shell. ";
  cout << "The output of this program is a list of commands for the isis setup script to execute." << endl;
  cout << "Usage: isiscomplete isisappname [isisappname2 isisappname3 ...]" << endl;
}

void MakeCompletion(const QString &appName) {
  char *argv[2] = { 0, 0 };
  int argc = 2;

  QByteArray appNameBytes = appName.toLatin1();
  argv[0] = appNameBytes.data();
  argv[1] = new char[16];
  strcpy(argv[1], "-nogui");

  // Do not complete image-viewer applications
  if(appName.compare("qview") == 0 || appName.compare("./qview") == 0) {
    cout << "complete " << appName.toStdString() << " 'c/-/(new)/'; ";
    return;
  }

  if(appName.compare("qnet") == 0) return;

  if(appName.compare("qmos") == 0 ||
     appName.compare("./qmos") == 0) {
    cout << "complete " << appName.toStdString()
         << " 'n@*@f:*.[mM][oO][sS]@'; ";
    return;
  }

  if(appName.compare("cneteditor") == 0 ||
     appName.compare("./cneteditor") == 0) {
    cout << "complete " << appName.toStdString() << " 'n@*@f:*.[nN][eE][tT]@'; ";
    return;
  }

  if(appName.compare("ipce") == 0 || appName.compare("./ipce") == 0) {
    cout << "complete " << appName.toStdString() << " 'n@*@f:*.[pP][cC][eE]@'; ";
    return;
  }

  if(appName.compare("isisui") == 0) {
    std::string binPath = FileName("$ISISROOT/bin").expanded();
    cout << "complete isisui 'n@*@F:" << binPath << "/@'; ";
    return;
  }

  // Do not complete self
  if(appName.compare("isiscomplete") == 0) return;

  Application app(argc, argv);
  UserInterface &ui = Application::GetUserInterface();
  QString paramList = "";
  QString completeCommand;
  vector<QString> paramDetails;

  for(int grp = 0; grp < ui.NumGroups(); grp++) {
    for(int param = 0; param < ui.NumParams(grp); param++) {
      paramList += " " + ui.ParamName(grp, param).toLower();
      paramDetails.push_back(GetParamCompletion(grp, param));
    }
  }

  for(unsigned int param = 0; param < paramDetails.size(); param ++) {
    completeCommand += "'" + paramDetails[param] + "' ";
  }

  completeCommand += " " + BuildStaticCompletes(paramList) + " ";
  completeCommand += "'n/*/(-" + paramList + ")/='";
  completeCommand = "complete " + appName + " " + completeCommand + "; ";
  cout << completeCommand.toStdString();

  delete [] argv[1];
  argv[1] = 0;
}

QString BuildStaticCompletes(QString paramList) {
  QString completion = "";

  // Batchlist
  completion += " 'c/-[bB][aA][tT][cC][hH][lL][iI][sS][tT]=/f/'";

  // Errlist
  completion += " 'c/-[eE][rR][rR][lL][iI][sS][tT]=/f/'";

  // Help
  completion += " 'c/-[hH][eE][lL][pP]=/(" + paramList + ")/'";

  // Info
  completion += " 'c/-[iI][nN][fF][oO]=/f/'";

  // Log
  completion += " 'c/-[lL][oO][gG]=/f/'";

  // Onerror
  completion += " 'c/-[oO][nN][eE][rR][rR][oO][rR]=/(abort continue)/'";

  // Preference
  completion += " 'c/-[pP][rR][eE][fF][eE][rR][eE][nN][cC][eE]=/f/'";

  // Restore
  completion += " 'c/-[rR][eE][sS][tT][oO][rR][eE]=/f:*/'";

  // Save
  completion += " 'c/-[sS][aA][vV][eE]=/f/'";

  completion += " 'c/-/(batchlist= errlist= gui nogui help help= info info= last log log=";
  completion += " onerror= preference= restore= save save= verbose webhelp)//'";

  return completion;
}

QString GetParamCompletion(int grp, int param) {
  QString completion = "c/";
  UserInterface &ui = Application::GetUserInterface();


  QString paramName = ui.ParamName(grp, param);
  for(int curIndex = 0; curIndex < paramName.length(); curIndex++) {
    if(paramName[curIndex].isLetter()) {
      completion += "[";
      completion += paramName[curIndex].toUpper();
      completion += paramName[curIndex].toLower();
      completion += "]";
    }
    else {
      completion += paramName[curIndex];
    }
  }
  completion += "=/";

  QString type = ui.ParamType(grp, param);

  if(type == "cube") {
    completion += "f:*.[cC][uU][bB]";
  }
  else if(type == "filename") {
    completion += "f";
  }
  else {
    completion += "(" + ui.ParamDefault(grp, param) + ")";
  }

  completion += "/";

  return completion;
}
