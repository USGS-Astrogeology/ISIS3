/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QtGlobal>
#include <QString>
#include <QSharedPointer>
#include <opencv2/opencv.hpp>
#include "GenericTransform.h"
#include "IException.h"

namespace Isis {
/** Generic constructor is simply an identity transform */
GenericTransform::GenericTransform() : ImageTransform("GenericTransform"),
                                       m_matrix(), m_inverse(), m_size(0,0) {
  setMatrix(cv::Mat::eye(3, 3, CV_64FC1));
}

/** Named generic identity matrix */
GenericTransform::GenericTransform(const QString &name) : ImageTransform(name),
                                                          m_matrix(),
                                                          m_inverse(),
                                                          m_size(0,0) {
  setMatrix(cv::Mat::eye(3, 3, CV_64FC1));
}

/** Construct named transform with a 3x3 transformation matrix */
GenericTransform::GenericTransform(const QString &name, const cv::Mat &matrix) :
                                   ImageTransform(name), m_matrix(), m_inverse(),
                 m_size(0,0) {
  setMatrix( matrix );
}

/** Construct named transfrom with 3x3 matrix with a size specification */
GenericTransform::GenericTransform(const QString &name, const cv::Mat &matrix,
                                   const cv::Size &imSize) :
                                   ImageTransform(name), m_matrix(),
                                   m_inverse(),
                                   m_size(imSize) {
  setMatrix( matrix );
}

/** Construct named transfrom with 3x3 matrix with a subarea specification */
GenericTransform::GenericTransform(const QString &name, const cv::Mat &matrix,
                                   const GenericTransform::RectArea &subarea) :
                                   ImageTransform(name), m_matrix(),
                                   m_inverse(), m_size(subarea.size()) {
  cv::Mat tmatrix = ImageTransform::translation(-subarea.x, -subarea.y);
  setMatrix( tmatrix * matrix );
  setSize( subarea.size() );
}

/** Destructor  */
GenericTransform::~GenericTransform() { }

/** Return transformation matrix */
cv::Mat GenericTransform::getMatrix() const {
  return ( m_matrix );
}

/** Return inverse transform matrix */
cv::Mat GenericTransform::getInverse() const {
  return ( m_inverse );
}

/** Return the resulting size of the transformed image */
cv::Size GenericTransform::getSize(const cv::Mat &image) const {
  if ( 0 == m_size.width ) {  return ( image.size() );  }
  return (m_size);
}

/** Transform the image matrix using the matrix and size constraints */
cv::Mat GenericTransform::render(const cv::Mat &image) const {
  cv::Mat result;
  warpPerspective(image, result, getMatrix(), getSize(image),
                  cv::INTER_LINEAR);
#if 0
  // Gots to be run in the GUI in order for this to work!!
  cv::namedWindow("Original", CV_WINDOW_AUTOSIZE);
  cv::imshow("Original", image);

  cv::namedWindow("Transformed", CV_WINDOW_AUTOSIZE);
  cv::imshow("Transformed", result);

  cv::waitKey(0);

#endif
  return ( result );
}

/**
 * @brief Compute the forward transform of a point
 *
 * This method applies the matrix to a point using a perspective transform.
 *
 * @param point   Point to transform
 *
 * @return cv::Point2f Rsulting transformed point using the matrix
 */
cv::Point2f GenericTransform::forward(const cv::Point2f &point) const {
  std::vector<cv::Point2f> src, dst(1);
  src.push_back(point);
  perspectiveTransform(src, dst, getMatrix());
  return ( dst[0] );
}

/**
 * @brief Compute the inverse transform of a point
 *
 * @param point Point to invert
 *
 * @return cv::Point2f Resulting inverse tranform
 */
cv::Point2f GenericTransform::inverse(const cv::Point2f &point) const {
  std::vector<cv::Point2f> src, dst(1);
  src.push_back(point);
  perspectiveTransform( src, dst, getInverse() );
  return ( dst[0] );
}

/**
 * @brief Compute inverse with validation
 *
 * OpenCV may silenty return an empty matrix using the inv() method. There is a
 * safer method that is applied here that will test if the matrix is invertable.
 * If its not invertable and verify == true, then an exception is thrown.
 *
 * @internal
 * @author 2021-10-03 Kris J. Becker
 * @history 2022-02-07 Kris Becker Modifications in response to code review
 *
 * @param matrix Square matrix to invert
 * @param verify If true will throw an exception if not invertable unless
 *                 an actual error does occur, which throws unconditionally
 *
 * @return cv::Mat Inverted matrix
 */
cv::Mat GenericTransform::computeInverse(const cv::Mat &matrix,
                                         const bool verify) {
  cv::Mat inverse;
  try {
    double result = cv::invert(matrix, inverse);
    if ( verify ) {
      if ( 0.0 == result ) {
        std::string msg = "Transformation matrix is not invertable";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }
  }
 catch ( std::exception &e ) {
    // This will also catch any ISIS error
    std::string msg = "Matrix inversion error: " + std::string(e.what());
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }

  return ( inverse );
}


/** Set the forward matrix */
void GenericTransform::setMatrix(const cv::Mat &matrix) {
  m_matrix = matrix;
  m_inverse = calculateInverse(matrix);
}

/** Set the inverse matrix */
void GenericTransform::setInverse(const cv::Mat &matrix) {
  m_inverse = matrix;
  return;
}

/**
 * @brief Calculate the inverse transform from the forward matrix
 *
 * This method will compute the inverse of the matrix and return the result. If
 * the matrix is not invertable an exception is thrown unless verify == false.
 * If verify == false, it is up to the caller to test the returned matrix (which
 * may be filled with 0's).
 *
 * @param matrix Matrix to invert
 *
 * @return cv:Mat Inverted matrix
 */
cv::Mat GenericTransform::calculateInverse(const cv::Mat &matrix,
                                           const bool verify) {
  return ( computeInverse(matrix, verify) );
}

/** Set the size of the transformed image */
void GenericTransform::setSize(const cv::Size &mSize) {
  m_size = mSize;
  return;
}

}  // namespace Isis
