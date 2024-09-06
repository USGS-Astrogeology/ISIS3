/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <vector>

#include <QString>

#include <SpiceUsr.h>
#include <SpiceZfc.h>
#include <SpiceZmc.h>

#include "CalParameters.h"
#include "Camera.h"
#include "Cube.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "LeastSquares.h"
#include "Pvl.h"
#include "TextFile.h"
#include "NaifStatus.h"

using namespace std;
namespace Isis {

  CalParameters::CalParameters(const QString &fname, Cube *icube) {
    try {
      // Extract Pvl Information from the file
      Pvl pvl(fname.toLatin1().data());

      // Get keywords from input cube label
      PvlGroup &instrument = pvl.findGroup("INSTRUMENT", Pvl::Traverse);

      // Make sure it is a viking mission
      QString spacecraft = QString::fromStdString(instrument["SPACECRAFTNAME"]);
      QString mission = spacecraft.split("_").first();
      spacecraft = spacecraft.split("_").last();
      if(mission != "VIKING") {
        std::string msg = "Invalid Keyword [SpacecraftName]. " +  spacecraft +
                     "must start with 'VIKING'";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      int spn = 0;
      if((QChar)spacecraft[spacecraft.size() - 1] == '1') spn = 1;
      if((QChar)spacecraft[spacecraft.size() - 1] == '2') spn = 2;
      if(spn == 0) {
        std::string msg = "Invalid Keyword [SpacecraftName]. " + spacecraft +
                     "must terminate with '1' or '2'";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      double clock = instrument["SPACECRAFTCLOCKCOUNT"];
      QString instId = QString::fromStdString(instrument["INSTRUMENTID"]);
      int cam;
      // Camera State 4 is used to indicate an extended mission. This is
      // necessary because the dust spot changed position during the extended
      // mission, requiring a new set of calibration files.
      int cs4 = 0;
      if((QChar)instId[instId.size() - 1] == 'A') {
        if(spn == 1) cam = 7;
        else cam = 8;
      }
      else if((QChar)instId[instId.size() - 1] == 'B') {
        if(spn == 1) {
          cam = 4;
          if(clock > 44800000) cs4 = 1;
        }
        else cam = 6;
      }
      else {
        std::string msg = "Invalid Keyword [InstrumentID]. " + instId;
        msg += "must terminate with an 'A' or 'B'";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      QString startTime = QString::fromStdString(instrument["STARTTIME"]);
      p_dist1 = CalcSunDist(startTime, icube);
      p_labexp = (double)instrument["EXPOSUREDURATION"] * 1000.0;  // convert to msec
      QString target = " ";
      PvlKeyword cs1 = instrument["FLOODMODEID"];
      if(QString::fromStdString(cs1) == "ON") cs1 = "1";
      else if(QString::fromStdString(cs1) == "OFF") cs1 = "0";
      PvlKeyword cs2 = instrument["GAINMODEID"];
      if(QString::fromStdString(cs2) == "LOW") cs2 = "0";
      else if(QString::fromStdString(cs2) == "HIGH") cs2 = "1";
      PvlKeyword cs3 = instrument["OFFSETMODEID"];
      if(QString::fromStdString(cs3) == "ON") cs3 = "1";
      else if(QString::fromStdString(cs3) == "OFF") cs3 = "0";
      PvlKeyword wav = pvl.findGroup("BANDBIN", Pvl::Traverse)["FILTERID"];

      // Set up calibration, linearity, and offset variables for the input file
      vikcalSetup(mission, spn, target, cam, QString::fromStdString(wav), (int)cs1, (int)cs2, (int)cs3, (int)cs4);
      viklinSetup(mission, spn, target, cam, QString::fromStdString(wav), (int)cs1, (int)cs2, (int)cs3, (int)cs4);
      vikoffSetup(mission, spn, target, cam, clock, (int)cs3);
    }
    catch(IException &e) {
      std::string msg = "Input file [" + fname + "] does not appear to be a viking image";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  /**
   * Finds the correct calibration data values for the input cube in the
   * vikcal.sav file
   *
   * @param mission The mission name found in the input cube label
   * @param spn The spacecraft number found in the input cube label
   * @param target The target name found in the input cube label (this value is
   *               not used in the vikcal.sav file, and is overriden with a space)
   * @param cam The camera number found in the input cube label
   * @param cs1 The state of camera 1 found in the input cube label
   *            (Light Flood Mode)
   * @param cs2 The state of camera 2 found in the input cube label (Gain Mode)
   * @param cs3 The state of camera 3 found in the input cube label (Offset Mode)
   * @param cs4 The state of camera 4, set only if camera is 4, and it is an
   *            extended mission
   *
   * @throws Isis::IException::Programmer - Could not find match in vikcal.sav
   */
  void CalParameters::vikcalSetup(QString mission, int spn, QString target,
                                  int cam, QString wav, int cs1, int cs2, int cs3, int cs4) {
    vector<QString> line;

    // Read in vikcal.sav calibration file
    TextFile cal("$viking" + toString(spn) + "/calibration/vikcal.sav",
                 "input", line, 0, true);

    // Search for a line in the vikcal.sav file that matches our data from the
    // input label
    for(int i = 0; i < (int)line.size(); i++) {
      QString temp = line[i].simplified();

      QStringList tokens = temp.split(" ");

      if(tokens.count() < 15) continue;
      if(tokens[0] != mission) continue;
      if(toInt(tokens[1]) != spn) continue;
      if(toInt(tokens[2]) != cam) continue;
      if(tokens[3] != wav) continue;
      if(toInt(tokens[4]) != cs1) continue;
      if(toInt(tokens[5]) != cs2) continue;
      if(toInt(tokens[6]) != cs3) continue;
      if(toInt(tokens[7]) != cs4) continue;


      // The line is a match for our data, so set all the
      // Calibration variables to their correct values
      p_w0 = toDouble(tokens[8]);
      p_dist = toDouble(tokens[9]);
      p_gain = toDouble(tokens[10]);
      p_offset = toDouble(tokens[11]);
      p_exp = toDouble(tokens[12]);
      p_gainFile = "$viking" + toString(spn) + "/calibration/" + tokens[13]
                   + ".cub";
      p_offsetFile = "$viking" + toString(spn) + "/calibration/" +
                     tokens[14] + ".cub";
      return;
    }
    std::string msg = "Could not find match in [vikcal.sav] calibration file";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }

  /**
   * Finds the correct linearization values for the input image.  This option is
   * in the Isis2 version of vikcal, but due to the values of B in the viklin.sav
   * file it is never actually used.  If this option is necessary for some reason,
   * place the following code in the vikcal.xml file.
   * @code
   * <group name="Option">
   *     <parameter name="LINEAR">
   *         <type>boolean</type>
   *         <brief>
   *             Linearizer option
   *         </brief>
   *         <description>
   *             Flag to indicate if LINEARIZER option is desired.  LINEAR=TRUE
   *             indicates linear correction to be applied if there is an entry in the
   *             viklin.sav file for the mission and camera to be processed.
   *             LINEAR=FALSE indicates that no linear correction is to be applied.
   *             Defaults to TRUE.
   *         </description>
   *         <default><item>TRUE</item></default>
   *     </parameter>
   * </group>
   * @endcode
   *
   * @param mission The mission name found in the input cube label
   * @param spn The spacecraft number found in the input cube label
   * @param target The target found in the input cube (this option is not used,
   *               and the actual target is overriden with a space)
   * @param cam The camera number found in the input cube
   * @param wav The wave length found in the input cube
   * @param cs1 The state of camera 1 found in the input cube label
   *            (Light Flood Mode)
   * @param cs2 The state of camera 2 found in the input cube label (Gain Mode)
   * @param cs3 The state of camera 3 found in the input cube label (Offset Mode)
   * @param cs4 The state of camera 4, set only if the camera is 4, and it is an
   *            extended mission
   *
   * @throws Isis::IException::Programmer - Could not find match in viklin.sav
   */
  void CalParameters::viklinSetup(QString mission, int spn, QString target,
                                  int cam, QString wav, int cs1, int cs2, int cs3, int cs4) {

    vector<QString> line;
    TextFile lin("$viking" + toString(spn) + "/calibration/viklin.sav",
                 "input", line, 0, true);

    for(int i = 0; i < (int)line.size(); i++) {
      QString temp = line[i].simplified();

      QStringList tokens = temp.split(" ");

      if(tokens.count() < 10) continue;
      if(tokens[0] != mission) continue;
      if(toInt(tokens[1]) != spn) continue;
      if(toInt(tokens[2]) != cam) continue;
      if(tokens[3] != wav) continue;
      if(toInt(tokens[4]) != cs1) continue;
      if(toInt(tokens[5]) != cs2) continue;
      if(toInt(tokens[6]) != cs3) continue;

      // Set all Linearity variables to the correct values
      p_b = toDouble(tokens[7]);
      p_k = toDouble(tokens[8]);
      p_normpow = toDouble(tokens[9]);
      return;
    }
    std::string msg = "Could not find match in [viklin.sav] calibration file";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }

  /**
   * Finds the correct offset data values for the input cube in the
   * vikoff.sav file
   *
   * @param mission The mission name found in the input cube label
   * @param spn The spacecraft number found in the input cube label
   * @param target The target found in the input cube (this option is not used,
   *               and the actual target is overriden with a space)
   * @param cam The camera number found in the input cube
   * @param clock The spacecraft clock count found in the input cube label
   * @param cs3 The state of camera 3 found in the input cube label (Offset Mode)
   *
   * @throws Isis::IException::Programmer - Could not find match in vikoff.sav
   */
  void CalParameters::vikoffSetup(QString mission, int spn, QString target,
                                  int cam, double clock, int cs3) {
    vector<QString> line;

    // Get the correct offset file - depends on which camera the input image is
    // from
    QString fname = "$viking" + toString(spn) + "/calibration/vikoffcam" +
                   toString(cam) + ".sav";
    TextFile off(fname, "input", line, 0, true);
    vector<double> pp[5], off3;
    double pp1_off[5], pp2_off[5], pp_off[5];
    double rise, run, xm, xb;
    double frm1 = -1.0;
    double frm2 = -1.0;
    for(int i = 0; i < (int)line.size(); i++) {
      QString temp = line[i].simplified();

      QStringList tokens = temp.split(" ");

      if(tokens.isEmpty()) continue;

      // Go through the first line of the offset file and set all the principle
      // point locations
      if(tokens[0] == "VIKING") {
        if (tokens.count() < 12) continue;

        if(toInt(tokens[1]) == spn) { }
        if(toInt(tokens[2]) == cam) { }
        pp[0].push_back(toDouble(tokens[3]));
        pp[0].push_back(toDouble(tokens[4]));
        pp[1].push_back(toDouble(tokens[5]));
        pp[1].push_back(toDouble(tokens[6]));
        pp[2].push_back(toDouble(tokens[7]));
        pp[2].push_back(toDouble(tokens[8]));
        pp[3].push_back(toDouble(tokens[9]));
        pp[3].push_back(toDouble(tokens[10]));
        pp[4].push_back(toDouble(tokens[11]));
        pp[4].push_back(toDouble(tokens[12]));
        continue;
      }
      if(toDouble(tokens[0]) < clock) {
        if (tokens.count() < 7) continue;

        frm1 = toDouble(tokens[0]);
        off3.push_back(toDouble(tokens[1]));
        pp1_off[2] = toDouble(tokens[2]);
        pp1_off[0] = pp1_off[2] + toDouble(tokens[3]);
        pp1_off[1] = pp1_off[2] + toDouble(tokens[4]);
        pp1_off[3] = pp1_off[2] + toDouble(tokens[5]);
        pp1_off[4] = pp1_off[2] + toDouble(tokens[6]);
        continue;
      }
      else {
        if (tokens.count() < 7) continue;

        frm2 = toDouble(tokens[0]);
        off3.push_back(toDouble(tokens[1]));
        pp2_off[2] = toDouble(tokens[2]);
        pp2_off[0] = pp2_off[2] + toDouble(tokens[3]);
        pp2_off[1] = pp2_off[2] + toDouble(tokens[4]);
        pp2_off[3] = pp2_off[2] + toDouble(tokens[5]);
        pp2_off[4] = pp2_off[2] + toDouble(tokens[6]);
      }
      if(frm1 == -1.0 || frm2 == -1.0) {
        std::string msg = "Could not find match in [vikoff.sav] calibration file";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      // Calculate the offset at each point
      run = frm2 - frm1;
      for(int i = 0; i < 5; i++) {
        rise = pp2_off[i] - pp1_off[i];
        xm = rise / run;
        xb = pp1_off[i] - xm * frm1;
        pp_off[i] = xm * clock + xb;
      }
      p_constOff = 0;
      p_off_off = 0.0;

      // Calculate the constant offset
      if(cs3 == 0) {
        p_constOff = 1;
        rise = off3[1] - off3[0];
        xm = rise / run;
        xb = off3[0] - xm * frm1;
        p_off_off = xm * clock + xb;
      }

      // Found correct clock time, set offset values
      BasisFunction bilinearInterp("leastSquares", 5, 5);
      LeastSquares lsq(bilinearInterp);
      vector<double> known(5);
      for(int i = 0; i < 5; i++) {
        known[0] = pp[i][0];
        known[1] = pp[i][0] * pp[i][0];
        known[2] = pp[i][1];
        known[3] = pp[i][0] * pp[i][1];
        known[4] = 1.0;
        lsq.AddKnown(known, pp_off[i], 1.0);
      }
      lsq.Solve();
      p_A = bilinearInterp.Coefficient(0);
      p_B = bilinearInterp.Coefficient(1);
      p_C = bilinearInterp.Coefficient(2);
      p_D = bilinearInterp.Coefficient(3);
      p_E = bilinearInterp.Coefficient(4);

      return;
    }
    std::string msg = "Could not find match in [vikoff.sav] calibration file";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }

  /**
   * Calculates the distance from Mars to the sun at the specified time.
   * Try to useing the camera assiciated with the cube first, if that
   * doesn't work fall back to using the SPICE data.
   *
   * @param t The UTC time at which the sun distance is being requested
   * @param iCube The cube we are calibrating
   *
   * @return Distance from the Sun to Mars in km
   */
  double CalParameters::CalcSunDist(QString t, Cube *iCube) {
    try {
      Camera *cam;
      cam = iCube->camera();
      iTime startTime(t);
      cam->setTime(startTime);
      return cam->sunToBodyDist();
    }
    catch(IException &e) {
      // Failed to instantiate a camera, try furnishing kernels directly
      try {
        NaifStatus::CheckErrors();
        double sunv[3];
        SpiceDouble lt, et;
        FileName fname1 = (FileName)"$base/kernels/lsk/naif0007.tls";
        FileName fname2 = (FileName)"$base/kernels/spk/de405.bsp";
        QString tempfname1 = fname1.expanded();
        QString tempfname2 = fname2.expanded();
        furnsh_c(tempfname1.toLatin1().data());
        furnsh_c(tempfname2.toLatin1().data());
        utc2et_c(t.toLatin1().data(), &et);
        spkezp_c(10, et, "J2000", "LT+S", 499, sunv, &lt);
        return sqrt(sunv[0] * sunv[0] + sunv[1] * sunv[1] + sunv[2] * sunv[2]);
        NaifStatus::CheckErrors();
      }
      catch(IException &e) {
        std::string msg = "Unable to determine the distance from Mars to the Sun";
        throw IException(e, IException::User, msg, _FILEINFO_);
      }
    }
  }
} // end namespace isis
