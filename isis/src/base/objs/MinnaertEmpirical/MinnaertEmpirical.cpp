/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <cmath>
#include "MinnaertEmpirical.h"
#include "IException.h"

namespace Isis {
  MinnaertEmpirical::MinnaertEmpirical(Pvl &pvl) : PhotoModel(pvl) {
    PvlGroup &algo = pvl.findObject("PhotometricModel")
                     .findGroup("Algorithm", Pvl::Traverse);
    // There are no default values for the Minnaert Empirical function; if user
    // does not provide information, then an exception is thrown
    if (algo.hasKeyword("PhaseList")) {
      SetPhotoPhaseList(algo["PhaseList"]);
    } else {
      std::string msg = "The empirical Minnaert phase list was not provided by user";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    if (algo.hasKeyword("KList")) {
      SetPhotoKList(algo["KList"]);
    } else {
      std::string msg = "The empirical Minnaert k exponent list was not provided by user";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    if (algo.hasKeyword("PhaseCurveList")) {
      SetPhotoPhaseCurveList(algo["PhaseCurveList"]);
    } else {
      std::string msg = "The empirical Minnaert phase brightness list was not provided by user";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Make sure all the vectors are the same size
    p_photoPhaseAngleCount = (int)p_photoPhaseList.size();

    if (p_photoPhaseAngleCount != (int)p_photoKList.size()) {
      std::string msg = "Number of empirical Minnaert k list values must be equal";
      msg += "to number of phase angles provided";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if (p_photoPhaseAngleCount != (int)p_photoPhaseCurveList.size()) {
      std::string msg = "Number of empirical Minnaert phase curve list values must be equal";
      msg += "to number of phase angles provided";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Create Cubic piecewise linear splines
    p_photoKSpline.Reset();
    p_photoKSpline.SetInterpType(NumericalApproximation::CubicClamped);
    p_photoKSpline.AddData(p_photoPhaseList,p_photoKList);
    p_photoKSpline.SetCubicClampedEndptDeriv(1.0e30,1.0e30);
    p_photoBSpline.Reset();
    p_photoBSpline.SetInterpType(NumericalApproximation::CubicClamped);
    p_photoBSpline.AddData(p_photoPhaseList,p_photoPhaseCurveList);
    p_photoBSpline.SetCubicClampedEndptDeriv(1.0e30,1.0e30);
  }

  MinnaertEmpirical::~MinnaertEmpirical() {
    p_photoKSpline.Reset();
    p_photoBSpline.Reset();
    p_photoPhaseList.clear();
    p_photoKList.clear();
    p_photoPhaseCurveList.clear();
  }

  /**
    * Set the empirical Minnaert function phase angle list.  This is the list
    * of phase angles that Minnaert K values and phase curve list values will
    * be provided for. A spline curve will be used to interpolate K values and
    * phase curve values that exist between the given phase angles. The values
    * in the phase angle list are limited to values that are >=0 and <=180.
    *
    * @param phasestrlist  List of phase angles to interpolate
    */
  void MinnaertEmpirical::SetPhotoPhaseList(QString phasestrlist) {
    double phaseangle;
    IString strlist(phasestrlist.toStdString());
    p_photoPhaseList.clear();

    while (strlist.length()) {
      phaseangle = strlist.Token(",");
      if (phaseangle < 0.0 || phaseangle > 180.0) {
        std::string msg = "Invalid value of empirical Minnaert phase angle list value [" +
                          IString(phaseangle) + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      p_photoPhaseList.push_back(phaseangle);
    }
  }

  /**
    * Set the empirical Minnaert function phase angle list.  This is the list
    * of phase angles that Minnaert K values and phase curve list values will
    * be provided for. A spline curve will be used to interpolate K values and
    * phase curve values that exist between the given phase angles. The values
    * in the phase angle list are limited to values that are >=0 and <=180.
    *
    * @param phaselist  PvlKeyword containing phase angles to interpolate
    */
  void MinnaertEmpirical::SetPhotoPhaseList(PvlKeyword phaseList) {

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
        std::string msg = "Invalid value of empirical Minnaert phase angle list value [" +
                          toString(phaseAngle) + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      p_photoPhaseList.push_back(phaseAngle);
    }
  }

  /**
    * Set the empirical Minnaert function K exponent list.  This is used to
    * govern the limb-darkening in the Minnaert photometric function.  Values
    * of the Minnaert exponent generally fall in the range from 0.5 ("lunar-like",
    * almost no limb darkening) to 1.0 (Lambert function).  This
    * parameter is limited to values that are >=0.
    *
    * @param kstrlist  List of Minnaert function exponents to interpolate
    */
  void MinnaertEmpirical::SetPhotoKList(QString kstrlist) {
    double kvalue;
    IString strlist(kstrlist.toStdString());
    p_photoKList.clear();

    while (strlist.length()) {
      kvalue = strlist.Token(",");
      if (kvalue < 0.0) {
        std::string msg = "Invalid value of Minnaert k list value [" +
                          IString(kvalue) + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      p_photoKList.push_back(kvalue);
    }
  }

  /**
    * Set the empirical Minnaert function K exponent list.  This is used to
    * govern the limb-darkening in the Minnaert photometric function.  Values
    * of the Minnaert exponent generally fall in the range from 0.5 ("lunar-like",
    * almost no limb darkening) to 1.0 (Lambert function).  This
    * parameter is limited to values that are >=0.
    *
    * @param kstrList  PvkKeyword containing List of Minnaert function exponents to interpolate
    */
  void MinnaertEmpirical::SetPhotoKList(PvlKeyword kstrList) {

    // If the format is Keyword="1,2,3,4,5" rather than Keyword = (1,2,3,4,5) 
    if (kstrList.size() == 1) {
      SetPhotoKList(QString::fromStdString(kstrList));
      return;
    }

    p_photoKList.clear();
    for (int i=0; i<kstrList.size(); i++) {
      p_photoKList.push_back(std::stod(kstrList[i]));
    }
  }

  /**
    * Set the empirical Minnaert function phase curve list.  This list provides
    * the brightness values that correspond to the limb-darkening values in the
    * empirical Minnaert photometric function.
    *
    * @param phasecurvelist  List of brightness values corresponding to Minnaert function exponents
    */
  void MinnaertEmpirical::SetPhotoPhaseCurveList(QString phasecurvestrlist) {
    double phasecurve;
    IString strlist(phasecurvestrlist.toStdString());
    p_photoPhaseCurveList.clear();

    while (strlist.length()) {
      phasecurve = strlist.Token(",");
      p_photoPhaseCurveList.push_back(phasecurve);
    }
  }

  /**
    * Set the empirical Minnaert function phase curve list.  This list provides
    * the brightness values that correspond to the limb-darkening values in the
    * empirical Minnaert photometric function.
    *
    * @param phasecurvelist  PvlKeyword containing list of brightness values corresponding 
    * to Minnaert function exponents
    */
  void MinnaertEmpirical::SetPhotoPhaseCurveList(PvlKeyword photocurvestrList) {

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

  double MinnaertEmpirical::PhotoModelAlgorithm(double phase, double incidence,
                                       double emission) {
    static double pht_minnaert_empirical;
    double incrad;
    double emarad;
    double munot;
    double mu;
    double kInterpolated = 0;
    double bInterpolated = 0;

    static double old_phase = -9999;
    static double old_incidence = -9999;
    static double old_emission= -9999;

    // Don't need to do anything if the photometric angles are the same as before
    if (old_phase == phase && old_incidence == incidence && old_emission == emission) {
      return pht_minnaert_empirical;
    }

    old_incidence = incidence;
    old_emission = emission;

    incrad = incidence * Isis::PI / 180.0;
    emarad = emission * Isis::PI / 180.0;
    munot = cos(incrad);
    mu = cos(emarad);

    if (phase != old_phase) {
      kInterpolated = p_photoKSpline.Evaluate(phase, NumericalApproximation::Extrapolate);
      bInterpolated = p_photoBSpline.Evaluate(phase, NumericalApproximation::Extrapolate);
      old_phase = phase;
    }

    if(munot <= 0.0 || mu <= 0.0 || incidence == 90.0 ||
        emission == 90.0) {
      pht_minnaert_empirical = 0.0;
    }
    else if(kInterpolated == 1.0) {
      pht_minnaert_empirical = munot * bInterpolated;
    }
    else {
      pht_minnaert_empirical = bInterpolated * munot * pow((munot * mu), (kInterpolated - 1.0));
    }

    return pht_minnaert_empirical;
  }
}

extern "C" Isis::PhotoModel *MinnaertEmpiricalPlugin(Isis::Pvl &pvl) {
  return new Isis::MinnaertEmpirical(pvl);
}
