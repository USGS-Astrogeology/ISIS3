#ifndef FunctionTools_h
#define FunctionTools_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include <math.h>
#include <cfloat>

#include <QList>
#include <QString>

#include "Constants.h"
#include "IException.h"
#include "IString.h"

namespace Isis {
  /**
   * A collection of tools for mathmatical function root finding, maximization, etc (eventually)
   * This class contains only static methods, and cannot be instantiated
   *
   * @ingroup Math
   *
   *
   * @author 2012-05-07 Orrin Thomas
   *
   * @internal
   *
   *   @history 2012-05-07 Orrin Thomas - Original version
   *   @history 2012-06-22 Orrin Thomas - Added realLinearRoots, realQuadraticRoots, 
   *                         and realCubicRoots
   */
  class FunctionTools {
    public:


     /** Find the real roots (0 or 1) of a linear equation
      *    Form: coeffLinearTerm*X + coeffConstTerm = 0.0
      *    NOTE: in the case of infinite roots an empty set is returned
      *   
      *  @param coefflinearTerm  coefficient of the linear term
      *  @param coeffConstTerm  coefficient of the constant term
      */
    static QList <double> realLinearRoots(double coeffLinearTerm, double coeffConstTerm) {
      double m=coeffLinearTerm, b=coeffConstTerm;

      QList<double> roots;

      //if the slope is zero there are either 0 or infinite roots
      //  for the present I see no need to handle the infinite roots situation more elegantly...
      if (m==0.0) 
        return roots;

      roots << -b/m;

      return roots;    
    }

     /** The correct way to find the real roots of a quadratic (0, 1, or 2) 
      *    (according to numerical recipies 3rd edtion page 227).
      *    Form: coeffQuadTerm*X^2 + coeffLinearTerm*X + coeffConstTerm = 0.0
      *   
      *  @param coeffQuadTerm  coefficient of the quadratic term
      *  @param coefflinearTerm  coefficient of the linear term
      *  @param coeffConstTerm  coefficient of the constant term
      */
    static QList <double> realQuadraticRoots(double coeffQuadTerm, double coeffLinearTerm, 
                                     double coeffConstTerm) {
      double A=coeffQuadTerm, B=coeffLinearTerm, C=coeffConstTerm;
      double disc,q,temp; //helpers on the way to a solution 

      if (A==0.0)
        return realLinearRoots(coeffLinearTerm, coeffConstTerm);

      QList<double> roots;
    
      disc = B*B-4*A*C;
      if (disc < 0) return roots;  //no solution, return empty set
      q = -0.5*(B + (B < 0 ? -1.0 : 1.0)*sqrt(disc));
 
      if (q == 0) roots << q/A;
      else {
        roots << q/A;
        //after the first root make sure there are no duplicates
        temp = C/q;
        if (!roots.contains(temp)) roots << temp; 
      }      
      return roots;
    }



    /** Find the real roots of a cubic (1, 2, or 3) 
      *    (see numerical recipies 3rd edtion page 227)
      *    Form: coeffCubicTerm*X^3 + coeffQuadTerm*X^2 + coeffLinearTerm*X + coeffConstTerm = 0.0
      *
      *  @param coeffCubicTerm  coefficient of the cubic term   
      *  @param coeffQuadTerm  coefficient of the quadratic term
      *  @param coefflinearTerm  coefficient of the linear term
      *  @param coeffConstTerm  coefficient of the constant term
      */
    static QList <double> realCubicRoots(double coeffCubicTerm,  double coeffQuadTerm, 
                                 double coeffLinearTerm, double coeffConstTerm) {
      double a=coeffQuadTerm, b=coeffLinearTerm, c=coeffConstTerm;
      double Q,R; //helpers on the way to a solution 

      //first verify this is really a cubic
      if (coeffCubicTerm == 0.0) 
        return realQuadraticRoots(coeffQuadTerm,coeffLinearTerm,coeffConstTerm);
      

      //algorithm wants the leading coefficient to be 1.0
      if ( coeffCubicTerm != 1.0) {
        a /= coeffCubicTerm;
        b /= coeffCubicTerm;
        c /= coeffCubicTerm;         
      }
      
      QList<double> roots;  

      Q = (a*a - 3.0*b) / 9.0;
      R = (2*a*a*a - 9.0*a*b + 27.0*c) / 54.0;
      //cout << "Q R: " << Q << " " << R << "\n";

      if ( a == 0.0 && b == 0.0 ) { //one simple root
        roots << (c < 0 ? 1.0 : -1.0)*pow(fabs(c),1.0/3.0);
      }
      else if (R*R <= Q*Q*Q) {  //there are three roots (one of them can be a double root)
        double theta = acos(R/sqrt(Q*Q*Q)),temp;
        //cout << "three roots, theta: " << theta << "\n";
        Q = -2.0*sqrt(Q); //just done to save some FLOPs
        roots << Q*cos(theta/3.0) - a /3.0;
        //after the first root make sure there are no duplicates
        temp = Q*cos((theta + TWOPI)/3.0) - a / 3.0;
        if (!roots.contains(temp)) roots << temp;
        temp = Q*cos((theta - TWOPI)/3.0) - a / 3.0;
        if (!roots.contains(temp)) roots << temp;
      }
      else {  //there is a single real root
        double A, B;
        A = (R < 0 ? 1.0 : -1.0 ) * pow(fabs(R) + sqrt(R*R - Q*Q*Q),1.0/3.0);
        B = A == 0.0 ? 0.0 : Q / A;
        
        roots << (A + B) - a / 3.0;
      }

      return roots;
    }

    /** Van Wijngaarden-Dekker-Brent Method for root finding on a discreetly defined function
      *  meaning that we can evaluate the function for discreet points, but we lack a global
      *  function and derivative definitions.  See Numerical Recipes 3rd eddition pages 454-456.
      *
      *  This method requires that the root be bounded on the interval [pt1,pt2], and is gaurenteed
      *  to convege a root (by Brent) in the interval as long the function is continous and 
      *  can evaluated on that interval.  
      *
      *  Note that if there are multiple roots on the interval the function will find one
      *  of them and there is no particular gaurentee which one.  Note, also that I have changed the
      *  convergance criteria to enforce the nearness of the function to zero rather than the
      *  precision of the root.
      *
      *  @param func  template parameter that must have a  double operator()(double) defined
      *  @param pt1   one of the already defined points (x, y) that bracket the root
      *  @param pt2   one of the already defined points (x, y) that bracket the root
      *  @param tol   how close to zero the function must come before iterations stop
      *  @param maxiter  the maximum number of iterations before stoping the root search
      *  @param root  the returned root (if any)
      *  @returns bool  true if the solution converged, false otherwise
      */
    template <typename Functor> 
    static bool brentsRootFinder(Functor &func, const QList<double> pt1, const QList<double> pt2, 
                                 double tol, int maxIter, double &root) {
      double a = pt1[0], b = pt2[0], c, d = 0;
      double fa = pt1[1], fb = pt2[1], fc;
      double p, q, r, s, t, tol1, bnew, fbnew, temp, deltaI, deltaF;

      // offset used for improved numerical stability
      double offset = (a + b) / 2.0;
      a -= offset;
      b -= offset;

      // check to see if the points bracket a root(s), if the signs are equal they don't
      if ( (fa > 0) - (fa < 0) == (fb > 0) - (fb < 0) ) {
        std::string msg = "The function evaluations of two bounding points passed to Brents Method "
                      "have the same sign.  Therefore, they don't necessary bound a root.  No "
                      "root finding will be attempted.\n";
        throw IException(IException::Programmer, msg, _FILEINFO_);
        return false;
      }

      bool mflag = true;

      if (fabs(fa) < fabs(fb)) {
        // if a is a better guess for the root than b, switch them
        // b is always the current best guess for the root
        temp = a;
        a = b;
        b = temp;
        temp = fa;
        fa = fb;
        fb = temp;
      }

      c = a;
      fc = fa;

      for (int iter = 0; iter < maxIter; iter++) {
        tol1 = DBL_EPSILON * 2.0 * fabs(b);  // numerical tolerance
        if (a != c && b != c) {
          // inverse quadratic interpolation
          r = fb / fc;
          s = fb / fa;
          t = fa / fc;
          p = s * (t * (r - t) * (c - b) - (1 - r) * (b - a));
          q = (t - 1) * (r - 1) * (s - 1);
          bnew = b + p / q;     
        }
        else {
          // secant rule
          bnew = b - fb * (b - a) / (fb - fa);
        }

        // five tests follow to determine if the iterpolation methods are working better than 
        // bisection. p and q are setup as the bounds we want the new root guess to fall within.
        // this enforces that the new root guess be withing the 3/4 of the interval closest to
        // b, the current best guess.
        temp = (3 * a + b) / 4.0;
        p = temp < b ? temp : b;
        q = b > temp ? b : temp;
        deltaI = fabs(b - bnew); // magnitude of the interpolated correctio
        if ( // if the root isn't within the 3/4 of the interval closest to b (the current best guess)
             (bnew < p || bnew > q )                     ||  
             // or if the last iteration was a bisection 
             // and the new correction is greater magnitude than half the magnitude of last
             // correction, ie it's doing less to narrow the root than a bisection would
             (mflag && deltaI >= fabs(b - c) / 2.0)  ||
             // or if the last iteration was an interpolation
             // and the new correction magnitude is greater than 
             // the half the magnitude of the correction from two iterations ago,
             // i.e. it's not converging faster than bisection
             (!mflag && deltaI >= fabs(c - d) / 2.0) ||
             // or if the last iteration was a bisection
             // and the last correction was less than the numerical tolerance  
             // ie we are reaching the limits of our numerical
             // ability to find a better root so lets do bisection, it's numerical safer
             (mflag && fabs(b - c) < tol1)                 ||
             // or it the last iteration was an interpolation
             // and the correction from two iteration ago was less than the current
             // numerical tolerance, ie we are reaching the limits of our numerical
             // ability to find a better root so lets do bisection, it's numerical safer
             (!mflag && fabs(c - d) < tol1)            ) {

          // bisection method
          bnew = (a + b) / 2.0;
          mflag = true;
        }
        else {
          mflag = false;
        }
        try {
          fbnew = func(bnew + offset);
        } catch (IException &e) {
          std::string msg = "Function evaluation failed at:" + toString(bnew) + 
                        ".  Function must be continuous and defined for the entire interval "
                        "inorder to gaurentee brentsRootFinder will work.";
          throw IException(e, IException::Programmer, msg, _FILEINFO_);
        }
        d = c;  // thus d always equals the best guess from two iterations ago
        c = b;  // thus c always equals the best guess from the previous iteration
        fc = fb;
        
        if ( (fa > 0) - (fa < 0) == (fbnew > 0) - (fbnew < 0) ) {
          // if b and bnew bracket the root
          deltaF = fabs(a - bnew); // the Final magnitude of the correction
          a = bnew;
          fa = fbnew;
        }
        else {
          // a and bnew bracket the root
          deltaF = fabs(b - bnew); // the Final magnitude of the correction
          b = bnew;
          fb = fbnew;
        }
    
        if (fabs(fa) < fabs(fb)) {
          // if a is a better guess for the root than b, switch them
          temp = a;
          a = b;
          b = temp;
          temp = fa;
          fa = fb;
          fb = temp;
        }
        if (fabs(fb) < tol) {
          // if the tolerance is meet
          root = b + offset;
          return true;  
        }
        else if ( deltaF < tol1 && fabs(b) < 100.0 * tol) {
          // we've reach the numerical limit to how well the root can be defined, and the function
          // is at least approaching zero
          // This was added specifically for the apollo pan camera, the camera classes (and maybe
          // NAIF as well can not actually converge to zero for the extreme edges of some pan
          // images (partial derivates WRT to line approach infinity).  They can get close 
          // 'enough' however
          root = b + offset;
          return true;  
        }
        else if (deltaF < tol1) {
          // we've reached the limit of the numerical ability to refine the root and the function
          // is not near zero.  This is a clasically ill defined root (nearly vertical function)
          // "This is not the [root] you're looking for"
          return false;
        }
      }
      // maximum number of iteration exceeded
      return false;
    } // end brentsRootFinder


    private:
      /** Constructor
       *
       * This is private and left undefined so this class cannot be instaniated. The functions 
       * are static and are intended to be used without an instance of the class. 
       */
      FunctionTools();
     
      /** destructor
       *
       * This is private and to left undefined so this class cannot be instaniated
       */
      ~FunctionTools();

  }; // End FuntionTools class definition

}; // End namespace Isis

#endif
