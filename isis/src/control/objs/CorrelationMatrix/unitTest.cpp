/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iostream>

#include <boost/numeric/ublas/symmetric.hpp>

#include <QDataStream>
#include <QDebug>
#include <QFile>

#include "CorrelationMatrix.h"
#include "IException.h"
#include "Preference.h"
#include "PvlObject.h"
#include "Pvl.h"
#include "SparseBlockMatrix.h"

using namespace std;
using namespace boost::numeric::ublas;
using namespace Isis;

int main() {
  Preference::Preferences(true);
  qDebug() << "Test Default Constructor";

  CorrelationMatrix corrMat;

  qDebug() << "Test Copy Constructor";

  try {
    CorrelationMatrix corrMat2(corrMat);
  }
  catch (IException &e) {
    throw IException(IException::Unknown,
                     "Failed to create object with copy constructor",
                     _FILEINFO_);
  }

  qDebug() << "Test Equal Operator ( = )";

  try {
    CorrelationMatrix corrMat3 = corrMat;
  }
  catch (IException &e) {
    throw IException(IException::Unknown,
                     "Failed to copy object using equal operator",
                     _FILEINFO_);
  }

  qDebug() << "Does the matrix exist?" << corrMat.isValid();

  qDebug() << "Creating Covariance Matrix.";
  FileName corrFile = "corrMat.dat";
  FileName covFile = "covMat.dat";

  corrMat.setCorrelationFileName(corrFile);
  qDebug() << "Does the Correlation Matrix have a covariance matrix? "  <<  corrMat.hasCovMat();
  corrMat.setCovarianceFileName(covFile);
  qDebug() << "Does the Correlation Matrix have a covariance matrix now that it has been set? "
           <<  corrMat.hasCovMat();


  // Make Dummy Covariance Matrix
  SparseBlockMatrix tmpMat;
  tmpMat.setNumberOfColumns(2);
  tmpMat.insertMatrixBlock(0, 0, 2, 2);
  tmpMat.insertMatrixBlock(1, 0, 2, 2);
  tmpMat.insertMatrixBlock(1, 1, 2, 2);
  int initVal = 1;
  for (int sbcm = 0; sbcm < 2; sbcm++) {
    for (int block = 0; block < tmpMat[sbcm]->size(); block++) {
      for (int row = 0; row < 2; row++) {
        for (int column = 0; column < 2; column++) {
          (*(*tmpMat[sbcm])[block])(row, column) = initVal;
          initVal++;
        }
      }
    }
  }

  qDebug() << "Covariance Matrix:";
  qDebug() << tmpMat;

  QFile covQFile( QString::fromStdString(covFile.expanded()) );
  covQFile.open(QIODevice::ReadWrite);
  QDataStream dataStream(&covQFile);

  foreach (SparseBlockColumnMatrix *col, tmpMat) {
    dataStream << *col;
  }
  covQFile.close();

  qDebug() << "Testing Compute Correlation Matrix Method.";
  corrMat.computeCorrelationMatrix();
  qDebug() << "Tesing Exception in Compute Correlation Matrix Method";

  CorrelationMatrix  cm;
  try {
    cm.computeCorrelationMatrix();
  }
  catch (IException &e) {
    qDebug() << "Exception 1:";
    qDebug().noquote() << QString::fromStdString(e.toString());

  }

  qDebug() << "Correlation Matrix:";
  foreach( SparseBlockColumnMatrix sbcm, *corrMat.visibleBlocks() ) {
    qDebug() << sbcm;
  }

  qDebug() << "Does the matrix exist?" << corrMat.isValid();

  QMap<QString, QStringList> imgsAndParams;

  QStringList param;
  param << "X" << "Y" ;

  QString img = "Image1";
  imgsAndParams.insert(img, param);
  img = "Image2";
  imgsAndParams.insert(img, param);

  corrMat.setImagesAndParameters(imgsAndParams);

  qDebug() << "";
  qDebug() << "***Correlation Matrix Information***";
  qDebug() << "";
  qDebug() << "Covariance File Name:" << QString::fromStdString(corrMat.covarianceFileName().expanded());
  qDebug() << "Correlation File Name:" << QString::fromStdString(corrMat.correlationFileName().expanded());
  qDebug() << "Images and Parameters:";
  QMapIterator<QString, QStringList> imgIt( *corrMat.imagesAndParameters() );
  while ( imgIt.hasNext() ) {
    imgIt.next();
    qDebug() << "\t" << imgIt.key();
    foreach ( QString param, imgIt.value() ) {
      qDebug() << "\t\t" << param;
    }
  }

   qDebug() << "Testing pvlObject()";

   PvlObject corrMatObject( corrMat.pvlObject() );
   Pvl tester;
   tester += corrMatObject;
   cout << tester;

   qDebug() << "Testing constructor that takes a pvlObject";

   try {
     CorrelationMatrix corrMat4(corrMatObject);

     qDebug() << "";
     qDebug() << "***Correlation Matrix Information***";
     qDebug() << "";
     qDebug() << "Covariance File Name:" << QString::fromStdString(corrMat4.correlationFileName().expanded());
     qDebug() << "Correlation File Name:" << QString::fromStdString(corrMat4.correlationFileName().expanded());
     qDebug() << "Images and Parameters:";
       QMapIterator<QString, QStringList> imgIt( *corrMat4.imagesAndParameters() );
     imgIt.toFront();
     while ( imgIt.hasNext() ) {
       imgIt.next();
       qDebug() << "\t" << imgIt.key();
       foreach ( QString param, imgIt.value() ) {
         qDebug() << "\t\t" << param;
       }
     }
   }
   catch (IException &e) {
     std::string msg = "Failed to create object using CorrelationMatrix(PvlObject).";
     throw IException(e, IException::Programmer, msg, _FILEINFO_);
   }


   qDebug() << "Testing CorrelationMatrix(PvlObject storedMatrixData)";


   PvlObject exception1("EmptyObject");
   PvlObject exception2("CorrelationMatrixData");


   PvlObject exception3("CorrelationMatrixData");
   exception3 += PvlKeyword("CovarianceMatrixFileName","CovMatFileName");

   PvlObject exception4("CorrelationMatrixData");
   exception4 += PvlKeyword("CovarianceMatrixFileName","CovMatFileName");
   exception4 += PvlKeyword("CorrelationMatrixFileName","CorrMatFileName");


   try {
      CorrelationMatrix except1(exception1);
   }

   catch (IException &e) {
     qDebug() << "Exception 1";
     qDebug().noquote() << QString::fromStdString(e.toString());
   }

   try {
      CorrelationMatrix except2(exception2);
   }

   catch (IException &e) {
     qDebug() << "Exception 2";
     qDebug().noquote() << QString::fromStdString(e.toString());
   }

   try {
      CorrelationMatrix except3(exception3);
   }

   catch (IException &e) {
     qDebug() << "Exception 3";
     qDebug().noquote() << QString::fromStdString(e.toString());
   }

   try {
      CorrelationMatrix except4(exception4);
   }

   catch (IException &e) {
     qDebug() << "Exception 4";
     qDebug().noquote() << QString::fromStdString(e.toString());
   }



  //delete mat files
  covQFile.remove();
  QFile(QString::fromStdString(corrMat.correlationFileName().expanded())).remove();

}
