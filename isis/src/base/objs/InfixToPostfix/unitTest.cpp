/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "InfixToPostfix.h"

#include <float.h>

#include "Calculator.h"
#include "IException.h"
#include "Preference.h"

using namespace std;
using namespace Isis;

int main(int argc, char *argv[]) {
  Isis::Preference::Preferences(true);

  cout << "-------------------------------------------------------" << endl;
  cout << "Test InfixToPostfix" << endl;

  const int NUM_EQUATIONS = 25;
  QString equations[NUM_EQUATIONS] = {
    "-4",
    "1*2",
    "((1)+(1))",
    "1*2/2-2",
    "sin(5)",
    "sin 5",
    "--sin(-(54+65))",
    "--sin(-54+--65)",
    "2/3^6",
    "atan2(5,--4)",
    "atan2(--5)",
    "atan2(1,2,3)",
    "atan2(1,)",
    "atan2(1,2",
    "999-548-126^2",
    "sin(0)^2",
    "somefunc(5)",
    "3#3",
    "(3)(2)", // Can't detect the problem here yet because f3 and f2 are functions/operators
    "(3)(2)",
    "atan2(1+2/3^(--6), 5^ (tan ( 42 ^ (1 / --264) / 4 ) - 65 ) != 0)",
    "1++2", // This doesn't work
    "1+-2", // This does work, however, because -2 is a negation and not a subtract.
    "(1+3*(4)",
    "(1+3*(4)))"
  };

  InfixToPostfix converter;
  for(int equation = 0; equation < NUM_EQUATIONS; equation ++) {
    cout << endl << endl << equation + 1 << ": Convert '" << equations[equation].toStdString() << "' to postfix" << endl;

    try {
      QString tokenized = converter.tokenizeEquation(equations[equation]);
      cout << "   Tokenized equation: '" << tokenized.toStdString() << "'" << endl;
      QString postfix = converter.convert(equations[equation]);
      cout << "   Postfix: '" << postfix.toStdString() << "'" << endl;
    }
    catch(IException &e) {
      e.print();
    }
  }
}
