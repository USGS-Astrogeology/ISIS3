/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "IException.h"
#include "FileName.h"
#include "Message.h"
#include "TextFile.h"
#include "IString.h"

#include <iostream>

#include <QFileInfo>

using namespace std;
namespace Isis {

  //! Constructs an empty TextFile object
  TextFile::TextFile() {
  }

  /**
   * Constructs a TextFile object and opens the specified file (including path).
   *
   * @param filename FileName (including path) to be opened by TextFile object.
   *
   * @param openmode Open Mode of file opened by TextFile object. InputOpens file
   *                 for Input: Read Only, Fails if file does not exist
   *                 OutputOpens file for Output: Read / Write - Creates file,
   *                 Fails if file exists. OverwriteOpens file for Output:
   *                 Read / Write - Creates file, Truncates if file exists.
   *                 AppendOpens file for Append: Read / Write- Creates file,
   *                 Appends if file exists. Defaults to "input"
   *
   * @param extension Extension to be added to filename (added only if not
   *                  already on filename). Defaults to ""
   */
  TextFile::TextFile(const QString &filename,
                     const char *openmode, const char *extension) {
    SetComment();
    SetNewLine();
    Open(filename, openmode, extension);
  }

  /**
   * Constructs a TextFile object and opens the specified file (including path),
   * reads or writes file, leaves file open for further use of the rest of the
   * methods
   *
   * @param filename FileName (including path) to be opened by TextFile object.
   *
   * @param openmode Open Mode of file opened by TextFile object. InputOpens file
   *                 for Input: Read Only, Fails if file does not exist
   *                 OutputOpens file for Output: Read / Write - Creates file,
   *                 Fails if file exists. OverwriteOpens file for Output:
   *                 Read / Write - Creates file, Truncates if file exists.
   *                 AppendOpens file for Append: Read / Write - Creates file,
   *                 Appends if file exists. Defaults to "input"
   *
   * @param lines IString Vector to fill if reading or put to file if writing
   *
   * @param maxLinesToReadWrite Limits the maximum lines read or written; 0=read
   *                            entire file or write entire vectorDefaults to 0
   *
   * @param skipComments Controls whether comments lines are filtered;
   *                     true=filter, false=return any line read from
   *                     fileDefaults to true
   */
  TextFile::TextFile(const QString &filename, const char *openmode,
                     std::vector<QString> &lines, const int &maxLinesToReadWrite,
                     const bool skipComments) {
    SetComment();
    SetNewLine();
    Open(filename, openmode);
    if(p_openmode == 1) {
      GetFile(lines, maxLinesToReadWrite, skipComments);
    }
    else {
      PutFile(lines, maxLinesToReadWrite);
    }
  }
  /**
   * Constructs a TextFile object and opens the specified file (including path),
   * reads or writes file, leaves file open for further use of the rest of the
   * methods
   *
   * @param filename FileName (including path) to be opened by TextFile object.
   *
   * @param openmode Open Mode of file opened by TextFile object. InputOpens file
   *                 for Input: Read Only, Fails if file does not exist
   *                 OutputOpens file for Output: Read / Write - Creates file,
   *                 Fails if file exists. OverwriteOpens file for Output:
   *                 Read / Write - Creates file, Truncates if file exists.
   *                 AppendOpens file for Append: Read / Write - Creates file,
   *                 Appends if file exists. Defaults to "input"
   *
   * @param lines
   *
   * @param maxLinesToReadWrite Limits the maximum lines read or written; 0=read
   *                            entire file or write entire vectorDefaults to 0
   *
   * @param skipComments Controls whether comments lines are filtered;
   *                     true=filter, false=return any line read from
   *                     fileDefaults to true
   */
  TextFile::TextFile(const char *filename, const char *openmode,
                     std::vector<QString> &lines, const int &maxLinesToReadWrite,
                     const bool skipComments) {
    TextFile(QString(filename), openmode, lines, maxLinesToReadWrite, skipComments);
  }

  /**
   * Constructs a TextFile object and opens the specified file (including path),
   * reads or writes file, leaves file open for further use of the rest of the
   * methods
   *
   * @param filename FileName (including path) to be opened by TextFile object.
   *
   * @param openmode Open Mode of file opened by TextFile object. InputOpens file
   *                 for Input: Read Only, Fails if file does not exist
   *                 OutputOpens file for Output: Read / Write - Creates file,
   *                 Fails if file exists. OverwriteOpens file for Output:
   *                 Read / Write - Creates file, Truncates if file exists.
   *                 AppendOpens file for Append: Read / Write - Creates file,
   *                 Appends if file exists. Defaults to "input"
   *
   * @param lines
   *
   *
   * @param maxLinesToReadWrite Limits the maximum lines read or written; 0=read
   *                            entire file or write entire vectorDefaults to 0
   *
   * @param skipComments Controls whether comments lines are filtered;
   *                     true=filter, false=return any line read from
   *                     fileDefaults to true
   */
  TextFile::TextFile(const QString &filename, const char *openmode,
                     QString *lines, const int &maxLinesToReadWrite,
                     const bool skipComments) {
    SetComment();
    SetNewLine();
    Open(filename, openmode);
    if(p_openmode == 1) {
      GetFile(lines, maxLinesToReadWrite, skipComments);
    }
    else {
      PutFile(lines, maxLinesToReadWrite);
    }
  }

  /**
   * Constructs a TextFile object and opens the specified file (including path),
   * reads or writes file, leaves file open for further use of the rest of the
   * methods
   *
   * @param filename FileName (including path) to be opened by TextFile object.
   *
   * @param openmode Open Mode of file opened by TextFile object. InputOpens file
   *                 for Input: Read Only, Fails if file does not exist
   *                 OutputOpens file for Output: Read / Write - Creates file,
   *                 Fails if file exists. OverwriteOpens file for Output:
   *                 Read / Write - Creates file, Truncates if file exists.
   *                 AppendOpens file for Append: Read / Write - Creates file,
   *                 Appends if file exists. Defaults to "input"
   *
   * @param lines
   *
   * @param maxLinesToReadWrite Limits the maximum lines read or written; 0=read
   *                            entire file or write entire vectorDefaults to 0
   *
   * @param skipComments Controls whether comments lines are filtered;
   *                     true=filter, false=return any line read from
   *                     fileDefaults to true
   */
  TextFile::TextFile(const char *filename, const char *openmode,
                     QString *lines, const int &maxLinesToReadWrite,
                     const bool skipComments) {
    TextFile(QString(filename), openmode, lines, maxLinesToReadWrite, skipComments);
  }


  //! Closes file (if still open). Destroys the TextFile object
  TextFile::~TextFile() {
    Close();
  }


  // Methods

  /**
   * Opens a text file.
   *
   * @param filename FileName (including path) to be opened.
   *
   * @param openmode Open Mode of file to be opened. There are
   *                 four options, "input", "output", "overwrite",
   *                 and "append". "input" opens with in fstream
   *                 option, "output" opens with in, out, and
   *                 trunc fstream options and verifies the file
   *                 does not exist, "overwrite" opens with in,
   *                 out, and trunc fstream options, and "append"
   *                 opens with in, out, and ate(at end) fstream
   *                 options, "append" will open with in and out
   *                 options if the file does not exist.
   *
   *
   * @param extension Extension to be added to filename (added only if not already
   *                  on filename). Defaults to ""
   *
   * @throws Isis::IException::Programmer
   * @throws Isis::IException::Io - output file already exists
   * @throws Isis::IException::Io - unable to open file
   */
  void TextFile::Open(const QString &filename, const char *openmode,
                      const char *extension) {
    // Open (filename [,openmode] [, with_extension ])
    // default openmode = 'input'
    // default extension = 'txt'; extension = "" opens without default

    // note: in append mode, the input and output pointers move together


    // Don't open if it already is
    if(p_stream.is_open()) {
      std::string message = "TextFile:Open:-> Already opened with this object: ["
                        + QString(openmode) + "]:[" + p_filename + "]";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    p_openmode = 0;

    // Save the filename for error messages

    Isis::FileName filenameTmp(filename.toStdString());
    filenameTmp.addExtension(extension);
    p_filename = QString::fromStdString(filenameTmp.expanded());


    //  input, output, overwrite, append

    string chkOpenmode = Isis::IString(openmode).DownCase();
    if(chkOpenmode == "input")      {
      p_openmode = 1;
    }
    else if(chkOpenmode == "output")     {
      p_openmode = 2;
    }
    else if(chkOpenmode == "overwrite")  {
      p_openmode = 3;
    }
    else if(chkOpenmode == "append")     {
      p_openmode = 4;
    }
    else {
      std::string message = "TextFile::-> Unknown openmode: (input, output, overwrite, append):["
                       + QString(openmode) + "]:[" + p_filename + "]";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    // Input
    if(p_openmode == 1) {
      p_stream.open(p_filename.toLatin1().data(), fstream::in);
    }
    // Output
    else if(p_openmode == 2) {
      // first check if file already exists
      if(filenameTmp.fileExists() && QFileInfo(QString::fromStdString(filenameTmp.toString())).size() > 0) {
        std::string message = "TextFile:Open: -> Output file already exists ["
                          + QString(openmode) + "]:[" + p_filename + "]";
        throw IException(IException::Io, message, _FILEINFO_);
      }

      p_stream.open(p_filename.toLatin1().data(), fstream::in | fstream::out | fstream::trunc);
      p_stream.clear();
    }
    // Overwrite
    else if(p_openmode == 3) {
      p_stream.open(p_filename.toLatin1().data(), fstream::in | fstream::out | fstream::trunc);
    }
    // Append
    else if(p_openmode == 4) {
      // Open in append if it does exist, otherwise, open in overwrite mode
      if(filenameTmp.fileExists()) {
        p_stream.open(p_filename.toLatin1().data(), fstream::in | fstream::out | fstream::ate);
      }
      else {
        p_stream.open(p_filename.toLatin1().data(), fstream::in | fstream::out | fstream::trunc);
      }
    }

    if(!p_stream.is_open()) {
      std::string message = "TextFile:Open:-> Unable to open: ["
                       + QString(openmode) + "]:[" + p_filename + "]";
      throw IException(IException::Io, message, _FILEINFO_);
    }
  }

  bool TextFile::OpenChk(bool bailIfNotOpen) {
    if(p_stream.is_open()) {
      return(true);
    }
    else {
      if(bailIfNotOpen) {
        std::string message = "TextFile::-> File not open: [" + p_filename + "]";
        throw IException(IException::Programmer, message, _FILEINFO_);
      }
      else {
        return(false);
      }
    }
  }

  //! Sets Read / Write pointer to begining of opened file
  void TextFile::Rewind() {
    OpenChk(true);
    if(p_stream.eof()) {
      p_stream.clear();
    }
    p_stream.seekg(0, ios_base::beg);
  }

  //! Closes file. Called automatically by TextFile destructor.
  void TextFile::Close() {
    if(p_stream.is_open()) {
      p_stream.flush();
      p_stream.close();
    }
  }

  // vector array
  void TextFile::GetFile(std::vector<QString> &lines, const int &maxLinesToRead,
                         const bool skipComments) {
    OpenChk(true);
    QString line;
    int lineCount = 0;
    while(GetLine(line, skipComments)) {
      if(maxLinesToRead > 0) {
        if(lineCount++ >= maxLinesToRead) {
          break;
        };
      }
      lines.push_back(line);
    }
  }

  // string array
  void TextFile::GetFile(QString *lines, const int &maxLinesToRead,
                         const bool skipComments) {
    OpenChk(true);
    QString line;
    int lineCount = 0;
    while(GetLine(line, skipComments)) {
      if(maxLinesToRead > 0) {
        if(lineCount > maxLinesToRead) {
          break;
        };
      }
      else if(lines[lineCount] == "\0") {
        break;
      }
      lines[lineCount] = line;
      lineCount++;
    }
  }

  // vector array
  void TextFile::PutFile(std::vector<QString> &lines, const int &maxLinesToWrite) {
    OpenChk(true);
    for(int lineCount = 0; lineCount < (int) lines.size(); lineCount++) {
      if(maxLinesToWrite > 0) {
        if(lineCount > maxLinesToWrite) {
          break;
        };
      }
      PutLine(lines[lineCount]);
    }
  }

  // string array
  void TextFile::PutFile(const QString *lines, const int &maxLinesToWrite) {
    OpenChk(true);
    int lineCount = 0;
    while(true) {
      if(maxLinesToWrite > 0) {
        if(lineCount > maxLinesToWrite) {
          break;
        };
      }
      else if(lines[lineCount] == "\0") {
        break;
      }
      PutLine(lines[lineCount]);
      lineCount++;
    }
  }

  /**
   * Gets next line from file. Returns True if read a line, False if End Of File.
   *
   * @param line Line read from file, with newline removed.
   *
   * @param skipComments Controls whether comments lines are filtered;
   *                     true=filter, false=return any line read from
   *                     fileDefaults to true
   *
   * @return bool
   */
  bool TextFile::GetLine(QString &line, bool skipComments) {
    return (p_GetLine(line, skipComments));
  }

  /**
   * Gets next line from file. Returns True if read a line, False if End Of File.
   *
   * @param skipComments Controls whether comments lines are filtered;
   *                     true=filter, false=return any line read from
   *                     fileDefaults to true
   *
   * @return bool
   */
  bool TextFile::GetLine(bool skipComments) {
    QString line;
    return (p_GetLine(line, skipComments));
  }

  /**
   * Gets next NON-COMMENT line from file. Returns True if read a line, False if
   * End Of File. See SetComment method.
   *
   * @param line Line read from file, with newline removed.
   *
   * @return bool
   */
  bool TextFile::GetLineNoFilter(QString &line) {
    return (p_GetLine(line, false));
  }

  /**
   * Gets next NON-COMMENT line from file. Returns True if read a line, False if
   * End Of File. See SetComment method.
   *
   * @return bool
   */
  bool TextFile::GetLineNoFilter() {
    QString line;
    return (p_GetLine(line, false));
  }

  /**
   * Gets next line from file. Returns True if read a line, False if End Of File.
   *
   * @param line Line read from file, with newline removed.
   *
   * @param chkComment True=Skip Comment Lines, False=Return All Lines.
   *
   * @return bool
   *
   * @throws Isis::IException::Io - error reading text file
   */
  bool TextFile::p_GetLine(QString &line, bool chkComment) {
    OpenChk(true);

    line = "";

    // Try to read the next line
    std::string lineStdString;
    getline(p_stream, lineStdString);
    line = lineStdString.c_str();

    // Check for end of file
    if(p_stream.eof()) {
      return false;
    }

    // See if an error occured
    if(!p_stream.good()) {
      line = "";
      std::string message = "TextFile:GetLine: -> Error reading text file: ["
                       + p_filename + "]";
      throw IException(IException::Io, message, _FILEINFO_);
    }

    // See if we have a comment and if we need to ignore
    if(chkComment) {
      if(p_commentString.length()) {
        int locComment = line.indexOf(p_commentString);
        if(locComment != -1) {
          int afterWhiteSpace = line.indexOf(QRegExp("[^\\s]"));
          if((locComment == 0) || (locComment == afterWhiteSpace)) {
            return p_GetLine(line, chkComment);
          }
        }
      }
    }

    // We have a good line
    return true;
  }

  /**
   * Writes string to file and appends a 'newline' string. See SetNewLine method.
   *
   * @param line IString to be written to file. Defaults to ""
   */
  void TextFile::PutLine(const QString &line) {
    PutLine(line.toLatin1().data());
  }

  /**
   * Writes char string to file and appends a 'newline' string.
   * See SetNewLine method.
   *
   * @param line Char string to be written to file.Defaults to ""
   *
   * @throws Isis::IException::Io - error writing text to file
   * @throws Isis::IException::Programmer - input is read only text file, cannot
   *                                        write to file
   */
  void TextFile::PutLine(const char *line) {
    OpenChk(true);

    // Try to write the next line
    p_stream << line << p_newLineString;
    // See if an error occured
    if(!p_stream.good()) {
      if(p_openmode != 1) {
        std::string message = "TextFile:PutLine: -> Error writing text file: ["
                         + p_filename + "]";
        throw IException(IException::Io, message, _FILEINFO_);
      }
      else {
        std::string message =
          "TextFile:PutLine: -> Attempt to write to INPUT - Read Only text file: ["
          + p_filename + "]";
        throw IException(IException::Programmer, message, _FILEINFO_);
      }
    }
  }

  /**
   * Writes string to file,  prepends a 'comment' string and appends a 'newline'
   * string.  See SetComment method and SetNewLine method.
   *
   * @param line IString to be written to file.Defaults to ""
   */
  void TextFile::PutLineComment(const QString &line) {
    PutLine(p_commentString + line);
  }

  /**
   * Writes char string to file,  prepends a 'comment' string and appends a
   * 'newline' string.  See SetComment method and SetNewLine method.
   *
   * @param line Char string to be written to file.Defaults to ""
   */
  void TextFile::PutLineComment(const char *line) {
    PutLine(p_commentString + QString(line));
  }


  QString TextFile::GetComment() {
    return(p_commentString);
  }


  /**
   * Sets the 'comment' string.  Default = '#' See ReadFilter method. Skips lines
   * that begin with this string. See PutLineComment method. Prepends 'comment'
   * string to output line.
   *
   * @param commentString The string of characters to be used for 'comment'
   *                      lines. Defaults to "#"
   */
  void TextFile::SetComment(const char *commentString) {
    p_commentString = commentString;
  }


  QString TextFile::GetNewLine() {
    return(p_newLineString);
  }

  /**
   * Sets the 'newline' string.  Default = '\n'. See PutLine and PutLineComment
   * methods. Appends 'newline' string to output line.
   *
   * @param newLineString The string of characters to be used for 'comment'
   *                      lines. Defaults to "\n"
   */
  void TextFile::SetNewLine(const char *newLineString) {
    p_newLineString = newLineString;
  }

  /**
   * Counts number of lines in file.
   *
   * @param maxLinesToRead Limits lines counted - intended for large files.
   *                       0=count all lines. n=count up to and including n
   *                       lines; if file longer than n lines, returns n+1.
   *                       Defaults to 0
   *
   * @return streamsize
   */
  int TextFile::LineCount(const int &maxLinesToRead) {
    OpenChk(true);

    // LineCount ( [maxLinesToRead] )  --- returns number of lines in open file

    bool eofStat = false;
    if(p_stream.eof()) {  // current state of stream is 'eof'
      eofStat = true;
      p_stream.clear();
    }

    streampos savePos = p_stream.tellg();
    p_stream.seekg(0, ios_base::beg);

    int lineCount = 0;
    string tmpLine;
    if(maxLinesToRead > 0) {
      while((getline(p_stream, tmpLine)) && (lineCount <= maxLinesToRead)) {
        lineCount++;
      }
    }
    else {
      while(getline(p_stream, tmpLine)) {
        lineCount++;
      }
    }

    if(p_stream.eof()) {
      p_stream.clear();
    }

    p_stream.seekg(savePos, ios_base::beg);

    if(eofStat) {           // restore current state of stream 'eof'
      p_stream.seekg(0, ios_base::end);
      p_stream.get();
    }
    return lineCount;
  }

  /**
   * Counts number of bytes in file.
   *
   * @return streamsize
   */
  streamsize TextFile::Size() {
    OpenChk(true);

    // Size () --- returns file size in bytes

    bool eofStat = false;
    if(p_stream.eof()) {  // current state of stream is 'eof'
      eofStat = true;
      p_stream.clear();
    }

    streampos savePos = p_stream.tellg();
    p_stream.seekg(0, ios_base::end);
    streamsize bytes = p_stream.tellg();
    p_stream.seekg(savePos, ios_base::beg);

    if(eofStat) {           // restore current state of stream 'eof'
      p_stream.seekg(0, ios_base::end);
      p_stream.get();
    }
    return bytes;
  }
} // end namespace isis


