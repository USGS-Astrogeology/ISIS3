/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Application.h"
#include "Cube.h"

#include "FileName.h"
#include "FileList.h"
#include "IException.h"
#include "iTime.h"

#include "Pvl.h"
#include "PvlKeyword.h"
#include "Table.h"
#include "TableField.h"
#include "TableRecord.h"
#include "UserInterface.h"

#include <iostream>
#include <fstream>
#include <istream>

#include "eisstitch.h"
#include <QDebug>
#include <QPair>
#include <QString>
#include <QVector>

using namespace std;

// This will eventually change to account for the pushbroom
// framelets but is just a linescanner for now
struct eisTiming {
  double start;
  double lines;
  double exposureDuration;
  // Should be the start + (lineCounts * exposureTimes)
  double stop;

  eisTiming(double start, double lines, double exposureDuration) : 
            start(start), lines(lines), exposureDuration(exposureDuration)
  {
    stop = start + (lines * exposureDuration);
  }
};

bool compareByStartTime(const eisTiming &time1, const eisTiming &time2) {
  return time1.start < time2.start;
}

namespace Isis {

  void eisstitch(UserInterface &ui) {
    // Get the list of names of input cubes to stitch together
    FileList fileList;
    ui = Application::GetUserInterface();
    try {
      fileList.read(ui.GetFileName("FROMLIST"));
    }
    catch(IException &e) {
      QString msg = "Unable to read [" + ui.GetFileName("FROMLIST") + "]";
      IException(e, IException::User, msg, _FILEINFO_);
    }
    if (fileList.size() < 1) {
      QString msg = "The list file[" + ui.GetFileName("FROMLIST") +
                    " does not contain any filenames";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    std::vector<struct eisTiming> eisTime = {};

    for (auto file : fileList) {
      Pvl cubeLabel(file.expanded());

      PvlGroup instGroup;
      try {
        instGroup = cubeLabel.findGroup("Instrument", PvlObject::FindOptions::Traverse);
      }
      catch(IException &e) {
        QString msg = "Unable to find instrument group in [" + file.name() + "]";
        throw IException(e, IException::User, msg, _FILEINFO_); 
      }

      PvlGroup dimGroup;
      try {
        dimGroup = cubeLabel.findGroup("Dimensions", PvlObject::FindOptions::Traverse);
      }
      catch(IException &e) {
        QString msg = "Unable to find instrument group in [" + file.name() + "]";
        throw IException(e, IException::User, msg, _FILEINFO_); 
      }
      QString startTime = QString(instGroup.findKeyword("StartTime"));
      iTime time(startTime);
      int lines = dimGroup.findKeyword("Lines");
      double exposureDuration = instGroup.findKeyword("LineExposureDuration");
      exposureDuration /= 1000;
      eisTime.push_back(eisTiming(time.Et(), lines, exposureDuration));
    }

    sort(eisTime.begin(), eisTime.end(), compareByStartTime);

    // Check for overlap
    for (int i = 0; i < eisTime.size() - 1; i++) {
      if (eisTime[i].stop > eisTime[i + 1].start) {
        QString msg = "Image " + QString(i + 1) + " and " + QString(i + 2) + " in the image list have " + 
        "overlapping times.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    // Assume the images don't know about each other
    TableField ephTimeField("EphemerisTime", TableField::Double);
    TableField expTimeField("ExposureTime", TableField::Double);
    TableField lineStartField("LineStart", TableField::Integer);

    TableRecord timesRecord;
    timesRecord += ephTimeField;
    timesRecord += expTimeField;
    timesRecord += lineStartField;

    Table timesTable("LineScanTimes", timesRecord);

    int currentLine = 1;
    for (auto time : eisTime) {
      timesRecord[0] = time.start;
      timesRecord[1] = time.exposureDuration;
      timesRecord[2] = currentLine;
      currentLine += time.lines;
      timesTable += timesRecord;
    }
    cout << Table::toString(timesTable, ",") << endl;
  }
}