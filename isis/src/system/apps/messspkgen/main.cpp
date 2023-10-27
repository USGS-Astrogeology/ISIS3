/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <iostream>
#include <sstream>
#include <QString>

#include <QString>

#include "FileName.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"


using namespace Isis;


QString convertUtcToTdb(QString utcTime);


void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  // Open the input file from the GUI or find the latest version of the DB file
  FileName dbFileName;
  if (ui.WasEntered("FROM")) {
    dbFileName = ui.GetFileName("FROM");
  }
  else {
    QString dbString("$messenger/kernels/spk/kernels.????.db");
    dbFileName = FileName(dbString).highestVersion();
  }
  Pvl kernelDb(dbFileName.expanded().toStdString());

  // Get our main objects
  PvlObject &position = kernelDb.findObject("SpacecraftPosition");

  // Pull out the reconstructed group and set the ending time to our orbit
  // cutoff
  PvlGroup &reconstructed = position.findGroup("Selection");
  PvlKeyword &time = reconstructed[reconstructed.keywords() - 3];
  QString reconstructedEnd = QString::fromStdString(time[1]);
  time[1] = convertUtcToTdb(ui.GetString("TIME")).toStdString();

  // Get the predicted group from the previous file, set the start time to the
  // orbit cutoff and the end to whatever the reconstructed end was before
  PvlGroup predicted("Selection");

  PvlKeyword predictedTime("Time");
  predictedTime += time[1];
  predictedTime += reconstructedEnd.toStdString();
  predicted.addKeyword(predictedTime);

  PvlKeyword predictedFile("File");
  predictedFile += reconstructed.findKeyword("File")[0];
  predicted.addKeyword(predictedFile);

  predicted.addKeyword(PvlKeyword("Type", "Predicted"));

  // Add the modified predicted group to the new DB file
  position.addGroup(predicted);

  // Get the output filename, either user-specified or the latest version for
  // the kernels area (as run by makedb)
  FileName outDBfile;
  if (ui.WasEntered("TO")) {
    outDBfile = ui.GetFileName("TO");
  }
  else {
    outDBfile = dbFileName;
  }

  // Write the updated PVL as the new SPK DB file
  kernelDb.write(outDBfile.expanded().toStdString());
}


QString convertUtcToTdb(QString utcTime) {
  // Remove any surrounding whitespace and the trailing " UTC", then replace
  // with TDB syntax
  QString orbitCutoffRaw = utcTime;
  (void)orbitCutoffRaw.trimmed();  // cast to void to silence unused reult warning
  orbitCutoffRaw.remove(QRegExp(" UTC$"));

  // We need to swap around the day and the year in order to go from UTC to TDB.
  // The year will be the first and only occurrence of 4 numbers in a row, so
  // pull that out.
  QRegExp yearRx("(\\d{4})");
  int pos = yearRx.indexIn(orbitCutoffRaw);
  QString year = (pos > -1) ? yearRx.cap(1) : "";

  // The day will come at the beginning of the QString, and will be 1 or 2
  // characters.  If it's only 1, add an extra 0 to make it 2.
  QRegExp dayRx("(^\\d{1,2})");
  pos = dayRx.indexIn(orbitCutoffRaw);
  QString day = (pos > -1) ? dayRx.cap(1) : "";
  if (day.length() == 1) day = "0" + day;

  // Swap the day and year
  orbitCutoffRaw.replace(yearRx, day);
  orbitCutoffRaw.replace(dayRx, year);

  // Tack on the necessary TDB tail
  QString orbitCutoff = orbitCutoffRaw;
  orbitCutoff += ".000 TDB";
  return orbitCutoff;
}
