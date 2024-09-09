/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <QString>

#include "PrincipalComponentAnalysis.h"
#include "jama/jama_eig.h"
#include "jama/jama_lu.h"

using namespace std;

namespace Isis {
  //! Constructs the PrincipalComponentAnalysis object.
  PrincipalComponentAnalysis::PrincipalComponentAnalysis(const int n) {
    p_dimensions = n;
    p_statistics.clear();
    p_statistics.resize(n * n);
    for(int i = 0; i < n; i++) {
      for(int j = 0; j <= i; j++) {
        p_statistics[n*i+j] = new Isis::MultivariateStatistics();
        p_statistics[n*j+i] = p_statistics[n*i+j];
      }
    }

    p_hasTransform = false;
  };

  // create a pca object from the trabsform matrix
  PrincipalComponentAnalysis::PrincipalComponentAnalysis(TNT::Array2D<double> transform) {
    if(transform.dim1() != transform.dim2()) {
      std::string m = "Illegal transform matrix";
      throw IException(IException::Programmer, m, _FILEINFO_);
    }

    p_dimensions = transform.dim1();
    p_transform = transform;
    ComputeInverse();

    p_hasTransform = true;
  }

  // Add data for all dimensions
  //  Note: the data should be stored as an array containing
  //   the first dimension in order, then the second, ...
  void PrincipalComponentAnalysis::AddData(const double *data, const unsigned int count) {
    // If this PCA object has a transform matrix
    //  we cannot add more data
    if(p_hasTransform) {
      std::string m = "Cannot add data to a PCA that has a defined transform matrix";
      throw IException(IException::Programmer, m, _FILEINFO_);
    }

    // add the data to the multivariate stats objects
    for(int i = 0; i < p_dimensions; i++) {
      for(int j = 0; j <= i; j++) {
        p_statistics[p_dimensions*i+j]->AddData(&data[count*i], &data[count*j], count);
      }
    }
  }

  // Use VDV' decomposition to obtain the eigenvectors
  void PrincipalComponentAnalysis::ComputeTransform() {
    if(p_hasTransform) {
      std::string m = "This PCA already has a computed transform";
      throw IException(IException::Programmer, m, _FILEINFO_);
    }

    TNT::Array2D<double> C(p_dimensions, p_dimensions);
    for(int i = 0; i < p_dimensions; i++) {
      for(int j = 0; j < p_dimensions; j++) {
        C[i][j] = (p_statistics[p_dimensions*i+j])->Correlation();
      }
    }

    JAMA::Eigenvalue<double> E(C);
    TNT::Array2D<double> D, V;

    E.getD(D);
    E.getV(V);
    p_transform = TNT::Array2D<double>(V.dim1(), V.dim2());

    // The transform matrix needs to have the eigenvectors
    //  sorted in *descending* order
    //  So we need to reverse the order of V
    //   which is sorted in *ascending* order
    for(int i = 0; i < V.dim1(); i++) {
      for(int j = 0; j < V.dim2(); j++) {
        p_transform[i][j] = V[i][V.dim2()-j-1];
      }
    }

    ComputeInverse();

    p_hasTransform = true;
  }

  // Use LU decomposition to compute the inverse
  void PrincipalComponentAnalysis::ComputeInverse() {
    TNT::Array2D<double> id(p_transform.dim1(), p_transform.dim2(), 0.0);
    for(int i = 0; i < p_transform.dim1(); i++) id[i][i] = 1;

    JAMA::LU<double> lu(p_transform);
    if(lu.det() == 0.0) {
      std::string m = "Cannot take the inverse of the transform matrix";
      throw IException(IException::Programmer, m, _FILEINFO_);
    }

    p_inverse = lu.solve(id);
  }

  // Transform the vector into principal component space
  TNT::Array2D<double> PrincipalComponentAnalysis::Transform(TNT::Array2D<double> data) {
    if(data.dim1() != 1 || data.dim2() != p_dimensions) {
      std::string m = "Transform input must be of dimension 1 x " +  std::to_string(p_dimensions);
      throw IException(IException::Programmer, m, _FILEINFO_);
    }

    // the vector times the transform matrix
    return TNT::matmult(data, p_transform);
  }

  // Transform the vector from principal component space
  TNT::Array2D<double> PrincipalComponentAnalysis::Inverse(TNT::Array2D<double> data) {
    if(data.dim1() != 1 || data.dim2() != p_dimensions) {
      std::string m = "Transform input must be of dimension 1 x " +  std::to_string(p_dimensions);
      throw IException(IException::Programmer, m, _FILEINFO_);
    }

    // the vector times the inverse matrix
    return TNT::matmult(data, p_inverse);
  }
}
