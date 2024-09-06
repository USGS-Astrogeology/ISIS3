/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

// $Id$
#include "Isis.h"
#include "FileList.h"
#include "FileName.h"
#include "Pvl.h"
#include "Process.h"
#include "ProgramAnalyzer.h"
#include "iTime.h"
#include "IException.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  Process p;
  ProgramAnalyzer analyzer;

  // Get the list of names of input CCD cubes to stitch together
  UserInterface &ui = Application::GetUserInterface();

  //  Add program exclusions
  if (ui.WasEntered("EXCLUDE")) analyzer.setExclude(ui.GetString("EXCLUDE"));
  if (ui.WasEntered("EXCLUDEFROM") ) {
    FileList elist(ui.GetFileName("EXCLUDEFROM"));
    for (int i = 0 ; i < elist.size() ; i++ ) {
      analyzer.exclude(elist[i].toString());
    }
  }

  // Add program inclusions
  if (ui.WasEntered("INCLUDE")) analyzer.setInclude(ui.GetString("INCLUDE"));
  if (ui.WasEntered("INCLUDEFROM") ) {
    FileList ilist(ui.GetFileName("INCLUDEFROM"));
    for (int i = 0 ; i < ilist.size() ; i++ ) {
      analyzer.include(ilist[i].toString());
    }
  }

  // Add the file
  analyzer.add(ui.GetFileName("FROM"));

  //  Log results
  PvlGroup logger = analyzer.review();
  Application::GuiLog(logger);
  Application::Log(logger);

  logger = analyzer.cumulative();
  Application::GuiLog(logger);
  Application::Log(logger);


  // Write the output file if requested for individual unique program summaries
  if(ui.WasEntered("SUMMARY")) {
    Pvl temp;
    temp.addGroup(analyzer.review());
    temp.addGroup(analyzer.cumulative());
    for (int i = 0 ; i < analyzer.Programs() ; i++) {
      temp.addGroup(analyzer.summarize(i));
    }
    temp.write(ui.GetFileName("SUMMARY").toStdString());
  }

  // Write the output file if requested of CSV formatted data
  if(ui.WasEntered("LOG")) {
   // Set up for opening
    FileName temp(ui.GetFileName("LOG"));
    QString file = temp.expanded();
    ofstream ostm;

    // Open the file
    ostm.open(file.toLatin1().data(), std::ios::out);
    if(!ostm) {
      std::string message = "Cannot open/create output file " + file;
      throw IException(IException::Io, message, _FILEINFO_);
    }

    analyzer.header(ostm);
    analyzer.listify(ostm);
    ostm.close();
  }

  p.EndProcess();
}
