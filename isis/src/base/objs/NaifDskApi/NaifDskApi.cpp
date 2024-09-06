/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */ 
#include "NaifDskApi.h"

// Specs for convenient NAIF vectors and matrices
#include <tnt/tnt_array1d.h>
#include <tnt/tnt_array1d_utils.h>
#include <tnt/tnt_array2d.h>
#include <tnt/tnt_array2d_utils.h>

#include "IString.h"

namespace Isis {
  /** 
   *  Verifies that the given NaifVector or NaifVertex is 3 dimensional.
   *  
   *  Also acceptable:
   *     bool validate(const NaifVector &v).
   *  
   * @param v Input can be a NaifVertex or NaifVector to validate.
   *  
   * @return <b>bool</b> Indicates whether the given array is size 3.
   */
  bool validate(const NaifVertex &v) {
    if ( 3 != v.dim1() ) return (false);
    return (true);
  }



  /** 
   *  Verifies that the given NaifTriangle is 3 x 3.
   *  
   * @param t NaifTriangle to validate.
   *  
   * @return <b>bool</b> Indicates whether the given 2D array is 3 x 3.
   */
  bool validate(const NaifTriangle &t) {
    if ( 3 != t.dim1() ) return (false);
    if ( 3 != t.dim2() ) return (false);
    return (true);
  }



  /** 
   *  Enables any TNT array of SpiceDoubles to be passed into qDebug() directly.
   *  Valid inputs include NaifVector and NaifVertex.
   *  Array values will be printed on a single line with precision=15 digits.
   *  
   * @param tntArray TNT array to be printed.
   */
  QDebug operator<<(QDebug dbg, const TNT::Array1D<SpiceDouble> &tntArray) {
    dbg.nospace() << toString(tntArray);
    return dbg;
  }



  /** 
   *  Enables any 2 dimensional TNT array of SpiceDoubles to be passed into
   *  qDebug() directly. Valid inputs include NaifTriangle. Each row will
   *  be indented 4 spaces and each entry will be printed with precision=15
   *  digits.
   *  
   * @param tntMatrix Two dimensional TNT array to be printed.
   */
  QDebug operator<<(QDebug dbg, const TNT::Array2D<SpiceDouble> &tntMatrix) {
    for (int i = 0; i < tntMatrix.dim1(); i++) {
      dbg.nospace() << "    ";
      for (int j = 0; j < tntMatrix.dim2(); j++) {
        dbg.nospace() << QString::fromStdString(toString(tntMatrix[i][j], 15)) << "     ";
      }
       dbg.nospace() << Qt::endl;
    }
    return dbg;
  }



  /** 
   *  Formats any TNT array of SpiceDoubles as a string with given precision.
   *  Valid inputs include NaifVector and NaifVertex. The array in the output
   *  string will be comma separated and enclosed in parentheses.
   *  
   * @param tntArray TNT array to be formatted as a string.
   * @param precision Number of digits each value in the array will be stored with. 
   *  
   * @return <b>QString</b> A string containing the array values in parentheses and comma separated.
   */
  QString toString(const TNT::Array1D<SpiceDouble> &naifArray, int precision) {
    QString result = "( ";
    for (int i = 0; i < naifArray.dim1(); i++) {
      result += QString::fromStdString(toString(naifArray[i], precision));
      if (i != naifArray.dim1() - 1) result += ", ";
    }
    result += " )";
    return result;
  }


}
// namespace Isis
