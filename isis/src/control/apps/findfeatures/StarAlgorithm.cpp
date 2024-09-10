/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include <boost/foreach.hpp>
#include "opencv2/opencv.hpp"
#include "opencv2/xfeatures2d.hpp"

#include "StarAlgorithm.h"
#include <QVariant>

namespace Isis {


  /**
   * Constructs the algorithm with default variables.
   */
  StarAlgorithm::StarAlgorithm() :
                 Feature2DAlgorithm("Star", "Feature2D",
                                    StarType::create()) {
    setupParameters();
  }


  /**
   * Constructs the algorithm with the input variables
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  StarAlgorithm::StarAlgorithm(const PvlFlatMap &cvars, const QString &config) :
                 Feature2DAlgorithm("Star", "Feature2D",
                                     StarType::create(), cvars) {
    setConfig(config);
    PvlFlatMap variables = setupParameters();
    variables.merge(cvars);
    const int maxSize = variables.get("MaxSize").toInt();
    const int responseThreshold = variables.get("ResponseThreshold").toInt();
    const int lineThresholdProjected = variables.get("LineThresholdProjected").toInt();
    const int lineThresholdBinarized = variables.get("LineThresholdBinarized").toInt();
    const int SuppressNonmaxSize = variables.get("SuppressNonmaxSize").toInt();

    m_algorithm = StarType::create(maxSize, responseThreshold, lineThresholdProjected,
                                   lineThresholdBinarized, SuppressNonmaxSize);

    m_variables.merge(variables);
  }


  /**
   * Destroys the algorithm
   */
  StarAlgorithm::~StarAlgorithm() { }


  /**
   * Sets up the algorithm parameters with default values.
   *
   * @return PvlFlatMap Algorithm parameters and their default values.
   */
  PvlFlatMap StarAlgorithm::setupParameters() {
    PvlFlatMap variables;
    variables.add("MaxSize",                "45");
    variables.add("ResponseThreshold",      "30");
    variables.add("LineThresholdProjected", "10");
    variables.add("LineThresholdBinarized", "8");
    variables.add("SuppressNonmaxSize",     "5");
    m_variables = variables;
    return (m_variables);
  }


  /**
   * Returns a description of the algorithm.
   *
   * @return @b QString A description of the algorithm.
   */
  QString StarAlgorithm::description() const {
    QString desc = "The OpenCV Star Feature2D detector/extractor algorithm."
                   " See the documentation at "
     "http://docs.opencv.org/3.1.0/d6/d36/classcv_1_1xfeatures2d_1_1Star.html";
    return (desc);
  }


  /**
   * Creates an instance of the algorithm.
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  Feature2DAlgorithm *StarAlgorithm::create(const PvlFlatMap &vars, const QString &config) {

    return ( new StarAlgorithm(vars, config) );
  }


  /**
   * Returns true if the algorithm has a detector.
   *
   * @return @b true if the algorithm has a detector.
   */
  bool StarAlgorithm::hasDetector() const {
    return true;
  }


  /**
   * Returns true if the algorithm has an extractor.
   *
   * @return @b true if the algorithm has an extractor.
   */
  bool StarAlgorithm::hasExtractor() const {
    return false;
  }


  /**
   * Returns true if the algorithm has a matcher.
   *
   * @return @b true if the algorithm has a matcher.
   */
  bool StarAlgorithm::hasMatcher() const {
    return false;
  }


  /**
   * Returns the variables and their values used by the algorithm.
   *
   * @return @b PvlFlatMap The variables and their values as keyword, value pairs.
   */
  PvlFlatMap StarAlgorithm::getAlgorithmVariables( ) const {
    return ( variables() );
  }


 /**
  * @brief Set parameters as provided by the variables
  *
  * @param variables Container of parameters to set
  *
  * @return @b int Always -1, variables cannot be set after initialization.
  *
  * @throws IException::Programmer "StarAlgorithm does not have the ability
  *                                 to set algorithm parameters."
  */
  int StarAlgorithm::setAlgorithmVariables(const PvlFlatMap &variables) {
    std::string msg = "StarAlgorithm does not have the ability to set algorithm parameters.";
    throw IException(IException::Programmer, msg, _FILEINFO_);

    return (-1);
  }

};  // namespace Isis
