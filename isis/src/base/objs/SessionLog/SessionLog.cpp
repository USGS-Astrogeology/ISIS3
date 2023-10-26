/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "SessionLog.h"
#include "Preference.h"
#include "IString.h"
#include "Application.h"

using namespace std;
namespace Isis {
  // Constructor
  SessionLog::SessionLog() {
    // Grab the user preferences for logging
    Isis::PvlGroup &slog = Isis::Preference::Preferences().findGroup("SessionLog");
    p_termOutput = QString::fromStdString(slog["TerminalOutput"]).toUpper() == "ON";
    p_fileOutput = QString::fromStdString(slog["FileOutput"]).toUpper() == "ON";
    p_outputFile = QString::fromStdString(slog["FileName"]);
    p_access = (QString::fromStdString(slog["FileAccess"])).toUpper();

    // Add root
    this->addObject(Isis::iApp->History());
    p_errorAdded = false;
    p_acctAdded = false;
    p_root = &this->object(0);

    atexit(Shutdown);
  }

  // Destructor
  SessionLog::~SessionLog() {
  }

  // Singleton creator
  SessionLog *SessionLog::p_log = NULL;

  SessionLog &SessionLog::TheLog(bool restart) {
    if(restart && (p_log != NULL)) {
      delete p_log;
      p_log = NULL;
    }

    if(p_log == NULL) {
      // Create the singleton
      p_log = new SessionLog();
    }
    return *p_log;
  }

  // Write info to the session log
  void SessionLog::Write() {
    AddAccounting();

    // See if we should write to the print file
    if(p_fileOutput) {
      setTerminator("\n");
      try {
        if(p_access == "OVERWRITE") {
          this->Isis::Pvl::write(p_outputFile.toStdString());
        }
        else {
          this->append(p_outputFile.toStdString());
        }
      }
      catch(...) {
        std::cerr << "**WARNING** Unable to write session log [" <<
                  p_outputFile << "] Disk may be full or directory permissions not writeable"
                  << std::endl;
        exit(1);
      }
      setTerminator("End");
    }
  }

  void SessionLog::AddAccounting() {
    // Update accounting if no errors
    if(p_acctAdded) return;
    if(!p_errorAdded) {
      p_root->addGroup(Isis::iApp->Accounting());
    }
    p_acctAdded = true;
  }

  // Add an error message
  void SessionLog::AddError(Isis::Pvl &e) {
    for(int i = 0; i < e.groups(); i++) {
      if(e.group(i).isNamed("Error")) {
        p_root->addGroup(e.group(i));
        p_errorAdded = true;
      }
    }
  }

  // Add results from an application
  void SessionLog::AddResults(Isis::PvlGroup &results) {
    p_root->addGroup(results);
  }

  std::ostream &operator<<(std::ostream &os, Isis::SessionLog &log) {
    log.AddAccounting();
    return operator<<(os, (Isis::Pvl &) log);
  }

  void SessionLog::Shutdown() {
    if(p_log) {
      delete p_log;
      p_log = NULL;
    }
  }
} // end namespace isis

