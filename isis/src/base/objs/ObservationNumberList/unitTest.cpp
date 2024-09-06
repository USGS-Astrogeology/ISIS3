/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>

#include <QFile>

#include "FileName.h"
#include "IException.h"
#include "ObservationNumberList.h"
#include "Preference.h"
#include "SerialNumberList.h"

using namespace Isis;
using namespace std;

/**
 * Unit Test for ObservationNumberList
 *  
 * @internal
 *   @history 2010-07-02 Steven Lambright - Original version.
 *   @history 2016-06-03 Ian Humphrey - Improved test coverage. References #3990.
 */
int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  try {
    SerialNumberList snl(false);

    // All of these are unique observations (i.e. 4 observation#'s, 4 serial#'s)
    snl.add("$ISISTESTDATA/isis/src/mgs/unitTestData/ab102401.cub");
    snl.add("$ISISTESTDATA/isis/src/mgs/unitTestData/m0402852.cub");
    snl.add("$ISISTESTDATA/isis/src/lo/unitTestData/3133_h1.cub");
    snl.add("$ISISTESTDATA/isis/src/odyssey/unitTestData/I00824006RDR.lev2.cub");

    // Testing constructor that takes SerialNumberList
    ObservationNumberList onl(&snl);

    // Testing observationSize, hasObservationNumber, observationNumberMapIndex
    cout << "size             = " << onl.size() << endl;
    cout << "observationSize  = " << onl.observationSize() << endl;
    // hasObservationNumber -> false
    cout << "has XYZ          = " << onl.hasObservationNumber("XYZ") << endl;
    // hasObservationNumber -> true
    cout << "has LO3/HRC/3133 = " << onl.hasObservationNumber("LO3/HRC/3133") << endl;
    // observationNumberMapIndex for 3rd SN
    cout << "observationIndex for LO3/HRC/3133 = " << onl.observationNumberMapIndex(2) << endl;
    cout << endl;

    // Testing observationNumber(int)
    for (int i = 0; i < onl.size(); i++) {
      cout << FileName(onl.fileName(i)).name().toStdString() << " = " 
           << onl.observationNumber(i).toStdString() << endl;
    }

    cout << endl;
    // Testing possibleFileNames
    vector<QString> filenames = onl.possibleFileNames(onl.observationNumber(2));
    for (unsigned i = 0; i < filenames.size(); i++) {
      cout << "Possible filename for [" << onl.observationNumber(2).toStdString()
           << "]: " << FileName(filenames[i]).name().toStdString() << endl;
    }
    // Testing possibleSerialNumbers
    vector<QString> serials = onl.possibleSerialNumbers(onl.observationNumber(2));
    for (unsigned i = 0; i < serials.size(); i++) {
      cout << "Possible serial number for [" << onl.observationNumber(2).toStdString()
           << "]: " << serials[i].toStdString() << endl;
    }

    // Testing observationNumber(QString)
    cout << "File->ON:" << onl.observationNumber("$ISISTESTDATA/isis/src/mgs/unitTestData/ab102401.cub").toStdString() 
         << endl;

    cout << endl << "SN->File (0): " << FileName(snl.fileName(0)).name().toStdString() << endl;
    cout << "SN->File (1): " << FileName(snl.fileName(1)).name().toStdString() << endl;
    cout << "SN->File (2): " << FileName(snl.fileName(2)).name().toStdString() << endl << endl;

    if (onl.hasObservationNumber("NotAnObservation"))
      cout << "This line shouldn't be showing!" << endl;
    else
      cout << "[NotAnObservation] is not an existing ObservationNumber" << endl;

    // Test remove(QString) method (with a SN that doesn't exist in the list)
    cout << endl << endl;
    cout << "Removing a SerialNumberList that doesn't have any SNs in the ObservationNumberList"
         << endl;
    FileName temp1("$temporary/temp1list.txt");
    QFile tempFile1(temp1.expanded());
    tempFile1.open(QIODevice::WriteOnly | QIODevice::Text);
    tempFile1.write("$ISISTESTDATA/isis/src/odyssey/unitTestData/I56632006EDR.lev2.cub\n");
    tempFile1.close();

    onl.remove(temp1.expanded());
    cout << "size            = " << onl.size() << endl;
    cout << "observationSize = " << onl.observationSize() << endl;

    // Cleanup tempFile1
    tempFile1.remove();
  
    // Test remove(SerialNumberList *) (with a SN that exists in the list) -- Not sure if this is correct
    cout << endl << "Removing a SerialNumberList with one SN that exists in the "
         << "ObservationNumberList" << endl;
    SerialNumberList snlToRemove(false);
    snlToRemove.add("$ISISTESTDATA/isis/src/mgs/unitTestData/ab102401.cub");
    onl.remove(&snlToRemove);
    cout << "size            = " << onl.size() << endl;
    cout << "observationSize = " << onl.observationSize() << endl;
    cout << endl << endl;


    // Now, test where one observation has 2SN's
    SerialNumberList snl2(false);
    cout << "Creating an observation list with two observations and three SNs" << endl;
    snl2.add("$ISISTESTDATA/isis/src/odyssey/unitTestData/I00824006RDR.lev2.cub");
    snl2.add("$ISISTESTDATA/isis/src/lo/unitTestData/5106_h1.cropped.cub");
    snl2.add("$ISISTESTDATA/isis/src/lo/unitTestData/5106_h2.cropped.cub"); // Same observation as above
    
    ObservationNumberList onl2(&snl2);
    cout << "size            = " << onl2.size() << endl;
    cout << "observationSize = " << onl2.observationSize() << endl;
    cout << "observationIndex for I008... = " << onl2.observationNumberMapIndex(0) << endl;
    cout << "observationIndex for 5106_h1 = " << onl2.observationNumberMapIndex(1) << endl;
    // Should be same as above observation index
    cout << "observationIndex for 5106_h2 = " << onl2.observationNumberMapIndex(2) << endl;
    cout << endl;
    

    // Test add method -- Not sure if this is correct?
    cout << "Adding 5106_h3 to the list" << endl;
    onl2.add(onl2.size(), 1, "LO5/HRC/5106");
    cout << "size            = " << onl2.size() << endl;
    cout << "observationSize = " << onl2.observationSize() << endl;
    cout << "observationIndex for 5106_h3 = " << onl2.observationNumberMapIndex(3) << endl;
    cout << endl;

    // Test remove method on observation with 2 SNs -- Not sure if this is correct?
    cout << "Removing 5106_h2 SN from the list" << endl;
    SerialNumberList snlToRemove2(false);
    snlToRemove2.add("$ISISTESTDATA/isis/src/lo/unitTestData/5106_h2.cropped.cub");
    onl2.remove(&snlToRemove2);
    cout << "size            = " << onl2.size() << endl;
    cout << "observationSize = " << onl2.observationSize() << endl;
  
  }
  catch (IException &e) {
    e.print();
  }


  // Setup temp file - Need to cleanup later!
  FileName temp("$temporary/templist.txt");
  QFile tempFile(temp.expanded());
  tempFile.open(QIODevice::WriteOnly | QIODevice::Text);
  tempFile.write("$ISISTESTDATA/isis/src/mgs/unitTestData/ab102401.cub\n");
  tempFile.write("$ISISTESTDATA/isis/src/mgs/unitTestData/m0402852.cub\n");
  tempFile.write("$ISISTESTDATA/isis/src/lo/unitTestData/3133_h1.cub\n");
  tempFile.close();

  // Test 1st constructor
  ObservationNumberList onl(temp.expanded(), false);

  // Setup empty serial number list for exceptions
  SerialNumberList empty(false);

  // Test Exceptions
  try {
    cout << endl << endl;
    // Create ONL from empty SNL
    ObservationNumberList emptyONL(&empty);
  }
  catch (IException &e) {
    std::string error = e.toString().replace(QRegExp("(\\[[^\\]]*/)([^\\]]*)"), "[.../\\2");
    cerr << error.toStdString() << endl;
  }

  try {
    cout << endl << endl;
    // Remove empty serial number list
    onl.remove(&empty);
  }
  catch (IException &e) {
    std::string error = e.toString().replace(QRegExp("(\\[[^\\]]*/)([^\\]]*)"), "[.../\\2");
    cerr << error.toStdString() << endl;
  }

  try {
    cout << endl << endl;
    // Request observation number by providing invalid serial number index
    onl.observationNumberMapIndex(-1);
  }
  catch (IException &e) {
    std::string error = e.toString().replace(QRegExp("(\\[[^\\]]*/)([^\\]]*)"), "[.../\\2");
    cerr << error.toStdString() << endl;
  }
 
  try {
    cout << endl << endl;
    // Request an observation number from a file that doesn't exist in the observation list
    onl.observationNumber("$ISISTESTDATA/isis/src/odyssey/unitTestData/I00824006RDR.lev2.cub");
  }
  catch (IException &e) {
    std::string error = e.toString().replace(QRegExp("(\\[[^\\]]*/)([^\\]]*)"), "[.../\\2");
    cerr << error.toStdString() << endl;
  }

  try {
    cout << endl << endl;
    // Request observation number with an invalid index
    onl.observationNumber(5);
  }
  catch (IException &e) {
    std::string error = e.toString().replace(QRegExp("(\\[[^\\]]*/)([^\\]]*)"), "[.../\\2");
    cerr << error.toStdString() << endl;
  } 

  try {
    cout << endl << endl;
    // Request possible file names for observation number that isn't in the list
    onl.possibleFileNames("DNE");
  }
  catch (IException &e) {
    std::string error = e.toString().replace(QRegExp("(\\[[^\\]]*/)([^\\]]*)"), "[.../\\2");
    cerr << error.toStdString() << endl;
  }

  cout << endl << endl;

  // Cleanup the temp file
  tempFile.remove();

}
