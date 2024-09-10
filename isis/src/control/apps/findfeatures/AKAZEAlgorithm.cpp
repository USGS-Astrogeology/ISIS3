/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include <boost/foreach.hpp>
#include "opencv2/opencv.hpp"
#include "opencv2/xfeatures2d.hpp"

#include "AKAZEAlgorithm.h"
#include <QVariant>

namespace Isis {


  /**
   * Constructs the algorithm with default variables.
   */
  AKAZEAlgorithm::AKAZEAlgorithm() :
                  Feature2DAlgorithm("AKAZE", "Feature2D",
                                     AKAZEType::create()) {
    setupMaps();
    m_variables.merge(getAlgorithmVariables());
  }


  /**
   * Constructs the algorithm with the input variables
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  AKAZEAlgorithm::AKAZEAlgorithm(const PvlFlatMap &cvars, const QString &config) :
                  Feature2DAlgorithm("AKAZE", "Feature2D",
                                     AKAZEType::create(), cvars) {
    setupMaps();
    setConfig(config);
    setAlgorithmVariables(cvars);
    m_variables.merge(getAlgorithmVariables());
  }


  /**
   * Destroys the algorithm
   */
  AKAZEAlgorithm::~AKAZEAlgorithm() { }


  /**
   * Fills the maps for converting the DescriptorType & Diffusivity variable.
   */
  void AKAZEAlgorithm::setupMaps() {
    // DescriptorType map. Int values are from OpenCV's API.
    m_descriptorTypeMap.left.insert(std::pair<QString,int>("DESCRIPTOR_KAZE_UPRIGHT", 2));
    m_descriptorTypeMap.left.insert(std::pair<QString,int>("DESCRIPTOR_KAZE",         3));
    m_descriptorTypeMap.left.insert(std::pair<QString,int>("DESCRIPTOR_MLDB_UPRIGHT", 4));
    m_descriptorTypeMap.left.insert(std::pair<QString,int>("DESCRIPTOR_MLDB",         5));

    // Diffusivity map. Int values are from OpenCV's API.
    m_diffusivityMap.left.insert(std::pair<QString,int>("DIFF_PM_G1",       0));
    m_diffusivityMap.left.insert(std::pair<QString,int>("DIFF_PM_G2",       1));
    m_diffusivityMap.left.insert(std::pair<QString,int>("DIFF_WEICKERT",    2));
    m_diffusivityMap.left.insert(std::pair<QString,int>("DIFF_CHARBONNIER", 3));
  }


  /**
   * Returns a description of the algorithm.
   *
   * @return @b QString A description of the algorithm.
   */
  QString AKAZEAlgorithm::description() const {
    QString desc = "The OpenCV AKAZE Feature2D detector/extractor algorithm."
                   " See the documentation at "
     "http://docs.opencv.org/3.1.0/d8/d30/classcv_1_1AKAZE.html";
    return (desc);
  }


  /**
   * Creates an instance of the algorithm.
   *
   * @param cvars  The variables and values the algorithm will use.
   *               Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  Feature2DAlgorithm *AKAZEAlgorithm::create(const PvlFlatMap &vars,
                                                            const QString &config) {
    return ( new AKAZEAlgorithm(vars, config) );
  }


  /**
   * Returns true if the algorithm has a detector.
   *
   * @return @b true if the algorithm has a detector.
   */
  bool AKAZEAlgorithm::hasDetector() const {
    return true;
  }


  /**
   * Returns true if the algorithm has an extractor.
   *
   * @return @b true if the algorithm has an extractor.
   */
  bool AKAZEAlgorithm::hasExtractor() const {
    return true;
  }


  /**
   * Returns true if the algorithm has a matcher.
   *
   * @return @b true if the algorithm has a matcher.
   */
  bool AKAZEAlgorithm::hasMatcher() const {
    return false;
  }


  /**
   * Returns the variables and their values used by the algorithm.
   *
   * @return @b PvlFlatMap The variables and their values as keyword, value pairs.
   */
  PvlFlatMap AKAZEAlgorithm::getAlgorithmVariables( ) const {
    AKAZEPtr algorithm = m_algorithm.dynamicCast<AKAZEType>();
    PvlFlatMap variables;
    variables.add("DescriptorType",     m_descriptorTypeMap.right.at(
                                              algorithm->getDescriptorType()));
    variables.add("DescriptorSize",     QString::number(algorithm->getDescriptorSize()));
    variables.add("DescriptorChannels", QString::number(algorithm->getDescriptorChannels()));
    variables.add("Threshold",          QString::number(algorithm->getThreshold()));
    variables.add("NOctaves",           QString::number(algorithm->getNOctaves()));
    variables.add("NOctaveLayers",      QString::number(algorithm->getNOctaveLayers()));
    variables.add("Diffusivity",        m_diffusivityMap.right.at(
                                              algorithm->getDiffusivity()));
    return (variables);
  }


/**
 * Set parameters as provided by the variables
 *
 * @param variables Container of parameters to set
 *
 * @return @b int Number of variables actually set
 *
 * @throws IException::User "The input value is not valid for AKAZE's [DescriptorType] variable"
 * @throws IException::User "The input value is not valid for AKAZE's [Diffusivity] variable"
 */
  int AKAZEAlgorithm::setAlgorithmVariables(const PvlFlatMap &variables) {

    AKAZEPtr algorithm = m_algorithm.dynamicCast<AKAZEType>();

    int numSet(0);
    if ( variables.exists("DescriptorType") ) {
      QString value = variables.get("DescriptorType");
      bool isInt;
      // Check if the value is an integer
      int intValue = value.toInt(&isInt);
      try {
        if (isInt) {
          // If it is an integer make sure it is a valid option
          m_descriptorTypeMap.right.at(intValue);
        }
        else {
          // If it is a string, then convert it to an integer
          intValue = m_descriptorTypeMap.left.at(value);
        }
      }
      catch (std::exception &e) {
        std::string msg = "The input value [" + value.toStdString() +
                      "] is not valid for AKAZE's [DescriptorType] variable";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      algorithm->setDescriptorType(AKAZEType::DescriptorType(intValue));
      numSet++;
    }

    if ( variables.exists("DescriptorSize") ) {
      algorithm->setDescriptorSize(variables.get("DescriptorSize").toInt());
      numSet++;
    }

    if ( variables.exists("DescriptorChannels") ) {
      algorithm->setDescriptorChannels(variables.get("DescriptorChannels").toInt());
      numSet++;
    }

    if ( variables.exists("Threshold") ) {
      algorithm->setThreshold(variables.get("Threshold").toInt());
      numSet++;
    }

    if ( variables.exists("NOctaves") ) {
      algorithm->setNOctaves(variables.get("NOctaves").toInt());
      numSet++;
    }

    if ( variables.exists("NOctaveLayers") ) {
      algorithm->setNOctaveLayers(variables.get("NOctaveLayers").toInt());
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
          m_diffusivityMap.right.at(intValue);
        }
        else {
          // If it is a string, then convert it to an integer
          intValue = m_diffusivityMap.left.at(value);
        }
      }
      catch (std::exception &e) {
        std::string msg = "The input value [" + value.toStdString() +
                      "] is not valid for AKAZE's [Diffusivity] variable";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      algorithm->setDiffusivity(KAZEType::DiffusivityType(intValue));
      numSet++;
    }

    return (numSet);
  }

};  // namespace Isis
