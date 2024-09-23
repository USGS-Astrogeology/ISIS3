#include <cmath>
#include "AlbedoAtm.h"
#include "NumericalApproximation.h"
#include "IException.h"
#include "IString.h"

namespace Isis {
  /**
   * Constructs AlbedoAtm object using a Pvl, PhotoModel, and
   * AtmosModel
   * @param pvl
   * @param pmodel
   * @param amodel
   *
   * @internal
   *   @history 2008-11-05 Jeannie Walldren - Modified references
   *          to NumericalMethods class and replaced Isis::PI with
   *          PI since this is in Isis namespace.
   */
  AlbedoAtm::AlbedoAtm(Pvl &pvl, PhotoModel &pmodel, AtmosModel &amodel) :
    NormModel(pvl, pmodel, amodel) {
    PvlGroup &algo = pvl.findObject("NormalizationModel")
                     .findGroup("Algorithm", Pvl::Traverse);
    // Set default value
    SetNormPharef(0.0);
    SetNormIncref(0.0);
    SetNormEmaref(0.0);

    // Get value from user
    if(algo.hasKeyword("Incref")) {
      SetNormIncref(algo["Incref"]);
    }

    if(algo.hasKeyword("Pharef")) {
      SetNormPharef(algo["Pharef"]);
    } else {
      p_normPharef = p_normIncref;
    }

    if(algo.hasKeyword("Emaref")) {
      SetNormEmaref(algo["Emaref"]);
    }

    // First-time setup:
    // Calculate normalization at standard conditions
    GetPhotoModel()->SetStandardConditions(true);
    p_normPsurfref = GetPhotoModel()->CalcSurfAlbedo(p_normPharef, p_normIncref, p_normEmaref);
    GetPhotoModel()->SetStandardConditions(false);

    // Get reference hemispheric albedo
    GetAtmosModel()->GenerateAhTable();

    p_normAhref = (GetAtmosModel()->AtmosAhSpline()).Evaluate(p_normIncref, NumericalApproximation::Extrapolate);

    p_normMunotref = cos((PI / 180.0) * p_normIncref);

    // Now calculate atmosphere at standard conditions
    GetAtmosModel()->SetStandardConditions(true);
    GetAtmosModel()->CalcAtmEffect(p_normPharef, p_normIncref, p_normEmaref,
                                   &p_normPstdref, &p_normTransref,
                                   &p_normTrans0ref, &p_normSbar,
                                   &p_normTranss);
    GetAtmosModel()->SetStandardConditions(false);
  }

  /**
   * Performs the normalization.
   * 
   * @param phase The phase angle.
   * @param incidence The incidence angle.
   * @param emission The emission angle.
   * @param demincidence The local incidence angle
   * @param dememission The local emission angle
   * @param dn The DN value
   * @param albedo ???
   * @param mult The multiplier of the image
   * @param base The base of the image
   * @internal
   *   @history 2008-11-05 Jeannie Walldren - Modified references
   *          to NumericalMethods class and replaced Isis::PI with
   *          PI since this is in Isis namespace.
   */
  void AlbedoAtm::NormModelAlgorithm(double phase, double incidence, double emission,
                                     double demincidence, double dememission, double dn,
                                     double &albedo, double &mult, double &base) {
    static double psurf;
    static double ahInterp;
    static double munot;
    double pstd;
    double trans;
    double trans0;
    double transs;
    double rho;
    double dpo;
    double q;
    double dpm;
    double firsterm;
    double secondterm;
    double thirdterm;
    double fourthterm;
    double fifthterm;

    static double old_phase = -9999;
    static double old_incidence = -9999;
    static double old_emission = -9999;
    static double old_demincidence = -9999;
    static double old_dememission = -9999;

    if (old_phase != phase || old_incidence != incidence || old_emission != emission ||
        old_demincidence != demincidence || old_dememission != dememission) {

      psurf = GetPhotoModel()->CalcSurfAlbedo(phase, demincidence,
                                              dememission);

      ahInterp = (GetAtmosModel()->AtmosAhSpline()).Evaluate(incidence, NumericalApproximation::Extrapolate);

      munot = cos(incidence * (PI / 180.0));

      old_phase = phase;
      old_incidence = incidence;
      old_emission = emission;
      old_demincidence = demincidence;
      old_dememission = dememission;
    }

    GetAtmosModel()->CalcAtmEffect(phase, incidence, emission, &pstd, &trans, &trans0, &p_normSbar,
                                   &transs);

    // With model at actual geometry, calculate rho from dn
    dpo = dn - pstd;
    dpm = (psurf - ahInterp * munot) * trans0;
    q = ahInterp * munot * trans + GetAtmosModel()->AtmosAb() * p_normSbar * dpo + dpm;

    if(dpo <= 0.0 && GetAtmosModel()->AtmosNulneg()) {
      rho = 0.0;
    }
    else {
      firsterm = GetAtmosModel()->AtmosAb() * p_normSbar;
      secondterm = dpo * dpm;
      thirdterm = firsterm * secondterm;
      fourthterm = pow(q, 2.0) - 4.0 * thirdterm;

      if(fourthterm < 0.0) {
        std::string msg = "Square root of negative (math) encountered";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
      else {
        fifthterm = q + sqrt(fourthterm);
      }

      rho = 2 * dpo / fifthterm;
    }

    // Now use rho and reference geometry to calculate output dnout
    if((1.0 - rho * GetAtmosModel()->AtmosAb()*p_normSbar) <= 0.0) {
      std::string msg = "Divide by zero (math) encountered";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    else {
      albedo = p_normPstdref + rho * (p_normAhref * p_normMunotref *
                                      p_normTransref / (1.0 - rho * GetAtmosModel()->AtmosAb() *
                                          p_normSbar) + (p_normPsurfref - p_normAhref *
                                              p_normMunotref) * p_normTrans0ref);
    }
  }

  /**
    * Set the normalization function parameter. This is the
    * reference phase angle to which the image photometry will
    * be normalized. This parameter is limited to values that are
    * >=0 and <180.
    *
    * @param pharef  Normalization function parameter, default
    *                is 0.0
    */
  void AlbedoAtm::SetNormPharef(const double pharef) {
    if(pharef < 0.0 || pharef >= 180.0) {
      std::string msg = "Invalid value of normalization pharef [" +
                        toString(pharef) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    p_normPharef = pharef;
  }

  /**
    * Set the normalization function parameter. This is the
    * reference incidence angle to which the image photometry will
    * be normalized. This parameter is limited to values that are
    * >=0 and <90.
    *
    * @param incref  Normalization function parameter, default
    *                is 0.0
    */
  void AlbedoAtm::SetNormIncref(const double incref) {
    if(incref < 0.0 || incref >= 90.0) {
      std::string msg = "Invalid value of normalization incref [" +
                        toString(incref) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    p_normIncref = incref;
  }

  /**
    * Set the normalization function parameter. This is the
    * reference emission angle to which the image photometry will
    * be normalized. This parameter is limited to values that are
    * >=0 and <90.
    *
    * @param emaref  Normalization function parameter, default
    *                is 0.0
    */
  void AlbedoAtm::SetNormEmaref(const double emaref) {
    if(emaref < 0.0 || emaref >= 90.0) {
      std::string msg = "Invalid value of normalization emaref [" +
                        toString(emaref) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    p_normEmaref = emaref;
  }
}

extern "C" Isis::NormModel *AlbedoAtmPlugin(Isis::Pvl &pvl, Isis::PhotoModel &pmodel, Isis::AtmosModel &amodel) {
  return new Isis::AlbedoAtm(pvl, pmodel, amodel);
}
