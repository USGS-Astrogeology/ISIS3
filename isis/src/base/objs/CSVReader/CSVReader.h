#ifndef CSVReader_h
#define CSVReader_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QString>
#include <QStringList>
#include <vector>
#include <fstream>
#include <tnt/tnt_array1d.h>
#include "CollectorMap.h"
#include "IException.h"
#include "IString.h"

namespace Isis {

  /**
   * @brief CSV Parser seperates fields (tokens) from a string  with a delimeter
   *
   * CSVParser is a lightweigh parser that takes a string as an argument, either
   * through the constructor or a method, and parses the string into tokens that
   * are separated by a single delimiting character, usually a comma.  It can work
   * on spaces as well, but typically these types of strings have multiple spaces
   * between them.  For these cases, set keepEmptyParts = false, which treats
   * succesive tokens as a single token.
   *
   * One important note about its token storage mechanism:  It uses the TNT 1-D
   * array class which is reference counted.  This makes exporting of tokens very
   * efficient at the expense of all instances referring to the same token list.
   * This is subtle but can cause many surprising results when other users change
   * the contents of the tokens.  Use the TNT copy() method if you require your
   * own list.
   *
   * This is a templated class that allows the user to select the token storage
   * type.  This type, which defaults to the QString class (I will explain
   * the reason for this shortly), must provide a few features that are
   * explicitly used in providing support for tokenization.
   *
   * The TokenStore class must provide a default, unparameterized constructor
   * since it will be used as the default initializer.
   *
   * The class must also accept a string as a constructor option. This class uses
   * a string splitter/tokenizer that returns a vector of strings.  It then
   * creates the token array and copies the result to it using an assignment
   * statement from the string-accepted constructor of the TokenStore type.
   *
   *  This token storage class is primarily used in the CSVReader which has
   *  additional requirements on the TokenStore type.  See the CSVReader class
   *  documentation for more details.
   *
   * @ingroup Parsing
   * @see CSVReader
   *
   * @author 2006-08-14 Kris Becker, USGS
   *
   * @internal
   *   @history 2007-06-05 Brendan George - Modified to work with
   *                           QString/StringTools merge
   *   @history 2008-06-18 Christopher Austin - Fixed documentation
   *   @history 2017-09-22 Cole Neubauer - Fixed documentation. References #4807
   *   @history 2018-10-18 Kaitlyn Lee - Added "[]" around file names in exception 
   *                           messages. References #5520.

   */
  template <typename TokenStore = QString>
  class CSVParser {
    public:
      typedef TokenStore              TokenType;  //!< Token storage type
      typedef TNT::Array1D<TokenType> TokenList;  //!< List of tokens

      /** Default constructor  */
      CSVParser() { }
      /** Destructor */
      virtual ~CSVParser()  { }

      /**
       * @brief Constructor that parses strings according to given parameters
       *
       * This constructor accepts a string to parse, the delimiter that separates
       * words and indicate whether successive occurances of the delimiter
       * translates into a single value or they are taken as empty tokens.
       *
       * @param str  QString to parse
       * @param delimiter Character that separates individual tokens in the string
       * @param keepEmptyParts Specifies the occurance of successive tokens is to
       *                       treated as one token (false) or each delimiter
       *                       indicates an empty token (true)
       */
      CSVParser(const QString &str, const char &delimiter = ',',
                bool keepEmptyParts = true) {
        parse(str, delimiter, keepEmptyParts);
      }

      /**
       * Returns the number of tokens in the parsed string
       * @return int
       */
      int size() const {
        return (_elements.dim());
      }

      /**
       * @brief Returns the nth token in the parsed string
       *
       * Use of this method and size(), one can iterate through all the tokens in
       * the resulting list using a for loop.  Be sure the element exists before
       * attempting to access it.
       *
       * @param nth Indicates the nth value to return - valid range is 0 to
       *            n_elements.
       * @return const TokenType& Reference to the nth token in the parsed list
       */
      const TokenType &operator()(const int nth) const {
        return (_elements[nth]);
      }

      /**
       * @brief Parser method accepting string, delimiter and multiple token
       *        handling
       *
       * This method duplicates the behavior of the constructor so that it can be
       * maintained for subsequent use.  There is little overhead involved in the
       * construction of this lightweight class, but this allows the same instance
       * to be resued.
       *
       * @param str QString to parse into tokens separated by the delimiter
       * @param delimiter Character that separates each token in str
       * @param keepEmptyParts Specifies the occurance of successive tokens is to
       *                       treated as one token (false) or each delimiter
       *                       indicates an empty token (true)
       *
       * @return int Number of tokens found/parsed in the input string
       */
      int parse(const QString &str, const char &delimiter = ',',
                bool keepEmptyParts = true) {
        QStringList tokens =
            str.split(delimiter, keepEmptyParts? Qt::KeepEmptyParts : Qt::SkipEmptyParts);
        TokenList slist(tokens.size());
        for(int i = 0 ; i < tokens.size() ; i++) {
          slist[i] = TokenType(tokens[i]);
        }
        _elements = slist;
        return (_elements.dim());
      }

      /**
       *  @brief Returns the list of tokens
       *
       *  @return This method returns the complete list of tokens.  Note that it utilizes
       *  the most efficient method of storing and exporting tokens, namely a
       *  reference counted array.
       */
      TokenList result() const {
        return (_elements);
      }

    private:
      TokenList _elements;   //!<  List of tokens parsed from string
  };



  /**
   * @brief Reads strings and parses them into tokens separated by a delimiter
   *        character.
   *
   * The class will read text strings from an input source stream or file where
   * each line (string) contains a single character delimeter that separates them
   * into tokens.  The input stream is text in nature and each line is terminated
   * with a newline as appropriate for the computer system.
   *
   * This class provides methods that support skipping irrelevant lines and
   * recognizing and utlizing a header line.  Tokens within a given line are
   * separated by a single character.  Consecutive delimiter characters can be
   * treated as empty tokens (columns) or translated as a single token.
   * Typically, consecutive tokens as empty strings is used for comma separated
   * values (CSV) whereas space delimited strings oftentimes require multiple
   * spaces to be treated as a single separator.  This class supports both cases.
   *
   * Comments can exist in a CSV and are indicated with '#' as the first character
   * in the line.  Default behavior (as of 2010/04/08) is to ignore these lines as
   * well as blank lines.  Use the setComment() method to alter this behavior.
   * Also note that the skip lines count does not include comments or blank lines.
   *
   * Each text line in the input source is read and stored in an internal stack.
   * Only when explicitly requested does parsing take place - no parsing is
   * performed during the reading of the input source.  This approach allows the
   * users of this class to alter or otherwise adjust parsing conditions after the
   * input source has been internalized.  This makes this implementation efficient
   * and flexible deligating more control to the users of this class.
   *
   * The mechanism in which parsed data is stored and returned to the callers
   * enviroment makes this class efficient.  The returned rows, columns and tables
   * use memory reference counting.  This allows parsed data to be exported with
   * virtually no cost to the calling environment in terms of efficiency.  It does
   * however, lend itself to utilization issues.  Reference counting means that
   * all instances of a parsed row, column or table refer to the same copy of the
   * data and a change in one instance of those elements is reflected in all
   * instances of that same row.  Note that this concern rests entirely on how the
   * caller's environment utilizes returned data as only the original lines read
   * from the input source are maintained internal to objects.
   *
   * The following example demonstrates how to use this class to read a comma
   * delimited file that may have consecutive commas and should be treated as
   * empty columns.  Furthermore, there are 2 lines to skip and a header line as
   * well:
   * @code
   *  cout << "\n\nProcessing comma table...\n";
   *  QString csvfile("comma.csv");
   *  CSVReader csv(csvfile,true,2,','true);
   * @endcode
   *
   * Another way to ingest this file using methods instead of the constructor is
   * as follows:
   * @code
   *  cout << "\n\nProcessing comma table using methods...\n";
   *  QString csvfile("comma.csv");
   *  CSVReader csv;
   *  csv.setSkip(2);
   *  csv.setHeader(true);
   *  csv.setDelimiter(',');
   *  csv.setKeepEmptyParts();
   *  csv.read(csvfile);
   * @endcode
   * Using this method will always purge any previously read data from the
   * CSVReader object.
   *
   * @ingroup Utility
   * @ingroup Parsing
   * @author 2006-08-14 Kris Becker
   *
   * @internal
   *   @history 2008-06-18 Christopher Austin - Fixed documentation
   *   @history 2010-04-08 Kris Becker - Added discarding of comment and blank lines
   */
  class CSVReader {

    private:
      typedef CSVParser<QString> Parser;    //!<  Defines single line parser

    public:
      friend std::istream &operator>>(std::istream &is, CSVReader &csv);

      typedef Parser::TokenList       CSVAxis;   //!<  Row/Column token list
      typedef TNT::Array1D<CSVAxis>   CSVTable;  //!<  Table of all rows/columns
      typedef CollectorMap<int, int>   CSVColumnSummary;  //!< Column summary for all rows

      typedef TNT::Array1D<double>    CSVDblVector;  //!< Double array def.
      typedef TNT::Array1D<int>       CSVIntVector;  //!< Integer array def.

      // Constructors and Destructor
      CSVReader();
      /**
       * @brief constructor
       * @param ignoreComments boolean whether to ignore comments or not
       */
      CSVReader(const QString &csvfile, bool header = false, int skip = 0,
                const char &delimiter = ',', const bool keepEmptyParts = true,
                const bool ignoreComments = true);

      /** Destructor (benign) */
      virtual ~CSVReader() { }

      /**
       * @brief Reports the total number of lines read from the stream
       * @return int Number of lines read from input source
       */
      int size() const {
        return (_lines.size());
      }

      /**
       * @brief Reports the number of rows in the table
       *
       * This method returns only the number of rows of data.  This count does
       * not include skipped lines or the header line if either exists.  Note
       * that if no lines are skipped and no header exists, this count will be
       * identical to size().
       *
       * @return int Number of rows of data from the input source
       */
      int rows() const {
        int nrows(_lines.size() - firstRowIndex());
        return ((nrows < 0) ? 0 : nrows);
      }

      int columns() const;
      int columns(const CSVTable &table) const;

      /**
       * @brief Allows the user to indicate comment disposition
       *
       * Comments are indicated in a CSV file by a '#' sign in the first
       * column.  If they are present, the default is to ignore them and
       * discard them when they are read in.  This method allows the user to
       * specify how to treat lines that begin with a '#' in the off chance they
       * are part of the good stuff.
       *
       * Comment lines are not part of the skip lines parameter unless this is
       * set to false.  Then skip lines will include lines that start with a '#'
       * if they exist.
       *
       * Also not that any and all blanl/empty lines are discarded and not
       * included in any count - includig the skip line count.
       *
       * @param ignore True indicates lines that start with a '#' are considered
       *               a comment and are discarded.  False will not discard
       *               these lines but include them in the parsing content.
       */
      void setComment(const bool ignore = true) {
        _ignoreComments = ignore;
      }

      /**
       * Indicate the number of lines at the top of the source to skip to data
       *
       * This method allows the user to indicate the number of lines that are to
       * be ignored at the begining of the input source.  These lines may
       * contain any text, but are persistantly ignored for all row and column
       * parsing operations.
       *
       * Note that this should not include a header line if one exists as the
       * header methods maintain that information for parsing operations.  It is
       * assumed that header lines always follow skipped lines and immediately
       * precede data lines.
       *
       * This count does not include comments lines (first character is a '#'),
       * if they are ignored (default) or blank lines.
       *
       * @param nskip Number of lines to skip
       */
      void setSkip(int nskip) {
        if(nskip >= 0) _skip = nskip;
      }

      /**
       * @brief Reports the number of lines to skip
       *
       * This is the number of lines to skip to get to the header, if one
       * exists, or to the first row of data to parse.
       *
       * @return int Number of lines to skip
       */
      int  getSkip() const {
        return (_skip);
      }

      /**
       * @brief Returns true if a header is present in the input source
       *
       * The existance of a header line is always determined by the user of this
       * class.  See the setHeader() method for additional information on header
       * maintainence.
       * @return bool whether has CSV has header
       */
      bool haveHeader() const {
        return (_header);
      }

      /**
       * @brief Allows the user to indicate header disposition
       *
       * The determination of a header is entirely up to the user of this class.
       * If a header exists, the user must indicate this with a true parameter
       * to this method.  That line is excluded from the row-by-row and column
       * data parsing operations.  If no header exists, provide false to this
       * method.
       *
       * It is assumed that headers exist immediately prior to data rows and any
       * skipped lines preceed the header line.  Only one line is presumed to be
       * a header.
       *
       * Note that this method can be set at any time in the process of reading
       * from a file or stream source as parsing is done on demand and not at
       * the time the source is read in.
       *
       * @param gotIt True indicates the presence of a header, false indicates
       *              one does not exist.
       */
      void setHeader(const bool gotIt = true) {
        _header = gotIt;
      }


      /**
       * @brief Set the delimiter character that separate tokens in the strings
       *
       * This method provides the user of this class to indicate the character
       * that separates individual tokens in each row, including the header
       * line.
       *
       * One must ensure the delimiter character is not within tokens (such as
       * comma delimited strings) or incorrect parsing will occur.
       *
       * @param delimiter Single character that delimits tokens in each string
       */
      void setDelimiter(const char &delimiter) {
        _delimiter = delimiter;
      }

      /**
       * @brief Reports the character used to delimit tokens in strings
       *
       * @return char Current character used to delimit tokens
       */
      char getDelimiter() const {
        return (_delimiter);
      }

      /**
       * @brief Indicate multiple occurances of delimiters are empty tokens
       *
       * Use of this method indicates that when multiple instances of the
       * delimiting character occure in succession, they should be treated as
       * empty tokens.  This is useful when input sources truly have empty
       * fields.
       */
      void setKeepEmptyParts() {
        _keepParts = true;
      }

      /**
       * @brief Indicate multiple occurances of delimiters are one token
       *
       * Use of this method indicates that when multiple instances of the
       * delimiting character occurs in succession, they should be treated as a
       * single token.  This is useful when input sources have space separated
       * tokens.  Frequently, there are many spaces between values when spaces
       * are used as the delimiting character.  Call this method when spaces are
       * used as token delimiters.
       */
      void setSkipEmptyParts() {
        _keepParts = false;
      }

      /** Returns true when preserving succesive tokens, false when they are
       *  treated as one token.
       *  @see setKeepEmptyParts()
       *  @see setSkipEmptyParts()
       * @return bool
       */
      bool keepEmptyParts() const {
        return (_keepParts);
      }

      void read(const QString &fname);

      CSVAxis getHeader() const;
      CSVAxis getRow(int index) const;
      CSVAxis getColumn(int index) const;
      CSVAxis getColumn(const QString &hname) const;
      CSVTable   getTable() const;
      bool isTableValid(const CSVTable &table) const;

      CSVColumnSummary getColumnSummary(const CSVTable &table) const;

      template <typename T> TNT::Array1D<T> convert(const CSVAxis &data) const;

      /**
       * @brief Discards all lines read from an input source
       *
       * This method discards all lines read from any previous stream.  Any
       * subsequent row or column requests will return an empty condition.
       */
      void clear() {
        _lines.clear();
      }

    private:
      typedef std::vector<QString> CSVList; //!< Input source line container
      bool          _header;     //!<  Indicates presences of header
      int           _skip;       //!<  Number of lines to skip
      char          _delimiter;  //!<  Separator of values
      bool          _keepParts;  //!<  Keep empty parts between delimiter
      CSVList       _lines;      //!<  List of lines from file
      bool          _ignoreComments; //!<  Ignore comments on read

      /**
       * @brief Computes the index of the first data
       *
       * This convenience method computes the index of the first data row
       * considering the number of lines to skip and the existance of a header
       * line.
       *
       * @return int  Index of the first row of data
       */
      int firstRowIndex() const {
        return (_skip + ((_header) ? 1 : 0));
      }

      std::istream &load(std::istream &ifile);
  };


  /**
   * @brief Converts a row or column of data to the specified type
   *
   * This method will convert a row or column of data to the specified type.
   * Since this is a template method, it must be invoked explicity through
   * template syntax.  Here is an example to extract a column by a header name
   * and convert it to a double precision array:
   * @code
   *    //  Convert column 0/1 to double
   *   CSVReader::CSVAxis scol = csv.getColumn("0/1");
   *   CSVReader::CSVDblVector dcol = csv.convert<double>(scol);
   * @endcode
   *
   * At present, this class uses the Isis QString class as its token storage type
   * (TokenType).  All that is required is that it have a cast operator for a
   * given type.  If the Isis QString class has the operator, it can be invoked
   * for that type.  The precise statement used to convert the token to the
   * explict type is:
   * @code
   *   out[i] = (T) s;
   * @endcode
   * In this example, \b s is the individual token and \b T is the type double
   * as in the previous example.
   *
   * Note that conversions of specific special pixel values is not inherently
   * handled by this method.  If you anticipate textual representations of
   * special pixels, such as NULL, LIS etc..., this is left up to the caller to
   * handle directly.
   *
   * @param data Input row or column
   *
   * @return TNT::Array1D<T>  Converted data array of specified type
   */
  template <typename T>
  TNT::Array1D<T> CSVReader::convert(const CSVAxis &data) const {
    TNT::Array1D<T> out(data.dim());
    for(int i = 0 ; i < data.dim() ; i++) {
      Parser::TokenType s = data[i];
      out[i] = s.toDouble();
    }
    return (out);
  }
}


#endif
