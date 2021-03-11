#include <vector>

#include <QString>

#include "PvlKeyword.h"
#include "PvlSequence.h"
#include "Preference.h"

using namespace Isis;

int main(void) {
  Preference::Preferences(true);

  PvlKeyword key("Key");
  key += QString("(xyzzy,plover)");
  key += QString("(2,b,|,^,2,b)");

  PvlSequence seq;
  seq = key;

  seq += QString("(a,b,c)");
  seq += QString("(d,e)");
  seq += QString("singleton");

  std::vector<QString> slist;
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
