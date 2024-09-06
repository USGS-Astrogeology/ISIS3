/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>
#include "Anisotropic2.h"
#include "AtmosModel.h"
#include "Constants.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "IException.h"
#include "IString.h"

using std::min;
using std::max;

namespace Isis {
  /**
   * Empty constructor
   * 
   * @param pvl  Pvl object containing a valid Anisotropic2 specification
   * @param pmodel PhotoModel object containing valid Anisotropic2 data
   */
  Anisotropic2::Anisotropic2(Pvl &pvl, PhotoModel &pmodel) : AtmosModel(pvl, pmodel) {
  }

  
  /**
   * Anisotropic atmospheric scattering with P1 single-particle
   * phase fn, in the second approximation.  This subroutine goes
   * through much of the derivation twice, once for the axisymmetric
   * (m=0) and once for the m=1 parts of scattered light.
   *
   * @param phase Value of the phase angle.
   * @param incidence Value of the incidence angle.
   * @param emission Value of the emission angle.
   *
   * @history 1998-12-21 Randy Kirk - USGS, Flagstaff - Original
   *          code
   * @history 1999-03-12 K Teal Thompson  Port to Unix/ISIS;
   *          declare vars; add implicit none.
   * @history 2007-02-20 Janet Barrett - Imported from Isis2
   *          pht_atm_functions to Isis3.
   * @history 2008-11-05 Jeannie Walldren - Replaced
   *          NumericalMethods::r8expint() with AtmosModel::En(),
   *          NumericalMethods::G11Prime() with
   *          AtmosModel::G11Prime(), and NumericalMethods::r8ei()
   *          with AtmosModel::Ei(). Replaced Isis::PI with PI
   *          since this is in Isis namespace.
   * @history 2011-12-19 Janet Barrett - Added code to estimate the
   *          shadow brightness value (transs). Also got rid of
   *          unnecessary check for identical photometric angle values
   *          between successive calls. This check should only be
   *          made in the photometric models.
   */
  void Anisotropic2::AtmosModelAlgorithm(double phase, double incidence, double emission) {
    double xx;
    double munot, mu;
    double emunot, emu;
    double munotp, mup;
    double gmunot, gmu;
    double hpsq1;
    double f1munot, f2munot, f3munot;
    double f1mmunot, f2mmunot, f3mmunot;
    double maxval;
    double f1mu, f2mu, f3mu;
    double f1mmu, f2mmu, f3mmu;
    double sum;
    double prod;
    double cosazss;
    double xystuff;
    double xmunot_0, ymunot_0;
    double xmu_0, ymu_0;
    double cxx, cyy;
    double xmunot_1, ymunot_1;
    double xmu_1, ymu_1;

    if(p_atmosBha == 0.0) {
      p_atmosBha = 1.0e-6;
    }

    if(p_atmosTau == 0.0) {
      p_pstd = 0.0;
      p_trans = 1.0;
      p_trans0 = 1.0;
      p_sbar = 0.0;
      p_transs = 1.0;
      return;
    }

    if(p_atmosWha == 1.0) {
      std::string msg = "Anisotropic conservative case not implemented yet - WHA parameter cannot be set to 1.0.";
      msg += "This will cause negative planetary curvature to occur.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if(TauOrWhaChanged()) {
      // preparation includes exponential integrals e sub 2 through 5
      p_wha2 = 0.5 * p_atmosWha;
      p_wham = 1.0 - p_atmosWha;
      p_e1   = AtmosModel::En(1, p_atmosTau);
      p_e1_2 = AtmosModel::En(1, 2.0 * p_atmosTau);
      p_e2   = AtmosModel::En(2, p_atmosTau);
      p_e3   = AtmosModel::En(3, p_atmosTau);
      p_e4   = AtmosModel::En(4, p_atmosTau);
      p_e5   = AtmosModel::En(5, p_atmosTau);

      // chandra's gmn functions require fm and fn at mu=-1
      xx = -p_atmosTau;
      if(xx < -69.0) {
        p_em = 0.0;
      }
      else if(xx > 69.0) {
        p_em = 1.0e30;
      }
      else {
        p_em = exp(xx);
      }

      p_f1m = log(2.0) - p_em * p_e1 + p_e1_2;
      p_f2m = -1.0 * (p_f1m + p_em * p_e2 - 1.0);
      p_f3m = -1.0 * (p_f2m + p_em * p_e3 - 0.5);
      p_f4m = -1.0 * (p_f3m + p_em * p_e4 - (1.0 / 3.0));
      p_g12 = (p_atmosTau * p_e1 * p_e2 + p_f1m + p_f2m) * 0.5;
      p_g13 = (p_atmosTau * p_e1 * p_e3 + p_f1m + p_f3m) * (1.0 / 3.0);
      p_g14 = (p_atmosTau * p_e1 * p_e4 + p_f1m + p_f4m) * 0.25;
      p_g32 = (p_atmosTau * p_e3 * p_e2 + p_f3m + p_f2m) * 0.25;
      p_g33 = (p_atmosTau * p_e3 * p_e3 + p_f3m + p_f3m) * 0.2;
      p_g34 = (p_atmosTau * p_e3 * p_e4 + p_f3m * p_f4m) * (1.0 / 6.0);

      // chandra's g'mn functions require g'11 and f at mu=+1
      xx = p_atmosTau;
      if(xx < -69.0) {
        p_e = 0.0;
      }
      else if(xx > 69.0) {
        p_e = 1.0e30;
      }
      else {
        p_e = exp(xx);
      }

      p_f1 = Eulgam() + log(p_atmosTau) + p_e * p_e1;
      p_f2 = p_f1 + p_e * p_e2 - 1.0;
      p_f3 = p_f2 + p_e * p_e3 - 0.5;
      p_f4 = p_f3 + p_e * p_e4 - (1.0 / 3.0);
      p_g11p = AtmosModel::G11Prime(p_atmosTau);
      p_g12p = (p_atmosTau * (p_e1 - p_g11p) + p_em * (p_f1 + p_f2)) * 0.25;
      p_g13p = (p_atmosTau * (0.5 * p_e1 - p_g12p) + p_em * (p_f1 + p_f3)) * 0.2;
      p_g14p = (p_atmosTau * ((1.0 / 3.0) * p_e1 - p_g13p) + p_em * (p_f1 + p_f4)) * (1.0 / 6.0);
      p_g32p = (p_atmosTau * (p_e1 - p_g13p) + p_em * (p_f3 + p_f2)) * (1.0 / 6.0);
      p_g33p = (p_atmosTau * (0.5 * p_e1 - p_g32p) + p_em * (p_f3 + p_f3)) * 0.142857;
      p_g34p = (p_atmosTau * ((1.0 / 3.0) * p_e1 - p_g33p) + p_em * (p_f3 + p_f4)) * 0.125;

      // first, get the required quantities for the axisymmetric m=0 part
      // zeroth moments of (uncorrected) x and y times characteristic fn
      p_x0_0 = p_wha2 * (1.0 + (1.0 / 3.0) * p_atmosBha * p_wham + p_wha2 * (p_g12 + p_atmosBha *
                         p_wham * (p_g14 + p_g32) + pow(p_atmosBha, 2.0) * pow(p_wham, 2.0) * p_g34));
      p_y0_0 = p_wha2 * (p_e2 + p_atmosBha * p_wham * p_e4 + p_wha2 * (p_g12p
                         + p_atmosBha * p_wham * (p_g14p + p_g32p) +
                         pow(p_atmosBha, 2.0) * pow(p_wham, 2.0) * p_g34p));

      // higher-order correction term for x and y
      p_delta_0 = (1.0 - (p_x0_0 + p_y0_0) - (1.0 - p_atmosWha * (1.0 + (1.0 / 3.0) * p_atmosBha *
                                              p_wham)) / (1.0 - (p_x0_0 - p_y0_0))) / (p_atmosWha * (0.5 - p_e3 + p_atmosBha * p_wham * (0.25 - p_e5)));

      //  moments of (corrected) x and y
      p_alpha0_0 = 1.0 + p_wha2 * (p_g12 + p_atmosBha * p_wham * p_g32) + p_delta_0 * (0.5 - p_e3);
      p_alpha1_0 = 0.5 + p_wha2 * (p_g13 + p_atmosBha * p_wham * p_g33) + p_delta_0 * ((1.0 / 3.0) - p_e4);
      p_beta0_0 = p_e2 + p_wha2 * (p_g12p + p_atmosBha * p_wham * p_g32p) + p_delta_0 * (0.5 - p_e3);
      p_beta1_0 = p_e3 + p_wha2 * (p_g13p + p_atmosBha * p_wham * p_g33p) + p_delta_0 * ((1.0 / 3.0) - p_e4);

      // gamma will be a weighted sum of m=0 x and y functions
      // but unlike before, call the weights q1 and p1, and we also
      // need additional weights q0 and p0
      p_fac = 2.0 - p_atmosWha * p_alpha0_0;
      p_den = pow(p_fac, 2.0) - pow((p_atmosWha * p_beta0_0), 2.0);
      p_q0 = p_atmosBha * p_atmosWha * p_wham * (p_fac * p_alpha1_0 - p_atmosWha * p_beta0_0 * p_beta1_0) / p_den;
      p_p0 = p_atmosBha * p_atmosWha * p_wham * (-p_fac * p_beta1_0 - p_atmosWha * p_beta0_0 * p_alpha1_0) / p_den;
      p_q02p02 = p_q0 * p_q0 - p_p0 * p_p0;
      p_q1 = (2.0 * p_wham * p_fac) / p_den;
      p_p1 = (2.0 * p_wham * p_atmosWha * p_beta0_0) / p_den;
      p_q12p12 = p_q1 * p_q1 - p_p1 * p_p1;

      // sbar is total diffuse illumination and comes from moments
      p_sbar = 1.0 - 2.0 * (p_q1 * p_alpha1_0 + p_p1 * p_beta1_0);

      // we're not done yet!  still have to calculate the m=1 portion
      // zeroth moments of (uncorrected) x and y times characteristic fn
      p_x0_1 = 0.5 * p_wha2 * p_atmosBha * (1.0 - (1.0 / 3.0) + 0.5 * p_wha2 * p_atmosBha *
                                            (p_g12 - (p_g14 + p_g32) + p_g34));
      p_y0_1 = 0.5 * p_wha2 * p_atmosBha * (p_e2 - p_e4 + 0.5 * p_wha2 * p_atmosBha *
                                            (p_g12p - (p_g14p + p_g32p) + p_g34p));

      // higher-order correction term for x and y
      p_delta_1 = (1.0 - (p_x0_1 + p_y0_1) - (1.0 - (1.0 / 3.0) * p_atmosWha * p_atmosBha) /
                   (1.0 - (p_x0_1 - p_y0_1))) / (p_wha2 * p_atmosBha *
                       ((0.5 - 0.25) - (p_e3 - p_e5)));

      // moments of (corrected) x and y are not needed for m=1, so we're done
      SetOldTau(p_atmosTau);
      SetOldWha(p_atmosWha);
    }

    // correct the path lengths for planetary curvature
    hpsq1 = pow((1.0 + p_atmosHnorm), 2.0) - 1.0;

    if(incidence == 90.0) {
      munot = 0.0;
    }
    else {
      munot = cos((PI / 180.0) * incidence);
    }

    maxval = max(1.0e-30, hpsq1 + munot * munot);
    munotp = p_atmosHnorm / (sqrt(maxval) - munot);
    munotp = max(munotp, p_atmosTau / 69.0);

    if(emission == 90.0) {
      mu = 0.0;
    }
    else {
      mu = cos((PI / 180.0) * emission);
    }

    maxval = max(1.0e-30, hpsq1 + mu * mu);
    mup = p_atmosHnorm / (sqrt(maxval) - mu);
    mup = max(mup, p_atmosTau / 69.0);

    // build the x and y functions of mu0 and mu
    maxval = max(1.0e-30, munotp);
    xx = -p_atmosTau / maxval;
    if(xx < -69.0) {
      emunot = 0.0;
    }
    else if(xx > 69.0) {
      emunot = 1.0e30;
    }
    else {
      emunot = exp(-p_atmosTau / munotp);
    }

    maxval = max(1.0e-30, mup);
    xx = -p_atmosTau / maxval;
    if(xx < -69.0) {
      emu = 0.0;
    }
    else if(xx > 69.0) {
      emu = 1.0e30;
    }
    else {
      emu = exp(-p_atmosTau / mup);
    }

    // in the second approximation the x and y include the f1, f3 functions
    xx = munotp;
    if(fabs(xx - 1.0) < 1.0e-10) {
      f1munot = p_f1;
      f1mmunot = xx * (log(1.0 + 1.0 / xx) - p_e1 * emunot +
                       AtmosModel::En(1, p_atmosTau * (1.0 + 1.0 / xx)));
    }
    else if(xx > 0.0) {
      f1munot = xx * (log(xx / (1.0 - xx)) + p_e1 / emunot +
                      AtmosModel::Ei(p_atmosTau * (1.0 / xx - 1.0)));
      f1mmunot = xx * (log(1.0 + 1.0 / xx) - p_e1 * emunot +
                       AtmosModel::En(1, p_atmosTau * (1.0 + 1.0 / xx)));
    }
    else {
      std::string msg = "Negative length of planetary curvature encountered";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    f2munot = munotp * (f1munot + p_e2 / emunot - 1);
    f2mmunot = -munotp * (f1mmunot + p_e2 * emunot - 1);
    f3munot = munotp * (f2munot + p_e3 / emunot - 0.5);
    f3mmunot = -munotp * (f2mmunot + p_e3 * emunot - 0.5);
    xx = mup;

    if(fabs(xx - 1.0) < 1.0e-10) {
      f1mu = p_f1;
      f1mmu = xx * (log(1.0 + 1.0 / xx) - p_e1 * emu + AtmosModel::En(1, p_atmosTau * (1.0 + 1.0 / xx)));
    }
    else if(xx > 0.0) {
      f1mu = xx * (log(xx / (1.0 - xx)) + p_e1 / emu + AtmosModel::Ei(p_atmosTau * (1.0 / xx - 1.0)));
      f1mmu = xx * (log(1.0 + 1.0 / xx) - p_e1 * emu + AtmosModel::En(1, p_atmosTau * (1.0 + 1.0 / xx)));
    }
    else {
      std::string msg = "Negative length of planetary curvature encountered";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    f2mu = mup * (f1mu + p_e2 / emu - 1);
    f2mmu = -mup * (f1mmu + p_e2 * emu - 1);
    f3mu = mup * (f2mu + p_e3 / emu - 0.5);
    f3mmu = -mup * (f2mmu + p_e3 * emu - 0.5);

    // first for m=0
    xmunot_0 = 1.0 + p_wha2 * (f1mmunot + p_atmosBha * p_wham * f3mmunot) + p_delta_0 * munotp * (1.0 - emunot);
    ymunot_0 = emunot * (1.0 + p_wha2 * (f1munot + p_atmosBha * p_wham * f3munot)) +
               p_delta_0 * munotp * (1.0 - emunot);
    xmu_0 = 1.0 + p_wha2 * (f1mmu + p_atmosBha * p_wham * f3mmu) + p_delta_0 * mup * (1.0 - emu);
    ymu_0 = emu * (1.0 + p_wha2 * (f1mu + p_atmosBha * p_wham * f3mu)) + p_delta_0 * mup * (1.0 - emu);

    // then for m=1
    xmunot_1 = 1.0 + 0.5 * p_wha2 * p_atmosBha * (f1mmunot - f3mmunot) + p_delta_1 * munotp * (1.0 - emunot);
    ymunot_1 = emunot * (1.0 + 0.5 * p_wha2 * p_atmosBha *
                         (f1munot - f3munot)) + p_delta_1 * munotp * (1.0 - emunot);
    xmu_1 = 1.0 + 0.5 * p_wha2 * p_atmosBha * (f1mmu - f3mmu) + p_delta_1 * mup * (1.0 - emu);
    ymu_1 = emu * (1.0 + 0.5 * p_wha2 * p_atmosBha * (f1mu - f3mu)) + p_delta_1 * mup * (1.0 - emu);

    // gamma1 functions come from x and y with m=0
    gmunot = p_p1 * xmunot_0 + p_q1 * ymunot_0;
    gmu = p_p1 * xmu_0 + p_q1 * ymu_0;

    // purely atmos term uses x and y of both orders and is complex
    sum = munot + mu;
    prod = munot * mu;
    cxx = 1.0 - p_q0 * sum + (p_q02p02 - p_atmosBha * p_q12p12) * prod;
    cyy = 1.0 + p_q0 * sum + (p_q02p02 - p_atmosBha * p_q12p12) * prod;

    if(phase == 90.0) {
      cosazss = 0.0 - munot * mu;
    }
    else {
      cosazss = cos((PI / 180.0) * phase) - munot * mu;
    }

    xystuff = cxx * xmunot_0 * xmu_0 - cyy * ymunot_0 *
              ymu_0 - p_p0 * sum * (xmu_0 * ymunot_0 + ymu_0 * xmunot_0) + cosazss * p_atmosBha * (xmu_1 *
                  xmunot_1 - ymu_1 * ymunot_1);
    p_pstd = 0.25 * p_atmosWha * munotp / (munotp + mup) * xystuff;

    // xmitted surface term uses gammas from m=0
    p_trans = gmunot * gmu;

    // finally, never-scattered term is given by pure attenuation
    p_trans0 = emunot * emu;

    // Calculate the transmission of light that must be subtracted from a
    // shadow. This includes direct flux and the scattered flux in the
    // upsun half of the sky downwelling onto the surface, and the usual
    // transmission upward. NOTE: We need to derive the analytic expression
    // for the light from half the sky in the Legendre scattering model. Until
    // we do so, we are setting the shadow transmission to the purely
    // unscattered part (same as trans0). This will give a result but is
    // not fully consistent with how the other scattering models are
    // implemented.
    p_transs = p_trans0;
  }
}


extern "C" Isis::AtmosModel *Anisotropic2Plugin(Isis::Pvl &pvl,
    Isis::PhotoModel &pmodel) {

  return new Isis::Anisotropic2(pvl, pmodel);
}
