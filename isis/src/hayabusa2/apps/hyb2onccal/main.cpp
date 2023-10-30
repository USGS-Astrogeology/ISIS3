/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

// $Id: hyb2onccal.cpp 6045 2015-02-07 02:06:59Z moses@GS.DOI.NET $
#include "Isis.h"

#include <vector>
#include <algorithm>
#include <memory>
#include <cstdio>
#include <cmath>

#include <QFile>
#include <QString>
#include <QScopedPointer>
#include <QTemporaryFile>
#include <QVector>

#include "AlphaCube.h"
#include "Buffer.h"
#include "FileName.h"
#include "Hyb2OncCalUtils.h"
#include "IException.h"
#include "iTime.h"
#include "LineManager.h"
#include "Pixel.h"
#include "ProcessByLine.h"
#include "ProcessBySample.h"
#include "ProcessByBrick.h"
#include "ProcessByBoxcar.h"
#include "ProgramLauncher.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "Spice.h"
#include "Statistics.h"
#include "TextFile.h"


using namespace Isis;
using namespace std;

// Calibration support routines
FileName DetermineFlatFieldFile(const QString &filter);
void Calibrate(vector<Buffer *>& in, vector<Buffer *>& out);

QString loadCalibrationVariables(const QString &config);

// Temporary cube file pointer deleter
struct TemporaryCubeDeleter {
  static inline void cleanup(Cube *cube) {
    if ( cube ) {

      FileName filename( cube->fileName() );
      delete cube;
      remove( filename.expanded().toLatin1().data() );
    }
  }
};


//For subimage and binning mapping
static AlphaCube *alpha(0);

QString g_filter = "";
static QString g_target ="";
static Pvl g_configFile;

//Bias calculation variables
static double g_b0(0);
static double g_b1(0);
static double g_b2(0);
static double g_bias(0);
static double g_AEtemperature(0);

static QString g_startTime;

//Dark Current variables
static double g_d0(0);
static double g_d1(0);
static double g_temp(0);
static double g_darkCurrent(0);

// TODO: we do not have the readout time (transfer period) for Hayabusa2 ONC.
//Smear calculation variables
// static double g_Tvct(0);       //!< Vertical charge-transfer period (in seconds).
static double g_texp(1);       //!< Exposure time.
// static double g_timeRatio(1.0);

// Calibration parameters
static int nsubImages(0);      //!< Number of sub images
static int binning(1);         //!< The number of samples/lines which are binned
static double g_compfactor(1.0);  // Default if OutputMode = LOSS-LESS; 16.0 for LOSSY

static QString g_iofCorrection("IOF");  //!< Is I/F correction to be applied?

//  I/F variables
static double g_solarDist(1.0);  /**< Distance from the Sun to the target body
(used to calculate g_iof) */
static double g_iof(1.0);        //!< I/F conversion value
static double g_iofScale(1.0);
static double g_solarFlux(1.0);  //!< The solar flux (used to calculate g_iof).
// TODO: we do not have this conversion factor for Hayabusa 2 ONC.
static double g_v_standard(1.0);
// static double g_v_standard(3.42E-3);//!< Base conversion for all filters (Tbl. 9)

void IsisMain() {

  UserInterface& ui = Application::GetUserInterface();
  // g_iofCorrection = ui.GetString("UNITS");

  const QString hyb2cal_program = "hyb2onccal";
  const QString hyb2cal_version = "1.0";
  const QString hyb2cal_revision = "$Revision$";
  QString hyb2cal_runtime = Application::DateTime();

  ProcessBySample p;

  Cube *icube = p.SetInputCube("FROM");

  // Basic assurances...
  if (icube->bandCount() != 1) {
    throw IException(IException::User,
      "ONC images may only contain one band", _FILEINFO_);
    }

  PvlGroup &inst = icube->group("Instrument");
  PvlGroup &bandbin = icube->group("BandBin");

  QString filter = QString::fromStdString(bandbin["FilterName"]);

  g_filter = filter;

  //Set up binning and image subarea mapping
  binning = inst["Binning"];
  int startLine = inst["SelectedImageAreaY1"];
  int startSample = inst["SelectedImageAreaX1"];
  int lastLine = inst["SelectedImageAreaY4"];
  int lastSample = inst["SelectedImageAreaX4"];

  AlphaCube myAlpha(1024,1024,icube->sampleCount(), icube->lineCount(),
  startSample,startLine,lastSample,lastLine);

  alpha = &myAlpha;

  try {
    g_texp = inst["ExposureDuration"] ;
  }
  catch(IException &e) {
    QString msg = "Unable to read [ExposureDuration] keyword in the Instrument group "
    "from input file [" + icube->fileName() + "]";
    throw IException(e, IException::Io,msg, _FILEINFO_);
  }

  QString instrumentId = QString::fromStdString(inst["InstrumentId"]);
  QString oncCCDTemperature = "ONC";
  try {
    oncCCDTemperature += instrumentId.section('-',1,1) + "CCDTemperature";
    g_temp = inst[oncCCDTemperature.toStdString()];
  }
  catch(IException &e) {
    QString msg = "Unable to read [" + oncCCDTemperature + "] keyword in the Instrument group "
    "from input file [" + icube->fileName() + "]";
    throw IException(e, IException::Io,msg, _FILEINFO_);

  }

  QString startTime = QString::fromStdString(inst["SpacecraftClockStartCount"]);

  g_startTime = startTime;

  nsubImages = inst["SubImageCount"];  // If > 1, some correction is not needed/performed

  QString compmode = QString::fromStdString(inst["Compression"]);
  // TODO: verify that the compression factor/scale is actually 16 for compressed Hayabusa2 images.
  g_compfactor = ( "lossy" == compmode.toLower() ) ? 16.0 : 1.0;

  QString target = QString::fromStdString(inst["TargetName"]);
  g_target = target;


  // NOTE we do not have a valid flat-field for the W1 or W2 images.
  FileName flatfile = "NONE";
  if (instrumentId == "ONC-T") {
    QScopedPointer<Cube, TemporaryCubeDeleter> flatcube;
    flatfile = DetermineFlatFieldFile(g_filter);

    QString reducedFlat(flatfile.expanded());

    // Image is not cropped
    if (startLine == 1 && startSample == 1) {

      // Determine if we need to subsample the flat field if pixel binning occurred
      // TODO: test a binned image (add test case).
      if (binning > 1) {
        QString scale(toString(binning));
        FileName newflat = FileName::createTempFile("$TEMPORARY/" +
        flatfile.baseName() + "_reduced.cub");
        reducedFlat = newflat.expanded();
        QString parameters = "FROM=" + flatfile.expanded() +
        " TO="   + newflat.expanded() +
        " MODE=SCALE" +
        " LSCALE=" + scale +
        " SSCALE=" + scale;

        try {
          ProgramLauncher::RunIsisProgram("reduce", parameters);
        }
        catch (IException& ie) {
          remove(reducedFlat.toLatin1().data());
          throw ie;
        }
        QScopedPointer<Cube, TemporaryCubeDeleter> reduced(new Cube(reducedFlat, "r"));
        flatcube.swap( reduced );
      }

      // Set up processing for flat field as a second input file
      CubeAttributeInput att;
      p.SetInputCube(reducedFlat, att);
    }
    else {
      // Image is cropped so we have to deal with it
      FileName transFlat =
      FileName::createTempFile("$TEMPORARY/" + flatfile.baseName() + "_translated.cub");

      Cube *flatOriginal = new Cube(flatfile.expanded() );

      int transform[5] = {binning,startSample,startLine,lastSample,lastLine};

      // Translates and scales the flatfield image.  Scaling
      // might be necessary in the event that the raw image was also binned.

      translate(flatOriginal,transform,transFlat.expanded());

      QScopedPointer<Cube, TemporaryCubeDeleter> translated(new Cube(transFlat.expanded(), "r"));
      flatcube.swap(translated);

      CubeAttributeInput att;
      std::cout<<" before second setinputcube()...\n"<<std::endl;
      p.SetInputCube(transFlat.expanded(),att);
      std::cout<<" after second setinputcube()...\n"<<std::endl;
    }
  }

  Cube *ocube  = p.SetOutputCube("TO");
  QString fname = ocube->fileName();

  QString calfile = loadCalibrationVariables(ui.GetAsString("CONFIG"));

  // TODO: we do not have g_Tvct (readout time for the ccd). Once we know this value for Hayabusa2
  // ONC and we know the smear model, we can correctly account for smear.
  //g_timeRatio = g_Tvct/(g_texp + g_Tvct);

  g_darkCurrent = g_d0 * exp(g_d1 *g_temp);
  std::cout << "\ndark current: " << g_darkCurrent << std::endl;

  g_iof = 1.0;  // Units of DN

  QString g_units = "DN";
  // if ( "radiance" == g_iofCorrection.toLower() ) {
  //   // Units of RADIANCE
  //   g_iof = g_iof * g_iofScale;
  //   g_units = "W / (m**2 micrometer sr)";
  // }
  //
  // if ( !sunDistanceAU(startTime, target, g_solarDist) ) {
  //    throw IException(IException::Programmer, "Cannot calculate distance to sun!",
  //                      _FILEINFO_);
  // }
  //
  // if ( "iof" == g_iofCorrection.toLower() ) {
  //   // Units of I/F
  //   // TODO: Note, we do not have a correct g_iofScale (== 1 right now). This was provided for
  //   // Hayabusa AMICA's v-band and all other bands were normalized according to this value. We do
  //   // not have this value for Hayabusa2 ONC. Need to correct this value.
  //   g_iof = pi_c() * (g_solarDist * g_solarDist) *
  //           g_iofScale / g_solarFlux / g_texp;
  //   g_units = "I over F";
  // }

  // Calibrate!
  try {
    p.Progress()->SetText("Calibrating Hayabusa2 Cube");
    p.StartProcess(Calibrate);
  }
  catch (IException &ie) {
    throw IException(ie, IException::Programmer,
      "Radiometric calibration failed!", _FILEINFO_);
  }

  // Log calibration activity performed so far
  PvlGroup calibrationLog("RadiometricCalibration");
  calibrationLog.addKeyword(PvlKeyword("SoftwareName", hyb2cal_program.toStdString()));
  calibrationLog.addKeyword(PvlKeyword("SoftwareVersion", hyb2cal_version.toStdString()));
  calibrationLog.addKeyword(PvlKeyword("ProcessDate", hyb2cal_runtime.toStdString()));
  calibrationLog.addKeyword(PvlKeyword("CalibrationFile", calfile.toStdString()));
  calibrationLog.addKeyword(PvlKeyword("FlatFieldFile", flatfile.originalPath().toStdString()
  + "/" + flatfile.name().toStdString()));
  calibrationLog.addKeyword(PvlKeyword("CompressionFactor", toString(g_compfactor, 2).toStdString()));

  // Parameters
  PvlKeyword key("Bias_Bn");
  key.addValue(toString(g_b0, 8).toStdString());
  key.addValue(toString(g_b1, 8).toStdString());
  key.addValue(toString(g_b2, 8).toStdString());
  calibrationLog.addKeyword(key);
  calibrationLog.addKeyword(PvlKeyword("Bias", toString(g_bias, 16).toStdString(), "DN"));

  // calibrationLog.addKeyword(PvlKeyword("Smear_Tvct", toString(g_Tvct, 16)));

  calibrationLog.addKeyword(PvlKeyword("CalibrationUnits", g_iofCorrection.toStdString()));
  calibrationLog.addKeyword(PvlKeyword("RadianceStandard", toString(g_v_standard, 16).toStdString()));
  calibrationLog.addKeyword(PvlKeyword("RadianceScaleFactor", toString(g_iofScale, 16).toStdString()));
  calibrationLog.addKeyword(PvlKeyword("SolarDistance", toString(g_solarDist, 16).toStdString(), "AU"));
  calibrationLog.addKeyword(PvlKeyword("SolarFlux", toString(g_solarFlux, 16).toStdString()));
  calibrationLog.addKeyword(PvlKeyword("IOFFactor", toString(g_iof, 16).toStdString()));
  calibrationLog.addKeyword(PvlKeyword("Units", g_units.toStdString()));

  // Write Calibration group to output file
  ocube->putGroup(calibrationLog);
  Application::Log(calibrationLog);
  //configFile.clear();
  p.EndProcess();

}


/**
* @brief Determine name of flat field file to apply
* @author 2016-03-30 Kris Becker
* @param filter  Name of ONC filter
* @return FileName Path and name of flat file file
* @internal
*   @history 2017-07-27 Ian Humphrey & Kaj Williams - Adapted from amicacal.
*/
FileName DetermineFlatFieldFile(const QString &filter) {

  QString fileName = "$hayabusa2/calibration/flatfield/";
  // FileName consists of binned/notbinned, camera, and filter
  fileName += "flat_" + filter.toLower() + ".cub";
  FileName final(fileName);
  return final;
}



/**
* @brief Loads the calibration variables into the program.
* @param config QString Name of the calibration file to load.
*/
QString loadCalibrationVariables(const QString &config)  {

  //  UserInterface& ui = Application::GetUserInterface();

  FileName calibFile(config);
  if ( config.contains("?") ) calibFile = calibFile.highestVersion();

  // Pvl configFile;
  g_configFile.read(calibFile.expanded().toStdString());

  // Load the groups
  PvlGroup &Bias = g_configFile.findGroup("Bias");
  PvlGroup &DarkCurrent = g_configFile.findGroup("DarkCurrent");
  // PvlGroup &Smear = g_configFile.findGroup("SmearRemoval");
  PvlGroup &solar = g_configFile.findGroup("SOLARFLUX");
  // PvlGroup &iof = g_configFile.findGroup("RAD");

  // Load Smear Removal Variables
  // g_Tvct = Smear["Tvct"];

  // Load DarkCurrent variables
  g_d0 = std::stod(DarkCurrent["D"][0]);
  g_d1 = std::stod(DarkCurrent["D"][1]);

  // Load Bias variables
  g_b0 = std::stod(Bias["B"][0]);
  g_b1 = std::stod(Bias["B"][1]);
  g_b2 = std::stod(Bias["B"][2]);

  // Compute BIAS correction factor (it's a constant so do it once!)
  g_bias = g_b0 + g_b1 * g_AEtemperature + g_b2 * (g_AEtemperature * g_AEtemperature);
  std::cout << "bias: " << g_bias << std::endl;

  // Load the Solar Flux for the specific filter
  g_solarFlux = solar[g_filter.toLower().toStdString()];

  // radiance = g_v_standard * g_iofScale
  // iof      = radiance * pi *dist_au^2
  // g_iofScale   = iof[g_filter];

  return ( calibFile.original() );
}


/**
* @brief Apply radiometric correction to each line of an AMICA image.
* @author 2016-03-30 Kris Becker
* @param in   Raw image and flat field
* @param out  Radometrically corrected image
* @internal
*   @history 2017-07-2017 Ian Humphrey & Kaj Williams - Adapted from amicacal.
*/
void Calibrate(vector<Buffer *>& in, vector<Buffer *>& out) {

  Buffer& imageIn   = *in[0];
  Buffer& flatField = *in[1];
  Buffer& imageOut  = *out[0];

  int pixelsToNull = 0;

  // Note that is isn't currently tested, as we do not have a test with a hayabusa2 image that
  // has been on-board cropped.
  int currentSample = imageIn.Sample();
  int alphaSample = alpha->AlphaSample(currentSample);

  if ( (alphaSample <= pixelsToNull)  || (alphaSample >= (1024 - pixelsToNull ))) {

    for (int i = 0; i < imageIn.size(); i++ ) {
      imageOut[i] = Isis::Null;
    }
    return;
  }


  // TODO: Once smear model and readout time are known, we can add smear correction.
  // Compute smear component here as its a constant for the entire sample
  // double t1 = g_timeRatio/imageIn.size();
  // double b = binning;
  // double c1(1.0);  //default if no binning
  //
  // if (binning > 1) {
  //   c1 = 1.0/(1.0 + t1*((b -1.0)/(2.0*b) ) );
  // }
  //
  // double smear = 0;
  // for (int j = 0; j < imageIn.size(); j++ ) {
  //   if ( !IsSpecial(imageIn[j]) ) {
  //     smear += t1 * ( (imageIn[j] * g_compfactor) - g_bias);
  //   }
  // }

  // Iterate over the line space
  for (int i = 0; i < imageIn.size(); i++) {

    imageOut[i] = imageIn[i];

    // Check for special pixel in input image and pass through
    if ( IsSpecial(imageOut[i]) ) {
      imageOut[i] = imageIn[i];
      continue;
    }

    // Apply compression factor here to raise LOSSY dns to proper response
    imageOut[i] *= g_compfactor;

    // 1) BIAS Removal - Only needed if not on-board corrected
    // TODO: find image with > 1 subimages
    if ( nsubImages <= 1 ) {

      if ( (imageOut[i] - g_bias) <= 0.0) {
        imageOut[i] = Null;
        continue;
      }
      else {
        imageOut[i] = imageOut[i] - g_bias;
      }
    }

    // 2) DARK Current
    imageOut[i] = imageOut[i] - g_darkCurrent;

    // READOUT Smear Removal - Not needed if on-board corrected.  Binning is
    //    accounted for in computation of c1 before loop.
    // if (nsubImages <= 1) {
    //  imageOut[i] = c1*(imageOut[i] - smear);
    // }

    // 3) FLATFIELD correction
    //  Check for any special pixels in the flat field (unlikely)
    // If we have only one input cube, that means that we do not have a flat-field (W1/W2).
    if (in.size() == 2) {
      // Note that our current flat-fields to not have special pixel values.
      if ( IsSpecial(flatField[i]) ) {
        imageOut[i] = Isis::Null;
        continue;
      }
      else {
        if (flatField[i] != 0) {
          imageOut[i] /= flatField[i];
        }
      }
    }

    // TODO: once the radiance values are known for each band, we can correctly compute I/F.
    // For now, g_iof is 1, so output will be in DNs.
    // 7) I/F or Radiance Conversion (or g_iof might = 1, in which case the output will be in DNs)
    imageOut[i] *= g_iof;
  }
  return;
}
