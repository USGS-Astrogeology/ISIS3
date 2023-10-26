/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <vector>

#include "PvlKeyword.h"
#include "PvlSequence.h"
#include "Preference.h"

using namespace Isis;

int main(void) {
  Preference::Preferences(true);

  PvlKeyword key("Key");
  key += std::string("(xyzzy,plover)");
  key += std::string("(2,b,|,^,2,b)");

  PvlSequence seq;
  seq = key;

  seq += "(a,b,c)";
  seq += "(d,e)";
  seq += "singleton";

  std::vector<std::string> slist;
  slist.push_back("1");
  slist.push_back("2");
  seq += slist;

  std::vector<int> ilist;
  ilist.push_back(-1);
  ilist.push_back(-2);
  ilist.push_back(-3);
  seq += ilist;

  std::vector<double> dlist;
  dlist.push_back(1.0);
  dlist.push_back(0.0);
  dlist.push_back(-1.0);
  seq += dlist;

  std::cout << seq.Size() << std::endl;
  for(int i = 0; i < seq.Size(); i++) {
    for(int j = 0; j < (int)seq[i].size(); j++) {
      std::cout << seq[i][j] << " ";
    }
    std::cout << std::endl;
  }

  seq.Clear();
  std::cout << seq.Size() << std::endl;
  return 0;
}
