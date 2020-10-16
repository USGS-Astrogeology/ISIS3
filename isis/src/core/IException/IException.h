#ifndef IException_h
#define IException_h

/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <exception>
#include <string>

#include "FileName.h"

template <typename T> class QList;

class QString;

/**
 * Macro for the filename and line number. This is typically used for the last
 *   arguments to constructing an IException.
 */
#define _FILEINFO_ Isis::FileName(__FILE__).name().toStdString().c_str(),__LINE__

namespace Isis {
  class Pvl;

  /**
   * @brief Isis exception class
   *
   * This class represents a general Isis exception. It contains an enumeration
   * for what type of error the exception represents, and can optionally track
   * what exceptions caused the current exception. This class also provides
   * access to string and PVL representations of the exception.
   *
   * Instances of this class should be thrown by value and caught be reference.
   * Please see the constructor documentation for code examples on how to create
   * and throw exceptions.
   *
   * All methods in this class are re-entrant.
   *
   * @author ????-??-?? Jeff Anderson
   *
   * @internal
   *   @history 2005-05-10 Leah Dahmer - Added class documentation
   *   @history 2005-12-28 Elizabeth Miller - Fixed bug in Pvl error output
   *   @history 2006-06-12 Tracie Sucharski - Change clear method to static
   *   @history 2006-11-02 Jeff Anderson - Fixed bug in Report method for
   *                           exit status
   *   @history 2007-12-31 Steven Lambright - Added stack trace
   *   @history 2008-05-23 Steven Lambright - Added stack trace
   *   @history 2008-06-18 Stuart Sides - Fixed doc error
   *   @history 2008-07-08 Steven Lambright - Changed memory cleanup; now uses
   *                           atexit
   *   @history 2008-10-30 Steven Lambright - iException::Message now takes a
   *                           const char* for the filename, instead of a char*,
   *                           issue pointed out by "novus0x2a" (Support Board
   *                           Member)
   *   @history 2008-12-15 Steven Lambright - iException::what no longer returns
   *                           deleted memory.
   *   @history 2009-07-29 Steven Lambright - Stack trace calculations moved to
   *                           IsisDebug.h
   *   @history 2011-08-05 Steven Lambright - Hacked to make multi-threaded
   *                           code which throws exceptions work.
   *   @history 2012-03-07 Jai Rideout and Steven Lambright - Completely
   *                           refactored how this class works so that it no
   *                           longer stores messages in static memory. This
   *                           refactoring was necessary for the upcoming
   *                           control network suite project to allow this class
   *                           to work with multi-threading. The ErrorType enum
   *                           was also shortened to include only four relevant
   *                           error types, as the other error types were often
   *                           misused, ambiguous, completely forgotten, and
   *                           not helpful to users. The code was updated
   *                           appropriately to follow the current Isis coding
   *                           standards.
   *   @history 2012-03-13 Steven Lambright - toString() was giving "End" with
   *                           an empty exception when using Format = Pvl. It
   *                           should (and must) be an empty string. This is
   *                           fixed -- fixes #755.
   *   @history 2012-07-30 Jeff Anderson - Updated internal documentation
   *                           to improve backward compatibility
   *   @history 2018-08-06 Kaitlyn Lee - With the new cmake system, we run unit tests from
   *                           build/untiTest, while in the old make system, we ran unit tests
   *                           directly from the object's directory. This change caused the c macro
   *                           __FILE__ to include the full path of unitTest.cpp when it expands.
   *                           Consequently, we have to strip the path from __FILE__ and put only
   *                           the name of the file into _FILEINFO_.
   */
  class IException : public std::exception {
    public:
      /**
       * Contains a set of exception error types. These types indicate the
       * source of the error. For example, if the error type is User, this
       * indicates that the exception was thrown due to something the user did
       * wrong, such as provide a cube with no SPICE data to an application that
       * needed a spiceinit'd cube as input.
       */

       /*
       * WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
       * If at all possible do not change the enumeration values for the
       * error codes.  The reason why is it that will change the return
       * status of error messages.  Ground data processing groups (e.g.,
       * HiRISE, LROC, Messenger) will sometime test on the error return
       * values in their scripts.  By keeping the enumerations the same
       * we improve backward compatibility.
       * WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
       */
      enum ErrorType {
        /**
         * A type of error that cannot be classified as any of the other error
         * types. This should be the most common error type.
         *
         * If in doubt, always use this error type.
         */
        Unknown = 1,

        /**
         * A type of error that could only have occurred due to a mistake on the
         * user's part (e.g. bad input to the application). You must be able to
         * guarantee that the user provided the input in the context that is
         * throwing the error.
         */
        User,

        /**
         * This error is for when a programmer made an API call that was
         * illegal.
         *
         * This includes:
         *   Making an API call with illegal input data that could and should
         *   have been checked before the call was made. For example, an out of
         *   array bounds exception is a programmer exception because the caller
         *   should have checked the size of the array.
         *
         *   Making an API call that requires a certain class state. For
         *   example, you need to call open before read.
         *
         * These categories have a lot of overlap, but they are what you are
         * looking for. A programmer exception is not appropriate if the caller
         * has no way to validate their inputs to a method ahead of time, or if
         * the caller is not expected to validate their inputs ahead of time.
         */
        Programmer,

        /**
         * A type of error that occurred when performing an actual I/O
         * operation. For example, fread/fwrite calls. This also includes
         * files not existing, inter-processes socket communications, and
         * network communications. This does not include a file not having the
         * expected values inside of it (for example, reading a corrupted PVL).
         */
        Io
      };

      IException();

      IException(ErrorType type, const char *message,
                 const char *fileName, int lineNumber);

      IException(ErrorType type, const std::string &message,
                 const char *fileName, int lineNumber);

      IException(ErrorType type, const QString &message,
                 const char *fileName, int lineNumber);

      IException(const IException &caughtException,
                 ErrorType newExceptionType, const char *message,
                 const char *fileName, int lineNumber);

      IException(const IException &caughtException,
                 ErrorType newExceptionType, const std::string &message,
                 const char *fileName, int lineNumber);

      IException(const IException &caughtException,
                 ErrorType newExceptionType, const QString &message,
                 const char *fileName, int lineNumber);

      IException(const IException &other);

      ~IException() throw();

      const char *what() const throw();

      void append(const IException &exceptionSource);

      ErrorType errorType() const;
      static QString errorTypeToString(ErrorType t);
      void print() const;
      void print(bool printFileInfo) const;
      Pvl toPvl() const;
      QString toString() const;
      QString toString(bool printFileInfo) const;

      void swap(IException &other);
      IException &operator=(const IException &rhs);

    private:
      static IException createStackTrace();
      static ErrorType stringToErrorType(const QString &s);
      char *buildWhat() const;
      void deleteEmptyMemberStrings();

    private:
      /**
       * This is used to store the return value of what() in a way that
       *   guarantees the returned data will not be deleted as long as this
       *   instance is unmodified. Any changes to the current exception will
       *   reallocate this and invalidate old values of what().
       */
      char * m_what;

      /**
       * This exception's error source. If the source cannot be positively
       *   identified, then this should be set to Unknown.
       */
      ErrorType m_errorType;

      /**
       * The message associated with this exception. This will be NULL if the
       *   message is empty (or only contained whitespace).
       */
      QString * m_message;

      /**
       * The source code file that threw this exception.
       */
      QString * m_fileName;

      /**
       * The line in the source code file that threw this exception.
       */
      int m_lineNumber;

      /**
       * A list of exceptions that caused this exception.
       */
      QList<IException> * m_previousExceptions;
  };
};

#endif
