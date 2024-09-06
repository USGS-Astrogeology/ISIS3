/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <cmath>
#include "Anisotropic1.h"
#include "AtmosModel.h"
#include "IException.h"
#include "IString.h"

using std::max;

namespace Isis {
  /**
   * Constructs an Anisotropic1 object
   *
   * @param pvl  Pvl object containing a valid Anisotropic1 specification
   * @param pmodel PhotoModel object containing valid Anisotropic1 data
   */
  Anisotropic1::Anisotropic1(Pvl &pvl, PhotoModel &pmodel) : AtmosModel(pvl, pmodel) {

    // Set default values
    p_atmosAlpha0_0 = 0.0;
    p_atmosAlpha1_0 = 0.0;
    p_atmosBeta0_0 = 0.0;
    p_atmosBeta1_0 = 0.0;
    p_atmosDelta_0 = 0.0;
    p_atmosDelta_1 = 0.0;
    p_atmosDen = 0.0;
    p_atmosE2 = 0.0;
    p_atmosE3 = 0.0;
    p_atmosE4 = 0.0;
    p_atmosE5 = 0.0;
    p_atmosFac = 0.0;
    p_atmosP0 = 0.0;
    p_atmosP1 = 0.0;
    p_atmosQ0 = 0.0;
    p_atmosQ02p02 = 0.0;
    p_atmosQ1 = 0.0;
    p_atmosQ12p12 = 0.0;
    p_atmosWha2 = 0.0;
    p_atmosWham = 0.0;
    p_atmosX0_0 = 0.0;
    p_atmosX0_1 = 0.0;
    p_atmosY0_0 = 0.0;
    p_atmosY0_1 = 0.0;

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
   *          pht_atm_functions to Isis.
   * @history 2008-11-05 Jeannie Walldren - Replaced
   *          NumericalMethods::r8expint() with AtmosModel::En().
   *          Replaced Isis::PI with PI since this is in Isis
   *          namespace.
   * @history 2011-12-19 Janet Barrett - Added code to estimate the
   *          shadow brightness value (transs). Also got rid of
   *          unnecessary check for identical photometric angle values
   *          between successive calls. This check should only be
   *          made in the photometric models.
   */
  void Anisotropic1::AtmosModelAlgorithm(double phase, double incidence, double emission) {
    double hpsq1;
    double munot, munotp;
    double maxval;
    double mu, mup;
    double xx;
    double emunot, emu;
    double gmunot, gmu;
    double sum, prod;
    double cosazss;
    double xmunot_0, ymunot_0;
    double xmu_0, ymu_0;
    double xmunot_1, ymunot_1;
    double xmu_1, ymu_1;
    double cxx, cyy;
    double xystuff;

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
      std::string msg = "Anisotropic conservative case not implemented yet - WHA parameter cannot be set to 1.0";
      msg += "This will cause negative planetary curvature to occur.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if(TauOrWhaChanged()) {
      // preparation includes exponential integrals e sub 2 through 5
      p_atmosWha2 = 0.5 * p_atmosWha;
      p_atmosWham = 1.0 - p_atmosWha;
      p_atmosE2 = AtmosModel::En(2, p_atmosTau);
      p_atmosE3 = AtmosModel::En(3, p_atmosTau);
      p_atmosE4 = AtmosModel::En(4, p_atmosTau);
      p_atmosE5 = AtmosModel::En(5, p_atmosTau);
      // first, get the required quantities for the axisymmetric m=0 part
      // zeroth moments of (uncorrected) x and y times characteristic fn
      p_atmosX0_0 = p_atmosWha2 * (1.0 + (1.0 / 3.0) * p_atmosBha *
                                   p_atmosWham);
      p_atmosY0_0 = p_atmosWha2 * (p_atmosE2 + p_atmosBha * p_atmosWham *
                                   p_atmosE4);
      // higher-order correction term for x and y
      p_atmosDelta_0 = (1.0 - (p_atmosX0_0 + p_atmosY0_0) - (1.0 - p_atmosWha *
                        (1.0 + (1.0 / 3.0) * p_atmosBha * p_atmosWham)) / (1.0 -
                            (p_atmosX0_0 - p_atmosY0_0))) / (p_atmosWha * (0.5 - p_atmosE3 +
                                p_atmosBha * p_atmosWham * (0.25 - p_atmosE5)));

      // moments of (corrected) x and y
      p_atmosAlpha0_0 = 1.0 + p_atmosDelta_0 * (0.5 - p_atmosE3);
      p_atmosAlpha1_0 = 0.5 + p_atmosDelta_0 * ((1.0 / 3.0) - p_atmosE4);
      p_atmosBeta0_0 = p_atmosE2 + p_atmosDelta_0 * (0.5 - p_atmosE3);
      p_atmosBeta1_0 = p_atmosE3 + p_atmosDelta_0 * ((1.0 / 3.0) - p_atmosE4);
      // gamma will be a weighted sum of m=0 x and y functions
      // but unlike before, call the weights q1 and p1, and we also
      // need additional weights q0 and p0
      p_atmosFac = 2.0 - p_atmosWha * p_atmosAlpha0_0;
      p_atmosDen = pow(p_atmosFac, 2.0) - pow(p_atmosWha * p_atmosBeta0_0, 2.0);
      p_atmosQ0 = p_atmosBha * p_atmosWha * p_atmosWham * (p_atmosFac *
                  p_atmosAlpha1_0 - p_atmosWha * p_atmosBeta0_0 * p_atmosBeta1_0) /
                  p_atmosDen;
      p_atmosP0 = p_atmosBha * p_atmosWha * p_atmosWham * (-p_atmosFac *
                  p_atmosBeta1_0 - p_atmosWha * p_atmosBeta0_0 * p_atmosAlpha1_0) /
                  p_atmosDen;
      p_atmosQ02p02 = p_atmosQ0 * p_atmosQ0 - p_atmosP0 * p_atmosP0;
      p_atmosQ1 = (2.0 * p_atmosWham * p_atmosFac) / p_atmosDen;
      p_atmosP1 = (2.0 * p_atmosWham * p_atmosWha * p_atmosBeta0_0) /
                  p_atmosDen;
      p_atmosQ12p12 = p_atmosQ1 * p_atmosQ1 - p_atmosP1 * p_atmosP1;
      // sbar is total diffuse illumination and comes from moments
      p_sbar = 1.0 - 2.0 * (p_atmosQ1 * p_atmosAlpha1_0 + p_atmosP1 *
                            p_atmosBeta1_0);
      // we're not done yet!  still have to calculate the m=1 portion
      // zeroth moments of (uncorrected) x and y times characteristic fn
      p_atmosX0_1 = 0.5 * p_atmosWha2 * p_atmosBha * (1.0 - (1.0 / 3.0));
      p_atmosY0_1 = 0.5 * p_atmosWha2 * p_atmosBha * (p_atmosE2 - p_atmosE4);
      // higher-order correction term for x and y
      p_atmosDelta_1 = (1.0 - (p_atmosX0_1 + p_atmosY0_1) - (1.0 -
                        (1.0 / 3.0) * p_atmosWha * p_atmosBha) / (1.0 - (p_atmosX0_1 -
                            p_atmosY0_1))) / (p_atmosWha2 * p_atmosBha * ((0.5 - 0.25) -
                                              (p_atmosE3 - p_atmosE5)));
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

    // first for m=0
    xmunot_0 = 1.0 + p_atmosDelta_0 * munotp * (1.0 - emunot);
    ymunot_0 = emunot + p_atmosDelta_0 * munotp * (1.0 - emunot);
    xmu_0 = 1.0 + p_atmosDelta_0 * mup * (1.0 - emu);
    ymu_0 = emu + p_atmosDelta_0 * mup * (1.0 - emu);

    // then for m=1
    xmunot_1 = 1.0 + p_atmosDelta_1 * munotp * (1.0 - emunot);
    ymunot_1 = emunot + p_atmosDelta_1 * munotp * (1.0 - emunot);
    xmu_1 = 1.0 + p_atmosDelta_1 * mup * (1.0 - emu);
    ymu_1 = emu + p_atmosDelta_1 * mup * (1.0 - emu);

    // gamma1 functions come from x and y with m=0
    gmunot = p_atmosP1 * xmunot_0 + p_atmosQ1 * ymunot_0;
    gmu = p_atmosP1 * xmu_0 + p_atmosQ1 * ymu_0;

    // purely atmos term uses x and y of both orders and is complex
    sum = munot + mu;
    prod = munot * mu;
    cxx = 1.0 - p_atmosQ0 * sum + (p_atmosQ02p02 - p_atmosBha *
                                   p_atmosQ12p12) * prod;
    cyy = 1.0 + p_atmosQ0 * sum + (p_atmosQ02p02 - p_atmosBha *
                                   p_atmosQ12p12) * prod;

    if(phase == 90.0) {
      cosazss = 0.0 - munot * mu;
    }
    else {
      cosazss = cos((PI / 180.0) * phase) - munot * mu;
    }

    xystuff = cxx * xmunot_0 * xmu_0 - cyy * ymunot_0 *
              ymu_0 - p_atmosP0 * sum * (xmu_0 * ymunot_0 + ymu_0 *
                                         xmunot_0) + cosazss * p_atmosBha * (xmu_1 *
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

extern "C" Isis::AtmosModel *Anisotropic1Plugin(Isis::Pvl &pvl, Isis::PhotoModel &pmodel) {
  return new Isis::Anisotropic1(pvl, pmodel);
}
