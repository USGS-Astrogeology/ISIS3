/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <boost/foreach.hpp>
#include "opencv2/opencv.hpp"
#include "opencv2/xfeatures2d.hpp"

#include "SURFAlgorithm.h"
#include <QVariant>

namespace Isis {

  /**
   * Constructs a default SURFAlgorithm with default variables.
   */
  SURFAlgorithm::SURFAlgorithm() : Feature2DAlgorithm("SURF", "Feature2D", SURFType::create()) {
    m_variables.merge( getAlgorithmVariables() );
  }


/**
 * Constructs a SURFAlgorithm with input variables.
 *
 * @param cvars Variables that are not included will be set to their default.
 * @param config The config string used to construct cvars.
 */
  SURFAlgorithm::SURFAlgorithm(const PvlFlatMap &cvars, const QString &config) :
                               Feature2DAlgorithm("SURF", "Feature2D",
                                                  SURFType::create()) {
    setConfig(config);
    setAlgorithmVariables(cvars);
    PvlFlatMap defaults = getAlgorithmVariables();
    defaults.merge(cvars);
    m_variables.merge(defaults);
  }


/**
 * Default Destructor
 */
  SURFAlgorithm::~SURFAlgorithm() { }


/**
 * Returns a description of the DaisyAlgorithm.
 *
 * @return @b QString A description of the algorithm.
 */
  QString SURFAlgorithm::description() const {
    QString desc = "The OpenCV SURF Feature2D detector/extractor algorithm."
                   " See the documentation at "
     "http://docs.opencv.org/3.1.0/d5/df7/classcv_1_1xfeatures2d_1_1SURF.html";
    return (desc);
  }


/**
 * Creates and returns an instance of SURFAlgorithm.
 *
 * @param vars PvlFlatMap containing algorithm parameters and their values
 * @param config A configuration string input by the user
 *
 * @return Feature2DAlgorithm
 */
  Feature2DAlgorithm *SURFAlgorithm::create(const PvlFlatMap &vars, const QString &config) {
    return ( new SURFAlgorithm(vars, config) );
  }


  /**
   * Returns true if the algorithm has a detector.
   *
   * @return @b true if the algorithm has a detector.
   */
  bool SURFAlgorithm::hasDetector() const {
    return true;
  }


  /**
   * Returns true if the algorithm has an extractor.
   *
   * @return @b true if the algorithm has an extractor.
   */
  bool SURFAlgorithm::hasExtractor() const {
    return true;
  }


  /**
   * Returns true if the algorithm has a matcher.
   *
   * @return @b true if the algorithm has a matcher.
   */
  bool SURFAlgorithm::hasMatcher() const {
    return false;
  }



/**
 * @brief Retreive all SURF algorithm variable defaults and populate container
 *
 * This method will retreive the current values of all the variables as they
 * are currently set in the algorithm.
 *
 * Typically, this is called upon instantiation of the algorithm which provides
 * the default state of the variables. However, it is reentrant such that it
 * will return the current state of all the variables.
 *
 * @author 2016-12-07 Kris Becker
 *
 * @return PvlFlatMap Container with all the SURF variables and values
 */
  PvlFlatMap SURFAlgorithm::getAlgorithmVariables( ) const {
    PvlFlatMap variables;
    SURFPtr v_ref = m_algorithm.dynamicCast<SURFType>();
    variables.add("HessianThreshold", QString::number(v_ref->getHessianThreshold()));
    variables.add("NOctaves",  QString::number(v_ref->getNOctaves()));
    variables.add("NOctaveLayers",  QString::number(v_ref->getNOctaveLayers()));
    variables.add("Extended", QString::number(v_ref->getExtended()));
    variables.add("Upright", QString::number(v_ref->getUpright()));
    return (variables);
  }

/**
 * @brief Set parameters as provided by the variables
 *
 * @author  2016-12-06 Kris Becker
 *
 * @param variables Container of parameters to set in the algorithm
 *
 * @return int Number of variables actually set
 */
  int SURFAlgorithm::setAlgorithmVariables(const PvlFlatMap &variables) {

    SURFPtr v_ref = m_algorithm.dynamicCast<SURFType>();
    int nset(0);
    if ( variables.exists("HessianThreshold") ) {
      v_ref->setHessianThreshold(variables.get("HessianThreshold").toDouble());
      nset++;
    }

    if ( variables.exists("NOctaves") ) {
      v_ref->setNOctaves(variables.get("NOctaves").toDouble());
      nset++;
    }

    if ( variables.exists("NOctaveLayers") ) {
      v_ref->setNOctaveLayers(variables.get("NOctaveLayers").toDouble());
      nset++;
    }

    if ( variables.exists("Extended") ) {
      v_ref->setExtended(toBool(variables.get("Extended").toStdString()));
      nset++;
    }

    if ( variables.exists("Upright") ) {
      v_ref->setUpright(toBool(variables.get("Upright").toStdString()));
      nset++;
    }

    return (nset);
  }

};
