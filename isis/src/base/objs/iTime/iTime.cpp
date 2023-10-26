/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <iomanip>
#include <sstream>

#include <QString>

#include "Preference.h"

#include "FileName.h"
#include "IString.h"
#include "iTime.h"
#include "SpecialPixel.h"
#include "NaifStatus.h"

using namespace std;
namespace Isis {

  // Static initializations
  bool iTime::p_lpInitialized = false;

  //---------------------------------------------------------------------------
  // Constructors
  //---------------------------------------------------------------------------

  //! Constructs an empty iTime object.
  iTime::iTime() {
    p_et = 0.0;
  }

  /**
   * Constructs a iTime object and initializes it to the time from the argument.
   *
   * @param time A time string formatted in standard UTC or similar format.
   *             Example:"2000/12/31 23:59:01.6789" or "2000-12-31T23:59:01.6789"
   */
  iTime::iTime(const QString &time) {
    LoadLeapSecondKernel();

    NaifStatus::CheckErrors();

    // Convert the time string to a double ephemeris time
    SpiceDouble et;
    str2et_c(time.toLatin1().data(), &et);

    p_et = et;
    NaifStatus::CheckErrors();
  }


  //---------------------------------------------------------------------------
  // Public members
  //---------------------------------------------------------------------------

  /**
   * Changes the value of the iTime object.
   *
   * @param time A time string formatted in standard UTC or similar format.
   *             Example:"2000/12/31 23:59:01.6789" or "2000-12-31T23:59:01.6789"
   */
  void iTime::operator=(const QString &time) {
    LoadLeapSecondKernel();

    NaifStatus::CheckErrors();
    // Convert the time string to a double ephemeris time
    SpiceDouble et;
    str2et_c(time.toLatin1().data(), &et);

    p_et = et;
    NaifStatus::CheckErrors();
  }

  // Overload of "=" with a c string
  void iTime::operator=(const char *time) {
    LoadLeapSecondKernel();

    NaifStatus::CheckErrors();
    // Convert the time string to a double ephemeris time
    SpiceDouble et;
    str2et_c(time, &et);

    p_et = et;
    NaifStatus::CheckErrors();
  }


  // Overload of "=" with a double
  void iTime::operator=(const double time) {
    LoadLeapSecondKernel();
    p_et = time;
  }

  /**
   * Compare two iTime objects for greater than or equal
   *
   * @param time The iTime object to be compared to "this"
   *
   * @return bool
   */
  bool iTime::operator>=(const iTime &time) {
    return (p_et >= time.p_et);
  }

  /**
   * Compare two iTime objects for less than or equal
   *
   * @param time The iTime object to be compared to "this"
   *
   * @return bool
   */
  bool iTime::operator<=(const iTime &time) {
    return (p_et <= time.p_et);
  }

  /**
   * Compare two iTime objects for greater than
   *
   * @param time The iTime object to be compared to "this"
   *
   * @return bool
   */
  bool iTime::operator>(const iTime &time) {
    return (p_et > time.p_et);
  }


  /**
   * Compare two iTime objects for less than
   *
   * @param time The iTime object to be compared to "this"
   *
   * @return bool
   */
  bool iTime::operator<(const iTime &time) {
    return (p_et < time.p_et);
  }

  /**
   * Compare two iTime objects for inequality
   *
   * @param time The iTime object to be compared to "this"
   *
   * @return bool
   */
  bool iTime::operator!=(const iTime &time) {
    return (p_et != time.p_et);
  }

  /**
   * Compare two iTime objects for equality
   *
   * @param time The iTime object to be compared to "this"
   *
   * @return bool
   */
  bool iTime::operator==(const iTime &time) {
    return (p_et == time.p_et);
  }


  iTime iTime::operator +(const double &secondsToAdd) const {
    iTime tmp(*this);
    tmp += secondsToAdd;
    return tmp;
  }


  void iTime::operator +=(const double &secondsToAdd) {
    if(!IsSpecial(secondsToAdd) && !IsSpecial(p_et))
      p_et += secondsToAdd;
  }


  iTime operator +(const double &secondsToAdd, iTime time) {
    time += secondsToAdd;
    return time;
  }




  iTime iTime::operator -(const double &secondsToSubtract) const {
    iTime tmp(*this);
    tmp -= secondsToSubtract;
    return tmp;
  }


  double iTime::operator -(const iTime &iTimeToSubtract) const {
    return p_et - iTimeToSubtract.p_et;
  }


  void iTime::operator -=(const double &secondsToSubtract) {
    if (!IsSpecial(secondsToSubtract) && !IsSpecial(p_et))
      p_et -= secondsToSubtract;
  }


  iTime operator -(const double &secondsToSubtract, iTime time) {
    time -= secondsToSubtract;
    return time;
  }






  /**
   * Returns the year portion of the time as a string
   *
   * @return string
   */
  QString iTime::YearString() const {
    return toString(Year());
  }

  /**
   * Returns the year portion of the time as an int
   *
   * @return int
   */
  int iTime::Year() const {
    NaifStatus::CheckErrors();
    SpiceChar out[5];

    // Populate the private year member
    timout_c(p_et, "YYYY", 5, out);
    NaifStatus::CheckErrors();
    return IString(out).ToInteger();
  }

  /**
   * Returns the month portion of the time as a string
   *
   * @return string
   */
  QString iTime::MonthString() const {
    return toString(Month());
  }

  /**
   * Returns the month portion of the time as an int
   *
   * @return int
   */
  int iTime::Month() const {
    NaifStatus::CheckErrors();
    SpiceChar out[3];

    // Populate the private year member
    timout_c(p_et, "MM", 3, out);
    NaifStatus::CheckErrors();
    return IString(out).ToInteger();
  }

  /**
   * Returns the dat portion of the time as a string
   *
   * @return string
   */
  QString iTime::DayString() const {
    return toString(Day());
  }

  /**
   * Returns the day portion of the time as an int
   *
   * @return int
   */
  int iTime::Day() const {
    NaifStatus::CheckErrors();
    SpiceChar out[3];

    // Populate the private year member
    timout_c(p_et, "DD", 3, out);
    NaifStatus::CheckErrors();
    return IString(out).ToInteger();
  }

  /**
   * Returns the hour portion of the time as a string
   *
   * @return string
   */
  QString iTime::HourString() const {
    return toString(Hour());
  }

  /**
   * Returns the hour portion of the time as an int
   *
   * @return int
   */
  int iTime::Hour() const {
    NaifStatus::CheckErrors();
    SpiceChar out[3];

    // Populate the private year member
    timout_c(p_et, "HR", 3, out);
    NaifStatus::CheckErrors();
    return IString(out).ToInteger();
  }

  /**
   * Returns the minute portion of the time as a string
   *
   * @return string
   */
  QString iTime::MinuteString() const {
    return toString(Minute());
  }

  /**
   * Returns the minute portion of the time as an int
   *
   * @return int
   */
  int iTime::Minute() const {
    NaifStatus::CheckErrors();
    SpiceChar out[3];

    // Populate the private year member
    timout_c(p_et, "MN", 3, out);
    NaifStatus::CheckErrors();
    return IString(out).ToInteger();
  }

  /**
   * Returns the second portion of the time as a string
   *
   * @return string
   */
  QString iTime::SecondString(int precision) const {
    ostringstream osec;
    osec.setf(ios::fixed);
    osec << setprecision(precision) << Second();
    QString sSeconds(osec.str().c_str());
    sSeconds = sSeconds.remove(QRegExp("(\\.0*|0*)$"));

    if(sSeconds.isEmpty()) sSeconds = "0";
    return sSeconds;
  }

  /**
   * Returns the second portion of the time as a double
   *
   * @return double
   */
  double iTime::Second() const {
    NaifStatus::CheckErrors();
    SpiceChar out[256];

    // Populate the private year member
    timout_c(p_et, "SC.#######::RND", 256, out);
    NaifStatus::CheckErrors();
    return IString(out).ToDouble();
  }

  /**
   * Returns the day of year portion of the time as a string
   *
   * @return string
   */
  QString iTime::DayOfYearString() const {
    return toString(DayOfYear());
  }

  /**
   * Returns the day of year portion of the time as an int
   *
   * @return int
   */
  int iTime::DayOfYear() const {
    NaifStatus::CheckErrors();
    SpiceChar out[4];

    // Populate the private year member
    timout_c(p_et, "DOY", 4, out);
    NaifStatus::CheckErrors();
    return IString(out).ToInteger();
  }

  /**
   * Returns the ephemeris time (TDB) representation of the time as a string.
   * See the Naif documentation "time.req" for more information.
   *
   * @return string
   */
  QString iTime::EtString() const {
    return toString(p_et);
  }

  /**
   * Returns the internally stored time, formatted as a UTC time
   *
   * @return string The internalized time, in UTC format
   */
  QString iTime::UTC(int precision) const {
    QString utc = YearString() + "-" ;
    if(Month() < 10) utc += "0" + MonthString() + "-";
    else utc += MonthString() + "-";

    if(Day() < 10) utc += "0" + DayString() + "T";
    else utc += DayString() + "T";

    if(Hour() < 10) utc += "0" + HourString() + ":";
    else utc += HourString() + ":";

    if(Minute() < 10) utc += "0" + MinuteString() + ":";
    else utc += MinuteString() + ":";

    if(Second() < 10) utc += "0" + SecondString(precision);
    else utc += SecondString(precision);

    return utc;
  }

  void iTime::setEt(double et) {
    if(!IsSpecial(et))
      p_et = et;
    else
      p_et = 0.0;
  }

  void iTime::setUtc(QString utcString) {
    // If the time string is in ISO basic format add separators for utc2et
    if ( utcString.contains("T") && // Check for ISO T format
         !utcString.contains("-") && // Check for missing data separator
         !utcString.contains(":")) { // Check for missing time separator
      QString dateString = utcString.split("T").front();
      dateString.insert(4, "-");
      // If format is YYYYDOY we are done with the date string
      // Otherwise we are in YYYYMMDD format
      if (dateString.size() > 8) {
        dateString.insert(7, "-");
      }

      QString timeString = utcString.split("T").back();
      // If the format is hh or hhmm, resize and pad with 0s out to hh0000 or hhmm00
      if (timeString.size() < 6) {
        timeString.resize(6, '0');
      }
      timeString.insert(2, ":");
      timeString.insert(5, ":");

      utcString = dateString + "T" + timeString;
    }

    NaifStatus::CheckErrors();
    LoadLeapSecondKernel();

    double et;
    utc2et_c(utcString.toLatin1().data(), &et);
    setEt(et);
    NaifStatus::CheckErrors();
  }

  //---------------------------------------------------
  // Private members
  //---------------------------------------------------


  //! Uses the Naif routines to load the most current leap second kernel.
  void iTime::LoadLeapSecondKernel() {
    // Inorder to improve the speed of iTime comparisons, the leapsecond
    // kernel is loaded only once and left open.
    if(p_lpInitialized) return;

    // Get the leap second kernel file open
    Isis::PvlGroup &dataDir = Isis::Preference::Preferences().findGroup("DataDirectory");
    QString baseDir = QString::fromStdString(dataDir["Base"]);
    baseDir += "/kernels/lsk/";
    FileName leapSecond(baseDir + "naif????.tls");
    QString leapSecondName;
    try {
      leapSecondName = QString(leapSecond.highestVersion().expanded());
    }
    catch (IException &e) {
      QString msg = "Unable to load leadsecond file. Either the data area is not set or there are no naif####.tls files present";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }

    NaifStatus::CheckErrors();
    furnsh_c(leapSecondName.toLatin1().data());
    NaifStatus::CheckErrors();

    p_lpInitialized = true;
  }

  /**
   * Returns the current Greenwich Mean iTime
   * The time is based on the system time, so it is only as
   * accurate as the local system clock.
   *
   * @return QString The Current GMT
   */
  QString iTime::CurrentGMT() {
    time_t startTime = time(NULL);
    struct tm *tmbuf = gmtime(&startTime);
    char timestr[80];
    strftime(timestr, 80, "%Y-%m-%dT%H:%M:%S", tmbuf);
    return (QString) timestr;
  }


  /**
   * Returns the current local time
   * This time is taken directly from the system clock, so
   * if the system clock is off, this will be, too.
   *
   * @return QString The cutrrent local time
   */
  QString iTime::CurrentLocalTime() {
    time_t startTime = time(NULL);
    struct tm *tmbuf = localtime(&startTime);
    char timestr[80];
    strftime(timestr, 80, "%Y-%m-%dT%H:%M:%S", tmbuf);
    return (QString) timestr;
  }
} // end namespace isis
