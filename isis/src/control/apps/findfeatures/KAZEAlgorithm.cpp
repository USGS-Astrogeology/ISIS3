/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "KAZEAlgorithm.h"
#include <QVariant>

namespace Isis {


  /**
   * Constructs the algorithm with default variables.
   */
  KAZEAlgorithm::KAZEAlgorithm() :
                 Feature2DAlgorithm("KAZE", "Feature2D",
                                    KAZEType::create()) {
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
  KAZEAlgorithm::KAZEAlgorithm(const PvlFlatMap &cvars, const QString &config) :
                  Feature2DAlgorithm("KAZE", "Feature2D",
                                     KAZEType::create(), cvars) {
    setupTypeMap();
    setConfig(config);
    setAlgorithmVariables(cvars);
    m_variables.merge(getAlgorithmVariables());
  }


  /**
   * Destroys the algorithm
   */
  KAZEAlgorithm::~KAZEAlgorithm() { }


  /**
   * Returns a description of the algorithm.
   *
   * @return @b QString A description of the algorithm.
   */
  QString KAZEAlgorithm::description() const {
    QString desc = "The OpenCV KAZE Feature2D detector/extractor algorithm."
                   " See the documentation at "
     "http://docs.opencv.org/3.1.0/d3/d61/classcv_1_1KAZE.html";
    return (desc);
  }


  /**
   * Fills the map for converting the type variable.
   */
  void KAZEAlgorithm::setupTypeMap() {
    m_typeMap.left.insert(std::pair<QString,int>("DIFF_PM_G1",       0));
    m_typeMap.left.insert(std::pair<QString,int>("DIFF_PM_G2",       1));
    m_typeMap.left.insert(std::pair<QString,int>("DIFF_WEICKERT",    2));
    m_typeMap.left.insert(std::pair<QString,int>("DIFF_CHARBONNIER", 3));
  }


  /**
   * Creates an instance of the algorithm.
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  Feature2DAlgorithm *KAZEAlgorithm::create(const PvlFlatMap &vars, const QString &config) {
    return ( new KAZEAlgorithm(vars, config) );
  }


  /**
   * Returns true if the algorithm has a detector.
   *
   * @return @b true if the algorithm has a detector.
   */
  bool KAZEAlgorithm::hasDetector() const {
    return true;
  }


  /**
   * Returns true if the algorithm has an extractor.
   *
   * @return @b true if the algorithm has an extractor.
   */
  bool KAZEAlgorithm::hasExtractor() const {
    return true;
  }


  /**
   * Returns true if the algorithm has a matcher.
   *
   * @return @b true if the algorithm has a matcher.
   */
  bool KAZEAlgorithm::hasMatcher() const {
    return false;
  }


  /**
   * Returns the variables and their values used by the algorithm.
   *
   * @return @b PvlFlatMap The variables and their values as keyword, value pairs.
   */
  PvlFlatMap KAZEAlgorithm::getAlgorithmVariables( ) const {
    KAZEPtr algorithm = m_algorithm.dynamicCast<KAZEType>();
    PvlFlatMap variables;
    variables.add("Diffusivity",   m_typeMap.right.at(algorithm->getDiffusivity()));
    variables.add("Extended",      QString::number(algorithm->getExtended()));
    variables.add("NOctaveLayers", QString::number(algorithm->getNOctaveLayers()));
    variables.add("NOctaves",      QString::number(algorithm->getNOctaves()));
    variables.add("Threshold",     QString::number(algorithm->getThreshold()));
    variables.add("Upright",       QString::number(algorithm->getUpright()));
    return (variables);
  }


/**
 * Set parameters as provided by the variables
 *
 * @param variables Container of parameters to set
 *
 * @return @b int Number of variables actually set
 *
 * @throws IException::User "The input value is not valid for KAZE's [Type] variable"
 */
  int KAZEAlgorithm::setAlgorithmVariables(const PvlFlatMap &variables) {
    KAZEPtr algorithm = m_algorithm.dynamicCast<KAZEType>();

    int numSet(0);
    if ( variables.exists("Extended") ) {
      algorithm->setExtended(variables.get("Extended").toInt());
      numSet++;
    }

    if ( variables.exists("Threshold") ) {
      algorithm->setThreshold(variables.get("Threshold").toInt());
      numSet++;
    }

    if ( variables.exists("NOctaveLayers") ) {
      algorithm->setNOctaveLayers(variables.get("NOctaveLayers").toInt());
      numSet++;
    }

    if ( variables.exists("NOctaves") ) {
      algorithm->setNOctaves(variables.get("NOctaves").toInt());
      numSet++;
    }

    if ( variables.exists("Upright") ) {
      algorithm->setUpright(variables.get("Upright").toInt());
      numSet++;
    }

    if ( variables.exists("Diffusivity") ) {
      QString value = variables.get("Diffusivity");
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
        std::string msg = "The input value [" + value.toStdString() +
                      "] is not valid for KAZE's [Type] variable";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      algorithm->setDiffusivity(KAZEType::DiffusivityType(intValue));
      numSet++;
    }

    return (numSet);
  }

};  // namespace Isis
