/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "MocLabels.h"

#include <cmath>
#include <cfloat>
#include <iostream>
#include <vector>
#include <fstream>
#include <boost/core/ignore_unused.hpp>

#include "Cube.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "mocxtrack.h"
#include "TextFile.h"
#include "AlphaCube.h"

using namespace std;
namespace Isis {
  /**
   * Construct MocLabels object using the file name
   */
  MocLabels::MocLabels(const QString &file) {
    Cube cube(file, "r");
    Init(cube);
  }

  /**
   * Construct MocLabels object using a Pvl object
   */
  MocLabels::MocLabels(Cube &cube) {
    Init(cube);
  }

  /**
   * General initializer
   * @param lab MOC label for the image
   */
  void MocLabels::Init(Cube &cube) {
    // Initialize gain tables
    InitGainMaps();

    try {
      ReadLabels(cube);
      ValidateLabels();
      Compute();
    }
    catch(IException &e) {
      string msg = "Labels do not appear contain a valid MOC instrument";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
  }
  /**
   * Reads required keywords from the labels
   * @param MOC label for the image
   */
  void MocLabels::ReadLabels(Cube &cube) {
    // Get stuff out of the instrument group
    Pvl &lab = *cube.label();
    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);
    p_instrumentId =  QString::fromStdString(inst["InstrumentId"]);
    p_startingSample = inst["FirstLineSample"];
    p_crosstrackSumming = inst["CrosstrackSumming"];
    p_downtrackSumming = inst["DowntrackSumming"];
    p_exposureDuration = inst["LineExposureDuration"];
    p_focalPlaneTemp = inst["FocalPlaneTemperature"];
    p_clockCount =  QString::fromStdString(inst["SpacecraftClockCount"]);
    p_orbitNumber = 0;
    if(inst.hasKeyword("OrbitNumber")) {
      p_orbitNumber = inst["OrbitNumber"];
    }
    p_gainModeId =  QString::fromStdString(inst["GainModeId"]);
    p_offsetModeId = inst["OffsetModeId"];
    p_startTime =  QString::fromStdString(inst["StartTime"]);

    // Get stuff out of the archive group
    p_dataQuality = "Unknown";
    PvlGroup &arch = lab.findGroup("Archive", Pvl::Traverse);
    if(arch.hasKeyword("DataQualityDesc")) {
      p_dataQuality =  QString::fromStdString(arch["DataQualityDesc"]);
    }

    // Get Stuff out of the band bind group
    PvlGroup &bandBin = lab.findGroup("BandBin", Pvl::Traverse);
    p_filter =  QString::fromStdString(bandBin["FilterName"]);

    // Get the number of samples in the initial cube as it may have been
    // cropped or projected
    AlphaCube a(cube);
    p_ns = a.AlphaSamples();
    p_nl = a.AlphaLines();

    // Get the two kernels for time computations
    PvlGroup &kerns = lab.findGroup("Kernels", Pvl::Traverse);
    p_lsk = FileName( QString::fromStdString(kerns["LeapSecond"][0]));
    p_sclk = FileName( QString::fromStdString(kerns["SpacecraftClock"][0]));
  }

  /**
   * Verifies that the labels are valid
   */
  void MocLabels::ValidateLabels() {
    // Validate the camera type
    p_mocNA = false;
    p_mocRedWA = false;
    p_mocBlueWA = false;

    if(p_instrumentId == "MOC-NA") p_mocNA = true;
    if(p_instrumentId == "MOC-WA") {
      if(p_filter == "RED") p_mocRedWA = true;
      if(p_filter == "BLUE") p_mocBlueWA = true;
    }

    if(!p_mocNA && !p_mocRedWA && !p_mocBlueWA) {
      QString msg = "InstrumentID [" + p_instrumentId + "] and/or FilterName ["
                    + p_filter + "] are inappropriate for the MOC camera";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    // Validate summing modes for narrow angle camera
    if(p_mocNA) {
      if((p_crosstrackSumming < 1) || (p_crosstrackSumming > 8)) {
        string msg = "MOC-NA keyword [CrosstrackSumming] must be between ";
        msg += "1 and 8, but is [" + IString(p_crosstrackSumming) + "]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }

      if((p_downtrackSumming < 1) || (p_downtrackSumming > 8)) {
        string msg = "MOC-NA keyword [DowntrackSumming] must be between ";
        msg += "1 and 8, but is [" + IString(p_downtrackSumming) + "]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
    }

    // Validate summing modes for the wide angle camera
    if((p_mocRedWA) || (p_mocBlueWA)) {
      if((p_crosstrackSumming < 1) || (p_crosstrackSumming > 127)) {
        string msg = "MOC-WA keyword [CrosstrackSumming] must be between ";
        msg += "1 and 127, but is [" + IString(p_crosstrackSumming) + "]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }

      if((p_downtrackSumming < 1) || (p_downtrackSumming > 127)) {
        string msg = "MOC-WA keyword [DowntrackSumming] must be between ";
        msg += "1 and 127, but is [" + IString(p_downtrackSumming) + "]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
    }
  }
  /**
   * Computes some constants
   */
  void MocLabels::Compute() {
    // Compute line rate in seconds
    p_trueLineRate = p_exposureDuration * (double) p_downtrackSumming;
    p_trueLineRate /= 1000.0;

    // Fix the exposure duration for NA images
    if(NarrowAngle() && (p_downtrackSumming != 1)) {
      p_exposureDuration *= p_downtrackSumming;
    }

    // Lookup the gain using the gain mode in the gain maps
    map<QString, double>::iterator p;
    if(NarrowAngle()) {
      p = p_gainMapNA.find(p_gainModeId);
      if(p == p_gainMapNA.end()) {
        QString msg = "Invalid value for PVL keyword GainModeId [" +
                      p_gainModeId + "]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
    }
    else {
      p = p_gainMapWA.find(p_gainModeId);
      if(p == p_gainMapWA.end()) {
        QString msg = "Invalid value for PVL keyword GainModeId [" +
                      p_gainModeId + "]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
    }
    p_gain = p->second;

    // Compute the offset using the offset mode id
    p_offset = p_offsetModeId * 5.0;

    // Ok the gain computation for narrow angle changed from
    // pre-mapping to mapping phase.  Fix it up if necessary
    // (when the Downtrack summing is not 1)
    if(NarrowAngle() && (p_downtrackSumming != 1)) {
      iTime currentTime(p_startTime);
      iTime mappingPhaseBeginTime("1999-04-03T01:00:40.441");
      if(currentTime < mappingPhaseBeginTime) {
        double newGain = p_gain / (double) p_downtrackSumming;
        double mindiff = DBL_MAX;
        map<QString, double>::iterator p;
        QString index = "";
        p = p_gainMapNA.begin();
        while(p != p_gainMapNA.end()) {
          double diff = abs(newGain - p->second);
          if(diff < mindiff) {
            index = p->first;
            mindiff = diff;
          }

          p ++;
        }

        p = p_gainMapNA.find(index);
        if(p == p_gainMapNA.end()) {
          string msg = "Could not find new gain for pre-mapping narrow angle image";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }
        p_gain = p->second;
      }
    }

    // Initialize the maps from sample coordinate to detector coordinates
    InitDetectorMaps();

    // Temporarily load some naif kernels
    QString lsk = p_lsk.expanded();
    QString sclk = p_sclk.expanded();
    furnsh_c(lsk.toLatin1().data());
    furnsh_c(sclk.toLatin1().data());

    // Compute the starting ephemeris time
    scs2e_c(-94, p_clockCount.toLatin1().data(), &p_etStart);
    p_etEnd = EphemerisTime((double)p_nl);

    // Unload the naif kernels
    unload_c(lsk.toLatin1().data());
    unload_c(sclk.toLatin1().data());
  }

  /**
   * Creates a lookup of gain modes to gain values.  These come
   * from the MSSS calibration report.
   */
  void MocLabels::InitGainMaps() {
    p_gainMapNA["F2"] = 1.0;
    p_gainMapNA["D2"] = 1.456;
    p_gainMapNA["B2"] = 2.076;
    p_gainMapNA["92"] = 2.935;
    p_gainMapNA["72"] = 4.150;
    p_gainMapNA["52"] = 5.866;
    p_gainMapNA["32"] = 8.292;
    p_gainMapNA["12"] = 11.73;
    p_gainMapNA["EA"] = 7.968;
    p_gainMapNA["CA"] = 11.673;
    p_gainMapNA["AA"] = 16.542;
    p_gainMapNA["8A"] = 23.386;
    p_gainMapNA["6A"] = 33.067;
    p_gainMapNA["4A"] = 46.740;
    p_gainMapNA["2A"] = 66.071;
    p_gainMapNA["0A"] = 93.465;

    p_gainMapWA["9A"] = 1.0;
    p_gainMapWA["8A"] = 1.412;
    p_gainMapWA["7A"] = 2.002;
    p_gainMapWA["6A"] = 2.832;
    p_gainMapWA["5A"] = 4.006;
    p_gainMapWA["4A"] = 5.666;
    p_gainMapWA["3A"] = 8.014;
    p_gainMapWA["2A"] = 11.34;
    p_gainMapWA["1A"] = 16.03;
    p_gainMapWA["0A"] = 22.67;
    p_gainMapWA["96"] = 16.030;
    p_gainMapWA["86"] = 22.634;
    p_gainMapWA["76"] = 32.092;
    p_gainMapWA["66"] = 45.397;
    p_gainMapWA["56"] = 64.216;
    p_gainMapWA["46"] = 90.826;
    p_gainMapWA["36"] = 128.464;
    p_gainMapWA["26"] = 181.780;
    p_gainMapWA["16"] = 256.961;
    p_gainMapWA["06"] = 363.400;
  };

  /**
   * Converts from sample to starting detector
   * @param sample Sample to be converted
   * @returns @b int Converted start detector
   */
  int MocLabels::StartDetector(int sample) const {
    if((sample < 1) || (sample > p_ns)) {
      string msg = "Out of array bounds in MocLabels::StartDetector";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    return p_startDetector[sample-1];
  }
  /**
   * Converts from sample to ending detector
   * @param sample Sample to be converted
   * @returns @b int Converted ending detector
   */
  int MocLabels::EndDetector(int sample) const {
    if((sample < 1) || (sample > p_ns)) {
      string msg = "Out of array bounds in MocLabels::EndDetector";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    return p_endDetector[sample-1];
  }
  /**
   * Converts from detector to sample
   * @param detector Detector to be converted
   * @returns @b double Converted sample
   */
  double MocLabels::Sample(int detector) const {
    if((detector < 0) || (detector >= Detectors())) {
      string msg = "Out of array bounds in MocLabels::Sample";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    return p_sample[detector];
  }
  /**
   * Creates lookup table from sample to detectors and vice versa.
   */
  void MocLabels::InitDetectorMaps() {
    // Create sample to detector maps
    if(p_crosstrackSumming == 13) {
      for(int i = 0; i < p_ns; i++) {
        p_startDetector[i] = mode13_table[i].starting_pixel +
                             p_startingSample - 1;
        p_endDetector[i] = mode13_table[i].ending_pixel +
                           p_startingSample - 1;
      }
    }
    else if(p_crosstrackSumming == 27) {
      for(int i = 0; i < p_ns; i++) {
        p_startDetector[i] = mode27_table[i].starting_pixel +
                             p_startingSample - 1;
        p_endDetector[i] = mode27_table[i].ending_pixel +
                           p_startingSample - 1;
      }
    }
    else {
      int detector = (p_startingSample - 1);
      for(int i = 0; i < p_ns; i++) {
        p_startDetector[i] = detector;
        detector += p_crosstrackSumming - 1;
        p_endDetector[i] = detector;
        detector++;
      }
    }

    // Now create a detector to sample map
    // Start by setting each position to a invalid sample
    for(int det = 0; det < Detectors(); det++) {
      p_sample[det] = -1.0;
    }

    for(int samp = 1; samp <= p_ns; samp++) {
      int sd = p_startDetector[samp-1];
      int ed = p_endDetector[samp-1];

      double m = ((samp + 0.5) - (samp - 0.5)) / ((ed + 0.5) - (sd - 0.5));
      for(int det = sd; det <= ed; det++) {
        p_sample[det] = m * (det - (sd - 0.5)) + (samp - 0.5);
      }
    }
  }
  /**
   * Returns the ephemeris time at the given line.
   * @param line Line to evaluate
   * @returns @b double Ephemeris time
   */
  double MocLabels::EphemerisTime(double line) const {
    return p_etStart + (line - 0.5) * p_trueLineRate;
  }
  /**
   * Returns the true gain at a given line.
   * @param line Line to evaluate
   * @returns @b double True gain
   */
  // return gain at a line
  double MocLabels::Gain(int line) {
    if(NarrowAngle()) return p_gain;

    InitWago();

    double etLine = EphemerisTime((double)line);
    for(int i = (int)p_wagos.size() - 1; i >= 0; i--) {
      if(etLine >= p_wagos[i].et) {
        return p_wagos[i].gain;
      }
    }

    return p_gain;
  }
  /**
   * Returns the offset at the given line.
   * @param line Line to evaluate
   * @returns @b double Offset
   */
  double MocLabels::Offset(int line) {
    if(NarrowAngle()) return p_offset;
    InitWago();

    double etLine = EphemerisTime((double)line);
    for(int i = (int)p_wagos.size() - 1; i >= 0; i--) {
      if(etLine >= p_wagos[i].et) {
        return p_wagos[i].offset;
      }
    }
    return p_offset;
  }
  /**
   * Reads the wide-angle gain/offset table and internalizes
   * @internal
   *   @history 2010-01-05 Jeannie Walldren - Fixed bug that
   *                                          passed sclkKern
   *                                          filename into
   *                                          scs2e_c instead of
   *                                          the sclk string.
   */
  void MocLabels::InitWago() {
    // Only do this once
    static bool firstTime = true;
    if(!firstTime) return;
    firstTime = false;

    // Load naif kernels
    QString lskKern = p_lsk.expanded();
    QString sclkKern = p_sclk.expanded();
    furnsh_c(lskKern.toLatin1().data());
    furnsh_c(sclkKern.toLatin1().data());

    //Set up file for reading
    FileName wagoFile("$mgs/calibration/MGSC_????_wago.tab");
    wagoFile = wagoFile.highestVersion();
    QString nameOfFile = wagoFile.expanded();
    ifstream temp(nameOfFile.toLatin1().data());
    vector<int> wholeFile;

    // Read file into a vector of bytes, ignoring EOL chars
    while(temp.good()) {
      int nextByte = temp.get();
      if(nextByte != 10 && nextByte != 13) {
        wholeFile.push_back(nextByte);
      }
    }
    temp.close();

    //Set up to binary search for the desired time
    int low = 1;
    int high = wholeFile.size() / 35;
    int middle;
    IString line, filter, sclk, offsetId;
    QString gainId;
    WAGO wago;

    //Binary search. This determines the middle of the current range and
    //moves through the file until it reaches that line, at which point it
    //analyzes to see if the time is within the set limits.
    while(low <= high) {
      middle = (low + high) / 2;
      int SclkStart = middle * 35 + 8;
      int SclkEnd = SclkStart + 15;
      string currentSclk;

      //Build sclk string and convert to an actual time
      for(int i = SclkStart; i < SclkEnd; i++) {
        currentSclk += (char)wholeFile[i];
      }
      sclk = currentSclk;
      sclk.Remove("\"");
      sclk.Trim(" ");
      double et;
      scs2e_c(-94, currentSclk.c_str(), &et);

      //Compare time against given parameters, if it fits, process
      if(et < p_etEnd && et > p_etStart) {
        int linenum = middle;
        int top = middle;
        int bottom = middle;

        //First, find the highest line that will meet requirements
        while(et >= p_etStart) {
          linenum--;
          int lineStart = (linenum * 35);
          int lineEnd = lineStart + 35;

          string currentLine = "";
          for(int i = lineStart; i < lineEnd; i++) {
            currentLine += (char)wholeFile[i];
          }
          line = currentLine;

          currentSclk = "";
          for(int i = 8; i < 23; ++i) {
            currentSclk += currentLine[i];
          }
          sclk = currentSclk;
          sclk.Trim(" ");
          scs2e_c(-94, currentSclk.c_str(), &et);

          bottom = linenum;
        }
        //Next, find the lowest line to meet requirements
        while(et <= p_etEnd) {
          linenum++;
          int lineStart = (linenum * 35);
          int lineEnd = lineStart + 35;

          string currentLine = "";
          for(int i = lineStart; i < lineEnd; i++) {
            currentLine += (char)wholeFile[i];
          }
          line = currentLine;

          currentSclk = "";
          for(int i = 8; i < 23; ++i) {
            currentSclk += currentLine[i];
          }
          sclk = currentSclk;
          sclk.Trim(" ");
          scs2e_c(-94, currentSclk.c_str(), &et);
          top = linenum;
        }
        //Now, go from the upper limit to the lower limit, and grab all lines
        //that meet requirements
        for(int i = bottom; i <= top; ++i) {
          int lineStart = (i * 35);
          int lineEnd = lineStart + 35;
          string currentLine = "";
          for(int j = lineStart; j < lineEnd; j++) {
            currentLine += (char)wholeFile[j];
          }
          line = currentLine;

          // Get the filter color (red or blue)
          filter = line.Token(",");
          filter.Remove("\"");
          filter.Trim(" ");

          // If it's not the filter we want then skip to loop end
          if((filter == "RED")  && (WideAngleBlue())) continue;
          if((filter == "BLUE") && (WideAngleRed()))  continue;

          // Get the sclk and convert to et
          sclk = line.Token(",");
          sclk.Remove("\"");
          sclk.Trim(" ");

          scs2e_c(-94, sclk.c_str(), &et);

          // Get the gain mode id
          gainId = line.Token(",").ToQt().remove("\"").trimmed();

          // Get the offset mode id
          offsetId = line;
          offsetId.Remove("\"");
          offsetId.ConvertWhiteSpace();
          offsetId.Trim(" ");

          // Compute the gain
          map<QString, double>::iterator p;
          p = p_gainMapWA.find(gainId);
          if(p == p_gainMapWA.end()) {
            // Unload the naif kernels
            unload_c(lskKern.toLatin1().data());
            unload_c(sclkKern.toLatin1().data());

            QString msg = "Invalid GainModeId [" + gainId + "] in wago table";
            throw IException(IException::Unknown, msg, _FILEINFO_);
          }
          double gain = p->second;

          // Compute the offset
          double offset = offsetId.ToDouble() * 5.0;

          // Push everything onto a stack
          wago.et = et;
          wago.gain = gain;
          wago.offset = offset;
          p_wagos.push_back(wago);

        }

        low = high++;
      }
      //If we're too high, search beginning of array
      else if(et < p_etStart) {
        low = middle + 1;
      }
      //If we're too low, search end of array
      else {
        high = middle - 1;
      }
    }

    // Ok sort and unique the wago list by time
    sort(p_wagos.begin(), p_wagos.end());
    boost::ignore_unused((unique(p_wagos.begin(), p_wagos.end())));

    // Unload the naif kernels
    unload_c(lskKern.toLatin1().data());
    unload_c(sclkKern.toLatin1().data());
  }
}
