/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "BundleSettings.h"

#include <QDataStream>
#include <QDebug>
#include <QList>
#include <QString>
#include <QtGlobal> // qMax()
#include <QUuid>
#include <QXmlStreamWriter>
#include <QXmlInputSource>

#include "BundleObservationSolveSettings.h"
#include "IException.h"
#include "IString.h"
#include "Project.h" // currently used for xml handler
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "SpecialPixel.h"

namespace Isis {

  /**
   * @brief Constructs a BundleSettings object.
   * Default values are set for all member variables. By default, BundleSettings allows creation
   * of the inverse correlation matrix file.
   *
   * @see createInverseMatrix()
   * @see setCreateInverseMatrix()
   */
  BundleSettings::BundleSettings() {
    init();
    BundleObservationSolveSettings defaultSolveSettings;
    m_observationSolveSettings.append(defaultSolveSettings);
  }

  /**
   * @brief Set Default vales for a BundleSettings object.
   * Note we call the default constructor to initialize the TargetBody information
   * that is not currently in the XML.
   */
  void BundleSettings::init() {
    m_validateNetwork = true;

    m_solveObservationMode = false;
    m_solveRadius          = false;
    m_updateCubeLabel      = false;
    m_errorPropagation     = false;
    m_createInverseMatrix  = false;
    m_cubeList             =    "";
    m_outlierRejection     = false;
    m_outlierRejectionMultiplier = 3.0;

    // Parameter Uncertainties (Weighting)
    // The units are meters for either coordinate type
    m_globalPointCoord1AprioriSigma  = Isis::Null;
    m_globalPointCoord2AprioriSigma = Isis::Null;
    m_globalPointCoord3AprioriSigma    = Isis::Null;

    // Convergence Criteria
    m_convergenceCriteria = BundleSettings::Sigma0;
    m_convergenceCriteriaThreshold = 1.0e-10;
    m_convergenceCriteriaMaximumIterations = 50;

    // Maximum Likelihood Estimation Options no default in the constructor - must be set.
    m_maximumLikelihood.clear();

    // Self Calibration ??? (from ipce only)

    // Target Body
    m_solveTargetBody = false;
//    m_solveTargetBodyPolePosition = false;
//    m_solveTargetBodyZeroMeridian = false;
//    m_solveTargetBodyRotationRate = false;
//    m_solveTargetBodyRadiusMethod = None;

    // Control Points
    m_cpCoordTypeReports = SurfacePoint::Latitudinal;
    m_cpCoordTypeBundle = SurfacePoint::Latitudinal;

    // Output Options
    m_outputFilePrefix = "";
  }


  /**
   * This copy constructor sets this BundleSettings' member data to match
   * that of the 'other' given BundleSettings.
   *
   * @param other The BundleSettings object to be copied.
   */
  BundleSettings::BundleSettings(const BundleSettings &other)
      : m_validateNetwork(other.m_validateNetwork),
        m_cubeList(other.m_cubeList),
        m_solveObservationMode(other.m_solveObservationMode),
        m_solveRadius(other.m_solveRadius),
        m_updateCubeLabel(other.m_updateCubeLabel),
        m_errorPropagation(other.m_errorPropagation),
        m_createInverseMatrix(other.m_createInverseMatrix),
        m_outlierRejection(other.m_outlierRejection),
        m_outlierRejectionMultiplier(other.m_outlierRejectionMultiplier),
        m_globalPointCoord1AprioriSigma(other.m_globalPointCoord1AprioriSigma),
        m_globalPointCoord2AprioriSigma(other.m_globalPointCoord2AprioriSigma),
        m_globalPointCoord3AprioriSigma(other.m_globalPointCoord3AprioriSigma),
        m_observationSolveSettings(other.m_observationSolveSettings),
        m_convergenceCriteria(other.m_convergenceCriteria),
        m_convergenceCriteriaThreshold(other.m_convergenceCriteriaThreshold),
        m_convergenceCriteriaMaximumIterations(other.m_convergenceCriteriaMaximumIterations),
        m_maximumLikelihood(other.m_maximumLikelihood),
        m_solveTargetBody(other.m_solveTargetBody),
        m_bundleTargetBody(other.m_bundleTargetBody),
        m_cpCoordTypeReports(other.m_cpCoordTypeReports),
        m_cpCoordTypeBundle(other.m_cpCoordTypeBundle),
        m_outputFilePrefix(other.m_outputFilePrefix){
  }


  /**
   * Destroys the BundleSettings object.
   */
  BundleSettings::~BundleSettings() {
  }


  /**
   * Assignment operator to allow proper copying of the 'other' BundleSettings
   * object to this one.
   *
   * @param other The BundleSettings object to be copied.
   *
   * @return @b BundleSettings& A reference to the copied BundleSettings object.
   *
   * @internal
   *   @history 2017-07-04 Debbie A. Cook - Added new coordType members and made
   *                           global coordinate names generic.
   */
  BundleSettings &BundleSettings::operator=(const BundleSettings &other) {
    if (&other != this) {
      m_validateNetwork = other.m_validateNetwork;
      m_cubeList = other.m_cubeList;
      m_solveObservationMode = other.m_solveObservationMode;
      m_solveRadius = other.m_solveRadius;
      m_updateCubeLabel = other.m_updateCubeLabel;
      m_errorPropagation = other.m_errorPropagation;
      m_createInverseMatrix = other.m_createInverseMatrix;
      m_outlierRejection = other.m_outlierRejection;
      m_outlierRejectionMultiplier = other.m_outlierRejectionMultiplier;
      m_globalPointCoord1AprioriSigma = other.m_globalPointCoord1AprioriSigma;
      m_globalPointCoord2AprioriSigma = other.m_globalPointCoord2AprioriSigma;
      m_globalPointCoord3AprioriSigma = other.m_globalPointCoord3AprioriSigma;
      m_observationSolveSettings = other.m_observationSolveSettings;
      m_convergenceCriteria = other.m_convergenceCriteria;
      m_convergenceCriteriaThreshold = other.m_convergenceCriteriaThreshold;
      m_convergenceCriteriaMaximumIterations = other.m_convergenceCriteriaMaximumIterations;
      m_solveTargetBody = other.m_solveTargetBody;
      m_bundleTargetBody = other.m_bundleTargetBody;
      m_cpCoordTypeReports = other.m_cpCoordTypeReports;
      m_cpCoordTypeBundle = other.m_cpCoordTypeBundle;
      m_maximumLikelihood = other.m_maximumLikelihood;
      m_outputFilePrefix = other.m_outputFilePrefix;
    }
    return *this;
  }


  /**
   * Sets the internal flag to indicate whether to validate the network before
   * the bundle adjustment.
   *
   * @see BundleAdjust::validateNetwork()
   *
   * @param validate Indicates whether the network should be validated by
   *                 BundleAdjust.
   *
   */
  void BundleSettings::setValidateNetwork(bool validate) {
    m_validateNetwork = validate;
  }


  /**
   * This method is used to determine whether to validate the network before
   * the bundle adjustment.
   *
   * @see BundleAdjust::validateNetwork()
   *
   * @return @b bool Indicates whether the network should be validated by
   *                 BundleAdjust.
   *
   */
  bool BundleSettings::validateNetwork() const {
    return m_validateNetwork;
  }


  /**
   * @brief BundleSettings::setCubeList
   *
   * @param cubeList
   *
   */
  void BundleSettings::setCubeList(QString cubeList)    {
    m_cubeList = cubeList;
  }


  /**
  * @brief BundleSettings::cubeList
  *
  * @return QString The name/path of the cube list.
  */
  QString BundleSettings::cubeList() const {
    return m_cubeList;
  }


  // =============================================================================================//
  // ======================== Solve Options ======================================================//
  // =============================================================================================//

  /**
   * Set the solve options for the bundle adjustment.
   *
   * @param solveObservationMode A boolean value indicating whether to solve for
   *                             observation mode.
   * @param updateCubeLabel A boolean value indicating whether to update the
   *                        cube labels after the bundle adjustment is
   *                        completed.
   * @param errorPropagation A boolean value indicating whether to use the
   *                         cholmod library's error propagation.
   * @param solveRadius A boolean value indicating whether to solve for radius.
   * @param coordType The type of coordinates used for control points
   * @param globalPointCoord1AprioriSigma The global a priori sigma for latitude.
   * @param globalPointCoord2AprioriSigma The global a priori sigma for longitude.
   * @param globalPointCoord3AprioriSigma The global a priori sigma for radius.
   */
  void BundleSettings::setSolveOptions(bool solveObservationMode,
                                       bool updateCubeLabel,
                                       bool errorPropagation,
                                       bool solveRadius,
                                       SurfacePoint::CoordinateType coordTypeBundle,
                                       SurfacePoint::CoordinateType coordTypeReports,
                                       double globalPointCoord1AprioriSigma,
                                       double globalPointCoord2AprioriSigma,
                                       double globalPointCoord3AprioriSigma) {
    m_solveObservationMode = solveObservationMode;
    m_solveRadius = solveRadius;
    m_updateCubeLabel = updateCubeLabel;
    m_errorPropagation = errorPropagation;
    m_cpCoordTypeReports = coordTypeReports;
    m_cpCoordTypeBundle = coordTypeBundle;
    // m_cpCoordTypeBundle = SurfacePoint::Latitudinal;

    if (globalPointCoord1AprioriSigma > 0.0) { // otherwise, we leave as default Isis::Null
      m_globalPointCoord1AprioriSigma = globalPointCoord1AprioriSigma;
    }
    else {
      m_globalPointCoord1AprioriSigma = Isis::Null;
    }

    if (globalPointCoord2AprioriSigma > 0.0) {
      m_globalPointCoord2AprioriSigma = globalPointCoord2AprioriSigma;
    }
    else {
      m_globalPointCoord2AprioriSigma = Isis::Null;
    }

    // This is ugly.  *** TODO *** Revisit this section to try to find a cleaner solution.
    // I think we will have to do similar checking other places.
    // See pvlObject, save,   (DAC 03-29-2017)
    if (coordTypeBundle == SurfacePoint::Latitudinal) {
      if (m_solveRadius && globalPointCoord3AprioriSigma > 0.0) {
        m_globalPointCoord3AprioriSigma = globalPointCoord3AprioriSigma;
      }
      else {
      m_globalPointCoord3AprioriSigma = Isis::Null;
      }
    }
    else if (coordTypeBundle == SurfacePoint::Rectangular) {
      if (globalPointCoord3AprioriSigma > 0.0) {
        m_globalPointCoord3AprioriSigma = globalPointCoord3AprioriSigma;
      }
      else {
      m_globalPointCoord3AprioriSigma = Isis::Null;
      }
    }
  }


  /**
   * Set the outlier rejection options for the bundle adjustment.
   *
   * @param outlierRejection Indicates whether to perform automatic outlier
   *                         rejection during the bundle adjustment.
   * @param mutliplier The outlier rejection multiplier.
   */
  void BundleSettings::setOutlierRejection(bool outlierRejection, double multiplier) {
    m_outlierRejection = outlierRejection;
    if (m_outlierRejection) {
      m_outlierRejectionMultiplier = multiplier;
    }
    else {
      m_outlierRejectionMultiplier = 3.0;
    }
  }


  /**
   * Add the list of solve options for each observation.
   *
   * @param observationSolveSettings A list of BundleObservationSolveSettings objects
   *                                 to indicate the settings for each observation of
   *                                 the bundle adjustment.
   */
  void BundleSettings::setObservationSolveOptions(
      QList<BundleObservationSolveSettings> obsSolveSettingsList) {
    m_observationSolveSettings = obsSolveSettingsList;
  }


  /**
   * Indicates the control point coordinate type for reports.
   *
   * This method returns the control point coordinate setting for reporting control points.
   *
   * @return @b SurfacePoint::CoordinateType Returns the control point coordinate type setting
   *
   */
  SurfacePoint::CoordinateType BundleSettings::controlPointCoordTypeReports() const {
    return (m_cpCoordTypeReports);
  }


  /**
   * Indicates the control point coordinate type for the actual bundle adjust.
   *
   * This method returns the control point coordinate setting for performing the  bundle adjust.
   *
   * @return @b SurfacePoint::CoordinateType Returns the control point coordinate type setting
   *
   * @see BundleAdjust::errorPropagation()
   */
  SurfacePoint::CoordinateType BundleSettings::controlPointCoordTypeBundle() const {
    return (m_cpCoordTypeBundle);
  }


  /**
   * Indicates if the settings will allow the inverse correlation matrix to be created.
   *
   * This method is used to determine if the inverse correlation matrix file will be created when
   * creating error propagation information in the bundle adjust. If error propagation is not
   * turned on, then the inverse correlation matrix file will not be created.
   *
   * @return @b bool Returns whether or now the inverse correlation matrix is allowed to be created.
   *
   * @see BundleAdjust::errorPropagation()
   */
  bool BundleSettings::createInverseMatrix() const {
    return (m_errorPropagation && m_createInverseMatrix);
  }


  /**
   * This method is used to determine whether outlier rejection will be
   * performed on this bundle adjustment.
   *
   * @return @b bool Indicates whether to perform automatic outlier
   *                 rejection during the bundle adjustment.
   */
  bool BundleSettings::outlierRejection() const {
    return m_outlierRejection;
  }


  /**
   * This method is used to determine whether this bundle adjustment will solve
   * for observation mode.
   *
   * @return @b bool Indicates whether to solve for observation mode.
   */
  bool BundleSettings::solveObservationMode() const {
    return m_solveObservationMode;
  }


  /**
   * This method is used to determine whether this bundle adjustment will solve
   * for radius.
   *
   * @return @b bool Indicates whether to solve for radius.
   */
  bool BundleSettings::solveRadius() const {
    return m_solveRadius;
  }


  /**
   * This method is used to determine whether this bundle
   * adjustment will update the cube labels.
   *
   * @return @b bool Indicates whether to update the cube labels after the bundle adjustment
   *                 is completed.
   */
  bool BundleSettings::updateCubeLabel() const {
    return m_updateCubeLabel;
  }


  /**
   * This method is used to determine whether this bundle adjustment will
   * perform error propagation.
   *
   * @return @b bool Indicates whether to perform error propagation.
   */
  bool BundleSettings::errorPropagation() const {
    return m_errorPropagation;
  }


  /**
   * Turn the creation of the inverse correlation matrix file on or off.
   *
   * Note that the inverse correlation matrix is created in BundleAdjust, and will only be created
   * if error propagation is turned on. By default, BundleSettings allows the inverse matrix to
   * be created. This requires stand-alone applications (e.g. jigsaw) to call this method
   * to turn of the correlation matrix creation.
   *
   * @param createMatrixFile Boolean indicating whether or not to allow the inverse matrix file to
   *                         be created.
   *
   * @see BundleAdjust::errorPropagation()
   */
  void BundleSettings::setCreateInverseMatrix(bool createMatrixFile) {
    m_createInverseMatrix = createMatrixFile;
  }


  /**
   * Retrieves the outlier rejection multiplier for the bundle adjustment.
   *
   * @return @b double The outlier rejection multiplier.
   */
  double BundleSettings::outlierRejectionMultiplier() const {
    return m_outlierRejectionMultiplier;
  }


  /**
   * Retrieves global a priori sigma for 1st coordinate of points for this bundle
   *
   * @return @b double The global a priori sigma for point coordinate 1.
   */
  double BundleSettings::globalPointCoord1AprioriSigma() const {
    return m_globalPointCoord1AprioriSigma;
  }


  /**
   * Retrieves the global a priori sigma for 2nd coordinate of points for this bundle
   *
   * @return @b double The global a priori sigma for point coordinate 2.
   */
  double BundleSettings::globalPointCoord2AprioriSigma() const {
    return m_globalPointCoord2AprioriSigma;
  }


  /**
   * Retrieves the global a priori sigma 3rd coordinate of points for this bundle
   *
   * @return @b double The global a priori sigma for point coordinate 3.
   */
  double BundleSettings::globalPointCoord3AprioriSigma() const {
    return m_globalPointCoord3AprioriSigma;
  }


  /**
   * Retrieves the number of observation solve settings.
   *
   * @return @b int The number of solve settings object for this run of the
   *                bundle adjustment.
   */
  int BundleSettings::numberSolveSettings() const {
     return m_observationSolveSettings.size();
  }


  /**
   * Retrieves solve settings for the observation corresponding to the given observation number.
   * If no corresponding settings object exists, return a new solve settings with no related
   * observation numbers.
   *
   * @param observationNumber The observation number associated with the
   *                          BundleObservationSolveSettings object to be accessed.
   *
   * @return @b BundleObservationSolveSettings The observation settings object that contains
   *                                           the observation number passed.
   */
  BundleObservationSolveSettings
      BundleSettings::observationSolveSettings(QString observationNumber) const {

    BundleObservationSolveSettings defaultSolveSettings;

    for (int i = 0; i < numberSolveSettings(); i++) {
      if (m_observationSolveSettings[i].observationNumbers().contains(observationNumber)) {
        return m_observationSolveSettings[i];
      }
    }
    return defaultSolveSettings;
  }


  /**
   * Retrieves solve settings for the observation corresponding to the given
   * index.
   *
   * @param n The index of the BundleObservationSolveSettings object to be
   *          accessed.
   * @return @b BundleObservationSolveSettings The observation settings object corresponding
   *                                           to the given index.
   * @throw IException::Unknown "Unable to find BundleObservationSolveSettings
   *                             with given index"
   */
  BundleObservationSolveSettings
      BundleSettings::observationSolveSettings(int n) const {

    if (n >= 0 && n < numberSolveSettings()) {
      return m_observationSolveSettings[n];
    }
    std::string msg = "Unable to find BundleObservationSolveSettings with index = ["
                  + toString(n) + "].";
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }


  /**
   * Retrieves solve settings for the observation corresponding to the given index.
   *
   * @return QList<BundleObservationSolveSettings> The QList of BundleObservationSolveSettings
   *                              objects
   */
  QList<BundleObservationSolveSettings> BundleSettings::observationSolveSettings() const {
    return m_observationSolveSettings;
  }


  // =============================================================================================//
  // ======================== Convergence Criteria ===============================================//
  // =============================================================================================//

  /**
   * Converts the given string value to a BundleSettings::ConvergenceCriteria
   * enumeration. Currently accepted inputs are listed below. This method is
   * case insensitive.
   * <ul>
   *   <li>Sigma0</li>
   *   <li>ParameterCorrections</li>
   * </ul>
   *
   * @param criteria Convergence criteria name to be converted.
   *
   * @return @b ConvergenceCriteria The enumeration corresponding to the given name.
   *
   * @throw Isis::Exception::Programmer "Unknown bundle convergence criteria."
   */
  BundleSettings::ConvergenceCriteria
      BundleSettings::stringToConvergenceCriteria(QString criteria) {
    if (criteria.compare("SIGMA0", Qt::CaseInsensitive) == 0) {
      return BundleSettings::Sigma0;
    }
    else if (criteria.compare("PARAMETERCORRECTIONS", Qt::CaseInsensitive) == 0) {
      return BundleSettings::ParameterCorrections;
    }
    else throw IException(IException::Programmer,
                          "Unknown bundle convergence criteria [" + criteria.toStdString() + "].",
                          _FILEINFO_);
  }


  /**
   * Converts the given BundleSettings::ConvergenceCriteria enumeration to a string.
   * This method is used to print the type of convergence criteria used in
   * the bundle adjustment.
   *
   * @param criteria The ConvergenceCriteria enumeration to be converted.
   *
   * @return @b QString The name associated with the given convergence criteria.
   *
   * @throw Isis::Exception::Programmer "Unknown bundle convergence criteria enum."
   */
  QString BundleSettings::convergenceCriteriaToString(
              BundleSettings::ConvergenceCriteria criteria) {
    if (criteria == Sigma0)                    return "Sigma0";
    else if (criteria == ParameterCorrections) return "ParameterCorrections";
    else  throw IException(IException::Programmer,
                           "Unknown convergence criteria enum [" + toString(criteria) + "].",
                           _FILEINFO_);
  }


  /**
   * Set the convergence criteria options for the bundle adjustment.
   *
   * @param criteria An enumeration for the convergence criteria to be
   *                 used for this bundle adjustment.
   * @param threshold The convergence threshold for this bundle adjustment.
   * @param maximumIterations The maximum number of iterations allowed
   *                          before the bundle adjustment determines
   *                          that the data is not converging.
   */
  void BundleSettings::setConvergenceCriteria(BundleSettings::ConvergenceCriteria criteria,
                                              double threshold,
                                              int maximumIterations) {
    m_convergenceCriteria = criteria;
    m_convergenceCriteriaThreshold = threshold;
    m_convergenceCriteriaMaximumIterations = maximumIterations;
  }


  /**
   * Retrieves the convergence criteria to be used to solve the bundle
   * adjustment.
   *
   * @return @b ConvergenceCriteria The enumeration of the convergence criteria.
   */
  BundleSettings::ConvergenceCriteria BundleSettings::convergenceCriteria() const {
    return m_convergenceCriteria;
  }


  /**
   * Retrieves the convergence threshold to be used to solve the bundle
   * adjustment.
   *
   * @return @b double The threshold that determines convergence.
   */
  double BundleSettings::convergenceCriteriaThreshold() const {
    return m_convergenceCriteriaThreshold;
  }


  /**
   * Retrieves the maximum number of iterations allowed to solve the
   * bundle adjustment.
   *
   * @param maximumIterations The maximum number of iterations allowed
   *                          before the bundle adjustment determines
   *                          that the data is not converging.
   */
  int BundleSettings::convergenceCriteriaMaximumIterations() const {
    return m_convergenceCriteriaMaximumIterations;
  }



  // =============================================================================================//
  // ======================== Parameter Uncertainties (Weighting) ================================//
  // =============================================================================================//
//   void BundleSettings::setGlobalLatitudeAprioriSigma(double sigma) {
//     m_globalLatitudeAprioriSigma = sigma;
//   }
//
//
//
//   void BundleSettings::setGlobalLongitudeAprioriSigma(double sigma) {
//     m_globalLongitudeAprioriSigma = sigma;
//   }
//
//
//
//   void BundleSettings::setGlobalRadiiAprioriSigma(double sigma) {
//     m_globalRadiusAprioriSigma = sigma;
//   }


  // =============================================================================================//
  // ======================== Maximum Likelihood Estimation Options ==============================//
  // =============================================================================================//


  /**
   * Add a maximum likelihood estimator (MLE) model to the bundle adjustment.
   *
   * @param model The enumeration for the model to be used.
   * @param maxModelCQuantile The C-Quantile of the residual to be used to
   *                          compute the tweaking constant.
   *
   *
   * @throw Isis::Exception::Programmer "For bundle adjustments with multiple maximum
   *                                     likelihood estimators, the first model must be of
   *                                     type HUBER or HUBER_MODIFIED."
   */
  void BundleSettings::addMaximumLikelihoodEstimatorModel(MaximumLikelihoodWFunctions::Model model,
                                                          double maxModelCQuantile) {

    if (m_maximumLikelihood.size() == 0 && model > MaximumLikelihoodWFunctions::HuberModified) {
      std::string msg = "For bundle adjustments with multiple maximum likelihood estimators, the first "
                    "model must be of type HUBER or HUBER_MODIFIED.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    m_maximumLikelihood.append(qMakePair(model, maxModelCQuantile));
  }


  /**
   * Retrieves the list of maximum likelihood estimator (MLE) models with their
   * corresponding C-Quantiles.
   *
   * @return QList< QPair< MaximumLikelihoodWFunctions::Model, double > >
   *             The list of tuples of the form (model, quantile) to be used for the
   *             bundle adjustment.
   */
  QList< QPair< MaximumLikelihoodWFunctions::Model, double > >
      BundleSettings::maximumLikelihoodEstimatorModels() const {
    return m_maximumLikelihood;
  }


  // =============================================================================================//
  // ======================== Self Calibration ??? (from ipce only) =========================//
  // =============================================================================================//

  // =============================================================================================//
  // ======================== Target Body ??? (from ipce only) ==============================//
  // =============================================================================================//

  /**
   * Sets the target body for the bundle adjustment.
   *
   * @param bundleTargetBody A pointer to the BundleTargetBody object for the
   *                         bundle adjustment to be run.
   */
  void BundleSettings::setBundleTargetBody(BundleTargetBodyQsp bundleTargetBody) {
    m_bundleTargetBody = bundleTargetBody;
  }


  /**
   * Retrieves a pointer to target body information for the bundle adjustment.
   *
   * @return @b BundleTargetBodyQsp A pointer to the BundleTargetBody object for
   *                                the bundle adjustment to be run.
   */
  BundleTargetBodyQsp BundleSettings::bundleTargetBody() const {
    return m_bundleTargetBody;
  }


  /**
   * This method is used to determine whether the bundle adjustment
   * will solve for target body pole position.
   *
   * @return @b bool Indicates whether to solve for target pole position.
   */
//  bool BundleSettings::solveTargetBodyPolePosition() const {
//    return m_solveTargetBodyPolePosition;
//  }


  /**
   * Retrieves the number of target body parameters. If the BundleTargetBody
   * associated with this bundle adjustment is NULL, this method returns 0.
   *
   * @return @b int The number of target body parameters.
   */
  int BundleSettings::numberTargetBodyParameters() const {
    if (!m_bundleTargetBody)
      return 0;

    return m_bundleTargetBody->numberParameters();
  }


  /**
   * This method is used to determine whether the bundle adjustment will solve
   * for target body.
   *
   * @return @b bool Indicates whether to solve for target body.
   */
  bool BundleSettings::solveTargetBody() const {
    if (!m_bundleTargetBody) {
      return false;
    }
    else {
      return (m_bundleTargetBody->numberParameters() > 0);
    }
  }


  /**
   * This method is used to determine whether the bundle adjustment will
   * solve for target body pole right ascension.
   *
   * @see BundleTargetBody::solvePoleRA()
   *
   * @return @b bool Indicates whether to solve for target pole RA.
   */
  bool BundleSettings::solvePoleRA() const {
    if (!m_bundleTargetBody) {
      return false;
    }
    return m_bundleTargetBody->solvePoleRA();
  }


  /**
   * This method is used to determine whether the bundle adjustment will
   * solve for target body pole right ascension velocity.
   *
   * @see BundleTargetBody::solvePoleRAVelocity()
   *
   * @return @b bool Indicates whether to solve for target pole RA velocity.
   */
  bool BundleSettings::solvePoleRAVelocity() const {
    if (!m_bundleTargetBody) {
      return false;
    }
    return m_bundleTargetBody->solvePoleRAVelocity();
  }


  /**
   * This method is used to determine whether the bundle adjustment will
   * solve for target body pole declination.
   *
   * @see BundleTargetBody::solvePoleDeclination()
   *
   * @return @b bool Indicates whether to solve for target pole declination.
   */
  bool BundleSettings::solvePoleDec() const {
    if (!m_bundleTargetBody) {
      return false;
    }
    return m_bundleTargetBody->solvePoleDec();
  }


  /**
   * This method is used to determine whether the bundle adjustment will
   * solve for target body pole declination velocity.
   *
   * @see BundleTargetBody::solvePoleDeclinationVelocity()
   *
   * @return @b bool Indicates whether to solve for target pole declination velocity.
   */
  bool BundleSettings::solvePoleDecVelocity() const {
    if (!m_bundleTargetBody) {
      return false;
    }
    return m_bundleTargetBody->solvePoleDecVelocity();
  }


  /**
   * This method is used to determine whether the bundle adjustment
   * will solve for target body prime meridian.
   *
   * @see BundleTargetBody::solvePM()
   *
   * @return @b bool Indicates whether to solve for target PM.
   */
  bool BundleSettings::solvePM() const {
    if (!m_bundleTargetBody) {
      return false;
    }
    return m_bundleTargetBody->solvePM();
  }


  /**
   * This method is used to determine whether the bundle adjustment
   * will solve for target body prime meridian velocity.
   *
   * @see BundleTargetBody::solvePMVelocity()
   *
   * @return @b bool Indicates whether to solve for target PM velocity.
   */
  bool BundleSettings::solvePMVelocity() const {
    if (!m_bundleTargetBody) {
      return false;
    }
    return m_bundleTargetBody->solvePMVelocity();
  }


  /**
   * This method is used to determine whether the bundle adjustment
   * will solve for target body prime meridian acceleration.
   *
   * @see BundleTargetBody::solvePMAcceleration()
   *
   * @return @b bool Indicates whether to solve for target PM acceleration.
   */
  bool BundleSettings::solvePMAcceleration() const {
    if (!m_bundleTargetBody) {
      return false;
    }
    return m_bundleTargetBody->solvePMAcceleration();
  }


  /**
   * This method is used to determine whether the bundle adjustment
   * will solve for target body triaxial radii.
   *
   * @see BundleTargetBody::solveTriaxialRadii()
   *
   * @return @b bool Indicates whether to solve for target triaxial radii.
   */
  bool BundleSettings::solveTriaxialRadii() const {
    if (!m_bundleTargetBody) {
      return false;
    }
    return m_bundleTargetBody->solveTriaxialRadii();
  }


  /**
   * This method is used to determine whether the bundle adjustment
   * will solve for target body mean radius.
   *
   * @see BundleTargetBody::solveMeanRadius()
   *
   * @return @b bool Indicates whether to solve for target mean radius.
   */
  bool BundleSettings::solveMeanRadius() const {
    if (!m_bundleTargetBody) {
      return false;
    }
    return m_bundleTargetBody->solveMeanRadius();
  }


//  void BundleSettings::setTargetBodySolveOptions(bool solveTargetBodyPolePosition,
//                                                 double aprioriRaPole, double sigmaRaPole,
//                                                 double aprioriDecPole, double sigmaDecPole,
//                                                 bool solveTargetBodyZeroMeridian,
//                                                 double aprioriW0, double sigmaW0,
//                                                 bool solveTargetBodyRotationRate,
//                                                 double aprioriWDot, double sigmaWDot,
//                                                 TargetRadiiSolveMethod solveRadiiMethod,
//                                                 double aprioriRadiusA, double sigmaRadiusA,
//                                                 double aprioriRadiusB, double sigmaRadiusB,
//                                                 double aprioriRadiusC, double sigmaRadiusC,
//                                                 double aprioriMeanRadius, double sigmaMeanRadius) {


//    m_solveTargetBody = true;
//    m_solveTargetBodyPolePosition = solveTargetBodyPolePosition;
//    m_aprioriRaPole = aprioriRaPole;
//    m_sigmaRaPole = sigmaRaPole;
//    m_aprioriDecPole = aprioriDecPole;
//    m_sigmaDecPole = sigmaDecPole;
//    m_solveTargetBodyZeroMeridian =solveTargetBodyZeroMeridian;
//    m_aprioriW0 = aprioriW0;
//    m_sigmaW0 = sigmaW0;
//    m_solveTargetBodyRotationRate = solveTargetBodyRotationRate;
//    m_aprioriWDot = aprioriWDot;
//    m_sigmaWDot = sigmaWDot;
//    m_solveTargetBodyRadiusMethod = solveRadiiMethod;
//    m_aprioriRadiusA = aprioriRadiusA;
//    m_sigmaRadiusA = sigmaRadiusA;
//    m_aprioriRadiusB = aprioriRadiusB;
//    m_sigmaRadiusB = sigmaRadiusB;
//    m_aprioriRadiusC = aprioriRadiusC;
//    m_sigmaRadiusC = sigmaRadiusC;
//    m_aprioriMeanRadius = aprioriMeanRadius;
//    m_sigmaMeanRadius = sigmaMeanRadius;

//    m_bundleTargetBody->setSolveSettings(solveTargetBodyPolePosition, aprioriRaPole, sigmaRaPole,
//                                         aprioriDecPole, sigmaDecPole, solveTargetBodyZeroMeridian,
//                                         aprioriW0, sigmaW0, solveTargetBodyRotationRate,
//                                         aprioriWDot, sigmaWDot, solveRadiiMethod, aprioriRadiusA,
//                                         sigmaRadiusA, aprioriRadiusB, sigmaRadiusB, aprioriRadiusC,
//                                         sigmaRadiusC, aprioriMeanRadius, sigmaMeanRadius);
//  }


  // =============================================================================================//
  // ========================= Output Options (from Jigsaw only) ================================//
  // =============================================================================================//
  /**
   * Set the output file prefix for the bundle adjustment.
   *
   * @param outputFilePrefix A string containing a prefix and/or directory path
   */
  void BundleSettings::setOutputFilePrefix(QString outputFilePrefix) {
    m_outputFilePrefix = outputFilePrefix;
  }


  /**
   * Retrieve the output file prefix. This string will be
   * appended to all of the output files created by the bundle
   * adjustment.
   *
   * @return @b QString A string containing a prefix and/or
   *                    directory path to be appended to all
   *                    output files.
   */
  QString BundleSettings::outputFilePrefix() const {
    return m_outputFilePrefix;
  }


  /**
   * @brief This method is used to write a BundleSettings object in an XML format
   *
   * @param stream The stream to write serialized XML output
   * @param project The project that contains the settings
   * @internal
   *   @history 2017-05-30 Debbie A. Cook - Added controlPointCoordType to the xml stream
   *                           and made global coordinate names generic.
   */
  void BundleSettings::save(QXmlStreamWriter &stream, const Project *project) const {
    stream.writeStartElement("bundleSettings");

    stream.writeStartElement("globalSettings");

    stream.writeTextElement("validateNetwork", QString::number(validateNetwork()));

    stream.writeStartElement("solveOptions");
    stream.writeAttribute("solveObservationMode", QString::number(solveObservationMode()));
    stream.writeAttribute("solveRadius", QString::number(solveRadius()));
    stream.writeAttribute("controlPointCoordTypeReports", QString::number(controlPointCoordTypeReports()));
    stream.writeAttribute("controlPointCoordTypeBundle", QString::number(controlPointCoordTypeBundle()));
    stream.writeAttribute("updateCubeLabel", QString::number(updateCubeLabel()));
    stream.writeAttribute("errorPropagation", QString::number(errorPropagation()));
    stream.writeAttribute("createInverseMatrix", QString::number(createInverseMatrix()));
    stream.writeEndElement();

    stream.writeStartElement("aprioriSigmas");
    if (IsSpecial(globalPointCoord1AprioriSigma())) {
      stream.writeAttribute("pointCoord1", "N/A");
    }
    else {
      stream.writeAttribute("pointCoord1", QString::number(globalPointCoord1AprioriSigma()));
    }
    if (IsSpecial(globalPointCoord2AprioriSigma())) {
      stream.writeAttribute("pointCoord2", "N/A");
    }
    else {
      stream.writeAttribute("pointCoord2", QString::number(globalPointCoord2AprioriSigma()));
    }
    if (IsSpecial(globalPointCoord3AprioriSigma())) {
      stream.writeAttribute("pointCoord3", "N/A");
    }
    else {
      stream.writeAttribute("pointCoord3", QString::number(globalPointCoord3AprioriSigma()));
    }
    stream.writeEndElement();

    stream.writeStartElement("outlierRejectionOptions");
    stream.writeAttribute("rejection", QString::number(outlierRejection()));
    if (outlierRejection()) {
      stream.writeAttribute("multiplier", QString::number(outlierRejectionMultiplier()));
    }
    else {
      stream.writeAttribute("multiplier", "N/A");
    }
    stream.writeEndElement();

    stream.writeStartElement("convergenceCriteriaOptions");
    stream.writeAttribute("convergenceCriteria",
                          convergenceCriteriaToString(convergenceCriteria()));
    stream.writeAttribute("threshold",
                          QString::number(convergenceCriteriaThreshold()));
    stream.writeAttribute("maximumIterations",
                          QString::number(convergenceCriteriaMaximumIterations()));
    stream.writeEndElement();

    stream.writeStartElement("maximumLikelihoodEstimation");
    for (int i = 0; i < m_maximumLikelihood.size(); i++) {
      stream.writeStartElement("model");
      stream.writeAttribute("type",
                          MaximumLikelihoodWFunctions::modelToString(m_maximumLikelihood[i].first));
      stream.writeAttribute("quantile", QString::number(m_maximumLikelihood[i].second));
      stream.writeEndElement();
    }
    stream.writeEndElement();

    stream.writeStartElement("outputFileOptions");
    stream.writeAttribute("fileNamePrefix", outputFilePrefix());
    stream.writeEndElement();

    stream.writeEndElement(); // end global settings

    if (!m_observationSolveSettings.isEmpty()) {
      stream.writeStartElement("observationSolveSettingsList");
      for (int i = 0; i < m_observationSolveSettings.size(); i++) {
        m_observationSolveSettings[i].save(stream, project);
      }
      stream.writeEndElement();
    }
    else {
      // throw error??? should not write if no observation settings...
    }
    stream.writeEndElement();
  }

  void BundleSettings::readBundleSettings(QXmlStreamReader *xmlReader) {
    init();
    Q_ASSERT(xmlReader->name() == "bundleSettings");
    while (xmlReader->readNextStartElement()) {
      if (xmlReader->qualifiedName() == "globalSettings") {
        while (xmlReader->readNextStartElement()) {
          if (xmlReader->qualifiedName() == "validateNetwork") {
            QString validateNetwork = xmlReader->readElementText();
            if (!validateNetwork.isEmpty()) {
              m_validateNetwork = toBool(validateNetwork.toStdString());
            }       
          }
          else if (xmlReader->qualifiedName() == "solveOptions") {
            QStringRef solveObservationMode = xmlReader->attributes().value("solveObservationMode");
            if (!solveObservationMode.isEmpty()) {
              m_solveObservationMode = toBool(solveObservationMode.toString().toStdString());
            }
            QStringRef solveRadius = xmlReader->attributes().value("solveRadius");
            if (!solveRadius.isEmpty()) {
              m_solveRadius = toBool(solveRadius.toString().toStdString());
            }
            QStringRef controlPointCoordTypeReports = xmlReader->attributes().value("controlPointCoordTypeReports");
            if (!controlPointCoordTypeReports.isEmpty()) {
              if (controlPointCoordTypeReports == "0") {
                m_cpCoordTypeReports = SurfacePoint::Latitudinal;
              }
              else if (controlPointCoordTypeReports == "1") {
                m_cpCoordTypeReports = SurfacePoint::Rectangular;
              }           
            }
            QStringRef controlPointCoordTypeBundle = xmlReader->attributes().value("controlPointCoordTypeBundle");
            if (!controlPointCoordTypeBundle.isEmpty()) {
              if (controlPointCoordTypeBundle == "0") {
                m_cpCoordTypeBundle = SurfacePoint::Latitudinal;
              }
              else if (controlPointCoordTypeBundle == "1") {
                m_cpCoordTypeBundle = SurfacePoint::Rectangular;
              }   
            }
            QStringRef updateCubeLabel = xmlReader->attributes().value("updateCubeLabel");
            if (!updateCubeLabel.isEmpty()) {
              m_updateCubeLabel = toBool(updateCubeLabel.toString().toStdString());
            }
            QStringRef errorPropagation = xmlReader->attributes().value("errorPropagation");
            if (!errorPropagation.isEmpty()) {
              m_errorPropagation = toBool(errorPropagation.toString().toStdString());
            }
            QStringRef createInverseMatrix = xmlReader->attributes().value("createInverseMatrix");
            if (!createInverseMatrix.isEmpty()) {
              m_createInverseMatrix = toBool(createInverseMatrix.toString().toStdString());
            }
            xmlReader->skipCurrentElement();
          }
          else if (xmlReader->qualifiedName() == "aprioriSigmas") {
            QStringRef globalPointCoord1AprioriSigma = xmlReader->attributes().value("pointCoord1");
            if (!globalPointCoord1AprioriSigma.isEmpty()) {
              if (globalPointCoord1AprioriSigma == "N/A") {
                m_globalPointCoord1AprioriSigma = Isis::Null;
              }
              else {
                m_globalPointCoord1AprioriSigma = globalPointCoord1AprioriSigma.toDouble();
              }
            }
            QStringRef globalPointCoord2AprioriSigma = xmlReader->attributes().value("pointCoord2");
            if (!globalPointCoord2AprioriSigma.isEmpty()) {
              if (globalPointCoord2AprioriSigma == "N/A") {
                m_globalPointCoord2AprioriSigma = Isis::Null;
              }
              else {
                m_globalPointCoord2AprioriSigma = globalPointCoord2AprioriSigma.toDouble();
              }
            }
            QStringRef globalPointCoord3AprioriSigma = xmlReader->attributes().value("radius");
            if (!globalPointCoord3AprioriSigma.isEmpty()) {
              if (globalPointCoord3AprioriSigma == "N/A") {
                m_globalPointCoord3AprioriSigma = Isis::Null;
              }
              else {
                m_globalPointCoord3AprioriSigma = globalPointCoord3AprioriSigma.toDouble();
              }
            }
            xmlReader->skipCurrentElement();
          }
          else if (xmlReader->qualifiedName() == "outlierRejectionOptions") {
            QStringRef outlierRejection = xmlReader->attributes().value("rejection");
            if (!outlierRejection.isEmpty()) {
              m_outlierRejection = toBool(outlierRejection.toString().toStdString());
            }
            QStringRef outlierRejectionMultiplier = xmlReader->attributes().value("multiplier");
            if (!outlierRejectionMultiplier.isEmpty()) {
              if (outlierRejectionMultiplier != "N/A") {
                m_outlierRejectionMultiplier = outlierRejectionMultiplier.toDouble();
              }
              else {
                m_outlierRejectionMultiplier = 3.0;
              }
            }
            xmlReader->skipCurrentElement();
          }
          else if (xmlReader->qualifiedName() == "convergenceCriteriaOptions") {
            QStringRef convergenceCriteria = xmlReader->attributes().value("convergenceCriteria");
            if (!convergenceCriteria.isEmpty()) {
              m_convergenceCriteria = stringToConvergenceCriteria(convergenceCriteria.toString());
            }
            QStringRef threshold = xmlReader->attributes().value("threshold");
            if (!threshold.isEmpty()) {
              m_convergenceCriteriaThreshold = threshold.toDouble();
            }
            QStringRef maximumIterations = xmlReader->attributes().value("maximumIterations");
            if (!maximumIterations.isEmpty()) {
              m_convergenceCriteriaMaximumIterations = maximumIterations.toInt();
            }
            xmlReader->skipCurrentElement();
          }
          else if (xmlReader->qualifiedName() == "maximumLikelihoodEstimation") {
            while (xmlReader->readNextStartElement()) {
              if (xmlReader->qualifiedName() == "model") {
                QStringRef type = xmlReader->attributes().value("type");
                QStringRef quantile = xmlReader->attributes().value("quantile");
                if (!type.isEmpty() && !quantile.isEmpty()) {
                  m_maximumLikelihood.append(qMakePair(MaximumLikelihoodWFunctions::stringToModel(type.toString()), quantile.toDouble()));
                }
                xmlReader->skipCurrentElement();
              }
              else {
                xmlReader->skipCurrentElement();
              }
            }
          }
          else if (xmlReader->qualifiedName() == "outputFileOptions") {
            QStringRef fileNamePrefix = xmlReader->attributes().value("fileNamePrefix");
            if (!fileNamePrefix.isEmpty()) {
              m_outputFilePrefix = fileNamePrefix.toString();
            }
            xmlReader->skipCurrentElement();
          }
          else {
            xmlReader->skipCurrentElement();
          }
        }
      }
      else if (xmlReader->qualifiedName() == "observationSolveSettingsList") {
        m_observationSolveSettings.clear();
        while (xmlReader->readNextStartElement()) {
          if (xmlReader->qualifiedName() == "bundleObservationSolveSettings") {
            BundleObservationSolveSettings *settings = new BundleObservationSolveSettings(xmlReader);
            m_observationSolveSettings.append(*settings);
          }
          else {
            xmlReader->skipCurrentElement();
          }
        }
      }
      else {
        xmlReader->skipCurrentElement();
      }
    }
  }
}
