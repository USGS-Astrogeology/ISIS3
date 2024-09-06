/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>

#include <QRegExp>
#include <QStack>
#include <QVector>

#include "Calculator.h"
#include "InfixToPostfix.h"
#include "IException.h"
#include "SpecialPixel.h"

using namespace std;

namespace Isis {
  /**
   * The code that performs math operations is designed
   *   to call a function and use the result. These helper
   *   methods convert standard operators into functions which
   *   perform the desired operations. See the implementation of
   *   Calculator::Negative for an example.
   */


  //! Constructor
  Calculator::Calculator() {
    p_valStack = NULL;

    p_valStack = new QStack< QVector<double> >();
  }

  //! Destructor
  Calculator::~Calculator() {
    if (p_valStack) {
      delete p_valStack;
      p_valStack = NULL;
    }
  }

  /**
   * Returns the nagative of the input parameter.
   *
   * @param a Input double
   *
   * @return double negative of a
   */
  double NegateOperator(double a) {
    return -1 * a;
  }


  /**
   * Returns the result of a multiplied by b.
   *
   * @param a Input double
   * @param b Input double
   *
   * @return double result of a*b
   */
  double MultiplyOperator(double a, double b) {
    return a * b;
  }


  /**
   * Returns the result of dividing a by b
   *
   * @param a Input double
   * @param b Intput double
   *
   * @return double result of a/b
   */
  double DivideOperator(double a, double b) {
    return a / b;
  }


  /**
   * Returns the result of additing a with b.
   *
   * @param a Input double
   * @param b Input double
   *
   * @return double result of a+b
   */
  double AddOperator(double a, double b) {
    return a + b;
  }


  /**
   * Returns the result of subtracting b from a.
   *
   * @param a Input subtractee
   * @param b Input subtractor
   *
   * @return double result of a-b
   */
  double SubtractOperator(double a, double b) {
    return a - b;
  }


  /**
   * Returns 1.0 if a is greater than b.  Otherwise 0.0 is returned.
   *
   * @param a Input double
   * @param b Input double
   *
   * @return 1.0 if a>b
   */
  double GreaterThanOperator(double a, double b) {
    return a > b ? 1.0 : 0.0;
  }


  /**
   * Returns 1.0 if a is less than b.  Otherwise 0.0 is returned.
   *
   * @param a Input double
   * @param b Input double
   *
   * @return 1.0 if a<b
   */
  double LessThanOperator(double a, double b) {
    return a < b ? 1.0 : 0.0;
  }


  /**
   * Returns 1.0 if a is equal ot b.
   *
   * @param a Input double
   * @param b Input double
   *
   * @return 1.0 if a==b
   */
  double EqualOperator(double a, double b) {
    return a == b ? 1.0 : 0.0;
  }


  /**
   * Returns 1.0 if a is greater than or equal to b.  Otherwise 0.0 is returned.
   *
   * @param a Input double
   * @param b Input double
   *
   * @return 1.0 if a>=b
   */
  double GreaterThanOrEqualOperator(double a, double b) {
    return a >= b ? 1.0 : 0.0;
  }


  /**
   * Returns 1.0 if a is less than or eqaul to b.  Otherwise 0.0 is returned.
   *
   * @param a Input double
   * @param b Input double
   *
   * @return 1.0 if a<=b
   */
  double LessThanOrEqualOperator(double a, double b) {
    return a <= b ? 1.0 : 0.0;
  }


  /**
   * Returns 1.0 is a is not equal to b.  Otherwise 0.0 is returned.
   *
   * @param a Input double
   * @param b Input double
   *
   * @return 1.0 if a!=b
   */
  double NotEqualOperator(double a, double b) {
    return a != b ? 1.0 : 0.0;
  }


  /**
   * Returns the cosecant of the input a
   *
   * @param a Input double
   *
   * @return the double result of the cosecant of a
   */
  double CosecantOperator(double a) {
    return 1.0 / sin(a);
  }


  /**
   * Returns the secant of the input a
   *
   * @param a Input double
   *
   * @return the double result of the secant of a
   */
  double SecantOperator(double a) {
    return 1.0 / cos(a);
  }

  /**
   * Returns the cotangent of the input a
   *
   * @param a Input double
   *
   * @return the double result of the cotangent of a
   */
  double CotangentOperator(double a) {
    return 1.0 / tan(a);
  }


  /**
   * Returns the result of rounding the input a to the closest integer.
   *
   * @param a Inut double
   *
   * @return the int result of rounding a to the closest whole number
   */
  int Round(double a) {
    return (a > 0) ? (int)(a + 0.5) : (int)(a - 0.5);
  }


  /**
   * Returns the result of a bitwise AND accross a and b
   *
   * @param a Input double
   * @param b Input double
   *
   * @return the double result of a bitwise AND operation
   */
  double BitwiseAndOperator(double a, double b) {
    return (double)(Round(a)&Round(b));
  }


  /**
   * Returns the result of a bitwise OR across a and b
   *
   * @param a Input double
   * @param b INput double
   *
   * @return the double result of a bitwise OR operation
   */
  double BitwiseOrOperator(double a, double b) {
    return (double)(Round(a) | Round(b));
  }


  /**
   * Returns the modulus of a by b
   *
   * @param a Input modulee
   * @param b Input modulator
   *
   * @return double result of a%b
   */
  double ModulusOperator(double a, double b) {
    return (double)(Round(a) % Round(b));
  }

  /**
   * Returns the max of a and b
   *
   * @param a First input value
   * @param b Second input value
   *
   * @return The larger of the two
   */
  double MaximumOperator(double a, double b) {

    if (std::isnan(a)) return (a);
    if (std::isnan(b)) return (b);
    return (a > b) ? a : b;
  }

  /**
   * Returns the min of a and b
   *
   * @param a First input value
   * @param b Second input value
   *
   * @return The smaller of the two
   */
  double MinimumOperator(double a, double b) {
    if (std::isnan(a)) return (a);
    if (std::isnan(b)) return (b);
    return (a < b) ? a : b;
  }



  /**
   * Pops an element, negates it, then pushes the result
   */
  void Calculator::Negative() {
    QVector<double> result = Pop();
    PerformOperation(result, result.begin(), result.end(), NegateOperator);
    Push(result);
  }


  /**
   * Pops two elements, multiplies them, then pushes the product on the stack
   */
  void Calculator::Multiply() {
    QVector<double> y = Pop();
    QVector<double> x = Pop();
    QVector<double> result;

    PerformOperation(result, x.begin(), x.end(), y.begin(), y.end(), MultiplyOperator);
    Push(result);
  }


  /**
   * Pops two elements, adds them, then pushes the sum on the stack
   */
  void Calculator::Add() {
    QVector<double> y = Pop();
    QVector<double> x = Pop();
    QVector<double> result;
    PerformOperation(result, x.begin(), x.end(), y.begin(), y.end(), AddOperator);
    Push(result);
  }


  /**
   * Pops two elements, subtracts them, then pushes the difference on the stack
   */
  void Calculator::Subtract() {
    QVector<double> y = Pop();
    QVector<double> x = Pop();
    QVector<double> result;
    PerformOperation(result, x.begin(), x.end(), y.begin(), y.end(), SubtractOperator);
    Push(result);
  }


  /**
   * Pops two, divides them, then pushes the quotient on the stack
   */
  void Calculator::Divide() {
    QVector<double> y = Pop();
    QVector<double> x = Pop();
    QVector<double> result;

    PerformOperation(result, x.begin(), x.end(), y.begin(), y.end(), DivideOperator);
    Push(result);
  }

  /**
   * Pops two elements, mods them, then pushes the result on the stack
   */
  void Calculator::Modulus() {
    QVector<double> y = Pop();
    QVector<double> x = Pop();
    QVector<double> result;

    PerformOperation(result, x.begin(), x.end(), y.begin(), y.end(), ModulusOperator);
    Push(result);
  }


  /**
   * Pops two elements, computes the power then pushes the result on the stack
   * The exponent has to be a scalar.
   *
   * @throws Isis::iException::Math
   */
  void Calculator::Exponent() {
    QVector<double> exponent = Pop();
    QVector<double> x = Pop();
    QVector<double> result;

    PerformOperation(result, x.begin(), x.end(), exponent.begin(), exponent.end(), pow);
    Push(result);
  }


  /**
   * Pop an element, compute its square root, then push the root on the stack
   *
   * @throws Isis::iException::Math
   */
  void Calculator::SquareRoot() {
    QVector<double> result = Pop();
    PerformOperation(result, result.begin(), result.end(), sqrt);
    Push(result);
  }


  /**
   * Pop an element, compute its absolute value, then push the result on the stack
   */
  void Calculator::AbsoluteValue() {
    QVector<double> result = Pop();
    PerformOperation(result, result.begin(), result.end(), fabs);
    Push(result);
  }


  /**
   * Pop an element, compute its log, then push the result on the stack
   *
   * @throws Isis::iException::Math
   */
  void Calculator::Log() {
    QVector<double> result = Pop();
    PerformOperation(result, result.begin(), result.end(), log);
    Push(result);
  }


  /**
   * Pop an element, compute its base 10 log, then push the result on the stack
   */
  void Calculator::Log10() {
    QVector<double> result = Pop();
    PerformOperation(result, result.begin(), result.end(), log10);
    Push(result);
  }


  /**
   * Pop the top element, then perform a left shift with zero fill
   *
   * @throws Isis::iException::Math
   */
  void Calculator::LeftShift() {
    QVector<double> y = Pop();
    if (y.size() != 1) {
      IString msg = "When trying to do a left shift calculation, a non-scalar "
                    "shift value was encountered. Shifting requires scalars.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    else {
      QVector<double> x = Pop();

      if ((int)y[0] > (int)x.size()) {
        IString msg = "When trying to do a left shift calculation, a shift "
                      "value greater than the data size was encountered. "
                      "Shifting by this value would erase all of the data.";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
      else {
        QVector<double> result;
        int shift = (int)y[0];
        result.resize(x.size());

        for (int i = 0; i < result.size(); i++) {
          if (i + shift < x.size() && i + shift >= 0)
            result[i] = x[i+shift];
          else
            result[i] = sqrt(-1.0); // create a NaN
        }

        Push(result);
      }
    }
  }


  /**
   * Pop the top element, then perform a right shift with zero fill
   *
   * @throws Isis::iException::Math
   */
  void Calculator::RightShift() {
    QVector<double> y = Pop();
    if (y.size() != 1) {
      IString msg = "When trying to do a right shift calculation, a non-scalar "
                    "shift value was encountered. Shifting requires scalars.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    else {
      QVector<double> x = Pop();

      if ((int)y[0] > (int)x.size()) {
        IString msg = "When trying to do a right shift calculation, a shift "
                      "value greater than the data size was encountered. "
                      "Shifting by this value would erase all of the data.";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
      else {
        QVector<double> result;
        int shift = (int)y[0];
        result.resize(x.size());

        for (int i = 0; i < (int)result.size(); i++) {
          if (i - shift < (int)x.size() && i - shift >= 0) {
            result[i] = x[i-shift];
          }
          else {
            result[i] = sqrt(-1.0); // create a NaN
          }
        }

        Push(result);
      }
    }
  }

  /**
   * Pop one element, then push the minimum on the stack
   */
  void Calculator::MinimumLine() {
    QVector<double> result = Pop();

    double minVal = result[0];
    for (int i = 0; i < result.size(); i++) {
      if (!IsSpecial(result[i])) {
        minVal = min(minVal, result[i]);
      }
    }

    result.clear();
    result.push_back(minVal);
    Push(result);
  }


  /**
   * Pop one element, then push the maximum on the stack
   */
  void Calculator::MaximumLine() {
    QVector<double> result = Pop();

    double maxVal = result[0];
    for (int i = 0; i < result.size(); i++) {
      if (!IsSpecial(result[i])) {
        maxVal = max(maxVal, result[i]);
      }
    }

    result.clear();
    result.push_back(maxVal);
    Push(result);
  }


  /**
   * Pop two elements, then push the minimum on a pixel by pixel
   * basis back on the stack
   */
  void Calculator::MinimumPixel() {
    QVector<double> x = Pop();
    QVector<double> y = Pop();
    QVector<double> result;

    PerformOperation(result, x.begin(), x.end(), y.begin(), y.end(),
                     MinimumOperator);
    Push(result);
  }


  /**
   * Pop two elements, then push the maximum on a pixel by pixel
   * basis back on the stack
   */
  void Calculator::MaximumPixel() {
    QVector<double> x = Pop();
    QVector<double> y = Pop();
    QVector<double> result;

    PerformOperation(result, x.begin(), x.end(), y.begin(), y.end(),
                     MaximumOperator);
    Push(result);
  }


  /**
   * Pop two elements off the stack and compare them to see where one is greater
   * than the other, then push the results on the stack.
   */
  void Calculator::GreaterThan() {
    QVector<double> y = Pop();
    QVector<double> x = Pop();
    QVector<double> result;

    PerformOperation(result, x.begin(), x.end(), y.begin(), y.end(),
                     GreaterThanOperator);
    Push(result);
  }


  /**
   * Pop two elements off the stack and compare them to see where one is less
   * than the other, then push the results on the stack.
   */
  void Calculator::LessThan() {
    QVector<double> y = Pop();
    QVector<double> x = Pop();
    QVector<double> result;

    PerformOperation(result, x.begin(), x.end(), y.begin(), y.end(),
                     LessThanOperator);
    Push(result);
  }


  /**
   * Pop two elements off the stack and compare them to see where one is equal
   * to the other, then push the results on the stack.
   */
  void Calculator::Equal() {
    QVector<double> y = Pop();
    QVector<double> x = Pop();
    QVector<double> result;

    PerformOperation(result, x.begin(), x.end(), y.begin(), y.end(),
                     EqualOperator);
    Push(result);
  }


  /**
   * Pop two elements off the stack and compare them to see where one is greater
   * than or equal to the other, then push the results on the stack.
   */
  void Calculator::GreaterThanOrEqual() {
    QVector<double> y = Pop();
    QVector<double> x = Pop();
    QVector<double> result;

    PerformOperation(result, x.begin(), x.end(), y.begin(), y.end(),
                     GreaterThanOrEqualOperator);
    Push(result);
  }


  /**
   * Pop two elements off the stack and compare them to see where one is less
   * than or equal to the other, then push the results on the stack.
   */
  void Calculator::LessThanOrEqual() {
    QVector<double> y = Pop();
    QVector<double> x = Pop();
    QVector<double> result;

    PerformOperation(result, x.begin(), x.end(), y.begin(), y.end(),
                     LessThanOrEqualOperator);
    Push(result);
  }


  /**
   * Pop two elements off the stack and compare them to see where one is not
   * equal to the other, then push the results on the stack.
   */
  void Calculator::NotEqual() {
    QVector<double> y = Pop();
    QVector<double> x = Pop();
    QVector<double> result;

    PerformOperation(result, x.begin(), x.end(), y.begin(), y.end(),
                     NotEqualOperator);
    Push(result);
  }


  // Commented out because bitwise ops only work with integers instead of
  // doubles

  /**
   * Pop two elements, AND them, then push the result on the stack
   */
  void Calculator::And() {
    QVector<double> y = Pop();
    QVector<double> x = Pop();
    QVector<double> result;

    PerformOperation(result, x.begin(), x.end(), y.begin(), y.end(),
                     BitwiseAndOperator);
    Push(result);
  }


  /**
   * Pop two elements, OR them, then push the result on the stack
   */
  void Calculator::Or() {
    QVector<double> y = Pop();
    QVector<double> x = Pop();
    QVector<double> result;

    PerformOperation(result, x.begin(), x.end(), y.begin(), y.end(),
                     BitwiseOrOperator);
    Push(result);
  }


  /**
   * Pops one element  and push the sine
   */
  void Calculator::Sine() {
    QVector<double> result = Pop();
    PerformOperation(result, result.begin(), result.end(), sin);
    Push(result);
  }


  /**
   * Pops one element  and push the cosine
   */
  void Calculator::Cosine() {
    QVector<double> result = Pop();
    PerformOperation(result, result.begin(), result.end(), cos);
    Push(result);
  }


  /**
   * Pops one element  and push the tangent
   */
  void Calculator::Tangent() {
    QVector<double> result = Pop();
    PerformOperation(result, result.begin(), result.end(), tan);
    Push(result);
  }


  /**
   * Pops one element  and push the cosecant
   */
  void Calculator::Cosecant() {
    QVector<double> result = Pop();
    PerformOperation(result, result.begin(), result.end(), CosecantOperator);
    Push(result);
  }


  /**
   * Pops one element  and push the secant
   */
  void Calculator::Secant() {
    QVector<double> result = Pop();
    PerformOperation(result, result.begin(), result.end(), SecantOperator);
    Push(result);
  }


  /**
   * Pops one element  and push the cotangent
   */
  void Calculator::Cotangent() {
    QVector<double> result = Pop();
    PerformOperation(result, result.begin(), result.end(), CotangentOperator);
    Push(result);
  }


  /**
   * Pops one element  and push the arcsine
   */
  void Calculator::Arcsine() {
    QVector<double> result = Pop();
    PerformOperation(result, result.begin(), result.end(), asin);
    Push(result);
  }


  /**
   * Pops one element  and push the arccosine
   */
  void Calculator::Arccosine() {
    QVector<double> result = Pop();
    PerformOperation(result, result.begin(), result.end(), acos);
    Push(result);
  }


  /**
   * Pops one element  and push the arctangent
   */
  void Calculator::Arctangent() {
    QVector<double> result = Pop();
    PerformOperation(result, result.begin(), result.end(), atan);
    Push(result);
  }


  /**
   * Pops one element and push the inverse hyperbolic sine
   */
  void Calculator::ArcsineH() {
    QVector<double> result = Pop();
    PerformOperation(result, result.begin(), result.end(), asinh);
    Push(result);
  }


  /**
   * Pops one element and push the inverse hyperbolic cosine
   */
  void Calculator::ArccosineH() {
    QVector<double> result = Pop();
    PerformOperation(result, result.begin(), result.end(), acosh);
    Push(result);
  }


  /**
   * Pops one element and push the inverse hyperbolic tangent
   */
  void Calculator::ArctangentH() {
    QVector<double> result = Pop();
    PerformOperation(result, result.begin(), result.end(), atanh);
    Push(result);
  }


  /**
   * Pops two elements  and push the arctangent
   */
  void Calculator::Arctangent2() {
    QVector<double> y = Pop();
    QVector<double> x = Pop();
    QVector<double> result;

    PerformOperation(result, x.begin(), x.end(), y.begin(), y.end(), atan2);
    Push(result);
  }


  /**
   * Pops one element  and push the hyperbolic sine
   */
  void Calculator::SineH() {
    QVector<double> result = Pop();
    PerformOperation(result, result.begin(), result.end(), sinh);
    Push(result);
  }


  /**
   * Pops one element  and push the hyperbolic cosine
   */
  void Calculator::CosineH() {
    QVector<double> result = Pop();
    PerformOperation(result, result.begin(), result.end(), cosh);
    Push(result);
  }


  /**
   * Pops one element  and push the hyperbolic tangent
   */
  void Calculator::TangentH() {
    QVector<double> result = Pop();
    PerformOperation(result, result.begin(), result.end(), tanh);
    Push(result);
  }


  // Stack methods

  /**
   * Get the current stack size
   * 
   * @return int Number of arguments in the stack
   */
  int Calculator::StackSize() {
    return p_valStack->size();
  }

  /**
   * Push a vector onto the stack
   *
   * @param vect The vector that will be pushed on the stack
   */
  void Calculator::Push(QVector<double> &vect) {
    p_valStack->push(vect);
  }


  /**
   * Push a scalar onto the stack
   *
   * @param scalar The scalar that will be pushed on the stack
   */
  void Calculator::Push(double scalar) {
    QVector<double> s;
    s.push_back(scalar);
    Push(s);
  }


  /**
   * Push a buffer onto the stack
   *
   * @param buff The buffer that will be pushed on the stack
   */
  void Calculator::Push(Buffer &buff) {
    QVector<double> b(buff.size());

    for (int i = 0; i < buff.size(); i++) {
      // Test for special pixels and map them to valid values
      if (IsSpecial(buff[i])) {
        if (Isis::IsNullPixel(buff[i])) {
          //b[i] = NAN;
          b[i] = sqrt(-1.0);
        }
        else if (Isis::IsHrsPixel(buff[i])) {
          //b[i] = INFINITY;
          b[i] = DBL_MAX * 2;
        }
        else if (Isis::IsHisPixel(buff[i])) {
          //b[i] = INFINITY;
          b[i] = DBL_MAX * 2;
        }
        else if (Isis::IsLrsPixel(buff[i])) {
          //b[i] = -INFINITY;
          b[i] = -DBL_MAX * 2;
        }
        else if (Isis::IsLisPixel(buff[i])) {
          //b[i] = -INFINITY;
          b[i] = -DBL_MAX * 2;
        }
      }
      else
        b[i] = buff[i];
    }

    Push(b);
  }


  /**
   * Pop an element off the stack
   *
   * @param keepSpecials If true, special pixels will be
   *                     preserved; otherwise, they will be mapped
   *                     to double values
   *
   * @return The top of the stack, which gets popped
   */
  QVector<double> Calculator::Pop(bool keepSpecials) {
    QVector<double> top;

    if (p_valStack->empty()) {
      IString msg = "Math calculator stack is empty, cannot perform any "
                    "more operations.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    top = p_valStack->top();

    if (keepSpecials) {
      for (int i = 0; i < (int)top.size(); i++) {
        if (std::isnan(top[i])) {
          top[i] = Isis::Null;
        }
        // Test for +INFINITY
        else if (top[i] > DBL_MAX) {
          top[i] = Isis::Hrs;
        }
        // Test for -INFINITY)
        else if (top[i] < -DBL_MAX) {
          top[i] = Isis::Lrs;
        }
        else {
          // Do nothing
        }
      }
    }

    p_valStack->pop();


    return top;
  }


  /**
   * Print the vector at the top of the stack
   */
  void Calculator::PrintTop() {
    if (p_valStack->empty()) return;

    QString temp;
    temp += "[ ";
    QVector<double> top = p_valStack->top();
    for (int i = 0; i < (int)top.size(); i++) {
      temp += QString::number(top[i]);
      temp += " ";
    }
    temp += "]";
    // On some operating systems, -nan was being outputted. 
    // Because this method is only used as a cout in our tests, we do not 
    // care about the difference between nan and -nan; they are the same in this case.
    temp.replace(QRegExp("-nan"), "nan");
    std::cout<<temp.toStdString()<<std::endl;
  }


  /**
   * Check if the stack is empty
   *
   * @return bool True if the stack is empty
   */
  bool Calculator::Empty() {
    return p_valStack->empty();
  }


  /**
   * Clear out the stack
   */
  void Calculator::Clear() {
    while (!p_valStack->empty()) {
      p_valStack->pop();
    }
  }


  /**
   * Performs the mathematical operations on each argument.
   *
   * @param results [out] The results of the performed operation
   * @param arg1Start The first argument to have the operation done on
   * @param arg1End One argument beyond the final argument to have the operation
   *                done upon
   * @param operation The operation to be done on all arguments
   */
  void Calculator::PerformOperation(QVector<double> &results,
                                    QVector<double>::iterator arg1Start,
                                    QVector<double>::iterator arg1End,
                                    double operation(double)) {
    results.resize(arg1End - arg1Start);

    for (int pos = 0; pos < results.size(); pos++) {
      results[pos] = operation(*arg1Start);

      arg1Start++;
    }
  }


  /**
   * Performs the mathematical operation on each pair of arguments, or a set of
   * agruments against a single argument.
   *
   * @param results [out] The results of the performed operation
   * @param arg1Start The first of the primary argument to have the operation done
   *                  on
   * @param arg1End One arguement beyond the final primary argument to have the
   *                operation done upon
   * @param arg2Start The first of the secondaty argument to have the operation
   *                  done on
   * @param arg2End One arguement beyond the final secondary argument to have the
   *                operation done upon
   * @param operation The operation to be done on all pairs of arguments
   */
  void Calculator::PerformOperation(QVector<double> &results,
                                    QVector<double>::iterator arg1Start,
                                    QVector<double>::iterator arg1End,
                                    QVector<double>::iterator arg2Start,
                                    QVector<double>::iterator arg2End,
                                    double operation(double, double)) {
    if (arg1End - arg1Start != 1 && arg2End - arg2Start != 1 &&
        arg1End - arg1Start != arg2End - arg2Start) {
      IString msg = "The stack based calculator cannot operate on vectors "
                    "of differing sizes.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    int iSize = max(arg1End - arg1Start, arg2End - arg2Start);
    results.resize(iSize);

    for (int pos = 0; pos < results.size(); pos++) {
      results[pos] = operation(*arg1Start, *arg2Start);

      if (arg1Start + 1 != arg1End) arg1Start++;
      if (arg2Start + 1 != arg2End) arg2Start++;
    }
  }
} // End of namespace Isis
