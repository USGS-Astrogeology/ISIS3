#ifndef GruenTypes_h
#define GruenTypes_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <tnt/tnt_array1d.h>
#include <tnt/tnt_array2d.h>
#include <tnt/tnt_array2d_utils.h>

#include "Affine.h"
#include "Chip.h"
#include "Constants.h"
#include "IException.h"
#include "PvlKeyword.h"
#include "SpecialPixel.h"


namespace Isis {

 typedef Affine::AMatrix      GMatrix;
 typedef TNT::Array1D<double> GVector;

  // Constraints
  enum { NCONSTR = 8 };

  /**
   * @brief Define a generic Y/X container
   *
   * This generic container is designed to be used as a line/sample or a
   * latitude/longitude container.  It can be used to contain other cartesian
   * coordinates if desired.
   *
   * The default initialization sets the points the ISIS Null pixel value
   * indicating it has not been initialized or can signal an invalid point if
   * either one of the values is not initialized to something other than an
   * ISIS special pixel.
   *
   * Operators are defined to ease performing simple add/subtract operations.
   *
   * @author ????-??-?? Unknown
   *
   * @internal
   */
  class Coordinate {
    public:
      Coordinate() : m_y(Null), m_x(Null) { }
      Coordinate(double y, double x) : m_y(y), m_x(x) { }
      Coordinate(const Chip &chip) : m_y(chip.CubeLine()),
                                     m_x(chip.CubeSample()){ }
      ~Coordinate() { }

      /** Use Latitude/Longitude interface */
      void setLatLon(const double &latitude, const double &longitude) {
        m_y = latitude;
        m_x = longitude;
        return;
      }

      /** Use the Line/Sample interface */
      void setLineSamp(const double &line, const double &sample) {
        m_y = line;
        m_x = sample;
        return;
      }

      /** Add a point to this point */
      Coordinate &operator+=(const Coordinate &other) {
        if ( isValid() && other.isValid()) {
          m_y += other.m_y;
          m_x += other.m_x;
        }
        else {
          m_y = m_x = Null;
        }
        return (*this);
      }

      /** Subtract a point from this point */
      Coordinate &operator-=(const Coordinate &other) {
        if ( isValid() && other.isValid() ) {
          m_y -= other.m_y;
          m_x -= other.m_x;
        }
        else {
          m_y = m_x = Null;
        }
        return (*this);
      }

      /** Computes the distance from this point and the point provided */
      double getDistance(const Coordinate &pntA = Coordinate(0.0, 0.0)) const {
        double yd = pntA.m_y - m_y;
        double xd = pntA.m_x - m_x;
        return (std::sqrt((xd * xd) + (yd * yd)));
      }

      /** Check for goodness */
      inline bool isValid() const {
        return ( !(IsSpecial(m_x) || IsSpecial(m_y)));
      }

      inline double getLatitude() const { return (m_y); }
      inline double getLongitude() const { return (m_x); }
      inline double getLine() const { return (m_y); }
      inline double getSample() const { return (m_x); }

      double m_y;  // Overloaded as Latitude or line
      double m_x;  // Overloaded as Longitude or sample
  };


  /**
   * @brief Summation operator for Coordinate
   *
   * @param A First operand
   * @param B Second operand
   *
   * @return Coordinate  Returns the sum of the two coordinates if they are both
   *         valid otherwise returns invalid point
   */
  inline Coordinate operator+(const Coordinate &A, const Coordinate &B) {
    if ( A.isValid() && B.isValid() ) {
      return (Coordinate(A.m_y+B.m_y, A.m_x+B.m_x));
    }
    else {
      return (Coordinate());
    }
  }

  /**
   * @brief Subtraction operator for Coordinate
   *
   * @param A First operand
   * @param B Second operand
   *
   * @return Coordinate Returns the difference between the two coordinates if
   *         they are both valid otherwise returns invalid point
   */
  inline Coordinate operator-(const Coordinate &A, const Coordinate &B) {
    if ( A.isValid() && B.isValid() ) {
      return (Coordinate(A.m_y-B.m_y, A.m_x-B.m_x));
    }
    else {
      return (Coordinate());
    }
  }

  /**
   * @brief Define a point set of left, right and geometry at that location
   *
   * The structure defines a Gruen point set that may or may not contain a valid
   * geometry.  This supports the (efficient) growing feature of SMTK in that a
   * grown point will have valid left and right points, but not neccesarily
   * geomertry.  Valid geometry requires using camera models and that is costly.
   *
   * Default initialize sets all points to an invalid state.
   *
   * @author ????-??-?? Unknown
   *
   * @internal
   */
  class PointPair {
    public:
      PointPair() : m_left(), m_right() { }
      PointPair(const double &line, const double &sample) : m_left(line, sample),
                                                            m_right() { }
      PointPair(const Coordinate &left,
               const Coordinate &right = Coordinate()) : m_left(left),
                                                         m_right(right) { }

      /** Left, right and geometry coordinates must all be good data */
      inline bool isValid() const {
        return (m_left.isValid() && m_right.isValid());
      }
      inline const Coordinate &getLeft()  const { return (m_left);  }
      inline const Coordinate &getRight() const { return (m_right); }

      inline double getLine() const { return (getLeftLine()); }
      inline double getSample() const { return (getLeftSample()); }
      inline double getLeftLine() const { return (m_left.getLine()); }
      inline double getLeftSample() const { return (m_left.getSample()); }
      inline double getRightLine() const { return (m_right.getLine()); }
      inline double getRightSample() const { return (m_right.getSample()); }

      Coordinate m_left;    // Left image line/sample
      Coordinate m_right;   // Right image line/sample
  };


  /**
   * @brief Store for radiometric gain and shift parameters
   *
   * @author ????-??-?? Unknown
   *
   * @internal
   */
  class Radiometric {
    public:
      Radiometric() : m_shift(0.0), m_gain(0.0) { }
      Radiometric(const double &shift, const double &gain) :
                       m_shift(shift), m_gain(gain) { }

      inline double Shift() const { return (m_shift);  }
      inline double Gain()  const { return (m_gain);   }


      /** Add radiometric parameters from another set of parameters */
      Radiometric &operator+=(const Radiometric &B) {
        m_shift += B.m_shift;
        m_gain += B.m_gain;
        return (*this);
      }

      double m_shift;   // Radiometric shift
      double m_gain;    // Radiometric gain
  };

  /** Operator to sum two radiometric parameters */
  inline Radiometric operator+(const Radiometric &A, const Radiometric &B) {
    return (Radiometric(A.m_shift+B.m_shift, A.m_gain+B.m_gain));
  }


  /**
   * @brief Container for affine and radiometric parameters
   *
   * These parameters are provided for input and output results.
   *
   * @author 2011-04-18 Kris Becker
   *
   * @internal
   */
  class AffineRadio {
    public:
      AffineRadio() : m_affine(Affine::getIdentity()), m_radio() {}
      AffineRadio(const GMatrix &A) : m_affine(A), m_radio() { }
      AffineRadio(const GMatrix &M, const double &shift,
                  const double &gain) : m_affine(M), m_radio(shift, gain) { }
      AffineRadio(const GVector &alpha) : m_affine(), m_radio() {
        clone(alpha);  // Clone from an alpha matrix
      }
      AffineRadio(const Radiometric &radio) : m_affine(Affine::getIdentity()),
                                               m_radio(radio) { }
      ~AffineRadio() { }

      /** Define update procedure for accumulating Gruen iterations */
      AffineRadio &operator+=(const AffineRadio &other) {
        m_affine = m_affine + (other.m_affine - Affine::getIdentity());
        m_radio += other.m_radio;
        return (*this);
      }

      /** Apply a translation to the given offset */
      void Translate(const Coordinate &offset) {
        GMatrix trans = Affine::getIdentity();
        trans[0][2] = offset.getSample();
        trans[1][2] = offset.getLine();
        m_affine = TNT::matmult(trans, m_affine);
        return;
      }

      /** Applies the affine transfrom to a point and returns result */
      Coordinate getPoint(const Coordinate &location) const {
        double x = m_affine[0][0] * location.getSample() +
                   m_affine[0][1] * location.getLine()   + m_affine[0][2];
        double y = m_affine[1][0] * location.getSample() +
                   m_affine[1][1] * location.getLine()   + m_affine[1][2];
        return (Coordinate(y, x));
      }

      GMatrix     m_affine; // Affine transform
      Radiometric m_radio;  // Radiometric gain and shift

    private:
      /** Generate a matrix from the Gruen alpha vector */
      void clone(const GVector &alpha) {
        if ( alpha.dim1() != 8 ) {
          std::string mess = "Alpha array for AffineRadio must have 8 elements "
                             " but has " + toString(alpha.dim1());
          throw IException(IException::Programmer, mess, _FILEINFO_);
        }
        m_affine = Affine::getIdentity();
        m_affine[0][0] += alpha[1];
        m_affine[0][1] += alpha[2];
        m_affine[0][2] += alpha[0];

        m_affine[1][0] += alpha[4];
        m_affine[1][1] += alpha[5];
        m_affine[1][2] += alpha[3];

        m_radio.m_shift = alpha[6];
        m_radio.m_gain  = alpha[7];
      }
  };


  /**
   * @brief Container for Affine limits parameters
   *
   * These parameters govern the convergence of the Gruen affine processing.
   * These are used in conjunction with a Chip (size) to determine the actual
   * convergence values.
   *
   * @see Threshold
   *
   * @author 2011-05-18 Kris Becker
   *
   * @internal
   */
  struct AffineTolerance {
    public:
      AffineTolerance() : m_transTol(0.1), m_scaleTol(0.5), m_shearTol(0.5) { }
      AffineTolerance(const double &transTol, const double &scaleTol,
                      const double &shearTol) : m_transTol(transTol),
                                          m_scaleTol(scaleTol),
                                          m_shearTol(shearTol) { }
      ~AffineTolerance() { }

      double       m_transTol;          //  Affine translation tolerance
      double       m_scaleTol;          //  Affine scale tolerance
      double       m_shearTol;          //  Affine shear tolerance
  };


  /**
   * @brief Compute/test the Affine convergence from given parameters/chip
   *
   * This method should be invoked using either the subsearch or pattern chip
   * since they are both the same size.  The six Affine convergence parameters
   * are computed from the size of the chip and the AffineTranslationTolerance
   * (offset), AffineShearTolerance (cos/sin shearing) and AffineScaleTolerance
   * (x/y scaling) registration parameters. These parameters typically come from
   * the PVL setup and the Gruen object.
   *
   * @author ????-??-?? Unknown
   *
   * @internal
   */
  class Threshold {
    public:
      Threshold() : m_thresh(6, 0.0) { }
      Threshold(const Chip &chip, const AffineTolerance &tolerance) : m_thresh(6) {
        m_thresh[0] = tolerance.m_scaleTol / (((double)(chip.Samples() - 1)) / 2.0);
        m_thresh[1] = tolerance.m_shearTol / (((double)(chip.Lines() - 1)) / 2.0);
        m_thresh[2] = tolerance.m_transTol;

        m_thresh[3] = tolerance.m_shearTol / (((double)(chip.Samples() - 1)) / 2.0);
        m_thresh[4] = tolerance.m_scaleTol / (((double)(chip.Lines() - 1)) / 2.0);
        m_thresh[5] = tolerance.m_transTol;
      }
      ~Threshold() { }

      /** Determines convergence from an affine/radiometric fit */
      bool hasConverged(const AffineRadio &affine) const {
        GMatrix Malpha = affine.m_affine - Affine::getIdentity();
        const double *alpha = Malpha[0];
        for ( int i = 0 ; i < m_thresh.dim1() ; i++ ) {
          if ( std::fabs(alpha[i]) >= m_thresh[i] ) return (false);
        }
        return (true);  // If all values are below threshold
      }

      GVector m_thresh;
  };


  /**
   * @brief Error analysis of Gruen match point solution
   *
   * @author 2011-04-18 Kris Becker
   *
   * @internal
   */
  struct Analysis {
    Analysis() : m_npts(0), m_variance(0.0), m_sevals(),
                 m_kmat(), m_status(-1) {
      for ( int i = 0 ; i < 2 ; i++ ) {
        m_sevals[i] = 999.0;
        m_kmat[i] = 999.0;
      }
    }
    ~Analysis() { }

    inline bool isValid() const { return (m_status == 0); }

    /** Returns the square of the of sum of the squares of eigenvalues */
    inline double getEigen() const {
      double eigen = std::sqrt(m_sevals[0] * m_sevals[0] +
                               m_sevals[1] * m_sevals[1]);
      return (eigen);
    }

    /** Resets eigenvalues to 0 */
    inline void setZeroState() {
      for ( int i = 0 ; i < 2 ; i++ ) {
        m_sevals[i] = 0.0;
        m_kmat[i] = 0.0;
      }
      m_status = 0;
      return;
    }

    BigInt m_npts;
    double m_variance;
    double m_sevals[2];     //  2 sorted  eigenvalues
    double m_kmat[2];       //  Sample/Line uncertainty
    int    m_status;        //  Status
  };


  /**
   * @brief Structure containing comprehensive registration info/results
   *
   * This structure is used to contain all the parameters from a Gruen
   * registration process.  It contains status of the match as well as point
   * analysis, error analysis and affine/radiometric paramters.  The offset
   * of the registration can be obtained through a call to getAffinePoint()
   * using the default point coordinate of (0,0).
   *
   * @author 2011-05-18 Kris Becker
   *
   * @internal
   */
  class MatchPoint {
    public:
      MatchPoint() : m_point(), m_affine(), m_analysis(), m_status(-1) { }
      MatchPoint(const AffineRadio &radio) : m_point(), m_affine(radio),
                                             m_analysis(), m_status(-1) { }
      MatchPoint(const PointPair &point) : m_point(point), m_affine(),
                                           m_analysis(), m_status(-1) { }
      ~MatchPoint() { }

      inline int getStatus() const { return (m_status);  }
      const MatchPoint &setStatus(int status) {
        m_status = status;
        return (*this);
      }

      inline bool isValid() const {  return (m_status == 0); }
      inline double getEigen() const { return (m_analysis.getEigen()); }

      /** Return registration offset of a given chip coordinate from center  */
      Coordinate getAffinePoint(const Coordinate &coord = Coordinate(0.0, 0.0))
                                const {
        return (m_affine.getPoint(coord));
      }

      PointPair   m_point;       // Pattern (left) and search (right) points
      AffineRadio m_affine;      // Resulting Affine transform
      Analysis    m_analysis;    // Error analysis of registration
      int         m_nIters;      // Number of iterations required to match
      int         m_status;      // Status - good is 0
  };

} // namespace Isis
#endif
