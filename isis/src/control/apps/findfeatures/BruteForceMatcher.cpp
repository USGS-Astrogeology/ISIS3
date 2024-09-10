/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include "opencv2/opencv.hpp"
#include "opencv2/xfeatures2d.hpp"

#include "PvlFlatMap.h"

#include <QVariant>

#include "BruteForceMatcher.h"

namespace Isis {


  /**
   * Constructs the algorithm with default variables.
   */
  BruteForceMatcher::BruteForceMatcher() :
        DescriptorMatcherAlgorithm("BFMatcher", "DecriptorMatcher",
                                   cv::makePtr<cv::BFMatcher>()) {
    m_normTypeMap = setupNormTypeMap();
    PvlFlatMap variables;
    variables.add("NormType",   "NORM_L2");
    variables.add("CrossCheck", "false");
    m_variables.merge(variables);
  }


  /**
   * Constructs the algorithm with the input variables
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   * @param normType The type of norm to use.  Options are
   *                 2: The L1 norm,
   *                 4: The L2 norm (DEFAULT),
   *                 6: The Hamming norm, or
   *                 7: The Hamming 2 norm.
   * @param crossCheck If true; when j, the nearest neightbor to a point i, is
   *                   found it will be checked that i is the nearest neighbor
   *                   to j.  Defaults to false.
   *
   * @throws IException::User "The input value is not valid for
   *                           BruteForceMatcher's [NormType] variable"
   *
   * @see cv::NormTypes
   */
  BruteForceMatcher::BruteForceMatcher(const PvlFlatMap &cvars, const QString &config,
                                       const int normType, const bool crossCheck) :
        DescriptorMatcherAlgorithm("BFMatcher", "DecriptorMatcher",
                                   cv::makePtr<cv::BFMatcher>(normType, crossCheck), cvars) {
    m_normTypeMap = setupNormTypeMap();
    setConfig(config);
    PvlFlatMap variables;
    try {
      variables.add("NormType", m_normTypeMap.right.at(normType));
    }
    catch (std::exception &e){
      std::string msg = "The input value [" + toString(normType) +
                    "] is not valid for BruteForceMatcher's [NormType] variable";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    variables.add("CrossCheck", QString::number(crossCheck));
    m_variables.merge(variables);
  }


  /**
   * Destroys the algorithm
   */
  BruteForceMatcher::~BruteForceMatcher() { }


  /**
   * Sets up and returns the bi-directional map for norm type.
   *
   * @return @b boost:bimap<QString,int> The map between name and int for norm type.
   *
   * @see cv::NormTypes
   */
  boost::bimap<QString, int> BruteForceMatcher::setupNormTypeMap() {
    boost::bimap<QString, int> normTypeMap;

    normTypeMap.left.insert(std::pair<QString,int>("NORM_L1",       2));
    normTypeMap.left.insert(std::pair<QString,int>("NORM_L2",       4));
    normTypeMap.left.insert(std::pair<QString,int>("NORM_HAMMING",  6));
    normTypeMap.left.insert(std::pair<QString,int>("NORM_HAMMING2", 7));

    return ( normTypeMap );
  }


  /**
   * Returns a description of the algorithm.
   *
   * @return @b QString A description of the algorithm.
   */
  QString BruteForceMatcher::description() const {
    QString desc = "The OpenCV BFMatcher DescriptorMatcher matcher algorithm."
                   " See the documentation at "
     "http://docs.opencv.org/3.1.0/d3/da1/classcv_1_1BFMatcher.html";
    return (desc);
  }


  /**
   * Creates an instance of the algorithm.
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   *
   * @throws IException::User "The input value is not valid for
   *                           BruteForceMatcher's [NormType] variable"
   */
  DescriptorMatcherAlgorithm *BruteForceMatcher::create(const PvlFlatMap &vars,
                                                        const QString &config) {
    boost::bimap<QString, int> normTypeMap = setupNormTypeMap();
    const QString norm = vars.get("NormType", "NORM_L2");
    bool isInt;
    int normType = norm.toInt(&isInt);
    try {
      if (isInt) {
        normTypeMap.right.at(normType);
      }
      else {
        normType = normTypeMap.left.at(norm);
      }
    }
    catch (std::exception &e) {
        std::string msg = "The input value [" + norm.toStdString() +
                      "] is not valid for BruteForceMatcher's [NormType] variable";
        throw IException(IException::User, msg, _FILEINFO_);
    }

    const bool crossCheck = toBool(vars.get("CrossCheck", "false").toStdString());

    return ( new BruteForceMatcher(vars, config, normType, crossCheck) );
  }


  /**
   * Returns true if the algorithm has a detector.
   *
   * @return @b true if the algorithm has a detector.
   */
  bool BruteForceMatcher::hasDetector() const {
    return false;
  }


  /**
   * Returns true if the algorithm has an extractor.
   *
   * @return @b true if the algorithm has an extractor.
   */
  bool BruteForceMatcher::hasExtractor() const {
    return false;
  }


  /**
   * Returns true if the algorithm has a matcher.
   *
   * @return @b true if the algorithm has a matcher.
   */
  bool BruteForceMatcher::hasMatcher() const {
    return true;
  }


  /**
   * Returns the variables and their values used by the algorithm.
   *
   * @return @b PvlFlatMap The variables and their values as keyword, value pairs.
   */
  PvlFlatMap BruteForceMatcher::getAlgorithmVariables( ) const {
    return ( variables() );
  }


/**
 * @brief Set parameters as provided by the variables
 *
 * @param variables Container of parameters to set
 *
 * @return @b int Always -1, variables cannot be set after initialization.
 *
 * @throws IException::Programmer "BruteForceMatcher does not have the ability
 *                                 to set algorithm parameters."
 */
  int BruteForceMatcher::setAlgorithmVariables(const PvlFlatMap &variables) {
    std::string msg = "BruteForceMatcher does not have the ability to set algorithm parameters.";
    throw IException(IException::Programmer, msg, _FILEINFO_);

    return (-1);
  }

};  // namespace Isis
