/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <sstream>

#include <QString>

#include "PvlSequence.h"
#include "Pvl.h"
#include "IString.h"

namespace Isis {
  /**
   * Load a sequence using a Pvl keyword.  Each value of the PvlKeyword will
   * be treated as an array for a sequence.  Typically, the values in the
   * PvlKeyword should be enclosed in parens and comma separated.
   * For example, (a,b,c).
   *
   * @param key keyword containing sequence
   */
  PvlSequence &PvlSequence::operator=(PvlKeyword &key) {
    for(int i = 0; i < key.size(); i++) {
      this->operator+=(key[i]);
    }
    return *this;
  }

  /**
   * Add a string array to the sequence.  The values in the string
   * must be enclosed in parens and comma separated.  For example,
   * (1,2,3).
   *
   * @param  array  A string representing an array.
   */
  PvlSequence &PvlSequence::operator+=(const QString &array) {
    std::stringstream str;
    str << "temp = " << array;
    Pvl pvl;
    str >> pvl;
    PvlKeyword &key = pvl["temp"];
    std::vector<QString> temp;
    for(int i = 0; i < key.size(); i++) {
      temp.push_back(key[i]);
    }
    p_sequence.push_back(temp);
    return *this;
  }

  /**
   * Add a vector of strings to the sequence.  This adds another array to the
   * sequence whose values are all strings
   *
   * @param  array  vector of strings
   */
  PvlSequence &PvlSequence::operator+=(std::vector<QString> &array) {
    std::vector<QString> temp;
    for(int i = 0; i < (int)array.size(); i++) {
      temp.push_back(array[i]);
    }
    p_sequence.push_back(temp);
    return *this;
  }

  /**
   * Add a vector of ints to the sequence.  This adds another array to the
   * sequence whose values are all integers.
   *
   * @param  array  vector of integers
   */
  PvlSequence &PvlSequence::operator+=(std::vector<int> &array) {
    std::vector<QString> temp;
    for(int i = 0; i < (int)array.size(); i++) {
      temp.push_back(toString(array[i]));
    }
    p_sequence.push_back(temp);
    return *this;
  }

  /**
   * Add a vector of ints to the sequence.  This adds another array to the
   * sequence whose values are all doubles.
   *
   * @param  array  vector of doubles
   */
  PvlSequence &PvlSequence::operator+=(std::vector<double> &array) {
    std::vector<QString> temp;
    for(int i = 0; i < (int)array.size(); i++) {
      temp.push_back(toString(array[i]));
    }
    p_sequence.push_back(temp);
    return *this;
  }
}
