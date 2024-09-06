/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <boost/foreach.hpp>
#include "opencv2/opencv.hpp"
#include "opencv2/xfeatures2d.hpp"

#include "MSDAlgorithm.h"
#include <QVariant>

namespace Isis {


  /**
   * Constructs the algorithm with default variables.
   */
  MSDAlgorithm::MSDAlgorithm() :
                 Feature2DAlgorithm("MSD", "Feature2D",
                                    MSDType::create()) {
    setupParameters();

  }


  /**
   * Constructs the algorithm with the input variables
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  MSDAlgorithm::MSDAlgorithm(const PvlFlatMap &cvars, const QString &config) :
                Feature2DAlgorithm("MSD", "Feature2D",
                                    MSDType::create(), cvars) {
    setConfig(config);
    PvlFlatMap variables = setupParameters();
    variables.merge(cvars);
    const int patchRadius = toInt(variables.get("PatchRadius"));
    const int searchAreaRadius = toInt(variables.get("SearchAreaRadius"));
    const int nmsRadius = toInt(variables.get("NMSRadius"));
    const int nmsScaleRadius = toInt(variables.get("NMSScaleRadius"));
    const float thSaliency = variables.get("THSaliency").toFloat();
    const int kNN = toInt(variables.get("KNN"));
    const float scaleFactor = variables.get("ScaleFactor").toFloat();
    const int nScales = toInt(variables.get("NScales"));
    const bool computeOrientation = toBool(variables.get("ComputeOrientation"));

    m_algorithm = MSDType::create(patchRadius, searchAreaRadius, nmsRadius, nmsScaleRadius,
                                  thSaliency, kNN, scaleFactor, nScales, computeOrientation);

    m_variables.merge(variables);
  }


  /**
   * Destroys the algorithm
   */
  MSDAlgorithm::~MSDAlgorithm() { }


  /**
   * Sets up the algorithm parameters with default values.
   *
   * @return PvlFlatMap Algorithm parameters and their default values.
   */
  PvlFlatMap MSDAlgorithm::setupParameters() {
    PvlFlatMap variables;
    variables.add("PatchRadius",        "3");
    variables.add("SearchAreaRadius",   "5");
    variables.add("NMSRadius",          "5");
    variables.add("NMSScaleRadius",     "0");
    variables.add("THSaliency",         "250.0");
    variables.add("KNN",                "4");
    variables.add("ScaleFactor",        "1.25");
    variables.add("NScales",            "-1");
    variables.add("ComputeOrientation", "false");
    m_variables = variables;
    return (m_variables);
  }


  /**
   * Returns a description of the algorithm.
   *
   * @return @b QString A description of the algorithm.
   */
  QString MSDAlgorithm::description() const {
    QString desc = "The OpenCV MSD Feature2D detector/extractor algorithm."
                   " See the documentation at "
     "http://docs.opencv.org/3.1.0/d6/d36/classcv_1_1xfeatures2d_1_1MSD.html";
    return (desc);
  }


  /**
   * Creates an instance of the algorithm.
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  Feature2DAlgorithm *MSDAlgorithm::create(const PvlFlatMap &vars, const QString &config) {

    return ( new MSDAlgorithm(vars, config) );
  }


  /**
   * Returns true if the algorithm has a detector.
   *
   * @return @b true if the algorithm has a detector.
   */
  bool MSDAlgorithm::hasDetector() const {
    return true;
  }


  /**
   * Returns true if the algorithm has an extractor.
   *
   * @return @b true if the algorithm has an extractor.
   */
  bool MSDAlgorithm::hasExtractor() const {
    return false;
  }


  /**
   * Returns true if the algorithm has a matcher.
   *
   * @return @b true if the algorithm has a matcher.
   */
  bool MSDAlgorithm::hasMatcher() const {
    return false;
  }


  /**
   * Returns the variables and their values used by the algorithm.
   *
   * @return @b PvlFlatMap The variables and their values as keyword, value pairs.
   */
  PvlFlatMap MSDAlgorithm::getAlgorithmVariables( ) const {
    return (variables());
  }


/**
 * @brief Set parameters as provided by the variables
 *
 * @param variables Container of parameters to set
 *
 * @return @b int Always -1, variables cannot be set after initialization.
 *
 * @throws IException::Programmer "MSDAlgorithm does not have the ability
 *                                 to set algorithm parameters."
 */
  int MSDAlgorithm::setAlgorithmVariables(const PvlFlatMap &variables) {
    std::string msg = "MSDAlgorithm does not have the ability to set algorithm parameters.";
    throw IException(IException::Programmer, msg, _FILEINFO_);

    return (-1);
  }

};  // namespace Isis
