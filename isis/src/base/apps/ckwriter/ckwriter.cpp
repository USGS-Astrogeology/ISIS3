#include <cmath>

#include "ckwriter.h"

#include "Application.h"
#include "Cube.h"
#include "FileList.h"
#include "Process.h"
#include "Pvl.h"
#include "SpiceKernel.h"

using namespace std;

namespace Isis {
  void ckwriter(UserInterface &ui, Pvl *log) {
    Process p;

    // Get the list of names of input CCD cubes to stitch together
    FileList flist;
    if (ui.WasEntered("FROM")) flist.push_back(FileName(ui.GetCubeName("FROM")));
    if (ui.WasEntered("FROMLIST")) flist.read(FileName(ui.GetFileName("FROMLIST")));
    if (flist.size() < 1) {
      QString msg = "Files must be specified in FROM and/or FROMLIST - none found!";
      throw IException(IException::User,msg,_FILEINFO_);
    }

    bool overlap_is_error = ( "ERROR" == ui.GetString("OVERLAP") ) ? true : false;

    SpiceKernel kernel;
    Progress prog;
    prog.SetMaximumSteps(flist.size());
    prog.CheckStatus();

    for (int i = 0 ; i < flist.size() ; i++) {
      // Add and process each image
      kernel.add(flist[i].toString());
      prog.CheckStatus();
    }

    try {
      kernel.validate();
    }
    catch ( IException &ie ) {

      // Check for user preference in treatment overlaps
      if ( overlap_is_error ) { throw; }

      //  Log it to file
      Pvl overrors = ie.toPvl();
      for(int i = 0; i < overrors.groups(); i++) {
        PvlGroup overlap = overrors.group(i);
        overlap.setName("Overlaps");
        overlap.addKeyword(PvlKeyword("Class", "WARNING"), PvlContainer::Replace);
        Application::AppendAndLog(overlap, log);
      }
    }


    //  Get comment file
    QString comfile("");
    if (ui.WasEntered("COMFILE")) comfile = ui.GetFileName("COMFILE");

    // Write the output file if requested
    if (ui.WasEntered("TO")) {
      int cktype = ui.GetInteger("CKTYPE");
      kernel.write(ui.GetFileName("TO"), comfile, cktype);
    }

    // Write a summary of the documentation
    if (ui.WasEntered("SUMMARY")) {
      QString fFile = FileName(ui.GetFileName("SUMMARY")).expanded();
      ofstream os;
      os.open(fFile.toLatin1().data(),ios::out);
      if (!os) {
        QString mess = "Cannot create SUMMARY output file " + fFile;
        throw IException(IException::User, mess, _FILEINFO_);
      }
      os << kernel.getSummary(comfile);
      os.close();
    }

    p.EndProcess();
  }
}
