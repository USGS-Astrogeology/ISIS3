/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QDir>
#include <algorithm>
#include <cmath>  //use ceiling and floor functions
#include <vector>
#include "Application.h"
#include "Brick.h"
#include "CisscalFile.h"
#include "CissLabels.h"
#include "Cube.h"
#include "DarkCurrent.h"
#include "NumericalApproximation.h"
#include "Message.h"
#include "Preference.h"
#include "Progress.h"
#include "Pvl.h"
#include "SpecialPixel.h"
#include "IException.h"
#include "IString.h"

using namespace std;
namespace Isis {
  /**
  * Constructs a DarkCurrent object.  Sets class variables.
  *
  * @param cissLab <B>CissLabels</B> object from Cassini ISS cube
  * @throws Isis::iException::Pvl If the input image has an
  *                   invalid InstrumentDataRate or SummingMode.
  * @internal
  *   @history 2008-11-05 Jeannie Walldren - Original Version
  *   @history 2009-05-27 Jeannie Walldren - Updated instrument data rate range
  *                          for telemetry rate of 32.
  */
  DarkCurrent::DarkCurrent(Isis::CissLabels &cissLab) {
    p_compType = cissLab.CompressionType();
    p_dataConvType = cissLab.DataConversionType();
    p_expDur = cissLab.ExposureDuration();
    p_flightSoftware = cissLab.FlightSoftwareVersion();
    p_gainMode = cissLab.GainModeId();
    p_narrow = cissLab.NarrowAngle();
    p_sum = QString::number(cissLab.SummingMode());
    p_imageTime = cissLab.ImageTime();

    if(cissLab.ReadoutCycleIndex() == "Unknown") {
      p_readoutIndex = -999;
    }
    else {
      p_readoutIndex = cissLab.ReadoutCycleIndex().toInt();
    }

    if(p_compType == "NotCompressed") {
      p_compRatio = 1.0;
    }
    else {
      p_compRatio = cissLab.CompressionRatio().toDouble();
    }

    if(cissLab.DelayedReadoutFlag() == "No") {
      p_btsm = 0;
    }
    else if(cissLab.DelayedReadoutFlag() == "Yes") {
      p_btsm = 1;
    }
    else {
      p_btsm = -1;
    }

    double instDataRate = cissLab.InstrumentDataRate();
    if(instDataRate >=  60.0 && instDataRate <=  61.0) p_telemetryRate = 8;
    else if(instDataRate >= 121.0 && instDataRate <= 122.0) p_telemetryRate = 16;
    else if(instDataRate >= 182.0 && instDataRate <= 183.0) p_telemetryRate = 24;
    else if(instDataRate >= 203.0 && instDataRate <= 204.0) p_telemetryRate = 32;
    else if(instDataRate >= 304.0 && instDataRate <= 305.0) p_telemetryRate = 40;
    else if(instDataRate >= 365.0 && instDataRate <= 366.0) p_telemetryRate = 48;
    else throw IException(IException::Unknown,
                          "Input file contains invalid InstrumentDataRate. See Software Interface Specification (SIS), Version 1.1, page 31.",
                          _FILEINFO_);

    p_readoutOrder = cissLab.ReadoutOrder();

    switch(p_sum.toInt()) {
      case 1:
        p_lines   = 1024;
        break;
      case 2:
        p_lines   = 512;
        break;
      case 4:
        p_lines   = 256;
        break;
      default:
        throw IException(IException::Unknown,
                         "Input file contains invalid SummingMode. See Software Interface Specification (SIS), Version 1.1, page 31.",
                         _FILEINFO_);
    }
    p_samples = p_lines;
    p_startTime.resize(p_samples);
    p_endTime.resize(p_samples);
    p_duration.resize(p_samples);
    for(int i = 0; i < p_samples; i++) {
      p_startTime[i].resize(p_lines);
      p_endTime[i].resize(p_lines);
      p_duration[i].resize(p_lines);
    }
  }//end constructor


  /**
  * @brief Compute dark current values to subtract from image
  *
  * This method computes the dark current DN values to be
  * subtracted from each pixel and returns those values in an
  * array of the equal dimension to the image.
  *
  * @returns <b>vector \<vector \<double\> \></b> Final array of
  *         dark current DNs to be subtracted
  * @throws Isis::IException::Pvl If the input image has an
  *             unknown ReadoutCycleIndex or DelayedReadoutFlag.
  * @throws Isis::IException::Pvl If the input image has an
  *             invalid GainModeId.
  * @throws Isis::IException::Math If MakeDarkArray() returns a
  *             vector of zeros.
  * @see MakeDarkArray()
  *
  * @internal
  *   @history 2008-11-05 Jeannie Walldren - Original Version
  *   @history 2009-01-26 Jeannie Walldren - Changed declarations of 2
  *                          dimensional vectors
  */
  vector <vector <double> > DarkCurrent::ComputeDarkDN() { //get dark_DN
    if(p_readoutIndex == -999) {
      throw IException(IException::Unknown,
                                "Readout cycle index is unknown.",
                                _FILEINFO_);
    }
    if(p_btsm == -1) {
      throw IException(IException::Unknown,
                       "Delayed readout flag is unknown.",
                       _FILEINFO_);
    }
    vector <vector <double> > dark_e(p_samples), dark_DN(p_samples);
    for(unsigned int i = 0; i < dark_e.size(); i++) {
      dark_e[i].resize(p_lines);
      dark_DN[i].resize(p_lines);
    }

    // create new file
    dark_e = DarkCurrent::MakeDarkArray();
    int notzero = 0;
    for(unsigned int i = 0; i < dark_e.size(); i++) {
      for(unsigned int j = 0; j < dark_e[0].size(); j++) {
        if(dark_e[i][j] != 0.0) {
          notzero++;
        }
      }
    }
    if(notzero != 0) {
      double DNSatPoint;
      if (p_dataConvType == "8Bit") {
        DNSatPoint = 255.0;
      }
      else {
        DNSatPoint = 4095.0;
      }
      // correct for gain:
      double gain2, gainRatio;
      int ESatPoint;
      if(p_narrow) {
        ESatPoint = 1.237e5;
        gain2 = 30.27;
        switch(p_gainMode) { // GainState()){
          case 215: //gs = 0:
            gainRatio = 0.135386;
            break;
          case 95: //1:
            gainRatio = 0.309569;
            break;
          case 29: //2:
            gainRatio = 1.0;
            break;
          case 12: //3:
            gainRatio = 2.357285;
            break;
          default:
            throw IException(IException::Unknown,
                             "Input file contains invalid GainModeId. See Software Interface Specification (SIS), Version 1.1, page 29.",
                             _FILEINFO_);
        }
      }
      else {
        ESatPoint = 1.133e5;
        gain2 = 27.68;
        switch(p_gainMode) { // GainState()){
          case 215://0:
            gainRatio = 0.125446;
            break;
          case 95://1:
            gainRatio = 0.290637;
            break;
          case 29://2:
            gainRatio = 1.0;
            break;
          case 12://3:
            gainRatio = 2.360374;
            break;
          default:
            throw IException(IException::Unknown,
                             "Input file contains invalid GainModeId. See Software Interface Specification (SIS), Version 1.1, page 29.",
                             _FILEINFO_);
        }
      }
      for(unsigned int i = 0; i < dark_e.size(); i++) {
        for(unsigned int j = 0; j < dark_e[0].size(); j++) {
          dark_DN[i][j] = (dark_e[i][j] / (gain2 / gainRatio));
          if (dark_DN[i][j] >= DNSatPoint || dark_e[i][j] >= ESatPoint){
            dark_DN[i][j] = DNSatPoint;
          }
        }
      }
      return dark_DN;
    }
    else {
      throw IException(IException::Unknown,
                       "Error in dark simulation; dark array conatains all zeros.",
                       _FILEINFO_);
    }
  }//end ComputeDarkDN


  /**
  * @brief Compute time spent on CCD for given line
  *
  * Compute the line-time from light-flood erase to read out for a
  * given image line.  Returns the real seconds that the given
  * line spends on the CCD. The parameter <B>lline</B> must be
  * between 1 and 1024.
  *
  * @param lline Current line should range from 1 to 1024
  * @returns <b>double</b> Time in seconds spent on CCD for given
  *        line
  * @throws Isis::iException::Programmer If the input parameter is
  *             out of valid range.
  * @throws Isis::iException::Pvl If the input image contains an
  *             invalid number of lines.
  * @throws Isis::iException::Pvl If the input image has an
  *             invalid ReadoutCycleIndex.
  * @internal
  *   @history 2008-11-05 Jeannie Walldren - Original Version
  *   @history 2009-05-27 Jeannie Walldren - Updated with new idl cisscal
  *                          version, 3.6, linetime code.
  */
  double DarkCurrent::ComputeLineTime(int lline) { //returns the time for this line, takes the line number
    // this method mimics linetime.pro from idl's cisscal program
    double fsw;
    if(p_flightSoftware == "Unknown") {
      fsw = 0.0;
    }
    else fsw = p_flightSoftware.toDouble();

    double linetime;
    double tlm = p_telemetryRate / 8;
    // updated idl code - change t0 initial value:
    double t0 = p_expDur / 1000 + 0.020;    //time from erase to first line
    t0 = t0 + 0.68 * (p_lines - lline) / ((double) p_lines);
    //time due to 680ms erase
    int line = lline - 1;       //lline is from 1 to 1024 => line is from 0 to 1023
    if(line < 0 || line > 1023) {
      IString msg = "DarkCurrent: For ComputeLineTime(lline), lline must be between 1 and 1024." + IString(lline) + " out of range";
      throw IException(IException::Programmer, msg.c_str() , _FILEINFO_);
    }

    double r1 = 0;
    if(p_compType == "Lossy") {
      switch(p_lines) {
        case 256:
          r1 = 89.754;
          break;
        case 512:
          r1 = 110.131;
          break;
        case 1024:
          r1 = 201.734;
          break;
        default:
          throw IException(IException::Unknown,
                           "Input file contains invalid number of lines. See Software Interface Specification (SIS), Version 1.1, page 50.",
                           _FILEINFO_);
      }
      double linetime = t0 + line / r1;
      return linetime;
    }
    double data;
    if(p_dataConvType == "12Bit") {
      data = 16.0;
    }
    else {
      data = 8.0;
    }

    double correction = 1.0;
    // Telemetry rate factors (0,8,16,24,32,40,48 pps)
    // The fastest science packet production rate is 48 packets per second.
    // When the camera is creating less packets per second, it can have more
    // time to service the CCD.  This can lead to a faster readout.  This
    // effect is mostly seen in full or non compressed modes.

    double r0;
    if(p_compType == "NotCompressed") {
      //  Non-compressed modes
      //  The following non-compressed rates were measured in ITL tests at 48
      //  packets per second.  Rates are specified in lines per second.  The
      //  values are for full, sum2 and sum4 modes.  Flight software timing
      //  is accurate to 5ms, so rates are specified to 2 decimal places.

      //  Ratio of rate for FSW 1.4 vs 1.3 (EGSE tests at 48 and 24pps)

      double rate_nc = 0;
      double telem_nc = 0;
      double telem_nc0 = 0;
      if(p_dataConvType == "12Bit") { // not converted
        switch(p_lines) {
          case 1024: {  // full, not converted
            rate_nc = 67.49;
            if(fsw >= 1.4) {
              correction = 1.0027;
            }
            telem_nc0 = 1.0161;
            switch(p_telemetryRate) {
              case 8: {
                telem_nc = 1.0128;
                break;
              }
              case 16: {
                telem_nc = 1.0095;
                break;
              }
              case 24: {
                telem_nc = 1.0082;
                break;
              }
              case 32: {
                telem_nc = 1.0031;
                break;
              }
              case 40: {
                telem_nc = 1.0033;
                break;
              }
              case 48: {
                telem_nc = 1.0;
                break;
              }
            }
            break;
          }
          case 512: {  // sum2, not converted
            rate_nc = 85.11;
            if(fsw >= 1.4) {
              correction = 1.0073;
            }
            telem_nc0 =  1.0297;
            switch(p_telemetryRate) {
              case 8: {
                telem_nc = 1.0296;
                break;
              }
              case 16: {
                telem_nc = 1.0252;
                break;
              }
              case 24: {
                telem_nc = 1.0148;
                break;
              }
              case 32: {
                telem_nc = 1.0114;
                break;
              }
              case 40: {
                telem_nc = 1.0071;
                break;
              }
              case 48: {
                telem_nc = 1.0;
                break;
              }
            }
            break;
          }
          case 256: {  // sum4, not converted
            rate_nc = 142.54;
            if(fsw >= 1.4) {
              correction = 1.0087;
            }
            telem_nc0 = 1.0356;
            switch(p_telemetryRate) {
              case 8: {
                telem_nc = 1.0320;
                break;
              }
              case 16: {
                telem_nc = 1.0260;
                break;
              }
              case 24: {
                telem_nc = 1.0201;
                break;
              }
              case 32: {
                telem_nc = 1.0128;
                break;
              }
              case 40: {
                telem_nc = 1.0057;
                break;
              }
              case 48: {
                telem_nc = 1.0;
                break;
              }
            }
            break;
          }
        }
      }
      else { // converted
        switch(p_lines) {
          case 1024: {  // full, converted
            rate_nc = 71.96;
            if(fsw >= 1.4) {
              correction = 1.0016;
            }
            telem_nc0 = 1.0194;
            switch(p_telemetryRate) {
              case 8: {
                telem_nc = 1.0148;
                break;
              }
              case 16: {
                telem_nc = 1.0028;
                break;
              }
              case 24: {
                telem_nc = 1.0011;
                break;
              }
              case 32: {
                telem_nc = 1.0014;
                break;
              }
              case 40: {
                telem_nc = 1.0009;
                break;
              }
              case 48: {
                telem_nc = 1.0;
                break;
              }
            }
            break;
          }
          case 512: {  // sum2, converted
            rate_nc = 88.99;
            if(fsw >= 1.4) {
              correction = 1.0042;
            }
            telem_nc0 = 1.0248;
            switch(p_telemetryRate) {
              case 8: {
                telem_nc = 1.0219;
                break;
              }
              case 16: {
                telem_nc = 1.0173;
                break;
              }
              case 24: {
                telem_nc = 1.0151;
                break;
              }
              case 32: {
                telem_nc = 1.0097;
                break;
              }
              case 40: {
                telem_nc = 1.0057;
                break;
              }
              case 48: {
                telem_nc = 1.0;
                break;
              }
            }
            break;
          }
          case 256: {  // sum4, converted
            rate_nc = 152.12;
            if(fsw >= 1.4) {
              correction = 0.9946;
            }
            telem_nc0 = 1.0010;
            switch(p_telemetryRate) {
              case 8: {
                telem_nc = 1.0000;
                break;
              }
              case 16: {
                telem_nc = 0.9970;
                break;
              }
              case 24: {
                telem_nc = 0.9910;
                break;
              }
              case 32: {
                telem_nc = 0.9821;
                break;
              }
              case 40: {
                telem_nc = 0.9763;
                break;
              }
              case 48: {
                telem_nc = 1.0;
                break;
              }
            }
            break;
          }
        }
      }
      r1 = rate_nc * telem_nc * correction;
      r0 = rate_nc * telem_nc0 * correction;
    }
    else { // Lossy has already returned linetime, so (p_compType == "Lossless")
      // Lossless linear model
      // The following are least square fits for Lossless modes at 48pps.  There is
      // a fit for each summation mode and converted and not converted (12bit).

      //   RMS of fit  0.255   0.076   0.496    not converted
      //   RMS of fit  0.172   0.162   0.429    converted

      //  Ratio of rate for FSW 1.4 vs 1.3 (EGSE tests at 48 and 24pps)

      double rate0 = 0;
      double slope = 0;
      double telem_L = 0;
      double telem_L0 = 0;

      if(p_dataConvType == "12Bit") { // not converted
        switch(p_lines) {
          case 1024: {  // full, not converted
            rate0 = 67.673;
            slope = 1.6972; // +/- 0.0102

            if(fsw >= 1.4) {
              correction = 0.9999;
            }
            telem_L0 = 1.0276;
            switch(p_telemetryRate) {
              case 8: {
                telem_L = 1.0284;
                break;
              }
              case 16: {
                telem_L = 1.0182;
                break;
              }
              case 24: {
                telem_L = 1.0122;
                break;
              }
              case 32: {
                telem_L = 1.0048;
                break;
              }
              case 40: {
                telem_L = 1.0016;
                break;
              }
              case 48: {
                telem_L = 1.0;
                break;
              }
            }
            break;
          }
          case 512: {  // sum2, not converted
            rate0 = 90.568;
            slope = 0.3671; // +/- 0.0255

            if(fsw >= 1.4) {
              correction = 1.0034;
            }
            telem_L0 = 1.0030;
            switch(p_telemetryRate) {
              case 8: {
                telem_L = 0.9979;
                break;
              }
              case 16: {
                telem_L = 0.9933;
                break;
              }
              case 24: {
                telem_L = 0.9854;
                break;
              }
              case 32: {
                telem_L = 0.9884;
                break;
              }
              case 40: {
                telem_L = 1.0023;
                break;
              }
              case 48: {
                telem_L = 1.0;
                break;
              }
            }
            break;
          }
          case 256: {  // sum4, not converted
            rate0 = 150.593;
            slope = 0.4541;  // +/-0.0450

            if(fsw >= 1.4) {
              correction = 1.0073;
            }
            telem_L0 = 1.0011;
            switch(p_telemetryRate) {
              case 8: {
                telem_L = 0.9976;
                break;
              }
              case 16: {
                telem_L = 0.9894;
                break;
              }
              case 24: {
                telem_L = 0.9864;
                break;
              }
              case 32: {
                telem_L = 1.0000;
                break;
              }
              case 40: {
                telem_L = 1.0000;
                break;
              }
              case 48: {
                telem_L = 1.0;
                break;
              }
            }
            break;
          }
        }
      }
      else { // converted
        switch(p_lines) {
          case 1024: {  // full, converted
            rate0 = 74.862;
            slope = 0.4918; // +/- 0.0069
            if(fsw >= 1.4) {
              correction = 1.0019;
            }
            telem_L0 = 1.0013;
            switch(p_telemetryRate) {
              case 8: {
                telem_L = 1.0004;
                break;
              }
              case 16: {
                telem_L = 0.9935;
                break;
              }
              case 24: {
                telem_L = 0.9920;
                break;
              }
              case 32: {
                telem_L = 1.0002;
                break;
              }
              case 40: {
                telem_L = 0.9992;
                break;
              }
              case 48: {
                telem_L = 1.0;
                break;
              }
            }
            break;
          }
          case 512: {  // sum2, converted
            rate0 = 91.429;
            slope = 0.4411; // +/- 0.0182
            if(fsw >= 1.4) {
              correction = 1.0050;
            }
            telem_L0 = 1.0013;
            switch(p_telemetryRate) {
              case 8: {
                telem_L = 0.9950;
                break;
              }
              case 16: {
                telem_L = 1.0000;
                break;
              }
              case 24: {
                telem_L = 1.0000;
                break;
              }
              case 32: {
                telem_L = 1.0000;
                break;
              }
              case 40: {
                telem_L = 1.0001;
                break;
              }
              case 48: {
                telem_L = 1.0;
                break;
              }
            }
            break;
          }
          case 256: {  // sum4, converted
            rate0 = 152.350;
            slope = 0.5417; // +/-  0.0697
            if(fsw >= 1.4) {
              correction = 1.0080;
            }
            telem_L0 = 0.9986;
            switch(p_telemetryRate) {
              case 8: {
                telem_L = 0.9863;
                break;
              }
              case 16: {
                telem_L = 1.0017;
                break;
              }
              case 24: {
                telem_L = 1.0021;
                break;
              }
              case 32: {
                telem_L = 1.0010;
                break;
              }
              case 40: {
                telem_L = 1.0017;
                break;
              }
              case 48: {
                telem_L = 1.0;
                break;
              }
            }
            break;
          }
        }
      }
      double ro_ratefit = rate0 + slope * p_compRatio;
      r1 = ro_ratefit * telem_L * correction;
      r0 = ro_ratefit * telem_L0 * correction;
    }

    // Calculation of BIU swap line which occurs upon completion of first packet
    // If one or more complete lines can fit into the first packet of 440 words,
    // they are moved from the Image Buffer allowing more to be read from the CCD
    // before the BIU pause.
    // Note: must also account for 4-word line header on each line
    double tratio = p_compRatio;
    if(p_compType == "Lossless" && tratio < 2.0) {
      tratio = 2.0;
    }
    int fpacket = 440 / (4 + ((int)(p_lines * data / 16 / tratio)));
    int biu_line = fpacket + 1;

    // if camera is opposite of read_out_order (second)
    // Calculate number of lines read in early pad of 0.262 seconds
    // BIU swap occurs after these number of lines or when
    // first science packet is complete (at biu_line) which
    // ever is greater
    bool second = false;

    if(p_narrow && p_readoutOrder == 1) {
      second = true;
    }
    else if(!p_narrow && p_readoutOrder == 0) {
      second = true;
    }

    // First line after biu wait is at 0.289 seconds
    double biutime = 0.289;
    int early_lines = 1;
    if(second && p_btsm == 0) {
      early_lines  = ((int)(0.262 * r0)) + 1;
      if(early_lines > biu_line) {
        biu_line = early_lines;
      }
      // If there is 0.262 pad before readout window (i.e. second image)
      // then biu swap occurs 2 rti later (0.25 sec)
      biutime = 0.539;
    }

    double rate;
    if(p_lines < 1024) {
      rate = r1;
      if(p_btsm == 1) {
        rate = r0;  // No science packet rate
      }
      if(p_btsm == 0 && line >= biu_line && fsw < 1.4) {
        linetime = t0 + biutime + (line - biu_line) / rate;
      }
      else {
        linetime = t0 + line / rate;
      }
      return linetime;
    }
    // Only FULL images can fill image buffer and cause r2 rate
    double r2 = 3.5989 * tlm; // ITL measured
    if(p_dataConvType != "12Bit") {
      r2 = 7.1989 * tlm; // ITL measured
    }
    // For Lossless, r2 depends on compression ratio: but not faster than r1.

    if(p_compType == "Lossless") {
      // r2 = cdsr * lines per packet
      // lines per packet = data words per packet / data words per line
      // data words per packet = (440 * 2% + 467 * 98%) - 4 (line header) = 462.46
      // data words per line = 4 (line header) + (1024 or 512)/tratio
      r2 = p_telemetryRate * 462.46 / (4.0 + 1024.0 * data / 16.0 / tratio);
    }
    if(r1 < r2) {
      r2 = r1;
    }
    // Due to bug, FSW < 1.4 did not use 4K words of image buffer
    int buffer;
    if(fsw < 1.4) {
      buffer = 336;
    }
    else {
      buffer = 340;
    }
    if(p_dataConvType != "12Bit") {
      buffer = 2 * buffer;
    }
    // Stores 2 compressed lines into one
    if(p_compType == "Lossless") {
      buffer = 2 * buffer;
    }
    // Due to bug, FSW < 1.4 declared image buffer full with one free line
    if(fsw < 1.4) {
      buffer = buffer - 1;
    }
    // Now treat more complicated 1x1 case.
    int line_break;
    if(p_btsm == 0) {
      int inbuffer;
      if(fsw >= 1.4) {
        // Calculate line break
        // Transmit starts at biutime after readout starts
        // after early_lines initially read before biutime
        // buffer has buffer-inbuffer left to fill
        early_lines = ((int)(biutime * r0)) + 1;
        if(early_lines > fpacket) {
          inbuffer = early_lines - fpacket;
        }
        else inbuffer = 0;
        if(r2 >= r1) {
          line_break = 1024;
        }
        else {
          line_break = early_lines + ((int)(r1 * (buffer - inbuffer) / (r1 - r2))) + 1;
        }
        linetime = t0 + line / r1;
        if(line > line_break) {
          linetime = t0 + line_break / r1 + (line - line_break) / r2;
        }
      }
      else {
        // Calculate line break
        // Transmit starts and readout resumes at biutime
        // after biu_lines initially read before biutime
        // fpacket lines in 1st packet, max(early_lines-fpacket,0) in buffer
        if(early_lines > fpacket) {
          inbuffer = early_lines - fpacket;
        }
        else {
          inbuffer = 0;
        }

        if(r2 >= r1) {
          line_break = 1024;
        }
        else {
          line_break = biu_line + ((int)(r1 * (buffer - inbuffer) / (r1 - r2))) + 1;
        }
        linetime = t0 + line / r1;
        if(line >= biu_line && line <= line_break) {
          linetime = t0 + biutime + (line - biu_line) / r1;
        }
        if(line > line_break) {
          linetime = t0 + biutime + (line_break - biu_line) / r1 + (line - line_break) / r2;
        }
      }
    }
    else { // p_btsm == 1
      // t1 is amount of time botsim image waits for first image readout window
      // t1 only depends on readout index and telem rate:
      // t1 is first camera readout window plus pad plus biu swap
      int readout;
      switch((int) p_readoutIndex / 4) {
        case 0:
          readout = 50;
          break;
        case 1:
          readout = 25;
          break;
        case 2:
          readout = 14;
          break;
        case 3:
          readout = 6;
          break;
        default:
          throw IException(IException::Unknown,
                           "Input file contains invalid ReadoutCycleIndex. See Software Interface Specification (SIS), Version 1.1, page 40.",
                           _FILEINFO_);
      }
      double t1;
      if(readout * (6.0 / ((double)(tlm))) - ((int) readout * (6.0 / ((double)(tlm)))) < .5) {
        t1 = floor(readout * (6.0 / ((double)(tlm)))) + 0.539;
      }
      else {
        t1 = ceil(readout * (6.0 / ((double)(tlm)))) + 0.539;
      }
      linetime = t0 + line / r0;
      int line_break = buffer  + fpacket + 1; // Full buffer
      // NotCompressed 12Bit always stops and waits when buffer filled
      if(p_dataConvType == "12Bit" && p_compType == "NotCompressed") {
        if(line >= line_break) {
          linetime = t0 + t1 + (line - line_break) / r2;
        }
        return linetime;
      }
      // Line at which transmission starts
      // Reading stops during BIU swap (0.25 sec) for FSW < 1.4
      int trans_line;
      double biu_swap;
      if(fsw < 1.4) {
        trans_line = ((int)((t1 - 0.25) * r0)) + 1;
        biu_swap = 0.25;
      }
      else {
        trans_line = ((int)(t1 * r0)) + 1;
        biu_swap = 0.0;
      }
      // NOTCOMP TABLE/8LSB may start reading out before buffer is filled
      // LOSSLESS 12BIT may start reading out before buffer is filled
      // If buffer is filled first, rest is read out at r2
      // If t0+t1 occurs first then read continues at r1 until filled, then at r2
      if((p_dataConvType != "12Bit" && p_compType == "NotCompressed")
          || (p_dataConvType == "12Bit" && p_compType == "Lossless")) {
        if(trans_line >= line_break) {
          if(line >= line_break) {
            linetime = t0 + t1 + (line - line_break) / r2; // waits
          }
        }
        else {
          if(r2 >= r1) {
            line_break = 1024;
          }
          else {
            line_break = trans_line + ((int)((line_break - trans_line) * r1 / (r1 - r2))) + 1;
          }
          if(line > trans_line) {
            linetime = t0 + trans_line / r0 + (line - trans_line) / r1 + biu_swap;
          }
          if(line > line_break) {
            linetime = t0 + trans_line / r0 + (line_break - trans_line) / r1 + (line - line_break) / r2 + biu_swap;
          }
        }
        return linetime;
      }
      // LOSSLESS with 8LSB or TABLE fits in image memory
      if(p_dataConvType != "12Bit" && p_compType == "LOSSLESS" && line > trans_line) {
        linetime = t0 + trans_line / r0 + (line - trans_line) / r1 + biu_swap;
      }
    }
    return linetime;
  }// end ComputeLineTime



  /**
  * @brief Find dark current files for this image.
  *
  * Determines which dark parameters file and bias distortion
  * table, if any, should be used for this image and assigns these
  * filenames to p_dparamfile and p_bdpath, respectively. These
  * are dependent on the Instrument ID (ISSNA or ISSWA) and
  * Instrument Mode ID (Full, Sum2, or Sum4).
  *
  * @see DarkParameterFile()
  * @see BiasDistortionTable()
  *
  * @internal
  *   @history 2008-11-05 Jeannie Walldren - Original Version
  */
  void DarkCurrent::FindDarkFiles() {
    // Get the directory where the CISS darkcurrent directory is
    PvlGroup &dataDir = Preference::Preferences().findGroup("DataDirectory");
    QString missionDir = QString::fromStdString(dataDir["Cassini"]);
    QString darkDir(missionDir + "/calibration/darkcurrent/");

    QString instrumentId("");

    if(p_narrow) {
      instrumentId += "nac";
      p_bdpath = darkDir.toStdString() + "nac_bias_distortion.tab";
    }
    else {
      instrumentId += "wac";
    }
    QString instModeId("");
    if(p_sum.toInt() > 1) {
      instModeId = instModeId + "sum" + p_sum;
    }
    else {
      instModeId += "full";
    }
    p_dparamfile = darkDir.toStdString() + instrumentId.toStdString() + "_dark_parameters" + "." + instModeId.toStdString() + ".cub";

    // needs to check directory for closeest epochs and compare
    double closestEpochFileNum = 0;
    QStringList fileList = QDir(darkDir).entryList();
    QString instrument;
    if (p_narrow) {
      instrument = "NAC";
    }
    else {
      instrument = "WAC";
    }
    double imgTime = p_imageTime.mid(0, 4).toDouble() + (p_imageTime.mid(5, 3).toDouble() / 365);
    for(int i = 0; i < fileList.count(); i++){
      if (FileName(fileList[i].toStdString()).baseName().substr(0, 3) != instrument.toStdString()){
        continue;
      }
      double currentEpoch = Isis::toDouble(FileName(fileList[i].toStdString()).baseName().substr(10, 6));
      if (abs(currentEpoch - imgTime) < abs(closestEpochFileNum - imgTime)) { // find closest epoch
          closestEpochFileNum = currentEpoch;
          p_hotpixfile = FileName(fileList[i].toStdString());
      }
    }

    p_hotpixfile = darkDir.toStdString() + p_hotpixfile.baseName() + ".tab";

    return;
  }//end FindDarkFiles


  /**
  * Computes begin time, end time, and duration for each pixel of
  * the image.
  *
  * @see ComputeLineTime()
  *
  * @internal
  *   @history 2008-11-05 Jeannie Walldren - Original Version
  */
  void DarkCurrent::ComputeTimeArrays() {
    // this method mimics get_line_times method of cassimg_subtractdark.pro from idl's cisscal program
    int numberNegTime = 0;
    vector <double> timeToRead(p_lines);
    for(int i = 0; i < p_lines; i++) {
      timeToRead[i] = ComputeLineTime(i + 1);
      if(timeToRead[i] < 0) {
        numberNegTime++;
      }
    }
    if(numberNegTime > 0) return;
    for(int i = 0; i < p_lines; i++) {
      for(int j = 0; j <= i; j++) {
        p_endTime[i][j] = timeToRead[i-j];
      }
      for(int j = 0; j <= i; j++) {
        if(j < i) {
          p_startTime[i][j] = p_endTime[i][j+1];
        }
        else {
          p_startTime[i][j] = 0.0;
        }
        p_duration[i][j] = p_endTime[i][j] - p_startTime[i][j];
      }
    }
    for(int i = 0; i < p_lines; i++) {
      for(int j = 0; j < p_samples; j++) {
        if(p_duration[i][j] <= 0) {
          p_duration[i][j] = 0;
          //I belive this is equivalent to the IDL code :
          //     p_duration(*,*) = p_duration(*,*) > 0.0
        }
      }
    }
    for(int i = 0; i < p_lines; i++) {
      for(int j = 0; j < p_samples; j++) {
        p_endTime[i][j] = p_startTime[i][j] + p_duration[i][j];
      }
    }
    return;
  }//end ComputeTimeArrays


  /**
   * @brief Creates dark array
   * This method reads in the coefficients from the dark
   * parameters file, calls MakeManyLineDark() to create dark_e,
   * removes artifacts of this array be taking the median of every
   * 5 values, and corrects for the average bias distortion at the
   * beginning of each line.
   *
   * @returns <b>vector \<vector \<double\> \></b> Secondary dark
   *         array removed of artifacts and corrected for average
   *         bias distortion.
   *  @throws Isis::iException::Io If the dark parameter file or
   *              bias distortion table is not found.
   *  @throws Isis::iException::Io If p_startTime equals p_endTime
   *              for all pixels.
   *  @see FindDarkFiles()
   *  @see ComputeTimeArrays()
   *  @see MakeManyLineDark()
   *
   *  @internal
   *   @history 2008-11-05 Jeannie Walldren - Original Version
   *   @history 2009-01-26 Jeannie Walldren - Changed declarations of 2
   *                          dimensional vectors
   */
  vector <vector <double> > DarkCurrent::MakeDarkArray() { //return dark_e
    // this method mimics makedarkarray method of cassimg_subtractdark.pro from idl's cisscal program
    FindDarkFiles();
    if(!p_dparamfile.fileExists()) {
      throw IException(IException::Io,
                       "DarkParameterFile ***"
                       + p_dparamfile.expanded() + "*** not found.", _FILEINFO_);
    }
    if(p_narrow && (!p_bdpath.fileExists())) {
      throw IException(IException::Io,
                       "BiasDistortionFile ***"
                       + p_bdpath.expanded() + "*** not found.", _FILEINFO_);
    }
    ComputeTimeArrays();//fill in values for p_startTime, p_endTime, p_duration
    int good = 0;
    for(int i = 0; i < p_lines; i++) {
      for(int j = 0; j < p_samples; j++) {
        if(p_startTime[i][j] != p_endTime[i][j]) {
          good++;
        }
      }
    }
    if(good != 0) {
      //read the coefficient cube into a Brick
      Brick *darkCoefficients;
      Cube dparamCube;
      dparamCube.open(QString::fromStdString(p_dparamfile.expanded()));
      darkCoefficients = new Brick(p_samples, p_lines, 8, dparamCube.pixelType());
      darkCoefficients->SetBasePosition(1, 1, 1);
      dparamCube.read(*darkCoefficients);
      dparamCube.close();

      if (p_hotpixfile.baseName() != "") {
        std::cout << p_hotpixfile.baseName() << '\n';
        CisscalFile *hotPixFile = new CisscalFile(QString::fromStdString(p_hotpixfile.expanded()));
        int num_params = 8;
        long numHotPixels = hotPixFile->LineCount();
        vector<double> exptimes;
        vector<double> tgrid(num_params);
        vector <vector <double> > v1(num_params);

        for(int i = 0; i < num_params; i++) {
          v1[i].resize(num_params);
          switch(i) {
            case 0:
              tgrid[i] = 0.0;
              break;
            case 1:
              tgrid[i] = 10.0;
              break;
            case 2:
              tgrid[i] = 32.0;
              break;
            case 3:
              tgrid[i] = 100.0;
              break;
            case 4:
              tgrid[i] = 220.0;
              break;
            case 5:
              tgrid[i] = 320.0;
              break;
            case 6:
              tgrid[i] = 460.0;
              break;
            case 7:
              tgrid[i] = 1200.0;
              break;
            default:
              tgrid[i] = Null;
          }
        }

        QString exposureLine;
        hotPixFile->GetLine(exposureLine);
        exposureLine=exposureLine.simplified().trimmed();
        QStringList exposureLineList = exposureLine.split(' ');
        for (int j = 2; j < exposureLineList.size(); j++ ){
          exptimes.push_back(exposureLineList[j].toDouble());
        }
        QString junk;
        hotPixFile->GetLine(junk); //this line is gain values which are not used

        int x;
        int y;
        double hotsat;
        vector<double> elec;
        for(long i = 0; i < numHotPixels; i++){
          QString line;
          hotPixFile->GetLine(line);
          if(line == "") {
            break;
          }
          line = line.simplified();
          QStringList lineList = line.split(' ');
          // make hotpix corrections
          x = floor((lineList[0].toDouble() / 1024) * p_samples);
          y = floor((lineList[1].toDouble() / 1024) * p_lines);
          elec.resize(lineList.size() - 3);
          for (unsigned int j = 0; j < elec.size(); j ++){
            elec[j] = (lineList[j + 2].toDouble());
          }
          hotsat = lineList.back().toDouble();

          if (hotsat != 0.0) {
            double maxExp;
            if (hotsat > 0.0) {
              maxExp = exptimes[hotsat - 1];
            }
            else {
              maxExp = *(std::max_element(exptimes.begin(), exptimes.end()));
            }

            IDLLinearInterpolation interpol;
            for (unsigned int j = 0; j < exptimes.size(); j++) {
              if (exptimes[j] <= maxExp) {
                interpol.insert(exptimes[j], elec[j]);
              }
            }
            if (interpol.size() > 1) {
              for (int j = 0; j < 8; j++){
                (*darkCoefficients)[darkCoefficients->Index(x+1, y+1, j+1)] = interpol.evaluate(tgrid[j]);
              }
            }
          }
        }
      }

      // Assume WAC dark current is 0 for 0.005 ms. This is not the case for
      // the NAC where there are negative values near the left edge of the frame:
      if(!p_narrow) {
        for(int i = 0; i < p_lines; i++) {
          for(int j = 0; j < p_samples; j++) {
            (*darkCoefficients)[darkCoefficients->Index(i+1, j+1, 1)] = 0.0;
          }
        }
      }
      // add functionality for summed images:
      vector <vector <double> > dark_e(p_samples), di1(p_samples);
      for(unsigned int i = 0; i < dark_e.size(); i++) {
        dark_e[i].resize(p_lines);
        di1[i].resize(p_lines);
      }

      dark_e = MakeManyLineDark(*darkCoefficients);

      // Median-ed dark images have some spikes below the fitted curve.
      // These are probably artifacts and the next section removes them.
      vector <double> neighborhood(5);

      //replace each value of di1 with the median of neighborhood of 5 values
      for(int i = 0; i < p_lines; i++) {
        for(int j = 0; j < p_samples; j++) {
          if(j < 2 || j > (p_samples - 3)) {
            di1[i][j] = dark_e[i][j];
          }
          else {
            for(int n = -2; n < 3; n++) {
              neighborhood[n+2] = dark_e[i][j+n];
            }
            //sort these five values
            sort(neighborhood.begin(), neighborhood.end());
            for(int f = 0; f < 5; f++) {
            }
            //set equal to median
            di1[i][j] = neighborhood[2];
          }
        }
      }
      for(int i = 0; i < p_lines; i++) {
        for(int j = 0; j < p_samples; j++) {
          if(di1[i][j] - dark_e[i][j] > 10) {
            dark_e[i][j] = di1[i][j];
          }
        }
      }
      // correct for the average bias distortion at the beginning of each line:
      if(p_narrow) {
        // They gave a new bias file to compare against but it currently
        // is not implemented in the IDL code as it is commented out in their code.
/*
        CisscalFile *biasDist = new CisscalFile(p_bdpath.expanded());
        vector<double> samp, bias_distortion;
        for(int i = 0; i < biasDist->LineCount(); i++) {
          QString line;
          biasDist->GetLine(line);  //assigns value to line
          line = line.simplified().trimmed();
          if(line == "") {
            break;
          }
          samp.push_back(toDouble(line.split(" ").first()));
          bias_distortion.push_back(toDouble(line.split(" ")[1]));
        }
        biasDist->Close();
        for(int i = 0; i < 21; i++) {
          for(int j = 0; j < p_lines; j++) {
            dark_e[i][j] = dark_e[i][j] - bias_distortion[i];
          }
        }
*/
      }
      return dark_e;
    }
    throw IException(IException::Io,
                     "StartTime == EndTime for all pixels.",
                     _FILEINFO_);
  }//end MakeDarkArray

  /**
  * @brief Creates preliminary dark array from dark parameters
  * file and line-time information
  *
  * Compute one line of a synthetic dark frame from timing tables
  * and dark current parameters for each pixel using a spline
  * interpretation method and evaluating at the start and end
  * times for that pixel.
  *
  * @param darkBrick Containing the coefficients found in the dark
  *                  parameters file for each pixel.
  *
  * @returns <b>vector \<vector \<double\> \></b> Preliminary dark
  *         array using the darkBrick values
  *
  * @internal
  *   @history 2008-11-05 Jeannie Walldren - Original Version
  *   @history 2009-01-26 Jeannie Walldren - Changed declarations of 2
  *                          dimensional vectors
  */
  vector <vector <double> > DarkCurrent::MakeManyLineDark(Brick &darkBrick) { //returns dark_e
    // this method mimics make_manyline_dark method of cassimg_subtractdark.pro from idl's cisscal program
    int num_params = 8;
    vector <vector <double> > dark(p_samples), v1(num_params);
    vector <double> temp(p_samples), tgrid(num_params);
    vector <double> c(2), timespan(2);
    for(int i = 0; i < 256 ; i ++) {
      for (int line = 0; line < 256 ; line++) {
      }
    }
    for(unsigned int i = 0; i < dark.size(); i++) {
      dark[i].resize(p_lines);
    }
    for(int i = 0; i < num_params; i++) {
      v1[i].resize(num_params);
      switch(i) {
        case 0:
          tgrid[i] = 0.0;
          break;
        case 1:
          tgrid[i] = 10.0;
          break;
        case 2:
          tgrid[i] = 32.0;
          break;
        case 3:
          tgrid[i] = 100.0;
          break;
        case 4:
          tgrid[i] = 220.0;
          break;
        case 5:
          tgrid[i] = 320.0;
          break;
        case 6:
          tgrid[i] = 460.0;
          break;
        case 7:
          tgrid[i] = 1200.0;
          break;
        default:
          tgrid[i] = Null;
      }
    }
    for(int j = 0; j < num_params; j++) {
      v1[j][j] = 1.0;
    }
    Progress progress;
    progress.SetText("Computing dark current array...");
    progress.SetMaximumSteps(p_lines);
    progress.CheckStatus();
    for(int jline = 0; jline < p_lines; jline++) {
      for(int i = 0; i < p_samples; i++) {
        temp[i] = darkBrick[darkBrick.Index(i+1, jline+1, 1)]; // constant term
      }
      // sum the contribution from every pixel downstream of jline, including jline
      for(int kline = 0; kline <= jline; kline++) {
        // derive coefficients so that parameters can be multiplied and added
        // rather than interpolated
        timespan[0] = p_startTime[jline][kline];
        timespan[1] = p_endTime[jline][kline];
        //  Interpolate by fitting a cubic spline to the
        //    4 point neighborhood (x[i-1], x[i], x[i+1], x[i+2]) surrounding
        //    the interval, x[i] <= u < x[i+1].
        NumericalApproximation spline(NumericalApproximation::CubicNeighborhood);
        for(int j = 0; j < num_params; j++) {
          spline.AddData(tgrid, v1[j]);
          //spline.Compute();
          c = spline.Evaluate(timespan);
          spline.Reset();
          c[0] =  c[1] - c[0];
          if(c[0] != 0.0) {
            for(int i = 0; i < p_samples; i++) {
              temp[i] = temp[i] + c[0] * darkBrick[darkBrick.Index(i+1, kline+1, j+1)];
            }
          }
        }
      }
      for(int i = 0; i < p_samples; i++) {
        dark[i][jline] = temp[i];
      }

      progress.CheckStatus();
    }
    return dark;
  }//end MakeManyLineDark


  /**
  * @brief Evaluates Interpol value for tgrid.
  *
  * Used to find the estimated values for tgrid that may or may not have an actual value saved in
  * tgrid. A new class and function was made to better mirror the way IDL's interpol function works
  *
  * @param input Value to be evaluated
  *
  * @returns estimate using m*x+b
  *
  * @internal
  *   @history 2017-08-24 Cole Neubauer Original Version
  */
  double DarkCurrent::IDLLinearInterpolation::evaluate(const double input) const{
    double lowKey, highKey, slope;
    IDLLinearInterpolation::const_iterator lowIt = lowerBound(input);
    if (lowIt == begin()){
      lowKey = lowIt.key();
      highKey = (++lowIt).key();
    }
    else if (lowIt == end()){
      highKey = (--lowIt).key();
      lowKey = (--lowIt).key();
    }
    else {
      highKey = lowIt.key();
      lowKey = (--lowIt).key();
    }
    if (fabs(highKey - lowKey) < 1.0e-10) {
      std::string msg("Cannot Interpolate Repeated X Values");
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    slope = (value(highKey) - value(lowKey)) / (highKey - lowKey);
    return slope * (input - lowKey) + value(lowKey);
  }
}//end namespace Isis
