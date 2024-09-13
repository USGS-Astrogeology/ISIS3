/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "CubeInfixToPostfix.h"

#include <float.h>

#include "Calculator.h"
#include "IException.h"
#include "Preference.h"

using namespace std;
using namespace Isis;

int main(int argc, char *argv[]) {
  Isis::Preference::Preferences(true);
  cout << "-------------------------------------------------------" << endl;
  cout << "Test CubeInfixToPostfix" << endl;

  const int NUM_EQUATIONS = 25;
  QString equations[NUM_EQUATIONS] = {
    "-4",
    "1*2",
    "((1)+(1))",
    "1*2/2-2",
    "sin(5)",
    "sin 5",
    "--sin(-(f54+f65()))",
    "--sin(-f54+--f65)",
    "2/3^6",
    "atan2(5,--4)",
    "atan2(--5)",
    "atan2(1,2,3)",
    "atan2(1,)",
    "atan2(1,2",
    "f999-f548-f126^2",
    "sin(0)^2",
    "somefunc(5)",
    "3#3",
    "(f3)(f2)", // Can't detect the problem here yet because f3 and f2 are functions/operators
    "(3)(2)",
    "atan2(1+2/3^(--6), 5^ (tan ( 42 ^ (f1 / --f264) / 4 ) - 65 ) != 0)",
    "1++2", // This doesn't work
    "1+-2", // This does work, however, because -2 is a negation and not a subtract.
    "(1+3*(4)",
    "(1+3*(4)))"
  };

  CubeInfixToPostfix converter;
  for(int equation = 0; equation < NUM_EQUATIONS; equation ++) {
    cout << endl << endl << equation + 1 << ": Convert '" << equations[equation].toStdString() << "' to postfix" << endl;

    try {
      IString tokenized = converter.tokenizeEquation(equations[equation]).toStdString();
      cout << "   Tokenized equation: '" << tokenized << "'" << endl;
      IString postfix = converter.convert(equations[equation]).toStdString();
      cout << "   Postfix: '" << postfix << "'" << endl;
    }
    catch(IException &e) {
      e.print();
    }
  }
}
