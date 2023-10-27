#include "Isis.h"

#include <iostream>

#include "FileName.h"
#include "Pvl.h"
#include "UserInterface.h"

using namespace Isis;
using namespace std;


void IsisMain() {

  // Get filename provided by the user
  UserInterface &ui = Application::GetUserInterface();
  QString file = ui.GetCubeName("FROM");
  
  // Extract label from file
  Pvl label(file.toStdString());

  // Output to file if entered
  if(ui.WasEntered("TO")) {
    if (ui.GetBoolean("APPEND")) {
      label.append(FileName(ui.GetFileName("TO")).expanded().toStdString());
    }
    else {
      label.write(FileName(ui.GetFileName("TO")).expanded().toStdString());
    }
  }

  // Print label to the gui log if it is interactive
  else if(ui.IsInteractive()) {
    Application::GuiLog(label);
  }

  // Print label to the screen if it is not
  else {
    cout << label << endl;
  }

}


