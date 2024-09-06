#ifndef HiCalUtil_h
#define HiCalUtil_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#include "HiCalTypes.h"
#include "DbProfile.h"
#include "IString.h"
#include "Statistics.h"
#include "PvlKeyword.h"
#include "NumericalApproximation.h"
#include "FileName.h"
#include "Pvl.h"
#include "IException.h"

namespace Isis {

/**
 * @brief Counts number of valid pixels in vector
 * @param v Vector to inspect
 * @return int Number valid pixels in vector
 */
inline int ValidCount(const HiVector &v) {
  int n = 0;
  for (int i = 0 ; i < v.dim() ; i++) {
    if (!IsSpecial(v[i])) n++;
  }
  return (n);
}

/**
 * @brief Counts number of invalid pixels in vector
 * @param v Vector to inspect
 * @return int Number invalid (special) pixels in vector
 */
inline int InValidCount(const HiVector &v) {
  int n = 0;
  for (int i = 0 ; i < v.dim() ; i++) {
    if (IsSpecial(v[i])) n++;
  }
  return (n);
}

/**
 * @brief Convert HiRISE Cpmm number to Ccd number
 *
 * @param cpmm Cpmm number
 */
inline int CpmmToCcd(int cpmm) {
  const int cpmm2ccd[] = {0,1,2,3,12,4,10,11,5,13,6,7,8,9};
  if ( (cpmm < 0) || (cpmm >= (int)(sizeof(cpmm2ccd)/sizeof(int))) ) {
    std::string mess = "CpmmToCdd: Bad CpmmNumber (" + toString(cpmm) + ")";
    throw IException(IException::User, mess, _FILEINFO_);
  }
  return (cpmm2ccd[cpmm]);
}


/**
 * @brief Convert HiRISE Ccd number to string filter name
 *
 * @param ccd Ccd number of device
 */
inline QString CcdToFilter(int ccd) {
  if ( (ccd < 0) || (ccd > 13) ) {
    std::string mess = "CcdToFilter: Bad Ccd Number (" + toString(ccd) + ")";
    throw IException(IException::User, mess, _FILEINFO_);
  }

  QString filter;
  if ( ccd <= 9 )     { filter = "RED"; }
  else if (ccd <= 11) { filter = "IR";  }
  else                { filter = "BG";  }
  return (filter);
}

/**
 * @brief Crop specified lines from a buffer
 *
 * This function extracts lines from a buffer and returns a new buffer with the
 * extracted lines.
 *
 * @param m      Buffer to extract lines from
 * @param sline  Starting line number (first line is 0)
 * @param eline  Last line to extract
 *
 * @return HiMatrix Buffer containing cropped lines
 */
inline HiMatrix cropLines(const HiMatrix &m, int sline, int eline) {
  int nlines(eline-sline+1);
  HiMatrix mcrop(nlines, m.dim2());
  for (int l =  0 ; l < nlines ; l++) {
    for (int s = 0 ; s < m.dim2() ; s++) {
      mcrop[l][s] = m[l+sline][s];
    }
  }
  return (mcrop);
}

/**
 * @brief Crop specified samples from a buffer
 *
 * This function extracts samples from a buffer and returns a new buffer with
 * the extracted samples.
 *
 * @param m      Buffer to extract samples from
 * @param ssamp  Startling sample (first sample 0)
 * @param esamp  Ending sample to extract
 *
 * @return HiMatrix  Buffer with cropped samples
 */
inline HiMatrix cropSamples(const HiMatrix &m, int ssamp, int esamp) {
  int nsamps(esamp-ssamp+1);
  HiMatrix mcrop(m.dim1(), nsamps);
  for (int l =  0 ; l < m.dim1() ; l++) {
    for (int s = 0 ; s < nsamps ; s++) {
      mcrop[l][s] = m[l][s+ssamp];
    }
  }
  return (mcrop);
}

/**
 * @brief Reduces by averaging specified lines from a buffer
 *
 * This function produces a vector from a 2-D buffer of averaged lines at each
 * vertical sample location
 *
 * @param m      Buffer to reduce
 * @param sline  Starting line number (first line is 0)
 * @param eline  Last line to average (-1 means use all lines)
 *
 * @return HiVector Buffer containing averaged lines
 */
inline HiVector averageLines(const HiMatrix &m, int sline = 0, int eline = -1) {
  eline = (eline == -1) ? m.dim1() - 1 : eline;
  HiVector v(m.dim2());
  for (int s =  0 ; s < m.dim2() ; s++) {
    Statistics stats;
    for (int l = sline ; l <= eline ; l++) {
      stats.AddData(&m[l][s], 1);
    }
    v[s] = stats.Average();
  }
  return (v);
}

/**
 * @brief Reduces by averaging specified samples from a buffer
 *
 * This function produces a vector from a 2-D buffer of averaged samples at each
 * horizontal line location
 *
 * @param m      Buffer to reduce
 * @param ssamp  Starting sample number (first sample is 0)
 * @param esamp  Last sample to average (-1 means use all samples)
 *
 * @return HiVector Buffer containing averaged samples
 */
inline HiVector averageSamples(const HiMatrix &m, int ssamp = 0,
                               int esamp = -1) {
  esamp = (esamp == -1) ? m.dim2() - 1 : esamp;
  HiVector v(m.dim1());
  for (int l =  0 ; l < m.dim1() ; l++) {
    Statistics stats;
    for (int s = ssamp ; s <= esamp ; s++) {
      stats.AddData(&m[l][s], 1);
    }
    v[l] = stats.Average();
  }
  return (v);
}

/**
 * @brief Find a keyword in a profile using default for non-existant keywords
 *
 * This template function will extract keyword values from a profile and provide
 * the values at the indicated index.  If the keyword does not exist or the
 * indicated value at index, the provided default will be used instead.
 *
 */
template <typename T>
  T ConfKey(const DbProfile &conf, const QString &keyname, const T &defval,
            int index = 0) {
  if (!conf.exists(keyname)) { return (defval); }
  if (conf.count(keyname) < index) { return (defval); }
  QString iValue(conf.value(keyname, index));
  T value = iValue;  // This makes it work with a string?
  return (value);
}

/**
 * @brief Helper function to convert values to Integers
 *
 * @param T Type of value to convert
 * @param value Value to convert
 *
 * @return int Converted value
 */
template <typename T> int ToInteger(const T &value) {
    return toInt(QString(value).trimmed());
}

/**
 * @brief Helper function to convert values to doubles
 *
 * @param T Type of value to convert
 * @param value Value to convert
 *
 * @return double Converted value
 */
template <typename T> double ToDouble(const T &value) {
    return toDouble(QString(value).trimmed());
}

/**
 * @brief Helper function to convert values to strings
 *
 * @param T Type of value to convert
 * @param value Value to convert
 *
 * @return string Converted value
 */
template <typename T> QString ToString(const T &value) {
    return (toString(value).trimmed());
}

/**
 * @brief Shortened string equality test
 *
 * @param v1 First value
 * @param v2 Second value
 *
 * @return bool True if they are equal w/o regard to case
 */
inline bool IsEqual(const QString &v1, const QString &v2 = "TRUE") {
    return (v1.toUpper() == v2.toUpper());
}

/**
 * @brief Determines if the keyword value is the expected value
 *
 * This function checks the existance of a keyword in a profile, extracts the
 * first value and tests it for equivelance to the expected value.  Note this
 * test is case insensitive.
 *
 * @param prof Profile to find the expected keyword in
 * @param key  Name of keyword in profile to test
 * @param value Value to test for in keyword
 *
 * @return bool Returns true only if the keyword exists in the profile and it is
 *              the value expected.
 */
inline bool IsTrueValue(const DbProfile &prof, const QString &key,
                   const QString &value = "TRUE") {
  if ( prof.exists(key) ) {
    return (IsEqual(prof(key), value));
  }
  return (false);
}

/**
 * @brief Checks profile flag to skip the current Module
 *
 * This function looks for the keyword \b Debug::SkipModule and checks its
 * value. True is returned if the value is TRUE (case insensentive).
 *
 * @param prof Module profile from config file
 *
 * @return bool  True if the value of the Debug::SkipModule keyword is TRUE,
 *         otherwise it returns false for all other values.
 */
inline bool SkipModule(const DbProfile &prof) {
  return (IsTrueValue(prof, "Debug::SkipModule", "TRUE"));
}


inline HiMatrix appendLines(const HiMatrix &top, const HiMatrix &bottom) {
  //  Ensure same number samples
  if (top.dim2() != bottom.dim2()) {
       std::ostringstream mess;
        mess << "Top buffer samples (" << top.dim2()
             << ") do not match bottom buffer samples (" << bottom.dim2()
             << ")";
        throw IException(IException::User, mess.str(), _FILEINFO_);
  }

  int nlines(top.dim1()+bottom.dim1());
  HiMatrix mat(nlines, top.dim2());
  for (int lt = 0 ; lt < top.dim1() ; lt++) {
    for (int s = 0 ; s < top.dim2() ; s++) {
      mat[lt][s] = top[lt][s];
    }
  }

  int topl = top.dim1();
  for (int lb = 0 ; lb < bottom.dim1() ; lb++) {
    for (int s = 0 ; s < top.dim2() ; s++) {
      mat[topl+lb][s] = bottom[lb][s];
    }
  }
  return (mat);
}

inline HiMatrix appendSamples(const HiMatrix &left, const HiMatrix &right) {
  //  Ensure same number samples
  if (right.dim1() != right.dim1()) {
       std::ostringstream mess;
        mess << "Left buffer lines (" << left.dim1()
             << ") do not match right buffer lines (" << right.dim1()
             << ")";
        throw IException(IException::User, mess.str(), _FILEINFO_);
  }

  int nsamps(left.dim2()+right.dim2());
  HiMatrix mat(left.dim1(), nsamps);
  for (int ll = 0 ; ll < left.dim1() ; ll++) {
    for (int s = 0 ; s < left.dim2() ; s++) {
      mat[ll][s] = left[ll][s];
    }
  }

  int lefts = left.dim2();
  for (int lr = 0 ; lr < right.dim1() ; lr++) {
    for (int s = 0 ; s < right.dim2() ; s++) {
      mat[lr][lefts+s] = right[lr][s];
    }
  }
  return (mat);
}

/**
 * @brief Compute HiRISE line times
 *
 * This class will compute the time (in seconds) for a HiRISE observation line
 * based upon the binning mode and line number.  It is assumed that the first
 * line will be time 0, but that is up to the caller to keep that straight.
 */
class HiLineTimeEqn {
  public:
    HiLineTimeEqn() : _bin(1), _ltime(1.0) { }
    HiLineTimeEqn(int bin, double ltime) : _bin(bin), _ltime(ltime) { }
    virtual ~HiLineTimeEqn() { }

    void setLineTime(double ltime) { _ltime = ltime; }
    void setBin(int bin) { _bin = bin; }
    double Time(const double line) const {
      return (line * (_bin * _ltime * 1.0E-6));
    }
    double operator()(const double line) const { return (Time(line)); }

  private:
    double _bin;
    double _ltime;
};

/**
 * @brief Implements (classic) HiRISE temperature equation
 *
 * This function computes the dark current temperature and returns the results
 * in electrons/sec/pixel.
 *
 * @param temperature Temperature (typically of the FPGA)
 * @param napcm2      Dark current for silicone diodes (nano-ampres/cm^2)
 * @param px          Pixel size in microns
 *
 * @return double     Dark current temperature (electrons/sec/pixel)
 */
inline double HiTempEqn(const double temperature, const double napcm2 = 2.0,
                        const double px = 12.0) {
  double temp = temperature + 273.0;
  double eg   = 1.1557 - (7.021E-4 * temp * temp) / (1108.0 + temp);
  const double K = 1.38E-23;
  const double Q = 1.6E-19;
  return (napcm2*(px*px)*2.55E7*std::pow(temp,1.5)*std::exp(-eg*Q/2.0/K/temp));
}

/**
 * @brief Rebins a vector to a different size
 *
 * This function can rebin to both larger and smaller sizes.  It fits the data
 * to a cubic spline and then computes the value at the rebin pixel index.  One
 * advantage to this approach is that on input, special pixels are ignored - on
 * output there will never be special pixels unless there are not enough points
 * to conpute the cubic spline on which case this function throws an exception.
 *
 * @param v  Input vector to rebin
 * @param n  Size of the new output vector
 *
 * @return HiVector
 * @history 2008-11-05 Jeannie Walldren Replaced references to
 *          DataInterp class with NumericalApproximation.
 */
inline HiVector rebin(const HiVector &v, int n) {
  if ( n == v.dim() ) { return (v); }
  NumericalApproximation nterp(NumericalApproximation::CubicNatural);
  double mag((double) v.dim()/ (double) n);

  for ( int i = 0 ; i < v.dim() ; i++ ) {
    double x((double) i);
    if ( !IsSpecial(v[i]) ) {
      nterp.AddData(x,v[i]);
    }
  }

  // Compute the spline and fill the output vector
  HiVector vout(n);
  for ( int j = 0 ; j < n ; j++ ) {
    double x = (double) j * mag;
    vout[j] = nterp.Evaluate(x, NumericalApproximation::NearestEndpoint);
  }
  return (vout);
}

/**
 * @brief Deletes HiRISE specific BLOBS from cube file
 *
 * Ths function removes only the HiRISE specific
 *
 * @param label Input label associated with file from which to remove the HiRISE
 *              blobs
 */
inline void RemoveHiBlobs(Pvl &label) {
  for ( int blob = 0 ; blob < label.objects() ; blob++ ) {
    if ( label.object(blob).isNamed("Table") ) {
      QString name = QString::fromStdString(label.object(blob)["Name"][0]);
      if ( name.toLower() == "hirise calibration ancillary" ||
           name.toLower() == "hirise calibration images" ||
           name.toLower() == "hirise ancillary") {
        label.deleteObject(blob);
        blob--;
      }
    }
  }
  return;
}

/**
 * @brief Return the statistics for a vector of data
 *
 * The default statistic returned is the median of the dataset but can be
 * changed with a compile time change.  The vector passed in will be sorted so
 * that the median can be determined.  If the vector has an even number of
 * elements in it, the average of the two center values will be returned.  The
 * array is assumed to be clean data, no ISIS special pixels.
 *
 * @param data    Vector containing data compute the statistic.  It will be
 *                altered (sorted) upon return to the caller.
 *
 * @return double The median (default) of the data
 */
inline double GainLineStat(std::vector<double> &data) {

  //  Check for special conditions
  if (data.size() == 0)  return (Null);
  if (data.size() == 1) return (data[0]);

#if defined(USE_AVERAGE)
  Statistics stats;
  stats.AddData(&data[0], data.size());
  return (stat.Average());
#else
  std::sort(data.begin(), data.end());
  int meanIndex = data.size()/2;
  if ((data.size() % 2) == 1)  return data[meanIndex];
  return ((data[meanIndex-1]+data[meanIndex])/2.0);
#endif
}

};     // namespace Isis
#endif
