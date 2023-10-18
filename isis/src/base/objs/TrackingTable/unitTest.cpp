/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>

#include "IException.h"
#include "Preference.h"
#include "Table.h"
#include "TableField.h"
#include "TableRecord.h"
#include "TrackingTable.h"

using namespace Isis;
using namespace std;

int main(int argc, char *argv[]) {

  Preference::Preferences(true);

  try{
    cout << "Unit test for TrackingTable\n" << endl;

    cout << endl;


    cout << "Testing default constructor ..." << endl;

    TrackingTable trackingTable1;

    trackingTable1.fileNameToPixel("fileName1.cub", "1");

    Table tableOut1 = trackingTable1.toTable();

    cout << "Record added: " << std::string(tableOut1[0][0]) << ", " << std::string(tableOut1[0][1]) << endl;

    cout << endl;


    cout << "Testing constructor with a Table object ..." << endl;

    cout << "Constructing Table ..." << endl;

    TableField fileNameField("FileName", TableField::Text, 50);
    TableField serialNumberField("SerialNumber", TableField::Text, 50);

    TableRecord record;
    record += fileNameField;
    record += serialNumberField;
    Table tableIn("TestingTable", record);

    record[0] = "fileName1.cub";
    record[1] = "1234567890";
    tableIn += record;

    record[0] = "fileName2.cub";
    record[1] = "123";
    tableIn += record;

    record[0] = "fileName3.dat";
    record[1] = "456789";
    tableIn += record;

    cout << "First record : " << (std::string)tableIn[0][0] << ", " << (std::string)tableIn[0][1] << endl;
    cout << "Second record: " << (std::string)tableIn[1][0] << ", " << (std::string)tableIn[1][1] << endl;
    cout << "Third record : " << (std::string)tableIn[2][0] << ", " << (std::string)tableIn[2][1] << endl;

    TrackingTable trackingTable2(tableIn);

    cout << "TrackingTable object created" << endl;

    cout << endl;


    cout << "Testing the pixelToFileName method ..." << endl;

    for (int i = 2; i < 7; i++) {
      try {
        cout << "FileName with pixel value " << i << ": "
             << trackingTable2.pixelToFileName(i).name() << endl;
      }
      catch (IException &e) {
        cout << "FileName with pixel value " << i 
             << ": does not exist and an exception is thrown." << endl;
        e.print();
      }
    }

    cout << endl;


    cout << "Testing the fileNameToPixel method ..." << endl;

    cout << "Pixel value of FileName fileName1.cub: "
           << trackingTable2.fileNameToPixel("fileName1.cub", "1234567890") << endl;
    cout << "Pixel value of FileName fileName2.cub: "
           << trackingTable2.fileNameToPixel("fileName2.cub", "123") << endl;
    cout << "Pixel value of FileName fileName3.cub: "
           << trackingTable2.fileNameToPixel("fileName3.dat", "456789") << endl;
    cout << "Pixel value of the non-existent FileName fileName4.cub (demonstrating its addition): "
           << trackingTable2.fileNameToPixel("fileName4.cub", "12345678901234567890") << endl;

    cout << endl;


    cout << "Testing the toTable method ..." << endl;

    Table tableOut2 = trackingTable2.toTable();

    cout << "First record : " << std::string(tableOut2[0][0]) << ", "
                              << std::string(tableOut2[0][1]) << ", "
                              << int(tableOut2[0][2]) << endl;
    cout << "Second record: " << std::string(tableOut2[1][0]) << ", "
                              << std::string(tableOut2[1][1]) << ", "
                              << int(tableOut2[1][2]) << endl;
    cout << "Third record : " << std::string(tableOut2[2][0]) << ", "
                              << std::string(tableOut2[2][1]) << ", "
                              << int(tableOut2[2][2]) << endl;
    cout << "Fourth record: " << std::string(tableOut2[3][0]) << ", "
                              << std::string(tableOut2[3][1]) << ", "
                              << int(tableOut2[3][2]) << endl;

    cout << endl;


    cout << "Creating a new TrackingTable object with the table returned from toTable method ..." << endl;

    TrackingTable trackingTable3(tableOut2);

    cout << "New TrackingTable object created" << endl;

    cout << endl;

    cout << "Verifying that the pixel values are the same ..." << endl;

    cout << "Pixel value of FileName fileName1.cub: "
         << trackingTable3.fileNameToPixel("fileName1.cub", "1234567890") << endl;
    cout << "Pixel value of FileName fileName2.cub: "
         << trackingTable3.fileNameToPixel("fileName2.cub", "123") << endl;
    cout << "Pixel value of FileName fileName3.cub: "
         << trackingTable3.fileNameToPixel("fileName3.dat", "456789") << endl;
    cout << "Pixel value of FileName fileName4.cub: "
         << trackingTable3.fileNameToPixel("fileName4.cub", "12345678901234567890") << endl;

    cout << endl;


    cout << "Testing that the Table returned from toTable on new TrackingTable matches ..." << endl;

    Table tableOut3 = trackingTable3.toTable();

    cout << "First record : " << std::string(tableOut3[0][0]) << ", "
                              << std::string(tableOut3[0][1]) << ", "
                              << int(tableOut3[0][2]) << endl;
    cout << "Second record: " << std::string(tableOut3[1][0]) << ", "
                              << std::string(tableOut3[1][1]) << ", "
                              << int(tableOut3[1][2]) << endl;
    cout << "Third record : " << std::string(tableOut3[2][0]) << ", "
                              << std::string(tableOut3[2][1]) << ", "
                              << int(tableOut3[2][2]) << endl;
    cout << "Fourth record: " << std::string(tableOut3[3][0]) << ", "
                              << std::string(tableOut3[3][1]) << ", "
                              << int(tableOut3[3][2]) << endl;

  }
  catch (IException &e) {
    cout << "Unit test failed." << endl;
    e.print();
  }


}
