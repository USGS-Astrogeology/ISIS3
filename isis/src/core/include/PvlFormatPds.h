#ifndef PvlFormatPds_h
#define PvlFormatPds_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <map>
#include <string>

#include "PvlFormat.h"

namespace Isis {
  /**
   * @brief Formats the value of a PvlKeyword into a PDS complient string
   *
   * This class is used to format a single PVL keyword-value pair. The supported
   * formats are Normal and PDS. The keyword to type mapping is not defined until
   * a Pvl or Pvl formatted file is supplied.
   *
   * @ingroup Parsing
   *
   * @author 2006-09-05 Stuart Sides
   *
   * @internal
   *  @history 2006-09-05 Stuart Sides - Original version
   *  @history 2006-12-14 Stuart Sides - Took out the upcaseing of units
   *  @history 2008-09-19 Kris Becker - Put quotes around "N/A", "NULL", "UNK";
   *                                    ensure units are placed after each element
   *                                    in array instead of one at the end and
   *                                    outside the closing right parenthesis.
   *                                    These changes bring us more in line with
   *                                    PDS compliancy.
   * @history 2008-09-30 Christopher Austin - added FormatEOL()
   * @history 2009-09-15 Jeannie Walldren - Fixed bug where code was adding 2
   *                                        sets of quotes to N/A when formatting
   *                                        value.  These changes were made in
   *                                        AddQuotes(), FormatString() and
   *                                        FormatUnknown() methods.
   *  @history 2013-03-11 Steven Lambright and Mathew Eis - Brought method names and member variable
   *                          names up to the current Isis 3 coding standards. Fixes #1533.
   */

  class PvlFormatPds : public PvlFormat {

    public:

      PvlFormatPds();
      PvlFormatPds(const std::string &file);
      PvlFormatPds(Pvl &keymap);
      virtual ~PvlFormatPds() {};

      virtual std::string formatValue(const PvlKeyword &keyword,
                                      int valueIndex = 0);
      virtual std::string formatName(const PvlKeyword &keyword);
      virtual std::string formatEnd(const std::string name,
                                    const PvlKeyword &keyword);
      virtual std::string formatEOL() {
        return "\015\012";
      }

    protected:
      virtual std::string addQuotes(const std::string value);

      std::string formatString(const PvlKeyword &keyword, int num);
      std::string formatInteger(const PvlKeyword &keyword, int num, int bytes);
      std::string formatReal(const PvlKeyword &keyword, int num, int precision);
      std::string formatEnum(const PvlKeyword &keyword, int num);
      std::string formatBinary(const PvlKeyword &keyword, int num, int bytes);
      std::string formatHex(const PvlKeyword &keyword, int num, int bytes);
      std::string formatBool(const PvlKeyword &keyword, int num);
      std::string formatUnknown(const PvlKeyword &keyword, int num);

    private:
      void init();
  };
};

#endif

