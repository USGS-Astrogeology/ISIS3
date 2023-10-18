/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

#include <QString>
#include <QStringList>

#include "IException.h"
#include "ImportPdsTable.h"
#include "IString.h"
#include "Preference.h"
#include "Table.h"
#include "TextFile.h"
#include "FileName.h"

using namespace std;
using namespace Isis;


/**
 * Test class that allows testing protected methods in ImportPdsTable
 *
 * @author 2016-02-24 Ian Humphrey
 *
 * @internal
 *   @history 2016-02-24 Ian Humphrey - Original version (created for #2397).
 */
class ImportPdsTableTester : public ImportPdsTable {

  public:
    ImportPdsTableTester(const QString &labelFile, const QString &tableFile,
                         QString tableName = "TABLE")
      : ImportPdsTable(labelFile, tableFile, tableName) {}

    // Wrapper around ImportPdsTable protected method
    const ColumnDescr &getColumnDescriptorWrap(const int nth) const {
      return getColumnDescriptor(nth);
    }

    // Wrapper around ImportPdsTable protected method
    QStringList getColumnFieldsWrap(const QString &tline,
                                    const ColumnDescr &cdesc,
                                    const QString &delimiter="") {
      return getColumnFields(tline, cdesc, delimiter);
    }

    // Test method to check that ColumnDescr are correctly storing label data
    static void printColumnDescr(const ColumnDescr &cd) {
      cout << "m_name: " << cd.m_name << endl;
      cout << "m_colnum: " << cd.m_colnum << endl;
      cout << "m_dataType: " << cd.m_dataType << endl;
      cout << "m_startByte: " << cd.m_startByte << endl;
      cout << "m_numBytes: " << cd.m_numBytes << endl;
      cout << "m_itemBytes: " << cd.m_itemBytes << endl;
      cout << "m_items: " << cd.m_items << endl;
    }
};


/**
 * Unit test for ImportPdsTable
 *
 * NOTE - Unit Test currently fails when running on a file system that is case insensative. (e.g
 *        running on a local OS X hard drive )
 *
 *      - exception thrown by fill Table "Unable to open file containing PDS table" seems untestable
 *        as the exception is thrown if a std::ifstream is incorrectly created.
 *
 *      - Need to test getColumnFields where the condition where item size is not specified. More
 *        specifically, if (1 >= colDesc.m_itemBytes) is true.
 *        * I changed this from the original (1 < coldescr.m_itemBytes), as this condition doesn't
 *        * suggest that there is not item size specified
 *
 *      - Need to test getColumnFields with delimited column fields in table.
 *
 *      - makeFieldFromBinaryTable has untested conditions (such as INTEGER, SUN_INTEGER, etc.)
 *
 *      - TODO does makeField or makeFieldFromBinaryTable actually account for multi-field columns?
 *        * colDesc.m_items and colDesc.m_itemBytes only used in getColumnDescriptor and
 *        * getColumnFields.
 *
 * @author original unknown
 *
 * @internal
 *   @history 2016-06-24 Ian Humphrey - Updated to test Kris' changes to ImportPdsTable. Added tests
 *                           for getColumnDescriptor.
 *                           Fixes #2397.
 *   @History 2016-08-26 Kelvin Rodriguez - Added note about testing on case insensative file
 *                           systems. Part of porting to OS X 10.11
 */
int main(int argc, char *argv[]) {
  Isis::Preference::Preferences(true);
  Isis::FileName data("data/");

  QString inputFile = data.expanded() + "VIR_IR_1A_1_332974737_1_HK.LBL";
  if (--argc == 1) { inputFile = argv[1]; }

  cout << "\n\nTesting ImportPdsTable class using file " << inputFile << "\n";

  ImportPdsTable myTable(inputFile);

  cout << "\n\nList of Columns found - Total: " << myTable.columns() << "\n";
  QStringList kfiles = myTable.getColumnNames();
  cout << kfiles.join("\n");

  cout << "\n\nNow without Name Conversion: \n";
  kfiles = myTable.getColumnNames(false);
  cout << kfiles.join("\n") << endl;

  // Update/correct column types
  myTable.setType("ShutterStatus", "CHARACTER");
  myTable.setType("ChannelId", "CHARACTER");
  myTable.setType("CompressionMode", "CHARACTER");
  myTable.setType("SpectralRange", "CHARACTER");
  myTable.setType("CurrentMode", "CHARACTER");
  myTable.setType("SubCurrentMode", "CHARACTER");
  myTable.setType("IrExpo", "DOUBLE");
  myTable.setType("IrTemp", "DOUBLE");
  myTable.setType("CcdExpo", "DOUBLE");
  myTable.setType("CcdTemp", "DOUBLE");
  myTable.setType("MirrorSin", "DOUBLE");
  myTable.setType("Mirror", "DOUBLE");
  myTable.setType("SpectTemp", "DOUBLE");
  myTable.setType("TeleTemp", "DOUBLE");
  myTable.setType("COLD TIP TEMP", "DOUBLE");
  myTable.setType("RADIATOR TEMP", "DOUBLE");
  myTable.setType("SU MOTOR CURR", "DOUBLE");
  myTable.setType("LEDGE TEMP", "DOUBLE");
  myTable.setType("FRAME COUNT", "CHARACTER");

  // Create a table from the input
  cout << "Getting ISIS Table...\n";
  Table newTable = myTable.importTable("VIR_DATA");
  for (int i = 0; i < newTable[0].Fields(); i++) {
    cout << newTable[0][i].name() << "\t";
  }
  cout << endl;


  // this test was commented out since the output is not what was expected.
  // See mantis ticket
  //
  //??? Table newTable2 = myTable.importTable("Apid,PacketsLength", "VIR_DATA_2");
  //??? for (int i = 0; i < newTable2[0].Fields(); i++) {
  //???   cout << newTable[0][i].name() << "\t";
  //??? }
  //??? cout << endl;//??? add mantis ticket -- appears to be failing
  //???
  //??? vector<string> cols;
  //??? cols.push_back("Apid");
  //??? cols.push_back("PacketsLength");
  //??? Table newTable3 = myTable.importTable(cols, "VIR_DATA_2");
  //??? for (int i = 0; i < newTable3[0].Fields(); i++) {
  //???   cout << newTable[0][i].name() << "\t";
  //??? }
  //??? cout << endl;

  // The following tests were added when the class was expanded to import binary
  // PDS tables also...

  QString pdsTableDir = data.expanded();
  QString pdsLabelFile = "";
  QString pdsTableFile = "";

  cout << "\n\n\nImport PDS table from PDS table exported as MSB...\n";
  pdsLabelFile = pdsTableDir + "msb_pds_binary_table.lbl";
  // TEST: Read the table file name from the labels
  ImportPdsTable pdsMsbTable(pdsLabelFile, pdsTableFile, "EXPORTED_ISIS_TABLE");
//  Table isisTableFromMsb = pdsMsbTable.importFromBinaryPds();
  Table isisTableFromMsb = pdsMsbTable.importTable("ReimportedMSB");
  // print the table
  cout << isisTableFromMsb.Name() << endl;
  cout << isisTableFromMsb[0][0].name() << "\t";
  cout << isisTableFromMsb[0][1].name() << "\t";
  cout << isisTableFromMsb[0][2].name() << "\t";
  cout << isisTableFromMsb[0][3].name() << "\n";
  for (int i = 0; i < isisTableFromMsb.Records(); i++) {
    cout << toString((double) isisTableFromMsb[i][0]) << "\t\t\t";
    cout << toString((int)    isisTableFromMsb[i][1]) << "\t\t\t\t";
    cout << std::string(          isisTableFromMsb[i][2]) << "\t\t\t";
    cout << toString((float)  isisTableFromMsb[i][3]) << "\n";
  }

  cout << "\n\n\nImport PDS table from PDS table exported as LSB...\n";
  pdsLabelFile = pdsTableDir + "lsb_pds_binary_table.lbl";
  pdsTableFile = pdsTableDir + "lsb_pds_binary_table.dat";
  // TEST: Use default constructor and load() method - default table name is TABLE
  //       The next 2 lines are equivalent to calling
  //       ImportPdsTable pdsLsbTable(pdsLabelFile, pdsTableFile, "TABLE");
  ImportPdsTable pdsLsbTable;
  pdsLsbTable.load(pdsLabelFile, pdsTableFile);
//  Table isisTableFromLsb = pdsLsbTable.importFromBinaryPds();
  Table isisTableFromLsb = pdsLsbTable.importTable("ReimportedLSB");
  // print the table
  cout << isisTableFromLsb.Name() << endl;
  cout << isisTableFromLsb[0][0].name() << "\t";
  cout << isisTableFromLsb[0][1].name() << "\t";
  cout << isisTableFromLsb[0][2].name() << "\t";
  cout << isisTableFromLsb[0][3].name() << "\n";
  for (int i = 0; i < isisTableFromLsb.Records(); i++) {
    cout << toString((double) isisTableFromLsb[i][0]) << "\t\t\t";
    cout << toString((int)    isisTableFromLsb[i][1]) << "\t\t\t\t";
    cout << std::string(          isisTableFromLsb[i][2]) << "\t\t\t";
    cout << toString((float)  isisTableFromLsb[i][3]) << "\n";
  }
  cout << endl;

  // the following currently does not work for binary tables
  // implement this test if the code is fixed...
  //
  //??? isisTableFromLsb = pdsLsbTable.importTable("Double Value,Real Value", "LsbTable");
  //??? cout << isisTableFromLsb[0][0].name() << "\t";
  //??? cout << isisTableFromLsb[0][1].name() << endl;
  //??? cout << IString((double) isisTableFromLsb[0][0]) << "\t\t";
  //??? cout << IString((float) isisTableFromLsb[0][1]) << endl;
  //??? cout << IString((double) isisTableFromLsb[1][0]) << "\t\t";
  //??? cout << IString((float) isisTableFromLsb[1][1]) << endl;
  //??? cout << endl;



  // Testing new changes made to ImportPdsTable class

  // Testing name() and setName()
  cout << "\n\nTesting name() (default TABLE): " << myTable.name() << "\n";
  cout << "\nTesting setName(\"My Table\"): ";
  myTable.setName(QString("My Table"));
  cout << myTable.name() << "\n";


  QString merLabelFile = data.expanded() + "edrindex.lbl";
  QString merTableFile = data.expanded() + "edrindex.tab";
  cout << "\n\nTesting ImportPdsTable protected methods with file " << merLabelFile;

  cout << "\n\nConstructing new ImportPdsTable where the PDS table object name is ";

  ImportPdsTableTester myTestTable(merLabelFile, merTableFile, QString("INDEX_TABLE"));

  cout << myTestTable.name() << "\n";

  // Testing getColumnFields method
  // NOTE - this is NOT using ImportPdsTable's public interface, as we need access to
  //        m_rows in order to get table row data to pass to getColumnFields
  cout << "\nTesting getColumnFields..." << "\n";
  TextFile tf(merTableFile);
  // Grab the first data record (first row) in the table
  QString rowData;
  tf.GetLine(rowData);

  //   Testing if the column's item count is 1 (i.e. no ITEMS or ITEM_BYTES keywords)
  //   Using edrindex.lbl's 43th column
  cout << "\nColumn TELEMETRY_SOURCE_NAME items: ";
  QStringList oneItem = myTestTable.getColumnFieldsWrap(rowData,
                            myTestTable.getColumnDescriptorWrap(42));
  cout << oneItem.size() << "\n";
  foreach (QString item, oneItem)
    cout << "  " << item << "\n";

  cout << "\nColumn Description for this column: " << endl;
  ImportPdsTableTester::printColumnDescr(myTestTable.getColumnDescriptorWrap(42));
  cout << "\n";

  //   Testing if the column has more than one item (i.e. has ITEMS and ITEM_BYTES keywords)
  //   Use edrindex.lbl's 44th column
  cout << "\nColumn ROVER_MOTION_COUNTER items: ";
  QStringList manyItems = myTestTable.getColumnFieldsWrap(rowData,
                              myTestTable.getColumnDescriptorWrap(43));
  cout << manyItems.size() << "\n";
  foreach (QString item, manyItems)
    cout << "  " << item << "\n";

  cout << "\nColumn Description for this column: " << endl;
  ImportPdsTableTester::printColumnDescr(myTestTable.getColumnDescriptorWrap(43));
  cout << "\n";

  //   Testing the delimiter parameter TODO - find table data with delimited column fields
//   QString fieldsDelimiter = ";";
//   QStringList manyDelimited = myTestTable.getColumnFieldsWrap(rowData,
//                                   myTestTable.getColumnDescriptorWrap(43),
//                                   fieldsDelimiter);

  cout << "\n\n";



  //  test error throws...
  // 1) load(pdsLabFile, pdsTabFile) -  Unable to import PDS table.
  //        Neither of the following possible table files were found.
  try {
    cout << "Throw error for invalid table file name: " << endl;
    ImportPdsTable error(pdsLabelFile, "INVALID_TABLE_FILE_NAME.DAT", "TABLE");
  }
  catch (IException &e) {
    e.print();
    cout << endl;
  }

  // 2) getColumnName(index) - Unable to import the binary PDS table into Isis.
  //        The requested column index exceeds the number of columns.
  try {
    cout << "Throw error for attempt to access invalid column index: " << endl;
    pdsLsbTable.getColumnName(5);
  }
  catch (IException &e) {
    e.print();
    cout << endl;
  }

  // 3) importTable(QStringList colNames, Qtring tableName) - Unable to import the PDS table into
  //        Isis. The requested column name does not exist in the table.
  try {
    cout << "Throw error for attempt to export non-existent columns: " << endl;
    QStringList columnNames;
    columnNames.append("Invalid Column Name");
    myTable.importTable(columnNames, "VIR_DATA");
  }
  catch (IException &e) {
    e.print();
    cout << endl;
  }

  // 4) loadLabel(labFile, tabFile) - The PDS file does not have the required TABLE object.
  //        The PDS label file is probably invalid.
  try {
    cout << "Throw error for missing table location in label file:" << endl;
    ImportPdsTable error(pdsLabelFile, pdsTableFile, "MISSING_TABLE");
  }
  catch (IException &e) {
    e.print();
    cout << endl;
  }

  // 5) loadLabel(labFile, tabFile) - Unable to import the PDS table from the PDS file.
  //        The PDS INTERCHANGE_FORMAT is not supported. Valid values are ASCII or BINARY.
  try {
    cout << "Throw error for invalid table format type in label file:" << endl;
    ImportPdsTable error(pdsTableDir + "invalidFormatType.lbl");
  }
  catch (IException &e) {
    e.print();
    cout << endl;
  }

  // 6) loadLabel(labFile, tabFile) - Number of columns in the COLUMNS label keyword does not
  //        match number of COLUMN objects found
  try {
    cout << "Print message when COLUMNS value not matching number of COLUMN objects:" << endl;
    ImportPdsTable error(pdsTableDir + "invalidColumnsValue.lbl");
  }
  catch (IException &e) {
    e.print();
  }
  cout << endl;

  // This will cause the following errors to be thrown
  // 7) extract() - Conversion failure of column
  // 8) fillTable() - Failed to convert data in row
  try {
    cout << "Throw error for invalid Text PDS table to be imported: " << endl;
    ImportPdsTable error(pdsTableDir + "invalidTextTable.lbl", "data/invalidTextTable.tab", "TABLE");
    error.importTable("InvalidTable");
  }
  catch (IException &e) {
    e.print();
    cout << endl;
  }

  // Not sure how to cause this error
  // 9) fillTable() - Unable to open file containing PDS table.
  // try {
  //   cout << "Throw error :" << endl;
  //   ImportPdsTable error(pdsLabelFile, "nonExistantFile.dat");
  //   error.importFromBinaryPds();
  // }
  // catch (IException &e) {
  //   e.print();
  //   cout << endl;
  // }

  // This will cause the following errors to be thrown
  // 10) importTable() - Unable to import the binary PDS table from the PDS file
  //                     into Isis.
  // 11) makeFieldFromBinaryTable()- Only 4 byte integer values are
  //                     supported in Isis. PDS column has an integer DATA_TYPE
  //                     with N bytes. For MSB_INTEGER
  try {
    cout << "Throw error for invalid MSB_INTEGER bytes in label file: " << endl;
    ImportPdsTable error(pdsTableDir + "invalidMSBIntegerBytes.lbl", "", "EXPORTED_ISIS_TABLE");
    error.importTable("ReimportedToIsis");
  }
  catch (IException &e) {
    e.print();
    cout << endl;
  }

  // This will cause the following errors to be thrown
  // 10) importTable() - Unable to import the binary PDS table from the PDS file
  //                     into Isis.
  // 12) makeFieldFromBinaryTable()- Only 4 byte integer values are
  //                     supported in Isis. PDS column has an integer DATA_TYPE
  //                     with N bytes. For LSB_INTEGER
  try {
    cout << "Throw error for invalid INTEGER bytes in label file: " << endl;
    ImportPdsTable error(pdsTableDir + "invalidLSBIntegerBytes.lbl");
    error.importTable("ReimportedToIsis");
  }
  catch (IException &e) {
    e.print();
    cout << endl;
  }

  // This will cause the following errors to be thrown
  // 10) importTable() - Unable to import the binary PDS table from the PDS file
  //                     into Isis.
  // 13) makeFieldFromBinaryTable()- Only 4 or 8 byte real values are
  //                     supported in Isis. PDS column has an integer DATA_TYPE
  //                     with N bytes. For IEEE_REAL
  try {
    cout << "Throw error for invalid REAL bytes in label file: " << endl;
    ImportPdsTable error(pdsTableDir + "invalidMSBRealBytes.lbl", "", "EXPORTED_ISIS_TABLE");
    error.importTable("ReimportedToIsis");
  }
  catch (IException &e) {
    e.print();
    cout << endl;
  }

  // This will cause the following errors to be thrown
  // 10) importTable() - Unable to import the binary PDS table from the PDS file
  //                     into Isis.
  // 14) makeFieldFromBinaryTable()- Only 4 or 8 byte real values are
  //                     supported in Isis. PDS column has an integer DATA_TYPE
  //                     with N bytes. For PC_REAL
  try {
    cout << "Throw error for invalid REAL bytes in label file: " << endl;
    ImportPdsTable error(pdsTableDir + "invalidLSBRealBytes.lbl");
    error.importTable("ReimportedToIsis");
  }
  catch (IException &e) {
    e.print();
    cout << endl;
  }

  // This will cause the following errors to be thrown
  // 10) importTable() - Unable to import the binary PDS table from the PDS file
  //                     into Isis.
  // 15) makeFieldFromBinaryTable()- PDS Column has an unsupported DATA_TYPE.
  try {
    cout << "Throw error for invalid column DATA_TYPE in label file: " << endl;
    ImportPdsTable error(pdsTableDir + "invalidDataType.lbl");
    error.importTable("ReimportedToIsis");
  }
  catch (IException &e) {
    e.print();
    cout << endl;
  }

  // This will cause the following errors to be thrown
  // 10) importTable() - Unable to import the binary PDS table from the PDS file
  //                     into Isis.
  // 16) setByteOrder()- The column DATA_TYPE values indicate differing byte
  //                     orders.
  try {
    cout << "Throw error for inconsistent byte order in label file: " << endl;
    ImportPdsTable error(pdsTableDir + "invalidByteOrder.lbl");
    error.importTable("ReimportedToIsis");
  }
  catch (IException &e) {
    e.print();
    cout << endl;
  }

  // 17) getColumnDescriptor(nth) - The nth index used to request the nth column is invalid.
  // There are two if conditions to test:
  //     This tests the nth >= columns() condition
  try {
    cout << "Throw error if index used to request a column description " << endl;
    cout << "is greater than the number of columns in the table: " << endl;
    myTestTable.getColumnDescriptorWrap(100);
  }
  catch (IException &e) {
    e.print();
    cout << endl;
  }

  //     This tests the nth < 0 condition
  try {
    cout << "Throw error if index used to request a column description is less than 0: " << endl;
    myTestTable.getColumnDescriptorWrap(-1);
  }
  catch (IException &e) {
    e.print();
    cout << endl;
  }


  //  Untested methods:
  cout << "cols = " << pdsLsbTable.columns()                 << endl;
  cout << "rows = " << pdsLsbTable.rows()                    << endl;
  cout << "has double = " << pdsLsbTable.hasColumn("Double Value") << endl;
  cout << "col 1 name = " << pdsLsbTable.getColumnName(1)          << endl;
  QStringList names = pdsLsbTable.getColumnNames();
  for (int i = 0; i < names.size(); i++) {
    cout << names[i] << endl;
  }
  cout << "type Double Value column = " << pdsLsbTable.getType("Double Value") << endl;
  pdsLsbTable.setType("Double Value", "MSB_INTEGER");
  cout << "set Double Value column to type MSB_INTEGER " << endl;
  cout << "type Double Value column = " << pdsLsbTable.getType("Double Value") << endl;

  return (0);
}
