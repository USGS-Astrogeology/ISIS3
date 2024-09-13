/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <QDebug>

#include "InlineCalculator.h"
#include "Preference.h"
#include "IException.h"

/**
 * Test class that allows testing protected methods in InlineCalculator
 *
 * @author 2016-02-24 Kristin Berry
 *
 * @internal
 *   @history 2016-02-24 Kristin Berry - Original version (created for #2401).
 *   @history 2016-08-24 Kelvin Rodriguez - moved print statements in compile's
 *                           exception test to better fit the difference between
 *                           printing on Linux and OS X 10.11
 */
class TestInlineCalculator : public Isis::InlineCalculator {
  public:
    TestInlineCalculator() : InlineCalculator() {};
    TestInlineCalculator(const QString &equation) : InlineCalculator(equation) {};
    ~TestInlineCalculator() {};

  typedef Isis::InlineCalculator::FxTypePtr FxTypePtr;

  QString toPostfixWrap(const QString &equation) const{
    return toPostfix(equation);
  }

  bool isScalarWrap(const QString &scalar) {
    return isScalar(scalar);
  }

  bool isVariableWrap(const QString &str) {
    return isVariable(str);
  }

  void piWrap() {
    pi();
    return;
  }

  void eConstantWrap() {
   eConstant();
   return;
  }

  void degreesWrap() {
   degrees();
   return;
  }

  void radiansWrap() {
   radians();
   return;
  }

  void floatModulusWrap()  {
    floatModulus();
    return;
  }

  void scalarWrap(const QVariant &val) {
    scalar(val);
    return;
  }

  void variableWrap(const QVariant &var) {
    variable(var);
    return;
  }

  bool fxExistsWrap(const QString &fxname) const {
    return fxExists(fxname);
  }

  bool orphanTokenHandlerWrap(const QString &token) {
      return orphanTokenHandler(token);
  }
};

int main(int argc, char *argv[]) {
  Isis::Preference::Preferences(true);

  std::cout << "Testing empty constrctor..." << std::endl;
  Isis::InlineCalculator calcEmpty;

  std::cout << "Testing constrctor with argument..." << std::endl;
  const QString equation = "1 + 2";
  TestInlineCalculator calc(equation);

  std::cout << "Testing eval right away" << std::endl;
  QVector<double> thing =  calc.evaluate();

  qDebug() << "The empty size is: " << QString::number(calcEmpty.size());
  qDebug() << "The size is: " << QString::number(calc.size());

  qDebug() << "The empty equation is: "  << calcEmpty.equation();
  qDebug() << "The equation is: "  << calc.equation();

  std::cout << "Does the empty one compile correctly: " << calcEmpty.compile("1 + 2") << std::endl;
  std::cout << "Did this compile correctly: " << calc.compile("1 + 2") << std::endl;
  std::cout << "Testing compile's exception." << std::endl;

  std::cout << "Did this compile correctly: " << std::endl;
  try {
      std::cout << calc.compile("@@@#!#$") << std::endl;
  }
  catch (Isis::IException &e) {
    qDebug() << QString::fromStdString(e.toString());
  }
  std::cout << "This should throw an exception: " << std::endl;
  try {
    std::cout << calc.compile("+ 1 2") << std::endl; //must be infix
  }
  catch (Isis::IException &e) {
    qDebug() << QString::fromStdString(e.toString());
  }

  qDebug() << "1 + 2 to postfix: " <<  calc.toPostfixWrap("1 + 2");
  qDebug() << "(1 + 2) * (3+4) to postfix: " <<  calc.toPostfixWrap("(1+2) * (3+4)");
  qDebug() << "1*2*4*5*0 to postfix: " <<  calc.toPostfixWrap("1*2*4*5*0");
  qDebug() << "1+2 * 3+4 to postfix: " <<  calc.toPostfixWrap("1+2 * 3+4");
  qDebug() << "1+a *4 to postfix: " <<  calc.toPostfixWrap("1+a * 4");

  std::cout << "IsScalar: " << calc.isScalarWrap("") << std::endl;
  std::cout << "IsScalar: " << calc.isScalarWrap("1") << std::endl;
  std::cout << "IsScalar: " << calc.isScalarWrap("b") << std::endl;
  std::cout << "IsScalar: " << calc.isScalarWrap("!") << std::endl;

  std::cout << "IsVariable: " << calc.isVariableWrap("") << std::endl;
  std::cout << "IsVariable: " << calc.isVariableWrap("!") << std::endl;
  std::cout << "IsVariable: " << calc.isVariableWrap("1") << std::endl;
  std::cout << "IsVariable: " << calc.isVariableWrap("b") << std::endl;

  // Test fxExists
  std::cout << "Function doens't exist: " << calc.fxExistsWrap("a") << std::endl;
  std::cout << "Function does exist: " << calc.fxExistsWrap("sin") << std::endl;

  // addFunction is tested by InlineCalculator::initialize(), which is called by the
  // constructor. The excpetion case is not tested.

  // Create vector for testing actual calculation abilities
  QVector<double> v1;

  v1.push_back(1);
  v1.push_back(2);
  v1.push_back(3);

  // Test the variable does not exist exception for InlineCalculator::variable()
  // Can't test the variable does exist case without getting into private methods.
  try {
    calc.variableWrap("dne");
  }
  catch (Isis::IException &e) {
    std::cout << "Variable doesn't exist, as expected." << std::endl;
  }

  // Set up stack to test the float modulus operator, then make sure we've done it.
  std::cout << "Testing modulus operator 9%7..." << std::endl;
  calc.scalarWrap(QString("9"));
  calc.scalarWrap(QString("7"));
  calc.floatModulusWrap();
  std::cout << "calc.PrintTop()" << std::endl;  // 9%7 = 2
  calc.PrintTop();

  // Set up stack and then test radians, degrees
  std::cout << "Testing conversion to degrees and radians..." << std::endl;
  QString ninetyDegrees("90");
  calc.scalarWrap(ninetyDegrees);
  calc.radiansWrap();
  std::cout << "calc.PrintTop()" << std::endl;
  calc.PrintTop();

  QString piOverTwoRadians("1.57");
  calc.scalarWrap(piOverTwoRadians);
  calc.degreesWrap();
  std::cout << "calc.PrintTop()" << std::endl;
  calc.PrintTop();

  // Push pi and e onto the stack and confirm they're there.
  std::cout << "Testing pushing e and pi onto the stack..." << std::endl;
  calc.piWrap();
  std::cout << "calc.PrintTop()" << std::endl;
  calc.PrintTop();

  calc.eConstantWrap();
  std::cout << "calc.PrintTop()" << std::endl;
  calc.PrintTop();

  // Test default value of orphanTokenHandler. Can't test anything else without getting into
  // private methods.
  std::cout << "Orphan Handler should be false:" << calc.orphanTokenHandlerWrap("") << std::endl;

  // Test CalculatorVariablePool class (also in InlineCalculator.h/.cpp)
  std::cout << "Testing CalculatorVariablePool class..." << std::endl;
  Isis::CalculatorVariablePool cvp;
  std::cout << "CVP's default value is true: " << cvp.exists("a") << std::endl;

  // Without getting into private methods we can't test, cvp.value() will always throw an
  // exception.
  try {
    cvp.value("a", 0);
  }
  catch (Isis::IException &e) {
    std::cout << "Expected exception" << std::endl;
  }

  // Without getting into private methods we can't test, cvp.add() will always throw an
  // exception.
  try {
    cvp.add("a", v1);
  }
  catch (Isis::IException &e) {
    std::cout << "Expected add exception" << std::endl;
  }

 // Don't test abstract FxBinder class (also in InlineCalculator.h/.cpp)

 // Create new Calc to test evaluate():
 std::cout << "Testing evaluate(1+2) =..." << std::endl;
 const QString eq = "1 + 2";
 TestInlineCalculator tcalc(eq);
 QVector<double> result = tcalc.evaluate();
 for (int i=0; i < result.size(); i++) {
   qDebug() << QString::number(result[i]);
 }

 // Create new Calc to test evaluate(*calculatorVariablePool);
 std::cout << "Testing evaluate(3*5) with a CalculatorVariablePool..." << std::endl;
 const QString eq2 = "3 * 5";
 TestInlineCalculator cvpCalc(eq2);
 result = cvpCalc.evaluate(new Isis::CalculatorVariablePool());
 for (int i=0; i < result.size(); i++) {
   qDebug() << QString::number(result[i]);
 }
}
