/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Application.h"
#include "ProcessByLine.h"
#include "Buffer.h"
#include "Camera.h"
#include "iTime.h"
#include "SpecialPixel.h"
#include "Spice.h"
#include "TextFile.h"
#include "NaifStatus.h"

#include "gllssical.h"

using namespace std;

/**
 * Shutter file:
 * The shutter on the galileo SSI camera took from
 * about 1ms at the top of the camera to about 1.5ms
 * at the bottom due to friction. The shutter offset file in
 * isis 2 is rotated 90 degrees.
 *
 * Some of the values and equations in the program were verified
 *   using the book "In Orbit at Jupiter, Contributions of the Galileo
 *   Science Team," Section H Part I.
 */


namespace Isis {
  
  // Globals
  static vector<double> weight;
  static double scaleFactor0; // Entire scale factor that e is multiplied by
  static double scaleFactor; // A1 or A2 in the equation, depending on units
  static double dcScaleFactor;
  static double exposureDuration;
  static bool eightBitDarkCube; // Is the dark cube of type unsigned byte?
  static bool iof; // Determines output units (i.e. I/F or radiance)
  static double s1;
  static double s2;
  static double rsun;
  static double cubeConversion;
  static double gainConversion;
  
  static void Calibrate(vector<Buffer *> &in, vector<Buffer *> &out);
  static FileName FindDarkFile(Cube *icube);
  static FileName FindGainFile(Cube *icube);
  static FileName FindShutterFile(Cube *icube);
  static FileName ReadWeightTable(Cube *icube);
  static FileName GetScaleFactorFile();
  static int getGainModeID(Cube *icube);
  static void calculateScaleFactor0(Cube *icube, Cube *gaincube);
  
  void gllssical(UserInterface &ui, Pvl *log) {
    Cube icube(ui.GetCubeName("FROM"));
    gllssical(&icube, ui, log);
  }   

  void gllssical(Cube *icube, UserInterface &ui, Pvl *log) {
    // Initialize Globals
    weight.clear();
    dcScaleFactor = 0.0;
    exposureDuration = 0.0;
    
    CubeAttributeInput &atts = ui.GetInputAttribute("FROM");
    if(atts.bands().size() != 0) {
      vector<QString> bands = atts.bands();
      icube->setVirtualBands(bands);
    }
    
    // Set up our ProcessByLine
    ProcessByLine p;
    p.SetInputCube(icube);
  
    Isis::CubeAttributeInput inAtt1;
    FileName darkFileName = FindDarkFile(icube);
    Cube *darkcube = p.SetInputCube(darkFileName.expanded(), inAtt1);
    dcScaleFactor = toDouble(darkcube->group("Instrument")["PicScale"][0]);
  
    Isis::CubeAttributeInput inAtt2;
    FileName gainFileName = FindGainFile(icube);
    Cube *gaincube = p.SetInputCube(gainFileName.expanded(), inAtt2);
  
    Isis::CubeAttributeInput inAtt3;
    p.SetInputCube(FindShutterFile(icube).expanded(), inAtt3, Isis::AllMatchOrOne);
  
    Cube *ocube = p.SetOutputCubeStretch("TO", &ui);
  
    (ui.GetString("UNITS") == "IOF") ? iof = true : iof = false;
    scaleFactor = ui.GetDouble("SCALE");
  
    if(ui.GetBoolean("BITWEIGHTING")) {
      ReadWeightTable(icube);
    }
    else {
      for(int i = 0; i < 256; i++) {
        weight.push_back(i);
      }
    }
  
    calculateScaleFactor0(icube, gaincube);
  
    exposureDuration = toDouble(icube->group("Instrument")["ExposureDuration"][0]) * 1000;
  
    if(darkcube->pixelType() == Isis::UnsignedByte) {
      eightBitDarkCube = true;
    }
    else {
      eightBitDarkCube = false;
    }
    p.StartProcess(Calibrate);
    PvlGroup calibrationLog("RadiometricCalibration");
    calibrationLog.addKeyword(PvlKeyword("From", ui.GetCubeName("FROM")));
  
    FileName shutterFileName = FindShutterFile(icube);
    calibrationLog.addKeyword(PvlKeyword("DarkCurrentFile", darkFileName.originalPath() + "/" +
                                         darkFileName.name()));
    calibrationLog.addKeyword(PvlKeyword("GainFile", gainFileName.originalPath() + "/" +
                                         gainFileName.name()));
    calibrationLog.addKeyword(PvlKeyword("ShutterFile", shutterFileName.originalPath() + "/" +
                                         shutterFileName.name()));
    calibrationLog.addKeyword(PvlKeyword("ScaleFactor", toString(scaleFactor)));
    calibrationLog.addKeyword(PvlKeyword("OutputUnits", iof ? "I/F" : "Radiance"));
    if (iof) {
      calibrationLog.addKeyword(PvlKeyword("S1", toString(s1), "I/F per Ft-Lambert"));
      calibrationLog.addKeyword(PvlKeyword("RSUN", toString(rsun), "(Planet-Sun range)/5.2 A.U."));
      calibrationLog.addKeyword(PvlKeyword("Scale", toString(scaleFactor), "I/F units per DN"));
      calibrationLog.addKeyword(PvlKeyword("GC", toString(cubeConversion), "Cube gain conversion"));
      calibrationLog.addKeyword(PvlKeyword("GG", toString(gainConversion), "Gain file gain conversion"));
      calibrationLog.addKeyword(PvlKeyword("IOF-SCALE0", toString(scaleFactor0), "(S1/Scale)*(GC/GG)/RSUN**2"));
    }
    else {
      calibrationLog.addKeyword(PvlKeyword("S2", toString(s2), "Nanowatts per Ft-Lambert"));
      calibrationLog.addKeyword(PvlKeyword("Scale", toString(scaleFactor),
                                           "Nanowatts/cm**2/steradian/nanometer/DN"));
      calibrationLog.addKeyword(PvlKeyword("GC", toString(cubeConversion), "Cube gain conversion"));
      calibrationLog.addKeyword(PvlKeyword("GG", toString(gainConversion), "Gain file gain conversion"));
      calibrationLog.addKeyword(PvlKeyword("Radiance-SCALE0", toString(scaleFactor0), "(S2/Scale)*(GC/GG)"));
    }
  
    ocube->putGroup(calibrationLog);

    Application::Log(calibrationLog);

    p.EndProcess();
  }
  
  
  void Calibrate(vector<Buffer *> &in, vector<Buffer *> &out) {
    Buffer &inputFile   = *in[0];
    Buffer &darkFile    = *in[1];
    Buffer &gainFile    = *in[2];
    Buffer &shutterFile = *in[3];
    Buffer &outputFile  = *out[0];
  
    /*
     * Calculate this part of the radiometric correction equation:
     *
     * scaleFactor0 / (t-to)
     */
    double scale = scaleFactor0 / (exposureDuration - shutterFile[0]);
  
    for(int samp = 0; samp < inputFile.size(); samp++) {
      // Some shutter files are only a single sample. Others may match the number
      // of samples in the cube.
      int shutterIndex = (shutterFile.size() == 1) ? 0 : samp;
  
      // Don't do anything to special pixels.
      if(IsSpecial(inputFile[samp])) {
        outputFile[samp] = inputFile[samp];
        continue;
      }
  
      if(IsSpecial(darkFile[samp]) || IsSpecial(gainFile[samp]) || IsSpecial(shutterFile[shutterIndex])) {
        outputFile[samp] = Isis::Null;
        continue;
      }
  
      /*
       * Calculate this part of the equation:
       *   e = z(d - dc)
       */
      double dn = weight[(unsigned long)(inputFile[samp])];
  
      double dc;
      if (eightBitDarkCube) {
        dc = weight[(unsigned long)(darkFile[samp])];
      }
      else {
        dc = (1.0 / dcScaleFactor) * darkFile[samp];
      }
  
      double e = gainFile[samp] * (dn - dc);
      double r = e * scale;
  
      // The following behavior does not match the Isis 2 version of this app.
      // In the Isis 2 version, negative I/F were kept in the output, which
      // doesn't make sense.
      if(r >= 0 || !iof) {
        outputFile[samp] = r;
      }
      else {
        outputFile[samp] = Isis::Lrs;
      }
    }
  }
  
  FileName FindDarkFile(Cube *icube) {
    QString file = "$galileo/calibration/gll_dc.sav";
  
    TextFile darkFile(file);
    darkFile.SetComment("C");
    QString data;
  
    /**
     * The dark current table requires the following information to match:
     *   Mission,Frame Mode ID,Gain State ID,Frame Rate ID,Extended Exposure,Readout Mode,Image Number
     * So let's grab the information we need from the image labels first
     */
    int gainModeId = getGainModeID(icube);
  
    int frameRateId = 0;
    /**
     * Frame rate code
     * 1 = 2 1/3 (sec)
     * 2 = 8 2/3
     * 3 = 30 1/3
     * 4 = 60 2/3
     * 5 = 15 1/6
     */
    if((int)(toDouble(icube->group("Instrument")["FrameDuration"][0])) == 2) frameRateId = 1;
    if((int)(toDouble(icube->group("Instrument")["FrameDuration"][0])) == 8) frameRateId = 2;
    if((int)(toDouble(icube->group("Instrument")["FrameDuration"][0])) == 30) frameRateId = 3;
    if((int)(toDouble(icube->group("Instrument")["FrameDuration"][0])) == 60) frameRateId = 4;
    if((int)(toDouble(icube->group("Instrument")["FrameDuration"][0])) == 15) frameRateId = 5;
  
    int exposureTypeId = (icube->group("Instrument")["ExposureType"][0] == "NORMAL") ? 0 : 1;
  
    // We have what we need from the image label, now go through the text file that is our table line by line
    // looking for a match.
    while(darkFile.GetLine(data)) {
      data = data.simplified().trimmed();
  
      QStringList tokens = data.split(" ");
      QString mission = (tokens.count()? tokens.takeFirst() : "");
      if(mission != "GALILEO") {
        continue;
      }
  
      QString frameMode = (tokens.count()? tokens.takeFirst() : "");
      if(frameMode.at(0) != icube->group("Instrument")["FrameModeId"][0].at(0)) {
        continue;
      }
  
      QString gainState = (tokens.count()? tokens.takeFirst() : "");
      if(toInt(gainState) != gainModeId) {
        continue;
      }
  
      QString frameRate = (tokens.count()? tokens.takeFirst() : "");
      if(frameRateId != toInt(frameRate)) {
        continue;
      }
  
      QString tableExposureTypeId = (tokens.count()? tokens.takeFirst() : "");
      if(toInt(tableExposureTypeId) != exposureTypeId) {
        continue;
      }
  
      QString readout = (tokens.count()? tokens.takeFirst() : "");
      if(readout.at(0) != icube->group("Instrument")["ReadoutMode"][0].at(0)) {
        continue;
      }
  
      int minImageNum = toInt(tokens.takeFirst());
      int maxImageNum = toInt(tokens.takeFirst());
  
      int imageNumber = (int)(toDouble(icube->group("Instrument")["SpacecraftClockStartCount"]) * 100 + 0.5);
      QString telemetry = icube->group("Instrument")["TelemetryFormat"][0];
      if(imageNumber > 99757701 && imageNumber < 159999999) {
        if((telemetry == "AI8" && (gainState == "1" || gainState == "2")) ||
            (telemetry == "IM4" && (gainState == "3" || gainState == "4"))) {
          imageNumber = 160000001;
        }
        else {
          imageNumber = 1;
        }
      }
  
      if(imageNumber < minImageNum || imageNumber > maxImageNum) {
        continue;
      }
  
      // By process of elimination, we found the dark current file successfully. Return it.
      return FileName("$galileo/calibration/darkcurrent/" + tokens.takeFirst() + ".cub");
    }
  
    throw IException(IException::Unknown, "Dark current file could not be determined.", _FILEINFO_);
  }
  
  FileName FindGainFile(Cube *icube) {
    QString file = "$galileo/calibration/gll_gain.sav";
  
    TextFile gainFile(file);
    gainFile.SetComment("C");
    QString data;
  
    int imageNumber = (int)((double)icube->group("Instrument")["SpacecraftClockStartCount"] * 100 + 0.5);
  
    while(gainFile.GetLine(data)) {
      data = data.simplified().trimmed();
  
      QStringList tokens = data.split(" ");
  
      QString mission = tokens.count()? tokens.takeFirst() : "";
      if(mission != "GALILEO") {
        continue;
      }
  
      /**
       * Filter codes
       * 0=clear,1=green,2=red,3=violet,4=7560,5=9680,6=7270,7=8890
       */
      QString filter = icube->group("BandBin")["FilterNumber"][0];
      if(filter != tokens.takeFirst()) {
        continue;
      }
  
      QString frameMode = tokens.count()? tokens.takeFirst() : "";
      if(frameMode.at(0) != icube->group("Instrument")["FrameModeId"][0].at(0)) {
        continue;
      }
  
      int minImageNum = toInt(tokens.takeFirst());
      int maxImageNum = toInt(tokens.takeFirst());
      if(imageNumber < minImageNum || imageNumber > maxImageNum) {
        continue;
      }
  
      return FileName("$galileo/calibration/gain/" + tokens.takeFirst() + ".cub");
    }
  
    throw IException(IException::Unknown, "Gain file could not be determined.", _FILEINFO_);
  }
  
  FileName ReadWeightTable(Cube *icube) {
    QString file = "$galileo/calibration/weightTables_v???.sav";
  
    FileName weightFile(file);
    weightFile = weightFile.highestVersion();
    Pvl weightTables(weightFile.expanded());
    QString group = QString("FrameMode") + icube->group("Instrument")["FrameModeId"][0].at(0);
    PvlGroup &frameGrp = weightTables.findGroup(group);
    QString keyword = QString("GainState") + ((getGainModeID(icube) < 3) ? QString("12") : QString("34"));
  
    for(int i = 0; i < frameGrp[keyword].size(); i++) {
      weight.push_back(toDouble(frameGrp[keyword][i]));
    }
  
    return weightFile;
  }
  
  int getGainModeID(Cube *icube) {
    int gainModeId = 0;
    /**
     * Gain mode ID code
     * 1 = 400,000
     * 2 = 100,000
     * 3 = 40,000
     * 4 = 10,000
     */
    if((int)toDouble(icube->group("Instrument")["GainModeId"][0]) == 4E5) {
      gainModeId = 1;
    }
    else if((int)toDouble(icube->group("Instrument")["GainModeId"][0]) == 1E5) {
      gainModeId = 2;
    }
    else if((int)toDouble(icube->group("Instrument")["GainModeId"][0]) == 4E4) {
      gainModeId = 3;
    }
    else if((int)toDouble(icube->group("Instrument")["GainModeId"][0]) == 1E4) {
      gainModeId = 4;
    }
    else {
      throw IException(IException::Unknown,
                       "Invalid value for Gain Mode ID [" +
                       icube->group("Instrument")["GainModeId"][0] +
                       "].", _FILEINFO_);
    }
  
    return gainModeId;
  }
  
  /*
   * Calculates scaleFactor0, which is:
   *
   *         S1       K
   *      -------- * --- (D/5.2)**2
   *         A1       Ko
   *
   * if output units are in I/F, or:
   *
   *         S2       K
   *      -------- * ---
   *         A2       Ko
   *
   * if output units are in radiance.
   */
  void calculateScaleFactor0(Cube *icube, Cube *gaincube) {
    Pvl conversionFactors(GetScaleFactorFile().expanded());
    PvlKeyword fltToRef, fltToRad;
  
    for(int grp = 0; grp < conversionFactors.groups(); grp++) {
      PvlGroup currGrp = conversionFactors.group(grp);
  
      // Match target name
      if(currGrp.hasKeyword("TargetName")) {
        if(!icube->group("Archive")["CalTargetCode"][0].startsWith(currGrp["TargetName"][0])) {
          continue;
        }
      }
  
      // Match MinimumEncounter
      if(currGrp.hasKeyword("MinimumTargetName")) {
        try {
          if((int)currGrp["MinimumTargetName"] >
              (int)toInt(icube->group("Archive")["CalTargetCode"][0].mid(0, 2))) {
            continue;
          }
        }
        catch(IException &) {
          continue;
        }
      }
  
      fltToRef = currGrp["FloatToRef"];
      fltToRad = currGrp["FloatToRad"];
    }
  
    int filterNumber = toInt(icube->group("BandBin")["FilterNumber"][0]);
  
    if(fltToRef.size() == 0) {
      throw IException(IException::Unknown,
                       "Unable to find matching reflectance and radiance values for target [" +
                       icube->group("Instrument")["TargetName"][0] + "] in [" +
                       GetScaleFactorFile().expanded() + "]",
                       _FILEINFO_);
    }
  
    s1 = toDouble(fltToRef[filterNumber]);
    s2 = toDouble(fltToRad[filterNumber]);
    cubeConversion = toDouble( conversionFactors["GainRatios"][getGainModeID(icube)-1]);
    gainConversion = toDouble(conversionFactors["GainRatios"][getGainModeID(gaincube)-1]);
  
    if (iof) {
      try {
        Camera *cam;
        cam = icube->camera();
        cam->instrumentPosition()->SetAberrationCorrection("LT+S");
        // Set time to the starting time of the image by setting image.
        cam->SetImage(0.5, 0.5);
  
        // rsun converted to AU
        rsun = cam->sunToBodyDist() / 1.49597870691E8 / 5.2;
      } 
      catch (IException &e) {
        // try original fallback for previously spiceinited data 
        try {
          Pvl *label = icube->label();
          QString startTime = label->findGroup("Instrument",Pvl::Traverse)["SpacecraftClockStartCount"][0];
  
          Spice spicegll(*icube);
          spicegll.instrumentPosition()->SetAberrationCorrection("LT+S");
          Isis::FileName sclk(label->findGroup("Kernels",Pvl::Traverse)["SpacecraftClock"][0]);
          QString sclkName(sclk.expanded());
  
          NaifStatus::CheckErrors();
          furnsh_c(sclkName.toLatin1().data());
          NaifStatus::CheckErrors();
  
          double obsStartTime;
          scs2e_c(-77, startTime.toLatin1().data(), &obsStartTime);
          spicegll.setTime(obsStartTime);
          double sunv[3];
          spicegll.sunPosition(sunv);
  
          double sunkm = vnorm_c(sunv);
          
          //  Convert to AU units
          rsun = sunkm / 1.49597870691E8 / 5.2;
        } 
        catch (IException &e) {
          QString message = "IOF option does not work with non-spiceinited cubes.";
          throw IException(e, IException::User, message, _FILEINFO_);
        }
      }
          
     /*
      * We are calculating I/F, so scaleFactor0 is:
      *
      *         S1       K
      *      -------- * --- (D/5.2)**2
      *         A1       Ko
      */
      scaleFactor0 = (s1 * (cubeConversion / gainConversion) * pow(rsun, 2)) / (scaleFactor);
    }
    else {
      /*
       * We are calculating radiance, so scaleFactor0 is:
       *
       *         S2       K
       *      -------- * ---
       *         A2       Ko
       */
      scaleFactor0 = (s2 / scaleFactor) * (cubeConversion / gainConversion);
    }
  }
  
  FileName GetScaleFactorFile() {
    QString file = "$galileo/calibration/conversionFactors_v???.sav";
    FileName scaleFactor(file);
    scaleFactor = scaleFactor.highestVersion();
    return scaleFactor;
  }
  
  FileName FindShutterFile(Cube *icube) {
    QString file = "$galileo/calibration/shutter/calibration.so02";
    file += icube->group("Instrument")["FrameModeId"][0].at(0);
    file += ".cub";
    FileName shutterFile(file);
    return shutterFile;
  }
}