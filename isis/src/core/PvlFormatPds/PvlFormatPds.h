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
      PvlFormatPds(const QString &file);
      PvlFormatPds(Pvl &keymap);
      virtual ~PvlFormatPds() {};

      virtual QString formatValue(const PvlKeyword &keyword,
                                      int valueIndex = 0);
      virtual QString formatName(const PvlKeyword &keyword);
      virtual QString formatEnd(const QString name,
                                    const PvlKeyword &keyword);
      virtual QString formatEOL() {
        return "\015\012";
      }

    protected:
      virtual QString addQuotes(const QString value);

      QString formatString(const PvlKeyword &keyword, int num);
      QString formatInteger(const PvlKeyword &keyword, int num, int bytes);
      QString formatReal(const PvlKeyword &keyword, int num, int precision);
      QString formatEnum(const PvlKeyword &keyword, int num);
      QString formatBinary(const PvlKeyword &keyword, int num, int bytes);
      QString formatHex(const PvlKeyword &keyword, int num, int bytes);
      QString formatBool(const PvlKeyword &keyword, int num);
      QString formatUnknown(const PvlKeyword &keyword, int num);

    private:
      void init();
  };
};

#endif

