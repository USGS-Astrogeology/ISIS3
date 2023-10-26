/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <float.h>
#include <string>
#include <iostream>

#include "IException.h"
#include "MultivariateStatistics.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"


namespace Isis {

  /**
   * Constructs a Multivariate Statistics object with accumulators and counters
   * set to zero.
   */
  MultivariateStatistics::MultivariateStatistics() {
    Reset();
  }


  /**
   * Constructs a MulitvariateStatistics object from a PvlObject.
   *
   * @param const PvlObject & - Input multivariate statistics.
   */
  MultivariateStatistics::MultivariateStatistics(const PvlObject &inStats) {
    Reset();
    fromPvl(inStats);
  }


  //! Resets all accumulators to zero
  void MultivariateStatistics::Reset() {
    p_x.Reset();
    p_y.Reset();
    p_sumxy = 0.0;

    p_validPixels = 0;
    p_invalidPixels = 0;
    p_totalPixels = 0;
  }


  //! Destructs a MultivariateStatistics object.
  MultivariateStatistics::~MultivariateStatistics() {};


  /**
   * Add two arrays of doubles to the accumulators and counters. This method can
   * be invoked multiple times, for example, once for each line in a cube, before
   * obtaining statistics.
   *
   * @param x Array of doubles to add.
   * @param y Array of doubles to add.
   * @param count Number of doubles to process.
   */
  void MultivariateStatistics::AddData(const double *x, const double *y,
                                       const unsigned int count) {
    for(unsigned int i = 0; i < count; i++) {
      double yVal = y[i];
      double xVal = x[i];
      p_totalPixels++;

      if(Isis::IsValidPixel(xVal) && Isis::IsValidPixel(yVal)) {
        p_x.AddData(xVal);
        p_y.AddData(yVal);
        p_sumxy += xVal * yVal;
        p_validPixels++;
      }
      else {
        p_invalidPixels++;
      }
    }
  }


  /**
   * Add an x,y value to the accumulators and counters count times. This method
   * can be invoked multiple times before obtaining statistics.
   *
   * @param x x value to add
   * @param y y value to add
   * @param count Number of times to add this x,y value
   */
  void MultivariateStatistics::AddData(double x, double y, unsigned int count) {
    p_totalPixels += count;

    if(IsValidPixel(x) && IsValidPixel(y)) {
      p_sumxy += x * y * count;
      p_validPixels += count;

      for (unsigned int i = 0; i < count; i++) {
        p_x.AddData(x);
        p_y.AddData(y);
      }
    }
    else {
      p_invalidPixels += count;
    }
  }


  /**
   * Remove an array of doubles from the accumulators and counters.
   *
   * @param x Pointer to an array of doubles to remove.
   * @param y Array of doubles to add.
   * @param count Number of doubles to process.
   *
   * @return (type)return description
   *
   * @internal
   *   @todo The description for param y doesn't make sense here. -Leah
   */
  void MultivariateStatistics::RemoveData(const double *x, const double *y,
                                          const unsigned int count) {
    for(unsigned int i = 0; i < count; i++) {
      p_totalPixels--;

      if(Isis::IsValidPixel(x[i]) && Isis::IsValidPixel(y[i])) {
        p_x.RemoveData(&x[i], 1);
        p_y.RemoveData(&y[i], 1);
        p_sumxy -= x[i] * y[i];
        p_validPixels--;
      }
      else {
        p_invalidPixels--;
      }
    }

    if(p_totalPixels < 0) {
      std::string m = "You are removing non-existant data in [MultivariateStatistics::RemoveData]";
      throw IException(IException::Programmer, m, _FILEINFO_);
    }
  }


  /**
   * Computes and returns the covariance between the two data sets If there are
   * no valid data (pixels) then NULL8 is returned.
   *
   * @return Covariance between the two data sets.
   */
  double MultivariateStatistics::Covariance() const {
    if(p_validPixels <= 1) return Isis::NULL8;
    double covar = p_sumxy - p_y.Average() * p_x.Sum() - p_x.Average() * p_y.Sum() +
                   p_x.Average() * p_y.Average() * (double)p_validPixels;
    return covar / (double)(p_validPixels - 1);
  }


  /**
   * Computes and returns the coefficient of correlation (between -1.0 and 1.0) of
   * the two data sets.  This can be used as a goodness-of-fit measurement.  The
   * close the correlation is two -1.0 or 1.0 the more likely the data sets are
   * related (and therefore the regression equation is valid).
   *
   * @return The coefficient of correlation. (between -1.0 and 1.0)
   * The closer to 0.0 implies there is less correlation between the data sets.
   * Returns NULL8 if correlation couldn't be computed.
   */
  double MultivariateStatistics::Correlation() const {

    if(p_validPixels <= 1) return Isis::NULL8;
    double covar = Covariance();
    double stdX = p_x.StandardDeviation();
    double stdY = p_y.StandardDeviation();
    if(stdX == 0.0 || stdX == Isis::NULL8) return Isis::NULL8;
    if(stdY == 0.0 || stdY == Isis::NULL8) return Isis::NULL8;
    if(covar == Isis::NULL8) return Isis::NULL8;
    return covar / (stdX * stdY);
  }


  /**
   * Returns the total number of pixels processed.
   *
   * @return The total number of pixel processed (valid and invalid).
   */
  BigInt MultivariateStatistics::TotalPixels() const {
    return p_totalPixels;
  }


  /**
   * Returns the number of valid pixels processed. Only valid pixels are utilized
   * when computing the average, standard deviation, variance, minimum, and
   * maximum.
   *
   * @return The number of valid pixels processed.
   */
  BigInt MultivariateStatistics::ValidPixels() const {
    return p_validPixels;
  }


  /**
   * Returns the number of invalid pixels encountered.
   *
   * @return The number of invalid (unprocessed) pixels.
   */
  BigInt MultivariateStatistics::InvalidPixels() const {
    return p_invalidPixels;
  }


  /**
   * Fits a line @f[ y=A+Bx @f] through the data.
   *
   * @param a The additive constant A.
   * @param b The additive constant B.
   */
  void MultivariateStatistics::LinearRegression(double &a, double &b) const {
    // From Modern Elementary Statistics - 5th edition, Freund, pp 367
    double denom = (double)p_validPixels * p_x.SumSquare() - p_x.Sum() * p_x.Sum();
    if(denom == 0.0) {
      std::string msg = "Unable to compute linear regression in Multivariate Statistics";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    a = p_y.Sum() * p_x.SumSquare() - p_x.Sum() * p_sumxy;
    a = a / denom;

    b = (double)p_validPixels * p_sumxy - p_x.Sum() * p_y.Sum();
    b = b / denom;
  }


  /**
   * Returns the sum of x*y for all data given through the AddData method.
   *
   * @return The sum of x*y for all data.
   */
  double MultivariateStatistics::SumXY() const {
    return p_sumxy;
  }


  /**
   * Returns a Stats object for all of the X data fed through the AddData method.
   *
   * @return A Stats object for all X data.
   */
  Isis::Statistics MultivariateStatistics::X() const {
    return p_x;
  };


  /**
   * Returns a Stats object for all of the Y data fed through the AddData method.
   *
   * @return A Stats object for all Y data.
   */
  Isis::Statistics MultivariateStatistics::Y() const {
    return p_y;
  };


  /**
   * Unserializes a multivariate statistics object from a PvlObject
   *
   * @param const PvlObject & - Input multivariate statistics
   */
  void MultivariateStatistics::fromPvl(const PvlObject &inStats) {
    p_sumxy = inStats["SumXY"];
    p_validPixels = inStats["ValidPixels"];
    p_invalidPixels = inStats["InvalidPixels"];
    p_totalPixels = inStats["TotalPixels"];

    // unserialize the X and Y Statistics as well
    PvlGroup xStats = inStats.findGroup("XStatistics"); //
    p_x = Statistics(xStats);
    PvlGroup yStats = inStats.findGroup("YStatistics");
    p_y = Statistics(yStats);
  }


  /**
   * Serializes a multivariate statistics object as a PvlObject
   *
   * @param QString (Default value is "MultivariateStatistics") - Name of the PvlObject
   *
   * @return PvlObject The serialized multivariate statistics
   */
  PvlObject MultivariateStatistics::toPvl(QString name) const {
    if (name.isEmpty()) {
      name = "MultivariateStatistics";
    }
    PvlObject mStats(name.toStdString()); 
    mStats += PvlKeyword("Covariance" , std::to_string(Covariance()));
    mStats += PvlKeyword("Correlation", std::to_string(Correlation()));
    mStats += PvlKeyword("SumXY", std::to_string(SumXY()));
    mStats += PvlKeyword("ValidPixels", std::to_string(ValidPixels()));
    mStats += PvlKeyword("InvalidPixels", std::to_string(InvalidPixels()));
    mStats += PvlKeyword("TotalPixels", std::to_string(TotalPixels()));

    PvlKeyword linReg("LinearRegression");
    double a, b;
    try {
      LinearRegression(a, b);
      linReg += std::to_string(a);
      linReg += std::to_string(b);
    } catch (IException &e) {
      // It is possible one of the overlaps was constant and therefore
      // the regression would be a vertical line (x=c instead of y=ax+b)
    }
    mStats += linReg; 

    PvlGroup xStats = X().toPvl("XStatistics");
    PvlGroup yStats = Y().toPvl("YStatistics");
    mStats.addGroup(xStats);
    mStats.addGroup(yStats);

    return mStats;
  }

}



