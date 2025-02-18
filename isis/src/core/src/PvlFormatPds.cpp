/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <sstream>
#include <iomanip>

#include <QDebug>

#include "IException.h"
#include "Message.h"
#include "IString.h"
#include "FileName.h"
#include "Constants.h"

#include "PvlFormatPds.h"

using namespace std;

namespace Isis {

  /*
  * Constructs an empty PvlFormatPds
  */
  PvlFormatPds::PvlFormatPds() {
    init();
  }


  /*
  * Constructs a PvlFormatPds using the file name to ingest to fill the keyword
  * to type map.
  *
  * @param file A file name with keyword=type. Where KEYWORD is the name of a
  * keyword and TYPE is one of [string | integer | float | ...]
  */
  PvlFormatPds::PvlFormatPds(const QString &file) : PvlFormat(file) {
    init();
  }


  /*
  * Constructs a PvlFormatPds using the specified pre populated map of keyword name
  * (QString) vs keyword type (KeywordType).
  *
  * @param keywordType A map with keyword, type. Where keyword is the name of a
  * keyword in a PvlKeyword and type is one of [string | integer | float ]
  */
  PvlFormatPds::PvlFormatPds(Pvl &keywordType) : PvlFormat(keywordType) {
    init();
  }


  //! Clears all PvlFormatPds specific data.
  void PvlFormatPds::init() {
  }


  /*
  * Returns the keyword value formatted in "PDS" format
  *
  * @param keyword The PvlKeyword to be formatted
  * @param num Use the ith value of the keyword
  */
  QString PvlFormatPds::formatValue(const PvlKeyword &keyword, int num) {

    QString name = keyword.name().toUpper();
    if(name == "OBJECT" || (name == "GROUP")) {
      QString val = (QString)keyword;
      return val.toUpper();
    }

    // Find out what type this keyword is
    KeywordType keyType = type(keyword);

    switch(keyType) {
      case StringKeyword:
        return formatString(keyword, num);
        break;

      case RealKeyword:
        return formatReal(keyword, num, accuracy(keyword));
        break;

      case IntegerKeyword:
        return formatInteger(keyword, num, accuracy(keyword));
        break;

      case HexKeyword:
        return formatHex(keyword, num, accuracy(keyword));
        break;

      case BinaryKeyword:
        return formatBinary(keyword, num, accuracy(keyword));
        break;

      case EnumKeyword:
        return formatEnum(keyword, num);
        break;

      case BoolKeyword:
        return formatBool(keyword, num);
        break;

      case NoTypeKeyword:
      default:
        return formatUnknown(keyword, num);
        break;
    }
    return formatUnknown(keyword, num);
  }


  /*
  * Returns the keyword value formatted as a "PDS" string
  *
  * @param keyword The PvlKeyword to be formatted
  * @param num Use the ith value of the keyword
  * @internal
  *   @history 2009-09-15 Jeannie Walldren - Moved the call to AddQuotes()
  *                                          inside the else-statement since
  *                                          the if portion of the code already
  *                                          adds quotes automatically
  */
  QString PvlFormatPds::formatString(const PvlKeyword &keyword, int num) {

    QString val;
    bool singleUnit = false;

    // Create a Null value if the value index is greater than the number of values
    if((num >= keyword.size()) || (keyword[num].size() == 0)) {
      return "NULL";
    }

    // Handle PDS special values "N/A" "NULL" "UNK"
    QString tmp = keyword[num][0].toUpper();
    if((tmp == "N/A") || (tmp == "NULL") || (tmp == "UNK")) {
      val += "\"" + tmp + "\"";
    }
    else {
      val += keyword[num];
      val = addQuotes(val);
    }


    // If it is an array start it off with a paren
    if((keyword.size() > 1) && (num == 0)) {
      val = "(" + val;
    }

    // Add the units to this value
    if((!singleUnit) && (keyword.unit(num).size() > 0)) {
      QString unit = keyword.unit(num);
      // For now PDS units are case sensitive, so we should not UpCase them
      //      unit.UpCase();
      val += " <" + unit + ">";
    }

    // Add a comma for arrays
    if(num != keyword.size() - 1) {
      val += ", ";
    }
    // If it is an array, close it off
    else if(keyword.size() > 1) {
      val += ")";
    }

    // Add the units to the end if all values have the same units
    if((singleUnit) && (num == keyword.size() - 1) &&
        (keyword.unit(num).size() > 0)) {
      QString unit = keyword.unit(num);
      // For now PDS units are case sensitive, so we should not UpCase them
      //      unit.UpCase();
      val += " <" + unit + ">";
    }

    return val;
  }


  /*
  * Returns the keyword value formatted as a "PDS" real number
  *
  * @param keyword The PvlKeyword to be formatted
  * @param num Use the ith value of the keyword
  */
  QString PvlFormatPds::formatReal(const PvlKeyword &keyword, int num,
                                   int places) {

    QString val;
    val.clear();
    bool singleUnit = false;

    // Create a Null value if the value index is greater than the number of values
    if((num >= keyword.size()) || (keyword[num].size() == 0)) {
      return "NULL";
    }

    // If it is an array start it off with a paren
    if((keyword.size() > 1) && (num == 0)) {
      val += "(";
    }

    // Handle PDS special values "N/A" "NULL" "UNK"
    QString tmp = keyword[num].toUpper();
    if((tmp == "N/A") || (tmp == "NULL") || (tmp == "UNK")) {
      val += "\"" + tmp + "\"";
    }
    else if(places >= 0) {
      stringstream out;
      out << setiosflags(ios::fixed) << setprecision(places) << toDouble((QString)keyword[num]);
      val += out.str().c_str();
    }
    else {
      val += keyword[num];
    }

    // Add the units to this value
    if((!singleUnit) && (keyword.unit(num).size() > 0)) {
      QString unit = keyword.unit(num);
      // unit.UpCase();
      val += " <" + unit + ">";
    }

    // Add a comma for arrays
    if(num != keyword.size() - 1) {
      val += ", ";
    }
    // If it is an array, close it off
    else if(keyword.size() > 1) {
      val += ")";
    }

    // Add the units to the end if all values have the same units
    if((singleUnit) && (num == keyword.size() - 1) &&
        (keyword.unit(num).size() > 0)) {
      QString unit = keyword.unit(num);
      // unit.UpCase();
      val += " <" + unit + ">";
    }

    return val;
  }


  /*
  * Returns the keyword value formatted as a "PDS" enumeration
  *
  * @param keyword The PvlKeyword to be formatted
  * @param num Use the ith value of the keyword
  */
  QString PvlFormatPds::formatEnum(const PvlKeyword &keyword, int num) {

    QString val;
    val.clear();
    bool singleUnit = false;

    // Create a Null value if the value index is greater than the number of values
    if((num >= keyword.size()) || (keyword[num].size() == 0)) {
      return "NULL";
    }

    // If it is an array start it off with a paren
    if((keyword.size() > 1) && (num == 0)) {
      val += "(";
    }

    // Handle PDS special values "N/A" "NULL" "UNK"
    QString tmp = keyword[num].toUpper();
    if((tmp == "N/A") || (tmp == "NULL") || (tmp == "UNK")) {
      val += "\"" + tmp + "\"";
    }
    else {
      val += keyword[num];
    }

    // Add the units to this value
    if((!singleUnit) && (keyword.unit(num).size() > 0)) {
      QString unit = keyword.unit(num);
      // unit.UpCase();
      val += " <" + unit + ">";
    }

    // Add a comma for arrays
    if(num != keyword.size() - 1) {
      val += ", ";
    }
    // If it is an array, close it off
    else if(keyword.size() > 1) {
      val += ")";
    }

    // Add the units to the end if all values have the same units
    if((singleUnit) && (num == keyword.size() - 1) &&
        (keyword.unit(num).size() > 0)) {
      QString unit = keyword.unit(num);
      // unit.UpCase();
      val += " <" + unit + ">";
    }

    return val;
  }


  /*
  * Returns the keyword value formatted without any knowledge of its type
  *
  * @param keyword The PvlKeyword to be formatted
  * @param num Use the ith value of the keyword
  * @internal
  *   @history 2009-09-15 Jeannie Walldren - Moved the call to AddQuotes()
  *                                          inside the else-statement since
  *                                          the if portion of the code already
  *                                          adds quotes automatically
  */
  QString PvlFormatPds::formatUnknown(const PvlKeyword &keyword, int num) {

    QString val;
    val.clear();
    bool singleUnit = false;

    // Create a Null value if the value index is greater than the number of values
    if((num >= keyword.size()) || (keyword[num].size() == 0)) {
      return "NULL";
    }

    // Handle PDS special values "N/A" "NULL" "UNK"
    QString tmp = keyword[num].toUpper();
    if((tmp == "N/A") || (tmp == "NULL") || (tmp == "UNK")) {
      val += "\"" + tmp + "\"";
    }
    else {
      val += keyword[num];
      val = PvlFormat::addQuotes(val);
    }


    // If it is an array start it off with a paren
    if((keyword.size() > 1) && (num == 0)) {
      val = "(" + val;
    }

    // Add the units to this value
    if((!singleUnit) && (keyword.unit(num).size() > 0)) {
      QString unit = keyword.unit(num);
      // unit.UpCase();
      val += " <" + unit + ">";
    }

    // Add a comma for arrays
    if(num != keyword.size() - 1) {
      val += ", ";
    }
    // If it is an array, close it off
    else if(keyword.size() > 1) {
      val += ")";
    }

    // Add the units to the end if all values have the same units
    if((singleUnit) && (num == keyword.size() - 1) &&
        (keyword.unit(num).size() > 0)) {
      QString unit = keyword.unit(num);
      // unit.UpCase();
      val += " <" + unit + ">";
    }

    return val;
  }


  /*
  * Returns the keyword value formatted as an integer
  *
  * @param keyword The PvlKeyword to be formatted
  * @param num Use the ith value of the keyword
  */
  QString PvlFormatPds::formatInteger(const PvlKeyword &keyword, int num, int bytes) {

    QString val;
    val.clear();
    bool singleUnit = false;

    // Create a Null value if the value index is greater than the number of values
    if((num >= keyword.size()) || (keyword[num].size() == 0)) {
      return "NULL";
    }

    // If it is an array start it off with a paren
    if((keyword.size() > 1) && (num == 0)) {
      val += "(";
    }

    // Handle PDS special values "N/A" "NULL" "UNK"
    QString tmp = keyword[num].toUpper();
    if((tmp == "N/A") || (tmp == "NULL") || (tmp == "UNK")) {
      val += "\"" + tmp + "\"";
    }
    else {
      val += keyword[num];
    }

    // Add the units to this value
    if((!singleUnit) && (keyword.unit(num).size() > 0)) {
      QString unit = keyword.unit(num);
      // unit.UpCase();
      val += " <" + unit + ">";
    }

    // Add a comma for arrays
    if(num != keyword.size() - 1) {
      val += ", ";
    }
    // If it is an array, close it off
    else if(keyword.size() > 1) {
      val += ")";
    }

    // Add the units to the end if all values have the same units
    if((singleUnit) && (num == keyword.size() - 1) &&
        (keyword.unit(num).size() > 0)) {
      QString unit = keyword.unit(num);
      // unit.UpCase();
      val += " <" + unit + ">";
    }

    return val;
  }


  /*
  * Returns the keyword value formatted as a binary value
  *
  * @param keyword The PvlKeyword to be formatted
  * @param num Use the ith value of the keyword
  */
  QString PvlFormatPds::formatBinary(const PvlKeyword &keyword, int num, int bits) {

    QString val;
    val.clear();
    bool singleUnit = false;

    // Create a Null value if the value index is greater than the number of values
    stringstream ss;
    if((num >= keyword.size()) || (keyword[num].size() == 0)) {
      return "NULL";
    }

    // If it is an array start it off with a paren
    if((keyword.size() > 1) && (num == 0)) {
      val += "(";
    }

    // Handle PDS special values "N/A" "NULL" "UNK"
    QString tmp = keyword[num].toUpper();
    if((tmp == "N/A") || (tmp == "NULL") || (tmp == "UNK")) {
      val += "\"" + tmp + "\"";
    }
    else {
      tmp.clear();
      BigInt value = toBigInt((QString)keyword[num]);
      string binDig = "01";
      do {
        tmp = binDig[value % 2] + tmp;
        value /= 2;
      }
      while(value);

      ss << right << setfill('0') << setw(bits) << tmp;
      tmp = ss.str().c_str();
      val += "2#" + tmp + "#";
    }

    // Add the units to this value
    if((!singleUnit) && (keyword.unit(num).size() > 0)) {
      QString unit = keyword.unit(num);
      // unit.UpCase();
      val += " <" + unit + ">";
    }

    // Add a comma for arrays
    if(num != keyword.size() - 1) {
      val += ", ";
    }
    // If it is an array, close it off
    else if(keyword.size() > 1) {
      val += ")";
    }

    // Add the units to the end if all values have the same units
    if((singleUnit) && (num == keyword.size() - 1) &&
        (keyword.unit(num).size() > 0)) {
      QString unit = keyword.unit(num);
      // unit.UpCase();
      val += " <" + unit + ">";
    }

    return val;
  }


  /*
  * Returns the keyword value formatted as a hexidecimal value
  *
  * @param keyword The PvlKeyword to be formatted
  * @param num Use the ith value of the keyword
  */


  QString PvlFormatPds::formatHex(const PvlKeyword &keyword, int num, int bytes) {

    QString val;
    val.clear();
    bool singleUnit = false;

    // Create a Null value if the value index is greater than the number of values
    if((num >= keyword.size()) || (keyword[num].size() == 0)) {
      return "NULL";
    }

    // If it is an array start it off with a paren
    if((keyword.size() > 1) && (num == 0)) {
      val += "(";
    }

    // Handle PDS special values "N/A" "NULL" "UNK"
    QString tmp = keyword[num].toUpper();
    if((tmp == "N/A") || (tmp == "NULL") || (tmp == "UNK")) {
      val += "\"" + tmp + "\"";
    }
    else {
      stringstream ss;
      if(bytes == 2) {
        ss << hex << (unsigned short int)toInt((QString)keyword[num]);
      }
      else if(bytes == 4) {
        ss << hex << (unsigned int)toInt((QString)keyword[num]);
      }
      else {
        ss << hex << toBigInt((QString)keyword[num]);
      }
      QString h = ss.str().c_str();
      h = h.toUpper();
      val += "16#" + h + "#";
    }

    // Add the units to this value
    if((!singleUnit) && (keyword.unit(num).size() > 0)) {
      QString unit = keyword.unit(num);
      // unit.UpCase();
      val += " <" + unit + ">";
    }

    // Add a comma for arrays
    if(num != keyword.size() - 1) {
      val += ", ";
    }
    // If it is an array, close it off
    else if(keyword.size() > 1) {
      val += ")";
    }

    // Add the units to the end if all values have the same units
    if((singleUnit) && (num == keyword.size() - 1) &&
        (keyword.unit(num).size() > 0)) {
      QString unit = keyword.unit(num);
      // unit.UpCase();
      val += " <" + unit + ">";
    }

    return val;
  }


  /*
  * Returns the keyword value formatted as a boolean value
  *
  * @param keyword The PvlKeyword to be formatted
  * @param num Use the ith value of the keyword
  */
  QString PvlFormatPds::formatBool(const PvlKeyword &keyword, int num) {

    QString val;
    val.clear();

    // Create a Null value if the value index is greater than the number of values
    if((num >= keyword.size()) || (keyword[num].size() == 0)) {
      return "NULL";
    }

    // If it is an array start it off with a paren
    if((keyword.size() > 1) && (num == 0)) {
      val += "(";
    }

    // Handle PDS special values "N/A" "NULL" "UNK"
    QString tmp = keyword[num];
    tmp = tmp.toUpper();
    if((tmp == "N/A") || (tmp == "NULL") || (tmp == "UNK")) {
      val += "\"" + tmp + "\"";
    }
    else {
      val += keyword[num];
    }

    // Add a comma for arrays
    if(num != keyword.size() - 1) {
      val += ", ";
    }
    // If it is an array, close it off
    else if(keyword.size() > 1) {
      val += ")";
    }

    return val;
  }




  /*
  * Format the name of this container
  *
  * @param keyword The keyword (i.e., the Object or Group)
  */
  QString PvlFormatPds::formatName(const PvlKeyword &keyword) {
    QString text = keyword.name();
    text = text.toUpper();
    return text;
  };


  /*
  * Format the end of a group or object
  *
  * @param name A string representing the end text.
  * @param keyword The keyword (i.e., the Object or Group) that is ending
  */
  QString PvlFormatPds::formatEnd(const QString name,
                                      const PvlKeyword &keyword) {
    QString left = name.toUpper();
    left += " = ";
    QString right = (QString)keyword;
    right = right.toUpper();
    return left + right;
  };


  /*
  * Put quotes around the value of a keyword of type string according to PDS
  * standards. All keywords identified as "string" are quoted for PDS labels.
  *
  * @param value The value of a PvlKeyword to be formatted.
  * @internal
  *   @history 2009-09-15 Jeannie Walldren - Added case to skip add quotes if
  *                                          the first character of the
  *                                          string is " or '
  */
  QString PvlFormatPds::addQuotes(const QString value) {

    QString val = value;

    bool quoteValue = true;
    bool singleQuoteValue = false;
    if(val.contains(" ")) {
      if(val.contains("\"")) {
        singleQuoteValue = true;
        quoteValue = false;
      }
    }

    // Turn the quoting back off if this value looks like a sequence
    // In this case the internal values should already be quoted.
    if(val[0] == '(') {
      singleQuoteValue = false;
      quoteValue = false;
    }
    else if(val[0] == '"') {
      singleQuoteValue = false;
      quoteValue = false;
    }
    else if(val[0] == '\'') {
      singleQuoteValue = false;
      quoteValue = false;
    }

    if(quoteValue) {
      val = "\"" + val + "\"";
    }
    else if(singleQuoteValue) {
      val = "'" + val + "'";
    }

    return val;
  }

}

