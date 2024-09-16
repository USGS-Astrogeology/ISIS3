/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <iostream>
#include <sstream>
#include <QString>

#include <QList>
#include <QMap>
#include <QSet>
#include <QString>

#include "FileName.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TextFile.h"

using namespace Isis;
using std::endl;
using std::ios;
using std::ofstream;

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  // Fetch the DEM DB file
  FileName dbFileName;
  if (ui.WasEntered("FROM")) {
    dbFileName = ui.GetFileName("FROM").toStdString();
  }
  else {
    // If not provided, assume the latest DB file in the data area
    QString demFile("$base/dems/kernels.????.db");
    dbFileName = FileName(demFile.toStdString()).highestVersion();
  }

  Pvl dems(dbFileName.expanded());
  PvlObject &demObject = dems.findObject("Dem");

  QMap< QString, QList<QString> > demMap;
  for (int i = 0; i < demObject.groups(); i++) {
    PvlGroup &group = demObject.group(i);

    if (group.isNamed("Selection")) {
      PvlKeyword &match = group.findKeyword("Match");
      PvlKeyword &file = group.findKeyword("File");

      // The third element in the Match keyword describes the DEM target (e.g.
      // Mars)
      std::string target = match[2];

      // The first element of the File keyword gives the "mission" associated
      // with the keyword (currently, always "base").  The second element gives
      // the path from "base" to the actual DEM cube.
      std::string mission = file[0];
      std::string area = "$" + file[0];
      std::string pattern = file[1];

      // Some DEMs are hardcoded, but others are versioned.
      FileName demFileName(area + "/" + pattern);
      if (demFileName.isVersioned()) demFileName = demFileName.highestVersion();

      // Find the corresponding Isis Preference if one exists
      if (Preference::Preferences().hasGroup("DataDirectory")) {
        PvlGroup &dataDir =
          Preference::Preferences().findGroup("DataDirectory");

        // If the mission name maps to a data area in the Isis Preferences file,
        // then replace the variable $MISSION with the path to that area
        if (dataDir.hasKeyword(mission)) {
          area = (std::string)dataDir[mission];
        }
      }

      // Construct the relative path with environment variable placeholders in
      // tact for outputting to XML
      std::string filePath = area + "/dems/" + demFileName.name();

      // Add this filename to the list of DEMs corresponding to its target.  If
      // that list doesn't already exist, create it first.
      if (!demMap.contains(QString::fromStdString(target))) demMap.insert(QString::fromStdString(target), QList<QString>());
      demMap[QString::fromStdString(target)].append(QString::fromStdString(filePath));
    }
  }

  // Prepare to write out the output XML.
  FileName outFile = ui.GetFileName("TO").toStdString();
  ofstream os;
  os.open(outFile.expanded().c_str(), ios::out);

  // Write the installation XML
  os << "<packs>";
  QList<QString> keys = demMap.keys();
  for (int i = 0; i < keys.size(); i++) {
    os << "<pack name=\"" << keys[i].toStdString() << "\">";
    QList<QString> fileList = demMap[keys[i]];
    for (int j = 0; j < fileList.size(); j++) {
      QString izpackFile = fileList[j].replace("ISISDATA", "{ENV[ISISDATA]}");
      os << "<file src=\"" << izpackFile.toStdString() << "\" />";
    }
    os << "</pack>";
  }
  os << "</packs>" << endl;
}
