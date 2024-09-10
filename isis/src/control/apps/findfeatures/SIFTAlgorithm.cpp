/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include <boost/foreach.hpp>
#include "opencv2/opencv.hpp"
#include "opencv2/xfeatures2d.hpp"

#include "SIFTAlgorithm.h"
#include <QVariant>

namespace Isis {

  /**
   * Constructs an empty SiftAlgorithm with default variables.
   */
  SIFTAlgorithm::SIFTAlgorithm() :
                 Feature2DAlgorithm( "SIFT", "Feature2D",
                                      SIFTType::create() ) {
    setupParameters();
  }


  /**
   * Constructs a SIFTAlgorithm with input variables.
   *
   * @param cvars Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  SIFTAlgorithm::SIFTAlgorithm(const PvlFlatMap &cvars, const QString &config) :
                 Feature2DAlgorithm("SIFT", "Feature2D",
                                     SIFTType::create(), cvars) {
    setConfig(config);
    PvlFlatMap variables = setupParameters();
    variables.merge(cvars);
    const int nfeatures = (variables.get("nfeatures").toInt());
    const int nOctaveLayers = (variables.get("nOctaveLayers").toInt());
    const double contrastThreshold = (variables.get("constrastThreshold").toDouble());
    const double edgeThreshold = (variables.get("edgeThreshold").toDouble());
    const double sigma = (variables.get("sigma").toDouble());

    m_algorithm = SIFTType::create(nfeatures, nOctaveLayers, contrastThreshold,
                                   edgeThreshold, sigma);

    m_variables.merge(variables);
  }


  /**
   * Destroys the algorithm
   */
  SIFTAlgorithm::~SIFTAlgorithm() { }


  /**
   * Sets up the algorithm parameters with default values.
   *
   * @return PvlFlatMap Algorithm parameters and their default values.
   */
  PvlFlatMap SIFTAlgorithm::setupParameters() {
    PvlFlatMap variables;
    variables.add("nfeatures",          "0");
    variables.add("nOctaveLayers",      "3");
    variables.add("constrastThreshold", "0.04");
    variables.add("edgeThreshold",      "10");
    variables.add("sigma",              "1.6");
    m_variables = variables;
    return (m_variables);
  }


  /**
   * Returns a description of the algorithm.
   *
   * @return @b QString A description of the algorithm.
   */
  QString SIFTAlgorithm::description() const {
    QString desc = "The OpenCV SIFT Feature2D detector/extractor algorithm."
                   " See the documentation at "
     "http://docs.opencv.org/3.1.0/d5/d3c/classcv_1_1xfeatures2d_1_1SIFT.html";
    return (desc);
  }


  /**
   * Creates and returns an intance of SIFTAlgorithm.
   *
   * @param vars PvlFlatMap containing algorithm parameters and their values
   * @param config A configuration string input by the user
   *
   * @return @b Feature2DAlgorithm An instance of SIFTAlgorithm
   */
  Feature2DAlgorithm *SIFTAlgorithm::create(const PvlFlatMap &vars, const QString &config) {
    return ( new SIFTAlgorithm(vars, config) );
  }



  /**
   * Returns true if the algorithm has a detector.
   *
   * @return @b true if the algorithm has a detector.
   */
  bool SIFTAlgorithm::hasDetector() const {
    return true;
  }


  /**
   * Returns true if the algorithm has an extractor.
   *
   * @return @b true if the algorithm has an extractor.
   */
  bool SIFTAlgorithm::hasExtractor() const {
    return true;
  }


  /**
   * Returns true if the algorithm has a matcher.
   *
   * @return @b true if the algorithm has a matcher.
   */
  bool SIFTAlgorithm::hasMatcher() const {
    return false;
  }


/**
 * Get and return SIFT's parameters and what they're set to as a PvlFlatMap.
 *
 * @return @b PvlFlatMap A PvlFlatMap of SIFT's currently set variables and their values.
 */
  PvlFlatMap SIFTAlgorithm::getAlgorithmVariables( ) const {
    return ( variables() );
  }


/**
 * @brief Set parameters as provided by the variables, does not work for the
 * SIFT algorithm in OpenCV3, so calling this will throw an exception.
 * Always returns -1.
 *
 * @param variables Container of parameters to set
 *
 * @throws IException::Programmer "SIFT does not have the ability to set algorithm parameters.";
 *
 * @return @b int -1 Cannot set the Algorithm Variables
 */
  int SIFTAlgorithm::setAlgorithmVariables(const PvlFlatMap &variables) {
    std::string message = "SIFT does not have the ability to set algorithm parameters.";
    throw IException(IException::Programmer, message, _FILEINFO_);

    return (-1);
  }

};  // namespace Isis
