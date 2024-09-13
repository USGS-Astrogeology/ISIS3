/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "PolynomialBivariate.h"
#include <iostream>
#include "Preference.h"

using namespace Isis;
using namespace std;

int main() {
  Preference::Preferences(true);

  PolynomialBivariate b(1);
  vector<double> coefs;
  coefs.push_back(0.5);
  coefs.push_back(0.5);
  coefs.push_back(0.5);
  b.SetCoefficients(coefs);

  cout << "Name   = " << b.Name().toStdString() << endl;
  cout << "Ncoefs = " << b.Coefficients() << endl;
  cout << "Vars   = " << b.Variables() << endl;
  for(int i = 0; i < b.Coefficients(); i++) {
    cout << b.Coefficient(i) << endl;
  }

  cout << "---" << endl;
  vector<double> vars;
  vars.push_back(2.0);
  vars.push_back(3.0);
  cout << b.Evaluate(vars) << endl;
  for(int i = 0; i < b.Coefficients(); i++) {
    cout << b.Term(i) << endl;
  }

  cout << "---" << endl;
  vars[0] = 1.0;
  vars[1] = -2.0;
  cout << b.Evaluate(vars) << endl;
  for(int i = 0; i < b.Coefficients(); i++) {
    cout << b.Term(i) << endl;
  }

  // Test 2nd order
  // 0.5 + 0.5*v1 + 0.5*v2 + 1.0*v1*v1 + 1.0*v1*v2 + 1.0*v2*v2
  PolynomialBivariate c(2);
  cout << "---- 2nd order ----" << endl;
  cout << "Name   = " << c.Name().toStdString() << endl;
  cout << "Ncoefs = " << c.Coefficients() << endl;
  cout << "Vars   = " << c.Variables() << endl;
  coefs.push_back(1.0);
  coefs.push_back(1.0);
  coefs.push_back(1.0);
  c.SetCoefficients(coefs);
  cout << c.Evaluate(vars) << endl;
  for(int i = 0; i < c.Coefficients(); i++) {
    cout << c.Term(i) << endl;
  }
}
