/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "IException.h"
#include "IString.h"
#include "Message.h"
#include "FileName.h"
#include "PvlKeyword.h"
#include "Pvl.h"
#include "PvlFormat.h"

using namespace std;

namespace Isis {

  /*
  * Constructs an empty PvlFormat
  */
  PvlFormat::PvlFormat() {
    init();
  }


  /*
  * Constructs a PvlFormat using the file name to ingest as the keyword to type
  * mapping. This is provided as a convience for child objects. The map is not
  * used for output of PvlKeywords in Normal Isis format.
  *
  * @param file A file name with keyword=type. Where KEYWORD is the name of a
  * keyword in this PvlKeyword and TYPE is one of [QString | integer | float ]
  */
  PvlFormat::PvlFormat(const QString &file) {
    init();
    add(file);
  }


  /*
  * Constructs a PvlFormat using the specified pre populated Pvl map of keyword
  * name (QString) vs keyword type (KeywordType).
  *
  * @param keywordType A Pvl with keyword=type. Where keyword is the name of a
  * keyword in a PvlKeyword and type is one of [QString | integer | float ]
  */
  PvlFormat::PvlFormat(Pvl &keywordType) {
    init();
    add(keywordType);
  }


  //! Clears all PvlFormat data.
  void PvlFormat::init() {
    m_keywordMap.clear();
    m_keywordMapFile.clear();
    m_charLimit = 80;
  }


  /*
  * Add the contents of a file to the keyword type mapping. The file should
  * contain KEYWORD=TYPE (one per line), where TYPE is one of the QStrings
  * KeywordType can convert.
  */
  void PvlFormat::add(const QString &file) {
    m_keywordMapFile = file;

    // Open the file and internalize it into the Pvl map
    try {
      Pvl pvl(file);
      add(pvl);
    }
    catch(IException &e) {
      QString msg;
      msg += "Unable to open or read keyword to type mapping file [";
      msg += file + "]";
      throw IException(e, IException::Programmer, msg, _FILEINFO_);
    }
  }


  /*
  * Add the contents of a Pvl to the keyword type mapping. The pvl should
  * contain KEYWORD=TYPE, where TYPE is one of the QStrings KeywordType can
  * convert.
  */
  void PvlFormat::add(Pvl &pvl) {
    for(int i = 0; i < pvl.keywords(); ++i) {
      PvlKeyword &key = pvl[i];
      QString name = key.name().toUpper();
      QString type = key[0].toUpper();
      PvlKeyword newKey(name, type);
      for(int j = 1; j < key.size(); ++j) newKey.addValue(key[j]);
      // Make sure we don't duplicate Keys
      if (m_keywordMap.hasKeyword(name)) {
        m_keywordMap.deleteKeyword(name);
      }
      m_keywordMap.addKeyword(newKey);
    }
  }


  /*
  * Returns the type of the keyword from the supplied map if any
  *
  * @param keyword The PvlKeyword to have its type returned
  */
  KeywordType PvlFormat::type(const PvlKeyword &keyword) {
    QString name = keyword.name().toUpper();
    if(m_keywordMap.hasKeyword(name)) {
      PvlKeyword &key = m_keywordMap.findKeyword(name);
      return toKeywordType(key[0]);
    }
    return NoTypeKeyword;
  }


  /*
  * Returns the number of digits of accuracy (right of decimal place) this
  * keyword should be output with
  *
  * @param keyword The PvlKeyword the accuracy is need for
  * @return The number of decimal places to be output. If this number is not
  *         available in keyword map return -1.
  */
  int PvlFormat::accuracy(const PvlKeyword &keyword) {
    QString name = keyword.name().toUpper();
    if(m_keywordMap.hasKeyword(name)) {
      PvlKeyword &key = m_keywordMap.findKeyword(name);
      if(key.size() > 1) {
        return toInt(key[1]);
      }
    }
    return -1;
  }


  /*
  * Returns the keyword name and value(s) formatted in "Normal" Isis format
  *
  * @param keyword The PvlKeyword to be formatted
  * @param num Use the ith value of the keyword
  */
  QString PvlFormat::formatValue(const PvlKeyword &keyword, int num) {

    QString val;
    val.clear();

    // Find out if the units are the same for all values
    bool singleUnit = isSingleUnit(keyword);

    // Create a Null value if the value index is greater than the number of values
    if(num >= keyword.size()) {
      return "Null";
    }

    // Create a Null value if the requested index is an empty QString
    if(keyword[num].size() == 0) {
      val += "Null";
    }
    else {
      val += keyword[num];
    }

    val = addQuotes(val);

    // If it is an array start it off with a paren
    if((keyword.size() > 1) && (num == 0)) {
      val = "(" + val;
    }

    // Add the units to this value
    if((!singleUnit) && (keyword.unit(num).size() > 0)) {
      val += " <" + keyword.unit(num) + ">";
    }

    // Add a comma for arrays
    if(num != keyword.size() - 1) {
      val += ", ";
    }
    // If it is an array close it off
    else if(keyword.size() > 1) {
      val += ")";
    }

    // Add the units to the end if all values have the same units
    if((singleUnit) && (num == keyword.size() - 1) &&
        (keyword.unit(num).size() > 0)) {
      val += " <" + keyword.unit(num) + ">";
    }

    return val;
  }


  /*
  * Format the name of the container
  *
  * @param keyword The PvlContainer being closed.
  */
  QString PvlFormat::formatName(const PvlKeyword &keyword) {
    return keyword.name();
  }


  /*
  * Format the end of a container
  *
  * @param name The text used to signify the end of a container
  * @param keyword The PvlContainer being closed.
  */
  QString PvlFormat::formatEnd(const QString name,
                                   const PvlKeyword &keyword) {
    return "End_" + formatName(keyword);
  }


  /*
  * Add single or double quotes around a value if necessary. The Isis definition
  * of when quotes are necessary is used.
  *
  * @param value The PvlKeyword value to be quoted if necessary.
  */
  QString PvlFormat::addQuotes(const QString value) {
    QString val = value;

    bool needQuotes = false;

    // find out if we need quotes and what kind of quotes might already exist
    char existingQuoteType = '\0';
    for (int pos = 0; !needQuotes && pos < val.size(); pos++) {
      // check for values indicating we need quotes, if we have a sequence
      //   it should already be properly quoted...
      if (pos == 0) {
        if (val[pos] == '(' || val[pos] == '{') {
          //  Find closing
          int closePos = -1;
          if (val[pos] == '(') {
            closePos = val.indexOf(')');
          }
          if (val[pos] == '{') {
            closePos = val.indexOf('}');
          }

          // If no closing paren or brace or If closing paren/brace not at end of value
          if (closePos == -1 || closePos != val.size() - 1) {
            needQuotes = true;
          }
          else {
            break;
          }
        }
      }

      if (val[pos] == ' ' || val[pos] == '(' ||
          val[pos] == '(' || val[pos] == ')' ||
          val[pos] == '{' || val[pos] == '}' ||
          val[pos] == ',' || val[pos] == '=') {
        needQuotes = true;
      }

      if (pos == val.size() - 1 && val[pos] == '-') {
        needQuotes = true;
      }

      // remember if we are a quote, what quote type we are
      if (existingQuoteType == '\0') {
        if (val[pos] == '"') {
          existingQuoteType = '"';
        }
        else if (val[pos] == '\'') {
          existingQuoteType = '\'';
        }
      }
      else {
        // make sure we dont have mixing of our outside quote type
        if (val[pos] == '"' || val[pos] == '\'') {
          val[pos] = existingQuoteType;
        }
      }
    }

    // figure out what kind of quotes we want to add
    char quoteValue = '"';

    if(existingQuoteType == '"') {
      quoteValue = '\'';
    }

    if(needQuotes) {
      val = quoteValue + val + quoteValue;
    }

    return val;
  }


  /**
   * Returns true if the units are the same for all value in the keyword
   * otherwise it returns false
   *
   * @param keyword The PvlKeyword to be formatted
   */
  bool PvlFormat::isSingleUnit(const PvlKeyword &keyword) {

    // See if the units are all the same
    bool singleUnit = true;
    for(int i = 0; i < keyword.size(); i ++) {
      if(!keyword.stringEqual(keyword.unit(i), keyword.unit(0))) {
        singleUnit = false;
      }
    }

    return singleUnit;
  }
}

