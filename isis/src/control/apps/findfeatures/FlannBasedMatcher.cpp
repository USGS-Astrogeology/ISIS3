/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include <boost/foreach.hpp>

#include "opencv2/opencv.hpp"
#include "opencv2/xfeatures2d.hpp"

#include "PvlFlatMap.h"

#include <QVariant>

#include "FlannBasedMatcher.h"

namespace Isis {


  /**
   * Constructs the algorithm with default variables.
   */
  FlannBasedMatcher::FlannBasedMatcher() :
        DescriptorMatcherAlgorithm("FlannBasedMatcher", "DecriptorMatcher",
                                   cv::makePtr<cv::FlannBasedMatcher>()) {
    PvlFlatMap variables;
    variables.add("Checks",  "32");
    variables.add("Epsilon", "0.0");
    variables.add("Sorted",  "true");
    m_variables.merge(variables);
  }


  /**
   * Constructs the algorithm with the input variables
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  FlannBasedMatcher::FlannBasedMatcher(const PvlFlatMap &cvars, const QString &config,
                                       const int checks, const float epsilon, const bool sorted) :
        DescriptorMatcherAlgorithm("FlannBasedMatcher", "DecriptorMatcher",
                        cv::makePtr<cv::FlannBasedMatcher>(
                                  cv::makePtr<cv::flann::SearchParams>(checks, epsilon, sorted))) {
    setConfig(config);
    PvlFlatMap variables;
    variables.add("Checks",  toString(checks));
    variables.add("Epsilon", toString(epsilon));
    variables.add("Sorted",  toString(sorted));
    m_variables.merge(variables);
  }


  /**
   * Destroys the algorithm
   */
  FlannBasedMatcher::~FlannBasedMatcher() { }


  /**
   * Returns a description of the algorithm.
   *
   * @return @b QString A description of the algorithm.
   */
  QString FlannBasedMatcher::description() const {
    QString desc = "The OpenCV FlannBasedMatcher DescriptorMatcher matcher algorithm."
                   " See the documentation at "
     "http://docs.opencv.org/3.1.0/dc/de2/classcv_1_1FlannBasedMatcher.html";
    return (desc);
  }


  /**
   * Creates an instance of the algorithm.
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  DescriptorMatcherAlgorithm *FlannBasedMatcher::create(const PvlFlatMap &vars,
                                                        const QString &config) {
    const int checks = toInt(vars.get("Checks", "32"));
    const float epsilon = vars.get("Epsilon", "0.0").toFloat();
    const bool sorted = toBool(vars.get("Sorted", "true"));

    return ( new FlannBasedMatcher(vars, config, checks, epsilon, sorted) );
  }


  /**
   * Returns true if the algorithm has a detector.
   *
   * @return @b true if the algorithm has a detector.
   */
  bool FlannBasedMatcher::hasDetector() const {
    return false;
  }


  /**
   * Returns true if the algorithm has an extractor.
   *
   * @return @b true if the algorithm has an extractor.
   */
  bool FlannBasedMatcher::hasExtractor() const {
    return false;
  }


  /**
   * Returns true if the algorithm has a matcher.
   *
   * @return @b true if the algorithm has a matcher.
   */
  bool FlannBasedMatcher::hasMatcher() const {
    return true;
  }


  /**
   * Returns the variables and their values used by the algorithm.
   *
   * @return @b PvlFlatMap The variables and their values as keyword, value pairs.
   */
  PvlFlatMap FlannBasedMatcher::getAlgorithmVariables( ) const {
    return ( variables() );
  }


/**
 * @brief Set parameters as provided by the variables
 *
 * @param variables Container of parameters to set
 *
 * @return @b int Always -1, variables cannot be set after initialization.
 *
 * @throws IException::Programmer "FlannBasedMatcher does not have the ability
 *                                 to set algorithm parameters."
 */
  int FlannBasedMatcher::setAlgorithmVariables(const PvlFlatMap &variables) {
    std::string msg = "FlannBasedMatcher does not have the ability to set algorithm parameters.";
    throw IException(IException::Programmer, msg, _FILEINFO_);

    return (-1);
  }

};  // namespace Isis
