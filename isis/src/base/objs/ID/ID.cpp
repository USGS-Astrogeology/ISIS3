/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ID.h"
#include "IException.h"
#include "Message.h"
#include "IString.h"
#include <iostream>
using namespace std;
namespace Isis {

  /**
   * Creates an ID object
   * @param name The string to be a base for the serial IDs
   * @param basenum The number to start the count at. Defaults to
   *                one.
   */
  ID::ID(const QString &name, int basenum) {
    p_current = basenum;
    p_namebase = name;
    if(!p_namebase.contains("?")) {
      std::string msg = "No replacement set in string [" + p_namebase.toStdString() + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    p_numStart = ((int) p_namebase.indexOf("?", 0));
    int endPos = (int)p_namebase.lastIndexOf("?");
    p_numLength = (endPos - p_numStart) + 1;
    QString sub = p_namebase.mid(p_numStart, p_numLength);
    for(int i = 0; i < (int)sub.length(); i++) {
      if(sub[i] != '?') {
        std::string msg = "IString [" + p_namebase.toStdString() + "] contains more than one replacement set";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    p_namebase.remove(p_numStart, p_numLength);
  }

  /**
   * Deconstructor
   */
  ID::~ID() {
  }

  /**
   * Returns the next ID in the sequence.
   *
   * @return QString The next ID in the sequence
   */
  QString ID::Next() {
    IString num(p_current);
    if((int)num.size() > p_numLength) {
      QString replacement = "?";
      while((int)replacement.size() < p_numLength) {
        replacement += "?";
      }
      QString original = p_namebase;
      original.insert(p_numStart, replacement);
      std::string msg = "Maximum number reached for string [" + original.toStdString() + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    while((int)num.size() < p_numLength) {
      num = "0" + num;
    }
    p_current++;
    QString temp = p_namebase;
    return temp.insert((p_numStart), num.c_str());
  }

}
