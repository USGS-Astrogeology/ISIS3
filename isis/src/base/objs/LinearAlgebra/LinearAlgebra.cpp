/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "LinearAlgebra.h"

// std library
#include <cmath>

// Qt Library
#include <QDebug>
#include <QtDebug>
#include <QtGlobal>

// boost library
#include <boost/assign/std/vector.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>

// Armadillo
#include <armadillo>

// other Isis library
#include "Angle.h"
#include "Constants.h"
#include "IException.h"
#include "IString.h"


namespace Isis {
  /**
   * Default constructor for a LinearAlgebra object. This is protected so it can
   * never be constructed. The static methods and typedefs in this class are
   * meant to be used without a LinearAlgebra object.
   */
  LinearAlgebra::LinearAlgebra() {
  }


  /**
   * Destructor for a LinearAlgebra object.
   */
  LinearAlgebra::~LinearAlgebra() {
  }


  /**
   * Determines whether the given matrix is the identity. Non-square matrices
   * always return false. A qfuzzycompare is used when checking the zeroes and
   * ones.
   *
   * @param matrix The matrix to check. Automatically returns false if not
   *               square.
   *
   * @return @b bool Returns true if the matrix is a square identity matrix
   *                 (i.e. there are ones down the diagonal and zeroes elsewhere).
   */
  bool LinearAlgebra::isIdentity(const Matrix &matrix) {
    if (matrix.size1() != matrix.size2()) return false;

    for (unsigned int row = 0; row < matrix.size1(); row++) {
      for (unsigned int col = 0; col < matrix.size2(); col++) {
        if (row == col) {
          if (!qFuzzyCompare(matrix(row, col), 1.0)) return false;
        }
        else {
          // When using qFuzzy compare against a number that is zero
          // you must offset the number per Qt documentation
          if (!qFuzzyCompare(1.0 + matrix(row, col), 1.0)) return false;
        }
      }
    }
    return true;
  }


  /**
   * Determines whether the given matrix is orthogonal by verifying
   * that the matrix and its tranpose are inverses.
   *
   * @param matrix The matrix to check. Automatically returns false
   *               if not square.
   *
   * @return @b bool Returns true if the matrix is orthogonal.
   */
  bool LinearAlgebra::isOrthogonal(const Matrix &matrix) {
    if (matrix.size1() != matrix.size2()) return false;
    LinearAlgebra::Matrix test = multiply(matrix, LinearAlgebra::transpose(matrix));
    if (isIdentity(test)) {
      // no need to test that transpose is both left and right inverse since the matrix is square
      return true;
    }
    return false;
  }


  /**
   * Determines whether the given matrix is a rotation matrix.
   *
   * @param matrix The matrix to check. Automatically returns false if not 2x2
   *               or 3x3.
   *
   * @return @b bool Returns true if the given matrix represents a rotation.
   *
   * @throw IException::Programmer "Unable to determine whether
   *                                the given matrix is a rotation matrix."
   *
   */
  // derived from naif's isrot routine
  bool LinearAlgebra::isRotationMatrix(const Matrix &matrix) {
    // rotation matrices must be square
    if (matrix.size1() != matrix.size2()) return false;

    /* An exerpt from NAIF's isrot() routine...
     *
     * A property of rotation matrices is that their columns form a
     * right-handed, orthonormal basis in 3-dimensional space.  The
     * converse is true:  all 3x3 matrices with this property are
     * rotation matrices.
     *
     * An ordered set of three vectors V1, V2, V3 forms a right-handed,
     * orthonormal basis if and only if
     *
     *    1)   || V1 ||  =  || V2 ||  =  || V3 ||  =  1
     *
     *    2)   V3 = V1 x V2.  Since V1, V2, and V3 are unit vectors,
     *         we also have
     *
     *         < V3, V1 x V2 > = 1.
     *
     *         This quantity is the determinant of the matrix whose
     *         colums are V1, V2 and V3.
     *
     * When finite precision numbers are used, rotation matrices will
     * usually fail to satisfy these criteria exactly.  We must use
     * criteria that indicate approximate conformance to the criteria
     * listed above.  We choose
     *
     *    1)   |   || Vi ||  -  1   |   <   NTOL,  i = 1, 2, 3.
     *                                  -
     *
     *    2)   Let
     *
     *                   Vi
     *            Ui = ------ ,   i = 1, 2, 3.
     *                 ||Vi||
     *
     *         Then we require
     *
     *            | < U3, U1 x U2 > - 1 |  <  DTOL;
     *                                     -
     *
     *         equivalently, letting U be the matrix whose columns
     *         are U1, U2, and U3, we insist on
     *
     *            | det(U) - 1 |  <  DTOL.
     *                            _
     * The columns of M must resemble unit vectors.  If the norms are
     * outside of the allowed range, M is not a rotation matrix.
     *
     * Also, the columns of M are required to be pretty nearly orthogonal.  The
     * discrepancy is gauged by taking the determinant of the matrix UNIT,
     * computed below, whose columns are the unitized columns of M.
     */

    // create a matrix whose columns are unit vectors of the columns of the
    // given matrix
    Matrix unitMatrix(matrix.size1(), matrix.size2());

    for (unsigned int i = 0; i < unitMatrix.size1(); i++) {
      // get the ith column vector from the given matrix
      Vector columnVector = column(matrix, i);
      // get the magnitude and if it is not near 1, this is not a rotation matrix
      double columnMagnitude = magnitude(columnVector);
      if ( columnMagnitude < 0.9 || columnMagnitude >  1.1 ) {
        return false;
      }

      // put the unitized row into the unitized matrix
      setColumn(unitMatrix, columnVector / columnMagnitude, i);

    }

    try {
      // get the determinant of the unitized matrix and if it is not near 1,
      // this is not a rotation matrix
      double det = determinant(unitMatrix);
      if ( det >= 0.9 && det <=  1.1 ) {
        return true;
      }
    }
    catch (IException &e) {
      // since we can only calculate the determinant for 2x2 or 3x3
      std::string msg = "Unable to determine whether the given matrix is a rotation matrix.";
      throw IException(e, IException::Programmer, msg, _FILEINFO_);
    }
    return false;
  }


  /**
   * Determines whether the given matrix is filled with zereos.
   *
   * @param vector The matrix to check.
   *
   * @return @b bool Returns true if the matrix is all zereos.
   */
  bool LinearAlgebra::isZero(const Matrix &matrix) {
    for (unsigned int i = 0; i < matrix.size1(); i++) {
      for (unsigned int j = 0; j < matrix.size2(); j++) {
        // When using qFuzzy compare against a number that is zero
        // you must offset the number per Qt documentation
        if (!qFuzzyCompare(matrix(i, j) + 1.0, 1.0)) return false;
      }
    }

    return true;
  }


  /**
   * Determines whether the given vector is filled with zereos.
   *
   * @param vector The vector to check.
   *
   * @return @b bool Returns true if the vector is all zereos.
   */
  bool LinearAlgebra::isZero(const Vector &vector) {
    for (unsigned int i = 0; i < vector.size(); i++) {
      // When using qFuzzy compare against a number that is zero
      // you must offset the number per Qt documentation
      if (!qFuzzyCompare(vector(i) + 1.0, 1.0)) return false;
    }

    return true;
  }


  /**
   * Determines whether the given vector is empty (i.e. size 0).
   *
   * @param vector The vector to check.
   *
   * @return @b bool Returns true if the size of the given vector is zero.
   *
   */
  bool LinearAlgebra::isEmpty(const LinearAlgebra::Vector &vector) {
    return vector.empty();
  }


  /**
   * Determines whether the given vector is a unit vector.
   *
   * @param vector The vector to check.
   *
   * @return @b bool Returns true if the vector is a unit vector.
   */
  bool LinearAlgebra::isUnit(const Vector &vector) {
    if (qFuzzyCompare(dotProduct(vector, vector), 1.0)) {
      return true;
    }
    return false;
  }


  /**
   * Returns the inverse of a 2x2 or 3x3 matrix. Throws an error if the given
   * matrix is not invertible.
   *
   * @param matrix The matrix to inverse.
   *
   * @return @b LinearAlgebra::Matrix The inverse matrix.
   *
   * @throw IException::Programmer "The given matrix is not invertible.
   *                                The determinant is 0.0."
   * @throw IException::Programmer "Unable to invert the given matrix."
   */
  LinearAlgebra::Matrix LinearAlgebra::inverse(const Matrix &matrix) {
    try {
      if (isOrthogonal(matrix)) {
        return transpose(matrix);
      }

      // the determinant method will verify the size of the matrix is 2x2 or 3x3
      double det = determinant(matrix);

      if (qFuzzyCompare(det + 1.0, 1.0)) {
        // the inverse exists <==> the determinant is not 0.0
        std::string msg = "The given matrix is not invertible. The determinant is 0.0.";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      // since the determinant is not zero, we can calculate the reciprocal
      double scale = 1 / det;

      LinearAlgebra::Matrix inverse = identity(matrix.size1());
      // find the inverse for 2x2
      if (matrix.size1() == 2) {
        inverse(0,  0) =  scale * matrix(1, 1);
        inverse(0,  1) = -scale * matrix(0, 1);

        inverse(1,  0) = -scale * matrix(1, 0);
        inverse(1,  1) =  scale * matrix(0, 0);
        return inverse;
      }
      // else we have a 3x3
      inverse(0,  0) = scale * ( matrix(1, 1) * matrix(2, 2) - matrix(2, 1) * matrix(1, 2) );
      inverse(0,  1) = scale * ( matrix(0, 2) * matrix(2, 1) - matrix(2, 2) * matrix(0, 1) );
      inverse(0,  2) = scale * ( matrix(0, 1) * matrix(1, 2) - matrix(1, 1) * matrix(0, 2) );

      inverse(1,  0) = scale * ( matrix(1, 2) * matrix(2, 0) - matrix(2, 2) * matrix(1, 0) );
      inverse(1,  1) = scale * ( matrix(0, 0) * matrix(2, 2) - matrix(2, 0) * matrix(0, 2) );
      inverse(1,  2) = scale * ( matrix(0, 2) * matrix(0, 1) - matrix(1, 2) * matrix(0, 0) );

      inverse(2,  0) = scale * ( matrix(1, 0) * matrix(2, 1) - matrix(2, 0) * matrix(1, 1) );
      inverse(2,  1) = scale * ( matrix(0, 1) * matrix(2, 0) - matrix(2, 1) * matrix(0, 0) );
      inverse(2,  2) = scale * ( matrix(0, 0) * matrix(1, 1) - matrix(1, 0) * matrix(0, 1) );

      return inverse;
    }
    catch (IException &e) {
      std::string msg = "Unable to invert the given matrix.";
      throw IException(e, IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Returns the pseudoinverse of a matrix.
   *
   * @param matrix The matrix to compute the pseudoinverse of.
   *
   * @return @b LinearAlgebra::Matrix The pseudoinverse matrix.
   */
  LinearAlgebra::Matrix LinearAlgebra::pseudoinverse(const Matrix &matrix) {
    // Copy values into Armadillo matrix
    arma::mat arMat(matrix.size1(), matrix.size2());
    for (size_t i = 0; i < matrix.size1(); i++) {
      for (size_t j = 0; j < matrix.size2(); j++) {
        arMat(i, j) = matrix(i, j);
      }
    }

    arma::mat invArMat = arma::pinv(arMat);

    // Copy values back to Boost matrix
    LinearAlgebra::Matrix inverse(invArMat.n_rows, invArMat.n_cols);
    for (size_t i = 0; i < invArMat.n_rows; i++) {
      for (size_t j = 0; j < invArMat.n_cols; j++) {
        inverse(i, j) = invArMat(i, j);
      }
    }

    return inverse;
  }


  /**
   * Returns the transpose of the given matrix.
   *
   * @param matrix The matrix to transpose.
   *
   * @return @b LinearAlgebra::Matrix The transposed matrix.
   */
  LinearAlgebra::Matrix LinearAlgebra::transpose(const Matrix &matrix) {
    // no error testing required so call the boost transpose function
    return boost::numeric::ublas::trans(matrix);
  }


  /**
   * Returns the identity matrix of size NxN.
   *
   * @param int The size of the square matrix.
   *
   * @return @b LinearAlgebra::Matrix The NxN square matrix.
   *
   * @throw IException::Programmer "Can not create identity matrix of negative size."
   */
  LinearAlgebra::Matrix LinearAlgebra::identity(int size) {
    if (size < 1) {
      std::string msg = "Can not create identity matrix of negative size ["
                    + toString((int) size) + "].";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    LinearAlgebra::Matrix m(size, size, 0.0);
    for (int i = 0; i < size; i++) {
      m(i, i) = 1.0;
    }

    return m;
  }


  /**
   * Returns a matrix with given dimensions that is filled with zeroes.
   *
   * @param rows The number of rows in the returned matrix.
   * @param columns The number of colums in the returned matrix.
   *
   * @return @b LinearAlgebra::Matrix A zero-filled matrix.
   */
  LinearAlgebra::Matrix LinearAlgebra::zeroMatrix(int rows, int columns) {
    boost::numeric::ublas::zero_matrix<double> m(rows, columns);
    return m;
  }


  /**
   * Returns a vector of given length that is filled with zeroes.
   *
   * @param size Size of the vector.
   *
   * @return @b LinearAlgebra::Vector A zero-filled vector.
   */
  LinearAlgebra::Vector LinearAlgebra::zeroVector(int size) {
    boost::numeric::ublas::zero_vector<double> v(size);
    return v;
  }


  /**
   * Returns the determinant of the given 3x3 matrix.
   *
   * @param LinearAlgebra::Matrix The 3x3 matrix whose determinant will be
   *                     calculated.
   *
   * @return @b double The determinant of the given matrix.
   *
   * @throw IException::Programmer "Unable to calculate the determinant for the
   *                                given matrix. This method only calculates the
   *                                determinant for 2x2 or 3x3 matrices."
   *
   */
  double LinearAlgebra::determinant(const Matrix &matrix) {
    if ( (matrix.size1() != matrix.size2()) || (matrix.size1() != 2 &&  matrix.size1() != 3) ) {
      std::string msg = "Unable to calculate the determinant for the given matrix. "
                    "This method only calculates the determinant for 2x2 or 3x3 matrices."
                    "The given matrix is [" + toString((int) matrix.size1()) + "x"
                    + toString((int) matrix.size2()) + "].";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (matrix.size1() == 2) {
      return matrix(0, 0) * matrix(1, 1) - matrix(1, 0) * matrix(0, 1);
    }
    else {

      return matrix(0, 0) * (matrix(1, 1)*matrix(2, 2) - matrix(1, 2)*matrix(2, 1))
           - matrix(0, 1) * (matrix(1, 0)*matrix(2, 2) - matrix(1, 2)*matrix(2, 0))
           + matrix(0, 2) * (matrix(1, 0)*matrix(2, 1) - matrix(1, 1)*matrix(2, 0));
    }

  }


  /**
   * Returns a unit vector that is codirectional with the given
   * vector by dividing each component of the vector by the vector magnitude.
   *
   * @f$ \hat{v} = \frac{v}{\| v \|} @f$
   * where @f$ \| u \|_ @f$ is the magnitude of u.
   *
   * @see magnitude()
   *
   * @param vector The vector to be normalized.
   *
   * @return @b LinearAlgebra::Vector A unit vector from the given vector.
   *
   * @throw IException::Programmer "Unable to normalize the zero vector."
   */
  LinearAlgebra::Vector LinearAlgebra::normalize(const Vector &vector) {
    // impossible to unitize the zero vector
    if (LinearAlgebra::isZero(vector)) {
      std::string msg = "Unable to normalize the zero vector.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    return  vector / LinearAlgebra::magnitude(vector);
  }


  /**
   * Computes the magnitude (i.e., the length) of the given
   * vector using the Euclidean norm (L2 norm).  The maximum norm
   * (L-infinity) is also used to stabilize the solution in order to avoid
   * overflow. This method uses the computation
   *
   * @f$ \| v \| = \| v \|_\infty \| \frac{v}{\| v \|_\infty} \|_2 @f$
   * where @f$ \| u \|_\infty = \max_{i=1}^n \{ \mid u_i \mid \}  @f$ is the maximum norm
   * and @f$ \| u \|_2 = \sqrt{\sum_{i=1}^n u_1^2} @f$ is the Euclidean norm.
   *
   *
   * @see absoluteMaximum()
   *
   * @param vector The vector whose magnitude will be computed.
   *
   * @return @b double The magnitude (length) of the given vector.
   */
  double LinearAlgebra::magnitude(const Vector &vector) {
    double magnitude;
    // avoid dividing by maxNorm = 0
    // no stabilization is needed for the zero vector, magnitude is always 0.0
    if (LinearAlgebra::isZero(vector)) {
      magnitude = 0.0;
    }
    else {
      double maxNorm = LinearAlgebra::absoluteMaximum(vector);
      magnitude = maxNorm * boost::numeric::ublas::norm_2(vector / maxNorm);
    }
    return magnitude;
  }


  /**
   * Returns the maximum norm (L-infinity norm) for the given vector.
   *
   * The maximum norm is defined by the absolute values of the vector
   * components:
   *
   * @f$ \| v \|_\infty = \max_{i=1}^n \{ \mid v_i \mid \} @f$
   *
   * @param vector The vector whose absolute maximum will be returned.
   *
   * @return @b double The maximum of the absolute values of the vector components.
   */
  double LinearAlgebra::absoluteMaximum(const Vector &vector) {
    return boost::numeric::ublas::norm_inf(vector);
  }


  /**
   * Returns the product of two matrices. Will throw an error if the matrices
   * are not properly sized (the number of columns of the first matrix must
   * match the number or rows of the second matrix).
   *
   * @param matrix1 The left matrix.
   * @param matrix2 The right matrix.
   *
   * @return @b LinearAlgebra::Matrix The resultant matrix product.
   *
   * @throw IException::Programmer "Unable to multiply matrices
   *                                with mismatched dimensions."
   */
  LinearAlgebra::Matrix LinearAlgebra::multiply(const Matrix &matrix1, const Matrix &matrix2) {
    // Check to make sure we can multiply
    if (matrix1.size2() != matrix2.size1()) {
      std::string msg = "Unable to multiply matrices with mismatched dimensions. "
                    "The left matrix has [" + toString((int) matrix1.size2())
                    + "] columns and the right matrix has [" + toString((int) matrix2.size1())
                    + "] rows.";

      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Use boost product routine
    return boost::numeric::ublas::prod(matrix1, matrix2);
  }


  /**
   * Computes the product of the given matrix and vector. The vector will
   * be multiplied on the right side of the given matrix. Will throw an
   * error if the two are not properly sized (matrix columns not equal to
   * vector size).
   *
   * @param matrix The matrix to be multiplied.
   * @param vector The column vector to be multiplied on the right side of
   *               the matrix.
   *
   * @return @b LinearAlgebra::Vector The resultant vector.
   *
   * @throw IException::Programmer "Unable to multiply matrix and vector
   *                                with mismatched dimensions."
   */
  LinearAlgebra::Vector LinearAlgebra::multiply(const Matrix &matrix, const Vector &vector) {
    // Check to make sure we can multiply
    if (matrix.size2() != vector.size()) {
      std::string msg = "Unable to multiply matrix and vector with mismatched dimensions."
                    "The given vector has [" + toString((int) vector.size())
                    + "] components and the given matrix has ["
                    + toString((int) matrix.size2()) + "] columns.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Use boost product routine
    return boost::numeric::ublas::prod(matrix, vector);
  }


  /**
   * Multiplies the given scalar and vector.
   *
   * @param scalar The scalar to be multiplied by each component of the vector.
   * @param vector The vector to be scaled.
   *
   * @return @b LinearAlgebra::Vector The resultant scaled vector.
   */
  LinearAlgebra::Vector LinearAlgebra::multiply(double scalar, const Vector &vector) {
    // no error checks necessary (use the boost operator* method)
    return scalar * vector;
  }


  /**
   * Multiplies the given scalar and matrix.
   *
   * @param scalar The scalar to be multiplied by each element of the matrix.
   * @param matrix The matrix to be scaled.
   *
   * @return @b LinearAlgebra::Matrix The resultant scaled matrix.
   */
  LinearAlgebra::Matrix LinearAlgebra::multiply(double scalar, const Matrix &matrix) {
    // no error checks necessary
    return scalar * matrix;
  }


  /**
   * Adds the two given vectors.
   *
   * @param vector1 The first vector.
   * @param vector2 The second vector.
   *
   * @return @b LinearAlgebra::Vector The sum of the vectors.
   *
   * @throw IException::Programmer "Unable to add vectors
   *                                with mismatched sizes."
   */
  LinearAlgebra::Vector LinearAlgebra::add(const Vector &vector1, const Vector &vector2) {
    // Vectors best be the same size
    if (vector1.size() != vector2.size()) {
      std::string msg = "Unable to add vectors with mismatched sizes."
                    "Vector1 has [" + toString((int) vector1.size())
                    + "] components and vector2 has ["
                    + toString((int) vector2.size()) + "] components.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return vector1 + vector2;
  }


  /**
   * Subtracts the right vector from the left vector.
   *
   * @param vector1 The vector to the left of the subtraction operator.
   * @param vector2 The vector to the right of the subtraction operator.
   *
   * @return @b LinearAlgebra::Vector The difference of the vectors
   *                                  (i.e. vector1 - vector2).
   *
   * @throw IException::Programmer "Unable to subtract vectors
   *                                with mismatched sizes."
   */
  LinearAlgebra::Vector LinearAlgebra::subtract(const Vector &vector1, const Vector &vector2) {
    // Vectors best be the same size
    if (vector1.size() != vector2.size()) {
      std::string msg = "Unable to subtract vectors with mismatched sizes."
                    "Vector1 has [" + toString((int) vector1.size())
                    + "] components and vector2 has ["
                    + toString((int) vector2.size()) + "] components.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return vector1 - vector2;
  }


  /**
   * Returns the cross product of two vectors. Note: the cross-product
   * requires the vectors to have exactly three components each.
   *
   * @param vector1 The vector to the left of the cross product operator.
   * @param vector2 The vector to the right of the cross product operator.
   *
   * @return @b LinearAlgebra::Vector The cross product of the given
   *                                  vectors (i.e. vector1 x vector2).
   *
   * @throw IException::Programmer "Unable to calculate the cross
   *                                product on vectors that are not
   *                                size 3. "
   */
  LinearAlgebra::Vector LinearAlgebra::crossProduct(const Vector &vector1, const Vector &vector2) {
    if ((vector1.size() != 3) || (vector2.size() != 3)) {
      std::string msg = "Unable to calculate the cross product on vectors that are not size 3. "
                    "Vector1 has [" + toString((int) vector1.size())
                    + "] components and vector2 has ["
                    + toString((int) vector2.size()) + "] components.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    Vector crossProd(3);
    crossProd(0) = vector1(1)*vector2(2) - vector1(2)*vector2(1);
    crossProd(1) = vector1(2)*vector2(0) - vector1(0)*vector2(2);
    crossProd(2) = vector1(0)*vector2(1) - vector1(1)*vector2(0);

    return crossProd;
  }


  /**
   * Divides each vector by its corresponding absolute maximum,
   * computes the cross product of the new vectors, and normalizes the
   * resultant vector from the cross product. Note: the
   * cross-product requires the vectors to have exactly three
   * components each.
   *
   * @param vector1 The vector to the left of the cross product operator.
   * @param vector2 The vector to the right of the cross product operator.
   *
   * @return @b LinearAlgebra::Vector The normalized cross product of the given vectors
   *                                  (i.e. normalize(vector1/absoluteMaximum(vector1) x
   *                                   vector2/absoluteMaximum(vector2))).
   */
  // is this derived from naif's ucrss routine???
  LinearAlgebra::Vector LinearAlgebra::normalizedCrossProduct(const Vector &vector1,
                                                              const Vector &vector2) {

    double maxVector1 = LinearAlgebra::absoluteMaximum(vector1);
    double maxVector2 = LinearAlgebra::absoluteMaximum(vector2);

    LinearAlgebra::Vector normalizedVector1 = vector1;
    LinearAlgebra::Vector normalizedVector2 = vector2;

    if (maxVector1 != 0.0) {
      normalizedVector1 /= maxVector1;
    }

    if (maxVector2 != 0.0) {
      normalizedVector2 /= maxVector2;
    }

    LinearAlgebra::Vector vcross = crossProduct(normalizedVector1, normalizedVector2);

    return normalize(vcross);
  }


  /**
   * Computes the outer product of the given vectors. The outer product
   * operation is defined as the cross product of vector1 and the conjugate
   * transpose of vector2.
   *
   * @param vector1 The vector to the left side of the outer product operator.
   * @param vector2 The vector to the right side of the outer product operator.
   *
   * @return @b LinearAlgebra::Matrix The outer product matrix of the given vectors.
   *
   * @throw IException::Programmer "Unable to compute the outer product for
   *                                vectors with mismatched sizes."
   */
  LinearAlgebra::Matrix LinearAlgebra::outerProduct(const Vector &vector1, const Vector &vector2) {
    if (vector1.size() != vector2.size()) {
      std::string msg = "Unable to compute the outer product for vectors with mismatched sizes."
                    "Vector1 has [" + toString((int) vector1.size())
                    + "] components and vector2 has ["
                    + toString((int) vector2.size()) + "] components.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return boost::numeric::ublas::outer_prod(vector1, vector2);
  }


  /**
   * Computes the dot product of the given vectors. For Euclidean space, this is the same
   * as the inner product, so these methods are interchangeable.
   *
   * @see innerProduct()
   *
   * @param vector1 The first vector.
   * @param vector2 The second vector.
   *
   * @return @b double The dot product of the vectors.
   *
   */
  double LinearAlgebra::dotProduct(const Vector &vector1, const Vector &vector2) {
    // no error check needed - this is done by innerProduct
    return LinearAlgebra::innerProduct(vector1, vector2);
  }


  /**
   * Computes the inner product of the given vectors. For Euclidean space, this is the same
   * as the dot product, so these methods are interchangeable.
   *
   * @see dotProduct()
   *
   * @param vector1 The first vector.
   * @param vector2 The second vector.
   *
   * @return @b double The inner product of the vectors.
   *
   * @throw IException::Programmer "Unable to compute the dot product for vectors
   *                                with mismatched sizes."
   */
  double LinearAlgebra::innerProduct(const Vector &vector1, const Vector &vector2) {
    if (vector1.size() != vector2.size()) {
      std::string msg = "Unable to compute the dot product for vectors with mismatched sizes."
                    "Vector1 has [" + toString((int) vector1.size())
                    + "] components and vector2 has ["
                    + toString((int) vector2.size()) + "] components.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return boost::numeric::ublas::inner_prod(vector1, vector2);
  }


  /**
   * Compute the vector projection of vector1 onto vector2. This is
   * the orthogonal projection of vector1 onto a line that is parallel
   * to vector2.
   *
   * It is defined by
   * @f$ proj_{v_2} v_1 = \frac{v_1 \cdot v_2}{\| v_2 \|^2}v_2 @f$
   * where @f$ \| u \|_ @f$ is the magnitude of u.
   *
   * @param vector1 The vector to the left of the project operator.
   * @param vector2 The vector to the right of the project operator.
   *
   * @return @b LinearAlgebra::Vector The resultant vector that is
   *                                  the orthogonal projection of
   *                                  vector1 onto vector2.
   *
   * @throw IException::Programmer "Unable to project vector1 onto vector2
   *                                with mismatched sizes."
   */
  // derived from naif's vproj routine
  LinearAlgebra::Vector LinearAlgebra::project(const Vector &vector1, const Vector &vector2) {
    if (vector1.size() != vector2.size()) {
      std::string msg = "Unable to project vector1 onto vector2 with mismatched sizes."
                    "Vector1 has [" + toString((int) vector1.size())
                    + "] components and vector2 has ["
                    + toString((int) vector2.size()) + "] components.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // if vector2 is the zero vector, then the projection of vector1 onto vector2
    // is also the zero vector
    if (LinearAlgebra::isZero(vector2)) return vector2;

    // find the dot product of the two vectors
    double v1Dotv2 = LinearAlgebra::dotProduct(vector1, vector2);

    // find the square of the length of vector2 (this is the dot product of the vector2 with itself)
    double v2Dotv2 = LinearAlgebra::dotProduct(vector2, vector2);

    return v1Dotv2/v2Dotv2 * vector2;
  }


  /**
   * Rotates a vector about an axis vector given a specified angle.
   * Note: this method only rotates a vector with three components since
   * the cross product calculation requires this.
   *
   * @param vector The vector to rotate, which must have three components.
   * @param axis  A vector defining the axis, which must also have three components.
   * @param angle The angle to rotate.
   *
   * @return @b LinearAlgebra::Vector The rotated vector.
   *
   * @throw IException::Programmer "Unable to rotate vector about the
   *                                given axis and angle. Vectors must
   *                                be of size 3 to perform rotation. "
   */
  // derived from naif's vrotv routine
  LinearAlgebra::Vector LinearAlgebra::rotate(const Vector &vector, const Vector &axis,
                                              Angle angle) {
    if ((vector.size() != 3) || (axis.size() != 3)) {
      std::string msg = "Unable to rotate vector about the given axis and angle. "
                    "Vectors must be of size 3 to perform rotation. "
                    "The given vector has [" + toString((int) vector.size())
                    + "] components and the given axis has ["
                    + toString((int) axis.size()) + "] components.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // if the given vector is the zero vector, then the rotation about the vector
    // is also the zero vector
    if (isZero(axis)) return vector;

    // Compute the unit vector that is codirectional with the given axis
    Vector axisUnitVector = normalize(axis);

    // Compute the projection of the given vector onto the axis unit vector.
    Vector projVectorOnAxis = project(vector,  axisUnitVector);

    // Compute the component of the input orthogonal to the AXIS.  Call it V1.
    Vector v1 = vector - projVectorOnAxis;

    // Rotate V1 by 90 degrees about the AXIS and call the result V2.
    Vector v2 = LinearAlgebra::crossProduct(axisUnitVector, v1);

    // Compute cos(angle)*v1 + sin(angle)*v2. This is v1 rotated about
    // the axis in the plane normal to the axis, call the result rplane
    double c = cos(angle.radians());
    double s = sin(angle.radians());
    Vector rplane = (c*v1) + (s*v2);

    // Add the rotated component in the normal plane to axis to the
    // projection of v onto axis to obtain rotation.
    return (rplane + projVectorOnAxis);
  }


 /**
  * Finds the unique vector P such that A = V + P, V is
  * parallel to B and P is perpendicular to B, where A is the first vector
  * (vector1) and B is the second vector (vector2) passed in by the user.
  *
  * For all vectors A and B, there exists unique vectors V and P such that
  * 1) A = V + P
  * 2) V is parallel to B
  * 3) P is perpendicular to B
  *
  * @param vector1 The first vector, denoted A in the description.
  * @param vector2 The second vector, denoted B in the description.
  *
  * @return @b LinearAlgebra::Vector A vector perpendicular to vector2 and
  *                                  equal to vector1-parallel2 (where
  *                                  parallel2 is some vector that is
  *                                  parallel to vector2).
  */
  // Derived from NAIF's vperp routine
  // possible that we may need to restrict size to dimesion 3, but doesn't seem
  // necessary in the code.
  LinearAlgebra::Vector LinearAlgebra::perpendicular(const Vector &vector1, const Vector &vector2) {

    // if vector1 is the zero vector, just set p to zero and return.
    if ( isZero(vector1) ) {
       return vector1;
    }

    // if vector2 is the zero vector, just set p to vector1 and return.
    if ( isZero(vector2) ) {
       return vector1;
    }

    // normalize (using the max norm) the given vectors and project the first onto the second
    double max1 = absoluteMaximum(vector1);
    double max2 = absoluteMaximum(vector2);
    Vector parallelVector = project(vector1 / max1,  vector2 / max2);

    Vector perpendicularVector = vector1 - parallelVector * max1;

    return perpendicularVector;

  }


  /**
   * Converts a rotation's representation from a matrix to a axis of rotation
   * and its corresponding rotation angle.
   *
   * @param rotationMatrix A matrix representing a rotation.
   *
   * @return @b LinearAlgebra::AxisAngle The axis-angle pair representing the rotation.
   *
   * @throw IException::Programmer "Unable to convert the given matrix to an
   *                                axis of rotation and a rotation angle. A
   *                                3x3 matrix is required."
   * @throw IException::Programmer "Unable to convert the given matrix to an
   *                                axis of rotation and a rotation angle.
   *                                The given matrix is not a rotation
   *                                matrix."
   */
  // Derived from NAIF's raxisa routine
  LinearAlgebra::AxisAngle LinearAlgebra::toAxisAngle(const Matrix &rotationMatrix) {

    if ((rotationMatrix.size1() != 3) || (rotationMatrix.size2() != 3)) {
      std::string msg = "Unable to convert the given matrix to an axis of rotation "
                    "and a rotation angle. A 3x3 matrix is required. The given matrix is ["
                    + toString((int) rotationMatrix.size1()) + "x"
                    + toString((int) rotationMatrix.size2()) + "].";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // if given matrix is not a rotation matrix, we can't proceed.
    if ( !isRotationMatrix(rotationMatrix) ) {
      std::string msg = "Unable to convert the given matrix to an axis of rotation "
                    "and a rotation angle. The given matrix is not a rotation matrix.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    Angle angle;
    Vector axis(3);

    // Construct the quaternion corresponding to the input rotation matrix
    Vector quaternion = LinearAlgebra::toQuaternion(rotationMatrix);

    LinearAlgebra::Vector subQuaternion(3);
    subQuaternion[0] = quaternion[1];
    subQuaternion[1] = quaternion[2];
    subQuaternion[2] = quaternion[3];


    // The quaternion we've just constructed is of the form:
    //
    //    cos(ANGLE/2) + sin(ANGLE/2) * AXIS
    //
    // We take a few precautions to handle the case of an identity
    // rotation.
    if (isZero(subQuaternion)) {
      axis(0) = 0.0;
      axis(1) = 0.0;
      axis(2) = 1.0;

      angle.setRadians(0.0);
    }
    else if (qFuzzyCompare(quaternion(0) + 1.0, 1.0)) {
      axis = subQuaternion;
      angle.setRadians(Isis::PI);
    }
    else {
      axis = LinearAlgebra::normalize(subQuaternion);
      angle.setRadians(2.0 * atan2(magnitude(subQuaternion), quaternion(0)));
    }

    return qMakePair(axis, angle);
  }


  /**
   * Converts a rotation's representation from a matrix to a set of Euler angles
   * with corresponding axes.
   *
   * @param rotationMatrix A matrix representing a rotation.
   * @param axes A list containing the order of axes.
   *
   * @return @b QList< LinearAlgebra::EulerAngle> The list of 3 Euler angles
   *                                              with their axes representing the rotation.
   *
   * @throw IException::Programmer "Unable to convert the given matrix to Euler angles.
   *                                Exactly 3 axis codes are required."
   * @throw IException::Programmer "Unable to convert the given matrix to Euler angles
   *                                using the given axis codes. Axis codes must be 1, 2, or 3."
   * @throw IException::Programmer "Unable to convert the given matrix to Euler angles
   *                                using the given axis codes. The middle axis
   *                                code must differ from its neighbors."
   * @throw IException::Programmer "Unable to convert the given matrix to Euler angles.
   *                                A 3x3 matrix is required."
   * @throw IException::Programmer "Unable to convert the given matrix to Euler angles.
   *                                The given matrix is not a rotation matrix."
   */
  // Derived from NAIF's m2eul routine
  QList<LinearAlgebra::EulerAngle> LinearAlgebra::toEulerAngles(const Matrix &rotationMatrix,
                                                                const QList<int> axes) {

    // check there are 3 axes in the set {1,2,3} with center axis not equal to first or last
    if (axes.size() != 3) {
      std::string msg = "Unable to convert the given matrix to Euler angles. "
                    "Exactly 3 axis codes are required. The given list has ["
                    + toString((int) axes.size()) + "] axes.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    QSet<int> validAxes;
    validAxes << 1 << 2 << 3;
    if (!validAxes.contains(axes[0])
        || !validAxes.contains(axes[1])
        || !validAxes.contains(axes[2])) {
      std::string msg = "Unable to convert the given matrix to Euler angles using the given axis codes "
                    "[" + toString(axes[0]) + ", " + toString(axes[1]) + ", " + toString(axes[2])
                    + "]. Axis codes must be 1, 2, or 3.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (axes[0] == axes[1] || axes[1] == axes[2]) {
      std::string msg = "Unable to convert the given matrix to Euler angles using the given axis codes "
                    "[" + toString(axes[0]) + ", " + toString(axes[1]) + ", " + toString(axes[2])
                    + "]. The middle axis code must differ from its neighbors.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // check the matrix is 3x3 rotation
    if ((rotationMatrix.size1() != 3) || (rotationMatrix.size2() != 3)) {
      std::string msg = "Unable to convert the given matrix to Euler angles. A 3x3 matrix is required. "
                    "The given matrix is ["
                    + toString((int) rotationMatrix.size1()) + "x"
                    + toString((int) rotationMatrix.size2()) + "].";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if ( !isRotationMatrix(rotationMatrix) ) {
      std::string msg = "Unable to convert the given matrix to Euler angles. "
                    "The given matrix is not a rotation matrix.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // AXIS3, AXIS2, AXIS1 and R have passed their tests at this
    // point.  We take the liberty of working with TEMPROT, a
    // version of R that has unitized columns.
    //  DO I = 1, 3
    //     CALL VHAT ( R(1,I), TEMPROT(1,I) )
    //  END DO

    LinearAlgebra::Matrix tempRotation(3, 3);
    for (int i = 0; i < 3; i++) {
      Vector columnVector = column(rotationMatrix, i);
      // replace the ith column with a unitized column vector
      setColumn(tempRotation, normalize(columnVector), i);
    }

    QList<int> nextAxis;
    nextAxis << 2 << 3 << 1;
    double sign;
    // create a matrix of zeros and we will fill in the non zero components below
    LinearAlgebra::Matrix change = zeroMatrix(3, 3);

    Angle angle1, angle2, angle3;
    if ( axes[0] == axes[2] ) {
      if ( axes[1] == nextAxis[axes[0] - 1] ) {
         sign =  1.0;
      }
      else {
         sign = -1.0;
      }
      int c = 6 - axes[0] - axes[1];

      change( axes[0] - 1, 2 ) =         1.0;
      change( axes[1] - 1, 0 ) =         1.0;
      change( c       - 1, 1 ) =  sign * 1.0;
      LinearAlgebra::Matrix tempMatrix = multiply( tempRotation,  change );
      tempRotation = multiply( transpose(change),  tempMatrix );

      bool degen =    (    qFuzzyCompare(tempRotation(0, 2) + 1.0, 1.0)
                        && qFuzzyCompare(tempRotation(1, 2) + 1.0, 1.0) )
                   || (    qFuzzyCompare(tempRotation(2, 0) + 1.0, 1.0)
                        && qFuzzyCompare(tempRotation(2, 1) + 1.0, 1.0) )
                   || qFuzzyCompare( qAbs(tempRotation(2, 2)), 1.0 );

      if ( degen ) {

        angle3.setRadians( 0.0 );
        angle2.setRadians(  acos( tempRotation(2, 2) ) );
        angle1.setRadians( atan2( tempRotation(0, 1), tempRotation(0, 0) ) );

      }
      else {
      // the normal case.
        angle3.setRadians( atan2( tempRotation(0, 2),  tempRotation(1, 2) ) );
        angle2.setRadians(  acos( tempRotation(2, 2) ) );
        angle1.setRadians( atan2( tempRotation(2, 0), -tempRotation(2, 1) ) );
      }
    }
    else {
    //   The axis order is c-b-a.  We're going to find a matrix CHANGE
    //   such that
    //
    //            T
    //      CHANGE  R  CHANGE
      if ( axes[1] == nextAxis[axes[0] - 1] ) {
         sign =  1.0;
      }
      else {
         sign = -1.0;
      }
      change( axes[0] - 1, 0 ) =        1.0;
      change( axes[1] - 1, 1 ) =        1.0;
      change( axes[2] - 1, 2 ) = sign * 1.0;
      LinearAlgebra::Matrix tempMatrix = multiply( tempRotation,  change );
      tempRotation = multiply( transpose(change),  tempMatrix );
      bool degen =    (    qFuzzyCompare(tempRotation(0, 0) + 1.0, 1.0)
                        && qFuzzyCompare(tempRotation(0, 1) + 1.0, 1.0) )
                   || (    qFuzzyCompare(tempRotation(1, 2) + 1.0, 1.0)
                        && qFuzzyCompare(tempRotation(2, 2) + 1.0, 1.0) )
                   || qFuzzyCompare( qAbs(tempRotation(0, 2)), 1.0 );

      if ( degen ) {

        angle3.setRadians( 0.0 );
        angle2.setRadians(         asin( -tempRotation(0, 2) ) );
        angle1.setRadians( sign * atan2( -tempRotation(1, 0), tempRotation(1, 1) ) );

      }
      else {
      // the normal case.
        angle3.setRadians(        atan2(  tempRotation(1, 2), tempRotation(2, 2) ) );
        angle2.setRadians(         asin( -tempRotation(0, 2) ) );
        angle1.setRadians( sign * atan2(  tempRotation(0, 1), tempRotation(0, 0) ) );
      }
    }

    QList<LinearAlgebra::EulerAngle> eulerAngles;
    eulerAngles << qMakePair(angle3, axes[0])
                << qMakePair(angle2, axes[1])
                << qMakePair(angle1, axes[2]);
    return eulerAngles;
  }


  /**
   * Converts a rotation's representation from a matrix to a unit quaternion.
   *
   * @param rotationMatrix A matrix representing a rotation.
   *
   * @return @b LinearAlgebra::Vector A unit quaternion representing the rotation.
   *
   * @throw IException::Programmer "Unable to convert the given matrix to a quaternion.
                                    A 3x3 matrix is required.
   * @throw IException::Programmer "Unable to convert the given matrix to an axis of rotation
   *                                and a rotation angle. The given matrix is not a rotation
   *                                matrix."
   */
  // Derived from NAIF's m2q routine
  LinearAlgebra::Vector LinearAlgebra::toQuaternion(const Matrix &rotationMatrix) {

    if ((rotationMatrix.size1() != 3) || (rotationMatrix.size2() != 3)) {
      std::string msg = "Unable to convert the given matrix to a quaternion. "
                    "A 3x3 matrix is required. The given matrix is ["
                    + toString((int) rotationMatrix.size1()) + "x"
                    + toString((int) rotationMatrix.size2()) + "].";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // if given matrix is not a rotation matrix, we can't proceed.
    if ( !isRotationMatrix(rotationMatrix) ) {
      std::string msg = "Unable to convert the given matrix to an axis of rotation "
                    "and a rotation angle. The given matrix is not a rotation matrix.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Let q = c + s1*i + s2*j + s3*k
    // where c is the constant and s1, s2, s3 are the coefficients of the imaginary parts
    // and |q| = 1, .

    // Denote the following for n,m in {1,2,3}
    // cc =  c  *  c
    // csn = c  *  sn
    // snm = sn *  sm

    // The rotation matrix corresponding to our quaternion is:
    //
    // |  cc+s11-s22-s33      2*s12-2*cs3       2*s13+2*cs2   |
    // |    2*s12+2*cs3     cc-s11+s22-s33      2*s23-2*cs1   |
    // |    2*s13-2*cs2       2*s23+2*cs1     cc-s11-s22+s33  |
    //
    //
    // since |q| = cc + s11 + s22 + s33 = 1, we can use substitution on the diagonal entries to get
    //
    // |  1-2*s22-2*s33      2*s12-2*cs3      2*s13+2*cs2   |
    // |    2*s12+2*cs3    1-2*s11-2*s33      2*s23-2*cs1   |
    // |    2*s13-2*cs2      2*s23+2*cs1    1-2*s11-2*s22   |
    //
    //
    //
    // r(1,1)      = 1.0 - 2*s22 - 2*s33
    // r(2,1)      =       2*s12 + 2*cs3
    // r(3,1)      =       2*s13 - 2*cs2
    //
    // r(1,2)      =       2*s12 - 2*cs3
    // r(2,2)      = 1.0 - 2*s11 - 2*s33
    // r(3,2)      =       2*s23 + 2*cs1
    //
    // r(1,3)      =       2*s13 + 2*cs2
    // r(2,3)      =       2*s23 - 2*cs1
    // r(3,3)      = 1.0 - 2*s11 - 2*s22

    // Using this matrix, we get the trace by summing the diagonal entries
    //    trace = (1-2*s22-2*s33) + (1-2*s11-2*s33) + (1-2*s11-2*s22)
    //          = 3 - 4*(s11 + s22 + s33)

    // Therefore
    //    1.0 + trace = 4 - 4*(s11 + s22 + s33)
    //                = 4*(1 - s11 - s22 - s33)       by factoring 4
    //                = 4*(|q| - s11 - s22 - s33)     since |q| = 1
    //                = 4*cc                          since |q| = cc + s11 + s22 + s33
    //
    // Solving for c, we get that
    // 4*cc = 1 + trace
    //  2*c = +/- sqrt(1 + trace)
    //    c = +/- 0.5 * sqrt( 1.0 + trace )

    // We also have the following where {n,m,p} = {1,2,3}
    //   1.0 + trace - 2.0*r(n,n) = 4.0 - 4.0(snn + smm + spp) - 2(1.0 - 2.0(smm + spp ))
    //                            = 4.0 - 4.0(snn + smm + spp) - 2.0 + 4.0(smm + spp )
    //                            = 2.0 - 4.0*snn
    //
    // Solving for snn, we get
    //     2.0 - 4.0*snn =  1.0 + trace - 2.0*r(n,n)
    //    -2.0 + 4.0*snn = -1.0 - trace + 2.0*r(n,n)
    //           4.0*snn =  1.0 - trace + 2.0*r(n,n) ******** we will use this value  *************
    //                                               ******** in our code to minimize *************
    //                                               ******** computations            *************
    //            2.0*sn =  +/- sqrt(1.0 - trace + 2.0*r(n,n))
    //                sn =  +/- 0.5 * sqrt(1.0 - trace + 2.0*r(n,n))    for any n in {1,2,3}


    // In addition to these observations, note that all of the product
    // pairs can easily be computed by simplifying the expressions
    // (r(n,m) - r(m,n)) / 4.0 and (r(n,m) + r(m,n)) / 4.0
    //
    // For example,
    //  (r(3,2) - r(2,3)) / 4.0 = ( (2*s23 + 2*cs1) - (2*s23 - 2*cs1) ) / 4.0
    //                          = ( 4*cs1 ) / 4.0
    //                          =     cs1
    //
    //  So we have the following
    //  cs1 = (r(3,2) - r(2,3))/4.0
    //  cs2 = (r(1,3) - r(3,1))/4.0
    //  cs3 = (r(2,1) - r(1,2))/4.0
    //  s12 = (r(2,1) + r(1,2))/4.0
    //  s13 = (r(3,1) + r(1,3))/4.0
    //  s23 = (r(2,3) + r(3,2))/4.0

    //but taking sums or differences of numbers that are nearly equal
    //or nearly opposite results in a loss of precision. as a result
    //we should take some care in which terms to select when computing
    //c, s1, s2, s3.  however, by simply starting with one of the
    //large quantities cc, s11, s22, or s33 we can make sure that we
    //use the best of the 6 quantities above when computing the
    //remaining components of the quaternion.

    double trace  = rotationMatrix(0, 0) + rotationMatrix(1, 1) + rotationMatrix(2, 2);
    double mtrace = 1.0 - trace;

    // cc4 = 4 * c * c where c is the constant term (first) of the quaternion
    double cc4    = 1.0 + trace;
    // snn4 = 4 * sn * sn where sn is the coefficient for the n+1 term of the quaternion vector
    double s114   = mtrace + 2.0*rotationMatrix(0, 0);
    double s224   = mtrace + 2.0*rotationMatrix(1, 1);
    double s334   = mtrace + 2.0*rotationMatrix(2, 2);

    // Note that cc4 + s114 + s224 + s334 = 4|q| = 4
    // Thus, at least one of the 4 terms is greater than 1.
    double normalizingFactor;
    LinearAlgebra::Vector quaternion(4);
    if ( cc4 >= 1.0 ) { // true if the trace is non-negative

      quaternion[0] = sqrt( cc4 * 0.25 ); // note that q0 = c = sqrt(4*c*c*0.25)
      normalizingFactor = 1.0 / ( quaternion[0] * 4.0 );

      // multiply the difference of the entries symmetric about the diagonal by the factor 1/4c
      // to get s1, s2, and s3
      // (see calculations for cs1, cs2, cs3 in comments above)
      quaternion[1] = ( rotationMatrix(2, 1) - rotationMatrix(1, 2) ) * normalizingFactor;
      quaternion[2] = ( rotationMatrix(0, 2) - rotationMatrix(2, 0) ) * normalizingFactor;
      quaternion[3] = ( rotationMatrix(1, 0) - rotationMatrix(0, 1) ) * normalizingFactor;

    }
    else if ( s114 >= 1.0 ) {

      quaternion[1] = sqrt( s114 * 0.25 ); // note that q1 = s1 = sqrt(4*s1*s1*0.25)
      normalizingFactor = 1.0 /( quaternion[1] * 4.0 );

      quaternion[0] = ( rotationMatrix(2, 1) - rotationMatrix(1, 2) ) * normalizingFactor;
      quaternion[2] = ( rotationMatrix(0, 1) + rotationMatrix(1, 0) ) * normalizingFactor;
      quaternion[3] = ( rotationMatrix(0, 2) + rotationMatrix(2, 0) ) * normalizingFactor;

    }
    else if ( s224 >= 1.0 ) {

      quaternion[2] = sqrt( s224 * 0.25 );
      normalizingFactor = 1.0 /( quaternion[2] * 4.0 );

      quaternion[0] = ( rotationMatrix(0, 2) - rotationMatrix(2, 0) ) * normalizingFactor;
      quaternion[1] = ( rotationMatrix(0, 1) + rotationMatrix(1, 0) ) * normalizingFactor;
      quaternion[3] = ( rotationMatrix(1, 2) + rotationMatrix(2, 1) ) * normalizingFactor;
    }
    else { // s334 >= 1.0

      quaternion[3]   = sqrt( s334 * 0.25 );
      normalizingFactor = 1.0 /( quaternion[3] * 4.0 );

      quaternion[0] = ( rotationMatrix(1, 0) - rotationMatrix(0, 1) ) * normalizingFactor;
      quaternion[1] = ( rotationMatrix(0, 2) + rotationMatrix(2, 0) ) * normalizingFactor;
      quaternion[2] = ( rotationMatrix(1, 2) + rotationMatrix(2, 1) ) * normalizingFactor;

    }

    // if the magnitude of this quaternion is not one, we polish it up a bit.
    if ( !isUnit(quaternion) ) {
       quaternion = normalize(quaternion);
    }

    // M2Q always returns a quaternion with scalar part greater than or equal to zero.
    if ( quaternion[0] < 0.0 ) {
       quaternion *= -1.0;
    }

    return quaternion;
  }


  /**
   * Converts a rotation's representation from an axis of rotation and its
   * corresponding rotation angle to a 3x3 matrix.
   *
   *
   * @param axis The axis of rotation for a rotation.
   * @param angle The rotation angle.
   *
   * @return @b LinearAlgebra::Matrix The matrix representing the rotation.
   *
   * @throw IException::Programmer "Unable to convert the given vector and
   *                                angle to a rotation matrix. The given
   *                                vector is not a 3D axis vector."
   */
  // Derived from NAIF's axisar routine
  LinearAlgebra::Matrix LinearAlgebra::toMatrix(const Vector &axis, Angle angle) {
    if (axis.size() != 3) {
      std::string msg = "Unable to convert the given vector and angle to a rotation matrix. "
                    "The given vector with size [" + toString((int) axis.size())
                    + "] is not a 3D axis vector.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // intialize the matrix with the 3x3 identity.
    LinearAlgebra::Matrix rotationMatrix = identity(3);

    // The matrix we want rotates every vector by angle about axis.
    // In particular, it does so to our basis vectors.  The columns
    // of the output matrix are the images of the basis vectors
    // under this rotation.
    for (unsigned int i = 0; i < axis.size(); i++) {
      // rotate the ith column of the matrix (the ith standard basis vector)
      // about the given axis using the given angle
      Vector stdBasisVector = column(rotationMatrix, i);

      // replace the ith column with the rotated vector
      setColumn(rotationMatrix, rotate(stdBasisVector, axis, angle), i);
    }

    return rotationMatrix;
  }


  /**
   * Converts a rotation's representation from an axis of rotation and its
   * corresponding rotation angle to a 3x3 matrix.
   *
   * @param axisAngle The axis-angle pair representation of a rotation
   *
   * @return @b LinearAlgebra::Matrix The matrix representation of the rotation.
   *
   */
  LinearAlgebra::Matrix LinearAlgebra::toMatrix(const AxisAngle &axisAngle) {
    return LinearAlgebra::toMatrix(axisAngle.first, axisAngle.second);
  }


  /**
   * Converts a rotation's representation from a set of Euler angles (3 angles,
   * each with a corresponding axis) to a 3x3 matrix.
   *
   * @param angle3 The third angle and its axis.
   * @param angle2 The second angle and its axis.
   * @param angle1 The first angle and its axis.
   *
   * @return @b LinearAlgebra::Matrix The matrix representation of the rotation.
   *
   * @throw IException::Programmer "Unable to convert the given Euler angles to
   *                                a matrix using the given axis codes. Axis
   *                                codes must be 1, 2, or 3."
   */
  // Derived from NAIF's eul2m routine
  LinearAlgebra::Matrix LinearAlgebra::toMatrix(const EulerAngle &angle3,
                                                const EulerAngle &angle2,
                                                const EulerAngle &angle1) {
    QSet<int> validAxes;
    validAxes << 1 << 2 << 3;
    if (!validAxes.contains(angle3.second)
        || !validAxes.contains(angle2.second)
        || !validAxes.contains(angle1.second)) {
      std::string msg = "Unable to convert the given Euler angles to a matrix using the given axis "
                    "codes [" + toString(angle3.second) + ", " + toString(angle2.second) + ", "
                    + toString(angle1.second) + "]. Axis codes must be 1, 2, or 3.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    LinearAlgebra::Matrix m(3, 3);
    // implement eul2m
    // Calculate the 3x3 rotation matrix generated by a rotation
    // of a specified angle about a specified axis. This rotation
    // is thought of as rotating the coordinate system.
    //
    //ROTATE(angle1, axis1);
    double sinAngle = sin(angle1.first.radians());
    double cosAngle = cos(angle1.first.radians());
    // get the index offset based on the axis number corresponding to this angle
    double indexOffset = ((angle1.second % 3) + 3) % 3;
    QList<int> indexSet;
    indexSet << 2 << 0 << 1 << 2 << 0;
    int index1 = indexSet[indexOffset + 0];
    int index2 = indexSet[indexOffset + 1];
    int index3 = indexSet[indexOffset + 2];
    m(index1, index1) = 1.0;   m(index1, index2) =  0.0;        m(index1, index3) = 0.0;
    m(index2, index1) = 0.0;   m(index2, index2) =  cosAngle;   m(index2, index3) = sinAngle;
    m(index3, index1) = 0.0;   m(index3, index2) = -sinAngle;   m(index3, index3) = cosAngle;

    //
    // C     ROTMAT applies a rotation of ANGLE radians about axis IAXIS to a
    // C     matrix.  This rotation is thought of as rotating the coordinate
    // C     system.
    // C
    //CALL ROTMAT ( R,   ANGLE2,  AXIS2,  R1 )
    sinAngle = sin(angle2.first.radians());
    cosAngle = cos(angle2.first.radians());
    indexOffset = ((angle2.second % 3) + 3) % 3;
    index1 = indexSet[indexOffset + 0];
    index2 = indexSet[indexOffset + 1];
    index3 = indexSet[indexOffset + 2];

    LinearAlgebra::Matrix tempMatrix(3, 3);
    for (int i = 0; i < 3; i++) {
       tempMatrix(index1, i) =           m(index1, i);
       tempMatrix(index2, i) =  cosAngle*m(index2, i) + sinAngle*m(index3, i);
       tempMatrix(index3, i) = -sinAngle*m(index2, i) + cosAngle*m(index3, i);
    }

    //CALL ROTMAT ( tempMatrix,  ANGLE3,  AXIS3,  m  )
    sinAngle = sin(angle3.first.radians());
    cosAngle = cos(angle3.first.radians());
    indexOffset = ((angle3.second % 3) + 3) % 3;
    index1 = indexSet[indexOffset + 0];
    index2 = indexSet[indexOffset + 1];
    index3 = indexSet[indexOffset + 2];
    for (int i = 0; i < 3; i++) {
       m(index1, i) =           tempMatrix(index1, i);
       m(index2, i) =  cosAngle*tempMatrix(index2, i) + sinAngle*tempMatrix(index3, i);
       m(index3, i) = -sinAngle*tempMatrix(index2, i) + cosAngle*tempMatrix(index3, i);
    }

    return m;
  }


  /**
   * Converts a rotation's representation from a list of Euler angles (3 angles,
   * each with a corresponding axis) to a 3x3 matrix.
   *
   * @param eulerAngles The Euler angle representation of a rotation.
   *
   * @return @b LinearAlgebra::Matrix The matrix representation of the rotation.
   *
   * @throw IException::Programmer "Unable to convert the given Euler angles to
   *                                a matrix. Exactly 3 Euler angles are
   *                                required."
   */
  LinearAlgebra::Matrix LinearAlgebra::toMatrix(const QList<EulerAngle> &eulerAngles) {

    if (eulerAngles.size() != 3) {
      std::string msg = "Unable to convert the given Euler angles to a matrix. "
                    "Exactly 3 Euler angles are required. The given list has ["
                    + toString((int) eulerAngles.size()) + "] angles.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return LinearAlgebra::toMatrix(eulerAngles[0], eulerAngles[1], eulerAngles[2]);
  }



  /**
   * Converts a rotation's representation from a quaternion to a 3x3 matrix.
   * Note, if the given vector is not a unit vector or the zero vector, this
   * method will normalize it before computing the corresponding matrix.
   *
   * @param quaternion   A unit quaternion representation of a rotation.
   *
   * @return @b LinearAlgebra::Matrix The matrix representation of the rotation.
   *
   * @throw IException::Programmer "Unable to convert the given vector to a
   *                                rotation matrix. The given vector is not
   *                                a quaternion."
   */
  // Derived from NAIF's q2m routine
  LinearAlgebra::Matrix LinearAlgebra::toMatrix(const Vector &quaternion) {
    if (quaternion.size() != 4) {
      std::string msg = "Unable to convert the given vector to a rotation matrix. "
                    "The given vector with [" + toString((int) quaternion.size())
                    + "] components is not a quaternion.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    Vector q = quaternion;
    if ( !isUnit(quaternion) && !isZero(quaternion) ) {
      q = normalize(quaternion);
    }

    LinearAlgebra::Matrix matrix(3, 3);
    // For efficiency, we avoid duplicating calculations where possible.
    double q0q1 =  q[0] * q[1];
    double q0q2 =  q[0] * q[2];
    double q0q3 =  q[0] * q[3];

    double q1q1 =  q[1] * q[1];
    double q1q2 =  q[1] * q[2];
    double q1q3 =  q[1] * q[3];

    double q2q2 =  q[2] * q[2];
    double q2q3 =  q[2] * q[3];

    double q3q3 =  q[3] * q[3];

    matrix(0, 0) =  1.0  -  2.0 * ( q2q2 + q3q3 );
    matrix(0, 1) =          2.0 * ( q1q2 - q0q3 );
    matrix(0, 2) =          2.0 * ( q1q3 + q0q2 );

    matrix(1, 0) =          2.0 * ( q1q2 + q0q3 );
    matrix(1, 1) =  1.0  -  2.0 * ( q1q1 + q3q3 );
    matrix(1, 2) =          2.0 * ( q2q3 - q0q1 );

    matrix(2, 0) =          2.0 * ( q1q3 - q0q2 );
    matrix(2, 1) =          2.0 * ( q2q3 + q0q1 );
    matrix(2, 2) =  1.0  -  2.0 * ( q1q1 + q2q2 );

    return matrix;
  }


  /**
   * Sets the row of the given matrix to the values of the given vector.
   *
   * @param matrix The address of the matrix to be altered.
   * @param vector The vector with the new values.
   * @param rowIndex The index of the row to be altered.
   *
   * @throw IException::Programmer "Unable to set the matrix row to the given
   *                                vector. Row index is out of bounds."
   */
  void LinearAlgebra::setRow(Matrix &matrix, const Vector &vector, int rowIndex) {
    if ( (rowIndex+1 > (int) matrix.size1()) ) {
      std::string msg = "Unable to set the matrix row to the given vector. Row index "
                    + toString(rowIndex) + " is out of bounds. The given matrix only has "
                    + toString((int) matrix.size1()) + " rows." ;
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    boost::numeric::ublas::row(matrix, rowIndex) = vector;
  }


  /**
   * Sets the column of the given matrix to the values of the given vector.
   *
   * @param matrix The address of the matrix to be altered.
   * @param vector The vector with the new values.
   * @param columnIndex The index of the column to be altered.
   *
   * @throw IException::Programmer "Unable to set the matrix column to the given
   *                                vector. Column index is out of bounds."
   */
  void LinearAlgebra::setColumn(Matrix &matrix, const Vector &vector, int columnIndex) {
    if ( (columnIndex+1 > (int) matrix.size2()) ) {
      std::string msg = "Unable to set the matrix column to the given vector. Column index "
                    + toString(columnIndex) + " is out of bounds. The given matrix only has "
                    + toString((int) matrix.size1()) + " columns." ;
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    boost::numeric::ublas::column(matrix, columnIndex) = vector;
  }


  /**
   * Returns a vector whose components match those of the given matrix row.
   *
   * @param matrix The matrix to pull values from.
   * @param rowIndex The index of the matrix row to grab.
   *
   * @return @b LinearAlgebra::Vector A vector whose values match the given row
   *                                  of the given matrix.
   *
   * @throw IException::Programmer "Unable to get the matrix row to the given
   *                                vector. Row index is out of bounds."
   */
  LinearAlgebra::Vector LinearAlgebra::row(const Matrix &matrix, int rowIndex) {
    if ( (rowIndex+1 > (int) matrix.size1()) ) {
      std::string msg = "Unable to get the matrix row to the given vector. Row index "
                    + toString(rowIndex) + " is out of bounds. The given matrix only has "
                    + toString((int) matrix.size1()) + " rows." ;
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return boost::numeric::ublas::row(matrix, rowIndex);
  }


  /**
   * Returns a vector whose components match those of the given matrix column.
   *
   * @param matrix The matrix to pull values from.
   * @param columnIndex The index of the matrix column to grab.
   *
   * @return @b LinearAlgebra::Vector A vector whose values match the given
   *                                  column of the given matrix.
   *
   * @throw IException::Programmer "Unable to get the matrix column to the given
   *                                vector. Column index is out of bounds."
   */
  LinearAlgebra::Vector LinearAlgebra::column(const Matrix &matrix, int columnIndex) {
    if ( (columnIndex+1 > (int) matrix.size2()) ) {
      std::string msg = "Unable to get the matrix column to the given vector. Column index "
                    + toString(columnIndex) + " is out of bounds. The given matrix only has "
                    + toString((int) matrix.size1()) + " columns." ;
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return boost::numeric::ublas::column(matrix, columnIndex);
  }


  /**
   * Constructs a 3 dimensional vector with the given component values.
   *
   * @param v0 The first component of the vector.
   * @param v1 The second component of the vector.
   * @param v2 The third component of the vector.
   *
   * @return @b LinearAlgebra::Vector A vector containing the given values.
   *
   */
  LinearAlgebra::Vector LinearAlgebra::vector(double v0, double v1, double v2) {
    Vector v(3);
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    return v;
  }


  /**
   * Constructs a 4 dimensional vector with the given component values.
   *
   * @param v0 The first component of the vector.
   * @param v1 The second component of the vector.
   * @param v2 The third component of the vector.
   * @param v3 The fourth component of the vector.
   *
   * @return @b LinearAlgebra::Vector A vector containing the given values.
   *
   */
  LinearAlgebra::Vector LinearAlgebra::vector(double v0, double v1, double v2, double v3) {
    Vector v(4);
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    v[3] = v3;
    return v;
  }


  /**
   * Fills the first three elements of the given vector with the given values.
   *
   * @param v A pointer to a 3-dimensional vector to be filled.
   * @param v0 The first component of the vector.
   * @param v1 The second component of the vector.
   * @param v2 The third component of the vector.
   */
  void LinearAlgebra::setVec3(Vector *v, double v0, double v1, double v2) {
    (*v)[0] = v0;
    (*v)[1] = v1;
    (*v)[2] = v2;
  }


  /**
   * Fills the first four elements of the given vector with the given values.
   *
   * @param v A pointer to a 4-dimensional vector to be filled.
   * @param v0 The first component of the vector.
   * @param v1 The second component of the vector.
   * @param v2 The third component of the vector.
   * @param v3 The fourth component of the vector.
   */
  void LinearAlgebra::setVec4(Vector *v, double v0, double v1, double v2, double v3) {
    (*v)[0] = v0;
    (*v)[1] = v1;
    (*v)[2] = v2;
    (*v)[3] = v3;
  }


  /**
   * Constructs a vector of given size using the given vector and starting
   * index.
   *
   * @param v The original vector to get values from.
   * @param startIndex The index of the original vector, used to indicate
   *                   the first value that will be copied to the new vector.
   * @param size The number of elements from the original vector to copy
   *             to the new vector.
   *
   * @return @b LinearAlgebra::Vector A sub-vector containing values from
   *                                  the original vector.
   */
  LinearAlgebra::Vector LinearAlgebra::subVector(const Vector &v, int startIndex, int size) {
    LinearAlgebra::Vector sub(size);
    for (int i = 0; i < size; i++) {
      sub[i] = v[i + startIndex];
    }
    return sub;
  }


  /**
   * A global function to format a LinearAlgebra::Vector as a QString and writes
   * it to a QDebug stream.
   *
   * @see toString(LinearAlgebra::Vector)
   *
   * @param dbg The stream where the vector will be written.
   * @param vector The vector to be written.
   *
   * @return @b QDebug The stream with the QString-formatted vector.
   *
   */
  QDebug operator<<(QDebug dbg, const LinearAlgebra::Vector &vector) {
    QDebugStateSaver saver(dbg);
    dbg.noquote() << toString(vector);
    return dbg;
  }


  /**
   * A global function to format a LinearAlgebra::Matrix as a QString and
   * write it to a QDebug stream. There will be 4 spaces between each matrix
   * entry and each row is written on a new line.
   *
   * @param dbg The stream where the vector will be written.
   * @param matrix The matrix to be written.
   *
   * @return @b QDebug The stream with the QString-formatted matrix.
   *
   */
  QDebug operator<<(QDebug dbg, const LinearAlgebra::Matrix &matrix) {
    QDebugStateSaver saver(dbg);
    for (unsigned int i = 0; i < matrix.size1(); i++) {
      dbg.noquote() << "    ";
      for (unsigned int j = 0; j < matrix.size2(); j++) {
        dbg.noquote() << QString::fromStdString(toString(matrix(i, j), 15)) << "     ";
      }
     dbg.noquote() << Qt::endl;
    }
    return dbg;
  }


  /**
   * A global function to format LinearAlgebra::Vector as a QString with the
   * given precision. The string will be comma-separated entries encased by
   * parentheses.
   *
   * @param vector The vector to be converted.
   * @param precision Number of significant figures to convert.
   *
   * @return @b QString The string-formatted vector.
   *
   */
  QString toString(const LinearAlgebra::Vector &vector, int precision) {
    QString result = "( ";
    for (unsigned int i = 0; i < vector.size(); i++) {
      result += QString::fromStdString(toString(vector(i), precision));
      if (i != vector.size() - 1) result += ", ";
    }
    result += " )";
    return result;
  }
}
