/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Message.h"

using namespace std;

QString Isis::Message::KeywordValueBad(const QString &key) {
  return "Keyword [" + key + "] has bad value";
}

QString Isis::Message::KeywordValueBad(const QString &key, const QString &value) {
  QString message;

  message = "Keyword [" + key + "] has bad value [";
  if(value.size() <= 20) {
    message += value + "]";
  }
  else {
    message += value.mid(0, 20) + " ...]";
  }
  return message;
}
