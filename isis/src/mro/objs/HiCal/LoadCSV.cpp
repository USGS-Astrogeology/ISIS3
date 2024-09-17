/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QString>
#include <vector>
#include <numeric>
#include <iostream>
#include <sstream>

#include "LoadCSV.h"
#include "FileName.h"
#include "IException.h"

using namespace std;

namespace Isis {


  LoadCSV::LoadCSV() : _base(), _csvSpecs("LoadCSV"), _data(0,0), _history() { }

  LoadCSV::LoadCSV(const QString &base, const HiCalConf &conf,
                   const DbProfile &profile) : _base(), _csvSpecs("LoadCSV"),
                   _data(0,0), _history() {
    load(base, conf, profile);
  }

  void LoadCSV::load(const QString &base, const HiCalConf &conf,
                     const DbProfile &profile) {

    //  Initialize the object with necessary info
    init(base, conf, profile);

    // Set up access to the CSV table.  Note that HiCalConf.getMatrixSource()
    // method is typically used, but the parsing has been broken up here for
    // implementation purposes.
    QString csvfile(conf.filepath(getValue()));
    addHistory("File", csvfile);
    CSVReader csv;

    //  Retrieve information regarding the format within the CSV
    bool colHeader(IsEqual(ConfKey(_csvSpecs,makeKey("Header"), QString("FALSE")), "TRUE"));
    bool rowHeader(colHeader);  // Both default to state of the {BASE}Header
    if (IsEqual(getValue("ColumnHeader"), "TRUE"))  colHeader = true;
    if (IsEqual(getValue("RowHeader"), "TRUE"))  rowHeader = true;
    if (_csvSpecs.exists(makeKey("ColumnName"))) colHeader = true;
    if (_csvSpecs.exists(makeKey("RowName")   )) rowHeader = true;

    // Skip lines, comment headers and separator
    int skip = ConfKey(_csvSpecs, makeKey("SkipLines"), QString("0")).toInt();
    addHistory("SkipLines", ToString(skip));
    bool comments = IsEqual(ConfKey(_csvSpecs, makeKey("IgnoreComments"), QString("TRUE")));
    QString separator = ConfKey(_csvSpecs, makeKey("Separator"), QString(","));
    if (separator.isEmpty()) separator = ",";   // Guarantees content

    // Apply conditions
    csv.setComment(comments);
    csv.setSkip(skip);
    csv.setHeader(colHeader);
    csv.setDelimiter(separator[0].toLatin1());
    if (separator[0] == ' ') csv.setSkipEmptyParts();

    //  Now read the file
    FileName csvF(csvfile.toStdString());
    csvfile = QString::fromStdString(csvF.expanded());
    try {
      csv.read(csvfile);
    } catch (IException &ie) {
      std::string mess =  "Could not read CSV file \'" + csvfile.toStdString() + "\'";
      throw IException(ie, IException::User, mess, _FILEINFO_);
    }

    //  Now get the data from the CSV table
    int ncols = csv.columns();
    int nrows = csv.rows();

    // Initial conditions selects all rows and columns
    int startColumn((rowHeader) ? 1 : 0), endColumn(ncols-1);
    int startRow(0), endRow(nrows-1);

    // Update columns
    QString colName(getValue("ColumnName"));
    if (!colName.isEmpty()) {
      addHistory("ColumnName", colName);
      CSVReader::CSVAxis chead = csv.getHeader();
      startColumn = getAxisIndex(colName, chead);
      if (startColumn < 0) {
        std::string mess = "Column name " + colName.toStdString() +
                      " not found in CSV file " + csvfile.toStdString();
        throw IException(IException::User, mess, _FILEINFO_);
      }
      endColumn = startColumn;
      addHistory("ColumnIndex", ToString(startColumn));
    }
    else if (!(getValue("ColumnIndex").isEmpty())) {
       startColumn = ToInteger(getValue("ColumnIndex")) +
                                 ((rowHeader) ? 1 : 0);
       endColumn = startColumn;
      addHistory("ColumnStart", ToString(startColumn));
      addHistory("ColumnEnd", ToString(endColumn));
    }

    // Update row indicies
    QString rowName(getValue("RowName"));
    if (!rowName.isEmpty()) {
      addHistory("RowName", rowName);
      if (!rowHeader) {
        std::string mess = "Row name given but config does not specify presence of row header!";
        throw IException(IException::User, mess, _FILEINFO_);
      }
      CSVReader::CSVAxis rhead = csv.getColumn(0);
      startRow = getAxisIndex(rowName, rhead);
      if (startRow < 0) {
        std::string mess = "Row name " + rowName.toStdString() + " not found in CSV file " + csvfile.toStdString();
        throw IException(IException::User, mess, _FILEINFO_);
      }
      endRow = startRow;
      addHistory("RowIndex", ToString(startRow));
    }
    else if (!(getValue("RowIndex").isEmpty())) {
       startRow = ToInteger(getValue("RowIndex"));
       if (rowHeader) startRow++;
       endRow = startRow;
      addHistory("RowStart", ToString(startRow));
      addHistory("RowEnd", ToString(endRow));
    }

    //  Now ready to read all row/columns and convert to matrix
    int drows(endRow-startRow+1);
    int dcols(endColumn-startColumn+1);
    HiMatrix d(drows, dcols);
    vector<QString> errors;
    for (int r = startRow, hr = 0 ; r <= endRow ; r++, hr++) {
      CSVReader::CSVAxis row = csv.getRow(r);
      for (int c = startColumn, hc = 0 ; c <= endColumn ; c++, hc++) {
        try {
          d[hr][hc] = ToDouble(row[c]);
        }
        catch (...) {
          std::ostringstream mess;
          mess << "Invalid real value (" << row[c].toStdString() << ") in row index " << r;
          if (!rowName.isEmpty()) mess << " (Name:" << rowName.toStdString() << ")";
          mess << ", column index " << c;
          if (!colName.isEmpty()) mess << " (Name:" << colName.toStdString() << ")";
          errors.push_back(mess.str().c_str());
          d[hr][hc] = Null;
        }
      }
    }

    // Save data anyway
    _data = d;

    // Check for errors
    if (errors.size() > 0) {
      //iException::Clear(); Not sure how this could ever do anything
      std::ostringstream mess;
      mess << "Conversion errors in CSV file " + csvfile.toStdString() + ": Errors: ";

      std::vector<QString>::const_iterator it = errors.begin();

      while (it != errors.end()) {
        mess << it->toStdString() << "; ";
        it++;
      }
      throw IException(IException::User, mess.str().c_str(), _FILEINFO_);
    }
    return;
  }


  QString LoadCSV::filename() const {
    return (getValue());
  }

  int LoadCSV::size() const {
    return (_data.dim1() * _data.dim2());
  }

  bool LoadCSV::validateSize(const int &expected, const bool &throw_on_error)
                             const {
    if (expected != size()) {
      if (!throw_on_error) return (false);
      ostringstream mess;
      mess << "Invalid count (Expected: " << expected << ", Received: "
           << size() << ") in CSV file " << getValue().toStdString();
      throw IException(IException::User, mess.str(), _FILEINFO_);
    }
    return (true);
  }

  HiVector LoadCSV::getVector() const {
    HiVector v(size(), const_cast<double *> (_data[0]));
    return (v.copy());
  }

  HiMatrix LoadCSV::getMatrix() const {
    return (_data.copy());
  }


  void LoadCSV::History(HiHistory &history) const {
    std::ostringstream mess;
    mess << "LoadCSV(";
    QString comma("");
    for (unsigned int i = 0 ; i < _history.size() ; i++) {
      mess << comma.toStdString() << _history[i].toStdString();
      comma = ",";
    }
    mess << ")";
    history.add(mess.str().c_str());
    return;
  }

  void LoadCSV::init(const QString &base, const HiCalConf &conf,
                     const DbProfile &profile) {
    _base = base;
    _csvSpecs = ResolveKeys(base, conf, profile);
    _history.clear();
    return;
  }

  void LoadCSV::addHistory(const QString &element,
                           const QString &desc) {
    std::ostringstream mess;
    mess << element.toStdString() << "[" << desc.toStdString() << "]";
    _history.push_back(mess.str().c_str());
  }

  void LoadCSV::getKeyList(const QString &base,
                           std::vector<QString> &keys) const {
    keys.clear();
    keys.push_back(base);
    keys.push_back(base+"IgnoreComments");
    keys.push_back(base+"ColumnHeader");
    keys.push_back(base+"ColumnName");
    keys.push_back(base+"ColumnIndex");
    keys.push_back(base+"RowHeader");
    keys.push_back(base+"RowName");
    keys.push_back(base+"RowIndex");
    keys.push_back(base+"SkipLines");
    keys.push_back(base+"Header");
    keys.push_back(base+"Separator");
    return;
  }

  DbProfile LoadCSV::ResolveKeys(const QString &base, const HiCalConf &conf,
                                 const DbProfile &prof) const {
    vector<QString> keys;
    getKeyList(base, keys);
    DbProfile keyprof("LoadCSV");
    for (unsigned int i = 0 ; i < keys.size() ; i++) {
      QString kvalue = ParsedKey(keys[i], conf, prof);
      if (!kvalue.isEmpty())  keyprof.add(keys[i], kvalue);
    }
    return (keyprof);
  }

  QString LoadCSV::ParsedKey(const QString &key, const HiCalConf &conf,
                        const DbProfile &prof) const {
    QString value("");
    if (prof.exists(key)) {
      value = conf.resolve(prof(key), prof);
    }
    return (value);
  }

  QString LoadCSV::makeKey(const QString &suffix) const {
    QString key = _base + suffix;
    return (key);
  }

  QString LoadCSV::getValue(const QString &suffix) const {
    QString key = makeKey(suffix);
    QString value("");
    if (_csvSpecs.exists(key)) value = _csvSpecs(key);
    return (value);
  }

  int LoadCSV::getAxisIndex(const QString &name,
                            const CSVReader::CSVAxis &header) const {
    QString head = name.trimmed();
    for (int i = 0 ; i < header.dim() ; i++) {
      if (head.toLower() == header[i].toLower().trimmed()) {
        return (i);
      }
    }
    return (-1);
  }

} // namespace ISIS
