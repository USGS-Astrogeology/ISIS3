/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Table.h"

#include <fstream>
#include <sstream>
#include <string>

#include "Blob.h"
#include "Endian.h"
#include "IException.h"
#include "TableField.h"

using namespace std;
namespace Isis {

  Table::Table(Blob &blob) {
    initFromBlob(blob);
  }


  /**
   * This constructor creates a new table using the given name and record.
   *
   * Note that the record is not added to this table.  It is used to read the
   * TableField names and set the record size (i.e. the number of bytes per
   * record). Thus any records added to this table will be required to match
   * this size.
   *
   * In this constructor, the Table::Association is set to None, the ByteOrder
   * keyword in the labels is set to NULL, and the record information is added
   * to the table.
   *
   * This constructor also calls the parent constructor Blob(tableName,
   * "Table").
   *
   * @param tableName Name of the Table to be read
   * @param rec Name of the TableRecord to be read into the Table
   */
  Table::Table(const QString &tableName, Isis::TableRecord &rec) {
    p_name = tableName;
    p_assoc = Table::None;
    p_label += Isis::PvlKeyword("Records", 0);
    p_label += Isis::PvlKeyword("ByteOrder", "NULL");
    for (int f = 0; f < rec.Fields(); f++) {
      p_label.addGroup(rec[f].pvlGroup());
    }
    p_record = rec;
  }


  /**
   * This constructor creates an empty table from an existing table name
   * to be read in when the Read() method is called. It should not be
   * used to construct a new table object whose data will be filled
   * in later since the record size will be set to 0. This constructor
   * sets the Table::Association to None.
   *
   * This constructor also calls the parent constructor
   * Blob(tableName, "Table").
   *
   * @param tableName Name of the Table to be read
   */
  Table::Table(const QString &tableName) {
    p_name = tableName;
    p_assoc = Table::None;
  }


  /**
   * This constructor reads an existing table using the given table name and
   * file containing the table. This constructor sets the Table::Association
   * to the Association keyword value in the Blob Pvl read from the file, if
   * the keyword exists.
   *
   * This constructor also calls the parent constructor
   * Blob(tableName, "Table").
   *
   * @param tableName Name of the Table to be read
   * @param file Name of the file to be read into the Table
   *
   * @see Blob::Read()
   */
  Table::Table(const QString &tableName, const QString &file) {
    Blob blob(tableName, "Table", file);
    initFromBlob(blob);
  }


  /**
   * This constructor reads an existing table using the given table name and
   * file containing the table and pvl labels. This constructor sets the
   * Table::Association to the Association keyword value in the Blob Pvl
   * read from the file, if the keyword exists.
   *
   * This constructor also calls the parent constructor
   * Blob(tableName, "Table").
   *
   * @param tableName The name of the Table to be read
   * @param file The name of the file to be read into the Table
   * @param fileHeader Pvl labels.
   *
   * @see Blob::Read()
   */
  Table::Table(const QString &tableName, const QString &file, const Pvl &fileHeader) {
    Blob blob(tableName, "Table");
    blob.Read(file, fileHeader);
    initFromBlob(blob);
  }


  /**
   * Copy constructor for an Table object.  This constructor copies TableRecords
   * and the member variable values for record, records, assoc, and swap.
   *
   * @param other The table to copy from
   */
  Table::Table(const Table &other) {
    p_name = other.p_name;
    p_label = other.p_label;
    p_record = other.p_record;
    p_records = other.p_records;
    p_assoc = other.p_assoc;
    p_swap = other.p_swap;

    for (unsigned int i = 0; i < other.p_recbufs.size(); i++) {
      char *data = new char[RecordSize()];

      for (int j = 0; j < RecordSize(); j++) {
        data[j] = other.p_recbufs[i][j];
      }

      p_recbufs.push_back(data);
    }
  }

  /**
   * This constructor takes in a string to create a Table object.
   * 
   * @param tableName The name of the Table to be read
   * @param tableStr The table string
   * @param fieldDelimiter The delimiter to separate fields with
  */
  Table::Table(const QString &tableName, const std::string &tableString, const char &fieldDelimiter) {
    p_name = tableName;

    std::stringstream tableStream;
    tableStream << tableString;

    std::vector<std::string> tableLinesStringList;
    std::string line;
    while(std::getline(tableStream, line, '\n')) {
      tableLinesStringList.push_back(line);
    }

    int numOfFieldValues = tableLinesStringList.size() - 1; // minus the header line 

    std::string fieldNamesLineString = tableLinesStringList.front();
    std::stringstream fieldNamesStringStream;
    fieldNamesStringStream << fieldNamesLineString;

    std::vector<QString> fieldNames;
    std::string fieldNameString;
    while(std::getline(fieldNamesStringStream, fieldNameString, fieldDelimiter)) {
      fieldNames.push_back(QString::fromStdString(fieldNameString));
    }

    // Clear error flags and set pointer back to beginning
    tableStream.clear();
    tableStream.seekg(0, ios::beg);

    // Add records to table
    std::string recordString;
    int index = 0;
    while(std::getline(tableStream, recordString, '\n')) {
      // skip first line bc that's the header line
      if (index == 0) {
        index++;
        continue;
      }
      
      TableRecord tableRecord(recordString, fieldDelimiter, fieldNames, numOfFieldValues);
      p_record = tableRecord;
      this->operator+=(tableRecord);
      index++;
    }

    // Add fields
    for (int f = 0; f < p_record.Fields(); f++) {
      p_label.addGroup(p_record[f].pvlGroup());
    }
  }


  /**
   * Initialize a Table from a Blob that has been read from a file.
   *
   * @param blob The blob to extract the data for the Table from.
   */
  void Table::initFromBlob(Blob &blob) {
    Clear();

    p_label = blob.Label();

    p_name = p_label["Name"][0];
    p_records = p_label["Records"];

    Isis::TableRecord rec;
    for (int g = 0; g < p_label.groups(); g++) {
      if (p_label.group(g).isNamed("Field")) {
        Isis::TableField f(p_label.group(g));
        rec += f;
      }
    }

    p_record = rec;

    p_assoc = Table::None;
    if (p_label.hasKeyword("Association")) {
      QString temp = (QString) p_label["Association"];
      temp = temp.toUpper();
      if (temp == "SAMPLES") p_assoc = Table::Samples;
      if (temp == "LINES") p_assoc = Table::Lines;
      if (temp == "BANDS") p_assoc = Table::Bands;
    }

    // Determine if we need to swap stuff when we read the data
    Isis::ByteOrder bo = Isis::ByteOrderEnumeration(p_label["ByteOrder"]);
    p_swap = false;
    if (Isis::IsLsb() && (bo == Isis::Msb)) p_swap = true;
    if (Isis::IsMsb() && (bo == Isis::Lsb)) p_swap = true;

    for (int rec = 0; rec < p_records; rec++) {
      size_t bufferPos = rec * RecordSize();

      char *buf = new char[RecordSize()];
      memcpy(buf, &blob.getBuffer()[bufferPos], RecordSize());

      if (p_swap) p_record.Swap(buf);
      p_recbufs.push_back(buf);
    }
  }


  /**
   * Sets the Table equal to the input Table object.  This method copies
   * TableRecords and the member variable values for record, records, assoc, and
   * swap.
   *
   * @param other The table to copy from
   *
   * @return @b Table The copied table.
   */
  Table &Table::operator=(const Isis::Table &other) {
    Clear();
    p_name = other.p_name;
    p_label = other.p_label;
    p_record = other.p_record;
    p_records = other.p_records;
    p_assoc = other.p_assoc;
    p_swap = other.p_swap;

    for (unsigned int i = 0; i < other.p_recbufs.size(); i++) {
      char *data = new char[RecordSize()];

      for (int j = 0; j < RecordSize(); j++) {
        data[j] = other.p_recbufs[i][j];
      }

      p_recbufs.push_back(data);
    }

    return *this;
  }


  //! Destroys the Table object
  Table::~Table() {
    Clear();
  }


  /**
   * Write the Table to a file.
   *
   * This uses a Blob to serialize the Table data, see Blob::Write.
   *
   * @param file The file to write the Table to.
   */
  void Table::Write(const QString &file) {
    Blob blob = toBlob();
    blob.Write(file);
  }


  /**
   * The Table's name
   *
   * @return @b QString the name of the Table
   */
  QString Table::Name() const {
    return p_name;
  }


  /**
   * The Table's label
   *
   * Additional information can be stored on the Table's label and will be serialized
   * in the Blob's label when written out to a file.
   *
   * @return @b PvlObject A reference to the label that can be modified
   */
  PvlObject &Table::Label() {
    return p_label;
  }


  /**
   * Sets the association to the input parameter
   *
   * @param assoc Association type
   */
  void Table::SetAssociation(const Table::Association assoc) {
    p_assoc = assoc;
  }


  /**
   * Checks to see if association is Samples
   *
   * @return @b bool Returns true if association is Samples, and false if it is
   *         not
   */
  bool Table::IsSampleAssociated() {
    return (p_assoc == Table::Samples);
  }


  /**
   * Checks to see if association is Lines
   *
   * @return @b bool Returns true if association is Lines, and false if it is
   *         not
   */
  bool Table::IsLineAssociated() {
    return (p_assoc == Table::Lines);
  }


  /**
   * Checks to see if association is Bands
   *
   * @return @b bool Returns true if association is Bands, and false if it is
   *         not
   */
  bool Table::IsBandAssociated() {
    return (p_assoc == Table::Bands);
  }


  /**
   * Returns the number of records
   *
   * @return @b int Number of records
   */
  int Table::Records() const {
    return p_recbufs.size();
  }


  /**
   * Returns the number of fields per record
   *
   * @return @b int Number of fields
   */
  int Table::RecordFields() const {
    return p_record.Fields();
  }


  /**
   * Returns the number of bytes per record
   *
   * @return @b int Number of bytes per record
   */
  int Table::RecordSize() const {
    return p_record.RecordSize();
  }


  /**
   * Reads a TableRecord from the Table
   *
   * @param index Index where desired TableRecord is located
   *
   * @return Returns the TableRecord at specific index
   */
  Isis::TableRecord &Table::operator[](const int index) {
    p_record.Unpack(p_recbufs[index]);
    return p_record;
  }


  /**
   * Adds a TableRecord to the Table
   *
   * @param rec The record to be added to the table
   */
  void Table::operator+=(Isis::TableRecord &rec) {
    if (RecordSize() == 0) {
      IString msg = "Unable to add records to Isis Table ["
                    + p_name + "]. Bytes per record = [0 bytes].";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

     if (RecordSize() != rec.RecordSize()) {
       QString msg = "Unable to add the given record with size = ["
                     + Isis::toString(rec.RecordSize()) + " bytes] to to Isis Table ["
                     + p_name + "] with record size = ["
                     + Isis::toString(RecordSize()) + " bytes]. Record sizes must match.";
       throw IException(IException::Unknown, msg, _FILEINFO_);
     }
    char *newbuf = new char[RecordSize()];
    rec.Pack(newbuf);
    p_recbufs.push_back(newbuf);
  }


  /**
   * Updates a TableRecord
   *
   * @param rec TableRecord to update old TableRecord with
   * @param index Index of TableRecord to be updated
   */
  void Table::Update(const Isis::TableRecord &rec, const int index) {
    rec.Pack(p_recbufs[index]);
  }


  /**
   * Deletes a TableRecord from the Table
   *
   * @param index Index of TableRecord to be deleted
   */
  void Table::Delete(const int index) {
    vector<char *>::iterator it = p_recbufs.begin();
    for (int i = 0; i < index; i++, it++);
    delete [] p_recbufs[index];
    p_recbufs.erase(it);
  }


  /**
   * Clear the table of all records
   */
  void Table::Clear() {
    for (int i = 0; i < (int)p_recbufs.size(); i++) delete [] p_recbufs[i];
    p_recbufs.clear();
  }


  /**
   * Serialze the Table to a Blob that can be written to a file.
   *
   * @return @b Blob The Blob contaning the Table's data
   */
  Blob Table::toBlob() const {
    Blob tableBlob(Name(), "Table");
    PvlObject &blobLabel = tableBlob.Label();

    // Label setup
    blobLabel += PvlKeyword("Records", Isis::toString(Records()));
    int nbytes = Records() * RecordSize();

    if (Isis::IsLsb()) {
      blobLabel+= PvlKeyword("ByteOrder", Isis::ByteOrderName(Isis::Lsb));
    }
    else {
      blobLabel+= PvlKeyword("ByteOrder", Isis::ByteOrderName(Isis::Msb));
    }

    if (p_assoc == Samples) {
      blobLabel += Isis::PvlKeyword("Association", "Samples");
    }
    else if (p_assoc == Lines) {
      blobLabel += Isis::PvlKeyword("Association", "Lines");
    }
    else if (p_assoc == Bands) {
      blobLabel += Isis::PvlKeyword("Association", "Bands");
    }

    for (int i = 0; i < p_label.keywords(); i++) {
      if (!blobLabel.hasKeyword(p_label[i].name())) {
        blobLabel += p_label[i];
      }
    }

    for (int i = 0; i < p_label.comments(); i++){
      blobLabel.addComment(p_label.comment(i));
    }

    for (int g = 0; g < p_label.groups(); g++) {
      blobLabel += p_label.group(g);
    }

    // Binary data setup
    char *buf = new char[nbytes];

    for (int rec = 0; rec < Records(); rec++) {
      size_t bufferPos = rec * RecordSize();

      memcpy(&buf[bufferPos], p_recbufs[rec], RecordSize());
    }

    tableBlob.takeData(buf, nbytes);

    return tableBlob;
  }


  /**
   * Convert the data from a Table into a string.
   *
   * This method will convert all of the Table's records and fields into a
   * string but will not serialze any label information. See TableRecord::toString
   * for how the records are converted into a string.
   *
   * @param table The Table to serialize
   * @param fieldDelimiter The delimiter to use between fields
   *
   * @return @b QString The Table data as a string
   */
  QString Table::toString(Table table, QString fieldDelimiter) {
    QString tableValues;
    // add the first record with header, the given delimiter, and a new line after each record
    tableValues += TableRecord::toString(table[0], fieldDelimiter, true, true);
    // add remaining records without header, the same delimeter, and new line after each record
    for (int recordIndex = 1; recordIndex < table.Records(); recordIndex++) {
      tableValues += TableRecord::toString(table[recordIndex], fieldDelimiter);
    }
    return tableValues;
  }

}
