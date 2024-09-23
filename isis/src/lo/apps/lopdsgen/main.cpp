/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "iTime.h"
#include "ProcessExportPds.h"
#include "PvlToPvlTranslationManager.h"
#include "PvlFormatPds.h"
#include "OriginalLabel.h"

using namespace std;
using namespace Isis;

enum Pixtype { NONE, NEG, BOTH };

void setRangeAndPixels(UserInterface &ui, ProcessExportPds &p,
                       double &min, double &max, Pixtype ptype);
void IsisMain() {
  // Set the processing object
  ProcessExportPds p;

  // Setup the input cube
  Cube *iCube = p.SetInputCube("FROM");

  UserInterface &ui = Application::GetUserInterface();

  double min = -DBL_MAX;
  double max = DBL_MAX;

  if(ui.GetString("BITTYPE") == "8BIT") {
    p.SetOutputType(Isis::UnsignedByte);
    min = 0.0;
    max = 255.0;
    setRangeAndPixels(ui, p, min, max, BOTH);
  }
  else if(ui.GetString("BITTYPE") == "S16BIT") {
    p.SetOutputType(Isis::SignedWord);
    min = -32768.0;
    max = 32767.0;
    setRangeAndPixels(ui, p, min, max, NEG);
  }
  else if(ui.GetString("BITTYPE") == "U16BIT") {
    p.SetOutputType(Isis::UnsignedWord);
    min = 0.0;
    max = 65535.0;
    setRangeAndPixels(ui, p, min, max, BOTH);
  }
  else {
    p.SetOutputType(Isis::Real);
    p.SetOutputNull(Isis::NULL4);
    p.SetOutputLrs(Isis::LOW_REPR_SAT4);
    p.SetOutputLis(Isis::LOW_INSTR_SAT4);
    p.SetOutputHrs(Isis::HIGH_REPR_SAT4);
    p.SetOutputHis(Isis::HIGH_INSTR_SAT4);
    setRangeAndPixels(ui, p, min, max, NONE);
  }

  if(ui.GetString("ENDIAN") == "MSB")
    p.SetOutputEndian(Isis::Msb);
  else if(ui.GetString("ENDIAN") == "LSB")
    p.SetOutputEndian(Isis::Lsb);

  p.SetExportType(ProcessExportPds::Fixed);

  // Get the PDS label from the process, omitting keywords unnecessary for LO
  p.ForceBands(false);
  p.ForceBandName(false);
  p.ForceCenterFilterWavelength(false);
  p.ForceBandStorageType(false);
  p.ForceOffset(false);
  p.ForceScalingFactor(false);
  Pvl &pdsLabel = p.StandardPdsLabel(ProcessExportPds::Image);

  // Add PRODUCT_ID keyword, the first part of the output filename
  FileName outFileNoExt(ui.GetFileName("TO").toStdString());
  outFileNoExt = outFileNoExt.removeExtension();
  QString productID(QString::fromStdString(outFileNoExt.baseName()));
  PvlKeyword productId("PRODUCT_ID", productID.toUpper().toStdString());
  pdsLabel.addKeyword(productId);

  // Translate the keywords from the original labels that go in this label
  OriginalLabel origBlob = iCube->readOriginalLabel();
  Pvl origLabel;
  PvlObject origLabelObj = origBlob.ReturnLabels();
  origLabelObj.setName("OriginalLabelObject");
  origLabel.addObject(origLabelObj);

  // Transfer the instrument group to the output cube
  QString transDir = "$ISISROOT/appdata/translations/";

  // Isis cubes being exported for the first time
  if(!origLabel.hasKeyword("PRODUCT_TYPE", Pvl::Traverse)) {

    PvlToPvlTranslationManager orig(origLabel, transDir + "LoOriginalExport.trn");
    orig.Auto(pdsLabel);

    // Add elements of SCAN_PARAMETER keyword to label
    PvlObject &qube = origLabelObj.findObject("QUBE");
    PvlGroup &ingestion = qube.findGroup("ISIS_INGESTION");

    // Strips and level1 products use the following two keywords in different ways
    PvlKeyword &scanResolution = ingestion.findKeyword("SCAN_RESOLUTION");
    PvlKeyword &scanDensityRange = ingestion.findKeyword("SCAN_DENSITY_RANGE");

    // Change the units of SCAN_RESOLUTION from "um" to "<micron>"
    QString scanResolutionStr = QString::fromStdString(scanResolution[0]);
    int umPos = scanResolutionStr.indexOf("um");
    QString resolution = scanResolutionStr.mid(0, umPos);
    scanResolution[0] = resolution.toStdString();
    scanResolution.setUnits("micron");
    scanResolution.setName("LO:FILMSTRIP_SCAN_RESOLUTION");

    // Break keyword SCAN_DENSITY_RANGE into two different parts
    QString scanDensityRangeStr = QString::fromStdString(scanDensityRange[0]);
    int toPos = scanDensityRangeStr.indexOf("_TO_");
    QString range1 = scanDensityRangeStr.mid(0, toPos);
    QString range2 = scanDensityRangeStr.mid(toPos + 4);

    // Label translation for strips
    if(qube.findGroup("ISIS_INSTRUMENT").hasKeyword("STRIP_NUMBER")) {
      PvlToPvlTranslationManager orig(origLabel, transDir + "LoStripExport.trn");
      orig.Auto(pdsLabel);

      pdsLabel.addKeyword(scanResolution, PvlContainer::Replace);
    }
    // Translation for level1 products
    else if(qube.findGroup("ISIS_INSTRUMENT").hasKeyword("START_TIME")) {
      PvlToPvlTranslationManager orig(origLabel, transDir + "LoLevel1Export.trn");
      orig.Auto(pdsLabel);

      pdsLabel.addKeyword(scanResolution, PvlContainer::Replace);

      PvlKeyword &outputMicron = qube.findKeyword("OUTPUT_MICRON");

      // Change the units of OUTPUT_MICRON from "um" to "<micron>"
      QString outputMicronStr = QString::fromStdString(outputMicron[0]);
      umPos = outputMicronStr.indexOf("um");
      resolution = outputMicronStr.mid(0, umPos);
      outputMicron[0] = resolution.toStdString();
      outputMicron.setUnits("micron");
      outputMicron.setName("LO:FILMSTRIP_SCAN_PROCESSING_RES");

      pdsLabel.addKeyword(outputMicron, PvlContainer::Replace);

      // Calculate statistics on the cube to be processed and place
      // its MINIMUM and MAXIMUM into the output label
      p.CalculateStatistics();
      pdsLabel.findObject("IMAGE").addKeyword(PvlKeyword("MINIMUM", Isis::toString(p.CubeStatistics(0)->Minimum())), Pvl::Replace);
      pdsLabel.findObject("IMAGE").addKeyword(PvlKeyword("MAXIMUM", Isis::toString(p.CubeStatistics(0)->Maximum())), Pvl::Replace);
    }
    else {
      FileName inputFile(ui.GetCubeName("FROM").toStdString());
      std::string msg = "[" + inputFile.expanded() + "] does not appear to be an LO file.  ";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }
  // Reexporting a product created by this program and reingested into Isis
  else {

    PvlToPvlTranslationManager orig(origLabel, transDir + "LoReimportExport.trn");
    orig.Auto(pdsLabel);

    // Reexporting strips
    if(origLabel.hasKeyword("STRIP_NUMBER", Pvl::Traverse)) {
      PvlToPvlTranslationManager strip(origLabel, transDir + "LoStripExport.trn");
      strip.Auto(pdsLabel);
    }
    // Reexporting level 1 products
    else {
      PvlToPvlTranslationManager lvl1(origLabel, transDir + "LoLevel1Export.trn");
      lvl1.Auto(pdsLabel);
    }
  }

  // Add to labels boresight or fiducial data
  QString bandBinTransFile;
  if(iCube->label()->hasKeyword("FiducialId", Pvl::Traverse)) {
    bandBinTransFile = transDir + "LoFiducialExport.trn";
    PvlToPvlTranslationManager bandLab(*(iCube->label()), bandBinTransFile);
    bandLab.Auto(pdsLabel);

    // Change the units of FIDCUAIL_COORDINATE_MICRON from "um" to "<micron>"
    PvlKeyword &coordMicron = iCube->label()->findKeyword(
        "FiducialCoordinateMicron", Pvl::Traverse);
    QString coordMicronStr = QString::fromStdString(coordMicron[0]);
    int umPos = coordMicronStr.indexOf("um");
    QString coord = coordMicronStr.mid(0, umPos);
    coordMicron[0] = coord.toStdString();
    coordMicron.setUnits("micron");
    coordMicron.setName("LO:FIDUCIAL_COORDINATE_MICRON");

    pdsLabel.addKeyword(coordMicron, PvlContainer::Replace);
  }
  else if(iCube->label()->hasKeyword("BoresightSample", Pvl::Traverse)) {
    bandBinTransFile = transDir + "LoBoresightExport.trn";
    PvlToPvlTranslationManager bandLab(*(iCube->label()), bandBinTransFile);
    bandLab.Auto(pdsLabel);
  }
  else {
    FileName inputFile(ui.GetCubeName("FROM").toStdString());
    std::string msg = "[" + inputFile.expanded() + "] does not contain boresight or fiducial information.  ";
    msg += "Try ingesting your data with lo2isis first.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Add PRODUCT_CREATION_TIME
  time_t startTime = time(NULL);
  struct tm *tmbuf = gmtime(&startTime);
  char timestr[80];
  strftime(timestr, 80, "%Y-%m-%dT%H:%M:%S", tmbuf);
  QString dateTime = (QString) timestr;
  iTime tmpDateTime(dateTime);
  pdsLabel += PvlKeyword("PRODUCT_CREATION_TIME", tmpDateTime.UTC().toStdString());

  if(ui.WasEntered("NOTE")) {
    pdsLabel.addKeyword(PvlKeyword("NOTE", ui.GetString("NOTE").toStdString()), Pvl::Replace);
  }

  // Add a keyword type (i.e., QString, bool, int...) file to the PDS label Pvl
  PvlFormatPds *formatter = new PvlFormatPds();
  formatter->setCharLimit(128);
  formatter->add(transDir.toStdString() + "LoExportFormatter.typ");
  pdsLabel.setFormat(formatter);

  // Add an output format template (group, object, & keyword output order) to
  // the PDS PVL
  QString formatDir = "$ISISROOT/appdata/translations/";
  pdsLabel.setFormatTemplate(formatDir.toStdString() + "LoExportTemplate.pft");

  // Write labels to output file
  FileName outFile(ui.GetFileName("TO", "img").toStdString());
  QString outFileName(QString::fromStdString(outFile.expanded()));
  ofstream oCube(outFileName.toLatin1().data());
  p.OutputLabel(oCube);
  p.StartProcess(oCube);
  oCube.close();
  p.EndProcess();

  return;
}

//Sets up special pixels and valid pixel ranges
void setRangeAndPixels(UserInterface &ui, ProcessExportPds &p, double &min, double &max, Pixtype ptype) {
  if(ptype == NEG) {
    if(ui.GetBoolean("NULL")) {
      p.SetOutputNull(min++);
    }
    if(ui.GetBoolean("LRS")) {
      p.SetOutputLrs(min++);
    }
    if(ui.GetBoolean("LIS")) {
      p.SetOutputLis(min++);
    }
    if(ui.GetBoolean("HIS")) {
      p.SetOutputHis(min++);
    }
    if(ui.GetBoolean("HRS")) {
      p.SetOutputHrs(min++);
    }
  }
  else if(ptype == BOTH) {
    if(ui.GetBoolean("NULL")) {
      p.SetOutputNull(min++);
    }
    if(ui.GetBoolean("LRS")) {
      p.SetOutputLrs(min++);
    }
    if(ui.GetBoolean("LIS")) {
      p.SetOutputLis(min++);
    }
    if(ui.GetBoolean("HRS")) {
      p.SetOutputHrs(max--);
    }
    if(ui.GetBoolean("HIS")) {
      p.SetOutputHis(max--);
    }
  }
  p.SetOutputRange(min, max);
}
