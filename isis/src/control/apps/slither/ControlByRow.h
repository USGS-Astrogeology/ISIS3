#ifndef ControlByRow_h
#define ControlByRow_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <vector>
#include "ControlMeasure.h"
#include "ControlMeasureLogData.h"
#include "Statistics.h"
#include "CollectorMap.h"
#include "IString.h"
#include <gsl/gsl_math.h>

namespace Isis {

  /**
   * @brief Container for point collection
   */
  struct PointData {
    ControlMeasure refPoint;
    ControlMeasure chpPoint;
  };

  /**
   * @brief Less than test for Control point group
   *
   * This function tests the reference line numbers and returns true if the first
   * point reference line is less than the second.
   *
   * @param p1  First PointData set to compare
   * @param p2  Second PointData set to compare
   *
   * @return bool If first point reference line is less than the second
   */
  inline bool PointLess(const PointData &p1, const PointData &p2) {
    return (p1.refPoint.GetLine() < p2.refPoint.GetLine());
  }

  /**
   * @brief Equality test for Control point group
   *
   * This function tests the reference line numbers for equality and returns true
   * if the line references are equivalent, according to an approximation using an
   * epsilon of 1.0e-6.
   *
   * @param p1  First PointData set to compare
   * @param p2  Second PointData set to compare
   *
   * @return bool If the reference point lines are (approximately) equivalent
   */
  inline bool PointEqual(const PointData &p1, const PointData &p2) {
    return (gsl_fcmp(p1.refPoint.GetLine(), p2.refPoint.GetLine(), 1.0E-6) == 0);
  }

  /**
   * @brief Collector for Control points within the same row for analysis
   *
   * This class is designed to be used as a Functor object collecting control net
   * file and collapsing all column measures into on row.  This is primarily used
   * for analysis of coregistration results with one or more columns specified
   * in the search/pattern chip strategy.
   *
   * @author ????-??-?? Unknown
   *
   * @internal
   */
  class ControlByRow {
    public:
      /**
       * @brief Structure to return control point statistics for row
       *
       * This structure contains the row statistics of merged control points.
       * This will eventually be used to compute the spline interpolations for
       * line and sample offsets.
       *
       */
      struct RowPoint {
        RowPoint() : refLine(0.0), refSamp(0.0), chpLine(0.0), chpSamp(0.0),
          total(0), count(0) { }
        double refLine;    //!<  Reference line (row)
        double refSamp;    //!<  Reference sample
        double chpLine;    //!<  Registered line
        double chpSamp;    //!<  Registered sample
        int total;         //!<  Total points in row
        int count;         //!<  Valid points found

        Statistics rSStats;
        Statistics cLStats;
        Statistics cSStats;
        Statistics cLOffset;
        Statistics cSOffset;
        Statistics GOFStats;
      };

    public:
      /**
       * @brief Default constructor
       */
      ControlByRow()  {
        _minGOF = DBL_MIN;
        _maxGOF = DBL_MAX;
      }

      /**
       * @brief Constructor that sets the maximum goodness of fit tolerance
       * @param maxGOF Value that specifies the maximum goodness of fit which is
       *               typically never expected to exceed 1.0 for a good fit.
       */
      ControlByRow(double maxGOF)  {
        _minGOF = DBL_MIN;
        _maxGOF = maxGOF;
      }

      /**
       * @brief Constructor that sets the maximum goodness of fit tolerance
       * @param minGOF Value of minimum goodness of fit.  Allows user selectable
       *               adjustment to coregistration minimum tolerance
       * @param maxGOF Value that specifies the maximum goodness of fit which is
       *               typically never expected to exceed 1.0 for a good fit.
       */
      ControlByRow(double minGOF, double maxGOF)  {
        _minGOF = minGOF;
        _maxGOF = maxGOF;
      }

      /**
       * @brief Destructor
       */
      ~ControlByRow() { }

      /**
       * @brief Determines the number of points (rows) found valid
       *
       * The number returned is really the number of unique rows of coregistration
       * chips gleened from the control net.
       *
       * @return unsigned int Row/line count
       */
      inline unsigned int size() const {
        return (_rowList.size());
      }

      /**
       * @brief Set the minimum acceptable goodness of fit value
       *
       * This sets the minimum (absolute) value used to gleen valid points from
       * the control net data.
       *
       * @param minGOF Minimum goodness of fit tolerance
       */
      void setMinGOF(double minGOF) {
        _minGOF = minGOF;
      }

      /**
       * @brief Set the maximum acceptable goodness of fit value
       *
       * This sets the maximum (absolute) value used to gleen valid points from
       * the control net data.  This is intended to use to exclude wild points
       * that exceed the level of reasonable tolerance.  This is typically 1.0 for
       * most coregistration algorithms.
       *
       * @param maxGOF Maximum goodness of fit tolerance
       */
      void setMaxGOF(double maxGOF) {
        _maxGOF = maxGOF;
      }

      /**
       * @brief Operator used to add a point to the data set
       *
       * This method provides support for the STL for_each algorithm.  This allows
       * this class to be used as a functor object.
       *
       * @param p Point to tested for acceptance in the list
       */
      void operator()(const PointData &p) {
        addPoint(p);
        return;
      }

      /**
       * @brief Formal method of adding a control point to the data set
       *
       * This method will add the provided point to the collection of rows (or
       * lines of points).
       *
       * @param p  Point to add to the list
       */
      void addPoint(const PointData &p) {
        if(_rowList.exists(p.refPoint.GetLine())) {
          PointList &r = _rowList.get(p.refPoint.GetLine());
          r.push_back(p);
        }
        else {
          PointList pl;
          pl.push_back(p);
          _rowList.add(p.refPoint.GetLine(), pl);
        }
        return;
      }

      /**
       * @brief Returns a point at the ith location
       *
       * Traverses the list of points after computing the merge statistics for the
       * ith row.
       *
       * @param i Index of point to return
       *
       * @return RowPoint  Structure containing the (statistically) merged row
       */
      RowPoint operator[](int i) const {
        try {
          return (computeStats(_rowList.getNth(i)));
        }
        catch(IException &oor) {
          std::string msg = "Requested value (" + toString(i) +  ") not found";
          throw IException(oor, IException::User, msg, _FILEINFO_);
        }
      }

    private:
      typedef std::vector<PointData> PointList;    //!< Composite list
      typedef CollectorMap<double, PointList, RobustFloatCompare> CNetRow;  /**
                                  * Nifty templated collector class works just
                                  * nicely for merging rows of correlations
                                                                         */
      double            _minGOF;  //!<  Minimum acceptable goodness of fit
      double            _maxGOF;  //!<  Maximum acceptable goodness of fit
      CNetRow           _rowList; //!<  Collection of merged rows/lines

      /**
       * @brief Convenience method for adding point to Statistics class
       *
       * The interface to the Isis Statistics class requires the value to be added
       * by address.  This method expedites the addition of values from say a
       * function or method.
       *
       * @param value  Value to add to Statistics class
       * @param stats  Statistics class to add the data point to
       */
      inline void addToStats(double value, Statistics &stats) const {
        stats.AddData(&value, 1);
        return;
      }

      /**
       * @brief  All important method that computes statistics for a row
       *
       * This method computes the statistics for a potentially merged row of
       * coregistration chips.  It applies the minimum and maximum goodness of fit
       * tolerance checks and add valid points to each statistical component of
       * the merge.
       *
       * @param cols  List of column chip registrations
       *
       * @return RowPoint Structure containing the statistics for row/line of
       *         merged registration columns
       */
      RowPoint computeStats(const std::vector<PointData> &cols) const {
        RowPoint rp;
        rp.refLine = cols[0].refPoint.GetLine();
        for(unsigned int i = 0; i < cols.size() ; i++) {
          double regGOF =
            cols[i].chpPoint.GetLogData(ControlMeasureLogData::GoodnessOfFit)
              .GetNumericalValue();
          if(fabs(regGOF) > _maxGOF) continue;
          if(fabs(regGOF) < _minGOF) continue;
          (rp.count)++;
          addToStats(cols[i].refPoint.GetSample(), rp.rSStats);
          addToStats(cols[i].chpPoint.GetLine(), rp.cLStats);
          addToStats(cols[i].chpPoint.GetSample(), rp.cSStats);
          addToStats(cols[i].chpPoint.GetLineResidual(), rp.cLOffset);
          addToStats(cols[i].chpPoint.GetSampleResidual(), rp.cSOffset);
          addToStats(regGOF, rp.GOFStats);
        }

        rp.total = cols.size();
        rp.refSamp = rp.rSStats.Average();
        rp.chpLine = rp.cLStats.Average();
        rp.chpSamp = rp.cSStats.Average();
        return (rp);
      }

  };


}  // namespace Isis

#endif
