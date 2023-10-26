/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "RosettaVirtisCamera.h"

#include <cctype>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

#include <QRegExp>
#include <QString>

#include <tnt/tnt_array2d_utils.h>

#include "Camera.h"
#include "CameraFocalPlaneMap.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "Kernels.h"
#include "LineScanCameraDetectorMap.h"
#include "LineScanCameraGroundMap.h"
#include "LineScanCameraSkyMap.h"
#include "NaifStatus.h"
#include "NumericalApproximation.h"

// #define DUMP_INFO 1

using namespace std;
namespace Isis {
    // constructors
  /**
   * Creates a camera for a Rosetta VIRTIS-M cube
   *
   * @param cube The cube to make a camera for
   *
   */
  RosettaVirtisCamera::RosettaVirtisCamera(Cube &cube) : LineScanCamera(cube) {

    m_instrumentNameLong = "Visual and Infrared Thermal Imaging Spectrometer";
    m_instrumentNameShort = "VIRTIS";
    m_spacecraftNameLong = "Rosetta";
    m_spacecraftNameShort = "Rosetta";

 //     cout << "Testing RosettaVirtisCamera...\n";

    Pvl &lab = *cube.label();
//    PvlGroup &archive = lab.findGroup("Archive", Isis::Pvl::Traverse);
    PvlGroup &inst = lab.findGroup("Instrument", Isis::Pvl::Traverse);

    QString instrumentId = inst["InstrumentId"];
    if ( "virtis" != instrumentId.toLower()) {
      QString mess = "This data is apparently not from the VIRTIS instrument but "
                      + instrumentId;
      throw IException(IException::User, mess, _FILEINFO_);
    }


    int procLevel = inst["ProcessingLevelId"];
    m_is1BCalibrated = (procLevel > 2) ? true : false;

    // Get the start time from labels
    QString channelId = inst["ChannelId"];

    QString instMode = inst["InstrumentModeId"];
    m_slitMode = instMode[14].toLatin1();   // "F" for full slit, Q for quarter slit


    // Check for presence of articulation kernel
    bool hasArtCK = hasArticulationKernel(lab);

    // Set proper end frame
    int virFrame(0);
    QString frameId ("");
    if (channelId == "VIRTIS_M_VIS") {
      // Frame ROS_VIRTIS-M_VIS : ROS_VIRTIS-M_VIS_ZERO
      virFrame = (hasArtCK) ? -226211 : -226112;
      frameId = "ROS_VIRTIS-M_VIS";
    }
    else if  (channelId == "VIRTIS_M_IR") {
      // Frame ROS_VIRTIS-M_IR : ROS_VIRTIS-M_IR_ZERO
      virFrame = (hasArtCK) ? -226213 : -226214;
      frameId = "ROS_VIRTIS-M_IR";
    }

    instrumentRotation()->SetFrame(virFrame);

    // We do not want to downsize the cache
    instrumentRotation()->MinimizeCache(SpiceRotation::No);

    // Set up the camera info from ik/iak kernels
    SetFocalLength();
    SetPixelPitch();

    // Get other info from labels
    PvlKeyword &frameParam = inst["FrameParameter"];

    // convert milliseconds to seconds

    m_exposureTime = toDouble(frameParam[0]) * 0.001;
    m_summing  = toDouble(frameParam[1]);
    m_scanRate = toDouble(frameParam[2]);

    // Setup detector map
    //  Get the line scan rates/times

    if (!m_is1BCalibrated) {
      readHouseKeeping(lab.fileName(), m_scanRate);
    }
    else {
      readSCET(lab.fileName());
    }

    new VariableLineScanCameraDetectorMap(this, m_lineRates);
    DetectorMap()->SetDetectorSampleSumming(m_summing);

    // Setup focal plane map
    new CameraFocalPlaneMap(this, naifIkCode());
    //  Retrieve boresight location from instrument kernel (IK) (addendum?)
    QString ikernKey = "INS" + toString(naifIkCode()) + "_BORESIGHT_SAMPLE";
    double sampleBoreSight = getDouble(ikernKey);

    ikernKey = "INS" + toString(naifIkCode()) + "_BORESIGHT_LINE";
    double lineBoreSight = getDouble(ikernKey);
    FocalPlaneMap()->SetDetectorOrigin(sampleBoreSight, lineBoreSight);

    // Setup distortion map
    new CameraDistortionMap(this);
    // Setup the ground and sky map
    new LineScanCameraGroundMap(this);
    new LineScanCameraSkyMap(this);
    // Set initial start time always (label start time is inaccurate)

    if (!m_is1BCalibrated){
      setTime(iTime(startTime()));
    }

    //  Now check to determine if we have a cache already.  If we have a cache
    //  table, we are beyond spiceinit and have already computed the proper
    //  point table from the housekeeping data or articulation kernel.
    if (!instrumentRotation()->IsCached() && !hasArtCK && !m_is1BCalibrated) {
      // Create new table here prior to creating normal caches
      Table quats = getPointingTable(frameId, virFrame);

      // Create all system tables - all kernels closed after this
      LoadCache();
      instrumentRotation()->LoadCache(quats);
    }
    else {
      LoadCache();
    }

#if defined(DUMP_INFO)
    Table cache = instrumentRotation()->Cache("Loaded");
    cout << "Total Records: " << cache.Records() << "\n";

    for (int i = 0 ; i < cache.Records() ; i++) {
      TableRecord rec = cache[i];
      string separator("");
      for (int f = 0 ; f < rec.Fields() ; f++) {
        cout << separator << (double) rec[f];
        separator = ", ";
      }
      cout << "\n";
    }
#endif
  }

  /**
   * Destructor.
   */
  RosettaVirtisCamera::~RosettaVirtisCamera() {
  }

  /**
   *  Returns CK frame identifier
   *
   * @return @b int The CK frame identifier.
   */
  int RosettaVirtisCamera::CkFrameId() const {
    return (-226000);
  }


  /**
   *  Returns CK reference frame identifier
   *
   * @return @b int The CK reference frame identifier.
   */
  int RosettaVirtisCamera::CkReferenceId() const {
    return (1);
  }


  /**
   *  Return PK reference frame identifier
   *
   * @return @b int The SPK reference frame identifier
   */
  int RosettaVirtisCamera::SpkReferenceId() const {
    return (1);
  }


  /**
   *  Return the pixel summing rate
   *
   * @return @b int The pixel summing rate.
   */
  int RosettaVirtisCamera::pixelSumming() const {
    return (m_summing);
  }


  /**
   *  Return the exposure time
   *
   * @return @b double The exposure time for a pixel.
   */
  double RosettaVirtisCamera::exposureTime() const {
    return (m_exposureTime);
  }


  /**
   *  Return the line scan rate
   *
   * @return @b double The time between lines in the cube.
   */
  double RosettaVirtisCamera::scanLineTime() const {
    return (m_scanRate);
  }


  /**
   *  Return the start time for a given line exposure time
   *
   * @return @b double The et time at the start of the line's exposure.
   */
  double RosettaVirtisCamera::lineStartTime(const double midExpTime) const {
    return (midExpTime - (exposureTime() / 2.0));
  }


  /**
   *  Return the end time for a given line exposure time
   *
   * @return @b double The et time at the end of the line's exposure.
   */
  double RosettaVirtisCamera::lineEndTime(const double midExpTime) const {
    return (midExpTime+(exposureTime()/2.0));
  }


  /**
   *  Return start time for the entire cube.
   *
   * @return @b double The et time at the start of the cube.
   */
  double RosettaVirtisCamera::startTime() const {
    return (lineStartTime(m_mirrorData[0].m_scanLineEt));
  }


  /**
   *  Return end time for the entire cube.
   *
   * @return @b double The et time at the end of the cube.
   */
  double RosettaVirtisCamera::endTime() const {
   return (lineEndTime(m_mirrorData[hkLineCount()-1].m_scanLineEt));
  }


  /**
   *  Returns number of housekeeping records found in the cube Table
   *
   * @return @b int The number of housekeeping records.
   */
  int RosettaVirtisCamera::hkLineCount() const {
    return (m_mirrorData.size());
  }


  /**
   * @brief For calibrated VIRTIS-M images, read the SCET values from the cube
   *
   *
   * @param filename The filename of the cube with the house keeping table.
   * @param linerate The linerate for the cube.
   *
   * @history 2011-07-22 Kris Becker
   */
  void RosettaVirtisCamera::readSCET(const QString &filename) {
   //  Open the ISIS table object
   std::vector<double> cacheTime;
   Table hktable("VIRTISHouseKeeping", filename);
   m_lineRates.clear();
   int lineno(1);
   double lineEndTime = 0;
   for (int i = 0; i < hktable.Records(); i++) {
     TableRecord &trec = hktable[i];
     QString scetString = QString::fromStdString(trec["dataSCET"]);
     lineEndTime = getClockTime(scetString, naifSpkCode()).Et();
     m_lineRates.push_back(LineRateChange(lineno,
                                          lineEndTime-exposureTime(),
                                          exposureTime()));
     cacheTime.push_back(lineEndTime-exposureTime());
     lineno++;
   }
   cacheTime.push_back(lineEndTime);

   // Adjust the last time
   LineRateChange lastR = m_lineRates.back();

    // Normally the line rate changes would store the line scan rate instead of exposure time.
    // Storing the exposure time instead allows for better time calculations within a line.
    // In order for the VariableLineScanCameraDetectorMap to work correctly with this change,
    // every line in the cube must have a LineRateChange object.  This is because determining
    // the start time for one line based on another line requires the line scan rate.  Having
    // a LineRateChange for every line means never needing to calculate the start time for a line
    // because the start time is stored in that line's LineRateChange.  So, the detector map only
    // calculates times within a given line.
    // See VariableLineScanCameraDetectorMap::exposureDuration() for a description of the
    // difference between exposure time and line scan rate.

    m_lineRates.back() = LineRateChange(lastR.GetStartLine(),
                                        lastR.GetStartEt(),
                                        exposureTime());

    instrumentRotation()->SetCacheTime(cacheTime);
  }


  /**
   * @brief Read the VIRTIS houskeeping table from cube
   *
   * This method reads an ISIS Table object from the cube.  This table named,
   * "VIRHouseKeeping", contains four fields: ScetTimeClock, ShutterStatus,
   * MirrorSin, and MirrorCos.  These fields contain the scan line time in
   * SCLK, status of shutter - open, closed (dark), sine and cosine of the
   * scan mirror, respectively.
   *
   * @param filename The filename of the cube with the house keeping table.
   * @param linerate The linerate for the cube.
   *
   * @history 2011-07-22 Kris Becker
   */
  void RosettaVirtisCamera::readHouseKeeping(const QString &filename,
                                       double lineRate) {
   //  Open the ISIS table object
   Table hktable("VIRTISHouseKeeping", filename);

   m_lineRates.clear();
   int lineno(1);
   NumericalApproximation angFit;
   for (int i = 0; i < hktable.Records(); i++) {
     TableRecord &trec = hktable[i];
     double scet = (double) trec["dataSCET"];
     int shutterMode =  (int) trec["Data Type__Shutter state"];

     // Compute the optical mirror angle
     double mirrorSin = trec["M_MIRROR_SIN_HK"];
     double mirrorCos = trec["M_MIRROR_COS_HK"];
     double scanElecDeg = atan(mirrorSin/mirrorCos) * dpr_c();
     double optAng = ((scanElecDeg - 3.7996979) * 0.25/0.257812);
     optAng /= 1000.0;


     ScanMirrorInfo smInfo;
     double lineMidTime;
     //  scs2e_c(naifSpkCode(), scet.c_str(), &lineMidTime);
     lineMidTime = getClockTime(toString(scet), naifSpkCode()).Et();
     bool isDark = (shutterMode == 1);

     // Add fit data for all open angles
     if ( ! isDark ) {  angFit.AddData(lineno, optAng);   }

#if defined(DUMP_INFO)
      cout << "Line(" << ((isDark) ? "C): " : "O): ") << i
           << ", OptAng(D): " << setprecision(12) << optAng * dpr_c()
           << ", MidExpTime(ET): " <<  lineMidTime
           << "\n";
#endif

      // Store line,
      smInfo.m_lineNum = lineno;
      smInfo.m_scanLineEt = lineMidTime;
      smInfo.m_mirrorSin = mirrorSin;
      smInfo.m_mirrorCos = mirrorCos;
      smInfo.m_opticalAngle = optAng;
      smInfo.m_isDarkCurrent = isDark;

      if ((!m_is1BCalibrated) || (!(m_is1BCalibrated && isDark))) {
        m_lineRates.push_back(LineRateChange(lineno,
                                             lineStartTime(lineMidTime),
                                             exposureTime()));
        m_mirrorData.push_back(smInfo);
        lineno++;
      }
    }

    // Adjust the last time
    LineRateChange  lastR = m_lineRates.back();

    // Normally the line rate changes would store the line scan rate instead of exposure time.
    // Storing the exposure time instead allows for better time calculations within a line.
    // In order for the VariableLineScanCameraDetectorMap to work correctly with this change,
    // every line in the cube must have a LineRateChange object.  This is because determining
    // the start time for one line based on another line requires the line scan rate.  Having
    // a LineRateChange for every line means never needing to calculate the start time for a line
    // because the start time is stored in that line's LineRateChange.  So, the detector map only
    // calculates times within a given line.
    // See VariableLineScanCameraDetectorMap::exposureDuration() for a description of the
    // difference between exposure time and line scan rate.

    m_lineRates.back() = LineRateChange(lastR.GetStartLine(),
                                        lastR.GetStartEt(),
                                        exposureTime());

    //  Run through replacing all closed optical angles with fitted data.
    //  These are mostly first/last lines so must set proper extrapolation
    //  option.
    for (unsigned int a = 0 ; a < m_mirrorData.size() ; a++) {
      if (m_mirrorData[a].m_isDarkCurrent) {
        m_mirrorData[a].m_opticalAngle = angFit.Evaluate(a+1,
                                      NumericalApproximation::NearestEndpoint);
      }
    }

    //  Gut check on housekeeping contents and cube lines
    if ((int) m_lineRates.size() != Lines()) {
      ostringstream mess;
      mess << "Number housekeeping lines determined (" << m_lineRates.size()
           << ") is not equal to image lines(" << Lines() << ")";
      throw IException(IException::Programmer, mess.str(), _FILEINFO_);
    }
  }


  /**
   * @brief Compute the pointing table for each line
   *
   * From the VIR housekeeping data, compute the pointing table for each line
   *  in the image.  This table is for instrumentRotation(Table &) to establish
   *  line/sample pointing information.
   *
   * @internal
   *   @history 2011-07-22 Kris Becker
   */
  Table RosettaVirtisCamera::getPointingTable(const QString &virChannel,
                                        const int zeroFrame)  {

    // Create Spice Pointing table
    TableField q0("J2000Q0", TableField::Double);
    TableField q1("J2000Q1", TableField::Double);
    TableField q2("J2000Q2", TableField::Double);
    TableField q3("J2000Q3", TableField::Double);
    TableField av1("AV1", TableField::Double);
    TableField av2("AV2", TableField::Double);
    TableField av3("AV3", TableField::Double);
    TableField t("ET", TableField::Double);

    TableRecord record;
    record += q0;
    record += q1;
    record += q2;
    record += q3;
    record += av1;
    record += av2;
    record += av3;
    record += t;

    // Get pointing table
    Table quats("SpiceRotation", record);
    int nfields = record.Fields();

    QString virZero = virChannel + "_ZERO";

    // Allocate output arrays
    int nvals = nfields - 1;
    int nlines = m_lineRates.size();

    SpiceDouble eulang[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    SpiceDouble xform[6][6], xform2[6][6];
    SpiceDouble m[3][3];
    SpiceDouble q_av[7], *av(&q_av[4]);

    for (int i = 0 ; i < nlines ; i++) {
      int index = min(i, nlines-1);
      double etTime = m_mirrorData[index].m_scanLineEt;  // mid exposure ET
      double optAng = m_mirrorData[index].m_opticalAngle;
      try {
        // J2000 -> ROS_VIRTIS-M_{channel}_ZERO
        SMatrix state = getStateRotation("J2000", virZero, etTime);

        // Set rotation of optical scan mirror (in radians)
        eulang[1] = -optAng;
        eul2xf_c(eulang, 1, 2, 3, xform);
        mxmg_c(xform, &state[0][0], 6, 6, 6, xform2);

        // Transform to output format
        xf2rav_c(xform2, m, av);  // Transfers AV to output q_av via pointer
        m2q_c(m, q_av);          // Transfers quaternion

        //  Now populate the table record with the line pointing
        for (int k = 0 ; k < nvals ; k++) {
          record[k] = q_av[k];
        }

        // Add time to record; record to table
        record[nvals] = etTime;
        quats += record;
      }
      catch (IException &ie) {
        ostringstream mess;
        mess << "Failed to get point state for line " << i+1;
        throw IException(ie, IException::User, mess.str(), _FILEINFO_);
      }
    }

    // Add some necessary keywords
    quats.Label() += PvlKeyword("CkTableStartTime", toString(startTime()));
    quats.Label() += PvlKeyword("CkTableEndTime", toString(endTime()));
    quats.Label() += PvlKeyword("CkTableOriginalSize", toString(quats.Records()));

    // Create the time dependant frames keyword
    int virZeroId = getInteger("FRAME_" + virZero);
    PvlKeyword tdf("TimeDependentFrames", toString(virZeroId)); // ROS_VIRTIS_M_{ID}_ZERO
    tdf.addValue("-226200");  //  ROS_VIRTIS
    tdf.addValue("-226000");  //  ROSETTA_SPACECRAFT
    tdf.addValue("1");        // J2000
    quats.Label() += tdf;

    //  Create constant rotation frames
    PvlKeyword cf("ConstantFrames", toString(virZeroId));
    cf.addValue(std::to_string(virZeroId));
    quats.Label() += cf;

    SpiceDouble identity[3][3];
    ident_c(identity);

    //  Store DAWN_VIR_{ID}_ZERO -> DAWN_VIR_{ID}_ZERO identity rotation
    PvlKeyword crot("ConstantRotation");
    for (int i = 0 ; i < 3 ; i++) {
      for (int j = 0 ; j < 3 ; j++) {
        crot.addValue(std::to_string(identity[i][j]));
      }
    }

    quats.Label() += crot;

    return (quats);
  }


  /**
   * @brief Compute the state rotation at a given time for given frames
   *
   *  Compute a 6x6 rotation state matrix between the two frames at the
   *  specified time. This method actually computes the complete
   *  pointing rotations at the given time (typically the mid
   *  exposure time).
   *
   *  If acceleration vectors are not present, then only the
   *  rotation properties are retrived from the CK kernels.  The
   *  acceleration vectors are then set to 0.
   *
   * @internal
   *   @history 2011-07-22 Kris Becker
   */

  RosettaVirtisCamera::SMatrix RosettaVirtisCamera::getStateRotation(const QString &frame1,
                                                         const QString &frame2,
                                                         const double &etTime)
                                                         const {
    SMatrix state(6,6);
    NaifStatus::CheckErrors();
    try {
      // Get pointing w/AVs
      sxform_c(frame1.toLatin1().data(), frame2.toLatin1().data(), etTime,
               (SpiceDouble (*)[6]) state[0]);
      NaifStatus::CheckErrors();
    }
    catch (IException &) {
      try {
        SMatrix rot(3,3);
        pxform_c(frame1.toLatin1().data(), frame2.toLatin1().data(), etTime,
                 (SpiceDouble (*)[3]) rot[0]);
        NaifStatus::CheckErrors();
        SpiceDouble av[3] = {0.0, 0.0, 0.0 };
        rav2xf_c((SpiceDouble (*)[3]) rot[0], av,
                 (SpiceDouble (*)[6]) state[0]);
      }
      catch (IException &ie2) {
        ostringstream mess;
        mess << "Could not get state rotation for Frame1 (" << frame1
             << ") to Frame2 (" <<  frame2 <<  ") at time " << etTime;
        throw IException(ie2, IException::User, mess.str(), _FILEINFO_);
      }
    }
    return (state);
  }


  /**
   * @brief determine if the CK articulation kernels are present/given
   *
   *  This method will determine if the CK articulation kernels are present in
   *  the labels.  If a kernel with the file pattern
   *  "ROS_VIRTIS_M_????_????_V?.BC" is present as a CK kernel, then that kernel
   *  contains mirror scan angles for each line.
   *
   *  If the kernel does not exist, this camera model will provide these angles
   *  from the VIR housekeeping data.
   *
   * @internal
   *   @history 2011-07-22 Kris Becker
   */
  bool RosettaVirtisCamera::hasArticulationKernel(Pvl &label) const {
    Kernels kerns(label);
    QStringList cks = kerns.getKernelList("CK");
    QRegExp virCk("*ROS_VIRTIS_M_????_????_V?.BC");
    virCk.setPatternSyntax(QRegExp::Wildcard);
    for (int i = 0 ; i < cks.size() ; i++) {
      if ( virCk.exactMatch(cks[i]) ) return (true);
    }
    return (false);
  }

}

/**
 *  Instantiate a new RosettaVirtisCamera model for the given label content
 */
extern "C" Isis::Camera *RosettaVirtisCameraPlugin(Isis::Cube &cube) {
  return new Isis::RosettaVirtisCamera(cube);
}
