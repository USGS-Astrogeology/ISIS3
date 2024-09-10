/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include <boost/foreach.hpp>
#include "opencv2/opencv.hpp"
#include "opencv2/xfeatures2d.hpp"

#include "GFTTAlgorithm.h"
#include <QVariant>

namespace Isis {


  /**
   * Constructs the algorithm with default variables.
   */
  GFTTAlgorithm::GFTTAlgorithm() :
                 Feature2DAlgorithm("GFTT", "Feature2D",
                                    GFTTType::create()) {
    m_variables.merge(getAlgorithmVariables());
  }


  /**
   * Constructs the algorithm with the input variables
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  GFTTAlgorithm::GFTTAlgorithm(const PvlFlatMap &cvars, const QString &config) :
                  Feature2DAlgorithm("GFTT", "Feature2D",
                                     GFTTType::create(), cvars) {
    setConfig(config);
    setAlgorithmVariables(cvars);
    m_variables.merge(getAlgorithmVariables());
  }


  /**
   * Destroys the algorithm
   */
  GFTTAlgorithm::~GFTTAlgorithm() { }


  /**
   * Returns a description of the algorithm.
   *
   * @return @b QString A description of the algorithm.
   */
  QString GFTTAlgorithm::description() const {
    QString desc = "The OpenCV GFTT Feature2D detector/extractor algorithm."
                   " See the documentation at "
     "http://docs.opencv.org/3.1.0/df/d21/classcv_1_1GFTTDetector.html";
    return (desc);
  }


  /**
   * Creates an instance of the algorithm.
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  Feature2DAlgorithm *GFTTAlgorithm::create(const PvlFlatMap &vars, const QString &config) {
    return ( new GFTTAlgorithm(vars, config) );
  }


  /**
   * Returns true if the algorithm has a detector.
   *
   * @return @b true if the algorithm has a detector.
   */
  bool GFTTAlgorithm::hasDetector() const {
    return true;
  }


  /**
   * Returns true if the algorithm has an extractor.
   *
   * @return @b true if the algorithm has an extractor.
   */
  bool GFTTAlgorithm::hasExtractor() const {
    return false;
  }


  /**
   * Returns true if the algorithm has a matcher.
   *
   * @return @b true if the algorithm has a matcher.
   */
  bool GFTTAlgorithm::hasMatcher() const {
    return false;
  }


  /**
   * Returns the variables and their values used by the algorithm.
   *
   * @return @b PvlFlatMap The variables and their values as keyword, value pairs.
   */
  PvlFlatMap GFTTAlgorithm::getAlgorithmVariables( ) const {
    GFTTPtr algorithm = m_algorithm.dynamicCast<GFTTType>();
    PvlFlatMap variables;
    variables.add("MaxFeatures",    QString::number(algorithm->getMaxFeatures()));
    variables.add("QualityLevel",   QString::number(algorithm->getQualityLevel()));
    variables.add("MinDistance",    QString::number(algorithm->getMinDistance()));
    variables.add("BlockSize",      QString::number(algorithm->getBlockSize()));
    variables.add("HarrisDetector", QString::number(algorithm->getHarrisDetector()));
    variables.add("K",              QString::number(algorithm->getK()));
    return (variables);
  }


/**
 * @brief Set parameters as provided by the variables
 *
 * @param variables Container of parameters to set
 *
 * @return int Number of variables actually set
 */
  int GFTTAlgorithm::setAlgorithmVariables(const PvlFlatMap &variables) {

    GFTTPtr algorithm = m_algorithm.dynamicCast<GFTTType>();

    int nset(0);
    if ( variables.exists("MaxFeatures") ) {
      algorithm->setMaxFeatures(variables.get("MaxFeatures").toInt());
      nset++;
    }

    if ( variables.exists("QualityLevel") ) {
      algorithm->setQualityLevel(variables.get("QualityLevel").toDouble());
      nset++;
    }

    if ( variables.exists("MinDistance") ) {
      algorithm->setMinDistance(variables.get("MinDistance").toDouble());
      nset++;
    }

    if ( variables.exists("BlockSize") ) {
      algorithm->setBlockSize(variables.get("BlockSize").toInt());
      nset++;
    }

    if ( variables.exists("HarrisDetector") ) {
      algorithm->setHarrisDetector(toBool(variables.get("HarrisDetector").toStdString()));
      nset++;
    }

    if ( variables.exists("K") ) {
      algorithm->setK(variables.get("K").toDouble());
      nset++;
    }

    return (nset);
  }

};  // namespace Isis
