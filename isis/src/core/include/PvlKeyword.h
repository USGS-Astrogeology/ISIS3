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
      PvlKeyword(std::string name);
      PvlKeyword(std::string name, std::string value,
                 std::string unit = "");
      PvlKeyword(const PvlKeyword &other);
      ~PvlKeyword();

      void setName(std::string name);

      /**
       * Returns the keyword name.
       *
       * @return The name of the keyword.
       */
      std::string name() const {
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
      bool isNamed(std::string name) const {
        return stringEqual(name, this->name());
      };

      void setValue(std::string value, std::string unit = "");
      void setJsonValue(nlohmann::json jsonobj, std::string unit = "");

      void setUnits(std::string units);
      void setUnits(std::string value, std::string units);

      PvlKeyword &operator=(std::string value);

      void addValue(std::string value, std::string unit = "");
      void addJsonValue(nlohmann::json jsonobj, std::string unit = "");

      PvlKeyword &operator+=(std::string value);

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
        return std::stod(operator[](0));
      };
      //! Returns the first value  in this keyword converted to an integer
      operator int() const {
        return std::stoi(operator[](0));
      };
      //! Returns the first value  in this keyword converted to a BigInt
      operator Isis::BigInt() const {
        return std::stoll(operator[](0));
      };

      operator std::string() const;

      const std::string &operator[](int index) const;
      std::string &operator[](int index);
      std::string unit(const int index = 0) const;

      void addComment(std::string comment);
      void addCommentWrapped(std::string comment);

      void addComments(const std::vector<std::string> &comments);

      //! Returns the number of lines of comments associated with this keyword
      int comments() const {
        return (m_comments ? m_comments->size() : 0);
      };
      std::string comment(const int index) const;
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

      bool isEquivalent(std::string string1, int index = 0) const;

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

      static bool stringEqual(const std::string &string1,
                              const std::string &string2);


      static std::string readLine(std::istream &is, bool insideComment);

      static bool readCleanKeyword(std::string keyword,
                                   std::vector< std::string > &keywordComments,
                                   std::string &keywordName,
                                   std::vector< std::pair<std::string, std::string> >
                                   &keywordValues);

      static std::string readValue(std::string &keyword, bool &quoteProblem);
      static std::string readValue(std::string &keyword, bool &quoteProblem,
                                   const std::vector< std::pair<char, char> > &
                                   otherDelimiters);

      const PvlKeyword &operator=(const PvlKeyword &other);

      //! Validate Keyword for type and required values
      void validateKeyword(PvlKeyword & pvlKwrd, std::string psValueType="", PvlKeyword* pvlKwrdRange=NULL);

    protected:
      std::string reform(const std::string &value) const;
      std::string toPvl(const std::string &value) const;
      std::string toIPvl(const std::string &value) const;
      std::ostream &writeWithWrap(std::ostream &os,
                                  const std::string &textToWrite,
                                  int startColumn,
                                  PvlFormat &format) const;

      //! Formatter object
      PvlFormat *m_formatter;

    private:
      //! The keyword's name... This is a c-string for memory efficiency
      char * m_name;

      std::vector<std::string> m_values;

      //! The units for the values.
      std::vector<std::string> *m_units;

      //! The comments for the keyword.
      std::vector<std::string> *m_comments;

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

