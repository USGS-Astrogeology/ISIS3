/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Histogram.h"

#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "LineManager.h"
#include "Message.h"
#include "SpecialPixel.h"

#include <iostream>
#include <math.h>
#include <stdio.h>
#include <string>

using namespace std;

namespace Isis {

  /**
   * Constructs a histogram object. Only data between the minimum and maximum
   * will be binned, and the bin range will be from the minimum to the maximum.
   *
   * @param minimum Minimum value for binning the data into the histogram.
   * @param maximum Maximum value for binning the data into the histogram.
   * @param nbins The number of bins to use
   */
  Histogram::Histogram(double minimum, double maximum, int nbins) {

    SetValidRange(minimum, maximum);
    SetBins(nbins);
  }


  /**
   * Constructs a histogram from a control netowrk
   *
   * @param net: Reference to a ControlNetwork used to access all the measures.
   * @param statFunc: Pointer to a ControlMeasure acessor, the returns of this
   *    function call will be used to build up the network.
   * @param bins:  The number of bins to divide the histogram into.
   * @throws The number of Histogram Bins must be greater than 0.
   */
  Histogram::Histogram(ControlNet &net, double(ControlMeasure::*statFunc)() const, int bins) {

    //check to make sure we have a reasonable number of bins
    if (bins < 1) {
      string msg = "The number of Histogram Bins must be greater than 0";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    SetBins(bins);

    //set the ranges
    rangesFromNet(net,statFunc);
    //add all the data to the now setup histogram
    addMeasureDataFromNet(net, statFunc);
  }


  /**
   * Constructs a histogram from a control netowrk
   *
   * @param Net:  Reference to a ControlNetwork used to access all the measures.
   * @param statFunc:  Pointer to a ControlMeasure acessor, the returns of this.
   *    function call will be used to build up the network.
   * @param binWidth:  The width of histogram bins.
   * @throws The width of Histogram Bins must be greater than 0.
   */
  Histogram::Histogram(ControlNet &net, double(ControlMeasure::*statFunc)() const,
                       double binWidth) {

    //check to make sure we have a reasonable number of bins
    if (binWidth <= 0 ) {
      string msg = "The width of Histogram Bins must be greater than 0";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

   //get the range of the data
   rangesFromNet(net,statFunc);

   //Keep an eye on this to see if it breaks anything.  Also, I need to create
   //a dataset to give this constructor for the unit test.

   //from the domain of the data and the requested bin width calculate the number of bins
   double domain = this->ValidMaximum() - this->ValidMinimum();
   int nBins = int ( ceil(domain/binWidth) ) - 1;
   SetBins(nBins);
   //add all the data to the now setup histogram
   addMeasureDataFromNet(net, statFunc);
  }


  /**
   * Iterates through all the measures in a network adding them to the histogram
   *
   * @param net  reference to a ControlNetwork used to access all the measures
   * @param statFunc  pointer to a ControlMeasure acessor, the returns of this
   *    function call will be used to build up the network
   */
  void Histogram::addMeasureDataFromNet(ControlNet &net,
                                        double(ControlMeasure::*statFunc)() const) {

    //get the number of object points
    int nObjPts =  net.GetNumPoints();
    for (int i=0; i<nObjPts; i++) { //for each Object point
      const ControlPoint *point = net.GetPoint(i);
      if (point->IsIgnored() ) continue;  //if the point is ignored then continue

      //get the number of measures
      int nObs = point->GetNumMeasures();
      for (int j=0; j<nObs; j++) {  //for every measure

        const ControlMeasure *measure = point->GetMeasure(j);

        if (measure->IsIgnored())  continue;
        this->AddData((measure->*statFunc)());
      }
    }
  }


  /**
   * Iterates through all the measures in a network in order to find the domain of the data
   *
   * @param net  reference to a ControlNetwork used to access all the measures
   * @param statFunc  pointer to a ControlMeasure acessor, the returns of this function call
   *        will be used to build up the network
   * @throw The net file appears to have 1 or fewer measures, thus no histogram can be formed
   */
  void Histogram::rangesFromNet(ControlNet &net, double(ControlMeasure::*statFunc)() const) {
    double min,max,temp;
    min =  DBL_MAX;
    max = -DBL_MAX;

    //get the number of object points
    int nObjPts =  net.GetNumPoints();
    for (int i=0; i<nObjPts; i++) { //for each Object point

      const ControlPoint *point = net.GetPoint(i);
      if (point->IsIgnored()) continue;  //if the point is ignored then continue

      //get the number of measures
      int nObs = point->GetNumMeasures();

      for (int j=0; j<nObs; j++) {  //for every measure

        const ControlMeasure *measure = point->GetMeasure(j);
        if (measure->IsIgnored())  continue;

        temp = (measure->*statFunc)();  //get the data using the passed ControlMeasure acessor function pointer
        if ( !IsSpecial(temp) && temp > max ) max = temp;
        if ( !IsSpecial(temp) && temp < min ) min = temp;
      }
    }

    //if DBL_MAX's weren't changed there's a problem
    if (max <= min) {
      string msg = "The net file appears to have 1 or fewer measures with residual data, "
                   "thus no histogram for this net file can be created;";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    //set up the histogram ranges

    SetValidRange(min, max);
  }


  //! Destructs a histogram object.
  Histogram::~Histogram() {
  }

  //2015-08-24,  Tyler Wilson:  Added Statistics::SetValidRange call to SetValidRange
  //So the two functions do not have to be called together when setting
  //up a histogram

  /**
   * Changes the range of the bins.  This function also sets the range
   * of the parent statistics class and resets the stats/histogram
   * counters.   So binRange = setValidRange.  Should only be called
   * once, prior to Adding data to the histogram.
   *
   * @param binStart The start of the bin range
   * @param binEnd   The end of the bin range
   */

  void Histogram::SetValidRange(double binStart, double binEnd) {
    if (binEnd < binStart) {
      string msg = "The binning range start [" + IString(binStart) + "]"
                   " must be less than the end [" + IString(binEnd) + "].";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    //(tjw): Flush the data buffers.  Since we are setting
    //the statistical range for the data, any data loaded
    //before this call is useless.
    Reset();
    Isis::Statistics::SetValidRange(binStart,binEnd);
    p_binRangeStart = binStart;
    p_binRangeEnd = binEnd;
  }


  //! Resets histogram counters to zero.
  void Histogram::Reset() {
    Statistics::Reset();
    for (int i = 0; i < (int)p_bins.size(); i++) {
      p_bins[i] = 0;
    }
  }


  //! Change the number of bins in the histogram and reset counters
  void Histogram::SetBins(const int nbins) {
    p_bins.resize(nbins);
    Histogram::Reset();
  }

  /**
   * Add an array of doubles to the histogram counters. This method can be
   * invoked multiple times. For example, once for each line in a cube, before
   * obtaining statistics and histogram information.
   *
   * @param data Pointer to array of double to add.
   * @param count Number of doubles to process.
   */
  void Histogram::AddData(const double *data,
                          const unsigned int count) {
    Statistics::AddData(data, count);

    int nbins = p_bins.size();
    int index;
    for (unsigned int i = 0; i < count; i++) {
      if (IsValidPixel(data[i]) && InRange(data[i]) ) {
        if (BinRangeStart() == BinRangeEnd() ) {
          index = 0;
        }
        else {
          index = (int) floor( ((double) nbins / (BinRangeEnd() - BinRangeStart())) *
                              (data[i] - BinRangeStart()) );
        }
        if (index < 0) index = 0;
        if (index >= nbins) index = nbins - 1;
        p_bins[index] += 1;
      }
    }
  }


  /**
   * Add a single double data to the histogram.  Of course this can be invoke multiple times.
   * e.g. once for each residual in a network for instance.
   *
   * @param data a single observation to be added to the histogram
   */

  void Histogram::AddData(const double data) {
    Statistics::AddData(data);

    int nbins = p_bins.size();
    int index;
    if (IsValidPixel(data) && InRange(data) ) {
      if (BinRangeStart() == BinRangeEnd() ) {
        index = 0;
      }
      else {
        index = (int) floor( ((double) nbins / (BinRangeEnd() - BinRangeStart())) *
                              (data - BinRangeStart()) );
      }
      if (index < 0) index = 0;
      if (index >= nbins) index = nbins - 1;
      p_bins[index] += 1;
    }
  }


  /**
   * Remove an array of doubles from the histogram counters. Note that this
   * invalidates the absolute minimum and maximum. They will no longer be
   * useable.
   * @see Stats
   *
   * @param data Pointer to array of doubles to remove.
   * @param count number of doubles to process.
   */
  void Histogram::RemoveData(const double *data,
                             const unsigned int count) {
    Statistics::RemoveData(data, count);

    int nbins = p_bins.size();
    int index;
    for (unsigned int i = 0; i < count; i++) {
      if (IsValidPixel(data[i]) ) {

        if (BinRangeStart() == BinRangeEnd() ) {
          index = 0;
        }
        else {
          index = (int) floor( ((double) nbins / (BinRangeEnd() - BinRangeStart())) *
                              (data[i] - BinRangeStart()) );
        }
        if (index < 0) index = 0;
        if (index >= nbins) index = nbins - 1;
        p_bins[index] -= 1;
      }
    }
  }

  /**
   * Returns the median.
   *
   * @returns The median
   */
  double Histogram::Median() const {
    return Percent(50.0);
  }

  /**
   * Returns the mode.
   *
   * @returns The mode
   */
  double Histogram::Mode() const {
    int mode = 0;
    for (int i = 0; i < (int)p_bins.size(); i++) {

      if (p_bins[i] > p_bins[mode]) mode = i;
    }

    if (p_bins[mode] < 1) return NULL8;

    return BinMiddle(mode);
  }


  /**
   * Computes and returns the value at X percent of the histogram.  For example,
   * Percent(50.0) is equivalent to the computing the median. While Percent(0.5)
   * and Percent(99.5) would obtain a minimum and maximum for the data that
   * could be used for a good contrast stretch.
   *
   * @param percent X percent of the histogram to compute.
   *
   * @returns The value at X percent of the histogram.
   */
  double Histogram::Percent(double percent) const {
    if ( (percent < 0.0) || (percent > 100.0) ) {
      string m = "Argument percent outside of the range 0 to 100 in";
      m += " [Histogram::Percent]";
      throw IException(IException::Programmer, m, _FILEINFO_);
    }

    if (ValidPixels() < 1) return NULL8;

    BigInt currentPixels = 0;
    double currentPercent;

    for (int i = 0; i < (int)p_bins.size(); i++) {

      currentPixels += p_bins[i];
      currentPercent = (double) currentPixels / (double) ValidPixels() * 100.0;

      if (currentPercent >= percent) {
        return BinMiddle(i);
      }
    }

    return BinMiddle( (int) p_bins.size() - 1);
  }


  /**
   * Computes and returns the skew. If there are no valid pixels then NULL8 is
   * returned. Recognize that because of the binning which occurs, in order to
   * generate the histogram, the skew may not be precise but will be very close.
   *
   * @return The skew.
   */
  double Histogram::Skew() const {

    if (ValidPixels() < 1) return NULL8;

    double sdev = StandardDeviation();

    if (sdev == 0.0) return 0.0;

    return 3.0 * (Average() - Median()) / sdev;
  }


  /**
   * Returns the count at a bin position in the histogram.
   *
   * @param index Index of the desired bin 0 to Bins()-1.
   *
   * @return The count at a bin position in the histogram.
   */
  BigInt Histogram::BinCount(const int index) const {

    if ( (index < 0) || (index >= (int)p_bins.size() ) ) {

      std::string message = Message::ArraySubscriptNotInRange(index);

      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    return p_bins[index];
  }


  /**
   * Returns the left edge and right edge values of a bin. That is the range of
   * data the bin covers.
   *
   * @throws iException The programmer has passed in an index outside of 0
   * to Bins()-1.
   *
   * @param index Index of the desired bin 0 to Bins()-1.
   * @param low The value at the left edge of the requested bin.
   * @param high The value at the right edge of the requested bin.
   */
  void Histogram::BinRange(const int index,
                           double &low, double &high) const {
    if ( (index < 0) || (index >= (int)p_bins.size() ) ) {

      std::string message = Message::ArraySubscriptNotInRange(index);
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    double binSize = (BinRangeEnd() - BinRangeStart()) / (double) p_bins.size();
    low = BinRangeStart() + binSize * (double) index;
    high = low + binSize;
  }


  /**
   * Returns the value represented by a bin. This is not the count, but the
   * actual data value at the middle of the bin.
   *
   * @param index Index of the desired bin 0 to Bins()-1.
   *
   * @returns The middle value of the bin.
   */
  double Histogram::BinMiddle(const int index) const {
    if ( (index < 0) || (index >= (int)p_bins.size() ) ) {

      std::string message = Message::ArraySubscriptNotInRange(index);

      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    double low, high;
    BinRange(index, low, high);
    return (low + high) / 2.0;
  }


  /**
   * Returns the size of an individual bin. Essentially, the difference of the
   * high and low edge values (BinRange) of a bin.  This value is constant for
   * all bins.
   *
   * @return The size of the individual bin.
   */
  double Histogram::BinSize() const {

    double low, high;
    BinRange(0, low, high);
    return high - low;
  }


  /**
   * Returns the number of bins in the histogram.
   *
   * @return The number of bins in the histogram.
   */
  int Histogram::Bins() const {
    return (int)p_bins.size();
  }


  /**
   * Returns the highest bin count.
   *
   * @return The highest bin count.
   */
  BigInt Histogram::MaxBinCount() const {

    BigInt maxBinCount = 0;
    for (int i = 0; i < (int)p_bins.size(); i++) {
      if (p_bins[i] > maxBinCount) maxBinCount = p_bins[i];
    }

    return maxBinCount;
  }
}
