/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QString>
#include <QSharedPointer>
#include <opencv2/opencv.hpp>
#include "GenericTransform.h"

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
  return ( m_inverse);
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

/** Calculate the inverse transform from the forward matrix */
cv::Mat GenericTransform::calculateInverse(const cv::Mat &matrix) {
  if (matrix.empty()) {  // inverting an empty matrix causes a segfault
    QString msg = "Can't invert empty matrix.";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }

  return ( matrix.inv() );
}

/** Set the size of the transformed image */
void GenericTransform::setSize(const cv::Size &mSize) {
  m_size = mSize;
  return;
}

}  // namespace Isis
