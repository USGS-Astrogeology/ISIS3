/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "MSERExtractor.h"
#include "IException.h"

#include <QString>

namespace Isis {

  /**
   * Always throws an exception.
   *
   * @param image input image
   * @param msers resulting list of point sets
   * @param bboxes bounding boxes
   *
   * @throws IException::Programmer "ISIS does not support this method for the OpenCV MSER
   *                                 algorithm."
   */
  void MSERExtractor::detectRegions (cv::InputArray image,
                                     std::vector< std::vector< cv::Point > > &msers,
                                     std::vector< cv::Rect > &bboxes)  {
    std::string mess = "ISIS does not support this method for the OpenCV MSER algorithm.";
    throw IException(IException::Programmer, mess, _FILEINFO_);
  }


  /**
   * Always throws an exception.
   *
   * @return int (-1)
   *
   * @throws IException::Programmer "ISIS does not support this method for the OpenCV MSER
   *                                 algorithm."
   */
  int MSERExtractor::getDelta () const {
    std::string mess = "ISIS does not support this method for the OpenCV MSER algorithm.";
    throw IException(IException::Programmer, mess, _FILEINFO_);
  }


  /**
   * Always throws an exception.
   *
   * @return int (-1)
   *
   * @throws IException::Programmer "ISIS does not support this method for the OpenCV MSER
   *                                algorithm."
   */
  int MSERExtractor::getMaxArea () const {
    std::string mess = "ISIS does not support this method for the OpenCV MSER algorithm.";
    throw IException(IException::Programmer, mess, _FILEINFO_);
  }


  /**
   * Always throws an exception.
   *
   * @return int (-1)
   *
   * @throws IException::Programmer "ISIS does not support this method for the OpenCV MSER
   *                                 algorithm."
   */
  int MSERExtractor::getMinArea () const {
    std::string mess = "ISIS does not support this method for the OpenCV MSER algorithm.";
    throw IException(IException::Programmer, mess, _FILEINFO_);
  }


  /**
   * Always throws an exception.
   *
   * @return bool false
   *
   * @throws IException::Programmer "ISIS does not support this method for the OpenCV MSER
   *                                 algorithm."
   */
  bool MSERExtractor::getPass2Only () const {
    std::string mess = "ISIS does not support this method for the OpenCV MSER algorithm.";
    throw IException(IException::Programmer, mess, _FILEINFO_);
  }


  /**
   * Always throws an exception.
   *
   * @param delta input delta to set
   *
   * @throws IException::Programmer "ISIS does not support this method for the OpenCV MSER
   *                                 algorithm."
   */
  void MSERExtractor::setDelta (int delta) {
    std::string mess = "ISIS does not support this method for the OpenCV MSER algorithm.";
    throw IException(IException::Programmer, mess, _FILEINFO_);
  }


  /**
   * Always throws an exception.
   *
   * @param maxArea input maximum area
   *
   * @throws IException::Programmer "ISIS does not support this method for the OpenCV MSER
   *                                 algorithm."
   */
  void MSERExtractor::setMaxArea (int maxArea) {
    std::string mess = "ISIS does not support this method for the OpenCV MSER algorithm.";
    throw IException(IException::Programmer, mess, _FILEINFO_);
  }


  /**
   * Always throws an exception.
   *
   * @param minArea input minimum area
   *
   * @throws IException::Programmer "ISIS does not support this method for the OpenCV MSER
   *                                 algorithm."
   */
  void MSERExtractor::setMinArea (int minArea) {
    std::string mess = "ISIS does not support this method for the OpenCV MSER algorithm.";
    throw IException(IException::Programmer, mess, _FILEINFO_);
  }


  /**
   * Always throws an exception.
   *
   * @param f input value
   *
   * @throws IException::Programmer "ISIS does not support this method for the OpenCV MSER
   *                                 algorithm."
   */
  void MSERExtractor::setPass2Only (bool f) {
    std::string mess = "ISIS does not support this method for the OpenCV MSER algorithm.";
    throw IException(IException::Programmer, mess, _FILEINFO_);
  }
};
