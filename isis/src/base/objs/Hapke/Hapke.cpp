/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <cmath>
#include "Constants.h"
#include "Hapke.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "IException.h"
#include "IString.h"

using namespace std;

namespace Isis {

  Hapke::Hapke(Pvl &pvl) : PhotoModel(pvl) {
    p_photoHh = 0.0;
    p_photoB0 = 0.0;
    p_photoTheta = 0.0;
    p_photoWh = 0.5;
    p_photoThetaold = -999.0;

    p_photoHg1 = 0.0;
    p_photoHg2 = 0.0;

    PvlGroup &algorithm = pvl.findObject("PhotometricModel").findGroup("Algorithm", Pvl::Traverse);

    p_algName = AlgorithmName().toUpper();

    if(algorithm.hasKeyword("Hg1")) {
      SetPhotoHg1(algorithm["Hg1"]);
    }

    if(algorithm.hasKeyword("Hg2")) {
      SetPhotoHg2(algorithm["Hg2"]);
    }

    if(algorithm.hasKeyword("Bh")) {
      SetPhotoBh(algorithm["Bh"]);
    }

    if(algorithm.hasKeyword("Ch")) {
      SetPhotoCh(algorithm["Ch"]);
    }

    if(algorithm.hasKeyword("ZeroB0Standard")) {
      SetPhoto0B0Standard(QString::fromStdString(algorithm["ZeroB0Standard"][0]));
    } else if (algorithm.hasKeyword("ZeroB0St")) {
      SetPhoto0B0Standard(QString::fromStdString(algorithm["ZeroB0St"][0]));
    } else {
      SetPhoto0B0Standard("TRUE");
    }

    if(algorithm.hasKeyword("Wh")) {
      SetPhotoWh(algorithm["Wh"]);
    }

    if(algorithm.hasKeyword("Hh")) {
      SetPhotoHh(algorithm["Hh"]);
    }

    if(algorithm.hasKeyword("B0")) {
      SetPhotoB0(algorithm["B0"]);
    }

    p_photoB0save = p_photoB0;

    if(algorithm.hasKeyword("Theta")) {
      SetPhotoTheta(algorithm["Theta"]);
    }
  }

  /*
   * Computes normal albedo mult factor w/o opp surge from
   * Hapke input parameters: W,H,BO,HG,THETA.
   * Full-up Hapke's Law with macroscopic roughness.  The photometric
   * function multiplied back in will be modified to take out oppos-
   * ition effect.  This requires saving the actual value of B0 while
   * temporarily setting it to zero in the common block to compute
   * the overall normalization.
   *
   * @param phase      Value of phase angle, in degrees.
   * @param incidence  Value of incidence angle, in degrees.
   * @param emission   Value of emission angle, in degrees.
   * @returns <b>double</b>
   *
   *
   * @history 1989-08-02 Unknown author in Isis2 under name pht_hapke
   * @history 1991-08-07 Tammy Becker  relinked hapke to new photompr
   * @history 1997-02-16 James M Anderson - changed nonstandard degree trig
   *                     to use radians
   * @history 1999-01-11 KTT - Remove mu,munot,and alpha from the argument
   *                     list and pass in only ema,inc, and phase.  Remove
   *                     lat and lon from argument list because they aren't
   *                     used.
   * @history 1999-03-01 K Teal Thompson  Implement 1999-01-08 Randy Kirk
   *                     Original Specs & Code.  Declare vars, add implicit none.
   * @history 1999-11-18 Randy Kirk - fixed minor typos, implemented return with
   *                     smooth Hapke (Theta=0) result before doing rough Hapke
   *                     calculations, allow single-particle-phase params = 0
   * @history 2008-01-14 Janet Barrett - Imported into Isis3. Changed name from pht_hapke to PhotoModelAlgorithm()
   * @history 2008-11-05 Jeannie Walldren - Added documentation
   *          from Isis2 files. Replaced Isis::PI with PI since this is in Isis namespace.
   *
   */
  double Hapke::PhotoModelAlgorithm(double phase, double incidence,
                                       double emission) {
    static double pht_hapke;
    double pharad;  //phase angle in radians
    double incrad;  // incidence angle in radians
    double emarad; // emission angle in radians
    double munot;
    double mu;
    double cost;
    double sint;
    double tan2t;
    double gamma;
    double hgs;
    double cosg;
    double tang2;
    double bg;
    double pg;
    double pg1;
    double pg2;
    double sini;
    double coti;
    double cot2i;
    double ecoti;
    double ecot2i;
    double u0p0;
    double sine;
    double cote;
    double cot2e;
    double cosei;
    double sinei;
    double caz;
    double az;
    double az2;
    double faz;
    double tanaz2;
    double sin2a2;
    double api;
    double ecote;
    double ecot2e;
    double up0;
    double q;
    double ecei;
    double s2ei;
    double u0p;
    double up;
    double ecee;
    double s2ee;
    double rr1;
    double rr2;

    static double old_phase = -9999;
    static double old_incidence = -9999;
    static double old_emission= -9999;

    if (old_phase == phase && old_incidence == incidence && old_emission == emission) {
      return pht_hapke;
    }

    old_phase = phase;
    old_incidence = incidence;
    old_emission = emission;

    pharad = phase * PI / 180.0;
    incrad = incidence * PI / 180.0;
    emarad = emission * PI / 180.0;
    munot = cos(incrad);
    mu = cos(emarad);

    if(p_photoTheta != p_photoThetaold) {
      cost = cos(p_photoTheta * PI / 180.0);
      sint = sin(p_photoTheta * PI / 180.0);
      p_photoCott = cost / max(1.0e-10, sint);
      p_photoCot2t = p_photoCott * p_photoCott;
      p_photoTant = sint / cost;
      tan2t = p_photoTant * p_photoTant;
      p_photoSr = sqrt(1.0 + PI * tan2t);
      p_photoOsr = 1.0 / p_photoSr;
      SetOldTheta(p_photoTheta);
    }

    if(incidence >= 90.0) {
      pht_hapke = 0.0;
      return pht_hapke;
    }

    gamma = sqrt(1.0 - p_photoWh);
    hgs = p_photoHg1 * p_photoHg1;

    cosg = cos(pharad);
    tang2 = tan(pharad/2.0);

    if(p_photoHh == 0.0) {
      bg = 0.0;
    }
    else {
      bg = p_photoB0 / (1.0 + tang2 / p_photoHh);
    }

    if (p_algName == "HAPKEHEN") {
      pg1 = (1.0 - p_photoHg2) * (1.0 - hgs) / pow((1.0 + hgs + 2.0 *
            p_photoHg1 * cosg), 1.5);
      pg2 = p_photoHg2 * (1.0 - hgs) / pow((1.0 + hgs - 2.0 *
                                            p_photoHg1 * cosg), 1.5);
      pg = pg1 + pg2;
    } else {  // Hapke Legendre
      pg = 1.0 + p_photoBh * cosg + p_photoCh * (1.5 * pow(cosg, 2.0) - .5);
    }

    // If smooth Hapke is wanted then set Theta<=0.0
    if(p_photoTheta <= 0.0) {
      pht_hapke = p_photoWh / 4.0 * munot / (munot + mu) * ((1.0 + bg) *
                     pg - 1.0 + Hfunc(munot, gamma) * Hfunc(mu, gamma));
      return pht_hapke;
    }

    sini = sin(incrad);
    coti = munot / max(1.0e-10, sini);
    cot2i = coti * coti;
    ecoti = exp(min(-p_photoCot2t * cot2i / PI , 23.0));
    ecot2i = exp(min(-2.0 * p_photoCott * coti / PI, 23.0));
    u0p0 = p_photoOsr * (munot + sini * p_photoTant * ecoti / (2.0 - ecot2i));

    sine = sin(emarad);
    cote = mu / max(1.0e-10, sine);
    cot2e = cote * cote;

    cosei = mu * munot;
    sinei = sine * sini;

    if(sinei == 0.0) {
      caz = 1.0;
      az = 0.0;
    }
    else {
      caz = (cosg - cosei) / sinei;
      if(caz <= -1.0) {
        az = 180.0;
      }
      else if(caz > 1.0) {
        az = 0.0;
      }
      else {
        az = acos(caz) * 180.0 / PI;
      }
    }

    az2 = az / 2.0;
    if(az2 >= 90.0) {
      faz = 0.0;
    }
    else {
      tanaz2 = tan(az2 * PI / 180.0);
      faz = exp(min(-2.0 * tanaz2, 23.0));
    }

    sin2a2 = pow(sin(az2 * PI / 180.0), 2.0);
    api = az / 180.0;

    ecote = exp(min(-p_photoCot2t * cot2e / PI, 23.0));
    ecot2e = exp(min(-2.0 * p_photoCott * cote / PI, 23.0));
    up0 = p_photoOsr * (mu + sine * p_photoTant * ecote / (2.0 - ecot2e));

    if(incidence <= emission) {
      q = p_photoOsr * munot / u0p0;
    }
    else {
      q = p_photoOsr * mu / up0;
    }

    if(incidence <= emission) {
      ecei = (2.0 - ecot2e - api * ecot2i);
      s2ei = sin2a2 * ecoti;
      u0p = p_photoOsr * (munot + sini * p_photoTant * (caz * ecote + s2ei) / ecei);
      up = p_photoOsr * (mu + sine * p_photoTant * (ecote - s2ei) / ecei);
    }
    else {
      ecee = (2.0 - ecot2i - api * ecot2e);
      s2ee = sin2a2 * ecote;
      u0p = p_photoOsr * (munot + sini * p_photoTant * (ecoti - s2ee) / ecee);
      up = p_photoOsr * (mu + sine * p_photoTant * (caz * ecoti + s2ee) / ecee);
    }

    rr1 = p_photoWh / 4.0 * u0p / (u0p + up) * ((1.0 + bg) * pg -
          1.0 + Hfunc(u0p, gamma) * Hfunc(up, gamma));
    rr2 = up * munot / (up0 * u0p0 * p_photoSr * (1.0 - faz + faz * q));
    pht_hapke = rr1 * rr2;

    return pht_hapke;
  }

  /**
    * Set the Hapke Henyey Greenstein coefficient for the single
    * particle phase function. This is one of two coefficients
    * needed for the single particle phase function. This parameter
    * is limited to values that are >-1 and <1.
    *
    * @param hg1  Hapke Henyey Greenstein coefficient, default is 0.0
    */
  void Hapke::SetPhotoHg1(const double hg1) {
    if(hg1 <= -1.0 || hg1 >= 1.0) {
      string msg = "Invalid value of Hapke Henyey Greenstein hg1 [" +
                   IString(hg1) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    p_photoHg1 = hg1;
  }

  /**
    * Set the Hapke Henyey Greenstein coefficient for the single
    * particle phase function. This is one of two coefficients
    * needed for the single particle phase function. This parameter
    * is limited to values that are >=0 and <=1.
    *
    * @param hg2  Hapke Henyey Greenstein coefficient, default is 0.0
    */
  void Hapke::SetPhotoHg2(const double hg2) {
    if(hg2 < 0.0 || hg2 > 1.0) {
      string msg = "Invalid value of Hapke Henyey Greenstein hg2 [" +
                   IString(hg2) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    p_photoHg2 = hg2;
  }

  /**
    * Set the Hapke Legendre coefficient for the single
    * particle phase function. This is one of two coefficients
    * needed for the single particle phase function. This parameter
    * is limited to values that are >=-1 and <=1.
    *
    * @param bh  Hapke Legendre coefficient, default is 0.0
    */
  void Hapke::SetPhotoBh(const double bh) {
    if(bh < -1.0 || bh > 1.0) {
      string msg = "Invalid value of Hapke Legendre bh [" +
                   IString(bh) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    p_photoBh = bh;
  }

  /**
    * Set the Hapke Legendre coefficient for the single
    * particle phase function. This is one of two coefficients
    * needed for the single particle phase function. This parameter
    * is limited to values that are >=-1 and <=1.
    *
    * @param ch  Hapke Legendre coefficient, default is 0.0
    */
  void Hapke::SetPhotoCh(const double ch) {
    if(ch < -1.0 || ch > 1.0) {
      string msg = "Invalid value of Hapke Legendre ch [" +
                   IString(ch) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    p_photoCh = ch;
  }

  /**
    * Set the Hapke single scattering albedo component.
    * This parameter is limited to values that are >0 and
    * <=1.
    *
    * @param wh  Hapke single scattering albedo component, default is 0.5
    */
  void Hapke::SetPhotoWh(const double wh) {
    if(wh <= 0.0 || wh > 1.0) {
      string msg = "Invalid value of Hapke wh [" +
                   IString(wh) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    p_photoWh = wh;
  }

  /**
    * Set the Hapke opposition surge component. This is one of
    * two opposition surge components needed for the Hapke model.
    * This parameter is limited to values that are >=0.
    *
    * @param hh  Hapke opposition surge component, default is 0.0
    */
  void Hapke::SetPhotoHh(const double hh) {
    if(hh < 0.0) {
      string msg = "Invalid value of Hapke hh [" +
                   IString(hh) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    p_photoHh = hh;
  }

  /**
    * Set the Hapke opposition surge component. This is one of
    * two opposition surge components needed for the Hapke model.
    * This parameter is limited to values that are >=0.
    *
    * @param b0  Hapke opposition surge component, default is 0.0
    */
  void Hapke::SetPhotoB0(const double b0) {
    if(b0 < 0.0) {
      string msg = "Invalid value of Hapke b0 [" +
                   IString(b0) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    p_photoB0 = b0;
  }


  /**
    * Determine if the Hapke opposition surge component is initialized
    * to zero during the SetStandardConditions phase.
    * This parameter is limited to values that are true or false.
    *
    * @param b0standard  Hapke opposition surge initialization, default is true
    */
  void Hapke::SetPhoto0B0Standard(const QString &b0standard) {
    IString temp(b0standard.toStdString());
    temp = temp.UpCase();

    if(temp != "NO" && temp != "YES" && temp != "FALSE" && temp != "TRUE") {
      string msg = "Invalid value of Hapke ZeroB0Standard [" +
                   temp + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if (temp == "NO" || temp == "FALSE") p_photo0B0Standard = "FALSE";
    if (temp == "YES" || temp == "TRUE") p_photo0B0Standard = "TRUE";
  }


  /**
    * Set the Hapke macroscopic roughness component. This parameter
    * is limited to values that are >=0 and <=90.
    *
    * @param theta  Hapke macroscopic roughness component, default is 0.0
    */
  void Hapke::SetPhotoTheta(const double theta) {
    if(theta < 0.0 || theta > 90.0) {
      string msg = "Invalid value of Hapke theta [" +
                   IString(theta) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    p_photoTheta = theta;
  }


  void Hapke::SetStandardConditions(bool standard) {
    PhotoModel::SetStandardConditions(standard);

    if(standard) {
      p_photoB0save = p_photoB0;
      if (p_photo0B0Standard == "TRUE") p_photoB0 = 0.0;
    }
    else {
      p_photoB0 = p_photoB0save;
    }
  }
}

extern "C" Isis::PhotoModel *HapkePlugin(Isis::Pvl &pvl) {
  return new Isis::Hapke(pvl);
}
