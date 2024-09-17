/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

//  $Id: mdisedrinfo.cpp,v 1.8 2008/07/11 22:38:41 nhilt Exp $
#include "Isis.h"

#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iostream>

#include "FileName.h"
#include "UserInterface.h"
#include "Cube.h"
#include "OriginalLabel.h"
#include "ProgramLauncher.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "IString.h"

#include "MdisGeometry.h"
#include "MdisEdrKeys.h"

using namespace Isis;
using namespace std;


void IsisMain() {

//  Input parameters
  UserInterface &ui = Application::GetUserInterface();
  QString sourceFile = ui.GetAsString("FROM");    //  Save off source filename
  FileName from(sourceFile.toStdString());

  QString to;
  if(ui.WasEntered("TO")) to = ui.GetAsString("TO");
  bool delete_from(false);

//  Is there a separate file containing keywords to report?
  QString keylist;
  if(ui.WasEntered("KEYLIST")) keylist = ui.GetAsString("KEYLIST");
  else keylist = to;

//  Get PVL parameter
  QString pvl;
  if(ui.WasEntered("PVL")) pvl = ui.GetAsString("PVL");

// Run mdis2isis if necessary.  If this step must be done, we must also
//  run spiceinit to initialize it with spice.
  if(IString(from.extension()).UpCase() != "CUB") {
    FileName temp = FileName::createTempFile("$TEMPORARY/" + from.baseName() + ".cub");
    QString params = QString::fromStdString("from=" + from.expanded()  + " to=" + temp.expanded());

    try {
      ProgramLauncher::RunIsisProgram("mdis2isis", params);

// Ensure a proper target before initialization
      Cube cube;
      cube.open(QString::fromStdString(temp.expanded()), "rw");
      Pvl *label = cube.label();
      MdisGeometry::validateTarget(*label, true);
      cube.close();

      //  Run spiceinit on it
      ProgramLauncher::RunIsisProgram("spiceinit", "from=" + QString::fromStdString(temp.expanded()));
    }
    catch(IException &ie) {
      QString tempName(QString::fromStdString(temp.expanded()));
      remove(tempName.toLatin1().data());
      throw IException(ie, IException::User,
                       "Failed to execute mdis2isis/spiceinit", _FILEINFO_);
    }

    //  FROM file is now the ISIS cube.  Stage cube file for deletion as well.
    from = temp;
    delete_from = true;
  }

//  Now process the label and instantiate the camera model(s)
  try {

//  Get the orginal PDS EDR labels and initialize the keyword map
    Pvl edrlab = OriginalLabel(from.expanded()).ReturnLabels();
    MdisEdrKeys edrkeys(edrlab);

//  Compute the Geometry
    MdisGeometry geom(QString::fromStdString(from.expanded()));
    Pvl geomkeys = geom.getGeometry(sourceFile);
    edrkeys.updateKeys(geomkeys);

    PvlGroup mdiskeys("MdisPdsKeys");

//  Only process the PDS EDR keyword mapping if KEYMAP (or TO) is entered.
    if(!keylist.isEmpty()) {
      FileName kmap(keylist.toStdString());
      if(!kmap.fileExists()) {
        std::string mess = "EDR keyword map source file, " + kmap.expanded()
                       + ", does not exist!";
        throw IException(IException::User, mess, _FILEINFO_);
      }

      // Get the keylist source line
      QString kmapName(QString::fromStdString(kmap.expanded()));
      ifstream ifile(kmapName.toLatin1().data(), ios::in);
      if(!ifile) {
        std::string mess = "Unable to open key map source file " + kmap.expanded();
        throw IException(IException::User, mess, _FILEINFO_);
      }

      string keystring;
      if(!getline(ifile, keystring)) {
        std::string mess = "I/O error reading key map line from  " + kmap.expanded();
        throw IException(IException::User, mess, _FILEINFO_);
      }

      ifile.close();

      //  Split the line using semi-colons as the delimiter
      QStringList keys = QString(keystring.c_str()).split(";");
      QString keyvalues = edrkeys.extract(keys, geom.getNull(), &mdiskeys);

      if(!to.isEmpty()) {
        //  Now open the output file and write the result
        FileName tomap(to.toStdString());
        QString tomapName(QString::fromStdString(tomap.expanded()));
        bool toExists = tomap.fileExists();
        ofstream ofile;
        if(toExists) {
          ofile.open(tomapName.toLatin1().data(), std::ios::out | std::ios::app);
        }
        else {
          ofile.open(tomapName.toLatin1().data(), std::ios::out);
        }

        if(!ofile) {
          std::string mess = "Could not open or create output TO file " +
                         tomapName.toStdString();
          throw IException(IException::User, mess, _FILEINFO_);
        }

        //  Write the header if requested by the user
        if(!toExists) ofile << keystring << endl;
        ofile << keyvalues.toStdString() << endl;
        ofile.close();
      }
    }
    else {
      PvlContainer::PvlKeywordIterator keyIter = geomkeys.begin();
      for(; keyIter != geomkeys.end() ;  ++keyIter) {
        mdiskeys.addKeyword(*keyIter);
      }
    }

//  See if the user wants to write out the PVL keywords
    if(!pvl.isEmpty()) {
      Pvl pout;
      pout.addGroup(mdiskeys);
      pout.write(pvl.toStdString());
    }

//  Log the results to the log/terminal/gui
    Application::Log(mdiskeys);
  }
  catch(IException &) {
    QString fromName(QString::fromStdString(from.expanded()));
    if(delete_from) remove(fromName.toLatin1().data());
    throw;
  }

// Delete the from file if it was temporarily created from an EDR
  QString fromName(QString::fromStdString(from.expanded()));
  if(delete_from) remove(fromName.toLatin1().data());
}
