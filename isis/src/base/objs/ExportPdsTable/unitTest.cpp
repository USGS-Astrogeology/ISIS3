/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "ExportPdsTable.h"

#include <QFile>

#include "FileName.h"
#include "IException.h"
#include "ImportPdsTable.h"
#include "Preference.h"
#include "Table.h"
#include "TableField.h"
#include "TableRecord.h"

using namespace std;
using namespace Isis;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  // Create an Isis Table with each type of field
  TableField dblField("Double Value", TableField::Double);
  TableField intField("Integer Value", TableField::Integer);
  TableField textField("Text Value", TableField::Text, 2);
  TableField realField("Real Value", TableField::Real);
  TableRecord record;
  record += dblField;
  record += intField;
  record += textField;
  record += realField;
  Table table("TableToExport", record);
  record[0] = 3.14159;  
  record[1] = 3;  
  record[2] = "PI";  
  record[3] = (float) (22.0/7.0);
  table += record; 
  record[0] = 2.71828;  
  record[1] = 2;  
  record[2] = "e";  
  record[3] = (float) (1/1 + 1/1 + 1/(1*2) + 1/(1*2*3) + 1/(1*2*3*4) + 1/(1*2*3*4*5));
  table += record; 

  // We will export and reimport this table (LSB export and MSB export) 
  ExportPdsTable exportedTable(table);
  int tableRecBytes = 1*8 + 1*4 + 1*2 + 1*4;
  char *buf;

  cout << "Testing export LSB with table containing each field type..." << endl;
  buf = new char[tableRecBytes*table.Records()];
  PvlObject metadata = exportedTable.exportTable(buf, tableRecBytes, "lsb");
  // Create a label file containing the needed information.
  FileName lsbLabelFile("$temporary/lsbPdsTable.lbl");
  ofstream outputLsbLabel((lsbLabelFile.expanded()).c_str());
  outputLsbLabel << PvlKeyword("RECORD_TYPE", "FIXED_LENGTH") << endl;
  outputLsbLabel << PvlKeyword("RECORD_BYTES", toString(tableRecBytes)) << endl;
  QString tableName = ExportPdsTable::formatPdsTableName(QString::fromStdString(table.Name()));
  outputLsbLabel << PvlKeyword("^" + tableName.toStdString(), "lsbPdsTable.dat") << endl;
  outputLsbLabel << endl;
  // Add table object to label keywords
  outputLsbLabel << metadata;
  outputLsbLabel.close();
  FileName lsbTableFile("$temporary/lsbPdsTable.dat");
  ofstream outputLsbTable((lsbTableFile.expanded()).c_str());
  outputLsbTable.write(buf, tableRecBytes*table.Records());
  outputLsbTable.close();
  ImportPdsTable lsbTable(QString::fromStdString(lsbLabelFile.expanded()), QString::fromStdString(lsbTableFile.expanded()), tableName);
  Table reimportedLsbTable = lsbTable.importTable("ReimportedLsbTable");
  // Loop through for each record
  cout << reimportedLsbTable.Name() << endl;
  cout << reimportedLsbTable[0][0].name() << "\t";
  cout << reimportedLsbTable[0][1].name() << "\t";
  cout << reimportedLsbTable[0][2].name() << "\t";
  cout << reimportedLsbTable[0][3].name() << "\n";
  for (int i = 0; i < reimportedLsbTable.Records(); i++) {
    cout << toString((double) reimportedLsbTable[i][0]) << "\t\t\t";
    cout << toString((int)    reimportedLsbTable[i][1]) << "\t\t\t\t";
    cout <<         (std::string) reimportedLsbTable[i][2]  << "\t\t\t";
    cout << toString((float)  reimportedLsbTable[i][3]) << "\n";
  }
  // remove files and reset buffer
  QFile::remove(QString::fromStdString(lsbLabelFile.expanded()));
  QFile::remove(QString::fromStdString(lsbTableFile.expanded()));
  delete [] buf;
  buf = NULL;
  cout << endl;

  cout << "Testing export MSB with table containing each field type..." << endl;
  cout << "In this case, record bytes > number of bytes of actual data. "
           "So padding will be added to the end of each record." << endl;
  tableRecBytes += 4;
  buf = new char[tableRecBytes*table.Records()];
  metadata = exportedTable.exportTable(buf, tableRecBytes, "msb");
  // Create a label file containing the needed information.
  FileName msbLabelFile("$temporary/msbPdsTable.lbl");
  ofstream outputMsbLabel((msbLabelFile.expanded()).c_str());
  outputMsbLabel << PvlKeyword("RECORD_TYPE", "FIXED_LENGTH") << endl;
  outputMsbLabel << PvlKeyword("RECORD_BYTES", toString(tableRecBytes)) << endl;
  outputMsbLabel << PvlKeyword("^" + tableName.toStdString(), "msbPdsTable.dat") << endl;
  outputMsbLabel << endl;
  // Add table object to label keywords
  outputMsbLabel << metadata;
  outputMsbLabel.close();
  FileName msbTableFile("$temporary/msbPdsTable.dat");
  ofstream outputMsbTable((msbTableFile.expanded()).c_str());
  outputMsbTable.write(buf, tableRecBytes*table.Records());
  outputMsbTable.close();
  ImportPdsTable msbTable(QString::fromStdString(msbLabelFile.expanded()), QString::fromStdString(msbTableFile.expanded()), tableName);
  Table reimportedMsbTable = msbTable.importTable("ReimportedMsbTable");
  // print the table
  cout << reimportedMsbTable.Name() << endl;
  cout << reimportedMsbTable[0][0].name() << "\t";
  cout << reimportedMsbTable[0][1].name() << "\t";
  cout << reimportedMsbTable[0][2].name() << "\t";
  cout << reimportedMsbTable[0][3].name() << "\n";
  for (int i = 0; i < reimportedMsbTable.Records(); i++) {
    cout << IString((double) reimportedMsbTable[i][0]) << "\t\t\t";
    cout << IString((int)    reimportedMsbTable[i][1]) << "\t\t\t\t";
    cout << IString((std::string) reimportedMsbTable[i][2]) << "\t\t\t";
    cout << IString((float)  reimportedMsbTable[i][3]) << "\n";
  }
  // remove files and reset buffer and tableRecBytes
  QFile::remove(QString::fromStdString(msbLabelFile.expanded()));
  QFile::remove(QString::fromStdString(msbTableFile.expanded()));
  delete [] buf;
  buf = NULL;
  tableRecBytes -= 4;
  cout << endl;


  cout << "Testing errors ..." << endl;
  buf = new char[tableRecBytes*table.Records()];
  try {
    metadata = exportedTable.exportTable(buf, tableRecBytes - 1, "LSB");
  }
  catch (IException &e) {
    e.print();
  }
  // reset buffer
  delete [] buf;
  buf = NULL;
  buf = new char[tableRecBytes*table.Records()];
  try {
    metadata = exportedTable.exportTable(buf, tableRecBytes, "L");
  }
  catch (IException &e) {
    e.print();
  }
  // reset buffer
  delete [] buf;
  buf = NULL;
  cout << endl;

  cout << "Testing static method, formatPdsTableName()..." << endl;
  cout << "formatPdsTableName(Table) = " << ExportPdsTable::formatPdsTableName("Table").toStdString() << endl;
  cout << "formatPdsTableName(IsisTable) = " << ExportPdsTable::formatPdsTableName("IsisTable").toStdString() << endl;
  cout << "formatPdsTableName(Isis) = " << ExportPdsTable::formatPdsTableName("Isis").toStdString() << endl;
  cout << "formatPdsTableName(CamelCase) = " << ExportPdsTable::formatPdsTableName("CamelCase").toStdString() << endl;
  cout << "formatPdsTableName(CamelCase2) = " << ExportPdsTable::formatPdsTableName("CamelCase2").toStdString() << endl;
}

