/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <boost/foreach.hpp>
#include "opencv2/opencv.hpp"

#include "FASTAlgorithm.h"

#include <QVariant>

namespace Isis {


  /**
   * Constructs the algorithm with default variables.
   */
  FASTAlgorithm::FASTAlgorithm() :
                 Feature2DAlgorithm("FAST", "Feature2D",
                                    FASTType::create()) {
    setupTypeMap();
    m_variables.merge( getAlgorithmVariables() );
  }


  /**
   * Constructs the algorithm with the input variables
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  FASTAlgorithm::FASTAlgorithm(const PvlFlatMap &cvars, const QString &config) :
                  Feature2DAlgorithm("FAST", "Feature2D",
                                     FASTType::create(), cvars) {
    setupTypeMap();
    setConfig(config);
    setAlgorithmVariables(cvars);
    m_variables.merge(getAlgorithmVariables());
  }


  /**
   * Destroys the algorithm
   */
  FASTAlgorithm::~FASTAlgorithm() { }


  /**
   * Returns a description of the algorithm.
   *
   * @return @b QString A description of the algorithm.
   */
  QString FASTAlgorithm::description() const {
    QString desc = "The OpenCV FAST Feature2D detector/extractor algorithm."
                   " See the documentation at "
     "http://docs.opencv.org/3.1.0/df/d74/classcv_1_1FASTFeatureDetector.html";
    return (desc);
  }


  /**
   * Fills the map for converting the type variable.
   */
  void FASTAlgorithm::setupTypeMap() {
    m_typeMap.left.insert(std::pair<QString,int>("TYPE_5_8",  0));
    m_typeMap.left.insert(std::pair<QString,int>("TYPE_7_12", 1));
    m_typeMap.left.insert(std::pair<QString,int>("TYPE_9_16", 2));
  }


  /**
   * Creates an instance of the algorithm.
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  Feature2DAlgorithm *FASTAlgorithm::create(const PvlFlatMap &vars, const QString &config) {
    return ( new FASTAlgorithm(vars, config) );
  }


  /**
   * Returns true if the algorithm has a detector.
   *
   * @return @b true if the algorithm has a detector.
   */
  bool FASTAlgorithm::hasDetector() const {
    return true;
  }


  /**
   * Returns true if the algorithm has an extractor.
   *
   * @return @b true if the algorithm has an extractor.
   */
  bool FASTAlgorithm::hasExtractor() const {
    return false;
  }


  /**
   * Returns true if the algorithm has a matcher.
   *
   * @return @b true if the algorithm has a matcher.
   */
  bool FASTAlgorithm::hasMatcher() const {
    return false;
  }


  /**
   * Returns the variables and their values used by the algorithm.
   *
   * @return @b PvlFlatMap The variables and their values as keyword, value pairs.
   */
  PvlFlatMap FASTAlgorithm::getAlgorithmVariables( ) const {
    FASTPtr algorithm = m_algorithm.dynamicCast<FASTType>();
    PvlFlatMap variables;
    variables.add("NonmaxSuppression", toString(algorithm->getNonmaxSuppression()));
    variables.add("Threshold",         toString(algorithm->getThreshold()));
    variables.add("Type",              m_typeMap.right.at(algorithm->getType()));
    return (variables);
  }


/**
 * Set parameters as provided by the variables
 *
 * @param variables Container of parameters to set
 *
 * @return @b int Number of variables actually set
 *
 * @throws IException::User "The input value is not valid for FAST's [Type] variable"
 */
  int FASTAlgorithm::setAlgorithmVariables(const PvlFlatMap &variables) {

    FASTPtr algorithm = m_algorithm.dynamicCast<FASTType>();

    int numSet(0);
    if ( variables.exists("NonmaxSuppression") ) {
      algorithm->setNonmaxSuppression(toInt(variables.get("NonmaxSuppression")));
      numSet++;
    }

    if ( variables.exists("Threshold") ) {
      algorithm->setThreshold(toInt(variables.get("Threshold")));
      numSet++;
    }

    if ( variables.exists("Type") ) {
      QString value = variables.get("Type");
      bool isInt;
      // Check if the value is an integer
      int intValue = value.toInt(&isInt);
      try {
        if (isInt) {
          // If it is an integer make sure it is a valid option
          m_typeMap.right.at(intValue);
        }
        else {
          // If it is a string, then convert it to an integer
          intValue = m_typeMap.left.at(value);
        }
      }
      catch (std::exception &e) {
        std::string msg = "The input value [" + value +
                      "] is not valid for FAST's [Type] variable";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      algorithm->setType(FASTType::DetectorType(intValue));
      numSet++;
    }

    return (numSet);
  }

};  // namespace Isis
