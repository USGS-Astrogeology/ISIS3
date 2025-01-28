/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "ExportPdsTable.h"

#include <cmath>
#include <iostream>

#include "Endian.h"
#include "EndianSwapper.h"
#include "IString.h"
#include "IException.h"
#include "Pvl.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "Table.h"
#include "TableField.h"
#include "TableRecord.h"


using namespace std;

namespace Isis {
  /**
   * Construct an ExportPdsTable object and set default member variable values.
   * This constructor sets the following defaults:
   * <ul>
   *   <li>BYTEORDER = "LSB"</li>
   *   <li>ROWS = Records</li>
   *   <li>ROW_BYTES = RecordSize</li>
   * </ul>
   *
   * @param isisTable The ISIS Table object to be exported.
   */
  ExportPdsTable::ExportPdsTable(Table isisTable) {
    // initialize member variables
    m_outputRecordBytes = 0;
    m_pdsByteOrder = "LSB";
    m_isisTable = new Table(isisTable);
    m_numRows = isisTable.Records(); //one row in pdsTable per record in isisTable
    m_rowBytes = m_isisTable->RecordSize(); //this should be the same value
                                            //for all pds rows and isis records
  }

  /**
   * Destructs for ExportPdsTable objects.
   */
  ExportPdsTable::~ExportPdsTable() {
    // delete pointers and set to null
    delete m_isisTable;
    m_isisTable = NULL;
  }

  /**
   * This methods fills the given buffer with the binary PDS table data and
   * returns label information.
   *
   * @param pdsTableBuffer This buffer will be filled with binary PDS table data.
   * @param pdsFileRecordBytes The number or RECORD_BYTES in the PDS file.
   * @param pdsByteOrder A string containing the byte order of the PDS file.
   *                     Valid values are "LSB" or "MSB".
   * @return PvlObject A Pvl containing the PDS table's label information.
   */
  PvlObject ExportPdsTable::exportTable(char *pdsTableBuffer,
                                   int outputFileRecordBytes,
                                   QString pdsTableByteOrder) {
    // Currently, we will not allow our table rows to be wrapped. So we must
    // check that the rowbytes of the output table are no larger than the total
    // record bytes allowed in the output pds file
    m_outputRecordBytes = outputFileRecordBytes;
    if (m_rowBytes > m_outputRecordBytes) {
      throw IException(IException::Unknown,
                       "Unable to export Isis::Table object to PDS. The "
                       "Isis::Table record size [" + toString(m_rowBytes)
                       + "] is larger than the record bytes allowed in the "
                       "PDS file [" + toString(m_outputRecordBytes) + "].",
                        _FILEINFO_);
    }

    // update variables
    m_pdsByteOrder = pdsTableByteOrder.toUpper();

    if (m_pdsByteOrder != "MSB" && m_pdsByteOrder != "LSB") {
      QString msg = "Unable to export the Isis Table [" + m_isisTable->Name()
                    + "] to a PDS table using the requested byte order ["
                    + m_pdsByteOrder + "]. Valid values are MSB or LSB.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    // create an array of nulls to pad
    // from the end of each row to the end of the record
    int numRowNulls = outputFileRecordBytes - m_rowBytes;
    char endRowPadding[numRowNulls];
    for (int i = 0; i < numRowNulls; i++) {
      endRowPadding[i] = '\0';
    }

    // loop through records in the input Isis::Table object
    // fill rowBuffer with packed record values, then fill with padding
    EndianSwapper *endianSwap = new EndianSwapper(m_pdsByteOrder);

    for(int recIndex = 0; recIndex < m_isisTable->Records(); recIndex++) {
      TableRecord record = (*m_isisTable)[recIndex];
      char rowBuffer[record.RecordSize()];
      Pack(record, rowBuffer, endianSwap);
      int i = recIndex*m_outputRecordBytes;
      memmove(pdsTableBuffer + i, &rowBuffer, record.RecordSize());
      memmove(pdsTableBuffer + i + m_rowBytes, &endRowPadding, numRowNulls);
    }
    return fillMetaData();
  }

  /**
   * Creates a PvlObject to be added to the PDS label with needed TABLE
   * information.
   *
   * @return PvlObject containing PDS Table metadata.
   */
  PvlObject ExportPdsTable::fillMetaData() {
    QString pdsTableName = formatPdsTableName();
    PvlObject pdsTableLabelInfo(pdsTableName);
    // Data Object Descriptions
    // NOTE: this class is currently only exporting BINARY format PDS tables.
    //       implementation may be added later to export ASCII PDS tables.
    pdsTableLabelInfo.addKeyword(PvlKeyword("INTERCHANGE_FORMAT", "BINARY"));
    pdsTableLabelInfo.addKeyword(PvlKeyword("ROWS", toString(m_isisTable->Records())));
    pdsTableLabelInfo.addKeyword(PvlKeyword("COLUMNS", toString(m_isisTable->RecordFields())));
    pdsTableLabelInfo.addKeyword(PvlKeyword("ROW_BYTES", toString(m_rowBytes)));
    pdsTableLabelInfo.addKeyword(PvlKeyword("ROW_SUFFIX_BYTES", toString(m_outputRecordBytes - m_rowBytes)));
    int startByte = 1;  // PDS begins indexing at 1
    for(int fieldIndex = 0; fieldIndex < m_isisTable->RecordFields(); fieldIndex++) {
      int columnBytes = 0;
      TableField field = (*m_isisTable)[0][fieldIndex];
      PvlObject columnObj("COLUMN");
      columnObj.addKeyword(PvlKeyword("COLUMN_NUMBER", toString(fieldIndex + 1)));
      columnObj.addKeyword(PvlKeyword("NAME", field.name()));


      if (field.type() == TableField::Text) {
        columnObj.addKeyword(PvlKeyword("DATA_TYPE", "CHARACTER"));
        QString val = field;
        for(int i = 0; i < field.size(); i++) {
          columnBytes++;
        }
      }
      else if (field.type() == TableField::Integer) {
        if (m_pdsByteOrder == "MSB") {
          columnObj.addKeyword(PvlKeyword("DATA_TYPE", "MSB_INTEGER"));
          columnBytes = sizeof(int);
        }
        else { // if (m_pdsByteOrder == "LSB") {
               // no need to check this. already validated in exportPdsTable()
          columnObj.addKeyword(PvlKeyword("DATA_TYPE", "LSB_INTEGER"));
          columnBytes = sizeof(int);
        }
      }
      else if (field.type() == TableField::Double) {
        if (m_pdsByteOrder == "MSB") {
          columnObj.addKeyword(PvlKeyword("DATA_TYPE", "IEEE_REAL"));
          columnBytes = sizeof(double);
        }
        else { // if (m_pdsByteOrder == "LSB") {
               // no need to check this. already validated in exportPdsTable()
          columnObj.addKeyword(PvlKeyword("DATA_TYPE", "PC_REAL"));
          columnBytes = sizeof(double);
        }
      }
      else if (field.type() == TableField::Real) {
        if (m_pdsByteOrder == "MSB") {
          columnObj.addKeyword(PvlKeyword("DATA_TYPE", "IEEE_REAL"));
          columnBytes = sizeof(float);
        }
        else { // if (m_pdsByteOrder == "LSB") {
               // no need to check this. already validated in exportPdsTable()
          columnObj.addKeyword(PvlKeyword("DATA_TYPE", "PC_REAL"));
          columnBytes = sizeof(float);
        }
      }
      else { // This error is not covered in the unitTest since currently, there
             // are no other valid values for TableField types. It is meant to
             // catch other values if they are added to Table Field.
        QString msg = "Unable to export Isis::Table object to PDS. Invalid "
                      "field type found for [" + field.name() + "].";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
      columnObj.addKeyword(PvlKeyword("START_BYTE", toString(startByte)));
      startByte += columnBytes;
      columnObj.addKeyword(PvlKeyword("BYTES", toString(columnBytes)));
      pdsTableLabelInfo.addObject(columnObj);
    }
    return pdsTableLabelInfo;
  }

  /**
   * Format the PDS table object name using the ISIS table name.
   *
   * @return QString containing the formatted PDS table name.
   */
  QString ExportPdsTable::formatPdsTableName() {
    return ExportPdsTable::formatPdsTableName(m_isisTable->Name());
  }

  /**
   * Static method that formats the given ISIS table name in PDS format.
   * This method takes the upper camel case Isis table name and returns
   * a PDS table name that is underscore separated, all upper case and
   * with "_TABLE" appended to the end of the name, if not already present.
   *
   * @param isisTableName An QString containing the ISIS upper camel case
   *        table name.
   * @return QString containing the formatted PDS table name.
   */
  QString ExportPdsTable::formatPdsTableName(QString isisTableName) {
    QString tableName = isisTableName.simplified();
    QString pdsTableName;
    pdsTableName.push_back(tableName[0]);
    for (int i = 1 ; i < tableName.size() ; i++) {
      if (tableName[i] >= 65 && tableName[i] <= 90) {
        pdsTableName.push_back('_');
        pdsTableName.push_back(tableName[i]);
      }
      else {
        pdsTableName.push_back(tableName[i]);
      }
    }
    pdsTableName = pdsTableName.toUpper();
    if (pdsTableName.indexOf("_TABLE") != pdsTableName.length() - 6) {
      pdsTableName += "_TABLE";
    }
    return pdsTableName;
  }

  /**
   * Pack the buffer with data from the table record, swapping bytes if needed.
   *
   * @param record ISIS TableRecord to be exported 
   * @param buffer Output buffer to be filled with PDS table row data in the
   *               appropriate byte order.
   * @param endianSwap Pointer to EndianSwapper object to swap bytes if the
   *                   input Isis Table byte order is not the same as the output
   *                   PDS byte order.
   */
  void ExportPdsTable::Pack(TableRecord record, char *buffer,
                            EndianSwapper *endianSwap) {
    // for each field, keep track of the start byte
    int startByte = 0;
    for(int fieldIndex = 0; fieldIndex < record.Fields(); fieldIndex++) {
      // check the data type of the field,
      // swap the appropriate number of bytes
      // fill the buffer at the appropriate address
      // find the start byte of the next field
      TableField &field = record[fieldIndex];
      if(field.isDouble()) {
        vector<double> fieldValues = field;
        for(unsigned int i = 0; i < fieldValues.size(); i++) {
          double value = endianSwap->Double(&fieldValues[i]);
          memmove(buffer + startByte, &value, 8);
          startByte += sizeof(double);
        }
      }
      else if(field.isInteger()) {
        vector<int> fieldValues = field;
        for(unsigned int i = 0; i < fieldValues.size(); i++) {
          int value = endianSwap->Int(&fieldValues[i]);
          memmove(buffer + startByte, &value, 4);
          startByte += sizeof(int);
        }
      }
      else if(field.isText()) {
        QString val = field;
        // copy each character and count each byte individually
        for(int i = 0; i < field.size(); i++) {
          if(i < (int)val.length()) {
            buffer[startByte] = val[i].toLatin1();
          }
          else {
            // this line is not covered by unitTest since this is a case that
            // should not happen.  When a Text TableField is created, the
            // string value length is resized to fit the field size.
            buffer[startByte] = 0;
          }
          startByte++;
        }
      }
      else if(field.isReal()) {
        vector<float> fieldValues = field;
        for(unsigned int i = 0; i < fieldValues.size(); i++) {
          float value = endianSwap->Float(&fieldValues[i]);
          memmove(buffer + startByte, &value, 4);
          startByte += sizeof(float);
        }
      }
      else { // This error is not covered in the unitTest since currently, there
             // are no other valid values for TableField types. It is meant to
             // catch other values if they are added to Table Field.
        QString msg = "Unable to export Isis::Table object to PDS. Invalid "
                      "field type found for [" + field.name() + "].";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }
    // after loopting through the fields, the value of the startByte variable
    // should match the total number of row bytes for the table
    if (startByte != m_rowBytes) { // thie error is not covered in the unitTest
                                   // tested since it should not be possible
                                   // unless the number of bytes reserved for
                                   // each of the types changes
      QString msg = "Unable to export Isis::Table object [" + m_isisTable->Name()
                    + "] to PDS. Record lengths are uneven.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
  }
}  //  namespace Isis
