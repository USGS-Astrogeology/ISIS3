/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "AutoReg.h"
#include "Buffer.h"
#include "Centroid.h"
#include "Chip.h"
#include "FileName.h"
#include "Histogram.h"
#include "IException.h"
#include "Interpolator.h"
#include "LeastSquares.h"
#include "Matrix.h"
#include "PixelType.h"
#include "Plugin.h"
#include "PolynomialBivariate.h"
#include "Pvl.h"

using namespace std;
namespace Isis {
  /**
   * Create AutoReg object.  Because this is a pure virtual class you can
   * not create an AutoReg class directly.  Instead, see the AutoRegFactory
   * class.  The default settings include:
   * <ul>
   *   <li> PatternChip
   *     <ul>
   *       <li>Samples = 3
   *       <li>Lines = 3
   *       <li>ValidPercent = 50.0
   *       <li>MinimumZScore = 1.0
   *     </ul>
   *   <li> SearchChip
   *     <ul>
   *       <li>Samples = 5
   *       <li>Lines = 5
   *       <li>SubchipValidPercent = 50.0
   *     </ul>
   *   <li> FitChip
   *     <ul>
   *       <li>Samples = 5
   *       <li>Lines = 5
   *     </ul>
   *  <li> Algorithm
   *     <ul>
   *       <li>Tolerance = Isis::Null
   *       <li>SubpixelAccuracy = True
   *       <li>ReductionFactor = 1
   *     </ul>
   *  <li> SurfaceModel
   *     <ul>
   *       <li>DistanceTolerance = 1.5
   *       <li>WindowSize = 5
   *     </ul>
   * </ul>
   * The reduced chips are initially set to the same size as their corresponding
   * chips in the constructor.
   *
   * @param pvl  A pvl object containing a valid AutoReg specification
   *
   * @see patternMatch.doc under the coreg application
   */
  AutoReg::AutoReg(Pvl &pvl) {
    p_template = pvl.findObject("AutoRegistration");

    // Set default parameters
    p_patternChip.SetSize(3, 3);
    p_searchChip.SetSize(5, 5);
    p_fitChip.SetSize(5, 5);
    p_reducedPatternChip.SetSize(3, 3);
    p_reducedSearchChip.SetSize(5, 5);
    p_reducedFitChip.SetSize(5, 5);
    p_gradientFilterType = None;

    SetPatternValidPercent(50.0);
    SetSubsearchValidPercent(50.0);
    SetPatternZScoreMinimum(1.0);
    SetTolerance(Isis::Null);

    SetSubPixelAccuracy(true);
    SetSurfaceModelDistanceTolerance(1.5);
    SetSurfaceModelWindowSize(5);

    SetReductionFactor(1);

    // Clear statistics
    //TODO: Delete these after control net refactor.
    p_totalRegistrations = 0;
    p_pixelSuccesses = 0;
    p_subpixelSuccesses = 0;
    p_patternChipNotEnoughValidDataCount = 0;
    p_patternZScoreNotMetCount = 0;
    p_fitChipNoDataCount = 0;
    p_fitChipToleranceNotMetCount = 0;
    p_surfaceModelNotEnoughValidDataCount = 0;
    p_surfaceModelSolutionInvalidCount = 0;
    p_surfaceModelDistanceInvalidCount = 0;

    p_sampMovement = 0.;
    p_lineMovement = 0.;

    Init();
    Parse(pvl);
  }

  /**
   * Initialize AutoReg object private variables.  Fill fit chip, reduced pattern
   * chip and reduced search chip with nulls.
   *
   */
  void AutoReg::Init() {
    // Set computed parameters to NULL so we don't use values from a previous
    // run
    p_zScoreMin = Isis::Null;
    p_zScoreMax = Isis::Null;
    p_goodnessOfFit = Isis::Null;

    p_bestSamp = 0;
    p_bestLine = 0;
    p_bestFit = Isis::Null;

    // --------------------------------------------------
    // Nulling out the fit chip
    // --------------------------------------------------
    for(int line = 1; line <= p_fitChip.Lines(); line++) {
      for(int samp = 1; samp <= p_fitChip.Samples(); samp++) {
        p_fitChip.SetValue(samp, line, Isis::Null);
      }
    }
    // --------------------------------------------------
    // Nulling out the reduced pattern chip
    // --------------------------------------------------
    for(int line = 1; line <= p_reducedPatternChip.Lines(); line++) {
      for(int samp = 1; samp <= p_reducedPatternChip.Samples(); samp++) {
        p_reducedPatternChip.SetValue(samp, line, Isis::Null);
      }
    }
    // --------------------------------------------------
    // Nulling out the reduced search chip
    // --------------------------------------------------
    for(int line = 1; line <= p_reducedSearchChip.Lines(); line++) {
      for(int samp = 1; samp <= p_reducedSearchChip.Samples(); samp++) {
        p_reducedSearchChip.SetValue(samp, line, Isis::Null);
      }
    }

  }

  //! Destroy AutoReg object
  AutoReg::~AutoReg() {

  }

  /**
   * Initialize parameters in the AutoReg class using a PVL specification.
   * An example of the PVL required for this is:
   *
   * @code
   * Object = AutoRegistration
   *   Group = Algorithm
   *     Name      = MaximumCorrelation
   *     Tolerance = 0.7
   *   EndGroup
   *
   *   Group = PatternChip
   *     Samples = 21
   *     Lines   = 21
   *   EndGroup
   *
   *   Group = SearchChip
   *     Samples = 51
   *     Lines = 51
   *   EndGroup
   * EndObject
   * @endcode
   *
   * There are many other options that can be set via the pvl and are
   * described in other documentation (see below).
   *
   * @see patternMatch.doc under the coreg
   *      application
   *
   * @param pvl The pvl object containing the specification
   * @throw  iException::User "Improper format for AutoReg PVL."
   * @internal
   *   @history 2010-06-15 Jeannie Walldren - Added ability to read
   *                          ChipInterpolator keyword from the Algorithm group.
   *   @history 2010-07-20 Jeannie Walldren - Added ability to read search sub
   *                          chip valid percent
  **/
  void AutoReg::Parse(Pvl &pvl) {
    try {
      // Get info from Algorithm group
      PvlGroup &algo = pvl.findGroup("Algorithm", Pvl::Traverse);
      SetTolerance(algo["Tolerance"]);
      if(algo.hasKeyword("ChipInterpolator")) {
        SetChipInterpolator(QString::fromStdString(algo["ChipInterpolator"]));
      }

      if(algo.hasKeyword("SubpixelAccuracy")) {
        SetSubPixelAccuracy(std::to_string((int)algo["SubpixelAccuracy"]) == "True");
      }

      if(algo.hasKeyword("ReductionFactor")) {
        SetReductionFactor((int)algo["ReductionFactor"]);
      }

      if (algo.hasKeyword("Gradient")) {
        SetGradientFilterType(QString::fromStdString(algo["Gradient"]));
      }

      // Setup the pattern chip
      PvlGroup &pchip = pvl.findGroup("PatternChip", Pvl::Traverse);
      PatternChip()->SetSize((int)pchip["Samples"], (int)pchip["Lines"]);

      double minimum = Isis::ValidMinimum;
      double maximum = Isis::ValidMaximum;
      if(pchip.hasKeyword("ValidMinimum")) minimum = pchip["ValidMinimum"];
      if(pchip.hasKeyword("ValidMaximum")) maximum = pchip["ValidMaximum"];
      PatternChip()->SetValidRange(minimum, maximum);

      if(pchip.hasKeyword("MinimumZScore")) {
        SetPatternZScoreMinimum((double)pchip["MinimumZScore"]);
      }
      if(pchip.hasKeyword("ValidPercent")) {
        SetPatternValidPercent((double)pchip["ValidPercent"]);
      }

      // Setup the search chip
      PvlGroup &schip = pvl.findGroup("SearchChip", Pvl::Traverse);
      SearchChip()->SetSize((int)schip["Samples"], (int)schip["Lines"]);

      minimum = Isis::ValidMinimum;
      maximum = Isis::ValidMaximum;
      if(schip.hasKeyword("ValidMinimum")) minimum = schip["ValidMinimum"];
      if(schip.hasKeyword("ValidMaximum")) maximum = schip["ValidMaximum"];
      SearchChip()->SetValidRange(minimum, maximum);
      if(schip.hasKeyword("SubchipValidPercent")) {
        SetSubsearchValidPercent((double)schip["SubchipValidPercent"]);
      }

      // Setup surface model
      PvlObject ar = pvl.findObject("AutoRegistration");
      if(ar.hasGroup("SurfaceModel")) {
        PvlGroup &smodel = ar.findGroup("SurfaceModel", Pvl::Traverse);
        if(smodel.hasKeyword("DistanceTolerance")) {
          SetSurfaceModelDistanceTolerance((double)smodel["DistanceTolerance"]);
        }

        if(smodel.hasKeyword("WindowSize")) {
          SetSurfaceModelWindowSize((int)smodel["WindowSize"]);
        }
      }

    }
    catch(IException &e) {
      std::string msg = "Improper format for AutoReg PVL [" + pvl.fileName() + "]";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }
    return;
  }

  /**
   * Set the gradient filter type to be applied to the search and pattern
   * chips.
   *
   * @param gradientFilterType the gradient filter type to use
   * @throw iException::User - "Invalid Gradient type."
   */
  void AutoReg::SetGradientFilterType(const QString &gradientFilterType) {
    if (gradientFilterType == "None") {
      p_gradientFilterType = None;
    }
    else if (gradientFilterType == "Sobel") {
      p_gradientFilterType = Sobel;
    }
    else {
      throw IException(IException::User,
                       "Invalid Gradient type.  Cannot use ["
                       + gradientFilterType + "] to filter chip",
                       _FILEINFO_);
    }
  }


  QString AutoReg::GradientFilterString() const {
    switch (p_gradientFilterType) {
      case None: return "None";
      case Sobel: return "Sobel";
      default: throw IException(
                   IException::Programmer,
                   "AutoReg only allows Sobel gradient filter or None",
                   _FILEINFO_);
    }
  }


  /**
   * If the sub-pixel accuracy is enabled, the Register() method will attempt to
   * match the pattern chip to the search chip at sub-pixel accuracy, otherwise it
   * will be registered at whole pixel accuracy.
   *
   * If this method is not called, the sub pixel accuracy defaults to on = true
   * in the AutoReg object constructor.
   *
   * @param on Set the state of registration accuracy.  The
   *           default is sub-pixel accuracy is on
   */
  void AutoReg::SetSubPixelAccuracy(bool on) {
    p_subpixelAccuracy = on;
  }

  /**
   * Set the amount of data in the pattern chip that must be valid.  For
   * example, a 21x21 pattern chip has 441 pixels.  If percent is 75 then
   * at least 330 pixels pairs must be valid in order for a comparision
   * between the pattern and search sub-region to occur.  That is, both
   * the pattern pixel and search pixel must be valid to be counted.  Pixels
   * are considered valid based on the min/max range specified on each of
   * the Chips (see Chip::SetValidRange method).
   *
   * If the pattern chip reduction option is used this percentage will
   * apply to all reduced patterns.  Additionally, the pattern sampling
   * effects the pixel count.  For example if pattern sampling is 50% then
   * only 220 pixels in the 21x21 pattern are considered so 165 must be
   * valid.
   *
   * If this method is not called, the PatternChip ValidPercent defaults to 50
   * in the AutoReg object constructor.
   *
   * @see SetValidRange()
   * @param percent   Percentage of valid data between 0 and 100,
   *                  default is 50% if never invoked
   * @throw iException::User - "Invalid value for PatternChip ValidPercent."
   */
  void AutoReg::SetPatternValidPercent(const double percent) {
    if((percent <= 0.0) || (percent > 100.0)) {
      string msg = "Invalid value for PatternChip ValidPercent ["
        + IString(percent)
        + "].  Must be greater than 0.0 and less than or equal to 100.0 (Default is 50.0).";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    p_patternValidPercent = percent;
  }


  /**
   * Set the amount of data in the search chip's subchip that must be valid.
   *
   *
   * If this method is not called, the SearchChip SubchipValidPercent defaults
   * to 50 in the AutoReg object constructor.
   *
   *
   * @param percent   Percentage of valid data between 0 and 100,
   *                  default is 50% if never invoked
   * @see SetPatternValidPercent()
   * @throw iException::User - "Invalid value for SearchChip
   *        SubchipValidPercent."
   * @internal
   *   @author 2010-07-20 Jeannie Walldren
   *   @history 2010-07-20 Jeannie Walldren - Original Version.
   */
  void AutoReg::SetSubsearchValidPercent(const double percent) {
    if((percent <= 0.0) || (percent > 100.0)) {
      string msg = "Invalid value for SearchChip SubchipValidPercent ["
        + IString(percent) + "]"
        + "].  Must be greater than 0.0 and less than or equal to 100.0 (Default is 50.0).";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    p_subsearchValidPercent = percent;
  }


  /**
   * Set the minimum pattern zscore.  This option is used to
   * ignore pattern chips which are bland (low standard
   * deviation). If the minimum or maximum pixel value in the
   * pattern chip does not meet the minimum zscore value (see a
   * statisitcs book for definition of zscore) then invalid
   * registration will occur.
   *
   *
   * If this method is not called, the z-score minimum defaults to 1.0 in the
   * AutoReg object constructor.
   *
   * @param minimum The minimum zscore value for the pattern chip.
   *                 Default is 1.0
   * @throw iException::User - "Invalid value for PatternChip MinimumZScore."
   */
  void AutoReg::SetPatternZScoreMinimum(double minimum) {
    if(minimum <= 0.0) {
      string msg = "Invalid value for PatternChip MinimumZScore ["
        + IString(minimum)
        + "].  Must be greater than 0.0. (Default is 1.0).";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    p_minimumPatternZScore = minimum;
  }


  /**
   * Set the tolerance for an acceptable goodness of fit
   *
   *
   * If this method is not called, the tolerance value defaults to Isis::Null in
   * the AutoReg object constructor.
   *
   * @param tolerance   This tolerance is used to test against the goodness
   *                    of fit returned by the MatchAlgorith method after
   *                    a surface model has been fit.  See TestGoodnessOfFit
   */
  void AutoReg::SetTolerance(const double tolerance) {
    p_tolerance = tolerance;
  }

  /**
   * Sets the Chip class interpolator type to be used to load pattern and search
   * chips.
   * Acceptable values for the interpolator parameter include:
   * <UL>
   *   <LI>NearestNeighborType</LI>
   *   <LI>BiLinearType</LI>
   *   <LI>CubicConvolutionType</LI>
   * </UL>
   *
   * If this method is not called, the chip interpolator type defaults to
   * CubicConvolutionType in the Chip class.
   *
   * @param interpolator Name of interpolator type to be used.  This is taken from
   *                     the Pvl's ChipInterpolator keyword value.
   * @throw iException::User - "Invalid Interpolator type."
   * @author Jeannie Walldren
   * @internal
   *   @history 2010-06-15 Jeannie Walldren - Original version.
   */
  void AutoReg::SetChipInterpolator(const QString &interpolator) {

    Isis::Interpolator::interpType itype;
    if(interpolator == "NearestNeighborType") {
      itype = Isis::Interpolator::NearestNeighborType;
    }
    else if(interpolator == "BiLinearType") {
      itype = Isis::Interpolator::BiLinearType;
    }
    else if(interpolator == "CubicConvolutionType") {
      itype = Isis::Interpolator::CubicConvolutionType;
    }
    else {
      throw IException(IException::User,
                       "Invalid Interpolator type.  Cannot use ["
                       + interpolator + "] to load chip",
                       _FILEINFO_);
    }

    // Set pattern and search chips to use this interpolator type when reading data from cube
    p_patternChip.SetReadInterpolator(itype);
    p_searchChip.SetReadInterpolator(itype);
    p_reducedPatternChip.SetReadInterpolator(itype);
    p_reducedSearchChip.SetReadInterpolator(itype);

  }

  /**
   * Set the surface model window size. The pixels in this window
   * will be used to fit a surface model in order to compute
   * sub-pixel accuracy.  In some cases the default (3x3) and
   * produces erroneous sub-pixel accuracy values.
   *
   * If this method is not called, the window size defaults to 5 in the AutoReg
   * object constructor.
   *
   *  @param size The size of the window must be three or greater
   *             and odd.
   * @throw iException::User - "Invalid value for SurfaceModel WindowSize."
   */
  void AutoReg::SetSurfaceModelWindowSize(int size) {
    if(size % 2 != 1 || size < 3) {
      string msg = "Invalid value for SurfaceModel WindowSize ["
        + IString(size) + "].  Must be an odd number greater than or equal to 3";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    p_windowSize = size;
  }

  /**
   * Set a distance the surface model solution is allowed to move
   * away from the best whole pixel fit in the fit chip.
   *
   * If this method is not called, the distance tolerance defaults to 1.5 in the
   * AutoReg object constructor.
   *
   * @param distance The distance allowed to move in pixels.  Must
   *                 be greater than 0.
   * @throw iException::User - "Invalid value for SurfaceModel
   *        DistanceTolerance."
   */
  void AutoReg::SetSurfaceModelDistanceTolerance(double distance) {
    if(distance <= 0.0) {
      string msg = "Invalid value for SurfaceModel DistanceTolerance ["
        + IString(distance) + "].  Must greater than 0.0.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    p_distanceTolerance = distance;
  }


  /**
   * Set the reduction factor used to speed up the pattern
   * matching algorithm.
   *
   * If this method is not called, the reduction factor defaults to 1 in the
   * AutoReg object constructor.
   *
   * @param factor Reduction factor.  Must be greater than or equal to 1.
   * @throw iException::User - "Invalid value for Algorithm ReductionFactor."
   */
  void AutoReg::SetReductionFactor(int factor) {
    if(factor < 1) {
      string msg = "Invalid value for Algorithm ReductionFactor ["
        + IString(factor) + "].  Must greater than or equal to 1.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    p_reduceFactor = factor;
  }

  /**
   * This method reduces the given chip by the given reduction
   * factor. Used to speed up the match algorithm.
   *
   * @param chip Chip to be reduced
   * @param reductionFactor Factor by which to reduce chip.
   *
   * @return @b Chip Reduced chip object
   */
  Chip AutoReg::Reduce(Chip &chip, int reductionFactor) {
    Chip rChip((int)chip.Samples() / reductionFactor,
               (int)chip.Lines() / reductionFactor);
    if((int)rChip.Samples() < 1 || (int)rChip.Lines() < 1) {
      return chip;
    }

    // ----------------------------------
    // Fill the reduced Chip with nulls.
    // ----------------------------------
    for(int line = 1; line <= rChip.Lines(); line++) {
      for(int samp = 1; samp <= rChip.Samples(); samp++) {
        rChip.SetValue(samp, line, Isis::Null);
      }
    }

    Statistics stats;
    for(int l = 1; l <= rChip.Lines(); l++) {
      int istartLine = (l - 1) * reductionFactor + 1;
      int iendLine = istartLine + reductionFactor - 1;
      for(int s = 1; s <= rChip.Samples(); s++) {

        int istartSamp = (s - 1) * reductionFactor + 1;
        int iendSamp = istartSamp + reductionFactor - 1;

        stats.Reset();
        for(int line = istartLine; line < iendLine; line++) {
          for(int sample = istartSamp; sample < iendSamp; sample++) {
            stats.AddData(chip.GetValue(sample, line));
          }
        }
        rChip.SetValue(s, l, stats.Average());
      }
    }
    return rChip;
  }


  /**
   * Walk the pattern chip through the search chip to find the best registration
   *
   * @return @b AutoReg::RegisterStatus  Returns the status of the registration.
   * @throw iException::User - "Search chips samples must be at least N pixels
   *        wider than the pattern chip samples for successful surface modeling"
   * @throw iException::User - "Search chips lines must be at least N pixels
   *        taller than the pattern chip lines for successful surface modeling"
   * @throw iException::User - "Reduction factor is too large"
   */
  AutoReg::RegisterStatus AutoReg::Register() {
    // The search chip must be bigger than the pattern chip by N pixels in
    // both directions for a successful surface model
    int N = p_windowSize / 2 + 1;

    if(p_searchChip.Samples() < p_patternChip.Samples() + N) {
      string msg = "Search chips samples [";
      msg += IString(p_searchChip.Samples()) + "] must be at ";
      msg += "least [" + IString(N) + "] pixels wider than the pattern chip samples [";
      msg += IString(p_patternChip.Samples()) + "] for successful surface modeling";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if(p_searchChip.Lines() < p_patternChip.Lines() + N) {
      string msg = "Search chips lines [";
      msg += IString(p_searchChip.Lines()) + "] must be at ";
      msg += "least [" + IString(N) + "] pixels taller than the pattern chip lines [";
      msg += IString(p_patternChip.Lines()) + "] for successful surface modeling";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    Init();
    p_totalRegistrations++;

    // Create copies of the search and pattern chips and run a gradient filter
    // over them before attempting to perform a match. We do this so that
    // multiple calls to this method won't result in having a gradient filter
    // applied multiple times to the same chip.
    Chip gradientPatternChip(p_patternChip);
    Chip gradientSearchChip(p_searchChip);
    ApplyGradientFilter(gradientPatternChip);
    ApplyGradientFilter(gradientSearchChip);

    // See if the pattern chip has enough good data
    if(!gradientPatternChip.IsValid(p_patternValidPercent)) {
      p_patternChipNotEnoughValidDataCount++;
      p_registrationStatus = PatternChipNotEnoughValidData;
      return PatternChipNotEnoughValidData;
    }

    if(!ComputeChipZScore(gradientPatternChip)) {
      p_patternZScoreNotMetCount++;
      p_registrationStatus = PatternZScoreNotMet;
      return PatternZScoreNotMet;
    }

    /**
     * Prep for walking the search chip by computing the starting and ending
     * sample and line positions of the search chip to extract a sub-search
     * chip to compare with the pattern chip.
     *
     * Because the sub-search chip needs to have the same pixel dimensions as
     * the pattern chip, and will be composed from its center pixel outwards,
     * buffer the start and end boundaries so an area the size of the pattern
     * chip can always be extracted around the current position.
     *
     * For example, consider trying to extract a 5x5 sub-search chip from some
     * search chip.  If one starts at sample 1 and line 1, then because the
     * "current position" is treated as the center of the sub-search chip, the
     * algorithm could not form a 5x5 chip because there is nothing up and to
     * the left of the current position.  Consequently, for this example,
     * there needs to be a two-pixel buffer from the edge of the search chip
     * as the algorithm walks through it to make sure a 5x5 sub-search chip
     * can always be extracted with the current position as its center.
     */
    int startSamp = (gradientPatternChip.Samples() - 1) / 2 + 1;
    int startLine = (gradientPatternChip.Lines() - 1) / 2 + 1;
    int endSamp = gradientSearchChip.Samples() - startSamp + 1;
    int endLine = gradientSearchChip.Lines() - startLine + 1;

    // ----------------------------------------------------------------------
    // Before we attempt to apply the reduction factor, we need to make sure
    // we won't produce a chip of a bad size.
    // ----------------------------------------------------------------------
    if (p_reduceFactor != 1) {
      if(gradientPatternChip.Samples() / p_reduceFactor < 2 || gradientPatternChip.Lines() / p_reduceFactor < 2) {
        string msg = "Reduction factor is too large";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    // Establish the center search tack point as best pixel to start for the
    // adaptive algorithm prior to reduction.
    int bestSearchSamp = gradientSearchChip.TackSample();
    int bestSearchLine = gradientSearchChip.TackLine();

    // ---------------------------------------------------------------------
    // if the reduction factor is still not equal to one, then we go ahead
    // with the reduction of the chips and call Match to get the first
    // estimate of the best line/sample.
    // ---------------------------------------------------------------------
    if(p_reduceFactor != 1) {
      p_reducedPatternChip.SetSize((int)gradientPatternChip.Samples() / p_reduceFactor,
          (int)gradientPatternChip.Lines() / p_reduceFactor);

      // ----------------------------------
      // Fill the reduced Chip with nulls.
      // ----------------------------------
      for(int line = 1; line <= p_reducedPatternChip.Lines(); line++) {
        for(int samp = 1; samp <= p_reducedPatternChip.Samples(); samp++) {
          p_reducedPatternChip.SetValue(samp, line, Isis::Null);
        }
      }

      p_reducedPatternChip = Reduce(gradientPatternChip, p_reduceFactor);
      if(!ComputeChipZScore(p_reducedPatternChip)) {
        p_patternZScoreNotMetCount++;
        p_registrationStatus = PatternZScoreNotMet;
        return PatternZScoreNotMet;
      }

      p_reducedSearchChip = Reduce(gradientSearchChip, p_reduceFactor);
      int reducedStartSamp = (p_reducedPatternChip.Samples() - 1) / 2 + 1;
      int reducedEndSamp = p_reducedSearchChip.Samples() - reducedStartSamp + 1;
      int reducedStartLine = (p_reducedPatternChip.Lines() - 1) / 2 + 1;
      int reducedEndLine = p_reducedSearchChip.Lines() - reducedStartLine + 1;

      Match(p_reducedSearchChip, p_reducedPatternChip, p_reducedFitChip,
          reducedStartSamp, reducedEndSamp, reducedStartLine, reducedEndLine);

      if(p_bestFit == Isis::Null) {
        p_fitChipNoDataCount++;
        p_registrationStatus = FitChipNoData;
        return FitChipNoData;
      }

      // ------------------------------------------------------
      // p_bestSamp and p_bestLine are set in Match() which is
      // called above.
      // -----------------------------------------------------
      int bs = (p_bestSamp - 1) * p_reduceFactor + ((p_reduceFactor - 1) / 2) + 1;
      int bl = (p_bestLine - 1) * p_reduceFactor + ((p_reduceFactor - 1) / 2) + 1;

      // ---------------------------------------------------------------
      // Now we grow our window size according to the reduction factor.
      // And we grow around where the first call Match() told us was the
      // best line/sample.
      // ---------------------------------------------------------------
      int newstartSamp = bs - p_reduceFactor - p_windowSize - 1;
      int newendSamp = bs + p_reduceFactor + p_windowSize + 1;
      int newstartLine = bl - p_reduceFactor - p_windowSize - 1;
      int newendLine = bl + p_reduceFactor + p_windowSize + 1;

      if(newstartLine < startLine) newstartLine = startLine;
      if(newendSamp > endSamp) newendSamp = endSamp;
      if(newstartSamp < startSamp) newstartSamp = startSamp;
      if(newendLine > endLine) newendLine = endLine;

      startSamp = newstartSamp;
      endSamp = newendSamp;
      startLine = newstartLine;
      endLine = newendLine;
      // We have found a good pixel in the reduction chip, but we
      // don't want to use its position, so reset in prep. for
      // non-adaptive registration.  Save it off for the adaptive algorithm.
      bestSearchSamp = bs;
      bestSearchLine = bl;
      p_bestSamp = 0;
      p_bestLine = 0;
      p_bestFit = Isis::Null;
    }

    p_registrationStatus = Registration(gradientSearchChip, gradientPatternChip,
        p_fitChip, startSamp, startLine, endSamp, endLine,
        bestSearchSamp, bestSearchLine);

    gradientSearchChip.SetChipPosition(p_chipSample, p_chipLine);
    p_searchChip.SetChipPosition(p_chipSample, p_chipLine);
    p_cubeSample = gradientSearchChip.CubeSample();
    p_cubeLine   = gradientSearchChip.CubeLine();

    // Save off the gradient search and pattern chips if we used a gradient
    // filter.
    if (p_gradientFilterType != None) {
      p_gradientSearchChip = gradientSearchChip;
      p_gradientPatternChip = gradientPatternChip;
    }

    p_goodnessOfFit = p_bestFit;

    if (Success()) {
      if (p_registrationStatus == AutoReg::SuccessSubPixel)
        p_subpixelSuccesses++;
      else
        p_pixelSuccesses++;
    }

    return p_registrationStatus;
  }


  /**
   * Performs matching between the pattern and search at both whole-pixel and
   * subpixel levels.  For adaptive algorithms, only subpixel matching occurs.
   * Such algorithms override this method to use their alternative matching
   * procedures.
   *
   * For those algorithms that need it, the best sample and line in the
   * search chip is provided.  This is either the initial tack sample and
   * line in the search chip or it is the centered sample and line after the
   * reduction algorithm is applied (KJB, 2009-08-26).
   *
   * @author janderson (6/2/2009)
   *
   * @param sChip Search chip
   * @param pChip Pattern chip
   * @param fChip Fit chip
   * @param startSamp Defines the starting sample of the window
   *                  the algorithm should remain inside
   *                  this boundary.
   * @param startLine Defines the starting line of the window
   *                  the algorithm should remain inside
   *                  this boundary.
   * @param endSamp Defines the ending sample of the window
   *                  the algorithm should remain inside
   *                  this boundary.
   * @param endLine Defines the ending line of the window
   *                  the algorithm should remain inside
   *                  this boundary.
   * @param bestSamp Best sample
   * @param bestLine Best line
   *
   * @return @b AutoReg::RegisterStatus  Status of match
   */
  AutoReg::RegisterStatus AutoReg::Registration(Chip &sChip, Chip &pChip,
      Chip &fChip, int startSamp, int startLine, int endSamp, int endLine,
      int bestSamp, int bestLine) {

    // Not adaptive, continue with slower search traverse
    Match(sChip, pChip, fChip, startSamp, endSamp, startLine, endLine);

    // Check to see if we went through the fit chip and never got a fit at
    // any location.
    if (p_bestFit == Isis::Null) {
      p_fitChipNoDataCount++;
      p_registrationStatus = FitChipNoData;
      return FitChipNoData;
    }

    // Now see if we satisified the goodness of fit tolerance
    if (!CompareFits(p_bestFit, Tolerance())) {
      p_fitChipToleranceNotMetCount++;
      p_registrationStatus = FitChipToleranceNotMet;
      return FitChipToleranceNotMet;
    }

    // Try to fit a model for sub-pixel accuracy if necessary
    if (p_subpixelAccuracy && !IsIdeal(p_bestFit)) {
      Chip window(p_windowSize, p_windowSize);
      fChip.Extract(p_bestSamp, p_bestLine, window);
      window.SetChipPosition(p_windowSize / 2 + 1, p_windowSize / 2 + 1);

      // Make sure more than 2/3 of the data in the window is valid.  Otherwise,
      // we are likely too close to the edge.
      if (!window.IsValid(100.0 * 2.1 / 3.0)) {
        p_surfaceModelNotEnoughValidDataCount++;
        p_registrationStatus = SurfaceModelNotEnoughValidData;
        p_chipSample = p_bestSamp;
        p_chipLine = p_bestLine;
        return SurfaceModelNotEnoughValidData;
      }

      // Now that we know we have enough data to model the surface we call
      // SetSubpixelPosition() to get the sub-pixel accuracy we are looking for.
      bool computedSubPixel = SetSubpixelPosition(window);
      if (!computedSubPixel) {
        p_chipSample = p_bestSamp;
        p_chipLine = p_bestLine;
        p_registrationStatus = SurfaceModelSolutionInvalid;
        return SurfaceModelSolutionInvalid;
      }
      
      // See if the surface model solution moved too far from our whole pixel
      // solution
      p_sampMovement = fabs(p_bestSamp - p_chipSample);
      p_lineMovement = fabs(p_bestLine - p_chipLine);
      if (p_sampMovement > p_distanceTolerance ||
          p_lineMovement > p_distanceTolerance) {

        p_surfaceModelDistanceInvalidCount++;
        p_registrationStatus = SurfaceModelDistanceInvalid;
        p_chipSample = p_bestSamp;
        p_chipLine = p_bestLine;
        return SurfaceModelDistanceInvalid;
      }

      p_registrationStatus = SuccessSubPixel;
      return SuccessSubPixel;
    }
    else {
      p_chipSample = p_bestSamp;
      p_chipLine = p_bestLine;
      p_registrationStatus = SuccessPixel;
      return SuccessPixel;
    }
  }


  /**
   * This method computes the given Chip's Z-Score. If this value is less than the
   * minimum pattern Z-Score or greater than the negative of the minimum pattern
   * Z-Score, the method will return false.  Otherwise, it returns true.
   *
   * @param chip Chip object whose Z-Score is calculated
   *
   * @return @b bool Indicates whether Z-Score calculated lies between the minimum
   *         and Pattern Z-Score and its negative.
   */
  bool AutoReg::ComputeChipZScore(Chip &chip) {
    Statistics patternStats;
    for(int i = 0; i < chip.Samples(); i++) {
      double pixels[chip.Lines()];
      for(int j = 0; j < chip.Lines(); j++) {
        pixels[j] = chip.GetValue(i + 1, j + 1);
      }
      patternStats.AddData(pixels, chip.Lines());
    }

    // If it does not pass, return
    p_zScoreMin = patternStats.ZScore(patternStats.Minimum());
    p_zScoreMax = patternStats.ZScore(patternStats.Maximum());

    // p_zScoreMin is made negative here so as to make it the equivalent of
    // taking the absolute value (because p_zScoreMin is guaranteed to be
    // negative)
    if (p_zScoreMax < p_minimumPatternZScore && -p_zScoreMin < p_minimumPatternZScore) {
      return false;
    }
    else {
      return true;
    }
  }

  /**
   * Run a gradient filter over the chip. The type of filter is determined by
   * the Gradient keyword in the Algorithm group.
   *
   * @param chip the chip to be filtered
   * @throw iException::Programmer - "Invalid Gradient type."
   */
  void AutoReg::ApplyGradientFilter(Chip &chip) {
    if (p_gradientFilterType == None) {
      return;
    }

    // Use a different subchip size depending on which gradient filter is
    // being applied.
    int subChipWidth;
    if (p_gradientFilterType == Sobel) {
      subChipWidth = 3;
    }
    else {
      // Perform extra sanity check.
      string msg =
        "No rule to set sub-chip width for selected Gradient Filter Type.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Create a new chip to hold output during processing.
    Chip filteredChip(chip.Samples(), chip.Lines());

    // Move the subchip through the chip, extracting the contents into a buffer
    // of the same shape. This simulates the processing of a cube by boxcar,
    // but since that can only operate on cubes, this functionality had to be
    // replicated for use on chips.
    for (int line = 1; line <= chip.Lines(); line++) {
      for (int sample = 1; sample <= chip.Samples(); sample++) {
        Chip subChip = chip.Extract(subChipWidth, subChipWidth,
            sample, line);

        // Fill a buffer with subchip's contents. Since we'll never be storing
        // raw bytes in the buffer, we don't care about the pixel type.
        Buffer buffer(subChipWidth, subChipWidth, 1, Isis::None);
        double *doubleBuffer = buffer.DoubleBuffer();
        int bufferIndex = 0;
        for (int subChipLine = 1; subChipLine <= subChip.Lines();
            subChipLine++) {
          for (int subChipSample = 1; subChipSample <= subChip.Samples();
              subChipSample++) {
            doubleBuffer[bufferIndex] = subChip.GetValue(subChipSample,
                subChipLine);
            bufferIndex++;
          }
        }

        // Calculate gradient based on contents in buffer and insert it into
        // output chip.
        double newPixelValue = 0;
        if (p_gradientFilterType == Sobel) {
          SobelGradient(buffer, newPixelValue);
        }
        filteredChip.SetValue(sample, line, newPixelValue);
      }
    }

    // Copy the data from the filtered chip back into the original chip.
    for (int line = 1; line <= filteredChip.Lines(); line++) {
      for (int sample = 1; sample <= filteredChip.Samples(); sample++) {
        chip.SetValue(sample, line, filteredChip.GetValue(sample, line));
      }
    }
  }


  /**
   * Compute a Sobel gradient based on an input buffer.
   *
   * TODO: Remove this method as it already exists in the
   * gradient application.
   *
   * @param in the input buffer
   * @param v the value of the gradient computed from the buffer
   */
  void AutoReg::SobelGradient(Buffer &in, double &v) {
    bool specials = false;
    for(int i = 0; i < in.size(); ++i) {
      if(IsSpecial(in[i])) {
        specials = true;
      }
    }
    if(specials) {
      v = Isis::Null;
      return;
    }
    v = abs((in[0] + 2 * in[1] + in[2]) - (in[6] + 2 * in[7] + in[8])) +
        abs((in[2] + 2 * in[5] + in[8]) - (in[0] + 2 * in[3] + in[6]));
  }

  /**
   * Here we walk from start sample to end sample and start line to end line, and
   * compare the pattern chip against the search chip to find the
   * best line/sample.
   *
   * @param sChip Search chip
   * @param pChip Pattern chip
   * @param fChip Fit chip
   * @param startSamp Start sample
   * @param endSamp End sample
   * @param startLine Start line
   * @param endLine End line
   *
   * @throw iException::Programmer - "StartSample = EndSample and StartLine =
   *        EndLine."
   */
  void AutoReg::Match(Chip &sChip, Chip &pChip, Chip &fChip, int startSamp, int endSamp, int startLine, int endLine) {
    // Sanity check.  Should have been caught by the two previous tests
    if(startSamp == endSamp && startLine == endLine) {
      string msg = "StartSample [" + IString(startSamp) + "] = EndSample ["
        + IString(endSamp) + "] and StartLine [" + IString(startLine) + " = EndLine ["
        + IString(endLine) + "].";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Ok create a fit chip whose size is the same as the search chip
    // and then fill it with nulls
    fChip.SetSize(sChip.Samples(), sChip.Lines());
    for(int line = 1; line <= fChip.Lines(); line++) {
      for(int samp = 1; samp <= fChip.Samples(); samp++) {
        fChip.SetValue(samp, line, Isis::Null);
      }
    }

    // Create a chip the same size as the pattern chip.
    Chip subsearch(pChip.Samples(), pChip.Lines());

    for(int line = startLine; line <= endLine; line++) {
      for(int samp = startSamp; samp <= endSamp; samp++) {
        // Extract the subsearch chip and make sure it has enough valid data
        sChip.Extract(samp, line, subsearch);

//        if(!subsearch.IsValid(p_patternValidPercent)) continue;
        if(!subsearch.IsValid(p_subsearchValidPercent)) continue;

        // Try to match the two subchips
        double fit = MatchAlgorithm(pChip, subsearch);

        // If we had a fit save off information about that fit
        if(fit != Isis::Null) {
          fChip.SetValue(samp, line, fit);
          if((p_bestFit == Isis::Null) || CompareFits(fit, p_bestFit)) {
            p_bestFit = fit;
            p_bestSamp = samp;
            p_bestLine = line;
          }
        }
      }
    }
  }


  /**
   * Set the search chip sample and line to subpixel values if possible.  This
   * method uses a centroiding method to gravitate the whole pixel best fit to a
   * subpixel extremum in the continuous image space.  The weights of the
   * centers of gravity in the centroiding algorithm are modeled by goodness of
   * fit values within a discrete search window.
   *
   * @param window The search window extracted from the fit chip
   * @return @b bool Returns true if the subpixel solution is valid
   */
  bool AutoReg::SetSubpixelPosition(Chip &window) {
    // The best correlation will be at the center of the window
    //   if it's smaller than the edge DN's invert the chip DNs
    double samples = window.Samples();
    double lines= window.Lines();
    double bestDN = window.GetValue(window.ChipSample(), window.ChipLine());
    if (bestDN < window.GetValue(1, 1)) {
      for (int s=1; s <= samples; s++)
        for (int l=1; l <= lines; l++)
          window.SetValue(s, l, 1.0/window.GetValue(s, l)); //invert all the window DN's
      bestDN = 1 / bestDN;
    }

    // Find the greatest edge DN
    double greatestEdgeDn = 0.0;
    for (int s = 1; s <= samples; s++) {
      greatestEdgeDn = max(window.GetValue(s, 1), greatestEdgeDn);
      greatestEdgeDn = max(window.GetValue(s, lines), greatestEdgeDn);
    }
    for (int l = 2; l <= lines - 1; l++) {
      greatestEdgeDn = max(window.GetValue(1, l), greatestEdgeDn);
      greatestEdgeDn = max(window.GetValue(samples, l), greatestEdgeDn);
    }

    //This is a small shift so the the centroid doesn't reach the edge, add 20%
    //  of the difference between the hightest edge DN and the max DN to the highest edge DN
    //The 20% shift added here is somewhat arbitrary, but was choosen because it worked well
    //  for the maximum correlation tests we did.  For new area based algorithms we may want
    //  to revist this.  Possible make it a function of the match type
    double temp = greatestEdgeDn + 0.2 * (bestDN - greatestEdgeDn);

    Centroid floodFill;
    floodFill.setDNRange(temp, 1e100);

    Chip selectionChip(window);
    floodFill.select(&window, &selectionChip);

    double windowSample;
    double windowLine;
    floodFill.centerOfMassWeighted(
        &window, &selectionChip, &windowSample, &windowLine);

    int offsetS = p_bestSamp - window.ChipSample();
    int offsetL = p_bestLine - window.ChipLine();
    p_chipSample = windowSample + offsetS;
    p_chipLine = windowLine + offsetL;

    if (p_chipSample != p_chipSample) {
      p_surfaceModelSolutionInvalidCount++;
      return false;  //this should never happen, but just in case...
    }

    return true;
  }


  /**
   * This virtual method must return if the 1st fit is equal to or better
   * than the second fit.
   *
   * @param fit1  1st goodness of fit
   * @param fit2  2nd goodness of fit
   * @return @b bool Indicates whether the first fit is as good or better than the
   *         second
   */
  bool AutoReg::CompareFits(double fit1, double fit2) {
    return(std::fabs(fit1 - IdealFit()) <= std::fabs(fit2 - IdealFit()));
  }

  /**
   * Returns true if the fit parameter is arbitrarily close to the ideal fit
   * value.
   * @param fit Fit value to be compared to the ideal fit
   * @return @b bool Indicates whether the fit is ideal
   */
  bool AutoReg::IsIdeal(double fit) {
    return(std::fabs(IdealFit() - fit) < 0.00001);
  }


  /**
   * This returns the cumulative registration statistics.  That
   * is, the Register() method accumulates statistics regard the
   * errors each time is called.  Invoking this method returns a
   * PVL summary of those statisitics
   *
   * @author janderson (3/26/2009)
   *
   * @return @b Pvl
   */
  Pvl AutoReg::RegistrationStatistics() {
    Pvl pvl;
    PvlGroup stats("AutoRegStatistics");
    stats += Isis::PvlKeyword("Total", std::to_string(p_totalRegistrations));
    stats += Isis::PvlKeyword("Successful", std::to_string(p_pixelSuccesses + p_subpixelSuccesses));
    stats += Isis::PvlKeyword("Failure", std::to_string(p_totalRegistrations - (p_pixelSuccesses + p_subpixelSuccesses)));
    pvl.addGroup(stats);

    PvlGroup successes("Successes");
    successes += PvlKeyword("SuccessPixel", std::to_string(p_pixelSuccesses));
    successes += PvlKeyword("SuccessSubPixel", std::to_string(p_subpixelSuccesses));
    pvl.addGroup(successes);

    PvlGroup grp("PatternChipFailures");
    grp += PvlKeyword("PatternNotEnoughValidData", std::to_string(p_patternChipNotEnoughValidDataCount));
    grp += PvlKeyword("PatternZScoreNotMet", std::to_string(p_patternZScoreNotMetCount));
    pvl.addGroup(grp);

    PvlGroup fit("FitChipFailures");
    fit += PvlKeyword("FitChipNoData", std::to_string(p_fitChipNoDataCount));
    fit += PvlKeyword("FitChipToleranceNotMet", std::to_string(p_fitChipToleranceNotMetCount));
    pvl.addGroup(fit);

    PvlGroup model("SurfaceModelFailures");
    model += PvlKeyword("SurfaceModelNotEnoughValidData", std::to_string(p_surfaceModelNotEnoughValidDataCount));
    model += PvlKeyword("SurfaceModelSolutionInvalid", std::to_string(p_surfaceModelSolutionInvalidCount));
    model += PvlKeyword("SurfaceModelDistanceInvalid", std::to_string(p_surfaceModelDistanceInvalidCount));
    pvl.addGroup(model);

    return (AlgorithmStatistics(pvl));
  }

  /**
   * This function returns the keywords that this object was
   * created from.
   *
   * @return @b PvlGroup The keywords this object used in
   *         initialization
   */
  PvlGroup AutoReg::RegTemplate() {
    PvlGroup reg("AutoRegistration");

    PvlGroup &algo = p_template.findGroup("Algorithm", Pvl::Traverse);
    reg += PvlKeyword("Algorithm", algo["Name"][0]);
    reg += PvlKeyword("Tolerance", algo["Tolerance"][0]);
    if(algo.hasKeyword("SubpixelAccuracy")) {
      reg += PvlKeyword("SubpixelAccuracy", algo["SubpixelAccuracy"][0]);
    }
    if(algo.hasKeyword("ReductionFactor")) {
      reg += PvlKeyword("ReductionFactor", algo["ReductionFactor"][0]);
    }
    if(algo.hasKeyword("Gradient")) {
      reg += PvlKeyword("Gradient", algo["Gradient"][0]);
    }

    PvlGroup &pchip = p_template.findGroup("PatternChip", Pvl::Traverse);
    reg += PvlKeyword("PatternSamples", pchip["Samples"][0]);
    reg += PvlKeyword("PatternLines", pchip["Lines"][0]);
    if(pchip.hasKeyword("ValidMinimum")) {
      reg += PvlKeyword("PatternMinimum", pchip["ValidMinimum"][0]);
    }
    if(pchip.hasKeyword("ValidMaximum")) {
      reg += PvlKeyword("PatternMaximum", pchip["ValidMaximum"][0]);
    }
    if(pchip.hasKeyword("MinimumZScore")) {
      reg += PvlKeyword("MinimumZScore", pchip["MinimumZScore"][0]);
    }
    if(pchip.hasKeyword("ValidPercent")) {
      SetPatternValidPercent((double)pchip["ValidPercent"]);
      reg += PvlKeyword("ValidPercent", pchip["ValidPercent"][0]);
    }

    PvlGroup &schip = p_template.findGroup("SearchChip", Pvl::Traverse);
    reg += PvlKeyword("SearchSamples", schip["Samples"][0]);
    reg += PvlKeyword("SearchLines", schip["Lines"][0]);
    if(schip.hasKeyword("ValidMinimum")) {
      reg += PvlKeyword("SearchMinimum", schip["ValidMinimum"][0]);
    }
    if(schip.hasKeyword("ValidMaximum")) {
      reg += PvlKeyword("SearchMaximum", schip["ValidMaximum"][0]);
    }
    if(schip.hasKeyword("SubchipValidPercent")) {
      SetSubsearchValidPercent((double)schip["SubchipValidPercent"]);
      reg += PvlKeyword("SubchipValidPercent", schip["SubchipValidPercent"][0]);
    }

    if(p_template.hasGroup("SurfaceModel")) {
      PvlGroup &smodel = p_template.findGroup("SurfaceModel", Pvl::Traverse);
      if(smodel.hasKeyword("DistanceTolerance")) {
        reg += PvlKeyword("DistanceTolerance", smodel["DistanceTolerance"][0]);
      }

      if(smodel.hasKeyword("WindowSize")) {
        reg += PvlKeyword("WindowSize", smodel["WindowSize"][0]);
      }
    }

    return reg;
  }


  /**
   * Returns a PvlGroup containing the PvlKeywords of the parameters this object
   * was most recently run with.  Because of publically accessible mutators,
   * AutoReg's runtime parameters can change, and this version of the template
   * is designed to reflect the paramters it was actually run with, as opposed
   * to simply initialized with.  If no mutators were ever called externally,
   * then this PvlGroup should be the same as the one received from
   * RegTemplate().
   *
   * @return @b PvlGroup The keywords this object was most recently run with
   */
  PvlGroup AutoReg::UpdatedTemplate() {
    PvlGroup reg("AutoRegistration");

    reg += PvlKeyword("Algorithm", AlgorithmName().toStdString());
    reg += PvlKeyword("Tolerance", std::to_string(Tolerance()));
    reg += PvlKeyword("SubpixelAccuracy",
        SubPixelAccuracy() ? "True" : "False");
    reg += PvlKeyword("ReductionFactor", std::to_string(ReductionFactor()));
    reg += PvlKeyword("Gradient", GradientFilterString().toStdString());

    Chip *pattern = PatternChip();
    reg += PvlKeyword("PatternSamples", std::to_string(pattern->Samples()));
    reg += PvlKeyword("PatternLines", std::to_string(pattern->Lines()));
    reg += PvlKeyword("MinimumZScore", std::to_string(MinimumZScore()));
    reg += PvlKeyword("ValidPercent", std::to_string(PatternValidPercent()));
    // TODO Chip needs accessors to valid minimum and maximum

    Chip *search = SearchChip();
    reg += PvlKeyword("SearchSamples", std::to_string(search->Samples()));
    reg += PvlKeyword("SearchLines", std::to_string(search->Lines()));
    reg += PvlKeyword("SubchipValidPercent", std::to_string(SubsearchValidPercent()));
    // TODO Chip needs accessors to valid minimum and maximum

    if (SubPixelAccuracy()) {
      reg += PvlKeyword("DistanceTolerance", std::to_string(DistanceTolerance()));
      reg += PvlKeyword("WindowSize", std::to_string(WindowSize()));
    }

    return reg;
  }
}

