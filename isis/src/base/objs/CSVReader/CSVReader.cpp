/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <string>
#include <vector>
#include <numeric>
#include <iostream>
#include <sstream>
#include "IString.h"
#include "CSVReader.h"
#include "CollectorMap.h"
#include "IException.h"

using namespace std;

namespace Isis {

  /**
   * @brief Default constructor for CSV reader
   *
   * The default constructor sets up to read a source that has not header and
   * skips no lines.  It also sets the delimiter to the comma, as implied by its
   * name (CSV = comma separated value), and treats multiple successive occurances
   * of the delimiting character as individual tokens (keeping empty parts).
   *
   * This method can be used when deferring the reading of the input source.
   * Other methods available in this class can be used to adjust the behavior of
   * the parsing before [i]and[/i] after reading of the source as parsing is
   * performed on demand.  This means a single input source can be parsed
   * repeatedly after adjusting parameters.
   */
  CSVReader::CSVReader() : _header(false), _skip(0),
    _delimiter(','), _keepParts(true), _lines(),
    _ignoreComments(true) { }

  /**
   * @brief Parameterized constructor for parsing an input file source
   *
   * This constructor can be used when the input source is an identified file.
   * Parameters are available for specifying the parsing behavior, but are not
   * necessarily required here as defaults are provided.  Other methods in this
   * class can set parsing conditions after the input file has been read in.
   *
   * If the file cannot be opened or an error is encountered during the reading of
   * the file, an Isis exception is thrown.
   *
   * All lines are read in from the file and stored for subsequent parsing.
   * Therefore, parsing can be performed at any time upon returning from this
   * constructor.
   *
   * @param csvfile  Name of file to open and read
   * @param header   Indicates if a header exists (true) in the file or not
   *                 (false)
   * @param skip  Number of lines to skip to header, if it exists, or to the first
   *              data line
   * @param delimiter  Indicates the character to be used to delimit each token in
   *                   the string/line
   * @param keepEmptyParts  Indicates successive delimiters are to be treated as
   *                        empty tokens (true) or collapsed into one token
   *                        (false)
   */
  CSVReader::CSVReader(const QString &csvfile, bool header, int skip,
                       const char &delimiter, const bool keepEmptyParts,
                       const bool ignoreComments) :
    _header(header), _skip(skip), _delimiter(delimiter),
    _keepParts(keepEmptyParts), _lines(),
    _ignoreComments(ignoreComments) {

    read(csvfile);
  }


  /**
   * @brief Determine the number of columns in the input source
   *
   * This method is applies the parsing conditions to all data lines to determine
   * the number of columns.  Note that it is assumed that all lines contain the
   * same number of columns.
   *
   * If the number of columns vary in any of the lines, the least number of
   * columns found in all lines is returned due to the nature of how the columns
   * are determined.  @see isTableValid().
   *
   * Note that this can be an expensive operation if the input source is large as
   * all lines are parsed.  This does not include the header.  @see columns(const
   * CSVReader::CSVTable &table) for an alternative and more efficient method.
   * That method takes a previously parsed table of all lines as an argument,
   * which is precisely how this method determines the columns.
   *
   * @return int Number of columns in table, smallest column count if some lines
   *         are different
   * @see getColumnSummary()
   */
  int CSVReader::columns()  const {
    return ((rows() > 0) ? columns(getTable()) : 0);
  }

  /**
   * @brief Determine the number of columns in a parser CSV Table
   *
   * This method computes the number of columns from a CSVTable.  This table is a
   * result of the getTable method.
   *
   * It is assumed each row in the table has the same number of columns after
   * parsing.  If one or more of the rows contain differing columns, only the
   * smallest number of columns are reported.
   *
   * @param table The table from which the CVSTable rows are obtained
   *
   * @return int  Number of columns in table, smallest column count if some lines
   *         are different
   * @see getColumnSummary()
   */
  int CSVReader::columns(const CSVReader::CSVTable &table) const {
    CSVColumnSummary summary = getColumnSummary(table);
    return ((summary.size() > 0) ? summary.key(0) : 0);
  }


  /**
   * @brief Reads the entire contents of a file for subsequent parsing
   *
   * This method opens the specified file and reads every line storing them in
   * this object.  It is assumed this file is a text file.  Other methods in this
   * class can be utilized to set parsing conditions before [i]or[/i] after the
   * file has been read.
   *
   * Note that parsing the file is deferred until explicity invoked through other
   * methods in this class.  Users of this class can extract individual rows,
   * columns or the complete table.
   *
   * This object is reentrant.  Additional files can be read in.  Any existing
   * data from previous input sources is discarded upon subsequent reads.
   *
   * @param csvfile  Name of file to read
   */
  void CSVReader::read(const QString &csvfile) {
    ifstream ifile(csvfile.toLatin1().data(), ios::in);
    if(!ifile) {
      std::string mess = "Unable to open file [" + csvfile.toStdString() + "]";
      throw IException(IException::User, mess, _FILEINFO_);
    }

    _lines.clear();
    load(ifile);
    ifile.close();
  }

  /**
   * @brief Retrieve the header from the input source if it exists
   *
   * This method will return the header if it exists after appling the parsing
   * rules.
   *
   * The existance of the header is determined entirely by the user of this class.
   * If the header does not exist, a zero-length array is returned.
   *
   * Note that this routine does not trim leading or trailing whitespace from each
   * header.  This must be handled by the caller.
   *
   * @return CSVReader::CSVAxis  Array containing the elements of the header
   * @see haveHeader()
   * @see setHeader()
   */
  CSVReader::CSVAxis CSVReader::getHeader() const {
    //  Return an empty header if we don't have one
    if((!_header) || (_skip >= rows())) {
      return (CSVAxis(0));
    }
    return (Parser(_lines[_skip], _delimiter, _keepParts).result());
  }

  /**
   * @brief Parse and return the requested row by index
   *
   * This method will parse and return the requested row from the input source as
   * an array.  If the requested row is determined to be an invalid index, then a
   * zero-length array is returned.  It is up to the caller to check for validity
   * of the returned row array.
   *
   * @param index  Index of the desired row to return
   *
   * @return CSVReader::CSVAxis  Array of tokens after parsing rules are applied
   */
  CSVReader::CSVAxis CSVReader::getRow(int index) const {
    //  Return an empty header if we don't have one
    if((index < 0) || (index >= rows())) {
      return (CSVAxis(0));
    }
    return (Parser(_lines[index+firstRowIndex()], _delimiter, _keepParts).result());
  }


  /**
   * @brief Parse and return a column specified by index order
   *
   * This method extracts a column from each row and returns the result.  Note
   * that parsing rules are applied to each row and the column at index is
   * extracted and returned in the array.  The array is always the number of rows
   * from the input source (less skipped lines and header if they exist).
   *
   * It is assumed that every row has the same number of columns (@see
   * isTableValid()) but in the event that the requested column does not exist for
   * any (or all rows for that matter) a default constructed token is returned for
   * that row.  If the requested index is less than 0, an empty column is
   * returned.
   *
   * Columns are 0-based index so the valid number of columns range 0 to
   * (columns() - 1).
   *
   * @param index  Zero-based column index to parse and return
   *
   * @return CSVReader::CSVAxis  Array of token element from each column
   */
  CSVReader::CSVAxis CSVReader::getColumn(int index) const {
    //  Return an empty header if we don't have one
    if(index < 0) {
      return (CSVAxis(0));
    }

    int nrows(rows());
    int nbad(0);
    CSVAxis column(nrows);
    Parser parser;
    for(int i = 0 ; i < nrows ; i++) {
      parser.parse(_lines[i+firstRowIndex()], _delimiter, _keepParts);
      if(parser.size() <= index) {
//      column[i] = Parser::TokenType("");
        nbad++;
      }
      else {
        column[i] = parser(index);
      }
    }

    //  If we had no good columns (index is invalid) return an empty column
    return ((nbad == nrows) ? CSVAxis(0) : column);
  }

  /**
   * @brief Parse and return column specified by header name
   *
   * This method will parse and extract a column that corresponds to named column
   * in the header.  This method return a zero-length array if a header does not
   * exist for this input source or the named column does not exist.
   *
   * The header is parsed using the same rules as each row.  It is the
   * responsibility of the user of this class to specify the existance of a
   * header.  Once the header is parsed, a case-insensitive search of the names is
   * performed until the requested column name is found.  The index of this header
   * name is then used to extract the column from each row.
   *
   * It is assumed the column exists in each row.  If it does not, a default
   * constructed token is returned for non-existant columns in a row.
   *
   * @param hname  Name of the column as it exists in the header
   *
   * @return CSVReader::CSVAxis Column array parsed from each row
   */
  CSVReader::CSVAxis CSVReader::getColumn(const QString &hname) const {
    //  Get the header
    CSVAxis header(getHeader());
    QString head = hname.trimmed();
    for(int i = 0 ; i < header.dim() ; i++) {
      if(head.toLower() == header[i].trimmed().toLower()) {
        return (getColumn(i));
      }
    }

    //  If we reach here, we did not find the column name
    return (CSVAxis(0));
  }


  /**
   * @brief Parse and return all rows and columns in a table array
   *
   * This method returns a 2-D table of all rows and columns after parsing rules
   * are applied.  Each column or token in each row is returned as a
   * CSVParser::TokenType.  Subsequent conversion can be performed if the type
   * sufficiently supports it or the user can provide its own conversion
   * techniques.
   *
   * The validity of the table with regards to column integrity (same number of
   * columns in each row) can be checked with the isTableValid method.  A summary
   * of the number of rows containing differing numbers of columns is provided by
   * the getColumnSummary method.
   *
   * The returned table does not include the header row or any skipped rows.  An
   * empty table, zero-length array is returned if no rows are present.
   *
   * The table itself is a 1-dimenional array that contains a row at each element.
   * This conceptually is a 2-dimensional table.  Each element in the row (first)
   * dimension of the table is a CSVAxis array containing parsed columns or
   * tokens.  Note that the number of columns may vary from row to row.
   *
   * @return CSVReader::CSVTable  2-D table of parsed columns in each row
   */
  CSVReader::CSVTable CSVReader::getTable() const {
    CSVTable table(rows());
    int nrows(rows());
    Parser parser;
    for(int row = 0 ; row < nrows ; row++) {
      parser.parse(_lines[row+firstRowIndex()], _delimiter, _keepParts);
      table[row] = parser.result();
    }
    return (table);
  }

  /**
   * @brief Computes a row summary of the number of distinct columns in table
   *
   * A CSVColumnSummary is a CollectorMap where the key is the number of columns
   * and the value is the number of rows that contain that number of columns.
   * This is useful to determine the consistancy of a parser input source such
   * that every row contains the same number of columns.
   *
   * Once this summary is computed, there should exist one and only ome element in
   * the summary where the key is the column count for each row and the value of
   * that key is the number of rows that contain those columns.
   *
   * This example shows how to determine this information:
   * @code
   *  CSVReader::CSVTable table = csv.getTable();
   *  CSVReader::CSVColumnSummary summary = csv.getColumnSummary(table);
   *  cout << "Number of columns:     " <<  csv.columns(table) << endl;
   *  cout << "Number distinct columns: " << summary.size() << endl;
   *  for (int ncols = 0 ; ncols < summary.size() ; ncols++) {
   *    cout << "--> " << summary.getNth(ncols) << " rows have "
   *         << summary.key(ncols) << " columns." << endl;
   *  }
   * @endcode
   *
   * @param table Input table as returned by the getTable method
   *
   * @return CSVReader::CSVColumnSummary  A CollectorMap that idicates the number
   *         of rows with distinct numbers of columns
   * @see getTable()
   * @see isTableValid()
   */
  CSVReader::CSVColumnSummary CSVReader::getColumnSummary(const CSVTable &table)
  const {
    CSVColumnSummary summary;
    for(int row = 0 ; row < table.dim() ; row++) {
      int n(table[row].dim());
      if(summary.exists(n)) {
        int &count = summary.get(n);
        count++;
      }
      else {
        summary.add(n, 1);
      }
    }

    return (summary);
  }

  /**
   * @brief Indicates if all rows have the same number of columns
   *
   * This method checks the integrity of all rows in the inputs source as to
   * whether they have the same number of columns.
   *
   * @param table  Input table to check for integrity/validty
   *
   * @return bool True if all rows have the same number of columns, false if they
   *         do not
   */
  bool CSVReader::isTableValid(const CSVReader::CSVTable &table) const {
    CSVColumnSummary summary = getColumnSummary(table);
    return (summary.size() <= 1);
  }

  /**
   * @brief Reads all lines from the input stream until an EOF is encoutered
   *
   * This method is the used to read from an input stream all lines of text until
   * an end-of-file (EOF) is encountered.  It is used to perform read operations
   * for all sources of input, files and direct streams as supplied by the users
   * of this class.
   *
   * All lines are assumed to end with a newline sequence pertinent to the systems
   * this software is compiled on.  All lines are stored as they are read in
   * unless they are empty lines.  The default behavior is to treat all lines that
   * begin with a '#' as a comment.  These lines are ignored by default and
   * excluded as they are read.  (Comment and blank line feature was added
   * 2010/04/08.)
   *
   * As lines are read in from the input stream, they are pushed onto the internal
   * stack in the order they are read.  The calling environment is responsible for
   * the state of the stack as to whether it is cleared or appended to an existing
   * state.
   *
   * @param ifile  Input source stream of lines of text
   *
   * @return std::istream& Returns the state of the input stream at the end of
   *         read operations.
   */
  std::istream &CSVReader::load(std::istream &ifile) {

    std::string iline;
    int nlines(0);
    while(getline(ifile, iline)) {
      if(!iline.empty()) {
        if(!(_ignoreComments && (iline[0] == '#'))) {
          _lines.push_back(iline.c_str());
          nlines++;
        }
      }
    }

    if(!ifile.eof()) {
      ostringstream mess;
      mess << "Error reading line [" << (nlines + 1) << "]" << ends;
      throw IException(IException::User, mess.str(), _FILEINFO_);
    }

    return (ifile);
  }

  /**
   * @brief Input read operator for input stream sources
   *
   * This input operator can be invoked directly from the users environment to
   * read the complete input source.  It can also be used to augment an existing
   * source as this technique does not discard existing data (lines).
   *
   * It is presumed that any additional input sources are consistant to
   * pre-established parsing guidelines otherwise, the integrity of the table is
   * compromized.
   *
   * Here is an example of how to use this method:
   * @code
   * ifstream ifile("myfile.csv");
   * CSVReader csv;
   * ifile >> csv;
   * @endcode
   *
   * @param is  Input stream source
   * @param csv CSVReader object to read input source lines from
   *
   * @return std::istream&  Returns the state of the input stream at EOF or error
   */
  std::istream &operator>>(std::istream &is, CSVReader &csv) {
    return (csv.load(is));
  }

}
