/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <math.h>
#include <QString>

#include "FileName.h"
#include "Constants.h"
#include "IException.h"
#include "NthOrderPolynomial.h"

using namespace std;
namespace Isis {

  /**
   * Create an NthOrderPolynomial
   * 
   * @param degree The order/degree of the polynomial
   * 
   */
  NthOrderPolynomial::NthOrderPolynomial(int degree) : 
    Isis::BasisFunction("NthOrderPolynomial", 2, degree) {
    p_degree = degree;
  }
  

  /**
   * This is the the overriding virtual function that provides the expansion into
   * the nth order polynomial equation. 
   *  
   * See BasisFunction for more information.
   *
   * @param vars A vector of double values to use for the expansion.
   */
  void NthOrderPolynomial::Expand(const std::vector<double> &vars) {

    if((int) vars.size() != Variables()) {
      std::string mess = "Number of variables given (" + std::to_string(vars.size())
          + ") does not match expected (" + std::to_string(Variables()) + ")!";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }
    
    double t1 = vars[0];
    double t2 = vars[1];
    p_terms.clear();
    for (int i = p_degree; i >= 1; i--) {
      p_terms.push_back(pow(t1, i) - pow(t2, i));
    }
    return;
  }
} // end namespace isis

