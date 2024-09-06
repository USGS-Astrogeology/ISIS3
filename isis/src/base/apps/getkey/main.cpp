#define GUIHELPERS

#include "Isis.h"

#include <QString>
#include <QStringList>
#include <sstream>
#include "Process.h"
#include "Pvl.h"
#include "SessionLog.h"

using namespace std;
using namespace Isis;

//helper button functins in the code
void helperButtonLog();

map <QString, void *> GuiHelpers() {
  map <QString, void *> helper;
  helper ["helperButtonLog"] = (void *) helperButtonLog;
  return helper;
}

void IsisMain() {
  // Set Preferences to always turn off Terminal Output
  PvlGroup &grp = Isis::Preference::Preferences().findGroup("SessionLog", Isis::Pvl::Traverse);
  grp["TerminalOutput"].setValue("Off");
 
  // Use a regular Process
  Process p;

  // Get the input file from the user interface
  UserInterface &ui = Application::GetUserInterface();
  QString labelFile = ui.GetCubeName("FROM");

  // Open the file ... it must be a label-type file
  Pvl lab;
  lab.read(labelFile.toStdString());
  bool recursive = ui.GetBoolean("RECURSIVE");

  // Set up the requested object
  PvlKeyword key;
  if(ui.WasEntered("OBJNAME")) {
    QString obj = ui.GetString("OBJNAME");

    // Get the keyword from the entered group
    if(ui.WasEntered("GRPNAME")) {
      PvlObject object = lab.findObject(obj.toStdString(), Pvl::Traverse);
      QString grp = ui.GetString("GRPNAME");
      key = object.findGroup(grp.toStdString(), Pvl::Traverse)[ui.GetString("KEYWORD").toStdString()];
    }
    // Find the keyword in the object
    else {
      if(recursive) {
        key = lab.findObject(obj.toStdString(), Pvl::Traverse).findKeyword(ui.GetString("KEYWORD").toStdString(), Pvl::Traverse);
      }
      else {
        key = lab.findObject(obj.toStdString(), Pvl::Traverse)[ui.GetString("KEYWORD").toStdString()];
      }
    }
  }

  // Set up the requested group
  else if(ui.WasEntered("GRPNAME")) {
    QString grp = ui.GetString("GRPNAME");
    key = lab.findGroup(grp.toStdString(), Pvl::Traverse)[ui.GetString("KEYWORD").toStdString()];
  }

  // Find the keyword in the label, outside of any object or group
  else {
    if(recursive) {
      key = lab.findKeyword(ui.GetString("KEYWORD").toStdString(), Pvl::Traverse);
    }
    else {
      key = lab[ui.GetString("KEYWORD").toStdString()];
    }
  }

  QString value;
  if(ui.WasEntered("KEYINDEX")) {
    int i = ui.GetInteger("KEYINDEX");

    // Make sure they requested a value inside the range of the list
    if(key.size() < i) {
      std::string msg = "The value entered for [KEYINDEX] is out of the array ";
      msg += "bounds for the keyword [" + ui.GetString("KEYWORD") + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    // Get the keyword value
    else value = QString::fromStdString(key[i-1]);
  }
  else {

    if(key.size() > 1) {

      QStringList values;

      for (int i = 0; i < key.size(); i++) {
        QString thisValue = QString::fromStdString(key[i]);

        if (thisValue.contains (" "))
          thisValue = "\"" + thisValue + "\"";

        values << thisValue;
      }
           
      value = values.join( ", ");
    }
    // Just get the keyword value since it isnt a list
    else value = QString::fromStdString(key);
  }

  if(ui.GetBoolean("UPPER")) value = value.toUpper();

  // Construct a label with the results
  PvlGroup results("Results");
  results += PvlKeyword("From", labelFile.toStdString());
  results += PvlKeyword(ui.GetString("KEYWORD").toStdString(), value.toStdString());
  if(ui.IsInteractive()) {
    Application::GuiLog(results);
  }
  else {
    cout << value << endl;
  }
  // Write the results to the log but not the terminal
  SessionLog::TheLog().AddResults(results);
}

//Helper function to output the input file to log.
void helperButtonLog() {
  UserInterface &ui = Application::GetUserInterface();
  QString file(ui.GetCubeName("FROM"));
  Pvl p;
  p.read(file.toStdString());
  Application::GuiLog(p);
}
//...........end of helper function LogMap ........
