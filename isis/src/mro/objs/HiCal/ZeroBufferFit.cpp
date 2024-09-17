/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>
#include <string>
#include <vector>
#include <numeric>
#include <iostream>
#include <sstream>

#include "ZeroBufferFit.h"
#include "IString.h"
#include "HiCalTypes.h"
#include "HiCalUtil.h"
#include "HiCalConf.h"
#include "LeastSquares.h"
#include "LowPassFilter.h"
#include "MultivariateStatistics.h"
#include "IException.h"

using namespace std;

namespace Isis {

  /**
   * @brief Compute second level drift correction (Zf module)
   *
   * This class provides the second level drift correction that is
   *
   *
   * @param conf
   */
  ZeroBufferFit::ZeroBufferFit(const HiCalConf &conf) :
                             NonLinearLSQ(), Module("ZeroBufferFit") {
    DbProfile prof = conf.getMatrixProfile();
    _history.add("Profile["+ prof.Name()+"]");
    _timet.setBin(ToInteger(prof("Summing")));
    _timet.setLineTime(ToDouble(prof("ScanExposureDuration")));

    _skipFit = IsEqual(ConfKey(prof, "ZeroBufferFitSkipFit", QString("TRUE")), "TRUE");
    _useLinFit = IsTrueValue(prof, "ZeroBufferFitOnFailUseLinear");


    _absErr = ConfKey(prof, "AbsoluteError", QString("1.0E-4")).toDouble();
    _relErr = ConfKey(prof, "RelativeError", QString("1.0E-4")).toDouble();

    _sWidth = ConfKey(prof, "GuessFilterWidth", QString("17")).toInt();
    _sIters = ConfKey(prof, "GuessFilterIterations", QString("1")).toInt();

    if ( prof.exists("MaximumIterations") ) {
      setMaxIters(ToInteger(prof("MaximumIterations")));
    }

    _maxLog = ConfKey(prof, "MaximumLog", QString("709.0")).toDouble();
    _badLines = ToInteger(prof("TrimLines"))/ToInteger(prof("Summing"));
    _minLines = ConfKey(prof,"ZeroBufferFitMinimumLines", QString("100")).toInt();


    QString histstr = "ZeroBufferFit(AbsErr[" + ToString(_absErr) +
                          "],RelErr[" + ToString(_relErr) +
                          "],MaxIter[" + ToString(maxIters()) + "])";
   _history.add(histstr);

  }

  /**
   * @brief Compute non-linear fit to (typically) ZeroBufferSmooth module
   *
   * This method computes a non-linear fit to the result of the
   * ZeroBufferSmooth module. There are several things that can go wrong and
   * some config conditions that dictate behavior of this process.
   *
   * Should the image be a short exposure (i.e., not many lines) the fit will
   * not succeed so it simply skips this entire module providing the input
   * result (d) as the solution.  This wil also occur when the user has selected
   * the skip option for the module.
   *
   * A fit is attempted on the ZeroBufferSmooth data.  The non-linear solution
   * must converge within the specifed number of iterations (MaximumIterations)
   * or a polynomial fit will be used in leui of a valid solution.
   *
   * @param d   ZeroBufferSmooth data solution as input to this method
   *
   * @return HiVector Returns result of the processing
   */
  HiVector ZeroBufferFit::Solve(const HiVector &d) {
    ostringstream hist;
    _data = d;
    if ( _skipFit || (!gotGoodLines(d))) {
      _b2 = _data;
      _coefs = HiVector(4, 0.0);
      _uncert = _coefs;
      _cc = HiVector(2, 0.0);
      _chisq = 0.0;
      if ( !gotGoodLines(d) ) {
        hist << "NotEnoughLines(GoodLines[" << goodLines(d)
             << "],MinimumLines[" << _minLines << "]);";
      }

      hist << "SkipFit(TRUE: Not using LMFit)";
      _history.add(hist.str().c_str());
    }
    else {
      hist << "Fit(";
      _b2 = HiVector(goodLines(_data));
      if ( success(curvefit()) ) {
        _coefs = coefs();
        _uncert = uncert();
        hist << "Solved,#Iters[" << nIterations() << "],ChiSq[" << Chisq()
             << "],DoF[" << DoF() << "])";
        _history.add(hist.str().c_str());
        _history.add("a0("+ToString(_coefs[0])+"+-"+ToString(_uncert[0])+")");
        _history.add("a1("+ToString(_coefs[1])+"+-"+ToString(_uncert[1])+")");
        _history.add("a2("+ToString(_coefs[2])+"+-"+ToString(_uncert[2])+")");
        _history.add("a3("+ToString(_coefs[3])+"+-"+ToString(_uncert[3])+")");
      }
      else {
        //  Punt, fit a straight line to the data
        _cc = poly_fit(d);
        HiVector a(4);
        a[0] = _cc[0];
        a[1] = _cc[1];
        a[2] = 0.0;
        a[3] = 0.0;
        _coefs = a;

        hist << "Failed::Reason("<< statusstr() << "),#Iters["
             << nIterations() << "])";
        _history.add(hist.str().c_str());
        _history.add("a0("+ToString(_coefs[0])+")");
        _history.add("a1("+ToString(_coefs[1])+")");
        _history.add("a2("+ToString(_coefs[2])+")");
        _history.add("a3("+ToString(_coefs[3])+")");
        if ( _useLinFit ) {
          _history.add("OnFailureUse(LinearFit(Zf))");
        }
        else {
          _skipFit = true;
          _history.add("OnFailureUse(ZfBuffer)");
        }
      }
    }
    return (Yfit());
  }

  /**
   * @brief Compute the initial guess of the fit
   *
   * This method provides the non-linear fit with an initial guess of the
   * solution.  It involves a linear fit to the latter half of the data to
   * provide the first two coefficents, the difference of the averages of the
   * residuals at both ends of the data set and 5 times the last line time as
   * the final (fourth) element...a bit involved really.
   *
   * @return NLVector  4-element vector of the initial guess coefficients
   */
  NonLinearLSQ::NLVector ZeroBufferFit::guess()  {
    int n = _data.dim();
    int nb = n - _badLines;

    HiVector b1 = _data.subarray(0, nb-1);
    LowPassFilter gfilter(b1, _history, _sWidth, _sIters);

    int nb2 = nb/2;
    _b2 = gfilter.ref();
    HiVector cc = poly_fit(_b2.subarray(nb2,_b2.dim()-1), nb2-1);

    //  Compute the 3rd term guess by getting the average of the residual
    //  at both ends of the data set.
    Statistics s;

    //  Get the head of the data set
    int n0 = std::min(nb, 20);
    for ( int k = 0 ; k < n0 ; k++ ) {
      double d = _b2[k] - (cc[0] + cc[1] * _timet(k));
      s.AddData(&d, 1);
    }
    double head = s.Average();

    //  Get the tail of the data set
    s.Reset();
    n0 = (int) (0.9 * nb);
    for ( int l = n0 ; l < nb ; l++ ) {
      double d = _b2[l] - (cc[0] + cc[1] * _timet(l));
      s.AddData(&d, 1);
    }
    double tail = s.Average();

    //  Populate the guess with the results
    NLVector g(4, 0.0);
    g[0] = cc[0];
    g[1] = cc[1];
    g[2] = head-tail;
    g[3] = -5.0/_timet(nb-1);
    _guess = g;
    _history.add("Guess["+ToString(_guess[0])+ ","+
                          ToString(_guess[1])+ ","+
                          ToString(_guess[2])+ ","+
                          ToString(_guess[3])+ "]");
    return (g);
  }

  /**
   * @brief Computes the interation check for convergence
   *
   * @param Iter     Current iteration
   * @param fitcoefs Vector of current fit coefficients
   * @param uncerts  Uncertainties
   * @param cplxconj Complex conjugate of the current iteration
   * @param Istatus  State of current iteration
   *
   * @return int Simply passes on the Istatus value
   */
  int ZeroBufferFit::checkIteration(const int Iter, const NLVector &fitcoefs,
                     const NLVector &uncerts, double cplxconj,
                     int Istatus) {
      _chisq = pow(cplxconj, 2.0);
      return (Istatus);
  }

  /** Computes the function value at the current iteration */
  NonLinearLSQ::NLVector ZeroBufferFit::f_x(const NLVector &a) {
    double a0 = a[0];
    double a1 = a[1];
    double a2 = a[2];
    double a3 = a[3];

    int n = _b2.dim();
    NLVector f(n);
    for (int i = 0 ; i < n ; i++) {
        double lt = _timet(i);
        double et = a3 * lt;
        double Yi = a0 + (a1 * lt) + a2 * exp(std::min(et,_maxLog));
        f[i] = (Yi - _b2[i]);
    }
    return (f);
  }

  /** Computes the first derivative of the function at the current iteration */
  NonLinearLSQ::NLMatrix ZeroBufferFit::df_x(const NLVector &a) {
    // double a0 = a[0];
    // double a1 = a[1];
    double a2 = a[2];
    double a3 = a[3];

    int n = _b2.dim();
    NLMatrix J(n, 4);
    for (int i = 0; i < n; i++) {
      double lt = _timet(i);
      double et = a3 * lt;
      double p0 = exp (std::min(et,_maxLog));
      J[i][0] = 1.0;
      J[i][1] = lt;
      J[i][2] = p0;
      J[i][3] = a2 * lt * p0;
    }
    return (J);
  }

  /** Computes the solution vector using current coefficents */
  HiVector ZeroBufferFit::Yfit() const {
    if ( _skipFit || (!gotGoodLines(_data))) {
      return (_data.copy());
    }
    else {
      HiVector dcorr(_data.dim());
      HiVector a = _coefs;
      for ( int i = 0 ; i < dcorr.dim() ; i++ ) {
        double lt = _timet(i);
        dcorr[i] = a[0] + (a[1] * lt) + a[2] * exp(a[3] * lt);
      }
      return (dcorr);
    }
  }

  /** Compute normalized solution vector from result */
  HiVector ZeroBufferFit::Normalize(const HiVector &v) {

    HiVector vNorm(v.dim());
    double v0 = v[0];
    for ( int i = 0 ; i < v.dim() ; i++ ) {
      vNorm[i] = v[i] - v0;
    }
    _history.add("Normalize[" + ToString(v0) + "]");
    return (vNorm);
  }

  /**
   * @brief Compute a polyonomial fit using multivariate statistics
   *
   * Used as a fallback solution, this method computes a linear statistical
   * solution from the linear regression analysis of the multivariate statistics
   * of the data.
   *
   * @param d     Data vector to fit
   * @param line0 Current line number in the image
   *
   * @return HiVector Returns the fitted data
   */
  HiVector ZeroBufferFit::poly_fit(const HiVector &d, const double line0) const {
    //  Needs a linear fit to latter half of data
    MultivariateStatistics fit;
    int n = d.dim();
    for ( int i = 0 ; i < n ; i++ ) {
      double t = _timet(line0+i);
      fit.AddData(&t, &d[i], 1);
    }
    NLVector cc(2);
    fit.LinearRegression(cc[0], cc[1]);
    return (cc);
  }

  /** Provides virtualized dump of data from this module */
  void ZeroBufferFit::printOn(std::ostream &o) const {
    o << "#  History = " << _history << endl;
    //  Write out the header
    o << setw(_fmtWidth) << "Line"
      << setw(_fmtWidth+1) << "Time"
      << setw(_fmtWidth+1) << "Data"
      << setw(_fmtWidth+1) << "Fit\n";

    HiVector fit = Yfit();
    for (int i = 0 ; i < _data.dim() ; i++) {
      o << formatDbl(i).toStdString() << " "
        << formatDbl(_timet(i)).toStdString() << " "
        << formatDbl(_data[i]).toStdString() << " "
        << formatDbl(fit[i]).toStdString() << endl;
    }
    return;
  }

}     // namespace Isis
