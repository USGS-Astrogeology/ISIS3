#ifndef IString_h
#define IString_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <string>
#include <vector>

#include <QStringList>

#include "Constants.h"

namespace Isis {
  class IString;

  bool toBool(const std::string &);
  int toInt(const std::string &);
  BigInt toBigInt(const std::string &);
  double toDouble(const std::string &);

  std::string toString(bool);
  std::string toString(char);
  std::string toString(const int &);
  std::string toString(const unsigned int &);
  std::string toString(const BigInt &);
  std::string toString(double, int precision = 14);

  /**
   * @brief Adds specific functionality to C++ strings
   *
   * This class extends the standard C++ string class with specific functionality
   * useable by ISIS programmers.
   *
   * @ingroup Parsing
   *
   * @author 2002-09-10 Stuart Sides
   *
   * @internal
   *  @history 2003-02-05 Jeff Anderson - Modified the constructor routine which
   *                                      accepts a type of double (i.e.,
   *                                      conversion of a double to a string). The
   *                                      constructor now generates nice output as
   *                                      indicated in the documentation.
   *  @history 2003-05-16 Stuart Sides - Modified schema from astrogeology...
   *                                     isis.astrogeology...
   *  @history 2003-05-30 Jeff Anderson - Updated unitTest and truth file to
   *                                      account for optimzation changes
   *  @history 2003-05-30 Stuart Sides - Modified conversion of double to string
   *                                     constructor to output 14 place of
   *                                     accuracy
   *  @history 2003-06-24 Stuart Sides - Modified UpCase and DownCase to use the
   *                                     transform from the STL instead of looping
   *                                     and converting individual chars
   *  @history 2003-06-25 Stuart Sides - Added member function to remove all
   *                                     characters which are in the parameter
   *                                     from the object string (Remove)
   *  @history 2003-06-25 Stuart Sides - Added documentation for new member
   *                                     (Remove)
   *  @history 2003-07-17 Stuart Sides - Fixed bug in Convert. It sometimes would
   *                                     not convert any characters even though it
   *                                     should have. find_first_of should be
   *                                     "!=npos" instead of ">0".
   *  @history 2003-08-14 Stuart Sides - Fixed bug in Token. It would not parse
   *                                     "a","b" correctly.
   *  @history 2004-02-20 Stuart Sides - Added ability for ToDouble to convert PDS
   *                                     hex values to a double
   *  @history 2004-04-14 Jeff Anderson - Added (int) (double) and (string) cast
   *                                      conversions and operator= methods
   *  @history 2005-02-15 Elizabeth Ribelin - Modified file to support Doxygen
   *                                          documentation
   *  @history 2005-05-20 Jeff Anderson - Added BigInt methods
   *
   *  @history 2006-01-23 Jacob Danton - Changed the scientific notation start value (from 0.1 to 0.0001)
   *
   *  @history 2007-06-05 Brendan George - Merged with StringTools
   *
   *  @history 2007-07-17 Steven Lambright - Fixed bug where -0.0 would result in SetDouble
   *
   *  @history 2007-07-25 Steven Koechle - Fixed a bug where one of the TrimTail
   *                                      methods was calling erase incorrectly.
   *
   *  @history 2008-02-08 Steven Koechle - Keeps Convert from infinite looping
   *
   *  @history 2008-02-22 Steven Koechle - Added a Replace method that honors quotes
   *
   *  @history 2008-06-12 Jeannie Walldren - Fixed a bug in
   *           Compress method so that it is able to deal with
   *           multiple quotations within the string
   *
   *  @history 2008-06-18 Stuart Sides - Fixed doc error
   *
   *  @history 2005-02-15 Stuart Sides - add coded and implementation examples to
   *                                     class documentation and document
   *                                     IString(const char *str), and private
   *                                      methods
   *  @history 2008-07-14 Steven Lambright - Made some members const
   *  @history 2008-07-16 Steven Lambright - Added support for double nans and
   *                                      inifinities
   *
   *  @history 2009-11-02 Mackenzie Boyd - Modified Token method
   *                                       to ignore any quote
   *                                       groupings. Problems
   *                                       arose with current
   *                                       method and unclosed quotes.
   *  @history 2010-03-18 Sharmila Prasad - Ability to set the exact precision digits for double
   *  @history 2010-09-27 Sharmila Prasad - Moved ParseExpression from ControlNetFilter to IString class
   *  @history 2010-10-04 Sharmila Prasad - Remove redundant ParseExpression
   *  @history 2011-06-16 Jai Rideout - Fixed size of double string buffer to
   *                                    work with doubles that are -DBLMAX
   *  @history 2012-08-20 Steven Lambright - Deprecated. Please use IString instead of IString or
   *                          std::string. This file now provides toBool(), toInt(), toBigInt(),
   *                          toDouble(), and toString() which are not deprecated. Renamed from
   *                          iString to IString to better match our new naming conventions and
   *                          because this class isn't going to be removed overnight. Here are
   *                          some equivalents to IString functionality:
   *                            Trim() - Please use IString::trimmed(), IString::simplified() or
   *                                     IString::remove(QRegExp("(^[abc]*|[abc]*$)"))
   *                            TrimHead() - Please use IString::trimmed(), IString::simplified() or
   *                                         IString::remove(QRegExp("^[abc]*"))
   *                            TrimTail() - Please use IString::trimmed(), IString::simplified() or
   *                                         IString::remove(QRegExp("[abc]*$"))
   *                            UpCase() - Please use IString::toUpper()
   *                            DownCase() - Please use IString::toLower()
   *                            ToQt() - N/A
   *                            Token() - Please use IString::split() or IString::section()
   *                            Split() - Please use IString::split()
   *                            Replace() - Please use IString::replace(). If you need to respect
   *                                       quotes, please create a standard-compliant static
   *                                       method
   *                            Convert() - Please use IString::replace()
   *                            ConvertWhiteSpace() - Please use IString::simplified() or
   *                                                 IString::replace(QRegExp("\\s"), " ")
   *                            Remove() - Please use IString::remove()
   *                            operator IString() - N/A
   *                            Equal() - Please use operator==()
   *                            ToStd() - Please use IString::toStdString()
   *                            ToQt(vector) - This is not a string operation, it's more of a string
   *                                           list operation.
   *                            ToStd(IStringList) - This is not a string operation, it's more of
   *                                                 a string list operation.
   *                            operator int()/ToInteger() - see the new function toInt()
   *                            operator double()/ToDouble() - see the new function toDouble()
   *                            operator BigInt()/ToBigInteger() - see the new function toBigInt()
   *                            operator=(const int &) - toString() handles this
   *                            operator=(const BigInt &) - toString() handles this
   *                            operator=(const double &) - toString() handles this
   *                            SetDouble() - No longer necessary, only one method converts from a
   *                                          double to a string
   *                            Compress() - Please use IString::trimmed(), IString::simplified()
   *                                         or IString::replace(). If you need to respect quotes,
   *                                         please create a standard-compliant static method.
   *  @history 2012-10-13 Kris Becker - Fixed compatability issue with Qt on MacOSX
   *  @history 2012-12-18 Steven Lambright - Isis' API now only utilizes QStrings. Please use the
   *                          toString(...) and to*(QString) (i.e. toBool(QString), toInt(QString),
   *                          ...) methods. All public API's should use/expect QString (not IString,
   *                          not std::string). This affects virtually every class in Isis. 
   *                          Fixes #1312.
   *  @history 2017-08-30 Tyler Wilson and Ian Humphrey - Added std:: namespace to isnan to avoid
   *                          ambiguity error when compiling for c++11. References #4809.
   */
  class IString : public std::string {
    public:
      IString();

      IString(const std::string &str);
      IString(const IString &str);
      IString(const char *str);
      IString(const int &num);
      IString(const double &num, const int piPrecision = 14);
      IString(const BigInt &num);
      IString(const QString &str);

      ~IString();

      IString Trim(const std::string &chars);
      static std::string Trim(const std::string &chars, const std::string &str);

      IString TrimHead(const std::string &chars);
      static std::string TrimHead(const std::string &chars, const std::string &str);

      IString TrimTail(const std::string &chars);
      static std::string TrimTail(const std::string &chars, const std::string &str);

      IString UpCase();
      static std::string UpCase(const std::string &str);

      IString DownCase();
      static std::string DownCase(const std::string &str);

      int ToInteger() const;
      static int ToInteger(const std::string &str);

      BigInt ToBigInteger() const;
      static BigInt ToBigInteger(const std::string &str);

      double ToDouble() const;
      static double ToDouble(const std::string &str);

      IString Token(const IString &separator);
      static int Split(const char separator, const std::string &instr,
                       std::vector<std::string> &tokens,
                       bool allowEmptyEntries = true);

      IString Compress(bool force = false);
      static std::string Compress(const std::string &str, bool force = false);

      IString Replace(const std::string &from, const std::string &to,
                      int maxReplaceCount = 20);
      static std::string  Replace(const std::string &str,
                                  const std::string &from,
                                  const std::string &to,
                                  int maxReplacementCount = 20);

      IString Replace(const std::string &from, const std::string &to , bool honorquotes);
      static IString Replace(const std::string &str, const std::string &from,
                             const std::string &to , bool honorquotes);

      IString Convert(const std::string &listofchars, const char &to);
      static std::string Convert(const std::string &str,
                                 const std::string &listofchars,
                                 const char &to);

      IString ConvertWhiteSpace();
      static std::string ConvertWhiteSpace(const std::string &str);

      IString Remove(const std::string &del);
      static std::string Remove(const std::string &del, const std::string &str);

      /**
       * Attempts to convert the stirng to a 32 bit integer and return that int
       *
       * @return int
       */
      operator int() const {
        return ToInteger();
      };

      /**
       * Attempts to convert the stirng to a 64 bit integer and return that
       * int
       *
       * @return BigInt
       */
      operator BigInt() const {
        return ToBigInteger();
      };

      /**
       * Attempts to convert the stirng to a 64 bit double and return that double
       *
       * @return double
       */
      operator double() const {
        return ToDouble();
      };

      /**
       * Attempts to convert the stirng to a QStirng (Qt) and return that IString
       *
       * @return IString
       */
#if 0
      operator IString() const {
        return ToQt();
      };
#endif

      IString &operator= (const int &value);

      IString &operator= (const BigInt &value);

      /**
       * Attempts to convert double into its string representation
       *
       * @param value [in] The double to be converted to a string
       *
       * @return The IString representation of the double
       */
      IString &operator= (const double &value) {
        SetDouble(value);
        return *this;
      }

      bool Equal(const std::string &str) const;
      static bool Equal(const std::string &str1, const std::string &str2);

    private:
      void SetDouble(const double &value, const int piPrecision = 14);
  };

  std::ostream &operator<<(std::ostream &os, const QString &string);
  std::ostream &operator<<(std::ostream &os, const QStringRef &string);
}

#endif
