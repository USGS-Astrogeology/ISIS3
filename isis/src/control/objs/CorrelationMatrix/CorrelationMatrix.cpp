/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "CorrelationMatrix.h"

#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QList>
#include <QString>
#include <QStringList>
#include <QtCore/qmath.h>

#include "IException.h"
#include "LinearAlgebra.h"
#include "Pvl.h"
#include "PvlObject.h"
#include "SparseBlockMatrix.h"

namespace Isis {
  /**
   * Default Constructor
   */
  CorrelationMatrix::CorrelationMatrix() {
    m_covarianceFileName = new FileName("");
    m_correlationFileName = new FileName("");
    m_visibleBlocks = new QList<SparseBlockColumnMatrix>();
    m_imagesAndParameters = new QMap<QString, QStringList>();
    m_diagonals = new QList<double>();
  }


  /**
   * This constructor will create a CorrelationMatrix object given a pvl.
   *
   * Object = CorrelationMatrix
   *
   *    CovarianceFileName = fileName.dat
   *    CorrelationFileName = fileName.dat
   *
   *    Group = ImagesAndParameters
   *       Image1 = "Parameter1", "Parameter2", "..."
   *       Image2 = "Parameter1", "Parameter2", "..."
   *       Image3 = "Parameter1", "Parameter2", "..."
   *    End_Group
   *
   * End_Object
   *
   * @param storedMatrixData  A PvlObject containing data about the
   * covariance/correlation matrix.
   *
   * @throws IException::User "This Pvl Object does not have the correct correlation information.
   * The Object you are looking for is called CorrelationMatrixData"
   *
   * @throws IException::User "Could not find the Covariance Matrix .dat file name."
   *
   * @throws IException::User "Could not find the Correlation Matrix .dat file name."
   *
   * @throws IException::User "Could not get Images and Parameters from ImagesAndParameters group."
   *
   */
  CorrelationMatrix::CorrelationMatrix(PvlObject storedMatrixData) {
    //m_imagesAndParameters = NULL;
    m_imagesAndParameters = new QMap<QString, QStringList>();
    m_covarianceFileName = new FileName("");
    m_correlationFileName = new FileName("");
    m_diagonals = NULL;
    m_visibleBlocks = NULL;

    if (storedMatrixData.name() != "CorrelationMatrixData") {
      std::string msg = "This Pvl Object does not have the correct correlation information. The Object "
                    "you are looking for is called CorrelationMatrixData.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    try {
      m_covarianceFileName =
          new FileName(storedMatrixData.findKeyword("CovarianceMatrixFileName")[0]);
    }
    catch (IException &e) {
      std::string msg = "Could not find the Covariance Matrix .dat file name.";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }

    try {
      QString corrFileName = QString::fromStdString(storedMatrixData.findKeyword("CorrelationMatrixFileName")[0]);
      if (corrFileName == "NULL") {
        m_correlationFileName = new FileName;
      }
      else {
        m_correlationFileName = new FileName(corrFileName.toStdString());
      }
    }
    catch (IException &e) {
      std::string msg = "Could not find the Correlation Matrix .dat file name.";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }

    try {
      PvlObject::PvlKeywordIterator
          imgsIt = storedMatrixData.findGroup("ImagesAndParameters").begin();
      while ( imgsIt != storedMatrixData.findGroup("ImagesAndParameters").end() ) {
        QStringList params = QString::fromStdString((*imgsIt)[0]).split(",");
        m_imagesAndParameters->insert(QString::fromStdString(imgsIt->name()), params);
        imgsIt++;
      }
    }
    catch (IException &e) {
      std::string msg = "Could not get Images and Parameters from ImagesAndParameters group.";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }
  }


  /**
   * @brief Copy Constructor
   *
   * @param other The CorrelationMatrix to copy.
   */
  CorrelationMatrix::CorrelationMatrix(const CorrelationMatrix &other) {
    m_imagesAndParameters = new QMap<QString, QStringList>(*other.m_imagesAndParameters);
    m_covarianceFileName = new FileName(*other.m_covarianceFileName);
    m_correlationFileName = new FileName(*other.m_correlationFileName);
    m_diagonals = new QList<double>(*other.m_diagonals);
    m_visibleBlocks = new QList<SparseBlockColumnMatrix>(*other.m_visibleBlocks);
  }


  /**
   * Destructor
   */
  CorrelationMatrix::~CorrelationMatrix() {
    delete m_imagesAndParameters;
    m_imagesAndParameters = NULL;

    delete m_covarianceFileName;
    m_covarianceFileName = NULL;

    delete m_correlationFileName;
    m_correlationFileName = NULL;

    delete m_diagonals;
    m_diagonals = NULL;

    delete m_visibleBlocks;
    m_visibleBlocks = NULL;
  }


  /**
   * @brief Equal Operator
   *
   * Should this call the copy constructor???
   *
   * @param other The matrix to assign to this matrix.
   * @return @b CorrelationMatrix Returns the new matrix.
   */
  CorrelationMatrix &CorrelationMatrix::operator=(const CorrelationMatrix &other) {

    if (&other != this) {

      delete m_imagesAndParameters;
      m_imagesAndParameters = NULL;
      m_imagesAndParameters = new QMap<QString, QStringList>(*other.m_imagesAndParameters);

      delete m_covarianceFileName;
      m_covarianceFileName = NULL;
      m_covarianceFileName = new FileName(*other.m_covarianceFileName);

      delete m_correlationFileName;
      m_correlationFileName = NULL;
      m_correlationFileName = new FileName(*other.m_correlationFileName);

      delete m_diagonals;
      m_diagonals = NULL;
      m_diagonals = new QList<double>(*other.m_diagonals);

      delete m_visibleBlocks;
      m_visibleBlocks = NULL;
      m_visibleBlocks = new QList<SparseBlockColumnMatrix>(*other.m_visibleBlocks);

    }

    return *this;
  }


  /**
  * @brief Read covariance matrix and compute correlation values
  * This method reads the covariance matrix in from a file,
  * one SparseBlockColumnMatrix at a time.  It then stores the diagonal values from that column
  * and computes the correlation values. The resulting SparseBlockMatrix is written to a
  * new file, one SparseBlockColumnMatrix at a time.
  *
  * @throws IException::Progammer  "Cannot compute correlation matrix without a specified
  * file name.  Use setCorrelationFileName(FileName) before calling computeCorrelationMatrix()."
  */
  void CorrelationMatrix::computeCorrelationMatrix() {

    if ( !isValid() ) {
      std::string msg = "Cannot compute correlation matrix without a specified file name. Use "
                    "setCorrelationFileName(FileName) before calling computeCorrelationMatrix().";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    delete m_visibleBlocks;
    m_visibleBlocks = NULL;
    m_visibleBlocks = new QList<SparseBlockColumnMatrix>;

    // Create file handle
    QFile matrixInput(QString::fromStdString(m_covarianceFileName->expanded()));
    QFile matrixOutput(QString::fromStdString(m_correlationFileName->expanded()));

    // Open file to write to
    matrixInput.open(QIODevice::ReadOnly);
    matrixOutput.open(QIODevice::WriteOnly);

    // Open Stream
    QDataStream inStream(&matrixInput);
    QDataStream outStream(&matrixOutput);

    double firstParam1 = 0, //starting param for each iteration
            firstParam2 = 0;
    double param1 = 0, // current param for each iteration
            param2 = 0;
    // Read one column at a time
    SparseBlockColumnMatrix sbcm;
    while ( !inStream.atEnd() ) {
      inStream >> sbcm;

      // Store diagonal
      int numOfBlocks = sbcm.size();
      int lastBlock = numOfBlocks - 1;
      int numDiagonals = sbcm[lastBlock]->size1();

      // Get Diagonals
      for (int i = 0; i < numDiagonals; i++) {
        double val = ( *(sbcm[lastBlock]) )(i, i);
        m_diagonals->append(val);
      }

      // compute correlations
      QMapIterator<int, LinearAlgebra::Matrix *> block(sbcm);

      while ( block.hasNext() ) { // each block in the column
        block.next();
        for (int row = 0; row < (int)block.value()->size1(); row++) { // each row in the block
          for (int column = 0; column < (int)block.value()->size2(); column++) { // each column
            // correlation = covariance / (variance1 * variance2)
            ( *block.value() )(row, column) = ( *block.value() )(row, column) /
                                              sqrt( (*m_diagonals)[param1] *
                                                    (*m_diagonals)[param2] );
            param2++;
          }
          param1++;
          param2 = firstParam2;
        }
        firstParam1 += block.value()->size1();
        param1 = firstParam1;
      }
      firstParam1 = 0;  // start each column at first element of diagonal list
      param1 = firstParam1;
      firstParam2 += block.value()->size2();
      param2 = firstParam2;

      outStream << sbcm;
      m_visibleBlocks->append(sbcm);
    }

    // close file
    matrixInput.close();
    matrixOutput.close();
  }



  /**
   * @brief Extract requested area from correlation matrix
   * This method will open the correlation matrix file and read in the blocks that
   * apply to the requested area. It will populate m_visibleElements.
   *
   * @param x first coordinate of the location in the matrix that the user wants to see.
   * @param y second coordinate of the location in the matrix that the user wants to see.
   */
  void CorrelationMatrix::retrieveVisibleElements(int x, int y) {

//     if ( !correlationMatrixExists() ) {
      // call computeCorrelationMatrix
//     }
    // read the values we want from the correlation matrix file.

    // store values by column?
    // return list of values in m_visibleElements
  }



  /**
   * This is the public accessor for the list of elements that should be displayed in the current
   *   view.
   *
   * @return QList<MatrixElement*> The list of currently visible elements.
   *
   */
//   QList<MatrixElement*> CorrelationMatrix::visibleElements() {
//     return *m_visibleElements;
//   }



  /**
   * @brief See if the correlation matrix has already been calculated by checking to see if
   *   the correlation matrix file has been created.
   *
   * @return @b bool Returns True if the correlation matrix has already been set.
   */
  bool CorrelationMatrix::isValid() {

      return !(m_correlationFileName->name() == "" || m_covarianceFileName->name() == "");
  }


  /**
   * @brief Check if the correlation matrix has a covariance matrix
   * This is used to make sure the covariance matrix exists.
   * If it doesn't this class is not valid. If this file exists, we can compute the
   * correlation matrix.
   *
   * @return @b bool Returns True if the covariance matrix exists, and False if it does not.
   */
  bool CorrelationMatrix::hasCovMat() {
    return !(m_covarianceFileName->name() == "");
  }


  // Set Methods
  /**
   * @brief Set the qmap of images and parameters.
   * @param correlationFileName The FileName of the stored correlation matrix data.
   */
  void CorrelationMatrix::setCorrelationFileName(FileName correlationFileName) {
    if (m_correlationFileName == NULL) {
      m_correlationFileName = new FileName(correlationFileName);
    }
    else {
      *m_correlationFileName = correlationFileName;
    }
  }


  /**
   * @brief Set the qmap of images and parameters.
   * @param  covarianceFileName  The FileName of the stored covariance matrix data.
   */
  void CorrelationMatrix::setCovarianceFileName(FileName covarianceFileName) {
    if (m_covarianceFileName == NULL) {
      m_covarianceFileName = new FileName(covarianceFileName);
    }
    else {
      *m_covarianceFileName = covarianceFileName;
    }
    //Make the correlation matrix file name match the covariance matrix file name.
    if (!isValid()) {
      QString fName = QString::fromStdString(covarianceFileName.expanded()).replace("inverse","correlation");
      setCorrelationFileName(FileName(fName.toStdString()));
    }
  }


  /**
   * @brief Set the qmap of images and parameters.
   *
   * @param imagesAndParameters a QMap structure indexed by image keys, with an arbitrary set of
   * parameters for each image.
   */
  void CorrelationMatrix::setImagesAndParameters(QMap<QString, QStringList> imagesAndParameters) {
    if (m_imagesAndParameters == NULL) {
      m_imagesAndParameters = new QMap<QString, QStringList>(imagesAndParameters);
    }
    else {
      *m_imagesAndParameters = imagesAndParameters;
    }
  }


  /**
   * @brief Public access for the correlation matrix file name.
   *
   * @return @b FileName  The FileName of the correlation matrix data file.
   */
  FileName CorrelationMatrix::correlationFileName() {
    return *m_correlationFileName;
  }


  /**
   * @brief Public access for the covariance matrix file name.
   *
   * @return @b FileName The FileName of the covariance data file.
   */
  FileName CorrelationMatrix::covarianceFileName() {
    return *m_covarianceFileName;
  }


  /**
   * @brief Public access for the qmap of images and parameters.
   *
   * @return  @b *QMap<QString,QStringList>  A pointer to the QMap structure containing a list
   * of images (the keys) and their associated parameter values.
   */
  QMap<QString, QStringList> *CorrelationMatrix::imagesAndParameters() {
    return m_imagesAndParameters;
  }


  /**
   * @brief This method will read the matrix in from the file and hold on to the whole thing.
   * This will only be used when the matrix is small enough that this will be useful.
   *
   */
  void CorrelationMatrix::retrieveWholeMatrix() {
//     SparseBlockColumnMatrix sbcm;
//     QFile matrixInput( m_correlationFileName->expanded() );
//     matrixInput.open(QIODevice::ReadOnly);
//     QDataStream inStream(&matrixInput);
//
//     while( !inStream.atEnd() ) {
//       inStream >> sbcm;
//       m_visibleBlocks->append(&sbcm);
//     }
  }


  /**
   * @brief Display only part of a matrix
   * This method will be used when the matrix is too big to display the whole thing.
   * It will read in the block we want to see and the two blocks for the diagonals that belong to
   * the right images.
   */
  void CorrelationMatrix::retrieveThreeVisibleBlocks() {
  }


  /**
   * @brief Get the visible part of the matrix.
   * @return @b QList Returns a list of the non-empty diagonal blocks of the correlation matrix.
   */
  QList<SparseBlockColumnMatrix> *CorrelationMatrix::visibleBlocks() {
    return m_visibleBlocks;
  }


  /**
   * @brief This method creates a Pvl group with the information necessary to recreate
   * this correlation matrix.
   *
   * Object = CorrelationMatrixData
   *   CovarianceMatrixFileName = /location/covarianceTmpFileName.dat
   *   CorrelationMatrixFileName = /location/correlationTmpFileName.dat
   *
   *   Group = ImagesAndParameters
   *     Image1Name = "Param1, Param2, ..., ParamN"
   *     ...
   *     ImageNName = "..."
   *   End_Group
   * End_Object
   *
   * @return @b PvlGroup Returns the information needed to recreate this correlation matrix.
   */
  PvlObject CorrelationMatrix::pvlObject() {
    PvlObject corrMatInfo("CorrelationMatrixData");

    corrMatInfo += PvlKeyword( "CovarianceMatrixFileName", m_covarianceFileName->expanded());
    corrMatInfo += PvlKeyword( "CorrelationMatrixFileName", m_correlationFileName->expanded());

    PvlGroup imgsAndParams("ImagesAndParameters");
    QMapIterator<QString, QStringList> imgParamIt(*m_imagesAndParameters);
    while ( imgParamIt.hasNext() ) {
      imgParamIt.next();
      imgsAndParams += PvlKeyword( imgParamIt.key().toStdString(), imgParamIt.value().join(",").toStdString() );
    }
    corrMatInfo += imgsAndParams;

    return corrMatInfo;
  }
}
