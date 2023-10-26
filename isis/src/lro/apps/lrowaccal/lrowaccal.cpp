#include <vector>

#include <QDir>
#include <QRegExp>
#include <QString>

#include "Brick.h"
#include "Camera.h"
#include "Constants.h"
#include "CubeAttribute.h"
#include "iTime.h"
#include "Message.h"
#include "NaifStatus.h"
#include "Preference.h"
#include "ProcessByBrick.h"
#include "PvlGroup.h"
#include "SpecialPixel.h"
#include "Statistics.h"

#include "lrowaccal.h"

#define POLAR_MODE_SAMPLES 1024
#define NO_POLAR_MODE_SAMPLES 704
#define BW_BANDS 1
#define VIS_LINES 14
#define COLOR_BANDS 5
#define UV_SAMPLES 128
#define UV_LINES 4
#define UV_BANDS 2
#define KM_PER_AU 149597871

using namespace std;
using namespace Isis;


namespace Isis {
  /**
   * @brief Calibrate a WAC cube. 
   *
   * This is the programmatic interface to the lrowaccal application.
   * 
   * @param ui the User Interface to parse the parameters from
   */
  void lrowaccal(UserInterface &ui) {
    Cube *icube = NULL;
    icube = new Cube();
    CubeAttributeInput inAtt = ui.GetInputAttribute("FROM");
    if (inAtt.bands().size() != 0) {
      icube->setVirtualBands(inAtt.bands());
    }
    icube->open(ui.GetCubeName("FROM"));

    lrowaccal(icube, ui);

    delete icube;
    icube = NULL;
  }

  /**
   * @brief Calibrate a WAC cube.
   *
   * This is the programmatic interface to the lrowaccal application.
   * 
   * @param cube input cube to be calibrated
   * @param ui the User Interface to parse the parameters from
   */
  void lrowaccal(Cube *icube, UserInterface &ui) {
    /**
    * Structure for storing list of available dark file temps/times.
    */
    struct DarkFileInfo {
      double temp;
      int time;

      DarkFileInfo(double temp, int time)
      {
        this->temp = temp;
        this->time = time;
      }
    };

    /**
    * @brief DarkFileInfo comparison object.
    *
    * Used for sorting DarkFileInfo objects. Sort first by difference from WAC temp, then difference from WAC time
    */
    struct DarkComp {
      double wacTemp;
      int wacTime;

      DarkComp(double wacTemp, int wacTime)
      {
        this->wacTemp = wacTemp;
        this->wacTime = wacTime;
      }

      // sort dark files by distance from wac temp, then distance from wac time
      bool operator() (const DarkFileInfo &A, const DarkFileInfo &B) {
        if (abs(wacTemp - A.temp) < abs(wacTemp - B.temp)) return true;
        if (abs(wacTemp - A.temp) > abs(wacTemp - B.temp)) return false;
        if (abs(wacTime - A.time) < abs(wacTime - B.time)) return true;
        return false;
      }
    };
    
    vector<double> g_iofResponsivity;
    vector<double> g_radianceResponsivity;
    double g_TempratureConstants[7][2];
    for (int b = 0; b < 7; b++){
      g_TempratureConstants[b][0] = 0;
      g_TempratureConstants[b][1] = 0;
    }

    bool g_dark = true, g_flatfield = true, g_radiometric = true, g_iof = true, g_specpix = true, g_temprature = true;

    double g_exposure = 1.0; //!< Exposure duration
    double g_solarDistance = 1.01; //!< average distance in [AU]
    double g_startTemperature, g_endTemperature;
    double g_temp1 = 0.0, g_temp2 = 0.0;

    int g_numFrames = 0;

    vector<int> g_bands;

    Buffer *g_darkCube1 = NULL, *g_darkCube2 = NULL, *g_flatCube = NULL, *g_specpixCube = NULL;
    
    g_dark = ui.GetBoolean("DARK");
    g_flatfield = ui.GetBoolean("FLATFIELD");
    g_radiometric = ui.GetBoolean("RADIOMETRIC");
    g_iof = (ui.GetString("RADIOMETRICTYPE") == "IOF");
    g_specpix = ui.GetBoolean("SPECIALPIXELS");
    g_temprature = ui.GetBoolean("TEMPERATURE");

    vector<QString> darkFiles;
    ui.GetAsString("DARKFILE", darkFiles);
    QString flatFile = ui.GetAsString("FLATFIELDFILE");
    QString radFile = ui.GetAsString("RADIOMETRICFILE");
    QString specpixFile = ui.GetAsString("SPECIALPIXELSFILE");
    QString tempFile = ui.GetAsString("TEMPERATUREFILE");

    //
    // Helper functions
    //
    auto min = [](double a, double b) -> double {
      return (a < b) ? a : b;
    };

    auto CopyCubeIntoBuffer = [](QString &fileString, Buffer* &data) -> void {
      Cube cube;
      FileName filename(fileString);
      if (filename.isVersioned())
        filename = filename.highestVersion();
      if (!filename.fileExists()) {
        QString msg = fileString + " does not exist.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      cube.open(filename.expanded());
      Brick brick(cube.sampleCount(), cube.lineCount(), cube.bandCount(), cube.pixelType());
      brick.SetBasePosition(1, 1, 1);
      cube.read(brick);

      data = NULL;
      data = new Buffer(brick);

      fileString = filename.expanded();
    };

    /**
    * Finds 2 best dark files for WAC calibration.
    *
    * GetDark will find the 2 closest available dark file tempuratures matching the given file name
    * pattern. Then find the dark file at each tempurature with the time closest to the WAC tempurature.
    * If there is only one tempurature, it will pick the 2 closest times at that tempurature.
    *
    *
    * @param fileString String pattern defining dark files to search (ie. lro/calibration/wac_darks/WAC_COLOR_Offset68_*C_*T_Dark.????.cub)
    * @param temp Tempurature of WAC being calibrated
    * @param time Time of WAC being calibrated
    * @param data1 Buffer to hold dark file 1 cub data
    * @param data2 Buffer to hold dark file 2 cub data
    * @param temp1 Tempurature of dark file 1
    * @param temp2 Tempurature of dark file 2
    * @param file1 Filename of dark file 1
    * @param file2 Filename of dark file 2
    */
    auto GetDark = [CopyCubeIntoBuffer](QString fileString, double temp, double time, Buffer* &data1, Buffer* &data2,
                      double & temp1, double & temp2, QString & file1, QString & file2) -> void {
      FileName filename(fileString);
      QString basename = FileName(filename.baseName()).baseName(); // We do it twice to remove the ".????.cub"

      // create a regular expression to capture the temp and time from filenames
      QString regexPattern(basename);
      regexPattern.replace("*", "([0-9\\.-]*)");
      QRegExp regex(regexPattern);

      // create a filter for the QDir to only load files matching our name
      QString filter(basename);
      filter.append(".*");

      // get a list of dark files that match our basename
      QDir dir(filename.path(), filter);

      vector<DarkFileInfo> darkFiles;
      darkFiles.reserve(dir.count());

      // Loop through all files in the dir that match our basename and extract time and temp
      for (unsigned int indx=0; indx < dir.count(); indx++) {
        // match against our regular expression
        int pos = regex.indexIn(dir[indx]);
        if (pos == -1) {
          continue; // filename did not match basename regex (time or temp contain non-digit)
        }

        // Get a list of regex matches. Item 0 should be the full QString, item 1
        // is temp and item 2 is time.
        QStringList texts = regex.capturedTexts();
        if (texts.size() < 3) {
          continue; // could not find time and/or temp
        }

        // extract time/temp from regex texts
        bool tempOK, timeOK;
        double fileTemp = texts[1].toDouble(&tempOK);
        int fileTime = texts[2].toInt(&timeOK);
        if (!tempOK || !timeOK) {
          continue; // time or temp was not a valid numeric value
        }

        DarkFileInfo info(fileTemp, fileTime);
        darkFiles.push_back(info);
      }

      // we require at least 2 different dark files to interpolate/extrapolate
      if (darkFiles.size() < 2) {
        QString msg = "Not enough Dark files exist for these image options [" + basename + "]. Need at least 2 files with different temperatures\n";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      // sort the files by distance from wac temp and time
      DarkComp darkComp(temp, (int)time);
      sort(darkFiles.begin(), darkFiles.end(), darkComp);

      size_t temp1Index = 0;
      size_t temp2Index;

      temp1 = darkFiles[temp1Index].temp;

      for (temp2Index = temp1Index+1; temp2Index < darkFiles.size(); temp2Index++) {
        if (darkFiles[temp2Index].temp != temp1) {
          break;
        }
      }

      if (temp2Index >= darkFiles.size()) {
        temp2Index = 1;
      }

      temp2 = darkFiles[temp2Index].temp;

      int time1 = darkFiles[temp1Index].time;
      int time2 = darkFiles[temp2Index].time;

      int tempIndex = fileString.indexOf("*C");
      int timeIndex = fileString.indexOf("*T");

      file1 = fileString;
      file1.replace(timeIndex, 1, toString(time1));
      file1.replace(tempIndex, 1, toString((int)temp1));

      file2 = fileString;
      file2.replace(timeIndex, 1, toString(time2));
      file2.replace(tempIndex, 1, toString((int)temp2));

      CopyCubeIntoBuffer(file1, data1);
      CopyCubeIntoBuffer(file2, data2);
    };

    auto GetMask = [CopyCubeIntoBuffer](QString &fileString, double temp, Buffer* &data) {
      FileName filename(fileString);
      QString basename = FileName(filename.baseName()).baseName(); // We do it twice to remove the ".????.cub"

      unsigned int index = basename.indexOf("*");

      // create a filter for the QDir to only load files matching our name
      QString filter(basename);
      filter.append(".*");

      QDir dir(filename.path(), filter);

      // create a regular expression to capture the temp and time from filenames
      QString regexPattern(basename);
      regexPattern.replace("*", "([0-9\\.-]*)");
      QRegExp regex(regexPattern);

      double bestTemp = DBL_MAX;
      for (unsigned int indx=0; indx < dir.count(); indx++) {
        // match against our regular expression
        int pos = regex.indexIn(dir[indx]);
        if (pos == -1) {
          continue; // filename did not match basename regex (temp contain non-digit)
        }

        // Get a list of regex matches. Item 0 should be the full QString, item 1 is temp
        QStringList texts = regex.capturedTexts();
        if (texts.size() < 2) {
          continue; // could not find temp
        }

        // extract time/temp from regex texts
        bool tempOK;
        double fileTemp = texts[1].toDouble(&tempOK);
        if (!tempOK) {
          continue; // temp was not a valid numeric value
        }

        if (abs(temp - fileTemp) < abs(temp - bestTemp)) {
          bestTemp = fileTemp;
        }
      }

      if (bestTemp == DBL_MAX) {
        QString msg = "No files exist for these mask options [" + basename + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      index = fileString.indexOf("*");
      fileString.replace(index, 1, toString((int)bestTemp));

      CopyCubeIntoBuffer(fileString, data);
    };

    // Calibrate each framelet
    auto Calibrate = [&](Buffer &inCube, Buffer &outCube) -> void {
      int correctBand = -1;
      //If we are passed in a single band (img.cub+4) we need to pay special attention that we don't start with band1
      if (inCube.BandDimension() == 1 && g_bands.size() == 1)
        correctBand = g_bands.front();

      int frameHeight = inCube.LineDimension();
      int frameSize = inCube.SampleDimension()*inCube.LineDimension();
      int frame = inCube.Line() / frameHeight;

      // Calculate a temperature factor for the current frame (this is done to avoid doing this for each pixel
      // Used in dark and temprature correction
      // frameTemp:
      //
      //    (WAC end temp - WAC start temp)
      //    -------------------------------   *   frame   +   WAC start temp
      //         (WAC num framelets)
      double frameTemp = (g_endTemperature - g_startTemperature)/g_numFrames * frame + g_startTemperature;

      for (int i = 0; i < outCube.size(); i++)
        outCube[i] = inCube[i];

      if (g_dark) {
        double tempFactor = (frameTemp - g_temp2) / (g_temp1-g_temp2);

        for (int b=0; b<inCube.BandDimension(); b++) {
          // We find the index of the corresponding dark frame band as the offset
          int offset;
          if (correctBand != -1)
            offset = g_darkCube1->Index(1, frameHeight * (int) min(frame, g_darkCube1->LineDimension()/frameHeight - 1) + 1, correctBand);
          else
            offset = g_darkCube1->Index(1, frameHeight * (int) min(frame, g_darkCube1->LineDimension()/frameHeight - 1) + 1, b+1);

          // We're bypassing Buffer::at for speed, so we need to make sure our
          // index will not overrun the buffer
          if(offset + frameSize > g_darkCube1->size()) {
            std::string message = Message::ArraySubscriptNotInRange(offset + frameSize) + " (Dark cube 1)";
            throw IException(IException::Programmer, message, _FILEINFO_);
          }
          if(offset + frameSize > g_darkCube2->size()) {
            std::string message = Message::ArraySubscriptNotInRange(offset + frameSize) + " (Dark cube 2)";
            throw IException(IException::Programmer, message, _FILEINFO_);
          }

          for (int i = 0; i < frameSize; i++) {
            double dark1Pixel = (*g_darkCube1)[offset + i];
            double dark2Pixel = (*g_darkCube2)[offset + i];
            double &outputPixel = outCube[i + b*frameSize];
            // Interpolate between the two darks with the current temperaturube1
            if (!IsSpecial(dark1Pixel) && !IsSpecial(dark2Pixel) && !IsSpecial(outputPixel)) {
              if (g_temp1 != g_temp2) {
                // Dark correction formula:
                //
                //    (dark1Pixel - dark2Pixel)
                //    -------------------------   *   (frameTemp - dark2Temp)   +   dark2Pixel
                //     (dark1Temp - dark2Temp)
                //
                // frameTemp:
                //
                //    (WAC end temp - WAC start temp)
                //    -------------------------------   *   frame   +   WAC start temp
                //         (WAC num framelets)
                //
                // tempFactor (calculated outside the loops for speed):
                //
                //    (frameTemp - dark2Temp)
                //    -----------------------
                //    (dark1Temp - dark2Temp)
                //
                outputPixel -= (dark1Pixel - dark2Pixel) * tempFactor + dark2Pixel;
              }
              else {
                outputPixel -= dark1Pixel;
              }
            }
            else {
              outputPixel = Isis::Null;
            }
          }
        }
      }

      if (g_flatfield) {
        for (int b=0; b<inCube.BandDimension(); b++) {
          // We find the index of the corresponding flat frame band as the offset
          int offset;
          if (correctBand != -1)
            offset = g_flatCube->Index(1, frameHeight * (int) min(frame, (g_flatCube->LineDimension()-1) / frameHeight)+1, correctBand);
          else
            offset = g_flatCube->Index(1, frameHeight * (int) min(frame, (g_flatCube->LineDimension()-1) / frameHeight)+1, b+1);

          // We're bypassing Buffer::at for speed, so we need to make sure our
          // index will not overrun the buffer
          if(offset + frameSize > g_flatCube->size()) {
            std::string message = Message::ArraySubscriptNotInRange(offset + frameSize) + " (Flat-field cube)";
            throw IException(IException::Programmer, message, _FILEINFO_);
          }

          int outFrameOffset = b*frameSize;
          for (int i = 0; i < frameSize; i++) {
            double flatPixel = (*g_flatCube)[offset + i];
            double &outputPixel = outCube[i + outFrameOffset];

            if (flatPixel > 0.0 && !IsSpecial(flatPixel) && !IsSpecial(outputPixel))
              outputPixel *= flatPixel; // The flat-field data was inverted during load so we don't have to divide here.
            else
              outputPixel = Isis::Null;
          }
        }
      }

      if (g_radiometric) {
        for (int i = 0; i < outCube.size(); i++) {
          if (IsSpecial(outCube[i]))
            outCube[i] = Isis::Null;
          else {
            outCube[i] /= g_exposure;
            if (g_iof)
              outCube[i] *= pow(g_solarDistance, 2) / g_iofResponsivity[outCube.Band(i) - 1];
            else
              outCube[i] /= g_radianceResponsivity[outCube.Band(i) - 1];
          }
        }
      }

      if (g_specpix) {
        for (int b=0; b<inCube.BandDimension(); b++) {
          // We find the index of the corresponding flat frame band as the offset
          int offset;
          if (correctBand != -1)
            offset = g_specpixCube->Index(1, frameHeight * (int) min(frame, (g_specpixCube->LineDimension()-1) / frameHeight)+1, correctBand);
          else
            offset = g_specpixCube->Index(1, frameHeight * (int) min(frame, (g_specpixCube->LineDimension()-1) / frameHeight)+1, b+1);

          for (int i = 0; i < frameSize; i++) {
            if (IsSpecial(g_specpixCube->at(offset + i)))
              outCube[i+b*frameSize] = g_specpixCube->at(offset + i);
          }
        }
      }

      if (g_temprature) {
        for (int i = 0; i < outCube.size(); i++) {
          if (IsSpecial(outCube[i]))
            outCube[i] = Isis::Null;
          else {

            // Temprature Correction Formula
            //
            //       inputPixel
            //  ---------------------
            //    a*(frameTemp) + b
            //
            // Where:
            //  'a' and 'b' are band dependant constants read in via a pvl file
            //
            //  AND
            //
            // frameTemp: (Pre-Calculated as it is used in multiple places)
            //
            //    (WAC end temp - WAC start temp)
            //    -------------------------------   *   frame   +   WAC start temp
            //         (WAC num framelets)
            //
            //
            //
            if (correctBand != -1)
              outCube[i] = outCube[i]/ (g_TempratureConstants[correctBand][0] * frameTemp + g_TempratureConstants[correctBand][1]);
            else
              outCube[i] = outCube[i]/ (g_TempratureConstants[outCube.Band(i)][0] * frameTemp + g_TempratureConstants[outCube.Band(i)][1]);

          }
        }
      }
    };

    /**
    * This method returns a QString containing the path of an
    * LRO calibration directory
    *
    * @param calibrationType The type of calibration data
    *
    * @return @b QString Path of the calibration directory
    *
    * @internal
    *   @history 2008-11-05 Jeannie Walldren - Original version
    *   @history 2016-08-16 Victor Silva - Added option for base calibration directory
    */
    auto GetCalibrationDirectory = [](QString calibrationType) -> QString {
      // Get the directory where the CISS calibration directories are.
      PvlGroup &dataDir = Preference::Preferences().findGroup("DataDirectory");
      QString missionDir = (QString) dataDir["LRO"];
      if(calibrationType != "") {
        calibrationType += "/";
      }

      return missionDir + "/calibration/" + calibrationType;
    };
    

    //
    // Start processing code
    //
    ProcessByBrick p;
    p.SetInputCube(icube);

    // Make sure it is a WAC cube
    Isis::PvlGroup &inst = icube->label()->findGroup("Instrument", Pvl::Traverse);
    QString instId = (QString) inst["InstrumentId"];
    instId = instId.toUpper();
    if (instId != "WAC-VIS" && instId != "WAC-UV") {
      QString msg = "This program is intended for use on LROC WAC images only. [";
      msg += icube->fileName() + "] does not appear to be a WAC image.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // And check if it has already run through calibration
    if (icube->label()->findObject("IsisCube").hasGroup("Radiometry")) {
      QString msg = "This image has already been calibrated";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if (icube->label()->findObject("IsisCube").hasGroup("AlphaCube")) {
      QString msg = "This application can not be run on any image that has been geometrically transformed (i.e. scaled, rotated, sheared, or reflected) or cropped.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Determine the dark/flat files to use
    QString offset = (QString) inst["BackgroundOffset"];
    QString mode = (QString) inst["Mode"];
    QString instModeId = (QString) inst["InstrumentModeId"];
    instModeId = instModeId.toUpper();

    if (instModeId == "COLOR" && (QString) inst["InstrumentId"] == "WAC-UV")
      instModeId = "UV";
    else if (instModeId == "VIS")
      instModeId = "COLOR";

    g_startTemperature = (double) inst["BeginTemperatureFpa"];
    g_endTemperature = (double) inst["EndTemperatureFpa"];

    g_numFrames = (int) inst["NumFramelets"];

    // Figure out which bands are input
    for (int i = 1; i <= icube->bandCount(); i++) {
      g_bands.push_back(icube->physicalBand(i));
    }

    Isis::PvlGroup &bandBin = icube->label()->findGroup("BandBin", Pvl::Traverse);
    QString filter = (QString) bandBin["Center"][0];
    QString filterNum = (QString) bandBin["FilterNumber"][0];
    //We have to pay special attention incase we are passed a
    //single band image that has been "exploded" from a multiband wac
    if (instModeId == "COLOR" && g_bands.size() == 1)
      g_bands[0] = (toInt(filterNum) -2);
    else if (instModeId == "UV" && g_bands.size() == 1)
      g_bands[0] = (toInt(filterNum));

    if (g_dark) {
      if (darkFiles.size() == 0 || darkFiles[0] =="Default" || darkFiles[0].length() == 0) {
        darkFiles.resize(2);
        double temp = (double) inst["MiddleTemperatureFpa"];
        double time = iTime(inst["StartTime"][0]).Et();
        QString darkFile = GetCalibrationDirectory("wac_darks") + "WAC_" + instModeId;
        if (instModeId == "BW")
          darkFile += "_" + filter + "_Mode" + mode;
        darkFile += "_Offset" + offset + "_*C_*T_Dark.????.cub";
        GetDark(darkFile, temp, time, g_darkCube1, g_darkCube2, g_temp1, g_temp2, darkFiles[0], darkFiles[1]);
      }
      else if (darkFiles.size() == 1) {
        CopyCubeIntoBuffer(darkFiles[0], g_darkCube1);
        g_temp1 = 0.0;
        g_darkCube2 = new Buffer(*g_darkCube1);
        g_temp2 = g_temp1;
      }
      else {
        CopyCubeIntoBuffer(darkFiles[0], g_darkCube1);
        int index = darkFiles[0].lastIndexOf("_");
        g_temp1 = IString(darkFiles[0].mid(darkFiles[0].lastIndexOf("_", index-1), index)).ToDouble();
        CopyCubeIntoBuffer(darkFiles[1], g_darkCube2);
        index = darkFiles[1].lastIndexOf("_");
        g_temp2 = IString(darkFiles[1].mid(darkFiles[1].lastIndexOf("_", index-1), index)).ToDouble();
      }
    }

    if (g_flatfield) {
      if (flatFile.toLower() == "default" || flatFile.length() == 0) {
        flatFile = GetCalibrationDirectory("wac_flats") + "WAC_" + instModeId;
        if (instModeId == "BW")
          flatFile += "_" + filter + "_Mode" + mode;
        flatFile += "_Flatfield.????.cub";
      }
      CopyCubeIntoBuffer(flatFile, g_flatCube);

      // invert the flat-field data here so we don't have to divide for every pixel of the wac
      for (int i = 0; i < g_flatCube->size(); i++) {
        (*g_flatCube)[i] = 1.0 / (*g_flatCube)[i];
      }
    }

    PvlKeyword responsivity;

    if (g_radiometric) {

      Isis::PvlKeyword &bands = icube->label()->findGroup("BandBin", Pvl::Traverse).findKeyword("FilterNumber");

      if (radFile.toLower() == "default" || radFile.length() == 0)
        radFile = GetCalibrationDirectory("") + "WAC_RadiometricResponsivity.????.pvl";

      FileName radFileName(radFile);
      if (radFileName.isVersioned())
        radFileName = radFileName.highestVersion();
      if (!radFileName.fileExists()) {
        QString msg = radFile + " does not exist.";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      Pvl radPvl(radFileName.expanded());

      if (g_iof) {
        responsivity = radPvl["IOF"];

        for (int i = 0; i < bands.size(); i++) {
          g_iofResponsivity.push_back(toDouble(responsivity[toInt(bands[i]) - 1]));
        }

        try {
          Camera *cam = NULL;
          cam = icube->camera();
          iTime startTime((QString) inst["StartTime"]);
          cam->setTime(startTime);
          g_solarDistance = cam->sunToBodyDist() / KM_PER_AU;
        }
        catch(IException &e) {
          try {
            iTime startTime((QString) inst["StartTime"]);
            double etStart = startTime.Et();
            // Get the distance between the Moon and the Sun at the given time in
            // Astronomical Units (AU)
            QString bspKernel1 = p.MissionData("lro", "/kernels/tspk/moon_pa_de421_1900-2050.bpc", false);
            QString bspKernel2 = p.MissionData("lro", "/kernels/tspk/de421.bsp", false);
            NaifStatus::CheckErrors();
            furnsh_c(bspKernel1.toLatin1().data());
            NaifStatus::CheckErrors();
            furnsh_c(bspKernel2.toLatin1().data());
            NaifStatus::CheckErrors();
            QString pckKernel1 = p.MissionData("base", "/kernels/pck/pck?????.tpc", true);
            QString pckKernel2 = p.MissionData("lro", "/kernels/pck/moon_080317.tf", false);
            QString pckKernel3 = p.MissionData("lro", "/kernels/pck/moon_assoc_me.tf", false);
            NaifStatus::CheckErrors();
            furnsh_c(pckKernel1.toLatin1().data());
            NaifStatus::CheckErrors();
            furnsh_c(pckKernel2.toLatin1().data());
            NaifStatus::CheckErrors();
            furnsh_c(pckKernel3.toLatin1().data());
            NaifStatus::CheckErrors();
            double sunpos[6], lt;
            spkezr_c("sun", etStart, "MOON_ME", "LT+S", "MOON", sunpos, &lt);
            g_solarDistance = vnorm_c(sunpos) / KM_PER_AU;
            unload_c(bspKernel1.toLatin1().data());
            unload_c(bspKernel2.toLatin1().data());
            unload_c(pckKernel1.toLatin1().data());
            unload_c(pckKernel2.toLatin1().data());
            unload_c(pckKernel3.toLatin1().data());
          }
          catch (IException &e) {
            QString msg = "Can not find necessary SPICE kernels for converting to IOF";
            throw IException(e, IException::User, msg, _FILEINFO_);
          }
        }
      }
      else {
        responsivity = radPvl["Radiance"];
        for (int i = 0; i < bands.size(); i++)
          g_radianceResponsivity.push_back(toDouble(responsivity[toInt(bands[i]) - 1]));
      }
    }

    if (g_specpix) {
      if (specpixFile.toLower() == "default" || specpixFile.length() == 0) {
        specpixFile = GetCalibrationDirectory("wac_masks") + "WAC_" + instModeId;
        double temp = (double) inst["MiddleTemperatureFpa"];
        if (instModeId == "BW")
          specpixFile += "_" + filter + "_Mode" + mode;
        specpixFile += "_*C_SpecialPixels.????.cub";
        GetMask(specpixFile, temp, g_specpixCube);
      }
      else
        CopyCubeIntoBuffer(specpixFile, g_specpixCube);
    }

    PvlKeyword temperaturePvl("TemperatureFile");
    if (g_temprature) {
      if (tempFile.toLower() == "default" || tempFile.length() == 0)
        tempFile = GetCalibrationDirectory("") + "WAC_TempratureConstants.????.pvl";

      FileName tempFileName(tempFile);
      if (tempFileName.isVersioned())
        tempFileName = tempFileName.highestVersion();
      if (!tempFileName.fileExists()) {
        QString msg = tempFile + " does not exist.";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      Isis::PvlKeyword &bands = icube->label()->findGroup("BandBin", Pvl::Traverse).findKeyword("FilterNumber");
      Pvl tempPvl(tempFileName.expanded());
      temperaturePvl.addValue(tempFileName.expanded());
      for (int b = 0; b < bands.size(); b++){
        g_TempratureConstants[g_bands[b]][0]=toDouble(tempPvl[bands[b]][0]);
        g_TempratureConstants[g_bands[b]][1]=toDouble(tempPvl[bands[b]][1]);
      }
    }

    if (instModeId == "BW") {
      if (mode == "1" || mode == "0")
        p.SetBrickSize(NO_POLAR_MODE_SAMPLES, VIS_LINES, (int)min(BW_BANDS, g_bands.size()));
      else
        p.SetBrickSize(POLAR_MODE_SAMPLES, VIS_LINES, (int)min(BW_BANDS, g_bands.size()));
    }
    else if (instModeId == "COLOR") {
      p.SetBrickSize(NO_POLAR_MODE_SAMPLES, VIS_LINES, (int)min(COLOR_BANDS, g_bands.size()));
    }
    else if (instModeId == "UV") {
      p.SetBrickSize(UV_SAMPLES, UV_LINES, (int)min(UV_BANDS, g_bands.size()));
    }

    g_exposure = inst["ExposureDuration"];

    Cube *ocube = NULL;
    ocube = p.SetOutputCube(ui.GetCubeName("TO"), ui.GetOutputAttribute("TO"));
    p.ProcessCube(Calibrate, false);

    // Add an output group with the appropriate information
    PvlGroup calgrp("Radiometry");
    if (g_temprature) {
      calgrp += PvlKeyword("TemperatureFile",  temperaturePvl);
    }
    if (g_dark) {
      PvlKeyword darks("DarkFiles");
      darks.addValue(darkFiles[0]);
      if (darkFiles.size() > 1)
        darks.addValue(darkFiles[1]);
      calgrp += darks;
    }
    if (g_flatfield)
      calgrp += PvlKeyword("FlatFile", flatFile);
    if (g_radiometric) {
      PvlKeyword vals("ResponsivityValues");
      if (g_iof) {
        calgrp += PvlKeyword("RadiometricType", "IOF");
        for (unsigned int i=0; i< g_iofResponsivity.size(); i++)
          vals.addValue(std::to_string(g_iofResponsivity[i]));
      }
      else {
        calgrp += PvlKeyword("RadiometricType", "AbsoluteRadiance", "W/m2/sr/um");
        for (unsigned int i=0; i< g_radianceResponsivity.size(); i++)
          vals.addValue(std::to_string(g_radianceResponsivity[i]));
      }
      calgrp += vals;
      calgrp += PvlKeyword("SolarDistance", toString(g_solarDistance));
    }
    if (g_specpix)
      calgrp += PvlKeyword("SpecialPixelsFile", specpixFile);
    ocube->putGroup(calgrp);

    delete g_darkCube1;
    g_darkCube1 = NULL;
    delete g_darkCube2;
    g_darkCube2 = NULL;
    delete g_flatCube;
    g_flatCube = NULL;
    delete g_specpixCube;
    g_specpixCube = NULL;
  }
}


