#include "Isis.h"

#include "getsn.h"
#include "PvlGroup.h"
#include "Pvl.h"

#include "Blob.h"
#include "Cube.h"
#include "History.h"
#include "ObservationNumber.h"
#include "Preference.h"

#include "SerialNumber.h"
#include "SessionLog.h"
#include "TextFile.h"


using namespace Isis;
using namespace std;

void IsisMain() {

  // Set Preferences to always turn off Terminal Output
  PvlGroup &grp = Isis::Preference::Preferences().findGroup("SessionLog", Isis::Pvl::Traverse);
  grp["TerminalOutput"].setValue("Off");

  UserInterface &ui = Application::GetUserInterface();
  Pvl appLog;

  getsn(ui, &appLog);

  PvlGroup results = appLog.findGroup("Results");
  SessionLog::TheLog().AddResults(results);
}
