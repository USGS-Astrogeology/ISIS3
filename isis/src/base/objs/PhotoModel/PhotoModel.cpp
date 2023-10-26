/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <cmath>
#include "FileName.h"
#include "PhotoModel.h"
#include "Plugin.h"
#include "Pvl.h"
#include "IException.h"

using namespace std;

namespace Isis {
  /**
   * Create a PhotoModel object.  Because this is a pure virtual
   * class you can not create a PhotoModel class directly.  Instead,
   * see the PhotoModelFactory class.
   *
   * @param pvl  A pvl object containing a valid PhotoModel specification
   *
   * @see photometricModels.doc
   */
  PhotoModel::PhotoModel(Pvl &pvl) {
    PvlGroup &algorithm = pvl.findObject("PhotometricModel").findGroup("Algorithm", Pvl::Traverse);

    // Use 'PhtName' instead of 'Name' if using the Gui combo box
    // for unique Pvl keyword in DefFile
    if(algorithm.hasKeyword("PhtName")) {
      p_photoAlgorithmName = QString::fromStdString(algorithm["PhtName"][0]);
    }
    else if(algorithm.hasKeyword("Name")) {
      p_photoAlgorithmName = QString::fromStdString(algorithm["Name"][0]);
    }
    else {
      IString msg = "Keyword [Name] or keyword [PhtName] must ";
      msg += "exist in [Group = Algorithm]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    p_standardConditions = false;
  }

  /**
   * Sets whether standard conditions will be used.
   * @param standard True if standard conditions are used.
   */
  void PhotoModel::SetStandardConditions(bool standard) {
    p_standardConditions = standard;
  }

  /**
   * Obtain topographic derivative of an arbitrary photometric
   * function
   *
   * @param phase  Input phase angle
   * @param incidence  Input incidence angle
   * @param emission  Input emission angle
   * @returns <B>double</B>  Gradient
   *
   */
  double PhotoModel::PhtTopder(double phase, double incidence,
                               double emission) {
    double xi, zi;
    double cphi;
    double phi;
    double xe, ye, ze;
    double epsh;
    double xy, z;
    double cinc;
    double inc1, inc2, inc3, inc4;
    double cema;
    double ema1, ema2, ema3, ema4;
    double d1, d2;
    double result;
    double eps;

    eps = 0.04;

    // set up incidence vector
    xi = sin((Isis::PI / 180.0) * incidence);
    zi = cos((Isis::PI / 180.0) * incidence);

    // phi is the azimuth from xz plane to emission direction; if
    // either incidence or emission is zero, it's arbitrary and gets
    // set to zero. Thus cos(phi), cphi, is set to one.
    if((incidence == 0.0) || (emission == 0.0)) {
      cphi = 1.0;
    }
    else {
      cphi = (cos((Isis::PI / 180.0) * phase) -
              cos((Isis::PI / 180.0) * incidence) *
              cos((Isis::PI / 180.0) * emission)) /
             (sin((Isis::PI / 180.0) * incidence) *
              sin((Isis::PI / 180.0) * emission));
    }

    // now calculate the emission vector
    phi = PhtAcos(cphi) * (180.0 / Isis::PI);
    xe = cphi * sin((Isis::PI / 180.0) * emission);
    ye = sin((Isis::PI / 180.0) * phi) * sin((Isis::PI / 180.0) * emission);
    ze = cos((Isis::PI / 180.0) * emission);

    // now evaluate two orthogonal derivatives
    epsh = eps * 0.5;
    xy = sin((Isis::PI / 180.0) * epsh);
    z = cos((Isis::PI / 180.0) * epsh);

    cinc = max(-1.0, min(xy * xi + z * zi, 1.0));
    inc1 = PhtAcos(cinc) * (180.0 / Isis::PI);
    cema = max(-1.0, min(xy * xe + z * ze, 1.0));
    ema1 = PhtAcos(cema) * (180.0 / Isis::PI);

    cinc = max(-1.0, min(-xy * xi + z * zi, 1.0));
    inc2 = PhtAcos(cinc) * (180.0 / Isis::PI);
    cema = max(-1.0, min(-xy * xe + z * ze, 1.0));
    ema2 = PhtAcos(cema) * (180.0 / Isis::PI);

    cinc = max(-1.0, min(z * zi, 1.0));
    inc3 = PhtAcos(cinc) * (180.0 / Isis::PI);
    cema = max(-1.0, min(xy * ye + z * ze, 1.0));
    ema3 = PhtAcos(cema) * (180.0 / Isis::PI);

    cinc = max(-1.0, min(z * zi, 1.0));
    inc4 = PhtAcos(cinc) * (180.0 / Isis::PI);
    cema = max(-1.0, min(-xy * ye + z * ze, 1.0));
    ema4 = PhtAcos(cema) * (180.0 / Isis::PI);

    d1 = (CalcSurfAlbedo(phase, inc1, ema1) - CalcSurfAlbedo(phase, inc2, ema2)) / eps;
    d2 = (CalcSurfAlbedo(phase, inc3, ema3) - CalcSurfAlbedo(phase, inc4, ema4)) / eps;

    //  Combine these two derivatives and return the gradient
    result = sqrt(max(1.0e-30, d1 * d1 + d2 * d2));
    return result;
  }

  /**
   * Obtain arccosine of input value. If the input value is outside
   * of the valid range (-1 to 1), then obtain the arccosine of the
   * closest valid value.
   *
   * @param cosang  input value to obtain arccosine of (in radians)
   * @returns <B>double</B> Arccosine of <B>cosang</B>, if valid.
   * @history 2008-11-05 Jeannie Walldren - This method was
   *        moved from NumericalMethods class.
   *
   */
  double PhotoModel::PhtAcos(double cosang) {
    double result;

    if(fabs(cosang) <= 1.0) {
      result = acos(cosang);
    }
    else {
      if(cosang < -1.0) {
        result = acos(-1.0);
      }
      else {
        result = acos(1.0);
      }
    }

    return result;
  }

  /**
   * Calculate the surface brightness using photometric angle information
   * @param pha Phase angle
   * @param inc Incidence angle
   * @param ema Emission angle
   * @returns <B>double</B>  Surface brightness
   *          calculated by the photometric function
   *
   */
  double PhotoModel::CalcSurfAlbedo(double pha, double inc, double ema) {

    // Check validity of photometric angles
    //if (pha > 180.0 || inc > 90.0 || ema > 90.0 || pha < 0.0 ||
    //    inc < 0.0 || ema < 0.0) {
    //  std::string msg = "Invalid photometric angles";
    //  throw iException::Message(iException::Programmer,msg,_FILEINFO_);
    //}

    // Apply photometric function
    double albedo = PhotoModelAlgorithm(pha, inc, ema);
    return albedo;
  }

  /**
   * Set the Lunar-Lambert function weight.  This is used to govern the
   * limb-darkening in the Lunar-Lambert photometric function.  Values of
   * the Lunar-Lambert weight generally fall in the range from 0.0
   * (Lambert function) to 1.0 (Lommel-Seeliger or "lunar" function).
   * There are no limits on the value of this parameter, but values far
   * outside the 0 to 1 range will not be very useful.
   *
   * @param l  Lunar-Lambert function weight, default is 1.0
   */
//  void PhotoModel::SetPhotoL(const double l) {
//    p_photoL = l;
//  }

  /**
    * Set the Minnaert function exponent.  This is used to govern the limb-
    * darkening in the Minnaert photometric function.  Values of the
    * Minnaert exponent generally fall in the range from 0.5 ("lunar-like",
    * almost no limb darkening) to 1.0 (Lambert function).  This
    * parameter is limited to values that are >=0.
    *
    * @param k  Minnaert function exponent, default is 1.0
    */
//  void PhotoModel::SetPhotoK(const double k) {
//    if(k < 0.0) {
//      std::string msg = "Invalid value of Minnaert k [" +
//                        IString(k) + "]";
//      throw iException::Message(iException::User, msg, _FILEINFO_);
//    }
//    p_photoK = k;
//  }
}
