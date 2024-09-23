/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "BundleObservationSolveSettings.h"

#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QList>
#include <QSet>
#include <QString>
#include <QUuid>
#include <QXmlInputSource>
#include <QXmlStreamWriter>

#include "BundleImage.h"
#include "Camera.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Project.h"
#include "PvlKeyword.h"
#include "PvlObject.h"


namespace Isis {

  /**
   * Constructor with default parameter initializations.
   */
  BundleObservationSolveSettings::BundleObservationSolveSettings() {
    initialize();
  }


  /**
   * Construct this BundleObservationSolveSettings object from XML.
   *
   * @param bundleSettingsFolder Where this settings XML resides -
   *                             /work/.../projectRoot/images/import1
   * @param xmlReader An XML reader that's up to an <bundleSettings/> tag.
   */
  BundleObservationSolveSettings::BundleObservationSolveSettings(QXmlStreamReader *xmlReader) {
    initialize();
    readSolveSettings(xmlReader);
  }


  BundleObservationSolveSettings::BundleObservationSolveSettings(const PvlGroup &scParameterGroup) {
    initialize();

    // group name must be instrument id
    m_instrumentId = QString::fromStdString(scParameterGroup.nameKeyword());

    // If CKDEGREE is not specified, then a default of 2 is used
    if (scParameterGroup.hasKeyword("CKDEGREE")) {
      m_ckDegree = (int)(scParameterGroup.findKeyword("CKDEGREE"));
    }

    // If CKSOLVEDEGREE is not specified, then a default of 2 is used
    if (scParameterGroup.hasKeyword("CKSOLVEDEGREE")) {
      m_ckSolveDegree = (int) (scParameterGroup.findKeyword("CKSOLVEDEGREE"));
    }

    // do we solve for No pointing, ANGLES only, ANGLES+ANGULAR VELOCITY, ANGLES+ANGULAR VELOCITY+
    // ANGULAR ACCELERATION, or a higher order polynomial
    QString csolve = "NONE";
    csolve = QString::fromStdString(scParameterGroup.findKeyword("CAMSOLVE"));
    csolve = csolve.toUpper();
    if (csolve == "NONE") {
      m_instrumentPointingSolveOption = NoPointingFactors;
      m_numberCamAngleCoefSolved = 0;
    }
    else if (csolve == "ANGLES") {
      m_instrumentPointingSolveOption = AnglesOnly;
      m_numberCamAngleCoefSolved = 1;
    }
    else if (csolve == "VELOCITIES") {
      m_instrumentPointingSolveOption = AnglesVelocity;
      m_numberCamAngleCoefSolved = 2;
    }
    else if (csolve == "ACCELERATIONS") {
      m_instrumentPointingSolveOption = AnglesVelocityAcceleration;
      m_numberCamAngleCoefSolved = 3;
    }
    else if (csolve == "ALL"){
      m_instrumentPointingSolveOption = AllPointingCoefficients;
      m_numberCamAngleCoefSolved = m_ckSolveDegree + 1;
    }

    // If OVEREXISTING is not specified, then a default of NO is used
    if (scParameterGroup.hasKeyword("OVEREXISTING")) {
      QString parval = QString::fromStdString(scParameterGroup.findKeyword("OVEREXISTING"));
      parval = parval.toUpper();
      if (parval == "TRUE" || parval == "YES") {
        m_solvePointingPolynomialOverExisting = true;
        m_pointingInterpolationType = SpiceRotation::PolyFunctionOverSpice;
      }
      else if (parval == "FALSE" || parval == "NO") {
        m_solvePointingPolynomialOverExisting = false;
        m_pointingInterpolationType = SpiceRotation::PolyFunction;
      }
      else {
        std::string msg = "The OVEREXISTING parameter must be set to TRUE or FALSE; YES or NO";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    // If SPKDEGREE is not specified, then a default of 2 is used
    if (scParameterGroup.hasKeyword("SPKDEGREE"))
      m_spkDegree = (int)(scParameterGroup.findKeyword("SPKDEGREE"));

    // If SPKSOLVEDEGREE is not specified, then a default of 2 is used
    if (scParameterGroup.hasKeyword("SPKSOLVEDEGREE"))
      m_spkSolveDegree = (int)(scParameterGroup.findKeyword("SPKSOLVEDEGREE"));

    // do we solve for No position, POSITION only, POSITION+VELOCITY, POSITION+VELOCITY+
    // ACCELERATION, or a higher order polynomial
    QString ssolve = "NONE";
    ssolve = QString::fromStdString(scParameterGroup.findKeyword("SPSOLVE"));
    ssolve = ssolve.toUpper();
    if (ssolve == "NONE") {
      m_instrumentPositionSolveOption = NoPositionFactors;
      m_numberCamPosCoefSolved = 0;
    }
    else if (ssolve == "POSITION") {
      m_instrumentPositionSolveOption = PositionOnly;
      m_numberCamPosCoefSolved = 1;
    }
    else if (ssolve == "VELOCITIES") {
      m_instrumentPositionSolveOption = PositionVelocity;
      m_numberCamPosCoefSolved = 2;
    }
    else if (ssolve == "ACCELERATIONS") {
      m_instrumentPositionSolveOption = PositionVelocityAcceleration;
      m_numberCamPosCoefSolved = 3;
    }
    else if (csolve == "ALL"){
      m_instrumentPositionSolveOption = AllPositionCoefficients;
      m_numberCamPosCoefSolved = m_spkSolveDegree + 1;
    }

    // If TWIST is not specified, then a default of YES is used
    if (scParameterGroup.hasKeyword("TWIST")) {
      QString parval = QString::fromStdString(scParameterGroup.findKeyword("TWIST"));
      parval = parval.toUpper();
      if (parval == "TRUE" || parval == "YES") { // is this necessary ??? i think pvl and toString can handle it...
        m_solveTwist = true;
      }
      else if (parval == "FALSE" || parval == "NO") {
        m_solveTwist = false;
      }
      else {
        std::string msg = "The TWIST parameter must be set to TRUE or FALSE; YES or NO";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    // If OVERHERMITE is not specified, then a default of NO is used
    if (scParameterGroup.hasKeyword("OVERHERMITE")) {
     QString parval = QString::fromStdString(scParameterGroup.findKeyword("OVERHERMITE"));
     parval = parval.toUpper();
     if (parval == "TRUE" || parval == "YES") {
       m_solvePositionOverHermiteSpline = true;
       m_positionInterpolationType = SpicePosition::PolyFunctionOverHermiteConstant;
     }
     else if (parval == "FALSE" || parval == "NO") {
       m_solvePositionOverHermiteSpline = false;
       m_positionInterpolationType = SpicePosition::PolyFunction;
     }
     else {
       std::string msg = "The OVERHERMITE parameter must be set to TRUE or FALSE; YES or NO";
       throw IException(IException::User, msg, _FILEINFO_);
     }
    }

    if (m_instrumentPointingSolveOption != NoPointingFactors) {
      if (scParameterGroup.hasKeyword("CAMERA_ANGLES_SIGMA")) {
        m_anglesAprioriSigma.append((double)(scParameterGroup.findKeyword("CAMERA_ANGLES_SIGMA")));
      }
      else {
        m_anglesAprioriSigma.append(Isis::Null);
      }
      if (scParameterGroup.hasKeyword("CAMERA_ANGULAR_VELOCITY_SIGMA")){
        m_anglesAprioriSigma.append(
           (double)(scParameterGroup.findKeyword("CAMERA_ANGULAR_VELOCITY_SIGMA")));
      }
      else {
        m_anglesAprioriSigma.append(Isis::Null);
      }
      if (scParameterGroup.hasKeyword("CAMERA_ANGULAR_ACCELERATION_SIGMA")){
        m_anglesAprioriSigma.append(
           (double)(scParameterGroup.findKeyword("CAMERA_ANGULAR_ACCELERATION_SIGMA")));
      }
      else {
        m_anglesAprioriSigma.append(Isis::Null);
      }
      if (scParameterGroup.hasKeyword("ADDITIONAL_CAMERA_POINTING_SIGMAS")) {
        PvlKeyword additionalSigmas = scParameterGroup.findKeyword("ADDITIONAL_CAMERA_POINTING_SIGMAS");
        for (int i = 0; i < additionalSigmas.size(); i++ ) {
          m_anglesAprioriSigma.append(Isis::toDouble(additionalSigmas[i]));
        }
      }
    }

    if (m_instrumentPositionSolveOption != NoPositionFactors) {
      if (scParameterGroup.hasKeyword("SPACECRAFT_POSITION_SIGMA")) {
        m_positionAprioriSigma.append((double)(scParameterGroup.findKeyword("SPACECRAFT_POSITION_SIGMA")));
      }
      else {
        m_positionAprioriSigma.append(Isis::Null);
      }
      if (scParameterGroup.hasKeyword("SPACECRAFT_VELOCITY_SIGMA")){
        m_positionAprioriSigma.append((double)(scParameterGroup.findKeyword("SPACECRAFT_VELOCITY_SIGMA")));
      }
      else {
        m_positionAprioriSigma.append(Isis::Null);
      }
      if (scParameterGroup.hasKeyword("SPACECRAFT_ACCELERATION_SIGMA")) {
        m_positionAprioriSigma.append((double)(scParameterGroup.findKeyword("SPACECRAFT_ACCELERATION_SIGMA")));
      }
      else {
        m_positionAprioriSigma.append(Isis::Null);
      }
      if (scParameterGroup.hasKeyword("ADDITIONAL_SPACECRAFT_POSITION_SIGMAS")) {
        PvlKeyword additionalSigmas = scParameterGroup.findKeyword("ADDITIONAL_SPACECRAFT_POSITION_SIGMAS");
        for (int i = 0; i < additionalSigmas.size(); i++ ) {
          m_positionAprioriSigma.append(Isis::toDouble(additionalSigmas[i]));
        }
      }
    }

    // CSM settings
    if (scParameterGroup.hasKeyword("CSMSOLVESET")) {
      setCSMSolveSet(stringToCSMSolveSet(QString::fromStdString(scParameterGroup.findKeyword("CSMSOLVESET"))));
    }
    else if (scParameterGroup.hasKeyword("CSMSOLVETYPE")) {
      setCSMSolveType(stringToCSMSolveType(QString::fromStdString(scParameterGroup.findKeyword("CSMSOLVETYPE"))));
    }
    else if (scParameterGroup.hasKeyword("CSMSOLVELIST")) {
      PvlKeyword csmSolveListKey = scParameterGroup.findKeyword("CSMSOLVELIST");
      QStringList csmSolveList;
      for (int i = 0; i < csmSolveListKey.size(); i++) {
        csmSolveList.append(QString::fromStdString(csmSolveListKey[i]));
      }
      setCSMSolveParameterList(csmSolveList);
    }
  }


  /**
   * Constructs a BundleObservationSolveSettings from another one.
   *
   * @param other The BundleObservationSolveSettings to copy
   */
  BundleObservationSolveSettings::BundleObservationSolveSettings(const BundleObservationSolveSettings &other) {

    m_id = NULL;
    m_id = new QUuid(other.m_id->toString());
     // TODO: add check to all copy constructors (verify other.xxx is not null) and operator= ???
     // or intit all variables in all constructors

    m_instrumentId = other.m_instrumentId;
    m_csmSolveOption = other.m_csmSolveOption;
    m_csmSolveSet = other.m_csmSolveSet;
    m_csmSolveType = other.m_csmSolveType;
    m_csmSolveList = other.m_csmSolveList;
    m_instrumentPointingSolveOption = other.m_instrumentPointingSolveOption;
    m_observationNumbers = other.m_observationNumbers;
    m_numberCamAngleCoefSolved = other.m_numberCamAngleCoefSolved;
    m_ckDegree = other.m_ckDegree;
    m_ckSolveDegree = other.m_ckSolveDegree;
    m_solveTwist = other.m_solveTwist;
    m_solvePointingPolynomialOverExisting = other.m_solvePointingPolynomialOverExisting;
    m_anglesAprioriSigma = other.m_anglesAprioriSigma;
    m_pointingInterpolationType = other.m_pointingInterpolationType;
    m_instrumentPositionSolveOption = other.m_instrumentPositionSolveOption;
    m_numberCamPosCoefSolved = other.m_numberCamPosCoefSolved;
    m_spkDegree = other.m_spkDegree;
    m_spkSolveDegree = other.m_spkSolveDegree;
    m_solvePositionOverHermiteSpline = other.m_solvePositionOverHermiteSpline;
    m_positionAprioriSigma = other.m_positionAprioriSigma;
    m_positionInterpolationType = other.m_positionInterpolationType;
  }


  /**
   * Destructor.
   */
  BundleObservationSolveSettings::~BundleObservationSolveSettings() {

    delete m_id;
    m_id = NULL;

  }


  /**
   * Assigns the state of another BundleObservationSolveSettings to this one.
   *
   * @param other The other BundleObservationSolveSettings to assign state from
   *
   * @internal
   *   @todo Check this (assignment operator)
   */
  BundleObservationSolveSettings
      &BundleObservationSolveSettings::operator=(const BundleObservationSolveSettings &other) {
    if (&other != this) {
      delete m_id;
      m_id = NULL;
      m_id = new QUuid(other.m_id->toString());

      m_instrumentId = other.m_instrumentId;
      m_observationNumbers = other.m_observationNumbers;

      // CSM related
      m_csmSolveOption = other.m_csmSolveOption;
      m_csmSolveSet = other.m_csmSolveSet;
      m_csmSolveType = other.m_csmSolveType;
      m_csmSolveList = other.m_csmSolveList;

      // pointing related
      m_instrumentPointingSolveOption = other.m_instrumentPointingSolveOption;
      m_numberCamAngleCoefSolved = other.m_numberCamAngleCoefSolved;
      m_ckDegree = other.m_ckDegree;
      m_ckSolveDegree = other.m_ckSolveDegree;
      m_solveTwist = other.m_solveTwist;
      m_solvePointingPolynomialOverExisting = other.m_solvePointingPolynomialOverExisting;
      m_pointingInterpolationType = other.m_pointingInterpolationType;
      m_anglesAprioriSigma = other.m_anglesAprioriSigma;

      // position related
      m_instrumentPositionSolveOption = other.m_instrumentPositionSolveOption;
      m_numberCamPosCoefSolved = other.m_numberCamPosCoefSolved;
      m_spkDegree = other.m_spkDegree;
      m_spkSolveDegree = other.m_spkSolveDegree;
      m_solvePositionOverHermiteSpline = other.m_solvePositionOverHermiteSpline;
      m_positionInterpolationType = other.m_positionInterpolationType;
      m_positionAprioriSigma = other.m_positionAprioriSigma;

    }

    return *this;

  }


  /**
   * Initializes the default state of this BundleObservationSolveSettings.
   */
  void BundleObservationSolveSettings::initialize() {
    m_id = NULL;
    m_id = new QUuid(QUuid::createUuid());

    m_instrumentId = "";

    // CSM solve options
    m_csmSolveOption = BundleObservationSolveSettings::NoCSMParameters;
    m_csmSolveSet = csm::param::ADJUSTABLE;
    m_csmSolveType = csm::param::REAL;
    m_csmSolveList = QStringList();

    // Camera Pointing Options
    // Defaults:
    //     m_instrumentPointingSolveOption = AnglesOnly;
    //     m_numberCamAngleCoefSolved = 1; // AnglesOnly;
    //     m_ckDegree = 2;
    //     m_ckSolveDegree = 2;
    //     m_solveTwist = true;
    //     m_solvePointingPolynomialOverExisting = false;
    //     m_pointingInterpolationType = SpiceRotation::PolyFunction;
    //     m_anglesAprioriSigma.append(Isis::Null); // num cam angle
    //     coef = 1
    setInstrumentPointingSettings(AnglesOnly, true, 2, 2, false);

    // Spacecraft Position Options
    // Defaults:
    //     m_instrumentPositionSolveOption = NoPositionFactors;
    //     m_numberCamPosCoefSolved = 0; // NoPositionFactors;
    //     m_spkDegree = 2;
    //     m_spkSolveDegree = 2;
    //     m_solvePositionOverHermiteSpline = false;
    //     m_positionInterpolationType = SpicePosition::PolyFunction;
    //     m_positionAprioriSigma.clear();
    setInstrumentPositionSettings(NoPositionFactors, 2, 2, false);

  }


  // =============================================================================================//
  // =============================================================================================//
  // =============================================================================================//

  /**
   * Sets the instrument id for this observation.
   *
   * @param instrumentId QString instrument id
   */
  void BundleObservationSolveSettings::setInstrumentId(QString instrumentId) {
    m_instrumentId = instrumentId;
  }


  /**
   * Accesses the instrument id for this observation.
   *
   * @return @b QString Returns the instrument id for this observation
   */
  QString BundleObservationSolveSettings::instrumentId() const {
    return m_instrumentId;
  }


  /**
   * Associates an observation number with these solve settings.
   *
   * These solve settings are to be applied to any associated observations.
   *
   * @param observationNumber QString observation number to associate with these settings.
   */
  void BundleObservationSolveSettings::addObservationNumber(QString observationNumber) {
    m_observationNumbers.insert(observationNumber);
  }


  /**
   * Removes an observation number from this solve settings. The observation is no longer
   * associated with this solve settings.
   *
   * @param QString observationNumber The observation number to remove from this solve settings.
   * @return bool Returns true if the observation number passed was actually removed; otherwise
   *              returns false.
   */
  bool BundleObservationSolveSettings::removeObservationNumber(QString observationNumber) {
    return m_observationNumbers.remove(observationNumber);
  }


  /**
   * Returns a list of observation numbers associated with these solve settings.
   *
   * @return @b QSet<QString> Returns a QSet containing the associated observation numbers.
   */
  QSet<QString> BundleObservationSolveSettings::observationNumbers() const {
    return m_observationNumbers;
  }


  // =============================================================================================//
  // ======================== CSM Options ========================================================//
  // =============================================================================================//


  /**
   * Convert a string to a CSM solve option enumeration value.
   *
   * @param option The option as a string
   *
   * @return @b CSMSolveOption The option's enumeration value
   */
  BundleObservationSolveSettings::CSMSolveOption
      BundleObservationSolveSettings::stringToCSMSolveOption(QString option) {
    if (option.compare("NoCSMParameters", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::NoCSMParameters;
    }
    else if (option.compare("Set", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::Set;
    }
    else if (option.compare("Type", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::Type;
    }
    else if (option.compare("List", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::List;
    }
    else {
      throw IException(IException::Unknown,
                       "Unknown bundle CSM solve option " + option.toStdString() + ".",
                       _FILEINFO_);
    }
  }


  /**
   * Convert a CSM solve option enumeration value to a string.
   *
   * @param option The option's enumeration value
   *
   * @return @b QString The option as a string
   */
  QString BundleObservationSolveSettings::csmSolveOptionToString(CSMSolveOption option) {
    if (option == BundleObservationSolveSettings::NoCSMParameters) {
      return "NoCSMParameters";
    }
    else if (option == BundleObservationSolveSettings::Set)  {
      return "Set";
    }
    else if (option == BundleObservationSolveSettings::Type) {
      return "Type";
    }
    else if (option == BundleObservationSolveSettings::List) {
      return "List";
    }
    else {
      throw IException(IException::Programmer,
                       "Unknown CSM solve option enum [" + toString(option) + "].",
                       _FILEINFO_);
    }
  }


  /**
   * Convert a string to its CSM parameter set enumeration value.
   *
   * @param set The set name
   *
   * @return @b csm::param::Set The set's enumeration value
   */
  csm::param::Set BundleObservationSolveSettings::stringToCSMSolveSet(QString set) {
    if (set.compare("VALID", Qt::CaseInsensitive) == 0) {
      return csm::param::VALID;
    }
    else if (set.compare("ADJUSTABLE", Qt::CaseInsensitive) == 0) {
      return csm::param::ADJUSTABLE;
    }
    else if (set.compare("NON_ADJUSTABLE", Qt::CaseInsensitive) == 0) {
      return csm::param::NON_ADJUSTABLE;
    }
    else {
      throw IException(IException::Unknown,
                       "Unknown bundle CSM parameter set " + set.toStdString() + ".",
                       _FILEINFO_);
    }
  }


  /**
   * Convert a CSM parameter set enumeration value to a string.
   *
   * @param set The set's enumeration value
   *
   * @return @b QString The set's name
   */
  QString BundleObservationSolveSettings::csmSolveSetToString(csm::param::Set set) {
    if (set == csm::param::VALID) {
      return "VALID";
    }
    else if (set == csm::param::ADJUSTABLE)  {
      return "ADJUSTABLE";
    }
    else if (set == csm::param::NON_ADJUSTABLE) {
      return "NON_ADJUSTABLE";
    }
    else {
      throw IException(IException::Programmer,
                       "Unknown CSM parameter set enum [" + toString(set) + "].",
                       _FILEINFO_);
    }
  }


  /**
   * Convert a string to its CSM parameter type enumeration value.
   *
   * @param type The type name
   *
   * @return @b csm::param::Type The types's enumeration value
   */
  csm::param::Type BundleObservationSolveSettings::stringToCSMSolveType(QString type) {
    if (type.compare("NONE", Qt::CaseInsensitive) == 0) {
      return csm::param::NONE;
    }
    else if (type.compare("FICTITIOUS", Qt::CaseInsensitive) == 0) {
      return csm::param::FICTITIOUS;
    }
    else if (type.compare("REAL", Qt::CaseInsensitive) == 0) {
      return csm::param::REAL;
    }
    else if (type.compare("FIXED", Qt::CaseInsensitive) == 0) {
      return csm::param::FIXED;
    }
    else {
      throw IException(IException::Unknown,
                       "Unknown bundle CSM parameter type " + type.toStdString() + ".",
                       _FILEINFO_);
    }
  }


  /**
   * Convert a CSM parameter type enumeration value to a string.
   *
   * @param type The type's enumeration value
   *
   * @return @b QString The type's name
   */
  QString BundleObservationSolveSettings::csmSolveTypeToString(csm::param::Type type) {
    if (type == csm::param::NONE) {
      return "NONE";
    }
    else if (type == csm::param::FICTITIOUS)  {
      return "FICTITIOUS";
    }
    else if (type == csm::param::REAL) {
      return "REAL";
    }
    else if (type == csm::param::FIXED) {
      return "FIXED";
    }
    else {
      throw IException(IException::Programmer,
                       "Unknown CSM parameter type enum [" + toString(type) + "].",
                       _FILEINFO_);
    }
  }


  /**
   * Set the set of CSM parameters to solve for. See the CSM API documentation
   * for what the different set values mean.
   *
   * @param set The set to solve for
   */
  void BundleObservationSolveSettings::setCSMSolveSet(csm::param::Set set) {
    m_csmSolveOption = BundleObservationSolveSettings::Set;
    m_csmSolveSet = set;
  }


  /**
   * Set the type of CSM parameters to solve for.
   *
   * @param type The parameter type to solve for
   */
  void BundleObservationSolveSettings::setCSMSolveType(csm::param::Type type) {
    m_csmSolveOption = BundleObservationSolveSettings::Type;
    m_csmSolveType = type;
  }


  /**
   * Set an explicit list of CSM parameters to solve for.
   *
   * @param list The names of the parameters to solve for
   */
  void BundleObservationSolveSettings::setCSMSolveParameterList(QStringList list) {
    m_csmSolveOption = BundleObservationSolveSettings::List;
    m_csmSolveList = list;
  }


  /**
   * Get how the CSM parameters to solve for are specified for this observation.
   *
   * @return @b CSMSolveOption
   */
  BundleObservationSolveSettings::CSMSolveOption
      BundleObservationSolveSettings::csmSolveOption() const {
    return m_csmSolveOption;
  }


  /**
   * Get the set of CSM parameters to solve for
   *
   * @return @b csm::param::Set The CSM parameter set to solve for
   */
  csm::param::Set BundleObservationSolveSettings::csmParameterSet() const {
    return m_csmSolveSet;
  }


  /**
   * Get the type of CSM parameters to solve for
   *
   * @return @b csm::param::Type The CSM parameter type to solve for
   */
  csm::param::Type BundleObservationSolveSettings::csmParameterType() const {
    return m_csmSolveType;
  }


  /**
   * Get the list of CSM parameters to solve for
   *
   * @return @b QStringList The names of the CSM parameters to solve for
   */
  QStringList BundleObservationSolveSettings::csmParameterList() const {
    return m_csmSolveList;
  }


  // =============================================================================================//
  // ======================== Camera Pointing Options ============================================//
  // =============================================================================================//


  /**
   * Translates a QString InstrumentPointingSolveOption to its enumerated value.
   *
   * @param option QString representation of the instrument pointing solve option
   *
   * @throws IException::Unknown "Unknown bundle instrument point solve option."
   *
   * @return @b BundleObservationSolveSettings::InstrumentPointingSolveOption Returns the enumerated
   *     value of the instrument pointing solve option
   */
  BundleObservationSolveSettings::InstrumentPointingSolveOption
      BundleObservationSolveSettings::stringToInstrumentPointingSolveOption(QString option) {
    if (option.compare("NONE", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::NoPointingFactors;
    }
    else if (option.compare("NoPointingFactors", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::NoPointingFactors;
    }
    else if (option.compare("ANGLES", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::AnglesOnly;
    }
    else if (option.compare("AnglesOnly", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::AnglesOnly;
    }
    else if (option.compare("VELOCITIES", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::AnglesVelocity;
    }
    else if (option.compare("AnglesAndVelocity", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::AnglesVelocity;
    }
    else if (option.compare("ACCELERATIONS", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::AnglesVelocityAcceleration;
    }
    else if (option.compare("AnglesVelocityAndAcceleration", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::AnglesVelocityAcceleration;
    }
    else if (option.compare("ALL", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::AllPointingCoefficients;
    }
    else if (option.compare("AllPolynomialCoefficients", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::AllPointingCoefficients;
    }
    else {
      throw IException(IException::Unknown,
                       "Unknown bundle instrument pointing solve option " + option.toStdString() + ".",
                       _FILEINFO_);
    }
  }


  /**
   * Tranlsates an enumerated InstrumentPointingSolveOption value to its string representation.
   *
   * @param option Enumerated InstrumentPointingSolveOption value
   *
   * @throws IException::Programmer "Unknown pointing solve option enum."
   *
   * @return @b QString Returns the QString representation of the passed
   *                    InstrumentPointingSolveOption
   */
  QString BundleObservationSolveSettings::instrumentPointingSolveOptionToString(
      InstrumentPointingSolveOption option) {
    if (option == NoPointingFactors)               return "None";
    else if (option == AnglesOnly)                 return "AnglesOnly";
    else if (option == AnglesVelocity)             return "AnglesAndVelocity";
    else if (option == AnglesVelocityAcceleration) return "AnglesVelocityAndAcceleration";
    else if (option == AllPointingCoefficients)    return "AllPolynomialCoefficients";
    else throw IException(IException::Programmer,
                          "Unknown pointing solve option enum [" + toString(option) + "].",
                          _FILEINFO_);
  }


  /**
   * Sets the instrument pointing settings.
   *
   * @param option Option for how to solve for instrument pointing
   * @param solveTwist Whether or not to solve for twist
   * @param ckDegree
   * @param ckSolveDegree
   * @param solvePolynomialOverExisting Indicates whether the polynomial will be fit over an
   *                                    existing pointing polynomial
   * @param anglesAprioriSigma A priori angle values
   * @param angularVelocityAprioriSigma A priori angular velocity
   * @param angularAccelerationAprioriSigma A priori angular acceleration
   */
  void BundleObservationSolveSettings::setInstrumentPointingSettings(
                                           InstrumentPointingSolveOption option,
                                           bool solveTwist,
                                           int ckDegree,
                                           int ckSolveDegree,
                                           bool solvePolynomialOverExisting,
                                           double anglesAprioriSigma,
                                           double angularVelocityAprioriSigma,
                                           double angularAccelerationAprioriSigma,QList<double> * additionalPointingSigmas) {

    // automatically set the solve option and ck degree to the user entered values
    m_instrumentPointingSolveOption = option;

    // the ck solve degree entered is only used if we are solving for all coefficients
    // otherwise it defaults to 2.
    if (option == AllPointingCoefficients) {
      // update spkDegree and spkSolveDegree
      m_ckDegree = ckDegree;
      m_ckSolveDegree = ckSolveDegree;

      // we are solving for (solve degree + 1) coefficients
      // this is the maximum number of apriori sigmas allowed
      m_numberCamAngleCoefSolved = m_ckSolveDegree + 1;
    }
    else {
      // let spkDegree and spkSolveDegree default to 2, 2
      m_ckDegree = 2;
      m_ckSolveDegree = 2;

      // solve for the appropriate number of coefficients, based on solve option enum
      m_numberCamAngleCoefSolved = ((int) option);
    }

    m_anglesAprioriSigma.clear();
    if (m_numberCamAngleCoefSolved > 0) {
      if (anglesAprioriSigma > 0.0) {
        m_anglesAprioriSigma.append(anglesAprioriSigma);
      }
      else {
        m_anglesAprioriSigma.append(Isis::Null);
      }

      if (m_numberCamAngleCoefSolved > 1) {
        if (angularVelocityAprioriSigma > 0.0) {
          m_anglesAprioriSigma.append(angularVelocityAprioriSigma);
        }
        else {
          m_anglesAprioriSigma.append(Isis::Null);
        }

        if (m_numberCamAngleCoefSolved > 2) {
          if (angularAccelerationAprioriSigma > 0.0) {
            m_anglesAprioriSigma.append(angularAccelerationAprioriSigma);
          }
          else {
            m_anglesAprioriSigma.append(Isis::Null);
          }
        }
      }
    }

    if (additionalPointingSigmas) {
      for (int i=0;i < additionalPointingSigmas->count();i++) {
          m_anglesAprioriSigma.append(additionalPointingSigmas->value(i));
      }
    }





    m_solveTwist = solveTwist; // dependent on solve option???

    // Set the SpiceRotation interpolation type enum appropriately
    m_solvePointingPolynomialOverExisting = solvePolynomialOverExisting;
    if (m_solvePointingPolynomialOverExisting) {
      m_pointingInterpolationType = SpiceRotation::PolyFunctionOverSpice;
    }
    else {
      m_pointingInterpolationType = SpiceRotation::PolyFunction;
    }

  }


  /**
   * Accesses the instrument pointing solve option.
   *
   * @return @b BundleObservationSolveSettings::InstrumentPointingSolveOption Returns the
   *     instrument pointing solve option
   */
  BundleObservationSolveSettings::InstrumentPointingSolveOption
      BundleObservationSolveSettings::instrumentPointingSolveOption() const {
    return m_instrumentPointingSolveOption;
  }


  /**
   * Accesses the flag for solving for twist.
   *
   * @return @b bool Returns whether or not to solve for twist
   */
  bool BundleObservationSolveSettings::solveTwist() const {
    return m_solveTwist;
  }


  /**
   * Accesses the degree of polynomial fit to original camera angles (ckDegree).
   *
   * @return @b int Returns the degree of the polynomial fit to the original camera angles
   */
  int BundleObservationSolveSettings::ckDegree() const {
    return m_ckDegree;
  }


  /**
   * Accesses the degree of the camera angles polynomial being fit to in the bundle adjustment
   * (ckSolveDegree).
   *
   * @return @b int Returns the degree of the camera angles polynomial in the bundle adjustment
   */
  int BundleObservationSolveSettings::ckSolveDegree()const {
    return m_ckSolveDegree;
  }


  /**
   * Accesses the number of camera angle coefficients in the solution.
   *
   * @return @b int Returns the number of camera angle coefficients in the solution
   */
  int BundleObservationSolveSettings::numberCameraAngleCoefficientsSolved() const {
    return m_numberCamAngleCoefSolved;
  }


  /**
   * Whether or not the solve polynomial will be fit over the existing pointing polynomial.
   *
   * @return @b bool Indicates whether the polynomial will be fit over the existing pointing
   *                 polynomial
   */
  bool BundleObservationSolveSettings::solvePolyOverPointing() const {
    return m_solvePointingPolynomialOverExisting;
  }


  /**
   * Accesses the a priori pointing sigmas.
   *
   * @return @b QList<double> Returns a QList of the a priori pointing sigmas
   */
  QList<double> BundleObservationSolveSettings::aprioriPointingSigmas() const {
    return m_anglesAprioriSigma;
  }


  /**
   * Accesses the SpiceRotation interpolation type for the instrument pointing.
   *
   * @return @b SpiceRotation::Source Returns the SpiceRotation interpolation type for pointing
   */
  SpiceRotation::Source BundleObservationSolveSettings::pointingInterpolationType() const {
    return m_pointingInterpolationType;
  }


  // =============================================================================================//
  // ======================== Spacecraft Position Options ========================================//
  // =============================================================================================//


  /**
   * Translates a QString InstrumentPositionSolveOption to its enumerated value.
   *
   * @param option QString representation of an instrument position solve option
   *
   * @throws IExeption::Unknown "Unknown bundle instrument position solve option."
   *
   * @return @b BundleObservationSolveSettings::InstrumentPositionSolveOption Returns the enumerated
   *     value of the instrument position solve option
   */
  BundleObservationSolveSettings::InstrumentPositionSolveOption
      BundleObservationSolveSettings::stringToInstrumentPositionSolveOption(QString option) {
    if (option.compare("NONE", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::NoPositionFactors;
    }
    else if (option.compare("NoPositionFactors", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::NoPositionFactors;
    }
    else if (option.compare("POSITIONS", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::PositionOnly;
    }
    else if (option.compare("PositionOnly", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::PositionOnly;
    }
    else if (option.compare("VELOCITIES", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::PositionVelocity;
    }
    else if (option.compare("PositionAndVelocity", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::PositionVelocity;
    }
    else if (option.compare("ACCELERATIONS", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::PositionVelocityAcceleration;
    }
    else if (option.compare("PositionVelocityAndAcceleration", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::PositionVelocityAcceleration;
    }
    else if (option.compare("ALL", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::AllPositionCoefficients;
    }
    else if (option.compare("AllPolynomialCoefficients", Qt::CaseInsensitive) == 0) {
      return BundleObservationSolveSettings::AllPositionCoefficients;
    }
    else {
      throw IException(IException::Unknown,
                          "Unknown bundle instrument position solve option " + option.toStdString() + ".",
                          _FILEINFO_);
    }
  }


  /**
   * Translates an enumerated InstrumentPositionSolveOption to its string representation.
   *
   * @param option Enumerated InstrumentPositionSolveOption value
   *
   * @throws IException::Programmer "Unknown position solve option enum."
   *
   * @return @b QString Returns the QString representation of the passed
   *                    InstrumentPointingSolveOption
   */
  QString BundleObservationSolveSettings::instrumentPositionSolveOptionToString(
      InstrumentPositionSolveOption option) {
    if (option == NoPositionFactors)                 return "None";
    else if (option == PositionOnly)                 return "PositionOnly";
    else if (option == PositionVelocity)             return "PositionAndVelocity";
    else if (option == PositionVelocityAcceleration) return "PositionVelocityAndAcceleration";
    else if (option == AllPositionCoefficients)      return "AllPolynomialCoefficients";
    else throw IException(IException::Programmer,
                          "Unknown position solve option enum [" + toString(option) + "].",
                          _FILEINFO_);
  }


  /**
   * Sets the instrument pointing settings.
   *
   * @param option Option for how to solve the instrument position
   * @param spkDegree
   * @param spkSolveDegree
   * @param positionOverHermite Whether or not the polynomial will be fit over an existing Hermite
   *                            spline
   * @param positionAprioriSigma A priori position sigma
   * @param velocityAprioriSigma A priori velocity sigma
   * @param accelerationAprioriSigma A priori acceleration sigma
   */
  void BundleObservationSolveSettings::setInstrumentPositionSettings(InstrumentPositionSolveOption option,
                                           int spkDegree,
                                           int spkSolveDegree,
                                           bool positionOverHermite,
                                           double positionAprioriSigma,
                                           double velocityAprioriSigma,
                                           double accelerationAprioriSigma,
                                           QList<double> *additionalPositionSigmas) {
    // automatically set the solve option and spk degree to the user entered values
    m_instrumentPositionSolveOption = option;

    // the spk solve degree entered is only used if we are solving for all coefficients
    // otherwise it defaults to 2.
    if (option == AllPositionCoefficients) {
      // update spkDegree and spkSolveDegree
      m_spkDegree = spkDegree;
      m_spkSolveDegree = spkSolveDegree;
      // we are solving for (solve degree + 1) coefficients
      // this is the maximum number of apriori sigmas allowed
      m_numberCamPosCoefSolved = m_spkSolveDegree + 1;
    }
    else {
      // let spkDegree and spkSolveDegree default to 2, 2
      m_spkDegree = 2;
      m_spkSolveDegree = 2;

      // solve for the appropriate number of coefficients, based on solve option enum
      m_numberCamPosCoefSolved = ((int) option);
    }

    m_positionAprioriSigma.clear();
    if (m_numberCamPosCoefSolved > 0) {
      if (positionAprioriSigma > 0.0) {
        m_positionAprioriSigma.append(positionAprioriSigma);
      }
      else {
        m_positionAprioriSigma.append(Isis::Null);
      }

      if (m_numberCamPosCoefSolved > 1) {
        if (velocityAprioriSigma > 0.0) {
          m_positionAprioriSigma.append(velocityAprioriSigma);
        }
        else {
          m_positionAprioriSigma.append(Isis::Null);
        }

        if (m_numberCamPosCoefSolved > 2) {
          if (accelerationAprioriSigma > 0.0) {
            m_positionAprioriSigma.append(accelerationAprioriSigma);
          }
          else {
            m_positionAprioriSigma.append(Isis::Null);
          }
        }
      }
    }

    if (additionalPositionSigmas) {
      for (int i=0;i < additionalPositionSigmas->count();i++) {
          m_positionAprioriSigma.append(additionalPositionSigmas->value(i));
      }
    }

    // Set the SpicePosition interpolation type enum appropriately
    m_solvePositionOverHermiteSpline = positionOverHermite;
    if (m_solvePositionOverHermiteSpline) {
      m_positionInterpolationType = SpicePosition::PolyFunctionOverHermiteConstant;
    }
    else {
      m_positionInterpolationType = SpicePosition::PolyFunction;
    }

  }


  /**
   * Accesses the instrument position solve option.
   *
   * @return @b BundleObservationSolveSettings::InstrumentPositionSolveOption Returns the
   *     instrument position solve option
   */
  BundleObservationSolveSettings::InstrumentPositionSolveOption
      BundleObservationSolveSettings::instrumentPositionSolveOption() const {
    return m_instrumentPositionSolveOption;
  }


  /**
   * Accesses the degree of the polynomial fit to the original camera position (spkDegree).
   *
   * @return @b int Returns the degree of the original camera position polynomial.
   */
  int BundleObservationSolveSettings::spkDegree() const {
    return m_spkDegree;
  }


  /**
   * Accesses the degree of thecamera position polynomial being fit to in the bundle adjustment
   * (spkSolveDegree).
   *
   * @return @b int Returns the degree of the camera position polynomial in the bundle adjustment.
   */
  int BundleObservationSolveSettings::spkSolveDegree() const {
    return m_spkSolveDegree;
  }


  /**
   * Accesses the number of camera position coefficients in the solution.
   *
   * @return @b int Returns the number of camera position coefficients.
   */
  int BundleObservationSolveSettings::numberCameraPositionCoefficientsSolved() const {
    return m_numberCamPosCoefSolved;
  }


  /**
   * Whether or not the polynomial for solving will be fit over an existing Hermite spline
   *
   * @return @b bool Returns whether or not to fit the solve polynomial over an existing Hermite
   *                 spline
   */
  bool BundleObservationSolveSettings::solvePositionOverHermite() const {
    return m_solvePositionOverHermiteSpline;
  }


  /**
   * Accesses the a priori position sigmas.
   *
   * @return @b QList<double> Returns a QList of the a priori position sigmas
   */
  QList<double> BundleObservationSolveSettings::aprioriPositionSigmas() const {
    return m_positionAprioriSigma;
  }


  /**
   * Accesses the SpicePosition interpolation type for the spacecraft position
   *
   * @return @b SpicePosition::Source Returns the SpicePositon interpolation type for position
   */
  SpicePosition::Source BundleObservationSolveSettings::positionInterpolationType() const {
    return m_positionInterpolationType;
  }


  // =============================================================================================//
  // =============================================================================================//
  // =============================================================================================//


  /**
   * Saves this BundleObservationSolveSettings to an xml stream.
   *
   * @param stream A QXmlStreamWriter to write to
   * @param project Pointer to the current project
   *
   * @internal
   *   @todo Does xml stuff need project???
   */
  void BundleObservationSolveSettings::save(QXmlStreamWriter &stream,
                                            const Project *project) const {

    stream.writeStartElement("bundleObservationSolveSettings");
    stream.writeTextElement("id", m_id->toString());
    stream.writeTextElement("instrumentId", instrumentId());

    // pointing related
    stream.writeStartElement("instrumentPointingOptions");
    stream.writeAttribute("solveOption",
                           instrumentPointingSolveOptionToString(m_instrumentPointingSolveOption));
    stream.writeAttribute("numberCoefSolved", QString::number(m_numberCamAngleCoefSolved));
    stream.writeAttribute("degree", QString::number(m_ckDegree));
    stream.writeAttribute("solveDegree", QString::number(m_ckSolveDegree));
    stream.writeAttribute("solveTwist", QString::number(m_solveTwist));
    stream.writeAttribute("solveOverExisting", QString::number(m_solvePointingPolynomialOverExisting));
    stream.writeAttribute("interpolationType", QString::number(m_pointingInterpolationType));

    stream.writeStartElement("aprioriPointingSigmas");
    for (int i = 0; i < m_anglesAprioriSigma.size(); i++) {
      if (IsSpecial(m_anglesAprioriSigma[i])) {
        stream.writeTextElement("sigma", "N/A");
      }
      else {
        stream.writeTextElement("sigma", QString::number(m_anglesAprioriSigma[i]));
      }
    }
    stream.writeEndElement();// end aprioriPointingSigmas
    stream.writeEndElement();// end instrumentPointingOptions

    // position related
    stream.writeStartElement("instrumentPositionOptions");
    stream.writeAttribute("solveOption",
                           instrumentPositionSolveOptionToString(m_instrumentPositionSolveOption));
    stream.writeAttribute("numberCoefSolved", QString::number(m_numberCamPosCoefSolved));
    stream.writeAttribute("degree", QString::number(m_spkDegree));
    stream.writeAttribute("solveDegree", QString::number(m_spkSolveDegree));
    stream.writeAttribute("solveOverHermiteSpline", QString::number(m_solvePositionOverHermiteSpline));
    stream.writeAttribute("interpolationType", QString::number(m_positionInterpolationType));

    stream.writeStartElement("aprioriPositionSigmas");
    for (int i = 0; i < m_positionAprioriSigma.size(); i++) {
      if (IsSpecial(m_positionAprioriSigma[i])) {
        stream.writeTextElement("sigma", "N/A");
      }
      else {
        stream.writeTextElement("sigma", QString::number(m_positionAprioriSigma[i]));
      }
    }
    stream.writeEndElement();// end aprioriPositionSigmas
    stream.writeEndElement(); // end instrumentPositionOptions

    stream.writeEndElement(); // end bundleObservationSolveSettings

  }

  void BundleObservationSolveSettings::readSolveSettings(QXmlStreamReader *xmlReader) {
    initialize();
    Q_ASSERT(xmlReader->name() == "bundleObservationSolveSettings");
    while (xmlReader->readNextStartElement()) {
      if (xmlReader->qualifiedName() == "instrumentId") {
        QString instrumentId = xmlReader->readElementText();
        if (!instrumentId.isEmpty()){
          setInstrumentId(instrumentId);    
        }
      }
      else if (xmlReader->qualifiedName() == "instrumentPointingOptions") {
        QStringRef solveOption = xmlReader->attributes().value("solveOption");
        if (!solveOption.isEmpty()) {
          m_instrumentPointingSolveOption = stringToInstrumentPointingSolveOption(solveOption.toString());
        }
        QStringRef numberCoefSolved = xmlReader->attributes().value("numberCoefSolved");
        if (!numberCoefSolved.isEmpty()) {
          m_numberCamAngleCoefSolved = numberCoefSolved.toInt();
        }
        QStringRef degree = xmlReader->attributes().value("degree");
        if (!degree.isEmpty()) {
          m_ckDegree = degree.toInt();
        }
        QStringRef solveDegree = xmlReader->attributes().value("solveDegree");
        if (!solveDegree.isEmpty()) {
         m_ckSolveDegree = solveDegree.toInt();
        }
        QStringRef solveTwist = xmlReader->attributes().value("solveTwist");
        if (!solveTwist.isEmpty()) {
          m_solveTwist = toBool(solveTwist.toString().toStdString());
        }
        QStringRef solveOverExisting = xmlReader->attributes().value("solveOverExisting");
        if (!solveOverExisting.isEmpty()) {
          m_solvePointingPolynomialOverExisting = toBool(solveOverExisting.toString().toStdString());
        }
        QStringRef interpolationType = xmlReader->attributes().value("interpolationType");
        if (!interpolationType.isEmpty()) {
          if (interpolationType == "3") {
            m_pointingInterpolationType = SpiceRotation::PolyFunction;
          }
          else if (interpolationType == "4") {
            m_pointingInterpolationType = SpiceRotation::PolyFunctionOverSpice;
          }
        }
        while (xmlReader->readNextStartElement()) {
          if (xmlReader->qualifiedName() == "aprioriPointingSigmas") {
            m_anglesAprioriSigma.clear();
            while (xmlReader->readNextStartElement()) {
              if (xmlReader->qualifiedName() == "sigma") {
                QString sigma = xmlReader->readElementText();
                if (!sigma.isEmpty()){
                  if (sigma == "N/A") {
                    m_anglesAprioriSigma.append(Isis::Null);
                  }
                  else {
                    m_anglesAprioriSigma.append(sigma.toDouble()); 
                  } 
                }      
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
      else if (xmlReader->qualifiedName() == "instrumentPositionOptions") {
        QStringRef solveOption = xmlReader->attributes().value("solveOption");
        if (!solveOption.isEmpty()) {
          m_instrumentPositionSolveOption = stringToInstrumentPositionSolveOption(solveOption.toString());
        }
        QStringRef numberCoefSolved = xmlReader->attributes().value("numberCoefSolved");
        if (!numberCoefSolved.isEmpty()) {
          m_numberCamPosCoefSolved = numberCoefSolved.toInt();
        }
        QStringRef degree = xmlReader->attributes().value("degree");
        if (!degree.isEmpty()) {
          m_spkDegree = degree.toInt();
        }
        QStringRef solveDegree = xmlReader->attributes().value("solveDegree");
        if (!solveDegree.isEmpty()) {
         m_spkSolveDegree = solveDegree.toInt();
        }
        QStringRef solveOverHermiteSpline = xmlReader->attributes().value("solveOverHermiteSpline");
        if (!solveOverHermiteSpline.isEmpty()) {
          m_solvePositionOverHermiteSpline = toBool(solveOverHermiteSpline.toString().toStdString());
        }
        QStringRef interpolationType = xmlReader->attributes().value("interpolationType");
        if (!interpolationType.isEmpty()) {
          if (interpolationType == "3") {
            m_positionInterpolationType = SpicePosition::PolyFunction;
          }
          else if (interpolationType == "4") {
            m_positionInterpolationType = SpicePosition::PolyFunctionOverHermiteConstant;
          }
        }
        while (xmlReader->readNextStartElement()) {
          if (xmlReader->qualifiedName() == "aprioriPositionSigmas") {
            m_positionAprioriSigma.clear();
            while (xmlReader->readNextStartElement()) {
              if (xmlReader->qualifiedName() == "sigma") {
                QString sigma = xmlReader->readElementText();
                if (!sigma.isEmpty()){
                  if (sigma == "N/A") {
                    m_positionAprioriSigma.append(Isis::Null);
                    
                  }
                  else {
                    m_positionAprioriSigma.append(sigma.toDouble());
                  }  
                }
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
      else {
        xmlReader->skipCurrentElement();
      }
    }
  }
}
