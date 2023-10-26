#ifndef Gruen_h
#define Gruen_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include "AutoReg.h"
#include "tnt/tnt_array2d.h"
#include "tnt/tnt_array1d.h"
#include "Affine.h"
#include "DbProfile.h"
#include "GruenTypes.h"
#include "CollectorMap.h"
#include "Statistics.h"
#include "Pvl.h"
#include "PvlGroup.h"

namespace Isis {
  class Chip;

  /**
   * @brief Gruen pattern matching algorithm
   *
   * This class provides adaptive image (chip) registration using
   * the AutoReg factory architecture.  This algorithm uses an
   * Affine transform to iteratively adjust the search chip at
   * each iteration.  Each iteration solves for new adjustments to
   * the Affine transform until the 6 affine parameters fall below the
   * tolerances as specified in AffineThreshHold1 and AffineThreshHold2.
   *
   * This class minimizes the 6 specifiable Affine transform components of a 3x3
   * matrix.  The three Affine components for X (sample) and Y (line) are scale,
   * shear and translation.  Gruen provides control over the maximum values
   * these three components should attain in order for the registration to
   * converge to a successful match.  These limits are specified
   * by <b>AffineScaleTolerance</b>,  <b>AffineShearTolerance</b> and
   * <b>AffineTranslationTolerance</b>.   <b>AffineShearTolerance</b> is
   * optional and if not specified, it defaults to the value of
   * <b>AffineScaleTolerance</b>.  These tolerances specify the maximum amount
   * of translation pixels can be shifted between one Gruen iteration and
   * another.  For example, <b>AffineTranslationTolerance = 0.2</b> means that a
   * subsearch chip cannot move in sample or line direction more than 0.2 pixels
   * in order to satisfy convergence.  <b>AffineScaleTolerance</b> constrains
   * the sample and line scale elements of the Affine transformation.  And
   * <b>AffineShearTolerance</b> constrains the sample and line shear elements
   * of the Affine transformation.  The scale and shear parameters are scaled by
   * size of the Chip.  The sample scale and shear Affine components maximum
   * limit is computed as <b>AffineScaleTolerance/((#Samples-1)/2)</b> and
   * <b>AffineShearTolerance/((#Samples-1)/2)</b>.  Likewise, the line scale and
   * shear maximums are computed using <b>#Lines</b> in the preceding equation.
   *
   * @ingroup PatternMatching
   *
   * @see MinimumGruen AdaptiveGruen AutoReg
   *
   * @author  2009-09-09 Kris Becker
   *
   * @internal
   *   @history 2009-09-10 Kris Becker Changed AffineThreshHold1 to
   *            AffineTranslationTolerance and AffineThreshHold2 to
   *            AffineScaleTolerance.  Added AffineShearTolerance. Added logging
   *            of Gruen parameters.
   *   @history 2009-09-11 Kris Becker Minor error in computation of radiometric
   *            shift
   *   @history 2010-06-14 Jeannie Walldren - Capitalized getTransform and
   *            setTransform to match changes in Chip class
   *   @history 2011-05-23 Kris Becker - Reworked major portions of
   *            implementation for a more modular support.
   */
  class Gruen : public AutoReg {
    public:

      /** Default constructor */
      Gruen();
      Gruen(Pvl &pvl);

      //! Destructor
      virtual ~Gruen() { }

      /** Returns the current call count */
      BigInt CallCount() const { return (m_callCount); }

      void WriteSubsearchChips(const QString &pattern = "SubChip");

      AffineTolerance getAffineTolerance() const;

      /** Returns the SPICE tolerance constraint as read from config file */
      inline double getSpiceConstraint() const { return (m_spiceTol); }

      /** Returns the Affine tolerance constraint as read from config file */
      inline double getAffineConstraint() const { return (m_affineTol); }

      void setAffineRadio(const AffineRadio &affrad);
      void setAffineRadio();

      /** Returns default settings for Affine/Radiometric parameters */
      const AffineRadio &getDefaultAffineRadio() const { return (m_defAffine);}

      /** Return current state of Affine/Radio state  */
      const AffineRadio &getAffineRadio() const { return (m_affine);  }

      /**
       * @brief Returns the register state of the last successful Gruen match
       *
       * This method returns the full match condition of the last call to Gruen
       * Register function that was successful.
       *
       * BEWARE:  This is only valid if Register returns successfully!  This is
       * due to AutoReg returning conditions that occur prior to the actual
       * Gruen algorithm being called.
       *
       * @author Kris Becker - 6/4/2011
       *
       * @return MatchPoint
       */
      MatchPoint getLastMatch() const { return (m_point);   }

    protected:
      /** Returns the default name of the algorithm as Gruen */
      virtual QString AlgorithmName() const {
        return ("Gruen");
      }

      int algorithm(Chip &pattern, Chip &subsearch, const Radiometric &radio,
                    BigInt &ptsUsed, double &resid, GMatrix &atai,
                    AffineRadio &affrad);

      Analysis errorAnalysis(const BigInt &npts, const double &resid,
                             const GMatrix &atai);

      //  These methods are for AutoReg non-adaptive requirements
      virtual double MatchAlgorithm(Chip &pattern, Chip &subsearch);
      virtual bool CompareFits(double fit1, double fit2);

      /** Returns the ideal fit for a perfect Gruen result */
      virtual double IdealFit() const {  return (0.0); }

      // AutoReg adaptive method
      virtual AutoReg::RegisterStatus Registration(Chip &sChip,
          Chip &pChip,
          Chip &fChip,
          int startSamp,
          int startLine,
          int endSamp,
          int endLine,
          int bestSamp,
          int bestLine);

      virtual Pvl AlgorithmStatistics(Pvl &pvl);

    private:
      /** Error enumeration values */
      enum ErrorTypes {  NotEnoughPoints = 1, CholeskyFailed = 2,
                         EigenSolutionFailed = 3, AffineNotInvertable = 4,
                         MaxIterationsExceeded = 5, RadShiftExceeded = 6,
                         RadGainExceeded = 7, MaxEigenExceeded = 8,
                         AffineDistExceeded = 9
                       };


      /** Struct that maintains error counts */
      struct ErrorCounter {
        ErrorCounter() : m_gerrno(0), m_keyname("Unknown"), m_count(0) { }
        ErrorCounter(int gerrno, const QString &keyname) : m_gerrno(gerrno),
          m_keyname(keyname), m_count(0) { }

        inline int  Errno() const {  return (m_gerrno); }
        inline BigInt Count() const { return (m_count); }
        inline void BumpIt() { m_count++; }
        PvlKeyword LogIt() const { return (PvlKeyword(m_keyname.toStdString(), std::to_string(m_count))); }

        int         m_gerrno;
        QString m_keyname;
        BigInt      m_count;
      };

      /// Declaration of error count list
      typedef CollectorMap<int, ErrorCounter> ErrorList;

      // Iteration loop variables
      BigInt       m_callCount;
      QString  m_filePattern;

      ErrorList    m_errors;            // Error logger
      BigInt       m_unclassified;      // Unclassified errors

      // Tolerance and count parameters
      int          m_maxIters;          // Maximum iterations
      int          m_nIters;            // Number iterations
      BigInt       m_totalIterations;   // Running total iteration count

      DbProfile    m_prof;              //  Parameter profile
      double       m_transTol;          //  Affine translation tolerance
      double       m_scaleTol;          //  Affine scale tolerance
      double       m_shearTol;          //  Affine shear tolerance
      double       m_spiceTol;          // SPICE tolerance
      double       m_affineTol;         // Affine Tolerance

      double       m_shiftTol;          // Radiometric shift tolerance
      double       m_rgainMinTol;       // Radiometric Gain minimum
      double       m_rgainMaxTol;       // Radiometric Gain maximum

      double       m_defGain;           // Default Radiometric gain
      double       m_defShift;          // Default Radiometric shift

      // These are for recomputing SMTK points
      AffineRadio  m_defAffine;         // Default affine/radiometric settings
      AffineRadio  m_affine;            // Incoming affine setting
      MatchPoint   m_point;             // Last resuting registration result

      //  Statistics gathered during processing
      Statistics   m_eigenStat;
      Statistics   m_iterStat;
      Statistics   m_shiftStat;
      Statistics   m_gainStat;


      //  Static init of default PVL parameters
      static Pvl &getDefaultParameters();

      /**
       * @brief Helper method to initialize parameters
       *
       * This method will check the existance of a keyword and extract the value

       * if it exists to the passed parameter (type).  If it doesn't exist, the
       * default values is returned.
       *
       * @param T Templated variable type
       * @param conf Parameter profile container
       * @param keyname Name of keyword to get a value from
       * @param defval Default value it keyword/value doesn't exist
       * @param index Optional index of the value for keyword arrays
       *
       * @return T Return type
       */
      QString ConfKey(const DbProfile &conf, const QString &keyname,
                const QString &defval, int index = 0) const {
        if(!conf.exists(keyname)) {
          return (defval);
        }
        if(conf.count(keyname) < index) {
          return (defval);
        }
        QString iValue(conf.value(keyname, index));
        IString tmp(iValue);
        QString value = tmp.ToQt();  // This makes it work with a string?
        return (value);
      };

      /**
       * @brief Helper method to initialize parameters
       *
       * This method will check the existance of a keyword and extract the value

       * if it exists to the passed parameter (type).  If it doesn't exist, the
       * default values is returned.
       *
       * @param T Templated variable type
       * @param conf Parameter profile container
       * @param keyname Name of keyword to get a value from
       * @param defval Default value it keyword/value doesn't exist
       * @param index Optional index of the value for keyword arrays
       *
       * @return T Return type
       */
      template <typename T>
      T ConfKey(const DbProfile &conf, const QString &keyname,
                const T &defval, int index = 0) const {
        if(!conf.exists(keyname)) {
          return (defval);
        }
        if(conf.count(keyname) < index) {
          return (defval);
        }
        QString iValue(conf.value(keyname, index));
        IString tmp(iValue);
        T value = tmp;  // This makes it work with a string?
        return (value);
      };


      /**
       * @brief Keyword formatter for Gruen parameters
       *
       * Constructs a keyword with actual user/programmer values if provided,
       * otherwise sets the value to "Unbounded".
       *
       * @param T       Type of value to record
       * @param keyname Name of keyword to create
       * @param value   Value to set keyword to if in profile
       * @param unit    Optional unit value
       *
       * @return PvlKeyword Constructed keyword for key/value
       */
      template <typename T>
      PvlKeyword ParameterKey(const QString &keyname,
                              const T &value,
                              const QString &unit = "") const {
        if(m_prof.exists(keyname)) {
          return(ValidateKey(keyname, value, unit));
        }
        return (PvlKeyword(keyname.toStdString(), "Unbounded"));
      };


      /**
       * @brief Checks value of key, produces appropriate value
       *
       * This function checks the value of the keyword for specialness
       * and will create the appropriate keyword if it is special.
       *
       * @param keyname Name of keyword to create
       * @param value   Keyword value
       * @param unit    Optional unit qualifer with value
       *
       * @return PvlKeyword Returns newly created keyword/value
       */
      inline PvlKeyword ValidateKey(const QString keyname,
                                    const double &value,
                                    const QString &unit = "") const {
        if(IsSpecial(value)) {
          return (PvlKeyword(keyname.toStdString(), "NULL"));
        }
        else {
          return (PvlKeyword(keyname.toStdString(), std::to_string(value), unit.toStdString()));
        }
      }

      ErrorList initErrorList() const;
      int logError(int gerrno, const QString &gerrmsg);
      PvlGroup StatsLog() const;
      PvlGroup ParameterLog() const;

      void init(Pvl &pvl);

      /** Returns the default radiometric gain value */
      inline Radiometric getDefaultRadio() const {
        return (Radiometric(m_defShift, m_defGain));
      }

      /** Returns number of degrees of freedom of points */
      inline double DegreesOfFreedom(const int npts) const {
        return ((double) (npts - NCONSTR));
      }

      void resetStats();

      GMatrix Identity(const int &ndiag = 3) const;
      GMatrix Choldc(const GMatrix &a, GVector &p) const;
      GMatrix Cholsl(const GMatrix &a, const GVector &p,
                     const GMatrix &b, const GMatrix &x) const;
      int Jacobi(const GMatrix &a, GVector &evals, GMatrix &evecs,
                  const int &MaxIters = 50) const;
      void EigenSort(GVector &evals, GMatrix &evecs) const;
      BigInt MinValidPoints(BigInt totalPoints) const;
      bool ValidPoints(BigInt totalPoints, BigInt nPoints) const;

      int CheckConstraints(MatchPoint &point);
      Coordinate getChipUpdate(Chip &sChip, MatchPoint &point) const;
      AutoReg::RegisterStatus Status(const MatchPoint &result);
  };

}  // namespace Isis
#endif
