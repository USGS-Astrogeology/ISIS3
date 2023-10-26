/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Chip.h"
#include "GruenTypes.h"
#include "Gruen.h"
#include "DbProfile.h"
#include "Pvl.h"

#include "tnt/tnt_array1d_utils.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <QTime>


using namespace std;

namespace Isis {

  /**
   * @brief Default constructor sets up default Gruen parameters
   *
   * @author Kris Becker - 5/22/2011
   */
  Gruen::Gruen() : AutoReg(Gruen::getDefaultParameters()) {
    init(Gruen::getDefaultParameters());
  }

  /**
   * @brief Construct a Gruen search algorithm
   *
   * This will construct a minimum difference search algorith.  It is
   * recommended that you use a AutoRegFactory class as opposed to
   * this constructor.  Direct construction is used commonly in stereo
   * matching.
   *
   * @param pvl  A Pvl object that contains a valid automatic registration
   * definition
   */
  Gruen::Gruen(Pvl &pvl) : AutoReg(pvl) {
    init(pvl);
  }

  /**
   * @brief Set up for writing subsearch for a a given registration call
   *
   * This method is provided to request the write of the subsearch chip at each
   * iteration.  This must be invoked prior to every call to AutoReg::Register()
   * method.  It will only write subchips from the Register() interface as it
   * interates to a solution.  Direct calls to Gruen methods don't iterate in
   * the same fashion.
   *
   * The "pattern" parameter is optional but is provided to direct the location
   * and naming convention of each subsearch chip.  The format for the output
   * file name for each subsearch chip is comprised of the pattern parameter,
   * call number, which can be retrieved by the CallCount() method and pertains
   * to the call after the Register() method is invoked and the interation
   * count.  Below is a code example:
   *
   * @code
   *
   * Gruen gruen(myPvldef);
   *
   * // Set up pattern and search chips here
   *
   * gruen.WriteSubsearchChips("/mydata/subchip");
   * gruen.Register();
   *
   * @endcode
   *
   * Note that prior to each call to Register(), it must be called again in
   * order for the subchips to be written.  The last part of the parameter
   * above, "subchip", cannot be a directory, but is a filename prefix.
   * Assuming this is the first call to Register(), a series of cube subsearch
   * chips will be written with the pattern
   * "/work1/kbecker/subchipC000001IXXX.cub" where "C" indicates call count and
   * the next 6 digits are the return of CallCount() method, "I" indicates the
   * iteration count "XXX" of the algorithm() method.  Note that the chip
   * written for a particular iteration is what is provided as a parameter into
   * the algorithm() method.
   *
   * @param pattern Specifies an optional directory and file pattern to write
   *                the subsearch chip at each algorithm iteration.
   */
  void Gruen::WriteSubsearchChips(const QString &pattern) {
    m_filePattern = pattern;
    return;
  }

  /**
   * @brief Sets initial chip transformation
   *
   * This method can be used with AutoReg registration to set initial affine
   * transform parameters.  This initial condition will be applied to the whole
   * search chip extraction for the first subsearch chip.  The caller must
   * define the contents of the affine and radiometric parameters. See the
   * AffineRadio construct for details.
   *
   * @author kbecker (5/14/2011)
   *
   * @param affrad Initial Affine/Radio parameters to apply on registration
   *               entry
   */
  void Gruen::setAffineRadio(const AffineRadio &affrad) {
    m_affine = affrad;
    return;
  }

  /**
   * @brief Set affine parameters to defaults
   *
   * This method differs from the one above in that it uses the defaults as
   * defined at construction.  The basic difference is that this call sets the
   * affine portion to the identity and the radiometric parameters to the
   * defaults as provided in the user input auto-regististration parameters.
   * It may have default shift and gain values to use.
   *
   * @author Kris Becker - 6/4/2011
   */
  void Gruen::setAffineRadio() {
    m_affine = getDefaultAffineRadio();
  }

  /**
   * @brief Real workhorse of the computational Gruen algorithm
   *
   * This method is called for all registration requests and actually performs
   * the registration of two chips.
   *
   * The pattern chip is deemed constant.  The subsearch chip is generally an
   * extraction from the search chip that has had an affine transform applied to
   * fill it.
   *
   * At each iteration of the Gruen algorithm, the affine transform is
   * incrementally updated based upon the results from this method.  There are
   * six affine parameters and two radiometric (shift and gain) parameters that
   * are solved/computed here.
   *
   * The algorithm itself is a first derivative computation of the subsearch
   * chip with small radiometric adjustments applied to better tone match the
   * two chips.  This is intended to minimize the affine variability.
   *
   * @param pattern   Fixed pattern chip which subsearch is trying to match
   * @param subsearch Affined extraction of the search chip
   *
   * @return Returns 0 if successful, otherwise returns the error number
   *         associated with the problem encountered (see ErrorTypes)
   */
  int Gruen::algorithm(Chip &pattern, Chip &subsearch,
                       const Radiometric &radio, BigInt &ptsUsed,
                       double &resid, GMatrix &atai, AffineRadio &affrad) {

    m_totalIterations++;  //  Bump iteration counter

    // Initialize loop variables
    int tackSamp = pattern.TackSample();
    int tackLine = pattern.TackLine();

    //  Internal variables
    double rshift = radio.Shift();
    double rgain  = radio.Gain();

    int maxPnts((pattern.Samples()-2) * (pattern.Lines()-2));
    GMatrix a(maxPnts, 8);
    GVector lvec(maxPnts);

    //  pattern chip is rh image , subsearch chip is lh image
    resid = 0.0;
    int npts = 0;
    for(int line = 2 ; line < pattern.Lines() ; line++) {
      for(int samp = 2; samp < pattern.Samples() ; samp++) {

        if(!pattern.IsValid(samp, line)) continue;
        if(!subsearch.IsValid(samp, line)) continue;
        if(!subsearch.IsValid(samp + 1, line)) continue;
        if(!subsearch.IsValid(samp - 1, line)) continue;
        if(!subsearch.IsValid(samp, line - 1)) continue;
        if(!subsearch.IsValid(samp, line + 1)) continue;

        //  Sample/Line numbers
        double x0 = (double)(samp - tackSamp);
        double y0 = (double)(line - tackLine);

        // Discrete derivatives (delta sample #)
        double gxtemp = subsearch.GetValue(samp + 1, line) -
                        subsearch.GetValue(samp - 1, line);
        double gytemp = subsearch.GetValue(samp, line + 1) -
                        subsearch.GetValue(samp, line - 1);

        a[npts][0] = gxtemp;
        a[npts][1] = gxtemp * x0;
        a[npts][2] = gxtemp * y0;
        a[npts][3] = gytemp;
        a[npts][4] = gytemp * x0;
        a[npts][5] = gytemp * y0;
        a[npts][6] = 1.0;
        a[npts][7] = subsearch.GetValue(samp, line);

        double ell = pattern.GetValue(samp, line) -
                     (((1.0 + rgain) * subsearch.GetValue(samp, line)) +
                      rshift);

        lvec[npts] = ell;
        resid += (ell * ell);
        npts++;
      }
    }

    // Check for enough points
    ptsUsed = npts;
    if(!ValidPoints(maxPnts, npts)) {
      std::ostringstream mess;
      mess << "Minimum points (" << MinValidPoints(maxPnts)
           << ") criteria not met (" << npts << ")";
      return (logError(NotEnoughPoints, mess.str().c_str()));
    }

    // Create the ATA array
    GMatrix ata(8,8);
    for(int i = 0 ; i < 8 ; i++) {
      for(int j = 0 ; j < 8 ; j++) {
        double temp(0.0);
        for (int k = 0 ; k < npts ; k++) {
          temp += (a[k][i] * a[k][j]);
        }
        ata[i][j] = temp;
      }
    }

    // Solve the ATAI with Cholesky
    try {
      GVector p(8);
      atai = Choldc(ata, p);
      GMatrix b = Identity(8);
      GMatrix x = b;
      atai = Cholsl(atai, p, b, x);
    }
    catch(IException &ie) {
      QString mess = "Cholesky Failed:: " + ie.toString();
      return (logError(CholeskyFailed, mess));
    }

    // Compute the affine update if result are requested by caller.
    GVector atl(8);
    for (int i = 0 ; i < 8 ; i++) {
      double temp(0.0);
      for (int k = 0 ; k < npts ; k++) {
        temp += a[k][i] * lvec[k];
      }
      atl[i] = temp;
    }

    GVector alpha(8, 0.0);
    for (int i = 0 ; i < 8 ; i++) {
      for (int k = 0 ; k < 8 ; k++) {
        alpha[i] += atai[i][k] * atl[k];
      }
    }

   try {
     affrad = AffineRadio(alpha);
    }
    catch(IException &ie) {
      QString mess = "Affine failed: " + ie.toString();
      return (logError(AffineNotInvertable, mess));
    }

    return (0);
  }

  /**
   * @brief Compute the error analysis of convergent Gruen matrix
   *
   * @author kbecker (5/15/2011)
   *
   * @param npts
   * @param resid
   * @param atai
   *
   * @return Analysis
   */
  Analysis Gruen::errorAnalysis(const BigInt &npts, const double &resid,
                                const GMatrix &atai) {

    Analysis results;
    results.m_npts = npts;

    // Converged, compute error anaylsis
    double variance = resid / DegreesOfFreedom(npts);
    GMatrix kmat(8,8);
    for(int r = 0 ; r < 8 ; r++) {
      for(int c = 0 ; c < 8 ; c++) {
        kmat[r][c] = variance * atai[r][c];
      }
    }
    results.m_variance = variance;


    // Set up submatrix
    GMatrix skmat(2,2);
    skmat[0][0] = kmat[0][0];
    skmat[0][1] = kmat[0][3];
    skmat[1][0] = kmat[3][0];
    skmat[1][1] = kmat[3][3];
    try {
      GVector eigen(2);
      Jacobi(skmat, eigen, skmat);
      EigenSort(eigen, skmat);
      for (int i = 0 ; i < 2 ; i++) {
        results.m_sevals[i] = eigen[i];
        results.m_kmat[i] = kmat[i*3][i*3];
      }
    }
    catch(IException &ie) {
      QString errmsg = "Eigen Solution Failed:: " + ie.toString();
      results.m_status = logError(EigenSolutionFailed, errmsg);
      return (results);
    }

    results.m_status = 0;
    return (results);
  }

  /**
   * @brief Compute Cholesky solution
   *
   * @author kbecker (5/15/2011)
   *
   * @param a
   * @param p
   *
   * @return Gruen::GMatrix
   */
  GMatrix Gruen::Choldc(const GMatrix &a, GVector &p) const {
    int nrows(a.dim1());
    int ncols(a.dim2());

    GMatrix aa = a.copy();
    p = GVector(ncols);

    for(int i = 0 ; i < nrows ; i++) {
      for(int j = i ; j < ncols ; j++) {
        double sum = aa[i][j];
        for(int k = i-1 ; k >= 0 ; k--) {
          sum -= (aa[i][k] * aa[j][k]);
        }
        // Handle diagonal special
        if (i == j) {
          if (sum <= 0.0) {
            throw IException(IException::Programmer,
                             "Choldc failed - matrix not postive definite",
                             _FILEINFO_);
          }
          p[i] = sqrt(sum);
        }
        else {
          aa[j][i] = sum / p[i];
        }
      }
    }
    return (aa);
  }

  /**
   * @brief Compute Cholesky solution matrix from correlation
   *
   * @author kbecker (5/15/2011)
   *
   * @param a
   * @param p
   * @param b
   * @param x
   *
   * @return Gruen::GMatrix
   */
  GMatrix Gruen::Cholsl(const GMatrix &a,const GVector &p, const GMatrix &b,
                        const GMatrix &x) const {
    assert(b.dim1() == x.dim1());
    assert(b.dim2() == x.dim2());
    assert(p.dim1() == b.dim2());

    int nrows(a.dim1());
    int ncols(a.dim2());

    GMatrix xout = x.copy();
    for(int j = 0 ; j < nrows ; j++) {

      for(int i = 0 ; i < ncols ; i++) {
        double sum = b[j][i];
        for(int k = i-1 ; k >= 0 ; k--) {
          sum -= (a[i][k] * xout[j][k]);
        }
        xout[j][i] = sum / p[i];
      }

      for (int i = ncols-1 ; i >= 0 ; i--) {
        double sum = xout[j][i];
        for(int k = i+1 ; k < ncols ; k++) {
          sum -= (a[k][i] * xout[j][k]);
        }
        xout[j][i] = sum / p[i];
      }

    }
    return (xout);
  }

  /**
   * @brief Compute the Jacobian of a covariance matrix
   *
   * @author kbecker (5/15/2011)
   *
   * @param a
   * @param evals
   * @param evecs
   * @param MaxIters
   *
   * @return int
   */
  int Gruen::Jacobi(const GMatrix &a, GVector &evals,
                    GMatrix &evecs, const int &MaxIters) const {

    int nrows(a.dim1());
    int ncols(a.dim2());
    GMatrix v = Identity(nrows);
    GVector d(nrows),b(nrows), z(nrows);

    for(int ip = 0 ; ip < nrows ; ip++) {
      b[ip] = a[ip][ip];
      d[ip] = b[ip];
      z[ip] = 0.0;
    }

    double n2(double(nrows) * double(nrows));
    GMatrix aa = a.copy();
    int nrot(0);
    for ( ; nrot < MaxIters ; nrot++) {
      double sm(0.0);
      for(int ip = 0 ; ip < nrows-1 ; ip++) {
        for(int iq = ip+1 ; iq < nrows ; iq++) {
          sm += fabs(aa[ip][iq]);
        }
      }

      //  Test for termination condition
      if (sm == 0.0) {
        evals = d;
        evecs = v;
        return (nrot);
      }

      double thresh = (nrot < 3) ? (0.2 * sm/n2 ): 0.0;
      for (int ip = 0 ; ip < nrows-1 ; ip++) {
        for (int iq = ip+1 ; iq < nrows ; iq++) {
          double g = 100.0 * fabs(aa[ip][iq]);
          if ( (nrot > 3) &&
               ( (fabs(d[ip]+g) == fabs(d[ip])) ) &&
               ( (fabs(d[iq]+g) == fabs(d[iq])) ) ) {
              aa[ip][iq] = 0.0;
          }
          else if ( fabs(aa[ip][iq]) > thresh ) {
            double h = d[iq] - d[ip];
            double t;
            if ( (fabs(h)+g) == fabs(h) ) {
              t = aa[ip][iq]/h;
            }
            else {
              double theta = 0.5 * h / aa[ip][iq];
              t = 1.0 / (fabs(theta) + sqrt(1.0 + theta * theta));
              if (theta < 0.0)  t = -1.0 * t;
            }
            double c = 1./sqrt(1.0 + t * t);
            double s = t * c;
            double tau = s / (1.0 + c);

            h = t * aa[ip][iq];
            z[ip] = z[ip] - h;
            z[iq] = z[iq] + h;
            d[ip] = d[ip] - h;
            d[iq] = d[iq] + h;
            aa[ip][iq] = 0.0;

            double g;
            for (int j = 0 ; j < ip-1 ; j++) {
              g = aa[j][ip];
              h = aa[j][iq];
              aa[j][ip] = g - s * (h + g * tau);
              aa[j][iq] = h + s * (g - h * tau);
            }

            for (int j = ip+1 ; j < iq-1 ; j++) {
              g = aa[ip][j];
              h = aa[j][iq];
              aa[ip][j] = g - s * (h + g * tau);
              aa[j][iq] = h + s * (g - h * tau);
            }

            for (int j = iq+1 ; j < ncols ; j++) {
              g = aa[ip][j];
              h = aa[j][iq];
              aa[ip][j] = g - s * (h + g * tau);
              aa[j][iq] = h + s * (g - h * tau);
            }

            for (int j = 0 ; j < ncols ; j++) {
              g = v[j][ip];
              h = v[j][iq];
              v[j][ip] = g - s * (h + g * tau);
              v[j][iq] = h + s * (g - h * tau);
            }
            nrot++;
          }
        }
      }

      for (int ip = 0 ; ip < nrows ; ip++) {
        b[ip] = b[ip] + z[ip];
        d[ip] = b[ip];
        z[ip] = 0.0;
      }
    }

    // Reach here and we have too many iterations
    evals = d;
    evecs = v;
    throw IException(IException::Programmer,
                     "Too many iterations in Jacobi",
                     _FILEINFO_);
    return (nrot);
  }


  GMatrix Gruen::Identity(const int &ndiag) const {
    GMatrix ident(ndiag, ndiag, 0.0);
    for (int i = 0 ; i < ndiag ; i++) {
      ident[i][i] = 1.0;
    }
    return (ident);
  }

  /**
   * @brief Sort eigenvectors from highest to lowest
   *
   * @author kbecker (5/15/2011)
   *
   * @param evals
   * @param evecs
   */
  void Gruen::EigenSort(GVector &evals, GMatrix &evecs) const {
    assert(evals.dim1() == evecs.dim1());
    int n(evals.dim1());
    for (int i = 0 ; i < n-1 ; i++ ) {
      int k = i;
      double p = evals[i];
      for (int j = i+1 ; j < n ; j++) {
        if (evals[j] >= p) {
          k = j;
          p = evals[j];
        }
      }
      if (k != i) {
        evals[k] = evals[i];
        evals[i] = p;
        for (int j = 0 ; j < n ; j++) {
          p = evecs[j][i];
          evecs[j][i] = evecs[j][k];
          evecs[j][k] = p;
        }
      }
    }
    return;
  }


  /**
   * @brief Minimization of data set using Gruen algorithm
   *
   * This is a very minimal application of the Gruen algorithm that provides the
   * ability to use it in a non-adaptive capacity.  This method processes two
   * chips of the same size, pattern and subsearch.  The subsearch has typically
   * been extracted in the same manner as the MinimumDifference or
   * MaximumCorrelation routines are utilized.
   *
   * It simply applies the algorithm to the current state of the two chips,
   * computes the error analysis on it and returns the eigen vector solution as an
   * indication of chip registration integrity.
   *
   * Note that in this mode, most all the parameters found in the definition file
   * that apply to the adaptive mode are ignored when utilizing the algorithm in
   * this fashion.
   *
   * @param pattern [in] A Chip object usually containing an nxm area of a cube.
   *                     Must be the same diminsions as \b subsearch.
   * @param subsearch [in] A Chip object usually containing an nxm area of a cube.
   *                  Must be the same dimensions as \b pattern. This is
   *                  normally a subarea of a larger portion of the image.
   *
   * @return The square root of the eigen values of DN differences OR Isis::NULL
   *         if the Gruen algorithm fails.
   */
  double Gruen::MatchAlgorithm(Chip &pattern, Chip &subsearch) {
    // reset();

    BigInt npts;
    double resid;
    GMatrix atai;
    AffineRadio affrad;
    int status = algorithm(pattern, subsearch, getDefaultRadio(),
                           npts, resid, atai, affrad);
    if (status == 0) {
      //  Compute fit quality
      Analysis analysis = errorAnalysis(npts, resid, atai);
      if (analysis.isValid()) {
        return (analysis.getEigen());
      }
    }

    // Error conditions return failure
    return (Null);
  }

  /**
   * This virtual method must return if the 1st fit is equal to or better
   * than the second fit.
   *
   * @param fit1  1st goodness of fit
   * @param fit2  2nd goodness of fit
   */
  bool Gruen::CompareFits(double fit1, double fit2) {
    return (fit1 <= fit2);
  }

  /**
   * @brief Applies the adaptive Gruen algorithm to pattern and search chips
   *
   * This method computes the adaptive Gruen algorithm for a pattern chip and
   * search chip.  The search chip is assumed to be of a larger size than the
   * pattern chip as dictated by the contents of the registration definition
   * file.
   *
   * This algorithm can be used with or without "fast geoming" the search chip.
   * It works quite well where the two images are assumed to be nearly spatially
   * registered.  Its real intent is to compute parallax angles between two
   * images taken at different viewing geometry.  This provides an efficient
   * process for deriving a digital elevation model (DEM) from two datasets.
   *
   * The Gruen algorithm is applied to the chips until the algorithm converges
   * (current iteration yields a detla affine within tolerance limits), an error
   * is encountered, or the maximum number of iterations is exceeded.
   *
   * Note that bestSamp and bestLine may not be the original center of the
   * search chip.  It is subject to chip reduction matching as specified by the
   * user.  All distance tolerances are compute from this postion.  The
   * process of chip reduction processing is handled by AutoReg prior to calling
   * this routine.
   *
   * @see getThreshHold() for Affine limit information
   *
   * @param sChip Full search chip as rendered from the search image
   * @param pChip Full pattern chip as rendered from the pattern/match image
   * @param fChip Maintains the solution vector at each chip location
   * @param startSamp Starting sample of the search image range
   * @param startLine Starting line of the search image range
   * @param endSamp  Ending sample of the search image range
   * @param endLine Ending line of the search image range
   * @param bestSamp Best registering sample of search chip
   * @param bestLine best registering line of search chip
   *
   * @return AutoReg::RegisterStatus Returns AutoReg::Success if the chip is
   *         successfully registered, otherwise returns
   *         AutoReg::AdaptiveAlgorithmFailed.
   */
  AutoReg::RegisterStatus Gruen::Registration(Chip &sChip, Chip &pChip,
      Chip &fChip, int startSamp,
      int startLine, int endSamp,
      int endLine, int bestSamp,
      int bestLine) {
    // Set conditions for writing subsearch chip states per call.  This code
    // implementation ensures caller must request it per call to this routine.
    //  See the WriteSubsearchChips() method.
    m_callCount++;
    QString chipOut = m_filePattern;
    m_filePattern = "";

    //  Initialize match point.  Ensure points are centered to get real cube
    //  line/sample positions
    pChip.SetChipPosition(pChip.TackSample(), pChip.TackLine());
    sChip.SetChipPosition(sChip.TackSample(), sChip.TackLine());
    MatchPoint matchpt = MatchPoint(PointPair(Coordinate(pChip),
                                              Coordinate(sChip)));

    // Create the fit chip whose size is the same as the pattern chip.
    // This chip will contain the final image at the last iteration.  This
    // usage differs from the non-adaptive purpose.
    // It is critical to use the original search chip to create the subsearch.
    // Copying the original search chip and then resizing preserves established
    // minimum/maximum value ranges.  Then, establish chip convergence
    // condition for Gruen's affine.
    fChip = sChip;
    fChip.SetSize(pChip.Samples(), pChip.Lines());
    Threshold thresh(fChip, getAffineTolerance());

    // Set up Affine transform by establishing search tack point
    AffineRadio affine = m_affine;
    m_affine = AffineRadio();  // Reset initial condition for next call

    //  Set up bestLine/bestSample position.  Do this using the local affine
    // and not the search chip.
    Coordinate best(bestLine-sChip.TackLine(), bestSamp-sChip.TackSample());
    affine.Translate(best);


    //  Algorithm parameters
    bool done = false;
    m_nIters = 0;
    do {

      // Extract the sub search chip.  The algorithm method handles the
      // determination of good data volumes
      Affine extractor(affine.m_affine);
      sChip.Extract(fChip, extractor);

      //  If requested for this run, write the current subsearch chip state
      if (!chipOut.isEmpty()) {
        std::ostringstream ss;
        ss << "C" << std::setw(6) << std::setfill('0') << m_callCount << "I"
           << std::setw(3) << std::setfill('0') << m_nIters;
        QString sfname = chipOut + ss.str().c_str() + ".cub";
        fChip.Write(sfname);
      }

      // Try to match the two subchips
      AffineRadio alpha;
      BigInt npts(0);
      GMatrix atai;
      double resid;
      int status = algorithm(pChip, fChip, affine.m_radio, npts, resid,
                             atai, alpha);
      if (status != 0) {
        //  Set failed return condition and give up!
        return (Status(matchpt.setStatus(status)));
      }

      //  Test for termination conditions - errors or convergence
      matchpt.m_nIters = ++m_nIters;
      if (m_nIters > m_maxIters) {
        QString errmsg = "Maximum Iterations exceeded";
        matchpt.setStatus(logError(MaxIterationsExceeded, errmsg));
        return (Status(matchpt));  //  Error condition
      }

      //  Check for convergence after the first pass
      if (m_nIters > 1) {
        if(thresh.hasConverged(alpha)) {
          //  Compute error analysis
          matchpt.m_affine = affine;
          matchpt.m_analysis = errorAnalysis(npts, resid, atai);
          matchpt.setStatus(matchpt.m_analysis.m_status);
          if (matchpt.isValid()) {
            //  Update the point even if constraints don't pass
            Coordinate uCoord = getChipUpdate(sChip, matchpt);
            SetChipSample(uCoord.getSample());
            SetChipLine(uCoord.getLine());
            SetGoodnessOfFit(matchpt.getEigen());

            // Check constraints
            matchpt.setStatus(CheckConstraints(matchpt));
          }

          //  Set output point
          m_point = matchpt;
          return (Status(matchpt));  // with AutoReg status
        }
      }
        //  Not done yet - update the affine transform for next loop
      try {
        affine += alpha;
      }
      catch (IException &ie) {
        QString mess = "Affine invalid/not invertable";
        matchpt.setStatus(logError(AffineNotInvertable, mess));
        return (Status(matchpt));  //  Another error condition to return
      }
    } while (!done);

    return (Status(matchpt));
  }

  /**
   * @brief Load default Gruen parameter file in $ISISROOT/appdata/templates
   *
   * @author Kris Becker - 5/22/2011
   *
   * @return Pvl Contents of default file
   */
  Pvl &Gruen::getDefaultParameters() {
    static Pvl regdef;
    regdef = Pvl("$ISISROOT/appdata/templates/autoreg/coreg.adaptgruen.p1515s3030.def");
    return (regdef);
  }

  /**
   * @brief Create Gruen error and processing statistics Pvl output
   *
   * This method generates two groups specific to the Gruen algorithm:  The
   * GruenFailures group which logs all the errors enountered during processing
   * and the GruenStatistics group which logs selected statistics gathered
   * during a registration run.
   *
   * These groups are added to the AutoReg log output Pvl container for
   * reporting to user/log files.
   *
   * @param pvl Input AutoReg Pvl container to add results to
   *
   * @return Pvl Output Pvl container with Gruen information
   */
  Pvl Gruen::AlgorithmStatistics(Pvl &pvl) {
    PvlGroup algo("GruenFailures");
    algo += PvlKeyword("Name", AlgorithmName().toStdString());
    algo += PvlKeyword("Mode", "Adaptive");

    //  Log errors
    for (int e = 0 ; e <  m_errors.size() ; e++) {
      algo += m_errors.getNth(e).LogIt();
    }

    if (m_unclassified > 0) {
      algo += PvlKeyword("UnclassifiedErrors", std::to_string(m_unclassified));
    }
    pvl.addGroup(algo);
    pvl.addGroup(StatsLog());
    pvl.addGroup(ParameterLog());
    return (pvl);
  }

  /**
   * @brief Create a PvlGroup with the Gruen specific statistics
   *
   * This method generates a PvlGroup from statistics collected for a particular
   * Gruen algorithm application.  This routine is called from the AutoReg
   * algorithm specific statistics routine and augments the AutoReg statistics
   * log output.
   *
   * @return PvlGroup Group containing Pvl keywords with collected statistics
   */
  PvlGroup Gruen::StatsLog() const {
    PvlGroup stats("GruenStatistics");

    stats += PvlKeyword("TotalIterations",   std::to_string(m_totalIterations));
    stats += ValidateKey("IterationMinimum", m_iterStat.Minimum());
    stats += ValidateKey("IterationAverage", m_iterStat.Average());
    stats += ValidateKey("IterationMaximum", m_iterStat.Maximum());
    stats += ValidateKey("IterationStandardDeviation", m_iterStat.StandardDeviation());

    stats += ValidateKey("EigenMinimum", m_eigenStat.Minimum());
    stats += ValidateKey("EigenAverage", m_eigenStat.Average());
    stats += ValidateKey("EigenMaximum", m_eigenStat.Maximum());
    stats += ValidateKey("EigenStandardDeviation", m_eigenStat.StandardDeviation());

    stats += ValidateKey("RadioShiftMinimum", m_shiftStat.Minimum());
    stats += ValidateKey("RadioShiftAverage", m_shiftStat.Average());
    stats += ValidateKey("RadioShiftMaximum", m_shiftStat.Maximum());
    stats += ValidateKey("RadioShiftStandardDeviation", m_shiftStat.StandardDeviation());

    stats += ValidateKey("RadioGainMinimum", m_gainStat.Minimum());
    stats += ValidateKey("RadioGainAverage", m_gainStat.Average());
    stats += ValidateKey("RadioGainMaximum", m_gainStat.Maximum());
    stats += ValidateKey("RadioGainStandardDeviation", m_gainStat.StandardDeviation());

    return (stats);

  }

  /**
   * @brief Create a PvlGroup with the Gruen specific parameters
   *
   * This method generates a PvlGroup of Gruen algorithm parameters. This
   * routine is called from the AutoReg algorithm specific statistics routine
   * and augments the AutoReg log output.
   *
   * @return PvlGroup Group containing Pvl keywords of Gruen parameters
   */
  PvlGroup Gruen::ParameterLog() const {
    PvlGroup parms("GruenParameters");

    parms += PvlKeyword("MaximumIterations", std::to_string(m_maxIters));
    parms += ValidateKey("AffineScaleTolerance", m_scaleTol);
    parms += ValidateKey("AffineShearTolerance", m_shearTol);
    parms += ValidateKey("AffineTranslationTolerance", m_transTol);

    parms += ParameterKey("AffineTolerance", m_affineTol);
    parms += ParameterKey("SpiceTolerance",  m_spiceTol);

    parms += ParameterKey("RadioShiftTolerance", m_shiftTol);

    parms += ParameterKey("RadioGainMinTolerance", m_rgainMinTol);
    parms += ParameterKey("RadioGainMaxTolerance", m_rgainMaxTol);

    parms += ValidateKey("DefaultRadioGain",  m_defGain);
    parms += ValidateKey("DefaultRadioShift", m_defShift);

    return (parms);
  }


  /**
   * @brief Creates an error list from know Gruen errors
   *
   * This method creates the list of known/expected Gruen errors that might
   * occur during processing.  This should be closely maintained with the
   * ErrorTypes enum list.
   *
   * @return Gruen::ErrorList Error list container
   */
  Gruen::ErrorList Gruen::initErrorList() const {
    ErrorList elist;
    elist.add(1, ErrorCounter(1, "NotEnoughPoints"));
    elist.add(2, ErrorCounter(2, "CholeskyFailed"));
    elist.add(3, ErrorCounter(3, "EigenSolutionFailed"));
    elist.add(4, ErrorCounter(4, "AffineNotInvertable"));
    elist.add(5, ErrorCounter(5, "MaxIterationsExceeded"));
    elist.add(6, ErrorCounter(6, "RadShiftExceeded"));
    elist.add(7, ErrorCounter(7, "RadGainExceeded"));
    elist.add(8, ErrorCounter(8, "MaxEigenExceeded"));
    elist.add(9, ErrorCounter(9, "AffineDistExceeded"));
    return (elist);
  }


  /**
   * @brief Logs a Gruen error
   *
   * A running count of errors that occur is maintained through this method.  If
   * an error occurs that is not in the list, it will also be counted.  This
   * would indicate that a new error condition has occured and needs to be added
   * to the list.
   *
   * @param gerrno  One of the errors as defined by GruenError enum
   * @param gerrmsg  Optional message although it is ignored in this context
   */
  int Gruen::logError(int gerrno, const QString &gerrmsg) {
    if (!m_errors.exists(gerrno)) {
      m_unclassified++;
    }
    else {
      m_errors.get(gerrno).BumpIt();
    }
    return (gerrno);
  }

  /**
   * @brief Initialize the object
   *
   * This method reads from the Algorithm group (if it exists) to set variables
   * used in this object.  If not all the keywords are present, then appropriate
   * values are provided.
   *
   * @param PvlObject &pvl PVL object/groups that contain algorithm parameters
   */
  void Gruen::init(Pvl &pvl) {
    //  Establish the parameters
    if (pvl.hasObject("AutoRegistration")) {
      m_prof = DbProfile(pvl.findGroup("Algorithm", Pvl::Traverse));
    }
    else {
      m_prof = DbProfile(pvl);
    }

    if (m_prof.Name().isEmpty())  m_prof.setName("Gruen");

    // Define internal parameters
    m_maxIters = std::stoi(ConfKey(m_prof, "MaximumIterations", std::to_string(30)));

    m_transTol = std::stod(ConfKey(m_prof, "AffineTranslationTolerance", std::to_string(0.1)));
    m_scaleTol = std::stod(ConfKey(m_prof, "AffineScaleTolerance", std::to_string(0.3)));
    m_shearTol = std::stod(ConfKey(m_prof, "AffineShearTolerance", std::to_string(m_scaleTol)));
    m_affineTol = std::stod(ConfKey(m_prof, "AffineTolerance", std::to_string(DBL_MAX)));

    m_spiceTol = std::stod(ConfKey(m_prof, "SpiceTolerance", std::to_string(DBL_MAX)));

    m_shiftTol = std::stod(ConfKey(m_prof, "RadioShiftTolerance", std::to_string(DBL_MAX)));
    m_rgainMinTol = std::stod(ConfKey(m_prof, "RadioGainMinTolerance", std::to_string(-DBL_MAX)));
    m_rgainMaxTol = std::stod(ConfKey(m_prof, "RadioGainMaxTolerance", std::to_string(DBL_MAX)));

    // Set radiometric defaults
    m_defGain  =  std::stod(ConfKey(m_prof, "DefaultRadioGain", std::to_string(0.0)));
    m_defShift =  std::stod(ConfKey(m_prof, "DefaultRadioShift", std::to_string(0.0)));

    m_callCount = 0;
    m_filePattern = "";

    m_nIters = 0;
    m_totalIterations = 0;

    m_errors = initErrorList();
    m_unclassified = 0;

    m_defAffine = AffineRadio(getDefaultRadio());
    m_affine = getAffineRadio();
    m_point  = MatchPoint(m_affine);

    //reset();
    return;
  }


  /**
   * @brief Reset Gruen statistics as needed
   *
   */
  void Gruen::resetStats() {
    m_eigenStat.Reset();
    m_iterStat.Reset();
    m_shiftStat.Reset();
    m_gainStat.Reset();
    return;
  }

  /**
   * @brief Computes the number of minimum valid points
   *
   * This method uses the pattern valid percent as specified in the registration
   * config file (or the programmer) to compute the minimum number of valid
   * points from the total.
   *
   * @param totalPoints Assumed to be total number of relavent pixels in a chip
   *
   * @return BigInt Minimum number of valid points determined from the
   *         percentage specified by user/programmer.
   */
  BigInt Gruen::MinValidPoints(BigInt totalPoints) const {
    double pts = (double) totalPoints * (PatternValidPercent() / 100.0);
    return ((BigInt) pts);
  }

  /**
   * @brief Determines if number of points is valid percentage of all points
   *
   * Computes the number of minimum valid points from user specified percentage
   * and tests the acutal number used.
   *
   * @param totalPoints Total number of possible valid points in chip
   * @param nPoints Actual number of valid points used in chips
   *
   * @return bool True if number of actual points exceeds percent mimimum valid,
   *         otherwise returns false.
   */
  bool Gruen::ValidPoints(BigInt totalPoints, BigInt nPoints) const {
    return (nPoints > MinValidPoints(totalPoints));
  }

  /**
   * @brief Return set of tolerances for affine convergence
   *
   * @author kbecker (5/15/2011)
   *
   * @return Tolerance Affine tolerances from PVL setup
   */
  AffineTolerance Gruen::getAffineTolerance() const {
    return (AffineTolerance(m_transTol, m_scaleTol, m_shearTol));
  }

  /**
   * @brief Test user limits/contraints after the algorithm has converged
   *
   * This method is invoked immediately after the Gruen algorithm has converged
   * to test against user specified limits.  This call is only valid in the
   * adaptive context as much of the error checking is handled by AutoReg when
   * using the non-adaptive algorithm.
   *
   * This method tests for convergence, maximum iterations exceeded, tolerance
   * limits of radiometric shift and gain and whether the eigenvalue of the
   * solution exceeds the limit.
   *
   * The result container is altered should a constraint not be meet which
   * indicates the registration failed.
   *
   * @param point Container with point information
   *
   * @return int Returns the status of the point.  If all constraints tests are
   *         valid, 0 is return, otherwise returns error number.
   */
  int Gruen::CheckConstraints(MatchPoint &point) {

    //  Point must be good for check to occur
    if (point.isValid()) {
      if (point.m_nIters > m_maxIters) {
        QString errmsg = "Maximum Iterations exceeded";
        return (logError(MaxIterationsExceeded, errmsg));
      }
      m_iterStat.AddData(point.m_nIters);

      if (point.getEigen() > Tolerance()) {
        QString errmsg = "Maximum Eigenvalue exceeded";
        return (logError(MaxEigenExceeded, errmsg));
      }
      m_eigenStat.AddData(point.getEigen());

      double shift = point.m_affine.m_radio.Shift();
      if ( shift > m_shiftTol) {
        QString errmsg = "Radiometric shift exceeds tolerance";
        return (logError(RadShiftExceeded, errmsg));
      }
      m_shiftStat.AddData(shift);

      double gain = point.m_affine.m_radio.Gain();
      if (((1.0 + gain) > m_rgainMaxTol) ||
          ((1.0 + gain) < m_rgainMinTol)) {
        QString errmsg = "Radiometric gain exceeds tolerances";
        return (logError(RadGainExceeded, errmsg));
      }
      m_gainStat.AddData(gain);


      double dist = point.getAffinePoint(Coordinate(0.0, 0.0)).getDistance();
      if (dist > getAffineConstraint()) {
        QString errmsg = "Affine distance exceeded";
        return (logError(AffineDistExceeded, errmsg));
      }
    }
    return (point.getStatus());
  }

  /**
   * @brief Compute the chip coordinate of the registered pixel
   *
   * @author Kris Becker - 5/19/2011
   *
   * @param chip  Chip to update with registration parameters
   * @param point Registration match information
   *
   * @return Coordinate
   */
  Coordinate Gruen::getChipUpdate(Chip &chip, MatchPoint &point) const {
    Coordinate chippt = point.getAffinePoint(Coordinate(0.0, 0.0));
    chip.SetChipPosition(chip.TackSample(), chip.TackLine());
    chip.TackCube(chip.CubeSample()+chippt.getSample(),
                  chip.CubeLine()+chippt.getLine());
    return (Coordinate(chip.ChipLine(), chip.ChipSample()));
  }


  /**
   * @brief Returns the proper status given a Gruen result container
   *
   * This method will return registration status consistant with
   * AutoReg::RegisterStatus return codes.
   *
   * @param result Gruen result container used to determine status
   * @return AutoReg::RegisterStatus Returns AutoReg::Success if the Gruen
   *         registration is successful, otherwise returns
   *         AutoReg::AdaptiveAlgorithmFailed.
   */
  AutoReg::RegisterStatus Gruen::Status(const MatchPoint &mpt) {
    if ( mpt.isValid() ) { return (AutoReg::SuccessSubPixel);  }
    return (AutoReg::AdaptiveAlgorithmFailed);
  }

} // namespace Isis
