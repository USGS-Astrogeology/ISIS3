#ifndef SessionLog_h
#define SessionLog_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <string>
#include <QString>
#include "Pvl.h"
#include "IException.h"

namespace Isis {
  /**
   * @author ????-??-?? Unknown
   *
   * @internal
   *   @history 2005-12-28 Elizabeth Miller - Added try/catch block in Write()
   *                           to catch an error that causes an abort
   *   @history 2008-01-09 Steven Lambright - Made Application a friend for
   *                           destruction purposes
   *   @history 2008-07-08 Steven Lambright - Now uses atexit for destruction
   */
  class SessionLog : private Isis::Pvl {
    public:
      static SessionLog &TheLog(bool restart = false);
      static bool HasLog() {
        return p_log != NULL;
      };

      // Add the Results group
      void AddResults(Isis::PvlGroup &results);

      // Write the log to the screen and/or file
      void Write();

      // Add an Error to the log
      void AddError(Isis::Pvl &e);

      // Will we be logging to the terminal?
      bool TerminalOutput() {
        return p_termOutput;
      };

    private:
      SessionLog();
      ~SessionLog();

      PvlObject *p_root;
      bool p_errorAdded;

      bool p_termOutput;
      bool p_fileOutput;
      QString p_outputFile;
      QString p_access;

      static SessionLog *p_log;

      bool p_acctAdded;
      void AddAccounting();
      friend std::ostream &operator<<(std::ostream &os, Isis::SessionLog &log);
      static void Shutdown();
  };
};

#endif
