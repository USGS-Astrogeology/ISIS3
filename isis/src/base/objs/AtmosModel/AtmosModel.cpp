/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <cmath>
#include <string>
#include "Pvl.h"
#include "IString.h"
#include "AtmosModel.h"
#include "NumericalApproximation.h"
#include "NumericalAtmosApprox.h"
#include "PhotoModel.h"
#include "Minnaert.h"
#include "LunarLambert.h"
#include "Plugin.h"
#include "IException.h"
#include "FileName.h"

using namespace std;

#define MAX(x,y) (((x) > (y)) ? (x) : (y))
namespace Isis {
  /**
   * Create an AtmosModel object.  Because this is a pure virtual
   * class you can not create an AtmosModel class directly.  Instead,
   * see the @b Isis::AtmosModelFactory class.
   *
   * @param pvl  @b Isis::Pvl object containing a valid AtmosModel
   *             specification
   * @param pmodel PhotoModel object containing valud AtmosModel
   *               data
   *
   * @see atmosphericModels.doc
   * @internal
   *   @history 2008-11-05 Jeannie Walldren - Replaced arrays with
   *          vectors
   *
   */
  AtmosModel::AtmosModel(Pvl &pvl, PhotoModel &pmodel) {
    p_atmosAlgorithmName = "Unknown";
    p_atmosPM = &pmodel;

    p_atmosIncTable.resize(91);
    p_atmosAhTable.resize(91);
    p_atmosHahgtTable.resize(91);
    p_atmosHahgt0Table.resize(91);
    p_atmosAb = 0.0;
    p_atmosCosphi = 0.0;
    p_atmosAtmSwitch = 0;
    p_atmosEulgam = 0.5772156;
    p_atmosHahgsb = 0.0;
    p_atmosHga = 0.68;
    p_atmosInc = 0.0;
    p_atmosMunot = 0.0;
    p_atmosNinc = 91;
    p_atmosPhi = 0.0;
    p_atmosSini = 0.0;
    p_atmosTau = 0.28;
    p_atmosTauref = 0.0;
    p_atmosTauold = -1.0;
    p_atmosWha = 0.95;
    p_atmosWhaold = 1.0e30;
    p_atmosBha = 0.85;
    p_atmosHnorm = 0.003;
    p_pstd = 0.0;
    p_sbar = 0.0;
    p_trans = 0.0;
    p_trans0 = 0.0;
    p_transs = 0.0;
    p_standardConditions = false;

    PvlGroup &algorithm = pvl.findObject("AtmosphericModel").findGroup("Algorithm", Pvl::Traverse);

    if(algorithm.hasKeyword("Nulneg")) {
      SetAtmosNulneg(algorithm["Nulneg"][0] == "YES");
    }
    else {
      p_atmosNulneg = false;
    }

    if(algorithm.hasKeyword("Tau")) {
      SetAtmosTau(algorithm["Tau"]);
    }
    p_atmosTausave = p_atmosTau;

    if(algorithm.hasKeyword("Tauref")) {
      SetAtmosTauref(algorithm["Tauref"]);
    }

    if(algorithm.hasKeyword("Wha")) {
      SetAtmosWha(algorithm["Wha"]);
    }
    p_atmosWhasave = p_atmosWha;

    if(algorithm.hasKeyword("Hga")) {
      SetAtmosHga(algorithm["Hga"]);
    }
    p_atmosHgasave = p_atmosHga;

    if(algorithm.hasKeyword("Bha")) {
      SetAtmosBha(algorithm["Bha"]);
    }
    p_atmosBhasave = p_atmosBha;

    if(algorithm.hasKeyword("Inc")) {
      SetAtmosInc(algorithm["Inc"]);
    }

    if(algorithm.hasKeyword("Phi")) {
      SetAtmosPhi(algorithm["Phi"]);
    }

    if(algorithm.hasKeyword("Hnorm")) {
      SetAtmosHnorm(algorithm["Hnorm"]);
    }

    if(algorithm.hasKeyword("Iord")) {
      SetAtmosIord(algorithm["Iord"][0] == "YES");
    }
    else {
      p_atmosAddOffset = false;
    }

    if(algorithm.hasKeyword("EstTau")) {
      SetAtmosEstTau(algorithm["EstTau"][0] == "YES");
    }
    else {
      p_atmosEstTau = false;
    }
  }

 /**
   * Perform Chandra and Van de Hulst's series approximation
   * for the g'11 function needed in second order scattering
   * theory.
   *
   * @param tau  normal optical depth of atmosphere
   * @returns @b double Value of the g'11 function evaluated
   *        at the given @b tau
   * @internal
   *   @history 1998-12-21 Randy Kirk, USGS - Flagstaff - Original
   *            code in Isis2 pht_am_functions
   *   @history 1999-03-12 K Teal Thompson - Port to Unix/ISIS;
   *            declare vars; cadd implicit none.
   *   @history 2007-02-20 Janet Barrett - Imported from Isis2 to
   *            Isis3 in NumericalMethods class.
   *   @history 2008-11-05 Jeannie Walldren - Moved this method from
   *           NumericalMethods class.
   *
   */
  double AtmosModel::G11Prime(double tau) {
    double sum;
    int icnt;
    double fac;
    double term;
    double tol;
    double fi;
    double elog;
    double eulgam;
    double e1_2;
    double result;

    tol = 1.0e-6;
    eulgam = 0.5772156;

    sum = 0.0;
    icnt = 1;
    fac = -tau;
    term = fac;
    while(fabs(term) > fabs(sum)*tol) {
      sum = sum + term;
      icnt = icnt + 1;
      fi = (double) icnt;
      fac = fac * (-tau) / fi;
      term = fac / (fi * fi);
    }
    elog = log(MAX(1.0e-30, tau)) + eulgam;
    e1_2 = sum + PI * PI / 12.0 + 0.5 *
           pow(elog, 2.0);
    result = 2.0 * (AtmosModel::En(1, tau)
                    + elog * AtmosModel::En(2, tau)
                    - tau * e1_2);
    return(result);
  }

  /**
   * This routine computes the exponential integral,
   * @a Ei(x). This is defined as
   * @f[
   *    Ei(x) = - \int_{-x}^{\infty}
   *    \frac{e^{-t}}{t}\mathrm{d}t
   *    = \int_{-\infty}^{x}
   *    \frac{e^{t}}{t}\mathrm{d}t
   * @f]
   * for @a x > 0.
   *
   * For small @a x, this method uses the power series,
   * @f[
   *    Ei(x) = \gamma + \ln x + \frac{x}{1*1!} + \frac{x^2}{2*2!}
   *    + ...
   * @f] where @f$ \gamma = 0.5772156649... @f$ is Euler's
   * constant, and for large @a x, the asymptotic series is
   * used,
   * @f[
   *    Ei(x) \sim \frac{e^x}{x} (1 + \frac{1!}{x} +
   *    \frac{2!}{x^2} + ...)
   * @f]
   *
   * @see mathworld.wolfram.com/ExponentialIntegral.html

   * @param x  Value at which the exponential integral will be
   *           computed, @a x > 0.0
   * @returns @b double The exponential integral for the given
   *        @a x.
   * @throws Isis::IException::Programmer "Invalid arguments.
   *             Definition requires x > 0.0."
   * @throws Isis::IException::Math "Power series failed to
   *             converge"
   * @internal
   *   @history 1999-08-11 K Teal Thompson - Original
   *               version named pht_r8ei in Isis2.
   *   @history 2007-02-20 Janet Barrett - Imported to Isis3 in
   *            NumericalMethods class named r8ei().
    *  @history 2008-11-05 Jeannie Walldren - Renamed and modified
    *           input parameters.  Added documentation.
   */
  double AtmosModel::Ei(double x) {
    //static double r8ei(double x) in original NumericalMethods class
    // This method was derived from an algorithm in the text
    // Numerical Recipes in C: The Art of Scientific Computing
    // Section 6.3 by Flannery, Press, Teukolsky, and Vetterling
    double result;
    double fpmin; // close to smallest representable floating-point number
    double euler; // Euler's constant, lower-case gamma
    double epsilon;   // desired relative error (tolerance)
    int maxit;    // maximum number of iterations allowed
    double sum, fact, term, prev;

    fpmin = 1.0e-30;
    maxit = 100;
    epsilon = 6.0e-8;
    euler = 0.57721566;

    if(x <= 0.0) {
      throw IException(IException::Programmer,
                       "AtmosModel::Ei() - Invalid argument. Definition requires x > 0.0. Entered x = "
                       + IString(x),
                       _FILEINFO_);
    }
    if(x < fpmin) {  //special case: avoid failure of convergence test because underflow
      result = log(x) + euler;
    }
    else if(x <= -log(epsilon)) {  //Use power series
      sum = 0.0;
      fact = 1.0;
      for(int k = 1; k <= maxit; k++) {
        fact = fact * x / k;
        term = fact / k;
        sum = sum + term;
        if(term < epsilon * sum) {
          result = sum + log(x) + euler;
          return(result);
        }
      }
      throw IException(IException::Unknown,
                       "AtmosModel::Ei() - Power series failed to converge in "
                       + IString(maxit) + " iterations. Unable to calculate exponential integral.",
                       _FILEINFO_);
    }
    else { // Use asymptotic series
      sum = 0.0;
      term = 1.0;
      for(int k = 1; k <= maxit; k++) {
        prev = term;
        term = term * k / x;
        if(term < epsilon) {
          result = exp(x) * (1.0 + sum) / x;
          return(result);
        }
        else {
          if(term < prev) {  // still converging: add new term
            sum = sum + term;
          }
          else { // diverging: subtract previous term and exit
            sum = sum - prev;
            result = exp(x) * (1.0 + sum) / x;
            return(result);
          }
        }
      }
      result = exp(x) * (1.0 + sum) / x;
    }
    return(result);
  }

  /**
   * This routine evaluates the generalized exponential integral,
   * @a E<sub>n</sub>(x). This is defined as
   * @f[
   *    E_n(x) = \int_{1}^{\infty} \frac{e^{-xt}}{t^n}\mathrm{d}t
   *       \mathrm{ for } x \in \mathbb{R }, n \in \mathbb{Z }
   *           \mathrm{such that } x \ge 0.0, n \ge 0
   * @f]
   * Notice that if @a x = 0, then the equation is not defined
   * for @a n = 0 or @a n = 1, as these will result in @f$
   * E_n(x) = \infty @f$.
   *
   * This method uses the following for cases:
   * <UL>
   *   <LI>For n > 1 and x = 0.0,
   *   @f[
   *      E_n(0) = \frac{1}{n-1}
   *   @f]
   *
   *   <LI>For n = 0 and x > 0.0,
   *   @f[
   *      E_0(x) = \frac{e^{-x}}{x}
   *   @f]
   *
   *   <LI>For @f$ x >\sim 1.0 @f$, Lentz's continued fraction
   *   algorithm is used,
   *   @f[
   *      E_n(x) = e^{-x}(\frac{1}{x + n - \frac{1*n}{x + n + 2 -
   *      \frac{2(n+1)}{x + n + 4 - ...}}})
   *   @f]
   *
   *   <LI>For @f$ 0.0 < x <\sim 1.0 @f$, series representation is
   *   used,
   *   @f[
   *      E_n(x) = \frac{(-x)^{n-1}}{(n-1)!}[- \ln x + \psi (n)] -
   *      \sum_{m=0, m \neq n-1}^{\infty}\frac{(-x)^m}{(m-n+1)m!}
   *   @f] where @f$ \psi (1) = \gamma @f$ and @f$ \psi (n) = -
   *   \gamma + \sum_{m=1}^{n-1} \frac{1}{m}@f$ and @f$ \gamma =
   *   0.5772156649... @f$ is Euler's constant.
   * </UL>
   *
   * The parameter @a n is of type @b unsigned @b int since @a n
   * cannot be negative.
   *
   * The routine allows fast evaluation of
   * @a E<sub>n</sub>(x) to any accuracy, @f$\epsilon@f$,
   * within the reach of your machine's word length for
   * floating-point numbers. The only modification required for
   * increased accuracy is to supply Euler's constant with enough
   * significant digits.
   *
   * @see mathworld.wolfram.com/En-Function.html
   *
   * @param n  Integer value at which the exponential integral
   *           will be evaluated (n >= 0)
   * @param x  The exponential integral En(x) will be evaluated
   * @returns @b double Value of the exponential integral for
   *        the give n and x.
   * @throws Isis::IException::Programmer "Invalid arguments.
   *             Definition requires (x > 0.0 and n >=0 ) or (x >=
   *             0.0 and n > 1)."
   * @throws Isis::IException::Math "Continued fraction failed to
   *             converge"
   * @throws Isis::IException::Math "Series representation failed to converge"
   * @internal
   *   @history 1999-08-10 K Teal Thompson - Original version in
   *            named pht_r8expint Isis2.
   *   @history 2007-02-20 Janet Barrett - Imported to Isis3 in
   *            NumericalMethods class named r8expint().
   *   @history 2008-11-05 Jeannie Walldren - Renamed and modified
   *            input parameters.  Added documentation.
   */
  double AtmosModel::En(unsigned int n, double x) {
    //static double r8expint(int n, double x) in original NumericalMethods class
    // This method was derived from an algorithm in the text
    // Numerical Recipes in C: The Art of Scientific Computing
    // Section 6.3 by Flannery, Press, Teukolsky, and Vetterling
    int nm1;
    double result;
    double a, b, c, d, h;
    double delta;
    double fact;
    double psi;
    double fpmin;  // close to smallest representable floating-point number
    double maxit;  // maximum number of iterations allowed
    double epsilon;    // desired relative error (tolerance)
    double euler;  // Euler's constant, gamma

    fpmin = 1.0e-30;
    maxit = 100;
    epsilon = 1.0e-7;
    euler = 0.5772156649;

    nm1 = n - 1;
    if((x < 0.0) || (x == 0.0 && (n == 0 || n == 1))) {
      IString msg = "AtmosModel::En() - Invalid arguments. ";
      msg += "Definition requires (x > 0.0 and n >=0 ) or (x >= 0.0 and n > 1). ";
      msg += "Entered x = " + IString(x) + " and n = " + IString((int) n);
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    else if(n == 0) {  // special case, this implies x > 0 by logic above
      result = exp(-x) / x;
    }
    else if(x == 0.0) {  // special case, this implies n > 1
      result = 1.0 / nm1;
    }
    else if(x > 1.0) {  // Lentz's algorithm, this implies n > 0
      b = x + n;
      c = 1.0 / fpmin;
      d = 1.0 / b;
      h = d;
      for(int i = 1; i <= maxit; i++) {
        a = -i * (nm1 + i);
        b = b + 2.0;
        d = 1.0 / (a * d + b);
        c = b + a / c;
        delta = c * d;
        h = h * delta;
        if(fabs(delta - 1.0) < epsilon) {
          result = h * exp(-x);
          return(result);
        }
      }
      throw IException(IException::Unknown,
                       "AtmosModel::En() - Continued fraction failed to converge in "
                       + IString(maxit) + " iterations. Unable to calculate exponential integral.",
                       _FILEINFO_);
    }
    else { // evaluate series
      if(nm1 != 0) {
        result = 1.0 / nm1;
      }
      else {
        result = -log(x) - euler;
      }
      fact = 1.0;
      for(int i = 1; i <= maxit; i++) {
        fact = -fact * x / i;
        if(i != nm1) {
          delta = -fact / (i - nm1);
        }
        else {
          psi = -euler;
          for(int ii = 1; ii <= nm1; ii++) {
            psi = psi + 1.0 / ii;
          }
          delta = fact * (-log(x) + psi);
        }
        result = result + delta;
        if(fabs(delta) < fabs(result)*epsilon) {
          return(result);
        }
      }
      throw IException(IException::Unknown,
                       "AtmosModel::En() - Series representation failed to converge in "
                       + IString(maxit) + " iterations. Unable to calculate exponential integral.",
                       _FILEINFO_);
    }
    return(result);
  }

  /**
   * Calculate the atmospheric scattering effect using photometric angle
   * information
   *
   * @param pha  phase angle
   * @param inc  incidence angle
   * @param ema  emission angle
   * @param pstd pure atmospheric-scattering term
   * @param trans transmission of surface reflected light through
   *              the atmosphere overall
   * @param trans0 transmission of surface reflected light
   *        through the atmosphere with no scatterings in the
   *        atmosphere
   * @param sbar illumination of the ground by the sky
   * @throw Isis::IException::User "Invalid photometric angles"
   */
  void AtmosModel::CalcAtmEffect(double pha, double inc, double ema,
                                 double *pstd, double *trans, double *trans0, double *sbar,
                                 double *transs) {

    // Check validity of photometric angles
    //if (pha > 180.0 || inc > 90.0 || ema > 90.0 || pha < 0.0 ||
    //    inc < 0.0 || ema < 0.0) {
    //  string msg = "Invalid photometric angles";
    //  throw IException::Message(IException::Programmer,msg,_FILEINFO_);
    //}

    // Apply atmospheric function
    AtmosModelAlgorithm(pha, inc, ema);
    *pstd = p_pstd;
    *trans = p_trans;
    *trans0 = p_trans0;
    *sbar = p_sbar;
    *transs = p_transs;
  }

  /**
   * Used to calculate atmosphere at standard conditions
   */
  void AtmosModel::SetStandardConditions(bool standard) {
    p_standardConditions = standard;
    if(p_standardConditions) {
      p_atmosTausave = p_atmosTau;
      p_atmosTau = p_atmosTauref;
    }
    else {
      p_atmosTau = p_atmosTausave;
    }
  }

  /**
   * This method computes the values of the atmospheric Ah
   * table and sets the properties of the atmospheric Ah spline.
   * It obtains the hemispheric albedo by integrating the
   * photometric function times mu over mu = 0 to 1 and then over
   * phi = 0 to 2*pi to calculate the hemispheric reflectance Ah
   * needed for the photometric model with atmosphere. The
   * Trapezoid rule is applied to the table of Ah to obtain
   * bihemispheric albedo Ab. The parameter @a p_atmosAtmSwitch is
   * set to 0 to integrate Ah.
   *
   * @internal
   *   @history 1998-12-21 Randy Kirk - USGS, Flagstaff - Original
   *            specs
   *   @history 1999-01-15 K Teal Thompson - Original code
   *   @history 2000-12-29 Randy Kirk - Modified /hide_inc/ so phi
   *            gets passed, etc. moved factors to outside
   *            integration
   *   @history 2006-05-30 Randy Kirk - Added code to report the
   *            directional hemispheric albedo that relates to
   *            thermal balance for simulating Themis images
   *   @history 2007-02-20 Janet Barrett - Imported to Isis3 from
   *            Isis2 pht_get_ah_table
   *   @history 2008-11-05 Jeannie Walldren - Modified references to
   *            NumericalMethods and replaced r8qromb() with
   *            NumericalAtmosApprox::RombergsMethod().  Changed
   *            name from PhtGetAhTable().
   */
  void AtmosModel::GenerateAhTable() {
    int inccnt;     //for loop incidence angle count, 1:ninc
    double fun_temp;//temporary variable for integral
    double factor;  //factor for integration: 1 except at ends of interval where it's 1/2
    double yp1, yp2; //first derivatives of first and last x values of @a p_atmosIncTable

    NumericalAtmosApprox::IntegFunc sub;
    sub = NumericalAtmosApprox::OuterFunction;

    p_atmosAb = 0.0;

    //Create NumericalAtmosApprox object here for RombergsMethod used in for loop
    NumericalAtmosApprox qromb;

    for(inccnt = 0; inccnt < p_atmosNinc; inccnt++) {
      p_atmosInc = (double) inccnt;
      p_atmosIncTable[inccnt] = p_atmosInc;
      p_atmosMunot = cos((PI / 180.0) * p_atmosInc);
      p_atmosSini = sin((PI / 180.0) * p_atmosInc);

      IString phtName = p_atmosPM->AlgorithmName().toStdString();
      phtName.UpCase();
      if(p_atmosInc == 90.0) {
        p_atmosAhTable[inccnt] = 0.0;
      }
      else if(phtName == "LAMBERT") {
        p_atmosAhTable[inccnt] = 1.0;
      }
      else if(phtName == "LOMMELSEELIGER") {
        p_atmosAhTable[inccnt] = 2.0 * log((1.0 / p_atmosMunot) / p_atmosMunot);
      }
      else if(phtName == "MINNAERT") {
        p_atmosAhTable[inccnt] = (pow(p_atmosMunot, ((Minnaert *)p_atmosPM)->PhotoK())) / ((Minnaert *)p_atmosPM)->PhotoK();
      }
      else if(phtName == "LUNARLAMBERT") {
        p_atmosAhTable[inccnt] = 2.0 * ((LunarLambert *)p_atmosPM)->PhotoL() *
                                 log((1.0 + p_atmosMunot) / p_atmosMunot) + 1.0 -
                                 ((LunarLambert *)p_atmosPM)->PhotoL();
      }
      else {
        // numerically integrate the other photometric models
        // outer integral is over phi from 0 to pi rad = 180 deg
        p_atmosAtmSwitch = 0;
        // integrate AtmosModel function from 0 to 180
        fun_temp = qromb.RombergsMethod(this, sub, 0, 180);
        // the correct normalization with phi in degrees is:
        p_atmosAhTable[inccnt] = fun_temp / (90.0 * p_atmosMunot);
      }
      // Let's get our estimate of Ab by integrating (summing)
      // A (i)sinicosi over our A  table
      if((inccnt == 0) || (inccnt == p_atmosNinc - 1)) {
        factor = 0.5;
      }
      else {
        factor = 1.0;
      }

      p_atmosAb = p_atmosAb + p_atmosAhTable[inccnt] * p_atmosMunot * p_atmosSini * factor;
    }

    factor = 2.0 * PI / 180.0;
    p_atmosAb = p_atmosAb * factor;

    // set up clamped cubic spline
    yp1 = 1.0e30;
    yp2 = 1.0e30;
    p_atmosAhSpline.Reset();
    p_atmosAhSpline.SetInterpType(NumericalApproximation::CubicClamped);
    p_atmosAhSpline.AddData(p_atmosIncTable, p_atmosAhTable);
    p_atmosAhSpline.SetCubicClampedEndptDeriv(yp1, yp2);
  }

  /**
   * This method computes the values of the atmospheric Hahg and
   * Hahg0 tables and sets the properties of the atmospheric Hahg
   * and Hahg0 splines. It integrates functions involving the
   * single particle phase function (assumed to be Hapke
   * Henyey-Greenstein) over mu = 0 to 1 and then over phi = 0 to
   * 2*pi to calculate the corrections needed for the anisotropic
   * photometric model with atmosphere. The Trapezoid rule is
   * applied to the table of Ah to obtain bihemispheric albedo Ab.
   * The parameter @a p_atmosAtmSwitch is set to 1, 2, 3 to
   * evaluate the 3 required integrals.
   *
   * @internal
   *   @history 1998-12-21 Randy Kirk - USGS, Flagstaff - Original
   *            specs
   *   @history 1999-01-15 K Teal Thompson - Original code
   *   @history 2006-07-07 Randy Kirk - Modify get_ah_table to get
   *            other integrals
   *   @history 2000-12-29 Randy Kirk - Modified /hide_inc/ so phi
   *            gets passed, etc. moved factors to outside
   *            integration
   *   @history 2007-02-20 Janet Barrett - Imported to Isis3 from
   *            Isis2 pht_get_ah_table
   *   @history 2008-11-05 Jeannie Walldren - Modified references to
   *            NumericalMethods and replaced r8qromb() with
   *            NumericalAtmosApprox::RombergsMethod(). Changed
   *            name from GetHahgTables().
   */
  void AtmosModel::GenerateHahgTables() {
    int inccnt;         // for loop incidence angle count,1:ninc
    double fun_temp;    // temporary variable for integral
    double hahgsb_temp; // save increment to hahgsb
    double factor;      // factor for integration: 1 except at ends of interval where it's 1/2
    double yp1, yp2;    // derivatives of endpoints of data set

    NumericalAtmosApprox::IntegFunc sub;
    sub = NumericalAtmosApprox::OuterFunction;

    p_atmosHahgsb = 0.0;
    NumericalAtmosApprox qromb;

    for(inccnt = 0; inccnt < p_atmosNinc; inccnt++) {
      p_atmosInc = (double) inccnt;
      p_atmosIncTable[inccnt] = p_atmosInc;
      p_atmosMunot = cos((PI / 180.0) * p_atmosInc);
      p_atmosSini = sin((PI / 180.0) * p_atmosInc);

      p_atmosAtmSwitch = 1;

      qromb.Reset();
      fun_temp = qromb.RombergsMethod(this, sub, 0, 180);

      p_atmosHahgtTable[inccnt] = fun_temp * AtmosWha() / 360.0;
      p_atmosAtmSwitch = 2;

      fun_temp = qromb.RombergsMethod(this, sub, 0, 180);

      hahgsb_temp = fun_temp * AtmosWha() / 360.0;

      if((inccnt == 0) || (inccnt == p_atmosNinc - 1)) {
        factor = 0.5;
      }
      else {
        factor = 1.0;
      }

      p_atmosHahgsb = p_atmosHahgsb + p_atmosSini * factor * hahgsb_temp;
      if(p_atmosInc == 0.0) {
        p_atmosHahgt0Table[inccnt] = 0.0;
      }
      else {
        p_atmosAtmSwitch = 3;
        fun_temp = qromb.RombergsMethod(this, sub, 0, 180);
        p_atmosHahgt0Table[inccnt] = fun_temp * AtmosWha() * p_atmosMunot / (360.0 * p_atmosSini);
      }
    }

    factor = 2.0 * PI / 180.0;
    p_atmosHahgsb = p_atmosHahgsb * factor;

    // set up clamped cubic splines
    yp1 = 1.0e30;
    yp2 = 1.0e30;
    p_atmosHahgtSpline.Reset();
    p_atmosHahgtSpline.SetInterpType(NumericalApproximation::CubicClamped);
    p_atmosHahgtSpline.AddData(p_atmosIncTable, p_atmosHahgtTable);
    p_atmosHahgtSpline.SetCubicClampedEndptDeriv(yp1, yp2);

    p_atmosHahgt0Spline.Reset();
    p_atmosHahgt0Spline.SetInterpType(NumericalApproximation::CubicClamped);
    p_atmosHahgt0Spline.AddData(p_atmosIncTable, p_atmosHahgt0Table);
    p_atmosHahgt0Spline.SetCubicClampedEndptDeriv(yp1, yp2);
  }

  /**
   * This method is a modified version of the GenerateHahgTables
   * method and is used solely for shadow modeling. Unlike the
   * GenerateHahgTables method, this method does not tabulate the
   * first or third integrals. It only evaluates the middle
   * integral that corrects the sbar variable (illumination of
   * the ground by the sky).
   *
   * @internal
   *   @history 1998-12-21 Randy Kirk - USGS, Flagstaff - Original
   *            specs
   *   @history 1999-01-15 K Teal Thompson - Original code
   *   @history 2006-07-07 Randy Kirk - Modify get_ah_table to get
   *            other integrals
   *   @history 2000-09-15 Randy Kirk - Delete first, last integrals
   *            and table of second integral
   *   @history 2000-12-29 Randy Kirk - Modified /hide_inc/ so phi
   *            gets passed, etc. moved factors to outside
   *            integration
   *   @history 2011-12-16 Janet Barrett - Imported to Isis3 from
   *            Isis2 get_hahgsb_shad routine in the not_shadow_tau
   *            set of routines
   */
  void AtmosModel::GenerateHahgTablesShadow() {
    int inccnt;         // for loop incidence angle count,1:ninc
    double fun_temp;    // temporary variable for integral
    double factor;      // factor for integration: 1 except at ends of interval where it's 1/2
    int nincl = 19;     // used instead of p_atmosNinc
    double deltaInc;    // limits table size and increment used

    NumericalAtmosApprox::IntegFunc sub;
    sub = NumericalAtmosApprox::OuterFunction;

    p_atmosHahgsb = 0.0;
    NumericalAtmosApprox qromb;
    deltaInc = 90.0/(double)(nincl-1.0);

    for(inccnt = 0; inccnt < nincl; inccnt++) {
      p_atmosInc = deltaInc * (double) inccnt;
      p_atmosMunot = cos((PI / 180.0) * p_atmosInc);
      p_atmosSini = sin((PI / 180.0) * p_atmosInc);

      p_atmosAtmSwitch = 2;

      qromb.Reset();
      if (p_atmosInc >= 90.0) {
        fun_temp = 0.0;
      } else {
        fun_temp = qromb.RombergsMethod(this, sub, 0, 180);
      }

      if((inccnt == 0) || (inccnt == nincl - 1)) {
        factor = 0.5;
      }
      else {
        factor = 1.0;
      }
      p_atmosHahgsb = p_atmosHahgsb + p_atmosSini * factor * fun_temp *
        AtmosWha() / 360.0;
    }

    factor = 2.0 * deltaInc * PI / 180.0;
    p_atmosHahgsb = p_atmosHahgsb * factor;
  }

  /**
  * Set the switch that controls the function that will be
  * integrated. This method is only used for testing the
  * methods in this class. This parameter is limited to the
  * values 0, 1, 2, and 3.
  *
  * @param atmswitch  Internal atmospheric function parameter,
  *                   there is no default
  * @throw Isis::IException::User "Invalid value of atmospheric
  *        atmswitch"
  */
  void AtmosModel::SetAtmosAtmSwitch(const int atmswitch) {
    if(atmswitch < 0 || atmswitch > 3) {
      string msg = "Invalid value of atmospheric atmswitch [" + IString(atmswitch) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    p_atmosAtmSwitch = atmswitch;
  }

  /**
   * Set the Anisotropic Atmospheric function parameter. This is
   * the coefficient of the single particle Legendre phase
   * function. This parameter is limited to values that are >=-1
   * and <=1.
   *
   * @param bha  Anisotropic atmospheric function parameter,
   *             default is 0.85
   * @throw Isis::IException::User "Invalid value of atmospheric
   *        bha"
   */
  void AtmosModel::SetAtmosBha(const double bha) {
    if(bha < -1.0 || bha > 1.0) {
      string msg = "Invalid value of Anisotropic atmospheric bha [" +
                   IString(bha) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    p_atmosBha = bha;
  }

  /**
   * Set the Hapke atmospheric function parameter. This is the
   * coefficient of the single particle Henyey-Greenstein phase
   * function. This parameter is limited to values that are >-1
   * and <1.
   *
   * @param hga  Hapke atmospheric function parameter,
   *             default is 0.68
   * @throw Isis::IException::User "Invalid value of atmospheric
   *        hga"
   */
  void AtmosModel::SetAtmosHga(const double hga) {
    if(hga <= -1.0 || hga >= 1.0) {
      string msg = "Invalid value of Hapke atmospheric hga [" + IString(hga) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    p_atmosHga = hga;
  }

  /**
   * Set the incidence angle. This method is only used for
   * testing the methods in this class. This parameter is
   * limited to values >=0 and <=90.
   *
   * @param inc  Internal atmospheric function parameter,
   *             there is no default
   * @throw Isis::IException::User "Invalid value of atmospheric
   *        inc"
   */
  void AtmosModel::SetAtmosInc(const double inc) {
    if(inc < 0.0 || inc > 90.0) {
      string msg = "Invalid value of atmospheric inc [" + IString(inc) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    p_atmosInc = inc;
    p_atmosMunot = cos(inc * PI / 180.0);
    p_atmosSini = sin(inc * PI / 180.0);
  }

  /**
   * Set the Atmospheric function parameter. This determines
   * if negative values after removal of atmospheric effects
   * will be set to NULL. This parameter is limited to values
   * of YES or NO.
   *
   * @param nulneg  Atmospheric function parameter, default is NO
   * @throw Isis::IException::User "Invalid value of atmospheric
   *        nulneg"
   *
   */
  void AtmosModel::SetAtmosNulneg(const string nulneg) {
    IString temp(nulneg);
    temp = temp.UpCase();

    if(temp != "NO" && temp != "YES") {
      string msg = "Invalid value of Atmospheric nulneg [" + temp + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    SetAtmosNulneg(temp.compare("YES") == 0);
  }

  /**
   * Set the azimuth angle. This method is only used for
   * testing the methods in this class. This parameter is
   * limited to values >=0 and <=360.
   *
   * @param phi  Internal atmospheric function parameter,
   *             there is no default
   * @throw Isis::IException::User "Invalid value of atmospheric
   *        phi"
   *
   */
  void AtmosModel::SetAtmosPhi(const double phi) {
    if(phi < 0.0 || phi > 360.0) {
      string msg = "Invalid value of atmospheric phi [" + IString(phi) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    p_atmosPhi = phi;
    p_atmosCosphi = cos(phi * PI / 180.0);
  }

  /**
   * Set the Atmospheric function parameter. This specifies the
   * normal optical depth of the atmosphere. This parameter is
   * limited to values that are >=0.
   *
   * @param tau  Atmospheric function parameter, default is 0.28
   * @throw Isis::IException::User "Invalid value of atmospheric
   *        tau"
   */
  void AtmosModel::SetAtmosTau(const double tau) {
    if(tau < 0.0) {
      string msg = "Invalid value of Atmospheric tau [" + IString(tau) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    p_atmosTau = tau;
  }

  /**
   * Set the Atmospheric function parameter. This specifies the
   * reference optical depth of the atmosphere to which the image
   * will be normalized. This parameter is limited to values that
   * are >=0.
   *
   * @param tauref  Atmospheric function parameter, default is 0.0
   * @throw Isis::IException::User "Invalid value of atmospheric
   *        tauref"
   */
  void AtmosModel::SetAtmosTauref(const double tauref) {
    if(tauref < 0.0) {
      string msg = "Invalid value of Atmospheric tauref [" + IString(tauref) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    p_atmosTauref = tauref;
  }

  /**
   * Set the Atmospheric function parameter. This is the single-
   * scattering albedo of atmospheric particles. This parameter
   * is limited to values that are >0 and <=1.
   *
   * @param wha  Atmospheric function parameter, default is 0.95
   * @throw Isis::IException::User "Invalid value of atmospheric
   *        wha"
   *
   */
  void AtmosModel::SetAtmosWha(const double wha) {
    if(wha <= 0.0 || wha > 1.0) {
      string msg = "Invalid value of Atmospheric wha [" + IString(wha) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    p_atmosWha = wha;
  }

  /**
   * Checks whether tau or wha have changed.
   * @return @b bool True if either have changed.
   */
  bool AtmosModel::TauOrWhaChanged() const {
    return (AtmosTau() != p_atmosTauold) || (AtmosWha() != p_atmosWhaold);
  }

  /**
   * Set the Atmospheric function parameter. This is the
   * atmospheric shell thickness normalized to the planet radius
   * and is used to modify angles to get more accurate path
   * lengths near the terminator (ratio of scale height to the
   * planetary radius). This parameter is limited to values that
   * are >=0.
   *
   * @param hnorm  Atmospheric function parameter, default is 0.003
   */
  void AtmosModel::SetAtmosHnorm(const double hnorm) {
    if(hnorm < 0.0) {
      std::string msg = "Invalid value of Atmospheric hnorm [" + toString(hnorm) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    p_atmosHnorm = hnorm;
  }

  /**
   * Set additive offset in fit
   *
   * @author Sharmila Prasad (8/18/2011)
   *
   * @param offset true/false
   */
  void AtmosModel::SetAtmosIord(const string offset) {
    IString temp(offset);
    temp = temp.UpCase();

    if(temp != "NO" && temp != "YES") {
      string msg = "Invalid value of Atmospheric additive offset[" + temp + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    SetAtmosIord(temp.compare("YES") == 0);
  }

  /**
   * Estimate the optical depth tau using shadows
   *
   * @author Janet Barrett (12/16/2011)
   *
   * @param esttau true/false
   */
  void AtmosModel::SetAtmosEstTau(const string esttau) {
    IString temp(esttau);
    temp = temp.UpCase();

    if(temp != "NO" && temp != "YES") {
      string msg = "Invalid value of Atmospheric optical depth estimation[" + temp + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    SetAtmosEstTau(temp.compare("YES") == 0);
  }
}
