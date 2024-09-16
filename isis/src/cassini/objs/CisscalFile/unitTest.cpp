/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iostream>
#include <stdio.h>
#include <vector>

#include <QFile>

#include "CisscalFile.h"
#include "FileName.h"
#include "IException.h"
#include "Preference.h"

using namespace std;
using namespace Isis;

/**
 *
 * @internal
 * @history 2012-03-12 Tracie Sucharski - Replaced /tmp with $temporary
 *
 */
int main(int argc, char *argv[]) {
  Preference::Preferences(true);
  cout << endl << "Unit test for CisscalFile" << endl;
  cout << "--------------------------------------------" << endl;


// ----------------------------------------------------------------------------------

  QString testFile = "$temporary/CisscalFile.tmp";
  // setup test data
  QString testLines[8];

  // setup line test data
  testLines[0]  = "PDS_VERSION_ID    = PDS3";
  testLines[1]  = "RECORD_TYPE       = STREAM";
  testLines[2]  = "  ";
  testLines[3]  = "\\begindata";
  testLines[4]  = "data line 1";
  testLines[5]  = "data line 2";
  testLines[6]  = "data line 3: last line of data, next line (last in file) is empty";
  testLines[7] = "";

  // calc cumulative byte counts for each line - to check CisscalFile.Size()
  streamsize numBytes = 0;
  streamsize numBytesFiltered = 0;
  vector<QString> testLinesVector;

  for(int i = 0; i <= 7; i++) {
    numBytes += testLines[i].length() + strlen("\n");
    testLinesVector.push_back(testLines[i]);
  }

  numBytesFiltered = numBytes - numBytesFiltered;
  Isis::TextFile p(testFile, "overwrite", testLinesVector);
  p.Close();

// ----------------------------------------------------------------------------------

  cout << "1) Read valid data from " << testFile.toStdString() << endl;
  try {
    Isis::CisscalFile f(testFile);
    bool lineFound = false;
    QString line;
    // data line 1
    lineFound = f.GetLine(line);
    if (!lineFound) {
      cout << "First Line Not Found" << endl;
      return 0;
    }
    if (line != testLines[4]) {
      cout << " *** Failed to Find \"\\begindata\" Tag *** " << endl;
      cout << "   First line of data should be:   -> " << testLines[4].toStdString() << " <-" << endl;
      cout << "   returned is: -> " << line.toStdString() << " <-" << endl;
      return 0;
    }
    cout << line.toStdString() << endl;
    // data line 2
    lineFound = f.GetLine(line);
    if (!lineFound) {
      cout << "Second Line Not Found" << endl;
      return 0;
    }
    if (line != testLines[5]) {
      cout << " *** Failed to Match Second Line *** " << endl;
      cout << "   First line of data should be:   -> " << testLines[5].toStdString() << " <-" << endl;
      cout << "   returned is: -> " << line.toStdString() << " <-" << endl;
      return 0;
    }
    cout << line.toStdString() << endl;
    // data line 3
    lineFound = f.GetLine(line);
    if (!lineFound) {
      cout << "Third Line Not Found" << endl;
      return 0;
    }
    if (line != testLines[6]) {
      cout << " *** Failed to Match Third Line *** " << endl;
      cout << "   First line of data should be:   -> " << testLines[6].toStdString() << " <-" << endl;
      cout << "   returned is: -> " << line.toStdString() << " <-" << endl;
      return 0;
    }
    cout << line.toStdString() << endl;
    // last line, empty
    lineFound = f.GetLine(line);
    if (!lineFound) {
      cout << "Last Line Not Found" << endl;
      return 0;
    }
    if (line != testLines[7]) {
      cout << " *** Failed to Match Last Line *** " << endl;
      cout << "   First line of data should be:   -> " << testLines[7].toStdString() << " <-" << endl;
      cout << "   returned is: -> " << line.toStdString() << " <-" << endl;
      return 0;
    }
    cout << line.toStdString() << endl;
    // grab line beyond end of file
    lineFound = f.GetLine(line);
    if (lineFound) {
      cout << "Extra Line Found:   -> " << line.toStdString() << " <-" << endl;
      lineFound = f.GetLine(line);
      cout << "Next Line True?   -> " << lineFound << " <-" << endl;
      return 0;
    }
    f.Close();
    cout << "--------------------------------------------" << endl;
  }
  catch(Isis::IException &e) {
    e.print();
  }
//-----------------------------------------------------------------------------------
  cout << "2) Remove temp file -> " << testFile.toStdString() << " <-" << endl;
  if (!QFile::remove(QString::fromStdString(FileName(testFile.toStdString()).expanded()))) {
    cout << "*** Failed to remove tmp file: " << testFile.toStdString() << endl;
  }
  return 0;
}
