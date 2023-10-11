/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "ImageHistogram.h"

#include "Brick.h"
#include "ControlNet.h"
#include "ControlMeasure.h"

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
  ImageHistogram::ImageHistogram(double minimum, double maximum, int nbins) :
  Histogram(minimum, maximum, nbins) {
  }


  /**
   * Constructs a histogram object using a cube. This constructor computes
   *   the minimum, maximum for the binning range and number of bins
   *   automatically. All statistics will still be collected, though data at
   *   either end of the histogram will be put into one bin in order to attempt to
   *   achieve better histogram statistics.
   *
   * TODO: progress needs to be a bool.
   *
   * @param cube  The cube to used to determine min/max and bins
   * @param statsBand  The band number the histogram will be collected from
   * @param progress  The Progress object to be used to output the percent
   *                  processed information
   * @param startSample  The sample to start reading cube data from
   * @param startLine  The line to start reading cube data from
   * @param endSample  The sample to stop reading cube data at (Null for nsamps)
   * @param endLine  The line to stop reading cube data at (Null for nlines)
   * @param bins The number of histogram bins to create (0 for automatic)
   * @param addCubeData True to fill the histogram with data in addition to
   *                    initializing the binning ranges.
   */
  ImageHistogram::ImageHistogram(Cube &cube, int statsBand, Progress *progress,
      double startSample, double startLine,
      double endSample, double endLine,
      int bins, bool addCubeData) {
    InitializeFromCube(cube, statsBand, progress, bins, startSample, startLine,
                       endSample, endLine);

    if (addCubeData) {
      Brick cubeDataBrick((int)(endSample - startSample + 1),
                          1, 1, cube.pixelType());

      // if band == 0, then we're gathering data for all bands.
      int startBand = statsBand;
      int endBand = statsBand;

      if (statsBand == 0) {
        startBand = 1;
        endBand = cube.bandCount();
      }

      if (progress != NULL) {
        progress->SetText("Gathering histogram");
        progress->SetMaximumSteps(
          (int)(endLine - startLine + 1) * (int)(endBand - startBand + 1));
        progress->CheckStatus();
      }

      for (int band = startBand; band <= endBand; band++) {
        for (int line = (int)startLine; line <= endLine; line++) {
          cubeDataBrick.SetBasePosition(qRound(startSample), line, band);
          cube.read(cubeDataBrick);
          AddData(cubeDataBrick.DoubleBuffer(), cubeDataBrick.size());
          if (progress != NULL) {
            progress->CheckStatus();
          }
        }
      }
    }
  }

  void ImageHistogram::InitializeFromCube(Cube &cube, int statsBand,
      Progress *progress, int nbins, double startSample, double startLine,
      double endSample, double endLine) {
    // Make sure band is valid, 0 is valid (means all bands)
    if ( (statsBand < 0) || (statsBand > cube.bandCount() ) ) {
      string msg = "Cannot gather histogram for band [" + IString(statsBand) +
          "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // We need to find the min/max DN value for our histogram bins to be the
    //   correct size.
    double minDnValue = Null;
    double maxDnValue = Null;

    if (cube.pixelType() == UnsignedByte) {
      // If we can discretely store every data point, then we can use the
      //   possible extent of the data range as our min/max dn values.
      if (nbins == 0) {
        minDnValue = 0.0 * cube.multiplier() + cube.base();
        maxDnValue = 255.0 * cube.multiplier() + cube.base();
        nbins = 256;
      }
    }
    else if (cube.pixelType() == UnsignedWord) {
      if (nbins == 0) {
        minDnValue = 0.0 * cube.multiplier() + cube.base();
        maxDnValue = 65535.0 * cube.multiplier() + cube.base();
        nbins = 65536;
      }
    }
    else if (cube.pixelType() == SignedWord) {
      if (nbins == 0) {
        minDnValue = -32768.0 * cube.multiplier() + cube.base();
        maxDnValue = 32767.0 * cube.multiplier() + cube.base();
        nbins = 65536;
      }
    }
    // 32-bit data covers too big of a range of values to use
    // the min and max possible values to set our value range.
    // So, just set the number of bins and then later we will
    // compute the min and max value in the actual cube.
    else if (cube.pixelType() == UnsignedInteger ||
             cube.pixelType() == SignedInteger ||
             cube.pixelType() == Real) {
      if (nbins == 0) {
        nbins = 65536;
      }
    }
    else {
      IString msg = "Unsupported pixel type";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (startSample == Null)
      startSample = 1.0;

    if (endSample == Null)
      endSample = cube.sampleCount();

    if (startLine == Null)
      startLine = 1.0;

    if (endLine == Null)
      endLine = cube.lineCount();

    // If we still need our min/max DN values, find them.
    if (minDnValue == Null || maxDnValue == Null) {

      Brick cubeDataBrick((int)(endSample - startSample + 1),
                          1, 1, cube.pixelType() );
      Statistics stats;

      // if band == 0, then we're gathering stats for all bands. I'm really
      //   not sure that this is correct, a good idea or called from anywhere...
      //   but I don't have time to track down the use case.
      int startBand = statsBand;
      int endBand = statsBand;

      if (statsBand == 0) {
        startBand = 1;
        endBand = cube.bandCount();
      }

      if (progress != NULL) {

        progress->SetText("Computing min/max for histogram");
        progress->SetMaximumSteps(
          (int)(endLine - startLine + 1) * (int)(endBand - startBand + 1) );
        progress->CheckStatus();
      }

      for (int band = startBand; band <= endBand; band++) {
        for (int line = (int)startLine; line <= endLine; line++) {

          cubeDataBrick.SetBasePosition(qRound(startSample), line, band);
          cube.read(cubeDataBrick);
          stats.AddData(cubeDataBrick.DoubleBuffer(), cubeDataBrick.size());

          if (progress != NULL) {
            progress->CheckStatus();
          }
        }
      }

      if (stats.ValidPixels() == 0) {
        minDnValue = 0.0;
        maxDnValue = 1.0;
      }
      else {
        minDnValue = stats.Minimum();
        maxDnValue = stats.Maximum();
      }
    }

    // Set the bins and range
    SetValidRange(minDnValue, maxDnValue);
    SetBins(nbins);
  }

  //! Destructs a histogram object.
  ImageHistogram::~ImageHistogram() {
  }

  /**
   * Add an array of doubles to the histogram counters. This method can be
   * invoked multiple times. For example, once for each line in a cube, before
   * obtaining statistics and histogram information.
   *
   * @param data Pointer to array of double to add.
   * @param count Number of doubles to process.
   */
  void ImageHistogram::AddData(const double *data,
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
          index = (int) floor((double)(nbins - 1) / (BinRangeEnd() - BinRangeStart()) *
                              (data[i] - BinRangeStart() ) + 0.5);
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
  void ImageHistogram::AddData(const double data) {
    Statistics::AddData(data);

    int nbins = p_bins.size();
    int index;
    if (IsValidPixel(data) && InRange(data) ) {
      if (BinRangeStart() == BinRangeEnd() ) {
        index = 0;
      }
      else {
        index = (int) floor((double)(nbins - 1) / (BinRangeEnd() - BinRangeStart() ) *
                            (data - BinRangeStart() ) + 0.5);
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
  void ImageHistogram::RemoveData(const double *data,
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
          index = (int) floor((double)(nbins - 1) / (BinRangeEnd() - BinRangeStart()) *
                              (data[i] - BinRangeStart()) + 0.5);
        }
        if (index < 0) index = 0;
        if (index >= nbins) index = nbins - 1;
        p_bins[index] -= 1;
      }
    }
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
  void ImageHistogram::BinRange(const int index,
                                double &low, double &high) const {
    if ( (index < 0) || (index >= (int)p_bins.size() ) ) {

      std::string message = Message::ArraySubscriptNotInRange(index);
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    double binSize = (BinRangeEnd() - BinRangeStart()) / (double)(p_bins.size() - 1);
    low = BinRangeStart() - binSize / 2.0 + binSize * (double) index;
    high = low + binSize;
  }
}
