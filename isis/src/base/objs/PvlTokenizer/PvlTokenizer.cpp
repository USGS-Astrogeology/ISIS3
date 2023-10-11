/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "PvlTokenizer.h"

#include <sstream>
#include <fstream>

#include <QDebug>

#include "IException.h"
#include "IString.h"
#include "Message.h"

using namespace std;
namespace Isis {

  //! Constructs a Tokenizer with an empty token list
  PvlTokenizer::PvlTokenizer() {
    Clear();
  }

  //! Destroys the Tokenizer object and token list
  PvlTokenizer::~PvlTokenizer() {
    Clear();
  }

  //!  Empties the token list.
  void PvlTokenizer::Clear() {
    tokens.clear();
  }

  /**
   * Loads the Token list from a stream. The loading will be terminated upon
   * reaching either 1) end-of-stream, or
   * 2) a programmer specified terminator QString
   *
   * @param stream The input stream to tokenize
   *
   * @param terminator If the tokenizer see's this QString as a token in the input
   *                   stream it will cease tokenizing.  Defaults to "END"
   *
   * @throws Isis::iException::Parse
   */
  void PvlTokenizer::Load(std::istream &stream, const QString &terminator) {
    QString upTerminator(terminator.toUpper());
    QString s;
    int c;
    bool newlineFound = false;

    while(true) {
      newlineFound = SkipWhiteSpace(stream);
      c = stream.peek();
      ValidateCharacter(c);
      if(c == EOF) return;

      if(c == '#') {
        s = ReadComment(stream);
        Isis::PvlToken t("_COMMENT_");
        t.addValue(s);

        if(newlineFound || tokens.size() == 0 || tokens[tokens.size()-1].valueSize() == 0) {
          // applies to next pvl item
          tokens.push_back(t);
        }
        else {
          // applies to previous pvl item
          tokens.push_back(tokens[tokens.size()-1]);
          tokens[tokens.size()-2] = t;
        }

        continue;
      }

      if(c == '/') {
        c = stream.get();
        c = stream.peek();
        stream.unget();
        ValidateCharacter(c);
        if(c == '*') {
          s = ReadComment(stream);
          Isis::PvlToken t("_COMMENT_");
          t.addValue(s);

          if(newlineFound || tokens.size() == 0 || tokens[tokens.size()-1].valueSize() == 0) {
            // applies to next pvl item
            tokens.push_back(t);
          }
          else {
            // applies to previous pvl item
            tokens.push_back(tokens[tokens.size()-1]);
            tokens[tokens.size()-2] = t;
          }

          continue;
        }
      }

      s = ReadToken(stream);
      Isis::PvlToken t(s);

      if(t.keyUpper() == upTerminator) {
        tokens.push_back(t);
        return;
      }

      SkipWhiteSpace(stream);
      c = stream.peek();
      ValidateCharacter(c);
      if(c == EOF) {
        tokens.push_back(t);
        return;
      }

      if(c != '=') {
        tokens.push_back(t);
        if(t.keyUpper() == upTerminator) return;
        continue;
      }

      stream.ignore();
      SkipWhiteSpace(stream);

      c = stream.peek();
      ValidateCharacter(c);
      if(c == EOF) {
        tokens.push_back(t);
        return;
      }

      if(c == '(') {
        stream.ignore();
        try {
          s = ReadToParen(stream);
          ParseCommaList(t, s);
        }
        catch(IException &e) {
          std::string message = Isis::Message::KeywordValueBad(t.key().toStdString());
          throw IException(e, IException::Unknown, message, _FILEINFO_);
        }
        tokens.push_back(t);
        continue;
      }

      if(c == '{') {
        stream.ignore();
        try {
          s = ReadToBrace(stream);
          ParseCommaList(t, s);
        }
        catch(IException &e) {
          std::string message = Isis::Message::KeywordValueBad(t.key().toStdString());
          throw IException(e, IException::Unknown, message, _FILEINFO_);
        }
        tokens.push_back(t);
        continue;
      }

      if(c == '"') {
        stream.ignore();
        try {
          s = ReadToDoubleQuote(stream);
        }
        catch(IException &e) {
          std::string message = Isis::Message::KeywordValueBad(t.key().toStdString());
          throw IException(e, IException::Unknown, message, _FILEINFO_);
        }
        t.addValue(s);
        tokens.push_back(t);
        continue;
      }

      if(c == '\'') {
        stream.ignore();
        try {
          s = ReadToSingleQuote(stream);
        }
        catch(IException &e) {
          std::string message = Isis::Message::KeywordValueBad(t.key().toStdString());
          throw IException(IException::Unknown, message, _FILEINFO_);
        }
        t.addValue(s);
        tokens.push_back(t);
        continue;
      }


      s = ReadToken(stream);
      t.addValue(s);
      tokens.push_back(t);
      continue;
    }
  }

  /**
   * Reads and returns a comment from the stream.
   *
   * @param stream Input stream to read from
   *
   * @return QString
   */
  QString PvlTokenizer::ReadComment(std::istream &stream) {
    QString s;
    int c;

    c = stream.get();
    while((c != '\r') && (c != '\n') && (c != '\0')) {
      s += (char) c;
      c = stream.peek();
      ValidateCharacter(c);
      if(c == EOF) return s;
      c = stream.get();
    }

    stream.unget();

    return s;
  }

  /**
   * Reads and returns a token from the stream. A token is delimited by either
   * whitespace or an equal sign. In the case of whitespace the token will be
   * considered valueless. That is, there will be no value in the value side of
   * the token (e.g., KEYWORD=).
   *
   * @param stream Input stream to read from
   *
   * @return QString
   */
  QString PvlTokenizer::ReadToken(std::istream &stream) {
    QString s;
    int c;

    c = stream.get();
    while((!isspace(c)) && (c != '\0') && (c != '=')) {
      s += (char) c;
      c = stream.peek();
      ValidateCharacter(c);
      if(c == EOF) return s;
      c = stream.get();
    }

    stream.unget();

    return s;
  }

  /**
   * Skips over whitespace so long as it is not inside quotes. Whitespace is
   * tabs, blanks, line feeds, carriage returns, and NULLs.
   *
   * @param stream Input stream to read from
   */
  bool PvlTokenizer::SkipWhiteSpace(std::istream &stream) {
    bool foundNewline = false;
    int c;

    c = stream.peek();
    ValidateCharacter(c);
    while((isspace(c)) || (c == '\0')) {
      if(c == '\n') {
        foundNewline = true;
      }

      c = stream.get();
      c = stream.peek();
      ValidateCharacter(c);
    }

    return foundNewline;
  }


  QString PvlTokenizer::ReadToDoubleQuote(std::istream &stream) {
    QString s;
    int c;

    do {
      c = stream.get();
      ValidateCharacter(c);
      if(c == EOF) {
        QString message = Isis::Message::MissingDelimiter('"', s);
        throw IException(IException::Unknown, message, _FILEINFO_);
      }
      else if(c != '"') {
        s += (char) c;
      }
    }
    while(c != '"');

    int pos = s.indexOf(QRegExp("[\\n\\r]"));
    while(pos != -1) {
      QString first = s.mid(0, pos);
      bool addspace = false;
      if(first[pos-1] == ' ') addspace = true;
      first = first.remove(QRegExp("[\\s]*$"));
      QString second = s.mid(pos + 1);
      if(second[0] == ' ') addspace = true;
      if(second[0] == '\r') addspace = true;
      if(second[0] == '\n') addspace = true;
      second = second.remove(QRegExp("^[\\s]*"));
      if(second[0] == ',') addspace = false;
      s = first;
      if(addspace) s += " ";
      s += second;

      pos = s.indexOf(QRegExp("[\\n\\r]"));
    }
    return s;
  }

  QString PvlTokenizer::ReadToSingleQuote(std::istream &stream) {
    QString s;
    int c;

    do {
      c = stream.get();
      ValidateCharacter(c);
      if(c == EOF) {
        QString message = Isis::Message::MissingDelimiter('\'', s);
        throw IException(IException::Unknown, message, _FILEINFO_);
      }
      else if(c != '\'') {
        s += (char) c;
      }
    }
    while(c != '\'');

    int pos = s.indexOf(QRegExp("[\\n\\r]"));
    while(pos != -1) {
      QString first = s.mid(0, pos);
      bool addspace = false;
      if(first[pos-1] == ' ') addspace = true;
      first = first.remove(QRegExp("[\\s]*$"));
      QString second = s.mid(pos + 1);
      if(second[0] == ' ') addspace = true;
      if(second[0] == '\r') addspace = true;
      if(second[0] == '\n') addspace = true;
      second = second.remove(QRegExp("^[\\s]*"));
      if(second[0] == ',') addspace = false;
      s = first;
      if(addspace) s += " ";
      s += second;
      pos = s.indexOf(QRegExp("[\\n\\r]"));
    }

    return s;
  }

  QString PvlTokenizer::ReadToParen(std::istream &stream) {
    QString s;
    int c;
    int leftParenCount = 1;

    do {
      c = stream.get();
      ValidateCharacter(c);
      if(c == EOF) {
        QString message = Isis::Message::MissingDelimiter(')', s);
        throw IException(IException::Unknown, message, _FILEINFO_);
      }
      else if(c == '"') {
        try {
          s += "\"" + ReadToDoubleQuote(stream) + "\"";
        }
        catch(IException &) {
          QString message = Isis::Message::MissingDelimiter('"', s);
          throw IException(IException::Unknown, message, _FILEINFO_);
        }
      }
      else if(c == '\'') {
        try {
          s += "'" + ReadToSingleQuote(stream) + "'";
        }
        catch(IException &) {
          QString message = Isis::Message::MissingDelimiter('\'', s);
          throw IException(IException::Unknown, message, _FILEINFO_);
        }
      }
      else if(c == ')') {
        leftParenCount--;
        if(leftParenCount > 0) s += (char) c;
      }
      else {
        s += (char) c;
        if(c == '(') leftParenCount++;
      }
    }
    while(leftParenCount > 0);

    return s;
  }

  QString PvlTokenizer::ReadToBrace(std::istream &stream) {
    QString s;
    int c;
    int leftBraceCount = 1;

    do {
      c = stream.get();
      ValidateCharacter(c);
      if(c == EOF) {
        QString message = Isis::Message::MissingDelimiter('}', s);
        throw IException(IException::Unknown, message, _FILEINFO_);
      }
      else if(c == '"') {
        try {
          s += "\"" + ReadToDoubleQuote(stream) + "\"";
        }
        catch(IException &e) {
          QString message = Isis::Message::MissingDelimiter('"', s);
          throw IException(IException::Unknown, message, _FILEINFO_);
        }
      }
      else if(c == '\'') {
        try {
          s += "'" + ReadToSingleQuote(stream) + "'";
        }
        catch(IException &) {
          QString message = Isis::Message::MissingDelimiter('\'', s);
          throw IException(IException::Unknown, message, _FILEINFO_);
        }
      }
      else if(c == '}') {
        leftBraceCount--;
        if(leftBraceCount > 0) s += (char) c;
      }
      else {
        s += (char) c;
        if(c == '{') leftBraceCount++;
      }
    }
    while(leftBraceCount > 0);

    return s;
  }

  /**
   * This routine parses a QString containing a comma separated list. Each of the
   * items in the list is stored as a value in the Token.
   *
   * @param t Token to load the comma separated list
   *
   * @param cl QString containing comma separated list
   */
  void PvlTokenizer::ParseCommaList(Isis::PvlToken &t, const QString &cl) {
    stringstream stream(cl.toLatin1().data());
    int c;
    QString s;

    do {
      SkipWhiteSpace(stream);
      c = stream.get();
      if(c == '"') {
        s += ReadToDoubleQuote(stream);
      }
      else if(c == '\'') {
        s += ReadToSingleQuote(stream);
      }
      else if(c == '(') {
        s += "(";
        s += ReadToParen(stream);
        s += ")";
      }
      else if(c == '{') {
        s += "{";
        s += ReadToBrace(stream);
        s += "}";
      }
      else if(c == ',') {
        t.addValue(s);
        s.clear();
      }
      else if(c != EOF) {
        s += (char) c;
      }
    }
    while(c != EOF);

    t.addValue(s);
  }


  vector<Isis::PvlToken> & PvlTokenizer::GetTokenList() {
    return tokens;
  }

  /**
   * Make sure a character is valid printable (non-control) character
   *
   * @param c Character to be validated
   */
  void PvlTokenizer::ValidateCharacter(int c) {
    if(c == EOF) return;
    if(isprint(c)) return;
    if(isspace(c)) return;
    if(c == '\0') return;

    QString message = "ASCII data expected but found unprintable (binary) data";
    throw IException(IException::Unknown, message, _FILEINFO_);
  }
} // end namespace isis
