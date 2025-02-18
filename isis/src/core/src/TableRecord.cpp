/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "TableRecord.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "IException.h"
#include "IString.h"
#include "TableField.h"

using namespace std;
namespace Isis {
  //! Constructs an empty TableRecord object. No member variables are set.
  TableRecord::TableRecord(){
  }

  /**
  * TableRecord constructor
  * 
  * @param tableRecordStr Table record string
  * @param fieldDelimiter The delimiter to separate fields with
  * @param fieldNames Table header names
  * @param numOfFieldValues Number of fields (rows)
  */
  TableRecord::TableRecord(std::string tableRecordStr, char fieldDelimiter, 
                            std::vector<QString> fieldNames, int numOfFieldValues) {
    std::stringstream tableRecordStream;
    tableRecordStream << tableRecordStr;

    std::string fieldStr;
    int i = 0;
    while(std::getline(tableRecordStream, fieldStr, fieldDelimiter)) {
      TableField tableField(fieldNames[i], TableField::Double);
      tableField = std::stod(fieldStr); // convert string to double
      this->operator+=(tableField);
      i++;
    }
  }

  //! Destroys the TableRecord object
  TableRecord::~TableRecord() {
  }


  /**
   * Adds a TableField to a TableRecord
   *
   * @param field - TableField to be added to the record
   */
  void TableRecord::operator+=(Isis::TableField &field) {
    p_fields.push_back(field);
  }

  /**
   *  Returns the TableField at the specified location in the TableRecord
   *
   * @param field  Index of desired field
   *
   * @return The TableField at specified location in the record
   */
  Isis::TableField &TableRecord::operator[](const int field) {
    return p_fields[field];
  }

  /**
   * Returns the TableField in the record whose name corresponds to the
   * input string
   *
   * @param field The name of desired TableField
   *
   * @return The specified TableField
   *
   * @throws Isis::IException::Programmer - The field does not exist in the
   *                                        record
   */
  TableField &TableRecord::operator[](const QString &field) {
    Isis::IString upTemp = field;
    upTemp.UpCase();
    for (int i = 0; i < (int)p_fields.size(); i++) {
      Isis::IString upField = p_fields[i].name();
      upField.UpCase();
      if (upTemp == upField) return p_fields[i];
    }

    QString msg = "Field [" + field + "] does not exist in record";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }

  /**
    * Returns the number of fields that are currently in the record
    *
    * @return The number of fields in the record
    */
   int TableRecord::Fields() const {
     return p_fields.size();
   }

  /**
   * Returns the number of bytes per record
   *
   * @return Number of bytes per record
   */
  int TableRecord::RecordSize() const {
    int bytes = 0;
    for (int i = 0; i < (int)p_fields.size(); i++) bytes += p_fields[i].bytes();
    return bytes;
  }

  /**
   * Writes record information into the binary buffer.
   *
   * @param buf Buffer to fill with binary record information.
   *
   * @throws Isis::IException::Programmer - Invalid field type
   */
  void TableRecord::Pack(char *buf) const {
    int sbyte = 0;
    for (int f = 0; f < Fields(); f++) {
      const Isis::TableField &field = p_fields[f];
      if (field.isDouble()) {
        vector<double> vals = field;
        for (unsigned int i = 0; i < vals.size(); i++) {
          //*((double *)(buf+sbyte)) = vals[i];
          memmove(buf + sbyte, &vals[i], 8);
          sbyte += sizeof(double);
        }
      }
      else if (field.isInteger()) {
        vector<int> vals = field;
        for (unsigned int i = 0; i < vals.size(); i++) {
          //*((int *)(buf+sbyte)) = vals[i];
          memmove(buf + sbyte, &vals[i], 4);
          sbyte += sizeof(int);
        }
      }
      else if (field.isText()) {
        QString val = (QString)field;
        for (int i = 0; i < field.size(); i++) {
          if (i < (int)val.length()) {
            buf[sbyte] = val[i].toLatin1();
          }
          else {
            buf[sbyte] = 0;
          }
          sbyte++;
        }
      }
      else if (field.isReal()) {
        vector<float> vals = field;
        for (unsigned int i = 0; i < vals.size(); i++) {
          //*((int *)(buf+sbyte)) = vals[i];
          memmove(buf + sbyte, &vals[i], 4);
          sbyte += sizeof(float);
        }
      }
      else {
        string msg = "Invalid field type";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }
  }

  /**
   * Reads record information from the binary buffer.
   *
   * @param buf Buffer from which to read record field values.
   */
  void TableRecord::Unpack(const char *buf) {
    int sbyte = 0;
    for (int f = 0; f < Fields(); f++) {
      Isis::TableField &field = p_fields[f];
      field = (void *)&buf[sbyte];
      sbyte += field.bytes();
    } 
  }

  /**
   * Swaps bytes of the buffer, depending on the TableField::Type
   *
   * @param buf Buffer containing record values to be swapped.
   *
   * @throws Isis::iException::Programmer - Invalid field type
   */
  void TableRecord::Swap(char *buf) const {
    int sbyte = 0;
    for (int f = 0; f < Fields(); f++) {
      const Isis::TableField &field = p_fields[f];
      if (field.isDouble()) {
        for (int i = 0; i < field.size(); i++) {
          char *swap = &buf[sbyte];
          char temp;
          temp = swap[0];
          swap[0] = swap[7];
          swap[7] = temp;
          temp = swap[1];
          swap[1] = swap[6];
          swap[6] = temp;
          temp = swap[2];
          swap[2] = swap[5];
          swap[5] = temp;
          temp = swap[3];
          swap[3] = swap[4];
          swap[4] = temp;

          sbyte += sizeof(double);
        }
      }
      else if (field.isInteger()) {
        for (int i = 0; i < field.size(); i++) {
          char *swap = &buf[sbyte];
          char temp;
          temp = swap[0];
          swap[0] = swap[3];
          swap[3] = temp;
          temp = swap[1];
          swap[1] = swap[2];
          swap[2] = temp;
          sbyte += sizeof(int);
        }
      }
      else if (field.isText()) {
        sbyte += field.bytes();
      }
      else if (field.isReal()) {
        for (int i = 0; i < field.size(); i++) {
          char *swap = &buf[sbyte];
          char temp;
          temp = swap[0];
          swap[0] = swap[3];
          swap[3] = temp;
          temp = swap[1];
          swap[1] = swap[2];
          swap[2] = temp;
          sbyte += sizeof(float);
        }
      }
      else {
        string msg = "Unable to swap bytes. Invalid field type";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }
  }



  QString TableRecord::toString(TableRecord record, QString fieldDelimiter, bool fieldNames, bool endLine) {
    QString recordValues;
    if (fieldNames) {
      for (int fieldIndex = 0;fieldIndex < record.Fields();fieldIndex++) {
        // write out the name of each field
        if (record[fieldIndex].size() == 1) {
          recordValues += record[fieldIndex].name();
        }
        else {
          for (int fieldValueIndex = 0;fieldValueIndex < record[fieldIndex].size();fieldValueIndex++) {
            recordValues += record[fieldIndex].name();
            if (record[fieldIndex].isText()) {
              // if it's a text field, exit the loop by adding the appropriate number of bytes
              fieldValueIndex += record[fieldIndex].bytes();
            }
            else {
              // if the field is multivalued, write the index of the field
              recordValues += "(" + Isis::toString(fieldValueIndex) + ")";
            }
            if (fieldValueIndex != record[fieldIndex].size() - 1) {
              // add a delimiter to all but the last value in this field
              recordValues += fieldDelimiter;
            }
          }
        }
        // add a delimiter to all but the last field in this record
        if (fieldIndex != record.Fields() - 1) {
          recordValues += fieldDelimiter;
        }
      }
      // end field names line
      recordValues += "\n";
    }

    for (int fieldIndex = 0;fieldIndex < record.Fields();fieldIndex++) {
      // add value for each field in the record
      recordValues += TableField::toString(record[fieldIndex], fieldDelimiter);
      if (fieldIndex != record.Fields() - 1) {
        // add delimiter to all but the last field in the record
        recordValues += fieldDelimiter;
      }
    }
    if (endLine) {
      recordValues += "\n";
    }
    return recordValues;
  }

} // end namespace isis
