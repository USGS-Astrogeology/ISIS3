/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

// $Id: amicacal.cpp 6045 2015-02-07 02:06:59Z moses@GS.DOI.NET $
#include "Isis.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

#include <QFile>
#include <QScopedPointer>
#include <QString>
#include <QTemporaryFile>
#include <QVector>

#include "AlphaCube.h"
#include "AmicaCalUtils.h"
#include "Buffer.h"
#include "FileName.h"
#include "IException.h"
#include "iTime.h"
#include "LineManager.h"
#include "Pixel.h"
#include "ProcessByBoxcar.h"
#include "ProcessByBrick.h"
#include "ProcessByLine.h"
#include "ProcessBySample.h"
#include "ProgramLauncher.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "Spice.h"
#include "Statistics.h"
#include "TextFile.h"


using namespace Isis;
using namespace std;

// Calibration support routines
FileName determineFlatFieldFile(const QString &filter, const bool nullPolarPix);
void calibrate(vector<Buffer *>& in, vector<Buffer *>& out);

QString loadCalibrationVariables(const QString &config, Cube *iCube);

#if 0
// PSF correction is currently not working and has been removed as an option.
//void psfCorrection(vector<Buffer *>& in, vector<Buffer *>& out);
//void psfCorrectionBoxcar(Buffer &in, double &result);
#endif

// Temporary cube file pointer deleter
struct TemporaryCubeDeleter {
   static inline void cleanup(Cube *cube) {
     if ( cube ) {

       FileName filename( cube->fileName().toStdString() );
       delete cube;
       remove( filename.expanded().c_str() );
     }
   }
};


//For subimage and binning mapping
static AlphaCube *alpha = 0;

QString g_filter = "";
static QString g_target ="";
static const int g_hayabusaNaifCode = -130;
static Pvl g_configFile;

//Bias calculation variables
static double g_b0 = 0;
static double g_b1 = 0;
static double g_b2 = 0;
static double g_bias = 0;

static QString g_launchTimeStr;
static iTime g_launchTime;
static QString g_startTime;


//Dark Current variables
static double g_d0 = 0;
static double g_d1 = 0;
static double g_temperature = 0;
static double g_darkCurrent = 0;

//Smear calculation variables
static double g_tvct = 0;         //!< Vertical charge-transfer period  ( in seconds).
static double g_exposureTime = 1;
static double g_timeRatio = 1.0;

//Linearity calculation variables
static double g_gamma = 0;
static double g_L0 = 0;
static double g_L1 = 0;


// Calibration parameters
static int nsubImages = 0;      //!< Number of sub images
static int binning = 1;         //!< The number of samples/lines which are binned
static bool g_nullPolarizedPixels = true;   /**< Flag which tells us if the Polarized pixels are
                                                 to be set to ISIS::Null */
static double g_compfactor = 1.0;  //!< Default if OutputMode = LOSS-LESS; 16.0 for LOSSY

static QString g_iofCorrection = "IOF";  //!< Is I/F correction to be applied?


//  I/F variables
static double g_solarDist = 1.0;  /**< Distance from the Sun to the target body
                                 (used to calculate g_calibrationScale) */
static double g_calibrationScale = 1.0;        //!< I/F conversion value
static double g_iofScale = 1.0;
static double g_solarFlux = 1.0;  //!< The solar flux (used to calculate g_calibrationScale).
static double g_radStd = 3.42E-3;//!< Base conversion for all filters (Tbl. 9)


//Hot pixel vector container
static QVector<Pixel> hotPixelVector;  //!< A pixel vector that contains the Hot Pixel locations

#if 0
// PSF correction is currently not working and has been removed as an option.
// PSF variables
//static bool g_applyPSF = false;
//static int ns, nl, nb;     //!< Number of samples, lines, bands of the input cube
//static int g_size = 23;   //!< The size of the Boxcar used for calculating the light diffusion model.
//static const int g_N = 6;
//static double g_alpha = 0.0;
//static double *g_psfFilter;
//static double g_sigma[g_N];
//static double g_A[g_N];
#endif

void IsisMain() {


  UserInterface& ui = Application::GetUserInterface();
  g_nullPolarizedPixels = ui.GetBoolean("NULLPOLARPIX");
  g_iofCorrection = ui.GetString("UNITS");

  const QString amicacalProgram = "amicacal";
  const QString amicacalVersion = "1.0";
  const QString amicacalRevision = "$Revision$";
  QString amicacalRuntime = Application::DateTime();

  ProcessBySample process;

  Cube *inputCube = process.SetInputCube("FROM");
#if 0
// PSF correction is currently not working and has been removed as an option.
//  g_applyPSF = ui.GetBoolean("APPLYPSF");
#endif

  // Basic assurances...
  if (inputCube->bandCount() != 1) {
    throw IException(IException::User,
                     "AMICA images may only contain one band",
                     _FILEINFO_);
  }

  PvlGroup& inst = inputCube->group("Instrument");
  PvlGroup &bandbin = inputCube->group("BandBin");
  PvlGroup &archive = inputCube->group("Archive");

  QString filter = QString::fromStdString(bandbin["Name"]);
  g_filter=filter;

  binning = inst["Binning"];
  int startLine = inst["FirstLine"];
  int startSample = inst["FirstSample"];
  int lastLine = inst["LastLine"];
  int lastSample = inst["LastSample"];

  //Set up binning and image subarea mapping

  AlphaCube myAlpha(1024, 1024, inputCube->sampleCount(),
                    inputCube->lineCount(),
                    startSample + 1,
                    startLine + 1,
                    lastSample + 1,
                    lastLine + 1);

  alpha = &myAlpha;

  try {
    g_exposureTime = inst["ExposureDuration"] ;
  }
  catch(IException &e) {
    std::string msg = "Unable to read [ExposureDuration] keyword in the Instrument group "
                  "from input file [" + inputCube->fileName().toStdString() + "]";
    throw IException(e, IException::Io, msg, _FILEINFO_);
  }


  try {
    g_temperature = inst["CcdTemperature"] ;
  }
  catch(IException &e) {
    std::string msg = "Unable to read [CcdTemperature] keyword in the Instrument group "
                  "from input file [" + inputCube->fileName().toStdString() + "]";
    throw IException(e, IException::Io, msg, _FILEINFO_);

  }

  QString startTime = QString::fromStdString(inst["SpacecraftClockStartCount"]);

  g_startTime = startTime;
  binning = inst["Binning"];
  int firstLine = inst["FirstLine"];
  int startsample = inst["FirstSample"];
  int lastline = inst["LastLine"];
  int lastsample = inst["LastSample"];

  nsubImages = archive["SubImageCount"];  // If > 1, some correction is
                                          // not needed/performed

  QString compmode = QString::fromStdString(archive["OutputMode"]);
  g_compfactor = ( "lossy" == compmode.toLower() ) ? 16.0 : 1.0;


  // I/F values
  QString target = QString::fromStdString(inst["TargetName"]);
  g_target = target;

  // Determine if we need to subsample the flat field should pixel binning
  // occurred
  QScopedPointer<Cube, TemporaryCubeDeleter> flatcube;
  FileName flatfile = determineFlatFieldFile(g_filter, g_nullPolarizedPixels);

  std::string reducedFlat = flatfile.expanded();

  // Image is not cropped
  if (firstLine ==0 && startsample == 0){

    if (binning > 1) {
      QString scale(QString::number(binning));
      FileName newflat = FileName::createTempFile("$TEMPORARY/" +
                                                  flatfile.baseName() + "_reduced.cub");
      reducedFlat = newflat.expanded();
      QString parameters = "FROM=" + QString::fromStdString(flatfile.expanded()) +
         " TO="   + QString::fromStdString(newflat.expanded()) +
         " MODE=SCALE" +
         " LSCALE=" + scale +
         " SSCALE=" + scale;

      try {
        ProgramLauncher::RunIsisProgram("reduce", parameters);
      }
      catch (IException& ie) {
        remove(reducedFlat.c_str());
        throw ie;
      }
      QScopedPointer<Cube, TemporaryCubeDeleter> reduced(new Cube(reducedFlat, "r"));
      flatcube.swap( reduced );
    }

    // Set up processing for flat field as a second input file
    CubeAttributeInput att;
    process.SetInputCube(QString::fromStdString(reducedFlat), att);
  }
  else {
    // Image is cropped so we have to deal with it
    FileName transFlat = FileName::createTempFile("$TEMPORARY/"
                                                  + flatfile.baseName()
                                                  + "_translated.cub");

    Cube *flatOriginal = new Cube(flatfile.expanded() );

    int transform[5] = {binning, startsample, firstLine, lastsample, lastline};

    // Translates and scales the flatfield image.  Scaling
    // might be necessary in the event that the raw image was also binned.

    translate(flatOriginal, transform, QString::fromStdString(transFlat.expanded()));

    QScopedPointer<Cube, TemporaryCubeDeleter> translated(new Cube(transFlat.expanded(), "r"));
    flatcube.swap(translated);

    CubeAttributeInput att;
    process.SetInputCube(QString::fromStdString(transFlat.expanded()), att);
  }

  Cube *outputCube  = process.SetOutputCube("TO");
  QString fname = outputCube->fileName();

#if 0
// PSF correction is currently not working and has been removed as an option.
  //ns = inputCube->sampleCount();
  //nl = inputCube->lineCount();
  //nb = inputCube->bandCount();
#endif

  QString calfile = loadCalibrationVariables(ui.GetAsString("CONFIG"), inputCube);

  g_timeRatio = g_tvct / (g_exposureTime + g_tvct);
  g_darkCurrent = g_d0 * exp(g_d1 * g_temperature);
  g_calibrationScale = 1.0;
  QString g_units = "DN";

  if ( !sunDistanceAU(inputCube, startTime, target, g_solarDist) ) {
     throw IException(IException::Programmer,
                      "Cannot calculated distance to sun!",
                      _FILEINFO_);
  }

  //  Add DN/S as an output option
  if ( QString::compare(g_iofCorrection, "dn/s", Qt::CaseInsensitive) == 0 ) {
      g_calibrationScale = 1.0 / g_exposureTime;
      g_units = "DN/S";
  }
  else if ( QString::compare(g_iofCorrection, "dn", Qt::CaseInsensitive) != 0 ) {
    /* Note - this radiance calibration scaling factor is applied to both radiance and iof
     *
     * Units of RADIANCE
     * equation:
     *     Rad(i)=image(DN/s) * C * S(i) where
     *         C = radStd = 3.42.10^-3
     *         S(i) = iofScale,
     *             with S(v) = 1,
     *             S(zs)=3.286 (factor computed by Lucille Le Corre of PSI),
     *             and the remaining scale factors from table 9 of the Ishiguro et al. 2010 paper
     */
    g_calibrationScale = g_radStd * g_iofScale / g_exposureTime;
    g_units = "W / (m**2 micrometer sr)";

    if ( QString::compare(g_iofCorrection, "iof", Qt::CaseInsensitive) == 0 ) {
      /* Note: iof (i.e. reflectance) equation described below is just
       *       Ref(i) = Rad(i) * pi * d^2 /Fv
       *       so this if-statement is nested
       *
       * Units of I/F (reflectance)
       * equation:
       *     Ref(v)=image(DN/s) * C * S(i) * pi * d^2/F(v) where
       *         C, S(i) are as described above,
       *         d is the solar distance from the label of the v filter image,
       *         Fv = solarflux(v) = 1861.145142 is the solar flux
       *             resampled to V filter bandpass with Quantum Efficiency
       */
      g_calibrationScale = g_calibrationScale
                           * Isis::PI
                           * (g_solarDist * g_solarDist)
                           / g_solarFlux;
      g_units = "I over F";
    }
  }

  // Calibrate!
  try {
    process.Progress()->SetText("Calibrating Hayabusa Cube");
    process.StartProcess(calibrate);
  }
  catch (IException &ie) {
    throw IException(ie,
                     IException::Programmer,
                     "Radiometric calibration failed!",
                     _FILEINFO_);
  }

  // Log calibration activity performed so far
  PvlGroup calibrationLog("RadiometricCalibration");
  calibrationLog.addKeyword(PvlKeyword("SoftwareName", amicacalProgram.toStdString()));
  calibrationLog.addKeyword(PvlKeyword("SoftwareVersion", amicacalVersion.toStdString()));
  calibrationLog.addKeyword(PvlKeyword("ProcessDate", amicacalRuntime.toStdString()));
  calibrationLog.addKeyword(PvlKeyword("CalibrationFile", calfile.toStdString()));
  calibrationLog.addKeyword(PvlKeyword("FlatFieldFile", flatfile.originalPath()
                                                        + "/" + flatfile.name()));
  calibrationLog.addKeyword(PvlKeyword("CompressionFactor", toString(g_compfactor, 2)));

  // Parameters
  PvlKeyword key("Bias_Bn");
  key.addValue(toString(g_b0, 8));
  key.addValue(toString(g_b1, 8));
  key.addValue(toString(g_b2, 8));
  calibrationLog.addKeyword(key);
  calibrationLog.addKeyword(PvlKeyword("Bias", toString(g_bias, 16), "DN"));

  key = PvlKeyword("Linearity_Ln");
  key.addValue(toString(g_L0, 8));
  key.addValue(toString(g_L1, 8));
  calibrationLog.addKeyword(key);
  calibrationLog.addKeyword(PvlKeyword("Linearity_Gamma", toString(g_gamma, 16)));

  calibrationLog.addKeyword(PvlKeyword("Smear_tvct", toString(g_tvct, 16)));

  calibrationLog.addKeyword(PvlKeyword("CalibrationUnits", g_iofCorrection.toStdString()));
  calibrationLog.addKeyword(PvlKeyword("RadianceStandard", toString(g_radStd, 16)));
  calibrationLog.addKeyword(PvlKeyword("RadianceScaleFactor", toString(g_iofScale, 16)));
  calibrationLog.addKeyword(PvlKeyword("SolarDistance", toString(g_solarDist, 16), "AU"));
  calibrationLog.addKeyword(PvlKeyword("SolarFlux", toString(g_solarFlux, 16)));
  calibrationLog.addKeyword(PvlKeyword("IOFFactor", toString(g_calibrationScale, 16)));
  calibrationLog.addKeyword(PvlKeyword("Units", g_units.toStdString()));

#if 0
// PSF correction is currently not working and has been removed as an option.
  // This section will apply the PSF correction
  if ( g_applyPSF ) {
    //PSF correction
    CubeAttributeInput attInput;
    CubeAttributeOutput attOutput;

    ProcessByBoxcar processDiffusionModel;

    QScopedPointer<Cube, TemporaryCubeDeleter> diffusionModel;

    FileName oname(outputCube->fileName());
    FileName psfModel = FileName::createTempFile("$TEMPORARY/" + oname.baseName() + "_psfmodel.cub");


    processDiffusionModel.SetInputCube(outputCube);
    processDiffusionModel.SetOutputCube(psfModel.name(), attOutput,
                                        outputCube->sampleCount(),
                                        outputCube->lineCount(),
                                        outputCube->bandCount());

    processDiffusionModel.SetBoxcarSize(g_size, g_size);

    g_psfFilter = setPSFFilter(g_size, g_A, g_sigma, g_alpha, g_N, binning);

    try {

         processDiffusionModel.StartProcess(psfCorrectionBoxcar);  //Determine the diffusion model.

      }

      catch(IException &ie) {
        processDiffusionModel.EndProcess();
        outputCube->putGroup(calibrationLog);
        process.EndProcess();
        remove( psfModel.expanded().c_str() );
        throw IException(ie,
                         IException::Programmer,
                         "Calculating the diffusion model failed!",
                         _FILEINFO_);
      }

      processDiffusionModel.EndProcess();

     //Apply the PSF correction
      ProcessByLine processPSFCorrection;

      //The diffusion model
      processPSFCorrection.SetInputCube(psfModel.name(), attInput);

      //The original output cube.
      processPSFCorrection.SetInputCube(outputCube);
      processPSFCorrection.AddOutputCube(outputCube, false);


      try {

        processPSFCorrection.StartProcess(psfCorrection);

        // Add PSF parameter to the calibration reporting
        key = PvlKeyword("PSF_KernelSize");
        key.addValue(Isis::toString(g_size));
        key.addValue(Isis::toString(g_size));
        calibrationLog.addKeyword(key);

        calibrationLog.addKeyword(PvlKeyword("PSF_Focused", Isis::toString(g_alpha, 6)));

        key = PvlKeyword("PSF_Sigma");
        for (int i = 0 ; i < g_N ; i++ ) {
          key.addValue(Isis::toString(g_sigma[i]));
        }
        calibrationLog.addKeyword(key);

        key = PvlKeyword("PSF_Diffuse");
        for (int i = 0 ; i < g_N ; i++ ) {
          key.addValue(Isis::toString(g_A[i]));
        }

      }

      catch(IException &ie){
        processPSFCorrection.EndProcess();
        outputCube->putGroup(calibrationLog);
        process.EndProcess();
        remove( psfModel.expanded().c_str() );
        throw IException(ie,
                         IException::Programmer,
                         "Applying the PSF correction failed!",
                         _FILEINFO_);

      }

      processPSFCorrection.EndProcess();

      // Remove the PSF file
      remove( psfModel.expanded().c_str() );
  }
#endif

  // Write Calibration group to output file
  outputCube->putGroup(calibrationLog);
  Application::Log(calibrationLog);
  //configFile.clear();
  process.EndProcess();

}


/**
 * @brief Determine name of flat field file to apply
 * @author 2016-03-30 Kris Becker
 * @param filter  Name of AMICA filter
 * @return FileName Path and name of flat file file
 */
FileName determineFlatFieldFile(const QString &filter, const bool nullPolarPix) {

  QString fileName = "$hayabusa/calibration/flatfield/";


  // FileName consists of binned/notbinned, camera, and filter

  if (nullPolarPix) {
      fileName += "flat_" + filter.toLower() + "np.cub";
  }
  else {

    fileName += "flat_" + filter.toLower() + ".cub";

  }
  FileName final(fileName.toStdString());


  //final = final.highestVersion();
  return final;
}


#if 0
// PSF correction is currently not working and has been removed as an option.
/**
 * @brief This function moves the PSF kernel through each pixel of the input cube and approximates
 * the amount of light diffusion produced by that pixel.
 * @param in The radiometrically calibrated cube (minus PSF correction).
 * @param result  The light diffusion estimate at the central pixel of the boxcar of all the
 * surrounding pixels.
 */

void psfCorrectionBoxcar(Buffer &in, double &result) {

    result = 0;
    int center = (int)((g_size * g_size - 1) / 2);

    Statistics stats;

    for (int i = 0; i < in.size(); i++) {

      if (!IsSpecial(in[i])) {
        //stats.AddData(in[i] * g_psfFilter[i]);
        stats.AddData(in[center] * g_psfFilter[i]);
        //result += in[i] * g_psfFilter[i];
      }

    }

    result = stats.Sum();
}


/**
 * @brief Applies the PSF function
 * @param in[0]  The PSF light diffusion model (a cube)
 * @parm  in[1]  The radiometrically corrected cube (without PSF correction).
 * @param out    The radiometrically corrected cube after PSF correction has been applied.
 */
void psfCorrection(vector<Buffer *> &in, vector<Buffer *> &out) {

  Buffer& nopsf    = *in[1];
  Buffer& psfVals =  *in[0];
  Buffer& imageOut  = *out[0];

  for (int i = 0; i < nopsf.size(); i++) {
    if (!IsSpecial(psfVals[i])) {

      imageOut[i] = nopsf[i] - psfVals[i];
      //imageOut[i] = psfVals[i];

    }
    else {
      imageOut[i] = nopsf[i];
    }
  }

}
#endif


/**
 * @brief Loads the calibration variables into the program.
 */

QString loadCalibrationVariables(const QString &config, Cube *iCube)  {

//  UserInterface& ui = Application::GetUserInterface();

//  FileName calibFile("$hayabusa/calibration/amica/amicaCalibration????.trn");
  FileName calibFile(config.toStdString());
  if ( config.contains("?") ) calibFile = calibFile.highestVersion();

  // Pvl configFile;
  g_configFile.read(calibFile.expanded());

  // Load the groups
  PvlGroup &biasGroup = g_configFile.findGroup("Bias");
  PvlGroup &darkCurrentGroup = g_configFile.findGroup("DarkCurrent");
  PvlGroup &smearGroup = g_configFile.findGroup("SmearRemoval");
  PvlGroup &linearityGroup = g_configFile.findGroup("Linearity");
  PvlGroup &hotPixelsGroup = g_configFile.findGroup("HotPixels");
  PvlGroup &radGroup = g_configFile.findGroup("Rad");
  PvlGroup &solarFluxGroup = g_configFile.findGroup("SolarFlux");

#if 0
// PSF correction is currently not working and has been removed as an option.
//  PvlGroup &psfDiffuse = g_configFile.findGroup("PSFDiffuse");
//  PvlGroup &psfFocused = g_configFile.findGroup("PSFFocused");
#endif

  // Load the hot pixels into a vector
  for (int i = 0; i< hotPixelsGroup.keywords(); i++ ){

    int samp(Isis::toInt(hotPixelsGroup[i][0]));
    int line (Isis::toInt(hotPixelsGroup[i][1]));

    hotPixelVector.append( Pixel(alpha->BetaSample(samp), alpha->BetaLine(line), 1, 0));
  }

  // Load linearity variables
  g_gamma = linearityGroup["Gamma"];
  g_gamma = 1.0 - g_gamma;

  g_L0 = Isis::toDouble(linearityGroup["L"][0]);
  g_L1 = Isis::toDouble(linearityGroup["L"][1]);

  // Load Smear Removal Variables
  g_tvct = smearGroup["tvct"];

  // Load DarkCurrent variables
  g_d0 = Isis::toDouble(darkCurrentGroup["D"][0]);
  g_d1 = Isis::toDouble(darkCurrentGroup["D"][1]);

  // Load Bias variables
  g_b0 = Isis::toDouble(biasGroup["B"][0]);
  g_b1 = Isis::toDouble(biasGroup["B"][1]);
  g_b2 = Isis::toDouble(biasGroup["B"][2]);


  g_launchTimeStr = QString::fromStdString(biasGroup["launchTime"]);
  g_launchTime = g_launchTimeStr;

  // Compute BIAS correction factor (it's a constant so do it once!)
  double obsStartTime;
  double tsecs;
  double tdays;
  
  try{
    Camera *cam; 
    cam = iCube->camera();
    cam->SetImage (0.5, 0.5);
    obsStartTime = cam->time().Et();
  }
  catch(IException &e) {
    try{
      loadNaifTiming();  // Ensure the proper kernels are loaded
      scs2e_c(g_hayabusaNaifCode, g_startTime.toLatin1().data(), &obsStartTime);
    }
    catch (IException &e) {
        std::string message = "IOF option does not work with non-spiceinited cubes.";
        throw IException(e, IException::User, message, _FILEINFO_);
    }
  }

  tsecs = obsStartTime - g_launchTime.Et();
  tdays = tsecs / 86400;
  g_bias = g_b0
           + g_b1 * tdays
           + g_b2 * (tdays * tdays);

#if 0
// PSF correction is not working and is temporarily removed.
  //g_bias = 0;
  //cout << "g_bias = "  << g_bias << endl;

  //Load the PSF constants.  These come from
  //Ishiguro, 2014 ('Scattered light correction of Hayabusa/AMICA data and
  //quantitative spectral comparisons of Itokawa')

//  QString kernel_sz=ui.GetString("KERNEL_SIZE");
//  g_size = kernel_sz.toInt();

  //Commenting out this code and making it a user parameter
  //to make it easier to try out optimum values
  if (psfFocused.hasKeyword("KernelSize") ) {
    g_size = psfFocused["KernelSize"];
  }
  else {
    g_size = 23;
  }

  g_alpha = psfFocused[g_filter.toLower()];

   for (int i =0; i < g_N; i++) {
     g_sigma[i] = psfDiffuse["sigma"][i].toDouble();
     g_A[i] = psfDiffuse[g_filter.toLower()][i].toDouble();
   }
#endif

  // Load the Solar Flux for the specific filter
  g_solarFlux = solarFluxGroup["v"];

  g_radStd = radGroup["iof_standard"];
  g_iofScale   = radGroup[g_filter.toStdString()];

  return ( QString::fromStdString(calibFile.original()) );
}


/**
 * @brief Apply radiometric correction to each line of an AMICA image.
 * @author 2016-03-30 Kris Becker
 * @param in   Raw image and flat field
 * @param out  Radometrically corrected image
 */
void calibrate(vector<Buffer *>& in, vector<Buffer *>& out) {

  Buffer& imageIn   = *in[0];
  Buffer& flatField = *in[1];
  Buffer& imageOut  = *out[0];

  int pixelsToNull = 12;

  int currentSample = imageIn.Sample();
  int alphaSample = alpha->AlphaSample(currentSample);

  if ( (alphaSample <= pixelsToNull)  || (alphaSample >= (1024 - pixelsToNull ))) {

    for (int i = 0; i < imageIn.size(); i++ ) {
      imageOut[i] = Isis::Null;
    }
    return;
  }


  // Compute smear component here as its a constant for the entire sample
  double t1 = g_timeRatio / imageIn.size();
  double b = binning;
  double c1 = 1.0;  //default if no binning

  if (binning > 1) {
    c1 = 1.0 / (1.0 + t1 * ((b - 1.0) / (2.0 * b) ) );
  }

  double smear = 0;
  for (int j = 0; j < imageIn.size(); j++ ) {
    if ( !IsSpecial(imageIn[j]) ) {
      smear += t1 * ( (imageIn[j] * g_compfactor) - g_bias);
    }
  }


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
    if ( nsubImages <= 1 ) {

      if ( (imageOut[i] - g_bias) <= 0.0) {
        imageOut[i] = Null;
        continue;
      }
      else {
        imageOut[i] = imageOut[i] - g_bias;
      }
    }

    // 2) LINEARITY Correction - always done
    imageOut[i] = pow(imageOut[i], g_gamma) + g_L0 * imageOut[i] * exp(g_L1 * imageOut[i]);


    // 3) DARK Current
      imageOut[i] = imageOut[i] - g_darkCurrent;

    // 4) HOT Pixel Removal

    bool hot = false;

    for (int j=0; j < hotPixelVector.size(); j++) {

      if ((hotPixelVector[j].sample() == currentSample) && (hotPixelVector[j].line() == i)) {
        imageOut[i] = Null;
        hot = true;
      }
    }

    if (hot == true)
      continue;

    // 5) READOUT Smear Removal - Not needed if on-board corrected.  Binning is
    //    accounted for in computation of c1 before loop.
    if (nsubImages <= 1) {
      imageOut[i] = c1 * (imageOut[i] - smear);
    }

    // 6) FLATFIELD correction
    //  Check for any special pixels in the flat field (unlikely)
    if ( IsSpecial(flatField[i]) ) {
      imageOut[i] = Isis::Null;
      continue;
    }
    else {
      imageOut[i] /= flatField[i];
    }

    // 7) I/F or Radiance Conversion (or g_calibrationScale might = 1, in which case the output will be in DNs)
    imageOut[i] *= g_calibrationScale;
  }
  return;
}
