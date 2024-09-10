/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "CsmBundleObservation.h"

#include <QDebug>
#include <QString>
#include <QStringList>
#include <QVector>

#include "BundleImage.h"
#include "BundleControlPoint.h"
#include "BundleObservationSolveSettings.h"
#include "BundleTargetBody.h"
#include "Camera.h"
#include "CSMCamera.h"
#include "LinearAlgebra.h"
#include "SpicePosition.h"
#include "SpiceRotation.h"

using namespace std;

namespace Isis {

  /**
   * Constructs a CsmBundleObservation initialized to a default state.
   */
  CsmBundleObservation::CsmBundleObservation() {
  }


  /**
   * Constructs a CsmBundleObservation from a BundleImage, an instrument id, an observation
   * number to assign to this CsmBundleObservation, and a target body.
   *
   * @param image QSharedPointer to the primary image in the observation
   * @param observationNumber Observation number of the observation
   * @param instrumentId Id of the instrument for the observation
   * @param bundleTargetBody QSharedPointer to the target body of the observation
   */
  CsmBundleObservation::CsmBundleObservation(BundleImageQsp image, QString observationNumber,
                                       QString instrumentId, BundleTargetBodyQsp bundleTargetBody) : BundleObservation(image, observationNumber, instrumentId, bundleTargetBody) {
    if (bundleTargetBody) {
      std::string msg = "Target body parameters cannot be solved for with CSM observations.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }


  /**
   * Creates a copy of another CsmBundleObservation.
   *
   * @param src Reference to the CsmBundleObservation to copy
   */
  CsmBundleObservation::CsmBundleObservation(const CsmBundleObservation &src) : BundleObservation(src) {
    m_solveSettings = src.m_solveSettings;
    m_paramIndices = src.m_paramIndices;
  }


  /**
   * Destructor.
   *
   * Contained BundleImages will remain until all shared pointers are deleted.
   */
  CsmBundleObservation::~CsmBundleObservation() {
  }


  /**
   * Assignment operator
   *
   * Assigns the state of the source CsmBundleObservation to this CsmBundleObservation
   *
   * @param CsmBundleObservation Reference to the source CsmBundleObservation to assign from
   *
   * @return @b CsmBundleObservation& Reference to this CsmBundleObservation
   */
  CsmBundleObservation &CsmBundleObservation::operator=(const CsmBundleObservation &src) {
    if (&src != this) {
      m_solveSettings = src.m_solveSettings;
      m_paramIndices = src.m_paramIndices;
    }
    return *this;
  }


  /**
   * Set solve parameters
   *
   * @param solveSettings The solve settings to use
   *
   * @return @b bool Returns true if settings were successfully set
   */
  bool CsmBundleObservation::setSolveSettings(BundleObservationSolveSettings solveSettings) {
    m_solveSettings = BundleObservationSolveSettingsQsp(
                        new BundleObservationSolveSettings(solveSettings));

    CSMCamera *csmCamera = dynamic_cast<CSMCamera*>(front()->camera());

    m_paramIndices.clear();
    m_weights.clear();
    m_corrections.clear();
    m_adjustedSigmas.clear();

    if (m_solveSettings->csmSolveOption() == BundleObservationSolveSettings::Set) {
      m_paramIndices = csmCamera->getParameterIndices(m_solveSettings->csmParameterSet());
    }
    else if (m_solveSettings->csmSolveOption() == BundleObservationSolveSettings::Type) {
      m_paramIndices = csmCamera->getParameterIndices(m_solveSettings->csmParameterType());
    }
    else if (m_solveSettings->csmSolveOption() == BundleObservationSolveSettings::List) {
      m_paramIndices = csmCamera->getParameterIndices(m_solveSettings->csmParameterList());
    }
    else {
      return false;
    }

    int nParams = m_paramIndices.size();

    m_weights.resize(nParams);
    m_corrections.resize(nParams);
    m_adjustedSigmas.resize(nParams);
    m_aprioriSigmas.resize(nParams);

    for (int i = 0; i < nParams; i++) {
      m_aprioriSigmas[i] = csmCamera->getParameterCovariance(m_paramIndices[i], m_paramIndices[i]);
    }

    return true;
  }


  /**
   * Accesses the solve settings
   *
   * @return @b const BundleObservationSolveSettingsQsp Returns a pointer to the solve
   *                                                    settings for this CsmBundleObservation
   */
  const BundleObservationSolveSettingsQsp CsmBundleObservation::solveSettings() {
    return BundleObservationSolveSettingsQsp(nullptr);
  }


  /**
   * Applies the parameter corrections
   *
   * @param corrections Vector of corrections to apply
   *
   * @throws IException::Unknown "Instrument position is NULL, but position solve option is
   *                              [not NoPositionFactors]"
   * @throws IException::Unknown "Instrument position is NULL, but pointing solve option is
   *                              [not NoPointingFactors]"
   * @throws IException::Unknown "Unable to apply parameter corrections to CsmBundleObservation."
   *
   * @return @b bool Returns true upon successful application of corrections
   */
  bool CsmBundleObservation::applyParameterCorrections(LinearAlgebra::Vector corrections) {
    // Check that the correction vector is the correct size
    if (corrections.size() != m_paramIndices.size()) {
      std::string msg = "Invalid correction vector passed to observation.";
      IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Apply the corrections to the CSM camera
    CSMCamera *csmCamera = dynamic_cast<CSMCamera*>(front()->camera());
    for (size_t i = 0; i < corrections.size(); i++) {
      csmCamera->applyParameterCorrection(m_paramIndices[i], corrections[i]);
    }

    // Accumulate the total corrections
    m_corrections += corrections;

    return true;
  }


  /**
   * Returns the number of total parameters there are for solving
   *
   * The total number of parameters is equal to the number of position parameters and number of
   * pointing parameters
   *
   * @return @b int Returns the number of parameters there are
   */
  int CsmBundleObservation::numberParameters() {
    return m_paramIndices.size();
  }


  /**
   * @brief Takes in an open std::ofstream and writes out information which goes into the
   * bundleout.txt file.
   *
   * @param fpOut The open std::ofstream object which is passed in from
   * BundleSolutionInfo::outputText()
   * @param errorPropagation Boolean indicating whether or not to attach more information
   *     (corrections, sigmas, adjusted sigmas...) to the output.
   */
  void CsmBundleObservation::bundleOutputString(std::ostream &fpOut, bool errorPropagation) {

    char buf[4096];

    QVector<double> finalParameterValues;
    CSMCamera *csmCamera = dynamic_cast<CSMCamera*>(front()->camera());

    int nParameters = numberParameters();

    QStringList parameterNamesList;
    QStringList parameterUnitList;

    for (int i = 0; i < nParameters; i++) {
      parameterNamesList.append(csmCamera->getParameterName(m_paramIndices[i]));
      parameterUnitList.append(csmCamera->getParameterUnits(m_paramIndices[i]));
      finalParameterValues.append(csmCamera->getParameterValue(m_paramIndices[i]));
    }


    // Set up default values when we are using default position
    QString sigma;
    QString adjustedSigma;
    double correction;

    for (int i = 0; i < nParameters; i++) {

      correction = m_corrections(i);
      adjustedSigma = QString::number(m_adjustedSigmas[i], 'f', 8);
      sigma = (IsSpecial(m_aprioriSigmas[i]) ? "FREE" : QString::number(m_aprioriSigmas[i], 'g', 8));

      snprintf(buf, sizeof(buf), "%.11s", parameterNamesList.at(i).toStdString().c_str());
      fpOut << buf;
      snprintf(buf, sizeof(buf), "%18.8lf  ", finalParameterValues[i] - correction);
      fpOut << buf;
      snprintf(buf, sizeof(buf), "%20.8lf  ", correction);
      fpOut << buf;
      snprintf(buf, sizeof(buf), "%23.8lf  ", finalParameterValues[i]);
      fpOut << buf;
      snprintf(buf, sizeof(buf), "            ");
      fpOut << buf;
      snprintf(buf, sizeof(buf), "%6s", sigma.toStdString().c_str());
      fpOut << buf;
      snprintf(buf, sizeof(buf), "            ");
      fpOut << buf;
      if (errorPropagation) {
        snprintf(buf, sizeof(buf), "%s", adjustedSigma.toStdString().c_str());
      }
      else {
        snprintf(buf, sizeof(buf),  "%s","N/A");
      }
      fpOut << buf;
      snprintf(buf, sizeof(buf), "        ");
      fpOut << buf;
      snprintf(buf, sizeof(buf), "%s\n", parameterUnitList.at(i).toStdString().c_str());
      fpOut << buf;

    }
  }

  /**
   * @brief Creates and returns a formatted QString representing the bundle coefficients and
   * parameters in csv format.
   *
   * @param errorPropagation Boolean indicating whether or not to attach more information
   *     (corrections, sigmas, adjusted sigmas...) to the output QString
   *
   * @return @b QString Returns a formatted QString representing the CsmBundleObservation in
   * csv format
   */
  QString CsmBundleObservation::bundleOutputCSV(bool errorPropagation) {
    QString finalqStr = "";
    CSMCamera *csmCamera = dynamic_cast<CSMCamera*>(front()->camera());

    for (size_t i = 0; i < m_paramIndices.size(); i++) {
      double finalValue = csmCamera->getParameterValue(m_paramIndices[i]);
      finalqStr += QString::number(finalValue - m_corrections[i]) + ",";
      finalqStr += QString::number(m_corrections[i]) + ",";
      finalqStr += QString::number(finalValue) + ",";
      finalqStr += QString::number(m_aprioriSigmas[i], 'g', 8) + ",";
      if (errorPropagation) {
        finalqStr += QString::number(m_adjustedSigmas[i], 'f', 8) + ",";
      }
      else {
        finalqStr += "N/A,";
      }
    }

    return finalqStr;
  }


  /**
   * Returns the list of observation parameter names.
   *
   * This will always return at least one set of positions and pointings
   * because we always output at least the center values even when not solving
   * for them.
   *
   * @return @b QStringList List of observation parameter names
   */
  QStringList CsmBundleObservation::parameterList() {
    QStringList paramList;
    CSMCamera *csmCamera = dynamic_cast<CSMCamera*>(front()->camera());

    for (int paramIndex : m_paramIndices) {
      paramList.push_back(csmCamera->getParameterName(paramIndex));
    }

    return paramList;
  }

  /**
   * Cannot compute target body parameters for a CSM observation,
   * so always throws an exception.
   *
   * @param coeffTarget Matrix for target body partial derivatives
   * @param measure The measure that the partials are being
   *                computed for.
   * @param bundleSettings The settings for the bundle adjustment
   * @param bundleTargetBody QSharedPointer to the target body of
   *                         the observation
   *
   * @return bool Always false
   */
  bool CsmBundleObservation::computeTargetPartials(LinearAlgebra::Matrix &coeffTarget, BundleMeasure &measure, BundleSettingsQsp &bundleSettings, BundleTargetBodyQsp &bundleTargetBody) {
    if (bundleTargetBody) {
      std::string msg = "Target body parameters cannot be solved for with CSM observations.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    return false;
  }


  /**
   * Calculates the sensor partials with respect to the solve
   * parameters and populates the coeffImage matrix.
   *
   * @param coeffImage A matrix that will be populated with the
   *                   sensor partials with respect to the solve
   *                   parameters.
   * @param measure The measure that the partials are being
   *                computed for.
   *
   * @return bool
   */
  bool CsmBundleObservation::computeImagePartials(LinearAlgebra::Matrix &coeffImage, BundleMeasure &measure) {
    coeffImage.clear();

    CSMCamera *csmCamera = dynamic_cast<CSMCamera*>(measure.camera());
    SurfacePoint groundPoint = measure.parentControlPoint()->adjustedSurfacePoint();

    // Loop over parameters and populate matrix
    for (size_t i = 0; i < m_paramIndices.size(); i++) {
      vector<double> partials = csmCamera->getSensorPartials(m_paramIndices[i], groundPoint);
      coeffImage(0, i) = partials[1];
      coeffImage(1, i) = partials[0];
    }

    return true;
  }


  /**
   * Calculates the ground partials for the line, sample currently
   * set in the sensor model.
   *
   * @param coeffPoint3D A matrix that will be populated with the
   *                     (line, sample) partials with respect to
   *                     the ground point.
   * @param measure The measure that the partials are being
   *                computed for.
   * @param coordType Not used in this class. Coordinates are
   *                  x,y,z
   *
   * @return bool
   */
  bool CsmBundleObservation::computePoint3DPartials(LinearAlgebra::Matrix &coeffPoint3D, BundleMeasure &measure, SurfacePoint::CoordinateType coordType) {
    coeffPoint3D.clear();

    CSMCamera *measureCamera = dynamic_cast<CSMCamera*>(measure.camera());

    // do ground partials
    SurfacePoint groundPoint = measure.parentControlPoint()->adjustedSurfacePoint();
    vector<double> groundPartials = measureCamera->GroundPartials(groundPoint);

    if (coordType == SurfacePoint::Rectangular) {
      // groundPartials is:
      // line WRT x
      // line WRT y
      // line WRT z
      // sample WRT x
      // sample WRT y
      // sample WRT z
      // Scale from WRT m to WRT Km
      coeffPoint3D(1,0) = groundPartials[0] * 1000;
      coeffPoint3D(1,1) = groundPartials[1] * 1000;
      coeffPoint3D(1,2) = groundPartials[2] * 1000;
      coeffPoint3D(0,0) = groundPartials[3] * 1000;
      coeffPoint3D(0,1) = groundPartials[4] * 1000;
      coeffPoint3D(0,2) = groundPartials[5] * 1000;
    }
    else if (coordType == SurfacePoint::Latitudinal) {
      std::vector<double> latDerivative = groundPoint.LatitudinalDerivative(SurfacePoint::One);
      std::vector<double> lonDerivative = groundPoint.LatitudinalDerivative(SurfacePoint::Two);
      std::vector<double> radDerivative = groundPoint.LatitudinalDerivative(SurfacePoint::Three);

      // Line w.r.t (lat, lon, radius)
      coeffPoint3D(1,0) = 1000 * (groundPartials[0]*latDerivative[0] + groundPartials[1]*latDerivative[1] + groundPartials[2]*latDerivative[2]);
      coeffPoint3D(1,1) = 1000 * (groundPartials[0]*lonDerivative[0] + groundPartials[1]*lonDerivative[1] + groundPartials[2]*lonDerivative[2]);
      coeffPoint3D(1,2) = 1000 * (groundPartials[0]*radDerivative[0] + groundPartials[1]*radDerivative[1] + groundPartials[2]*radDerivative[2]);

      // Sample w.r.t (lat, lon, radius)
      coeffPoint3D(0,0) = 1000 * (groundPartials[3]*latDerivative[0] + groundPartials[4]*latDerivative[1] + groundPartials[5]*latDerivative[2]);
      coeffPoint3D(0,1) = 1000 * (groundPartials[3]*lonDerivative[0] + groundPartials[4]*lonDerivative[1] + groundPartials[5]*lonDerivative[2]);
      coeffPoint3D(0,2) = 1000 * (groundPartials[3]*radDerivative[0] + groundPartials[4]*radDerivative[1] + groundPartials[5]*radDerivative[2]);
    }
    else {
      IString msg ="Unknown surface point coordinate type enum [" + toString(coordType) + "]." ;
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return true;
  }


  /**
   * Calculates the sample, line residuals between the values
   * measured in the image and the ground-to-image sample, line
   * calculated by the sensor model.
   *
   * @param coeffRHS  A vector that will contain the sample, line
   *                  residuals.
   * @param measure The measure that the partials are being
   *                computed for.
   *
   * @return bool
   */
  bool CsmBundleObservation::computeRHSPartials(LinearAlgebra::Vector &coeffRHS, BundleMeasure &measure) {
    // Clear old values
    coeffRHS.clear();

    Camera *measureCamera = measure.camera();
    BundleControlPoint* point = measure.parentControlPoint();

    // Get ground-to-image computed coordinates for this point.
    if (!(measureCamera->SetGround(point->adjustedSurfacePoint()))) {
      std::string msg = "Unable to map apriori surface point for measure ";
      msg += measure.cubeSerialNumber().toStdString() + " on point " + point->id().toStdString() + " back into image.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    double computedSample = measureCamera->Sample();
    double computedLine = measureCamera->Line();

    // The RHS is the difference between the measured coordinates on the image
    // and the coordinates calculated by the ground to image call.
    double deltaSample = measure.sample() - computedSample;
    double deltaLine = measure.line() - computedLine;

    coeffRHS(0) = deltaSample;
    coeffRHS(1) = deltaLine;

    return true;
  }


  /**
   * Returns the observed value in (sample, line) coordinates.
   * This requires no modification for Csm.
   *
   * @param measure measure The measure that the partials are
   *                being computed for.
   * @param deltaVal The difference between the measured and
   *                 calculate sample, line coordinates
   *
   * @return double The The difference between the measured and
   *                calculated (line, sample) coordinate
   */
  double CsmBundleObservation::computeObservationValue(BundleMeasure &measure, double deltaVal) {
    return deltaVal;
  }
}
