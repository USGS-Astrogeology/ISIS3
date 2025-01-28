#define GUIHELPERS
#include "Isis.h"

#include "Application.h"

#include "pixel2map.h"

#include "UserInterface.h"

using namespace std;
using namespace Isis;

void PrintMap();

// Check for docs for GUI helpers -> KEEP and move it to pixel2map.cpp 
  std::map <QString, void *> GuiHelpers() {
  map <QString, void *> helper;
  helper ["PrintMap"] = (void *) PrintMap;
  return helper;
}


void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  Pvl appLog;
  pixel2map(ui, &appLog);
  
  if( ui.WasEntered("TO") && ui.IsInteractive() ) {
    Application::GuiLog(appLog);
  }
    
}


/**
  * Helper function to print out mapfile to session log
  */
void PrintMap() {
//removed in the refactoring process 
UserInterface &ui = Application::GetUserInterface();

  // Get mapping group from map file
  Pvl userMap(ui.GetFileName("MAP"));
  PvlGroup &userGrp = userMap.findGroup("Mapping", Pvl::Traverse);

  //Write map file out to the log
//
  Isis::Application::GuiLog(userGrp);
} // PrintMap

  

