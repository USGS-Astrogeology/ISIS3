/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <sstream>
#include <iomanip>

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
  PvlFormatPds::PvlFormatPds(const std::string &file) : PvlFormat(file) {
    init();
  }


  /*
  * Constructs a PvlFormatPds using the specified pre populated map of keyword name
  * (std::string) vs keyword type (KeywordType).
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
  std::string PvlFormatPds::formatValue(const PvlKeyword &keyword, int num) {

    std::string name = keyword.name();
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    if(name == "OBJECT" || (name == "GROUP")) {
      std::string val = (std::string)keyword;
      std::transform(val.begin(), val.end(), val.begin(), ::toupper);
      return val;
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
  std::string PvlFormatPds::formatString(const PvlKeyword &keyword, int num) {

    std::string val;
    bool singleUnit = false;

    // Create a Null value if the value index is greater than the number of values
    if((num >= keyword.size()) || (keyword[num].size() == 0)) {
      return "NULL";
    }

    // Handle PDS special values "N/A" "NULL" "UNK"
    std::string tmp = Isis::toString(keyword[num][0]);
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
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
      std::string unit = keyword.unit(num);
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
      std::string unit = keyword.unit(num);
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
  std::string PvlFormatPds::formatReal(const PvlKeyword &keyword, int num,
                                   int places) {

    std::string val;
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
    std::string tmp = keyword[num];
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
    if((tmp == "N/A") || (tmp == "NULL") || (tmp == "UNK")) {
      val += "\"" + tmp + "\"";
    }
    else if(places >= 0) {
      stringstream out;
      out << setiosflags(ios::fixed) << setprecision(places) << Isis::toDouble(keyword[num]);
      val += out.str().c_str();
    }
    else {
      val += keyword[num];
    }

    // Add the units to this value
    if((!singleUnit) && (keyword.unit(num).size() > 0)) {
      std::string unit = keyword.unit(num);
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
      std::string unit = keyword.unit(num);
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
  std::string PvlFormatPds::formatEnum(const PvlKeyword &keyword, int num) {

    std::string val;
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
    std::string tmp = keyword[num];
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
    if((tmp == "N/A") || (tmp == "NULL") || (tmp == "UNK")) {
      val += "\"" + tmp + "\"";
    }
    else {
      val += keyword[num];
    }

    // Add the units to this value
    if((!singleUnit) && (keyword.unit(num).size() > 0)) {
      std::string unit = keyword.unit(num);
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
      std::string unit = keyword.unit(num);
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
  std::string PvlFormatPds::formatUnknown(const PvlKeyword &keyword, int num) {

    std::string val;
    val.clear();
    bool singleUnit = false;

    // Create a Null value if the value index is greater than the number of values
    if((num >= keyword.size()) || (keyword[num].size() == 0)) {
      return "NULL";
    }

    // Handle PDS special values "N/A" "NULL" "UNK"
    std::string tmp = keyword[num];
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
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
      std::string unit = keyword.unit(num);
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
      std::string unit = keyword.unit(num);
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
  std::string PvlFormatPds::formatInteger(const PvlKeyword &keyword, int num, int bytes) {

    std::string val;
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
    std::string tmp = keyword[num];
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
    if((tmp == "N/A") || (tmp == "NULL") || (tmp == "UNK")) {
      val += "\"" + tmp + "\"";
    }
    else {
      val += keyword[num];
    }

    // Add the units to this value
    if((!singleUnit) && (keyword.unit(num).size() > 0)) {
      std::string unit = keyword.unit(num);
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
      std::string unit = keyword.unit(num);
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
  std::string PvlFormatPds::formatBinary(const PvlKeyword &keyword, int num, int bits) {

    std::string val;
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
    std::string tmp = keyword[num];
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
    if((tmp == "N/A") || (tmp == "NULL") || (tmp == "UNK")) {
      val += "\"" + tmp + "\"";
    }
    else {
      tmp.clear();
      BigInt value = Isis::toBigInt(keyword[num]);
      string binDig = "01";
      do {
        tmp = binDig[value % 2] + tmp;
        value /= 2;
      }
      while(value);
      size_t len = bits;
      std::string paddedString = std::string(len - std::min(len, tmp.length()), '0') + tmp;
      val += "2#" + paddedString + "#";
    }

    // Add the units to this value
    if((!singleUnit) && (keyword.unit(num).size() > 0)) {
      std::string unit = keyword.unit(num);
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
      std::string unit = keyword.unit(num);
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
  std::string PvlFormatPds::formatHex(const PvlKeyword &keyword, int num, int bytes) {

    std::string val;
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
    std::string tmp = keyword[num];
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
    if((tmp == "N/A") || (tmp == "NULL") || (tmp == "UNK")) {
      val += "\"" + tmp + "\"";
    }
    else {
      stringstream ss;
      if(bytes == 2) {
        ss << hex << (unsigned short) std::stoul(keyword[num]);
      }
      else if(bytes == 4) {
        ss << hex << (unsigned int)std::stoul(keyword[num]);
      }
      else {
        ss << hex << std::stoll(keyword[num]);
      }
      std::string h = ss.str().c_str();
      std::transform(h.begin(), h.end(), h.begin(), ::toupper);

      val += "16#" + h + "#";
    }

    // Add the units to this value
    if((!singleUnit) && (keyword.unit(num).size() > 0)) {
      std::string unit = keyword.unit(num);
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
      std::string unit = keyword.unit(num);
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
  std::string PvlFormatPds::formatBool(const PvlKeyword &keyword, int num) {

    std::string val;
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
    std::string tmp = keyword[num];
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
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
  std::string PvlFormatPds::formatName(const PvlKeyword &keyword) {
    std::string text = keyword.name();
    std::transform(text.begin(), text.end(), text.begin(), ::toupper);
    return text;
  }


  /*
  * Format the end of a group or object
  *
  * @param name A string representing the end text.
  * @param keyword The keyword (i.e., the Object or Group) that is ending
  */
  std::string PvlFormatPds::formatEnd(const std::string name,
                                      const PvlKeyword &keyword) {
    std::string left = name;
    std::transform(left.begin(), left.end(), left.begin(), ::toupper);
    left += " = ";
    std::string right = (std::string)keyword;
    std::transform(right.begin(), right.end(), right.begin(), ::toupper);
    return left + right;
  }


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
  std::string PvlFormatPds::addQuotes(const std::string value) {

    std::string val = value;

    bool quoteValue = true;
    bool singleQuoteValue = false;
    if (val.find(" ") != std::string::npos) {
      if(val.find("\"") != std::string::npos) {
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

