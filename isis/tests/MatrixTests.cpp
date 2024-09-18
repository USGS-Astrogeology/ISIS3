#include "gtest/gtest.h"
#include "tnt/tnt_array2d.h"

#include "Matrix.h"
#include "IException.h"

#include <QDebug>



namespace Isis {

  TEST (MatrixTests, ConstructorWithDimensions) {
    Matrix A(2, 2, 2);

    ASSERT_EQ(2, A.Rows());
    ASSERT_EQ(2, A.Columns());
    EXPECT_EQ(2, A[0][0]);
    EXPECT_EQ(2, A[0][1]);
    EXPECT_EQ(2, A[1][0]);
    EXPECT_EQ(2, A[1][1]);
  }
  
  
  TEST (MatrixTests, ConstructorWithTNTArray) {
    TNT::Array2D<double> matrix = TNT::Array2D<double>(2, 2, 2);
    Matrix A(matrix);
    
    EXPECT_EQ(2, A[0][0]);
    EXPECT_EQ(2, A[0][1]);
    EXPECT_EQ(2, A[1][0]);
    EXPECT_EQ(2, A[1][1]);
  }


  TEST (MatrixTests, IdentityMatrix) {
    Matrix I = Matrix::Identity(2);
    
    EXPECT_EQ(1, I[0][0]);
    EXPECT_EQ(0, I[0][1]);
    EXPECT_EQ(0, I[1][0]);
    EXPECT_EQ(1, I[1][1]);
  }
  
  
  TEST (MatrixTests, SetValues) {
    Matrix A(2, 2);
    
    A[0][0] = 1;
    A[0][1] = 2;
    A[1][0] = 3;
    A[1][1] = 4;
    
    EXPECT_EQ(1, A[0][0]);
    EXPECT_EQ(2, A[0][1]);
    EXPECT_EQ(3, A[1][0]);
    EXPECT_EQ(4, A[1][1]);
  }


  TEST (MatrixTests, Add) {
    Matrix I = Matrix::Identity(2);
    Matrix A(2, 2, 2);
    Matrix APopI = A + I;
    
    EXPECT_EQ(3, APopI[0][0]);
    EXPECT_EQ(2, APopI[0][1]);
    EXPECT_EQ(2, APopI[1][0]);
    EXPECT_EQ(3, APopI[1][1]);
    
    Matrix API = A.Add(I);
    
    EXPECT_EQ(3, API[0][0]);
    EXPECT_EQ(2, API[0][1]);
    EXPECT_EQ(2, API[1][0]);
    EXPECT_EQ(3, API[1][1]);
  }


  TEST (MatrixTests, Subtract) {
    Matrix I = Matrix::Identity(2);
    Matrix A(2, 2, 2);
    Matrix AMopI = A - I;
    
    EXPECT_EQ(1, AMopI[0][0]);
    EXPECT_EQ(2, AMopI[0][1]);
    EXPECT_EQ(2, AMopI[1][0]);
    EXPECT_EQ(1, AMopI[1][1]);
    
    Matrix AMI = A.Subtract(I);
    
    EXPECT_EQ(1, AMI[0][0]);
    EXPECT_EQ(2, AMI[0][1]);
    EXPECT_EQ(2, AMI[1][0]);
    EXPECT_EQ(1, AMI[1][1]);  
  }


  TEST (MatrixTests, MultiplyByScalar) {
    Matrix A(2, 2, 2);
    Matrix AMSopI = A * 2;
    
    EXPECT_EQ(4, AMSopI[0][0]);
    EXPECT_EQ(4, AMSopI[0][1]);
    EXPECT_EQ(4, AMSopI[1][0]);
    EXPECT_EQ(4, AMSopI[1][1]);
    
    Matrix AMSI = A.Multiply(2);
    
    EXPECT_EQ(4, AMSI[0][0]);
    EXPECT_EQ(4, AMSI[0][1]);
    EXPECT_EQ(4, AMSI[1][0]);
    EXPECT_EQ(4, AMSI[1][1]);  
  }


  TEST (MatrixTests, MultiplyByMatrix) {
    Matrix I = Matrix::Identity(2);
    Matrix A(2, 2, 2);
    Matrix AMMopI = A * I;
    
    EXPECT_EQ(2, AMMopI[0][0]);
    EXPECT_EQ(2, AMMopI[0][1]);
    EXPECT_EQ(2, AMMopI[1][0]);
    EXPECT_EQ(2, AMMopI[1][1]); 
    
    Matrix AMMI = A.Multiply(I);
    
    EXPECT_EQ(2, AMMI[0][0]);
    EXPECT_EQ(2, AMMI[0][1]);
    EXPECT_EQ(2, AMMI[1][0]);
    EXPECT_EQ(2, AMMI[1][1]);  
  }
  
  
  TEST (MatrixTests, MultiplyElementWise) {
    Matrix I = Matrix::Identity(2);
    Matrix A(2, 2, 2);
    Matrix AMMI = A.MultiplyElementWise(I);
    
    EXPECT_EQ(2, AMMI[0][0]);
    EXPECT_EQ(0, AMMI[0][1]);
    EXPECT_EQ(0, AMMI[1][0]);
    EXPECT_EQ(2, AMMI[1][1]);  
  }


  TEST (MatrixTests, ConstructorWithBadArgs) {
    try {
      Matrix A(0, 1);
      FAIL() << "Expected an IException";
    }
    catch(IException &e) {
      EXPECT_TRUE(e.toString().find("Invalid matrix dimensions") != std::string::npos) 
            <<  e.toString();
    }
    catch(...) {
      FAIL() << "Expected error message: Invalid matrix dimensions";
    }
    
    try {
      Matrix A(-1, 1);
      FAIL() << "Expected an IException";
    }
    catch(IException &e) {
      EXPECT_TRUE(e.toString().find("Invalid matrix dimensions") != std::string::npos) 
            <<  e.toString();
    }
    catch(...) {
      FAIL() << "Expected error message: Invalid matrix dimensions";
    }
  }


  TEST (MatrixTests, IdentityConstructorWithBadArgs) {
    try {
      Matrix::Identity(0);
      FAIL() << "Expected an IException";
    }
    catch(IException &e) {
      EXPECT_TRUE(e.toString().find("Invalid matrix dimensions") != std::string::npos) 
            <<  e.toString();
    }
    catch(...) {
      FAIL() << "Expected error message: Invalid matrix dimensions";
    }
    
    try {
      Matrix::Identity(-1);
      FAIL() << "Expected an IException";
    }
    catch(IException &e) {
      EXPECT_TRUE(e.toString().find("Invalid matrix dimensions") != std::string::npos) 
            <<  e.toString();
    }
    catch(...) {
      FAIL() << "Expected error message: Invalid matrix dimensions";
    }
  }


  TEST (MatrixTests, DeterminantWithBadArgs) {
    Matrix A(1, 2);
    
    try {
      A.Determinant();
      FAIL() << "Expected an IException";
    }
    catch(IException &e) {
      EXPECT_TRUE(e.toString().find("Unable to calculate the determinant, the " 
                                                   "matrix is not square") != std::string::npos) <<  e.toString();
    }
    catch(...) {
      FAIL() << "Expected error message: Unable to calculate the determinant, the matrix is not square.";
    }
  }


  TEST (MatrixTests, TraceWithBadArgs) {
    Matrix A(1, 2);
    
    try {
      A.Trace();
      FAIL() << "Expected an IException";
    }
    catch(IException &e) {
      EXPECT_TRUE(e.toString().find("Unable to calculate the trace, the matrix is "
                                                   "not square") != std::string::npos) <<  e.toString();
    }
    catch(...) {
      FAIL() << "Expected error message: Unable to calculate the trace, the matrix is not square.";
    }
  }


  TEST (MatrixTests, AddMatricesWithBadArgs) {
    Matrix A(1, 2);
    Matrix B(2, 2);
    Matrix C(2, 1);
    
    try {
      A + B;
      FAIL() << "Expected an IException";
    }
    catch(IException &e) {
      EXPECT_TRUE(e.toString().find("Incompatible matrix dimensions, cannot add "
                                                   "the matrices") != std::string::npos) <<  e.toString();
    }
    catch(...) {
      FAIL() << "Expected error message: Incompatible matrix dimensions, cannot add the matrices.";
    }
    
    try {
      A + C;
      FAIL() << "Expected an IException";
    }
    catch(IException &e) {
      EXPECT_TRUE(e.toString().find("Incompatible matrix dimensions, cannot add "  
                                                   "the matrices") != std::string::npos) <<  e.toString();
    }
    catch(...) {
      FAIL() << "Expected error message: Incompatible matrix dimensions, cannot add the matrices.";
    }
  }


  TEST (MatrixTests, SubtractMatricesWithBadArgs) {
    Matrix A(1, 2);
    Matrix B(2, 2);
    Matrix C(2, 1);
    
    try {
      A - B;
      FAIL() << "Expected an IException";
    }
    catch(IException &e) {
      EXPECT_TRUE(e.toString().find("Incompatible matrix dimensions, cannot "  
                                                   "subtract the matrices") != std::string::npos) <<  e.toString();
    }
    catch(...) {
      FAIL() << "Expected error message: Incompatible matrix dimensions, cannot subtract the matrices.";
    }
    
    try {
      A - C;
      FAIL() << "Expected an IException";
    }
    catch(IException &e) {
      EXPECT_TRUE(e.toString().find("Incompatible matrix dimensions, cannot "  
                                                   "subtract the matrices") != std::string::npos) <<  e.toString();
    }
    catch(...) {
      FAIL() << "Expected error message: Incompatible matrix dimensions, cannot subtract the matrices.";
    }
  }


  TEST (MatrixTests, MultiplyMatrixWithBadArgs) {
    Matrix A(1, 1);
    Matrix B(2, 1);
    
    try {
      A * B;
      FAIL() << "Expected an IException";
    }
    catch(IException &e) {
      EXPECT_TRUE(e.toString().find("Incompatible matrix dimensions, cannot "  
                                                   "multiply the matrices") != std::string::npos) <<  e.toString();
    }
    catch(...) {
      FAIL() << "Expected error message: Incompatible matrix dimensions, cannot multiply the matrices.";
    }
  }


  TEST (MatrixTests, MultiplyMatrixElementwiseWithBadArgs) {
    Matrix A(1, 2);
    Matrix B(2, 2);
    Matrix C(2, 1);
    
    try {
      A.MultiplyElementWise(B);
      FAIL() << "Expected an IException";
    }
    catch(IException &e) {
      EXPECT_TRUE(e.toString().find("Incompatible matrix dimensions, cannot "  
                                                   "multiply the matrices") != std::string::npos) <<  e.toString();
    }
    catch(...) {
      FAIL() << "Expected error message: Incompatible matrix dimensions, cannot multiply the matrices.";
    }
    
    try {
      A.MultiplyElementWise(C);
      FAIL() << "Expected an IException";
    }
    catch(IException &e) {
      EXPECT_TRUE(e.toString().find("Incompatible matrix dimensions, cannot "  
                                                   "multiply the matrices") != std::string::npos) <<  e.toString();
    }
    catch(...) {
      FAIL() << "Expected error message: Incompatible matrix dimensions, cannot multiply the matrices.";
    }
  }


  TEST (MatrixTests, InverseWithBadArgs) {
    Matrix A(1, 2);
    
    try {
      A.Inverse();
      FAIL() << "Expected an IException";
    }
    catch(IException &e) {
      EXPECT_TRUE(e.toString().find("Unable to calculate the inverse, the matrix "  
                                                   "is not square") != std::string::npos) <<  e.toString();
    }
    catch(...) {
      FAIL() << "Expected error message: Unable to calculate the inverse, the matrix is not square.";
    }
  }


  TEST (MatrixTests, EigenValuesWithBadArgs) {
    Matrix A(1, 2);
    
    try {
      A.Eigenvalues();
      FAIL() << "Expected an IException";
    }
    catch(IException &e) {
      EXPECT_TRUE(e.toString().find("Unable to calculate eigenvalues, the matrix "  
                                                   "is not square") != std::string::npos) <<  e.toString();
    }
    catch(...) {
      FAIL() << "Expected error message: Unable to calculate eigenvalues, the matrix is not square.";
    }
  }


  TEST (MatrixTests, EigenVectorsWithBadArgs) {
    Matrix A(1, 2);
    
    try {
      A.Eigenvectors();
      FAIL() << "Expected an IException";
    }
    catch(IException &e) {
      EXPECT_TRUE(e.toString().find("Unable to calculate eigenvectors, the matrix "  
                                                   "is not square") != std::string::npos) <<  e.toString();
    }
    catch(...) {
      FAIL() << "Expected error message: Unable to calculate eigenvectors, the matrix is not square.";
    }
  }
  
  
}

