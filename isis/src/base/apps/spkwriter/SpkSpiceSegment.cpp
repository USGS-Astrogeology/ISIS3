/**
 * @file
 * $Revision: 6715 $
 * $Date: 2016-04-28 10:58:43 -0700 (Thu, 28 Apr 2016) $
 * $Id: SpkSpiceSegment.cpp 6715 2016-04-28 17:58:43Z tsucharski@GS.DOI.NET $
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are
 *   public domain. See individual third-party library and package descriptions
 *   for intellectual property information, user agreements, and related
 *   information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or
 *   implied, is made by the USGS as to the accuracy and functioning of such
 *   software and related material nor shall the fact of distribution
 *   constitute any such warranty, and no responsibility is assumed by the
 *   USGS in connection therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html
 *   in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include <SpiceUsr.h>

#include "Camera.h"
#include "Cube.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "NaifStatus.h"
#include "SpkSpiceSegment.h"

using namespace std;

namespace Isis {


  /** Initialize  */
SpkSpiceSegment::SpkSpiceSegment() {
  init();
}

/** Initialize with a cube extracting BLOB content */
SpkSpiceSegment::SpkSpiceSegment(Cube &cube) {
  init(cube);
}

/** Set the segment Id that will be written to the kernel */
void SpkSpiceSegment::setId(const QString &name) {
  _name = name;
  return;
}



/**
 * @brief Provide on-demand loading of a kernel type in the NAIF pool
 *
 * This method provides the ability for users to load particular kernels
 * associated with an ISIS cube (or segment) when the need arises.  This
 * commonly occurs when transforming states and/or frames and body ids.  This is
 * implemented through the Kernels class so that documentation would be useful
 * to review for usage details.
 *
 *
 * @param ktypes  Optional string type of kernel to load (e.g., "LSK,FK").
 *
 * @return int  Number of kernels loaded for the requested type
 */
int SpkSpiceSegment::LoadKernelType(const QString &ktypes) const {
  return (_kernels.Load(ktypes));
}

/**
 * @brief Unload specific kernels from the NAIF pool
 *
 * This is the compliment of the LoadKernelType() method that will unload
 * kernels that were requested through that method.
 *
 * @param ktypes   Optional string type of kernel to unload (e.g., "LSK,FK").
 *
 * @return int Number kernels unloaded
 */
int SpkSpiceSegment::UnloadKernelType(const QString &ktypes) const {
  return (_kernels.UnLoad(ktypes));
}

/**
 * @brief Initializes an ISIS cube converting it into a SPICE segment
 *
 * This method is called to extract the perinent contents of an ISIS cube file
 * and accumulate generic information that is used to create the output SPICE
 * kernel segment.  Other specific kernel types can use this class as its base
 * class and add to it additional elements to complete the needed content for
 * the NAIF kernel.
 *
 * @param cube ISIS cube file to accumulate information from
 */
void SpkSpiceSegment::init(Cube &cube) {

  _kernels.UnLoad();  // Unload all active, owned kernels
  init();            // Init local variables

  _fname = cube.fileName();

  //  Extract ISIS CK blob and transform to CK 3 content
  NaifStatus::CheckErrors();
  try {

    // Order is somewhat important here.  The call to initialize Kernels
    // object checks the NAIF pool for existance.  It logs their NAIF
    // status as loaded which may cause trouble from here on...
    Pvl *label = cube.label();
    _kernels.Init(*label);
    Camera *camera = cube.camera();

    //  Determine segment ID from product ID if it exists, otherwise basename
    if ( _name.isEmpty() ) {
      _name = getKeyValue(*label, "ProductId");
      if (_name.isEmpty() ) {
        _name = QString::fromStdString(FileName(_fname.toStdString()).baseName());
      }
    }

    // Get instrument and target ids
    QString value("");
    value = getKeyValue(*label, "InstrumentId");
    if (!value.isEmpty()) { _instId = value; }
    value = getKeyValue(*label, "TargetName");
    if (!value.isEmpty()) { _target = value; }

    // Get default times for sorting purposes
    setStartTime(camera->cacheStartTime().Et());
    setEndTime(camera->cacheEndTime().Et());

  } catch ( IException &ie  ) {
    ostringstream mess;
    mess << "Failed to construct Spice Segment basics from ISIS file " << _fname;
    throw IException(ie, IException::User, mess.str(), _FILEINFO_);
  }

  return;
}

/**
 * @brief Get specified keyword values from an ISIS label
 *
 * This routine provides access to an ISIS label w/out regard for structure.  In
 * other words, it will traverse the label looking for the first occurance of
 *the specified keyword and return the first value of the first occurance.
 *
 *
 * @param label   Label to search the keyword
 * @param keyword Nanme of keyword to find
 *
 * @return QString Returns first value in the found keyword.  If the keyword
 *         does not exist, an empty string is returned.
 */
QString SpkSpiceSegment::getKeyValue(PvlObject &label,
                                      const QString &keyword) {
  std::string value("");
  if ( label.hasKeyword(keyword.toStdString(), Pvl::Traverse) ) {
    value = label.findKeyword(keyword.toStdString(), Pvl::Traverse)[0];
  }
  return QString::fromStdString(value);
}


/**
 * @brief Reentrant initializer for the variables of this object
 *
 * All variables are set to their respective defaults.  Strings are empty,
 * binaries are 0 and the Kernels object is cleared.
 *
 */
void SpkSpiceSegment::init() {
  _name = _fname = _instId = _target = "";
  _startTime = _endTime = 0.0;
  _utcStartTime = _utcEndTime = "";
  _kernels.Clear();
}

/**
 * @brief Retrieve and convert image times from labels
 *
 * This method retrieves the start and end times of the image observation from
 * the labels.  It mimicks what the spiceinit application does when making this
 * determination.
 *
 * @param lab   Label to get times from
 * @param start Returns start time of the image from the Instrument/StartTime
 *              keyword if it exists
 * @param end   Returns end time of the image from the Instrument/EndTime
 *              keyword if it exists
 *
 * @return bool Always returns true
 */
bool SpkSpiceSegment::getImageTimes(Pvl &lab, double &start, double &end) const {

  _kernels.Load("LSK,SCLK");
  PvlObject &cube = lab.findObject("IsisCube");
  // Get the start and end time for the cube
  start = UTCtoET(QString::fromStdString(cube.findGroup("Instrument")["StartTime"]));
  if(cube.findGroup("Instrument").hasKeyword("StopTime")) {
    end = UTCtoET(QString::fromStdString(cube.findGroup("Instrument")["StopTime"]));
  }
  else {
    end = UTCtoET (QString::fromStdString(cube.findGroup("Instrument")["StartTime"]));
  }

  return (true);
}


/**
 * @brief Add elements to top and bottom of a matrix
 *
 * This method is to expand a matrix to add additional records for padding
 * purposes. The parameter ntop indicate the number to add to the top of the
 * matrix. nbot indicates the number to add to the bottom
 *
 * Elements added to the top have the contents of the first element of the
 * input matrix copied to it. Elements added to the bottom have the last
 * element copied to it.
 *
 * The new matrix has the contents of the original copied to it place
 * immediately after the number of elements added to it.
 *
 * @author Kris Becker - 4/6/2011
 *
 * @param ntop Number of elements to add to the top
 * @param vec  number of elements to add to the bottom
 * @param matrix Matrix to add elements to
 *
 *  @return SpkSpiceSegment::SMatrix Expanded matrix
 */
SpkSpiceSegment::SMatrix SpkSpiceSegment::expand(int ntop, int nbot,
                                           const SpkSpiceSegment::SMatrix &matrix)
                                           const {
  //  Add lines to matrix at top and bottom
  int ndim(matrix.dim1());
  ntop = std::max(0, ntop);
  nbot = std::max(0, nbot);
  int nlines(ndim+ntop+nbot);
  SMatrix mat(nlines, matrix.dim2());


  // Duplicate top lines from first input matrix line
  for (int n = 0 ; n < ntop ; n++) {
    for (int s = 0 ; s < matrix.dim2() ; s++) {
      mat[n][s] = matrix[0][s];
    }
  }

  // Copy the contents of the input matrix to the output
  for (int n = 0 ; n < ndim ; n++) {
    for (int s = 0 ; s < matrix.dim2() ; s++) {
      mat[n+ntop][s] = matrix[n][s];
    }
  }

  // Duplicate bottom lines from last input matrix lines
  for (int n = 0 ; n < nbot ; n++) {
    for (int s = 0 ; s < matrix.dim2() ; s++) {
      mat[nlines-1-n][s] = matrix[ndim-1][s];
    }
  }

  return (mat);
}

/**
 * @brief Add elements to top and bottom of a vector
 *
 * This method is to expand a vector to add additional records for padding
 * purposes. The parameter ntop indicate the number to add to the top of the
 * vector. nbot indicates the number to add to the bottom
 *
 * Elements added to the top have the contents of the first element of the
 * input vector copied to it. Elements added to the bottom have the last
 * element copied to it.
 *
 * The new vector has the contents of the original copied to it place
 * immediately after the number of elements added to it.
 *
 * @author Kris Becker - 4/6/2011
 *
 * @param ntop Number of elements to add to the top
 * @param vec  number of elements to add to the bottom
 * @param vector Vector to add elements to
 *
 *  @return SpkSpiceSegment::SVector Expanded vector
 */
SpkSpiceSegment::SVector SpkSpiceSegment::expand(int ntop, int nbot,
                                           const SpkSpiceSegment::SVector &vec)
                                           const {
  //  Add lines to matrix at top and bottom
  int ndim(vec.dim1());
  ntop = std::max(0, ntop);
  nbot = std::max(0, nbot);
  int nvals(ndim+ntop+nbot);
  SVector myvec(nvals);

  // Duplicate top elements to expanded elements
  for (int n = 0 ; n < ntop ; n++) {
    myvec[n] = vec[0];
  }

  // Copy elements from input vector to output
  for (int n = 0 ; n < vec.dim1() ; n++) {
    myvec[n+ntop] = vec[n];
  }


  // Duplicate bottom elements to expanded elements
  for (int n = 0 ; n < nbot ; n++) {
    myvec[nvals-1-n] = vec[ndim-1];
  }

  return (myvec);
}


/** Sets start time  */
void SpkSpiceSegment::setStartTime(double et) {
  _startTime = et;
  _utcStartTime = toUTC(_startTime);
}

/** Sets end time */
void SpkSpiceSegment::setEndTime(double et) {
  _endTime = et;
  _utcEndTime = toUTC(_endTime);
}

/**
 * @brief Convert NAIF code to frame or body name
 *
 * This routine will convert a NAIF integer code to either the frame (first) or
 * body (second) name.
 *
 * @author kbecker (3/26/2011)
 *
 * @param naifid NAIF integer code to convert to a name
 *
 * @return QString Returns the frame or body name.
 */
QString SpkSpiceSegment::getNaifName(int naifid) const {
  SpiceChar naifBuf[40];

  NaifStatus::CheckErrors();
  frmnam_c ( (SpiceInt) naifid, sizeof(naifBuf), naifBuf);
  string cframe(naifBuf);

  if ( cframe.empty() ) {
    SpiceBoolean found;
    bodc2n_c((SpiceInt) naifid, sizeof(naifBuf), naifBuf, &found);
    if ( found ) cframe = naifBuf;
  }

  // If it fails, just report it missing
  if ( cframe.empty() ) {
    string mess = "Failed to convert FrameId (" + IString(naifid) +
                  ") to string - perhaps the frame kernel is missing or not" +
                  " loaded.";
    cframe = "_UNKNOWN_";
  //  throw iException::Message(iException::User, mess.c_str(), _FILEINFO_);
  }

  NaifStatus::CheckErrors();
  return (cframe.c_str());
}

/** Converts and ET time to UTC string */
QString SpkSpiceSegment::toUTC(const double &et) const {
  const int UTCLEN = 80;
  char utcout[UTCLEN];

  NaifStatus::CheckErrors();
  et2utc_c(et, "ISOC", 3, UTCLEN, utcout);
  NaifStatus::CheckErrors();

  return (QString(utcout));
}

/** Converts a UTC time string to ET  */
double SpkSpiceSegment::UTCtoET(const QString &utc) const {
  SpiceDouble et;

  NaifStatus::CheckErrors();
  utc2et_c(utc.toLatin1().data(), &et);
  NaifStatus::CheckErrors();

  return (et);
}

};  // namespace Isis
