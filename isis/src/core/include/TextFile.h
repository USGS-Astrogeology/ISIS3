#ifndef TextFile_h
#define TextFile_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <fstream>
#include <vector>

#include <QString>

namespace Isis {
  /**
   * @brief Provides access to sequential ASCII stream I/O
   *
   * Provides access to sequential ASCII stream I/O. Checks for errors in 
   * opening, reading, and writing ASCII files. Can check for 'comment lines' 
   * while reading a text file. 
   *
   * @ingroup Utility
   *
   * @author 2003-08-29 Glenn Bennett
   *
   * @internal
   *  @history 2005-02-16 Elizabeth Ribelin - Modified file to support Doxygen
   *                          documentation
   *  @history 2010-03-30 Mackenzie Boyd - Modified open method append option to
   *                          create the file if it doesn't exist, changed file
   *                          existence check to use FileName class.
   *  @history 2011-08-08 Jeannie Backer - Modified unitTest to use $temporary
   *                          variable instead of /tmp directory
   *
   *  @todo 2005-02-16 Glenn Bennett - add coded and implementation examples,
   *                       and finish documentation
   */
  class TextFile {

    protected:
      std::fstream p_stream;           //!< File stream handle
      int     p_openmode;              /**< openmode of file: Input, Output,
                                           Overwrite, Append*/
      QString  p_filename;         //!< FileName of the opened file.
      QString  p_commentString;    /**< 'comment' string used by GetLine and
                                           PutLineComment*/
      QString  p_newLineString;    /**< 'newline' string used by PutLine and
                                           PutLineComment*/

      bool p_GetLine(QString &line, bool chkComment);

    public:
      // Constructors and destructors
      TextFile();

      // implied open file
      TextFile(const QString &filename, const char *openmode = "input",
               const char *extension = "");

      // implied getFile/putFile
      TextFile(const   char *filename, const char *openmode,
               std::vector<QString> &lines,
               const int &maxLinesToReadWrite = 0,
               const bool skipComments = true);
      TextFile(const QString &filename,
               const char *openmode, std::vector<QString> &lines,
               const int &maxLinesToReadWrite = 0,
               const bool skipComments = true);
      TextFile(const   char *filename,
               const char *openmode, QString *lines,
               const int &maxLinesToReadWrite, const bool skipComments = true);
      TextFile(const QString &filename,
               const char *openmode, QString *lines,
               const int &maxLinesToReadWrite, const bool skipComments = true);

      ~TextFile();

      void Open(const QString &filename, const char *openmode = "input",
                const char *extension = "");
      void Open(const char *filename, const char *openmode = "input",
                const char *extension = "");

      bool OpenChk(bool bailIfNotOpen = false);

      void Rewind();
      void Close();

      void GetFile(std::vector<QString> &lines, 
                   const int &maxLinesToRead = 0, 
                   const bool skipComments = true);
      void GetFile(QString *lines, const int &maxLinesToRead, 
                   const bool skipComments = true);

      void PutFile(std::vector<QString> &lines, 
                   const int &maxLinesToWrite = 0);
      void PutFile(const QString *lines, const int &maxLinesToWrite);

      bool GetLine(QString &line, const bool skipComments = true);
      bool GetLineNoFilter(QString &line);

      bool GetLine(const bool skipComments = true);
      bool GetLineNoFilter();

      void PutLine(const QString &line);
      void PutLine(const char *line = "");

      void PutLineComment(const QString &line);
      void PutLineComment(const char *line = "");

      QString GetComment();
      QString GetNewLine();

      void SetComment(const char *commentString = "#");
      void SetNewLine(const char *newLineString = "\n");

      int LineCount(const int &maxLinesToRead = 0);
      std::streamsize Size();
  };
};

#endif
