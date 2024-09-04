/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#ifndef Chip_h
#define Chip_h


#include "Affine.h"
#include "Interpolator.h"
#include "Pvl.h"
#include "SpecialPixel.h"

#include <vector>

#include <geos/geom/MultiPolygon.h>

namespace Isis {
  class Cube;
  class Statistics;

  /**
   * @brief A small chip of data used for pattern matching.
   *
   * A chip is a small rectangular area that can be used for pattern
   * matching. Data can be loaded into the chip manually or by reading
   * directly from a cube.
   *
   * @ingroup PatternMatching
   *
   * @author 2005-05-05 Jeff Anderson
   *
   * @see AutoReg
   * @see AutoRegFactory
   *
   * @internal
   *   @history 2006-07-11 Tracie Sucharski - Added reLoad method to use m_cube instead of cube
   *                           passed in.
   *   @history 2006-08-03 Tracie Sucharski - Added Load and ReLoad method to apply scale factor
   *                           to chip.
   *   @history 2006-08-04 Stuart Sides - Added SetClipPolygon method. If the clip polygon is set
   *                           all pixel values outside the polygon will be set to NULL.
   *   @history 2007-10-01 Steven Koechle - Fixed inc in LoadChip to fix an infinite loop problem
   *                           when x.size() never grew to be more than 3.
   *   @history 2009-01-19 Steven Koechle - Fixed memory leak
   *   @history 2009-06-02 Stacy Alley - Added a check in the SetSize() method to make sure the
   *                           given samples and lines are not equal to or less than zero.
   *   @history 2009-08-19 Kris Becker - Added new Extract method that applies an Affine transform
   *                           to the extract a portion of the chip; added an assigment operator
   *                           that sets the entire chip to a single value for convenience. Added a
   *                           getter that returns a const reference to the internal Affine
   *                           transform of this chip.
   *   @history 2009-08-20 Steven Lambright - Removed local cube pointer and parenthesis operator
   *   @history 2009-08-20 Travis Addair - Added Statistics method
   *   @history 2009-08-28 Kris Becker - Added new Affine setter method to establish a new Affine
   *                           transform to the chip; added another Load method that uses a new
   *                           Affine transform to load a chip from a cube.
   *   @history 2009-09-01 Travis Addair - Added valid Min/Max pixel value functionality for
   *                           Statistics method
   *   @history 2010-01-28 Tracie Sucharski - In the Load method (with match chip) when calculating
   *                           control points away from the corners, added a linc to move into the
   *                           center of the chip in a non-linear fashion to prevent control points
   *                           that fall in a line and cause the matrix inversion to fail.
   *   @history 2010-05-24 Jeannie Walldren - Fixed bug in the Load() method (with match chip).
   *                           Modified to look for control points from each corner, rather than
   *                           looping around. Added a method, PointsColinear() to check whether
   *                           the points added are almost along the same line. Moved the code from
   *                           Load() that chose new points to a new method, MovePoints().
   *   @history 2010-06-10 Jeannie Walldren - Modified PointsColinear() method to take in user
   *                           defined tolerance as parameter to allow registration of more narrow
   *                           search chip areas. Updated documentation, error messages, unitTest.
   *   @history 2010-06-15 Jeannie Walldren - Added set and accessor methods for Read() method's
   *                           Interpolator::interpType. Updated documentation and unitTest.
   *   @history 2010-09-16 Jeannie Walldren - Updated unitTest, truth file and test cube to run
   *                           properly with ShapeModel changes to Sensor class.
   *   @history 2011-03-29 Jai Rideout - Added copy constructor and equals operator.
   *   @history 2011-10-02 Kris Becker - Corrected a bug in the Extract(Chip, Affine) method when
   *                           computing output pixels coordinates.
   *   @history 2015-07-06 David Miller - Modified code to better reflect current Coding Standards.
   *                           Updated truth data. Fixes #2273
   *   @history 2017-08-30 Summer Stapleton - Updated documentation. References #4807.
   */
  class Chip {
    public:
      Chip();
      Chip(const Chip &other);
      Chip(const int samples, const int lines);
      virtual ~Chip();

      void SetSize(const int samples, const int lines);
      bool IsInsideChip(double sample, double line);

      /**
       * @returns The number of samples in the chip
       */
      inline int Samples() const {
        return m_chipSamples;
      };

      /**
       * @returns The number of lines in the chip
       */
      inline int Lines() const {
        return m_chipLines;
      };

      /**
       * @returns The expanded filename of the cube from which this chip was chipped.
       */
      inline QString FileName() const {
        return m_filename;
      };

      void SetAllValues(const double &d);

      /**
       * @brief Sets a value in the chip.
       *
       * @param sample  Sample position to load (1-based)
       * @param line    Line position to load (1-based)
       * @param value   Value to set
       */
      void SetValue(int sample, int line, const double &value) {
        m_buf[line-1][sample-1] = value;
      }

      /**
       * @brief Loads a Chip with a value.
       *
       * For example,
       * @code
       *   Chip c(10,5);
       *   c(1,1) = 1.1;
       *   c(10,5) = 1.2;
       * @endcode
       *
       * @param sample  Sample position to load (1-based)
       * @param line    Line position to load (1-based)
       * 
       * @returns (double) The value of the chip at the specified line/sample
       */
      inline double GetValue(int sample, int line) {
        return m_buf[line-1][sample-1];
      }

      /**
       * @brief Get a value from a Chip.
       *
       * For example,
       * @code
       *   Chip c(10,5);
       *   cout << c[3,3] << endl;
       * @endcode
       *
       * @param sample  Sample position to get (1-based)
       * @param line    Line position to get (1-based)
       * 
       * @returns (double) The value of the chip at the specified line/sample
       */
      inline double GetValue(int sample, int line) const {
        return m_buf[line-1][sample-1];
      }

      void TackCube(const double cubeSample, const double cubeLine);

      /**
       * This method returns a chip's fixed tack sample; the middle of the chip. It is a chip
       * coordinate, not a cube coordinate. For example, a chip with 5 samples will return 3,
       * the middle pixel. A chip with 4 samples will return 2.
       * 
       * @returns The fixed tack sample of the chip.
       */
      inline int TackSample() const {
        return m_tackSample;
      };

      /**
       * This method returns a chip's fixed tack line; the middle of the chip. It is a chip
       * coordinate, not a cube coordinate. For example, a chip with 5 lines will return 3,
       * the middle pixel. A chip with 4 lines will return 2.
       * 
       * @returns The fixed tack line of the chip.
       */
      inline int TackLine() const {
        return m_tackLine;
      };

      void Load(Cube &cube, const double rotation = 0.0, const double scale = 1.0,
                const int band = 1);
      void Load(Cube &cube, Chip &match, Cube &matchChipCube,
                const double scale = 1.0, const int band = 1);
      void Load(Cube &cube, const Affine &affine, const bool &keepPoly = true,
                const int band = 1);

      void SetChipPosition(const double sample, const double line);

      /**
       * @returns The cube sample after invoking SetChipPosition
       */
      inline double CubeSample() const {
        return m_cubeSample;
      };

      /**
       * @returns The cube line after invoking SetChipPosition
       */
      inline double CubeLine() const {
        return m_cubeLine;
      };

      void SetCubePosition(const double sample, const double line);

      /**
       * @returns The chip sample after invoking SetCubePosition
       */
      double ChipSample() const {
        return m_chipSample;
      };

      /**
       * @returns The chip line after invoking SetCubePosition
       */
      double ChipLine() const {
        return m_chipLine;
      };

      void SetValidRange(const double minimum = Isis::ValidMinimum,
                         const double maximum = Isis::ValidMaximum);

      /**
       * @param sample  Sample position
       * @param line    Line position
       * 
       * @returns (bool) Whether the value at the given sample,
       * line position is within the valid range
       */
      inline bool IsValid(int sample, int line) {
        double value = GetValue(sample, line);
        if (value < m_validMinimum) return false;
        if (value > m_validMaximum) return false;
        return true;
      }

      /**
       * Return if total number of valid pixels in chip meets a specified percentage of the entire chip.
       *
       * @param percentage The percentage that the valid pixels percentage must exceed
       *
       * @return bool Returns true if the percentage of valid pixels is greater than the
       *              specified percentage, and false if it is not
       */
      bool IsValid(double percentage);
      Chip Extract(int samples, int lines, int samp, int line);
      void Extract(int samp, int line, Chip &output);
      Isis::Statistics *Statistics();
      void Extract(Chip &output, Affine &affine);
      void Write(const QString &filename);

      void SetClipPolygon(const geos::geom::MultiPolygon &clipPolygon);

      Chip &operator=(const Chip &other);

      /**
       * @brief Returns the Affine transformation of chip-to-cube indices
       *
       * This method returns the affine transform used to load a chip from the same area
       * as a match cube. It also is used to track the tack point line and sample
       * translations from the chip indices to the absolute cube coordiates.
       *
       * @return @b const @b Affine& Transform map from chip coordinates to cube coordinates
       */
      const Affine &GetTransform() const {
        return (m_affine);
      }

      /**
       * @brief Sets the internal Affine transform to new translation.
       *
       * Provides the ability to establish a new affine transformation without overhead of, say,
       * loading the chip with a new translation. The caller also has the option to specify the
       * disposition of an established polygon.
       *
       * @param affine    New affine tranform to set for this chip
       * @param keepPoly  Indicates whether an existing polygon clipper should be kept
       *                 (default of true)
       */
      void SetTransform(const Affine &affine, const bool &keepPoly = true) {
        m_affine = affine;
        if (!keepPoly) {
          delete m_clipPolygon;
          m_clipPolygon = 0;
        }
        return;
      }

      /**
       * @brief Access method that returns the Interpolator Type used for loading a chip.
       * @return @b const @b Interpolator::interpType Interpolator used to read data from cube and
       *             put it into a chip.
       * @see Read()
       * @see SetReadInterpolator()
       * @author Jeannie Walldren
       * @internal
       *   @history 2010-06-05 Jeannie Walldren - Original version
       */
      Interpolator::interpType GetReadInterpolator() {
        return m_readInterpolator;
      }

      /**
       * @brief Sets Interpolator Type for loading a chip. This type is used in the Read() method.
       * @param type Interpolator type to be used.
       * @throws IException::Programmer - Invalid Interpolator Type
       * @see Read()
       * @see SetReadInterpolator()
       * @author Jeannie Walldren
       * @internal
       *   @history 2010-06-05 Jeannie Walldren - Original version
       */
      void SetReadInterpolator(const Interpolator::interpType type) {
        if (type == Interpolator::NearestNeighborType ||
            type == Interpolator::BiLinearType ||
            type == Interpolator::CubicConvolutionType) {
          m_readInterpolator = type;
          return;
        }
        // Interpolator::None is not valid type
        std::string msg = "Invalid Interpolator type. Cannot use [";
        msg += toString(type) + "] to read cube into chip.";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

    private:
      void Init(const int samples, const int lines);
      void Read(Cube &cube, const int band);
      std::vector<int> MovePoints(int startSamp, int startLine,
                             int endSamp, int endLine);
      bool PointsColinear(double x0, double y0,
                          double x1, double y1,
                          double x2, double y2,
                          double tol);

      int m_chipSamples;                           //!< Number of samples in the chip
      int m_chipLines;                             //!< Number of lines in the chip
      std::vector< std::vector<double> > m_buf;    //!< Chip buffer
      int m_tackSample;                            //!< Middle sample of the chip
      int m_tackLine;                              //!< Middle line of the chip

      double m_cubeTackSample;                     //!< cube sample at the chip tack
      double m_cubeTackLine;                       //!< cube line at the chip tack

      double m_validMinimum;                       //!< valid minimum chip pixel value
      double m_validMaximum;                       //!< valid maximum chip pixel value

      double m_chipSample;                         //!< chip sample set by SetChip/CubePosition
      double m_chipLine;                           //!< chip line set by SetChip/CubePosition
      double m_cubeSample;                         //!< cube sample set by SetCubePosition
      double m_cubeLine;                           //!< cube line set by SetCubePosition

      geos::geom::MultiPolygon *m_clipPolygon;     //!< clipping polygon set by SetClipPolygon
                                                   // (line,samp)

      Affine m_affine;                             //!< Transform set by SetTransform.
                                                   // Used to load cubes into chip

      Interpolator::interpType m_readInterpolator; //!< Interpolator type set by
                                                   // SetReadInterpolator. Used to read
                                                   // cubes into chip.

      QString m_filename;                          //!< FileName of loaded cube
  };
};

#endif
