#ifndef AutoReg_h
#define AutoReg_h

/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <vector>

#include "Chip.h"
#include "Statistics.h"
#include <QString>

class QString;

namespace Isis {
  class AutoRegItem;
  class Buffer;
  class Pvl;

  /**
   * @brief Auto Registration class
   *
   * Create AutoReg object.  Because this is a pure virtual class you can
   * not create an AutoReg class directly.  Instead, see the AutoRegFactory
   * class.
   *
   * @ingroup PatternMatching
   *
   * @see AutoRegFactory MaximumCorrelation MinimumDifference
   *
   * @author ????-??-?? Unknown
   *
   * @internal
   *   @history 2006-01-11 Jacob Danton Added idealFit variable, option for sub-pixel
   *                                     accuracy, new CompareFit method, an arbitrarily
   *                                     chosen EPSILON value, a minimum standard deviation
   *                                     cutoff for the pattern chip, and created a unitTest
   *   @history 2006-02-13 Jacob Danton Added shrinking pattern and sampling option for the pattern chip.
   *   @history 2006-05-15 Jeff Anderson Moved ZScoreMinimum from
   *                                     Algorithm group to the
   *                                     PatternChip group
   *   @history 2006-05-22 Jacob Danton Added statistics reporting
   *   @history 2006-06-28 Brendan George Added copy constructor
   *            (private member function)
   *   @history 2008-06-23 Steven Lambright Fixed naming and some documentation
   *            fixes, added ZScores
   *   @history 2008-08-13 Noah Hilt Added two new optional arguments to AutoReg: WindowSize and
        DistanceTolerance. These two arguments affect how AutoReg gathers and
   *    compares data for surface modeling and accuracy and should be contained
   *    inside the group "SurfaceModel" in the Pvl. Also changed the Pvl returned
   *    by RegistrationStatistics so that group names do not contain spaces and
   *    the Pvl only contains groups.
   *    @history 2008-11-18 Steven Koechle - Changed all keywords *NotEnoughData
   *             to *NotEnoughValidData
   *    @history 2009-03-17  Tracie Sucharski - Added method to return fit chip
   *    @history 2009-03-26  Jeff Anderson - Removed pattern chip
   *             reduction as it was broken, added skewness check
   *             for the fit chip, cleaned up the
   *             RegistrationStatistics method and added setters
   *             methods where appropriate
   *    @history 2009-03-30 Jeff Anderson - Modified code to reset
   *             parameters (goodness of fit and skewness) for
   *             successive calls to Register method.  Also check
   *             to see if skewness is null.
   *    @history 2009-05-08 Stacy Alley - Took out the skewness
   *             test and added the ellipse eccentricity test in
   *             the ModelSurface method.  Also added the 'reduce'
   *             option to speed up the pattern matching process.
   *    @history 2009-06-02 Stacy Alley - ModelSurface method now
   *             returns a bool instead of an int.  The p_registrationStatus
   *             is set within this method now. The Match method
   *             now takes another arg... fChip, passed in from
   *             Register. Also took out a redundant test,
   *             'CompareFits' after the ModelSurface call.  Also
   *             changed all the Chips in this header file from
   *             pointers to non-pointers.
   *             Saved all the reduced chips and have
   *             methods to return them so they can be views from
   *             Qnet.
   *    @history 2009-06-02 Jeff Anderson - Added
   *             AdaptiveRegistration virtual methods
   *    @history 2009-08-07 Travis Addair - Added functionality
   *             allowing it and all its sublcasses to return
   *             their auto registration template parameters
   *    @history 2009-08-25 Kris Becker - Correct calls to abs instead of the
   *             proper fabs.
   *    @history 2009-08-26 Kris Becker - Added chip parameters to Adaptive
   *             method to coorespond to the Match method.  Also added best
   *             search sample and line parameters to Adaptive method.  Also
   *             needed way to relate best fit value.
   *    @history 2009-09-02 Kris Becker - Set the default valid minimum and
   *             maximum values for pattern and search chips to
   *             Isis::ValidMinimum and Isis::ValidMaximum, respectively, as
   *             opposed to -DBL_MAX and _DBL_MAX.  This modification has the
   *             net effect of excluding special pixels from a valid test.  Also
   *             fix a bug whereby the extracted subsearch chip valid percent
   *             was divided by 100 - it instead should be passed in as is with
   *             the pattern chip test.
   *    @history 2009-09-07 Kris Becker - Set the goodness of fit
   *             when successful Adaptive algorithm.
   *    @history 2010-02-22 Tracie Sucharski - Added return
   *             methods for settings.
   *    @history 2010-03-16 Travis Addair - Added option to skip
   *             eccentricity testing, and made it so that the
   *             eccentricity is assumed to be 0 when it cannot
   *             otherwise be computed.
   *    @history 2010-03-18 Travis Addair - Added optional surface
   *             model validity test to ensure that the average
   *             residual of the least squares solution is within
   *             a tolerance.
   *    @history 2010-03-19 Travis Addair - Changed eccentricity
   *             and residual tests to be disabled by default.
   *             Including the keyword EccentricityRatio or
   *             ResidualTolerance in the Pvl that constructs an
   *             AutoReg object will enable the respective test.
   *    @history 2010-03-26 Travis Addair - Added methods
   *             Eccentricity() and AverageResidual() to retrieve
   *             the last computed value for those variables
   *    @history 2010-04-08 Travis Addair - Added methods
   *             EccentricityRatio() and
   *             EccentricityRatioTolerance() to retrieve  an
   *             eccentricity value as the antecedent in an A:1
   *             ratio.
   *    @history 2010-06-15 Jeannie Walldren - Modified code to read in an
   *             Interpolator type from the pvl and set the pattern and search
   *             chips' Read() methods to use this Interpolator type. Updated
   *             documentation and unitTest.
   *    @history 2010-07-09 Travis Addair - ComputeChipZScore now requires that
   *             both p_ZScoreMin and p_ZScoreMax are less than the pattern
   *             stats minimum z-score in order to fail.
   *    @history 2010-07-20 Jeannie Walldren - Added ability to set a valid
   *             percentage for the search chip's subchip. Updated documentation
   *             and unitTest.
   *    @history 2010-08-04 Jeannie Walldren - Updated documentation.
   *    @history 2010-10-07 Travis Addair - Changed enumeration value
   *             "Success" into two separate values "SuccessPixel" and
   *             "SuccessSubPixel" to differentiate between registrations
   *             computed and not computed to sub-pixel accuracy.  Also added
   *             method "Success()" to fill the role of looking at the
   *             original "Success" value to know if AutoReg succeeded.
   *    @history 2011-03-08 Jai Rideout - Added ability to apply a gradient
   *             filter to chips before attempting to perform a match. Renamed
   *             member variables to be consistent with the variable naming
   *             scheme in the rest of the file.
   *    @history 2011-03-29 Jai Rideout - Fixed bug where gradient filter was
   *             being reapplied to pattern and search chips each time
   *             Register() was called. Also fixed bug where filtered chips
   *             lost cube positioning information in ApplyGradientFilter().
   *    @history 2011-05-04 Jai Rideout - Added ability to save gradient-
   *             filtered chips after registration instead of just saving the
   *             original chips.
   *    @history 2011-05-18 Steven Lambright - Fixed a bug with ReductionFactor
   *    @history 2011-10-12 Jai Rideout - Removed Roberts gradient. It did not
   *             give useful results due to the 2x2 matrix it uses.
   *    @history 2012-01-05 Travis Addair - Added separate variables for Whole
   *             Pixel Correlation and Subpixel Correlation.
   *    @history 2018-07-27 Makayla Shepherd - Fixed a bug that made registration fail for
   *                            one dimensional pattern chips. Fixes #5247.
   *    @history 2018-07-27 Makayla Shepherd - Fixed a bug that upon sub-pixel registration failure
   *                            caused the previous registration to be returned. If sub-pixel 
   *                            registration fails now it will return to the whole pixel 
   *                            registration values. Fixes #5248.
   */
  class AutoReg {
    public:
      AutoReg(Pvl &pvl);

      virtual ~AutoReg();

      /**
       * Enumeration of the Register() method's return status.  All of the
       * enumerations describe a failure to register except "Success".  These status
       * values can be used to provide the user with more specific feedback on why
       * registration did not succeed.
       */
      enum RegisterStatus {
        SuccessPixel,                        //!< Success registering to whole pixel
        SuccessSubPixel,                     //!< Success registering to sub-pixel accuracy
        PatternChipNotEnoughValidData,       //!< Not enough valid data in pattern chip
        FitChipNoData,                       //!< Fit chip did not have any valid data
        FitChipToleranceNotMet,              //!< Goodness of fit tolerance not satisfied
        SurfaceModelNotEnoughValidData,      //!< Not enough points to fit a surface model for sub-pixel accuracy
        SurfaceModelSolutionInvalid,         //!< Could not model surface for sub-pixel accuracy
        SurfaceModelDistanceInvalid,         //!< Surface model moves registration more than one pixel
        PatternZScoreNotMet,                 //!< Pattern data max or min does not pass the z-score test
        AdaptiveAlgorithmFailed              //!< Error occured in Adaptive algorithm
      };

      /**
       * Enumeration of the different types of gradient filters that can be
       * applied to the pattern and search chips before matching them.
       */
      enum GradientFilterType {
        None,   //!< default, no gradient filter
        Sobel   //!< Sobel gradient filter
      };

      //! Return pointer to pattern chip
      inline Chip *PatternChip() {
        return &p_patternChip;
      };

      //! Return pointer to search chip
      inline Chip *SearchChip() {
        return &p_searchChip;
      };

      //! Return pointer to fit chip
      inline Chip *FitChip() {
        return &p_fitChip;
      };

      //! Return pointer to pattern chip used in registration
      inline Chip *RegistrationPatternChip() {
        if (p_gradientFilterType == None) {
          return &p_patternChip;
        }
        else {
          return &p_gradientPatternChip;
        }
      };

      //! Return pointer to search chip used in registration
      inline Chip *RegistrationSearchChip() {
        if (p_gradientFilterType == None) {
          return &p_searchChip;
        }
        else {
          return &p_gradientSearchChip;
        }
      };

      //! Return pointer to reduced pattern chip
      inline Chip *ReducedPatternChip() {
        return &p_reducedPatternChip;
      };

      //! Return pointer to reduced search chip
      inline Chip *ReducedSearchChip() {
        return &p_reducedSearchChip;
      };

      //! Return pointer to reduced fit chip
      inline Chip *ReducedFitChip() {
        return &p_reducedFitChip;
      };

      void SetSubPixelAccuracy(bool on);
      void SetPatternValidPercent(const double percent);
      void SetSubsearchValidPercent(const double percent);
      void SetTolerance(double tolerance);
      void SetChipInterpolator(const QString &interpolator);
      void SetSurfaceModelWindowSize(int size);
      void SetSurfaceModelDistanceTolerance(double distance);
      void SetReductionFactor(int reductionFactor);
      void SetPatternZScoreMinimum(double minimum);
      void SetGradientFilterType(const QString &gradientFilterType);

      QString GradientFilterString() const;

      /**
       * Return whether this object will attempt to register to whole or
       * sub-pixel accuracy.
       *
       * @return on Is sub-pixel accuracy enabled?
       */
      bool SubPixelAccuracy() {
        return p_subpixelAccuracy;
      }

      //! Return the reduction factor.
      int ReductionFactor() {
        return p_reduceFactor;
      }

      //! Return pattern chip valid percent.  The default value is
      double PatternValidPercent() const {
        return p_patternValidPercent;
      };

      //! Return subsearch chip valid percent
      double SubsearchValidPercent() const {
        return p_subsearchValidPercent;
      };

      //! Return match algorithm tolerance
      inline double Tolerance() const {
        return p_tolerance;
      };

      //! Return window size
      double WindowSize() const {
        return p_windowSize;
      };

      //! Return distance tolerance
      double DistanceTolerance() const {
        return p_distanceTolerance;
      };

      /**
       * Return the distance point moved
       *
       * @param sampDistance Sample movement
       * @param lineDistance Line movement
       */
      void Distance(double &sampDistance, double &lineDistance) {
        sampDistance = p_sampMovement;
        lineDistance = p_lineMovement;
      }

      AutoReg::RegisterStatus Register();

      //! Return whether the match algorithm succeeded or not
      inline bool Success() const {
        return (p_registrationStatus == SuccessPixel || p_registrationStatus == SuccessSubPixel);
      }

      //! Return the goodness of fit of the match algorithm
      inline double GoodnessOfFit() const {
        return p_goodnessOfFit;
      };

      inline bool IsIdeal(double fit);

      //! Return the search chip sample that best matched
      inline double ChipSample() const {
        return p_chipSample;
      };

      //! Return the search chip line that best matched
      inline double ChipLine() const {
        return p_chipLine;
      };

      //! Return the search chip cube sample that best matched
      inline double CubeSample() const {
        return p_cubeSample;
      };

      //! Return the search chip cube line that best matched
      inline double CubeLine() const {
        return p_cubeLine;
      };

      //! Return minimumPatternZScore
      double MinimumZScore() const {
        return p_minimumPatternZScore;
      };

      /**
       * Return the ZScores of the pattern chip
       *
       * @param score1 First Z Score
       * @param score2 Second Z Score
       */
      void ZScores(double &score1, double &score2) const {
        score1 = p_zScoreMin;
        score2 = p_zScoreMax;
      }

      Pvl RegistrationStatistics();

      /**
       * Minimum tolerance specific to algorithm
       */
      virtual double MostLenientTolerance() {
        return DBL_MIN;
      }

      /**
       * Returns the name of the algorithm.
       *
       *
       * @return QString
       */
      virtual QString AlgorithmName() const = 0;

      PvlGroup RegTemplate();

      PvlGroup UpdatedTemplate();

    protected:
      /**
       * Sets the search chip subpixel sample that matches the pattern
       * tack sample.
       *
       * @param sample Value to set for search chip subpixel sample
       */
      inline void SetChipSample(double sample) {
        p_chipSample = sample;
      };

      /**
       * Sets the search chip subpixel line that matches the pattern
       * tack line.
       *
       *
       * @param line Value to set for search chip subpixel line
       */
      inline void SetChipLine(double line) {
        p_chipLine = line;
      };

      /**
       * Sets the goodness of fit for adaptive algorithms
       *
       * @param fit Fit value to set
       */
      inline void SetGoodnessOfFit(double fit) {
        p_bestFit = fit;
      };

      virtual AutoReg::RegisterStatus Registration(Chip &sChip, Chip &pChip,
          Chip &fChip, int startSamp, int startLine, int endSamp, int endLine,
          int bestSamp, int bestLine);

      void Parse(Pvl &pvl);
      virtual bool CompareFits(double fit1, double fit2);
      bool SetSubpixelPosition(Chip &window);
      Chip Reduce(Chip &chip, int reductionFactor);

      /**
       * Returns the ideal (perfect) fit that could be returned by the
       * MatchAlgorithm.
       *
       *
       * @return double
       */
      virtual double IdealFit() const = 0;

      /**
       *  Given two identically sized chips return a double that
       *  indicates how well they match.  For example, a correlation
       *  match algorithm would return a correlation coefficient
       *  ranging from -1 to 1.
       *
       *
       * @param pattern Pattern chip to match against
       * @param subsearch Subchip of the search chip to match with
       *
       * @return double
       */
      virtual double MatchAlgorithm(Chip &pattern, Chip &subsearch) = 0;

      PvlObject p_template; //!< AutoRegistration object that created this projection

      /**
       * @brief Provide (adaptive) algorithms a chance to report results
       *
       * Provide Adaptive objects the opportunity to report behavior.  It is
       * called at the final step prior to program termination.
       *
       * @param pvl Pvl structure to add report to
       *
       * @return Pvl Results
       */
      virtual Pvl AlgorithmStatistics(Pvl &pvl) {
        return (pvl);
      }

    private:
      /**
       * Empty copy constructor.
       * @param original AutoReg object
       */

      void Match(Chip &sChip,
                 Chip &pChip,
                 Chip &fChip,
                 int startSamp,
                 int endSamp,
                 int startLine,
                 int endLine);
      bool ComputeChipZScore(Chip &chip);
      void Init();
      void ApplyGradientFilter(Chip &chip);
      void SobelGradient(Buffer &in, double &v);

      Chip p_patternChip;                                  //!< Chip to be matched
      Chip p_searchChip;                                   //!< Chip to be searched for best registration
      Chip p_fitChip;                                      //!< Results from MatchAlgorithm() method
      Chip p_gradientSearchChip;                           //!< Chip to be searched for best registration with gradient applied
      Chip p_gradientPatternChip;                          //!< Chip to be matched with gradient applied
      Chip p_reducedPatternChip;                           //!< Pattern Chip with reduction factor
      Chip p_reducedSearchChip;                            //!< Search Chip with reduction factor
      Chip p_reducedFitChip;                               //!< Fit Chip with reduction factor

      bool p_subpixelAccuracy;                             //!< Indicates whether sub-pixel accuracy is enabled. Default is true.

      //TODO: remove these after control points are refactored.
      int p_totalRegistrations;                            //!< Registration Statistics Total keyword.
      int p_pixelSuccesses;                                //!< Registration statistics Success keyword.
      int p_subpixelSuccesses;                             //!< Registration statistics Success keyword.
      int p_patternChipNotEnoughValidDataCount;            //!< Registration statistics PatternNotEnoughValidData keyword.
      int p_patternZScoreNotMetCount;                      //!< Registration statistics PatternZScoreNotMet keyword.
      int p_fitChipNoDataCount;                            //!< Registration statistics FitChipNoData keyword.
      int p_fitChipToleranceNotMetCount;                   //!< Registration statistics FitChipToleranceNotMet keyword.
      int p_surfaceModelNotEnoughValidDataCount;           //!< Registration statistics SurfaceModelNotEnoughValidData keyword.
      int p_surfaceModelSolutionInvalidCount;              //!< Registration statistics SurfaceModelSolutionInvalid keyword.
      int p_surfaceModelDistanceInvalidCount;              //!< Registration statistics SurfaceModelDistanceInvalid keyword.

      double p_zScoreMin;                                  //!< First Z-Score of pattern chip
      double p_zScoreMax;                                  //!< Second Z-Score of pattern chip

      double p_minimumPatternZScore;                       //!< Minimum pattern Z-Score
      double p_patternValidPercent;                        //!< Percentage of data in pattern chip that must be valid
      double p_subsearchValidPercent;                      //!< Percentage of data in subsearch chip that must be valid

      double p_chipSample;                                 //!< Chip sample
      double p_chipLine;                                   //!< Chip line
      double p_cubeSample;                                 //!< Cube sample
      double p_cubeLine;                                   //!< Cube line
      double p_goodnessOfFit;                              //!< Goodness of fit of the match algorithm
      double p_tolerance;                                  //!< Tolerance for acceptable goodness of fit in match algorithm

      int p_windowSize;                                    //!< Surface model window size
      double p_distanceTolerance;                          //!< Maximum distance the surface model solution may be from the best whole pixel fit in the fit chip

      double p_bestFit;                                    //!< Goodness of fit for adaptive algorithms.
      int p_bestSamp;                                      //!< Sample value of best fit.
      int p_bestLine;                                      //!< Line value of best fit.
      double p_sampMovement;                               //!< The number of samples the point moved.
      double p_lineMovement;                               //!< The number of lines the point moved.
      int p_reduceFactor;                                  //!< Reduction factor.
      Isis::AutoReg::RegisterStatus p_registrationStatus;  //!< Registration status to be returned by Register().
      AutoReg::GradientFilterType p_gradientFilterType;    //!< Type of gradient filter to use before matching.
  };
};

#endif
