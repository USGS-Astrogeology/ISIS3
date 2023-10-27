#define GUIHELPERS

#include "Isis.h"

#include "mappt.h"

#include "Application.h"
#include "Pvl.h"

using namespace std;
using namespace Isis;

void PrintMap();

map <QString, void *> GuiHelpers() {
  map <QString, void *> helper;
  helper ["PrintMap"] = (void *) PrintMap;
  return helper;
}


void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  Pvl appLog;
  mappt(ui, &appLog);
}


// Helper function to print out mapfile to session log
void PrintMap() {
  UserInterface &ui = Application::GetUserInterface();

  // Get mapping group from map file
  Pvl userMap;
  userMap.read(ui.GetFileName("MAP").toStdString());
  PvlGroup &userGrp = userMap.findGroup("Mapping", Pvl::Traverse);

  //Write map file out to the log
  Isis::Application::GuiLog(userGrp);
}
