/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "jama/jama_svd.h"
#include "jama/jama_qr.h"

#include <armadillo>

#include "LeastSquares.h"
#include "IException.h"
#include "IString.h"

namespace Isis {
  /**
   * Creates a LeastSquares Object.
   *
   * @param basis A BasisFunction. This parameter allows for the least squares
   *              fitting to be applied to arbitrary equations.
   */
  LeastSquares::LeastSquares(Isis::BasisFunction &basis, bool sparse,
                             int sparseRows, int sparseCols, bool jigsaw) {
    p_jigsaw = jigsaw;
    p_basis = &basis;
    p_solved = false;
    p_sparse = sparse;
    p_sigma0 = 0.;

    p_sparseRows = sparseRows;
    p_sparseCols = sparseCols;


    if (p_sparse) {

      //  make sure sparse nrows/ncols have been set
      if (sparseRows == 0  ||  sparseCols == 0) {
        std::string msg = "If solving using sparse matrices, you must enter the "
                      "number of rows/columns";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }


      p_sparseA.set_size(sparseRows, sparseCols);
      p_normals.set_size(sparseCols, sparseCols);
      p_ATb.resize(sparseCols, 1);
      p_xSparse.resize(sparseCols);

      if( p_jigsaw ) {
        p_epsilonsSparse.resize(sparseCols);
        std::fill_n(p_epsilonsSparse.begin(), sparseCols, 0.0);

        p_parameterWeights.resize(sparseCols);
      }

    }
    p_currentFillRow = -1;
  }

  //! Destroys the LeastSquares object.
  LeastSquares::~LeastSquares() {
  }

  /**
   * Invoke this method for each set of knowns. Given our example in the
   * description, we have three knowns and expecteds. They are
   * @f[
   * (1,1) = 3
   * @f]
   * @f[
   * (-2,3) = 1
   * @f]
   * @f[
   * (2,-1) = 2
   * @f]
   *
   * @param data A vector of knowns.
   *
   * @param result The expected value for the knowns.
   *
   * @param weight (Default = 1.0) How strongly to weight this known. Weight less
   *               than 1 increases residual for this known, while weight greater
   *               than 1 decreases the residual for this known.
   *
   * @throws Isis::IException::Programmer - Number of elements in data does not
   *                                        match basis requirements
   *
   * @internal
   * @history 2008-04-22  Tracie Sucharski,  Fill sparse matrix.
   * @history 2009-12-21  Jeannie Walldren,  Modified code to add
   *          the square root of the weight to the vector
   *          p_sqrtweight.
   *
   */
  void LeastSquares::AddKnown(const std::vector<double> &data, double result,
                              double weight) {
    if((int) data.size() != p_basis->Variables()) {
      std::string msg = "Number of elements in data does not match basis [" +
                        p_basis->Name().toStdString() + "] requirements";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    p_expected.push_back(result);

    if (weight == 1) {
      p_sqrtWeight.push_back(weight);
    }
    else {
      p_sqrtWeight.push_back(sqrt(weight));
    }

    if(p_sparse) {
      FillSparseA(data);
    }
    else {
      p_input.push_back(data);
    }
  }



  /**
   *  Invoke this method for each set of knowns for sparse solutions.  The A
   *  sparse matrix must be filled as we go or we will quickly run out of memory
   *  for large solutions. So, expand the basis function, apply weights (which is
   *  done in the Solve method for the non-sparse case.
   *
   * @param data A vector of knowns.
   *
   * @internal
   * @history 2008-04-22  Tracie Sucharski - New method for sparse solutions.
   * @history 2009-12-21  Jeannie Walldren - Changed variable name
   *          from p_weight to p_sqrtweight.
   *
   */
  void LeastSquares::FillSparseA(const std::vector<double> &data) {

    p_basis->Expand(data);

    p_currentFillRow++;

    int ncolumns = (int)data.size();

    for(int c = 0;  c < ncolumns; c++) {
      p_sparseA(p_currentFillRow, c) = p_basis->Term(c) * p_sqrtWeight[p_currentFillRow];
    }
  }


  /**
   * This method returns the data at the given row.
   *
   * @param row
   *
   * @return std::vector<double>
   */
  std::vector<double> LeastSquares::GetInput(int row) const {
    if((row >= Rows()) || (row < 0)) {
      std::string msg = "Index out of bounds [Given = " + toString(row) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return p_input[row];
  }

  /**
   * This method returns the expected value at the given row.
   *
   * @param row
   *
   * @return double
   */
  double LeastSquares::GetExpected(int row) const {
    if((row >= Rows()) || (row < 0)) {
      std::string msg = "Index out of bounds [Given = " + toString(row) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return p_expected[row];
  }

  /**
   * This methods returns the number of rows in the matrix.
   *
   *
   * @return int
   */
  int LeastSquares::Rows() const {
    return (int)p_input.size();
  }

  /**
   * After all the data has been registered through AddKnown, invoke this
   * method to solve the system of equations. You can then use the Evaluate
   * and Residual methods freely.
   *
   * @internal
   * @history  2008-04-16 Debbie Cook / Tracie Sucharski, Added SolveSparse.
   * @history  2009-04-08 Tracie Sucharski - Added return value which will
   *                          pass on what is returned from SolveSparse which
   *                          is a column number of a column that contained
   *                          all zeros.
   * @history  2010-12-12 Debbie A. Cook  Fixed "no data" test for SPARSE
   *                          case
   */
  int LeastSquares::Solve(Isis::LeastSquares::SolveMethod method) {

    if((method == SPARSE  &&  p_sparseRows == 0)  ||
       (method != SPARSE  &&  Rows() == 0 )) {
      p_solved = false;
      std::string msg = "No solution available because no input data was provided";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    if(method == SVD) {
      SolveSVD();
    }
    else if(method == QRD) {
      SolveQRD();
    }
    else if(method == SPARSE) {
      int column = SolveSparse();
      return column;
    }
    return 0;
  }

  /**
   * After all the data has been registered through AddKnown, invoke this
   * method to solve the system of equations. You can then use the Evaluate
   * and Residual methods freely.
   * @internal
   * @history 2009-12-21  Jeannie Walldren - Changed variable name
   *          from p_weight to p_sqrtweight.
   *
   */
  void LeastSquares::SolveSVD() {

    // We are solving Ax=b ... start by creating A
    TNT::Array2D<double> A(p_input.size(), p_basis->Coefficients());
    for(int r = 0; r < A.dim1(); r++) {
      p_basis->Expand(p_input[r]);
      for(int c = 0; c < A.dim2(); c++) {
        A[r][c] = p_basis->Term(c) * p_sqrtWeight[r];
      }
    }

    // Ok use singular value decomposition to solve for the coefficients
    // A = [U][S][V']  where [U] is MxN, [S] is NxN, [V'] is NxN transpose
    // of [V].  We are solving for [A]x=b and need inverse of [A] such
    // that x = [invA]b. Since inverse may not exist we use the
    // pseudo-inverse [A+] from SVD which is [A+] = [V][invS][U']
    // Our coefficents are then x = [A+]b where b is p_b.
    JAMA::SVD<double> svd(A);

    TNT::Array2D<double> V;
    svd.getV(V);

    // The inverse of S is the 1 over each diagonal element of S
    TNT::Array2D<double> invS;
    svd.getS(invS);

    for(int i = 0; i < invS.dim1(); i++) {
      if(invS[i][i] != 0.0) invS[i][i] = 1.0 / invS[i][i];
    }

    // Transpose U
    TNT::Array2D<double> U;
    svd.getU(U);
    TNT::Array2D<double> transU(U.dim2(), U.dim1());

    for(int r = 0; r < U.dim1(); r++) {
      for(int c = 0; c < U.dim2(); c++) {
        transU[c][r] = U[r][c];
      }
    }

    // Now multiply everything together to get [A+]
    TNT::Array2D<double> VinvS = TNT::matmult(V, invS);
    TNT::Array2D<double> Aplus = TNT::matmult(VinvS, transU);

    // Using Aplus and our b we can solve for the coefficients
    TNT::Array2D<double> b(p_expected.size(), 1);

    for(int r = 0; r < (int)p_expected.size(); r++) {
      b[r][0] = p_expected[r] * p_sqrtWeight[r];
    }

    TNT::Array2D<double> coefs = TNT::matmult(Aplus, b);

    // If the rank of the matrix is not large enough we don't
    // have enough coefficients for the solution
    if (coefs.dim1() < p_basis->Coefficients()) {
      std::string msg = "Unable to solve least-squares using SVD method. No "
                    "solution available. Not enough knowns or knowns are "
                    "co-linear ... [Unknowns = "
                    + toString(p_basis->Coefficients()) + "] [Knowns = "
                    + toString(coefs.dim1()) + "]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    // Set the coefficients in our basis equation
    std::vector<double> bcoefs;
    for (int i = 0; i < coefs.dim1(); i++) bcoefs.push_back(coefs[i][0]);

    p_basis->SetCoefficients(bcoefs);

    // Compute the errors
    for(int i = 0; i < (int)p_input.size(); i++) {
      double value = p_basis->Evaluate(p_input[i]);
      p_residuals.push_back(value - p_expected[i]);
      p_sigma0 += p_residuals[i]*p_residuals[i]*p_sqrtWeight[i]*p_sqrtWeight[i];
    }
    // calculate degrees of freedom (or redundancy)
    // DOF = # observations + # constrained parameters - # unknown parameters
    p_degreesOfFreedom = p_basis->Coefficients() - coefs.dim1();

    if( p_degreesOfFreedom > 0.0 )  {
      p_sigma0 = p_sigma0/(double)p_degreesOfFreedom;
    }

    // check for p_sigma0 < 0
    p_sigma0 = sqrt(p_sigma0);

    // All done
    p_solved = true;
  }

  /**
   * After all the data has been registered through AddKnown, invoke this
   * method to solve the system of equations with a QR
   * decomposition of A = QR. You can then use the Evaluate and
   * Residual methods freely. The QR decomposition is only slightly
   * less reliable than the SVD, but much faster.
   * @internal
   * @history 2009-12-21  Jeannie Walldren - Changed variable name
   *          from p_weight to p_sqrtweight.
   *
   */
  void LeastSquares::SolveQRD() {

    // We are solving Ax=b ... start by creating an MxN matrix, A
    TNT::Array2D<double> A(p_input.size(), p_basis->Coefficients());
    for(int r = 0; r < A.dim1(); r++) {
      p_basis->Expand(p_input[r]);
      for(int c = 0; c < A.dim2(); c++) {
        A[r][c] = p_basis->Term(c) * p_sqrtWeight[r];
      }
    }

    // Ok use  to solve for the coefficients
    // [A] = [Q][R]  where [Q] is MxN and orthogonal and  [R] is an NxN,
    // upper triangular matrix.  TNT provides the solve method that inverts
    // [Q] and backsolves [R] to get the coefficients in the vector x.
    // That is, we solve the system Rx = Q^T b
    JAMA::QR<double> qr(A);

    // Using A and our b we can solve for the coefficients
    TNT::Array1D<double> b(p_expected.size());
    for(int r = 0; r < (int)p_expected.size(); r++) {
      b[r] = p_expected[r] * p_sqrtWeight[r];
    }// by construction, we know the size of b is equal to M, so b is conformant

    // Check to make sure the matrix is full rank before solving
    // -- rectangular matrices must be full rank in order for the solve method
    //    to be successful
    int full = qr.isFullRank();
    if(full == 0) {
      std::string msg = "Unable to solve-least squares using QR Decomposition. "
                    "The upper triangular R matrix is not full rank";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    TNT::Array1D<double> coefs = qr.solve(b);

    // Set the coefficients in our basis equation
    std::vector<double> bcoefs;
    for(int i = 0; i < coefs.dim1(); i++) {
      bcoefs.push_back(coefs[i]);
    }
    p_basis->SetCoefficients(bcoefs);

    // Compute the errors
    for(int i = 0; i < (int)p_input.size(); i++) {
      double value = p_basis->Evaluate(p_input[i]);
      p_residuals.push_back(value - p_expected[i]);
    }

    // All done
    p_solved = true;
  }




  /**
   * @brief  Solve using sparse class
   *
   * After all the data has been registered through AddKnown, invoke this
   * method to solve the system of equations Nx = b, where
   *
   * N = ATPA
   * b = ATPl, and
   *
   * N is the "normal equations matrix;
   * A is the so-called "design" matrix;
   * P is the observation weight matrix (typically diagonal);
   * l is the "computed - observed" column vector;
   *
   * The solution is achieved using a sparse matrix formulation of
   * the LU decomposition of the normal equations.
   *
   * You can then use the Evaluate and Residual methods freely.
   *
   * @internal
   * @history  2008-04-16 Debbie Cook / Tracie Sucharski, New method
   * @history  2008-04-23 Tracie Sucharski,  Fill sparse matrix as we go in
   *                          AddKnown method rather than in the solve method,
   *                          otherwise we run out of memory very quickly.
   * @history  2009-04-08 Tracie Sucharski - Added return value which is a
   *                          column number of a column that contained all zeros.
   * @history 2009-12-21  Jeannie Walldren - Changed variable name
   *                          from p_weight to p_sqrtweight.
   * @history 2010        Ken Edmundson
   * @history 2010-11-20  Debbie A. Cook Merged Ken Edmundson verion with system version
   * @history 2011-03-17  Ken Edmundson Corrected computation of residuals
   *
   */
  int LeastSquares::SolveSparse() {

    // form "normal equations" matrix by multiplying ATA
    p_normals = p_sparseA.t()*p_sparseA;

    // Create the right-hand-side column vector 'b'
    arma::mat b(p_sparseRows, 1);

    // multiply each element of 'b' by it's associated weight
    for ( int r = 0; r < p_sparseRows; r++ )
      b(r,0) = p_expected[r] * p_sqrtWeight[r];

    // form ATb
    p_ATb = p_sparseA.t()*b;

    // apply parameter weighting if Jigsaw (bundle adjustment)
    if ( p_jigsaw ) {
      for( int i = 0; i < p_sparseCols; i++) {
        double weight = p_parameterWeights[i];

        if( weight <= 0.0 )
          continue;

        p_normals(i, i) += weight;
        p_ATb(i, 0) -= p_epsilonsSparse[i]*weight;
      }
    }

    bool status = spsolve(p_xSparse, p_normals, p_ATb, "superlu");

    if (status == false) {
      std::string msg = "Could not solve sparse least squares problem.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    // Set the coefficients in our basis equation
    p_basis->SetCoefficients(arma::conv_to< std::vector<double> >::from(p_xSparse));

    // if Jigsaw (bundle adjustment)
    // add corrections into epsilon vector (keeping track of total corrections)
    if ( p_jigsaw ) {
      for( int i = 0; i < p_sparseCols; i++ )
        p_epsilonsSparse[i] += p_xSparse[i];
    }

    // Compute the image coordinate residuals and sum into Sigma0
    // (note this is exactly what was being done before, but with less overhead - I think)
    // ultimately, we should not be using the A matrix but forming the normals
    // directly. Then we'll have to compute the residuals by back projection

    p_residuals.resize(p_sparseRows);
    p_residuals = arma::conv_to< std::vector<double> >::from(p_sparseA*p_xSparse);
    p_sigma0 = 0.0;

    for ( int i = 0; i < p_sparseRows; i++ ) {
        p_residuals[i] = p_residuals[i]/p_sqrtWeight[i];
        p_residuals[i] -= p_expected[i];
        p_sigma0 += p_residuals[i]*p_residuals[i]*p_sqrtWeight[i]*p_sqrtWeight[i];
    }

    // if Jigsaw (bundle adjustment)
    // add contibution to Sigma0 from constrained parameters
    if ( p_jigsaw ) {
      double constrained_vTPv = 0.0;

      for ( int i = 0; i < p_sparseCols; i++ ) {
        double weight = p_parameterWeights[i];

        if ( weight <= 0.0 )
          continue;

        constrained_vTPv += p_epsilonsSparse[i]*p_epsilonsSparse[i]*weight;
      }
      p_sigma0 += constrained_vTPv;
    }
    // calculate degrees of freedom (or redundancy)
    // DOF = # observations + # constrained parameters - # unknown parameters
    p_degreesOfFreedom = p_sparseRows + p_constrainedParameters - p_sparseCols;

    if( p_degreesOfFreedom <= 0.0 ) {
      p_sigma0 = 1.0;
    }
    else {
      p_sigma0 = p_sigma0/(double)p_degreesOfFreedom;
    }

    // check for p_sigma0 < 0
    p_sigma0 = sqrt(p_sigma0);

    // All done
    p_solved = true;
    return 0;
  }


  void LeastSquares::Reset ()
  {
    if ( p_sparse ) {
      p_sparseA.zeros();
      p_ATb.zeros();
      p_normals.zeros();
      p_currentFillRow = -1;
    }
    else {
      p_input.clear();
    }
      p_sigma0 = 0.;
    p_residuals.clear();
    p_expected.clear();
    p_sqrtWeight.clear();
    p_solved = false;
  }



  /**
   * Invokes the BasisFunction Evaluate method.
   *
   * @param data The input variables to evaluate.
   *
   * @return The evaluation for the input variable.
   *
   * @throws Isis::IException::Programmer - Unable to evaluate until a
   *                                        solution has been computed
   */
  double LeastSquares::Evaluate(const std::vector<double> &data) {
    if(!p_solved) {
      std::string msg = "Unable to evaluate until a solution has been computed";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return p_basis->Evaluate(data);
  }

  /**
   * Returns a vector of residuals (errors). That is, the difference between the
   * evaluation of a known with the solution against the expected value.
   *
   * @return The vector of residuals.
   *
   * @throws Isis::IException::Programmer - Unable to return residuals until a
   *                                        solution has been computed
   */
  std::vector<double> LeastSquares::Residuals() const {
    if(!p_solved) {
      std::string msg = "Unable to return residuals until a solution has been computed";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return p_residuals;
  }

  /**
   * Returns the ith residual. That is, the difference between the evaluation of
   * a known with the solution against the expected value. There is one residual
   * for each time AddKnown was invoked.
   *
   * @param i The number of times AddKnown was invoked to be evaluated.
   *
   * @return The output value of the residual.
   *
   * @throws Isis::IException::Programmer - Unable to return residuals until a
   *                                        solution has been computed
   */
  double LeastSquares::Residual(int i) const {
    if(!p_solved) {
      std::string msg = "Unable to return residuals until a solution has been computed";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return p_residuals[i];
  }

  /**
   * Reset the weight for the ith known. This weight will not be used unless
   * the system is resolved using the Solve method.
   *
   * @param index The position in the array to assign the given weight value
   * @param weight A weight factor to apply to the ith known. A weight less
   *               than one increase the residual for this known while a
   *               weight greater than one decrease the residual for this
   *               known.
   * @internal
   * @history 2009-12-21  Jeannie Walldren,  Modified code to add
   *          the square root of the weight to the vector
   *          p_sqrtweight.
   *
   */

  void LeastSquares::Weight(int index, double weight) {
    if(weight == 1) {
      p_sqrtWeight[index] = weight;
    }
    else {
      p_sqrtWeight[index] = sqrt(weight);
    }
  }

} // end namespace isis
