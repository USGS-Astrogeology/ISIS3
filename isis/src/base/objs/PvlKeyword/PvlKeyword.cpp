/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QDebug>
#include <QString>
#include <QRegularExpression>
#include "PvlKeyword.h"
#include "IException.h"
#include "Message.h"
#include "IString.h"
#include "PvlFormat.h"
#include "PvlSequence.h"

#include <sstream>

using namespace std;
using json = nlohmann::json;
namespace Isis {
  //! Constructs a blank PvlKeyword object.
  PvlKeyword::PvlKeyword() {
    init();
  }


  /**
   * Constructs a PvlKeyword object with a name.
   *
   * @param name The keyword name
   */
  PvlKeyword::PvlKeyword(QString name) {
    init();
    setName(name);
  }


  /**
   * Constructs a PvlKeyword object with a name, value and units.
   * Defaults to unit="".
   *
   * @param name The keyword name.
   * @param value The keyword values.
   * @param unit The units the values are given in.
   */
  PvlKeyword::PvlKeyword(QString name, QString value,
                         QString unit) {
    init();
    setName(name);
    addValue(value, unit);
  }


  //! Copy constructor
  PvlKeyword::PvlKeyword(const PvlKeyword &other) {
    init();
    *this = other;
  }


  /**
   * Destructs a PvlKeyword object.
   */
  PvlKeyword::~PvlKeyword() {
    if (m_units) {
      delete m_units;
      m_units = NULL;
    }

    if (m_comments) {
      delete m_comments;
      m_comments = NULL;
    }

    if (m_name) {
      delete [] m_name;
      m_name = NULL;
    }
  }


  //! Clears all PvlKeyword data.
  void PvlKeyword::init() {
    m_name = NULL;
    m_units = NULL;
    m_comments = NULL;
    m_width = 0;
    m_indent = 0;
    m_formatter = NULL;

    clear();
  }

  /**
   * Decides whether a value is null or not at a given index.
   * Defaults to index = 0.
   *
   * @param index The value index
   * @return <B>bool</B> True if the value is null, false if it's
   *         not.
   */
  bool PvlKeyword::isNull(int index) const {
    if (size() == 0) return true;
    if (index < 0 || index >= (int)m_values.size()) {
      QString msg = Message::ArraySubscriptNotInRange(index);
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    if (stringEqual("NULL", m_values[index])) return true;
    if (stringEqual("", m_values[index])) return true;
    if (stringEqual("\"\"", m_values[index])) return true;
    if (stringEqual("''", m_values[index])) return true;
    return false;
  }

  /**
   * Sets the keyword name.
   *
   * @param name The new keyword name.
   */
  void PvlKeyword::setName(QString name) {
    QString final = name.trimmed();
    if (final.contains(QRegExp("\\s"))) {
      QString msg = "[" + name + "] is invalid. Keyword name cannot ";
      msg += "contain whitespace.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if (m_name) {
      delete [] m_name;
      m_name = NULL;
    }

    if (final != "") {
      QByteArray finalAscii = final.toLatin1();
      m_name = new char[finalAscii.size() + 1];
      strncpy(m_name, finalAscii.data(), final.size() + 1);
    }
  }

  /**
   * Sets new values.
   *
   * If no current value exists, this method sets the given value
   * to the PvlKeyword.  Otherwise, it clears any existing values
   * and resets to the value given using addValue(). Defaults to
   * unit = "" (empty QString).
   *
   * @param value New value to be assigned.
   * @param unit Units of measurement corresponding to the value.
   *
   * @see addValue()
   * @see operator=
   * @see operator+=
   */
  void PvlKeyword::setValue(QString value, QString unit) {
    clear();
    addValue(value, unit);
  }

  /**
   * Sets new value from Json.
   *
   * If no current value exists, this method sets the given json value
   * to the PvlKeyword.  Otherwise, it clears any existing values
   * and resets to the value given using addJsonValue(). Defaults to
   * unit = "" (empty QString).
   *
   * @param jsonobj New jsobobj to be parsed and assigned.
   * @param unit Units of measurement corresponding to the value.
   *
   * @see addJsonValue()
   */
  void PvlKeyword::setJsonValue(json jsonobj, QString unit)
  {
    clear();
    addJsonValue(jsonobj, unit);
  }

  /**
   * Sets the unit of measure for all current values if any exist
   *
   * @param units New units to be assigned.
   */
  void PvlKeyword::setUnits(QString units) {
    if (!m_units) {
      m_units = new std::vector<QString>();
    }

    m_units->clear();

    for (int i = 0; i < m_values.size(); i++) {
      m_units->push_back(units);
    }
  }


  /**
   * Sets the unit of measure for a given value
   *
   * @param value The value to match
   * @param units New units to be assigned.
   *
   * @throws Isis::iException::Programmer - Given value must exist
   */
  void PvlKeyword::setUnits(QString value, QString units) {

    bool found = false;
    int i = -1;
    while(!found && ++i < (int) m_values.size()) {
      if (value == m_values[i]) {
        found = true;
      }
    }

    if (found) {
      if (!m_units) {
        m_units = new std::vector<QString>(m_values.size());
      }
      else {
        m_units->resize(m_values.size());
      }

      (*m_units)[i] = units;
    }
    else {
      IString msg = "PvlKeyword::setUnits called with value [" + value +
                    "] which does not exist in this Keyword";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }

  /**
   *
   * Sets new values.
   *
   * Overwrites the '=' operator to add a new value using addValue(). Like
   * setValue(), this method clears any previously existing values and resets to
   * the given value with unit = "" (empty QString).
   *
   * @param value The value to be added.
   * @return <B>PvlKeyword&</B> Reference to PvlKeyword object.
   *
   * @see addValue()
   * @see setValue()
   * @see operator+=
   */
  PvlKeyword &PvlKeyword::operator=(QString value) {
    clear();
    addValue(value);
    return *this;
  }

  /**
   * Adds a value with units.
   *
   * If no current value exists, this method sets the given value.
   * Otherwise, it retains any current values and adds the value
   * given to the array of values for this PvlKeyword object.
   * Defaults to unit = "" (empty QString).
   *
   * @param value New value to be assigned.
   * @param unit Units of measurement corresponding to the value.
   *
   * @see setValue()
   * @see operator=
   * @see operator+=
   */
  void PvlKeyword::addValue(QString value, QString unit) {
    m_values.append(value);

    if (unit != "") {
      if (!m_units) {
        m_units = new std::vector<QString>(m_values.size());
      }
      else {
        m_units->resize(m_values.size());
      }

      (*m_units)[m_units->size() - 1] = unit;
    }
    else if (m_units) {
      m_units->push_back("");
    }
  }

  /**
   * Adds a value with units.
   *
   * If no current value exists, this method sets the given json value.
   * Otherwise, it retains any current values and adds the json value
   * given to the array of values for this PvlKeyword object using addValue.
   * Defaults to unit = "" (empty QString).
   *
   * @param jsonobj New jsonobj to be parsed and assigned.
   * @param unit Units of measurement corresponding to the value.
   *
   * @see setJsonValue()
   * @see addValue()
   *
   * @throws Isis::iException::Unknown - jsonobj cannot be an array of values
   */
  void PvlKeyword::addJsonValue(json jsonobj, QString unit) {
    QString value;
    if (jsonobj.is_array()) {
      QString msg = "Unable to convert " + name() + " with nested json array value into PvlKeyword";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    else if (jsonobj.is_number())
    {
      value = QString::number(jsonobj.get<double>(), 'g', 16);
    }
    else if (jsonobj.is_boolean())
    {
      value = QString(jsonobj.get<bool>() ? "true" : "false");
    }
    else if (jsonobj.is_null())
    {
      value = QString("Null");
    }
    else
    {
      value = QString::fromStdString(jsonobj);
    }
    addValue(value, unit);
  }

  /**
   * Adds a value.
   *
   * Overwrites the '+=' operators to add a new value. Like
   * addValue(), this method keeps any previously existing values
   * and adds the new value with unit = "" (empty QString) to the
   * array of values for this PvlKeyword object.
   *
   * @param value The new value.
   * @return <B>PvlKeyword&</B> Reference to PvlKeyword object.
   *
   * @see addValue()
   * @see setValue()
   * @see operator=
   */
  PvlKeyword &PvlKeyword::operator+=(QString value) {
    addValue(value);
    return *this;
  }

  //! Clears all values and units for this PvlKeyword object.
  void PvlKeyword::clear() {
    m_values.clear();

    if (m_units) {
      delete m_units;
      m_units = NULL;
    }
  }


  PvlKeyword::operator QString() const {
    return operator[](0);
  }


  /**
   * Gets value for this object at specified index.
   *
   * Overrides the '[]' operator to return the element in the
   * array of values at the specified index.
   *
   * @param index The index of the value.
   * @return <B>IString</B> The value at the index.
   * @throws iException ArraySubscriptNotInRange (index) Index out of bounds.
   *
   * @see const operator[]
   */
  QString &PvlKeyword::operator[](int index) {
    if (index < 0 || index >= (int)m_values.size()) {
      QString msg = (Message::ArraySubscriptNotInRange(index)) +
                   "for Keyword [" + QString(m_name) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return m_values[index];
  }

  /**
   * Gets value for this object at specified index.
   *
   * Overrides the '[]' operator to return the element in the
   * array of values at the specified index.
   *
   * @param index The index of the value.
   * @return <b>IString</b> The value at the index.
   * @throws iException ArraySubscriptNotInRange (index) Index out of bounds.
   *
   * @see operator[]
   */
  const QString &PvlKeyword::operator[](int index) const {
    if (index < 0 || index >= (int)m_values.size()) {
      QString msg = Message::ArraySubscriptNotInRange(index);
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return m_values[index];
  }

  /**
   * Returns the units of measurement of the element of the array
   * of values for the object at the specified index. Defaults to
   * index = 0.
   *
   * @param index The index of the unit.
   * @return <B>QString</B> The unit at the index.
   * @throws iException ArraySubscriptNotInRange (index) Index out of bounds.
   */
  QString PvlKeyword::unit(int index) const {
    if (!m_units) return "";

    if (index < 0 || index >= (int)m_units->size()) {
      QString msg = Message::ArraySubscriptNotInRange(index);
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return (*m_units)[index];
  }

  /**
   * Add a comment to the PvlKeyword.
   *
   * @param comment The new comment.
   *
   * @see addCommentWrapped()
   * @see addComments()
   * @see clearComment()
   */
  void PvlKeyword::addComment(QString comment) {
    if (!m_comments) {
      m_comments = new std::vector<QString>();
    }

    if (comment.size() == 0) {
      m_comments->push_back("#");
    }
    if (comment[0] == '#') {
      m_comments->push_back(comment);
    }
    else if (comment.size() == 1) {
      m_comments->push_back("# " + comment);
    }
    else if ((comment[0] == '/') && (comment[1] == '*')) {
      m_comments->push_back(comment);
    }
    else if ((comment[0] == '/') && (comment[1] == '/')) {
      m_comments->push_back(comment);
    }
    else {
      m_comments->push_back("# " + comment);
    }
  }

  /**
   * Automatically wraps and adds long comments to the PvlKeyword
   *
   * @param comment The new comment to add
   *
   * @see addComment()
   * @see addComments()
   * @see clearComment()
   */
  void PvlKeyword::addCommentWrapped(QString comment) {
    IString cmt = comment;
    IString token = cmt.Token(" ");
    while(cmt != "") {
      IString temp = token;
      token = cmt.Token(" ");
      int length = temp.size() + token.size() + 1;
      while((length < 72) && (token.size() > 0)) {
        temp += " " + token;
        token = cmt.Token(" ");
        length = temp.size() + token.size() + 1;
      }
      addComment(temp.c_str());
    }
    if (token.size() != 0) addComment(token.c_str());
  }

  //! Clears the current comments.
  void PvlKeyword::clearComment() {
    if (m_comments) {
      delete m_comments;
      m_comments = NULL;
    }
  }

  /**
   * Return a comment at the specified index.
   * @param index The index of the comment.
   * @return <B>QString</B> The comment at the index.
   * @throws iException ArraySubscriptNotInRange (index) Index out of bounds.
   */
  QString PvlKeyword::comment(int index) const {
    if (!m_comments) return "";

    if (index < 0 || index >= (int)m_comments->size()) {
      QString msg = Message::ArraySubscriptNotInRange(index);
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return (*m_comments)[index];
  };

  /**
   * Checks if the value needs to be converted to PVL or iPVL and returns it in
   * the correct format.
   * @param value The value to be converted.
   * @return <B>QString</B> The value in its proper format (iPVL or
   *         PVL).
   */
  QString PvlKeyword::reform(const QString &value) const {
#if 0
    static bool firstTime = true;
    static bool iPVL = true;
    if (firstTime) {
      firstTime = false;
      Isis::PvlGroup &g = Isis::Preference::Preferences().findGroup(
                            "UserInterface", Isis::Pvl::Traverse);

      Isis::IString s = (QString) g["PvlFormat"];
      s.UpCase();
      if (s == "PVL") iPVL = false;
    }

    if (iPVL) return toIPvl(value);
#endif
    return toPvl(value);
  }

  /**
   * Converts a value to iPVL format.
   * @param value The value to be converted.
   * @return <B>QString</B> The value in iPVL format.
   */
  QString PvlKeyword::toIPvl(const QString &value) const {
    QString out;
    bool upcase = true;
    bool lastlower = true;
    for (int i = 0; i < value.size(); i++) {
      if ((lastlower) && (value[i].isUpper())) upcase = true;
      if (value[i] == '_') {
        upcase = true;
      }
      else if (upcase) {
        out += value[i].toUpper();
        lastlower = false;
        upcase = false;
      }
      else {
        out += value[i].toLower();
        if (value[i].isLower()) lastlower = true;
        upcase = false;
      }
    }
    return out;
  }

  /**
   * Converts a value to PVL format.
   * @param value The value to be converted.
   * @return <B>QString</B> The value in PVL format.
   */
  QString PvlKeyword::toPvl(const QString &value) const {
    QString out;
    bool lastlower = false;
    for (int i = 0; i < value.size(); i++) {
      if ((lastlower) && (value[i].isUpper())) out += "_";
      if (value[i] == '_') {
        out += "_";
        lastlower = false;
      }
      else {
        out += value[i].toUpper();
        if (value[i].isLower()) lastlower = true;
      }
    }
    return out;
  }

  /**
   * Checks to see if two QStrings are equal. Each is converted to uppercase
   * and removed of underscores and whitespaces.
   * @param QString1 The first QString
   * @param QString2 The second QString
   * @return <B>bool</B> True or false, depending on whether
   *          the QString values are equal.
   */
  bool PvlKeyword::stringEqual(const QString &QString1,
                               const QString &QString2) {
    Isis::IString s1(QString1);
    Isis::IString s2(QString2);

    s1.ConvertWhiteSpace();
    s2.ConvertWhiteSpace();

    s1.Remove(" _");
    s2.Remove(" _");

    s1.UpCase();
    s2.UpCase();

    if (s1 == s2) return true;
    return false;
  }

  /**
   * Checks to see if a value with a specified index is equivalent to another
   * QString.
   * @param QString1 The QString to compare the value to.
   * @param index The index of the existing value.
   * @return <B>bool</B> True if the two QStrings are equivalent,
   *         false if they're not.
   * @throws iException ArraySubscriptNotInRange (index) Index out of bounds.
   */
  bool PvlKeyword::isEquivalent(QString QString1, int index) const {
    if (index < 0 || index >= (int)m_values.size()) {
      QString msg = Message::ArraySubscriptNotInRange(index);
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return stringEqual(m_values[index], QString1);
  }

  /**
   * Add values and units from a PvlSequence. (Clears current values and units)
   * @param seq The PvlSequence to add from.
   * @return <B>PvlKeyword&</B> Reference to PvlKeyword object.
   */
  PvlKeyword &PvlKeyword::operator=(Isis::PvlSequence &seq) {
    clear();
    for (int i = 0; i < seq.Size(); i++) {
      QString temp = "(";
      for (int j = 0; j < (int)seq[i].size(); j++) {
        QString val = seq[i][j];
        if (val.contains(" ")) {
          temp += "\"" + val + "\"";
        }
        else {
          temp += val;
        }
        if (j < (int) seq[i].size() - 1) temp += ", ";
      }
      temp += ")";
      this->operator+=(temp);
    }

    return *this;
  }

  /**
   * Wraps output so that length doesn't exceed the character
   * limit.  By default, the character limit is set to 80, and can
   * be changed with the method SetCharLimit.  Used as a helper
   * method for output of PvlKeyword.
   *
   * @param os Designated output stream
   * @param textToWrite The text to be written
   * @param startColumn The starting column after the "=" sign.
   * @param endOfLine The EOL character
   *
   * @return <B>ostream&</B> Reference to ostream.
   * @see operator<<
   */
  ostream &PvlKeyword::writeWithWrap(std::ostream &os,
                                     const QString &textToWrite,
                                     int startColumn,
                                     PvlFormat &format) const {

    /*
    http://pds.jpl.nasa.gov/tools/standards-reference.shtml

    pds.jpl.nasa.gov/documents/sr/Chapter12.pdf

    Object Description Language Specification and Usage
    The following provides a complete specification for Object Description Language
    (ODL), the language used to encode data labels for the Planetary Data System
    (PDS) and other NASA data systems. This standard contains a formal definition of
    the grammar semantics of the language. PDS specific implementation notes and
    standards are referenced in separate sections.

    12.5.3.1 Implementation of String Values
    A text QString read in from a label is reassembled into a QString of characters.
    The way in which the QString is broken into lines in a label does not affect the
    format of the QString after it has been reassembled. The following rules are used
              when reading text QStrings: If a format effector or a sequence of
              format effectors is encountered within a text QString,
              the effector (or sequence of effectors) is replaced by a single space
              character, unless the last character is a hyphen (dash) character. Any
              spacing characters at the end of the line are removed and any spacing
              characters at the beginning of the following line are removed. This
              allows a text QString in a label to appear with the left and right
              margins set at arbitrary points without changing the QString value. For
                       example, the following two QStrings are the same: "To be or
                       not to be" and
                       "To be or
                       not to be"
              If the last character on a line prior to a format effector is a hyphen
              (dash) character, the hyphen is removed with any spacing characters at
              the beginning of the following line. This follows the standard
              convention in English of using a hyphen to break a word across lines.
              For example, the following two QStrings are the same:
                        "The planet Jupiter is very big" and
                       "The planet Jupi-
                       ter is very big"
              Control codes, other than the horizontal tabulation character and
              format effectors, appearing within a text QString are removed.
        */

    /*
      We will be adding a condition for human-readable purposes:
        If a quoted QString of text does not fit on the current line,
        but will fit on the next line, use the next line.
    */

    // Position set
    QString remainingText = textToWrite;
    int spaceForText = format.charLimit() - 1 - format.formatEOL().length() - startColumn;

    // indexOf quote positions to better determine which line to put the
    //  QString on. Data structure: vector< startPos, endPos > where
    //  remainingText[startPos] and remainingText[endPos] must both be quotes.
    vector< pair<int, int> > quotedAreas;
    int  quoteStart = -1;

    // if its an array, indent subsequent lines 1 more
    if (textToWrite.count() > 0 && (textToWrite[0] == '(' || textToWrite[0] == '"')) {
      startColumn ++;
    }

    /* Standard 12.3.3.1 ->
      A quoted text QString may not contain the quotation mark, which is reserved
      to be the text QString delimiter.

      So we don't have to worry about escaped quotes.
    */

    vector< pair<char, char> > quoteStartEnds;
    quoteStartEnds.push_back(pair<char, char>('"', '"'));
    quoteStartEnds.push_back(pair<char, char>('\'', '\''));
    quoteStartEnds.push_back(pair<char, char>('<', '>'));

    // clean up any EOL characters, they mustn't interfere, remove sections of
    //   multiple spaces (make them into one), and indexOf quoted areas
    for (int pos = 0; pos < remainingText.size(); pos++) {
      // remove \r and \n from QString
      if (remainingText[pos] == '\n' || remainingText[pos] == '\r') {
        if (pos != remainingText.size() - 1) {
          remainingText = remainingText.mid(0, pos) +
                          remainingText.mid(pos + 1);
        }
        else {
          remainingText = remainingText.mid(0, pos);
        }
      }

      // convert "      " to " " if not quoted
      if (quoteStart == -1) {
        while(pos > 0 &&
              remainingText[pos-1] == ' ' &&
              remainingText[pos] == ' ') {
          remainingText = remainingText.mid(0, pos) +
                          remainingText.mid(pos + 1);
        }
      }

      // Find quotes
      for (unsigned int i = 0;
          (quoteStart < 0) && i < quoteStartEnds.size();
          i++) {
        if (quoteStartEnds[i].first == remainingText[pos]) {
          quoteStart = pos;
        }
      }


      //bool mismatchQuote = false;

      // Check to see if we're ending a quote if we didn't just
      //   start the quote and we are inside a quote
      if (quoteStart != (int)pos && quoteStart != -1) {
        for (unsigned int i = 0; i < quoteStartEnds.size(); i++) {
          if (quoteStartEnds[i].second == remainingText[pos]) {
            if (quoteStartEnds[i].first != remainingText[quoteStart]) {
              continue;
              //  mismatchQuote = true;
            }

            quotedAreas.push_back(pair<int, int>(quoteStart, pos));

            quoteStart = -1;
          }
        }
      }

      //if (mismatchQuote) {
      //  IString msg = "Pvl keyword values [" + textToWrite +
      //    "] can not have embedded quotes";
      //  throw iException::Message(iException::Programmer, msg, _FILEINFO_);
      //}
    }

    int charsLeft = spaceForText;
    int printedSoFar = 0;

    // while we have something to write, keep going
    while(!remainingText.isEmpty()) {
      // search backwards for the last space or comma *in the limit* (80 chars)
      int lastSpacePosition = charsLeft;

      // if everything fits into our remaining space, consider the last
      //   spot in the QString to be printed still the split position.
      if (lastSpacePosition >= (int)remainingText.length()) {
        lastSpacePosition = remainingText.length();
      }
      else {
        // Everything does not fit; use good space for mediocre splits (inside
        //  quoted QStrings), and excellent space for good splits (between array
        //  values for example)
        int goodSpace = -1;
        int excellentSpace = -1;
        int searchPosition = lastSpacePosition;
        bool doneSearching = false;

        while(!doneSearching) {
          bool currentPosQuoted = false;

          for (unsigned int i = 0; i < quotedAreas.size(); i++) {
            if (searchPosition + printedSoFar >= quotedAreas[i].first &&
                searchPosition + printedSoFar <= quotedAreas[i].second) {
              currentPosQuoted = true;
            }
          }

          if (remainingText[searchPosition] == ' ') {
            bool validSpace = true;

            // this really isn't a good space if the previous character is a
            // '-' though - then it would be read wrong when re-imported.
            if (searchPosition > 0 && remainingText[searchPosition - 1] == '-') {
              validSpace = false;
            }

            if (validSpace && goodSpace < 0) {
              goodSpace = searchPosition;
            }

            // An excellent space is the prefential break - not quoted and
            //   not units next.
            // we were already done if we had an excellent space
            if (validSpace && !currentPosQuoted) {
              if ((int)searchPosition < (int)(remainingText.size() - 1) &&
                  remainingText[searchPosition+1] != '<') {
                excellentSpace = searchPosition;
              }
            }
          }

          doneSearching = (excellentSpace >= 0 || searchPosition <= 1);
          searchPosition --;
        }

        // Use the best breaking point we have
        if (excellentSpace > 0) {
          lastSpacePosition = excellentSpace;
        }
        else if (goodSpace > 0) {
          lastSpacePosition = goodSpace;
        }
        else {
          lastSpacePosition = -1;
        }
      }

      // we found a space or comma in our limit, write to that chatacter
      //   and repeat the loop
      if (lastSpacePosition >= 0) {
        os << remainingText.mid(0, lastSpacePosition);

        remainingText = remainingText.mid(lastSpacePosition);
        printedSoFar += lastSpacePosition;
      }
      // we failed to indexOf a space or a comma in our limit,
      //   use a hyphen (-)
      else {
        // Make sure we don't break on "//" since Isis thinks that is a comment
        if (remainingText.mid(charsLeft-1, 2) == "//") {
          os << remainingText.mid(0, charsLeft - 2);
          os << "-";
          remainingText = remainingText.mid(charsLeft - 2);
          printedSoFar += charsLeft - 2;
        }
        else {
          os << remainingText.mid(0, charsLeft - 1);
          os << "-";
          remainingText = remainingText.mid(charsLeft - 1);
          printedSoFar += charsLeft - 1;
        }
      }

      // we wrote as much as possible, do a newline and repeat
      if (!remainingText.isEmpty()) {
        os << format.formatEOL();
        writeSpaces(os, startColumn);

        // dont allow spaces to begin the next line inside what we're printing
        if (remainingText[0] == ' ') {
          remainingText = remainingText.mid(1);
          printedSoFar += 1;
        }
      }

      charsLeft = spaceForText;
    }

    return os;
  }


  /**
   * This writes numSpaces spaces to the ostream.
   *
   * @param os Stream to write to
   * @param numSpaces number of spaces to write
   */
  void PvlKeyword::writeSpaces(std::ostream &os, int numSpaces) const {
    for (int space = 0; space < numSpaces; space ++) {
      os << " ";
    }
  }


  /**
   *
   * Set the PvlFormatter used to format the keyword name and value(s)
   *
   * @param formatter A pointer to the formatter to be used
   */
  void PvlKeyword::setFormat(PvlFormat *formatter) {
    m_formatter = formatter;
  }


  /**
   *
   * Get the current PvlFormat or create one
   * @return <B>PvlFormat*</B> Pointer to PvlFormat.
   *
   */
  PvlFormat *PvlKeyword::format() {
    return m_formatter;
  };

  /**
   * Read in a keyword
   *
   * http://pds.jpl.nasa.gov/tools/standards-reference.shtml
   *
   * @param is The input stream
   * @param result The keyword to read into (OUTPUT)
   *
   */
  std::istream &operator>>(std::istream &is, PvlKeyword &result) {
    result = PvlKeyword();
    QString line;
    QString keywordString;

    bool keywordDone = false;
    bool multiLineComment = false;
    bool error = !is.good();

    while(!error && !keywordDone) {
      istream::pos_type beforeLine = is.tellg();

      line = PvlKeyword::readLine(is, multiLineComment);

      // We read an empty line (failed to read next non-empty line)
      // and didnt complete our keyword, essentially we hit the implicit
      // keyword named "End"
      if (line.isEmpty() && !is.good()) {
        if (keywordString.isEmpty() ||
            keywordString[keywordString.size()-1] == '\n') {
          line = "End";

          if (multiLineComment) {
            error = true;
          }
        }
        else {
          error = true;
        }
      }

      bool comment = false;

      if (!multiLineComment) {
        if (line.size() > 0 && line[0] == '#') {
          comment = true;
        }

        if (line.size() > 1 && line[0] == '/' &&
            (line[1] == '*' || line[1] == '/')) {
          comment = true;

          if (line[1] == '*') {
            multiLineComment = true;
            keywordString += line.mid(0, 2);
            line = line.mid(2).trimmed();
          }
        }
      }

      if (multiLineComment) {
        comment = true;

        if (line.contains("/*")) {
          IString msg = "Error when reading a pvl: Cannot have ['/*'] inside a "
                        "multi-line comment";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }

        if (line.contains("*/")) {
          multiLineComment = false;

          line = line.mid(0, line.indexOf("*/")).trimmed() + " */";
        }
      }

      if (line.isEmpty()) {
        continue;
      }
      // comment line
      else if (comment) {
        keywordString += line + '\n';
        continue;
      }
      // first line of keyword data
      else if (keywordString.isEmpty()) {
        keywordString = line;
      }
      // concatenation
      else if (!comment && keywordString[keywordString.size()-1] == '-') {
        keywordString = keywordString.mid(0, keywordString.size() - 1) + line;
      }
      // Non-commented and non-concatenation -> put in the space
      else {
        keywordString += " " + line;
      }
      // if this line concatenates with the next, read the next
      if (line[line.size()-1] == '-') {
        continue;
      }

      std::vector<QString> keywordComments;
      QString keywordName;
      std::vector< std::pair<QString, QString> > keywordValues;

      bool attemptedRead = false;

      try {
        attemptedRead = PvlKeyword::readCleanKeyword(keywordString,
                        keywordComments,
                        keywordName,
                        keywordValues);
      }
      catch (IException &e) {
        if (is.eof() && !is.bad()) {
          is.clear();
          is.unget();
        }

        is.seekg(beforeLine, ios::beg);

        QString msg = "Unable to read PVL keyword [";
        msg += keywordString;
        msg += "]";

        throw IException(e, IException::Unknown, msg, _FILEINFO_);
      }

      // Result valid?
      if (attemptedRead) {
        // if the next line starts with '<' then it should be read too...
        // it should be units
        // however, you can't have units if there is no value
        if (is.good() && is.peek() == '<' && !keywordValues.empty()) {
          continue;
        }

        result.setName(keywordName);
        result.addComments(keywordComments);

        for (unsigned int value = 0; value < keywordValues.size(); value++) {
          result.addValue(keywordValues[value].first,
                          keywordValues[value].second);
        }

        keywordDone = true;
      }

      if (!attemptedRead) {
        error = error || !is.good();
      }
      // else we need to keep reading
    }

    if (error) {
      // skip comments
      while(keywordString.contains('\n')) {
        keywordString = keywordString.mid(keywordString.indexOf('\n') + 1);
      }

      QString msg;

      if (keywordString.isEmpty() && !multiLineComment) {
        msg = "PVL input contains no Pvl Keywords";
      }
      else if (multiLineComment) {
        msg = "PVL input ends while still in a multi-line comment";
      }
      else {
        msg = "The PVL keyword [" + keywordString + "] does not appear to be";
        msg += " a valid Pvl Keyword";
      }

      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    if (!keywordDone) {
      // skip comments
      while(keywordString.contains('\n'))
        keywordString = keywordString.mid(keywordString.indexOf('\n') + 1);

      QString msg;

      if (keywordString.isEmpty()) {
        msg = "Error reading PVL keyword";
      }
      else {
        msg = "The PVL keyword [" + keywordString + "] does not appear to be";
        msg += " complete";
      }

      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    return is;
  }

  /**
   * This method adds multiple comments at once by calling AddComments on each
   * element in the vector.
   *
   * @param comments Comments to associate with this keyword
   */
  void PvlKeyword::addComments(const std::vector<QString> &comments) {
    for (unsigned int i = 0; i < comments.size(); i++) {
      addComment(comments[i]);
    }
  }

  /**
   * This reads a keyword compressed back to 1 line of data (excluding comments,
   * which are included on separate lines of data before the keyword). Line
   * concatenations must have already been handled. This returns the data of the
   * keyword (if valid) and its status.
   *
   * @param keyword Pvl "#COMMENT\n//COMMENT\nKeyword = (Value1,Value2,...)"
   *                QString
   * @param keywordComments Output: Lines of data that are comments
   * @param keywordName Output: Name of keyword
   * @param keywordValues Output: vector< pair<Value, Units> >
   *
   * @return bool false if it is invalid but could become valid given more data,
   *              true if it is a valid keyword and successful
   */
  bool PvlKeyword::readCleanKeyword(QString keyword,
                                    std::vector<QString> &keywordComments,
                                    QString &keywordName,
                                    std::vector< std::pair<QString, QString> > &keywordValues) {
    // Reset outputs
    keywordComments.clear();
    keywordName = "";
    keywordValues.clear();

    // This is in case a close quote doesn't exist
    bool explicitIncomplete = false;

    // Possible (known) comment starts in pvl
    QString comments[] = {
      "#",
      "//"
    };

    // Need more data if nothing is here!
    if (keyword.isEmpty()) return 0;

    /*
      Step 1: Read Comments

      Theoretically, we should have an input that looks like this:
        #Comment
        //Comment
        / * Comment
            Comment * /
        Keyword = Data

      So we could just grab all of the first lines; however, this method
      needs to be as error-proof as possible (it is the basis of reading
      all PVLs after all), so verify we have things that look like comments
      first, strip them & store them.
     */

    // While we have newlines, we have comments
    while(keyword.contains("\n")) {
      // Make sure we strip data every loop of comment types; otherwise we
      // have no comment and need to error out.
      bool noneStripped = true;

      // Check every comment type now and make sure this line (it isn't the last
      // line since a newline exists) starts in a comment
      QString keywordStart = keyword.mid(0, 2);

      // Handle multi-line comments
      if (keywordStart == "/*") {
        noneStripped = false;
        bool inComment = true;

        while(inComment && keyword.contains("*/")) {
          // Correct the */ to make sure it has a \n after it,
          // without this we risk an infinite loop
          int closePos = keyword.indexOf("*/\n");

          if (closePos == -1) {
            closePos = keyword.indexOf("*/") + 2;
            keyword = keyword.mid(0, closePos) + "\n" +
                      keyword.mid(closePos);
          }

          QString comment = keyword.mid(0, keyword.indexOf("\n")).trimmed();

          // Set these to true if too small, false if not (they need if
          //   cant currently fit).
          bool needsStart = (comment.size() < 2);
          bool needsStartSpace = comment.size() < 3;

          int commentEndPos = comment.size() - 2;
          bool needsEnd = (commentEndPos < 0);
          bool needsEndSpace = comment.size() < 3;

          // Needs are currently set based on QString size, apply real logic
          //   to test for character sequences (try to convert them from false
          //   to true).
          if (!needsStart) {
            needsStart = (comment.mid(0, 2) != "/*");
          }

          if (!needsEnd) {
            needsEnd = (comment.mid(commentEndPos, 2) != "*/");
          }

          if (!needsStartSpace) {
            needsStartSpace = (comment.mid(0, 3) != "/* ");
          }

          if (!needsEndSpace) {
            needsEndSpace = (comment.mid(commentEndPos - 1, 3) != " */");
          }

          if (needsStart) {
            comment = "/* " + comment;
          }
          else if (needsStartSpace) {
            comment = "/* " + comment.mid(2);
          }

          if (needsEnd) {
            comment = comment + " */";
          }
          else if (needsEndSpace) {
            comment = comment.mid(0, comment.size() - 2) + " */";;
          }

          inComment = needsEnd;

          keywordComments.push_back(comment);

          if (keyword.contains("\n")) {
            keyword = keyword.mid(keyword.indexOf("\n") + 1).trimmed();
          }

          // Check for another comment start
          if (!inComment) {
            if (keyword.size() >= 2)
              keywordStart = keyword.mid(0, 2);

            inComment = (keywordStart == "/*");
          }
        }

        // So we have a bunch of multi-line commands... make them the same size
        //   Find longest
        int longest = 0;
        for (unsigned int index = 0; index < keywordComments.size(); index++) {
          QString comment = keywordComments[index];

          if (comment.size() > longest)
            longest = comment.size();
        }

        // Now make all the sizes match longest
        for (unsigned int index = 0; index < keywordComments.size(); index++) {
          QString comment = keywordComments[index];

          while(comment.size() < longest) {
            // This adds a space to the end of the comment
            comment = comment.mid(0, comment.size() - 2) + " */";
          }

          keywordComments[index] = comment;
        }
        // They should all be the same length now
      }

      // Search for single line comments
      for (unsigned int commentType = 0;
          commentType < sizeof(comments) / sizeof(QString);
          commentType++) {

        if (keywordStart.startsWith(comments[commentType])) {
          // Found a comment start; strip this line out and store it as a
          // comment!
          QString comment = keyword.mid(0, keyword.indexOf("\n"));
          keywordComments.push_back(comment.trimmed());

          noneStripped = false;

          if (keyword.contains("\n")) {
            keyword = keyword.mid(keyword.indexOf("\n") + 1).trimmed();
          }
        }
      }

      // Does it look like Name=Value/*comm
      //                              mment*/ ?
      if (noneStripped && keyword.contains("/*") &&
          keyword.contains("*/")) {
        QString firstPart = keyword.mid(0, keyword.indexOf("\n"));
        QString lastPart = keyword.mid(keyword.indexOf("\n") + 1);

        keyword = firstPart.trimmed() + " " + lastPart.trimmed();
        noneStripped = false;
      }

      if (noneStripped) {
        QString msg = "Expected a comment in PVL but found [";
        msg += keyword;
        msg += "]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
    }

    // Do we have a keyword at all?
    if (keyword.isEmpty()) {
      return false; // need more data
    }

    /*
      Step 2: Determine Keyword Format

      Make sure we have a keyword after the comments first. We expect
      one of three formats:
        KEYWORD                             PROCESSED IN STEP 3.1
        KEYWORD = (VALUE,VALUE,...)         PROCESSED IN STEP 3.2
        KEYWORD = VALUE                     PROCESSED IN STEP 3.3
     */

    // Get the keyword name
    keywordName = readValue(keyword, explicitIncomplete);

    // we have taken the name; if nothing remains then it is value-less
    // and we are done.
    if (keyword.isEmpty()) {
      /*
        Step 3.1

        Format is determined to be:
          KEYWORD

        Well, no value/units may exist so we're done processing this keyword.
        Now check if it's a reserved keyword that is valueless
       */
      bool allowedValueless = false;
      static const std::vector<QString> reservedKeywordNames = {"OBJECT",
                                                                "BEGIN_OBJECT",
                                                                "END_OBJECT",
                                                                "ENDOBJECT",
                                                                "GROUP",
                                                                "END_GROUP",
                                                                "ENDGROUP",
                                                                "END"};
      const QString keywordNameUpper = keywordName.toUpper();
      for (const auto &reservedKeywordName : reservedKeywordNames) {
        if (keywordNameUpper == reservedKeywordName) {
          allowedValueless = true;
          break;
        }
      }
      return allowedValueless;
    }

    // if we don't have an equal, then we have a problem - an invalid symbol.
    // Our possible remaining formats both are KEYWORD = ...
    if (keyword[0] != '=') {
      QString msg = "Expected an assignment [=] when reading PVL, but found [";
      msg += keyword[0];
      msg += "]";

      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    keyword = keyword.mid(1).trimmed();

    if (keyword.isEmpty()) {
      return false;
    }

    // now we need to split into two possibilities: array or non-array
    if (keyword[0] == '(' || keyword[0] == '{') {
      /*
        Step 3.2

        Our format is confirmed as:
          KEYWORD = (...)

        We need to read each value/unit in the array.
      */

      char closingParen = ((keyword[0] == '(') ? ')' : '}');
      char wrongClosingParen = ((keyword[0] == '(') ? '}' : ')');
      bool closedProperly = false;

      vector< pair<char, char> > extraDelims;
      extraDelims.push_back(pair<char, char>('(', ')'));
      extraDelims.push_back(pair<char, char>('{', '}'));

      // strip '(' - onetime, this makes every element in the array the same
      // (except the last)
      keyword = keyword.mid(1).trimmed();

      // handle empty arrays: KEYWORD = ()
      if (!keyword.isEmpty() && keyword[0] == closingParen) {
        closedProperly = true;
      }

      // Each iteration of this loop should consume 1 value in the array,
      // including the comma, i.e. we should start out with:
      //  'VALUE,VALUE,...)' until we hit ')' (our exit condition)
      while(!keyword.isEmpty() && keyword[0] != closingParen) {
        // foundComma delimits the end of this element in the array (remains
        //  false for last value which has no comma at the end)
        bool foundComma = false;
        // keyword should be of the format: VALUE <UNIT>,....)
        // Read VALUE from it
        QString nextItem = readValue(keyword, explicitIncomplete, extraDelims);

        if (!keyword.isEmpty() && keyword[0] == wrongClosingParen) {

          QString msg = "Incorrect array close when reading PVL; expected [";
          msg += closingParen;
          msg += "] but found [";
          msg += wrongClosingParen;
          msg += "] in keyword named [";
          msg += keywordName;
          msg += "]";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }

        // This contains <VALUE, UNIT>
        pair<QString, QString> keywordValue;

        // Store VALUE
        keywordValue.first = nextItem;

        // Now there's 2 possibilities: units or no units ->
        //   if we have units, read them
        if (!keyword.isEmpty() && keyword[0] == '<') {
          QString unitsValue = readValue(keyword, explicitIncomplete);
          keywordValue.second = unitsValue;
        }

        // Now we should* have a comma, strip it
        if (!keyword.isEmpty() && keyword[0] == ',') {
          foundComma = true;
          keyword = keyword.mid(1).trimmed();
        }

        // No comma and nothing more in QString - we found
        //  KEYWORD = (VALUE,VALUE\0
        //  we need more information to finish this keyword
        if (!foundComma && keyword.isEmpty()) {
          return false; // could become valid later
        }

        bool foundCloseParen = (!keyword.isEmpty() && keyword[0] == closingParen);

        if (foundCloseParen) {
          closedProperly = true;
        }

        // Check for the condition of:
        //  keyword = (VALUE,VALUE,)
        // which is unrecoverable
        if (foundComma && foundCloseParen) {
          QString msg = "Unexpected close of keyword-value array when reading "
                       "PVL";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }

        // Check for (VALUE VALUE
        if (!foundComma && !foundCloseParen) {
          // We have ("VALUE VALUE
          if (explicitIncomplete) return false;

          // We have (VALUE VALUE
          QString msg = "Found extra data after [";
          msg += nextItem;
          msg += "] in array when reading PVL";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }

        // we're good with this element of the array, remember it
        keywordValues.push_back(keywordValue);
      }

      if (!closedProperly) {
        return false;
      }

      // Trim off the closing paren
      if (!keyword.isEmpty()) {
        keyword = keyword.mid(1).trimmed();
      }

      // Read units here if they exist and apply them
      //  case: (A,B,C) <unit>
      if (!keyword.isEmpty() && keyword[0] == '<') {
        QString units = readValue(keyword, explicitIncomplete);
        for (unsigned int val = 0; val < keywordValues.size(); val++) {
          if (keywordValues[val].second.isEmpty()) {
            keywordValues[val].second = units;
          }
        }
      }
    }
    else {
      /*
        Step 3.3

        Our format is confirmed as:
          "KEYWORD = VALUE <UNIT>"

        We need to read the single value/unit in the keywordValues array.
       */
      pair<QString, QString> keywordValue;
      keywordValue.first = readValue(keyword, explicitIncomplete);

      if (!keyword.isEmpty() && keyword[0] == '<') {
        keywordValue.second = readValue(keyword, explicitIncomplete);
      }

      keywordValues.push_back(keywordValue);
    }

    /*
      This is set when a quote is opened somewhere and never closed, it means
      we need more information
     */
    if (explicitIncomplete) {
      return false; // unclosed quote at end... need more information
    }

    /*
      See if we have a comment at the end of the keyword...
     */
    if (!keyword.isEmpty()) {
      // if the data left is a comment, we can handle it probably
      if (keyword[0] == '#' ||
          ((keyword.size() > 1 && keyword[0] == '/') &&
           (keyword[1] == '/' || keyword[1] == '*'))) {
        keywordComments.push_back(keyword);

        if (keyword.size() > 1 && keyword.mid(0, 2) == "/*") {
          if (keyword.mid(keyword.size() - 2, 2) != "*/")
            return false; // need more comment data
        }

        keyword = "";
      }
    }

    // check for extraneous data in the keyword ... ,2,3
    QRegularExpression regex("^,");
    QRegularExpressionMatch match = regex.match(keyword);
    if (!keyword.isEmpty() && match.hasMatch()) {
          keywordValues.at(0).first += keyword;
          keyword = "";
    }

    /*
      If more data remains, it is unrecognized.
    */
    if (!keyword.isEmpty()) {
      QString msg = "Keyword has extraneous data [";
      msg += keyword;
      msg += "] at the end";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    // We've parsed this keyword! :)
    return true;
  }


  QString PvlKeyword::readValue(QString &keyword, bool &quoteProblem) {
    std::vector< std::pair<char, char> > otherDelims;

    return readValue(keyword, quoteProblem, otherDelims);
  }

  /**
   * This method looks for a data element in the QString. A data element is a
   * quoted QString, a units value, or one value of an array (not including
   * units). As an example, each value in the following QString is quoted:
   *
   * 'VALUE' '=' ('VALUE','VALUE',  'VALUE' '<VALUE>')
   *
   *  The returned values of each of these elements is VALUE. Explicitly defined
   *  quotes (', ", <>) are stripped from the return value.
   *
   * @param keyword Input/Output: The keyword to get the next value from
   *                (DESTRUCTIVE)
   * @param quoteProblem Output: The QString has an unclosed quote character
   *
   * @return QString The stripped out token.
   */
  QString PvlKeyword::readValue(QString &keyword, bool &quoteProblem,
                                    const std::vector< std::pair<char, char> > &
                                    otherDelimiters) {
    QString value = "";

    // This method ignores spaces except as delimiters; let's trim the QString
    // to start
    keyword = keyword.trimmed();

    if (keyword.isEmpty()) {
      return "";
    }

    // An implied quote is one that is started without a special character, for
    //  example HELLO WORLD has HELLO and WORLD separately as implied quotes for
    //  PVLs. However, "HELLO WORLD" has an explicit (not implied) double quote.
    //  We do consider <> as quotes.
    bool impliedQuote = true;
    QChar quoteEnd = ' ';
    bool keepQuotes = false;

    if (keyword[0] == '\'' || keyword[0] == '"') {
      quoteEnd = keyword[0];
      impliedQuote = false;
    }
    else if (keyword[0] == '<') {
      quoteEnd = '>';
      impliedQuote = false;
    }
    else {
      // we're not quoted, look for alternative delimiters.
      char implicitQuotes[] = {
        ')',
        '}',
        ',',
        ' ',
        '\t',
        '<',
        '='
      };

      bool foundImplicitQuote = false;

      int currentPos = 0;
      while(!foundImplicitQuote && currentPos != keyword.size()) {
        for (unsigned int quote = 0;
            quote < sizeof(implicitQuotes) / sizeof(char);
            quote ++) {
          if (keyword[currentPos] == implicitQuotes[quote]) {
            quoteEnd = implicitQuotes[quote];
            foundImplicitQuote = true;
          }
        }

        if (!foundImplicitQuote) {
          currentPos ++;
        }
      }
    }

    for (unsigned int delim = 0; delim < otherDelimiters.size(); delim ++) {
      if (keyword[0] == otherDelimiters[delim].first) {
        quoteEnd = otherDelimiters[delim].second;
        keepQuotes = true;
        impliedQuote = false;
      }
    }

    QString startQuote;
    // non-implied delimeters need the opening delimiter ignored. Remember
    //  startQuote in case of error later on we can reconstruct the original
    //  QString.
    if (!impliedQuote) {
      startQuote += keyword[0];
      keyword = keyword.mid(1);
    }

    // Do we have a known quote end?
    int quoteEndPos = keyword.indexOf(quoteEnd);

    if (quoteEndPos != -1) {
      value = keyword.mid(0, quoteEndPos);

      // Trim keyword 1 past end delimiter (if end delimiter is last, this
      // results in empty QString). If the close delimiter is ')' or ',' then
      // leave it be however, since we need to preserve that to denote this was
      //  an end of a valuein array keyword.
      if (!impliedQuote) {
        keyword = keyword.mid(quoteEndPos + 1);
      }
      else {
        keyword = keyword.mid(quoteEndPos);
      }

      // Make sure we dont have padding
      keyword = keyword.trimmed();

      if (keepQuotes) {
        value = startQuote + value + quoteEnd;
      }

      return value;
    }
    // implied quotes terminate at end of keyword; otherwise we have a problem
    //   (which is this condition)
    else if (!impliedQuote) {
      // restore the original QString
      keyword = startQuote + keyword;
      quoteProblem = true;

      return "";
    }

    // we have an implied quote but no quote character, the rest must be the
    //  value.
    value = keyword;
    keyword = "";

    return value;
  }


  /**
   * This method reads one line of data from the input stream.
   *
   * All spaces, newlines, returns and tabs are trimmed from the result.
   * Once a newline is encountered, if the line we read is blank, we keep
   * reading. Once a line with data is encountered, that is the result. All
   * newlines, spaces, returns and tabs are consumed past this line of data
   * until the next different character (seeks to next valid data).
   *
   * @param is The stream to read from
   *
   * @return QString The first encountered line of data
   */
  QString PvlKeyword::readLine(std::istream &is, bool insideComment) {
    QString lineOfData;

    while(is.good() && lineOfData.isEmpty()) {

      // read until \n (works for both \r\n and \n) or */
      while(is.good() &&
            (!lineOfData.size() || lineOfData[lineOfData.size() - 1] != '\n')) {
        char next = is.get();

        // if non-ascii found then we're done... immediately
        if (next <= 0) {
          is.seekg(0, ios::end);
          is.get();
          return lineOfData;
        }

        // if any errors (i.e. eof) happen in the get operation then don't
        //   store this data
        if (is.good()) {
          lineOfData += next;
        }

        if (insideComment &&
            lineOfData.size() >= 2 && lineOfData[lineOfData.size() - 2] == '*' &&
            lineOfData[lineOfData.size() - 1] == '/') {
          // End of multi-line comment = end of line!
          break;
        }
        else if (lineOfData.size() >= 2 &&
                lineOfData[lineOfData.size() - 2] == '/' &&
                lineOfData[lineOfData.size() - 1] == '*') {
          insideComment = true;
        }
      }

      // Trim off non-visible characters from this line of data
      lineOfData = lineOfData.trimmed();

      // read up to next non-whitespace in input stream
      while(is.good() &&
            (is.peek() == ' ' ||
             is.peek() == '\r' ||
             is.peek() == '\n')) {
        is.get();
      }

      // if lineOfData is empty (line was empty), we repeat
    }

    return lineOfData;
  }


  /**
   * Write out the keyword.
   *
   * @param os The output stream.
   * @param keyword The PvlKeyword object to output.
   * @return <B>ostream&</B> Reference to ostream.
   * @see writeWithWrap()
   */
  ostream &operator<<(std::ostream &os, const Isis::PvlKeyword &keyword) {
    // Set up a Formatter
    PvlFormat *tempFormat = keyword.m_formatter;
    bool removeFormatter = false;
    if (tempFormat == NULL) {
      tempFormat = new PvlFormat();
      removeFormatter = true;
    }

    // Write out the comments
    for (int i = 0; i < keyword.comments(); i++) {
      for (int j = 0; j < keyword.indent(); j++) os << " ";
      os << keyword.comment(i) << tempFormat->formatEOL();
    }

    // Write the keyword name & add length to startColumn.
    int startColumn = 0;
    for (int i = 0; i < keyword.indent(); i++) {
      os << " ";
      ++startColumn;
    }
    QString keyname = tempFormat->formatName(keyword);
    os << keyname;
    startColumn += keyname.length();

    // Add padding and then write equal sign.
    for (int i = 0; i < keyword.width() - (int)keyname.size(); ++i) {
      os << " ";
      ++startColumn;
    }
    os << " = ";
    startColumn += 3;

    // If it has no value then write a NULL
    if (keyword.size() == 0) {
      os << tempFormat->formatValue(keyword);
    }

    // Loop and write each array value
    QString stringToWrite;
    for (int i = 0; i < keyword.size(); i ++) {
      stringToWrite += tempFormat->formatValue(keyword, i);
    }

    keyword.writeWithWrap(os,
                          stringToWrite,
                          startColumn,
                          *tempFormat);

    if (removeFormatter) delete tempFormat;

    return os;
  }


  //! This is an assignment operator
  const PvlKeyword &PvlKeyword::operator=(const PvlKeyword &other) {
    if (this != &other) {
      m_formatter = other.m_formatter;

      if (m_name) {
        delete [] m_name;
        m_name = NULL;
      }

      if (other.m_name) {
        setName(other.m_name);
      }

      m_values = other.m_values;

      if (m_units) {
        delete m_units;
        m_units = NULL;
      }

      if (other.m_units) {
        m_units = new std::vector<QString>(*other.m_units);
      }

      if (m_comments) {
        delete m_comments;
        m_comments = NULL;
      }

      if (other.m_comments) {
        m_comments = new std::vector<QString>(*other.m_comments);
      }

      m_width = other.m_width;
      m_indent = other.m_indent;
    }

    return *this;
  }

  /**
   * Validate a Keyword, comparing against corresponding Template Keyword.
   *
   * Template Keyword has the format:
   * keyName = (valueType, optional/required, Values allowed separated by comma)
   *
   * @author Sharmila Prasad (9/22/2010)
   *
   * @param pvlKwrd      - Keyword to be validated
   * @param psValueType  - Value Type (positive / negative) for numbers
   * @param pvlKwrdValue - Template Keyword __Value or __Range to validate keyword's value
   */
  void PvlKeyword::validateKeyword(PvlKeyword & pvlKwrd, QString psValueType, PvlKeyword* pvlKwrdValue)
  {
    int iSize = pvlKwrd.size();

    QString sType = m_values[0].toLower();

    // Value type
    if (psValueType.length()) {
      psValueType = psValueType.toLower();
    }

    double dRangeMin=0, dRangeMax=0;
    bool bRange = false;
    bool bValue = false;
    if (pvlKwrdValue != NULL) {
      QString sValueName = pvlKwrdValue->name();

      // Check for Range
      if (sValueName.contains("__Range")) {
        dRangeMin = toDouble((*pvlKwrdValue)[0]);
        dRangeMax = toDouble((*pvlKwrdValue)[1]);
        bRange = true;
      }
      else if (sValueName.contains("__Value")) {
        bValue = true;
      }
    }

    // Type integer
    if (sType == "integer") {
      for (int i=0; i<iSize; i++) {
        QString sValue = pvlKwrd[i].toLower();
        if (sValue != "null"){
          int iValue=0;
          try {
            iValue = toInt(sValue);
          } catch (IException & e) {
            QString sErrMsg = "\"" +pvlKwrd.name() +"\" expects an Integer value";
            throw IException(e, IException::User, sErrMsg, _FILEINFO_);
          }
          if (bRange && (iValue < dRangeMin || iValue > dRangeMax)) {
            QString sErrMsg = "\"" +pvlKwrd.name() +"\" is not in the specified Range";
            throw IException(IException::User, sErrMsg, _FILEINFO_);
          }
          if (bValue) {
            bool bFound = false;
            for (int j=0; j<pvlKwrdValue->size(); j++) {
              if (iValue == toInt((*pvlKwrdValue)[j])) {
                bFound = true;
                break;
              }
            }
            if (!bFound) {
              QString sErrMsg = "\"" +pvlKwrd.name() +"\" has value not in the accepted list";
              throw IException(IException::User, sErrMsg, _FILEINFO_);
            }
          }
          // Type is specified (positive / negative)
          if (psValueType.length()) {
            if ((psValueType == "positive" && iValue < 0) || (psValueType == "negative" && iValue >= 0) ) {
              QString sErrMsg = "\"" +pvlKwrd.name() +"\" has invalid value";
              throw IException(IException::User, sErrMsg, _FILEINFO_);
            }
          }
        }
      }
      return;
    }

    // Type double
    if (sType == "double") {
      for (int i=0; i<iSize; i++) {
        QString sValue = pvlKwrd[i].toLower();
        if (sValue != "null"){
          double dValue = toDouble(sValue);
          if (bRange && (dValue < dRangeMin || dValue > dRangeMax)) {
            QString sErrMsg = "\"" +pvlKwrd.name() +"\" is not in the specified Range";
            throw IException(IException::User, sErrMsg, _FILEINFO_);
          }
          if (bValue) {
            bool bFound = false;
            for (int j=0; j<pvlKwrdValue->size(); j++) {
              if (dValue == toDouble((*pvlKwrdValue)[j])) {
                bFound = true;
                break;
              }
            }
            if (!bFound) {
              QString sErrMsg = "\"" +pvlKwrd.name() +"\" has value not in the accepted list";
              throw IException(IException::User, sErrMsg, _FILEINFO_);
            }
          }
          // Type is specified (positive / negative)
          if (psValueType.length()) {
            if ((psValueType == "positive" && dValue < 0) || (psValueType == "negative" && dValue >= 0) ) {
              QString sErrMsg = "\"" +pvlKwrd.name() +"\" has invalid value";
              throw IException(IException::User, sErrMsg, _FILEINFO_);
            }
          }
        }
      }
      return;
    }

    // Type boolean
    if (sType == "boolean") {
      for (int i=0; i<iSize; i++) {
        QString sValue = pvlKwrd[i].toLower();
        if (sValue != "null" && sValue != "true" && sValue != "false"){
          QString sErrMsg = "Wrong Type of value in the Keyword \"" + name() + "\" \n";
          throw IException(IException::User, sErrMsg, _FILEINFO_);
        }
      }
      return;
    }

    // Type String
    if (sType == "string") {
      for (int i=0; i<iSize; i++) {
        QString sValue = pvlKwrd[i].toLower();
        if (bValue) {
          bool bValFound = false;
          for (int i=0; i<pvlKwrdValue->size(); i++) {
            if (sValue == (*pvlKwrdValue)[i].toLower()) {
              bValFound = true;
              break;
            }
          }
          if (!bValFound) {
            QString sErrMsg = "Wrong Type of value in the Keyword \"" + name() + "\" \n";
            throw IException(IException::User, sErrMsg, _FILEINFO_);
          }
        }
      }
    }
  }
}
