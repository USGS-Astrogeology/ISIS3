#include "Isis.h"

#include <QString>

using namespace std;
using namespace Isis;

void IsisMain() {

  UserInterface &ui = Application::GetUserInterface();
  bool append = ui.GetBoolean("APPEND");
  Pvl input(ui.GetFileName("FROM").toStdString());
  Pvl output;

  // Check to see if output file exists
  FileName outFile = ui.GetFileName("TO");
  if(outFile.fileExists() && !append) {
    QString msg = "Output file [" + outFile.expanded() + "] already exists.";
    msg += " Append option set to False.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  int numErrors = 0;
  // Search for errors and add to output
  for(int i = 0; i < input.objects(); i++) {
    PvlObject o = input.object(i);
    if(o.hasGroup("Error")) {
      output.addObject(o);
      numErrors++;
    }
  }
  PvlKeyword errors("TotalErrors", std::to_string(numErrors));
  output.addKeyword(errors);
  // write output to file
  if(!append) {
    output.write(outFile.expanded().toStdString());
  }
  else {
    output.append(outFile.expanded().toStdString());
  }
  cout << errors << endl;
}
