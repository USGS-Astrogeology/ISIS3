/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <cmath>
#include "LunarLambertEmpirical.h"
#include "IException.h"

namespace Isis {
  LunarLambertEmpirical::LunarLambertEmpirical(Pvl &pvl) : PhotoModel(pvl) {
    PvlGroup &algo = pvl.findObject("PhotometricModel")
                     .findGroup("Algorithm", Pvl::Traverse);
    // There are no default values for the Lunar Lambert Empirical function; if user
    // does not provide information, then an exception is thrown
    if (algo.hasKeyword("PhaseList")) {
      SetPhotoPhaseList(algo["PhaseList"]);
    } else {
      std::string msg = "The empirical Lunar Lambert phase list was not provided by user";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    if (algo.hasKeyword("LList")) {
      SetPhotoLList(algo["LList"]);
    } else {
      std::string msg = "The empirical Lunar Lambert l exponent list was not provided by user";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    if (algo.hasKeyword("PhaseCurveList")) {
      SetPhotoPhaseCurveList(algo["PhaseCurveList"]);
    } else {
      std::string msg = "The empirical Lunar Lambert phase brightness list was not provided by user";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Make sure all the vectors are the same size
    p_photoPhaseAngleCount = (int)p_photoPhaseList.size();

    if (p_photoPhaseAngleCount != (int)p_photoLList.size()) {
      std::string msg = "Number of empirical Lunar Lambert l list values must be equal";
      msg += "to number of phase angles provided";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if (p_photoPhaseAngleCount != (int)p_photoPhaseCurveList.size()) {
      std::string msg = "Number of empirical Lunar Lambert phase curve list values must be equal";
      msg += "to number of phase angles provided";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Create Cubic piecewise linear splines
    p_photoLSpline.Reset();
    p_photoLSpline.SetInterpType(NumericalApproximation::CubicClamped);
    p_photoLSpline.AddData(p_photoPhaseList,p_photoLList);
    p_photoLSpline.SetCubicClampedEndptDeriv(1.0e30,1.0e30);
    p_photoBSpline.Reset();
    p_photoBSpline.SetInterpType(NumericalApproximation::CubicClamped);
    p_photoBSpline.AddData(p_photoPhaseList,p_photoPhaseCurveList);
    p_photoBSpline.SetCubicClampedEndptDeriv(1.0e30,1.0e30);
  }

  LunarLambertEmpirical::~LunarLambertEmpirical() {
    p_photoLSpline.Reset();
    p_photoBSpline.Reset();
    p_photoPhaseList.clear();
    p_photoLList.clear();
    p_photoPhaseCurveList.clear();
  }


  /**
    * Set the empirical Lunar Lambert function phase angle list.  This is the list
    * of phase angles that Lunar Lambert L values and phase curve list values will
    * be provided for. A spline curve will be used to interpolate L values and
    * phase curve values that exist between the given phase angles. The values
    * in the phase angle list are limited to values that are >=0 and <=180.
    *
    * @param phaselist  List of phase angles to interpolate
    */
  void LunarLambertEmpirical::SetPhotoPhaseList(QString phasestrlist) {
    double phaseangle;
    IString strlist(phasestrlist.toStdString());
    p_photoPhaseList.clear();

    while (strlist.length()) {
      phaseangle = strlist.Token(",");
      if (phaseangle < 0.0 || phaseangle > 180.0) {
        std::string msg = "Invalid value of empirical Lunar Lambert phase angle list value [" +
                          std::to_string(phaseangle) + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      p_photoPhaseList.push_back(phaseangle);
    }
  }


  /**
    * Set the empirical Lunar Lambert function phase angle list.  This is the list
    * of phase angles that Lunar Lambert L values and phase curve list values will
    * be provided for. A spline curve will be used to interpolate L values and
    * phase curve values that exist between the given phase angles. The values
    * in the phase angle list are limited to values that are >=0 and <=180.
    *
    * @param phaselist  PvlKeyword containing phase angles to interpolate
    */
  void LunarLambertEmpirical::SetPhotoPhaseList(PvlKeyword phaseList) {

    // If the format is Keyword="1,2,3,4,5" rather than Keyword = (1,2,3,4,5) 
    if (phaseList.size() == 1) {
      SetPhotoPhaseList(QString::fromStdString(phaseList));
      return;
    }

    double phaseAngle;
    p_photoPhaseList.clear();

    for (int i=0; i< phaseList.size(); i++) {
      phaseAngle = std::stod(phaseList[i]);

      if (phaseAngle < 0.0 || phaseAngle > 180.0) {
        std::string msg = "Invalid value of empirical Lunar Lambert phase angle list value [" +
                          toString(phaseAngle) + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      p_photoPhaseList.push_back(phaseAngle);
    }
  }


  /**
    * Set the empirical Lunar Lambert function L exponent list.  This is used to
    * govern the limb-darkening in the Lunar Lambert photometric function.  Values
    * of the Lunar Lambert exponent generally fall in the range from 0.0
    * (Lambert function) to 1.0 (Lommel-Seeliger or "lunar" function). There are
    * no limits on the value of this parameter, but values far outside the 0 to 1
    * range will not be very useful.
    *
    * @param llist  List of Lunar Lambert function exponents to interpolate
    */
  void LunarLambertEmpirical::SetPhotoLList(QString lstrlist) {
    double lvalue;
    IString strlist(lstrlist.toStdString());
    p_photoLList.clear();

    while (strlist.length()) {
      lvalue = strlist.Token(",");
      p_photoLList.push_back(lvalue);
    }
  }


  /**
    * Set the empirical Lunar Lambert function L exponent list.  This is used to
    * govern the limb-darkening in the Lunar Lambert photometric function.  Values
    * of the Lunar Lambert exponent generally fall in the range from 0.0
    * (Lambert function) to 1.0 (Lommel-Seeliger or "lunar" function). There are
    * no limits on the value of this parameter, but values far outside the 0 to 1
    * range will not be very useful.
    *
    * @param llist  PvkKeyword containing List of Lunar Lambert function exponents to interpolate
    */
  void LunarLambertEmpirical::SetPhotoLList(PvlKeyword lstrList) {

    // If the format is Keyword="1,2,3,4,5" rather than Keyword = (1,2,3,4,5) 
    if (lstrList.size() == 1) {
      SetPhotoLList(QString::fromStdString(lstrList));
      return;
    }

    p_photoLList.clear();
    for (int i=0; i<lstrList.size(); i++) {
      p_photoLList.push_back(std::stod(lstrList[i]));
    }
  }


  /**
    * Set the empirical Lunar Lambert function phase curve list.  This list provides
    * the brightness values that correspond to the limb-darkening values in the
    * empirical Lunar Lambert photometric function.
    *
    * @param phasecurvelist  List of brightness values corresponding to Lunar Lambert function exponents
    */
  void LunarLambertEmpirical::SetPhotoPhaseCurveList(QString phasecurvestrlist) {
    double phasecurve;
    IString strlist(phasecurvestrlist.toStdString());
    p_photoPhaseCurveList.clear();

    while (strlist.length()) {
      phasecurve = strlist.Token(",");
      p_photoPhaseCurveList.push_back(phasecurve);
    }
  }


  /**
    * Set the empirical Lunar Lambert function phase curve list.  This list provides
    * the brightness values that correspond to the limb-darkening values in the
    * empirical Lunar Lambert photometric function.
    *
    * @param phasecurvelist  PvlKeyword containing list of brightness values corresponding 
    * to Lunar Lambert function exponents
    */
  void LunarLambertEmpirical::SetPhotoPhaseCurveList(PvlKeyword photocurvestrList) {

    // If the format is Keyword="1,2,3,4,5" rather than Keyword = (1,2,3,4,5) 
    if (photocurvestrList.size() == 1) {
      SetPhotoPhaseCurveList(QString::fromStdString(photocurvestrList));
      return;
    }

    p_photoPhaseCurveList.clear();
    for (int i=0; i<photocurvestrList.size(); i++) {
      p_photoPhaseCurveList.push_back(std::stod(photocurvestrList[i]));
    }
  }


  double LunarLambertEmpirical::PhotoModelAlgorithm(double phase, double incidence,
                                       double emission) {
    static double pht_lunarlambert_empirical;
    double incrad;
    double emarad;
    double munot;
    double mu;
    double lInterpolated = 0;
    double bInterpolated = 0;

    static double old_phase = -9999;
    static double old_incidence = -9999;
    static double old_emission= -9999;

    // Don't need to do anything if the photometric angles are the same as before
    if (old_phase == phase && old_incidence == incidence && old_emission == emission) {
      return pht_lunarlambert_empirical;
    }

    old_incidence = incidence;
    old_emission = emission;

    incrad = incidence * Isis::PI / 180.0;
    emarad = emission * Isis::PI / 180.0;
    munot = cos(incrad);
    mu = cos(emarad);

    if (phase != old_phase) {
      lInterpolated = p_photoLSpline.Evaluate(phase, NumericalApproximation::Extrapolate);
      bInterpolated = p_photoBSpline.Evaluate(phase, NumericalApproximation::Extrapolate);
      old_phase = phase;
    }

    if(munot <= 0.0 || mu <= 0.0) {
      pht_lunarlambert_empirical = 0.0;
    }
    else if(lInterpolated == 0.0) {
      pht_lunarlambert_empirical = munot * bInterpolated;
    }
    else if(lInterpolated == 1.0) {
      pht_lunarlambert_empirical = bInterpolated * 2.0 * munot / (munot + mu);
    }
    else {
      pht_lunarlambert_empirical = bInterpolated * munot * ((1.0 - lInterpolated) + 2.0 * lInterpolated / (munot + mu));
    }

    return pht_lunarlambert_empirical;
  }
}

extern "C" Isis::PhotoModel *LunarLambertEmpiricalPlugin(Isis::Pvl &pvl) {
  return new Isis::LunarLambertEmpirical(pvl);
}
