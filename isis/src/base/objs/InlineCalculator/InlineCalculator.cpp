/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "InlineCalculator.h"

// std library
#include <cmath>

// Qt library
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>

// boost library
#include <boost/foreach.hpp>

// naif library
#include <SpiceUsr.h>
#include <SpiceZfc.h>
#include <SpiceZmc.h>

// other ISIS
#include "IException.h"
#include "InlineInfixToPostfix.h"
#include "IString.h"
#include "NaifStatus.h"

using namespace std;

namespace Isis {

  /**
   * Constructs an InlineCalculator object by initializing the operator lookup
   * list.
   */  
  InlineCalculator::InlineCalculator() : Calculator() {
    initialize();
  }
 

  /**
   * Constructs an InlineCalculator object by initializing the operator lookup
   * list and compiles the given equation to prepare for evaluation.
   *  
   * @param equation A string representing an equation in infix format.
   */  
  InlineCalculator::InlineCalculator(const QString &equation) : Calculator() {
    initialize();
    compile(equation);
  }
 

  /**
   * Destroys the InlineCalculator object.
   */  
  InlineCalculator::~InlineCalculator() {
    destruct();
  }
 
 
  /**
   * Accesses the number of functions, operators, variables, and scalars to be
   * executed.
   *  
   * @return int The number of functions, operators, variables, and scalars
   *         stored in the Calculator.
   */  
  int InlineCalculator::size() const {
    return (m_functions.size());
  }
 
 
  /**
   * Accesses the string representation of the current equation, in postfix
   * format. This equation is most commonly entered using infix, but it is
   * reformatted to postfix when set.
   *  
   * @return QString A string representing the equation, in postfix.
   */  
  QString InlineCalculator::equation() const {
    return (m_equation);
  }
 
 
  /**
   * @brief Compiles the given equation for evaluation.
   *  
   * This method first converts the given infix equation into a postfix
   * equation for evaluation and saves the postfix formatted string. It then
   * ensures that the equation is ready for evaluation by parsing and
   * verifying that all tokens are recognized.
   *  
   * @param equation A string representing an equation to be compiled, in
   *                 infix format.
   *
   * @return bool Indicates whether the compilation was successful.
   * @throw IException::User "Error parsing inline equation. Equation
   *        element invalid - token not recognized."
   *  
   */
  bool InlineCalculator::compile(const QString &equation) {
    //  Transform equation to postfix order
 
    QString tokenOps = toPostfix(equation);
    tokenOps = tokenOps.simplified();

    int nerrors = 0;
    std::string error =  "Errors parsing inline equation[" + equation.toStdString() + "].";
    IException errList(IException::User, error, _FILEINFO_);
 
    Clear();  // Clear the stack
    m_equation = equation;
    m_functions.clear();  // Clear function list
 
    QStringList tokenList = tokenOps.split(" ");
    while ( !tokenList.isEmpty() ) {
      QString token = tokenList.takeFirst();
      if ( !token.isEmpty() ) {
         
        // See if the function already exists.  Note that scalars and variables
        // that are already present can safely be reused.  New occuring ones are
        // created fresh!
        FxTypePtr fx = find(token);
        if ( 0 != fx ) {
          m_functions.push_back(fx);
        }
        // New scalars and variables will create new unique function objects if
        // they do not already exist (they would be found above then)
        else if ( isScalar(token)  ) {
          fx = addFunction(new ParameterFx(token, &InlineCalculator::scalar, this));
          m_functions.push_back(fx);
        }
        else if ( isVariable(token)  ) {
          // Will also get line, sample, band, etc...
          fx = addFunction(new ParameterFx(token, &InlineCalculator::variable, this));
          m_functions.push_back(fx);
        }
        else {
            //  Parameter not recognized during compile.  All unknown tokens are
            //  assumed to be variables until run time when they are searched for
            //  in the current state of the resource pool.
          try {
            if ( !orphanTokenHandler(token) ) {
              error = "Equation element (" + token.toStdString() + ") invalid - token not recognized.";
              errList.append(IException(IException::User, error, _FILEINFO_));
              nerrors++;
            }
          }
          // Catch all failures from orphaned tokens
          catch (IException &e) {
            errList.append(e);
            nerrors++;
          }
        }
      }
    }
 
    //  Might want to make this optional here
    if (nerrors > 0) {  
      throw errList;
    }
    return (nerrors == 0);
  }
 

  /**
   * @brief Evaluate with a variable pool
   *  
   * This method accepts a varible resource pool, stores it off for evaluation
   * and then executes the operands to provide the result of the equation.  It
   * stores the variable pool for subsequent runs that should yield identical
   * results when operator() is invoked.
   *
   * @param variablePool Keyword resource for variables substituted at run time
   *                     of the equation
   *
   * @return QVector \< double \> Result of the stored equation given the
   *         resource.
   * @throw IException::Programmer "Calculation with variable pool failed."
   *  
   */
  QVector<double> InlineCalculator::evaluate(CalculatorVariablePool *variablePool) {
    QVector<double> value;
    try {
      pushVariables(variablePool);
      value = evaluate();
      popVariables();
    }
    catch (IException &ie) {
      popVariables();
      throw IException(ie, IException::Programmer,
                       "Calculation with variable pool failed.",
                       _FILEINFO_);
    }
    return (value);
  }
 

  /**
   * @brief Evaluate compiled equation with existing variable pool
   *  
   * This executes the operands to provide the result of the equation.  The
   * currently stored variable pool, refernenced via variables(), is used to
   * provide the variables utilized in the equation.
   *  
   * This is reentrant and as long as the variable pool is unchanged between
   * succesive calls, they all should return the same result.
   *
   * @return QVector \< double \> Result of the stored equation.
   * @throw IException::Unknown "Too many operands in the equation."
   *  
   */
  QVector<double> InlineCalculator::evaluate() {
 
    BOOST_FOREACH (FxTypePtr function,  m_functions) {
      function->execute();
    }
 
    if (StackSize() != 1) {
      std::string msg = "Too many operands in the equation [" + m_equation.toStdString() + "].";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
 
    return (Pop(true));
  }
 
 
  /**
   * Converts the given string from infix to postfix format.
   *  
   * @param equation A string representing an infix equation.
   *  
   * @return QString A string representing the given equation converted to
   *         postfix format.
   */  
  QString InlineCalculator::toPostfix(const QString &equation) const {
    InlineInfixToPostfix parser;
    return (parser.convert(equation));
  }
 
 
  /**
   * Determines whether the given string contains a scalar value (i.e. can be
   * converted to a double precision value).
   *  
   * @param scalar A string to be checked.
   *  
   * @return bool Indicates whether the given string is a scalar value.
   */  
  bool InlineCalculator::isScalar(const QString &scalar) {
    if (scalar.isEmpty())  return (false);
    try {
      Isis::toDouble(scalar.toStdString());
      return (true);
    }
    catch (IException &e) {
      return (false);
    }
  }
 

  /**
   * Determines whether the given string is a variable. If the string is empty
   * or scalar, this method returns false. Otherwise, it is assumed to be a
   * variable and this method returns true.
   *  
   * @param str A string to be checked.
   * @return bool Indicates whether the given string is a variable.
   */  
  bool InlineCalculator::isVariable(const QString &str) {
    if (str.isEmpty()) {
      return (false);
    }
    if (!isScalar(str)) {
      return (true);
    }
    return (false);
  }
 

  /**
   * @brief Pushes the given value onto the stack as a scalar.
   *
   * This method adds the given variant value to the stack as a scalar.
   *
   * @param scalar QVariant containing the scalar. Usually a QString.
   *  
   */  
  void InlineCalculator::scalar(const QVariant &scalar) {
    Push((scalar.toString()).toDouble());
  }
 

  /**
   * @brief Pushes the given value onto the stack as a variable.
   *
   * This method adds the given variant value to the stack as a variable.
   *
   * @param variable QVariant containing the name of the variable. Usually
   *                 a QString.
   *  
   * @throw IException::User "Could not find variable in variable pool."
   */  
  void InlineCalculator::variable(const QVariant &variable) {
    CalculatorVariablePool *variablePool = variables();
    QString key = variable.toString();
    if (variablePool->exists(key)) {
      QVector<double> values = variablePool->value(key);
      Push(values);
      return;
    }
 
    // Error!
    std::string error = "Could not find variable [" + key.toStdString() + "] in variable pool.";
    throw IException(IException::User, error, _FILEINFO_);
  }
 

  /**
   * Determines the remainder of the quotient a/b whose sign is the same as that
   * of a. In other words, this method finds the value r = a - bq such that q is
   * the integer found by truncating a/b.
   *  
   * @param a The dividend (numerator).
   * @param b The divisor (denominator).
   *  
   * @return double The remainder of the quotient that has the same sign
   *         as the dividend.
   */  
  double floatModulusOperator(double a, double b) {
    return (fmod(a, b));
  }
 

  /**
   * Pops the top two vectors off the current stack and performs the
   * floatModulusOperator() on the corresponding components of these vectors.
   * The result is then pushed back onto the stack.
   */  
  void InlineCalculator::floatModulus() {
    QVector<double> y = Pop();
    QVector<double> x = Pop();
    QVector<double> result;
    PerformOperation(result, x.begin(), x.end(), y.begin(), y.end(),
                     floatModulusOperator);
    Push(result);
    return;
  }
 

  /**
   * Pops the top vector off the current stack and converts from degrees to
   * radians. The result is then pushed back onto the stack.
   */  
  void InlineCalculator::radians() {
    QVector<double> degree = Pop();
    QVector<double> result;
    BOOST_FOREACH(double d, degree) {
      result.push_back(d * rpd_c());
    }
    Push(result);
    return;
  }
 

  /**
   * Pops the top vector off the current stack and converts from radians to
   * degrees. The result is then pushed back onto the stack.
   */  
  void InlineCalculator::degrees() {
    QVector<double> radians = Pop();
    QVector<double> result;
    BOOST_FOREACH(double r, radians) {
      result.push_back(r * dpr_c());
    }
    Push(result);
    return;
  }


  /**
   * Pops the top two vectors off the current stack and performs a logical or
   * on each pair.
   * 
   * @throws IException::Unknown "Failed performing logical or operation, "
   *                             "input vectors are of differnet lengths."
   */
  void InlineCalculator::logicalOr() {
    QVector<double> inputA = Pop();
    QVector<double> inputB = Pop();
    QVector<double> results;
    if ( inputA.size() != inputB.size() ) {
      std::string msg = "Failed performing logical or operation, "
                    "input vectors are of differnet lengths.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    for (int i = 0; i < inputA.size(); i++) {
      results.push_back( inputA[i] || inputB[i] );
    }
    Push(results);
    return;
  }


  /**
   * Pops the top two vectors off the current stack and performs a logical and
   * on each pair.
   * 
   * @throws IException::Unknown "Failed performing logical and operation, "
   *                             "input vectors are of differnet lengths."
   */
  void InlineCalculator::logicalAnd() {
    QVector<double> inputA = Pop();
    QVector<double> inputB = Pop();
    QVector<double> results;
    if ( inputA.size() != inputB.size() ) {
      std::string msg = "Failed performing logical and operation, "
                    "input vectors are of differnet lengths.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    for (int i = 0; i < inputA.size(); i++) {
      results.push_back( inputA[i] && inputB[i] );
    }
    Push(results);
    return;
  }
 

  /**
   * Pushes the PI constant onto the current stack.
   */  
  void InlineCalculator::pi() {
    Push(pi_c());
    return;
  }
 

  /**
   * Pushes the Euler constant (e) onto the current stack.
   */  
  void InlineCalculator::eConstant() {
    Push(E);
    return;
  }
 

  /**
   * Determines whether the given function name exists in the current function
   * pool.
   *  
   * @param fxname A string containing the name of the function we are looking
   *               for in the pool.
   *  
   * @return bool Indicates whether the pool contains the given function.
   */  
  bool InlineCalculator::fxExists(const QString &fxname) const {
    return (m_fxPool.contains(fxname));
  }
 

  /**
   * Adds a function to the function pool. Once in the pool, functions cannot
   * be overwritten. This method will throw an exception if we attempt to add
   * a function with the same name as one already in the pool.
   *  
   * @param function The function type pointer to be added to the pool
   *  
   * @return InlineCalculator::FxTypePtr The function type pointer that has been
   *         added to the pool.
   * @throw IException::Programmer "Function operator exists!  Cannot replace
   *                                existing functions in the pool"
   */  
  InlineCalculator::FxTypePtr InlineCalculator::addFunction(InlineCalculator::FxTypePtr function) {
    FxTypePtr func = find(function->name());
    if (!func) {
      m_fxPool.insert(function->name(), function);
    }
    else {
      std::string msg = "Function operator [" + function->name().toStdString() +
                    "] exists!  Cannot replace existing functions in the pool :-(";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return (function);
  }
 

  /**
   * @brief Default token handler if it is undefined during parsing/compilation
   *  
   * This method provides the default handling of an undefined token found while
   * compiling an equation.  Users may code their own handler that, for example,
   * could provide additional further functionality.
   *  
   * If the virtualized reimplementation is unable to process it, it can throw an
   * exception which will be caught, recorded and compiling will continue until
   * all tokens are processed.  If the optional implementation cannot handle it,
   * return false from your version and an exception will be thrown for you.
   *
   * @param token Character representation of the token.  This could be an
   *              undefined constant, function or soemthing else that occurs in
   *              the equation that is not resolved when this is called.
   *
   * @return bool True if it is handled, false if it cannot (which is an error
   *              and an exception will be invoked).
   *
   */
  bool InlineCalculator::orphanTokenHandler(const QString &token) {
    return (false);
  }
 
 
  /**
   * Push the given variable pool onto the current variable pool list.
   *  
   * @param variablePool A pointer to the CalculatorVariablePool object to be
   *                     pushed onto the current list.
   */  
  void InlineCalculator::pushVariables(CalculatorVariablePool *variablePool) {
    m_variablePoolList.push_back(variablePool);
    return;
  }
 

  /**
   * Accesses the last variable pool in the current pool list. If the list is
   * empty, an error will be thrown.
   *  
   * @return CalculatorVariablePool The last variable pool in the current list.
   * @throw IException::Programmer "Request for nonexistent variable pool.",
   */  
  CalculatorVariablePool *InlineCalculator::variables() {
    if ( !m_variablePoolList.isEmpty() ) {
      return (m_variablePoolList.back());
    }
    throw IException(IException::Programmer,
                     "Request for nonexistent variable pool.",
                     _FILEINFO_);
  }
 

  /**
   * Removes the last variable pool in the current variable pool list.
   */  
  void InlineCalculator::popVariables() {
    Clear();
    if ( !m_variablePoolList.isEmpty() ) {
      m_variablePoolList.pop_back();
    }
    return;
  }
 
 
  /**
   * Gets a pointer to the function from the current pool that corresponds
   * to the given function name. This method returns null if no function
   * with the given name is found.
   *
   * @param fxname A string containing the name of the function to be
   *               found.
   * @return InlineCalculator::FxTypePtr A pointer to the corresponding
   *         function type, if found.
   */  
  InlineCalculator::FxTypePtr InlineCalculator::find(const QString &fxname) {
    FxPoolType::iterator fx = m_fxPool.find(fxname);
    if ( m_fxPool.end() == fx ) return NULL;
    return (*fx);
  }
 
 
  /**
   * Adds the recognized functions to the function pool.
   */  
  void InlineCalculator::initialize() {
    //  Set up calculator function lookup list
    addFunction(new VoidFx("^", &Calculator::Exponent, this));
    addFunction(new VoidFx("/", &Calculator::Divide, this));
    addFunction(new VoidFx("*", &Calculator::Multiply, this));
    addFunction(new VoidFx("<<", &Calculator::LeftShift, this));
    addFunction(new VoidFx(">>", &Calculator::RightShift, this));
    addFunction(new VoidFx("+", &Calculator::Add, this));
    addFunction(new VoidFx("-", &Calculator::Subtract, this));
    addFunction(new VoidFx(">", &Calculator::GreaterThan, this));
    addFunction(new VoidFx("<", &Calculator::LessThan, this));
    addFunction(new VoidFx(">=", &Calculator::GreaterThanOrEqual, this));
    addFunction(new VoidFx("<=", &Calculator::LessThanOrEqual, this));
    addFunction(new VoidFx("==", &Calculator::Equal, this));
    addFunction(new VoidFx("!=", &Calculator::NotEqual, this));
 
    //  These are not part of the Calculator class because InfixToPostfix didn't
    //  add/recognize them in the tokenizer.  See InlineInfixToPostfix class which
    //  is part of this calculator.
    addFunction(new VoidFx("&", &Calculator::And, this));
    addFunction(new VoidFx("and", &Calculator::And, this));
    addFunction(new VoidFx("|", &Calculator::Or, this));
    addFunction(new VoidFx("or", &Calculator::Or, this));
    addFunction(new InlineVoidFx("%", &InlineCalculator::floatModulus, this));  
    addFunction(new VoidFx("mod", &Calculator::Modulus, this));
    addFunction(new InlineVoidFx("fmod", &InlineCalculator::floatModulus, this));
     
    addFunction(new VoidFx("--", &Calculator::Negative, this));
    addFunction(new VoidFx("neg", &Calculator::Negative, this));  
 
    addFunction(new VoidFx("min", &Calculator::MinimumPixel, this));
    addFunction(new VoidFx("max", &Calculator::MaximumPixel, this));
    addFunction(new VoidFx("abs", &Calculator::AbsoluteValue, this));
    addFunction(new VoidFx("sqrt", &Calculator::SquareRoot, this));
    addFunction(new VoidFx("log", &Calculator::Log, this));
    addFunction(new VoidFx("ln", &Calculator::Log, this));
    addFunction(new VoidFx("log10", &Calculator::Log10, this));
    addFunction(new InlineVoidFx("pi", &InlineCalculator::pi, this));
 
    addFunction(new VoidFx("sin", &Calculator::Sine, this));
    addFunction(new VoidFx("cos", &Calculator::Cosine, this));
    addFunction(new VoidFx("tan", &Calculator::Tangent, this));
    addFunction(new VoidFx("sec", &Calculator::Secant, this));
    addFunction(new VoidFx("csc", &Calculator::Cosecant, this));
    addFunction(new VoidFx("cot", &Calculator::Cotangent, this));
    addFunction(new VoidFx("asin", &Calculator::Arcsine, this));
    addFunction(new VoidFx("acos", &Calculator::Arccosine, this));
    addFunction(new VoidFx("atan", &Calculator::Arctangent, this));
    addFunction(new VoidFx("atan2", &Calculator::Arctangent2, this));
 
    addFunction(new InlineVoidFx("degs", &InlineCalculator::degrees, this));
    addFunction(new InlineVoidFx("rads", &InlineCalculator::radians, this));
    addFunction(new InlineVoidFx("e", &InlineCalculator::eConstant, this));
    addFunction(new InlineVoidFx("||", &InlineCalculator::logicalOr, this));
    addFunction(new InlineVoidFx("&&", &InlineCalculator::logicalAnd, this));
 
    // Add new functions available for inlining
  //  m_variablePoolList = defaultVariables();
 
    return;
  }
 

  /**
   * Discard of all the function pool and class resources.
   *
   */
  void InlineCalculator::destruct() {
    BOOST_FOREACH (FxTypePtr function,  m_fxPool) {
      delete function;
    }
 
    m_fxPool.clear();
    m_functions.clear();
    return;
  }


  /**
   * Constructs a CalculatorVariablePool object.
   */
  CalculatorVariablePool::CalculatorVariablePool() {
  }


  /**
   * Destroys the CalculatorVariablePool object.
   */
  CalculatorVariablePool::~CalculatorVariablePool() {
  }
 

  /**
   * Returns true so the real error can be reported.
   *  
   * @param variable A string containing the variable we are looking for.
   * @return bool True
   */
  bool CalculatorVariablePool::exists(const QString &variable) const {
    return (true);
  }
 

  /**
   * Return vector of doubles for Calculator functions.
   *  
   * @param variable A string containing the variable.
   * @param index The location in the pool.
   *  
   * @return QVector \< double \> A vector of calculator functions.
   * @throw IException::Programmer "No implementation in Calculator variable pool to provide
   *                                value for variable."
   */
  QVector<double> CalculatorVariablePool::value(const QString &variable,
                                                const int &index) const {
    std::string mess = "No implementation in Calculator variable pool to provide "
                   " value for variable [" + variable.toStdString() + "].";
    throw IException(IException::Programmer, mess, _FILEINFO_);
  }
 

  /**
   * Add a parameter to the variable pool.  Some implementations can take
   * advantage of this if desired but it is not standard.
   *  
   * @param key A string containing the name of the parameter to be
   *            added.
   * @param values A vector of double precision values to be added to the
   *               variable pool.
   *  
   * @throw IException::Programmer "No implementation in Calculator variable pool to add
   *                                value for variable."
   */
  void CalculatorVariablePool::add(const QString &key, QVector<double> &values) {
    std::string mess = "No implementation in Calculator variable pool to add "
                   " value for variable [" + key.toStdString() + "].";
    throw IException(IException::Programmer, mess, _FILEINFO_);
  }


  /**
   * Constructs a function binder given a name.
   *  
   * @param name A string containing a name for this function.
   */
  FxBinder::FxBinder(const QString &name) : m_name(name) {
  }


  /**
   * Destroys the FxBinder object.
   */
  FxBinder::~FxBinder() {
  }
 

  /**
   * The name assigned to this function binder.
   *  
   * @return QString A string containing the name of this function.
   */
  QString FxBinder::name() const {
    return (m_name);
  }
 

  /**
   * Executes the function. This method is a wrapper for the virtual dispatch
   * method.
   */
  void FxBinder::execute() {
    dispatch();
  }
 

  /**
   * Executes the function. This method is a wrapper for the virtual dispatch
   * method.
   */
  void FxBinder::operator()() {  
    dispatch();
  }
 

  /**
   * Accesses the arguments for this function. For scalars and variables, the
   * argument is also the function name.
   *  
   * @return QVariant The parameters of this function, as a QVariant.
   */
  QVariant FxBinder::args()  {
    return (QVariant(m_name));
  }
 

  /**
   * Constructs an InlineVoid function from the given name, InlineCalculator
   * operator, and InlineCalculator.
   *  
   * @param name A string containing a name for this function.
   * @param function An InlineCalculator operator that takes no arguments.
   * @param calculator The InlineCalculator used to evaluate this function.
   */
  InlineVoidFx::InlineVoidFx(const QString &name, calcOp function,
                InlineCalculator *calculator) : FxBinder(name),
                                                m_func(function),
                                                m_calc(calculator)  {
  }
 

  /**
   * Destroys the InlineVoidFx object.
   */
  InlineVoidFx::~InlineVoidFx() {
  }
 

  /**
   * Calls the function corresponding to this object using its
   * stored InlineCalculator and InlineCalculator operator.
   */
  void InlineVoidFx::dispatch() {  
    CALL_MEMBER_FN(*m_calc, m_func)();  
  }
   
   
  /**
   * Constructs a Parameter function from the given name (containing the
   * appropriate parameters), InlineCalculator operator, and
   * InlineCalculator.
   *  
   * @param name A string containing a name for this function. Note: The
   *             name given should include the parameters to be passed
   *             into this function.
   * @param function An InlineCalculator operator that takes parameters.
   * @param calculator The InlineCalculator used to evaluate this function.
   */
  ParameterFx::ParameterFx(const QString &name, calcOp function,
              InlineCalculator *calculator) : FxBinder(name),
                                              m_func(function),
                                              m_calc(calculator){
  }
 

  /**
   * Destroys the ParameterFx object.
   */
  ParameterFx::~ParameterFx() {
  }
 

  /**
   * Calls the function corresponding to this object using its
   * stored InlineCalculator, InlineCalculator operator, and
   * arguments.
   */
  void ParameterFx::dispatch() {  
    CALL_MEMBER_FN(*m_calc, m_func)(args());  
  }
 
 
  /**
   * Constructs a Void function from the given name, Calculator operator, and
   * Calculator.
   *  
   * @param name A string containing a name for this function.
   * @param function A Calculator operator that takes no arguments.
   * @param calculator The Calculator used to evaluate this function.
   */
  VoidFx::VoidFx(const QString &name, calcOp function,
         InlineCalculator *calculator) : FxBinder(name),
                                         m_func(function),
                                         m_calc(calculator)  {
  }
 
 
  /**
   * Destroys the VoidFx object.
   */
  VoidFx::~VoidFx() {
  }
 
 
  /**
   * Calls the function corresponding to this object using its stored Calculator
   * and Calculator operator.
   */
  void VoidFx::dispatch() {  
    CALL_MEMBER_FN(*m_calc, m_func)();  
  }
} // namespace Isis
