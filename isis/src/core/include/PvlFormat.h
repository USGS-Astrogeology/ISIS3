#ifndef PvlFormat_h
#define PvlFormat_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <map>
#include <string>
#include <regex>

#include "Pvl.h"

#include "PvlKeyword.h"

namespace Isis {

  //! The different types of keywords that can be formatted
  enum KeywordType { NoTypeKeyword,
                     StringKeyword,
                     BoolKeyword,
                     IntegerKeyword,
                     RealKeyword,
                     OctalKeyword,
                     HexKeyword,
                     BinaryKeyword,
                     EnumKeyword
                   };

  /**
  * Convert a string representing a type of keyword to the corresponding
  * enumeration. All white space, quotes, underscores, and dashes will be
  * removed from the input string.
  *
  * @param type The string to be converted.
  * @return The corresponding KeywordType enum.
  */
  inline KeywordType toKeywordType(const std::string type) {

    std::string t(type);
    std::regex e("[\\w_-\"'");
    t = std::regex_replace(t, e, "");

    std::transform(t.begin(), t.end(), t.begin(), ::toupper);

    if(t == "STRING") return StringKeyword;
    else if(t == "BOOL") return BoolKeyword;
    else if(t == "INTEGER") return IntegerKeyword;
    else if(t == "REAL") return RealKeyword;
    else if(t == "OCTAL") return OctalKeyword;
    else if(t == "HEX") return HexKeyword;
    else if(t == "BINARY") return BinaryKeyword;
    else if(t == "ENUM") return EnumKeyword;
    return NoTypeKeyword;
  }

  /**
   * @brief Formats a Pvl name value pair to Isis standards.
   *
   * This class is used to format a single PVL keyword-value pair using normal
   * Isis formatting. The class serves as a base class for others to override and
   * implement their own formatting.
   *
   * This class uses a Pvl or Pvl formatted file to populate its internal data
   * structure. This structure is used to lookup the type of a keyword and/or
   * other information for another Pvl{Object|Group|Keyword}. The format of the
   * file or Pvl is:
   *
   * @code
   * NAME=TYPE
   * NAME2=(TYPE,PLACES)
   * @endcode
   *
   * Example:
   * @code
   *   DESCRIPTION = STRING
   *   MinimumLatitude = (REAL,5)
   *   BITTYPE = ENUM
   *   RECORDS = INTEGER
   * @endcode
   *
   * Where NAME is the name of a keyword in the Pvl to be formatted. TYPE is the
   * type of keyword (STRING,BOOL,INTEGER,REAL,OCTAL,HEX,BINARY,ENUM) (see
   * ToKeywordType). PLACES is the number of digits to the right of the decimal
   * place for a keyword of TYPE REAL.
   *
   * NOTE: The capabilities to use this Pvl are not implemented in this base
   * class. They are provided as a convience for child classes only. This class
   * only implements the normal Isis foramatting, which is not dependent on the
   * type of the keyword. It is dependent on value of the keyword.
   *
   * @ingroup Parsing
   *
   * @author 2006-09-05 Stuart Sides
   *
   * @internal
   *  @history 2006-09-05 Stuart Sides - Original version
   *  @history 2008-09-30 Christopher Austin - added FormatEOL()
   *  @history 2009-12-17 Steven Lambright - {} are now treated the same as ()
   *  @history 2010-02-04 Travis Addair - Added SetCharLimit
   *           method allowing users to set the point at which a
   *           keyword value is output to the next line down.
   *  @history 2010-12-09 Steven Lambright - Values ending in '-' no longer fail
   *           to be quoted properly
   *  @history 2013-03-11 Steven Lambright and Mathew Eis - Brought method names and member variable
   *                          names up to the current Isis 3 coding standards. Fixes #1533.
   *  @history 2013-06-03 Tracie Sucharski and Jeannie Backer - Fixed the addQuotes method to
   *                          better handle multi-dimensional arrays vs equations with multiple
   *                          sets of parens.  Fixes #569.
   */
  class PvlFormat {

    public:

      PvlFormat();
      PvlFormat(const std::string &file);
      PvlFormat(Pvl &keymap);
      virtual ~PvlFormat() {};

      void add(const std::string &file);
      void add(Pvl &keymap);

      /**
       * Sets the maximum number of characters in a keyword value that
       * can be printed to a line before it wraps to the next line. By
       * default, the limit is set to 80 characters.
       *
       * @param limit The new character limit.
       */
      void setCharLimit(const unsigned int limit) {
        m_charLimit = limit;
      };

      /**
       * Retrieves the maximum number of characters in a keyword value
       * that can be printed to a line before it wraps to the next
       * line. By default, the limit is set to 80 characters.
       *
       * @return <B>unsigned int</B> Maximum number of characters.
       */
      unsigned int charLimit() const {
        return m_charLimit;
      };

      virtual std::string formatValue(const PvlKeyword &keyword,
                                      int valueIndex = 0);
      virtual std::string formatName(const PvlKeyword &keyword);
      virtual std::string formatEnd(const std::string name,
                                    const PvlKeyword &keyword);
      virtual std::string formatEOL() {
        return "\n";
      }

      virtual KeywordType type(const PvlKeyword &keyword);
      virtual int accuracy(const PvlKeyword &keyword);

    protected:

      virtual std::string addQuotes(const std::string value);
      bool isSingleUnit(const PvlKeyword &keyword);

      std::string m_keywordMapFile;
      Pvl m_keywordMap;

      //! Maximum number of characters on a single line of a keyword value.
      unsigned int m_charLimit;

    private:
      void init();
  };
};

#endif

