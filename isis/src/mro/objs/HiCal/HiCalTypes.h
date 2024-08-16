#ifndef HiCalTypes_h
#define HiCalTypes_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
                                                                       
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#include "tnt_array1d.h"
#include "tnt_array1d_utils.h"
#include "tnt_array2d.h"
#include "tnt_array2d_utils.h"

#include "IString.h"
#include "PvlKeyword.h"

namespace Isis {

typedef TNT::Array1D<double> HiVector;       //!<  1-D Buffer
typedef TNT::Array2D<double> HiMatrix;       //!<  2-D buffer

class HiHistory {
  public:
    /**
     * @brief Output operator for history events
     * @param o Output stream
     * @param h History class to write contents of
     *
     * @return std::ostream& Returns new state of stream
     */
    friend std::ostream &operator<<(std::ostream &o, const HiHistory &h) {
      std::vector<QString>::const_iterator it = h._events.begin();

      while (it != h._events.end()) {
        o << *it << "; ";
        it++;
      }
      return (o);
    }

  public:
    HiHistory() { }
    HiHistory(const HiHistory &h) : _events(h._events) { }
    virtual ~HiHistory() { }

    inline int size() const { return (_events.size()); }
    void add(const QString &event) { _events.push_back(event); }
    QString get(unsigned int index = 0) const {
      if (index < _events.size()) {
        return (_events[index]);
      }
      else {
        return (QString(""));
      }
    }

    void clear() { _events.clear(); }

    PvlKeyword makekey(const QString &name = "History") const {
      PvlKeyword key(name.toStdString());
      for (unsigned int i = 0 ; i < _events.size() ; i++) {
        key.addValue(_events[i].toStdString());
      }
      return (key);
    }

  private:
    std::vector<QString> _events;
};

};
#endif
