/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ControlNetValidMeasure.h"
#include "Cube.h"
#include "Camera.h"
#include "ControlMeasure.h"
#include "CubeManager.h"
#include "MeasureValidationResults.h"
#include "SerialNumberList.h"
#include "SpecialPixel.h"
#include "UniversalGroundMap.h"

namespace Isis {

  /**
   * Constructor - Initializes the data members and
   * parses the input Pvl . The Pvl Def File is optional.
   *
   * @author Sharmila Prasad (5/13/2010)
   *
   * @param pPvl - Pvl DefFile
   */
  ControlNetValidMeasure::ControlNetValidMeasure(Pvl *pPvl) {
    InitStdOptions();

    if(pPvl != NULL && pPvl->hasGroup("ValidMeasure")) {
      Parse(*pPvl);
    }
    else {
      InitStdOptionsGroup();
    }
    mStatisticsGrp = PvlGroup("Statistics");
  }

  /**
   * Constructor with a reference to Pvl Def file. Used for
   * Interest Operator where Def File is a requirement
   *
   * @author Sharmila Prasad (6/8/2010)
   *
   * @param pPvl - Pvl DefFile
   */
  ControlNetValidMeasure::ControlNetValidMeasure(Pvl &pPvl) {
    InitStdOptions();

    if(pPvl.hasGroup("ValidMeasure")) {
      Parse(pPvl);
    }
    else {
      InitStdOptionsGroup();
    }
    mStatisticsGrp = PvlGroup("Statistics");
  }

  /**
   * Init all the standard options to default
   *
   * @author Sharmila Prasad (6/8/2010)
   */
  void ControlNetValidMeasure::InitStdOptions(void) {
    mdMinEmissionAngle     = 0;
    mdMaxEmissionAngle     = 135;
    mdMinIncidenceAngle    = 0;
    mdMaxIncidenceAngle    = 135;;
    miPixelsFromEdge       = 0;
    mdMinResolution        = 0;
    mdMaxResolution        = DBL_MAX;
    mdMinDN                = Isis::ValidMinimum;
    mdMaxDN                = Isis::ValidMaximum;
    miPixelsFromEdge       = 0;
    mdMetersFromEdge       = 0;
    mdSampleResTolerance   = DBL_MAX;
    mdLineResTolerance     = DBL_MAX;
    mdResidualTolerance    = DBL_MAX;
    m_sampleShiftTolerance = DBL_MAX;
    m_lineShiftTolerance   = DBL_MAX;
    m_pixelShiftTolerance  = DBL_MAX;

    mbCameraRequired       = false;
    mbValidateDN           = false;
    mbValidateFromEdge     = false;
  }

  /**
   * Set the Standard Options group for logging.
   *
   * @author Sharmila Prasad (6/8/2010)
   */
  void ControlNetValidMeasure::InitStdOptionsGroup(void) {
    mStdOptionsGrp = PvlGroup("StandardOptions");

    mStdOptionsGrp += Isis::PvlKeyword("MinDN",             (mdMinDN == Isis::ValidMinimum ? "NA" : toString(mdMinDN)));
    mStdOptionsGrp += Isis::PvlKeyword("MaxDN",             (mdMaxDN == Isis::ValidMaximum ? "NA" : toString(mdMaxDN)));

    mStdOptionsGrp += Isis::PvlKeyword("MinEmission",       toString(mdMinEmissionAngle));
    mStdOptionsGrp += Isis::PvlKeyword("MaxEmission",       toString(mdMaxEmissionAngle));
    mStdOptionsGrp += Isis::PvlKeyword("MinIncidence",      toString(mdMinIncidenceAngle));
    mStdOptionsGrp += Isis::PvlKeyword("MaxIncidence",      toString(mdMaxIncidenceAngle));
    mStdOptionsGrp += Isis::PvlKeyword("MinResolution",     toString(mdMinResolution));
    mStdOptionsGrp += Isis::PvlKeyword("MaxResolution",     (mdMaxResolution == DBL_MAX ? "NA" : toString(mdSampleResTolerance)));
    mStdOptionsGrp += Isis::PvlKeyword("PixelsFromEdge",    toString(miPixelsFromEdge));
    mStdOptionsGrp += Isis::PvlKeyword("MetersFromEdge",    toString(mdMetersFromEdge));

    mStdOptionsGrp += Isis::PvlKeyword("SampleResidual",    (mdSampleResTolerance   == DBL_MAX ? "NA" : toString(mdSampleResTolerance)));
    mStdOptionsGrp += Isis::PvlKeyword("LineResidual",      (mdLineResTolerance     == DBL_MAX ? "NA" : toString(mdLineResTolerance)));
    mStdOptionsGrp += Isis::PvlKeyword("ResidualMagnitude", (mdResidualTolerance    == DBL_MAX ? "NA" : toString(mdResidualTolerance)));

    mStdOptionsGrp += Isis::PvlKeyword("SampleShift",       (m_sampleShiftTolerance == DBL_MAX ? "NA" : toString(m_sampleShiftTolerance)));
    mStdOptionsGrp += Isis::PvlKeyword("LineShift",         (m_lineShiftTolerance   == DBL_MAX ? "NA" : toString(m_lineShiftTolerance)));
    mStdOptionsGrp += Isis::PvlKeyword("PixelShift",        (m_pixelShiftTolerance  == DBL_MAX ? "NA" : toString(m_pixelShiftTolerance)));
  }

  /**
   * Destructor: clean up stuff relevant for this class
   *
   * @author Sharmila Prasad (6/3/2010)
   */
  ControlNetValidMeasure::~ControlNetValidMeasure() {
    mCubeMgr.CleanCubes();
  }

  /**
   * Read Serial Numbers from specified file and populate the Cube and UniversalGround Maps
   * using the serial numbers
   *
   * @author Sharmila Prasad (6/3/2010)
   *
   * @param psSerialNumfile - File with list of Serial Numbers
   */
  void ControlNetValidMeasure::ReadSerialNumbers(QString psSerialNumfile) {
    mSerialNumbers = SerialNumberList(psSerialNumfile, true, &mStatus);

    mCubeMgr.SetNumOpenCubes(50);
  }

  /**
   * Virtual member that parses the common Cnet Options and
   * checks for their validity
   *
   * @author Sharmila Prasad (5/13/2010)
   *
   * @param pvlDef - Pvl DefFile
   */
  void ControlNetValidMeasure::Parse(Pvl &pvlDef) {
    mPvlOpGrp = pvlDef.findGroup("ValidMeasure", Pvl::Traverse);

    mStdOptionsGrp = PvlGroup("StandardOptions");

    ValidatePvlDN();
    ValidatePvlEmissionAngle();
    ValidatePvlIncidenceAngle();
    ValidatePvlResolution();
    ValidatePvlFromEdge();
    ValidatePvlResidualTolerances();
    ValidatePvlShiftTolerances();

    mPvlLog += mStdOptionsGrp;
  }

  /**
   * Validate a point on an image and the Control Measure if not Null
   *
   * @author Sharmila Prasad (5/17/2011)
   *
   * @param pSample      - Image Sample
   * @param pLine        - Image Line
   * @param pMeasure     - Control Measure
   * @param pCube        - Control Measure's image
   * @param pMeasureGrp  - Result PvlGroup
   *
   * @return MeasureValidationResults
   */
  MeasureValidationResults ControlNetValidMeasure::ValidStandardOptions(
      double pSample, double pLine, const ControlMeasure *pMeasure, Cube *pCube,
      PvlGroup *pMeasureGrp) {

    // Get the Camera
    Camera *measureCamera = NULL;
    if(mbCameraRequired) {
      try {
        measureCamera = pCube->camera();
      }
      catch(IException &e) {
        std::string msg = "Cannot Create Camera for Image:" + pCube->fileName().toStdString();
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    return ValidStandardOptions(pSample, pLine, pMeasure,
        pCube, measureCamera, pMeasureGrp);
  }


  MeasureValidationResults ControlNetValidMeasure::ValidStandardOptions(
      double pSample, double pLine, const ControlMeasure *pMeasure, Cube *pCube,
      Camera *measureCamera, PvlGroup *pMeasureGrp) {

    mdEmissionAngle  = Null;
    mdIncidenceAngle = Null;
    mdDnValue        = 0;
    mdResolution     = Null;
    mdSampleResidual = 0;
    mdLineResidual   = 0;
    mdResidualMagnitude=0;
    m_sampleShift = 0;
    m_lineShift = 0;
    m_pixelShift = 0;

    if (measureCamera != NULL) {
      bool success = measureCamera->SetImage(pSample, pLine);
      if (success) {
        mdEmissionAngle     = measureCamera->EmissionAngle();
        mdIncidenceAngle    = measureCamera->IncidenceAngle();
        mdResolution        = measureCamera->PixelResolution();
      }
    }

    if (pMeasure != NULL) {
      double temp = pMeasure->GetSampleResidual();
      if (temp != Null) {
        mdSampleResidual = fabs(temp);
      }

      temp = pMeasure->GetLineResidual();
      if (temp != Null) {
        mdLineResidual = fabs(temp);
      }

      temp = pMeasure->GetResidualMagnitude();
      if (temp != Null) {
        mdResidualMagnitude = fabs(temp);
      }

      temp = pMeasure->GetSampleShift();
      if (temp != Null) {
        m_sampleShift = fabs(temp);
      }

      temp = pMeasure->GetLineShift();
      if (temp != Null) {
        m_lineShift = fabs(temp);
      }

      temp = pMeasure->GetPixelShift();
      if (temp != Null) {
        m_pixelShift = fabs(temp);
      }
    }

    if(mbValidateDN) {
      Isis::Portal inPortal(1, 1, pCube->pixelType());
      inPortal.SetPosition(pSample, pLine, 1);
      pCube->read(inPortal);
      mdDnValue = inPortal[0];
    }

    if(pMeasureGrp != NULL) {
      if(mbCameraRequired && (mdEmissionAngle != Null || mdIncidenceAngle != Null || mdResolution != Null)) {
        *pMeasureGrp += Isis::PvlKeyword("EmissionAngle",  toString(mdEmissionAngle));
        *pMeasureGrp += Isis::PvlKeyword("IncidenceAngle", toString(mdIncidenceAngle));
        *pMeasureGrp += Isis::PvlKeyword("Resolution",     toString(mdResolution));
      }
      else {
        *pMeasureGrp += Isis::PvlKeyword("EmissionAngle",  "Invalid Emission Angle");
        *pMeasureGrp += Isis::PvlKeyword("IncidenceAngle", "Invalid Incidence Angle");
        *pMeasureGrp += Isis::PvlKeyword("Resolution",     "Invalid Resolution");
      }
      if(mbValidateDN) {
        *pMeasureGrp += Isis::PvlKeyword("DNValue", toString(mdDnValue));
      }
      *pMeasureGrp += Isis::PvlKeyword("SampleResidual",    toString(mdSampleResidual));
      *pMeasureGrp += Isis::PvlKeyword("LineResidual",      toString(mdLineResidual));
      *pMeasureGrp += Isis::PvlKeyword("ResidualMagnitude", toString(mdResidualMagnitude));

      *pMeasureGrp += Isis::PvlKeyword("SampleShift", toString(m_sampleShift));
      *pMeasureGrp += Isis::PvlKeyword("LineShift", toString(m_lineShift));
      *pMeasureGrp += Isis::PvlKeyword("PixelShift", toString(m_pixelShift));
    }

    MeasureValidationResults results;

    if(mbCameraRequired) {
      if(!ValidEmissionAngle(mdEmissionAngle)) {
        results.addFailure(MeasureValidationResults::EmissionAngle,
            mdEmissionAngle, mdMinEmissionAngle, mdMaxEmissionAngle);
      }

      if(!ValidIncidenceAngle(mdIncidenceAngle)) {
        results.addFailure(MeasureValidationResults::IncidenceAngle,
            mdIncidenceAngle, mdMinIncidenceAngle, mdMaxIncidenceAngle);
      }

      if(!ValidResolution(mdResolution)) {
        results.addFailure(MeasureValidationResults::Resolution,
          mdResolution, mdMinResolution, mdMaxResolution);
      }
    }

    if(mbValidateDN) {
      if(!ValidDnValue(mdDnValue)) {
        results.addFailure(MeasureValidationResults::DNValue,
            mdDnValue, mdMinDN, mdMaxDN);
      }
    }

    if(mbValidateFromEdge) {
      if(!PixelsFromEdge((int)pSample, (int)pLine, pCube)) {
        results.addFailure(MeasureValidationResults::PixelsFromEdge, miPixelsFromEdge);
      }

      if(!MetersFromEdge((int)pSample, (int)pLine, pCube)) {
        results.addFailure(MeasureValidationResults::MetersFromEdge,
            mdMetersFromEdge);
      }
    }

    if(pMeasure != NULL) {
      ValidResidualTolerances(mdSampleResidual, mdLineResidual,
                              mdResidualMagnitude, results);
      ValidShiftTolerances(m_sampleShift, m_lineShift,
                           m_pixelShift, results);
    }

    return results;
  }

  /**
   * Validate a point on an image for Standard Options
   *
   * @author Sharmila Prasad (5/17/2011)
   *
   * @param pSample     - Image Sample
   * @param pLine       - Image Line
   * @param pCube       - Image
   * @param pMeasureGrp - Optional Result Group
   *
   * @return MeasureValidationResults
   */
  MeasureValidationResults ControlNetValidMeasure::ValidStandardOptions(
       double pSample, double pLine, Cube *pCube, PvlGroup *pMeasureGrp) {

    return ValidStandardOptions(pSample, pLine, NULL, pCube, pMeasureGrp);

  }


  /**
   * Validate a measure for all the Standard Options
   *
   * @author Sharmila Prasad (6/22/2010)
   *
   * @param piSample    - Point Sample location
   * @param piLine      - Point Line location
   * @param pCube       - Control Measure Cube
   * @param pMeasureGrp - Log PvlGroup
   *
   * @return bool
   */
  MeasureValidationResults ControlNetValidMeasure::ValidStandardOptions(
    const ControlMeasure * pMeasure, Cube *pCube, PvlGroup *pMeasureGrp) {

    double dSample, dLine;
    dSample = pMeasure->GetSample();
    dLine   = pMeasure->GetLine();

    return (ValidStandardOptions(dSample, dLine, pMeasure, pCube, pMeasureGrp));
  }


  MeasureValidationResults ControlNetValidMeasure::ValidStandardOptions(
    const ControlMeasure * pMeasure, Cube *pCube, Camera *camera,
    PvlGroup *pMeasureGrp) {

    double dSample, dLine;
    dSample = pMeasure->GetSample();
    dLine   = pMeasure->GetLine();

    return ValidStandardOptions(dSample, dLine, pMeasure,
          pCube, camera, pMeasureGrp);
  }

  /**
   * Validate and Read the Pixels and Meters from Edge Standard
   * Options
   *
   * @author Sharmila Prasad (6/22/2010)
   */
  void ControlNetValidMeasure::ValidatePvlFromEdge(void) {
    // Parse the Pixels from edge
    if(mPvlOpGrp.hasKeyword("PixelsFromEdge")) {
      miPixelsFromEdge = mPvlOpGrp["PixelsFromEdge"];
      if(miPixelsFromEdge < 0) {
        miPixelsFromEdge = 0;
      }
      else {
        mbValidateFromEdge = true;
      }
      mStdOptionsGrp += Isis::PvlKeyword("PixelsFromEdge", toString(miPixelsFromEdge));
    }
    // Parse the Meters from edge
    if(mPvlOpGrp.hasKeyword("MetersFromEdge")) {
      mdMetersFromEdge = mPvlOpGrp["MetersFromEdge"];
      if(mdMetersFromEdge < 0) {
        mdMetersFromEdge = 0;
      }
      else {
        mbValidateFromEdge = true;
      }
      mStdOptionsGrp += Isis::PvlKeyword("MetersFromEdge", toString(mdMetersFromEdge));
    }
  }

  /**
   * Validate the Min and Max Resolution Values set by the user in the Operator pvl file.
   * If not set then set the options to default and enter their names in the Unused Group.
   * If the user set values are invalid then exception is thrown.
   *
   * @author Sharmila Prasad (6/4/2010)
   */
  void ControlNetValidMeasure::ValidatePvlResolution(void) {
    if(mPvlOpGrp.hasKeyword("MinResolution")){
      mdMinResolution = mPvlOpGrp["MinResolution"];
      mbCameraRequired = true;
    }
    else {
      mdMinResolution = 0;
    }
    mStdOptionsGrp += Isis::PvlKeyword("MinResolution", toString(mdMinResolution));

    if(mPvlOpGrp.hasKeyword("MaxResolution")){
      mdMaxResolution = mPvlOpGrp["MaxResolution"];
      mbCameraRequired = true;
    }
    else {
      mdMaxResolution = DBL_MAX;
    }
    mStdOptionsGrp += Isis::PvlKeyword("MaxResolution", (mdMaxResolution   == DBL_MAX ? "NA" : toString(mdMaxResolution)));

    if(mdMinResolution < 0 || mdMaxResolution < 0) {
      std::string msg = "Invalid Resolution value(s), Resolution must be greater than zero";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if(mdMaxResolution < mdMinResolution) {
      std::string msg = "MinResolution must be less than MaxResolution";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  /**
   * Validate the Min and Max Dn Values set by the user in the Operator pvl file.
   * If not set then set the options to default and enter their names in the Unused Group.
   * If the user set values are invalid then exception is thrown.
   *
   * @author Sharmila Prasad (5/10/2010)
   *
   */
  void ControlNetValidMeasure::ValidatePvlDN(void) {
    if(mPvlOpGrp.hasKeyword("MinDN")) {
      mdMinDN = mPvlOpGrp["MinDN"];
      mbValidateDN = true;
    }
    else {
      mdMinDN = Isis::ValidMinimum;
    }
    mStdOptionsGrp += Isis::PvlKeyword("MinDN", (mdMinDN == Isis::ValidMinimum ? "NA" : toString(mdMinDN)));

    if(mPvlOpGrp.hasKeyword("MaxDN")) {
      mdMaxDN = mPvlOpGrp["MaxDN"];
      mbValidateDN = true;
    }
    else {
      mdMaxDN = Isis::ValidMaximum;
    }
    mStdOptionsGrp += Isis::PvlKeyword("MaxDN", (mdMaxDN == Isis::ValidMaximum ? "NA" : toString(mdMaxDN)));

    if(mdMaxDN < mdMinDN) {
      std::string msg = "MinDN must be less than MaxDN";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  /**
   * ValidateEmissionAngle: Validate the Min and Max Emission Values set by the user in the Operator pvl file.
   * If not set then set the options to default and enter their names in the Unused Group.
   * If the user set values are invalid then exception is thrown, the valid range being [0-135]
   *
   * @author Sharmila Prasad (5/10/2010)
   *
   */
  void ControlNetValidMeasure::ValidatePvlEmissionAngle(void) {
    if(mPvlOpGrp.hasKeyword("MinEmission")) {
      mdMinEmissionAngle = mPvlOpGrp["MinEmission"];
      mbCameraRequired = true;
      if(mdMinEmissionAngle < 0 || mdMinEmissionAngle > 135) {
        std::string msg = "Invalid Min Emission Angle, Valid Range is [0-135]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    mStdOptionsGrp += Isis::PvlKeyword("MinEmission", toString(mdMinEmissionAngle));

    if(mPvlOpGrp.hasKeyword("MaxEmission")) {
      mdMaxEmissionAngle = mPvlOpGrp["MaxEmission"];
      mbCameraRequired = true;
      if(mdMaxEmissionAngle < 0 || mdMaxEmissionAngle > 135) {
        std::string msg = "Invalid Max Emission Angle, Valid Range is [0-135]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    mStdOptionsGrp += Isis::PvlKeyword("MaxEmission", toString(mdMaxEmissionAngle));

    if(mdMaxEmissionAngle < mdMinEmissionAngle) {
      std::string msg = "Min EmissionAngle must be less than Max EmissionAngle";
      throw IException(IException::User, msg, _FILEINFO_);
    }

  }

  /**
   * ValidateIncidenceAngle: Validate the Min and Max Incidence Values set by the user in the Operator pvl file.
   * If not set then set the options to default and enter their names in the Unused Group.
   * If the user set values are invalid then exception is thrown, the valid range being [0-135]
   *
   * @author Sharmila Prasad (5/10/2010)
   *
   */
  void ControlNetValidMeasure::ValidatePvlIncidenceAngle(void) {
    if(mPvlOpGrp.hasKeyword("MinIncidence")) {
      mdMinIncidenceAngle = mPvlOpGrp["MinIncidence"];
      mbCameraRequired = true;
      if(mdMinIncidenceAngle < 0 || mdMinIncidenceAngle > 135) {
        std::string msg = "Invalid Min Incidence Angle, Valid Range is [0-135]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    mStdOptionsGrp += Isis::PvlKeyword("MinIncidence", toString(mdMinIncidenceAngle));

    if(mPvlOpGrp.hasKeyword("MaxIncidence")) {
      mdMaxIncidenceAngle = mPvlOpGrp["MaxIncidence"];
      mbCameraRequired = true;
      if(mdMaxIncidenceAngle < 0 || mdMaxIncidenceAngle > 135) {
        std::string msg = "Invalid Max Incidence Angle, Valid Range is [0-135]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    mStdOptionsGrp += Isis::PvlKeyword("MaxIncidence", toString(mdMaxIncidenceAngle));

    if(mdMaxIncidenceAngle < mdMinIncidenceAngle) {
      std::string msg = "Min IncidenceAngle must be less than Max IncidenceAngle";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  /**
   * Validate Pvl Sample, Line, Residual Magnitude Tolerances
   *
   * @author Sharmila Prasad (5/16/2011)
   */
  void ControlNetValidMeasure::ValidatePvlResidualTolerances(void){
    bool bRes=false;
    bool bResMag = false;
    if(mPvlOpGrp.hasKeyword("SampleResidual")) {
      mdSampleResTolerance = mPvlOpGrp["SampleResidual"];
      if(mdSampleResTolerance < 0) {
        std::string msg = "Invalid Sample Residual, must be greater than zero";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      bRes = true;
    }
    mStdOptionsGrp += Isis::PvlKeyword("SampleResidual", (mdSampleResTolerance   == DBL_MAX ? "NA" : toString(mdSampleResTolerance)));

    if(mPvlOpGrp.hasKeyword("LineResidual")) {
      mdLineResTolerance = mPvlOpGrp["LineResidual"];
      if(mdLineResTolerance < 0) {
        std::string msg = "Invalid Line Residual, must be greater than zero";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      bRes = true;
    }
    mStdOptionsGrp += Isis::PvlKeyword("LineResidual", (mdLineResTolerance   == DBL_MAX ? "NA" : toString(mdLineResTolerance)));

    if(mPvlOpGrp.hasKeyword("ResidualMagnitude")) {
      mdResidualTolerance = mPvlOpGrp["ResidualMagnitude"];
      if(mdResidualTolerance < 0) {
        std::string msg = "Invalid Residual Magnitude Tolerance, must be greater than zero";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      bResMag = true;
    }
    mStdOptionsGrp += Isis::PvlKeyword("ResidualMagnitude", (mdResidualTolerance   == DBL_MAX ? "NA" : toString(mdResidualTolerance)));

    if(bRes && bResMag) {
      std::string msg = "Cannot have both Sample/Line Residuals and Residual Magnitude.";
      msg += "\nChoose either Sample/Line Residual or Residual Magnitude";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }


  /**
   * Validate Pvl Sample, Line, Pixel (Sample and Line) Magnitude Shift
   * Tolerances
   */
  void ControlNetValidMeasure::ValidatePvlShiftTolerances() {
    bool hasSampleLineShift = false;
    if (mPvlOpGrp.hasKeyword("SampleShift")) {
      m_sampleShiftTolerance = mPvlOpGrp["SampleShift"];
      if (m_sampleShiftTolerance < 0) {
        std::string msg = "Invalid Sample Shift tolerance:"
            " must be greater than or equal to zero";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      hasSampleLineShift = true;
    }
    mStdOptionsGrp += Isis::PvlKeyword("SampleShift", (m_sampleShiftTolerance == DBL_MAX ? "NA" : toString(m_sampleShiftTolerance)));

    if (mPvlOpGrp.hasKeyword("LineShift")) {
      m_lineShiftTolerance = mPvlOpGrp["LineShift"];
      if (m_lineShiftTolerance < 0) {
        std::string msg = "Invalid Line Shift tolerance:"
            " must be greater than or equal to zero";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      hasSampleLineShift = true;
    }
    mStdOptionsGrp += Isis::PvlKeyword("LineShift", (m_lineShiftTolerance == DBL_MAX ? "NA" : toString(m_lineShiftTolerance)));

    bool hasPixelShift = false;
    if (mPvlOpGrp.hasKeyword("PixelShift")) {
      m_pixelShiftTolerance = mPvlOpGrp["PixelShift"];
      if (m_pixelShiftTolerance < 0) {
        std::string msg = "Invalid Pixel Shift tolerance:"
            " must be greater than or equal to zero";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      hasPixelShift = true;
    }
    mStdOptionsGrp += Isis::PvlKeyword("PixelShift", (m_pixelShiftTolerance == DBL_MAX ? "NA" : toString(m_pixelShiftTolerance)));

    if (hasSampleLineShift && hasPixelShift) {
      std::string msg = "Cannot have both Sample/Line Shift and Pixel Shift";
      msg += " tolerances.\n";
      msg += "Choose either Sample/Line Shift or Pixel Shift to validate on";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }


  /**
   * Validates an Emission angle by comparing with the min and max values in the def file.
   * If Emission Angle is greater or lesser than the max/min values in the def file or the defaults
   * it returns false else true.
   *
   * @author Sharmila Prasad (3/30/2010)
   *
   * @param pdEmissionAngle - Emission Angle to Valdiate
   *
   * @return bool
   */
  bool ControlNetValidMeasure::ValidEmissionAngle(double pdEmissionAngle) {
    if(pdEmissionAngle < mdMinEmissionAngle || pdEmissionAngle > mdMaxEmissionAngle) {
      return false;
    }
    return true;
  }

  /**
   * Validates an Incidence angle by comparing with the min and max values in the def file.
   * If Incidence Angle is greater or lesser than the max/min values in the def file or the defaults
   * it returns false else true.
   *
   * @author Sharmila Prasad (5/10/2010)
   *
   * @param pdIncidenceAngle - Incidence Angle to Valdiate
   *
   * @return bool
   */
  bool ControlNetValidMeasure::ValidIncidenceAngle(double pdIncidenceAngle) {
    if(pdIncidenceAngle < mdMinIncidenceAngle || pdIncidenceAngle > mdMaxIncidenceAngle) {
      return false;
    }
    return true;
  }

  /**
   * Validates Dn Value by comparing against the Min and Max DN Values set in the
   * def file or the defaults.
   *
   * @author Sharmila Prasad (3/30/2010)
   *
   * @param pdDnValue - DN Value to Valdiate
   *
   * @return bool
   */
  bool ControlNetValidMeasure::ValidDnValue(double pdDnValue) {
    if(Isis::IsSpecial(pdDnValue) || pdDnValue < mdMinDN || pdDnValue >  mdMaxDN) {
      return false;
    }
    return true;
  }

  /**
   * Validates Dn Value by comparing against the Min and Max DN Values set in the
   * def file or the defaults.
   *
   * @author Sharmila Prasad (6/4/2010)
   *
   * @param pdResolution - Resolution to Validate
   *
   * @return bool
   */
  bool ControlNetValidMeasure::ValidResolution(double pdResolution) {
    if(pdResolution < mdMinResolution || pdResolution >  mdMaxResolution) {
      return false;
    }
    return true;
  }


  /**
   * Validate whether the Sample and Line Residuals and Residual Magnitudes are
   * within the set Tolerances'
   *
   * @author Sharmila Prasad (5/16/2011)
   *
   * @param pdSampleResidual    - Measure's Sample residual
   * @param pdLineResidual      - Measure's Line residual
   * @param pdResidualMagnitude - Measure's Residual Magnitude
   * @param pResults            - MeasureValidationResults
   *
   * @return bool - Valid (true/false)
   */
  bool ControlNetValidMeasure::ValidResidualTolerances(double pdSampleResidual,
                             double pdLineResidual, double pdResidualMagnitude,
                                            MeasureValidationResults & pResults){
    bool bFlag = true;

    if(pdSampleResidual > mdSampleResTolerance) {
      bFlag = false;
      pResults.addFailure(MeasureValidationResults::SampleResidual, mdSampleResTolerance, "greater");
    }
    if(pdLineResidual > mdLineResTolerance) {
      bFlag = false;
      pResults.addFailure(MeasureValidationResults::LineResidual, mdLineResTolerance, "greater");
    }
    if(pdResidualMagnitude > mdResidualTolerance) {
      bFlag = false;
      pResults.addFailure(MeasureValidationResults::ResidualMagnitude, mdResidualTolerance, "greater");
    }

    return bFlag;
  }


  /**
   * Validate whether the Sample and Line Shifts and Pixel Shift are within the
   * set Tolerances.
   *
   * @param sampleShift Measure's sample shift (current - apriori)
   * @param lineShift Measure's line shift (current - apriori)
   * @param pixelShift Measure's pixel shift (Euclidean distance shifted)
   * @param results Validation results populated with any new failures
   *
   * @return Whether every test was valid or not
   */
  bool ControlNetValidMeasure::ValidShiftTolerances(
      double sampleShift, double lineShift, double pixelShift,
      MeasureValidationResults &results) {

    bool valid = true;

    if (sampleShift > m_sampleShiftTolerance) {
      valid = false;
      results.addFailure(MeasureValidationResults::SampleShift,
          m_sampleShiftTolerance, "greater");
    }
    if (lineShift > m_lineShiftTolerance) {
      valid = false;
      results.addFailure(MeasureValidationResults::LineShift,
          m_lineShiftTolerance, "greater");
    }
    if (pixelShift > m_pixelShiftTolerance) {
      valid = false;
      results.addFailure(MeasureValidationResults::PixelShift,
          m_pixelShiftTolerance, "greater");
    }

    return valid;
  }


  /**
   * Validate if a point has a valid lat, lon for that camera
   *
   * @author Sharmila Prasad (6/4/2010)
   *
   * @param pCamera
   * @param piSample
   * @param piLine
   *
   * @return bool
   */
  bool ControlNetValidMeasure::ValidLatLon(Camera *pCamera, int piSample, int piLine) {
    return true;
  }

  /**
   * Validate if a point in Measure is user defined number of pixels from the edge
   *
   * @author Sharmila Prasad (6/21/2010)
   *
   * @param piSample - Point Sample Location
   * @param piLine   - Point Line Location
   * @param pCube    - Control Measure Cube
   *
   * @return bool
   */
  bool ControlNetValidMeasure::PixelsFromEdge(int piSample, int piLine, Cube *pCube) {
    if(miPixelsFromEdge <= 0) {
      return true;
    }

    int iNumSamples = pCube->sampleCount();
    int iNumLines   = pCube->lineCount();

    // test right
    if((iNumSamples - piSample) < miPixelsFromEdge) {
      return false;
    }

    // test left
    if((piSample - miPixelsFromEdge) <= 0) {
      return false;
    }

    // test down
    if((iNumLines - piLine) < miPixelsFromEdge) {
      return false;
    }

    // test up
    if((piLine - miPixelsFromEdge) <= 0) {
      return false;
    }

    return true;
  }

  /**
   * Validate if a point in Measure is user defined number of meters from the edge
   *
   * @author Sharmila Prasad (6/21/2010)
   *
   * @param piSample - Point Sample Location
   * @param piLine   - Point Line Location
   * @param pCube    - Control Measure Cube
   *
   * @return bool
   */
  bool ControlNetValidMeasure::MetersFromEdge(int piSample, int piLine, Cube *pCube) {
    if(mdMetersFromEdge <= 0) {
      return true;
    }

    int iNumSamples = pCube->sampleCount();
    int iNumLines   = pCube->lineCount();

    try {
      // Get the image's camera to get pixel resolution
      Camera *camera = pCube->camera();
      double resMetersTotal = 0;
      bool bMinDistance     = false;

      // test top
      for(int line = piLine - 1; line > 0; line--) {
        camera->SetImage(piSample, line);
        double resolution = camera->PixelResolution();
        resMetersTotal += resolution;
        if(resMetersTotal >= mdMetersFromEdge) {
          bMinDistance = true;
          break;
        }
      }
      if(!bMinDistance) {
        return false;
      }

      // test bottom
      bMinDistance   = false;
      resMetersTotal = 0;
      for(int line = piLine + 1; line <= iNumLines; line++) {
        camera->SetImage(piSample, line);
        double resolution = camera->PixelResolution();
        resMetersTotal += resolution;
        if(resMetersTotal >= mdMetersFromEdge) {
          bMinDistance = true;
          break;
        }
      }
      if(!bMinDistance) {
        return false;
      }

      // test left
      resMetersTotal = 0;
      bMinDistance   = false;
      for(int sample = piSample - 1; sample > 0; sample--) {
        camera->SetImage(sample, piLine);
        double resolution = camera->PixelResolution();
        resMetersTotal += resolution;
        if(resMetersTotal >= mdMetersFromEdge) {
          bMinDistance = true;
          break;
        }
      }
      if(!bMinDistance) {
        return false;
      }

      // test right
      resMetersTotal = 0;
      bMinDistance   = false;
      for(int sample = piSample + 1; sample <= iNumSamples; sample++) {
        camera->SetImage(sample, piLine);
        double resolution = camera->PixelResolution();
        resMetersTotal += resolution;
        if(resMetersTotal >= mdMetersFromEdge) {
          return true;
        }
      }
      return false;
    }
    catch(IException &e) {
      std::string msg = "Cannot Create Camera for Image [" +
          pCube->fileName().toStdString() + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }
};
