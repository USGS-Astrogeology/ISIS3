/**
 * @file
 * $Revision$
 * $Date$
 * $Id$
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
#include <vector>

#include <QString>
#include <QStringList>

#include <boost/foreach.hpp>

#include <SpiceUsr.h>

#include "Camera.h"
#include "CameraFactory.h"
#include "Cube.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "NaifStatus.h"
#include "CkSpiceSegment.h"
#include "Table.h"

using namespace std;

namespace Isis {


  /** Default constructor */
CkSpiceSegment::CkSpiceSegment() {
  init();
}

/** Construct with an ISIS cube file */
CkSpiceSegment::CkSpiceSegment(const QString &fname) {
  init();
  Cube cube;
  cube.open(fname);
  import(cube);
}

/** Construct with a Cube and optional naming of table */
CkSpiceSegment::CkSpiceSegment(Cube &cube, const QString &tblname) {
  init();
  import(cube, tblname);
}

/** Set the name of the CK SPICE segment */
void CkSpiceSegment::setId(const QString &name) {
  _name = name;
  return;
}

/**
 * @brief Provide specified kernel types
 *
 * This method is provided to load (furnsh) NAIF kernels.  This is typically
 * required at the time the output CK file is created.
 *
 * @param ktypes Kernel types to load.  If empty, all kernels in the cube file
 *               will be loaded.  Example would be "FK,SCLK,LSK".
 *
 * @return int Returns number of kernels loaded
 */
int CkSpiceSegment::FurnshKernelType(const QString &ktypes) const {
  return (_kernels.Load(ktypes));
}

/**
 * @brief Unload NAIF kernels of specified type
 *
 * This method unloads NAIF SPICE kernels that are associate with the specified
 * types in the ktypes parameter.  It can be used in succession with the
 * FurnshKernelType() method above.
 *
 * @param ktypes  Kernel types to unload.  If empty, all kernels in the cube
 *               file will be unloaded.  Example would be "FK,SCLK,LSK".
 *
 * @return int  Returns number of kernels unloaded
 */
int CkSpiceSegment::UnloadKernelType(const QString &ktypes) const {
  return (_kernels.UnLoad(ktypes));
}

/**
 * @brief Returns the start time of intervals in segments
 *
 * This routine is needed for writing CK kernels.  It is assumed that the
 * complete segment is one interval, thus this implementation will return a
 * 1-element vector with the start time of the first quaternion.
 *
 *
 * @return CkSpiceSegment::SVector& Vector of start times for each interval
 */
CkSpiceSegment::SVector CkSpiceSegment::SCLKStartIntervals() const {
  return (SVector(1, _times[0]));
}

/**
 * @brief Returns the stop time of intervals in segments
 *
 * This routine is needed for writing CK kernels.  It is assumed that the
 * complete segment is one interval, thus this implementation will return a
 * 1-element vector with the stop time of the last quaternion.
 *
 *
 * @return CkSpiceSegment::SVector& Vector of stop times for each interval
 */
CkSpiceSegment::SVector CkSpiceSegment::SCLKStopIntervals() const {
  return (SVector(1, _times[size(_times)-1]));
}

/**
 * @brief Returns the SCLK tick rates of intervals in segments
 *
 * This routine is needed for writing CK kernels.  It is assumed that the
 * complete segment is one interval, thus this implementation will return a
 * 1-element vector with a single tick rate.
 *
 * The tick rate is determined by taking the SCLK of the first quaternion,
 * adding one tick to it and converting the two times to ET.  The difference of
 * the two ET times is the tick rate.  This determination is done at import
 * time.
 *
 *
 * @return CkSpiceSegment::SVector& Vector of tick rates for each interval
 */
CkSpiceSegment::SVector CkSpiceSegment::TickRate() const {
  return (SVector(1, _tickRate));
}

QString CkSpiceSegment::getKeyValue(PvlObject &label,
                                      const QString &keyword) {
  std::string value("");
  if ( label.hasKeyword(keyword.toStdString(), Pvl::Traverse) ) {
    value = label.findKeyword(keyword.toStdString(), Pvl::Traverse)[0];
  }
  return QString::fromStdString(value);
}

void CkSpiceSegment::import(Cube &cube, const QString &tblname) {

  _fname = cube.fileName();

  //  Extract ISIS CK blob and transform to CK 3 content
  NaifStatus::CheckErrors();
  try {

    // Order is somewhat important here.  The call to initialize Kernels
    // object checks the NAIF pool for existance.  It logs their NAIF
    // status as loaded which may cause trouble from here on...
    Pvl *label = cube.label();
    _kernels.Init(*label);

    Camera *camera;
    // Remove the ideal camera instrument group and rename the
    // OriginalInstrument group to Instrument for the scope of this
    // application.  Only label manipulation occurs and no change to the
    // pixels.
    if (label->findObject("IsisCube").hasGroup("OriginalInstrument")) {
      label->findObject("IsisCube").deleteGroup("Instrument");
      Isis::PvlGroup inst =
          label->findObject("IsisCube").findGroup("OriginalInstrument",
          Isis::Pvl::Traverse);
      inst.setName("Instrument");
      label->findObject("IsisCube").addGroup(inst);
      camera = CameraFactory::Create(cube);
    }
    else {
      camera = cube.camera();
    }

    //  Determine segment ID from product ID if it exists, otherwise basename
    if ( _name.isEmpty() ) {
      _name = getKeyValue(*label, "ProductId");
      if (_name.isEmpty() ) {
        _name = QString::fromStdString(FileName(_fname.toStdString()).baseName());
      }
    }

    QString value("");
    value = getKeyValue(*label, "InstrumentId");
    if (!value.isEmpty()) { _instId = value; }
    value = getKeyValue(*label, "TargetName");
    if (!value.isEmpty()) { _target = value; }
     _camVersion = _kernels.CameraVersion();

    QString labStartTime = getKeyValue(*label, "StartTime");
    QString labEndTime;
    value = getKeyValue(*label, "StopTime");
    if (!value.isEmpty()) {
      labEndTime = value;
    }
    else {
      labEndTime = labStartTime;
    }
    iTime etLabStart(labStartTime);
    iTime etLabEnd(labEndTime);

    //  Get the SPICE data
    Table ckCache = camera->instrumentRotation()->LineCache(tblname);
    SMatrix spice = load(ckCache);

    _quats = getQuaternions(spice);
    _avvs = getAngularVelocities(spice);
    _times = getTimes(spice);

    _startTime = _times[0];
    _endTime = _times[size(_times)-1];

    // Load necessary kernels (IAK for Cassini, mainly)
    _kernels.Load("CK,FK,SCLK,LSK,IAK");

    _kernels.getLoadedList();

    //  Here's where all the heavy lifting occurs.
    SMatSeq lmats, rmats;
    SVector sclks;
    getRotationMatrices(cube, *camera, ckCache, lmats, rmats, sclks);
    SMatrix ckQuats, ckAvvs;
    convert(_quats, _avvs, lmats, rmats, ckQuats, ckAvvs);

    // Compute small increment to pad each end
    const double Epsilon(3.0e-3);
    double topSclk = ETtoSCLK(camera->naifSclkCode(), _times[0] - Epsilon);
    double botSclk = ETtoSCLK(camera->naifSclkCode(),
                              _times[size(_times)-1] + Epsilon);

    // Pad the top and bottom of the CK data.  This copies the top and bottom
    //  contents of the data - that will work for us.
    ckQuats = expand(1, 1, ckQuats);
    if ( size(ckAvvs) > 0 ) ckAvvs = expand(1, 1, ckAvvs);
    sclks   = expand(1, 1, sclks);

    //  Finally, adjust the top and bottom times by pad time
    sclks[0]             = topSclk;
    sclks[size(sclks)-1] = botSclk;

    // Replace contents with converted quaternions, angular velocities (if
    // they exist) and sclk times.
    _quats = ckQuats;
    _avvs =  ckAvvs;
    _times = sclks;

    _startTime = SCLKtoET(camera->naifSclkCode(),_times[0]);
    _endTime = SCLKtoET(camera->naifSclkCode(),_times[size(_times)-1]);

    _utcStartTime = toUTC(startTime());
    _utcEndTime   = toUTC(endTime());

    // These offsets are absolute values. If there is a StartOffset, then this
    // must be subtracted from the label's original start time and if there is
    // an EndOffset, then the offset must be added to the label's original end
    // time.
    _startOffset =  etLabStart.Et() - startTime();
    _endOffset =  etLabEnd.Et() - endTime();


    // Label start/end times are 3 decimal places, so round offsets to match.
    _startOffset = qRound(_startOffset * 1000.0) / 1000.0;
    _endOffset = qRound(_endOffset * 1000.0) / 1000.0;

    // account for padding
    if (_startOffset >= 0.003) {
      _startOffset = 0.0;
    }
    else {
      _startOffset = fabs(_startOffset);
    }
    if (_endOffset <= 0.003) {
      _endOffset = 0.0;
    }
    else {
      _endOffset = fabs(_endOffset);
    }

    _kernels.UnLoad("CK,FK,SCLK,LSK,IAK");

  } catch ( IException &ie  ) {
    ostringstream mess;
    mess << "Failed to construct CK content from ISIS file " << _fname.toStdString();
    throw IException(ie, IException::User, mess.str(), _FILEINFO_);
  }

  return;
}


CkSpiceSegment::SMatrix CkSpiceSegment::getQuaternions(const SMatrix &spice) const {
  int nrecs = size(spice);
  SMatrix quats(nrecs, 4);

  for ( int i = 0 ; i < nrecs ; i++ ) {
    for ( int j = 0 ; j < 4 ; j++ ) {
      quats[i][j] = spice[i][j];
    }
  }
  return (quats);
}

CkSpiceSegment::SMatrix CkSpiceSegment::getAngularVelocities(const SMatrix &spice)
                                                      const {
  int nrecs = size(spice);
  int fields = spice.dim2();
  //  Must have 8 fields
  if ( fields != 8 ) {
    return (SMatrix(0,0));
  }

  SMatrix avvs(nrecs, 3);
  for ( int i = 0 ; i < nrecs ; i++ ) {
    for ( int j = 0 ; j < 3 ; j++ ) {
      avvs[i][j] = spice[i][j+4];
    }
  }
  return (avvs);
}


CkSpiceSegment::SVector CkSpiceSegment::getTimes(const SMatrix &spice) const {
  int nrecs = size(spice);
  SVector etdp(nrecs);
  int tcol = spice.dim2() - 1;

  for ( int i = 0 ; i < nrecs ; i++ ) {
    etdp[i] = spice[i][tcol];
  }
  return (etdp);
}

bool CkSpiceSegment::getTimeDependentFrameIds(Table &table, int &toId, int &fromId) const {
  // Load the constant and time-based frame traces and mission frame ids
  std::vector<int> tdfids;
  if ( table.Label().hasKeyword("TimeDependentFrames") ) {
    PvlKeyword labelTimeFrames = table.Label()["TimeDependentFrames"];
    for (int i=0; i<labelTimeFrames.size(); i++) {
      tdfids.push_back(Isis::toInt(labelTimeFrames[i]));
    }
  }
  else {
    toId = fromId = 0;
    return (false);
  }

  //  Return the needed IDs
  toId = tdfids[0];
  fromId = tdfids[tdfids.size()-1];
  return (true);
}


/**
 * @brief Determine left/right CK rotation chains that match mission CK format
 *
 * This method determines the left and right rotation chains that are needed to
 * convert the quaternions stored in ISIS CK blobs to reference and frame states
 * as represented in the mission CK kernels.  These chains are determined solely
 * from the time dependent frames from the blob labels.
 *
 * Note that if there is no rotation required, an empty or 1 element vector is
 * returned.  It is up to the caller to decide how to handle this situation.
 *
 * @author 2013-06-07 Kris Becker
 * @internal
 *   @history 2012-12-16 Kris Becker - Fixed problem when the
 *                         TimeDependentFrames keyword does not contain one or
 *                         both of the CK reference frames.  Fixes #1737.
 *
 * @param table
 * @param leftBase
 * @param rightBase
 * @param leftChain
 * @param rightChain
 *
 * @return bool  True if both left and right chains are valid, false if failure
 *               occurs.
 */
bool CkSpiceSegment::getFrameChains(Table &table, const int &leftBase,
                        const int &rightBase, QVector<int> &leftChain,
                        QVector<int> &rightChain) const {

  leftChain.clear();
  rightChain.clear();

  // Load the constant and time-based frame traces and mission frame ids
  QVector<int> tdfids;
  if ( table.Label().hasKeyword("TimeDependentFrames") ) {
    PvlKeyword labelTimeFrames = table.Label()["TimeDependentFrames"];
    for (int i = 0 ; i < labelTimeFrames.size() ; i++) {
      tdfids.push_back(Isis::toInt(labelTimeFrames[i]));
    }
  }
  else {
    return (false);
  }

  //  First, check to see if any of the ID are in the list.  Gotta have at
  // least 1.
  int nfound(0);
  BOOST_FOREACH (int fid, tdfids) {
    if ( rightBase == fid ) nfound++;
    if ( leftBase  == fid ) nfound++;
  }

  if ( (nfound == 0) || (nfound > 2) ) {
    ostringstream mess;
    mess << "Left/Right CK frame ids invalid in TimeDependentFrames label keyword."
         <<  " Must have at least 1 and no more than 2 ids but have "
         << Isis::toString(nfound);
    throw IException(IException::User, mess.str(), _FILEINFO_);
  }

  // Get the left CK ID chain
  int lastLeft(leftBase);
  BOOST_FOREACH (int leftId, tdfids) {
    //  Order is important in this code section!
    if ( rightBase == leftId ) break;  // Reached right reference frame
    leftChain.push_back(leftId);
    lastLeft = leftId;  // Record last valid one
    if (leftId == leftBase) break;
  }

  // Ensure the left chain is complete
  if ( leftBase != lastLeft) leftChain.push_back(leftBase);

    // Get the right CK ID chain
  int lastRight(rightBase);
  BOOST_REVERSE_FOREACH (int rightId, tdfids) {
//    if  (rightId == 1) continue;  //  REMOVE THIS - its only testing B1950!
    //  Order is important in this code section!
    if ( leftBase == rightId ) break;  // Reached left reference frame
    if ( lastLeft == rightId ) break;  // Reach last left id that we cannot ignore
    rightChain.push_front(rightId);
    lastRight = rightId;
    if (rightId == rightBase) break;
  }

   // Ensure the right chain is complete
  if ( rightBase != lastRight) rightChain.push_front(rightBase);

  return (true);
}

QString CkSpiceSegment::getFrameName(int frameid) const {
  SpiceChar frameBuf[40];
  NaifStatus::CheckErrors();
  frmnam_c ( (SpiceInt) frameid, sizeof(frameBuf), frameBuf);
  NaifStatus::CheckErrors();
  return (QString(frameBuf));
}

CkSpiceSegment::SMatrix CkSpiceSegment::getConstantRotation(Table &table) const {
  // Get constant rotation matrix from label
  SMatrix crot(3,3);
  try {
    PvlKeyword conrot = table.Label()["ConstantRotation"];
    SVector rot(9, crot[0]);
    for (int i=0; i < 9 ; i++) {  //  Loop count ensures valid matrices
      rot[i] = Isis::toDouble(conrot[i]);
    }
  } catch ( IException &ie ) {
    ostringstream mess;
    mess << "Failed to get rotation (via ConstantRotation keyword) from table "
         << table.Name() << " label. "
         << "  Most likely outdated ISIS file - may need to rerun spiceinit.";
    throw IException(ie, IException::User, mess.str(), _FILEINFO_);
  }
  return (crot);
}

CkSpiceSegment::SMatrix CkSpiceSegment::getIdentityRotation(const int &nelements) const {
  // Get constant rotation matrix from label
  SMatrix irot(nelements, nelements, 0.0);
  for (int i=0; i < nelements ; i++) {
    irot[i][i] = 1.0;
  }
  return (irot);
}


/**
 * @brief Retrieve state rotation matrix from kernels
 *
 * This method is invoked when a dynamic frame is encountered.  This condition
 * requires the reloading of kernels to resolve time dependent rotations
 * (MESSENGER is one such instrument).
 *
 * There are two ways the state rotation matrix can be determined.  One is with
 * the sxform_c NAIF routine that returns a 6x6 state matrix sufficient to
 * transform quaternions and angular velocity vectors if they exist in the
 * CK kernel. If angular velocites do not exist in the CK, then pxform_c is used
 * to get the 3x3 rotation matrix and then rav2fx_c is used to create the 6x6
 * state matrix with a constant angular velocity (set to 0.0).
 *
 * At any rate, a 6x6 state matrix is returned upon success.
 *
 * @param frame1 Frame name of quaternion as stored in ISIS blob
 * @param frame2 Frame name of desired state
 * @param etTime Empheris time to acquire state rotation for
 *
 * @return CkSpiceSegment::SMatrix A 6x6 state rotation matrix
 */
CkSpiceSegment::SMatrix CkSpiceSegment::computeStateRotation(const QString &frame1,
                                                         const QString &frame2,
                                                         double etTime) const {
  SMatrix state(6,6);
  NaifStatus::CheckErrors();

  try {
    // Get pointing w/AVs
    sxform_c(frame1.toLatin1().data(), frame2.toLatin1().data(), etTime,
             (SpiceDouble (*)[6]) state[0]);
    NaifStatus::CheckErrors();
  } catch ( IException & ) {
    try {
      SMatrix rot(3,3);
      pxform_c(frame1.toLatin1().data(), frame2.toLatin1().data(), etTime,
               (SpiceDouble (*)[3]) rot[0]);
      NaifStatus::CheckErrors();
      SVector av(3, 0.0);
      rav2xf_c((SpiceDouble (*)[3]) rot[0], &av[0],
               (SpiceDouble (*)[6]) state[0]);
    } catch ( IException &ie ) {
      ostringstream mess;
      mess << "Could not get dynamic state for time " << etTime;
      throw IException(ie, IException::User, mess.str(), _FILEINFO_);
    }
  }
  return (state);
}

CkSpiceSegment::SMatrix CkSpiceSegment::computeChainRotation(
                                       const QVector<int> &fChain,
                                       const int &terminatorID,
                                       const double &etTime) const {

  // Set up identity default
  SMatrix state = computeStateRotation("J2000", "J2000", etTime);

//  cout << "\nCompute Chain Rotations...\n";
  if ( fChain.size() > 0 ) {
    QVector<int> chain = fChain;

    //  Check for case where only 1 frame is given.  It should be the
    // terminating frame.  If it isn't, append it to the list as the last
    // to frame
   if ( chain.size() == 1 ) {
      if ( terminatorID != chain[0] ) chain.push_back(terminatorID);
    }

   SpiceInt toId = chain[0];
   for ( int i = 1  ; i < chain.size() ; i++ ) {
     SpiceInt fromId = chain[i];
     QString CfromId = getFrameName(fromId);
     QString CtoId = getFrameName(toId);
     SMatrix left = computeStateRotation(CtoId, CfromId, etTime);
     NaifStatus::CheckErrors();
     mxmg_c(left[0], state[0], 6, 6, 6, state[0]);
     NaifStatus::CheckErrors();
     toId = fromId;
   }
  }

  return (state);
}


void CkSpiceSegment::getRotationMatrices(Cube &cube, Camera &camera, Table &table,
                                       SMatSeq &lmats, SMatSeq &rmats,
                                       SVector &sclks) {


  //  Base CK frame and reference frames
  int leftId = camera.CkFrameId();
  int rightId = camera.CkReferenceId();

#if 0
  int LtoId, LfromId;
  if ( !getTimeDependentFrameIds(table, LtoId, LfromId) ) {
    std::string mess = "Cannot determine time dependent frames! - perhaps a spiceinit is in order.";
    throw IException(IException::User, mess, _FILEINFO_);
  }
#else
  QVector<int> leftFrames, rightFrames;
  if ( !getFrameChains(table, leftId, rightId, leftFrames, rightFrames) ) {
    std::string mess = "Cannot determine left/right frame chains! - perhaps a spiceinit is in order.";
    throw IException(IException::User, mess, _FILEINFO_);
  }
#endif

  // Set CK instrument code
  _instCode = leftId;

  _instFrame = getFrameName(leftId);
  _refFrame = getFrameName(rightId);

  SMatSeq lmat(size(_times)), rmat(size(_times)), avr(size(_times));
  for ( int i = 0 ; i < size(_times) ; i++ ) {
    SMatrix left = computeChainRotation(leftFrames, leftId, _times[i]);
    SMatrix right = computeChainRotation(rightFrames, rightId, _times[i]);
    lmat[i] = left;
    rmat[i] = right;
  }

  lmats = lmat;
  rmats = rmat;
  sclks = convertTimes(camera.naifSclkCode(), _times);

  return;
}

const CkSpiceSegment::SMatrix &CkSpiceSegment::getMatrix(const CkSpiceSegment::SMatSeq &seq,
                                                     const int &nth) const {
  if ( (nth < size(seq)) && (nth >= 0)  ) {
    return (seq[nth]);
  }
  return (seq[0]);
}

CkSpiceSegment::SVector CkSpiceSegment::convertTimes(
                                          int sclkCode,
                                          const CkSpiceSegment::SVector &etTimes
                                                ) {
  NaifStatus::CheckErrors();
  SVector sclks(size(etTimes));
  for ( int i  = 0 ; i < size(etTimes) ; i++ ) {
    sce2c_c(sclkCode, etTimes[i], &sclks[i]);
  }

  //  Determine the tick rate in case we need to create a type 2 CK
  SpiceDouble et0, et1;
  sct2e_c(sclkCode, sclks[0], &et0);
  sct2e_c(sclkCode, sclks[0]+1.0, &et1);
  _tickRate = fabs(et1 - et0);

  _utcStartTime = toUTC(startTime());
  _utcEndTime   = toUTC(endTime());

  NaifStatus::CheckErrors();
  return (sclks);
}


void CkSpiceSegment::convert(const CkSpiceSegment::SMatrix &quats,
                           const CkSpiceSegment::SMatrix &avvs,
                           const CkSpiceSegment::SMatSeq &lmats,
                           const CkSpiceSegment::SMatSeq &rmats,
                           CkSpiceSegment::SMatrix &ckQuats,
                           CkSpiceSegment::SMatrix &ckAvvs) const {

  ckQuats = SMatrix(quats.dim1(), quats.dim2());
  ckAvvs = SMatrix(avvs.dim1(), avvs.dim2());

  SpiceDouble m[3][3];
  SpiceDouble xform[6][6];
  SpiceDouble mout[6][6];
  SpiceDouble avZero[3] = { 0.0, 0.0, 0.0 };
  SpiceDouble avRav[3];
  ConstSpiceDouble *avIn(&avZero[0]);
  SpiceDouble *avOut(&avRav[0]); // For no AVs
  bool hasAv = (size(avvs) > 0);

  for ( int i = 0 ; i < size(quats) ; i++ ) {
    //  Handle option angular velocities
    if ( hasAv ) {
      avIn = avvs[i];
      avOut = ckAvvs[i];
    }

    NaifStatus::CheckErrors();
    // Convert quaternion to rotation and then to state matrix
    q2m_c(quats[i], m);
    rav2xf_c(m, avIn, xform);

    // Do the left and right multiplies
    mxmg_c(getMatrix(lmats, i)[0], xform, 6, 6, 6, mout);
    mxmg_c(mout, getMatrix(rmats, i)[0], 6, 6, 6, xform);

    // Transform to output format
    xf2rav_c(xform, m, avOut);  // Transfers AV to output ckAvvs
    m2q_c(m, ckQuats[i]);       // Transfers quaternion
    NaifStatus::CheckErrors();
  }
  return;
}


QString CkSpiceSegment::getComment() const {
  ostringstream comment;

  FileName fname(_fname.toStdString());

  comment <<
"\n-----------------------------------------------------------------------\n" <<
"  File:       " << fname.name() << endl <<
"  ProductId:  " << _name.toStdString() << endl <<
"  StartTime:  " << _utcStartTime.toDouble() << endl <<
"  EndTime:    " << _utcEndTime.toDouble() << endl <<
"  Instrument: " << _instId.toDouble() << endl <<
"  Target:     " << _target.toDouble() << endl <<
"  InstFrame:  " << _instFrame.toDouble() << endl <<
"  RefFrame:   " << _refFrame.toDouble() << endl <<
"  Records:    " << size() << endl;

  if (_startOffset != 0) {
    comment <<
"  StartOffset: " << _startOffset << endl;
  }

  if (_endOffset != 0) {
    comment <<
"  EndOffset: " << _endOffset << endl;
  }

  QString hasAV = (size(_avvs) > 0) ? "YES" : "NO";
  comment <<
"  HasAV:      " << hasAV.toStdString() << endl;

  comment <<
"  CamVersion: " << _camVersion << endl;
  QStringList klist = _kernels.getKernelList();
  if ( klist.size() > 0 ) {
    comment <<
"  Kernels:    \n";
    for ( int i = 0 ; i < klist.size() ; i++  ) {
      comment <<
"    " << klist[i].toStdString() << endl;
    }
  }

  return (QString(comment.str().c_str()));
}

void CkSpiceSegment::init() {
  _camVersion = 1;
  _name = _fname = "";
  _startTime = _endTime  = 0.0;
  _utcStartTime = _utcEndTime = "";
  _instId = _target = "UNKNOWN";
  _instCode = 0;
  _instFrame = _refFrame = "";
  _startOffset = 0.0;
  _endOffset = 0.0;
  _quats = _avvs = SMatrix(0,0);
  _times = SVector(0);
  _tickRate = 0.0;
  return;
}

CkSpiceSegment::SMatrix CkSpiceSegment::load(Table &table) {

//  Allocate the internal cache and transfer.
  //  Makes some assumptions about the format of the SPICE table in that
  //  all fields are double.
  int nrecs = table.Records();
  TableRecord &rec = table[0];
  int nvals = rec.Fields();

  // Ensure the table has the expected format, error out if not valid.
  if ( !((nvals == 8) || (nvals == 5)) ) {
    ostringstream mess;
    mess << "SPICE (CK) Table " << table.Name()
         << " must have 8 (with angular velocities) or 5 fields but has "
         << nvals;
    throw IException(IException::User, mess.str(), _FILEINFO_);
  }

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
 *  @return CkSpiceSegment::SMatrix Expanded matrix
 */
CkSpiceSegment::SMatrix CkSpiceSegment::expand(int ntop, int nbot,
                                           const CkSpiceSegment::SMatrix &matrix)
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
 *  @return CkSpiceSegment::SVector Expanded vector
 */
CkSpiceSegment::SVector CkSpiceSegment::expand(int ntop, int nbot,
                                           const CkSpiceSegment::SVector &vec)
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

double CkSpiceSegment::SCLKtoET(SpiceInt scCode, double sclk) const {
  SpiceDouble et;

  NaifStatus::CheckErrors();
  sct2e_c(scCode, sclk, &et);
  NaifStatus::CheckErrors();

  return (et);
}

double CkSpiceSegment::ETtoSCLK(SpiceInt scCode, double et) const {
  SpiceDouble sclk;

  NaifStatus::CheckErrors();
  sce2c_c(scCode, et, &sclk);
  NaifStatus::CheckErrors();

  return (sclk);
}

QString CkSpiceSegment::toUTC(const double &et) const {
  const int UTCLEN = 80;
  char utcout[UTCLEN];

  NaifStatus::CheckErrors();
  et2utc_c(et, "ISOC", 3, UTCLEN, utcout);
  NaifStatus::CheckErrors();

  return (QString(utcout));
}

};  // namespace Isis
