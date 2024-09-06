/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include "opencv2/opencv.hpp"
#include "opencv2/xfeatures2d.hpp"

#include "DaisyAlgorithm.h"

#include <QVariant>
#include <QString>
#include <QStringList>

namespace Isis {

  /**
   * Constructs a default DaisyAlgorithm with default variables. Note the OpenCV
   * Daisy algorithm does not provide direct parameter access after construction
   * so all variable values must be know when constructed.
   */
  DaisyAlgorithm::DaisyAlgorithm() : Feature2DAlgorithm("DAISY", "Feature2D",
                                                         DAISYType::create()) {
    setupTypeMap();
    setupParameters();
  }


  /**
   * Constructs a DaisyAlgorithm with input variables.
   *
   * This constuctor provides a custom Daisy algorithm that allows users to
   * provide specfic values for all algorithm parameters.
   *
   * H is an optional 3x3 homography matrix used to warp the grid of Daisy. If
   * not entereted, it will default to the identity matrix.
   *
   *
   * @throws IException::Programmer "Homography matrix, H, was not input as a string of the form
   *                   \"d,d,d,d,d,d,d,d,d\" where d is a double or integer numerical value."
   *
   * @param cvars Variables that are not included will be set to their default.
   * @param config The config string used to construct cvars.
   */
  DaisyAlgorithm::DaisyAlgorithm(const PvlFlatMap &cvars, const QString &config) :
                  Feature2DAlgorithm("DAISY", "Feature2D", DAISYType::create()) {
    setupTypeMap();
    PvlFlatMap variables = setupParameters();
    variables.merge(cvars);
    setConfig(config);

    float radius = (float) toDouble(variables.get("radius"));
    int q_radius = toInt(variables.get("q_radius"));
    int q_theta = toInt(variables.get("q_theta"));
    int q_hist = toInt(variables.get("q_hist"));
    int norm = m_typeMap.left.at(variables.get("norm"));
    cv::Mat H = cv::Mat::eye(3,3,CV_64FC1);
    if ( variables.exists("H")) {
      // Convert H-string to a 3x3 matrix represented by a std::vector (opencv will accept this
      // as an alternative to a cv::InputArray)
      QString Hparm = QString(variables.get("H"));
      QStringList elts = Hparm.split( "," );

      if (elts.size() != 9 ) {
        std::string mess = "Homography matrix, H, was not input as a string of the form \"d,d,d,d,d,d,d,"
                       "d,d\" where d is a double or integer numerical value.";
          throw IException(IException::Programmer, mess, _FILEINFO_);
      }

      // potentially add a try/catch around this for failed .at's or .toDoubles
      for (int row = 0; row < 3; row++) {
        double* Hi = H.ptr<double>(row);
        for (int column = 0 ; column < 3 ; column++) {
          Hi[column] = elts.value( column + (row * 3)).toDouble();
        }
      }
    }
    bool interpolation = toBool(variables.get("interpolation"));
    bool use_orientation = toBool(variables.get("use_orientation"));

    // Direct creation of DAISY algorithm, replacing default in constructor
    // initialization in FeatureAlgorithm::m_algorithm
    m_algorithm = DAISYType::create(radius, q_radius, q_theta, q_hist, DAISYType::NormalizationType(norm), H,
                                    interpolation, use_orientation);

    // Set the input parameter conditions
    m_variables.merge(variables);
  }


/**
 * Default Destructor
 */
  DaisyAlgorithm::~DaisyAlgorithm() { }


 /**
   * Returns a description of the DaisyAlgorithm.
   *
   * @return @b QString A description of the algorithm.
   */
  QString DaisyAlgorithm::description() const {
    QString desc = "The OpenCV DAISY Feature2D detector/extractor algorithm."
                   " See the documentation at "
      "http://docs.opencv.org/3.1.0/d9/d37/classcv_1_1xfeatures2d_1_1DAISY.html";
    return (desc);
  }


  /**
   * Fills the map for converting the type variable.
   */
  void DaisyAlgorithm::setupTypeMap() {
    m_typeMap.left.insert(std::pair<QString, int>("NRM_NONE",    (int) DAISYType::NRM_NONE));
    m_typeMap.left.insert(std::pair<QString, int>("NRM_PARTIAL", (int) DAISYType::NRM_PARTIAL));
    m_typeMap.left.insert(std::pair<QString, int>("NRM_FULL",    (int) DAISYType::NRM_FULL));
    m_typeMap.left.insert(std::pair<QString, int>("NRM_SIFT",    (int) DAISYType::NRM_SIFT));
  }


/**
 * Creates and returns an instance of DaisyAlgorithm.
 *
 * @param vars PvlFlatMap containing algorithm parameters and their values
 * @param config A configuration string input by the user
 *
 * @return @b Feature2DAlgorithm A DaisyAlgorithm instance
 */
  Feature2DAlgorithm *DaisyAlgorithm::create(const PvlFlatMap &vars,
                                            const QString &config) {
    return ( new DaisyAlgorithm(vars, config) );
  }


  /**
   * Returns true if the algorithm has a detector.
   *
   * @return @b true if the algorithm has a detector.
   */
  bool DaisyAlgorithm::hasDetector() const {
    return false;
  }


  /**
   * Returns true if the algorithm has an extractor.
   *
   * @return @b true if the algorithm has an extractor.
   */
  bool DaisyAlgorithm::hasExtractor() const {
    return true;
  }


  /**
   * Returns true if the algorithm has a matcher.
   *
   * @return @b true if the algorithm has a matcher.
   */
  bool DaisyAlgorithm::hasMatcher() const {
    return false;
  }


/**
 * Get and return DAISY's parameters and what they're set to as a PvlFlatMap.
 *
 * @return PvlFlatMap A PvlFlatMap of DAISY's currently set variables and their values.
 */
  PvlFlatMap DaisyAlgorithm::getAlgorithmVariables( ) const {
    return ( variables() );
  }


/**
 * @brief Set parameters as provided by the variables, does not work for the
 * DAISY algorithm in OpenCV3, so calling this will throw an exception.
 *
 * @param variables Container of parameters to set
 *
 * @return int Number of variables actually set
 */
  int DaisyAlgorithm::setAlgorithmVariables(const PvlFlatMap &variables) {
    std::string message = "DAISY does not have the ability to set algorithm parameters.";
    throw IException(IException::Programmer, message, _FILEINFO_);

    return (-1);
  }


/**
 * @brief Initiolize the Daisy default parameters according to documentation
 *
 * This methiod provides the Daisy algorithm parameter defaults according to the
 * OpenCV documentation found at
 * http://docs.opencv.org/3.1.0/d9/d37/classcv_1_1xfeatures2d_1_1DAISY.html.
 *
 * This will reset the Daisy parameters to the default conditions and cotnained
 * in FeatureAlgorithm::m_variables.
 *
 * @author Kris Becker 2016-12-18
 *
 * @return PvlFlatMap Container of all Daisy parameters
 */
  PvlFlatMap DaisyAlgorithm::setupParameters() {
    PvlFlatMap variables;
    variables.add("radius", "15");
    variables.add("q_radius", "3");
    variables.add("q_theta", "8");
    variables.add("q_hist", "8");
    variables.add("norm", "NRM_NONE");
    variables.add("H", "1,0,0,0,1,0,0,0,1");
    variables.add("interpolation", "true");
    variables.add("use_orientation", "false");
    m_variables = variables;
    return (m_variables);
   }

};
// namespace Isis
