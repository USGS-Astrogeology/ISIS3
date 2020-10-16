#ifndef PvlKeyword_h
#define PvlKeyword_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <vector>
#include <map>
#include <iostream>

#include <QString>
#include <QVariant>
#include <QVarLengthArray>

#include "Constants.h"
#include "IString.h"

#include <nlohmann/json.hpp>

namespace Isis {
  class PvlSequence;
  class PvlFormat;

  /**
   * @brief A single keyword-value pair.
   *
   * This class is used to create a single PVL keyword-value pair. PvlContainer
   * can combine PvlKeyword objects and organize them so they look clean on
   * output.
   *
   * @ingroup Parsing
   *
   * @author 2002-10-11 Jeff Anderson
   *
   * @internal
   *  @history 2005-04-08 Leah Dahmer - Wrote class documentation.
   *  @history 2005-04-08 Leah Dahmer - Added the writeWithWrap() method so keyword values will now
   *                          be wrapped when the length exceeds 80 characters.
   *  @history 2005-05-18 Jeff Anderson - Fixed minor problems with wrapping code.
   *  @history 2006-04-05 Elizabeth Miller - Added AddCommentWrapped() method.
   *  @history 2006-09-05 Stuart Sides - Added ability to format keywords in different ways using
   *                          the PvlFormat class.
   *  @history 2007-08-20 Brendan George - Added checking to ensure Keyword Name contains no
   *                          whitespace.
   *  @history 2008-02-08 Christopher Austin - Altered WriteWithWrap to not bomb when 2 or more
   *                          single statement lines in a row are over 78 characters long.
   *  @history 2008-07-03 Steven Lambright - Added const functionality. 
   *  @history 2008-07-10 Steven Lambright - stringEqual is now static, all AddComments methods are
   *                          public.
   *  @history 2008-09-30 Christopher Austin - replaced all std::endl in the << operator as well as
   *                          writeWithWrap() with PvlFormat.FormatEOL(), and formatted wraps
   *                          accordingly.
   *  @history 2009-08-18 Eric Hyer - Added both SetUnits methods and ASSERT macro.
   *  @history 2009-09-09 Steven Lambright - Removed ASSERT macro, fixed formatting of error in
   *                          SetUnits, and fixed text wrapping when a single array element needed
   *                          split up into multiple lines.
   *  @history 2009-12-07 Steven Lambright - Added stream input operator for reading.
   *  @history 2010-01-19 Travis Addair - Added SetCharLimit method allowing users to set the point
   *                          at which a keyword value is output to the next line down.
   *  @history 2010-02-04 Travis Addair - Moved the SetCharLimit method to PvlFormat class.
   *  @history 2010-04-13 Eric Hyer - Added copy constructor. Added assignment operator.
   *  @history 2010-06-25 Steven Lambright - NULLs ('\0') now count as binary.
   *  @history 2010-09-27 Sharmila Prasad - API to Validate a Keyword for type and values.
   *  @history 2010-10-18 Sharmila Prasad - Added more options for the keyword validation.
   *  @history 2011-04-12 Steven Lambright - Lessened the memory footprint by changing m_comments
   *                          and m_units to pointers, m_values to a QVarLengthArray and m_name
   *                          to a char *.
   *  @history 2011-07-07 Sharmila Prasad - While validating keyword, display appropriate
   *                          error msg when converting string to integer which has a double value.
   *  @history 2013-03-11 Steven Lambright and Mathew Eis - Brought method names and member variable
   *                          names up to the current Isis 3 coding standards. Fixes #1533.
   *  @history 2013-06-05 Tracie Sucharski - When splitting keywords make sure the continuation
   *                          lines does not start with "//" since Isis Pvl considers that a
   *                          comment.  Fixes #1230.
   *  @history 2014-05-13 Jeannie Backer - Changed operator[] error message to construct a
   *                          QString from the m_name char* instead of using the toString() method.
   *                          The call to this method was using toString(bool) and always printing
   *                          the QString "Yes" instead of the keyword name. Added padding on
   *                          control statements to bring the code closer to ISIS Coding Standards.
   *                          References #1659.
   * 
   *  @history 2022-08-15 Adam Paquette - Added the ability to add and set PvlKeyword values from JSON
   *                          keyword value pairs.
   */
  class PvlKeyword {
    public:
      PvlKeyword();
      PvlKeyword(QString name);
      PvlKeyword(QString name, QString value,
                 QString unit = "");
      PvlKeyword(const PvlKeyword &other);
      ~PvlKeyword();

      void setName(QString name);

      /**
       * Returns the keyword name.
       *
       * @return The name of the keyword.
       */
      QString name() const {
        if(m_name)
          return m_name;
        else
          return "";
      };
      /**
       * Determines whether two PvlKeywords have the same name or not.
       *
       * @param name The name of the keyword to compare with this one.
       * @return True if the names are equal, false if not.
       */
      bool isNamed(QString name) const {
        return stringEqual(name, this->name());
      };

      void setValue(QString value, QString unit = "");
      void setJsonValue(nlohmann::json jsonobj, QString unit = "");

      void setUnits(QString units);
      void setUnits(QString value, QString units);

      PvlKeyword &operator=(QString value);

      void addValue(QString value, QString unit = "");
      void addJsonValue(nlohmann::json jsonobj, QString unit = "");

      PvlKeyword &operator+=(QString value);

      //! Returns the number of values stored in this keyword
      int size() const {
        return m_values.size();
      };
      bool isNull(const int index = 0) const;
      void clear();

      friend std::istream &operator>>(std::istream &is, PvlKeyword &result);
      friend std::ostream &operator<<(std::ostream &os,
                                      const PvlKeyword &keyword);

      //! Returns the first value  in this keyword converted to a double
      operator double() const {
        return toDouble(operator[](0));
      };
      //! Returns the first value  in this keyword converted to an integer
      operator int() const {
        return toInt(operator[](0));
      };
      //! Returns the first value  in this keyword converted to a BigInt
      operator Isis::BigInt() const {
        return toBigInt(operator[](0));
      };

      operator QString() const;

      const QString &operator[](int index) const;
      QString &operator[](int index);
      QString unit(const int index = 0) const;

      void addComment(QString comment);
      void addCommentWrapped(QString comment);

      void addComments(const std::vector<QString> &comments);

      //! Returns the number of lines of comments associated with this keyword
      int comments() const {
        return (m_comments ? m_comments->size() : 0);
      };
      QString comment(const int index) const;
      void clearComment();

      /**
       * Returns true of the keyword names match
       *
       * @param key The keyword to compare names with
       */
      bool operator==(const PvlKeyword &key) const {
        if(!m_name && !key.m_name) return true;
        if(!m_name || !key.m_name) return false;

        return (stringEqual(m_name, key.m_name));
      };

      /**
       * Returns true of the keyword names do not match
       *
       * @param key The keyword to compare names with
       */
      bool operator!=(const PvlKeyword &key) const {
        return !(*this == key);
      };

      bool isEquivalent(QString string1, int index = 0) const;

      /**
       *  The width of the longest keyword name (for formatting)
       *
       *  @param width the new width
       */
      void setWidth(int width) {
        m_width = width;
      };

      /**
       * Sets the indent level when outputted(for formatting)
       *
       * @param indent The new indent
       */
      void setIndent(int indent) {
        m_indent = indent;
      };

      //! Returns the current set longest keyword name
      int width() const {
        return m_width;
      };

      //! Returns the current indent level
      int indent() const {
        return m_indent;
      };

      PvlKeyword &operator=(Isis::PvlSequence &seq);

      void setFormat(PvlFormat *formatter);
      PvlFormat *format();

      static bool stringEqual(const QString &string1,
                              const QString &string2);


      static QString readLine(std::istream &is, bool insideComment);

      static bool readCleanKeyword(QString keyword,
                                   std::vector< QString > &keywordComments,
                                   QString &keywordName,
                                   std::vector< std::pair<QString, QString> >
                                   &keywordValues);

      static QString readValue(QString &keyword, bool &quoteProblem);
      static QString readValue(QString &keyword, bool &quoteProblem,
                                   const std::vector< std::pair<char, char> > &
                                   otherDelimiters);

      const PvlKeyword &operator=(const PvlKeyword &other);

      //! Validate Keyword for type and required values
      void validateKeyword(PvlKeyword & pvlKwrd, QString psValueType="", PvlKeyword* pvlKwrdRange=NULL);

    protected:
      QString reform(const QString &value) const;
      QString toPvl(const QString &value) const;
      QString toIPvl(const QString &value) const;
      std::ostream &writeWithWrap(std::ostream &os,
                                  const QString &textToWrite,
                                  int startColumn,
                                  PvlFormat &format) const;

      //! Formatter object
      PvlFormat *m_formatter;

    private:
      //! The keyword's name... This is a c-string for memory efficiency
      char * m_name;

      /**
       * The values in the keyword. This is a QVarLengthArray purely for
       *   optimization purposes. The amount of memory consumed by other data
       *   types introduces very significant overhead relative to this type
       *   which is meant to be as cost-effective and cheap as possible. Most
       *   of the time we have one value per keyword so that is what we're
       *   allocating by default with this variable.
       */
      QVarLengthArray<QString, 1> m_values;

      //! The units for the values.
      std::vector<QString> *m_units;

      //! The comments for the keyword.
      std::vector<QString> *m_comments;

      void init();

      void writeSpaces(std::ostream &, int) const;

      /**
       * The width of the longest keyword. This is used for spacing out the
       * equals signs on output.
       */
      int m_width;
      /**
       * The number of indentations to make. This is based on whether the
       * keyword is in a group, etc.
       */
      int m_indent;
  };
};

#endif

