/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <string>
#include "IException.h"
#include "FileName.h"
#include "Message.h"
#include "CisscalFile.h"
#include "TextFile.h"
#include "IString.h"

using namespace std;
namespace Isis {
  /**
   * Constructs a CisscalFile object and opens the specified file
   * (including path).
   *
   * @param filename FileName (including path) to be opened by <b>TextFile</b>  object.
   *
   * @param openmode Open Mode of file opened by <b>TextFile</b>  object.
   *                 <UL>
   *                   <LI><TT><I>"input"</I></TT> Opens file for
   *                       Input: Read Only. Fails if file does
   *                       not exist.
   *                   <LI><TT><I>"output"</I></TT> Opens file
   *                       for Output: Read/Write.  Creates file,
   *                       fails if file exists.
   *                   <LI><TT><I>"overwrite"</I></TT> Opens file
   *                       for Output: Read/Write.  Creates file,
   *                       truncates if file exists.
   *                   <LI><TT><I>"append"</I></TT> Opens file
   *                       for Append: Read/Write.  Creates file,
   *                       appends if file exists.
   *   </UL>
   *                 Defaults to <TT><I>"input"</I></TT>.
   *
   * @param extension Extension to be added to filename (added only if not
   *                  already on filename). Defaults to ""
   */
  CisscalFile::CisscalFile(const QString &filename, const char *openmode,
                           const char *extension): TextFile(filename, openmode, extension) {
    p_begindataFound = false;
  }

  /**
   * @brief Get next line of valid data, protected
   *
   * This method overwrites <b>TextFile</b>'s protected method of
   * the same name.  It is called by the public overwritten method
   * <b>GetLine()</b>, as in the parent class.
   *
   * @param line String to be rewritten with contents of the
   *             next line read from file
   * @returns <b>bool</b> Indicates whether line was read
   * @throws Isis::iException::Io "Error reading text file"
   * @see GetLine()
   */
  bool CisscalFile::p_GetLine(QString &line) {
    OpenChk(true);
    // Try to read the next line
    std::string lineTmp;
    getline(p_stream, lineTmp);
    line = lineTmp.c_str();

    // Check for end of file
    if(p_stream.eof()) {
      line = "";
      return false;
    }
    // See if an error occured
    if(!p_stream.good()) {
      line = "";
      std::string message = "TextFile:GetLine: -> Error reading text file: ["
                        + p_filename.toStdString() + "]";
      throw IException(IException::Io, message, _FILEINFO_);
    }
    // Search for tag "\begindata" if it was not already found by recursively using this method
    if(!p_begindataFound) {
      if(!line.contains("\\begindata")) {
        return p_GetLine(line);
      }
      p_begindataFound = true;
      return p_GetLine(line);
    }
    // We have a good line
    return true;
  }

  /**
   * @brief Get next line of valid data
   *
   * This protected method overrides the base class method from
   * <b>TextFile</b> of the same name. It finds the next line of
   * valid data. If the <tt>"\begindata"</tt> tag has not been
   * already found, it searches for that tag. Once found, every
   * line beyond that is considered data and the method will
   * retrieve the next line. Returns True if read a line, False if
   * End Of File.
   *
   * @param line String to be rewritten with contents of the
   *             next line read from file
   * @returns <b>bool</b> Indicates whether line was read
   * @throws Isis::iException::Io "Error reading text file"
   * @see p_GetLine()
   */
  bool CisscalFile::GetLine(QString &line) {
    return p_GetLine(line);
  }

}//end CisscalFile.cpp
