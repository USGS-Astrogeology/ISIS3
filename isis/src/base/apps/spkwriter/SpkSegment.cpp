/**
 * @file
 * $Revision: 6715 $
 * $Date: 2016-04-28 10:58:43 -0700 (Thu, 28 Apr 2016) $
 * $Id: SpkSegment.cpp 6715 2016-04-28 17:58:43Z tsucharski@GS.DOI.NET $
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
#include <algorithm>
#include <functional>
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
#include "Spice.h"
#include "SpkSegment.h"
#include "Table.h"

using namespace std;

namespace Isis {

/** Default constructor */
SpkSegment::SpkSegment() : SpkSpiceSegment() {
  init();
}

/** Constructor from ISIS cube file by name of the cube */
SpkSegment::SpkSegment(const QString &fname, const int spkType) : SpkSpiceSegment() {
  init(spkType);
  Cube cube;
  cube.open(fname);
  SpkSpiceSegment::init(cube);
  import(cube);
}

/** Constructor from ISIS cube object */
SpkSegment::SpkSegment(Cube &cube, const int spkType) : SpkSpiceSegment(cube) {
  init(spkType);
  import(cube);
}

/**
 * @brief Load and process SPICE data from an ISIS cube object
 *
 * This method extracts SPK SPICE date from an ISIS cube object.  This object
 * must have been spiceinit'ed at a minimum and, by definition, have a
 * supporting camera model.
 *
 * SPK data is extracted from the SpicePosition object via the Table it provides
 * of this information.  The SPICE position state vectors are potentially
 * trasformed to the proper state for target body, center body and reference
 * frame.
 *
 * @author kbecker (5/2/2011)
 *
 * @param cube  Cube to generate SPICE segment from
 *
 * @internal
 *   @history 2011-05-27 Debbie A. Cook - Added call to create Hermite cache
 *                           to make this option available for testing.
 *                           This will need to be an option in the future.
 */
void SpkSegment::import(Cube &cube) {
  //typedef std::vector<QString>  StrList;

  //  Extract ISIS SPK blob and transform to requested content
  NaifStatus::CheckErrors();
  try {

    Camera *camera(cube.camera());
    Kernels kernels = getKernels();

    // Load necessary kernels and id frames
    kernels.Load("PCK,LSK,FK,SPK,EXTRA");
    m_body = camera->SpkTargetId();
    m_center = camera->SpkCenterId();
    m_refFrame = getNaifName(camera->SpkReferenceId());
    m_bodyFrame = getNaifName(m_body);
    m_centerFrame = getNaifName(m_center);

    //  Get the SPICE data
    Table spkCache("SpkSegment");
    if ( 9 == m_spkType ) {
      spkCache = camera->instrumentPosition()->LineCache("SpkSegment");
    }
    else if ( 13 == m_spkType ) {
      spkCache = camera->instrumentPosition()->LoadHermiteCache("SpkSegment");
    }
    else {
      std::string mess = "Unsupported SPK kernel type (" +
                     QString::number(m_spkType) + ") - must be 9 or 13.";
      throw IException(IException::User, mess, _FILEINFO_);
    }

    getStates(*camera, load(spkCache), m_states, m_epochs, m_hasVV);

    // Save current time
    SpicePosition *ipos(camera->instrumentPosition());
    double currentTime = ipos->EphemerisTime();

    // Add records with 3 milliseconds padding to top and bottom of records
    const double Epsilon(3.0e-3);

    // Pad the top and bottom of the
    m_states = expand(1, 1, m_states);
    m_epochs = expand(1, 1, m_epochs);

    int ndim1(m_states.dim1());
    int ndim2(m_states.dim2());

    // Add record to top of states
    SVector stateT(ndim2, m_states[1]);  // Map to to first record
    double sTime = m_epochs[1] - Epsilon;
    SVector s0 = makeState(ipos, m_epochs[1], stateT, sTime);
    SVector(ndim2, m_states[0]).inject(s0);
    m_epochs[0] = sTime;

    // Add record to bottom of states
    stateT = SVector(ndim2, m_states[ndim1-2]);
    sTime = m_epochs[ndim1-2] + Epsilon;
    s0 = makeState(ipos, m_epochs[ndim1-2], stateT, sTime);
    SVector(ndim2, m_states[ndim1-1]).inject(s0);
    m_epochs[ndim1-1] = sTime;

    //  Restore saved time and determine degree of NAIF interpolation
    ipos->SetEphemerisTime(currentTime);
    m_degree = std::min((int) MaximumDegree, m_states.dim1()-1);

    //  Ensure the degree is odd and less that the even value
    m_degree = (((m_degree - 1) / 2) * 2) + 1;

#if 0
    // Prints state vectors
    for ( int i = 0 ; i < size() ; i++ ) {
      cout << i << ":";
      for ( int j = 0 ; j < m_states.dim2() ; j++ ) {
        cout << " " << setw(26) << setprecision(13) << m_states[i][j];
      }
      cout << " " << setw(26) << setprecision(13) << m_epochs[i] << "\n";
    }
#endif
    setStartTime(m_epochs[0]);
    setEndTime(m_epochs[size(m_epochs)-1]);

    Pvl *label = cube.label();
    QString labStartTime = getKeyValue(*label, "StartTime");
    QString labEndTime;
    QString value = getKeyValue(*label, "StopTime");
    if (!value.isEmpty()) {
      labEndTime = value;
    }
    else {
      labEndTime = labStartTime;
    }

    iTime etLabStart(labStartTime);
    iTime etLabEnd(labEndTime);

    m_startOffset = etLabStart.Et() - m_epochs[0];
    m_endOffset = etLabEnd.Et() - m_epochs[size(m_epochs)-1];

    // Label start/end times are 3 decimal places, so round offsets to match.
    m_startOffset = qRound(m_startOffset * 1000.0) / 1000.0;
    m_endOffset = qRound(m_endOffset * 1000.0) / 1000.0;

    // account for padding
    if (m_startOffset >= 0.003) {
      m_startOffset = 0.0;
    }
    else {
      m_startOffset = fabs(m_startOffset);
    }
    if (m_endOffset <= 0.003) {
      m_endOffset = 0.0;
    }
    else {
      m_endOffset = fabs(m_endOffset);
    }

    m_instId = getKeyValue(*label, "InstrumentId");

  } catch ( IException &ie  ) {
    ostringstream mess;
    mess << "Failed to construct SPK content from ISIS file " << Source();
    throw IException(ie, IException::User, mess.str(), _FILEINFO_);
  }

  return;
}

/**
 * @brief Convert J2000 positions to frame relative to center body
 *
 * This method converts the data from SpicePostion to state vectors relative to
 * the center of motion of the object identified by body.  The return results
 * will be ready to write to (at least) SPK kernels of type 9 and 13.
 *
 * The "states" parameters will be a matrix of the form states[nrecs][6], where
 * "nrecs" is the number of states in the table, and times[nrecs] corresponds
 * to TDB time for each nrecs record.
 *
 * PreRequisites:
 *   The internal _body, _center and _refFrame are required to be defined prior
 *     to calling this routine.
 *   The FK kernel is likely to be required to be loaded in the NAIF kernel
 *     pool so that frame translations can occur.  The caller is burdened with
 *     ensuring the kernel is loaded.
 *
 * @author Kris Becker - 2/21/2011
 *
 * @param camera Camera object for the data
 * @param spice  Data from the SpicePosition::LineCache method
 * @param states Matrix containing "nrecs" states vectors
 * @param times  TBD epoch times for each record
 */
void SpkSegment::getStates(Camera &camera, const SMatrix &spice,
                           SMatrix &states, SVector &epochs, bool &hasVV)
                           const {
  int nrecs = size(spice);
  int nelems = spice.dim2();
  states = SMatrix(nrecs, 6, 0.0);  // Initialize to 0 should no deltas exist
  epochs = SVector(nrecs);
  hasVV = (nelems == 7);

   // cout << "Number Records: " << nrecs << "\n";

  // Extract contents
  for ( int i = 0 ; i < nrecs ; i++ ) {
    // cout  << "Rec: " << i;
    for ( int j = 0 ; j < (nelems-1) ; j++ ) {
      states[i][j] = spice[i][j];
      // cout << " " << setw(26) << setprecision(13) << m_states[i][j];
    }
    epochs[i] = spice[i][nelems-1];
    // cout << " " << setw(26) << setprecision(13) << _epochs[i] << "\n";
  }

  //  Compute state rotations relative to the reference frame
  QString j2000 = getNaifName(1);  // ISIS stores in J2000
  if (j2000 != m_refFrame) {
    // cout << "FromFrame = " << j2000 << ", TOFrame = " << _refFrame << "\n";
    NaifStatus::CheckErrors();
    for (int n = 0 ; n < nrecs ; n++) {
      SpiceDouble xform[6][6];
      sxform_c(j2000.toLatin1().data(), m_refFrame.toLatin1().data(), epochs[n], xform);
      mxvg_c(xform, states[n], 6, 6, states[n]);
    }
    NaifStatus::CheckErrors();
  }

  return;
}

/**
 * @brief Make a new state vector from the current state and time
 *
 * This method creates a new state from the given state0 using the position
 * object and current time, time0, at that position.  timeT is the new time of
 * the desired state.
 *
 * @author kbecker (5/2/2011)
 *
 * @param position  Position object for the state
 * @param time0     Current time for give state
 * @param state0    Current state vector
 * @param timeT     Desired time of the new state
 *
 * @return SpkSegment::SVector New state at timeT
 * @internal
 * @history 2011-06-03 Debbie A. Cook Put extrapolation code back
 *                                    in use since it gives the best results.
 */
SpkSegment::SVector SpkSegment::makeState(SpicePosition *position, const double &time0,
                                          const SVector &state0, const double &timeT) const {


  SVector stateT = state0.copy();
  // The code below seems to be working well for fixing the ends so I am putting it back in DAC
  position->SetEphemerisTime(time0);
  std::vector<double> tstate = position->Extrapolate(timeT);
  int nElems = std::min((int) tstate.size(), state0.dim1());
  for ( int i = 0 ; i < nElems ; i++ ) {
    stateT[i] = tstate[i];
  }

  return (stateT);
}

/**
 * @brief Construct a comment for the given segment
 *
 * @author kbecker (5/2/2011)
 *
 * @return QString Comment string for the segment
 */
QString SpkSegment::getComment() const {
  ostringstream comment;

  comment <<
"\n-----------------------------------------------------------------------\n" <<
"  File:        " << FileName(Source().toStdString()).name() << endl <<
"  Segment ID:  " << Id() << " (ProductId)" << endl <<
"  StartTime:   " << utcStartTime() << endl <<
"  EndTime:     " << utcEndTime() << endl <<
"  Instrument:  " << m_instId << endl <<
"  Target Body: " << "Body " << m_body << ", " << m_bodyFrame << endl <<
"  Center Body: " << "Body " << m_center << ", " << m_centerFrame << endl <<
"  RefFrame:    " << m_refFrame << endl <<
"  Records:     " << size() << endl;

  if (m_startOffset != 0) {
    comment <<
"  StartOffset: " << m_startOffset << endl;
  }

  if (m_endOffset != 0) {
    comment <<
"  EndOffset:   " << m_endOffset << endl;
  }

  string hasVV = (m_hasVV) ? "YES" : "NO";
  comment <<
"  HasVV:       " << hasVV << endl;

  comment <<
"  SpkType:     " << m_spkType << endl <<
"  PolyDegree:  " << m_degree << endl <<
"  CamVersion:  " << CameraVersion() << endl;
  QStringList klist = getKernels().getKernelList();
  if ( klist.size() > 0 ) {
    comment <<
"  Kernels:     \n";
    for ( int i = 0 ; i < klist.size() ; i++  ) {
      comment <<
"    " << klist[i] << endl;
    }
  }

  return ((comment.str().c_str()));
}

/**
 * @brief Determine if another SPK segment has common time/body coverage
 *
 * This method is used to determine if another SPK segment contains some of the
 * same coverage information as this one.  This is typically a conflict when
 * creating SPK kernels from a list of files.
 *
 * If the body and center codes of the two segments are not the same, this is
 * allowed even if the times are the same as it indicates different position
 * data.  If the codes are the same, then if any portion of the segements
 * contain common times of coverage, then this would indicate one of them would
 * be hidden in the resulting SPK kernel.
 *
 * Using this method, users can determine how to handle common times of
 * coverage.
 *
 * @author 2014-03-26 Kris Becker
 *
 * @param other  Other SpkSegment to check for common coverage
 *
 * @return bool True if data represents the same coverage, false otherwise.
 */
bool SpkSegment::overlaps(const SpkSegment &other) const {
  if ( BodyCode()   != other.BodyCode() )   { return (false); }
  if ( CenterCode() != other.CenterCode() ) { return (false); }
  if ( endTime()    <  other.startTime() )  { return (false); }
  if ( startTime()  >  other.endTime() )    { return (false); }
  return (true);
}


/**
 * @brief Initialize object parameters
 *
 * @author kbecker (5/2/2011)
 */
void SpkSegment::init(const int spkType) {
  validateType(spkType);
  m_spkType = spkType;
  m_body = m_center = 1;
  m_bodyFrame = m_centerFrame = m_refFrame = "";
  m_instId = "UNKNOWN";
  m_states = SMatrix(0,0);
  m_epochs = SVector(0);
  m_hasVV = false;
  m_degree = 1;
  m_startOffset = 0.0;
  return;
}

/**
 * @brief Load the SPK segements from the ISIS table object
 *
 * This method extracts position vectors, velocity vectors (if they exist) and
 * epochs (times) from an ISIS SpicePosition BLOB/table.  The table content
 * (number of fields) determine if the vectors exist.
 *
 * @author kbecker (5/2/2011)
 *
 * @param table   SpicePosition Table to extract SPK data
 *
 * @return SpkSegment::SMatrix Returns SPK data
 */
SpkSegment::SMatrix SpkSegment::load(Table &table) {

//  Allocate the internal cache and transfer.
  //  Makes some assumptions about the format of the SPICE table in that
  //  all fields are double.
  int nrecs = table.Records();
  TableRecord &rec = table[0];
  int nvals = rec.Fields();

  // Ensure the table has the expected format, error out if not valid.
//   if ( !((nvals == 7) || (nvals == 4)) ) {
//     ostringstream mess;
//     mess << "SPICE (SPK) Table " << table.Name()
//          << " must have 7 (with velocity vectors) or 4 fields but has "
//          << nvals;
//     throw iException::Message(iException::User, mess.str(), _FILEINFO_);
//   }

  // Extract contents
  SMatrix spice = SMatrix(nrecs,nvals);
  for ( int i = 0 ; i < nrecs ; i++ ) {
    TableRecord &rec = table[i];
    for ( int f = 0 ; f < rec.Fields() ; f++ ) {
      TableField &field = rec[f];
      spice[i][f] = (double) field;
    }
  }
  return (spice);
}

void SpkSegment::validateType(const int spktype) const {
  if ( !(( 9 == spktype ) || ( 13 == spktype )) ) {
    std::string mess = "Unsupported SPK kernel type (" +
                   QString::number(spktype) + ") - must be 9 or 13.";
    throw IException(IException::User, mess, _FILEINFO_);
  }
  return;
}

};  // namespace Isis
