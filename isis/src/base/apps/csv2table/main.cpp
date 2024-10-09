/**
 * @file
 * $Revision: 1.8 $
 * $Date: 2010/04/08 15:28:20 $
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are public
 *   domain. See individual third-party library and package descriptions for
 *   intellectual property information,user agreements, and related information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or implied,
 *   is made by the USGS as to the accuracy and functioning of such software
 *   and related material nor shall the fact of distribution constitute any such
 *   warranty, and no responsibility is assumed by the USGS in connection
 *   therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html in a browser or see
 *   the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */

#include "Isis.h"

#include <vector>

#include <QRegularExpression>
#include <QString>

#include "Cube.h"
#include "CSVReader.h"
#include "IException.h"
#include "IString.h"
#include "Pvl.h"
#include "PvlObject.h"
#include "Table.h"
#include "TableField.h"
#include "TableRecord.h"

using namespace Isis;


void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  // Read the CSV file and get the header
  QString csvFileName = ui.GetFileName("csv");
  CSVReader reader;
  try {
    reader = CSVReader(csvFileName, true);
  }
  catch(IException &e) {
    QString msg = "Failed to read CSV file [" + csvFileName + "].";
    throw IException(e, IException::Io, msg, _FILEINFO_);
  }
  int numColumns = reader.columns();
  int numRows = reader.rows();
  if (numColumns < 1 || numRows < 1) {
    QString msg = "CSV file does not have data.\nFile has [" + toString(numRows) +
                  "] rows and [" + toString(numColumns) +"] columns.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  std::vector<QString> fieldTypes;
  ui.GetAsString("coltypes", fieldTypes);

  std::vector<TableField::Type> tableTypes;
  if (fieldTypes.size() == 1 && fieldTypes[0] == "") {
      for (int i = 0; i < numColumns; i++) {
        tableTypes.push_back(TableField::Double);
      }
  }
  else {
    if (fieldTypes.size() == numColumns) {
      for (QString type: fieldTypes) {
        QString upper_type = type.toUpper();
        if (upper_type == "INTEGER") {
          tableTypes.push_back(TableField::Type::Integer);
        }
        else if (upper_type == "DOUBLE") {
          tableTypes.push_back(TableField::Type::Double);
        }
        else if (upper_type == "TEXT") {
          tableTypes.push_back(TableField::Type::Text);
        }
        else if (upper_type == "REAL") {
          tableTypes.push_back(TableField::Type::Real);
        }
        else {
          QString msg = "Field [" + type + "] cannot be translated. Accepted types are "
                        "Integer, Double, Text, and Real";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }
    }
    else {
      int numFields = fieldTypes.size();
      QString msg = "Number of fields provided does not equal the number of columns in the CSV. "
                    "Number of fields [" + toString(numFields) + 
                    "] vs Number of Columns [" + toString(numColumns) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }


  CSVReader::CSVAxis header = reader.getHeader();

  // Construct an empty table with the CSV header as field names
  // Collect identical field names together, including those with (###) at the end, so a single
  // table field with multiple values can be created.
  TableRecord tableRow;
  QRegularExpression rex(R"((?<name>\w+)(\((?<index>[0-9]*)\)|))");
  for (int columnIndex = 0; columnIndex < numColumns; columnIndex++) {
    QRegularExpressionMatch match = rex.match(header[columnIndex]);
    if (match.hasMatch()) {
      QString name = match.captured("name");
      QString index = match.captured("index");

      // If the next column header is different, create a field for this one
      QRegularExpressionMatch nextMatch = (columnIndex<numColumns-1)?rex.match(header[columnIndex+1]):QRegularExpressionMatch();
      if ((columnIndex == numColumns-1) || (nextMatch.hasMatch() && (name != nextMatch.captured("name")))) {
        TableField columnField(name, tableTypes[columnIndex], (index.length()>0)?(index.toInt()+1):1);
        tableRow += columnField;
      }
    }
  }

  QString tableName = ui.GetString("tablename");
  Table table(tableName, tableRow);

  // Fill the table from the csv
  for (int rowIndex = 0; rowIndex < numRows; rowIndex++) {
    CSVReader::CSVAxis csvRow = reader.getRow(rowIndex);
    for (int columnIndex = 0, fieldIndex = 0; columnIndex < numColumns; ) {
      if (tableRow[fieldIndex].size() == 1) {
        switch(tableTypes[columnIndex]) {
          case TableField::Type::Integer:
            tableRow[fieldIndex] = toInt(csvRow[columnIndex++]);
            break;
          case TableField::Type::Double:
            tableRow[fieldIndex] = toDouble(csvRow[columnIndex++]);
            break;
          case TableField::Type::Text:
            tableRow[fieldIndex] = csvRow[columnIndex++];
            break;
          case TableField::Type::Real:
            tableRow[fieldIndex] = toDouble(csvRow[columnIndex++]);
            break;
        }
      }
      else {
        std::vector<int> intVector;
        std::vector<double> dblVector;
        std::vector<double> realVector;
        QString strMsg = "TableRecord can't handle list of Strings";
        switch(tableTypes[columnIndex]) {
          case TableField::Type::Integer:
            for (int arrayLen = 0; arrayLen < tableRow[fieldIndex].size(); arrayLen++) {
              intVector.push_back(toInt(csvRow[columnIndex++]));
            }
            tableRow[fieldIndex] = intVector;
            break;
          case TableField::Type::Double:
            for (int arrayLen = 0; arrayLen < tableRow[fieldIndex].size(); arrayLen++) {
              dblVector.push_back(toDouble(csvRow[columnIndex++]));
            }
            tableRow[fieldIndex] = dblVector;
            break;
          case TableField::Type::Text:
            throw IException(IException::User, strMsg, _FILEINFO_);
            break;
          case TableField::Type::Real:
            for (int arrayLen = 0; arrayLen < tableRow[fieldIndex].size(); arrayLen++) {
              realVector.push_back(toDouble(csvRow[columnIndex++]));
            }
            tableRow[fieldIndex] = realVector;
            break;
        }
        intVector.clear();
        dblVector.clear();
        realVector.clear();
      }
      fieldIndex++;
    }
    table += tableRow;
  }

  // If a set of additional label keywords was given then add them to the table's pvl description
  if (ui.WasEntered("label")) {
    QString labelPvlFilename = ui.GetFileName("label");
    Pvl labelPvl;
    try {
      labelPvl.read(labelPvlFilename);
    }
    catch(IException &e) {
      QString msg = "Failed to read PVL label file [" + labelPvlFilename + "].";
      throw IException(e, IException::Io, msg, _FILEINFO_);
    }

    PvlObject &tableLabel = table.Label();
    for (int keyIndex = 0; keyIndex < labelPvl.keywords(); keyIndex++) {
      tableLabel.addKeyword(labelPvl[keyIndex]);
    }
  }

  // Write the table to the cube
  QString outCubeFileName(ui.GetCubeName("to"));
  Cube outCube;
  try {
    outCube.open(outCubeFileName, "rw");
  }
  catch(IException &e) {
    QString msg = "Could not open output cube [" + outCubeFileName + "].";
    throw IException(e, IException::Io, msg, _FILEINFO_);
  }

  try {
    outCube.write(table);
  }
  catch(IException &e) {
    QString msg = "Could not write output table [" + tableName +
                  "] to output cube [" + outCubeFileName + "].";
    throw IException(e, IException::Io, msg, _FILEINFO_);
  }

  outCube.close();
}
