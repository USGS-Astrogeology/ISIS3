/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <fstream>
#include <string>

#include "Angle.h"
#include "Cube.h"
#include "Camera.h"
#include "CameraFocalPlaneMap.h"
#include "Distance.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "FileName.h"
#include "ImageHistogram.h"
#include "LineManager.h"
#include "Longitude.h"
#include "OriginalLabel.h"
#include "PixelType.h"
#include "ProcessExportPds.h"
#include "Pvl.h"
#include "PvlFormat.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "PvlToPvlTranslationManager.h"
#include "Table.h"
#include "UserInterface.h"

using namespace Isis;
using namespace std;

pair<double, double> inputRange(Cube *inputCube);
void updatePdsLabelTimeParametersGroup(Pvl &pdsLabel);
void updatePdsLabelImageObject(PvlObject *isisCubeLab, Pvl &pdsLabel);
void updatePdsLabelRootObject(PvlObject *isisCubeLab, Pvl &pdsLabel,
                              UserInterface &ui, Camera *cam);
void IsisMain() {
  // Get user interface and create a ProcessExportPds object
  UserInterface &ui = Application::GetUserInterface();
  ProcessExportPds p;
  Process pHist;
  double *band_min, *band_max;


  Cube *inputCube = p.SetInputCube("FROM");
  PvlObject *isisCubeLab = inputCube->label();

  // Error check to make sure this is a valid cube for this program
  QString origInstrument = QString::fromStdString(isisCubeLab->findObject("IsisCube")
                           .findGroup("OriginalInstrument")["InstrumentId"][0]);
  if (origInstrument != "HIRISE") {
    std::string msg = "Input cube must from a HiRISE image. The original "
                  "InstrumentId = [" + origInstrument
                  + "] is unsupported by hideal2pds.";
    throw IException(IException::Io, msg, _FILEINFO_);
  }
  QString instrumentId = QString::fromStdString(isisCubeLab->findObject("IsisCube")
                                    .findGroup("Instrument")["InstrumentId"][0]);
  if (instrumentId != "IdealCamera") {
    std::string msg = "Input cube must be IdealCamera. InstrumentId = ["
                  + instrumentId + "] is unsupported by hideal2pds.";
    throw IException(IException::Io, msg, _FILEINFO_);
  }
  QString target = QString::fromStdString(isisCubeLab->findObject("IsisCube")
                              .findGroup("Instrument")["TargetName"][0]);
  if (target.toUpper() != "MARS") {
    std::string msg = "Input cube must from a HiRise image. The target = ["
                  + target + "] is unsupported by hideal2pds.";
    throw IException(IException::Io, msg, _FILEINFO_);
  }

  band_min = new double[inputCube->bandCount()];
  band_max = new double[inputCube->bandCount()];

  for (int band = 1; band <= inputCube->bandCount(); ++band) {

    if (ui.GetString("TYPE").compare("AUTOMATIC") == 0) {
      // Set up a histogram for this band. This call sets the input range
      // by making an initial stats pass to find the data min and max
      ImageHistogram hist(*inputCube, band, pHist.Progress());

      // Loop and accumulate histogram
      pHist.Progress()->SetText("Gathering Histogram");
      pHist.Progress()->SetMaximumSteps(inputCube->lineCount());
      pHist.Progress()->CheckStatus();
      LineManager line(*inputCube);
      for (int i = 1; i <= inputCube->lineCount(); i++) {
        line.SetLine(i, band);
        inputCube->read(line);
        hist.AddData(line.DoubleBuffer(), line.size());
        pHist.Progress()->CheckStatus();
      }

      // get the requested cumulative percentages
      band_min[band-1] = ui.GetDouble("MINPER") == 0.0 ? hist.Minimum() : hist.Percent(ui.GetDouble("MINPER"));
      band_max[band-1] = ui.GetDouble("MAXPER") == 100.0 ? hist.Maximum() : hist.Percent(ui.GetDouble("MAXPER"));
    }
    else {
      band_min[band-1] = ui.GetDouble("MIN");
      band_max[band-1] = ui.GetDouble("MAX");
    }
  }

  // Find the minimum min and maximum max for all bands
  double minmin = band_min[0];
  double maxmax = band_max[0];
  for (int band = 1; band < inputCube->bandCount(); ++band) {
    if (band_min[band] < minmin) minmin = band_min[band];
    if (band_max[band] > maxmax) maxmax = band_max[band];
  }

  pHist.EndProcess();

  // use histogram to calculate min/max for input range
  pair<double, double> inRange;
  inRange = inputRange(inputCube);
  p.SetInputRange(minmin, maxmax);

  int nbits = ui.GetInteger("BITS");
  switch (nbits) {
    case 8:
      p.SetOutputType(UnsignedByte);
      p.SetOutputRange(VALID_MIN1, VALID_MAX1);
      p.SetOutputNull(NULL1);
      p.SetOutputLis(LOW_INSTR_SAT1);
      p.SetOutputLrs(LOW_REPR_SAT1);
      p.SetOutputHis(HIGH_INSTR_SAT1);
      p.SetOutputHrs(HIGH_REPR_SAT1);
      break;

    case 16:
      p.SetOutputType(UnsignedWord);
      p.SetOutputRange(VALID_MINU2, VALID_MAXU2);
      p.SetOutputNull(NULLU2);
      p.SetOutputLis(LOW_INSTR_SATU2);
      p.SetOutputLrs(LOW_REPR_SATU2);
      p.SetOutputHis(HIGH_INSTR_SATU2);
      p.SetOutputHrs(HIGH_REPR_SATU2);
      break;

    default:
      p.SetOutputType(UnsignedWord);
      p.SetOutputRange(3.0, pow(2.0, (double)(nbits)) - 1.0 - 2.0);
      p.SetOutputNull(0);
      p.SetOutputLrs(1);
      p.SetOutputLis(2);
      p.SetOutputHis(pow(2.0, (double)(nbits)) - 1.0 - 1.0);
      p.SetOutputHrs(pow(2.0, (double)(nbits)) - 1.0);
  }


  // output byte order will be MSB
  p.SetOutputEndian(Isis::Msb);
  p.setFormat(ProcessExport::BSQ);
  // multiple table files should be Fixed according to PDS documentation
  p.SetExportType(ProcessExportPds::Fixed);
  p.SetPdsResolution(ProcessExportPds::Meter);

  // output PDS file with detached labels and tables for this application
  FileName outPdsFile(ui.GetFileName("TO", "img"));
  QString pdsLabelFile = outPdsFile.path() + "/" + outPdsFile.baseName() + ".lbl";
  p.SetDetached(pdsLabelFile);
  // create generic pds label - this will be finalized with proper line/byte counts later
  Pvl &pdsLabel = p.StandardPdsLabel(ProcessExportPds::Image);

  QString isisLabelFile = ui.GetCubeName("FROM");

  // Translate the keywords from the input cube label that go in the PDS label
  PvlToPvlTranslationManager cubeLab(*(inputCube->label()),
                             "$ISISROOT/appdata/translations/MroHiriseIdealPdsExportCubeLabel.trn");
  cubeLab.Auto(pdsLabel);

  // get original label information
  OriginalLabel origBlob = inputCube->readOriginalLabel();
  Pvl origLabel;
  PvlObject origLabelObj = origBlob.ReturnLabels();
  origLabelObj.setName("OriginalLabelObject");
  origLabel.addObject(origLabelObj);
  PvlToPvlTranslationManager orig(origLabel,
                                 "$ISISROOT/appdata/translations/MroHirisePdsRdrOriginalLabel.trn");
  orig.Auto(pdsLabel);

  updatePdsLabelTimeParametersGroup(pdsLabel);
  updatePdsLabelImageObject(isisCubeLab, pdsLabel);

  // change SAMPLE_BIT_MASK value according to BITS input
  PvlObject &image = pdsLabel.findObject("IMAGE");
  image.addKeyword(PvlKeyword("SAMPLE_BIT_MASK", std::to_string((int)pow(2.0, (double)nbits) - 1)),
                   Pvl::Replace);

  Camera *cam = inputCube->camera();
  updatePdsLabelRootObject(isisCubeLab, pdsLabel, ui, cam);

  // Export each of the spice tables and update table keywords in PDS file
  //
  // *** NOTE ***
  //    This could change the start byte/line values for the tables that have
  //    already been set in the labels by the ExportTable call. This is not
  //    a problem since our tables are detached.  However, it could be a
  //    problem if we decide to allow attached PDS products in the future.
  QString pdsTableFile = "";
  pdsTableFile = outPdsFile.baseName() + "_INSTRUMENT_POINTING_TABLE.dat";
  Table instRotationTable = cam->instrumentRotation()->Cache("InstrumentPointing");
  p.ExportTable(instRotationTable, pdsTableFile);
  PvlObject isisTableLab = instRotationTable.Label();
  PvlObject &instPtTabLab = pdsLabel.findObject("INSTRUMENT_POINTING_TABLE");
  PvlKeyword tableKeyword = isisTableLab.findKeyword("TimeDependentFrames");
  tableKeyword.setName("TIME_DEPENDENT_FRAMES");
  instPtTabLab += tableKeyword;
  tableKeyword = isisTableLab.findKeyword("ConstantFrames");
  tableKeyword.setName("CONSTANT_FRAMES");
  instPtTabLab += tableKeyword;
  tableKeyword = isisTableLab.findKeyword("ConstantRotation");
  tableKeyword.setName("CONSTANT_ROTATION");
  instPtTabLab += tableKeyword;
  tableKeyword = isisTableLab.findKeyword("CkTableStartTime");
  tableKeyword.setName("CK_TABLE_START_TIME");
  instPtTabLab += tableKeyword;
  tableKeyword = isisTableLab.findKeyword("CkTableEndTime");
  tableKeyword.setName("CK_TABLE_END_TIME");
  instPtTabLab += tableKeyword;
  tableKeyword = isisTableLab.findKeyword("CkTableOriginalSize");
  tableKeyword.setName("CK_TABLE_ORIGINAL_SIZE");
  instPtTabLab += tableKeyword;

  pdsTableFile = outPdsFile.baseName() + "_INSTRUMENT_POSITION_TABLE.dat";
  Table instPositionTable = cam->instrumentPosition()->Cache("InstrumentPosition");
  p.ExportTable(instPositionTable, pdsTableFile);
  isisTableLab = instPositionTable.Label();
  PvlObject &instPosTabLab = pdsLabel.findObject("INSTRUMENT_POSITION_TABLE");
  tableKeyword = isisTableLab.findKeyword("CacheType");
  tableKeyword.setName("CACHE_TYPE");
  instPosTabLab += tableKeyword;
  tableKeyword = isisTableLab.findKeyword("SpkTableStartTime");
  tableKeyword.setName("SPK_TABLE_START_TIME");
  instPosTabLab += tableKeyword;
  tableKeyword = isisTableLab.findKeyword("SpkTableEndTime");
  tableKeyword.setName("SPK_TABLE_END_TIME");
  instPosTabLab += tableKeyword;
  tableKeyword = isisTableLab.findKeyword("SpkTableOriginalSize");
  tableKeyword.setName("SPK_TABLE_ORIGINAL_SIZE");
  instPosTabLab += tableKeyword;

  pdsTableFile = outPdsFile.baseName() + "_BODY_ROTATION_TABLE.dat";
  Table bodyRotationTable = cam->bodyRotation()->Cache("BodyRotation");
  p.ExportTable(bodyRotationTable, pdsTableFile);
  isisTableLab = bodyRotationTable.Label();
  PvlObject &bodyRotTabLab = pdsLabel.findObject("BODY_ROTATION_TABLE");
  tableKeyword = isisTableLab.findKeyword("TimeDependentFrames");
  tableKeyword.setName("TIME_DEPENDENT_FRAMES");
  bodyRotTabLab += tableKeyword;
  tableKeyword = isisTableLab.findKeyword("CkTableStartTime");
  tableKeyword.setName("CK_TABLE_START_TIME");
  bodyRotTabLab += tableKeyword;
  tableKeyword = isisTableLab.findKeyword("CkTableEndTime");
  tableKeyword.setName("CK_TABLE_END_TIME");
  bodyRotTabLab += tableKeyword;
  tableKeyword = isisTableLab.findKeyword("CkTableOriginalSize");
  tableKeyword.setName("CK_TABLE_ORIGINAL_SIZE");
  bodyRotTabLab += tableKeyword;
  if (isisTableLab.hasKeyword("SolarLongitude")) {
    tableKeyword = isisTableLab.findKeyword("SolarLongitude");
    tableKeyword.setName("SOLAR_LONGITUDE");
  }
  else {
    tableKeyword = PvlKeyword("SOLAR_LONGITUDE",
                              std::to_string(cam->solarLongitude().force360Domain()
                                   .positiveEast(Angle::Degrees)),
                              "DEGREES");
  }
  bodyRotTabLab += tableKeyword;

  pdsTableFile = outPdsFile.baseName() + "_SUN_POSITION_TABLE.dat";
  Table sunPositionTable  = cam->sunPosition()->Cache("SunPosition");
  p.ExportTable(sunPositionTable, pdsTableFile);
  isisTableLab = sunPositionTable.Label();
  PvlObject &sunPosTabLab = pdsLabel.findObject("SUN_POSITION_TABLE");
  tableKeyword = isisTableLab.findKeyword("CacheType");
  tableKeyword.setName("CACHE_TYPE");
  sunPosTabLab += tableKeyword;
  tableKeyword = isisTableLab.findKeyword("SpkTableStartTime");
  tableKeyword.setName("SPK_TABLE_START_TIME");
  sunPosTabLab += tableKeyword;
  tableKeyword = isisTableLab.findKeyword("SpkTableEndTime");
  tableKeyword.setName("SPK_TABLE_END_TIME");
  sunPosTabLab += tableKeyword;
  tableKeyword = isisTableLab.findKeyword("SpkTableOriginalSize");
  tableKeyword.setName("SPK_TABLE_ORIGINAL_SIZE");
  sunPosTabLab += tableKeyword;

  // Read in the proper keyword types (Real, Enum, String, Integer, etc) for
  // each PvlKeyword so that the PDS labels have proper format
  PvlFormat *formatter = pdsLabel.format();

  if( nbits != 8 ) {
    formatter->add("$ISISROOT/appdata/translations/MroHiriseIdealPds_16bit.typ");
  } else {
    formatter->add("$ISISROOT/appdata/translations/MroHiriseIdealPds_8bit.typ");
  }

  // Format ordering of keywords/objects/groups/comments in the PDS labels
  pdsLabel.setFormatTemplate("$ISISROOT/appdata/translations/MroHiriseIdealPds.pft");

  // image line/byte offsets are calculated and values are updated in the labels
  // now that all translations/additions/modifications to the labels have been
  // completed
  p.OutputDetachedLabel();
  QString outFileName(outPdsFile.expanded());
  ofstream outputStream(outFileName.toLatin1().data());
  p.StartProcess(outputStream);
  p.EndProcess();
  outputStream.close();

  delete [] band_min;
  delete [] band_max;
}

/**
 * This method uses a Histogram object to find the minimum and maximum DN
 * values of the input cube.  These values are used by the Process object
 * to set the input range.
 *
 * @param inputCube Pointer to the inputCube
 *
 * @return A pair whose first value is the minimum DN of the input cube and
 *         second value is the maximum DN of the input cube.
 */
pair<double, double> inputRange(Cube *inputCube) {
  Process histProcess;
  int band = 1;
  ImageHistogram hist(*inputCube, band, histProcess.Progress());

  // Loop and accumulate histogram
  histProcess.Progress()->SetText("Gathering Histogram to Find Input Range");
  histProcess.Progress()->SetMaximumSteps(inputCube->lineCount());
  histProcess.Progress()->CheckStatus();
  LineManager line(*inputCube);
  for(int i = 1; i <= inputCube->lineCount(); i++) {
    line.SetLine(i, band);
    inputCube->read(line);
    hist.AddData(line.DoubleBuffer(), line.size());
    histProcess.Progress()->CheckStatus();
  }

  // get the requested cumulative percentages
  pair<double, double> inRange(hist.Minimum(), hist.Maximum());
  histProcess.EndProcess();
  return inRange;
}

/**
 * This method updates the values of the keywords in the IMAGE object of
 * the pds label file.
 *
 * The DESCRIPTION keyword is added.
 *
 * If the input cube has an AlphaCube group that indicates a crop has
 * been performed, SOURCE_LINE_SAMPLES, SOURCE_LINES, FIRST_LINE_SAMPLE,
 * and FIRST_LINE keywords are added.
 *
 * The values for CENTER_FILTER_WAVELENGTH and BAND_WIDTH are updated.
 *
 * @param inputCubeLab PvlObject pointer to the input cube labels
 * @param pdsLabel Pvl of the output PDS labels
 */
void updatePdsLabelImageObject(PvlObject *isisCubeLab, Pvl &pdsLabel) {
  // Add the image description to the IMAGE object in the label of the PDS product
  PvlObject &image = pdsLabel.findObject("IMAGE");
  image += PvlKeyword("DESCRIPTION",
                      "HiRISE mosaicked product, not map projected");

  // Add AlphaCube values to the IMAGE object
  // if AlphaCube doesn't exist in the Isis cube, add default values
  double sourceSamples = double(image["LINE_SAMPLES"]);
  double sourceLines = double(image["LINES"]);
  double firstSample = 0.5;
  double firstLine = 0.5;
  if (isisCubeLab->findObject("IsisCube").hasGroup("AlphaCube")) {
    PvlGroup alphaCubeGroup = isisCubeLab->findObject("IsisCube")
                                         .findGroup("AlphaCube");

    int alphaSamples = int(alphaCubeGroup["AlphaSamples"]);
    int alphaLines = int(alphaCubeGroup["AlphaLines"]);

    double alphaStartingSample = double(alphaCubeGroup["AlphaStartingSample"]);
    double alphaEndingSample = double(alphaCubeGroup["AlphaEndingSample"]);

    double alphaStartingLine = double(alphaCubeGroup["AlphaStartingLine"]);
    double alphaEndingLine = double(alphaCubeGroup["AlphaEndingLine"]);

    double betaSamples = double(alphaCubeGroup["BetaSamples"]);
    double betaLines = double(alphaCubeGroup["BetaLines"]);

    if ( betaSamples != double(image["LINE_SAMPLES"])
         || betaLines != double(image["LINES"])
         || (double) alphaSamples < betaSamples
         || (double) alphaLines < betaLines
         || betaSamples != (alphaEndingSample - alphaStartingSample)
         || betaLines != (alphaEndingLine - alphaStartingLine) ) {
      // The given input cube is not supported for export to PDS. The AlphaCube
      // group values indicate that this cube has been not only cropped, but
      // also scaled (reduced or enlarged)
      std::string msg = "The AlphaCube group values of the input Isis cube indicate "
                    "that this cube has been scaled. Unable to export scaled "
                    "cubes to PDS using hideal2pds.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    sourceSamples = alphaSamples;
    sourceLines = alphaLines;
    firstSample = alphaStartingSample;
    firstLine = alphaStartingLine;
  }
  image += PvlKeyword("SOURCE_LINE_SAMPLES", std::to_string(sourceSamples));
  image += PvlKeyword("SOURCE_LINES", std::to_string(sourceLines));
  image += PvlKeyword("FIRST_LINE_SAMPLE", std::to_string(firstSample));
  image += PvlKeyword("FIRST_LINE", std::to_string(firstLine));


  // Add center wavelength and bandwidth with correct units to the IMAGE object
  PvlKeyword &oldCenter = image["CENTER_FILTER_WAVELENGTH"];
  PvlKeyword newCenter("CENTER_FILTER_WAVELENGTH");
  for(int val = 0; val < oldCenter.size(); ++val) {
    if((QString::fromStdString((oldCenter.unit(val)))).toUpper() == "NANOMETERS") {
      newCenter.addValue(oldCenter[val], "NM");
    }
    else {
      newCenter.addValue(oldCenter[val], oldCenter.unit(val));
    }
  }
  image.addKeyword(newCenter, Pvl::Replace);

  PvlKeyword &oldBandWidth = image["BAND_WIDTH"];
  PvlKeyword newBandWidth("BAND_WIDTH");
  for(int val = 0; val < oldBandWidth.size(); ++val) {
    if((QString::fromStdString(oldBandWidth.unit(val))).toUpper() == "NANOMETERS") {
      newBandWidth.addValue(oldBandWidth[val], "NM");
    }
    else {
      newBandWidth.addValue(oldBandWidth[val], oldBandWidth.unit(val));
    }
  }
  image.addKeyword(newBandWidth, Pvl::Replace);
}


/**
 * This method updates the values of the keywords in the ROOT object of
 * the pds label file.
 *
 * The RATIONALE_DESC keyword is updated if the user entered this
 * parameter.
 *
 * The PRODUCT_VERSION_ID is added based on the user entered parameter.
 *
 * The NOT_APPLICABLE_CONSTANT keyword is added.
 *
 * The SOFTWARE_NAME keyword is determined and added.
 *
 * The SHAPE_MODEL keyword from the Kernels group of the input cube is
 * added with the path removed.
 *
 * The NaifKeywords values are added if the Object exists in the input
 * cube.  Otherwise, the corresponding values are calculated and added
 * to the pds labels. These values are added: BODY_FRAME_CODE,
 * IDEAL_FOCAL_LENGTH, IDEAL_PIXEL_PITCH, IDEAL_TRANSX, IDEAL_TRANSY,
 * IDEAL_TRANSS, and IDEAL_TRANSL. The BODY_RADII keyword is split into
 * A_AXIS_RADIUS, B_AXIS_RADIUS, and C_AXIS_RADIUS
 *
 * @param inputCubeLab PvlObject pointer to the input cube labels
 * @param pdsLabel Pvl of the output PDS labels
 * @param ui UserInterface reference, so that the user entered
 *           RATIONALE_DESC and PRODUCT_VERSION_ID parameters can be
 *           read in.
 * @param cam Pointer to the Camera object created from the input cube.
 */
void updatePdsLabelRootObject(PvlObject *isisCubeLab, Pvl &pdsLabel,
                              UserInterface &ui, Camera *cam) {
  // Replace INSTRUMENT_ID value in the output labels
  PvlKeyword instId("INSTRUMENT_ID", "HIRISE_IDEAL_CAMERA");
  pdsLabel.addKeyword(instId, PvlContainer::Replace);

  // Add user-entered keywords to ROOT object in the label of the PDS product
  if(ui.WasEntered("RATIONALE_DESC")) {
    PvlKeyword rationale("RATIONALE_DESC", ui.GetAsString("RATIONALE_DESC").toStdString());
    pdsLabel.addKeyword(rationale, PvlContainer::Replace);
  }
  else if ( !pdsLabel.hasKeyword("RATIONALE_DESC")
            || QString::fromStdString(pdsLabel["RATIONALE_DESC"]) == "NULL" ){
    std::string msg = "Unable to export HiRise product to PDS without "
                  "RationaleDescription value. The input cube value for this "
                  "keyword is Null, the user is required to enter a value.";
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }
  pdsLabel += PvlKeyword("PRODUCT_VERSION_ID", ui.GetString("VERSION").toStdString());

  // Add the N/A constant keyword to the ROOT object
  pdsLabel += PvlKeyword("NOT_APPLICABLE_CONSTANT", std::to_string(-9998));

  // Compute and add SOFTWARE_NAME to the ROOT object
  QString sfname;
  sfname.clear();
  sfname += "Isis " + Application::Version() + " " +
            Application::GetUserInterface().ProgramName();
  pdsLabel += PvlKeyword("SOFTWARE_NAME", sfname.toStdString());
  QString matchedCube = QString::fromStdString(isisCubeLab->findObject("IsisCube").findGroup("Instrument")
                                  .findKeyword("MatchedCube")[0]);
  FileName matchedCubeFileNoPath(matchedCube);
  pdsLabel += PvlKeyword("MATCHED_CUBE", matchedCubeFileNoPath.name().toStdString());

  // Add jitter correction flag value to the ROOT object
  bool jitter = false;
  if (isisCubeLab->findObject("IsisCube").findGroup("Instrument")
                 .hasKeyword("ImageJitterCorrected")) {
    jitter = std::stoi(isisCubeLab->findObject("IsisCube")
                            .findGroup("Instrument")["ImageJitterCorrected"][0]);
    pdsLabel += PvlKeyword("IMAGE_JITTER_CORRECTED", std::to_string((int)jitter));
  }
  else {
    pdsLabel += PvlKeyword("IMAGE_JITTER_CORRECTED", "UNK");
  }

  // Add Isis Kernels group keywords to the ROOT object
  QString shapeModel = QString::fromStdString(isisCubeLab->findObject("IsisCube").findGroup("Kernels")
                                  .findKeyword("ShapeModel")[0]);
  FileName shapeModelFileNoPath(shapeModel);
  pdsLabel += PvlKeyword("SHAPE_MODEL", shapeModelFileNoPath.name().toStdString());

  // PRODUCT_ID and SOURCE_PRODUCT_ID should be keywords added when creating the
  // mosaic input cube.

  // Add NaifKeywords Object values to the ROOT object
  QString radiiName = "BODY" + QString::number(cam->naifBodyCode()) + "_RADII";
  PvlObject naifKeywordGroup = cam->getStoredNaifKeywords();

  if (naifKeywordGroup.hasKeyword(radiiName.toStdString())) {
    PvlKeyword naifBodyRadii = naifKeywordGroup.findKeyword(radiiName.toStdString());
    pdsLabel += PvlKeyword("A_AXIS_RADIUS", naifBodyRadii[0], "KILOMETERS");
    pdsLabel += PvlKeyword("B_AXIS_RADIUS", naifBodyRadii[1], "KILOMETERS");
    pdsLabel += PvlKeyword("C_AXIS_RADIUS", naifBodyRadii[2], "KILOMETERS");
  }
  else {
    Distance naifBodyRadii[3];
    cam->radii(naifBodyRadii);
    pdsLabel += PvlKeyword("A_AXIS_RADIUS", std::to_string(naifBodyRadii[0].kilometers()), "KILOMETERS");
    pdsLabel += PvlKeyword("B_AXIS_RADIUS", std::to_string(naifBodyRadii[1].kilometers()), "KILOMETERS");
    pdsLabel += PvlKeyword("C_AXIS_RADIUS", std::to_string(naifBodyRadii[2].kilometers()), "KILOMETERS");
  }

  if (naifKeywordGroup.hasKeyword("BODY_FRAME_CODE")) {
    pdsLabel += naifKeywordGroup.findKeyword("BODY_FRAME_CODE");
  }
  else {
    pdsLabel += PvlKeyword("BODY_FRAME_CODE", std::to_string(cam->naifBodyFrameCode()));
  }

  if (naifKeywordGroup.hasKeyword("IDEAL_FOCAL_LENGTH")) {
    pdsLabel += naifKeywordGroup.findKeyword("IDEAL_FOCAL_LENGTH");
  }
  else {
    pdsLabel += PvlKeyword("IDEAL_FOCAL_LENGTH", std::to_string(cam->FocalLength()));
  }

  if (naifKeywordGroup.hasKeyword("IDEAL_PIXEL_PITCH")) {
    pdsLabel += naifKeywordGroup.findKeyword("IDEAL_PIXEL_PITCH");
  }
  else {
    pdsLabel += PvlKeyword("IDEAL_PIXEL_PITCH", std::to_string(cam->PixelPitch()));
  }

  if (naifKeywordGroup.hasKeyword("IDEAL_TRANSX")) {
    pdsLabel += naifKeywordGroup.findKeyword("IDEAL_TRANSX");
  }
  else {
    const double *transXValues = cam->FocalPlaneMap()->TransX();
    PvlKeyword transX("IDEAL_TRANSX");
    for (int i = 0; i < 3; i++) {
      transX += std::to_string(transXValues[i]);
    }
    pdsLabel += transX;
  }

  if (naifKeywordGroup.hasKeyword("IDEAL_TRANSY")) {
    pdsLabel += naifKeywordGroup.findKeyword("IDEAL_TRANSY");
  }
  else {
    const double *transYValues = cam->FocalPlaneMap()->TransY();
    PvlKeyword transY("IDEAL_TRANSY");
    for (int i = 0; i < 3; i++) {
      transY += std::to_string(transYValues[i]);
    }
    pdsLabel += transY;
  }

  if (naifKeywordGroup.hasKeyword("IDEAL_TRANSS")) {
    pdsLabel += naifKeywordGroup.findKeyword("IDEAL_TRANSS");
  }
  else {
    const double *transSValues = cam->FocalPlaneMap()->TransS();
    PvlKeyword transS("IDEAL_TRANSS");
    for (int i = 0; i < 3; i++) {
      transS += std::to_string(transSValues[i]);
    }
    pdsLabel += transS;
  }

  if (naifKeywordGroup.hasKeyword("IDEAL_TRANSL")) {
    pdsLabel += naifKeywordGroup.findKeyword("IDEAL_TRANSL");
  }
  else {
    const double *transLValues = cam->FocalPlaneMap()->TransL();
    PvlKeyword transL("IDEAL_TRANSL");
    for (int i = 0; i < 3; i++) {
      transL += std::to_string(transLValues[i]);
    }
    pdsLabel += transL;
  }
}


/**
 * This method updates the values of the keywords in the Time Parameters
 * Group of the pds label file.
 *
 * The PRODUCT_CREATION_TIME keyword is determined and added to the PDS
 * labels.
 *
 * @param pdsLabel Pvl of the output PDS labels
 */
void updatePdsLabelTimeParametersGroup(Pvl &pdsLabel) {
  // Calculate and add PRODUCT_CREATION_TIME to the TIME_PARAMETERS group
  time_t startTime = time(NULL);
  struct tm *tmbuf = gmtime(&startTime);
  char timestr[80];
  strftime(timestr, 80, "%Y-%m-%dT%H:%M:%S", tmbuf);
  QString dateTime = (QString) timestr;
  iTime tmpDateTime(dateTime);
  PvlGroup &timeParam = pdsLabel.findGroup("TIME_PARAMETERS");
  timeParam += PvlKeyword("PRODUCT_CREATION_TIME", tmpDateTime.UTC().toStdString());
}
