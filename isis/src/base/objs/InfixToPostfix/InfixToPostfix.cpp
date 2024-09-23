/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "InfixToPostfix.h"

#include <iostream>

#include "IException.h"
#include "IString.h"

using namespace std;

namespace Isis {
  //! Constructor
  InfixToPostfix::InfixToPostfix() {
    initialize();
  }

  InfixToPostfix::~InfixToPostfix() {
    uninitialize();
  }

  /**
   * This populates the known operators/functions list. If the operators list
   * is not empty, this function will do nothing.
   */
  void InfixToPostfix::initialize() {
    p_operators.push_back(new InfixOperator(7, "^"));
    p_operators.push_back(new InfixOperator(5, "/"));
    p_operators.push_back(new InfixOperator(5, "*"));
    p_operators.push_back(new InfixOperator(3, "<<"));
    p_operators.push_back(new InfixOperator(3, ">>"));
    p_operators.push_back(new InfixOperator(2, "+"));
    p_operators.push_back(new InfixOperator(2, "-"));
    p_operators.push_back(new InfixOperator(1, ">"));
    p_operators.push_back(new InfixOperator(1, "<"));
    p_operators.push_back(new InfixOperator(1, ">="));
    p_operators.push_back(new InfixOperator(1, "<="));
    p_operators.push_back(new InfixOperator(1, "=="));
    p_operators.push_back(new InfixOperator(1, "!="));
    p_operators.push_back(new InfixOperator(-1, "("));

    // This makes multiple argument functions work
    p_operators.push_back(new InfixOperator(-1, ","));

    p_operators.push_back(new InfixFunction("--",     1));
    p_operators.push_back(new InfixFunction("neg",    1));
    p_operators.push_back(new InfixFunction("sqrt",   1));
    p_operators.push_back(new InfixFunction("abs",    1));
    p_operators.push_back(new InfixFunction("sin",    1));
    p_operators.push_back(new InfixFunction("cos",    1));
    p_operators.push_back(new InfixFunction("tan",    1));
    p_operators.push_back(new InfixFunction("csc",    1));
    p_operators.push_back(new InfixFunction("sec",    1));
    p_operators.push_back(new InfixFunction("cot",    1));
    p_operators.push_back(new InfixFunction("asin",   1));
    p_operators.push_back(new InfixFunction("acos",   1));
    p_operators.push_back(new InfixFunction("atan",   1));
    p_operators.push_back(new InfixFunction("atan2",  2));
    p_operators.push_back(new InfixFunction("sinh",   1));
    p_operators.push_back(new InfixFunction("cosh",   1));
    p_operators.push_back(new InfixFunction("tanh",   1));
    p_operators.push_back(new InfixFunction("asinh",  1));
    p_operators.push_back(new InfixFunction("acosh",  1));
    p_operators.push_back(new InfixFunction("atanh",  1));
    p_operators.push_back(new InfixFunction("log",    1));
    p_operators.push_back(new InfixFunction("log10",  1));
    p_operators.push_back(new InfixFunction("ln",     1));
    p_operators.push_back(new InfixFunction("degs",    1));
    p_operators.push_back(new InfixFunction("rads",    1));
    p_operators.push_back(new InfixFunction("linemin", 1));
    p_operators.push_back(new InfixFunction("linemax", 1));
    p_operators.push_back(new InfixFunction("min",     2));
    p_operators.push_back(new InfixFunction("max",     2));
    p_operators.push_back(new InfixFunction("line",   0));
    p_operators.push_back(new InfixFunction("sample", 0));
    p_operators.push_back(new InfixFunction("band",   0));
    p_operators.push_back(new InfixFunction("pi",     0));
    p_operators.push_back(new InfixFunction("e",      0));
  }

  /**
   * This cleans the known operators/functions list.
   */
  void InfixToPostfix::uninitialize() {
    for(int i = 0; i < p_operators.size(); i ++) {
      delete p_operators[i];
    }

    p_operators.clear();
  }

  /**
   * This function takes a space-delimited string and removes empty
   * delimiters. In order words, it compresses the spaces. This is used
   * to keep strings small, clean, and remove the necessity to check for
   * empty tokens constantly.
   *
   * That is, if your string is "a  b" the result will be "a b".
   *
   * @param equation A space-delimited string with excessive spaces
   *
   * @return IString A space-delimited string with data between every pair of spaces
   */
  QString InfixToPostfix::cleanSpaces(QString equation) {
    IString equationIStr = equation.toStdString();
    IString clean = "";
    while(!equationIStr.empty()) {
      IString data = equationIStr.Token(" ");
      if(data.empty()) {
        continue;
      }

      if(clean.empty()) {
        clean = data;
      }
      else {
        clean += " " + data;
      }
    }

    return QString::fromStdString(clean);
  }

  /**
   * This method converts infix to postfix. It uses an enhanced verion of
   * the algorithm found on page 159 of "Data Structures & Algorithms in Java"
   * Second Edition by Robert Lafore. First, we prep the equation with
   * TokenizeEquation and then parse through it using the known-good algorithm.
   *
   * @param infix The infix equation
   *
   * @return IString The postfix equation
   */
  QString InfixToPostfix::convert(const QString &infix) {
    // Prep our equation for the conversion
    IString equation = tokenizeEquation(infix).toStdString();
    IString postfix = "";

    // The algorithm uses a stack
    std::stack<InfixOperator> theStack;

    // Use this to look for two operands in a row. If we find such,
    //   then we know the infix equation is bogus.
    int numConsecutiveOperands = 0;

    // Use this to look for two operators in a row. If we find such,
    //   then we know the infix equation is bogus.
    int numConsecutiveOperators = 0;

    // We'll use tokens to get through the entire equation, which is space-delimited
    //   because of TokenizeEquation. So continue processing until we're out of tokens
    //   and the string is empty.
    while(!equation.empty()) {

      // There will be no empty tokens, so don't worry about checking for it.
      //   TokenizeEquation cleans excess spaces in it's return value.
      QString data = QString::fromStdString(equation.Token(" "));

      if(data.compare("(") == 0) {
        theStack.push(*findOperator(data));
      }
      else if(data.compare(")") == 0) {
        QString postfixQStr = QString::fromStdString(postfix);
        closeParenthesis(postfixQStr, theStack);
        postfix = postfixQStr.toStdString();
      }
      else if(isKnownSymbol(data)) {
        QString postfixQStr = QString::fromStdString(postfix);
        addOperator(postfixQStr, *findOperator(data), theStack);
        postfix = postfixQStr.toStdString();

        if(isFunction(data)) {
          // For a general check, zero single argument functions the
          //   same as an operand.
          if(((InfixFunction *)findOperator(data))->argumentCount() == 0) {
            numConsecutiveOperators = 0;
            numConsecutiveOperands ++;
          }
          else {
            // We have a function with arguments, an operand or function call is expected next
            numConsecutiveOperators = 1;
            numConsecutiveOperands = 0;
          }
        }
        else {
          // We found an operator, not a function, so we expect a number next
          numConsecutiveOperators ++;
          numConsecutiveOperands = 0;
        }
      }
      else {
        try {
          // Make sure this is truly an operand and not an operator by casting it
          data.toDouble();
        }
        catch(IException &) {
          throw IException(IException::User,
                           "The operator '" + data.toStdString() + "' is not recognized.",
                           _FILEINFO_);
        }

        // This was clearly an operand at this point
        numConsecutiveOperators = 0;
        numConsecutiveOperands ++;

        postfix += IString(' ' + data.toStdString() + ' ');
      }

      // If we found consecutive operators or operands, tell the user
      if(numConsecutiveOperators > 1) {
        throw IException(IException::User, "Missing an operand near the operator '" + data.toStdString() + "'.", _FILEINFO_);
      }
      else if(numConsecutiveOperands > 1) {
        throw IException(IException::User, "Missing an operator before " + data.toStdString() + ".", _FILEINFO_);
      }
    }

    while(!theStack.empty()) {
      IString op = theStack.top().outputString().toStdString();

      // Any opening parentheses here are invalid at this point
      if(op == "(") {
        throw IException(IException::User,
                         "There are too many opening parentheses ('(') in the equation.",
                         _FILEINFO_);
      }

      postfix += ' ' + op + ' ';
      theStack.pop();
    }

    // The , is set to an operator that causes multiple-argument functions to work, but
    //   it needs to be stripped out for postfix. This is the correct way to do it.
    postfix = postfix.Remove(",");

    // Clean spaces just to double check and return our postfix answer
    return cleanSpaces(QString::fromStdString(postfix));
  }

  /**
   * This method will return true if it believes the argument represents
   *   a valid function or operator.
   *
   * @param representation The symbolic representation of the operator, such as 'sin'
   *
   * @return bool True if it looks valid, false if it's not known
   */
  bool InfixToPostfix::isKnownSymbol(QString representation) {
    for(int i = 0; i < p_operators.size(); i++) {
      if(representation.compare(p_operators[i]->inputString()) == 0) {
        return true;
      }
    }

    return false;
  }

  /**
   * This method will return true if 'representation' is a known function.
   *
   * @param representation The symbolic representation of a function, such as 'abs'
   *
   * @return bool True if it's a known function, false otherwise
   */
  bool InfixToPostfix::isFunction(QString representation) {
    if(isKnownSymbol(representation)) {
      return findOperator(representation)->isFunction();
    }
    else {
      return false;
    }
  }

  /**
   * This is straight from the algorithm found on page 159 of "Data Structures & Algorithms in Java"
   * Second Edition by Robert Lafore.
   *
   * @param postfix The postix generated thus far
   * @param op The operator
   * @param theStack The operator stack
   */
  void InfixToPostfix::addOperator(QString &postfix, const InfixOperator &op, std::stack<InfixOperator> &theStack) {
    while(!theStack.empty()) {
      InfixOperator top = theStack.top();
      theStack.pop();

      if(top.inputString().compare("(") == 0) {
        theStack.push(top);
        break;
      }

      if(top.precedence() < op.precedence()) {
        theStack.push(top);
        break;
      }
      else {
        postfix += ' ' + top.outputString() + ' ';
      }
    }

    theStack.push(op);
  }

  /**
   * This is straight from the algorithm found on page 159 of "Data Structures & Algorithms in Java"
   * Second Edition by Robert Lafore.
   *
   * @param postfix The postix generated thus far
   * @param theStack The operator stack
   */
  void InfixToPostfix::closeParenthesis(QString &postfix, std::stack<InfixOperator> &theStack) {
    bool openingFound = false;
    while(!theStack.empty()) {
      InfixOperator op = theStack.top();
      theStack.pop();

      if(op.inputString().compare("(") == 0) {
        openingFound = true;
        break;
      }
      else {
        postfix += ' ' + op.outputString() + ' ';
      }
    }

    if(!openingFound) {
      throw IException(IException::User,
                       "There are too many closing parentheses (')') in the equation.",
                       _FILEINFO_);
    }
  }

  /**
   * This method will return a pointer to the operator represented by
   *   'representation.' Because in this model a function is an operator,
   *   this will return a pointer to functions as well (in a base class pointer).
   *
   * @param representation The symbolic representation of the operator, such as '+'
   *
   * @return InfixOperator* A pointer to the operator object that contains known information about the operator
   */
  InfixOperator *InfixToPostfix::findOperator(QString representation) {
    for(int i = 0; i < p_operators.size(); i++) {
      if(representation.compare(p_operators[i]->inputString()) == 0) {
        return p_operators[i];
      }
    }

    // Nothing found
    throw IException(IException::User, "The operator '" + representation.toStdString() + "' is not recognized.", _FILEINFO_);
  }

  /**
   * This method will add spaces between all operators and numbers, making it
   * possible to get each element of the equation one by one. It will also parse
   * out the function calls, adding parenthesis where needed so the user doesn't
   * have to. The result is an equation ready for parsing (but NOT fully parenthesized,
   * just enough to make sure our algorithm makes no mistakes).
   *
   * @param equation An unformatted infix equation
   *
   * @return IString A tokenized equation with additional parentheses
   */
  QString InfixToPostfix::tokenizeEquation(const QString &equation) {
    IString output = "";

    // Insert whitespace, make everything lowercase, and change all braces to
    // parenthesis
    for(int i = 0; i < equation.size(); i++) {
      // Ensure there is whitespace in the equation
      if(!equation[i].isLetterOrNumber() && !equation[i].isSpace() &&
         equation[i] != '.' && equation[i] != '_') {
        // Convert all braces to parens
        if(equation[i] == '[' || equation[i] == '{') {
          output += " ( ";
        }
        else if(equation[i] == ']' || equation[i] == '}') {
          output += " ) ";
        }
        // Test for multicharacter operators
        else if(i < equation.size() - 1 && equation[i] == '-' && equation[i+1] == '-') {
          output += " -- ";
          i++;
        }
        else if(i < equation.size() - 1 && equation[i] == '<' && equation[i+1] == '<') {
          output += " << ";
          i++;
        }
        else if(i < equation.size() - 1 && equation[i] == '>' && equation[i+1] == '>') {
          output += " >> ";
          i++;
        }
        else if(i < equation.size() - 1 && equation[i] == '>' && equation[i+1] == '=') {
          output += " >= ";
          i++;
        }
        else if(i < equation.size() - 1 && equation[i] == '<' && equation[i+1] == '=') {
          output += " <= ";
          i++;
        }
        else if(i < equation.size() - 1 && equation[i] == '=' && equation[i+1] == '=') {
          output += " == ";
          i++;
        }
        else if(i < equation.size() - 1 && equation[i] == '!' && equation[i+1] == '=') {
          output += " != ";
          i++;
        }
        else if(i < equation.size() - 1 && equation[i] == '|' && equation[i+1] == '|') {
          output += " || ";
          i++;
        }
        else if(i < equation.size() - 1 && equation[i] == '&' && equation[i+1] == '&') {
          output += " && ";
          i++;
        }
        // Take care of scientific notiation where the exponent is negative
        else if((i > 1) && equation[i] == '-' && equation[i-1].toLower() == 'e' && equation[i-2].isLetterOrNumber()) {
          output += equation[i].toLatin1();
        }
        // Look for negative operator disguised as '-'
        else if(equation[i] == '-') {
          bool isNegative = true;

          // If we run into a '(' or the beginning, then the '-' must mean '--' really.
          for(int index = i - 1; index >= 0; index --) {
            if(equation[index] == ' ') {
              continue;
            }

            if(equation[index] != '(' && equation[index] != '/' &&
                equation[index] != '*' && equation[index] != '+') {
              isNegative = false;
              break;
            }

            break;
          }

          if(isNegative) {
            output += " -- ";
          }
          else {
            output += " - ";
          }
        }
        else {
          output += ' ';
          output += equation[i].toLatin1();
          output += ' ';
        }
      }
      else {
        output += equation[i].toLatin1();
      }
    }

    QString cleanedEquation = cleanSpaces(formatFunctionCalls(QString::fromStdString(output.DownCase())));

    return cleanedEquation;
  }

  /**
   * This method looks through equation for function calls, parenthesizes them,
   * and calls itself again with each argument in order to parse any embedded functions.
   * This ensures order of operations holds for cases like sin(.5)^2. This method might add
   * too many parentheses, but that is harmless. This does not affect operators (excepting --) or
   * numbers. Only functions. The input should be space-delimited.
   *
   * @param equation The unparenthesized equation
   *
   * @return IString The parenthesized equation
   */
  QString InfixToPostfix::formatFunctionCalls(QString equation) {
    // Clean our space-delimited equation
    equation = cleanSpaces(equation);
    QString output = "";

    // We'll use tokens to get through the entire equation, which is space-delimited.
    //   So continue processing until we're out of tokens and the string is empty.
    while(!equation.isEmpty()) {
      IString tmp = equation.toStdString();

      QString element = QString::fromStdString(tmp.Token(" "));
      equation = QString::fromStdString(tmp);

      // Did we find a function? Figure out what it is!
      if(isFunction(element)) {
        // Point to the function. We know if IsFunction returned true,
        //   the result is really a InfixFunction*
        InfixFunction *func = (InfixFunction *)findOperator(element);

        // We want to wrap the entire thing in parentheses, and it's argument string.
        //   So sin(.5)^2 becomes (sin(.5))^2
        output += " ( " + func->inputString() + " (";

        // Deal with 0-argument functions
        if(func->argumentCount() == 0) {
          IString tmp = equation.toStdString();
          QString next = QString::fromStdString(tmp.Token(" "));
          equation = QString::fromStdString(tmp);

          // If they didn't add parentheses around the zero-argument
          //   function, we still know what they mean. Close the arguments
          //   and the wrapping parentheses.
          if(next != "(") {
            output += " ) ) ";

            // Step back, we did too much
            equation = next + " " + equation;
          }
          else {
            IString tmp = equation.toStdString();
            // We see a zero-arg function, and we grabbed an open parenthesis from it.
            //   Make sure the next thing is a close or we have a problem.
            if(tmp.Token(" ") != ")") {
              throw IException(IException::User,
                               "The function " + func->inputString().toStdString() + " should not have any arguments.",
                               _FILEINFO_);
            }
            equation = QString::fromStdString(tmp);

            // Close the arguments and the wrapping parentheses. They wrote their call correct :)
            output += " ) ) ";
          }
        }
        else {
          // Deal with 1+ argument functions by parsing out the arguments

          IString tmp = equation.toStdString();

          // Make sure the user put parentheses around these, otherwise we're left in the dark.
          if (func->argumentCount() > 1 && tmp.Token(" ") != "(") {
            throw IException(IException::User,
                             "Missing parenthesis after " + func->inputString().toStdString(),
                             _FILEINFO_);
          }

          equation = QString::fromStdString(tmp);

          // Single argument missing parenthesis?
          if(func->argumentCount() == 1) {

            IString tmp = equation.toStdString();
            QString argument = QString::fromStdString(tmp.Token(" "));
            equation = QString::fromStdString(tmp);

            if(argument != "(") {
              // We might have a problem. They're calling a function without adding parentheses....
              //   unless it's a negate, because we insert those, tell them their mistake. It's not
              //   my job to figure out what they mean!
              if(func->inputString() != "--") {
                throw IException(IException::User,
                                 "Missing parenthesis after " + func->inputString().toStdString(),
                                 _FILEINFO_);
              }

              // It's a negate without parentheses, so they mean the next term?
              if(!isFunction(argument)) {
                // If it isn't a function, it's safe to just append
                output += " " + formatFunctionCalls(argument) + " ) ) ";
                continue;
              }
              else {
                // We are negating a function result. We must do a mini-parse to figure out
                //   the function and resursively call, then append the negation.
                QString functionName = argument;

                IString tmp = equation.toStdString();;
                QString openParen = QString::fromStdString(tmp.Token(" "));
                equation = QString::fromStdString(tmp);

                // No open parens? Call ourself again with this supposed function
                if(openParen != "(") {
                  output += " " + formatFunctionCalls(functionName) + " ) ) ";
                  equation = openParen + " " + equation;
                  continue;
                }
                else {
                  functionName += " (";
                }

                // Parse out our equation quickly, call ourself with it to properly handle it,
                //   and append the negation.
                int numParens = 0;
                while(numParens > -1) {
                  IString tmp = equation.toStdString();;
                  QString newElem = QString::fromStdString(tmp.Token(" "));
                  equation = QString::fromStdString(tmp);

                  if(newElem == "") {
                    throw IException(IException::User,
                                     "Missing closing parentheses after '" + argument.toStdString() + "'.",
                                     _FILEINFO_);
                  }

                  if(newElem == "(") numParens++;
                  else if(newElem == ")") numParens--;

                  functionName += " " + newElem;
                }

                output += " " + formatFunctionCalls(functionName) + " ) ) ";
                continue;
              }
            }
          }

          /**
           * This code block is for multi-parameter functions. Functions with 1+ parameters
           * are parsed here, excepting the negation (with no parentheses) which is done just above.
           *
           * We figure out the arguments by looking for commas, outside of all parentheses, for each argument
           * up until the last one. We look for an extra closing parenthesis for the last argument. When we figure
           * out what an argument is, we recursively call FormatFunctionCalls to format any and all functionality
           * inside of the argument.
           */
          QString argument = "";
          int numParens = 0;
          int argNum = 0;
          while(argNum < func->argumentCount()) {
            IString tmp = equation.toStdString();;
            QString elem = QString::fromStdString(tmp.Token(" "));
            equation = QString::fromStdString(tmp);

            // Ran out of data, the function call is not complete.
            if(elem == "") {
              throw IException(IException::User,
                               "The definition of '" + func->inputString().toStdString() + "' is not complete.",
                               _FILEINFO_);
            }

            if(elem == "(") {
              numParens ++;
              argument += " (";
            }
            else if(elem == ")") {
              numParens --;

              // Ignore last close, it's not part of the argument and will be added later
              if(numParens != -1) {
                argument += " )";
              }
            }
            else if(elem == "," && numParens == 0) {
              checkArgument(func->inputString(), argNum, argument);
              argument = formatFunctionCalls(argument);
              output += " ( " + argument + " ) , ";
              argNum++;
              argument = "";

              // Too many arguments? We don't expect a comma delimiter on the last argument.
              if(argNum == func->argumentCount()) {
                throw IException(IException::User,
                                 "There were too many arguments supplied to the function '" + func->inputString().toStdString() + "'.",
                                 _FILEINFO_);
              }
            }
            else {
              argument += " " + elem;
            }

            if(argNum == func->argumentCount() - 1 && numParens == -1) {
              checkArgument(func->inputString(), argNum, argument);
              argument = formatFunctionCalls(argument);
              // close function call & function wrap
              output += " " + argument + " ) ) ";
              argNum++;
              argument = "";
            }
            // Closed the function early?
            else if(numParens == -1) {
              throw IException(IException::User,
                               "There were not enough arguments supplied to the function '" + func->inputString().toStdString() + "'.",
                               _FILEINFO_);
            }
          }
        }
      }
      // Not a function, preserve & ignore
      else {
        output = output + " " + element;
      }
    }

    return output;
  }

  void InfixToPostfix::checkArgument(QString funcName, int argNum, QString argument) {
    argument = argument.remove(QRegExp("[ ()]"));

    if(argument == "") {
      throw IException(IException::User,
                       "Argument " + toString(argNum + 1) + " in function " + funcName.toStdString() + " must not be empty.",
                       _FILEINFO_);
    }
  }
}
