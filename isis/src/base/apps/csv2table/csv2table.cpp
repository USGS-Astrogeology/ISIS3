/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "csv2table.h"

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

using namespace std;

namespace Isis {
    /**
     * csminit a cube in an Application
     *
     * @param ui The Application UI
     * @param(out) log The Pvl that attempted models will be logged to
    */
    void csv2table(UserInterface &ui, Pvl *log) {
        // Read the CSV file and get the header
        QString csvFileName = ui.GetFileName("csv");
        CSVReader reader;
        try {
            reader = CSVReader(csvFileName, true);
        }
        catch(IException &e) {
            QString msg = "Failed to read CSV file [" + csvFileName + "].";
            throw IException(e, IException::Io, msg.toStdString(), _FILEINFO_);
        }
        int numColumns = reader.columns();
        int numRows = reader.rows();
        if (numColumns < 1 || numRows < 1) {
            std::string msg = "CSV file does not have data.\nFile has [" + toString(numRows) +
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
                throw IException(IException::User, msg.toStdString(), _FILEINFO_);
                }
            }
            }
            else {
            int numFields = fieldTypes.size();
            std::string msg = "Number of fields provided does not equal the number of columns in the CSV. "
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
                    TableField columnField(name.toStdString(), tableTypes[columnIndex], (index.length()>0)?(index.toInt()+1):1);
                    tableRow += columnField;
                }
            }
        }

        QString tableName = ui.GetString("tablename");
        Table table(tableName.toStdString(), tableRow);

        // Fill the table from the csv
        for (int rowIndex = 0; rowIndex < numRows; rowIndex++) {
            CSVReader::CSVAxis csvRow = reader.getRow(rowIndex);
            for (int columnIndex = 0, fieldIndex = 0; columnIndex < numColumns; ) {
                if (tableRow[fieldIndex].size() == 1 ||
                    tableRow[fieldIndex].isText()) {
                    switch(tableTypes[columnIndex]) {
                    case TableField::Type::Integer:
                        tableRow[fieldIndex] = csvRow[columnIndex++].toInt();
                        break;
                    case TableField::Type::Double:
                        tableRow[fieldIndex] = csvRow[columnIndex++].toDouble();
                        break;
                    case TableField::Type::Text:
                        tableRow[fieldIndex] = csvRow[columnIndex++].toStdString();
                        break;
                    case TableField::Type::Real:
                        tableRow[fieldIndex] = (float)csvRow[columnIndex++].toDouble();
                        break;
                    }
                }
                else {
                    std::vector<int> intVector;
                    std::vector<double> dblVector;
                    std::vector<float> realVector;
                    QString strMsg = "TableRecord can't handle list of Strings";
                    switch(tableTypes[columnIndex]) {
                    case TableField::Type::Integer:
                        for (int arrayLen = 0; arrayLen < tableRow[fieldIndex].size(); arrayLen++) {
                        intVector.push_back((csvRow[columnIndex++]).toInt());
                        }
                        tableRow[fieldIndex] = intVector;
                        break;
                    case TableField::Type::Double:
                        for (int arrayLen = 0; arrayLen < tableRow[fieldIndex].size(); arrayLen++) {
                        dblVector.push_back((csvRow[columnIndex++]).toDouble());
                        }
                        tableRow[fieldIndex] = dblVector;
                        break;
                    case TableField::Type::Text:
                        throw IException(IException::User, strMsg.toStdString(), _FILEINFO_);
                        break;
                    case TableField::Type::Real:
                        for (int arrayLen = 0; arrayLen < tableRow[fieldIndex].size(); arrayLen++) {
                        realVector.push_back((float)csvRow[columnIndex++].toDouble());
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
            labelPvl.read(labelPvlFilename.toStdString());
            }
            catch(IException &e) {
            QString msg = "Failed to read PVL label file [" + labelPvlFilename + "].";
            throw IException(e, IException::Io, msg.toStdString(), _FILEINFO_);
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
            throw IException(e, IException::Io, msg.toStdString(), _FILEINFO_);
        }

        try {
            outCube.write(table);
        }
        catch(IException &e) {
            QString msg = "Could not write output table [" + tableName +
                        "] to output cube [" + outCubeFileName + "].";
            throw IException(e, IException::Io, msg.toStdString(), _FILEINFO_);
        }

        outCube.close();
    }
}