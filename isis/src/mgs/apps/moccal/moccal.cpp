/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ProcessByLine.h"
#include "SpecialPixel.h"
#include "MocLabels.h"
#include "iTime.h"
#include "IException.h"
#include "TextFile.h"
#include "LineManager.h"
#include "NaifStatus.h"
#include "Camera.h"

#include "moccal.h"

using namespace std;

namespace Isis {
  // Working functions and parameters
  namespace gbl {
    void Calibrate(Buffer &in, Buffer &out);
    void LoadCoefficients(const QString &file, int ns);
    void FixWagoLines(QString file);

    double a;      // Commanded system gain
    double off;    // Commanded system offset
    double ex;     // Exposure duration
    double z;      // Fixed zero offset
    double dc;     // dark current term
    double g;      // gain dependent offset
    double w0;     // omega naught
    double iof;    // conversion from counts/ms to IOF

    vector<double> pixelGain;    // Pixel dependent gain table
    vector<double> pixelOffset;  // Pixel dependent offset table

    vector<double> inLineAvg;     // Average of each input line
    vector<double> outLineAvg;    // Average of each output line

    MocLabels *moc;

    bool nullWago;
  }


  void moccal(UserInterface &ui) {
    Cube icube(ui.GetCubeName("FROM"), "rw");
    moccal(&icube, ui);
  }


  void moccal(Cube *icube, UserInterface &ui) {
    // We will be processing by line
    ProcessByLine p;

    // Setup the input and make sure it is a moc file
    p.SetInputCube(icube, OneBand);
    gbl::moc = new MocLabels(icube->fileName());

    // If it is already calibrated then complain
    if(icube->hasGroup("Radiometry")) {
      QString msg = "The MOC image [" + icube->fileName() + "] has already "
                  "been radiometrically calibrated";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Get label parameters we will need for calibration equation
    gbl::a = gbl::moc->Gain();
    gbl::off = gbl::moc->Offset();
    gbl::ex = gbl::moc->ExposureDuration();

    // Get the starting, ending, and activation et's.  For now, the
    // activation et is set to the largest double precision value.  If
    // The narrow angle B detectors ever get activated then the value
    // will need to be changed to the appropriate et
    iTime startTime(gbl::moc->StartTime());
    double etStart = startTime.Et();
    double etNABActivation = DBL_MAX;

    // Open the calibration kernel that contains constants for each camera
    // and internalize it in a pvl object
    QString calKernelFile;
    if(ui.WasEntered("CALKERNEL")) {
      calKernelFile = ui.GetFileName("CALKERNEL");
    }
    else {
      calKernelFile = p.MissionData("mgs", "/calibration/moccal.ker.???", true);
    }
    Pvl calKernel(calKernelFile.toStdString());

    // Point to the right group of camera parameters
    QString camera;
    if(gbl::moc->WideAngleRed()) {
      camera = "WideAngleRed";
    }
    else if(gbl::moc->WideAngleBlue()) {
      camera = "WideAngleBlue";
    }
    else if(etStart > etNABActivation) {
      camera = "NarrowAngleB";
    }
    else {
      camera = "NarrowAngleA";
    }
    PvlGroup &calCamera = calKernel.findGroup(camera.toStdString());

    // Get the camera specific calibration parameters from the kernel file
    // and load detector coefficients (gain/offsets at each pixel)
    gbl::z = calCamera["Z"];
    gbl::dc = calCamera["DC"];
    gbl::g = calCamera["G"];
    gbl::w0 = calCamera["W0"];
    QString coefFile =  QString::fromStdString(calCamera["CoefFile"]);
    gbl::LoadCoefficients(coefFile, icube->sampleCount());

  #if 0
    // Override with these with any user selected parameters
    if(ui.WasEntered("Z")) gbl::z = ui.GetDouble("Z");
    if(ui.WasEntered("DC")) gbl::dc = ui.GetDouble("DC");
    if(ui.WasEntered("G")) gbl::g = ui.GetDouble("G");
    if(ui.WasEntered("W0")) gbl::w0 = ui.GetDouble("W0");
  #endif
    gbl::nullWago = ui.GetBoolean("NULLWAGO");

    double kmPerAU = 1.4959787066E8;
    double sunAU = 0;
    try {
      Camera *cam;
      cam = icube->camera();
      cam->setTime(startTime);
      sunAU = cam->sunToBodyDist() / kmPerAU;
    }
    catch(IException &e) {
      // Get the distance between Mars and the Sun at the given time in
      // Astronomical Units (AU)
      
      NaifStatus::CheckErrors();
      QString bspKernel = p.MissionData("base", "/kernels/spk/de???.bsp", true);
      furnsh_c(bspKernel.toLatin1().data());
      QString satKernel = p.MissionData("base", "/kernels/spk/mar???.bsp", true);
      furnsh_c(satKernel.toLatin1().data());
      QString pckKernel = p.MissionData("base", "/kernels/pck/pck?????.tpc", true);
      furnsh_c(pckKernel.toLatin1().data());
      NaifStatus::CheckErrors();

      double sunpos[6], lt;
      spkezr_c("sun", etStart, "iau_mars", "LT+S", "mars", sunpos, &lt);
      double dist = vnorm_c(sunpos);
      sunAU = dist / kmPerAU;
      
      NaifStatus::CheckErrors();
      unload_c(bspKernel.toLatin1().data());
      unload_c(satKernel.toLatin1().data());
      unload_c(pckKernel.toLatin1().data());
      NaifStatus::CheckErrors();
    }

    // See if the user wants counts/ms or i/f but if w0 is 0 then
    // we must go to counts/ms
    //    iof = conversion factor from counts/ms to i/f
    bool convertIOF = ui.GetBoolean("IOF") && (gbl::w0 > 0.0);
    if(convertIOF) {
      gbl::iof = sunAU * sunAU / gbl::w0;
    }
    else {
      gbl::iof = 1.0;
    }

    // Setup the output cube
    Cube *ocube = p.SetOutputCubeStretch("TO", &ui);

    // Add the radiometry group
    PvlGroup calgrp("Radiometry");
    calgrp += PvlKeyword("CalibrationKernel", calKernelFile.toStdString());
    calgrp += PvlKeyword("CoefficientFile", coefFile.toStdString());

    calgrp += PvlKeyword("a", std::to_string(gbl::a));
    calgrp["a"].addComment("Radiometric equation in moccal");
    calgrp["a"].addComment("r = (pixel - z + off) / a - g / ex - dc");
    calgrp += PvlKeyword("off", std::to_string(gbl::off));
    calgrp += PvlKeyword("ex", std::to_string(gbl::ex));
    calgrp += PvlKeyword("z", std::to_string(gbl::z));
    calgrp += PvlKeyword("dc", std::to_string(gbl::dc));
    calgrp += PvlKeyword("g", std::to_string(gbl::g));

    calgrp += PvlKeyword("w0", std::to_string(gbl::w0));
    calgrp["w0"].addComment("Reflectance = r * iof, where iof = (s * s) / w0");
    calgrp += PvlKeyword("s", std::to_string(sunAU));
    calgrp += PvlKeyword("iof", std::to_string(gbl::iof));

    ocube->putGroup(calgrp);

    // Start the line-by-line calibration sequence
    p.StartProcess(gbl::Calibrate);
    p.EndProcess();

    // Now go fix errors around the wago changes
    gbl::FixWagoLines(ui.GetCubeName("TO"));

    // Cleanup
    gbl::pixelGain.clear();
    gbl::pixelOffset.clear();
    gbl::inLineAvg.clear();
    gbl::outLineAvg.clear();
  }

  // Line processing routine
  void gbl::Calibrate(Buffer &in, Buffer &out) {
    // Initialize counters
    double isum = 0.0;
    double osum = 0.0;
    int count = 0;

    // Get the line/time dependent gain/offset
    gbl::a = gbl::moc->Gain((int)in.Line());
    gbl::off = gbl::moc->Offset((int)in.Line());

    // Loop and apply calibration
    for(int i = 0; i < in.size(); i++) {
      // Handle good pixels
      if(IsValidPixel(in[i])) {
        // compute r in counts/ms
        double r = ((in[i] - gbl::z + gbl::off) / gbl::a - gbl::g) /
                  gbl::ex - gbl::dc;
        r = gbl::pixelGain[i] * r + gbl::pixelOffset[i];

        // Convert to i/f
        out[i] = r * gbl::iof;

        osum += out[i];
        isum += in[i];
        count ++;
      }

      // Handle special pixels
      else {
        out[i] = in[i];
      }
    }

    // Compute the average of the input line and output line
    double inAverage = 0.0;
    double outAverage = 0.0;
    if(count > 0) {
      inAverage = isum / (double) count;
      outAverage = osum / (double) count;
    }

    // Need to keep track of the averages for each line so push them onto
    // a stack
    gbl::inLineAvg.push_back(inAverage);
    gbl::outLineAvg.push_back(outAverage);
  }


  void gbl::LoadCoefficients(const QString &file, int ns) {
    // First create space for our coefficients
    gbl::pixelGain.resize(ns);
    gbl::pixelOffset.resize(ns);

    // Now initialize them to unity
    for(int i = 0; i < ns; i++) {
      gbl::pixelGain[i] = 1.0;
      gbl::pixelOffset[i] = 0.0;
    }

    // If the file is not provided we are done
    if(file == "") return;

    // Otherwise read in the coefficients
    vector<double> gainCoef;
    vector<double> offsetCoef;
    TextFile coef(file);
    QString record, tok;
    coef.GetLine(record, true);
    int numCoefs = toInt(record);
    for(int i = 0; i < numCoefs; i++) {
      coef.GetLine(record, true);
      record = record.simplified().trimmed();

      QStringList records = record.split(" ");

      if (records.count() > 1) {
        gainCoef.push_back(toDouble(records.takeFirst()));
        offsetCoef.push_back(toDouble(records.takeFirst()));
      }
    }

    // Make sure the file had the correct number of coefficients.  It should
    // match the number of detectors in the NA or WA camera
    if((int)gainCoef.size() != gbl::moc->Detectors()) {
      QString msg = "Coefficient file [" + file + "] size is wrong ... should have [";
      msg += toString(gbl::moc->Detectors()) + "] gain/offset entries";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // The gain and offset will need to be based on the compression from
    // detectors to sample (crosstrack summing mode)
    for(int samp = 1; samp <= ns; samp++) {
      int ss = gbl::moc->StartDetector(samp);
      int es = gbl::moc->EndDetector(samp);
      int n;
      double gsum = 0.0;
      double osum = 0.0;
      for(n = 0; ss <= es; ss++, n++) {
        if(ss >= (int)gainCoef.size()) {
          QString msg = "Array bounds exceeded for gainCoef/offsetCoef";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }
        gsum += gainCoef[ss];
        osum += offsetCoef[ss];
      }
      gbl::pixelGain[samp-1]   = gsum / (double) n;
      gbl::pixelOffset[samp-1] = osum / (double) n;
    }
  }

  void gbl::FixWagoLines(QString file) {
    // Nothing to do for narrow angle
    if(gbl::moc->NarrowAngle()) return;

    // Open the cube to repair
    Cube fix;
    fix.open(file, "rw");
    const int nl = fix.lineCount();

    // Create a line manager on the cube for I/O
    LineManager lbuf(fix);

    // Determine which lines need to be examined
    double lastGain = moc->Gain();
    double lastOffset = moc->Offset();
    vector<int> fixList;
    for(int line = 2; line <= nl; line++) {
      double gain = moc->Gain(line);
      double offset = moc->Offset(line);
      if((lastGain != gain) || (lastOffset != offset)) fixList.push_back(line);
      lastGain = gain;
      lastOffset = offset;
    }

    const int nlBefore = 2;
    const int nlAfter = 2;
    const int nlavg = 4;
    const double fixFactor = 1.5;

    // Loop for each line to fix
    for(unsigned int i = 0; i < fixList.size(); i++) {
      // We will examine a window of lines around the wago change
      int centerLine = fixList[i];
      int sl = centerLine - nlBefore - nlavg;
      int el = centerLine + nlAfter + nlavg;

      // Don't do anything if the window is outside the image
      if(sl < 1) continue;
      if(el < 1) continue;
      if(sl > nl) continue;
      if(el > nl) continue;

      // Find the closest non-zero output line average before the wago line
      int slBefore = sl;
      int elBefore = sl + nlavg;
      int indexBefore = -1;
      for(int line = elBefore; line >= slBefore; line--) {
        if(gbl::outLineAvg[line-1] != 0.0) {
          indexBefore = line - 1;
          break;
        }
      }
      if(indexBefore < 0) continue;
      double oavgBefore = gbl::outLineAvg[indexBefore];

      // Find the closest non-zero output line average before the wago line
      int slAfter = el - nlavg;
      int elAfter = el;
      int indexAfter = -1;
      for(int line = slAfter; line <= elAfter; line++) {
        if(gbl::outLineAvg[line-1] != 0.0) {
          indexAfter = line - 1;
          break;
        }
      }
      if(indexAfter < 0) continue;
      double oavgAfter = gbl::outLineAvg[indexAfter];

      // Get the corresponding input averages and compute net WAGO change
      // Don't do anything if the net change is invalid
      double iavgBefore = gbl::inLineAvg[indexBefore];
      double iavgAfter  = gbl::inLineAvg[indexAfter];

      double sum = (iavgBefore / iavgAfter) / (oavgBefore / oavgAfter);
      if(abs(1.0 - sum) < 0.05) continue;

      // Prep to fix the lines
      sl = centerLine - nlBefore;
      el = centerLine + nlAfter;
      int nlFix = el - sl + 1;
      double fixinc = (oavgAfter - oavgBefore) / (nlFix + 1);
      double base = oavgBefore;
      double avgTol = abs(fixinc * fixFactor);

      // Loop and fix each one
      for(int line = sl; line <= el; line++) {
        base += fixinc;
        double avg = base - gbl::outLineAvg[line-1];

        // Do we need to fix this line?
        if(abs(avg) <= avgTol) continue;

        // Read the line
        lbuf.SetLine(line);
        fix.read(lbuf);

        // Null it
        if(gbl::nullWago) {
          for(int samp = 0 ; samp < lbuf.size(); samp++) {
            lbuf[samp] = Null;
          }
          outLineAvg[line-1] = 0.0;
        }

        // or repair it
        else {
          avg = outLineAvg[line-1];
          outLineAvg[line-1] = base;

          for(int samp = 0; samp < lbuf.size(); samp++) {
            if(IsValidPixel(lbuf[samp])) {
              lbuf[samp] = lbuf[samp] / avg * base;
            }
          }
        }

        // Write the line
        fix.write(lbuf);
      }
    }

    // Cleanup
    fix.close();
  }
}