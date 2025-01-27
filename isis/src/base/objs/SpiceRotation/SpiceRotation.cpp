/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "SpiceRotation.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <iomanip>
#include <string>
#include <vector>

#include <QDebug>
#include <QString>
#include <SpiceUsr.h>
#include <SpiceZfc.h>
#include <SpiceZmc.h>

#include "BasisFunction.h"
#include "IException.h"
#include "IString.h"
#include "LeastSquares.h"
#include "LineEquation.h"
#include "NaifStatus.h"
#include "PolynomialUnivariate.h"
#include "Quaternion.h"
#include "Table.h"
#include "TableField.h"

using json = nlohmann::json;

// Declarations for bindings for Naif Spicelib routines that do not have
// a wrapper
extern int refchg_(integer *frame1, integer *frame2, doublereal *et,
                   doublereal *rotate);
extern int frmchg_(integer *frame1, integer *frame2, doublereal *et,
                   doublereal *rotate);
extern int invstm_(doublereal *mat, doublereal *invmat);
// Temporary declarations for bindings for Naif supportlib routines

// These three declarations should be removed once supportlib is in Isis3
extern int ck3sdn(double sdntol, bool avflag, int *nrec,
                  double *sclkdp, double *quats, double *avvs,
                  int nints, double *starts, double *dparr,
                  int *intarr);

namespace Isis {
  /**
   * Construct an empty SpiceRotation class using a valid Naif frame code to
   * set up for getting rotation from Spice kernels.  See required reading
   * ftp://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/req/naif_ids.html
   *
   * @param frameCode Valid naif frame code.
   */
  SpiceRotation::SpiceRotation(int frameCode) {
    p_constantFrames.push_back(frameCode);
    p_timeBias = 0.0;
    p_source = Spice;
    p_CJ.resize(9);
    p_matrixSet = false;
    p_et = -DBL_MAX;
    p_degree = 2;
    p_degreeApplied = false;
    p_noOverride = true;
    p_axis1 = 3;
    p_axis2 = 1;
    p_axis3 = 3;
    p_minimizeCache = No;
    p_hasAngularVelocity = false;
    p_av.resize(3);
    p_fullCacheStartTime = 0;
    p_fullCacheEndTime = 0;
    p_fullCacheSize = 0;
    m_frameType = UNKNOWN;
    m_tOrientationAvailable = false;
    m_orientation = NULL;
  }


  /**
   * Construct an empty SpiceRotation object using valid Naif frame code and.
   * body code to set up for computing nadir rotation.  See required reading
   * ftp://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/req/naif_ids.html
   *
   * @param frameCode Valid naif frame code.
   * @param targetCode Valid naif body code.
   *
   * @throws IException::Io "Cannot find [key] in text kernels"
   */
  SpiceRotation::SpiceRotation(int frameCode, int targetCode) {
    NaifStatus::CheckErrors();

    p_constantFrames.push_back(frameCode);
    p_targetCode = targetCode;
    p_timeBias = 0.0;
    p_source = Nadir;
    p_CJ.resize(9);
    p_matrixSet = false;
    p_et = -DBL_MAX;
    p_axisP = 3;
    p_degree = 2;
    p_degreeApplied = false;
    p_noOverride = true;
    p_axis1 = 3;
    p_axis2 = 1;
    p_axis3 = 3;
    p_minimizeCache = No;
    p_hasAngularVelocity = false;
    p_av.resize(3);
    p_fullCacheStartTime = 0;
    p_fullCacheEndTime = 0;

    p_fullCacheSize = 0;
    m_frameType = DYN;
    m_tOrientationAvailable = false;
    m_orientation = NULL;

    // Determine the axis for the velocity vector
    QString key = "INS" + toString(frameCode) + "_TRANSX";
    SpiceDouble transX[2];
    SpiceInt number;
    SpiceBoolean found;
    //Read starting at element 1 (skipping element 0)
    gdpool_c(key.toLatin1().data(), 1, 2, &number, transX, &found);

    if (!found) {
      QString msg = "Cannot find [" + key + "] in text kernels";
      throw IException(IException::Io, msg, _FILEINFO_);
    }

    p_axisV = 2;
    if (transX[0] < transX[1]) p_axisV = 1;

    NaifStatus::CheckErrors();
  }


  /**
   * Construct a SpiceRotation object by copying from an existing one.
   *
   * @param rotToCopy const reference to other SpiceRotation to copy
   */
  SpiceRotation::SpiceRotation(const SpiceRotation &rotToCopy) {
    p_cacheTime = rotToCopy.p_cacheTime;
    p_av = rotToCopy.p_av;
    p_degree = rotToCopy.p_degree;
    p_axis1 = rotToCopy.p_axis1;
    p_axis2 = rotToCopy.p_axis2;
    p_axis3 = rotToCopy.p_axis3;

    p_constantFrames = rotToCopy.p_constantFrames;
    p_timeFrames = rotToCopy.p_timeFrames;
    p_timeBias = rotToCopy.p_timeBias;

    p_et = rotToCopy.p_et;
    p_quaternion = rotToCopy.p_quaternion;
    p_matrixSet = rotToCopy.p_matrixSet;
    p_source = rotToCopy.p_source;
    p_axisP = rotToCopy.p_axisP;
    p_axisV = rotToCopy.p_axisV;
    p_targetCode = rotToCopy.p_targetCode;
    p_baseTime = rotToCopy.p_baseTime;
    p_timeScale = rotToCopy.p_timeScale;
    p_degreeApplied = rotToCopy.p_degreeApplied;

//    for (std::vector<double>::size_type i = 0; i < rotToCopy.p_coefficients[0].size(); i++)
    for (int i = 0; i < 3; i++)
      p_coefficients[i] = rotToCopy.p_coefficients[i];

    p_noOverride = rotToCopy.p_noOverride;
    p_overrideBaseTime = rotToCopy.p_overrideBaseTime;
    p_overrideTimeScale = rotToCopy.p_overrideTimeScale;
    p_minimizeCache = rotToCopy.p_minimizeCache;
    p_fullCacheStartTime = rotToCopy.p_fullCacheStartTime;
    p_fullCacheEndTime = rotToCopy.p_fullCacheEndTime;
    p_fullCacheSize = rotToCopy.p_fullCacheSize;
    p_TC = rotToCopy.p_TC;

    p_CJ = rotToCopy.p_CJ;
    p_degree = rotToCopy.p_degree;
    p_hasAngularVelocity = rotToCopy.p_hasAngularVelocity;
    m_frameType = rotToCopy.m_frameType;

    if (rotToCopy.m_orientation) {
      m_orientation = new ale::Orientations;
      *m_orientation = *rotToCopy.m_orientation;
    }
    else {
      m_orientation = NULL;
    }
  }


  /**
   * Destructor for SpiceRotation object.
   */
  SpiceRotation::~SpiceRotation() {
    if (m_orientation) {
      delete m_orientation;
      m_orientation = NULL;
    }
  }


  /**
   * Change the frame to the given frame code.  This method has no effect if
   * spice is cached.
   *
   * @param frameCode The integer-valued frame code
   */
  void SpiceRotation::SetFrame(int frameCode) {
    p_constantFrames[0] = frameCode;
  }


  /**
   * Accessor method that returns the frame code. This is the first value of the
   * constant frames member variable.
   *
   * @return @b int An integer value indicating the frame code.
   */
  int SpiceRotation::Frame() {
    return p_constantFrames[0];
  }


  /**
   * Apply a time bias when invoking SetEphemerisTime method.
   *
   * The bias is used only when reading from NAIF kernels.  It is added to the
   * ephermeris time passed into SetEphemerisTime and then the body
   * position is read from the NAIF kernels and returned.  When the cache
   * is loaded from a table the bias is ignored as it is assumed to have
   * already been applied.  If this method is never called the default bias is
   * 0.0 seconds.
   *
   * @param timeBias time bias in seconds
   */
  void SpiceRotation::SetTimeBias(double timeBias) {
    p_timeBias = timeBias;
  }


  /**
   * Return the J2000 to reference frame quaternion at given time.
   *
   * This method returns the J2000 to reference frame rotational matrix at a
   * given et in seconds.  The quaternion is obtained from either valid NAIF ck
   * and/or fk, or alternatively from an internal cache loaded from an ISIS
   * Table object.  In the first case, the kernels must contain the rotation
   * for the frame specified in the constructor at the given time (as well as
   * all the intermediate frames going from the reference frame to J2000) and
   * they must be loaded using the SpiceKernel class.
   *
   * @param et   ephemeris time in seconds
   */
  void SpiceRotation::SetEphemerisTime(double et) {

    // Save the time
    if (p_et == et) return;
    p_et = et;

    // Read from the cache
    if (p_source == Memcache) {
      setEphemerisTimeMemcache();
    }

    // Apply coefficients defining a function for each of the three camera angles and angular
    //  velocity if available
    else if (p_source == PolyFunction) {
      setEphemerisTimePolyFunction();
    }
    // Apply coefficients defining a function for each of the three camera angles and angular
    //  velocity if available
    else if (p_source == PolyFunctionOverSpice) {
      setEphemerisTimePolyFunctionOverSpice();
    }
    // Read from the kernel
    else if (p_source == Spice) {
      setEphemerisTimeSpice();
      // Retrieve the J2000 (code=1) to reference rotation matrix
    }
    // Apply coefficients from PCK version of IAU solution for target body orientation and angular
    //  velocity???
    else if (p_source == PckPolyFunction) {
      setEphemerisTimePckPolyFunction();
    }
    // Compute from Nadir
    else {
      setEphemerisTimeNadir();
    }

    // Set the quaternion for this rotation
//    p_quaternion.Set ( p_CJ );
  }


  /**
   * Accessor method to get current ephemeris time.
   *
   * @return @b double The current ephemeris time.
   */
  double SpiceRotation::EphemerisTime() const {
    return p_et;
  }


  /**
   * Checks if the cache is empty.
   *
   * @return @b bool Indicates whether this rotation is cached.
   */
  bool SpiceRotation::IsCached() const {
    return (m_orientation != NULL);
  }


  /**
   * Set the downsize status to minimize cache.
   *
   * @param status The DownsizeStatus enumeration value.
   */
  void SpiceRotation::MinimizeCache(DownsizeStatus status) {
    p_minimizeCache = status;
  }


  /**
   * Cache J2000 rotation quaternion over a time range.
   *
   * This method will load an internal cache with frames over a time
   * range.  This prevents the NAIF kernels from being read over-and-over
   * again and slowing an application down due to I/O performance.  Once the
   * cache has been loaded then the kernels can be unloaded from the NAIF
   * system.
   *
   * @param startTime   Starting ephemeris time in seconds for the cache
   * @param endTime     Ending ephemeris time in seconds for the cache
   * @param size        Number of frames to keep in the cache
   *
   * @throws IException::Programmer "Argument cacheSize must not be less than or equal to zero"
   * @throws IException::Programmer "Argument startTime must be less than or equal to endTime"
   * @throws IException::Programmer "Cache size must be more than 1 if startTime and endTime differ"
   * @throws IException::Programmer "A SpiceRotation cache has already men
   */
  void SpiceRotation::LoadCache(double startTime, double endTime, int size) {

    // Check for valid arguments
    if (size <= 0) {
      QString msg = "Argument cacheSize must not be less or equal to zero";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (startTime > endTime) {
      QString msg = "Argument startTime must be less than or equal to endTime";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if ((startTime != endTime) && (size == 1)) {
      QString msg = "Cache size must be more than 1 if startTime and endTime differ";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Make sure cache isn't already loaded
    if (p_source == Memcache) {
      QString msg = "A SpiceRotation cache has already been created";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Save full cache parameters
    p_fullCacheStartTime = startTime;
    p_fullCacheEndTime = endTime;
    p_fullCacheSize = size;

    if (m_orientation != NULL) {
      delete m_orientation;
      m_orientation = NULL;
    }

    // Make sure the constant frame is loaded.  This method also does the frame trace.
    if (p_timeFrames.size() == 0) InitConstantRotation(startTime);

    // Set the frame type.  If the frame class is PCK, load the constants.
    if (p_source == Spice) {
      setFrameType();
    }

    LoadTimeCache();
    int cacheSize = p_cacheTime.size();

    // Loop and load the cache
    std::vector<ale::Rotation> rotationCache;
    std::vector< std::vector<double> > cache;
    std::vector<ale::Vec3d> avCache;
    for (int i = 0; i < cacheSize; i++) {
      double et = p_cacheTime[i];
      SetEphemerisTime(et);
      rotationCache.push_back(ale::Rotation(p_CJ));
      cache.push_back(p_CJ);

      if (p_hasAngularVelocity) {
        avCache.push_back(ale::Vec3d(p_av));
      }
    }

    if (p_TC.size() > 1) {
      m_orientation = new ale::Orientations(rotationCache, p_cacheTime, avCache,
                                            ale::Rotation(p_TC), p_constantFrames, p_timeFrames);
    }
    else {
      m_orientation = new ale::Orientations(rotationCache, p_cacheTime, avCache,
                                            ale::Rotation(1,0,0,0), p_constantFrames, p_timeFrames);
    }

    p_source = Memcache;

    // Downsize already loaded caches (both time and quats)
    if (p_minimizeCache == Yes  &&  cacheSize > 5) {
      LoadTimeCache();
    }
  }


  /**
   * Cache J2000 to frame rotation for a time.
   *
   * This method will load an internal cache with a rotation for a single
   * time (e.g. useful for framing cameras). This prevents
   * the NAIF kernels from being read over-and-over again and slowing a
   * application down due to I/O performance.  Once the
   * cache has been loaded then the kernels can be unloaded from the NAIF
   * system.  This calls the LoadCache(stime,etime,size) method using the
   * time as both the starting and ending time with a size of 1.
   *
   * @param time   single ephemeris time in seconds to cache
   */
  void SpiceRotation::LoadCache(double time) {
    LoadCache(time, time, 1);
  }


  /**
   * Load the cached data from an ALE ISD.
   *
   * The SpiceRotation object must be set to a SPICE source before loading the
   * cache.
   *
   * @param isdRot The ALE ISD as a JSON object.
   *
   */
  void SpiceRotation::LoadCache(json &isdRot){
    if (p_source != Spice) {
        throw IException(IException::Programmer, "SpiceRotation::LoadCache(json) only supports Spice source", _FILEINFO_);
    }

    p_timeFrames.clear();
    p_TC.clear();
    p_cacheTime.clear();
    p_hasAngularVelocity = false;
    m_frameType = CK;

    if (m_orientation) {
      delete m_orientation;
      m_orientation = NULL;
    }

    // Load the full cache time information from the label if available
    p_fullCacheStartTime = isdRot["ck_table_start_time"].get<double>();
    p_fullCacheEndTime = isdRot["ck_table_end_time"].get<double>();
    p_fullCacheSize = isdRot["ck_table_original_size"].get<double>();
    p_cacheTime = isdRot["ephemeris_times"].get<std::vector<double>>();
    p_timeFrames = isdRot["time_dependent_frames"].get<std::vector<int>>();

    std::vector<ale::Rotation> rotationCache;
    for (auto it = isdRot["quaternions"].begin(); it != isdRot["quaternions"].end(); it++) {
      std::vector<double> quat = {it->at(0).get<double>(), it->at(1).get<double>(), it->at(2).get<double>(), it->at(3).get<double>()};
      Quaternion q(quat);
      std::vector<double> CJ = q.ToMatrix();
      rotationCache.push_back(ale::Rotation(CJ));
    }

    std::vector<ale::Vec3d> avCache;
    if (isdRot["angular_velocities"].size() != 0) {
      for (auto it = isdRot["angular_velocities"].begin(); it != isdRot["angular_velocities"].end(); it++) {
        std::vector<double> av = {it->at(0).get<double>(), it->at(1).get<double>(), it->at(2).get<double>()};
        avCache.push_back(ale::Vec3d(av));
      }
      p_hasAngularVelocity = true;
    }

    if (isdRot.contains("constant_frames")) {
      p_constantFrames = isdRot["constant_frames"].get<std::vector<int>>();
      p_TC = isdRot["constant_rotation"].get<std::vector<double>>();
      m_orientation = new ale::Orientations(rotationCache, p_cacheTime, avCache,
                                          ale::Rotation(p_TC), p_constantFrames, p_timeFrames);
    }
    else {
      p_TC.resize(9);
      ident_c((SpiceDouble( *)[3]) &p_TC[0]);
      m_orientation = new ale::Orientations(rotationCache, p_cacheTime, avCache,
                                          ale::Rotation(1,0,0,0), p_constantFrames, p_timeFrames);
    }


    p_source = Memcache;
    SetEphemerisTime(p_cacheTime[0]);
  }


  /**
   * Cache J2000 rotations using a table file.
   *
   * This method will load either an internal cache with rotations (quaternions)
   * or coefficients (for 3 polynomials defining the camera angles) from an
   * ISIS table file.  In the first case, the table must have 5 columns and
   * at least one row. The 5 columns contain the following information, J2000
   * to reference quaternion (q0, q1, q2, q3)  and the ephemeris time of that
   * position. If there are multiple rows, it is assumed the quaternions between
   * the rows can be interpolated.  In the second case, the table must have
   * three columns and at least two rows.  The three columns contain the
   * coefficients for each of the three camera angles.  Row one of the
   * table contains coefficient 0 (constant term) for angles 1, 2, and 3.
   * If the degree of the fit equation is greater than 1, row 2 contains
   * coefficient 1 (linear) for each of the three angles.  Row n contains
   * coefficient n-1 and the last row contains the time parameters, base time,
   * and time scale, and the degree of the polynomial.
   *
   * @param table   An ISIS table blob containing valid J2000 to reference
   *                quaternion/time values
   *
   * @throws IException::Programmer "Expecting either three, five, or eight fields in the
   *                                 SpiceRotation table"
   */
  void SpiceRotation::LoadCache(Table &table) {
    // Clear any existing cached data to make it reentrant (KJB 2011-07-20).
    p_timeFrames.clear();
    p_TC.clear();
    p_cacheTime.clear();

    p_hasAngularVelocity = false;

    if (m_orientation) {
      delete m_orientation;
      m_orientation = NULL;
    }

    // Load the constant and time-based frame traces and the constant rotation
    if (table.Label().hasKeyword("TimeDependentFrames")) {
      PvlKeyword labelTimeFrames = table.Label()["TimeDependentFrames"];
      for (int i = 0; i < labelTimeFrames.size(); i++) {
        p_timeFrames.push_back(toInt(labelTimeFrames[i]));
      }
    }
    else {
      p_timeFrames.push_back(p_constantFrames[0]);
      p_timeFrames.push_back(J2000Code);
    }

    if (table.Label().hasKeyword("ConstantRotation")) {
      PvlKeyword labelConstantFrames = table.Label()["ConstantFrames"];
      p_constantFrames.clear();

      for (int i = 0; i < labelConstantFrames.size(); i++) {
        p_constantFrames.push_back(toInt(labelConstantFrames[i]));
      }
      PvlKeyword labelConstantRotation = table.Label()["ConstantRotation"];

      for (int i = 0; i < labelConstantRotation.size(); i++) {
        p_TC.push_back(toDouble(labelConstantRotation[i]));
      }
    }
    else {
      p_TC.resize(9);
      ident_c((SpiceDouble( *)[3]) &p_TC[0]);
    }

    // Load the full cache time information from the label if available
    if (table.Label().hasKeyword("CkTableStartTime")) {
      p_fullCacheStartTime = toDouble(table.Label().findKeyword("CkTableStartTime")[0]);
    }
    if (table.Label().hasKeyword("CkTableEndTime")) {
      p_fullCacheEndTime = toDouble(table.Label().findKeyword("CkTableEndTime")[0]);
    }
    if (table.Label().hasKeyword("CkTableOriginalSize")) {
      p_fullCacheSize = toInt(table.Label().findKeyword("CkTableOriginalSize")[0]);
    }

    // Load FrameTypeCode from labels if available and the planetary constants keywords
    if (table.Label().hasKeyword("FrameTypeCode")) {
      m_frameType = (FrameType) toInt(table.Label().findKeyword("FrameTypeCode")[0]);
    }
    else {
      m_frameType = UNKNOWN;
    }

    if (m_frameType  == PCK) {
      loadPCFromTable(table.Label());
    }

    int recFields = table[0].Fields();

    // Loop through and move the table to the cache.  Retrieve the first record to
    // establish the type of cache and then use the appropriate loop.

    // list table of quaternion and time

    std::vector<ale::Rotation> rotationCache;
    std::vector<ale::Vec3d> avCache;
    if (recFields == 5) {
      for (int r = 0; r < table.Records(); r++) {
        TableRecord &rec = table[r];

        if (rec.Fields() != recFields) {
          // throw and error
        }

        std::vector<double> j2000Quat;
        j2000Quat.push_back((double)rec[0]);
        j2000Quat.push_back((double)rec[1]);
        j2000Quat.push_back((double)rec[2]);
        j2000Quat.push_back((double)rec[3]);

        Quaternion q(j2000Quat);
        std::vector<double> CJ = q.ToMatrix();
        rotationCache.push_back(ale::Rotation(CJ));

        p_cacheTime.push_back((double)rec[4]);
      }
      if (p_TC.size() > 1) {
        m_orientation = new ale::Orientations(rotationCache, p_cacheTime, avCache,
                                              ale::Rotation(p_TC), p_constantFrames, p_timeFrames);
      }
      else {
        m_orientation = new ale::Orientations(rotationCache, p_cacheTime, avCache,
                                              ale::Rotation(1,0,0,0), p_constantFrames, p_timeFrames);
      }
      p_source = Memcache;
    }

    // list table of quaternion, angular velocity vector, and time
    else if (recFields == 8) {
      for (int r = 0; r < table.Records(); r++) {
        TableRecord &rec = table[r];

        if (rec.Fields() != recFields) {
          // throw and error
        }

        std::vector<double> j2000Quat;
        j2000Quat.push_back((double)rec[0]);
        j2000Quat.push_back((double)rec[1]);
        j2000Quat.push_back((double)rec[2]);
        j2000Quat.push_back((double)rec[3]);


        Quaternion q(j2000Quat);
        std::vector<double> CJ = q.ToMatrix();
        rotationCache.push_back(ale::Rotation(CJ));

        std::vector<double> av;
        av.push_back((double)rec[4]);
        av.push_back((double)rec[5]);
        av.push_back((double)rec[6]);
        avCache.push_back(ale::Vec3d(av));
        p_cacheTime.push_back((double)rec[7]);
        p_hasAngularVelocity = true;
      }

      if (p_TC.size() > 1) {
        m_orientation = new ale::Orientations(rotationCache, p_cacheTime, avCache,
                                              ale::Rotation(p_TC), p_constantFrames, p_timeFrames);
      }
      else {
        m_orientation = new ale::Orientations(rotationCache, p_cacheTime, avCache,
                                              ale::Rotation(1,0,0,0), p_constantFrames, p_timeFrames);
      }
      p_source = Memcache;
    }

    // coefficient table for angle1, angle2, and angle3
    else if (recFields == 3) {
      std::vector<double> coeffAng1, coeffAng2, coeffAng3;

      for (int r = 0; r < table.Records() - 1; r++) {
        TableRecord &rec = table[r];

        if (rec.Fields() != recFields) {
          // throw an error
        }
        coeffAng1.push_back((double)rec[0]);
        coeffAng2.push_back((double)rec[1]);
        coeffAng3.push_back((double)rec[2]);
      }

      // Take care of time parameters
      TableRecord &rec = table[table.Records()-1];
      double baseTime = (double)rec[0];
      double timeScale = (double)rec[1];
      double degree = (double)rec[2];
      SetPolynomialDegree((int) degree);
      SetOverrideBaseTime(baseTime, timeScale);
      SetPolynomial(coeffAng1, coeffAng2, coeffAng3);
      p_source = PolyFunction;
      if (degree > 0)  p_hasAngularVelocity = true;
      if (degree == 0  && m_orientation->getAngularVelocities().size() > 0) {
        p_hasAngularVelocity = true;
      }
    }
    else  {
      QString msg = "Expecting either three, five, or eight fields in the SpiceRotation table";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Cache J2000 rotation over existing cached time range using polynomials
   *
   * This method will reload an internal cache with matrices
   * formed from rotation angles fit to functions over a time
   * range.
   *
   * @throws IException::Programmer "The SpiceRotation has not yet been fit to a function"
   */
  void SpiceRotation::ReloadCache() {
    // Save current et
    double et = p_et;
    p_et = -DBL_MAX;

    std::vector<ale::Rotation> rotationCache;
    std::vector<ale::Vec3d> avCache;
    if (p_source == PolyFunction) {
    // Clear existing matrices from cache
      p_cacheTime.clear();

      // Load the time cache first
      p_minimizeCache = No;
      LoadTimeCache();

      if (p_fullCacheSize > 1) {
      // Load the matrix and av caches
        for (std::vector<double>::size_type pos = 0; pos < p_cacheTime.size(); pos++) {
          SetEphemerisTime(p_cacheTime.at(pos));
          rotationCache.push_back(ale::Rotation(p_CJ));
          avCache.push_back(ale::Vec3d(p_av));
        }
      }
      else {
      // Load the matrix for the single updated time instance
        SetEphemerisTime(p_cacheTime[0]);
        rotationCache.push_back(ale::Rotation(p_CJ));
        avCache.push_back(ale::Vec3d(p_av));
      }
    }
    else if (p_source == PolyFunctionOverSpice) {
      SpiceRotation tempRot(*this);
      std::vector<double>::size_type maxSize = p_fullCacheSize;

      // Clear the existing caches
      p_cacheTime.clear();

      // Reload the time cache first
      p_minimizeCache = No;
      LoadTimeCache();

      for (std::vector<double>::size_type pos = 0; pos < maxSize; pos++) {
        tempRot.SetEphemerisTime(p_cacheTime.at(pos));
        std::vector<double> CJ = tempRot.TimeBasedMatrix();
        rotationCache.push_back(ale::Rotation(CJ));
        if (p_hasAngularVelocity){
          avCache.push_back(ale::Vec3d(tempRot.AngularVelocity()));
        }
      }
    }
    else { //(p_source < PolyFunction)
      QString msg = "The SpiceRotation has not yet been fit to a function";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (m_orientation) {
      delete m_orientation;
      m_orientation = NULL;
    }

    if (p_TC.size() > 1) {
      m_orientation = new ale::Orientations(rotationCache, p_cacheTime, avCache,
                                            ale::Rotation(p_TC), p_constantFrames, p_timeFrames);
    }
    else {
        m_orientation = new ale::Orientations(rotationCache, p_cacheTime, avCache,
                                              ale::Rotation(1,0,0,0), p_constantFrames, p_timeFrames);
    }

    // Set source to cache and reset current et
    // Make sure source is Memcache now
    p_source = Memcache;
    p_et = -DBL_MAX;
    SetEphemerisTime(et);
  }


  /**
   * Return a table with J2000 to reference rotations.
   *
   * Return a table containing the cached pointing with the given
   * name. The table will have eight columns, quaternion, angular
   * velocity, and time of J2000 to reference frame rotation.
   *
   * @param tableName    Name of the table to create and return
   *
   * @throws IException::Programmer "Only cached rotations can be returned as a line cache of
   *                                 quaternions and time"
   *
   * @return @b Table Table with given name that contains the cached pointing
   */
  Table SpiceRotation::LineCache(const QString &tableName) {

    // Apply the function and fill the caches
    if (p_source >= PolyFunction)  ReloadCache();

    if (p_source != Memcache) {
      QString msg = "Only cached rotations can be returned as a line cache of quaternions and time";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    // Load the table and return it to caller
    return Cache(tableName);
  }


  /**
   * Return a table with J2000 to reference rotations.
   *
   * Return a table containing the cached pointing with the given
   * name. The table will have either five columns (for a list cache)
   * of J2000 to reference quaternions and times, eight columns (if
   * angular velocity is available), or three columns
   * (for a coefficient cache), of J2000 to reference frame rotation
   * angles defined by coefficients of a polynomial function (see
   * SetPolynommial).  In the coefficient cache the last row of
   * the table is the base time, time scale, and polynomial degree.
   * Note:  In the case of the coefficient cache, the angular
   * velocity is not written to the table since it can be calculated
   * from the polynomials.
   *
   * @param tableName    Name of the table to create and return
   *
   * @throws IException::Programmer "To create table source of data must be either Memcache or
   *                                 PolyFunction"
   *
   * @return @b Table Table with given name that contains the cached pointing
   */
  Table SpiceRotation::Cache(const QString &tableName) {
   // First handle conversion of PolyFunctionOverSpiceConstant
    // by converting it to the full Memcache and try to downsize it
    if (p_source == PolyFunctionOverSpice) {
      LineCache(tableName);

      //std::cout << "Full cache size is " << p_cache.size() << endl;
      p_minimizeCache = Yes;
      LoadTimeCache();

      //std::cout << "Minimized cache size is " << p_cache.size() << endl;
    }

    // Load the list of rotations and their corresponding times
    if (p_source == Memcache) {
      TableField q0("J2000Q0", TableField::Double);
      TableField q1("J2000Q1", TableField::Double);
      TableField q2("J2000Q2", TableField::Double);
      TableField q3("J2000Q3", TableField::Double);
      TableField t("ET", TableField::Double);

      TableRecord record;
      record += q0;
      record += q1;
      record += q2;
      record += q3;
      int timePos = 4;

      if (p_hasAngularVelocity) {
        TableField av1("AV1", TableField::Double);
        TableField av2("AV2", TableField::Double);
        TableField av3("AV3", TableField::Double);
        record += av1;
        record += av2;
        record += av3;
        timePos = 7;
      }

      record += t;
      Table table(tableName, record);

      std::vector<ale::Rotation> rots = m_orientation->getRotations();
      std::vector<ale::Vec3d> angularVelocities = m_orientation->getAngularVelocities();
      for (int i = 0; i < (int) p_cacheTime.size(); i++) {
        std::vector<double> quat = rots[i].toQuaternion();

        // If the first component is less than zero, multiply the whole quaternion by -1. This
        // matches NAIF.
        if (quat[0] < 0) {
          quat[0] = -1 * quat[0];
          quat[1] = -1 * quat[1];
          quat[2] = -1 * quat[2];
          quat[3] = -1 * quat[3];
        }

        record[0] = quat[0];
        record[1] = quat[1];
        record[2] = quat[2];
        record[3] = quat[3];

        if (angularVelocities.size() > 0 && p_hasAngularVelocity ) {
          ale::Vec3d angularVelocity = angularVelocities[i];
          record[4] = angularVelocity.x;
          record[5] = angularVelocity.y;
          record[6] = angularVelocity.z;
        }

        record[timePos] = p_cacheTime[i];
        table += record;
      }

      CacheLabel(table);
      return table;
    }
    // Just load the position for the single epoch
    else if (p_source == PolyFunction  &&  p_degree == 0  &&  p_fullCacheSize == 1) {
      return LineCache(tableName);
    }
    // Load the coefficients for the curves fit to the 3 camera angles
    else if (p_source == PolyFunction) {
      TableField angle1("J2000Ang1", TableField::Double);
      TableField angle2("J2000Ang2", TableField::Double);
      TableField angle3("J2000Ang3", TableField::Double);

      TableRecord record;
      record += angle1;
      record += angle2;
      record += angle3;

      Table table(tableName, record);

      for (int cindex = 0; cindex < p_degree + 1; cindex++) {
        record[0] = p_coefficients[0][cindex];
        record[1] = p_coefficients[1][cindex];
        record[2] = p_coefficients[2][cindex];
        table += record;
      }

      // Load one more table entry with the time adjustments for the fit equation
      // t = (et - baseTime)/ timeScale
      record[0] = p_baseTime;
      record[1] = p_timeScale;
      record[2] = (double) p_degree;

      table += record;
      CacheLabel(table);
      return table;
    }
    else {
      // throw an error -- should not get here -- invalid Spice Source
      QString msg = "To create table source of data must be either Memcache or PolyFunction";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Initialize planetary orientation constants from Spice PCK
   *
   * Retrieve planetary orientation constants from a Spice PCK and store them in the class.
   *
   * @param centerBody NAIF id for the planetary body to retrieve the PCK for
   */
  void SpiceRotation::loadPCFromSpice(int centerBody) {
    NaifStatus::CheckErrors();
    SpiceInt centerBodyCode = (SpiceInt) centerBody;

    // Retrieve the frame class from Naif.  We distinguish PCK types into text PCK,
    // binary PCK, and PCK not referenced to J2000.  Isis binary PCK can
    // not be used to solve for target body orientation because of the variety and
    // complexity models used with binary PCK.  Currently ISIS does not solve for
    // target body orientation on bodies not referenced to J2000, but it could be
    // changed to handle that case.
    checkForBinaryPck();

    if (m_frameType == PCK) {
      // Make sure the reference frame is J2000.  We will need to modify FrameTrace and
      // the pck methods in this class to handle this case.  At the time this method was
      // added, this case is not being used in the pck file.  It is mentioned in the Naif required
      // reading file, pck.req, as a possibility for future pck files.
      // Look for a reference frame keyword for the body.  The default is J2000.
      QString naifKeyword = "BODY" + toString(centerBodyCode) + "_CONSTANTS_REF_FRAME" ;
      SpiceInt numExpected;
      SpiceInt numReturned;
      SpiceChar naifType;
      SpiceDouble relativeFrameCode = 0;
      SpiceBoolean found;
      dtpool_c(naifKeyword.toLatin1().data(), &found, &numExpected, &naifType);

      if (found) {
        // Go get the frame name if it is not the default J2000
        SpiceDouble relativeFrameCode;
        bodvcd_c(centerBodyCode, "CONSTANTS_REF_FRAME", 1,
                        &numReturned, &relativeFrameCode);
      }

      if (!found || relativeFrameCode == 1) {  // We only work with J2000 relative frames for now

        // Make sure the standard coefficients are available for the body code by
        // checking for ra
        naifKeyword = "BODY" + toString(centerBodyCode) + "_POLE_RA" ;
        dtpool_c(naifKeyword.toLatin1().data(), &found, &numExpected, &naifType);

        if (found) {
          std::vector<SpiceDouble> d(3);
          m_raPole.resize(numExpected);
          m_decPole.resize(numExpected);
          m_pm.resize(numExpected);

          bodvcd_c(centerBodyCode, "POLE_RA", numExpected, &numReturned, &d[0]);
          m_raPole[0].setDegrees(d[0]);
          m_raPole[1].setDegrees(d[1]);
          m_raPole[2].setDegrees(d[2]);

          bodvcd_c(centerBodyCode, "POLE_DEC", numExpected, &numReturned, &d[0]);
          m_decPole[0].setDegrees(d[0]);
          m_decPole[1].setDegrees(d[1]);
          m_decPole[2].setDegrees(d[2]);

          bodvcd_c(centerBodyCode, "PM", numExpected, &numReturned, &d[0]);
          m_pm[0].setDegrees(d[0]);
          m_pm[1].setDegrees(d[1]);
          m_pm[2].setDegrees(d[2]);

          // ***TODO*** Get long axis value
          m_tOrientationAvailable = true;

          // Now check for nutation/precession terms.  Check for nut/prec ra values
          // first to see if the terms are even used for this body.
          naifKeyword = "BODY" + toString(centerBodyCode) + "_NUT_PREC_RA" ;
          dtpool_c(naifKeyword.toLatin1().data(), &found, &numReturned, &naifType);
          if (found) {
            // Get the barycenter (bc) linear coefficients first (2 for each period).
            // Then we can get the maximum expected coefficients.
            SpiceInt bcCode = centerBodyCode/100;  // Ex: bc code for Jupiter (599) & its moons is 5
            naifKeyword = "BODY" + toString(bcCode) + "_NUT_PREC_ANGLES" ;
            dtpool_c(naifKeyword.toLatin1().data(), &found, &numExpected, &naifType);
            std::vector<double>npAngles(numExpected, 0.);
            bodvcd_c(bcCode, "NUT_PREC_ANGLES", numExpected, &numReturned, &npAngles[0]);
            numExpected /= 2.;
            m_raNutPrec.resize(numExpected, 0.);
            m_decNutPrec.resize(numExpected, 0.);
            m_pmNutPrec.resize(numExpected, 0.);

            std::vector<SpiceDouble> angles(numExpected);
            bodvcd_c(centerBodyCode, "NUT_PREC_RA", numExpected,  &numReturned, &m_raNutPrec[0]);
            bodvcd_c(centerBodyCode, "NUT_PREC_DEC", numExpected,  &numReturned, &m_decNutPrec[0]);
            bodvcd_c(centerBodyCode, "NUT_PREC_PM", numExpected,  &numReturned, &m_pmNutPrec[0]);

            // Finally get the system linear terms separated into vectors of the constants and the
            //  linear coefficients

            for (int i = 0; i < numExpected; i++) {
              m_sysNutPrec0.push_back(Angle(npAngles[i*2], Angle::Degrees));
              m_sysNutPrec1.push_back(Angle(npAngles[i*2+1], Angle::Degrees));
            }
          } // barycenter terms
        }  // PCK constants available
      }  // Relative to J2000
      else {
        m_frameType = NOTJ2000PCK;
      }
    }   // PCK

    NaifStatus::CheckErrors();
  }


  /**
   * Initialize planetary orientation constants from an cube body rotation label
   *
   * Retrieve planetary orientation constants from a cube body rotation label if they are present.
   *
   * @param label const reference to the cube body rotation pvl label
   */
  void SpiceRotation::loadPCFromTable(const PvlObject &label) {
    NaifStatus::CheckErrors();

    // First clear existing cached data
    m_raPole.clear();
    m_decPole.clear();
    m_pm.clear();
    m_raNutPrec.clear();
    m_decNutPrec.clear();
    m_sysNutPrec0.clear();
    m_sysNutPrec1.clear();
    int numLoaded = 0;

    // Load the PCK coeffcients if they are on the label
    if (label.hasKeyword("PoleRa")) {
      PvlKeyword labelCoeffs = label["PoleRa"];
      for (int i = 0; i < labelCoeffs.size(); i++){
        m_raPole.push_back(Angle(toDouble(labelCoeffs[i]), Angle::Degrees));
      }
      numLoaded += 1;
    }
    if (label.hasKeyword("PoleDec")) {
      PvlKeyword labelCoeffs = label["PoleDec"];
      for (int i = 0; i < labelCoeffs.size(); i++){
        m_decPole.push_back(Angle(toDouble(labelCoeffs[i]), Angle::Degrees));
      }
      numLoaded += 1;
    }
    if (label.hasKeyword("PrimeMeridian")) {
      PvlKeyword labelCoeffs = label["PrimeMeridian"];
      for (int i = 0; i < labelCoeffs.size(); i++){
        m_pm.push_back(Angle(toDouble(labelCoeffs[i]), Angle::Degrees));
      }
      numLoaded += 1;
    }
    if (numLoaded > 2) m_tOrientationAvailable = true;

    if (label.hasKeyword("PoleRaNutPrec")) {
      PvlKeyword labelCoeffs = label["PoleRaNutPrec"];
      for (int i = 0; i < labelCoeffs.size(); i++){
        m_raNutPrec.push_back(toDouble(labelCoeffs[i]));
      }
    }
    if (label.hasKeyword("PoleDecNutPrec")) {
      PvlKeyword labelCoeffs = label["PoleDecNutPrec"];
      for (int i = 0; i < labelCoeffs.size(); i++){
        m_decNutPrec.push_back(toDouble(labelCoeffs[i]));
      }
    }
    if (label.hasKeyword("PmNutPrec")) {
      PvlKeyword labelCoeffs = label["PmNutPrec"];
      for (int i = 0; i < labelCoeffs.size(); i++){
        m_pmNutPrec.push_back(toDouble(labelCoeffs[i]));
      }
    }
    if (label.hasKeyword("SysNutPrec0")) {
      PvlKeyword labelCoeffs = label["SysNutPrec0"];
      for (int i = 0; i < labelCoeffs.size(); i++){
        m_sysNutPrec0.push_back(Angle(toDouble(labelCoeffs[i]), Angle::Degrees));
      }
    }
    if (label.hasKeyword("SysNutPrec1")) {
      PvlKeyword labelCoeffs = label["SysNutPrec1"];
      for (int i = 0; i < labelCoeffs.size(); i++){
        m_sysNutPrec1.push_back(Angle(toDouble(labelCoeffs[i]), Angle::Degrees));
      }
    }

    NaifStatus::CheckErrors();
  }


  /**
   * Add labels to a SpiceRotation table.
   *
   * Return a table containing the labels defining the rotation.
   *
   * @param Table    Table to receive labels
   */
  void SpiceRotation::CacheLabel(Table &table) {
    NaifStatus::CheckErrors();
    // Load the constant and time-based frame traces and the constant rotation
    // into the table as labels
    if (p_timeFrames.size() > 1) {
      table.Label() += PvlKeyword("TimeDependentFrames");

      for (int i = 0; i < (int) p_timeFrames.size(); i++) {
        table.Label()["TimeDependentFrames"].addValue(toString(p_timeFrames[i]));
      }
    }

    if (p_constantFrames.size() > 1) {
      table.Label() += PvlKeyword("ConstantFrames");

      for (int i = 0; i < (int) p_constantFrames.size(); i++) {
        table.Label()["ConstantFrames"].addValue(toString(p_constantFrames[i]));
      }

      table.Label() += PvlKeyword("ConstantRotation");

      for (int i = 0; i < (int) p_TC.size(); i++) {
        table.Label()["ConstantRotation"].addValue(toString(p_TC[i]));
      }
    }

    // Write original time coverage
    if (p_fullCacheStartTime != 0) {
      table.Label() += PvlKeyword("CkTableStartTime");
      table.Label()["CkTableStartTime"].addValue(toString(p_fullCacheStartTime));
    }
    if (p_fullCacheEndTime != 0) {
      table.Label() += PvlKeyword("CkTableEndTime");
      table.Label()["CkTableEndTime"].addValue(toString(p_fullCacheEndTime));
    }
    if (p_fullCacheSize != 0) {
      table.Label() += PvlKeyword("CkTableOriginalSize");
      table.Label()["CkTableOriginalSize"].addValue(toString(p_fullCacheSize));
    }

 // Begin section added 06-20-2015 DAC
    table.Label() += PvlKeyword("FrameTypeCode");
    table.Label()["FrameTypeCode"].addValue(toString(m_frameType));

    if (m_frameType == PCK) {
      // Write out all the body orientation constants
      // Pole right ascension coefficients for a quadratic equation
      table.Label() += PvlKeyword("PoleRa");

      for (int i = 0; i < (int) m_raPole.size(); i++) {
        table.Label()["PoleRa"].addValue(toString(m_raPole[i].degrees()));
      }

      // Pole right ascension, declination coefficients for a quadratic equation
      table.Label() += PvlKeyword("PoleDec");
      for (int i = 0; i < (int) m_decPole.size(); i++) {
        table.Label()["PoleDec"].addValue(toString(m_decPole[i].degrees()));
      }

      // Prime meridian coefficients for a quadratic equation
      table.Label() += PvlKeyword("PrimeMeridian");
      for (int i = 0; i < (int) m_pm.size(); i++) {
        table.Label()["PrimeMeridian"].addValue(toString(m_pm[i].degrees()));
      }

      if (m_raNutPrec.size() > 0) {
        // Pole right ascension nutation precision coefficients to the trig terms
        table.Label() += PvlKeyword("PoleRaNutPrec");
        for (int i = 0; i < (int) m_raNutPrec.size(); i++) {
          table.Label()["PoleRaNutPrec"].addValue(toString(m_raNutPrec[i]));
        }

        // Pole declination nutation precision coefficients to the trig terms
        table.Label() += PvlKeyword("PoleDecNutPrec");
        for (int i = 0; i < (int) m_decNutPrec.size(); i++) {
          table.Label()["PoleDecNutPrec"].addValue(toString(m_decNutPrec[i]));
        }

        // Prime meridian nutation precision coefficients to the trig terms
        table.Label() += PvlKeyword("PmNutPrec");
        for (int i = 0; i < (int) m_pmNutPrec.size(); i++) {
          table.Label()["PmNutPrec"].addValue(toString(m_pmNutPrec[i]));
        }

        // System nutation precision constant terms of linear model of periods
        table.Label() += PvlKeyword("SysNutPrec0");
        for (int i = 0; i < (int) m_sysNutPrec0.size(); i++) {
          table.Label()["SysNutPrec0"].addValue(toString(m_sysNutPrec0[i].degrees()));
        }

        // System nutation precision linear terms of linear model of periods
        table.Label() += PvlKeyword("SysNutPrec1");
        for (int i = 0; i < (int) m_sysNutPrec1.size(); i++) {
          table.Label()["SysNutPrec1"].addValue(toString(m_sysNutPrec1[i].degrees()));
        }
      }
    }
 // End section added 06-20-2015 DAC

    NaifStatus::CheckErrors();
  }


  /**
   * Return the camera angles at the center time of the observation
   *
   * @return @b vector<double> Camera angles at center time
   */
  std::vector<double> SpiceRotation::GetCenterAngles() {
    // Compute the center time
    double etCenter = (p_fullCacheEndTime + p_fullCacheStartTime) / 2.;
    SetEphemerisTime(etCenter);

    return Angles(p_axis3, p_axis2, p_axis1);
  }


  /**
   * Return the camera angles (right ascension, declination, and twist) for the
   * time-based matrix CJ
   *
   * @param axis3 The rotation axis for the third angle
   * @param axis2 The rotation axis for the second angle
   * @param axis1 The rotation axis for the first angle
   *
   * @return @b vector<double> Camera angles (ra, dec, twist)
   */
  std::vector<double> SpiceRotation::Angles(int axis3, int axis2, int axis1) {
    NaifStatus::CheckErrors();

    SpiceDouble ang1, ang2, ang3;
    m2eul_c((SpiceDouble *) &p_CJ[0], axis3, axis2, axis1, &ang3, &ang2, &ang1);

    std::vector<double> angles;
    angles.push_back(ang1);
    angles.push_back(ang2);
    angles.push_back(ang3);

    NaifStatus::CheckErrors();
    return angles;
  }



  /**
   * Set the rotation angles (phi, delta, and w) for the current time to define the
   * time-based matrix CJ. This method was created for unitTests and should not
   * be used otherwise.  It only works for cached data with a cache size of 1.
   *
   * @param[in]  angles The angles defining the rotation (phi, delta, and w) in radians
   * @param[in]  axis3    The rotation axis for the third angle
   * @param[in]  axis2    The rotation axis for the second angle
   * @param[in]  axis1    The rotation axis for the first angle
   */
  void SpiceRotation::SetAngles(std::vector<double> angles, int axis3, int axis2, int axis1) {
    eul2m_c(angles[2], angles[1], angles[0], axis3, axis2, axis1, (SpiceDouble (*)[3]) &(p_CJ[0]));

    if (m_orientation) {
      delete m_orientation;
      m_orientation = NULL;
    }
    std::vector<ale::Rotation> rotationCache;
    rotationCache.push_back(ale::Rotation(p_CJ));
    if (p_TC.size() > 1) {
      m_orientation = new ale::Orientations(rotationCache, p_cacheTime,  std::vector<ale::Vec3d>(),
                                            ale::Rotation(p_TC), p_constantFrames, p_timeFrames);
    }
    else {
      m_orientation = new ale::Orientations(rotationCache, p_cacheTime,  std::vector<ale::Vec3d>(),
                                            ale::Rotation(1,0,0,0), p_constantFrames, p_timeFrames);
    }

    // Reset to get the new values
    p_et = -DBL_MAX;
    SetEphemerisTime(p_et);
  }


  /**
   * Accessor method to get the angular velocity
   *
   * @return @b vector<double> Angular velocity
   */
  std::vector<double> SpiceRotation::AngularVelocity() {
    return p_av;
  }


  /**
   * Accessor method to get the frame chain for the constant part of the
   * rotation (ends in target)
   *
   * @return @b vector<int> The frame chain for the constant part of the rotation.
   */
  std::vector<int> SpiceRotation::ConstantFrameChain() {
    return p_constantFrames;
  }


  /**
   * Accessor method to get the frame chain for the rotation (begins in J2000).
   *
   * @return @b vector<int> The frame chain for the rotation.
   */
  std::vector<int> SpiceRotation::TimeFrameChain() {
    return p_timeFrames;
  }


  /**
   * Checks whether the rotation has angular velocities.
   *
   * @return @b bool Indicates whether the rotation has angular velocities.
   */
  bool SpiceRotation::HasAngularVelocity() {
    return p_hasAngularVelocity;
  }


  /**
   * Given a direction vector in the reference frame, return a J2000 direction.
   *
   * @param[in] rVec A direction vector in the reference frame
   *
   * @return vector<double>  A direction vector in J2000 frame.
   */
  std::vector<double> SpiceRotation::J2000Vector(const std::vector<double> &rVec) {
    NaifStatus::CheckErrors();

    std::vector<double> jVec;
    if (rVec.size() == 3) {
      double TJ[3][3];
      mxm_c((SpiceDouble *) &p_TC[0], (SpiceDouble *) &p_CJ[0], TJ);
      jVec.resize(3);
      mtxv_c(TJ, (SpiceDouble *) &rVec[0], (SpiceDouble *) &jVec[0]);
    }

    else if (rVec.size() == 6) {
      // See Naif routine frmchg for the format of the state matrix.  The constant rotation, TC,
      // has a derivative with respect to time of I.
      if (!p_hasAngularVelocity) {
        // throw an error
      }
      std::vector<double> stateTJ(36);
      stateTJ = StateTJ();

      // Now invert (inverse of a state matrix is NOT simply the transpose)
      xpose6_c(&stateTJ[0], (SpiceDouble( *) [6]) &stateTJ[0]);
      double stateJT[6][6];
      invstm_((doublereal *) &stateTJ[0], (doublereal *) stateJT);
      xpose6_c(stateJT, stateJT);
      jVec.resize(6);

      mxvg_c(stateJT, (SpiceDouble *) &rVec[0], 6, 6, (SpiceDouble *) &jVec[0]);
    }
    NaifStatus::CheckErrors();
    return (jVec);
  }


  /**
   * Return the coefficients used to calculate the target body pole ra
   *
   * Return the coefficients used to calculate the target body right
   * ascension without nutation/precession.  The model is a standard
   * quadratic polynomial in time in Julian centuries (36525 days) from
   * the standard epoch (usually J2000).  To match the Naif PCK pole
   * right ascension (usually the same as the most recent IAU Report)
   * the trignometric terms to account for nutation/precession need to
   * be added.
   *
   * pole ra = ra0 + ra1*T + ra2*T**2 + sum(racoef[i]i*sin(angle[i]))
   *
   * @return @b vector<double> A vector of length 3 containing the pole ra coefficients
   */
  std::vector<Angle> SpiceRotation::poleRaCoefs() {
    return m_raPole;
  }


  /**
   * Return the coefficients used to calculate the target body pole dec
   *
   * Return the coefficients used to calculate the target body declination
   * without nutation/precession.  The model is a standard quadratic
   * polynomial in time in Julian centuries (36525 days) from
   * the standard epoch (usually J2000).  To match the Naif PCK pole
   * declination (usually the same as the most recent IAU Report)
   * the trignometric terms to account for nutation/precession need to
   * be added.
   *
   * pole dec = dec0 + dec1*T + dec2*T**2 + sum(racoef[i]i*sin(angle[i]))
   *
   * @return @b vector<double> A vector of length 3 containing the pole declination coefficients.
   */
  std::vector<Angle> SpiceRotation::poleDecCoefs() {
    return m_decPole;
  }


  /**
   * Return the coefficients used to calculate the target body prime meridian
   *
   * Return the coefficients used to calculate the target body prime
   * meridian without nutation/precession.  The model is a standard
   * quadratic polynomial in time in days from the standard epoch
   * (usually J2000).  To match the Naif PCK prime meridian, (usually
   * the same as the most recent IAU Report) the trignometric terms
   * to account for nutation/precession need to be added.
   *
   * pm = pm0 + pm1*d + pm2*d**2 + sum(pmcoef[i]i*sin(angle[i]))
   *
   * @return @b vector<double> A vector of length 3 containing the prime meridian coefficients.
   */
  std::vector<Angle> SpiceRotation::pmCoefs() {
    return m_pm;
  }


  /**
   * Return the coefficients used to calculate the target body pole ra nut/prec coefficients
   *
   * Return the coefficients used to calculate the target body right
   * ascension nutation/precession contribution.  The model is the
   * sum of the products of the coefficients returned by this method
   * with the sin of the corresponding angles associated with the
   * barycenter related to the target body.
   *
   * pole ra = ra0 + ra1*T + ra2*T**2 + sum(raCoef[i]i*sin(angle[i]))
   *
   * @return @b vector<double> A vector containing the pole ra nut/prec coefficients.
   */
  std::vector<double> SpiceRotation::poleRaNutPrecCoefs() {
    return m_raNutPrec;
  }


  /**
   * Return the coefficients used to calculate the target body pole dec nut/prec coefficients
   *
   * Return the coefficients used to calculate the target body declination
   * nutation/precession contribution.  The model is the sum of the products
   * of the coefficients returned by this method with the sin of the corresponding
   * angles associated with the barycenter related to the target body.
   *
   * pole dec = dec0 + dec1*T + dec2*T**2 + sum(decCoef[i]i*sin(angle[i]))
   *
   * @return @b vector<double> A vector containing the pole dec nut/prec coeffcients.
   */
  std::vector<double> SpiceRotation::poleDecNutPrecCoefs() {
    return m_decNutPrec;
  }


  /**
   * Return the coefficients used to calculate the target body pm nut/prec coefficients
   *
   * Return the coefficients used to calculate the target body prime meridian
   * nutation/precession contribution.  The model is the sum of the products
   * of the coefficients returned by this method with the sin of the corresponding
   * angles associated with the barycenter related to the target body.
   *
   * prime meridian = pm0 + pm1*T + pm2*d**2 + sum(pmCoef[i]i*sin(angle[i]))
   *
   * @return @b vector<double> A vector containing the pm nut/prec coeffcients.
   */
  std::vector<double> SpiceRotation::pmNutPrecCoefs() {
    return m_pmNutPrec;
  }


  /**
   * Return the constants used to calculate the target body system nut/prec angles
   *
   * Return the constant terms used to calculate the target body system (barycenter)
   * nutation/precession angles (periods).  The model for the angles is linear in
   * time in Julian centuries since the standard epoch (usually J2000).
   * angles associated with the barycenter related to the target body.
   *
   * angle[i] = constant[i] + coef[i]*T
   *
   * @return @b vector<Angle> A vector containing the system nut/prec constant terms.
   */
  std::vector<Angle> SpiceRotation::sysNutPrecConstants() {
    return m_sysNutPrec0;
  }


  /**
   * Return the coefficients used to calculate the target body system nut/prec angles
   *
   * Return the linear coefficients used to calculate the target body system
   * (barycenter) nutation/precession angles (periods).  The model for the
   * angles is linear in time in Julian centuries since the standard epoch
   * (usually J2000). angles associated with the barycenter related to the
   * target body.
   *
   * angle[i] = constant[i] + coef[i]*T
   *
   * @return @b vector<Angle> A vector containing the system nut/prec linear coefficients.
   */
  std::vector<Angle> SpiceRotation::sysNutPrecCoefs() {
    return m_sysNutPrec1;
  }


  /**
   * Given a direction vector in the reference frame, compute the derivative
   * with respect to one of the coefficients in the angle polynomial fit
   * equation of a vector rotated from the reference frame to J2000.
   * TODO - merge this method with ToReferencePartial
   *
   * @param[in]  lookT A direction vector in the targeted reference frame
   * @param[in]  partialVar  Variable derivative is to be with respect to
   * @param[in]  coeffIndex  Coefficient index in the polynomial fit to the variable (angle)
   *
   * @throws IException::User "Body rotation uses a binary PCK. Solutions for this model are not
   *                           supported"
   * @throws IException::User "Body rotation uses a PCK not referenced to J2000. Solutions for this
   *                           model are not supported"
   * @throws IException::User "Solutions are not supported for this frame type."
   *
   * @return @b vector<double>   A direction vector rotated by derivative
   *                             of reference to J2000 rotation.
   */
  std::vector<double> SpiceRotation::toJ2000Partial(const std::vector<double> &lookT,
                                                    SpiceRotation::PartialType partialVar,
                                                    int coeffIndex) {
    NaifStatus::CheckErrors();

    std::vector<double> jVec;

    // Get the rotation angles and form the derivative matrix for the partialVar
    std::vector<double> angles = Angles(p_axis3, p_axis2, p_axis1);
    int angleIndex = partialVar;
    int axes[3] = {p_axis1, p_axis2, p_axis3};

    double angle = angles.at(angleIndex);

    // Get TJ and apply the transpose to the input vector to get it to J2000
    double dmatrix[3][3];
    drotat_(&angle, (integer *) axes + angleIndex, (doublereal *) dmatrix);
    // Transpose to obtain row-major order
    xpose_c(dmatrix, dmatrix);

    // Get the derivative of the polynomial with respect to partialVar
    double dpoly = 0.;
    QString msg;
    switch (m_frameType) {
     case CK:
     case DYN:
       dpoly = DPolynomial(coeffIndex);
       break;
     case PCK:
       dpoly = DPckPolynomial(partialVar, coeffIndex);
       break;
     case BPC:
       msg = "Body rotation uses a binary PCK.  Solutions for this model are not supported";
       throw IException(IException::User, msg, _FILEINFO_);
       break;
     case NOTJ2000PCK:
       msg = "Body rotation uses a PCK not referenced to J2000. "
             "Solutions for this model are not supported";
       throw IException(IException::User, msg, _FILEINFO_);
       break;
     default:
       msg = "Solutions are not supported for this frame type.";
       throw IException(IException::User, msg, _FILEINFO_);
    }

    // Multiply dpoly to complete dmatrix
    for (int row = 0;  row < 3;  row++) {
      for (int col = 0;  col < 3;  col++) {
        dmatrix[row][col] *= dpoly;
      }
    }

    // Apply the other 2 angles and chain them all together to get J2000 to constant frame
    double dCJ[3][3];
    switch (angleIndex) {
      case 0:
        rotmat_c(dmatrix, angles[1], axes[1], dCJ);
        rotmat_c(dCJ, angles[2], axes[2], dCJ);
        break;
      case 1:
        rotate_c(angles[0], axes[0], dCJ);
        mxm_c(dmatrix, dCJ, dCJ);
        rotmat_c(dCJ, angles[2], axes[2], dCJ);
        break;
      case 2:
        rotate_c(angles[0], axes[0], dCJ);
        rotmat_c(dCJ, angles[1], axes[1], dCJ);
        mxm_c(dmatrix, dCJ, dCJ);
        break;
    }

    // Multiply the constant matrix to rotate to target reference frame
    double dTJ[3][3];
    mxm_c((SpiceDouble *) &p_TC[0], dCJ[0], dTJ);

    // Finally rotate the target vector with the transpose of the
    // derivative matrix, dTJ to get a J2000 vector
    std::vector<double> lookdJ(3);

    mtxv_c(dTJ, (const SpiceDouble *) &lookT[0], (SpiceDouble *) &lookdJ[0]);

    NaifStatus::CheckErrors();
    return (lookdJ);
  }


  /**
   * Given a direction vector in J2000, return a reference frame direction.
   *
   * @param[in] jVec A direction vector in J2000
   *
   * @return @b vector<double> A direction vector in reference frame.
   */
  std::vector<double> SpiceRotation::ReferenceVector(const std::vector<double> &jVec) {
    NaifStatus::CheckErrors();

    std::vector<double> rVec(3);

    if (jVec.size() == 3) {
      double TJ[3][3];
      mxm_c((SpiceDouble *) &p_TC[0], (SpiceDouble *) &p_CJ[0], TJ);
      rVec.resize(3);
      mxv_c(TJ, (SpiceDouble *) &jVec[0], (SpiceDouble *) &rVec[0]);
    }
    else if (jVec.size() == 6) {
      // See Naif routine frmchg for the format of the state matrix.  The constant rotation, TC,
      // has a derivative with respect to time of I.
      if (!p_hasAngularVelocity) {
        // throw an error
      }
      std::vector<double>  stateTJ(36);
      stateTJ = StateTJ();
      rVec.resize(6);
      mxvg_c((SpiceDouble *) &stateTJ[0], (SpiceDouble *) &jVec[0], 6, 6, (SpiceDouble *) &rVec[0]);
    }

    NaifStatus::CheckErrors();
    return (rVec);
  }


  /**
   * Set the coefficients of a polynomial fit to each
   * of the three camera angles for the time period covered by the
   * cache, angle = a + bt + ct**2, where t = (time - p_baseTime)/ p_timeScale.
   *
   * @param type Rotation source type
   *
   * @internal
   *   @history 2012-05-01 Debbie A. Cook - Added type argument to allow other function types
   *                           beyond PolyFunction.
   */
  void SpiceRotation::SetPolynomial(const Source type) {
    NaifStatus::CheckErrors();
    std::vector<double> coeffAng1, coeffAng2, coeffAng3;

    // Rotation is already stored as a polynomial -- throw an error
    if (p_source == PolyFunction) {
      // Nothing to do
      return;
//      QString msg = "Rotation already fit to a polynomial -- spiceint first to refit";
//      throw IException(IException::User,msg,_FILEINFO_);
    }

    // Adjust degree of polynomial on available data
    int size = m_orientation->getRotations().size();
    if (size == 1) {
      p_degree = 0;
    }
    else if (size == 2) {
      p_degree = 1;
    }

    //Check for polynomial over original pointing constant and initialize coefficients
    if (type == PolyFunctionOverSpice) {
      coeffAng1.assign(p_degree + 1, 0.);
      coeffAng2.assign(p_degree + 1, 0.);
      coeffAng3.assign(p_degree + 1, 0.);
      SetPolynomial(coeffAng1, coeffAng2, coeffAng3, type);
      return;
    }

    Isis::PolynomialUnivariate function1(p_degree);   //!< Basis function fit to 1st rotation angle
    Isis::PolynomialUnivariate function2(p_degree);   //!< Basis function fit to 2nd rotation angle
    Isis::PolynomialUnivariate function3(p_degree);   //!< Basis function fit to 3rd rotation angle

    LeastSquares *fitAng1 = new LeastSquares(function1);
    LeastSquares *fitAng2 = new LeastSquares(function2);
    LeastSquares *fitAng3 = new LeastSquares(function3);

    // Compute the base time
    ComputeBaseTime();
    std::vector<double> time;

    if (size == 1) {
      double t = p_cacheTime.at(0);
      SetEphemerisTime(t);
      std::vector<double> angles = Angles(p_axis3, p_axis2, p_axis1);
      coeffAng1.push_back(angles[0]);
      coeffAng2.push_back(angles[1]);
      coeffAng3.push_back(angles[2]);
    }
    else if (size == 2) {
      // Load the times and get the corresponding rotation angles
      p_degree = 1;
      double t1 = p_cacheTime.at(0);
      SetEphemerisTime(t1);
      t1 -= p_baseTime;
      t1 = t1 / p_timeScale;
      std::vector<double> angles1 = Angles(p_axis3, p_axis2, p_axis1);
      double t2 = p_cacheTime.at(1);
      SetEphemerisTime(t2);
      t2 -= p_baseTime;
      t2 = t2 / p_timeScale;
      std::vector<double> angles2 = Angles(p_axis3, p_axis2, p_axis1);
      angles2[0] = WrapAngle(angles1[0], angles2[0]);
      angles2[2] = WrapAngle(angles1[2], angles2[2]);
      double slope[3];
      double intercept[3];

// Compute the linear equation for each angle and save them
      for (int angleIndex = 0; angleIndex < 3; angleIndex++) {
        Isis::LineEquation angline(t1, angles1[angleIndex], t2, angles2[angleIndex]);
        slope[angleIndex] = angline.Slope();
        intercept[angleIndex] = angline.Intercept();
      }
      coeffAng1.push_back(intercept[0]);
      coeffAng1.push_back(slope[0]);
      coeffAng2.push_back(intercept[1]);
      coeffAng2.push_back(slope[1]);
      coeffAng3.push_back(intercept[2]);
      coeffAng3.push_back(slope[2]);
    }
    else {
      // Load the known values to compute the fit equation
      double start1 = 0.; // value of 1st angle1 in cache
      double start3 = 0.; // value of 1st angle1 in cache

      for (std::vector<double>::size_type pos = 0; pos < p_cacheTime.size(); pos++) {
        double t = p_cacheTime.at(pos);
        time.push_back((t - p_baseTime) / p_timeScale);
        SetEphemerisTime(t);
        std::vector<double> angles = Angles(p_axis3, p_axis2, p_axis1);

// Fix 180/-180 crossovers on angles 1 and 3 before doing fit.
        if (pos == 0) {
          start1 = angles[0];
          start3 = angles[2];
        }
        else {
          angles[0] = WrapAngle(start1, angles[0]);
          angles[2] = WrapAngle(start3, angles[2]);
        }

        fitAng1->AddKnown(time, angles[0]);
        fitAng2->AddKnown(time, angles[1]);
        fitAng3->AddKnown(time, angles[2]);
        time.clear();

      }
      //Solve the equations for the coefficients
      fitAng1->Solve();
      fitAng2->Solve();
      fitAng3->Solve();

      // Delete the least squares objects now that we have all the coefficients
      delete fitAng1;
      delete fitAng2;
      delete fitAng3;

      // For now assume all three angles are fit to a polynomial.  Later they may
      // each be fit to a unique basis function.
      // Fill the coefficient vectors

      for (int i = 0;  i < function1.Coefficients(); i++) {
        coeffAng1.push_back(function1.Coefficient(i));
        coeffAng2.push_back(function2.Coefficient(i));
        coeffAng3.push_back(function3.Coefficient(i));
      }

    }

    // Now that the coefficients have been calculated set the polynomial with them
    SetPolynomial(coeffAng1, coeffAng2, coeffAng3);

    NaifStatus::CheckErrors();
    return;
  }


  /**
   * Set the coefficients of a polynomial fit to each of the
   * three camera angles for the time period covered by the
   * cache, angle = c0 + c1*t + c2*t**2 + ... + cn*t**n,
   * where t = (time - p_baseTime) / p_timeScale, and n = p_degree.
   *
   * @param[in] coeffAng1 Coefficients of fit to Angle 1
   * @param[in] coeffAng2 Coefficients of fit to Angle 2
   * @param[in] coeffAng3 Coefficients of fit to Angle 3
   * @param[in] type Rotation source type
   *
   * @internal
   *   @history 2012-05-01 Debbie A. Cook - Added type argument to allow other function types.
   */
  void SpiceRotation::SetPolynomial(const std::vector<double> &coeffAng1,
                                    const std::vector<double> &coeffAng2,
                                    const std::vector<double> &coeffAng3,
                                    const Source type) {

    if (type == PolyFunctionOverSpice && !m_orientation) {
      QString msg = "The quaternion SPICE tables are no longer available. " 
                    "Either re-run spiceinit or set OVEREXISTING to False.";
      throw IException(IException::User, msg, _FILEINFO_);
    }                             
    NaifStatus::CheckErrors();
    Isis::PolynomialUnivariate function1(p_degree);
    Isis::PolynomialUnivariate function2(p_degree);
    Isis::PolynomialUnivariate function3(p_degree);

    // Load the functions with the coefficients
    function1.SetCoefficients(coeffAng1);
    function2.SetCoefficients(coeffAng2);
    function3.SetCoefficients(coeffAng3);

    // Compute the base time
    ComputeBaseTime();

    // Save the current coefficients
    p_coefficients[0] = coeffAng1;
    p_coefficients[1] = coeffAng2;
    p_coefficients[2] = coeffAng3;

    // Set the flag indicating p_degree has been applied to the camera angles, the
    // coefficients of the polynomials have been saved, and the cache reloaded from
    // the polynomials
    p_degreeApplied = true;
    //    p_source = PolyFunction;
    p_source = type;

    // Update the current rotation
    double et = p_et;
    p_et = -DBL_MAX;
    SetEphemerisTime(et);

    NaifStatus::CheckErrors();
    return;
  }


  /**
   * Set the coefficients of a polynomial fit to each
   * of the three planet angles for the time period covered by the
   * cache, ra = ra0 + ra1*t + ra2*t**2 + raTrig,
   *            dec = dec0 + dec1*t + dec2*t**2 + decTrig,
   *            pm = pm0 + pm1*d + pm2*t**2 + pmTrig,
   * where t = time / (seconds per day). for time = et
   *            d = t  / (days per Julian century), and
   * the trig terms are of the form sum(i = 0, numTerms) tcoef[i] * sin(constant[i] + lcoef[i]*t)
   * for ra and pm and sum(i = 0, numTerms) tcoef[i] * cos(constant[i] + lcoef[i]*t)
   * for dec.
   *
   * @throws IException::User "Target body orientation information not available. Rerun spiceinit."
   *
   * @internal
   *   @history 2015-07-01 Debbie A. Cook - Original version.
   */
  void SpiceRotation::usePckPolynomial() {

    // Check to see if rotation is already stored as a polynomial
    if (p_source == PckPolyFunction) {
      p_hasAngularVelocity = true;
      return;
    }

    // No target body orientation information available -- throw an error
    if (!m_tOrientationAvailable) {
      QString msg = "Target body orientation information not available.  Rerun spiceinit.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    /* I think we have nothing to do, but check for available coefficients and set the Source
    // Set the scales
    p_dscale =  0.000011574074074;   // seconds to days conversion
    p_tscale = 3.1688087814029D-10; // seconds to Julian centuries conversion
    //Set the quadratic part of the equations
    //TODO consider just hard coding this part in evaluate
    Isis::PolynomialUnivariate functionRa(p_degree);  // Basis function fit to target body pole ra
    Isis::PolynomialUnivariate functionDec(p_degree);  // Basis function fit to target body pole dec
    Isis::PolynomialUnivariate functionPm(p_degree);  // Basis function fit to target body pm

    // Load the functions with the coefficients
    function1.SetCoefficients(p_raPole);
    function2.SetCoefficients(p_decPole);
    function3.SetCoefficients(p_pm);

    int numNutPrec = p_sysNutPrec0.size;
    if (numNutPrec > 0) {
      // Set the trigonometric part of the equation
      Isis::TrigBasis functionRaNutPrec("raNutPrec", Isis::TrigBasis::Sin, numNutPrec);
      Isis::TrigBasis functionRaNutPrec("decNutPrec", Isis::TrigBasis::Cos, numNutPrec);
      Isis::TrigBasis functionRaNutPrec("pmNutPrec", Isis::TrigBasis::Sin, numNutPrec);
    // Load the functions with the coefficients
      functionRaNutPrec.SetTCoefficients(p_raNutPrec);
      functionDecNutPrec.SetTCoefficients(p_decNutPrec);
      functionPmNutPrec.SetTCoefficients(p_pmNutPrec);
      functionRaNutPrec.SetConstants(p_sysNutPrec0);
      functionDecNutPrec.SetConstants(p_sysNutPrec0);
      functionPmNutPrec.SetConstants(p_sysNutPrec0);
      functionRaNutPrec.SetLCoefficients(p_sysNutPrec1);
      functionDecNutPrec.SetLCoefficients(p_sysNutPrec1);
      functionPmNutPrec.SetLCoefficients(p_sysNutPrec1);
    }
    */

    // Set the flag indicating p_degree has been applied to the planet angles, the
    // coefficients of the polynomials have been saved, and the cache reloaded from
    // TODO cache reloaded???
    // the polynomials.
    // ****At least for the planet angles, I don't think we want to reload the cache until just
    // before we write out the table
    p_degreeApplied = true;
    p_hasAngularVelocity = true;
    p_source = PckPolyFunction;
   return;
  }


  /**
   * Set the coefficients of a polynomial fit to each
   * of the three planet angles for the time period covered by the
   * cache, ra = ra0 + ra1*t + ra2*t**2 + raTrig,
   *            dec = dec0 + dec1*t + dec2*t**2 + decTrig,
   *            pm = pm0 + pm1*d + pm2*t**2 + pmTrig,
   * where t = time / (seconds per day). for time = et
   *            d = t  / (days per Julian century), and
   * the trig terms are of the form sum(i = 0, numTerms) tcoef[i] * sin(constant[i] + lcoef[i]*t)
   * for ra and pm and sum(i = 0, numTerms) tcoef[i] * cos(constant[i] + lcoef[i]*t)
   * for dec.
   *
   * @param[in] raCoeff  pole ra coefficients to set
   * @param[in] decCoeff pole dec coefficients to set
   * @param[in] pmCoeff  pole pm coefficients to set
   *
   * @internal
   *   @history 2012-05-01 Debbie A. Cook - Original version.
   */
  void SpiceRotation::setPckPolynomial(const std::vector<Angle> &raCoeff,
                                       const std::vector<Angle> &decCoeff,
                                       const std::vector<Angle> &pmCoeff) {
    // Just set the constants and let usePckPolynomial() do the rest
    m_raPole = raCoeff;
    m_decPole = decCoeff;
    m_pm = pmCoeff;
    usePckPolynomial();
    // Apply new function parameters
    setEphemerisTimePckPolyFunction();
  }


  /**
   * Return the coefficients of a polynomial fit to each of the
   * three camera angles for the time period covered by the cache, angle =
   * c0 + c1*t + c2*t**2 + ... + cn*t**n, where t = (time - p_basetime) / p_timeScale
   * and n = p_degree.
   *
   * @param[out] coeffAng1 Coefficients of fit to Angle 1
   * @param[out] coeffAng2 Coefficients of fit to Angle 2
   * @param[out] coeffAng3 Coefficients of fit to Angle 3
   */
  void SpiceRotation::GetPolynomial(std::vector<double> &coeffAng1,
                                    std::vector<double> &coeffAng2,
                                    std::vector<double> &coeffAng3) {
    coeffAng1 = p_coefficients[0];
    coeffAng2 = p_coefficients[1];
    coeffAng3 = p_coefficients[2];

    return;
  }


  /**
   * Return the coefficients of a polynomial fit to each of the
   * three planet angles.  See SetPckPolynomial() for more
   * information on the polynomial.
   *
   * TODO??? Should the other coefficients be returned as well???
   *
   * @param[out] raCoeff Quadratic coefficients of fit to ra
   * @param[out] decCoeff Quadratic coefficients of fit to dec
   * @param[out] pmcoeff  Quadratic coefficients of fit to pm
   */
  void SpiceRotation::getPckPolynomial(std::vector<Angle> &raCoeff,
                                       std::vector<Angle> &decCoeff,
                                       std::vector<Angle> &pmCoeff) {
    raCoeff = m_raPole;
    decCoeff = m_decPole;
    pmCoeff = m_pm;

    return;
  }


  /**
   * Compute the base time using cached times
   */
  void SpiceRotation::ComputeBaseTime() {
    if (p_noOverride) {
      p_baseTime = (p_cacheTime.at(0) + p_cacheTime.at(p_cacheTime.size() - 1)) / 2.;
      p_timeScale = p_baseTime - p_cacheTime.at(0);
      // Take care of case where 1st and last times are the same
      if (p_timeScale == 0)  p_timeScale = 1.0;
    }
    else {
      p_baseTime = p_overrideBaseTime;
      p_timeScale = p_overrideTimeScale;
    }

    return;
  }


  /**
   * Set an override base time to be used with observations on scanners to allow all
   * images in an observation to use the save base time and polynomials for the angles.
   *
   * @param[in] baseTime The base time to use and override the computed base time
   * @param[in] timeScale The time scale to use and override the computed time scale
   */
  void SpiceRotation::SetOverrideBaseTime(double baseTime, double timeScale) {
    p_overrideBaseTime = baseTime;
    p_overrideTimeScale = timeScale;
    p_noOverride = false;
    return;
 }

  void SpiceRotation::SetCacheTime(std::vector<double> cacheTime) {
    // Do not reset the cache times if they are already loaded.
    if (p_cacheTime.size() <= 0) {
      p_cacheTime = cacheTime;
    }
  }

  /**
   * Evaluate the derivative of the fit polynomial defined by the
   * given coefficients with respect to the coefficient at the given index, at
   * the current time.
   *
   * @param coeffIndex The index of the coefficient to differentiate
   *
   * @throws IException::Programmer "Unable to evaluate the derivative of the SPICE rotation fit
   *                                 polynomial for the given coefficient index. Index is negative
   *                                 or exceeds degree of polynomial"
   *
   * @return @b double The derivative evaluated at the current time.
   */
  double SpiceRotation::DPolynomial(const int coeffIndex) {
    double derivative;
    double time = (p_et - p_baseTime) / p_timeScale;

    if (coeffIndex > 0  && coeffIndex <= p_degree) {
      derivative = pow(time, coeffIndex);
    }
    else if (coeffIndex == 0) {
      derivative = 1;
    }
    else {
      QString msg = "Unable to evaluate the derivative of the SPICE rotation fit polynomial for "
                    "the given coefficient index [" + toString(coeffIndex) + "]. "
                    "Index is negative or exceeds degree of polynomial ["
                    + toString(p_degree) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return derivative;
  }


  /**
   * Evaluate the derivative of the fit polynomial defined by the
   * given coefficients with respect to the coefficient at the given index, at
   * the current time.
   *
   * @param partialVar Variable derivative is to be with respect to
   * @param coeffIndex The index of the coefficient to differentiate
   *
   * @throws IException::Programmer "Unable to evaluate the derivative of the SPICE rotation fit
   *                                 polynomial for the given coefficient index. Index is negative
   *                                 or exceeds degree of polynomial"
   *
   * @return @b double The derivative evaluated at the current time.
   */
  double SpiceRotation::DPckPolynomial(SpiceRotation::PartialType partialVar,
                                       const int coeffIndex) {
    const double p_dayScale = 86400; // number of seconds in a day
 // Number of 24 hour days in a Julian century = 36525
    const double p_centScale = p_dayScale * 36525;
    double time = 0.;

    switch (partialVar) {
     case WRT_RightAscension:
     case WRT_Declination:
       time = p_et / p_centScale;
       break;
     case WRT_Twist:
       time = p_et / p_dayScale;
       break;
    }

    double derivative;

    switch (coeffIndex) {
     case 0:
       derivative = 1;
       break;
    case 1:
    case 2:
      derivative = pow(time, coeffIndex);
      break;
    default:
      QString msg = "Unable to evaluate the derivative of the target body rotation fit polynomial "
                    "for the given coefficient index [" + toString(coeffIndex) + "]. "
                    "Index is negative or exceeds degree of polynomial ["
                    + toString(p_degree) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Now apply any difference to the partials due to building the rotation matrix on the angles
    //    90. + ra, 90. - dec, and pm.  The only partial that changes is dec.
    if (partialVar == WRT_Declination) {
      derivative = -derivative;
    }

    return derivative;
  }


  /**
   * Compute the derivative with respect to one of the coefficients in the
   * angle polynomial fit equation of a vector rotated from J2000 to a
   * reference frame.  The polynomial equation is of the form
   * angle = c0 + c1*t + c2*t**2 + ... cn*t**n, where t = (time - p_basetime) / p_timeScale
   * and n = p_degree (the degree of the equation)
   *
   * @param[in]  lookJ       Look vector in J2000 frame
   * @param[in]  partialVar  Variable derivative is to be with respect to
   * @param[in]  coeffIndex  Coefficient index in the polynomial fit to the variable (angle)
   *
   * @throws IException::Programmer "Only CK, DYN, and PCK partials can be calculated"
   *
   * @return @b vector<double> Vector rotated by derivative of J2000 to reference rotation.
   */
  std::vector<double> SpiceRotation::ToReferencePartial(std::vector<double> &lookJ,
                                                        SpiceRotation::PartialType partialVar,
                                                        int coeffIndex) {
    NaifStatus::CheckErrors();
    //TODO** To save time possibly save partial matrices

    // Get the rotation angles and form the derivative matrix for the partialVar
    std::vector<double> angles = Angles(p_axis3, p_axis2, p_axis1);
    int angleIndex = partialVar;
    int axes[3] = {p_axis1, p_axis2, p_axis3};

    double angle = angles.at(angleIndex);

    double dmatrix[3][3];
    drotat_(&angle, (integer *) axes + angleIndex, (doublereal *) dmatrix);
    // Transpose to obtain row-major order
    xpose_c(dmatrix, dmatrix);

    // Get the derivative of the polynomial with respect to partialVar
    double dpoly = 0.;
    switch (m_frameType) {
     case UNKNOWN:  // For now let everything go through for backward compatability
     case CK:
     case DYN:
      dpoly = DPolynomial(coeffIndex);
       break;
     case PCK:
       dpoly = DPckPolynomial(partialVar, coeffIndex);
       break;
     default:
      QString msg = "Only CK, DYN, and PCK partials can be calculated";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Multiply dpoly to complete dmatrix
    for (int row = 0;  row < 3;  row++) {
      for (int col = 0;  col < 3;  col++) {
        dmatrix[row][col] *= dpoly;
      }
    }

    // Apply the other 2 angles and chain them all together
    double dCJ[3][3];
    switch (angleIndex) {
      case 0:
        rotmat_c(dmatrix, angles[1], axes[1], dCJ);
        rotmat_c(dCJ, angles[2], axes[2], dCJ);
        break;
      case 1:
        rotate_c(angles[0], axes[0], dCJ);
        mxm_c(dmatrix, dCJ, dCJ);
        rotmat_c(dCJ, angles[2], axes[2], dCJ);
        break;
      case 2:
        rotate_c(angles[0], axes[0], dCJ);
        rotmat_c(dCJ, angles[1], axes[1], dCJ);
        mxm_c(dmatrix, dCJ, dCJ);
        break;
    }

    // Multiply the constant matrix to rotate to the targeted reference frame
    double dTJ[3][3];
    mxm_c((SpiceDouble *) &p_TC[0], dCJ[0], dTJ);

    // Finally rotate the J2000 vector with the derivative matrix, dTJ to
    // get the vector in the targeted reference frame.
    std::vector<double> lookdT(3);

    mxv_c(dTJ, (const SpiceDouble *) &lookJ[0], (SpiceDouble *) &lookdT[0]);

    NaifStatus::CheckErrors();
    return lookdT;
  }


  /**
   * Wrap the input angle to keep it within 2pi radians of the angle to compare.
   *
   * @param[in]  compareAngle Look vector in J2000 frame
   * @param[in]  angle Angle to be wrapped if needed
   *
   * @return @b double Wrapped angle.
   */
  double SpiceRotation::WrapAngle(double compareAngle, double angle) {
    NaifStatus::CheckErrors();
    double diff1 = compareAngle - angle;

    if (diff1 < -1 * pi_c()) {
      angle -= twopi_c();
    }
    else if (diff1 > pi_c()) {
      angle += twopi_c();
    }

    NaifStatus::CheckErrors();
    return angle;
  }


  /**
   * Set the degree of the polynomials to be fit to the
   * three camera angles for the time period covered by the
   * cache, angle = c0 + c1*t + c2*t**2 + ... + cn*t**n,
   * where t = (time - p_baseTime) / p_timeScale, and n = p_degree.
   *
   * @param[in] degree Degree of the polynomial to be fit
   *
   * @internal
   *   @history 2011-03-22 Debbie A. Cook - Fixed bug in second branch where existing
   *                           degree is greater than new degree.
   */
  void SpiceRotation::SetPolynomialDegree(int degree) {
    // Adjust the degree for the data
    if (p_fullCacheSize == 1) {
      degree = 0;
    }
    else if (p_fullCacheSize == 2) {
      degree = 1;
    }
    // If polynomials have not been applied yet then simply set the degree and return
    if (!p_degreeApplied) {
      p_degree = degree;
    }

    // Otherwise the existing polynomials need to be either expanded ...
    else if (p_degree < degree) {   // (increase the number of terms)
      std::vector<double> coefAngle1(p_coefficients[0]),
          coefAngle2(p_coefficients[1]),
          coefAngle3(p_coefficients[2]);

      for (int icoef = p_degree + 1;  icoef <= degree; icoef++) {
        coefAngle1.push_back(0.);
        coefAngle2.push_back(0.);
        coefAngle3.push_back(0.);
      }
      p_degree = degree;
      SetPolynomial(coefAngle1, coefAngle2, coefAngle3, p_source);
    }
    // ... or reduced (decrease the number of terms)
    else if (p_degree > degree) {
      std::vector<double> coefAngle1(degree + 1),
          coefAngle2(degree + 1),
          coefAngle3(degree + 1);

      for (int icoef = 0;  icoef <= degree;  icoef++) {
        coefAngle1[icoef] = p_coefficients[0][icoef];
        coefAngle2[icoef] = p_coefficients[1][icoef];
        coefAngle3[icoef] = p_coefficients[2][icoef];
      }
      p_degree = degree;
      SetPolynomial(coefAngle1, coefAngle2, coefAngle3, p_source);
    }
  }


  /**
   * Accessor method to get the rotation frame type.
   *
   * @return @b SpiceRotation::FrameType The frame type of the rotation.
   */
  SpiceRotation::FrameType SpiceRotation::getFrameType() {
    return m_frameType;
  }


  /**
   * Accessor method to get the rotation source.
   *
   * @return @b SpiceRotation::Source The source of the rotation.
   */
  SpiceRotation::Source SpiceRotation::GetSource() {
    return p_source;
  }


  /**
   * Resets the source of the rotation to the given value.
   *
   * @param source The rotation source to be set.
   */
  void SpiceRotation::SetSource(Source source) {
    p_source = source;
    return;
  }


  /**
   * Accessor method to get the rotation base time.
   *
   * @return @b double The base time for the rotation.
   */
  double SpiceRotation::GetBaseTime() {
    return p_baseTime;
  }


  /**
   * Accessor method to get the rotation time scale.
   *
   * @return @b double The time scale for the rotation.
   */
  double SpiceRotation::GetTimeScale() {
    return p_timeScale;
  }


  /**
   * Set the axes of rotation for decomposition of a rotation
   * matrix into 3 angles.
   *
   * @param[in]  axis1 Axes of rotation of first angle applied (right rotation)
   * @param[in]  axis2 Axes of rotation of second angle applied (center rotation)
   * @param[in]  axis3 Axes of rotation of third angle applied (left rotation)
   *
   * @throws IException::Programmer "A rotation axis is outside the valid range of 1 to 3"
   *
   * @return @b double Wrapped angle.
   */
  void SpiceRotation::SetAxes(int axis1, int axis2, int axis3) {
    if (axis1 < 1  ||  axis2 < 1  || axis3 < 1  || axis1 > 3  || axis2 > 3  || axis3 > 3) {
      QString msg = "A rotation axis is outside the valid range of 1 to 3";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    p_axis1 = axis1;
    p_axis2 = axis2;
    p_axis3 = axis3;
  }


  /**
   * Load the time cache.  This method should works with the LoadCache(startTime, endTime, size)
   * method to load the time cache.
   *
   * @throws IException::Programmer "Full cache size does NOT match cache size in LoadTimeCache --
   *                                 should never happen"
   * @throws IException::Programmer "Observation crosses segment boundary--unable to interpolate
   *                                 pointing"
   * @throws IException::User "No camera kernels loaded...Unable to determine time cache to
   *                           downsize"
   */
  void SpiceRotation::LoadTimeCache() {
    NaifStatus::CheckErrors();
    int count = 0;

    double observStart  =  p_fullCacheStartTime + p_timeBias;
    double observEnd  = p_fullCacheEndTime + p_timeBias;
    //  Next line added 12-03-2009 to allow observations to cross segment boundaries
    double currentTime = observStart;
    bool timeLoaded = false;

    // Get number of ck loaded for this rotation.  This method assumes only one SpiceRotation
    // object is loaded.
    NaifStatus::CheckErrors();
    ktotal_c("ck", (SpiceInt *) &count);

    // Downsize the loaded cache
    if ((p_source == Memcache) && p_minimizeCache == Yes) {
      // Multiple ck case, type 5 ck case, or PolyFunctionOverSpice
      //  final step -- downsize loaded cache and reload

      if (p_fullCacheSize != (int) p_cacheTime.size()) {
        QString msg =
          "Full cache size does NOT match cache size in LoadTimeCache -- should never happen";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      SpiceDouble timeSclkdp[p_fullCacheSize];
      SpiceDouble quats[p_fullCacheSize][4];
      double avvs[p_fullCacheSize][3]; // Angular velocity vector

      // We will treat et as the sclock time and avoid converting back and forth
     std::vector<ale::Rotation> fullRotationCache = m_orientation->getRotations();
     std::vector<ale::Vec3d> angularVelocities = m_orientation->getAngularVelocities();
     for (int r = 0; r < p_fullCacheSize; r++) {
        timeSclkdp[r] = p_cacheTime[r];
        std::vector<double> rotationMatrix = fullRotationCache[r].toRotationMatrix();
        SpiceDouble CJ[9] = { rotationMatrix[0], rotationMatrix[1], rotationMatrix[2],
                              rotationMatrix[3], rotationMatrix[4], rotationMatrix[5],
                              rotationMatrix[6], rotationMatrix[7], rotationMatrix[8]
                            };
        m2q_c(CJ, quats[r]);
        if (p_hasAngularVelocity) {
          ale::Vec3d angularVelocity = angularVelocities[r];
          vequ_c((SpiceDouble *) &angularVelocity.x, avvs[r]);
        }
     }

      double cubeStarts = timeSclkdp[0]; //,timsSclkdp[ckBlob.Records()-1] };
      double radTol = 0.000000017453; //.000001 degrees  Make this instrument dependent TODO
      SpiceInt avflag = 1;            // Don't use angular velocity for now
      SpiceInt nints = 1;             // Always make an observation a single interpolation interval
      double dparr[p_fullCacheSize];    // Double precision work array
      SpiceInt intarr[p_fullCacheSize]; // Integer work array
      SpiceInt sizOut = p_fullCacheSize; // Size of downsized cache

      ck3sdn(radTol, avflag, (int *) &sizOut, timeSclkdp, (doublereal *) quats,
             (SpiceDouble *) avvs, nints, &cubeStarts, dparr, (int *) intarr);

      // Clear full cache and load with downsized version
      p_cacheTime.clear();
      std::vector<double> av;
      av.resize(3);

      if (m_orientation) {
        delete m_orientation;
        m_orientation = NULL;
      }

      std::vector<ale::Rotation> rotationCache;
      std::vector<ale::Vec3d> avCache;

      for (int r = 0; r < sizOut; r++) {
        SpiceDouble et;
        // sct2e_c(spcode, timeSclkdp[r], &et);
        et = timeSclkdp[r];
        p_cacheTime.push_back(et);
        std::vector<double> CJ(9);
        q2m_c(quats[r], (SpiceDouble( *)[3]) &CJ[0]);
        rotationCache.push_back(CJ);
        vequ_c(avvs[r], (SpiceDouble *) &av[0]);
        avCache.push_back(av);
      }

      if (p_TC.size() > 1 ) {
        m_orientation = new ale::Orientations(rotationCache, p_cacheTime, avCache,
                                              ale::Rotation(p_TC), p_constantFrames, p_timeFrames);
      }
      else {
        m_orientation = new ale::Orientations(rotationCache, p_cacheTime,  std::vector<ale::Vec3d>(),
                                            ale::Rotation(1,0,0,0), p_constantFrames, p_timeFrames);
      }
      timeLoaded = true;
      p_minimizeCache = Done;
    }
    else if (count == 1  && p_minimizeCache == Yes) {
      // case of a single ck -- read instances and data straight from kernel for given time range
      SpiceInt handle;

      // Define some Naif constants
      int FILESIZ = 128;
      int TYPESIZ = 32;
      int SOURCESIZ = 128;
//      double DIRSIZ = 100;

      SpiceChar file[FILESIZ];
      SpiceChar filtyp[TYPESIZ];  // kernel type (ck, ek, etc.)
      SpiceChar source[SOURCESIZ];

      SpiceBoolean found;
      bool observationSpansToNextSegment = false;

      double segStartEt;
      double segStopEt;

      kdata_c(0, "ck", FILESIZ, TYPESIZ, SOURCESIZ, file, filtyp, source, &handle, &found);
      dafbfs_c(handle);
      daffna_c(&found);
      int spCode = ((int)(p_constantFrames[0] / 1000)) * 1000;

      while (found) {
        observationSpansToNextSegment = false;
        double sum[10];   // daf segment summary
        double dc[2];     // segment starting and ending times in tics
        SpiceInt ic[6];   // segment summary values:
        // instrument code for platform,
        // reference frame code,
        // data type,
        // velocity flag,
        // offset to quat 1,
        // offset to end.
        dafgs_c(sum);
        dafus_c(sum, (SpiceInt) 2, (SpiceInt) 6, dc, ic);

        // Don't read type 5 ck here
        if (ic[2] == 5) break;

        // Check times for type 3 ck segment if spacecraft matches
        if (ic[0] == spCode && ic[2] == 3) {
          sct2e_c((int) spCode / 1000, dc[0], &segStartEt);
          sct2e_c((int) spCode / 1000, dc[1], &segStopEt);
          NaifStatus::CheckErrors();
          double et;

          // Get times for this segment
          if (currentTime >= segStartEt  &&  currentTime <= segStopEt) {

            // Check for a gap in the time coverage by making sure the time span of the observation
            //  does not cross a segment unless the next segment starts where the current one ends
            if (observationSpansToNextSegment && currentTime > segStartEt) {
              QString msg = "Observation crosses segment boundary--unable to interpolate pointing";
              throw IException(IException::Programmer, msg, _FILEINFO_);
            }
            if (observEnd > segStopEt) {
              observationSpansToNextSegment = true;
            }

            // Extract necessary header parameters
            int dovelocity = ic[3];
            int end = ic[5];
            double val[2];
            dafgda_c(handle, end - 1, end, val);
//            int nints = (int) val[0];
            int ninstances = (int) val[1];
            int numvel  =  dovelocity * 3;
            int quatnoff  =  ic[4] + (4 + numvel) * ninstances - 1;
//            int nrdir = (int) (( ninstances - 1 ) / DIRSIZ); /* sclkdp directory records */
            int sclkdp1off  =  quatnoff + 1;
            int sclkdpnoff  =  sclkdp1off + ninstances - 1;
//            int start1off = sclkdpnoff + nrdir + 1;
//            int startnoff = start1off + nints - 1;
            int sclkSpCode = spCode / 1000;

            // Now get the times
            std::vector<double> sclkdp(ninstances);
            dafgda_c(handle, sclkdp1off, sclkdpnoff, (SpiceDouble *) &sclkdp[0]);

            int instance = 0;
            sct2e_c(sclkSpCode, sclkdp[0], &et);

            while (instance < (ninstances - 1)  &&  et < currentTime) {
              instance++;
              sct2e_c(sclkSpCode, sclkdp[instance], &et);
            }

            if (instance > 0) instance--;
            sct2e_c(sclkSpCode, sclkdp[instance], &et);

            while (instance < (ninstances - 1)  &&  et < observEnd) {
              p_cacheTime.push_back(et - p_timeBias);
              instance++;
              sct2e_c(sclkSpCode, sclkdp[instance], &et);
            }
            p_cacheTime.push_back(et - p_timeBias);

            if (!observationSpansToNextSegment) {
              timeLoaded = true;
              p_minimizeCache = Done;
              break;
            }
            else {
              currentTime = segStopEt;
            }
          }
        }
        dafcs_c(handle);     // Continue search in daf last searched
        daffna_c(&found);    // Find next forward array in current daf
      }
    }
    else if (count == 0  &&  p_source != Nadir  &&  p_minimizeCache == Yes) {
      QString msg = "No camera kernels loaded...Unable to determine time cache to downsize";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Load times according to cache size (body rotations) -- handle first round of type 5 ck case
    //   and multiple ck case --Load a time for every line scan line and downsize later
    if (! (timeLoaded || (p_cacheTime.size() > 1))) {
      double cacheSlope = 0.0;
      if (p_fullCacheSize > 1)
        cacheSlope = (p_fullCacheEndTime - p_fullCacheStartTime) / (double)(p_fullCacheSize - 1);
      for (int i = 0; i < p_fullCacheSize; i++)
        p_cacheTime.push_back(p_fullCacheStartTime + (double) i * cacheSlope);
      if (p_source == Nadir) {
        p_minimizeCache = No;
      }
    }
    NaifStatus::CheckErrors();
  }


  /**
   * Return full listing (cache) of original time coverage requested.
   *
   * @throws IException::User "Time cache not availabe -- rerun spiceinit"
   *
   * @return @b vector<double> Cache of original time coverage.
   */
  std::vector<double> SpiceRotation::GetFullCacheTime() {

    // No time cache was initialized -- throw an error
    if (p_fullCacheSize < 1) {
      QString msg = "Time cache not available -- rerun spiceinit";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    std::vector<double> fullCacheTime;
    double cacheSlope = 0.0;
    if (p_fullCacheSize > 1) {
      cacheSlope = (p_fullCacheEndTime - p_fullCacheStartTime) / (double)(p_fullCacheSize - 1);
    }

    for (int i = 0; i < p_fullCacheSize; i++)
      fullCacheTime.push_back(p_fullCacheStartTime + (double) i * cacheSlope);

    return fullCacheTime;
  }


  /**
   * Compute frame trace chain from target frame to J2000
   *
   * @param et Ephemeris time
   *
   * @throws IException::Programmer "The frame is not supported by Naif"
   * @throws IException::Programmer "The ck rotation from frame can not be found due to no pointing
   *                                 available at request time or a problem with the frame"
   * @throws IException::Programmer "The frame has a type not supported by your version of Naif
   *                                 Spicelib. You need to update."
   */
  void SpiceRotation::FrameTrace(double et) {
    NaifStatus::CheckErrors();
    // The code for this method was extracted from the Naif routine rotget written by N.J. Bachman &
    //   W.L. Taber (JPL)
    int           center;
    FrameType type;
    int           typid;
    SpiceBoolean  found;
    int           frmidx;  // Frame chain index for current frame
    SpiceInt      nextFrame;   // Naif frame code of next frame
    NaifStatus::CheckErrors();
    std::vector<int> frameCodes;
    std::vector<int> frameTypes;
    frameCodes.push_back(p_constantFrames[0]);

    while (frameCodes[frameCodes.size()-1] != J2000Code) {
      frmidx  =  frameCodes.size() - 1;
      // First get the frame type  (Note:: we may also need to save center if we use dynamic frames)
      // (Another note:  the type returned is the Naif from type.  This is not quite the same as the
      // SpiceRotation enumerated FrameType.  The SpiceRotation FrameType differentiates between
      // pck types.  FrameTypes of 2, 6, and 7 will all be considered to be Naif frame type 2.  The
      // logic for FrameTypes in this method is correct for all types except type 7.  Current pck
      // do not exercise this option.  Should we ever use pck with a target body not referenced to
      // the J2000 frame and epoch, both this method and loadPCFromSpice will need to be modified.
      frinfo_c((SpiceInt) frameCodes[frmidx],
               (SpiceInt *) &center,
               (SpiceInt *) &type,
               (SpiceInt *) &typid, &found);

      if (!found) {

        if (p_source == Nadir) {
          frameTypes.push_back(0);
          break;
        }

        QString msg = "The frame" + toString((int) frameCodes[frmidx]) + " is not supported by Naif";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      double matrix[3][3];

      // To get the next link in the frame chain, use the frame type
      if (type == INERTL ||  type == PCK) {
        nextFrame = J2000Code;
      }
      else if (type == CK) {
        ckfrot_((SpiceInt *) &typid, &et, (double *) matrix, &nextFrame, (logical *) &found);

        if (!found) {

          if (p_source == Nadir) {
            frameTypes.push_back(0);
            break;
          }

          QString msg = "The ck rotation from frame " + toString(frameCodes[frmidx]) + " can not "
               + "be found due to no pointing available at requested time or a problem with the "
               + "frame";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }
      }
      else if (type == TK) {
        tkfram_((SpiceInt *) &typid, (double *) matrix, &nextFrame, (logical *) &found);
        if (!found) {
          QString msg = "The tk rotation from frame " + toString(frameCodes[frmidx]) +
                        " can not be found";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }
      }
      else if (type == DYN) {
        //
        //        Unlike the other frame classes, the dynamic frame evaluation
        //        routine ZZDYNROT requires the input frame ID rather than the
        //        dynamic frame class ID. ZZDYNROT also requires the center ID
        //        we found via the FRINFO call.

        zzdynrot_((SpiceInt *) &typid, (SpiceInt *) &center, &et, (double *) matrix, &nextFrame);
      }

      else {
        QString msg = "The frame " + toString(frameCodes[frmidx]) +
            " has a type " + toString(type) + " not supported by your version of Naif Spicelib."
            + "You need to update.";
        throw IException(IException::Programmer, msg, _FILEINFO_);

      }
      frameCodes.push_back(nextFrame);
      frameTypes.push_back(type);
    }

    if ((int) frameCodes.size() == 1  &&  p_source != Nadir) {  // Must be Sky
      p_constantFrames.push_back(frameCodes[0]);
      p_timeFrames.push_back(frameCodes[0]);
      return;
    }

    int nConstants = 0;
    p_constantFrames.clear();
    while (frameTypes[nConstants] == TK  &&  nConstants < (int) frameTypes.size()) nConstants++;


    for (int i = 0; i <= nConstants; i++) {
      p_constantFrames.push_back(frameCodes[i]);
    }

    if (p_source != Nadir)  {
      for (int i = nConstants;  i < (int) frameCodes.size(); i++) {
        p_timeFrames.push_back(frameCodes[i]);
      }
    }
    else {
      // Nadir rotation is from spacecraft to J2000
      p_timeFrames.push_back(frameCodes[nConstants]);
      p_timeFrames.push_back(J2000Code);
    }
    NaifStatus::CheckErrors();
  }


  /**
   * Return the full rotation TJ as a matrix
   *
   * @return @b vector<double> Returned matrix.
   */
  std::vector<double> SpiceRotation::Matrix() {
    NaifStatus::CheckErrors();
    std::vector<double> TJ;
    TJ.resize(9);
    mxm_c((SpiceDouble *) &p_TC[0], (SpiceDouble *) &p_CJ[0], (SpiceDouble( *) [3]) &TJ[0]);
    NaifStatus::CheckErrors();
    return TJ;
  }


  /**
   * Return the constant 3x3 rotation TC matrix as a quaternion.
   *
   * @return @b vector<double> Constant rotation quaternion, TC.
   */
  std::vector<double> SpiceRotation::ConstantRotation() {
    NaifStatus::CheckErrors();
    std::vector<double> q;
    q.resize(4);
    m2q_c((SpiceDouble( *)[3]) &p_TC[0], &q[0]);
    NaifStatus::CheckErrors();
    return q;
  }


  /**
   * Return the constant 3x3 rotation TC matrix as a vector of length 9.
   *
   * @return @b vector<double> Constant rotation matrix, TC.
   */
  std::vector<double> &SpiceRotation::ConstantMatrix() {
    return p_TC;
  }


  /**
   * Set the constant 3x3 rotation TC matrix from a vector of length 9.
   *
   * @param constantMatrix Constant rotation matrix, TC.
   */
  void SpiceRotation::SetConstantMatrix(std::vector<double> constantMatrix) {
    p_TC = constantMatrix;
    return;
  }

  /**
   * Return time-based 3x3 rotation CJ matrix as a quaternion.
   *
   * @return @b vector<double> Time-based rotation quaternion, CJ.
   */
  std::vector<double> SpiceRotation::TimeBasedRotation() {
    NaifStatus::CheckErrors();
    std::vector<double> q;
    q.resize(4);
    m2q_c((SpiceDouble( *)[3]) &p_CJ[0], &q[0]);
    NaifStatus::CheckErrors();
    return q;
  }


  /**
   * Return time-based 3x3 rotation CJ matrix as a vector of length 9.
   *
   * @return @b vector<double> Time-based rotation matrix, CJ.
   */
  std::vector<double> &SpiceRotation::TimeBasedMatrix() {
    return p_CJ;
  }


  /**
   * Set the time-based 3x3 rotation CJ matrix from a vector of length 9.
   *
   * @param timeBasedMatrix Time-based rotation matrix, TC.
   */
  void SpiceRotation::SetTimeBasedMatrix(std::vector<double> timeBasedMatrix) {
    p_CJ = timeBasedMatrix;
    return;
  }


  /**
   * Initialize the constant rotation
   *
   * @param et Ephemeris time.
   */
  void SpiceRotation::InitConstantRotation(double et) {
    FrameTrace(et);
    // Get constant rotation which applies in all cases
    int targetFrame = p_constantFrames[0];
    int fromFrame = p_timeFrames[0];
    p_TC.resize(9);
    refchg_((SpiceInt *) &fromFrame, (SpiceInt *) &targetFrame, &et, (doublereal *) &p_TC[0]);
    // Transpose to obtain row-major order
    xpose_c((SpiceDouble( *)[3]) &p_TC[0], (SpiceDouble( *)[3]) &p_TC[0]);
  }


  /**
   * Compute the angular velocity from the time-based functions fit to the pointing angles
   * This method computes omega = angular velocity matrix, and extracts the angular velocity.
   * See comments in the Naif Spicelib routine xf2rav_c.c.
   *
   *            _                     _
   *           |                       |
   *           |   0    -av[2]  av[1]  |
   *           |                       |
   *   omega = |  av[2]    0   -av[0]  |
   *           |                       |
   *           | -av[1]   av[0]   0    |
   *           |_                     _|
   *
   * @throws IException::Programmer "The SpiceRotation pointing angles must be fit to polynomials
   *                                 in order to compute angular velocity."
   * @throws IException::Programmer "Planetary angular velocity must be fit computed with PCK
   *                                 polynomials"
   */
  void SpiceRotation::ComputeAv() {
    NaifStatus::CheckErrors();

    // Make sure the angles have been fit to polynomials so we can calculate the derivative
    if (p_source < PolyFunction ) {
      QString msg = "The SpiceRotation pointing angles must be fit to polynomials in order to ";
      msg += "compute angular velocity.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    if (p_source == PckPolyFunction) {
      QString msg = "Planetary angular velocity must be fit computed with PCK polynomials ";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    std::vector<double> dCJdt;
    dCJdt.resize(9);

    switch (m_frameType) {
        // Treat all cases the same except for target body rotations
      case UNKNOWN:
      case INERTL:
      case TK:
      case DYN:
      case CK:
        DCJdt(dCJdt);
        break;
      case PCK:
      case BPC:
      case NOTJ2000PCK:
        break;
      }
    double omega[3][3];
    mtxm_c((SpiceDouble( *)[3]) &dCJdt[0], (SpiceDouble( *)[3]) &p_CJ[0], omega);
    p_av[0] = omega[2][1];
    p_av[1] = omega[0][2];
    p_av[2] = omega[1][0];
    NaifStatus::CheckErrors();
  }


  /**
   * Compute the derivative of the 3x3 rotation matrix CJ with respect to time.
   * The derivative is computed based on p_CJ (J2000 to first constant frame).
   *
   * @param[out]  dCJ       Derivative of p_CJ
   */
  void SpiceRotation::DCJdt(std::vector<double> &dCJ) {
    NaifStatus::CheckErrors();

    // Get the rotation angles and axes
    std::vector<double> angles = Angles(p_axis3, p_axis2, p_axis1);
    int axes[3] = {p_axis1, p_axis2, p_axis3};

    double dmatrix[3][3];
    double dangle;
    double wmatrix[3][3]; // work matrix
    dCJ.assign(9, 0.);

    for (int angleIndex = 0; angleIndex < 3; angleIndex++) {
      drotat_(&(angles[angleIndex]), (integer *) axes + angleIndex, (doublereal *) dmatrix);
      // Transpose to obtain row-major order
      xpose_c(dmatrix, dmatrix);

      // To get the derivative of the polynomial fit to the angle with respect to time
      // first create the function object for this angle and load its coefficients
      Isis::PolynomialUnivariate function(p_degree);
      function.SetCoefficients(p_coefficients[angleIndex]);

      // Evaluate the derivative of function at p_et
      //      dangle = function.DerivativeVar((p_et - p_baseTime) / p_timeScale);
      dangle = function.DerivativeVar((p_et - p_baseTime) / p_timeScale) / p_timeScale;

      // Multiply dangle to complete dmatrix
      for (int row = 0;  row < 3;  row++) {
        for (int col = 0;  col < 3;  col++) {
          dmatrix[row][col] *= dangle;
        }
      }
      // Apply the other 2 angles and chain them all together
      switch (angleIndex) {
        case 0:
          rotmat_c(dmatrix, angles[1], axes[1], dmatrix);
          rotmat_c(dmatrix, angles[2], axes[2], dmatrix);
          break;
        case 1:
          rotate_c(angles[0], axes[0], wmatrix);
          mxm_c(dmatrix, wmatrix, dmatrix);
          rotmat_c(dmatrix, angles[2], axes[2], dmatrix);
          break;
        case 2:
          rotate_c(angles[0], axes[0], wmatrix);
          rotmat_c(wmatrix, angles[1], axes[1], wmatrix);
          mxm_c(dmatrix, wmatrix, dmatrix);
          break;
      }
      int i, j;
      for (int index = 0; index < 9; index++) {
        i = index / 3;
        j = index % 3;
        dCJ[index] += dmatrix[i][j];
      }
    }

    NaifStatus::CheckErrors();
  }


  /**
   * Compute & return the rotation matrix that rotates vectors from J2000 to the targeted frame.
   *
   * @return @b vector<double> Returned rotation matrix.
   */
  std::vector<double> SpiceRotation::StateTJ() {
    std::vector<double> stateTJ(36);

    // Build the state matrix for the time-based rotation from the matrix and angulary velocity
    double stateCJ[6][6];
    rav2xf_c(&p_CJ[0], &p_av[0], stateCJ);
// (SpiceDouble (*) [3]) &p_CJ[0]
    int irow = 0;
    int jcol = 0;
    int vpos = 0;

    for (int row = 3; row < 6; row++) {
      irow  =  row - 3;
      vpos  =  irow * 3;

      for (int col = 0; col < 3; col++) {
        jcol  =  col + 3;
        // Fill the upper left corner
        stateTJ[irow*6 + col] = p_TC[vpos] * stateCJ[0][col] + p_TC[vpos+1] * stateCJ[1][col]
                                              + p_TC[vpos+2] * stateCJ[2][col];
        // Fill the lower left corner
        stateTJ[row*6 + col]  =  p_TC[vpos] * stateCJ[3][col] + p_TC[vpos+1] * stateCJ[4][col]
                                              + p_TC[vpos+2] * stateCJ[5][col];
        // Fill the upper right corner
        stateTJ[irow*6 + jcol] = 0;
        // Fill the lower right corner
        stateTJ[row*6 +jcol] = stateTJ[irow*6 + col];
      }
    }
    return stateTJ;
  }


  /**
   * Extrapolate pointing for a given time assuming a constant angular velocity.
   * The pointing and angular velocity at the current time will be used to
   * extrapolate pointing at the input time.  If angular velocity does not
   * exist, the value at the current time will be output.
   *
   * @param[in]   timeEt    The time of the pointing to be extrapolated
   *
   * @return @b vector<double> A quaternion defining the rotation at the input time.
   */
   std::vector<double> SpiceRotation::Extrapolate(double timeEt) {
    NaifStatus::CheckErrors();

    if (!p_hasAngularVelocity){
      return p_CJ;
    }

    double diffTime = timeEt - p_et;
    std::vector<double> CJ(9, 0.0);
    double dmat[3][3];

    // Create a rotation matrix for the axis and magnitude of the angular velocity multiplied by
    //   the time difference
    axisar_c((SpiceDouble *) &p_av[0], diffTime*vnorm_c((SpiceDouble *) &p_av[0]), dmat);

    // Rotate from the current time to the desired time assuming constant angular velocity
    mxm_c(dmat, (SpiceDouble *) &p_CJ[0], (SpiceDouble( *)[3]) &CJ[0]);
    NaifStatus::CheckErrors();
    return CJ;
   }


   /**
    * Set the full cache time parameters.
    *
    * @param[in]   startTime The earliest time of the full cache coverage
    * @param[in]   endTime   The latest time of the full cache coverage
    * @param[in]   cacheSize The number of epochs in the full (line) cache
    */
   void SpiceRotation::SetFullCacheParameters(double startTime, double endTime, int cacheSize) {
     // Save full cache parameters
     p_fullCacheStartTime = startTime;
     p_fullCacheEndTime = endTime;
     p_fullCacheSize = cacheSize;
   }


  /**
   * Check loaded pck to see if any are binary and set frame type to indicate binary pck.
   *
   * This is strictly a local method to be called only when the source is Spice.  Its purpose is
   * to check loaded PCK to see if any are binary.  If any loaded PCK is binary, the body rotation
   * label keyword FrameTypeCode is set to BPC (6).
   */
  void SpiceRotation::checkForBinaryPck() {
    // Get a count of all the loaded kernels
    SpiceInt count;
    ktotal_c("PCK", &count);

    // Define some Naif constants
    int FILESIZ = 128;
    int TYPESIZ = 32;
    int SOURCESIZ = 128;
    SpiceChar file[FILESIZ];
    SpiceChar filetype[TYPESIZ];
    SpiceChar source[SOURCESIZ];
    SpiceInt handle;
    SpiceBoolean found;

    // Get the name of each loaded kernel.  The accuracy of this test depends on the use of the
    // "bpc" suffix to name a binary pck.  If a binary bpc does not have the "bpc" suffix, it will not
    // be found by this test.  This test was suggested by Boris Semenov at Naif.
    for (SpiceInt knum = 0; knum < count; knum++){
      kdata_c(knum, "PCK", FILESIZ, TYPESIZ, SOURCESIZ, file, filetype, source, &handle, &found);

      // search file for "bpc"
      if (strstr(file, "bpc") != NULL) {
        m_frameType = BPC;
      }
    }
  }


  /**
   * Set the frame type (m_frameType).
   *
   * This is strictly a local method to be called only when the source is Spice.  Its purpose is
   * to determine the frame type.  If the frame type is PCK, this method also loads the
   * planetary constants.  See SpiceRotation.h to see the valid frame types.
   */
  void SpiceRotation::setFrameType() {
    SpiceInt frameCode = p_constantFrames[0];
    SpiceBoolean found;
    SpiceInt centerBodyCode;
    SpiceInt frameClass;
    SpiceInt classId;
    frinfo_c(frameCode, &centerBodyCode, &frameClass, &classId, &found);

    if (found) {
      if (frameClass == 2  ||  (centerBodyCode > 0 && frameClass != 3)) {
        m_frameType = PCK;
        // Load the PC information while it is available and set member values
        loadPCFromSpice(centerBodyCode);
      }
      else if (p_constantFrames.size() > 1) {
        for (std::vector<int>::size_type idx = 1; idx < p_constantFrames.size(); idx++) {
          frameCode = p_constantFrames[idx];
          frinfo_c(frameCode, &centerBodyCode, &frameClass, &classId, &found);
          if (frameClass == 3) m_frameType = CK;
        }
      }
      else {
        switch (frameClass) {
        case 1:
          m_frameType = INERTL;
          break;
          // case 2:  handled in first if block
        case 3:
          m_frameType = CK;
          break;
        case 4:
          m_frameType = TK;
          break;
        case 5:
          m_frameType = DYN;
          break;
        default:
          m_frameType = UNKNOWN;
        }
      }
    }
  }


  /**
   * Updates rotation state based on the rotation cache
   *
   * When setting the ephemeris time, this method is used to update the rotation state by reading
   * from the rotation cache
   *
   * @see SpiceRotation::SetEphemerisTime
   */
  void SpiceRotation::setEphemerisTimeMemcache() {
    // If the cache has only one rotation, set it
    NaifStatus::CheckErrors();
    if (p_cacheTime.size() == 1) {
      p_CJ = m_orientation->getRotations()[0].toRotationMatrix();
      if (p_hasAngularVelocity) {
        ale::Vec3d av = m_orientation->getAngularVelocities()[0];
        p_av[0] = av.x;
        p_av[1] = av.y;
        p_av[2] = av.z;
      }
    }
    // Otherwise determine the interval to interpolate
    else {
      p_CJ = m_orientation->interpolateTimeDep(p_et).toRotationMatrix();

      if (p_hasAngularVelocity) {
        ale::Vec3d av = m_orientation->interpolateAV(p_et);
        p_av[0] = av.x;
        p_av[1] = av.y;
        p_av[2] = av.z;
      }
    }
    NaifStatus::CheckErrors();
  }


  /**
   * When setting the ephemeris time, uses spacecraft nadir source to update the rotation state
   *
   * @see SpiceRotation::SetEphemerisTime
   */
  void SpiceRotation::setEphemerisTimeNadir() {
   // TODO what about spk time bias and mission setting of light time corrections
   //      That information has only been passed to the SpicePosition class and
   //      is not available to this class, but probably should be applied to the
   //      spkez call.

   // Make sure the constant frame is loaded.  This method also does the frame trace.
   NaifStatus::CheckErrors();
    if (p_timeFrames.size() == 0) InitConstantRotation(p_et);

    SpiceDouble stateJ[6];  // Position and velocity vector in J2000
    SpiceDouble lt;
    SpiceInt spkCode = p_constantFrames[0] / 1000;
    spkez_c((SpiceInt) spkCode, p_et, "J2000", "LT+S",
            (SpiceInt) p_targetCode, stateJ, &lt);
   // reverse the position to be relative to the spacecraft.  This may be
   // a mission dependent value and possibly the sense of the velocity as well.
    SpiceDouble sJ[3], svJ[3];
    vpack_c(-1 * stateJ[0], -1 * stateJ[1], -1 * stateJ[2], sJ);
    vpack_c(stateJ[3], stateJ[4], stateJ[5], svJ);
    twovec_c(sJ,
             p_axisP,
             svJ,
             p_axisV,
             (SpiceDouble( *)[3]) &p_CJ[0]);
    NaifStatus::CheckErrors();
  }


  /**
   * When setting the ephemeris time, updates the rotation state based on data read directly
   * from NAIF kernels using NAIF Spice routines
   *
   * @throws IException::Io "[framecode] is an unrecognized reference frame code. Has the mission
   *                         frames kernel been loaded?"
   * @throws IException::Io "No pointing is availabe at request time for frame code"
   *
   * @see SpiceRotation::SetEphemerisTime
   */
  void SpiceRotation::setEphemerisTimeSpice() {
   NaifStatus::CheckErrors();
   SpiceInt j2000 = J2000Code;

   SpiceDouble time = p_et + p_timeBias;
   // Make sure the constant frame is loaded.  This method also does the frame trace.
   if (p_timeFrames.size() == 0) InitConstantRotation(p_et);
   int toFrame = p_timeFrames[0];

   // First try getting the entire state matrix (6x6), which includes CJ and the angular velocity
   double stateCJ[6][6];
   frmchg_((integer *) &j2000, (integer *) &toFrame, &time, (doublereal *) stateCJ);

   // If Naif fails attempting to get the state matrix, assume the angular velocity vector is
   // not available and just get the rotation matrix.  First turn off Naif error reporting and
   // return any error without printing them.
   SpiceBoolean ckfailure = failed_c();
   reset_c();                   // Reset Naif error system to allow caller to recover

   if (!ckfailure) {
     xpose6_c(stateCJ, stateCJ);
     xf2rav_c(stateCJ, (SpiceDouble( *)[3]) &p_CJ[0], (SpiceDouble *) &p_av[0]);
     p_hasAngularVelocity = true;
   }
   else {
     refchg_((integer *) &j2000, (integer *) &toFrame, &time, (SpiceDouble *) &p_CJ[0]);

     if (failed_c()) {
       char naifstr[64];
       getmsg_c("SHORT", sizeof(naifstr), naifstr);
       reset_c();  // Reset Naif error system to allow caller to recover

       if (eqstr_c(naifstr, "SPICE(UNKNOWNFRAME)")) {
         Isis::IString msg = Isis::IString((int) p_constantFrames[0]) + " is an unrecognized " +
                             "reference frame code.  Has the mission frames kernel been loaded?";
         throw IException(IException::Io, msg, _FILEINFO_);
       }
       else {
         Isis::IString msg = "No pointing available at requested time [" +
                             Isis::IString(p_et + p_timeBias) + "] for frame code [" +
                             Isis::IString((int) p_constantFrames[0]) + "]";
         throw IException(IException::Io, msg, _FILEINFO_);
       }
     }

     // Transpose to obtain row-major order
     xpose_c((SpiceDouble( *)[3]) &p_CJ[0], (SpiceDouble( *)[3]) &p_CJ[0]);
   }

   NaifStatus::CheckErrors();
  }


  /**
   * Evaluate the polynomial fit function for the three pointing angles for the current
   * ephemeris time.
   *
   * @return @b vector<double> Vector containing the three rotation angles.
   */
  std::vector<double> SpiceRotation::EvaluatePolyFunction() {
   Isis::PolynomialUnivariate function1(p_degree);
   Isis::PolynomialUnivariate function2(p_degree);
   Isis::PolynomialUnivariate function3(p_degree);

   // Load the functions with the coefficients
   function1.SetCoefficients(p_coefficients[0]);
   function2.SetCoefficients(p_coefficients[1]);
   function3.SetCoefficients(p_coefficients[2]);

   std::vector<double> rtime;
   rtime.push_back((p_et - p_baseTime) / p_timeScale);
   std::vector<double> angles;
   angles.push_back(function1.Evaluate(rtime));
   angles.push_back(function2.Evaluate(rtime));
   angles.push_back(function3.Evaluate(rtime));

   // Get the first angle back into the range Naif expects [-180.,180.]
   if (angles[0] <= -1 * pi_c()) {
     angles[0] += twopi_c();
   }
   else if (angles[0] > pi_c()) {
     angles[0] -= twopi_c();
   }
   return angles;
  }


  /**
   * When setting the ephemeris time, updates the rotation according to a polynomial
   * that defines the three camera angles and angular velocity, if available.
   *
   * @see SpiceRotatation::SetEphemerisTime
   */
  void SpiceRotation::setEphemerisTimePolyFunction() {
   NaifStatus::CheckErrors();
   Isis::PolynomialUnivariate function1(p_degree);
   Isis::PolynomialUnivariate function2(p_degree);
   Isis::PolynomialUnivariate function3(p_degree);

   // Load the functions with the coefficients
   function1.SetCoefficients(p_coefficients[0]);
   function2.SetCoefficients(p_coefficients[1]);
   function3.SetCoefficients(p_coefficients[2]);

   std::vector<double> rtime;
   rtime.push_back((p_et - p_baseTime) / p_timeScale);
   double angle1 = function1.Evaluate(rtime);
   double angle2 = function2.Evaluate(rtime);
   double angle3 = function3.Evaluate(rtime);

   // Get the first angle back into the range Naif expects [-180.,180.]
   if (angle1 < -1 * pi_c()) {
     angle1 += twopi_c();
   }
   else if (angle1 > pi_c()) {
     angle1 -= twopi_c();
   }

   eul2m_c((SpiceDouble) angle3, (SpiceDouble) angle2, (SpiceDouble) angle1,
           p_axis3,              p_axis2,              p_axis1,
           (SpiceDouble( *)[3]) &p_CJ[0]);

   if (p_hasAngularVelocity) {
     if ( p_degree == 0){
       ale::Vec3d av = m_orientation->getAngularVelocities()[0];
       p_av[0] = av.x;
       p_av[1] = av.y;
       p_av[2] = av.z;
     }
       else {
       ComputeAv();
     }
   }
   NaifStatus::CheckErrors();
  }


  /**
   * When setting the ephemeris time, updates the rotation state based on a polynomial fit
   * over spice kernel data.
   *
   * @see SpiceRotation::SetEphemerisTime
   */
  void SpiceRotation::setEphemerisTimePolyFunctionOverSpice() {
    setEphemerisTimeMemcache();
    NaifStatus::CheckErrors();
    std::vector<double> cacheAngles(3);
    std::vector<double> cacheVelocity(3);
    cacheAngles = Angles(p_axis3, p_axis2, p_axis1);
    cacheVelocity = p_av;
    setEphemerisTimePolyFunction();
    std::vector<double> polyAngles(3);
    // The decomposition fails because the angles are outside the valid range for Naif
    // polyAngles = Angles(p_axis3, p_axis2, p_axis1);
    polyAngles = EvaluatePolyFunction();
    std::vector<double> polyVelocity(3);
    polyVelocity = p_av;
    std::vector<double> angles(3);

    for (int index = 0; index < 3; index++) {
      angles[index] = cacheAngles[index] + polyAngles[index];
      p_av[index] += cacheVelocity[index];
    }

    if (angles[0] <= -1 * pi_c()) {
      angles[0] += twopi_c();
    }
    else if (angles[0] > pi_c()) {
      angles[0] -= twopi_c();
    }

    if (angles[2] <= -1 * pi_c()) {
      angles[2] += twopi_c();
    }
    else if (angles[2] > pi_c()) {
      angles[2] -= twopi_c();
    }

    eul2m_c((SpiceDouble) angles[2], (SpiceDouble) angles[1], (SpiceDouble) angles[0],
            p_axis3,                 p_axis2,                 p_axis1,
            (SpiceDouble( *)[3]) &p_CJ[0]);
  }


  /**
   * When setting the ephemeris time, updates the rotation state based on the PcK polynomial
   *
   * @see SpiceRotation::SetEphemerisTime
   */
  void SpiceRotation::setEphemerisTimePckPolyFunction() {
    // Set the scales
    double dTime =  p_et / 86400.;   // seconds to days conversion
    double centTime = dTime / 36525; // seconds to Julian centuries conversion
    double secondsPerJulianCentury = 86400. * 36525.;

    // Calculate the quadratic part of the expression for the angles in degrees
    // Note: phi = ra + 90. and delta = 90 - dec for comparing results with older Isis 2 data
    Angle ra = m_raPole[0] + centTime*(m_raPole[1] + m_raPole[2]*centTime);
    Angle dec =  m_decPole[0] + centTime*(m_decPole[1] + m_decPole[2]*centTime);
    Angle pm = m_pm[0] + dTime*(m_pm[1] + m_pm[2]*dTime);
    Angle dra = (m_raPole[1] + 2.*m_raPole[2]*centTime) / secondsPerJulianCentury;
    Angle ddec = (m_decPole[1] + 2.*m_decPole[2]*centTime) / secondsPerJulianCentury;
    Angle dpm = (m_pm[1] + 2.*m_pm[2]*dTime) / 86400;

    // Now add the nutation/precession (trig part) adjustment to the expression if any
    int numNutPrec = (int) m_raNutPrec.size();
    Angle theta;
    double dtheta;
    double costheta;
    double sintheta;

    for (int ia = 0;  ia < numNutPrec; ia++) {
      theta = m_sysNutPrec0[ia] + m_sysNutPrec1[ia]*centTime;
      dtheta = m_sysNutPrec1[ia].degrees() * DEG2RAD;
      costheta = cos(theta.radians());
      sintheta = sin(theta.radians());
      ra += Angle(m_raNutPrec[ia] * sintheta, Angle::Degrees);
      dec += Angle(m_decNutPrec[ia] * costheta, Angle::Degrees);
      pm += Angle(m_pmNutPrec[ia] * sintheta, Angle::Degrees);
      dra += Angle(m_raNutPrec[ia] * dtheta / secondsPerJulianCentury * costheta, Angle::Degrees);
      ddec -= Angle(m_decNutPrec[ia] * dtheta / secondsPerJulianCentury * sintheta, Angle::Degrees);
      dpm += Angle(m_pmNutPrec[ia] * dtheta / secondsPerJulianCentury * costheta, Angle::Degrees);
    }

    // Get pm in correct range
    pm = Angle(fmod(pm.degrees(), 360), Angle::Degrees);  // Get pm back into the domain range

    if (ra.radians() < -1 * pi_c()) {
      ra += Angle(twopi_c(), Angle::Radians);
    }
    else if (ra.radians() > pi_c()) {
      ra -= Angle(twopi_c(), Angle::Radians);
    }

    if (pm.radians() < -1 * pi_c()) {
      pm += Angle(twopi_c(), Angle::Radians);
    }
    else if (pm.radians() > pi_c()) {
      pm -= Angle(twopi_c(), Angle::Radians);
    }

    // Convert to Euler angles to get the matrix
    SpiceDouble phi = ra.radians() + HALFPI;
    SpiceDouble delta = HALFPI - dec.radians();
    SpiceDouble w = pm.radians();
    SpiceDouble dphi = dra.radians();
    SpiceDouble ddelta = -ddec.radians();
    SpiceDouble dw = dpm.radians();

    // Load the Euler angles and their derivatives into an array and create the state matrix
    SpiceDouble angsDangs[6];
    SpiceDouble BJs[6][6];
    vpack_c(w, delta, phi, angsDangs);
    vpack_c(dw, ddelta, dphi, &angsDangs[3]);
    eul2xf_c (angsDangs, p_axis3, p_axis2, p_axis1, BJs);

    // Decompose the state matrix to the rotation and its angular velocity
    xf2rav_c(BJs, (SpiceDouble( *)[3]) &p_CJ[0], (SpiceDouble *) &p_av[0]);
  }
}
