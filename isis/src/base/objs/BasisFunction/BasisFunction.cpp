/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include "BasisFunction.h"
#include "IException.h"
#include "IString.h"

namespace Isis {

  /**
   * Creates a BasisFunction object.
   *
   * @param name Name of the BasisFunction. For example, "affine".
   * @param numVars Number of variables in the equation. For example:
   * @f[
   * x = C1 + C2*y + C3*z
   * @f]
   * has two variables: y and z.
   * @param numCoefs Number of coefficients in the equation. For example:
   * @f[
   * x = C1 + C2*y + C3*z
   * @f]
   * has three coefficients: C1, C2 & C3.
   */
  BasisFunction::BasisFunction(const QString &name, int numVars, int numCoefs) {
    
    p_name = name;
    p_numVars = numVars;
    p_numCoefs = numCoefs;
  }

  
  /**
   * Set the coefficients for the equation.
   *
   * @param coefs A vector of coefficients for the equation.
   */
  void BasisFunction::SetCoefficients(const std::vector<double> &coefs) {
    
    if ( (int)coefs.size() != p_numCoefs ) {
      std::string msg = "Unable to set coefficients vector. The size of the given vector [" 
                    + toString((int)coefs.size()) + "] does not match number of coefficients "
                    "in the basis equation [" + toString(p_numCoefs) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    
    p_coefs = coefs;
  }

  
  /**
   * Compute the equation using the input variables.
   *
   * @param vars A vector of double values to use for the equation. After setting
   * the coefficients, this can be invoked many times to compute output values
   * given input values.
   *
   * @return The output value.
   */
  double BasisFunction::Evaluate(const std::vector<double> &vars) {
    
    if ( (int)vars.size() != p_numVars ) {
      std::string msg = "Unable to evaluate function for the given vector of values. "
                    "The size of the given vector [" 
                    + toString((int)vars.size()) + "] does not match number of variables "
                    "in the basis equation [" + toString(p_numVars) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    Expand(vars);
    
    if ( (int)p_terms.size() != p_numCoefs ) {
      std::string msg = "Unable to evaluate function for the given vector of values. "
                    "The number of terms in the expansion [" 
                    + toString( (int)p_terms.size() ) + "] does not match number of coefficients "
                    "in the basis equation [" + toString(p_numCoefs) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    double result = 0.0;
    
    for (int i = 0; i < p_numCoefs; i++) {
      result += p_coefs[i] * p_terms[i];
    }
    
    return result;
  }
  
  
  /**
   * Compute the equation using the input variable.
   *
   * @param var A single double value to use for the equation.
   *
   * @return The output double value resulting from the equation.
   */
  double BasisFunction::Evaluate(const double &var) {
    
    std::vector<double> vars;
    vars.push_back(var);
    return BasisFunction::Evaluate(vars);
  }

  
  /**
   * This is the function you should replace depending on your needs. It will
   * expand the variables into the terms of the equation. For example,
   * @f[
   * x = C1 + C2*y + C3*z + C4*y*z
   * @f]
   * must be expanded into the p_terms vector as (1.0, y, z, y*z).  Note that
   * the term expansion is not limited, you can use cos, sin, sqrt, abs, etc.
   * This virtual method is automatically invoked by the Evaluate method. We
   * provide a default expansion of p_terms = vars, just a linear combination of
   * the variables.
   *
   * @param vars A vector of double values to use for the expansion.
   */
  void BasisFunction::Expand(const std::vector<double> &vars) {
    p_terms = vars;
  }
}
