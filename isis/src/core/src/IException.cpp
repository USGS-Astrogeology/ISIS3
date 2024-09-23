/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "IException.h"

#include <stdlib.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include <QList>

#include "Preference.h"
#include "Pvl.h"

using namespace std;

namespace Isis {
  /**
   * The default constructor creates an IException instance with no message,
   * previous exceptions, or file info. The exception error type is Unknown.
   *
   * This default constructor exists so that IException instances can be stored
   * in Qt containers such as lists. This is also used for cancelling a program
   * run.
   *
   * @code
   *   throw IException();
   * @endcode
   */
  IException::IException() {
    m_what = NULL;
    m_errorType = Unknown;
    m_message = NULL;
    m_fileName = NULL;
    m_previousExceptions = NULL;
    m_lineNumber = -1;

    m_what = buildWhat();
  }


  /**
   * This version of the constructor creates an IException instance with the
   * given error type, message, and file info. The IException instance will not
   * have any previous exceptions associated with it initially (i.e. no
   * exception caused this one to be thrown).
   *
   * @code
   *   throw IException(IException::Unknown,
   *                    "While doing an important process, we could not do ... "
   *                    "because the data [" ... "] is invalid",
   *                    _FILEINFO_);
   * @endcode
   *
   * @param type the source of the error that this exception represents
   * @param message the string message containing details about the error, which
   *          may be displayed to the user
   * @param fileName the filename of the file that this exception was thrown in
   * @param lineNumber the line in the source code file that threw this
   *          exception
   */
  IException::IException(ErrorType type, const char *message,
                         const char *fileName, int lineNumber) {
    m_what = NULL;
    m_message = NULL;
    m_fileName = NULL;
    m_previousExceptions = NULL;

    m_errorType = type;
    m_message = new std::string(IString(message).Trim("\t\n\v\f\r "));
    m_fileName = new std::string(fileName);
    m_lineNumber = lineNumber;

    deleteEmptyMemberStrings();

    m_what = buildWhat();
  }


  /**
   * This version of the constructor creates an IException instance with the
   * given error type, message, and file info. The IException instance will not
   * have any previous exceptions associated with it initially (i.e. no
   * exception caused this one to be thrown). 
   *
   * @code
   *   std::string message = "While doing an important process, we could not do .. "
   *                     "because the data [" ... "] is invalid";
   *   throw IException(IException::Unknown, message, _FILEINFO_);
   * @endcode
   *
   * @param type the source of the error that this exception represents
   * @param message the string message containing details about the error, which
   *          may be displayed to the user
   * @param fileName the filename of the file that this exception was thrown in
   * @param lineNumber the line in the source code file that threw this
   *          exception
   */
  IException::IException(ErrorType type, const std::string &message,
                         const char *fileName, int lineNumber) {
    m_what = NULL;
    m_message = NULL;
    m_fileName = NULL;
    m_previousExceptions = NULL;

    m_errorType = type;
    m_message = new std::string(IString(message).Trim("\t\n\v\f\r "));
    m_fileName = new std::string(fileName);
    m_lineNumber = lineNumber;

    deleteEmptyMemberStrings();

    m_what = buildWhat();
  }


  /**
   * This version of the constructor creates an IException instance with the
   * given error type, message, file info. The IException instance will append
   * the given exception to its list of previous exceptions (as well as any
   * previous exceptions associated with the caught exception). Use this
   * constructor when you want to rethrow a new exception after catching an
   * exception and preserve the previous message(s).
   *
   * @code
   *   try {
   *     ...
   *   }
   *   catch (IException &e) {
   *     throw IException(e,
   *                      IException::Unknown,
   *                      "While doing an important process, we could not do "
   *                      "... ",
   *                      _FILEINFO_);
   *   }
   * @endcode
   *
   * @param caughtException the previous exception that caused this exception to
   *          be thrown
   * @param type the source of the error that this exception represents
   * @param message the string message containing details about the error, which
   *          may be displayed to the user
   * @param fileName the filename of the file that this exception was thrown in
   * @param lineNumber the line in the source code file that threw this
   *          exception
   */
  IException::IException(const IException &caughtException,
      ErrorType type, const char *message,
      const char *fileName, int lineNumber) {
    m_what = NULL;
    m_message = NULL;
    m_fileName = NULL;
    m_previousExceptions = NULL;

    m_errorType = type;
    m_message = new std::string(IString(message).Trim("\t\n\v\f\r "));
    m_fileName = new std::string(fileName);
    m_lineNumber = lineNumber;

    deleteEmptyMemberStrings();

    append(caughtException);
  }


  /**
   * This version of the constructor creates an IException instance with the
   * given error type, message, file info. The IException instance will append
   * the given exception to its list of previous exceptions (as well as any
   * previous exceptions associated with the caught exception). Use this
   * constructor when you want to rethrow a new exception after catching an
   * exception and preserve the previous message(s).
   *
   * @code
   *   try {
   *     ...
   *   }
   *   catch (IException &e) {
   *     string message = "While doing an important process, we could not do "
   *                       "... ";
   *     throw IException(e, IException::Unknown, message, _FILEINFO_);
   *   }
   * @endcode
   *
   * @param caughtException the previous exception that caused this exception to
   *          be thrown
   * @param type the source of the error that this exception represents
   * @param message the string message containing details about the error, which
   *          may be displayed to the user
   * @param fileName the filename of the file that this exception was thrown in
   * @param lineNumber the line in the source code file that threw this
   *          exception
   */
  IException::IException(const IException &caughtException,
      ErrorType type, const std::string &message,
      const char *fileName, int lineNumber) {
    m_what = NULL;
    m_message = NULL;
    m_fileName = NULL;
    m_previousExceptions = NULL;

    m_errorType = type;
    m_message = new std::string(IString(message).Trim("\t\n\v\f\r "));
    m_fileName = new std::string(fileName);
    m_lineNumber = lineNumber;

    deleteEmptyMemberStrings();

    append(caughtException);
  }


  /**
   * The copy constructor creates a copy of the given exception.
   *
   * @param other the exception to copy from
   */
  IException::IException(const IException &other) : exception(other) {
    m_what = NULL;
    m_message = NULL;
    m_fileName = NULL;
    m_previousExceptions = NULL;

    m_errorType = other.m_errorType;
    m_lineNumber = other.m_lineNumber;

    if (other.m_what) {
      int length = strlen(other.m_what);
      m_what = new char[length + 1];
      strncpy(m_what, other.m_what, length);
      m_what[length] = '\0';
    }

    if (other.m_message) {
      m_message = new std::string(*other.m_message);
    }

    if (other.m_fileName) {
      m_fileName = new std::string(*other.m_fileName);
    }

    if (other.m_previousExceptions) {
      m_previousExceptions = new QList<IException>(*other.m_previousExceptions);
    }
  }


  /**
   * The destructor frees memory allocated for the message, filename, and list
   * of previous exceptions.
   */
  IException::~IException() throw() {
    delete [] m_what;
    m_what = NULL;

    m_errorType = Unknown;

    delete m_message;
    m_message = NULL;

    delete m_fileName;
    m_fileName = NULL;

    m_lineNumber = -1;

    delete m_previousExceptions;
    m_previousExceptions = NULL;
  }


  /**
   * Returns a string representation of this exception in its current state. The
   * results of this method are guaranteed to be valid until the instance is
   * modified.
   *
   * @return a C string representation of this exception
   */
  const char *IException::what() const throw() {
    return m_what;
  }

  /**
   * Appends the given exception (and its list of previous exceptions) to this
   * exception's causational exception list. The passed in exception is the
   * cause of this exception. Exceptions should be appended in the original
   * order that they were thrown - earlier first.
   *
   * This method causes the results of what() to be rebuilt, invalidating any
   * previous results.
   *
   * @code
   * try {
   *    ...
   * }
   * catch (IException &error1) {
   *   try {
   *     ...
   *   }
   *   catch (IException &error2) {
   *     IException finalError(IException::Unknown, "...", _FILEINFO_);
   *     finalError.append(error1);
   *     finalError.append(error2);
   *     throw finalError;
   *   }
   * }
   *
   * @endcode
   *
   * @param exceptionSource the exception that should be added to the list of
   *          previous exceptions
   */
  void IException::append(const IException &exceptionSource) {
    if (!m_previousExceptions) {
      m_previousExceptions = new QList<IException>;
    }

    if (exceptionSource.m_previousExceptions) {
      m_previousExceptions->append(*exceptionSource.m_previousExceptions);
    }

    m_previousExceptions->append(exceptionSource);

    delete [] m_what;
    m_what = NULL;
    m_what = buildWhat();
  }

  /**
   * Returns the source of the error for this exception.
   *
   * @return the source of the error (i.e. ErrorType)
   */
  IException::ErrorType IException::errorType() const {
    return m_errorType;
  }


  /**
   * Prints a string representation of this exception to stderr. File info (i.e.
   * filename and line number) are only printed if the user has that option
   * enabled in their preferences file. The printed exception will be either
   * PVL-formatted or formatted as plain sentences according to the user's
   * preferences file.
   *
   * This should be the preferred method call in unit tests when testing
   * exceptions thrown by your class.
   */
  void IException::print() const {
    std::string errorString = toString();
    if (errorString != "")
      cerr << errorString << endl;
  }


  /**
   * Prints a string representation of this exception to stderr, including file
   * info if specified. The printed exception will be either PVL-formatted or
   * formatted as plain sentences according to the user's preferences file.
   *
   * @param printFileInfo whether or not to include file info in the printed
   *          version of this exception. This ignores the option set in the
   *          user's preferences file regarding file info.
   */
  void IException::print(bool printFileInfo) const {
    std::string errorString = toString(printFileInfo);
    if (errorString != "")
      cerr << errorString << endl;
  }


  /**
   * Returns a PVL object representing the contents of this exception. File info
   * is included unless there is no filename or line number associated with an
   * exception, regardless of the user's preferences file.
   *
   * @return a PVL object representing this exception
   */
  Pvl IException::toPvl() const {
    Pvl errors;

    QList<IException> exceptionsToConvert;

    if(m_previousExceptions) {
      exceptionsToConvert.append(*m_previousExceptions);
    }
    exceptionsToConvert.append(*this);

    for (int exceptionIndex = exceptionsToConvert.size() - 1;
         exceptionIndex >= 0;
         exceptionIndex--) {
      const IException &exception(exceptionsToConvert.at(exceptionIndex));

      bool exceptionIsBlank = true;
      PvlGroup errGroup("Error");

      if (exception.m_errorType != Unknown) {
        errGroup += PvlKeyword("Class",
                               errorTypeToString(exception.m_errorType));
        exceptionIsBlank = false;
      }

      errGroup += PvlKeyword("Code", to_string(exception.m_errorType));

      if (exception.m_message) {
        exceptionIsBlank = false;
        std::string message(*exception.m_message);

        if (message.size() && message[message.size() - 1] == '.')
          message = message.substr(0, message.size() - 1);
        errGroup += PvlKeyword("Message", message);
      }

      if (exception.m_fileName) {
        exceptionIsBlank = false;
        errGroup += PvlKeyword("File", *exception.m_fileName);

        if (exception.m_lineNumber != -1)
          errGroup += PvlKeyword("Line", Isis::toString(exception.m_lineNumber));
      }

      if (!exceptionIsBlank)
        errors.addGroup(errGroup);
    }

    return errors;
  }


  /**
   * Returns a string representation of this exception. File info (i.e. filename
   * and line number) are only included if the user has that option
   * enabled in their preferences file. The string representation of the
   * exception will be either PVL-formatted or formatted as plain sentences
   * according to the user's preferences file.
   *
   * @return a string representation of this exception
   */
  std::string IException::toString() const {
    return toString(Preference::Preferences().reportFileLine());
  }


  /**
   * Returns a string representation of this exception, including file info if
   * specified. The string representation of the exception will be either
   * PVL-formatted or formatted as plain sentences according to the user's
   * preferences file.
   *
   * @param includeFileInfo whether or not to include file info in the string
   *          representation of this exception. This ignores the option set in
   *          the user's preferences file regarding file info.
   * @return a string representation of this exception
   */
  std::string IException::toString(bool includeFileInfo) const {
    std::string result;

    if (Preference::Preferences().outputErrorAsPvl()) {
      Pvl errors = toPvl();

      if (errors.groups() != 0) {
        stringstream stringStream;
        stringStream << errors;
        result = stringStream.str();
      }
    }
    else {
      QList<IException> exceptionsToConvert;

      if(m_previousExceptions) {
        exceptionsToConvert.append(*m_previousExceptions);
      }
      exceptionsToConvert.append(*this);

      for (int exceptionIndex = exceptionsToConvert.size() - 1;
           exceptionIndex >= 0;
           exceptionIndex--) {
        const IException &exception(exceptionsToConvert.at(exceptionIndex));

        // Don't put **ERROR** if there is no message or type
        if (exception.m_errorType != Unknown || exception.m_message) {
          result += "**" + errorTypeToString(exception.m_errorType) + "**";
        }

        bool needsPeriod = false;
        if (exception.m_message) {
          std::string message(*exception.m_message);

          if (message.size() && message[message.size() - 1] == '.')
            message = message.substr(0, message.size() - 1);
          // There is always a **TYPE** already in the string, so prefix the
          //   message with a space.
          result += " " + message;
          needsPeriod = true;
        }

        if(includeFileInfo && exception.m_fileName) {
          result += " in " + *exception.m_fileName;
          if (exception.m_lineNumber != -1)
            result += " at " + Isis::toString(exception.m_lineNumber);
          needsPeriod = true;
        }

        if (needsPeriod)
          result += ".";

        if (result.size() && result[result.size() - 1] != '\n')
          result += "\n";
      }
    }

    return IString(result).Trim("\t\n\v\f\r ");
  }


  /**
   * Swaps the values of this instance's member data with other. This method is
   * exception-safe.
   *
   * @param other the exception to swap member data with
   */
  void IException::swap(IException &other) {
    std::swap(m_what, other.m_what);
    std::swap(m_errorType, other.m_errorType);
    std::swap(m_message, other.m_message);
    std::swap(m_fileName, other.m_fileName);
    std::swap(m_lineNumber, other.m_lineNumber);
    std::swap(m_previousExceptions, other.m_previousExceptions);
  }


  /**
   * Assign the values of rhs to this instance. This is a deep copy and is
   * exception-safe.
   *
   * @param rhs the IException on the right-hand side of the equals operator
   * @return *this
   */
  IException &IException::operator=(const IException &rhs) {
    IException copy(rhs);
    swap(copy);

    return *this;
  }


  /**
   * Returns the source of the error in string format for the given ErrorType.
   *
   * @param type the ErrorType enum to convert to a string
   * @return a string representation of the error type. For example,
   *     "USER ERROR" if the error type is User
   */
  std::string IException::errorTypeToString(ErrorType type) {
    std::string result;

    switch(type) {
      case User:
        result = "USER ERROR";
        break;
      case Programmer:
        result = "PROGRAMMER ERROR";
        break;
      case Io:
        result = "I/O ERROR";
        break;
      case Unknown:
        result = "ERROR";
        break;
    }

    return result;
  }


  /**
   * Given a string, returns the error type associated with it.
   *
   * @param string the string to convert into the appropriate ErrorType
   * @return The ErrorType enum associated with the given string. For example,
   *     if the string reads "USER ERROR", will return type User.
   */
  IException::ErrorType IException::stringToErrorType(
      const std::string &string) {
    ErrorType result = Unknown;

    if(string == "USER ERROR")
      result = User;
    else if(string == "PROGRAMMER ERROR")
      result = Programmer;
    else if(string == "I/O ERROR")
      result = Io;

    return result;
  }


  /**
   * Returns a C string containing a string representation of this exception.
   * This method should be called any time the current exception is modified
   * (e.g. appending an exception to the previous exceptions list, etc.) in
   * order to keep m_what synched with the current state of the exception.
   *
   * @return a C string representation of this exception
   */
  char *IException::buildWhat() const {
    std::string whatStr = toString();

    char *result = new char[whatStr.size() + 1];
    strncpy(result, whatStr.c_str(), whatStr.size());
    result[whatStr.size()] = '\0';

    return result;
  }


  /**
   * This is a helper method for the constructors. When the message or source
   *   code file name are empty strings, we want our members to be NULL instead
   *   of empty IStrings.
   */
  void IException::deleteEmptyMemberStrings() {
    if (m_message->size() == 0) {
      delete m_message;
      m_message = NULL;
    }

    if (m_fileName->size() == 0) {
      delete m_fileName;
      m_fileName = NULL;
    }
  }
}
