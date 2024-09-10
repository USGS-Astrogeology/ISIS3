/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include <boost/foreach.hpp>
#include "opencv2/opencv.hpp"
#include "opencv2/xfeatures2d.hpp"

#include "BRISKAlgorithm.h"
#include <QVariant>

namespace Isis {


  /**
   * Constructs the algorithm with default variables.
   */
  BRISKAlgorithm::BRISKAlgorithm() :
                 Feature2DAlgorithm("BRISK", "Feature2D",
                                    BRISKType::create()) {
    setupParameters();
  }


  /**
   * Constructs the algorithm with the input variables
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  BRISKAlgorithm::BRISKAlgorithm(const PvlFlatMap &cvars, const QString &config) :
                  Feature2DAlgorithm("BRISK", "Feature2D",
                                     BRISKType::create(), cvars) {
   setConfig(config);
   PvlFlatMap variables = setupParameters();
   variables.merge(cvars);

   // Constructs the algorithm for a custom pattern
   if (variables.exists("RadiusList") && variables.exists("NumberList")) {
     std::vector<float> radiusList;
     QStringList radiusStrings = variables.get("RadiusList").split(",");
     BOOST_FOREACH(QString radius, radiusStrings) {
       radiusList.push_back(radius.toFloat());
     }

     std::vector<int> numberList;
     QStringList numberStrings = variables.get("NumberList").split(",");
     BOOST_FOREACH(QString number, numberStrings) {
       numberList.push_back(number.toInt());
     }
     const float dMax = variables.get("DMax", "5.85").toFloat();
     const float dMin = variables.get("DMin", "8.2").toFloat();

     std::vector<int> indexChange;
     if (!variables.get("IndexChange", "").isEmpty()) {
       QStringList indexStrings = variables.get("IndexChange").split(",");
       BOOST_FOREACH(QString index, indexStrings) {
         indexChange.push_back(index.toInt());
       }
     }
     m_algorithm = BRISKType::create(radiusList, numberList, dMax, dMin, indexChange);
   }
   // Constructs the algorithm with the input variables
   else {
     const int thresh = variables.get("Threshold").toInt();
     const int octaves = variables.get("NOctaves").toInt();
     const float patternScale = variables.get("PatternScale").toFloat();

     m_algorithm = BRISKType::create(thresh, octaves, patternScale);
   }
    m_variables.merge(variables);
  }


  /**
   * Destroys the algorithm
   */
  BRISKAlgorithm::~BRISKAlgorithm() { }


  /**
   * Sets up the algorithm parameters with default values.
   *
   * @return PvlFlatMap Algorithm parameters and their default values.
   */
  PvlFlatMap BRISKAlgorithm::setupParameters() {
    PvlFlatMap variables;
    variables.add("Threshold",    "30");
    variables.add("NOctaves",     "3");
    variables.add("PatternScale", "1.0");
    m_variables = variables;
    return (m_variables);
  }


  /**
   * Returns a description of the algorithm.
   *
   * @return @b QString A description of the algorithm.
   */
  QString BRISKAlgorithm::description() const {
    QString desc = "The OpenCV BRISK Feature2D detector/extractor algorithm."
                   " See the documentation at "
     "http://docs.opencv.org/3.1.0/de/dbf/classcv_1_1BRISK.html";
    return (desc);
  }


  /**
   * Creates an instance of the algorithm.
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  Feature2DAlgorithm *BRISKAlgorithm::create(const PvlFlatMap &vars, const QString &config) {
    return ( new BRISKAlgorithm(vars, config) );
  }


  /**
   * Returns true if the algorithm has a detector.
   *
   * @return @b true if the algorithm has a detector.
   */
  bool BRISKAlgorithm::hasDetector() const {
    return true;
  }


  /**
   * Returns true if the algorithm has an extractor.
   *
   * @return @b true if the algorithm has an extractor.
   */
  bool BRISKAlgorithm::hasExtractor() const {
    return true;
  }


  /**
   * Returns true if the algorithm has a matcher.
   *
   * @return @b true if the algorithm has a matcher.
   */
  bool BRISKAlgorithm::hasMatcher() const {
    return false;
  }


  /**
   * Returns the variables and their values used by the algorithm.
   *
   * @return @b PvlFlatMap The variables and their values as keyword, value pairs.
   */
  PvlFlatMap BRISKAlgorithm::getAlgorithmVariables( ) const {
    return (variables());
  }


/**
 * @brief Set parameters as provided by the variables
 *
 * @param variables Container of parameters to set
 *
 * @return @b int Always -1, variables cannot be set after initialization.
 *
 * @throws IException::Programmer "BRISKAlgorithm does not have the ability
 *                                 to set algorithm parameters."
 */
  int BRISKAlgorithm::setAlgorithmVariables(const PvlFlatMap &variables) {
    std::string msg = "BRISKAlgorithm does not have the ability to set algorithm parameters.";
    throw IException(IException::Programmer, msg, _FILEINFO_);

    return (-1);
  }

};  // namespace Isis
