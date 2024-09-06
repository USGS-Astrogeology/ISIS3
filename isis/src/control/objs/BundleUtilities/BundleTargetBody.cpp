/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "BundleTargetBody.h"

#include <QDebug>
#include <QObject>

#include "BundleSettings.h"
#include "IException.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "Target.h"

namespace Isis {

  /**
   * Creates an empty BundleTargetBody object.
   */
  BundleTargetBody::BundleTargetBody() {
    m_solveTargetBodyRadiusMethod = None;
    m_parameterNamesList.clear();
    m_parameterSolveCodes.clear();

    m_weights.clear();
    m_corrections.clear();
    m_solution.clear();
    m_aprioriSigmas.clear();
    m_adjustedSigmas.clear();

    m_radii.resize(3);
    m_raPole.resize(3);
    m_decPole.resize(3);
    m_pm.resize(3);
  }


  /**
   * Creates an BundleTargetBody object from a target.
   *
   * @param target A pointer to the target body that this object will represent.
   */
  BundleTargetBody::BundleTargetBody(Target *target) {
    m_solveTargetBodyRadiusMethod = None;
    m_parameterNamesList.clear();
    //TODO This should probably be set to something based on the target. JAM
    m_parameterSolveCodes.clear();

    m_weights.clear();
    m_corrections.clear();
    m_solution.clear();
    m_aprioriSigmas.clear();
    m_adjustedSigmas.clear();

    // initialize radii and coefficients from target
    m_radii.resize(3);
    m_radii[0] = target->radii().at(0);
    m_radii[1] = target->radii().at(1);
    m_radii[2] = target->radii().at(2);

    m_raPole.resize(3);
    m_decPole.resize(3);
    m_pm.resize(3);

    m_raPole = target->poleRaCoefs();
    m_decPole = target->poleDecCoefs();
    m_pm = target->pmCoefs();
  }


  /**
   * Copy constructor. Makes a copy of another BundleTargetBody.
   *
   * @param src The BundleTargetBody to copy from.
   */
  BundleTargetBody::BundleTargetBody(const BundleTargetBody &src) {
    m_solveTargetBodyRadiusMethod = src.m_solveTargetBodyRadiusMethod;

    m_aprioriRadiusA = src.m_aprioriRadiusA;
    m_sigmaRadiusA = src.m_sigmaRadiusA;
    m_aprioriRadiusB = src.m_aprioriRadiusB;
    m_sigmaRadiusB = src.m_sigmaRadiusB;
    m_aprioriRadiusC = src.m_aprioriRadiusC;
    m_sigmaRadiusC = src.m_sigmaRadiusC;
    m_aprioriMeanRadius = src.m_aprioriMeanRadius;
    m_sigmaMeanRadius = src.m_sigmaMeanRadius;

    m_radii = src.m_radii;
    m_meanRadius = src.m_meanRadius;

    m_raPole = src.m_raPole;
    m_decPole = src.m_decPole;
    m_pm = src.m_pm;

    m_parameterSolveCodes = src.m_parameterSolveCodes;

    m_parameterNamesList = src.m_parameterNamesList;

    m_weights = src.m_weights;
    m_corrections = src.m_corrections;
    m_solution = src.m_solution;
    m_aprioriSigmas = src.m_aprioriSigmas;
    m_adjustedSigmas = src.m_adjustedSigmas;
  }


  /**
   * Destructor.
   */
  BundleTargetBody::~BundleTargetBody() {
  }


  /**
   * Assignment operator.  Makes this a copy of another BundleTargetBody.
   *
   * @param src The other BundleTargetBody to assign from
   */
  BundleTargetBody &BundleTargetBody::operator=(const BundleTargetBody &src) {
    if (this != &src) {
      m_solveTargetBodyRadiusMethod = src.m_solveTargetBodyRadiusMethod;

      m_aprioriRadiusA = src.m_aprioriRadiusA;
      m_sigmaRadiusA = src.m_sigmaRadiusA;
      m_aprioriRadiusB = src.m_aprioriRadiusB;
      m_sigmaRadiusB = src.m_sigmaRadiusB;
      m_aprioriRadiusC = src.m_aprioriRadiusC;
      m_sigmaRadiusC = src.m_sigmaRadiusC;
      m_aprioriMeanRadius = src.m_aprioriMeanRadius;
      m_sigmaMeanRadius = src.m_sigmaMeanRadius;

      m_radii = src.m_radii;
      m_meanRadius = src.m_meanRadius;

      m_raPole = src.m_raPole;
      m_decPole = src.m_decPole;
      m_pm = src.m_pm;

      m_parameterSolveCodes = src.m_parameterSolveCodes;

      m_parameterNamesList = src.m_parameterNamesList;

      m_weights = src.m_weights;
      m_corrections = src.m_corrections;
      m_solution = src.m_solution;
      m_aprioriSigmas = src.m_aprioriSigmas;
      m_adjustedSigmas = src.m_adjustedSigmas;
    }

    return *this;
  }


  /**
   * @brief Sets the solve settings for the target body.
   *
   * Sets the solve settings for the target body's right ascension, declination,
   * prime meridian, and radius based on the input values.
   * Parameter vectors, sigma vectors and the weight vector will be filled in the following order:
   *
   * pole right ascension,
   * pole right ascension velocity,
   * pole right ascension acceleration,
   * pole declination,
   * pole declination velocity,
   * pole declination acceleration,
   * prime meridian,
   * prime meridian velocity,
   * prime meridian acceleration,
   * triaxial radius A,
   * triaxial radius B,
   * triaxial radius C,
   * mean radius.
   *
   * Any parameters that are not being solved for (based on targetParameterSolveCodes)
   * will be omitted.  For example, if solving for pole right ascension, pole declination,
   * prime meridian, and mean radius; the vectors would be:
   *
   * pole right ascension,
   * pole declination,
   * prime meridian,
   * mean radius.
   *
   * @param targetParameterSolveCodes A set of TargetSolveCodes indicating what to solve for.
   * @param aprioriPoleRA The apriori pole right ascension angle.
   * @param sigmaPoleRA The apriori pole right ascension angle sigma.
   * @param aprioriVelocityPoleRA The apriori pole right ascension velocity.
   * @param sigmaVelocityPoleRA The apriori pole right ascension velocity sigma.
   * @param aprioriPoleDec The apriori pole declination angle.
   * @param sigmaPoleDec The apriori pole declination angle sigma.
   * @param aprioriVelocityPoleDec The apriori pole declination velocity.
   * @param sigmaVelocityPoleDec The apriori pole declination velocity sigma.
   * @param aprioriPM The apriori prime meridian angle.
   * @param sigmaPM The apriori prime meridian angle sigma.
   * @param aprioriVelocityPM The apriori prime meridian velocity.
   * @param sigmaVelocityPM The apriori prime meridian velocity sigma.
   * @param solveRadiiMethod What radius to solve for.
   * @param aprioriRadiusA The apriori A radius distance.
   * @param sigmaRadiusA The apriori A radius distance sigma.
   * @param aprioriRadiusB The apriori B radius distance.
   * @param sigmaRadiusB The apriori B Radius distance sigma.
   * @param aprioriRadiusC The apriori C radius distance.
   * @param sigmaRadiusC The apriori C radius distance sigma.
   * @param aprioriMeanRadius The apriori mean radius distance.
   * @param sigmaMeanRadius the apriori mean radius distance sigma.
   *
   * @see readFromPvl(PvlObject &tbObject)
   */
  void BundleTargetBody::setSolveSettings(std::set<int> targetParameterSolveCodes,
                                          Angle aprioriPoleRA, Angle sigmaPoleRA,
                                          Angle aprioriVelocityPoleRA,
                                          Angle sigmaVelocityPoleRA,
                                          Angle aprioriPoleDec, Angle sigmaPoleDec,
                                          Angle aprioriVelocityPoleDec,
                                          Angle sigmaVelocityPoleDec,
                                          Angle aprioriPM, Angle sigmaPM,
                                          Angle aprioriVelocityPM,
                                          Angle sigmaVelocityPM,
                                          TargetRadiiSolveMethod solveRadiiMethod,
                                          Distance aprioriRadiusA, Distance sigmaRadiusA,
                                          Distance aprioriRadiusB, Distance sigmaRadiusB,
                                          Distance aprioriRadiusC, Distance sigmaRadiusC,
                                          Distance aprioriMeanRadius, Distance sigmaMeanRadius) {

    m_solveTargetBodyRadiusMethod = solveRadiiMethod;

    m_parameterSolveCodes = targetParameterSolveCodes;

    // size corrections and adjusted sigma vectors and set to zero
    m_aprioriSigmas.resize(m_parameterSolveCodes.size());
    m_aprioriSigmas.clear();
    m_adjustedSigmas.resize(m_parameterSolveCodes.size());
    m_adjustedSigmas.clear();
    m_weights.resize(m_parameterSolveCodes.size());
    m_weights.clear();
    m_corrections.resize(m_parameterSolveCodes.size());
    m_corrections.clear();

// ken testing
    m_raPole[0] = aprioriPoleRA;
    m_raPole[1] = aprioriVelocityPoleRA;
    m_raPole[2] = Angle(0.0,Angle::Radians);
    m_decPole[0] = aprioriPoleDec;
    m_decPole[1] = aprioriVelocityPoleDec;
    m_decPole[2] = Angle(0.0,Angle::Radians);
    m_pm[0] = aprioriPM;
    m_pm[1] = aprioriVelocityPM;
    m_pm[2] = Angle(0.0,Angle::Radians);
// ken testing


    int n = 0;
    if (solvePoleRA()) {
      m_parameterSolveCodes.insert(PoleRA);
      m_raPole[0] = aprioriPoleRA;

      if (sigmaPoleRA.degrees() > 0.0) {
        m_aprioriSigmas(n) = sigmaPoleRA.degrees();
        m_weights(n) = 1.0/(sigmaPoleRA.radians()*sigmaPoleRA.radians());
      }
      else {
        m_aprioriSigmas(n) = -1.0;
        m_weights(n) = -1.0;
      }
      n++;
     }

    if (solvePoleRAVelocity()) {
      m_parameterSolveCodes.insert(VelocityPoleRA);
      m_raPole[1] = aprioriVelocityPoleRA;

      if (sigmaVelocityPoleRA.degrees() > 0.0) {
        m_aprioriSigmas(n) = sigmaVelocityPoleRA.degrees();
        m_weights(n) = 1.0/(sigmaVelocityPoleRA.radians()*sigmaVelocityPoleRA.radians());
      }
      else {
        m_aprioriSigmas(n) = -1.0;
        m_weights(n) = -1.0;
      }
      n++;
    }
//    if (solvePoleRAAcceleration()) {
//      m_parameterSolveCodes.insert(AccelerationPoleRA);
//      m_raPole[2].setDegrees(aprioriAccelerationPoleRA);
//      m_aprioriSigmas.insert_element(m_aprioriSigmas.size(), sigmaAccelerationPoleRA);
//    }
    if (solvePoleDec()) {
      m_parameterSolveCodes.insert(PoleDec);
      m_decPole[0] = aprioriPoleDec;

      if (sigmaPoleDec.degrees() > 0.0) {
        m_aprioriSigmas(n) = sigmaPoleDec.degrees();
        m_weights(n) = 1.0/(sigmaPoleDec.radians()*sigmaPoleDec.radians());
      }
      else {
        m_aprioriSigmas(n) = -1.0;
        m_weights(n) = -1.0;
      }
      n++;
    }
    if (solvePoleDecVelocity()) {
      m_parameterSolveCodes.insert(VelocityPoleDec);
      m_decPole[1] = aprioriVelocityPoleDec;

      if (sigmaVelocityPoleDec.degrees() > 0.0) {
        m_aprioriSigmas(n) = sigmaVelocityPoleDec.degrees();
        m_weights(n) = 1.0/(sigmaVelocityPoleDec.radians()*sigmaVelocityPoleDec.radians());
      }
      else {
        m_aprioriSigmas(n) = -1.0;
        m_weights(n) = -1.0;
      }
      n++;
    }

//    bool solveAccelerationPoleDec = false;
//    if (solvePoleDecAcceleration()) {
//      m_parameterSolveCodes.insert(AccelerationPoleDec);
//    }

    if (solvePM()) {
      m_parameterSolveCodes.insert(PM);
      m_pm[0] = aprioriPM;

      if (sigmaPM.degrees() > 0.0) {
        m_aprioriSigmas(n) = sigmaPM.degrees();
        m_weights(n) = 1.0/(sigmaPM.radians()*sigmaPM.radians());
      }
      else {
        m_aprioriSigmas(n) = -1.0;
        m_weights(n) = -1.0;
      }
      n++;
    }

    if (solvePMVelocity()) { // programmer's note: also referred to as "spin rate"
      m_parameterSolveCodes.insert(VelocityPM);
      m_pm[1] = aprioriVelocityPM;

      if (sigmaVelocityPM.degrees() > 0.0) {
        m_aprioriSigmas(n) = sigmaVelocityPM.degrees();
        m_weights(n) = 1.0/(sigmaVelocityPM.radians()*sigmaVelocityPM.radians());
      }
      else {
        m_aprioriSigmas(n) = -1.0;
        m_weights(n) = -1.0;
      }
      n++;
    }

//    bool solveAccelerationPM = false;
//    if (solvePMAcceleration()) {
//      m_parameterSolveCodes.insert(AccelerationPM);
//    }

    if (m_solveTargetBodyRadiusMethod == All) {
      m_parameterSolveCodes.insert(TriaxialRadiusA);
      m_radii[0] = aprioriRadiusA;

      if (sigmaRadiusA.kilometers() > 0.0) {
        m_aprioriSigmas(n) = sigmaRadiusA.kilometers();
        m_weights(n) = 1.0/(sigmaRadiusA.kilometers()*sigmaRadiusA.kilometers());
      }
      else {
        m_aprioriSigmas(n) = -1.0;
        m_weights(n) = -1.0;
      }
      n++;

      m_parameterSolveCodes.insert(TriaxialRadiusB);
      m_radii[1] = aprioriRadiusB;

      if (sigmaRadiusB.kilometers() > 0.0) {
        m_aprioriSigmas(n) = sigmaRadiusB.kilometers();
        m_weights(n) = 1.0/(sigmaRadiusB.kilometers()*sigmaRadiusB.kilometers());
      }
      else {
        m_aprioriSigmas(n) = -1.0;
        m_weights(n) = -1.0;
      }
      n++;

      m_parameterSolveCodes.insert(TriaxialRadiusC);
      m_radii[2] = aprioriRadiusC;

      if (sigmaRadiusC.kilometers() > 0.0) {
        m_aprioriSigmas(n) = sigmaRadiusC.kilometers();
        m_weights(n) = 1.0/(sigmaRadiusC.kilometers()*sigmaRadiusC.kilometers());
      }
      else {
        m_aprioriSigmas(n) = -1.0;
        m_weights(n) = -1.0;
      }
      n++;
    }

    else if (m_solveTargetBodyRadiusMethod == Mean) {
      m_parameterSolveCodes.insert(MeanRadius);
      m_meanRadius = aprioriMeanRadius;

      if (sigmaMeanRadius.kilometers() > 0.0) {
        m_aprioriSigmas(n) = sigmaMeanRadius.kilometers();
        m_weights(n) = 1.0/(sigmaMeanRadius.kilometers()*sigmaMeanRadius.kilometers());
      }
      else {
        m_aprioriSigmas(n) = -1.0;
        m_weights(n) = -1.0;
      }
      n++;
    }

//    for (int i = 0; i < nParameters; i++) {
//      std::cout << m_solution[i] << " " << m_aprioriSigmas[i] << " " << m_weights[i] << std::endl;
//    }
//    std::cout << "parameters: " << numberParameters() << std::endl;
//    std::cout << "      pole: " << numberPoleParameters() << std::endl;
//    std::cout << "        w0: " << numberW0Parameters() << std::endl;
//    std::cout << "      WDot: " << numberWDotParameters() << std::endl;
//    std::cout << "    Radius: " << numberRadiusParameters() << std::endl;
//    int fred=1;
  }


  /**
   * If the pole right ascension angle will be solved for with this target body.
   *
   * @return @b bool If it will be solved for.
   */
  bool BundleTargetBody::solvePoleRA() {
    if (m_parameterSolveCodes.find(PoleRA) != m_parameterSolveCodes.end())
      return true;
    return false;
  }


  /**
   * If the pole right ascension velocity will be solved for with this target body.
   *
   * @return @b bool If it will be solved for.
   */
  bool BundleTargetBody::solvePoleRAVelocity() {
    if (m_parameterSolveCodes.find(VelocityPoleRA) != m_parameterSolveCodes.end())
      return true;
    return false;
  }


  /**
   * If the pole right ascension acceleration will be solved for with this target body.
   *
   * @return @b bool If it will be solved for.
   */
  bool BundleTargetBody::solvePoleRAAcceleration() {
    if (m_parameterSolveCodes.find(AccelerationPoleRA) != m_parameterSolveCodes.end())
      return true;
    return false;
  }


  /**
   * If the pole declination angle will be solved for with this target body.
   *
   * @return @b bool If it will be solved for.
   */
  bool BundleTargetBody::solvePoleDec() {
    if (m_parameterSolveCodes.find(PoleDec) != m_parameterSolveCodes.end())
      return true;
    return false;
  }


  /**
   * If the pole declination velocity will be solved for with this target body.
   *
   * @return @b bool If it will be solved for.
   */
  bool BundleTargetBody::solvePoleDecVelocity() {
    if (m_parameterSolveCodes.find(VelocityPoleDec) != m_parameterSolveCodes.end())
      return true;
    return false;
  }


  /**
   * If the pole declination acceleration will be solved for with this target body.
   *
   * @return @b bool If it will be solved for.
   */
  bool BundleTargetBody::solvePoleDecAcceleration() {
    if (m_parameterSolveCodes.find(AccelerationPoleDec) != m_parameterSolveCodes.end())
      return true;
    return false;
  }


  /**
   * If the prime meridian angle will be solved for with this target body.
   *
   * @return @b bool If it will be solved for.
   */
  bool BundleTargetBody::solvePM() {
    if (m_parameterSolveCodes.find(PM) != m_parameterSolveCodes.end())
      return true;
    return false;
  }


  /**
   * If the prime meridian velocity will be solved for with this target body.
   *
   * @return @b bool If it will be solved for.
   */
  bool BundleTargetBody::solvePMVelocity() {
    if (m_parameterSolveCodes.find(VelocityPM) != m_parameterSolveCodes.end())
      return true;
    return false;
  }


  /**
   * If the prime meridian acceleration will be solved for with this target body.
   *
   * @return @b bool If it will be solved for.
   */
  bool BundleTargetBody::solvePMAcceleration() {
    if (m_parameterSolveCodes.find(AccelerationPM) != m_parameterSolveCodes.end())
      return true;
    return false;
  }


  /**
   * If the triaxial radii will be solved for with this target body.
   *
   * @return @b bool If it will be solved for.
   */
  bool BundleTargetBody::solveTriaxialRadii() {
    if (m_parameterSolveCodes.find(TriaxialRadiusA) != m_parameterSolveCodes.end() &&
        m_parameterSolveCodes.find(TriaxialRadiusB) != m_parameterSolveCodes.end() &&
        m_parameterSolveCodes.find(TriaxialRadiusC) != m_parameterSolveCodes.end())
      return true;
    return false;
  }


  /**
   * If the mean radius will be solved for with this target body.
   *
   * @return @b bool If it will be solved for.
   */
  bool BundleTargetBody::solveMeanRadius() {
    if (m_parameterSolveCodes.find(MeanRadius) != m_parameterSolveCodes.end())
      return true;
    return false;
  }


  /**
   * @brief Applies a vector of corrections to the parameters for the target body.
   *
   * Applies a vector of corrections to the internal parameters for the
   * target body and records the corrections in the internal corrections vector.
   * The corrections vector should be ordered the same as the parameter vector descriped
   * in setSolveSettings.
   *
   * @param corrections The vector containing correction values.
   *
   * @throws IException::Programmer "In BundleTargetBody::applyParameterCorrections:
   *                                 correction and m_targetParameter vectors sizes don't match."
   * @throws IException::Unknown "Unable to apply parameter corrections to BundleTargetBody."
   *
   * @see setSolveSettings
   */
  void BundleTargetBody::applyParameterCorrections(
      LinearAlgebra::Vector corrections) {
    if (corrections.size() != m_parameterSolveCodes.size()) {
      std::string msg = "In BundleTargetBody::applyParameterCorrections: "
                    "correction and m_targetParameter vectors sizes don't match.\n";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

//    for (int i = 0; i < corrections.size(); i++ )
//      printf("%lf\n",corrections(i));
//    std::cout << std::endl;

    try {
      int n = 0;
      for (std::set<int>::iterator it=m_parameterSolveCodes.begin();
           it!=m_parameterSolveCodes.end(); ++it) {
        switch (*it) {
          case PoleRA:
            m_raPole[0] += Angle(corrections[n], Angle::Radians);
            break;
          case VelocityPoleRA:
            m_raPole[1] += Angle(corrections[n], Angle::Radians);
            break;
          case AccelerationPoleRA:
            m_raPole[2] += Angle(corrections[n], Angle::Radians);
            break;
          case PoleDec:
            m_decPole[0] += Angle(corrections[n], Angle::Radians);
            break;
          case VelocityPoleDec:
            m_decPole[1] += Angle(corrections[n], Angle::Radians);
            break;
          case AccelerationPoleDec:
            m_decPole[2] += Angle(corrections[n], Angle::Radians);
            break;
          case PM:
            m_pm[0] += Angle(corrections[n], Angle::Radians);
            break;
          case VelocityPM:
            m_pm[1] += Angle(corrections[n], Angle::Radians);
            break;
          case AccelerationPM:
            m_pm[2] += Angle(corrections[n], Angle::Radians);
            break;
          case TriaxialRadiusA:
            {
              double c = m_radii[0].kilometers();
              double d = c + corrections[n];
              m_radii[0] = Distance(d, Distance::Kilometers);
//            m_radii[0] += Distance(corrections[n], Distance::Kilometers);
            }
            break;
          case TriaxialRadiusB:
            {
              double c = m_radii[1].kilometers();
              double d = c + corrections[n];
              m_radii[1] = Distance(d, Distance::Kilometers);
//            m_radii[1] += Distance(corrections[n], Distance::Kilometers);
            }
            break;
          case TriaxialRadiusC:
            {
              double c = m_radii[2].kilometers();
              double d = c + corrections[n];
              m_radii[2] = Distance(d, Distance::Kilometers);
//            m_radii[2] += Distance(corrections[n], Distance::Kilometers);
            }
            break;
          case MeanRadius:
            {
            double c = m_meanRadius.kilometers();
            double d = c + corrections[n];
            m_meanRadius = Distance(d, Distance::Kilometers);
//            m_meanRadius += Distance(corrections[n], Distance::Kilometers);
            }
            break;
          default:
            break;
        } // end switch

        m_corrections[n] += corrections[n];
        n++;
      } // end loop over m_parameterSolveCodes
    } // end try

    catch (IException &e) {
      std::string msg = "Unable to apply parameter corrections to BundleTargetBody.";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
  }


  /**
   * Converts a QString to a TargetRadiiSolveMethod.
   *
   * @param method The Qstring of a solve method.
   *
   * @return @b TargetRadiiSolveMethod The converted solve method.
   *
   * @throws IException::Programmer "Unknown target body radius solution method"
   */
  BundleTargetBody::TargetRadiiSolveMethod BundleTargetBody::stringToTargetRadiiOption(
      QString method) {
    if (method.compare("NONE", Qt::CaseInsensitive) == 0) {
      return None;
    }
    else if (method.compare("MEAN", Qt::CaseInsensitive) == 0) {
      return Mean;
    }
    else if (method.compare("ALL", Qt::CaseInsensitive) == 0) {
      return All;
    }
    else {
      throw IException(IException::Programmer,
                       "Unknown target body radius solution method [" + method + "].",
                       _FILEINFO_);
    }
  }


  /**
   * Converts a TargetRadiiSolveMethod to a QString
   *
   * @param method The TargetRadiiSolveMethod to convert.
   *
   * @return @b QString The solve method as a QString.
   *
   * @throws IException::Programmer "Unknown target body radius solve method enum"
   */
  QString BundleTargetBody::targetRadiiOptionToString(TargetRadiiSolveMethod method) {
    if (method == None)
      return "None";
    else if (method == Mean)
      return "MeanRadius";
    else if (method == All)
      return "Radii";
    else throw IException(IException::Programmer,
                          "Unknown target body radius solve method enum [" + toString(method) + "].",
                          _FILEINFO_);
  }


  /**
   * Returns the vector of parameter weights.
   * See setSolveSettings for how the vector is ordered.
   *
   * @return @b vector<double>& The vector of parameter weights.
   *
   * @see setSolveSettings
   */
  LinearAlgebra::Vector &BundleTargetBody::parameterWeights() {
    return m_weights;
  }


  /**
   * Returns the vector of corrections applied to the parameters.
   * See setSolveSettings for how the vector is ordered.
   *
   * @return @b vector<double>& The vector of parameter corrections.
   *
   * @see setSolveSettings
   */
  LinearAlgebra::Vector &BundleTargetBody::parameterCorrections() {
    return m_corrections;
  }


  /**
   * Returns the vector of parameters solution.
   * See setSolveSettings for how the vector is ordered.
   *
   * @return @b vector<double>& The vector of parameter solution.
   *
   * @see setSolveSettings
   */
  LinearAlgebra::Vector &BundleTargetBody::parameterSolution() {
    return m_solution;
  }


  /**
   * Returns the vector of apriori parameters sigmas.
   * See setSolveSettings for how the vector is ordered.
   *
   * @return @b vector<double>& The vector of apriori parameter sigmas.
   *
   * @see setSolveSettings
   */
  LinearAlgebra::Vector &BundleTargetBody::aprioriSigmas() {
    return m_aprioriSigmas;
  }


  /**
   * Returns the vector of adjusted parameters sigmas.
   * See setSolveSettings for how the vector is ordered.
   *
   * @return @b vector<double>& The vector of adjusted parameter sigmas.
   *
   * @see setSolveSettings
   */
  LinearAlgebra::Vector &BundleTargetBody::adjustedSigmas() {
    return m_adjustedSigmas;
  }

//  int BundleTargetBody::numberPoleParameters() {
//    int nPoleParameters = 0;
//    if (m_solveTargetBodyRaPole)
//      nPoleParameters++;
//    if (m_solveTargetBodyDecPole)
//      nPoleParameters++;
//    return nPoleParameters;
//  }


//  int BundleTargetBody::numberW0Parameters() {
//    if (m_solveTargetBodyZeroMeridian)
//      return 1;
//    return 0;
//  }


//  int BundleTargetBody::numberWDotParameters() {
//    if (m_solveTargetBodyRotationRate)
//      return 1;
//    return 0;
//  }


  /**
   * @brief Returns the number of radius parameters being solved for.
   *
   * Returns the number of radius parameters being solved for
   * which is based on the target radius solve method:
   * None -> 0
   * Mean -> 1
   * All  -> 3
   *
   * @return @b int The number of radius parameters being solved for.
   *
   * @see TargetRadiiSolveMethod
   */
  int BundleTargetBody::numberRadiusParameters() {
    if (m_solveTargetBodyRadiusMethod == All)
      return 3;
    else if (m_solveTargetBodyRadiusMethod == Mean)
      return 1;
    return 0;
  }


  /**
   * Returns the total number of parameters being solved for.
   *
   * @return @b int The number of parameters being solved for.
   */
  int BundleTargetBody::numberParameters() {
    return m_parameterSolveCodes.size();
  }


  /**
   * @brief Returns the coefficients of the right ascension polynomial.
   *
   * Returns The vector of right ascension polynomial coefficients ordered as:
   *
   * angle,
   * velocity,
   * acceleration.
   *
   * Only coefficients that are being solved for will be included.
   *
   * @return @b vector<Angle> The right ascension polynomial coefficients.
   */
  std::vector<Angle> BundleTargetBody::poleRaCoefs() {
    return m_raPole;
  }


  /**
   * @brief Returns the coefficients of the declination polynomial.
   *
   * Returns The vector of declination polynomial coefficients ordered as:
   *
   * angle,
   * velocity,
   * acceleration.
   *
   * Only coefficients that are being solved for will be included.
   *
   * @return @b vector<Angle> The declination polynomial coefficients.
   */
  std::vector<Angle> BundleTargetBody::poleDecCoefs() {
    return m_decPole;
  }


  /**
   * @brief Returns the coefficients of the prime meridian polynomial.
   *
   * Returns The vector of prime meridian polynomial coefficients ordered as:
   *
   * angle,
   * velocity,
   * acceleration.
   *
   * Only coefficients that are being solved for will be included.
   *
   * @return @b vector<Angle> The prime meridian polynomial coefficients.
   */
  std::vector<Angle> BundleTargetBody::pmCoefs() {
    return m_pm;
  }


  /**
   * @brief Returns the radius values.
   *
   * Returns the vector of radius values formatted as RadiusA, RadiusB, RadiusC.
   *
   * @return @b vector<Distance> The vector of radius values.
   *
   * @throws IException::Programmer "The triaxial radii can only be accessed
   *                                 when solving for triaxial radii."
   */
  std::vector<Distance> BundleTargetBody::radii() {
    if (!solveTriaxialRadii()) {
      std::string msg = "The triaxial radii can only be accessed when solving for triaxial radii.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return m_radii;
  }


  /**
   * Returns the mean radius.
   *
   * @return @b Distance The mean radius.
   *
   * @throws IException::Programmer "The mean radius can only be accessed
   *                                 when solving for mean radius."
   */
  Distance BundleTargetBody::meanRadius() {
    if (!solveMeanRadius()) {
      std::string msg = "The mean radius can only be accessed when solving for mean radius.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return m_meanRadius;
  }


  /**
   * @brief Calculates and returns the weighted sum of the squares of the corrections.
   *
   * Calculates and returns the weighted sum of the squares of the corrections
   * computed by V(T)*P*V where:
   * V is the vector of corrections,
   * P is the weight matrix,
   * V(T) is the transpose of V.
   *
   * @return @b double The weighted sum of the squares of the corrections (vtpv).
   */
  double BundleTargetBody::vtpv() {
    double vtpv = 0.0;
    double v;

    int nParameters = m_parameterSolveCodes.size();
    for (int i = 0; i < nParameters; i++) {
      v = m_corrections(i);

      if (m_weights(i) > 0.0)
        vtpv += v * v * m_weights(i);
    }

    return vtpv;
  }


  /**
   * Formats and returns the parameter values as a QString.
   *
   * @param errorPropagation If adjusted sigmas should be output.
   *
   * @return @b QString A formatted QString containing the parameter values.
   */
  QString BundleTargetBody::formatBundleOutputString(bool errorPropagation) {

    // for convenience, create vectors of parameters names and values in the correct sequence
    std::vector<double> finalParameterValues;
    QStringList parameterNamesList;
    QString str("%1");
    int nAngleParameters = 0;
    int nRadiusParameters = 0;
    if (solvePoleRA()) {
      finalParameterValues.push_back(m_raPole[0].degrees());
      parameterNamesList.append( str.arg("POLE RA    ") );
      nAngleParameters++;
    }
    if (solvePoleRAVelocity()) {
      finalParameterValues.push_back(m_raPole[1].degrees());
      parameterNamesList.append( str.arg("POLE RAv   ") );
      nAngleParameters++;
    }
    if (solvePoleRAAcceleration()) {
      finalParameterValues.push_back(m_raPole[2].degrees());
      parameterNamesList.append( str.arg("POLE RAa   ") );
      nAngleParameters++;
    }
    if (solvePoleDec()) {
      finalParameterValues.push_back(m_decPole[0].degrees());
      parameterNamesList.append( str.arg("POLE DEC   ") );
      nAngleParameters++;
    }
    if (solvePoleDecVelocity()) {
      finalParameterValues.push_back(m_decPole[1].degrees());
      parameterNamesList.append( str.arg("POLE DECv  ") );
      nAngleParameters++;
    }
    if (solvePoleDecAcceleration()) {
      finalParameterValues.push_back(m_decPole[2].degrees());
      parameterNamesList.append( str.arg("POLE DECa  ") );
      nAngleParameters++;
    }
    if (solvePM()) {
      finalParameterValues.push_back(m_pm[0].degrees());
      parameterNamesList.append( str.arg("  PM       ") );
      nAngleParameters++;
    }
    if (solvePMVelocity()) {
      finalParameterValues.push_back(m_pm[1].degrees());
      parameterNamesList.append( str.arg("  PMv      ") );
      nAngleParameters++;
    }
    if (solvePoleDecAcceleration()) {
      finalParameterValues.push_back(m_pm[2].degrees());
      parameterNamesList.append( str.arg("  PMa      ") );
      nAngleParameters++;
    }
    if (solveTriaxialRadii()) {
      finalParameterValues.push_back(m_radii[0].kilometers());
      finalParameterValues.push_back(m_radii[1].kilometers());
      finalParameterValues.push_back(m_radii[2].kilometers());
      parameterNamesList.append( str.arg("  RadiusA  ") );
      parameterNamesList.append( str.arg("  RadiusB  ") );
      parameterNamesList.append( str.arg("  RadiusC  ") );
      nRadiusParameters += 3;
    }
    if (solveMeanRadius()) {
      finalParameterValues.push_back(m_meanRadius.kilometers());
      parameterNamesList.append( str.arg("MeanRadius ") );
      nRadiusParameters++;
    }

    int nParameters = nAngleParameters + nRadiusParameters;

    m_parameterNamesList = parameterNamesList;
    QString finalqStr = "";
    QString qStr = "";
    QString sigma = "";

//    for (std::set<int>::iterator it=m_parameterSolveCodes.begin(); it!=m_parameterSolveCodes.end(); ++it) {
    for (int i = 0; i < nAngleParameters; i++) {

      if (m_aprioriSigmas[i] <= 0.0)
        sigma = "FREE";
      else
        sigma = toString(m_aprioriSigmas[i], 8);

      if (errorPropagation) {
        Angle corr_temp = Angle(m_corrections(i),Angle::Radians);
        qStr = QString("%1%2%3%4%5%6\n").
            arg( parameterNamesList.at(i) ).
            arg(finalParameterValues[i] - corr_temp.degrees(), 17, 'f', 8).
            arg(corr_temp.degrees(), 21, 'f', 8).
            arg(finalParameterValues[i], 20, 'f', 8).
            arg(sigma, 18).
            arg(m_adjustedSigmas[i], 18, 'f', 8);
      }
      else {
        Angle corr_temp = Angle(m_corrections(i),Angle::Radians);
        qStr = QString("%1%2%3%4%5%6\n").
            arg( parameterNamesList.at(i) ).
            arg(finalParameterValues[i] - corr_temp.degrees(), 17, 'f', 8).
            arg(corr_temp.degrees(), 21, 'f', 8).
            arg(finalParameterValues[i], 20, 'f', 8).
            arg(sigma, 18).
            arg("N/A", 18);
      }
      finalqStr += qStr;
    }

    for (int i = nAngleParameters; i < nParameters; i++) {

      if (m_aprioriSigmas[i] <= 0.0)
        sigma = "FREE";
      else
        sigma = toString(m_aprioriSigmas[i], 8);

      if (errorPropagation) {
        double d1 = finalParameterValues[i];
        double d2 = m_corrections(i);
        qStr = QString("%1%2%3%4%5%6\n").
            arg( parameterNamesList.at(i) ).
            arg(d1 - d2, 17, 'f', 8).
            arg(d2, 21, 'f', 8).
            arg(d1, 20, 'f', 8).
            arg(sigma, 18).
            arg(m_adjustedSigmas[i], 18, 'f', 8);
      }
      else {
        double d1 = finalParameterValues[i];
        double d2 = m_corrections(i);
        qStr = QString("%1%2%3%4%5%6\n").
            arg( parameterNamesList.at(i) ).
            arg(d1 - d2, 17, 'f', 8).
            arg(d2, 21, 'f', 8).
            arg(d1, 20, 'f', 8).
            arg(sigma, 18).
            arg("N/A", 18);
      }
      finalqStr += qStr;
    }

    return finalqStr;
  }


  /**
   * Returns a list of all the parameters being solved for as QStrings.
   * This should only be called after formatBundleOutputString.
   *
   * @return @b QStringList A list of the parameters being solved for.
   *
   * @see formatBundleOutputString
   */
  QStringList BundleTargetBody::parameterList() {
    return m_parameterNamesList;
  }


  /**
   * Set bundle solve parameters for target body from a pvl file.
   *
   * specifically for standard jigsaw interface, not the
   * Integrated Photogrammetric Control Environment (IPCE)
   *
   * @param tbObject The Pvl file to read from.
   *
   * @return @b bool If the solve parameters were successfuly set.
   *
   * @throws IException::User "Ra must be given as none, position, velocity, or acceleration"
   * @throws IException::User "Dec must be given as none, position, velocity, or acceleration"
   * @throws IException::User "Pm must be given as none, position, velocity, or acceleration"
   * @throws IException::User "RadiiSolveOption must be given as none, triaxial, or mean"
   * @throws IException::User "RaValue must be a valid double (>= 0; blank defaults to 0)."
   * @throws IException::User "RaSigma must be a valid double (>= 0; blank defaults to 0)."
   * @throws IException::User "RaVelocityValue must be a valid double (>= 0; blank defaults to 0)."
   * @throws IException::User "RaVelocitySigma must be a valid double (>= 0; blank defaults to 0)."
   * @throws IException::User "RaAccelerationValue must be a valid double
   *                           (>= 0; blank defaults to 0)."
   * @throws IException::User "RaAccelerationSigma must be a valid double
   *                           (>= 0; blank defaults to 0)."
   * @throws IException::User "DecValue must be a valid double (>= 0; blank defaults to 0)."
   * @throws IException::User "DecSigma must be a valid double (>= 0; blank defaults to 0)."
   * @throws IException::User "DecVelocityValue must be a valid double
   *                           (>= 0; blank defaults to 0)."
   * @throws IException::User "DecVelocitySigma must be a valid double
   *                           (>= 0; blank defaults to 0)."
   * @throws IException::User "DecAccelerationValue must be a valid double
   *                           (>= 0; blank defaults to 0)."
   * @throws IException::User "DecAccelerationSigma must be a valid double
   *                           (>= 0; blank defaults to 0)."
   * @throws IException::User "PmValue must be a valid double (>= 0; blank defaults to 0)."
   * @throws IException::User "PmSigma must be a valid double (>= 0; blank defaults to 0)."
   * @throws IException::User "PmVelocityValue must be a valid double (>= 0; blank defaults to 0)."
   * @throws IException::User "PmVelocitySigma must be a valid double (>= 0; blank defaults to 0)."
   * @throws IException::User "PmAccelerationValue must be a valid double
   *                           (>= 0; blank defaults to 0)."
   * @throws IException::User "PmAccelerationSigma must be a valid double
   *                           (>= 0; blank defaults to 0)."
   * @throws IException::User "RadiusAValue must be a valid double (blank defaults to 0)."
   * @throws IException::User "RadiusAValue must be >= 0."
   * @throws IException::User "RadiusASigma must be a valid double (blank defaults to 0)."
   * @throws IException::User "RadiusASigma must be >= 0."
   * @throws IException::User "RadiusBValue must be a valid double (blank defaults to 0)."
   * @throws IException::User "RadiusBValue must be >= 0".
   * @throws IException::User "RadiusBSigma must be a valid double (blank defaults to 0)."
   * @throws IException::User "RadiusBSigma must be >= 0".
   * @throws IException::User "RadiusCValue must be a valid double (blank defaults to 0)."
   * @throws IException::User "RadiusCValue must be >= 0".
   * @throws IException::User "RadiusCSigma must be a valid double (blank defaults to 0)."
   * @throws IException::User "RadiusCSigma must be >= 0".
   * @throws IException::User "MeanRadiusValue must be a valid double (blank defaults to 0)."
   * @throws IException::User "MeanRadiusValue must be >= 0".
   * @throws IException::User "MeanRadiusSigma must be a valid double (blank defaults to 0)."
   * @throws IException::User "MeanRadiusSigma must be >= 0".
   *
   * @see setSolveSettings
   */
  bool BundleTargetBody::readFromPvl(PvlObject &tbObject) {
    // reset defaults
    //initialize();

    double d = -1.0;
    QString str;
    TargetRadiiSolveMethod solveRadiiMethod = None;

    //TODO Solve method keywords need to be checked for validity. JAM

    // determine which parameters we're solving for
    std::set<int> targetParameterSolveCodes;
    PvlObject::PvlGroupIterator g;
    for (g = tbObject.beginGroup(); g != tbObject.endGroup(); ++g) {
      if (g->hasKeyword("Ra")) {
        try {
          str = QString::fromStdString(g->findKeyword("Ra")[0]);
        }
        catch (IException &e) {
          std::string msg = "Ra must be given as none, position, velocity, or acceleration";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        if (str == "position") {
          targetParameterSolveCodes.insert(BundleTargetBody::PoleRA);
        }
        else if (str == "velocity") {
          targetParameterSolveCodes.insert(BundleTargetBody::PoleRA);
          targetParameterSolveCodes.insert(BundleTargetBody::VelocityPoleRA);
        }
        else if (str == "acceleration") {
          targetParameterSolveCodes.insert(BundleTargetBody::PoleRA);
          targetParameterSolveCodes.insert(BundleTargetBody::VelocityPoleRA);
          targetParameterSolveCodes.insert(BundleTargetBody::AccelerationPoleRA);
        }
      }

      if (g->hasKeyword("Dec")) {
        try {
          str = QString::fromStdString(g->findKeyword("Dec")[0]);
        }
        catch (IException &e) {
          std::string msg = "Dec must be given as none, position, velocity, or acceleration";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        if (str == "position") {
          targetParameterSolveCodes.insert(BundleTargetBody::PoleDec);
        }
        else if (str == "velocity") {
          targetParameterSolveCodes.insert(BundleTargetBody::PoleDec);
          targetParameterSolveCodes.insert(BundleTargetBody::VelocityPoleDec);
        }
        else if (str == "acceleration") {
          targetParameterSolveCodes.insert(BundleTargetBody::PoleDec);
          targetParameterSolveCodes.insert(BundleTargetBody::VelocityPoleDec);
          targetParameterSolveCodes.insert(BundleTargetBody::AccelerationPoleDec);
        }
      }

      if (g->hasKeyword("Pm")) {
        try {
          str = QString::fromStdString(g->findKeyword("Pm")[0]);
        }
        catch (IException &e) {
          std::string msg = "Pm must be given as none, position, velocity, or acceleration";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        if (str == "position") {
          targetParameterSolveCodes.insert(BundleTargetBody::PM);
        }
        else if (str == "velocity") {
          targetParameterSolveCodes.insert(BundleTargetBody::PM);
          targetParameterSolveCodes.insert(BundleTargetBody::VelocityPM);
        }
        else if (str == "acceleration") {
          targetParameterSolveCodes.insert(BundleTargetBody::PM);
          targetParameterSolveCodes.insert(BundleTargetBody::VelocityPM);
          targetParameterSolveCodes.insert(BundleTargetBody::AccelerationPM);
        }
      }

      if (g->hasKeyword("RadiiSolveOption")) {
        try {
          str = QString::fromStdString(g->findKeyword("RadiiSolveOption")[0]);
        }
        catch (IException &e) {
          std::string msg = "RadiiSolveOption must be given as none, triaxial, or mean";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        if (str == "triaxial") {
          targetParameterSolveCodes.insert(BundleTargetBody::TriaxialRadiusA);
          targetParameterSolveCodes.insert(BundleTargetBody::TriaxialRadiusB);
          targetParameterSolveCodes.insert(BundleTargetBody::TriaxialRadiusC);
          solveRadiiMethod = All;
        }
        else if (str == "mean") {
          targetParameterSolveCodes.insert(BundleTargetBody::MeanRadius);
          solveRadiiMethod = Mean;
        }
        else
          solveRadiiMethod = None;
      }
    }

    Angle aprioriPoleRA;
    Angle poleRASigma;
    Angle aprioriVelocityPoleRA;
    Angle poleRAVelocitySigma;
    Angle aprioriAccelerationPoleRA;
    Angle poleRAAccelerationSigma;
    Angle aprioriPoleDec;
    Angle poleDecSigma;
    Angle aprioriVelocityPoleDec;
    Angle sigmaVelocityPoleDec;
    Angle aprioriAccelerationPoleDec;
    Angle sigmaAccelerationPoleDec;
    Angle aprioriPM;
    Angle sigmaPM;
    Angle aprioriVelocityPM;
    Angle sigmaVelocityPM;
    Angle aprioriAccelerationPM;
    Angle pmAccelerationSigma;
    Distance aprioriRadiusA;
    Distance sigmaRadiusA;
    Distance aprioriRadiusB;
    Distance sigmaRadiusB;
    Distance aprioriRadiusC;
    Distance sigmaRadiusC;
    Distance aprioriMeanRadius;
    Distance sigmaMeanRadius;

    //TODO Determine which need to be non-negative. JAM

    for (g = tbObject.beginGroup(); g != tbObject.endGroup(); ++g) {
      if (g->hasKeyword("RaValue")) {
        try {
          d = (double)(g->findKeyword("RaValue"));
        }
        catch (IException &e) {
          std::string msg = "RaValue must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        aprioriPoleRA = Angle(d, Angle::Degrees);
      }

      if (g->hasKeyword("RaSigma")) {
        try {
          d = (double)(g->findKeyword("RaSigma"));
        }
        catch (IException &e) {
          std::string msg = "RaSigma must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        poleRASigma = Angle(d, Angle::Degrees);
      }

      if (g->hasKeyword("RaVelocityValue")) {
        try {
          d = (double)(g->findKeyword("RaVelocityValue"));
        }
        catch (IException &e) {
          std::string msg = "RaVelocityValue must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        aprioriVelocityPoleRA = Angle(d, Angle::Degrees);
      }

      if (g->hasKeyword("RaVelocitySigma")) {
        try {
          d = (double)(g->findKeyword("RaVelocitySigma"));
        }
        catch (IException &e) {
          std::string msg = "RaVelocitySigma must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        poleRAVelocitySigma = Angle(d, Angle::Degrees);
      }

      if (g->hasKeyword("RaAccelerationValue")) {
        try {
          d = (double)(g->findKeyword("RaAccelerationValue"));
        }
        catch (IException &e) {
          std::string msg = "RaAccelerationValue must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        aprioriAccelerationPoleRA = Angle(d, Angle::Degrees);
      }

      if (g->hasKeyword("RaAccelerationSigma")) {
        try {
          d = (double)(g->findKeyword("RaAccelerationSigma"));
        }
        catch (IException &e) {
          std::string msg = "RaAccelerationSigma must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        poleRAAccelerationSigma = Angle(d, Angle::Degrees);
      }

      if (g->hasKeyword("DecValue")) {
        try {
          d = (double)(g->findKeyword("DecValue"));
        }
        catch (IException &e) {
          std::string msg = "DecValue must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        aprioriPoleDec = Angle(d, Angle::Degrees);
      }

      if (g->hasKeyword("DecSigma")) {
        try {
          d = (double)(g->findKeyword("DecSigma"));
        }
        catch (IException &e) {
          std::string msg = "DecSigma must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        poleDecSigma = Angle(d, Angle::Degrees);
      }

      if (g->hasKeyword("DecVelocityValue")) {
        try {
          d = (double)(g->findKeyword("DecVelocityValue"));
        }
        catch (IException &e) {
          std::string msg = "DecVelocityValue must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        aprioriVelocityPoleDec = Angle(d, Angle::Degrees);
      }

      if (g->hasKeyword("DecVelocitySigma")) {
        try {
          d = (double)(g->findKeyword("DecVelocitySigma"));
        }
        catch (IException &e) {
          std::string msg = "DecVelocitySigma must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        sigmaVelocityPoleDec = Angle(d, Angle::Degrees);
      }

      if (g->hasKeyword("DecAccelerationValue")) {
        try {
          d = (double)(g->findKeyword("DecAccelerationValue"));
        }
        catch (IException &e) {
          std::string msg = "DecAccelerationValue must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        aprioriAccelerationPoleDec = Angle(d, Angle::Degrees);
      }

      if (g->hasKeyword("DecAccelerationSigma")) {
        try {
          d = (double)(g->findKeyword("DecAccelerationSigma"));
        }
        catch (IException &e) {
          std::string msg = "DecAccelerationSigma must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        sigmaAccelerationPoleDec = Angle(d, Angle::Degrees);
      }

      if (g->hasKeyword("PmValue")) {
        try {
          d = (double)(g->findKeyword("PmValue"));
        }
        catch (IException &e) {
          std::string msg = "PmValue must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        aprioriPM = Angle(d, Angle::Degrees);
      }

      if (g->hasKeyword("PmSigma")) {
        try {
          d = (double)(g->findKeyword("PmSigma"));
        }
        catch (IException &e) {
          std::string msg = "PmSigma must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        sigmaPM = Angle(d, Angle::Degrees);
      }


      if (g->hasKeyword("PmVelocityValue")) {
        try {
          d = (double)(g->findKeyword("PmVelocityValue"));
        }
        catch (IException &e) {
          std::string msg = "PmVelocityValue must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        aprioriVelocityPM = Angle(d, Angle::Degrees);
      }

      if (g->hasKeyword("PmVelocitySigma")) {
        try {
          d = (double)(g->findKeyword("PmVelocitySigma"));
        }
        catch (IException &e) {
          std::string msg = "PmVelocitySigma must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        sigmaVelocityPM = Angle(d, Angle::Degrees);
      }

      if (g->hasKeyword("PmAccelerationValue")) {
        try {
          d = (double)(g->findKeyword("PmAccelerationValue"));
        }
        catch (IException &e) {
          std::string msg = "PmAccelerationValue must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        aprioriAccelerationPM = Angle(d, Angle::Degrees);
      }

      if (g->hasKeyword("PmAccelerationSigma")) {
        try {
          d = (double)(g->findKeyword("PmAccelerationSigma"));
        }
        catch (IException &e) {
          std::string msg = "PmAccelerationSigma must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        pmAccelerationSigma = Angle(d, Angle::Degrees);
      }

      if (g->hasKeyword("RadiusAValue")) {
        try {
          d = (double)(g->findKeyword("RadiusAValue"));
        }
        catch (IException &e) {
          std::string msg = "RadiusAValue must be a valid double (blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        try {
          aprioriRadiusA = Distance(d, Distance::Meters);
        }
        catch (IException &e) {
          std::string msg = "RadiusAValue must be >= 0.";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }

      if (g->hasKeyword("RadiusASigma")) {
        try {
          d = (double)(g->findKeyword("RadiusASigma"));
        }
        catch (IException &e) {
          std::string msg = "RadiusASigma must be a valid double (blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        try {
          sigmaRadiusA = Distance(d, Distance::Meters);
        }
        catch (IException &e) {
          std::string msg = "RadiusASigma must be >= 0.";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }

      if (g->hasKeyword("RadiusBValue")) {
        try {
          d = (double)(g->findKeyword("RadiusBValue"));
        }
        catch (IException &e) {
          std::string msg = "RadiusBValue must be a valid double (blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        try {
          aprioriRadiusB = Distance(d, Distance::Meters);
        }
        catch (IException &e) {
          std::string msg = "RadiusBValue must be >= 0.";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }

      if (g->hasKeyword("RadiusBSigma")) {
        try {
          d = (double)(g->findKeyword("RadiusBSigma"));
        }
        catch (IException &e) {
          std::string msg = "RadiusBSigma must be a valid double (blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        try {
          sigmaRadiusB = Distance(d, Distance::Meters);
        }
        catch (IException &e) {
          std::string msg = "RadiusBSigma must be >= 0.";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }

      if (g->hasKeyword("RadiusCValue")) {
        try {
          d = (double)(g->findKeyword("RadiusCValue"));
        }
        catch (IException &e) {
          std::string msg = "RadiusCValue must be a valid double (blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        try {
          aprioriRadiusC = Distance(d, Distance::Meters);
        }
        catch (IException &e) {
          std::string msg = "RadiusCValue must be >= 0.";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }

      if (g->hasKeyword("RadiusCSigma")) {
        try {
          d = (double)(g->findKeyword("RadiusCSigma"));
        }
        catch (IException &e) {
          std::string msg = "RadiusCSigma must be a valid double (blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        try {
          sigmaRadiusC = Distance(d, Distance::Meters);
        }
        catch (IException &e) {
          std::string msg = "RadiusCSigma must be >= 0.";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }

      if (g->hasKeyword("MeanRadiusValue")) {
        try {
          d = (double)(g->findKeyword("MeanRadiusValue"));
        }
        catch (IException &e) {
          std::string msg = "MeanRadiusValue must be a valid double (blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        try {
          aprioriMeanRadius = Distance(d, Distance::Meters);
        }
        catch (IException &e) {
          std::string msg = "MeanRadiusValue must be >= 0.";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }

      if (g->hasKeyword("MeanRadiusSigma")) {
        try {
          d = (double)(g->findKeyword("MeanRadiusSigma"));
        }
        catch (IException &e) {
          std::string msg = "MeanRadiusSigma must be a valid double (blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        try {
          sigmaMeanRadius = Distance(d, Distance::Meters);
        }
        catch (IException &e) {
          std::string msg = "MeanRadiusSigma must be >= 0.";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }
    }

/*
    try {
      // set target id
//      setInstrumentId((QString)tbParameterGroup.nameKeyword());

      }
    catch (IException &e) {
      std::string msg = "Unable to set target body solve options from the given PVL.";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }
*/

      setSolveSettings(targetParameterSolveCodes,
           aprioriPoleRA, poleRASigma,
           aprioriVelocityPoleRA, poleRAVelocitySigma,
           aprioriPoleDec, poleDecSigma,
           aprioriVelocityPoleDec, sigmaVelocityPoleDec,
           aprioriPM, sigmaPM,
           aprioriVelocityPM, sigmaVelocityPM,
           solveRadiiMethod,
           aprioriRadiusA, sigmaRadiusA,
           aprioriRadiusB, sigmaRadiusB,
           aprioriRadiusC, sigmaRadiusC,
           aprioriMeanRadius, sigmaMeanRadius);

    return true;
  }


  /**
   * Gets the local radius for the given latitude/longitude coordinate.
   *
   * @param lat Latitude coordinate.
   * @param lon Longitude coordinate.
   *
   * @return @b Distance The distance from the center of the ellipsoid to its surface
   *                     at the given lat/lon location.
   *
   * @throws IException::Programmer "Local radius can only be found if
   *                                 triaxial radii were solved for."
   */
  Distance BundleTargetBody::localRadius(const Latitude &lat, const Longitude &lon) {

    if (!solveTriaxialRadii()) {
      std::string msg = "Local radius can only be found if triaxial radii were solved for.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    double a = m_radii[0].kilometers();
    double b = m_radii[1].kilometers();
    double c = m_radii[2].kilometers();

    double rlat = lat.radians();
    double rlon = lon.radians();

    double xyradius = a * b / sqrt(pow(b * cos(rlon), 2) +
                      pow(a * sin(rlon), 2));
    const double &radius = xyradius * c / sqrt(pow(c * cos(rlat), 2) +
                           pow(xyradius * sin(rlat), 2));

    return Distance(radius, Distance::Kilometers);
  }


  /**
   * set bundle solve parameters for an observation
   */
/*
  bool BundleTargetBody::readFromPvl(PvlGroup &tbParameterGroup) {
    // reset defaults
    //initialize();

    bool b = false;
    double d = -1.0;

    try {
      // set target id
//      setInstrumentId((QString)tbParameterGroup.nameKeyword());

      // pole right ascension settings
      if (tbParameterGroup.hasKeyword("Ra")) {
        try {
          b = toBool(tbParameterGroup.findKeyword("Ra")[0]);
        }
        catch (IException &e) {
          std::string msg = "Ra must be a valid boolean (yes/no; true/false).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        if (b)
          m_parameterSolveCodes.insert(BundleTargetBody::PoleRA);
      }

      if (tbParameterGroup.hasKeyword("RaValue")) {
        try {
          d = (double)(tbParameterGroup.findKeyword("RaValue"));
        }
        catch (IException &e) {
          std::string msg = "RaValue must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        m_raPole[0] = Angle(d, Angle::Degrees);
      }

      if (tbParameterGroup.hasKeyword("RaSigma")) {
        try {
          d = (double)(tbParameterGroup.findKeyword("RaSigma"));
        }
        catch (IException &e) {
          std::string msg = "RaSigma must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        //m_raPole[1] = Angle(d, Angle::Degrees);
      }

      if (tbParameterGroup.hasKeyword("RaVelocity")) {
        try {
          b = toBool(tbParameterGroup.findKeyword("RaVelocity")[0]);
        }
        catch (IException &e) {
          std::string msg = "RaVelocity must be a valid boolean (yes/no; true/false).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        if (b)
          m_parameterSolveCodes.insert(BundleTargetBody::VelocityPoleRA);
      }

      if (tbParameterGroup.hasKeyword("RaVelocityValue")) {
        try {
          d = (double)(tbParameterGroup.findKeyword("RaVelocityValue"));
        }
        catch (IException &e) {
          std::string msg = "RaVelocityValue must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        m_raPole[1] = Angle(d, Angle::Degrees);
      }

      if (tbParameterGroup.hasKeyword("RaVelocitySigma")) {
        try {
          d = (double)(tbParameterGroup.findKeyword("RaVelocitySigma"));
        }
        catch (IException &e) {
          std::string msg = "RaVelocitySigma must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        //m_raPole[1] = Angle(d, Angle::Degrees);
      }

      if (tbParameterGroup.hasKeyword("RaAcceleration")) {
        try {
          b = toBool(tbParameterGroup.findKeyword("RaAcceleration")[0]);
        }
        catch (IException &e) {
          std::string msg = "RaAcceleration must be a valid boolean (yes/no; true/false).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        if (b)
          m_parameterSolveCodes.insert(BundleTargetBody::VelocityPoleRA);
      }

      if (tbParameterGroup.hasKeyword("RaAccelerationValue")) {
        try {
          d = (double)(tbParameterGroup.findKeyword("RaAccelerationValue"));
        }
        catch (IException &e) {
          std::string msg = "RaAccelerationValue must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        m_raPole[2] = Angle(d, Angle::Degrees);
      }

      if (tbParameterGroup.hasKeyword("RaAccelerationSigma")) {
        try {
          d = (double)(tbParameterGroup.findKeyword("RaAccelerationSigma"));
        }
        catch (IException &e) {
          std::string msg = "RaAccelerationSigma must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        //m_raPole[1] = Angle(d, Angle::Degrees);
      }

      // pole declination settings
      if (tbParameterGroup.hasKeyword("Dec")) {
        try {
          b = toBool(tbParameterGroup.findKeyword("Dec")[0]);
        }
        catch (IException &e) {
          std::string msg = "Dec parameter must be a valid boolean (yes/no; true/false).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        if (b)
          m_parameterSolveCodes.insert(BundleTargetBody::PoleDec);
      }

      if (tbParameterGroup.hasKeyword("DecValue")) {
        try {
          d = (double)(tbParameterGroup.findKeyword("DecValue"));
        }
        catch (IException &e) {
          std::string msg = "DecValue must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        m_decPole[0] = Angle(d, Angle::Degrees);
      }

      if (tbParameterGroup.hasKeyword("DecSigma")) {
        try {
          d = (double)(tbParameterGroup.findKeyword("DecSigma"));
        }
        catch (IException &e) {
          std::string msg = "DecSigma must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        //m_raPole[1] = Angle(d, Angle::Degrees);
      }

      if (tbParameterGroup.hasKeyword("DecVelocity")) {
        try {
          b = toBool(tbParameterGroup.findKeyword("DecVelocity")[0]);
        }
        catch (IException &e) {
          std::string msg = "DecVelocity must be a valid boolean (yes/no; true/false).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        if (b)
          m_parameterSolveCodes.insert(BundleTargetBody::VelocityPoleDec);
      }

      if (tbParameterGroup.hasKeyword("DecVelocityValue")) {
        try {
          d = (double)(tbParameterGroup.findKeyword("DecVelocityValue"));
        }
        catch (IException &e) {
          std::string msg = "DecVelocityValue must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        m_decPole[1] = Angle(d, Angle::Degrees);
      }

      if (tbParameterGroup.hasKeyword("DecVelocitySigma")) {
        try {
          d = (double)(tbParameterGroup.findKeyword("DecVelocitySigma"));
        }
        catch (IException &e) {
          std::string msg = "DecVelocitySigma must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        //m_raPole[1] = Angle(d, Angle::Degrees);
      }

      if (tbParameterGroup.hasKeyword("DecAcceleration")) {
        try {
          b = toBool(tbParameterGroup.findKeyword("DecAcceleration")[0]);
        }
        catch (IException &e) {
          std::string msg = "DecAcceleration must be a valid boolean (yes/no; true/false).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        if (b)
          m_parameterSolveCodes.insert(BundleTargetBody::AccelerationPoleDec);
      }

      if (tbParameterGroup.hasKeyword("DecAccelerationValue")) {
        try {
          d = (double)(tbParameterGroup.findKeyword("DecAccelerationValue"));
        }
        catch (IException &e) {
          std::string msg = "DecAccelerationValue must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        m_decPole[2] = Angle(d, Angle::Degrees);
      }

      if (tbParameterGroup.hasKeyword("DecAccelerationSigma")) {
        try {
          d = (double)(tbParameterGroup.findKeyword("DecAccelerationSigma"));
        }
        catch (IException &e) {
          std::string msg = "DecAccelerationSigma must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        //m_raPole[1] = Angle(d, Angle::Degrees);
      }

      // prime meridian settings
      if (tbParameterGroup.hasKeyword("Pm")) {
        try {
          b = toBool(tbParameterGroup.findKeyword("Pm")[0]);
        }
        catch (IException &e) {
          std::string msg = "Pm parameter must be a valid boolean (yes/no; true/false).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        if (b)
          m_parameterSolveCodes.insert(BundleTargetBody::PM);
      }

      if (tbParameterGroup.hasKeyword("PmValue")) {
        try {
          d = (double)(tbParameterGroup.findKeyword("PmValue"));
        }
        catch (IException &e) {
          std::string msg = "PmValue must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        m_pm[0] = Angle(d, Angle::Degrees);
      }

      if (tbParameterGroup.hasKeyword("PmSigma")) {
        try {
          d = (double)(tbParameterGroup.findKeyword("PmSigma"));
        }
        catch (IException &e) {
          std::string msg = "PmSigma must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        //m_raPole[1] = Angle(d, Angle::Degrees);
      }

      if (tbParameterGroup.hasKeyword("PmVelocity")) {
        try {
          b = toBool(tbParameterGroup.findKeyword("PmVelocity")[0]);
        }
        catch (IException &e) {
          std::string msg = "PmVelocity must be a valid boolean (yes/no; true/false).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        if (b)
          m_parameterSolveCodes.insert(BundleTargetBody::VelocityPM);
      }

      if (tbParameterGroup.hasKeyword("PmVelocityValue")) {
        try {
          d = (double)(tbParameterGroup.findKeyword("PmVelocityValue"));
        }
        catch (IException &e) {
          std::string msg = "PmVelocityValue must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        m_pm[1] = Angle(d, Angle::Degrees);
      }

      if (tbParameterGroup.hasKeyword("PmVelocitySigma")) {
        try {
          d = (double)(tbParameterGroup.findKeyword("PmVelocitySigma"));
        }
        catch (IException &e) {
          std::string msg = "PmVelocitySigma must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        //m_raPole[1] = Angle(d, Angle::Degrees);
      }

      if (tbParameterGroup.hasKeyword("PmAcceleration")) {
        try {
          b = toBool(tbParameterGroup.findKeyword("PmAcceleration")[0]);
        }
        catch (IException &e) {
          std::string msg = "PmAcceleration must be a valid boolean (yes/no; true/false).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        if (b)
          m_parameterSolveCodes.insert(BundleTargetBody::AccelerationPM);
      }

      if (tbParameterGroup.hasKeyword("PmAccelerationValue")) {
        try {
          d = (double)(tbParameterGroup.findKeyword("PmAccelerationValue"));
        }
        catch (IException &e) {
          std::string msg = "PmAccelerationValue must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        m_decPole[2] = Angle(d, Angle::Degrees);
      }

      if (tbParameterGroup.hasKeyword("PmAccelerationSigma")) {
        try {
          d = (double)(tbParameterGroup.findKeyword("PmAccelerationSigma"));
        }
        catch (IException &e) {
          std::string msg = "PmAccelerationSigma must be a valid double (>= 0; blank defaults to 0).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        //m_raPole[1] = Angle(d, Angle::Degrees);
      }

      // radii settings
      if (tbParameterGroup.hasKeyword("AllRadii")) {
        try {
          b = toBool(tbParameterGroup.findKeyword("AllRadii")[0]);
        }
        catch (IException &e) {
          std::string msg = "AllRadii parameter must be a valid boolean (yes/no; true/false).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        if (b) {
          m_parameterSolveCodes.insert(BundleTargetBody::TriaxialRadiusA);
          m_parameterSolveCodes.insert(BundleTargetBody::TriaxialRadiusB);
          m_parameterSolveCodes.insert(BundleTargetBody::TriaxialRadiusC);

          if (tbParameterGroup.hasKeyword("RadiusAValue")) {
            try {
              d = (double)(tbParameterGroup.findKeyword("RadiusAValue"));
            }
            catch (IException &e) {
              std::string msg = "RadiusAValue must be a valid double (>= 0; blank defaults to 0).";
              throw IException(IException::User, msg, _FILEINFO_);
            }
            m_radii[0].setKilometers(d);
          }

          if (tbParameterGroup.hasKeyword("RadiusASigma")) {
            try {
              d = (double)(tbParameterGroup.findKeyword("RadiusASigma"));
            }
            catch (IException &e) {
              std::string msg = "RadiusASigma must be a valid double (>= 0; blank defaults to 0).";
              throw IException(IException::User, msg, _FILEINFO_);
            }
            //m_raPole[1] = Angle(d, Angle::Degrees);
          }

          if (tbParameterGroup.hasKeyword("RadiusBValue")) {
            try {
              d = (double)(tbParameterGroup.findKeyword("RadiusBValue"));
            }
            catch (IException &e) {
              std::string msg = "RadiusBValue must be a valid double (>= 0; blank defaults to 0).";
              throw IException(IException::User, msg, _FILEINFO_);
            }
            m_radii[1].setKilometers(d);
          }

          if (tbParameterGroup.hasKeyword("RadiusBSigma")) {
            try {
              d = (double)(tbParameterGroup.findKeyword("RadiusBSigma"));
            }
            catch (IException &e) {
              std::string msg = "RadiusBSigma must be a valid double (>= 0; blank defaults to 0).";
              throw IException(IException::User, msg, _FILEINFO_);
            }
            //m_raPole[1] = Angle(d, Angle::Degrees);
          }

          if (tbParameterGroup.hasKeyword("RadiusCValue")) {
            try {
              d = (double)(tbParameterGroup.findKeyword("RadiusCValue"));
            }
            catch (IException &e) {
              std::string msg = "RadiusCValue must be a valid double (>= 0; blank defaults to 0).";
              throw IException(IException::User, msg, _FILEINFO_);
            }
            m_radii[2].setKilometers(d);
          }

          if (tbParameterGroup.hasKeyword("RadiusCSigma")) {
            try {
              d = (double)(tbParameterGroup.findKeyword("RadiusCSigma"));
            }
            catch (IException &e) {
              std::string msg = "RadiusCSigma must be a valid double (>= 0; blank defaults to 0).";
              throw IException(IException::User, msg, _FILEINFO_);
            }
            //m_raPole[1] = Angle(d, Angle::Degrees);
          }
        }
      }
      else if (tbParameterGroup.hasKeyword("MeanRadius")) {
        try {
          b = toBool(tbParameterGroup.findKeyword("MeanRadius")[0]);
        }
        catch (IException &e) {
          std::string msg = "MeanRadius parameter must be a valid boolean (yes/no; true/false).";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        if (b) {
          m_parameterSolveCodes.insert(BundleTargetBody::MeanRadius);

          if (tbParameterGroup.hasKeyword("MeanRadiusValue")) {
            try {
              d = (double)(tbParameterGroup.findKeyword("MeanRadiusValue"));
            }
            catch (IException &e) {
              std::string msg = "MeanRadiusValue must be a valid double (>= 0; blank defaults to 0).";
              throw IException(IException::User, msg, _FILEINFO_);
            }
            m_radii[2].setKilometers(d);
          }

          if (tbParameterGroup.hasKeyword("MeanRadiusSigma")) {
            try {
              d = (double)(tbParameterGroup.findKeyword("MeanRadiusSigma"));
            }
            catch (IException &e) {
              std::string msg = "MeanRadiusSigma must be a valid double (>= 0; blank defaults to 0).";
              throw IException(IException::User, msg, _FILEINFO_);
            }
            //m_raPole[1] = Angle(d, Angle::Degrees);
          }
        }
      }
    }

    catch (IException &e) {
      std::string msg = "Unable to set target body solve options from the given PVL.";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }

    return true;
  }
*/
}
