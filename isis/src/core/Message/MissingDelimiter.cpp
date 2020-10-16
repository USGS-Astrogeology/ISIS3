/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <sstream>

#include "IString.h"
#include "Message.h"

using namespace std;

QString Isis::Message::MissingDelimiter(char d) {
  return "Missing delimiter [" + toString(d) + "]";
}

QString Isis::Message::MissingDelimiter(char d, const QString &near) {
  QString message = "Missing delimiter [" + toString(d) + "] at or near [";

  if(near.size() <= 20) {
    message += near + "]";
  }
  else {
    message += near.mid(0, 20) + " ...]";
  }

  return message;
}
