/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <sstream>

#include "IString.h"
#include "Message.h"

using namespace std;

std::string Isis::Message::MissingDelimiter(char d) {
  std::string del(1, d);
  return "Missing delimiter [" + del + "]";
}

std::string Isis::Message::MissingDelimiter(char d, const std::string near) {
  std::string del(1, d);
  std::string message = "Missing delimiter [" + del + "] at or near [";

  if(near.size() <= 20) {
    message += near + "]";
  }
  else {
    message += near.substr(0, 20) + " ...]";
  }

  return message;
}
