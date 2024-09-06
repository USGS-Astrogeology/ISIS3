/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Albedo.h"
#include "SpecialPixel.h"
#include "IException.h"
#include "IString.h"
#include "NormModel.h"

namespace Isis {
  /**
   * Constructs an Albedo object.
   * 
   * @param pvl
   * @param pmodel
   */
  Albedo::Albedo(Pvl &pvl, PhotoModel &pmodel) : NormModel(pvl, pmodel) {
    PvlGroup &algorithm = pvl.findObject("NormalizationModel").findGroup("Algorithm", Pvl::Traverse);

    SetNormPharef(0.0);
    SetNormIncref(0.0);
    SetNormEmaref(0.0);
    SetNormIncmat(0.0);
    SetNormThresh(30.0);
    SetNormAlbedo(1.0);

    // Get value from user
    if(algorithm.hasKeyword("Incref")) {
      SetNormIncref(algorithm["Incref"]);
    }

    if(algorithm.hasKeyword("Pharef")) {
      SetNormPharef(algorithm["Pharef"]);
    } else {
      p_normPharef = p_normIncref;
    }

    if(algorithm.hasKeyword("Emaref")) {
      SetNormEmaref(algorithm["Emaref"]);
    }

    if(algorithm.hasKeyword("Incmat")) {
      SetNormIncmat(algorithm["Incmat"]);
    }

    if(algorithm.hasKeyword("Thresh")) {
      SetNormThresh(algorithm["Thresh"]);
    }

    if(algorithm.hasKeyword("Albedo")) {
      SetNormAlbedo(algorithm["Albedo"]);
    }

    // Calculate normalization at standard conditions.
    GetPhotoModel()->SetStandardConditions(true);
    p_normPsurfref = GetPhotoModel()->CalcSurfAlbedo(p_normPharef, p_normIncref, p_normEmaref);
    GetPhotoModel()->SetStandardConditions(false);
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
   */
  void Albedo::NormModelAlgorithm(double phase, double incidence,
                                  double emission, double demincidence, double dememission,
                                  double dn, double &albedo, double &mult, double &base) {
    double psurf;
    double result;

    // code for scaling each pixel
    psurf = GetPhotoModel()->CalcSurfAlbedo(phase, demincidence, dememission);

    // thresh is a parameter limiting how much we amplify the dns
    if(p_normPsurfref > psurf * p_normThresh) {
      result = NULL8;
      albedo = NULL8;
      mult = 0.0;
      base = 0.0;
    }
    else {
      if(psurf == 0.0) {
        std::string msg = "Albedo math divide by zero error";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
      else {
        result = dn * p_normPsurfref / psurf;
        albedo = result;
        mult = p_normPsurfref / psurf;
        base = 0.0;
      }
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
  void Albedo::SetNormPharef(const double pharef) {
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
  void Albedo::SetNormIncref(const double incref) {
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
  void Albedo::SetNormEmaref(const double emaref) {
    if(emaref < 0.0 || emaref >= 90.0) {
      std::string msg = "Invalid value of normalization emaref [" +
                        toString(emaref) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    p_normEmaref = emaref;
  }

  /**
   * Set the normalization function parameter. This
   * parameter is limited to values that are >=0 and <90.
   *
   * @param incmat  Normalization function parameter, default is 0.0
   */
  void Albedo::SetNormIncmat(const double incmat) {
    if(incmat < 0.0 || incmat >= 90.0) {
      std::string msg = "Invalid value of normalization incmat [" +
                        toString(incmat) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    p_normIncmat = incmat;
  }

  /**
   * Set the normalization function parameter. This is
   * the albedo that the image will be normalized to have. To
   * construct mosaics, the same value of albedo should be used
   * for all images to achieve a uniform result.
   *
   * @param albedo  Normalization function parameter
   */
  void Albedo::SetNormAlbedo(const double albedo) {
    p_normAlbedo = albedo;
  }

  /**
   * Set the normalization function parameter. It is
   * used to amplify variations in the input image in regions
   * of small incidence angle where the shading in the input
   * image is weak. This parameter sets the upper limit on the
   * amount of amplification that will be attempted. If it is
   * set too low, low incidence areas of the image may appear
   * bland. If it is set too high, then low incidence areas of
   * the image may contain amplified noise rather than useful
   * shading information.
   *
   * @param thresh  Normalization function parameter, default
   *                is 30.0
   */
  void Albedo::SetNormThresh(const double thresh) {
    p_normThresh = thresh;
  }
}

extern "C" Isis::NormModel *AlbedoPlugin(Isis::Pvl &pvl, Isis::PhotoModel &pmodel) {
  return new Isis::Albedo(pvl, pmodel);
}
