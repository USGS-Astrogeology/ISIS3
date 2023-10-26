#ifndef InterestOperator_h
#define InterestOperator_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <string>
#include <vector>
#include "PvlGroup.h"
#include "Camera.h"
#include "UniversalGroundMap.h"
#include "ControlNetValidMeasure.h"
#include "ImageOverlapSet.h"

#include "geos/geom/Point.h"
#include "geos/geom/Coordinate.h"
#include "geos/geom/MultiPolygon.h"
#include "geos/util/GEOSException.h"

namespace Isis {
  class Chip;
  class Pvl;
  class Cube;
  class PvlObject;
  class ControlNet;
  class ControlPoint;
  class ControlMeasure;

  /**
   * @brief Interest Operator class
   *
   * Create InterestOperator object.  Because this is a pure virtual class you
   * can not create an InterestOperator class directly.  Instead, see the
   * InterestOperatorFactory class.
   *
   * @see StandardDeviationOperator GradientOperator
   *
   * @author 2006-02-11 Jacob Danton
   *
   * @internal
   *   @history 2006-02-11 Jacob Danton - Original Version
   *   @history 2007-08-02 Steven Koechle - Added better documentation to CompareInterests().
   *   @history 2007-08-02 Steven Koechle - Fixed looping error that caused subchip to go
   *                           outside the chip to the left and top, and not check the
   *                           bottom and right.
   *   @history 2007-08-14 Steven Koechle - Added virtual method Padding() which default returns 0.
   *   @history 2007-08-16 Steven Koechle - Fixed Looping error in Operate. Made the
   *                           loops <= instead of just <. Changed from accepting one
   *                           delta to accepting a deltaSamp and a deltaLine.
   *   @history 2008-06-18 Stuart Sides - Fixed doc error
   *   @history 2008-08-19 Steven Koechle - Updated to work with Geos3.0.0
   *   @history 2009-08-11 Travis Addair - Added functionality allowing it and all its
   *                           subclasses to return the pvl group that they were initialized from
   *   @history 2010-04-09 Sharmila Prasad - API's to check valid DN and Emission Angle.
   *                           Also changed functionality of Operate and made it overloaded.
   *   @history 2010-04-30 Sharmila Prasad - Added class members mdBestEmissionAngle,
   *                           mdBestDnValue and their access functions.Also added member
   *                           mUnusedParamGrp to check for the default values used for
   *                           the operator.
   *   @history 2010-04-30 Sharmila Prasad - (1) Interest Operator child of ControlNetValidMeasure
   *                           which validates all the standard control network options. Changed
   *                           functionality to accomadate ControlNetValidMeasure (2) Removed
   *                           class members  mdBestEmissionAngle, mdBestDnValue..., instead
   *                           stored in structure InterestResults structure (3) Move processing
   *                           ImageOverlaps from app to here. (4) Added API's to compute Interest
   *                           by point and by measure.
   *   @history 2010-06-18 Sharmila Prasad - (1) Fixed Bug to ignore Points with bad interest
   *                           (2) Do not process previously Ignored points in the Original
   *                           Control Net
   *   @history 2010-06-21 Sharmila Prasad - Remove references to UniversalGroundMap and Cubes
   *                           use CubeManager instead.
   *   @history 2010-06-23 Sharmila Prasad - Use ControlNetValidMeasure's Validate Standard
   *                           Options & Std Options Pixels/Meters from Edge
   *   @history 2010-07-13 Tracie Sucharski - Make changes to implement the new or modified
   *                           keywords for the implementation of binary control networks.
   *   @history 2010-10-05 Sharmila Prasad - Process EditLock feature
   *   @history 2010-10-15 Sharmila Prasad - Use only a single copy of Control Net
   *   @history 2010-10-22 Sharmila Prasad - Reset apriori for source==Reference
   *   @history 2010-10-26 Tracie Sucharski Added missing includes to cpp after
   *                           removing includes from ControlNet.h.
   *   @history 2010-10-28 Sharmila Prasad - Log, if Reference changed and apriorisource==Reference
   *   @history 2010-11-10 Sharmila Prasad - Modify unit test to accomodate changes in the deffile
   *   @history 2010-11-12 Sharmila Prasad - Move definition of structure InterestResults to private
   *   @history 2010-12-29 Sharmila Prasad - Modified for new ControlNet API's
   *                           (UpdatePoint, UpdateMeasure)
   *   @history 2011-02-24 Sharmila Prasad - Fixed segmentation fault
   *   @history 2011-05-14 Sharmila Prasad - Modified to accomodate changes to
   *                           ControlNetValidMeasure which added line, sample
   *                           residuals for Measure validation
   *   @history 2011-06-07 Debbie A. Cook and Tracie Sucharski - Modified point types
   *                           Ground ------> Fixed
   *                           Tie----------> Free
   *   @history 2011-11-21 Sharmila Prasad - (1) InterestOperator should run without ValidMeasure
   *                           Group. (2) The Validity test must be performed on Measure after
   *                           moving to new location. Fixes Mantis #584
   *   @history 2011-11-23 Sharmila Prasad - Process for control nets with Reference not explicitly set.
   *                           Fixes #589
   *   @history 2013-03-08 Kimberly Oyama and Steven Lambright - Added a try/catch to
   *                           InterestByPoint(). References #825.
   *   @history 2014-03-03 Janet Barrett - Changed the FindCnetRef method to report status on number
   *                           of points processed instead of number of measures. Fixes #2040.
   *
   */
  class InterestOperator : public ControlNetValidMeasure {
    public:
      InterestOperator(Pvl &pPvl);

      virtual ~InterestOperator();

      void InitInterestOptions();

      void SetPatternValidPercent(const double percent);
      void SetPatternSampling(const double percent);
      void SetSearchSampling(const double percent);
      void SetTolerance(double tolerance);
      void SetPatternReduction(std::vector<int> samples, std::vector<int> lines);

      //! Return name of the matching operator
      inline std::string operatorName() const {
        return mOperatorGrp["Name"];
      };

      //! Operate used by the app interestcube- to calculate interest by sample,line
      bool Operate(Cube &pCube, UniversalGroundMap &pUnivGrndMap, int piSample, int piLine);

      //! Operate - to calculate interest for entire control net to get better reference
      void Operate(ControlNet &pNewNet, QString psSerialNumFile, QString psOverlapListFile = "");

      //! Return the Interest Amount
      inline double InterestAmount() const {
        return p_interestAmount;
      };

      //! Return the Worst(least value) Interest
      inline double WorstInterest() const {
        return p_worstInterest;
      }

      //! Return the search chip cube sample that best matched
      inline double CubeSample() const {
        return p_cubeSample;
      };

      //! Return the search chip cube line that best matched
      inline double CubeLine() const {
        return p_cubeLine;
      };

      //! Compare for int1 greater than / equal to int2
      virtual bool CompareInterests(double int1, double int2);
      void addGroup(Isis::PvlObject &obj);  //???? check if used

      //! Set the Clip Polygon for points to be contained in the overlaps
      void SetClipPolygon(const geos::geom::MultiPolygon &clipPolygon);

      //! Return the Operator name
      Isis::PvlGroup Operator();

    protected:
      //! Parse the Interest specific keywords
      void Parse(Pvl &pPvl);

      //! Calculate the interest
      virtual double Interest(Chip &subCube) = 0;

      //! Find if a point is in the overlap
      const geos::geom::MultiPolygon *FindOverlap(Isis::ControlPoint &pCnetPoint);

      //! Find imageoverlaps by finding the intersection of image footprints
      const geos::geom::MultiPolygon *FindOverlapByImageFootPrint(Isis::ControlPoint &pCnetPoint);

      //! Find best ref for an entire control net by calculating the interest and
      //! moving point to a better interest area.
      void FindCnetRef(ControlNet &pNewNet);

      //! Process (Validate and Log) Point with Lock or with Referemce Measure Locked
      void ProcessLocked_Point_Reference(ControlPoint &pCPoint, PvlObject &pPvlObj, int &piMeasuresModified);

      //! Calculate interest for a Control Point
      int InterestByPoint(ControlPoint &pCnetPoint);

      //! Calculate interest for a measure by index
      bool InterestByMeasure(int piMeasure, Isis::ControlMeasure &pCnetMeasure, Isis::Cube &pCube);

      //! Init Interest Results structure
      void InitInterestResults(int piIndex);

      virtual int Padding();

      double p_worstInterest, p_interestAmount;

      //! Clipping polygon set by SetClipPolygon (line,samp)
      geos::geom::MultiPolygon *p_clipPolygon;

      Isis::PvlGroup mOperatorGrp;        //!< Operator group that created this projection

    private:
      double p_cubeSample, p_cubeLine;    //!< Point in a cube from a chip perspective
      double p_minimumInterest;           //!< Specified in the Pvl Operator group
      Isis::ImageOverlapSet mOverlaps;    //!< Holds the overlaps from the Overlaplist
      bool mbOverlaps;                    //!< If Overlaplist exists

      //! Specified in the Pvl Operator group for the box car size
      int p_deltaSamp, p_deltaLine, p_lines, p_samples;

      //! Structure to hold Interest Results
      typedef struct {
        QString msSerialNum;     //!< Serial Number of the Measure
        double mdInterest;           //!< Resulting interest amt from InterestOperator
        double mdBestSample;         //!< Most interesting sample
        double mdBestLine;           //!< Most interesting line
        double mdOrigSample;         //!< Control Measure's original sample
        double mdOrigLine;           //!< Control Measure's original line
        double mdEmission;           //!< Emission angle at most interesting sample,line
        double mdIncidence;          //!< Incidence angle at most interesting sample,line
        double mdDn;                 //!< Cube DN value at most interesting sample,line
        double mdResolution;         //!< Camera resolution at most interesting sample,line
        bool mbValid;                //!< Value of the interest operator result (success)
        int  miDeltaSample;          //!< The number of Samples the point has been moved
        int  miDeltaLine;            //!< The number of Lines the point has been moved
      } InterestResults;
      InterestResults *mtInterestResults;  //!< Holds the results of an interest computation
  };
};

#endif
