/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "IsisBundleObservation.h"

#include <QDebug>
#include <QString>
#include <QStringList>
#include <QVector>

#include "BundleImage.h"
#include "BundleControlPoint.h"
#include "BundleObservationSolveSettings.h"
#include "BundleTargetBody.h"
#include "Camera.h"
#include "LinearAlgebra.h"
#include "SpicePosition.h"
#include "SpiceRotation.h"
#include "CameraGroundMap.h"

using namespace std;
using namespace boost::numeric::ublas;

namespace Isis {

  /**
   * Constructs a IsisBundleObservation initialized to a default state.
   */
  IsisBundleObservation::IsisBundleObservation() {
    m_instrumentPosition = NULL;
    m_instrumentRotation = NULL;
  }


  /**
   * Constructs a IsisBundleObservation from an BundleImage, an instrument id, an observation
   * number to assign to this IsisBundleObservation, and a target body.
   *
   * @param image QSharedPointer to the primary image in the observation
   * @param observationNumber Observation number of the observation
   * @param instrumentId Id of the instrument for the observation
   * @param bundleTargetBody QSharedPointer to the target body of the observation
   */
  IsisBundleObservation::IsisBundleObservation(BundleImageQsp image, QString observationNumber,
                                       QString instrumentId, BundleTargetBodyQsp bundleTargetBody) :
        BundleObservation(image, observationNumber, instrumentId, bundleTargetBody) {
    m_bundleTargetBody = bundleTargetBody;
    m_instrumentRotation = NULL;
    m_instrumentPosition = NULL;

    if (image) {
      // set the observations spice position and rotation objects from the primary image in the
      // observation (this is, by design at the moment, the first image added to the observation)
      // if the image, camera, or instrument position/orientation is null, then set to null
      m_instrumentPosition = (image->camera() ?
                               (image->camera()->instrumentPosition() ?
                                 image->camera()->instrumentPosition() : NULL)
                               : NULL);
      m_instrumentRotation = (image->camera() ?
                               (image->camera()->instrumentRotation() ?
                                  image->camera()->instrumentRotation() : NULL)
                               : NULL);
    }
  }


  /**
   * Creates a copy of another IsisBundleObservation.
   *
   * @param src Reference to the IsisBundleObservation to copy
   */
  IsisBundleObservation::IsisBundleObservation(const IsisBundleObservation &src) : BundleObservation(src) {
    m_instrumentPosition = src.m_instrumentPosition;
    m_instrumentRotation = src.m_instrumentRotation;
    m_solveSettings = src.m_solveSettings;
    m_bundleTargetBody = src.m_bundleTargetBody;
  }


  /**
   * Destructor.
   *
   * Contained BundleImages will remain until all shared pointers are deleted.
   */
  IsisBundleObservation::~IsisBundleObservation() {
  }


  /**
   * Assignment operator
   *
   * Assigns the state of the source IsisBundleObservation to this IsisBundleObservation
   *
   * @param IsisBundleObservation Reference to the source IsisBundleObservation to assign from
   *
   * @return @b IsisBundleObservation& Reference to this IsisBundleObservation
   */
  IsisBundleObservation &IsisBundleObservation::operator=(const IsisBundleObservation &src) {
    if (&src != this) {
      BundleObservation::operator=(src);
      m_instrumentPosition = src.m_instrumentPosition;
      m_instrumentRotation = src.m_instrumentRotation;
      m_solveSettings = src.m_solveSettings;
      m_bundleTargetBody = src.m_bundleTargetBody;
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
  bool IsisBundleObservation::setSolveSettings(BundleObservationSolveSettings solveSettings) {
    m_solveSettings = BundleObservationSolveSettingsQsp(
                        new BundleObservationSolveSettings(solveSettings));

    // initialize solution parameters for this observation
    int nCameraAngleCoefficients = m_solveSettings->numberCameraAngleCoefficientsSolved();
    int nCameraPositionCoefficients = m_solveSettings->numberCameraPositionCoefficientsSolved();

    int nParameters = 3*nCameraPositionCoefficients + 2*nCameraAngleCoefficients;
    if (nCameraAngleCoefficients >= 1 && m_solveSettings->solveTwist()) {
      nParameters += nCameraAngleCoefficients;
    }
    // size vectors and set to zero
    m_weights.resize(nParameters);
    m_weights.clear();
    m_corrections.resize(nParameters);
    m_corrections.clear();
    m_adjustedSigmas.resize(nParameters);
    m_adjustedSigmas.clear();
    m_aprioriSigmas.resize(nParameters);
    for ( int i = 0; i < nParameters; i++) {
      m_aprioriSigmas[i] = Isis::Null;
    }

    if (!initParameterWeights()) {
      return false;
    }

    return true;
  }


  /**
   * Accesses the instrument's spice rotation
   *
   * @return @b SpiceRotation* Returns the SpiceRotation for this observation
   */
  SpiceRotation *IsisBundleObservation::spiceRotation() {
    return m_instrumentRotation;
  }


  /**
   * Accesses the instrument's spice position
   *
   * @return @b SpicePosition* Returns the SpicePosition for this observation
   */
  SpicePosition *IsisBundleObservation::spicePosition() {
    return m_instrumentPosition;
  }


  /**
   * Accesses the solve settings
   *
   * @return @b const IsisBundleObservationSolveSettingsQsp Returns a pointer to the solve
   *                                                    settings for this IsisBundleObservation
   */
  const BundleObservationSolveSettingsQsp IsisBundleObservation::solveSettings() {
    return m_solveSettings;
  }


  /**
   * Initializes the exterior orientation
   *
   * @return @b bool Returns true upon successful intialization
   */
  bool IsisBundleObservation::initializeExteriorOrientation() {
    if (m_solveSettings->instrumentPositionSolveOption() !=
        BundleObservationSolveSettings::NoPositionFactors) {

      double positionBaseTime = 0.0;
      double positiontimeScale = 0.0;
      std::vector<double> posPoly1, posPoly2, posPoly3;

      for (int i = 0; i < size(); i++) {
        BundleImageQsp image = at(i);
        SpicePosition *spicePosition = image->camera()->instrumentPosition();

        // If this image isn't the first, copy the position from the first
        if (i > 0) {
          spicePosition->SetPolynomialDegree(m_solveSettings->spkSolveDegree());
          spicePosition->SetOverrideBaseTime(positionBaseTime, positiontimeScale);
          spicePosition->SetPolynomial(posPoly1, posPoly2, posPoly3,
                                       m_solveSettings->positionInterpolationType());
        }
        // Otherwise we need to fit the initial position
        else {
          // first, set the degree of the spk polynomial to be fit for a priori values
          spicePosition->SetPolynomialDegree(m_solveSettings->spkDegree());

          // now, set what kind of interpolation to use (polynomial function or
          // polynomial function over hermite spline)
          spicePosition->SetPolynomial(m_solveSettings->positionInterpolationType());

          // finally, set the degree of the position polynomial actually used in the bundle adjustment
          spicePosition->SetPolynomialDegree(m_solveSettings->spkSolveDegree());

          if (m_instrumentPosition) {
            positionBaseTime = m_instrumentPosition->GetBaseTime();
            positiontimeScale = m_instrumentPosition->GetTimeScale();
            m_instrumentPosition->GetPolynomial(posPoly1, posPoly2, posPoly3);
          }
        }
      }
    }

    if (m_solveSettings->instrumentPointingSolveOption() !=
        BundleObservationSolveSettings::NoPointingFactors) {

      double rotationBaseTime = 0.0;
      double rotationtimeScale = 0.0;
      std::vector<double> anglePoly1, anglePoly2, anglePoly3;

      for (int i = 0; i < size(); i++) {
        BundleImageQsp image = at(i);
        SpiceRotation *spicerotation = image->camera()->instrumentRotation();

        // If this image isn't the first, copy the pointing from the first
        if (i > 0) {
          spicerotation->SetPolynomialDegree(m_solveSettings->ckSolveDegree());
          spicerotation->SetOverrideBaseTime(rotationBaseTime, rotationtimeScale);
          spicerotation->SetPolynomial(anglePoly1, anglePoly2, anglePoly3,
                                       m_solveSettings->pointingInterpolationType());
        }
        // Otherwise we need to fit the initial pointing
        else {
          // first, set the degree of the polynomial to be fit for a priori values
          spicerotation->SetPolynomialDegree(m_solveSettings->ckDegree());

          // now, set what kind of interpolation to use (polynomial function or
          // polynomial function over a pointing cache)
          spicerotation->SetPolynomial(m_solveSettings->pointingInterpolationType());

          // finally, set the degree of the pointing polynomial actually used in the bundle adjustment
          spicerotation->SetPolynomialDegree(m_solveSettings->ckSolveDegree());

          rotationBaseTime = spicerotation->GetBaseTime();
          rotationtimeScale = spicerotation->GetTimeScale();
          spicerotation->GetPolynomial(anglePoly1, anglePoly2, anglePoly3);
        }
      }
    }
    return true;
  }


  /**
   * Intializes the body rotation
   */
  void IsisBundleObservation::initializeBodyRotation() {
    std::vector<Angle> raCoefs = m_bundleTargetBody->poleRaCoefs();
    std::vector<Angle> decCoefs = m_bundleTargetBody->poleDecCoefs();
    std::vector<Angle> pmCoefs = m_bundleTargetBody->pmCoefs();

    for (int i = 0; i < size(); i++) {
      BundleImageQsp image = at(i);
      image->camera()->bodyRotation()->setPckPolynomial(raCoefs, decCoefs, pmCoefs);
    }
  }


  /**
   * Updates the body rotation
   */
  void IsisBundleObservation::updateBodyRotation() {
    std::vector<Angle> raCoefs = m_bundleTargetBody->poleRaCoefs();
    std::vector<Angle> decCoefs = m_bundleTargetBody->poleDecCoefs();
    std::vector<Angle> pmCoefs = m_bundleTargetBody->pmCoefs();

    for (int i = 0; i < size(); i++) {
      BundleImageQsp image = at(i);
      image->camera()->bodyRotation()->setPckPolynomial(raCoefs, decCoefs, pmCoefs);
    }
  }


  /**
   * Initializes the paramater weights for solving
   *
   * @return @b bool Returns true upon successful intialization
   */
  bool IsisBundleObservation::initParameterWeights() {

                                   // weights for
    double posWeight    = 0.0;     // position
    double velWeight    = 0.0;     // velocity
    double accWeight    = 0.0;     // acceleration
    double angWeight    = 0.0;     // angles
    double angVelWeight = 0.0;     // angular velocity
    double angAccWeight = 0.0;     // angular acceleration

    QList<double> aprioriPointingSigmas = m_solveSettings->aprioriPointingSigmas();
    QList<double> aprioriPositionSigmas = m_solveSettings->aprioriPositionSigmas();

    int nCamPosCoeffsSolved = 3  *m_solveSettings->numberCameraPositionCoefficientsSolved();

    int nCamAngleCoeffsSolved;
    if (m_solveSettings->solveTwist()) {
      nCamAngleCoeffsSolved = 3  *m_solveSettings->numberCameraAngleCoefficientsSolved();
    }
    else {
      nCamAngleCoeffsSolved = 2  *m_solveSettings->numberCameraAngleCoefficientsSolved();
    }

    if (aprioriPositionSigmas.size() >= 1 && aprioriPositionSigmas.at(0) > 0.0) {
      posWeight = aprioriPositionSigmas.at(0);
      posWeight = 1.0 / (posWeight  *posWeight * 1.0e-6);
    }
    if (aprioriPositionSigmas.size() >= 2 && aprioriPositionSigmas.at(1) > 0.0) {
      velWeight = aprioriPositionSigmas.at(1);
      velWeight = 1.0 / (velWeight  *velWeight * 1.0e-6);
    }
    if (aprioriPositionSigmas.size() >= 3 && aprioriPositionSigmas.at(2) > 0.0) {
      accWeight = aprioriPositionSigmas.at(2);
      accWeight = 1.0 / (accWeight  *accWeight * 1.0e-6);
    }

    if (aprioriPointingSigmas.size() >= 1 && aprioriPointingSigmas.at(0) > 0.0) {
      angWeight = aprioriPointingSigmas.at(0);
      angWeight = 1.0 / (angWeight  *angWeight * DEG2RAD * DEG2RAD);
    }
    if (aprioriPointingSigmas.size() >= 2 && aprioriPointingSigmas.at(1) > 0.0) {
      angVelWeight = aprioriPointingSigmas.at(1);
      angVelWeight = 1.0 / (angVelWeight * angVelWeight * DEG2RAD * DEG2RAD);
    }
    if (aprioriPointingSigmas.size() >= 3 && aprioriPointingSigmas.at(2) > 0.0) {
      angAccWeight = aprioriPointingSigmas.at(2);
      angAccWeight = 1.0 / (angAccWeight * angAccWeight * DEG2RAD * DEG2RAD);
    }

    int nSpkTerms = m_solveSettings->spkSolveDegree()+1;
    nSpkTerms = m_solveSettings->numberCameraPositionCoefficientsSolved();
    for ( int i = 0; i < nCamPosCoeffsSolved; i++) {
      if (i % nSpkTerms == 0) {
       m_aprioriSigmas[i] = aprioriPositionSigmas.at(0);
       m_weights[i] = posWeight;
      }
      if (i % nSpkTerms == 1) {
       m_aprioriSigmas[i] = aprioriPositionSigmas.at(1);
       m_weights[i] = velWeight;
      }
      if (i % nSpkTerms == 2) {
       m_aprioriSigmas[i] = aprioriPositionSigmas.at(2);
       m_weights[i] = accWeight;
      }
    }

    int nCkTerms = m_solveSettings->ckSolveDegree()+1;
    nCkTerms = m_solveSettings->numberCameraAngleCoefficientsSolved();
    for ( int i = 0; i < nCamAngleCoeffsSolved; i++) {
      if (i % nCkTerms == 0) {
        m_aprioriSigmas[nCamPosCoeffsSolved + i] = aprioriPointingSigmas.at(0);
        m_weights[nCamPosCoeffsSolved + i] = angWeight;
      }
      if (i % nCkTerms == 1) {
        m_aprioriSigmas[nCamPosCoeffsSolved + i] = aprioriPointingSigmas.at(1);
        m_weights[nCamPosCoeffsSolved + i] = angVelWeight;
      }
      if (i % nCkTerms == 2) {
        m_aprioriSigmas[nCamPosCoeffsSolved + i] = aprioriPointingSigmas.at(2);
        m_weights[nCamPosCoeffsSolved + i] = angAccWeight;
      }
    }

    return true;
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
   * @throws IException::Unknown "Unable to apply parameter corrections to IsisBundleObservation."
   *
   * @return @b bool Returns true upon successful application of corrections
   */
  bool IsisBundleObservation::applyParameterCorrections(LinearAlgebra::Vector corrections) {

    int index = 0;

    try {
      int nCameraAngleCoefficients = m_solveSettings->numberCameraAngleCoefficientsSolved();
      int nCameraPositionCoefficients = m_solveSettings->numberCameraPositionCoefficientsSolved();

      BundleObservationSolveSettings::InstrumentPositionSolveOption positionOption
          = m_solveSettings->instrumentPositionSolveOption();
      if (positionOption != BundleObservationSolveSettings::NoPositionFactors) {

        if (!m_instrumentPosition) {
          QString msg = "Instrument position is NULL, but position solve option is ";
          msg.append(BundleObservationSolveSettings::instrumentPositionSolveOptionToString(
                     positionOption));
          throw IException(IException::Unknown, msg.toStdString(), _FILEINFO_);
        }

        std::vector<double> coefX(nCameraPositionCoefficients);
        std::vector<double> coefY(nCameraPositionCoefficients);
        std::vector<double> coefZ(nCameraPositionCoefficients);

        m_instrumentPosition->GetPolynomial(coefX, coefY, coefZ);

        // update X coordinate coefficient(s) and sum parameter correction
        for (int i = 0; i < nCameraPositionCoefficients; i++) {
          coefX[i] += corrections(index);
          index++;
        }

        // update Y coordinate coefficient(s) and sum parameter correction
        for (int i = 0; i < nCameraPositionCoefficients; i++) {
          coefY[i] += corrections(index);
          index++;
        }

        // update Z coordinate coefficient(s) and sum parameter correction
        for (int i = 0; i < nCameraPositionCoefficients; i++) {
          coefZ[i] += corrections(index);
          index++;
        }

        // apply updates to all images in observation
        for (int i = 0; i < size(); i++) {
          BundleImageQsp image = at(i);
          SpicePosition *spiceposition = image->camera()->instrumentPosition();
          spiceposition->SetPolynomial(coefX, coefY, coefZ,
                                       m_solveSettings->positionInterpolationType());
        }
      }

      BundleObservationSolveSettings::InstrumentPointingSolveOption pointingOption
          = m_solveSettings->instrumentPointingSolveOption();
      if (pointingOption != BundleObservationSolveSettings::NoPointingFactors) {

        if (!m_instrumentRotation) {
          QString msg = "Instrument rotation is NULL, but pointing solve option is ";
          msg.append(BundleObservationSolveSettings::instrumentPointingSolveOptionToString(
                     pointingOption));
          throw IException(IException::Unknown, msg.toStdString(), _FILEINFO_);
        }

        std::vector<double> coefRA(nCameraPositionCoefficients);
        std::vector<double> coefDEC(nCameraPositionCoefficients);
        std::vector<double> coefTWI(nCameraPositionCoefficients);

        m_instrumentRotation->GetPolynomial(coefRA, coefDEC, coefTWI);

        // update RA coefficient(s)
        for (int i = 0; i < nCameraAngleCoefficients; i++) {
          coefRA[i] += corrections(index);
          index++;
        }

        // update DEC coefficient(s)
        for (int i = 0; i < nCameraAngleCoefficients; i++) {
          coefDEC[i] += corrections(index);
          index++;
        }

        if (m_solveSettings->solveTwist()) {
          // update TWIST coefficient(s)
          for (int i = 0; i < nCameraAngleCoefficients; i++) {
            coefTWI[i] += corrections(index);
            index++;
          }
        }

        // apply updates to all images in observation
        for (int i = 0; i < size(); i++) {
          BundleImageQsp image = at(i);
          SpiceRotation *spiceRotation = image->camera()->instrumentRotation();
          spiceRotation->SetPolynomial(coefRA, coefDEC, coefTWI,
                                       m_solveSettings->pointingInterpolationType());
        }
      }

      // update corrections
      m_corrections += corrections;

    }
    catch (IException &e) {
      std::string msg = "Unable to apply parameter corrections to IsisBundleObservation.";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
    return true;
  }


  /**
   * Returns the number of position parameters there are
   *
   * @return @b int Returns the number of position parameters
   */
  int IsisBundleObservation::numberPositionParameters() {
    return 3.0 * m_solveSettings->numberCameraPositionCoefficientsSolved();
  }


  /**
   * Returns the number of pointing parameters being solved for
   *
   * @return @b int Returns the number of pointing parameters
   */
  int IsisBundleObservation::numberPointingParameters() {
    int angleCoefficients = m_solveSettings->numberCameraAngleCoefficientsSolved();

    if (m_solveSettings->solveTwist()) {
      return 3.0 * angleCoefficients;
    }
    return 2.0 * angleCoefficients;
  }


  /**
   * Returns the number of total parameters there are for solving
   *
   * The total number of parameters is equal to the number of position parameters and number of
   * pointing parameters
   *
   * @return @b int Returns the number of parameters there are
   */
  int IsisBundleObservation::numberParameters() {
    return numberPositionParameters() + numberPointingParameters();
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
QStringList IsisBundleObservation::parameterList() {
  QStringList paramList;

  // We still want to output the center postion even if not solving for it
  // so always do at least 1
  int numberCamPosCoefSolved = std::max(
      solveSettings()->numberCameraPositionCoefficientsSolved(),
      1);
  for (int i = 0; i < numberCamPosCoefSolved; i++) {
    if (numberCamPosCoefSolved == 1) {
      paramList.push_back("X");
    }
    else {
      QString str = "X(t" + QString::fromStdString(toString(i)) + ")";
      paramList.push_back(str);
    }
  }
  for (int i = 0; i < numberCamPosCoefSolved; i++) {
    if (numberCamPosCoefSolved == 1) {
      paramList.push_back("Y");
    }
    else {
      QString str = "Y(t" + QString::fromStdString(toString(i)) + ")";
      paramList.push_back(str);
    }
  }
  for (int i = 0; i < numberCamPosCoefSolved; i++) {
    if (numberCamPosCoefSolved == 1) {
      paramList.push_back("Z");
    }
    else {
      QString str = "Z(t" + QString::fromStdString(toString(i)) + ")";
      paramList.push_back(str);
    }
  }

  // We still want to output the center pointing even if not solving for it
  // so always do at least 1
  int numberCamAngleCoefSolved = std::max(
      solveSettings()->numberCameraAngleCoefficientsSolved(),
      1);
  for (int i = 0; i < numberCamAngleCoefSolved; i++) {
    if (numberCamAngleCoefSolved == 1) {
      paramList.push_back("RA");
    }
    else {
      QString str = "RA(t" + QString::fromStdString(toString(i)) + ")";
      paramList.push_back(str);
    }
  }
  for (int i = 0; i < numberCamAngleCoefSolved; i++) {
    if (numberCamAngleCoefSolved == 1) {
      paramList.push_back("DEC");
    }
    else {
      QString str = "DEC(t" + QString::fromStdString(toString(i)) + ")";
      paramList.push_back(str);
    }
  }
  for (int i = 0; i < numberCamAngleCoefSolved; i++) {
    if (numberCamAngleCoefSolved == 1) {
      paramList.push_back("TWIST");
    }
    else {
      QString str = "TWIST(t" + QString::fromStdString(toString(i)) + ")";
      paramList.push_back(str);
    }
  }

  return paramList;
}


  /**
  * @brief Fetches data for the log file output methods.
  *
  * @param finalParameterValues Reference to QVector<double> of calculated
  * position and pointing
  * @param nPositionCoefficients Reference to int of the number of position coefficients
  * @param nPointingCoefficients Reference to int of the number of pointing coefficients
  * @param useDefaultPosition Reference to boolean of whether to use default position
  * @param useDefaultPointing Reference to boolean of whether to use default pointing
  * @param useDefaultTwist Reference to bollean of whether to use defualt twist
  */
  void IsisBundleObservation::bundleOutputFetchData(QVector<double> &finalParameterValues,
                          int &nPositionCoefficients, int &nPointingCoefficients,
                          bool &useDefaultPosition,
                          bool &useDefaultPointing, bool &useDefaultTwist) {

    std::vector<double> coefX,coefY,coefZ,coefRA,coefDEC,coefTWI;
    nPositionCoefficients = m_solveSettings->numberCameraPositionCoefficientsSolved();
    nPointingCoefficients = m_solveSettings->numberCameraAngleCoefficientsSolved();

    // Indicate if we need to obtain default position or pointing values
    useDefaultPosition = false;
    useDefaultPointing = false;
    // Indicate if we need to use default values when not solving twist
    useDefaultTwist = !(m_solveSettings->solveTwist());

    // If we aren't solving for position, set the number of coefficients to 1 so we
    // can output the instrumentPosition's center coordinate values for X, Y, and Z
    if (nPositionCoefficients == 0) {
      nPositionCoefficients = 1;
      useDefaultPosition = true;
    }

    // If we arent' solving for pointing, set the number of coefficients to 1 so we
    // can output the instrumentPointing's center angles for RA, DEC, and TWI
    if (nPointingCoefficients == 0) {
      nPointingCoefficients = 1;
      useDefaultPointing = true;
    }

    coefX.resize(nPositionCoefficients);
    coefY.resize(nPositionCoefficients);
    coefZ.resize(nPositionCoefficients);
    coefRA.resize(nPointingCoefficients);
    coefDEC.resize(nPointingCoefficients);
    coefTWI.resize(nPointingCoefficients);

    if (m_instrumentPosition) {
      if (!useDefaultPosition) {
        m_instrumentPosition->GetPolynomial(coefX,coefY,coefZ);
      }
      // Use the position's center coordinate if not solving for spacecraft position
      else {
        const std::vector<double> centerCoord = m_instrumentPosition->GetCenterCoordinate();
        coefX[0] = centerCoord[0];
        coefY[0] = centerCoord[1];
        coefZ[0] = centerCoord[2];
      }
    }

    if (m_instrumentRotation) {
      if (!useDefaultPointing) {
        m_instrumentRotation->GetPolynomial(coefRA,coefDEC,coefTWI);
      }
      // Use the pointing's center angles if not solving for pointing (rotation)
      else {
        const std::vector<double> centerAngles = m_instrumentRotation->GetCenterAngles();
        coefRA[0] = centerAngles[0];
        coefDEC[0] = centerAngles[1];
        coefTWI[0] = centerAngles[2];
      }
    }

    // Combine all vectors into one
    if (nPositionCoefficients > 0) {
      for (int i=0; i < nPositionCoefficients; i++) {
        finalParameterValues.append(coefX[i]);
      }
      for (int i=0; i < nPositionCoefficients; i++) {
        finalParameterValues.append(coefY[i]);
      }
      for (int i=0; i < nPositionCoefficients; i++) {
        finalParameterValues.append(coefZ[i]);
      }
    }
    if (nPointingCoefficients > 0) {
      for (int i=0; i < nPointingCoefficients; i++) {
        finalParameterValues.append(coefRA[i]);
      }
      for (int i=0; i < nPointingCoefficients; i++) {
        finalParameterValues.append(coefDEC[i]);
      }
      for (int i=0; i < nPointingCoefficients; i++) {
        finalParameterValues.append(coefTWI[i]);
      }
    }

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
  void IsisBundleObservation::bundleOutputString(std::ostream &fpOut, bool errorPropagation) {

    char buf[4096];

    QVector<double> finalParameterValues;
    int nPositionCoefficients, nPointingCoefficients;
    bool useDefaultPosition, useDefaultPointing,useDefaultTwist;

    bundleOutputFetchData(finalParameterValues,
                          nPositionCoefficients,nPointingCoefficients,
                          useDefaultPosition,useDefaultPointing,useDefaultTwist);

    int nPositionParameters = 3 * nPositionCoefficients;
    int nPointingParameters = 3 * nPointingCoefficients;
    int nParameters = nPositionParameters + nPointingParameters;

    // for convenience, create vectors of parameters names and values in the correct sequence
    QStringList parameterNamesListX,parameterNamesListY,parameterNamesListZ,
        parameterNamesListRA,parameterNamesListDEC,parameterNamesListTWI,
        parameterNamesList;
    QStringList correctionUnitListX,correctionUnitListY,correctionUnitListZ,
        correctionUnitListRA,correctionUnitListDEC,correctionUnitListTWI,
        correctionUnitList;

    QString str("%1(%2)  ");
    QString str2("%1(%2) ");
    QString strN("%1(%2)");


    if (nPositionCoefficients > 0) {
      for (int j = 0; j < nPositionCoefficients;j++) {
        if (j == 0) {
          parameterNamesListX.append(str.arg("  X  ").arg("km"));
          parameterNamesListY.append(str.arg("  Y  ").arg("km"));
          parameterNamesListZ.append(str.arg("  Z  ").arg("km"));
          correctionUnitListX.append("m");
          correctionUnitListY.append("m");
          correctionUnitListZ.append("m");
        } //end inner-if

        else if (j==1) {
          parameterNamesListX.append( str2.arg("    ").arg("km/s") );
          parameterNamesListY.append( str2.arg("    ").arg("km/s") );
          parameterNamesListZ.append( str2.arg("    ").arg("km/s") );
          correctionUnitListX.append("m/s");
          correctionUnitListY.append("m/s");
          correctionUnitListZ.append("m/s");
        }
        else {
          QString str("%1(%2)");
          parameterNamesListX.append(strN.arg("   ").arg("km/s^" + QString::fromStdString(toString(j) ) ));
          parameterNamesListY.append(strN.arg("   ").arg("km/s^" + QString::fromStdString(toString(j) ) ));
          parameterNamesListZ.append(strN.arg("   ").arg("km/s^" + QString::fromStdString(toString(j) ) ));
          correctionUnitListX.append("m/s^" + QString::fromStdString(toString(j)));
          correctionUnitListY.append("m/s^" + QString::fromStdString(toString(j)));
          correctionUnitListZ.append("m/s^" + QString::fromStdString(toString(j)));
        }
      }//end for
    }//end outer-if

    if (nPointingCoefficients > 0) {
      for (int j = 0; j < nPointingCoefficients;j++) {
        if (j == 0) {
          parameterNamesListRA.append(str.arg(" RA  ").arg("dd"));
          parameterNamesListDEC.append(str.arg("DEC  ").arg("dd"));
          parameterNamesListTWI.append(str.arg("TWI  ").arg("dd"));
          correctionUnitListRA.append("dd");
          correctionUnitListDEC.append("dd");
          correctionUnitListTWI.append("dd");
        } //end inner-if

        else if (j==1) {
          parameterNamesListRA.append( str2.arg("    ").arg("dd/s") );
          parameterNamesListDEC.append( str2.arg("    ").arg("dd/s") );
          parameterNamesListTWI.append( str2.arg("    ").arg("dd/s") );
          correctionUnitListRA.append("dd/s");
          correctionUnitListDEC.append("dd/s");
          correctionUnitListTWI.append("dd/s");
        }
        else {
          parameterNamesListRA.append(strN.arg("   ").arg("dd/s^" + QString::fromStdString(toString(j) )) );
          parameterNamesListDEC.append(strN.arg("   ").arg("dd/s^" + QString::fromStdString(toString(j) )) );
          parameterNamesListTWI.append(strN.arg("   ").arg("dd/s^" + QString::fromStdString(toString(j) ) ));
          correctionUnitListRA.append("dd/s^" + QString::fromStdString(toString(j)));
          correctionUnitListDEC.append("dd/s^" + QString::fromStdString(toString(j)));
          correctionUnitListTWI.append("dd/s^" + QString::fromStdString(toString(j)));
        }
      }//end for
    }// end outer-if

     //Put all of the parameter names together into one QStringList
    parameterNamesList.append(parameterNamesListX);
    parameterNamesList.append(parameterNamesListY);
    parameterNamesList.append(parameterNamesListZ);
    parameterNamesList.append(parameterNamesListRA);
    parameterNamesList.append(parameterNamesListDEC);
    parameterNamesList.append(parameterNamesListTWI);

    //Put all of the correction unit names together into one QStringList
    correctionUnitList.append(correctionUnitListX);
    correctionUnitList.append(correctionUnitListY);
    correctionUnitList.append(correctionUnitListZ);
    correctionUnitList.append(correctionUnitListDEC);
    correctionUnitList.append(correctionUnitListRA);
    correctionUnitList.append(correctionUnitListTWI);

    // Set up default values when we are using default position
    QString sigma = "N/A";
    QString adjustedSigma = "N/A";
    double correction = 0.0;

    // position parameters
    for (int i = 0; i < nPositionParameters; i++) {
      // If not using the default position, we can correctly access sigmas and corrections
      // members
      if (!useDefaultPosition) {
        correction = m_corrections(i);
        adjustedSigma = QString::fromStdString(toString(m_adjustedSigmas[i],  8));
        sigma = ( IsSpecial(m_aprioriSigmas[i]) ? "FREE" : QString::fromStdString(toString(m_aprioriSigmas[i], 8)) );
      }

      snprintf(buf, sizeof(buf),"%s", parameterNamesList.at(i).toStdString().c_str() );
      fpOut << buf;
      snprintf(buf, sizeof(buf),"%18.8lf  ", finalParameterValues[i] - correction);
      fpOut << buf;
      snprintf(buf, sizeof(buf),"%20.8lf  ", correction);
      fpOut << buf;
      snprintf(buf, sizeof(buf),"%23.8lf  ", finalParameterValues[i]);
      fpOut << buf;
      snprintf(buf, sizeof(buf),"            ");
      fpOut << buf;
      snprintf(buf, sizeof(buf),"%6s", sigma.toStdString().c_str());
      fpOut << buf;
      snprintf(buf, sizeof(buf),"            ");
      fpOut << buf;
      if (errorPropagation) {
        snprintf(buf, sizeof(buf),"%s", adjustedSigma.toStdString().c_str());
      }
      else {
        snprintf(buf, sizeof(buf),"%s", "N/A");
      }
      fpOut << buf;
      snprintf(buf, sizeof(buf),"        ");
      fpOut << buf;
      snprintf(buf, sizeof(buf),"%s\n", correctionUnitList.at(i).toStdString().c_str() );
      fpOut << buf;

    }

    // We need to use an offset of -3 (1 coef; X,Y,Z) if we used the default center coordinate
    // (i.e. we did not solve for position), as m_corrections and m_*sigmas are populated
    // according to which parameters are solved
    int offset = 0;
    if (useDefaultPosition) {
      offset = 3;
    }

    // pointing parameters
    for (int i = nPositionParameters; i < nParameters; i++) {
      if (!useDefaultPointing) {
        // If solving camera and not solving for twist, provide default values for twist to
        // prevent bad indexing into m_corrections and m_*sigmas
        // TWIST is last parameter, which corresponds to nParameters - nPointingCoefficients
        if ( (i >= nParameters - nPointingCoefficients) && useDefaultTwist) {
          correction = 0.0;
          adjustedSigma = "N/A";
          sigma = "N/A";
        }
        else {
          correction = m_corrections(i - offset);
          adjustedSigma = QString::fromStdString(toString(m_adjustedSigmas(i-offset) * RAD2DEG, 8));
          sigma = ( IsSpecial(m_aprioriSigmas[i - offset]) ? "FREE" :
                  QString::fromStdString(toString(m_aprioriSigmas[i-offset], 8) ));
        }
      }
      // We are using default pointing, so provide default correction and sigma values to output
      else {
        correction = 0.0;
        adjustedSigma = "N/A";
        sigma = "N/A";
      }

      snprintf(buf, sizeof(buf),"%s",parameterNamesList.at(i).toStdString().c_str() );
      fpOut << buf;
      snprintf(buf, sizeof(buf),"%18.8lf  ",(finalParameterValues[i]*RAD2DEG - correction*RAD2DEG));
      fpOut << buf;
      snprintf(buf, sizeof(buf),"%20.8lf  ",(correction*RAD2DEG));
      fpOut << buf;
      snprintf(buf, sizeof(buf),"%23.8lf  ",(finalParameterValues[i]*RAD2DEG));
      fpOut << buf;
      snprintf(buf, sizeof(buf),"            ");
      fpOut << buf;
      snprintf(buf, sizeof(buf),"%6s",sigma.toStdString().c_str());
      fpOut << buf;
      snprintf(buf, sizeof(buf),"            ");
      fpOut << buf;
      if (errorPropagation) {
        snprintf(buf, sizeof(buf),"%s",adjustedSigma.toStdString().c_str());
      }
      else {
        snprintf(buf, sizeof(buf),"%s","N/A");
      }
      fpOut<<buf;
      snprintf(buf, sizeof(buf),"        ");
      fpOut<<buf;
      snprintf(buf, sizeof(buf),"%s\n",correctionUnitList.at(i).toStdString().c_str() );
      fpOut<<buf;
    }

  }

  /**
   * @brief Creates and returns a formatted QString representing the bundle coefficients and
   * parameters in csv format.
   *
   * @param errorPropagation Boolean indicating whether or not to attach more information
   *     (corrections, sigmas, adjusted sigmas...) to the output QString
   *
   * @return @b QString Returns a formatted QString representing the IsisBundleObservation in
   * csv format
   */
  QString IsisBundleObservation::bundleOutputCSV(bool errorPropagation) {

    QVector<double> finalParameterValues;
    int nPositionCoefficients, nPointingCoefficients;
    bool useDefaultPosition, useDefaultPointing,useDefaultTwist;

    bundleOutputFetchData(finalParameterValues,
                          nPositionCoefficients,nPointingCoefficients,
                          useDefaultPosition,useDefaultPointing,useDefaultTwist);

    int nPositionParameters = 3 * nPositionCoefficients;
    int nPointingParameters = 3 * nPointingCoefficients;
    int nParameters = nPositionParameters + nPointingParameters;

    QString finalqStr = "";

    // Set up default values when we are using default position
    QString sigma = "N/A";
    QString adjustedSigma = "N/A";
    double correction = 0.0;

    // Position parameters
    for (int i = 0; i < nPositionParameters; i++) {
      if (!useDefaultPosition) {
        correction = m_corrections(i);
        adjustedSigma = QString::fromStdString(toString(m_adjustedSigmas[i],  8));
        sigma = ( IsSpecial(m_aprioriSigmas[i]) ? "FREE" : QString::fromStdString(toString(m_aprioriSigmas[i],  8) ));
      }
      // Provide default values for position if not solving position
      else {
        correction = 0.0;
        adjustedSigma = "N/A";
        sigma = "N/A";
      }

      finalqStr += QString::fromStdString(toString(finalParameterValues[i] - correction)) + ",";
      finalqStr += QString::fromStdString(toString(correction)) + ",";
      finalqStr += QString::fromStdString(toString(finalParameterValues[i])) + ",";
      finalqStr += sigma + ",";
      if (errorPropagation) {
        finalqStr += adjustedSigma + ",";
      }
      else {
        finalqStr += "N/A,";
      }

    }

    // If not solving position, we need to offset access to correction and sigma members by -3
    // (X,Y,Z) since m_corrections and m_*sigmas are populated according to which parameters are
    // solved
    int offset = 0;
    if (useDefaultPosition) {
      offset = 3;
    }
    // pointing parameters
    for (int i = nPositionParameters; i < nParameters; i++) {
      if (!useDefaultPointing) {
        // Use default values if solving camera but not solving for TWIST to prevent bad indexing
        // into m_corrections and m_*sigmas
        if ( (i >= nParameters - nPointingCoefficients) && useDefaultTwist) {
          correction = 0.0;
          adjustedSigma = "N/A";
          sigma = "N/A";
        }
        else {
          correction = m_corrections(i - offset);
          adjustedSigma = QString::fromStdString(toString(m_adjustedSigmas(i-offset) * RAD2DEG, 8));
          sigma = ( IsSpecial(m_aprioriSigmas[i-offset]) ? "FREE" :
              QString::fromStdString(toString(m_aprioriSigmas[i-offset],  8) ));
        }
      }
      // Provide default values for pointing if not solving pointing
      else {
        correction = 0.0;
        adjustedSigma = "N/A";
        sigma = "N/A";
      }

      finalqStr += QString::fromStdString(toString(finalParameterValues[i]*RAD2DEG - correction * RAD2DEG)) + ",";
      finalqStr += QString::fromStdString(toString(correction * RAD2DEG)) + ",";
      finalqStr += QString::fromStdString(toString(finalParameterValues[i]*RAD2DEG)) + ",";
      finalqStr += sigma + ",";
      if (errorPropagation) {
        finalqStr += adjustedSigma + ",";
      }
      else {
        finalqStr += "N/A,";
      }

    }

    return finalqStr;
  }


  /**
   * Computes any needed partials for the target body parameters.
   *
   * @param coeffTarget Matrix for target body partial derivatives
   * @param measure The measure that the partials are being
   *                computed for.
   * @param bundleSettings The settings for the bundle adjustment
   * @param bundleTargetBody QSharedPointer to the target body of
   *                         the observation
   *
   * @return bool
   */
  bool IsisBundleObservation::computeTargetPartials(matrix<double> &coeffTarget, BundleMeasure &measure,
                                                BundleSettingsQsp &bundleSettings, BundleTargetBodyQsp &bundleTargetBody) {
    coeffTarget.clear();

    Camera *measureCamera = measure.camera();
    BundleControlPoint *point = measure.parentControlPoint();
    SurfacePoint surfacePoint = point->adjustedSurfacePoint();

    int index = 0;

    if (bundleSettings->solvePoleRA()) {
      measureCamera->GroundMap()->GetdXYdTOrientation(SpiceRotation::WRT_RightAscension, 0,
                                                      &coeffTarget(0, index),
                                                      &coeffTarget(1, index));
      index++;
    }

    if (bundleSettings->solvePoleRAVelocity()) {
      measureCamera->GroundMap()->GetdXYdTOrientation(SpiceRotation::WRT_RightAscension, 1,
                                                      &coeffTarget(0, index),
                                                      &coeffTarget(1, index));
      index++;
    }

    if (bundleSettings->solvePoleDec()) {
      measureCamera->GroundMap()->GetdXYdTOrientation(SpiceRotation::WRT_Declination, 0,
                                                      &coeffTarget(0, index),
                                                      &coeffTarget(1, index));
      index++;
    }

    if (bundleSettings->solvePoleDecVelocity()) {
      measureCamera->GroundMap()->GetdXYdTOrientation(SpiceRotation::WRT_Declination, 1,
                                                      &coeffTarget(0, index),
                                                      &coeffTarget(1, index));
      index++;
    }

    if (bundleSettings->solvePM()) {
      measureCamera->GroundMap()->GetdXYdTOrientation(SpiceRotation::WRT_Twist, 0,
                                                      &coeffTarget(0, index),
                                                      &coeffTarget(1, index));
      index++;
    }

    if (bundleSettings->solvePMVelocity()) {
      measureCamera->GroundMap()->GetdXYdTOrientation(SpiceRotation::WRT_Twist, 1,
                                                      &coeffTarget(0, index),
                                                      &coeffTarget(1, index));
      index++;
    }

    if (bundleTargetBody->solveMeanRadius()) {
      std::vector<double> lookBWRTMeanRadius =
          measureCamera->GroundMap()->MeanRadiusPartial(surfacePoint,
                                                        bundleTargetBody->meanRadius());

      measureCamera->GroundMap()->GetdXYdPoint(lookBWRTMeanRadius, &coeffTarget(0, index),
                                               &coeffTarget(1, index));
      index++;
    }

    if (bundleTargetBody->solveTriaxialRadii()) {

      std::vector<double> lookBWRTRadiusA =
          measureCamera->GroundMap()->EllipsoidPartial(surfacePoint,
                                                       CameraGroundMap::WRT_MajorAxis);

      measureCamera->GroundMap()->GetdXYdPoint(lookBWRTRadiusA, &coeffTarget(0, index),
                                               &coeffTarget(1, index));
      index++;

      std::vector<double> lookBWRTRadiusB =
          measureCamera->GroundMap()->EllipsoidPartial(surfacePoint,
                                                       CameraGroundMap::WRT_MinorAxis);

      measureCamera->GroundMap()->GetdXYdPoint(lookBWRTRadiusB, &coeffTarget(0, index),
                                               &coeffTarget(1, index));
      index++;

      std::vector<double> lookBWRTRadiusC =
          measureCamera->GroundMap()->EllipsoidPartial(surfacePoint,
                                                       CameraGroundMap::WRT_PolarAxis);

      measureCamera->GroundMap()->GetdXYdPoint(lookBWRTRadiusC, &coeffTarget(0, index),
                                               &coeffTarget(1, index));
      index++;
    }

    double observationSigma = 1.4 * measureCamera->PixelPitch();
    double observationWeight = 1.0 / observationSigma;

    // Multiply coefficients by observation weight
    coeffTarget *= observationWeight;

    return true;
  }


  /**
   * Calculates the sensor partials with respect to the selected
   * solve parameters and populates the coeffImage matrix.
   *
   * @param coeffImage A matrix that will be populated with the
   *                   sensor partials with respect to the
   *                   specified solve parameters.
   * @param measure The measure that the partials are being
   *                 computed for.
   *
   * @return bool
   */
  bool IsisBundleObservation::computeImagePartials(matrix<double> &coeffImage, BundleMeasure &measure) {
    coeffImage.clear();

    Camera *camera = measure.camera();

    int index = 0;

    // Parials for X, Y, Z position coordinates
    if (solveSettings()->instrumentPositionSolveOption() !=
        BundleObservationSolveSettings::NoPositionFactors) {

      int numCamPositionCoefficients =
          solveSettings()->numberCameraPositionCoefficientsSolved();

      // Add the partial for the x coordinate of the position (differentiating
      // point(x,y,z) - spacecraftPosition(x,y,z) in J2000
      for (int cameraCoef = 0; cameraCoef < numCamPositionCoefficients; cameraCoef++) {
        camera->GroundMap()->GetdXYdPosition(SpicePosition::WRT_X, cameraCoef,
                                                    &coeffImage(0, index),
                                                    &coeffImage(1, index));
        index++;
      }

      // Add the partial for the y coordinate of the position
      for (int cameraCoef = 0; cameraCoef < numCamPositionCoefficients; cameraCoef++) {
        camera->GroundMap()->GetdXYdPosition(SpicePosition::WRT_Y, cameraCoef,
                                                    &coeffImage(0, index),
                                                    &coeffImage(1, index));
        index++;
      }

      // Add the partial for the z coordinate of the position
      for (int cameraCoef = 0; cameraCoef < numCamPositionCoefficients; cameraCoef++) {
        camera->GroundMap()->GetdXYdPosition(SpicePosition::WRT_Z, cameraCoef,
                                                    &coeffImage(0, index),
                                                    &coeffImage(1, index));
        index++;
      }
    }

    // Partials for RA, DEC, twist
    if (solveSettings() ->instrumentPointingSolveOption() !=
        BundleObservationSolveSettings::NoPointingFactors) {

      int numCamAngleCoefficients =
          solveSettings()->numberCameraAngleCoefficientsSolved();

      // Add the partials for ra
      for (int cameraCoef = 0; cameraCoef < numCamAngleCoefficients; cameraCoef++) {
        camera->GroundMap()->GetdXYdOrientation(SpiceRotation::WRT_RightAscension,
                                                       cameraCoef, &coeffImage(0, index),
                                                       &coeffImage(1, index));
        index++;
      }

      // Add the partials for dec
      for (int cameraCoef = 0; cameraCoef < numCamAngleCoefficients; cameraCoef++) {
        camera->GroundMap()->GetdXYdOrientation(SpiceRotation::WRT_Declination,
                                                       cameraCoef, &coeffImage(0, index),
                                                       &coeffImage(1, index));
        index++;
      }

      // Add the partial for twist if necessary
      if (solveSettings()->solveTwist()) {
        for (int cameraCoef = 0; cameraCoef < numCamAngleCoefficients; cameraCoef++) {
          camera->GroundMap()->GetdXYdOrientation(SpiceRotation::WRT_Twist,
                                                         cameraCoef, &coeffImage(0, index),
                                                         &coeffImage(1, index));
          index++;
        }
      }
    }

    // Multiply coefficients by observation weight
    double observationSigma = 1.4 * camera->PixelPitch();
    double observationWeight = 1.0 / observationSigma;
    coeffImage *= observationWeight;

    return true;
  }


  /**
   * Calculates the ground partials for the ground point currently
   * set in the sensor model.
   *
   * @param coeffPoint3D A matrix that will be populated with the
   *                     (line, sample) partials with respect to
   *                     the ground point.
   * @param measure The measure that the partials are being
   *                computed for.
   * @param coordType Specifies whether latitudinal or (x, y, z)
   *                  coordinates are used.
   *
   * @return bool
   */
  bool IsisBundleObservation::computePoint3DPartials(matrix<double> &coeffPoint3D, BundleMeasure &measure, SurfacePoint::CoordinateType coordType) {
    coeffPoint3D.clear();
    Camera *measureCamera = measure.camera();
    BundleControlPoint* point = measure.parentControlPoint();

    // These vectors are either body-fixed latitudinal (lat/lon/radius) or rectangular (x/y/z)
    // depending on the value of coordinate type in SurfacePoint
    std::vector<double> lookBWRTCoord1 = point->adjustedSurfacePoint().Partial(coordType, SurfacePoint::One);
    std::vector<double> lookBWRTCoord2 = point->adjustedSurfacePoint().Partial(coordType, SurfacePoint::Two);
    std::vector<double> lookBWRTCoord3 = point->adjustedSurfacePoint().Partial(coordType, SurfacePoint::Three);

    measureCamera->GroundMap()->GetdXYdPoint(lookBWRTCoord1,
                                             &coeffPoint3D(0, 0),
                                             &coeffPoint3D(1, 0));
    measureCamera->GroundMap()->GetdXYdPoint(lookBWRTCoord2,
                                             &coeffPoint3D(0, 1),
                                             &coeffPoint3D(1, 1));
    measureCamera->GroundMap()->GetdXYdPoint(lookBWRTCoord3,
                                             &coeffPoint3D(0, 2),
                                             &coeffPoint3D(1, 2));

    double observationSigma = 1.4 * measureCamera->PixelPitch();
    double observationWeight = 1.0 / observationSigma;

    // Multiply coefficients by observation weight
    coeffPoint3D *= observationWeight;

    return true;
  }


  /**
   * Calculates the sample, line residuals between the measured
   * focal plane values and the focal plane coordinates calculated
   * for the ground point by the sensor model.
   *
   * @param coeffRHS  A vector that will contain the focal plane
   *                  x, y residuals.
   * @param measure The measure that the partials are being
   *                computed for.
   *
   * @return bool
   */
  bool IsisBundleObservation::computeRHSPartials(boost::numeric::ublas::vector<double> &coeffRHS, BundleMeasure &measure) {
    coeffRHS.clear();
    Camera *measureCamera = measure.camera();
    BundleControlPoint* point = measure.parentControlPoint();
    // Compute the look vector in instrument coordinates based on time of observation and apriori
    // lat/lon/radius.  As of 05/15/2019, this call no longer does the back-of-planet test. An optional
    // bool argument was added CameraGroundMap::GetXY to turn off the test.
    double computedX, computedY;
    if (!(measureCamera->GroundMap()->GetXY(point->adjustedSurfacePoint(),
                                            &computedX, &computedY, false))) {
      std::string msg = "Unable to map apriori surface point for measure ";
      msg += measure.cubeSerialNumber().toStdString() + " on point " + point->id().toStdString() + " into focal plane";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    double measuredX = measure.focalPlaneMeasuredX();
    double measuredY = measure.focalPlaneMeasuredY();

    double deltaX = measuredX - computedX;
    double deltaY = measuredY - computedY;

    coeffRHS(0) = deltaX;
    coeffRHS(1) = deltaY;

    // Multiply coefficients by observation weight
    double observationSigma = 1.4 * measureCamera->PixelPitch();
    double observationWeight = 1.0 / observationSigma;

    coeffRHS *= observationWeight;

    return true;
  }


  /**
   * Converts the observed value from a focal plane coordinate to
   * an image sample or line.
   *
   * @param measure The measure that the partials are
   *                being computed for.
   * @param deltaVal The difference between the measured and
   *                 calculated focal plane coordinate
   *
   * @return double The The difference between the measured and
   *                calculated (line, sample) coordinate
   */
  double IsisBundleObservation::computeObservationValue(BundleMeasure &measure, double deltaVal) {
    return deltaVal * 1.4;
  }
}

