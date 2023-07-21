#include <cmath>
#include "spkwriter.h"

#include <QString>
#include <QStringList>

#include "Application.h"
#include "IException.h"
#include "FileList.h"
#include "Cube.h"
#include "Process.h"
#include "Pvl.h"
#include "SpiceKernel.h"
#include "SpkSegment.h"
#include "Commentor.h"
#include "SpkKernelWriter.h"

using namespace std;

namespace Isis {
  typedef SpiceKernel<SpkSegment> SpkKernelContainer;

  /** Validation routine for SPK kernel segments */
  void validate(const SpkKernelContainer &kernel) {
    QStringList errors;
    for (int k = 1 ; k < kernel.size() ; k++) {
      if ( kernel.at(k).overlaps(kernel.at(k-1)) ) {
        errors  << "SPKSegment " + kernel.at(k).Id() + " overlaps SPKSegment "
                    + kernel.at(k-1).Id();
      }
    }

    //  Now check for problems
    if ( 0 < errors.size() ) {
       QString mess = "Time/body overlap conflicts are present in segment (image) list. "
                      "This will likely create erroneous positions in one or more "
                      "images.  You should create a seperate kernel for conflicting "
                      "images that overlap another.  Images with time/body overlap "
                      "conflicts are:   \n"
                      + errors.join("; ");
       throw IException(IException::User, mess, _FILEINFO_);
    }
    return;
  }


  void spkwriter(UserInterface &ui, Pvl *log) {
    Process p;

    // Get the list of names of input CCD cubes to stitch together
    FileList flist;
    if (ui.WasEntered("FROM")) flist.push_back(ui.GetCubeName("FROM"));
    if (ui.WasEntered("FROMLIST")) flist.read(ui.GetFileName("FROMLIST"));
    if (flist.size() < 1) {
      QString msg = "Files must be specified in FROM and/or FROMLIST - none found!";
      throw IException(IException::User,msg,_FILEINFO_);
    }

    bool overlap_is_error = ( "ERROR" == ui.GetString("OVERLAP") ) ? true : false;
    int spkType = ui.GetInteger("TYPE");

    SpkKernelContainer kernel;
    Progress prog;
    prog.SetMaximumSteps(flist.size());
    prog.CheckStatus();

    for (int i = 0 ; i < flist.size() ; i++) {
      // Add and process each image
      try {
        kernel.add(SpkSegment(flist[i].toString(), spkType));
      }
      catch (IException &ie) {
        QString mess = "Cannot create type 13 SPK. Please use type 9 or run jigsaw to create a "
                       " polynomical solution for the Spice Position.";
       throw IException(ie, IException::User, mess, _FILEINFO_);
      }
      prog.CheckStatus();
    }

    // Validate the segments
    try {
      validate(kernel);
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


    //  Process CK kernel requests
    QString comfile("");
    if (ui.WasEntered("COMFILE")) comfile = ui.GetFileName("COMFILE");

    SpkKernelWriter kwriter(spkType);

    // Write the output file if requested
    if (ui.WasEntered("TO")) {
      kwriter.write(kernel, ui.GetFileName("TO"), comfile);
    }

    // Write a summary of the documentation
    if (ui.WasEntered("SUMMARY")) {
      QString fFile = FileName(ui.GetFileName("SUMMARY")).expanded();
      ofstream os;
      os.open(fFile.toLatin1().data(),ios::out);
      if (!os) {
        QString mess = "Cannot create SPK SUMMARY output file " + fFile;
        throw IException(IException::User, mess, _FILEINFO_);
      }
      os << kwriter.getComment(kernel, comfile) << endl;
      os.close();
    }
    p.EndProcess();
  }
}
