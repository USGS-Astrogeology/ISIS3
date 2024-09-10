/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "SumFile.h"

#include <cmath>
#include <numeric>
#include <string>
#include <vector>

#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QtGlobal>

// boost library
#include <boost/foreach.hpp>

#include "Camera.h"
#include "IException.h"
#include "IString.h"
#include "Kernels.h"
#include "NaifStatus.h"
#include "Progress.h"


using namespace std;

namespace Isis {

  /**
   *  Default constructor creates empty SumFile object.
   */
  SumFile::SumFile()  { }


  /**
   * @brief Constructor reads and parses contents of SUMFILE
   *
   * @param sumFile Name of SUMFILE to read/parse
   */
  SumFile::SumFile(const QString &sumFile) {
    parseSumFile(sumFile);
  }


//  /**
//   *  Tests vailidty of the SumFile point.
//   *
//   *  @return @b bool
//   */
//  bool SumFile::isValid() const {
//    return (true);
//  }


  /**
   *  Returns the name of the SUMFILE (contents of first line).
   *
   *  @return @b QString Name of sumfile identifier
   */
  QString SumFile::name() const {
    return (m_id);

  }


  /**
   *  Returns the time found in the SUMFILE in UTC form
   *
   *  @return @b QString Time found in SUMFILE
   */
  QString SumFile::utc() const {
    return (m_obsTime.UTC());

  }


  /**
   *  Returns the SUMFILE time, in ET.
   *
   *  @return @b double Time converted to ET
   */
  double  SumFile::et() const {
    return (m_obsTime.Et());
  }

/**
 * @brief Returns the Time object found in SUMFILE
 *
 * @return const iTime& Time in SUMFILE
 */
  const iTime &SumFile::time() const {
    return ( m_obsTime );
  }

/**
 * @brief Update SPICE data in ISIS cube
 *
 * This method will update the SPICE blobs with the contents of the SUMSPICE.
 * The contents of the InstrumentPosition will be replaced with SUMSPICE SZ
 * vector and InstrumentPointing is replaced by the CX, CY, CZ matrix.
 *
 * @param cube    An intialized ISIS Cube object
 * @param camera  An option pointer to the camera. If not provided, it is
 *                retrieved from the Cube object
 *
 * @return bool   True if succesful, false if the operation fails
 */
  bool SumFile::updateSpice(Cube &cube, Camera *camera) const {
    bool good = updatePointing(cube, camera);
    if ( good ) {
     good = updatePosition(cube, camera);
    }
    return (good);
  }


  /**
   * @brief Convert the SUMFILE contents and apply pointing to ISIS camera
   *
   * It is up to the caller to ensure appropriate kernels are loaded. See
   * loadKernels();
   *
   * @param cube    Cube to update pointing for
   * @param camera  Camera object associated with cube for update
   *
   * @return bool  True if successful, false otherwise
   */
  bool SumFile::updatePointing(Cube &cube, Camera *camera) const {

    Camera *mycam = ( camera != NULL ) ? camera : cube.camera();

    // get new pointing quaternion from sum file

    // first, we get the rotattion between j2000 and target (i.e. body fixed frame)
    SpiceRotation *body = mycam->bodyRotation();
    Quaternion j2000ToTarget(body->Matrix());

    // next, get the constant rotation for the camera from the cube's table
    SpiceRotation *oldRotation = mycam->instrumentRotation();
    Quaternion oldConstantRotation(oldRotation->ConstantRotation()); // old TC rotation

    // Get the new rotation from the sum file.
    // this is the instrument frame (relative to the target, i.e. body-fixed)
    Quaternion newRotation = getPointing();

    // new target to instrument rotation (TC) is found by
    //     TC = inverse(rotation from sum file) * (old TC rotation)
    //
    // So, the new instrument frame relative to J2000 is
    //     CJ = inverse(TC) * TJ
    Quaternion newTimeBasedRotation = oldConstantRotation.Conjugate() * newRotation * j2000ToTarget; // new CJ

    Table table = oldRotation->Cache("InstrumentPointing");
    if ( table.Records() > 1 ) {
      std::string message = "Expected/support only one InstrumentPointing record "
                     "(i.e., Framing camera) but got " +
                     toString(table.Records()) + " for file " + cube.fileName().toStdString();
      throw IException(IException::User, message, _FILEINFO_);
    }

    TableRecord &rec0 = table[0];
    if ( rec0.Fields() < 5 ) {
      std::string message = "Expected/support quaternion Table containing 5 or more fields "
                     "(i.e., Framing camera) but got " +
                     toString(rec0.Fields()) + " for file " + cube.fileName().toStdString();
      throw IException(IException::User, message, _FILEINFO_);
    }

    for (int i = 0; i < 4; i++) {
      rec0[i] = newTimeBasedRotation[i];
    }

    // Put a comment in the record
    std::string message = "Updated pointing with SUMFILE " + name().toStdString() + " on " + iTime::CurrentLocalTime().toStdString();
    PvlKeyword sumFileKeyword("SUMFILE", name().toStdString());
    sumFileKeyword.addComment(message);
    table.Label().addKeyword(sumFileKeyword, PvlContainer::Replace);
    table.Update(rec0, 0);

    cube.write(table);

    // Delete polygon if it has one
    if ( cube.label()->hasObject("Polygon") ) {
      cube.label()->deleteObject("Polygon");
    }

    return (true);
  }


  /**
   * @brief Update spacecraft position with SUMFILE data
   *
   * @param cube    Cube to update position for
   * @param camera  Camera object associated with cube for update
   *
   * @return bool   True if successful, false otherwise
   */
  bool SumFile::updatePosition(Cube &cube, Camera *camera) const {

    Camera *mycam = ( !camera ) ? cube.camera() : camera;

    SpiceRotation *body = mycam->bodyRotation();
    Quaternion j2000ToTarget(body->Matrix());

    // Get body-fixed s/c vector - points from body to s/c
    vector<double> spacecraftPos = getPosition();

    // Have vector point from s/c to body
    vminus_c(&spacecraftPos[0], &spacecraftPos[0]);
    // Transform position vector from body-fixed to J2000
    spacecraftPos = j2000ToTarget.Conjugate().Qxv(spacecraftPos);

    // Retrieve s/c cache and update the position
    SpicePosition *oldPosition = mycam->instrumentPosition();
    Table table = oldPosition->Cache("InstrumentPosition");
    if ( table.Records() > 1 ) {
      std::string message = "Expected/support only one InstrumentPosition record "
                     "(i.e., Framing camera) but got " +
                     toString(table.Records()) + " for file " + cube.fileName().toStdString();
      throw IException(IException::User, message, _FILEINFO_);
    }

    TableRecord &rec0 = table[0];
    if ( rec0.Fields() < 4 ) {
      std::string message = "Expected/support vector Table containing 4 or more fields "
                     "(i.e., Framing camera) but got " +
                     toString(rec0.Fields()) + " for file " + cube.fileName().toStdString();
      throw IException(IException::User, message, _FILEINFO_);
    }

    // Now replace the quaternions in table
    for (unsigned int i = 0; i < spacecraftPos.size(); i++) {
      rec0[i] = spacecraftPos[i];
    }

    // Put a comment in the record
    std::string message = "Updated position with SUMFILE " + name().toStdString() + " on " + iTime::CurrentLocalTime().toStdString();
    PvlKeyword sumFileKeyword("SUMFILE", name().toStdString());
    sumFileKeyword.addComment(message);
    table.Label().addKeyword(sumFileKeyword, PvlContainer::Replace);
    table.Update(rec0, 0);

    cube.write(table);

    // Delete polygon if it has one
    if ( cube.label()->hasObject("Polygon") ) {
      cube.label()->deleteObject("Polygon");
    }

    return (true);

   }



  /**
   * @brief Return the point matrix, in body-fixed format.
   *
   * This method will return a quaternion, in body fixed coordinates,
   * corresponding to the 3x3 instrument pointing matrix found in the SUMFILE.
   * Note that there is no need to load kernels (or have transformations
   * determined) to provide this data.
   *
   * @return Quaternion The SUMFILE's pointing matrix
   */
  Quaternion SumFile::getPointing() const {
    vector<SpiceDouble> cmatrixBodyFixed(9);

    // copy the transposed matrix to a 9 unit vector to be converted to quaternion
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        cmatrixBodyFixed[3*i+j] = m_pointingMatrix[i][j];// ji = instrument to target
      }
    }

    Quaternion quaternionBodyFixed(cmatrixBodyFixed);
    return quaternionBodyFixed;
  }


  /**
   * @brief Get spacecraft postion in body-fixed coordinates
   *
   * This method will return the vector found in the SUMFILE that represents
   * the coordinates of the spacecraft, in body fixed position.
   *
   * @return std::vector<double> The SUMFILE's spacecraft position
   */
  std::vector<double> SumFile::getPosition()  const{
    std::vector<double> spacecraftPosition;
    spacecraftPosition.reserve(3);
    spacecraftPosition.push_back(m_spacecraftPosition[0]);
    spacecraftPosition.push_back(m_spacecraftPosition[1]);
    spacecraftPosition.push_back(m_spacecraftPosition[2]);
    return (spacecraftPosition);
  }



  /**
   * @brief Get Sun postion in body-fixed coordinates
   *
   * This method will return the vector found in the SUMFILE that represents
   * the coordinates of the Sun, in body fixed position.
   *
   * @return std::vector<double> The SUMFILE's Sun position
   */
  std::vector<double> SumFile::getSunPosition()  const{
    std::vector<double> sunPosition;
    sunPosition.reserve(3);
    sunPosition.push_back(m_sunPosition[0]);
    sunPosition.push_back(m_sunPosition[1]);
    sunPosition.push_back(m_sunPosition[2]);
    return (sunPosition);
  }


  /**
   * Adds information about the SUMFILE to the output stream: name, start time
   * (UTC), start time (ET), number of lines, number of samples, DN minimum, DN
   * maximum.
   *
   * @param outs The output Stream to be filled.
   *
   * @return @b std::ostream& The filled output stream.
   */
  std::ostream &SumFile::brief(std::ostream &outs) const {
    outs << name().toStdString() << ", " << utc().toStdString() << ", "  << std::fixed << et() << ", "
         << m_numLines << ", " << m_numSamples
         << ", " << m_dnMin << ", " << m_dnMax;
    return (outs);
  }


  /**
   * @brief Opens and parses the contents of a SUMFILE.
   *
   * THis method will open a Gaskell SPC SUMFILE and parse the contents. The
   * SUMFILE contents are internalized in the SumFile object.
   *
   * @param sumFile Name of a SPC SUMFILE
   */
  void SumFile::parseSumFile(const QString &sumFile) {
    FileName sfile(sumFile.toStdString());
    QFile sumF(QString::fromStdString(sfile.expanded()));
    if ( !sumF.open(QIODevice::ReadOnly | QIODevice::Text) ) {
      std::string message = "Failed to open FROM file \"" + sfile.original() + "\"";
      throw IException(IException::User, message, _FILEINFO_);
    }

    // Open the text stream
    QTextStream sumin(&sumF);

    // Get the image id
    QStringList values = getSumLine(sumin.readLine(), 1);
    m_id  = values.first();

    // Get the time. We expect spaces in the time specification
    values = getSumLine(sumin.readLine(), 4);
    m_obsTime = iTime(values.join("-"));

    // Get image lines/samples and lower/upper dn boundaries
    values = getSumLine(sumin.readLine(), 7, "THRSH");
    m_numSamples = cvtDouble(values[0]);
    m_numLines = cvtDouble(values[1]);
    m_dnMin  = cvtDouble(values[2]);
    m_dnMax  = cvtDouble(values[3]);

    // Get pixel size and boresight line/sample
    values = getSumLine(sumin.readLine(), 5, "CTR");
    m_pxlSize = cvtDouble(values[0]);
    m_centerSample   = cvtDouble(values[1]);
    m_centerLine   = cvtDouble(values[2]);

    // Get spacecraft position
    values = getSumLine(sumin.readLine(), 4, "SCOBJ");
    for (int i = 0; i < 3; i++) {
      m_spacecraftPosition[i] = cvtDouble(values[i]);
    }

    // Get X component of pointing matrix
    values = getSumLine(sumin.readLine(), 4, "CX");
    for (int i = 0; i < 3; i++) m_pointingMatrix[0][i] = cvtDouble(values[i]);

    // Get Y component of pointing matrix
    values = getSumLine(sumin.readLine(), 4,"CY");
    for (int i = 0; i < 3; i++) m_pointingMatrix[1][i] = cvtDouble(values[i]);

    // Get Z component of pointing matrix
    values = getSumLine(sumin.readLine(), 4, "CZ");
    for (int i = 0; i < 3; i++) m_pointingMatrix[2][i] = cvtDouble(values[i]);


    // Get sun position
    values = getSumLine(sumin.readLine(), 4, "SZ");
    for (int i = 0; i < 3; i++) m_sunPosition[i] = cvtDouble(values[i]);

    // Get K-matrix
    values = getSumLine(sumin.readLine(), 7, "K-MATRIX");
    for (int i = 0; i < 6; i++) m_kmatrix[i] = cvtDouble(values[i]);

    // Get distortion parameters or SIGPXL values.
    values = getSumLine(sumin.readLine(), 0);

    if ( values.last() == "DISTORTION" ) {
      for (int i = 0; i < 4; i++) m_distortion[i] = cvtDouble(values[i]);
    }

    // Get formal spacecraft position uncertainty
    values = getSumLine(sumin.readLine(), 4, "SIGMA_VSO");
    for (int i = 0; i < 3; i++) m_sigmaSCPos[i] = cvtDouble(values[i]);


    // Get formal pointing uncertainty
    values = getSumLine(sumin.readLine(), 4, "SIGMA_PTG");
    for (int i = 0; i < 3; i++) m_sigmaSCPos[i] = cvtDouble(values[i]);

    // Should be at the landmarks line in the file now.  This test ensures it
    values = getSumLine(sumin.readLine(), 1, "LANDMARKS");

    // Read all landmarks (ControlMeasures) in this section until we encounter
    //limb fits...

    // Read to end of file
    while ( !sumin.atEnd() ) {
      values = getSumLine(sumin.readLine());
    }

   return;
  }


  /**
   * @brief Get referred input line from SUMFILE line
   *
   * This method will tokenize a simple string with optional error checking for
   * known line content. The string is expected to be line of a Gaskell SUMFILE.
   *
   * Each data string has control characters removed, commas converted to spaces
   * and multiple spaces compressed to a single space. The string is then
   * tokenized with each value separated by a space.
   *
   * Callers can use the nexpected parameter to ensure the expected fields are
   * read from the input stream. If 0, it will accept any number of tokens
   * (including 0). If the expected number of tokens are not met, an exception is
   * thrown to indicate unexpected input.
   *
   * The tag parameter is provided to check for a specific value in the last
   * column of a parsed SUMFILE line. The tag string is compared to the last
   * field string parsed from the processed input data string using a case
   * insensitve comparison. If the values are not equivalent, an exception is
   * thrown. If a value is not provided, this check is not applied.
   *
   * @param data       Line from a Gaskell SUMFILE
   * @param nexpected  Optional number of columns/fields after tokenization. a
   *                   value of 0 will not apply this check.
   * @param tag        If specified, is the expect string in last column of a
   *                   parsed SUMFILE line. String comparison is case
   *                   insensitive.
   *
   * @return QStringList List of tokenized string fields
   */
  QStringList SumFile::getSumLine(const QString &data, const int &nexpected,
                                  const QString &tag) const {

    // Copy string and parse results
    QString line = data.simplified();
    line.replace(",", " ");
    QStringList values = line.split(" ", Qt::SkipEmptyParts);

    if ( nexpected > 0 ) {
      if ( values.size() != nexpected ) {
        std::string message = "Expected " + toString(nexpected) + " but got "
                       + toString(values.size());
        throw IException(IException::User, message, _FILEINFO_);
      }
    }

    // Check for tags
    if ( !tag.isEmpty() ) {
      if ( values.size() > 0 ) {
        if ( values.last().toLower() != tag.toLower() ) {
          std::string message = "Expected line tag given (" + tag.toStdString() +
                         ") does not match contents (" + values.last().toStdString() + ")";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
      else {
        std::string message = "Line tag given (" + tag.toStdString() + ") but line has not values";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }

    return (values);
  }


  /**
   * @brief Convert text to double value
   *
   * This method will convert special forms of double text value formats to binary
   * form. It is particulary useful when handle text of the form 2.440D+3". This
   * format is not directly supported by conversion facilities.  The "D" is
   * converted to "e" in these cases.
   *
   * @param value Text value to convert
   *
   * @return double Converted text value
   */
  double SumFile::cvtDouble(const QString &value) const {
    QString dblstr(value);
    return (dblstr.replace("D","e").toDouble());
  }


  /**
   * @brief Load a list of SUMFILEs from a file
   *
   * This static method loads a list of SUMFILEs contained in a file. Each line in
   * the file is assumsed to be a SUMFILE and will be initialized using a new
   * SumFile object for each file contained in the list.
   *
   * @param sumFiles     Name of file containing SUMFILEs
   *
   * @return SumFileList List of SumFile objects created from SUMFILE list
   */
  SumFileList loadSumFiles(const FileList &sumFiles) {
    Progress progress;
    progress.SetText("Loading Sum File List");
    progress.SetMaximumSteps(sumFiles.size());
    SumFileList sum_list;

    for (int f = 0; f < sumFiles.size(); f++) {
      progress.CheckStatus();
      SharedSumFile sumF(new SumFile(QString::fromStdString(sumFiles[f].original())));
      sum_list.append(sumF);
    }

    return (sum_list);
  }

}
// namespace Isis
