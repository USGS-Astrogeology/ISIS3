/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <cstdio>
#include <QString>

#include "ProcessImportPds.h"

#include "UserInterface.h"
#include "FileName.h"
#include "IException.h"
#include "IException.h"
#include "iTime.h"
#include "Preference.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  void TranslateMocEdrLabels(FileName & labelFile, Cube * ocube);

  ProcessImportPds p;
  Pvl pdsLabel;
  UserInterface &ui = Application::GetUserInterface();

  // Get the input filename and make sure it is a MOC EDR
  FileName in = ui.GetFileName("FROM");
  QString id;
  bool compressed = false;
  bool projected;
  try {
    Pvl lab(in.expanded().toStdString());
    id = QString::fromStdString(lab["DATA_SET_ID"]);
    if(lab.findObject("IMAGE").hasKeyword("ENCODING_TYPE")) compressed = true;
    projected = lab.hasObject("IMAGE_MAP_PROJECTION");
  }
  catch(IException &e) {
    QString msg = "Unable to read [DATA_SET_ID] from input file [" +
                 in.expanded() + "]";
    throw IException(e, IException::Io, msg, _FILEINFO_);
  }

  //Checks if in file is rdr
  if(projected) {
    QString msg = "[" + in.name() + "] appears to be an rdr file.";
    msg += " Use pds2isis.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  id = id.simplified().trimmed();
  if((id != "MGS-M-MOC-NA/WA-2-DSDP-L0-V1.0") &&
      (id != "MGS-M-MOC-NA/WA-2-SDP-L0-V1.0")) {
    QString msg = "Input file [" + in.expanded() + "] does not appear to be " +
                 "in MOC EDR format. DATA_SET_ID [" + id + "]";
    throw IException(IException::Io, msg, _FILEINFO_);
  }

  FileName uncompressed;

  // Set up conditional transfer of PDS labels to output cube
  FileName &translbl(in);

  // If the input file is compressed, use "mocuncompress" to uncompress it
  if(compressed) {
    // Get a temporary file for the uncompressed version incase we need it
    uncompressed = FileName::createTempFile("$TEMPORARY/" + in.baseName() + ".img");

    QString command = "mocuncompress " + in.expanded() + " " +
                      uncompressed.expanded();
    if(system(command.toLatin1().data()) == 1) {
      QString msg = "Unable to execute [mocuncompress]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    p.SetPdsFile(uncompressed.expanded(), "", pdsLabel);
    translbl = uncompressed;
  }
  else {
    p.SetPdsFile(in.expanded(), "", pdsLabel);
  }

  Cube *ocube = p.SetOutputCube("TO");
  p.StartProcess();
  TranslateMocEdrLabels(translbl, ocube);
  p.EndProcess();

  if(compressed) {
    QString uncompressedName(uncompressed.expanded());
    remove(uncompressedName.toLatin1().data());
  }

  return;
}

void TranslateMocEdrLabels(FileName &labelFile, Cube *ocube) {
  QString startTime, productId, clockCount;

  // Transfer the instrument group to the output cube
  QString transDir = "$ISISROOT/appdata/translations/";

  FileName transFile(transDir + "MgsMocInstrument.trn");

  // Get the translation manager ready
  Pvl labelPvl(labelFile.expanded().toStdString());
  PvlToPvlTranslationManager instrumentXlater(labelPvl, transFile.expanded());

  PvlGroup inst("Instrument");

  if(instrumentXlater.InputHasKeyword("SpacecraftName")) {
    QString str = instrumentXlater.Translate("SpacecraftName");
    inst += PvlKeyword("SpacecraftName", str.toStdString());
  }

  if(instrumentXlater.InputHasKeyword("InstrumentId")) {
    QString str = instrumentXlater.Translate("InstrumentId");
    inst += PvlKeyword("InstrumentId", str.toStdString());
  }

  if(instrumentXlater.InputHasKeyword("TargetName")) {
    QString str = instrumentXlater.Translate("TargetName");
    inst += PvlKeyword("TargetName", str.toStdString());
  }

  if(instrumentXlater.InputHasKeyword("StartTime")) {
    QString str = instrumentXlater.Translate("StartTime");
    inst += PvlKeyword("StartTime", str.toStdString());
    // isisLab.addKeyword ("StartTime", str+"Z");
    startTime = str;
  }

  if(instrumentXlater.InputHasKeyword("StopTime")) {
    QString str = instrumentXlater.Translate("StopTime");
    inst += PvlKeyword("StopTime", str.toStdString());
    // isisLab.addKeyword ("StopTime", str+"Z");
  }

  if(instrumentXlater.InputHasKeyword("CrosstrackSumming")) {
    QString str = instrumentXlater.Translate("CrosstrackSumming");
    inst += PvlKeyword("CrosstrackSumming", str.toStdString());
  }

  if(instrumentXlater.InputHasKeyword("DowntrackSumming")) {
    QString str = instrumentXlater.Translate("DowntrackSumming");
    inst += PvlKeyword("DowntrackSumming", str.toStdString());
  }

  if(instrumentXlater.InputHasKeyword("FocalPlaneTemperature")) {
    QString str = instrumentXlater.Translate("FocalPlaneTemperature");
    inst += PvlKeyword("FocalPlaneTemperature", str.toStdString());
  }

  if(instrumentXlater.InputHasKeyword("GainModeId")) {
    QString str = instrumentXlater.Translate("GainModeId");
    inst += PvlKeyword("GainModeId", str.toStdString());
  }

  if(instrumentXlater.InputHasKeyword("LineExposureDuration")) {
    QString str = instrumentXlater.Translate("LineExposureDuration");
    inst += PvlKeyword("LineExposureDuration", str.toStdString(), "milliseconds");
  }

  if(instrumentXlater.InputHasKeyword("MissionPhaseName")) {
    QString str = instrumentXlater.Translate("MissionPhaseName");
    inst += PvlKeyword("MissionPhaseName", str.toStdString());
  }

  if(instrumentXlater.InputHasKeyword("OffsetModeId")) {
    QString str = instrumentXlater.Translate("OffsetModeId");
    inst += PvlKeyword("OffsetModeId", str.toStdString());
  }

  if(instrumentXlater.InputHasKeyword("SpacecraftClockCount")) {
    QString str = instrumentXlater.Translate("SpacecraftClockCount");
    inst += PvlKeyword("SpacecraftClockCount", str.toStdString());
    clockCount = str;
  }

  if(instrumentXlater.InputHasKeyword("RationaleDesc")) {
    QString str = instrumentXlater.Translate("RationaleDesc");
    inst += PvlKeyword("RationaleDesc", str.toStdString());
  }

  if(instrumentXlater.InputHasKeyword("OrbitNumber")) {
    QString str = instrumentXlater.Translate("OrbitNumber");
    inst += PvlKeyword("OrbitNumber", str.toStdString());
  }

  if(instrumentXlater.InputHasKeyword("FirstLineSample")) {
    QString str = instrumentXlater.Translate("FirstLineSample");
    int num = toInt(str);
    num++;
    inst += PvlKeyword("FirstLineSample", std::to_string(num));
  }

  // Add the instrument specific info to the output file
  ocube->putGroup(inst);

  // Transfer the archive group to the output cube
  FileName transFileArchive(transDir + "MgsMocArchive.trn");

  // Get the translation manager ready for the archive group
  PvlToPvlTranslationManager archiveXlater(labelPvl, transFileArchive.expanded());

  PvlGroup arch("Archive");

  if(archiveXlater.InputHasKeyword("DataSetId")) {
    QString str = archiveXlater.Translate("DataSetId");
    arch += PvlKeyword("DataSetId", str.toStdString());
  }

  if(archiveXlater.InputHasKeyword("ProductId")) {
    QString str = archiveXlater.Translate("ProductId");
    arch += PvlKeyword("ProductId", str.toStdString());
    productId = str;
  }

  if(archiveXlater.InputHasKeyword("ProducerId")) {
    QString str = archiveXlater.Translate("ProducerId");
    arch += PvlKeyword("ProducerId", str.toStdString());
  }

  if(archiveXlater.InputHasKeyword("ProductCreationTime")) {
    QString str = archiveXlater.Translate("ProductCreationTime");
    arch += PvlKeyword("ProductCreationTime", str.toStdString());
  }

  if(archiveXlater.InputHasKeyword("SoftwareName")) {
    QString str = archiveXlater.Translate("SoftwareName");
    arch += PvlKeyword("SoftwareName", str.toStdString());
  }

  if(archiveXlater.InputHasKeyword("UploadId")) {
    QString str = archiveXlater.Translate("UploadId");
    arch += PvlKeyword("UploadId", str.toStdString());
  }

  if(archiveXlater.InputHasKeyword("DataQualityDesc")) {
    QString str = archiveXlater.Translate("DataQualityDesc");
    arch += PvlKeyword("DataQualityDesc", str.toStdString());
  }

  // New labels (not in the PDS file)

  // The ImageNumber is made up of pieces of the StartTime
  // The first piece is the
  //   Last digit of the year (eg, 1997 => 7), followed by the
  //   Day of the year (Julian day) followed by the
  //   Last five digits of the ProductId
  if(startTime.length() > 0 && productId.length() > 0) {
    iTime tim(startTime);
    QString imageNumber = tim.YearString();
    imageNumber = imageNumber.mid(3, 1);
    imageNumber += tim.DayOfYearString();

    imageNumber += productId.mid(4);

    arch += PvlKeyword("ImageNumber", imageNumber.toStdString());
  }

  // The ImageKeyId is made up of the:
  //   First five digits of the SpacecraftClockCount followed by the
  //   Last five dights of the ProductId
  if(clockCount.length() > 0 && productId.length() > 0) {
    arch += PvlKeyword("ImageKeyId", clockCount.mid(0, 5).toStdString() +
                       productId.mid(4).toStdString());
  }

  // Add the archive info to the output file
  ocube->putGroup(arch);

  // Create the BandBin Group and populate it
  FileName transFileBandBin(transDir + "MgsMocBandBin.trn");

  // Get the translation manager ready for the BandBin group
  PvlToPvlTranslationManager bandBinXlater(labelPvl, transFileBandBin.expanded());

  PvlGroup bandBin("BandBin");
  QString frameCode;

  if(bandBinXlater.InputHasKeyword("FilterName")) {
    QString str = bandBinXlater.Translate("FilterName").toUpper();

    if(str == "BLUE") {
      bandBin += PvlKeyword("FilterName", str.toStdString());
      bandBin += PvlKeyword("OriginalBand", std::to_string(1));
      bandBin += PvlKeyword("Center", std::to_string(0.4346), "micrometers");
      bandBin += PvlKeyword("Width", std::to_string(0.050), "micrometers");
      frameCode = "-94033";
    }
    else if(str == "RED") {
      bandBin += PvlKeyword("FilterName", str.toStdString());
      bandBin += PvlKeyword("OriginalBand", std::to_string(1));
      bandBin += PvlKeyword("Center", std::to_string(0.6134), "micrometers");
      bandBin += PvlKeyword("Width", std::to_string(0.050), "micrometers");
      frameCode = "-94032";
    }
    else {
      QString msg = "Invalid value for filter name [" + str + "]";
    }
  }
  else {
    bandBin += PvlKeyword("FilterName", "BROAD_BAND");
    bandBin += PvlKeyword("OriginalBand", std::to_string(1));
    bandBin += PvlKeyword("Center", std::to_string(0.700), "micrometers");
    bandBin += PvlKeyword("Width", std::to_string(0.400), "micrometers");
    frameCode = "-94031";
  }

  // Add the bandbin info to the output file
  ocube->putGroup(bandBin);

  // Create the Kernel Group
  PvlGroup kerns("Kernels");
  kerns += PvlKeyword("NaifFrameCode", frameCode.toStdString());
  ocube->putGroup(kerns);
}
