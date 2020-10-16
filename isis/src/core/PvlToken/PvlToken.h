#ifndef PvlToken_h
#define PvlToken_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <vector>
#include <cctype>

#include <QString>

namespace Isis {
  /**
   * @brief Container for Keyword-value pair
   *
   * This class is used for internalizing keyword-value(s) pairs. For example,
   * SPACECRAFT=MARS_GLOBAL_SURVEYOR or FROM=file.cub. This is useful when parsing
   * ASCII files such as PDS labels or command lines.
   *
   * @ingroup Parsing
   *
   * @author 2002-03-18 Jeff Anderson
   *
   * @internal
   *  @history 2003-05-16 Stuart Sides - Modified schema from astrogeology...
   *                                     isis.astrogeology...
   *  @history 2005-02-14 Elizabeth Ribelin - Modified file to support Doxygen
   *  @history 2013-03-11 Steven Lambright and Mathew Eis - Brought method names and member variable
   *                          names up to the current Isis 3 coding standards. Fixes #1533.
   *                                          documentation
   *
   *  @todo 2005-02-14 Jeff Anderson - add coded and implemetation example to
   *                                   class documentation
   */

  class PvlToken {
    private:
      QString m_key;                 //!< Storage for the keyword name
      std::vector<QString> m_value;   /**<Vector storage for a list of values.
                                            See the standard template library
                                            for more information on vectors.*/

    public:
      PvlToken(const QString &k);
      PvlToken();
      ~PvlToken();

      void setKey(const QString &k);
      QString key() const;
      QString keyUpper() const;

      void addValue(const QString &v);
      QString value(const int index = 0) const;
      QString valueUpper(const int index = 0) const;
      int valueSize() const;
      void valueClear();

      inline const std::vector<QString> &valueVector() const {
        return m_value;
      };
  };
};

#endif
