/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <fstream>
#include <vector>

#include "ImageHistogram.h"
#include "iTime.h"
#include "IString.h"
#include "JP2Encoder.h"
#include "LineManager.h"
#include "OriginalLabel.h"
#include "PixelType.h"
#include "ProcessExportPds.h"
#include "TProjection.h"
#include "ProjectionFactory.h"
#include "PvlFormatPds.h"
#include "PvlToPvlTranslationManager.h"

using namespace Isis;
using namespace std;

double *g_min, *g_max;

PixelType g_oType;
char **g_jp2buf;
JP2Encoder *g_jp2Encoder;
int g_jp2ns;
int g_jp2nb;
int g_jp2band;

void writeJP2Image(Buffer &in);

void IsisMain() {

  QString projName;

  Process pHist;
  Cube *icube = pHist.SetInputCube("FROM");

  // Check to see if the input cube looks like a HiRISE RDR
  if (icube->bandCount() > 3) {
    std::string msg = "Input file [" +
                 Application::GetUserInterface().GetCubeName("FROM").toStdString() +
                 "] does not appear to be a HiRISE RDR product. Number of " +
                 "bands is greater than 3";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }

  // Setup to get a histogram for each band
  g_min = new double[icube->bandCount()];
  g_max = new double[icube->bandCount()];

  UserInterface &ui = Application::GetUserInterface();

  // Determine if the data is to be converted to JPEG2000
  IString enctype = ui.GetString("ENCODING_TYPE").toStdString();
  enctype.DownCase();

  for (int band = 1; band <= icube->bandCount(); ++band) {

    if (ui.GetString("TYPE").compare("AUTOMATIC") == 0) {
      // Set up a histogram for this band. This call sets the input range
      // by making an initial stats pass to find the data min and max
      ImageHistogram hist(*icube, band, pHist.Progress());

      // Loop and accumulate histogram
      pHist.Progress()->SetText("Gathering Histogram");
      pHist.Progress()->SetMaximumSteps(icube->lineCount());
      pHist.Progress()->CheckStatus();
      LineManager line(*icube);
      for (int i = 1; i <= icube->lineCount(); i++) {
        line.SetLine(i, band);
        icube->read(line);
        hist.AddData(line.DoubleBuffer(), line.size());
        pHist.Progress()->CheckStatus();
      }

      // get the requested cumulative percentages
      g_min[band-1] = ui.GetDouble("MINPER") == 0.0 ? hist.Minimum() : hist.Percent(ui.GetDouble("MINPER"));
      g_max[band-1] = ui.GetDouble("MAXPER") == 100.0 ? hist.Maximum() : hist.Percent(ui.GetDouble("MAXPER"));
    }
    else {
      g_min[band-1] = ui.GetDouble("MIN");
      g_max[band-1] = ui.GetDouble("MAX");
    }
  }

  // Find the minimum min and maximum max for all bands
  double minmin = g_min[0];
  double maxmax = g_max[0];
  for (int band = 1; band < icube->bandCount(); ++band) {
    if (g_min[band] < minmin) minmin = g_min[band];
    if (g_max[band] > maxmax) maxmax = g_max[band];
  }

  pHist.EndProcess();

  // Set up for writing the data to a PDS formatted file
  ProcessExportPds p;
  Cube *icube2 = p.SetInputCube("FROM");

  if (enctype.Equal("jp2")) {
    g_jp2buf = new char* [icube2->bandCount()];
    FileName lblFile(ui.GetFileName("TO").toStdString());
    QString lblFileName = QString::fromStdString(lblFile.path() + "/" + lblFile.baseName() + ".lbl");
    p.SetDetached(lblFileName);
    p.setFormat(ProcessExport::JP2);
  }

  // Set the output pixel type and the special pixel values
  int nbits = ui.GetInteger("BITS");
  if (nbits == 8) {
    if (enctype.Equal("jp2")) {
      for (int i = 0; i < icube2->bandCount(); i++) {
        g_jp2buf[i] = new char[icube2->sampleCount()];
      }
    }
    g_oType = Isis::UnsignedByte;
    p.SetOutputType(g_oType);
    p.SetOutputRange(VALID_MIN1, VALID_MAX1);
    p.SetOutputNull(NULL1);
    p.SetOutputLis(LOW_INSTR_SAT1);
    p.SetOutputLrs(LOW_REPR_SAT1);
    p.SetOutputHis(HIGH_INSTR_SAT1);
    p.SetOutputHrs(HIGH_REPR_SAT1);
  }
  else if (nbits == 16) {
    if (enctype.Equal("jp2")) {
      for (int i = 0; i < icube2->bandCount(); i++) {
        g_jp2buf[i] = new char[icube2->sampleCount()*2];
      }
    }
    g_oType = UnsignedWord;
    p.SetOutputType(g_oType);
    p.SetOutputRange(VALID_MINU2, VALID_MAXU2);
    p.SetOutputNull(NULLU2);
    p.SetOutputLis(LOW_INSTR_SATU2);
    p.SetOutputLrs(LOW_REPR_SATU2);
    p.SetOutputHis(HIGH_INSTR_SATU2);
    p.SetOutputHrs(HIGH_REPR_SATU2);
  }
  else {
    if (enctype.Equal("jp2")) {
      for (int i = 0; i < icube2->bandCount(); i++) {
        g_jp2buf[i] = new char[icube2->sampleCount()*2];
      }
    }
    g_oType = UnsignedWord;
    p.SetOutputType(g_oType);
    p.SetOutputRange(3.0, pow(2.0, (double)(nbits)) - 1.0 - 2.0);
    p.SetOutputNull(0);
    p.SetOutputLrs(1);
    p.SetOutputLis(2);
    p.SetOutputHis(pow(2.0, (double)(nbits)) - 1.0 - 1.0);
    p.SetOutputHrs(pow(2.0, (double)(nbits)) - 1.0);
  }
  p.SetOutputEndian(Isis::Msb);
  p.SetInputRange(minmin, maxmax);

  // Get the PDS label from the process
  ProcessExportPds::PdsFileType type;
  if (enctype.Equal("jp2")) {
    type = ProcessExportPds::JP2Image;
  }
  else {
    type = ProcessExportPds::Image;
  }
  Pvl &pdsLabel = p.StandardPdsLabel(type);

  // Translate the keywords from the input cube label that go in the PDS label
  PvlToPvlTranslationManager cubeLab(*(icube2->label()),
                                "$ISISROOT/appdata/translations/MroHirisePdsRdrCubeLabel.trn");
  cubeLab.Auto(pdsLabel);

  // Translate the keywords from the original EDR PDS label that go in
  // this RDR PDS label
  OriginalLabel origBlob = icube2->readOriginalLabel();
  Pvl origLabel;
  PvlObject origLabelObj = origBlob.ReturnLabels();
  origLabelObj.setName("OriginalLabelObject");
  origLabel.addObject(origLabelObj);
  PvlToPvlTranslationManager orig(origLabel,
                             "$ISISROOT/appdata/translations/MroHirisePdsRdrOriginalLabel.trn");
  orig.Auto(pdsLabel);

  // Add labels to the PDS product that could not be handled by the translater

  if (ui.WasEntered("RATIONALE_DESC")) {
    pdsLabel.addKeyword(
        PvlKeyword("RATIONALE_DESC", ui.GetString("RATIONALE_DESC").toStdString()),
        Pvl::Replace);
  }

  // Add PRODUCT_CREATION_TIME
  time_t startTime = time(NULL);
  struct tm *tmbuf = gmtime(&startTime);
  char timestr[80];
  strftime(timestr, 80, "%Y-%m-%dT%H:%M:%S", tmbuf);
  QString dateTime = (QString) timestr;
  iTime tmpDateTime(dateTime);
  PvlGroup &timeParam = pdsLabel.findGroup("TIME_PARAMETERS");
  timeParam += PvlKeyword("PRODUCT_CREATION_TIME", tmpDateTime.UTC().toStdString());

  // Add the N/A constant keyword to the ROOT
  pdsLabel += PvlKeyword("NOT_APPLICABLE_CONSTANT", toString(-9998));

  // Add SOFTWARE_NAME to the ROOT
  QString sfname;
  sfname.clear();
  sfname += "Isis " + Application::Version() + " " +
            Application::GetUserInterface().ProgramName();
  pdsLabel += PvlKeyword("SOFTWARE_NAME", sfname.toStdString());

  // Add the PRODUCT_VERSION_ID from the user parameter VERSION
  pdsLabel += PvlKeyword("PRODUCT_VERSION_ID", ui.GetString("VERSION").toStdString());

  // Add MRO:CCD_FLAG, MRO:BINNING, MRO:TDI
  // As pulled from the input Isis cube, the values are in CPMM order, so
  // convert them to CCD order
  PvlKeyword ccdFlag("MRO:CCD_FLAG");
  PvlKeyword &cpmmFlag = origLabel.findObject("OriginalLabelObject").
                         findGroup("INSTRUMENT_SETTING_PARAMETERS").
                         findKeyword("MRO:POWERED_CPMM_FLAG");
  PvlKeyword ccdBin("MRO:BINNING");
  PvlKeyword &cpmmBin = icube2->label()->findObject("IsisCube").
                        findGroup("Mosaic")["cpmmSummingFlag"];
  PvlKeyword ccdTdi("MRO:TDI");
  PvlKeyword &cpmmTdi = icube2->label()->findObject("IsisCube").
                        findGroup("Mosaic")["cpmmTdiFlag"];
  PvlKeyword ccdSpecial("MRO:SPECIAL_PROCESSING_FLAG");
  PvlKeyword &cpmmSpecial = icube2->label()->findObject("IsisCube").
                            findGroup("Mosaic")["SpecialProcessingFlag"];
  for (int ccd = 0; ccd < 14; ++ccd) {
    const unsigned int cpmmByCcd[] = {0, 1, 2, 3, 5, 8, 10,
                                      11, 12, 13, 6, 7, 4, 9};
    ccdFlag.addValue(cpmmFlag[cpmmByCcd[ccd]]);
    ccdBin.addValue(cpmmBin[cpmmByCcd[ccd]] != "Null" ? cpmmBin[cpmmByCcd[ccd]] : "-9998");
    ccdTdi.addValue(cpmmTdi[cpmmByCcd[ccd]] != "Null" ? cpmmTdi[cpmmByCcd[ccd]] : "-9998");
    IString tmp = cpmmSpecial[cpmmByCcd[ccd]];
    tmp.Trim("\"");
    ccdSpecial.addValue(tmp);
  }

  if (!pdsLabel.hasGroup("INSTRUMENT_SETTING_PARAMETERS")) {
    pdsLabel.addGroup(PvlGroup("INSTRUMENT_SETTING_PARAMETERS"));
  }
  pdsLabel.findGroup("INSTRUMENT_SETTING_PARAMETERS") += ccdFlag;
  pdsLabel.findGroup("INSTRUMENT_SETTING_PARAMETERS") += ccdBin;
  pdsLabel.findGroup("INSTRUMENT_SETTING_PARAMETERS") += ccdTdi;
  pdsLabel.findGroup("INSTRUMENT_SETTING_PARAMETERS") += ccdSpecial;

  // Add/modify projection info if there is a projection
  if (pdsLabel.hasObject("IMAGE_MAP_PROJECTION")) {
    PvlObject &mapObject = pdsLabel.findObject("IMAGE_MAP_PROJECTION");
    mapObject += PvlKeyword("^DATA_SET_MAP_PROJECTION", "DSMAP.CAT");

    // Add the HiRISE comment to the CENTER_LATITUDE keyword
    PvlKeyword &clat = mapObject["CENTER_LATITUDE"];
    clat.addComment("/* NOTE:  CENTER_LATITUDE and CENTER_LONGITUDE describe the location  */");
    clat.addComment("/* of the center of projection, which is not necessarily equal to the */");
    clat.addComment("/* location of the center point of the image.                         */");

    if (mapObject.hasKeyword("CENTER_LATITUDE")) {
      PvlKeyword &centerLat = mapObject["CENTER_LATITUDE"];
      // if (centerLat[0] == "N/A") centerLat = -9998;
      if (centerLat[0] == "N/A") mapObject.deleteKeyword("CENTER_LATITUDE");
    }
    if (mapObject.hasKeyword("CENTER_LONGITUDE")) {
      PvlKeyword &centerLon = mapObject["CENTER_LONGITUDE"];
      // if (centerLon[0] == "N/A") centerLon = -9998;
      if (centerLon[0] == "N/A") mapObject.deleteKeyword("CENTER_LONGITUDE");
    }
    if (mapObject.hasKeyword("REFERENCE_LATITUDE")) {
      PvlKeyword &refLat = mapObject["REFERENCE_LATITUDE"];
      // if (refLat[0] == "N/A") refLat = -9998;
      if (refLat[0] == "N/A") mapObject.deleteKeyword("REFERENCE_LATITUDE");
    }
    if (mapObject.hasKeyword("REFERENCE_LONGITUE")) {
      PvlKeyword &refLon = mapObject["REFERENCE_LONGITUDE"];
      // if (refLon[0] == "N/A") refLon = -9998;
      if (refLon[0] == "N/A") mapObject.deleteKeyword("REFERENCE_LONGITUDE");
    }
    if (mapObject.hasKeyword("FIRST_STANDARD_PARALLEL")) {
      PvlKeyword &firstSP = mapObject["FIRST_STANDARD_PARALLEL"];
      // if (firstSP[0] == "N/A") firstSP = -9998;
      if (firstSP[0] == "N/A") mapObject.deleteKeyword("FIRST_STANDARD_PARALLEL");
    }
    if (mapObject.hasKeyword("SECOND_STANDARD_PARALLEL")) {
      PvlKeyword &secondSP = mapObject["SECOND_STANDARD_PARALLEL"];
      // if (secondSP[0] == "N/A") secondSP = -9998;
      if (secondSP[0] == "N/A") mapObject.deleteKeyword("SECOND_STANDARD_PARALLEL");
    }

    // For Equirectangular ONLY
    // Modify the radii in the pds label to use the radius at the center latitude
    // instead of the target radii from NAIF
    if (mapObject["MAP_PROJECTION_TYPE"][0] == "EQUIRECTANGULAR") {
      TProjection *proj = (TProjection *) ProjectionFactory::CreateFromCube(*icube2);
      PvlGroup &mapping = icube2->label()->findGroup("MAPPING", Pvl::Traverse);
      double radius = proj->LocalRadius((double)mapping["CenterLatitude"]) / 1000.0;
      mapObject["A_AXIS_RADIUS"].setValue(toString(radius), "KM");
      mapObject["B_AXIS_RADIUS"].setValue(toString(radius), "KM");
      mapObject["C_AXIS_RADIUS"].setValue(toString(radius), "KM");
    }

    projName = QString::fromStdString(mapObject["MAP_PROJECTION_TYPE"][0]);
  }

  // Calculate the min/max per band keywords
  // These come from the input real DN and are converted to the PDS file DN
  // The input to output mapping is opposite from the one above
  double slope = (p.GetOutputMaximum() - p.GetOutputMinimum()) / (maxmax - minmin);
  double intercept = p.GetOutputMaximum() - slope * maxmax;
  PvlKeyword minimum("MRO:MINIMUM_STRETCH", toString(slope * g_min[0] + intercept));
  PvlKeyword maximum("MRO:MAXIMUM_STRETCH", toString(slope * g_max[0] + intercept));
  for (int band = 1; band < icube2->bandCount(); ++band) {
    minimum += toString(slope * g_min[band] + intercept);
    maximum += toString(slope * g_max[band] + intercept);
  }

  if (enctype.Equal("jp2")) {
    // Add keywords to the PDS JP2 IMAGE object
    PvlObject &imagejp2 = pdsLabel.findObject("UNCOMPRESSED_FILE").findObject("IMAGE");

    // Add the HiRISE specific description of the IMAGE object
    imagejp2 += PvlKeyword("DESCRIPTION", "HiRISE projected and mosaicked product");

    // Add the SCALLING_FACTOR and OFFSET keywords
    imagejp2.addKeyword(PvlKeyword("SCALING_FACTOR", toString(slope)), Pvl::Replace);
    imagejp2.addKeyword(PvlKeyword("OFFSET", toString(intercept)), Pvl::Replace);

    // Reformat some keyword units in the image object
    // This is lame, but PDS units are difficult to work with, so for now???
    PvlKeyword &oldFilterNamejp2 = imagejp2["FILTER_NAME"];
    PvlKeyword newFilterName("FILTER_NAME");
    for (int val = 0; val < oldFilterNamejp2.size(); ++val) {
      QString  filtname(QString::fromStdString(oldFilterNamejp2[val]).toUpper());
      if (filtname == "BLUEGREEN") filtname = "BLUE-GREEN";
      else if (filtname == "NEARINFRARED") filtname = "NEAR-INFRARED";
      newFilterName.addValue(filtname.toStdString());
    }
    imagejp2.addKeyword(newFilterName, Pvl::Replace);

    PvlKeyword &oldCenterjp2 = imagejp2["CENTER_FILTER_WAVELENGTH"];
    PvlKeyword newCenter("CENTER_FILTER_WAVELENGTH");
    for (int val = 0; val < oldCenterjp2.size(); ++val) {
      if (((IString)(oldCenterjp2.unit(val))).UpCase() == "NANOMETERS") {
        newCenter.addValue(oldCenterjp2[val], "NM");
      }
      else {
        newCenter.addValue(oldCenterjp2[val], oldCenterjp2.unit(val));
      }
    }
    imagejp2.addKeyword(newCenter, Pvl::Replace);

    PvlKeyword &oldBandWidthjp2 = imagejp2["BAND_WIDTH"];
    PvlKeyword newBandWidth("BAND_WIDTH");
    for (int val = 0; val < oldBandWidthjp2.size(); ++val) {
      if (((IString)(oldBandWidthjp2.unit(val))).UpCase() == "NANOMETERS") {
        newBandWidth.addValue(oldBandWidthjp2[val], "nm");
      }
      else {
        newBandWidth.addValue(oldBandWidthjp2[val], oldBandWidthjp2.unit(val));
      }
    }
    imagejp2.addKeyword(newBandWidth, Pvl::Replace);

    // Add the min/max per band keywords
    imagejp2 += minimum;
    imagejp2 += maximum;

    // Modify the default SAMPLE_BIT_MASK keyword placed there by the
    // ProcessExportPds
    if (nbits != 8 && nbits != 16) {
      imagejp2.addKeyword(PvlKeyword("SAMPLE_BIT_MASK",
                                     toString((int)pow(2.0, (double)ui.GetInteger("BITS")) - 1)),
                          Pvl::Replace);
    }
  }
  else {
    // Add keywords to the PDS IMAGE object
    PvlObject &image = pdsLabel.findObject("IMAGE");

    // Add the HiRISE specific description of the IMAGE object
    image += PvlKeyword("DESCRIPTION", "HiRISE projected and mosaicked product");

    /**
     *  Calculate the SCALING_FACTOR and OFFSET keywords
     *  Set these so the unsigned 16bit PDS disk values can be converted back
     *  to the correct values Isis had
     *  These keywords are used to map stored/disk values to the correct values so,
     *  the input(x axis) values are the unsigned Xbit values from the PDS file
     */
    // ??? unneccessary calculation - this is done by ProcessExportPds class.
    double slope = (maxmax - minmin) / (p.GetOutputMaximum() - p.GetOutputMinimum());
    double intercept = maxmax - slope * p.GetOutputMaximum();
    image.addKeyword(PvlKeyword("SCALING_FACTOR", toString(slope)), Pvl::Replace);
    image.addKeyword(PvlKeyword("OFFSET", toString(intercept)), Pvl::Replace);

    // Reformat some keyword units in the image object
    // This is lame, but PDS units are difficult to work with, so for now
    PvlKeyword &oldFilterName = image["FILTER_NAME"];
    PvlKeyword newFilterName("FILTER_NAME");
    for (int val = 0; val < oldFilterName.size(); ++val) {
      QString  filtname(QString::fromStdString(oldFilterName[val]).toUpper());
      if (filtname == "BLUEGREEN") filtname = "BLUE-GREEN";
      else if (filtname == "NEARINFRARED") filtname = "NEAR-INFRARED";
      newFilterName.addValue(filtname.toStdString());
    }
    image.addKeyword(newFilterName, Pvl::Replace);

    PvlKeyword &oldCenter = image["CENTER_FILTER_WAVELENGTH"];
    PvlKeyword newCenter("CENTER_FILTER_WAVELENGTH");
    for (int val = 0; val < oldCenter.size(); ++val) {
      if (((IString)(oldCenter.unit(val))).UpCase() == "NANOMETERS") {
        newCenter.addValue(oldCenter[val], "NM");
      }
      else {
        newCenter.addValue(oldCenter[val], oldCenter.unit(val));
      }
    }
    image.addKeyword(newCenter, Pvl::Replace);

    PvlKeyword &oldBandWidth = image["BAND_WIDTH"];
    PvlKeyword newBandWidth("BAND_WIDTH");
    for (int val = 0; val < oldBandWidth.size(); ++val) {
      if (((IString)(oldBandWidth.unit(val))).UpCase() == "NANOMETERS") {
        newBandWidth.addValue(oldBandWidth[val], "NM");
      }
      else {
        newBandWidth.addValue(oldBandWidth[val], oldBandWidth.unit(val));
      }
    }
    image.addKeyword(newBandWidth, Pvl::Replace);

    // Add the min/max per band keywords
    image += minimum;
    image += maximum;

    // Modify the default SAMPLE_BIT_MASK keyword placed there by the
    // ProcessExportPds
    if (nbits != 8 && nbits != 16) {
      image.addKeyword(PvlKeyword("SAMPLE_BIT_MASK",
                                  toString((int)pow(2.0, (double)ui.GetInteger("BITS")) - 1)),
                       Pvl::Replace);
    }
  }

  // Modify the units in the viewing_parameters group
//  if (pdsLabel.hasGroup("VIEWING_PARAMETERS")) {
//    PvlGroup &viewGroup = pdsLabel.findGroup("VIEWING_PARAMETERS");

//    PvlKeyword &incidence = viewGroup["INCIDENCE_ANGLE"];
//    IString tstr = incidence.unit();
//    if (tstr.UpCase() == "DEG") incidence.setValue((QString)incidence, "deg");

//    PvlKeyword &emission = viewGroup["EMISSION_ANGLE"];
//    tstr = emission.unit();
//    if (tstr.UpCase() == "DEG") emission.setValue((QString)emission, "deg");

//    PvlKeyword &phase = viewGroup["PHASE_ANGLE"];
//    tstr = phase.unit();
//    if (tstr.UpCase() == "DEG") phase.setValue((QString)phase, "deg");

//    PvlKeyword &solarLon = viewGroup["SOLAR_LONGITUDE"];
//    tstr = solarLon.unit();   q
//    if (tstr.UpCase() == "DEG") solarLon.setValue((QString)solarLon, "deg");

//    PvlKeyword &localTime = viewGroup["LOCAL_TIME"];
//    tstr = localTime.unit();
//    if (tstr.UpCase() == "LOCALDAY/24") localTime.setValue((QString)localTime, "local day/24");
//  }

  // Add a keyword type (i.e., QString, bool, int...) file to the PDS label Pvl
  PvlFormat *formatter = pdsLabel.format();
  formatter->add("$ISISROOT/appdata/translations/MroHirisePdsRdrExtras.typ");

  // Add an output format template (group, object, & keyword output order) to
  // the PDS PVL
  if (projName == "EQUIRECTANGULAR") {
    if (enctype.Equal("jp2")) {
      pdsLabel.setFormatTemplate("$ISISROOT/appdata/translations/MroHirisePdsRdrEquiJP2.pft");
    }
    else {
      pdsLabel.setFormatTemplate("$ISISROOT/appdata/translations/MroHirisePdsRdrEqui.pft");
    }
  }
  else {
    if (enctype.Equal("jp2")) {
      pdsLabel.setFormatTemplate("$ISISROOT/appdata/translations/MroHirisePdsRdrPolarJP2.pft");
    }
    else {
      pdsLabel.setFormatTemplate("$ISISROOT/appdata/translations/MroHirisePdsRdrPolar.pft");
    }
  }

  // Open the output PDS file and dump the label and cube data
  if (enctype.Equal("jp2")) {
    p.OutputDetachedLabel();
    g_jp2Encoder = new JP2Encoder(ui.GetFileName("TO"), icube2->sampleCount(),
                                 icube2->lineCount(), icube2->bandCount(), g_oType);
    g_jp2Encoder->OpenFile();
    g_jp2ns = icube2->sampleCount();
    g_jp2nb = icube2->bandCount();
    g_jp2band = 0;
    p.StartProcess(writeJP2Image);
    p.EndProcess();
    delete g_jp2Encoder;
    for (int i = 0; i < icube2->bandCount(); i++) {
      delete [] g_jp2buf[i];
    }
  }
  else {
    FileName outFile(ui.GetFileName("TO").toStdString());
    ofstream oCube(outFile.expanded().c_str());
    p.OutputLabel(oCube);
    p.StartProcess(oCube);
    oCube.close();
    p.EndProcess();
  }

  delete [] g_min;
  delete [] g_max;
}

void writeJP2Image(Buffer &in) {
  for (int i = 0; i < g_jp2ns; i++) {
    if (g_oType == Isis::UnsignedByte) {
      ((unsigned char *)g_jp2buf[g_jp2band])[i] = (unsigned char)((int)in[i]);
    }
    else {
      ((short unsigned int *)g_jp2buf[g_jp2band])[i] = (short unsigned int)((int)in[i]);
    }
  }
  g_jp2band++;
  if (g_jp2band == g_jp2nb) {
    g_jp2band = 0;
    if (g_oType == Isis::UnsignedByte) {
      g_jp2Encoder->Write((unsigned char **)g_jp2buf);
    }
    else {
      g_jp2Encoder->Write((short int **)g_jp2buf);
    }
  }
}
