/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include <boost/foreach.hpp>
#include "opencv2/opencv.hpp"
#include "opencv2/xfeatures2d.hpp"

#include "BriefDescriptorAlgorithm.h"
#include <QVariant>

namespace Isis {


  /**
   * Constructs the algorithm with default variables.
   */
  BriefDescriptorAlgorithm::BriefDescriptorAlgorithm() :
                 Feature2DAlgorithm("Brief", "Feature2D",
                                    BriefType::create()) {
    setupParameters();
  }


  /**
   * Constructs the algorithm with the input variables
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  BriefDescriptorAlgorithm::BriefDescriptorAlgorithm(const PvlFlatMap &cvars,
                                                     const QString &config) :
                           Feature2DAlgorithm("Brief", "Feature2D",
                                               BriefType::create(), cvars) {
    setConfig(config);
    PvlFlatMap variables = setupParameters();
    variables.merge(cvars);
    int bytes = toInt(variables.get("Bytes"));
    bool useOrientation = toBool(variables.get("UseOrientation"));

    m_algorithm = BriefType::create(bytes, useOrientation);

    m_variables.merge(variables);
  }


  /**
   * Destroys the algorithm
   */
  BriefDescriptorAlgorithm::~BriefDescriptorAlgorithm() { }


  /**
   * Sets up the algorithm parameters with default values.
   *
   * @return PvlFlatMap Algorithm parameters and their default values.
   */
  PvlFlatMap BriefDescriptorAlgorithm::setupParameters() {
    PvlFlatMap variables;
    variables.add("Bytes",          "32");
    variables.add("UseOrientation", "true");
    m_variables = variables;
    return (m_variables);
  }


  /**
   * Returns a description of the algorithm.
   *
   * @return @b QString A description of the algorithm.
   */
  QString BriefDescriptorAlgorithm::description() const {
    QString desc = "The OpenCV simple blob detection algorithm."
                   " See the documentation at "
     "http://docs.opencv.org/3.1.0/d0/d7a/classcv_1_1SimpleBlobDetector.html";
    return (desc);
  }


  /**
   * Creates an instance of the algorithm.
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  Feature2DAlgorithm *BriefDescriptorAlgorithm::create(const PvlFlatMap &vars,
                                                       const QString &config) {

    return ( new BriefDescriptorAlgorithm(vars, config) );
  }



  /**
   * Returns true if the algorithm has a detector.
   *
   * @return @b true if the algorithm has a detector.
   */
  bool BriefDescriptorAlgorithm::hasDetector() const {
    return false;
  }


  /**
   * Returns true if the algorithm has an extractor.
   *
   * @return @b true if the algorithm has an extractor.
   */
  bool BriefDescriptorAlgorithm::hasExtractor() const {
    return true;
  }


  /**
   * Returns true if the algorithm has a matcher.
   *
   * @return @b true if the algorithm has a matcher.
   */
  bool BriefDescriptorAlgorithm::hasMatcher() const {
    return false;
  }


  /**
   * Returns the variables and their values used by the algorithm.
   *
   * @return @b PvlFlatMap The variables and their values as keyword, value pairs.
   */
  PvlFlatMap BriefDescriptorAlgorithm::getAlgorithmVariables( ) const{
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
  int BriefDescriptorAlgorithm::setAlgorithmVariables(const PvlFlatMap &variables) {
    std::string msg = "BriefDescriptorAlgorithm does not have the ability to set algorithm parameters.";
    throw IException(IException::Programmer, msg, _FILEINFO_);

    return (-1);
  }

};  // namespace Isis
