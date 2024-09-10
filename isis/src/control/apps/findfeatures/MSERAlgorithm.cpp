/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <boost/foreach.hpp>
#include "opencv2/opencv.hpp"
#include "opencv2/xfeatures2d.hpp"

#include "MSERAlgorithm.h"
#include <QVariant>

namespace Isis {


  /**
   * Constructs the algorithm with default variables.
   */
  MSERAlgorithm::MSERAlgorithm() :
                 Feature2DAlgorithm("MSER", "Feature2D",
                                    MSERType::create()) {
    setupParameters();
  }


  /**
   * Constructs the algorithm with the input variables
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  MSERAlgorithm::MSERAlgorithm(const PvlFlatMap &cvars, const QString &config) :
                 Feature2DAlgorithm("MSER", "Feature2D",
                                    MSERType::create(), cvars) {
    setConfig(config);
    PvlFlatMap variables = setupParameters();
    variables.merge(cvars);
    const int delta = variables.get("Delta").toInt();
    const int minArea = variables.get("MinArea").toInt();
    const int maxArea = variables.get("MaxArea").toInt();
    const double maxVariation = variables.get("MaxVariation").toDouble();
    const double minDiversity = variables.get("MinDiversity").toDouble();
    const int maxEvolution = variables.get("MaxEvolution").toInt();
    const double areaThreshold = variables.get("AreaThreshold").toDouble();
    const double minMargin = variables.get("MinMargin").toDouble();
    const int edgeBlurSize = variables.get("EdgeBlurSize").toInt();

    m_algorithm = MSERType::create(delta, minArea, maxArea, maxVariation, minDiversity,
                                   maxEvolution, areaThreshold, minMargin, edgeBlurSize);

    m_variables.merge(variables);
  }


  /**
   * Destroys the algorithm
   */
  MSERAlgorithm::~MSERAlgorithm() { }


  /**
   * Sets up the algorithm parameters with default values.
   *
   * @return PvlFlatMap Algorithm parameters and their default values.
   */
  PvlFlatMap MSERAlgorithm::setupParameters() {
    PvlFlatMap variables;
    variables.add("Delta",         "5");
    variables.add("MinArea",       "60");
    variables.add("MaxArea",       "14400");
    variables.add("MaxVariation",  "0.25");
    variables.add("MinDiversity",  "0.2");
    variables.add("MaxEvolution",  "200");
    variables.add("AreaThreshold", "1.01");
    variables.add("MinMargin",     "0.003");
    variables.add("EdgeBlurSize",  "5");
    m_variables = variables;
    return (m_variables);
  }


  /**
   * Returns a description of the algorithm.
   *
   * @return @b QString A description of the algorithm.
   */
  QString MSERAlgorithm::description() const {
    QString desc = "The OpenCV MSER Feature2D detector/extractor algorithm."
                   " See the documentation at "
     "http://docs.opencv.org/3.1.0/d3/d28/classcv_1_1MSER.html";
    return (desc);
  }


  /**
   * Creates an instance of the algorithm.
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  Feature2DAlgorithm *MSERAlgorithm::create(const PvlFlatMap &vars, const QString &config) {
    return ( new MSERAlgorithm(vars, config) );
  }


  /**
   * Returns true if the algorithm has a detector.
   *
   * @return @b true if the algorithm has a detector.
   */
  bool MSERAlgorithm::hasDetector() const {
    return true;
  }


  /**
   * Returns true if the algorithm has an extractor.
   *
   * @return @b true if the algorithm has an extractor.
   */
  bool MSERAlgorithm::hasExtractor() const {
    return false;
  }


  /**
   * Returns true if the algorithm has a matcher.
   *
   * @return @b true if the algorithm has a matcher.
   */
  bool MSERAlgorithm::hasMatcher() const {
    return false;
  }


  /**
   * Returns the variables and their values used by the algorithm.
   *
   * @return @b PvlFlatMap The variables and their values as keyword, value pairs.
   */
  PvlFlatMap MSERAlgorithm::getAlgorithmVariables( ) const {
    return ( variables() );
  }


/**
 * @brief Set parameters as provided by the variables
 *
 * @param variables Container of parameters to set
 *
 * @return @b int Always -1, variables cannot be set after initialization.
 *
 * @throws IException::Programmer "BriefDescriptorAlgorithm does not have the ability
 *                                 to set algorithm parameters."
 */
  int MSERAlgorithm::setAlgorithmVariables(const PvlFlatMap &variables) {
    std::string msg = "MSERAlgorithm does not have the ability to set algorithm parameters.";
    throw IException(IException::Programmer, msg, _FILEINFO_);

    return (-1);
  }

};  // namespace Isis
