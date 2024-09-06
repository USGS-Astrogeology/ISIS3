/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include "AgastAlgorithm.h"

namespace Isis {


  /**
   * Constructs the algorithm with default variables.
   */
  AgastAlgorithm::AgastAlgorithm() :
                 Feature2DAlgorithm("AGAST", "Feature2D",
                                    AgastType::create()) {
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
  AgastAlgorithm::AgastAlgorithm(const PvlFlatMap &cvars, const QString &config) :
                  Feature2DAlgorithm("AGAST", "Feature2D",
                                     AgastType::create(), cvars) {
    setupTypeMap();
    setConfig(config);
    setAlgorithmVariables(cvars);
    m_variables.merge(getAlgorithmVariables());
  }


  /**
   * Destroys the algorithm
   */
  AgastAlgorithm::~AgastAlgorithm() { }


  /**
   * Returns a description of the algorithm.
   *
   * @return @b QString A description of the algorithm.
   */
  QString AgastAlgorithm::description() const {
    QString desc = "The OpenCV AGAST Feature2D detector/extractor algorithm."
                   " See the documentation at "
     "http://docs.opencv.org/3.1.0/d7/d19/classcv_1_1AgastFeatureDetector.html";
    return (desc);
  }


  /**
   * Fills the map for converting the type variable.
   */
  void AgastAlgorithm::setupTypeMap() {
    m_typeMap.left.insert(std::pair<QString,int>("AGAST_5_8",   0));
    m_typeMap.left.insert(std::pair<QString,int>("AGAST_7_12D", 1));
    m_typeMap.left.insert(std::pair<QString,int>("AGAST_7_12S", 2));
    m_typeMap.left.insert(std::pair<QString,int>("OAST_9_16",   3));
  }


  /**
   * Creates an instance of the algorithm.
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  Feature2DAlgorithm *AgastAlgorithm::create(const PvlFlatMap &vars, const QString &config) {
    return ( new AgastAlgorithm(vars, config) );
  }


 /**
   * Returns true if the algorithm has a detector.
   *
   * @return @b true if the algorithm has a detector.
   */
  bool AgastAlgorithm::hasDetector() const {
    return true;
  }


  /**
   * Returns true if the algorithm has an extractor.
   *
   * @return @b true if the algorithm has an extractor.
   */
  bool AgastAlgorithm::hasExtractor() const {
    return false;
  }


  /**
   * Returns true if the algorithm has a matcher.
   *
   * @return @b true if the algorithm has a matcher.
   */
  bool AgastAlgorithm::hasMatcher() const {
     return false;
  }


  /**
   * Returns the variables and their values used by the algorithm.
   *
   * @return @b PvlFlatMap The variables and their values as keyword, value pairs.
   */
  PvlFlatMap AgastAlgorithm::getAlgorithmVariables( ) const {
    AgastPtr algorithm = m_algorithm.dynamicCast<AgastType>();
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
 * @throws IException::User "The input value is not valid for AGAST's [Type] variable"
 */
  int AgastAlgorithm::setAlgorithmVariables(const PvlFlatMap &variables) {

    AgastPtr algorithm = m_algorithm.dynamicCast<AgastType>();

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
          intValue = m_typeMap.left.at(value.toUpper());
        }
      }
      catch (std::exception &e) {
        std::string msg = "The input value [" + value +
                      "] is not valid for AGAST's [Type] variable";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      algorithm->setType(AgastType::DetectorType(intValue));
      numSet++;
    }

    return (numSet);
  }

};  // namespace Isis
