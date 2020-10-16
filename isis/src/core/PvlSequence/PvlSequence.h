#ifndef PvlSequence_h
#define PvlSequence_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <vector>

#include <QString>

#include "PvlKeyword.h"

namespace Isis {
  /**
   * @brief Parse and return elements of a Pvl sequence.
   *
   * A Pvl sequence is essentially an array of arrays.  For example,
   * @code
   * Keyword = ((a,b,c), (d,e))
   * @endcode
   * To extract the invidual arrays from a PvlKeyword you must use a
   * PvlSequence.
   *
   * Here is an example of how to use PvlSequence
   * @code
   * PvlKeyword k;
   * k += "(a,b,c)";
   * k += "(d,e)";
   *
   * PvlSequence s = k;
   * cout << s.Size() << endl; // should be 2
   * cout << s[0][0] << endl;  // should be a
   * cout << s[1][1] << endl;  // should be e
   * @endcode
   * @ingroup Parsing
   *
   * @author 2005-02-16 Jeff Anderson
   *
   * @internal
   *  @todo Add PvlSequence(PvlKeyword) constructor
   *  so that we can code PvlSequence s = k;
   *  where k is a PvlKeyword.
   */
  class PvlSequence {
    public:
      //! Construct an empty sequence
      PvlSequence() {};

      //! Destruct sequence
      ~PvlSequence() {};

      PvlSequence &operator=(PvlKeyword &key);

      PvlSequence &operator+=(const QString &array);

      PvlSequence &operator+=(std::vector<QString> &array);

      PvlSequence &operator+=(std::vector<int> &array);

      PvlSequence &operator+=(std::vector<double> &array);

      //! Return the ith Array of the sequence
      std::vector<QString> &operator[](int i) {
        return p_sequence[i];
      };

      //! Number of arrays in the sequence
      inline int Size() const {
        return p_sequence.size();
      };

      //! Clear the sequence
      inline void Clear() {
        p_sequence.clear();
      };

    private:
      std::vector<std::vector<QString> > p_sequence; /**<A vector of Strings
                                                        that contains the values
                                                        for the keyword. */

  };
};

#endif
