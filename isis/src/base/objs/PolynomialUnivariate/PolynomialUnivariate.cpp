/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <cmath>

#include "PolynomialUnivariate.h"
#include "IString.h"
#include "IException.h"

namespace Isis {
  /**
   * Create a PolynomialUnivariate object
   *
   * @param degree The order/degree of the polynomial
   *
   */
  PolynomialUnivariate::PolynomialUnivariate(int degree) :
    Isis::Basis1VariableFunction("PolynomialUnivariate", (degree + 1)) {
    p_degree = degree;
  }


  /**
   * Create a PolynomialUnivariate object
   *
   * @param degree The order/degree of the polynomial
   * @param coeffs a list of the coefficients in increasing degree. So the first
   *               element is the constant coefficient.
   */
   PolynomialUnivariate::PolynomialUnivariate(int degree, std::vector<double> coeffs) :
    Isis::Basis1VariableFunction("PolynomialUnivariate", (degree + 1)) {
     p_degree = degree;
     SetCoefficients(coeffs);
   }


   /**
   * This is the the overriding virtual function that provides the expansion of
   * the two input variables into the polynomial equation.
   * See BasisFunction for more information.
   *
   * @param vars A vector of double values to use for the expansion.
   */
  void PolynomialUnivariate::Expand(const std::vector<double> &vars) {
    p_terms.clear();
    p_terms.push_back(1.0);

    for(int i = 1; i <= p_degree; i++) {
      p_terms.push_back(p_terms[i-1]*vars[0]);
    }
  }


  /**
   * This will take the Derivative with respect to the variable and evaluate at
   * given value.
   *
   * @param [in] value   (const double)  value at which to evaluate derivative
   *
   * @history  2008-01-09  Tracie Sucharski, Original Version
   *
   */
  double PolynomialUnivariate::DerivativeVar(const double value) {

    double derivative = 0;

    for(int i = 1; i < Coefficients(); i++) {
      derivative += i * Coefficient(i) * pow(value, i - 1);
    }
    return derivative;
  }



  /**
   *  Evaluate the derivative of the polynomial defined by the given coefficients
   *  with respect to the coefficient at the given index, at the current value.
   *
   * @param [in]  value      (const double) value at which to evaluate derivative
   * @param [in]  coefIndex  (const int)    The index of the coefficient to
   *                                          differentiate with respect to
   *
   * @return    (double) The derivative evaluated at given value
   *
   */
  double PolynomialUnivariate::DerivativeCoef(const double value,
      const int coefIndex) {
    double derivative;

    if(coefIndex > 0  && coefIndex <= Coefficients()) {
      derivative = pow(value, coefIndex);
    }
    else if(coefIndex == 0) {
      derivative = 1;
    }
    else {
      std::string msg = "Unable to evaluate the derivative of the univariate polynomial for the given "
                    "coefficient index [" + toString(coefIndex) + "]. "
                    "Index is negative or exceeds degree of polynomial ["
                    + toString(Coefficients()) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return derivative;
  }


} // end namespace isis
