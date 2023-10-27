/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "FileName.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "iTime.h"

using namespace Isis;


void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  // Open the input file from the GUI or find the latest version of the DB file
  FileName inputName;
  if (ui.WasEntered("FROM")) {
    inputName = ui.GetFileName("FROM");
  }
  else {
    // Stores highest version
    inputName = "$cassini/kernels/pck/kernels.????.db";
    inputName = inputName.highestVersion();
  }

  // Read PCK DB file into a PVL and search the PVL for the main object
  Pvl db(inputName.expanded().toStdString());
  PvlObject &main = db.findObject("TargetAttitudeShape");

  // Add a timestamp for when this file was created
  PvlObject latestMain("TargetAttitudeShape");
  latestMain += PvlKeyword("RunTime", iTime::CurrentLocalTime().toStdString());

  // Add our dependencies, only the Leapsecond Kernel
  PvlGroup dependencies("Dependencies");
  FileName lskName("$base/kernels/lsk/naif????.tls");
  lskName = lskName.highestVersion();
  QString lskString = lskName.originalPath() + "/" + lskName.name();
  dependencies += PvlKeyword("LeapsecondKernel", lskString.toStdString());
  latestMain += dependencies;

  for (int g = 0; g < main.groups(); g++) {
    PvlGroup &group = main.group(g);

    // Look for Selection groups with date-versioned filenames
    if (group.isNamed("Selection")) {
      // Copy the Selection group in case we need to replace one or more
      // date-versioned filenames
      PvlGroup latestGroup = group;

      bool hasDateVersioning = false;
      for (int k = 0; k < group.keywords(); k++) {
        PvlKeyword &keyword = group[k];
        if (keyword.isNamed("File")) {
          FileName pckName(QString::fromStdString(keyword[0]));
          if (pckName.isDateVersioned()) {
            pckName = pckName.highestVersion();
            QString latestPck = pckName.originalPath() + "/" + pckName.name();

            // Replace the date-versioned filename with the direct path to the
            // latest PCK
            PvlKeyword &latestKeyword = latestGroup[k];
            latestKeyword[0] = latestPck.toStdString();

            hasDateVersioning = true;
          }
        }
      }

      // When we find date versioning, we need to add the a group without it to
      // the PVL
      if (hasDateVersioning) {
        // Add back the date-versioned path for new versions of Isis
        latestMain.addGroup(group);

        // Add comment specifying that this PCK is hardcoded for legacy support
        QString comment = "This PCK is hardcoded to support versions of "
          "Isis prior to v3.3.2";
        latestGroup.addComment(comment.toStdString());

        // Add the direct path to the DB file to support older versions of Isis
        // that do not support date-versioned filenames
        latestMain.addGroup(latestGroup);
      }
    }
  }

  // Determine whether we want to update the data area directly or write to a
  // user-specified location
  FileName outputName;
  if (ui.WasEntered("TO")) {
    outputName = ui.GetFileName("TO");
  }
  else {
    outputName = "$cassini/kernels/pck/kernels.????.db";
    outputName = outputName.newVersion();
  }

  // Write the updated PVL as the new PCK DB file
  Pvl latestDb;
  latestDb.addObject(latestMain);
  latestDb.write(outputName.expanded().toStdString());
}
