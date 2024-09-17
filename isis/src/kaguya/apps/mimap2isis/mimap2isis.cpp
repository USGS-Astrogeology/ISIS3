/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "mimap2isis.h"

#include "Application.h"
#include "FileName.h"
#include "ProcessImportPds.h"

#include <QFile>

using namespace std;

namespace Isis {
  void mimap2isis(UserInterface &ui, Pvl *log) {
  ProcessImportPds p;
  Pvl label;

  QString labelFile = ui.GetFileName("FROM");
  label.read(labelFile.toStdString());

  // The Kaguya MI MAP files have an incorrect SAMPLE_PROJECTION_OFFSET
  // keyword value in their labels. The following code creates a temporary
  // detached PDS label with the correct (negated) keyword value.
  PvlObject obj = label.findObject("IMAGE_MAP_PROJECTION");
  double soff = std::stod(obj.findKeyword("SAMPLE_PROJECTION_OFFSET")[0]);
  soff = -soff;
  label.findObject("IMAGE_MAP_PROJECTION").addKeyword(PvlKeyword("SAMPLE_PROJECTION_OFFSET",std::to_string(soff)),Pvl::Replace);
  FileName tempFileName = FileName::createTempFile("TEMPORARYlabel.pvl").name();
  QString fn(QString::fromStdString(tempFileName.expanded()));
  label.write(fn.toStdString());

  QString imageFile("");
  QString datafile = labelFile;
  if (ui.WasEntered("IMAGE")) {
    imageFile = ui.GetFileName("IMAGE");
    datafile = imageFile;
  }

  p.SetPdsFile(label, datafile);
  QFile::remove(fn);

  Cube *ocube = p.SetOutputCube("TO", ui);

  // Get user entered special pixel ranges
  if(ui.GetBoolean("SETNULLRANGE")) {
    p.SetNull(ui.GetDouble("NULLMIN"), ui.GetDouble("NULLMAX"));
  }
  if(ui.GetBoolean("SETHRSRANGE")) {
    p.SetHRS(ui.GetDouble("HRSMIN"), ui.GetDouble("HRSMAX"));
  }
  if(ui.GetBoolean("SETHISRANGE")) {
    p.SetHIS(ui.GetDouble("HISMIN"), ui.GetDouble("HISMAX"));
  }
  if(ui.GetBoolean("SETLRSRANGE")) {
    p.SetLRS(ui.GetDouble("LRSMIN"), ui.GetDouble("LRSMAX"));
  }
  if(ui.GetBoolean("SETLISRANGE")) {
    p.SetLIS(ui.GetDouble("LISMIN"), ui.GetDouble("LISMAX"));
  }

  // Export the cube
  p.StartProcess();

  // Translate the mapping labels
  Pvl otherLabels;
  p.TranslatePdsProjection(otherLabels);

  // Translate the remaining MI MAP labels
  std::string transDir = "$ISISROOT/appdata/translations/";

  FileName transFile(transDir + "KaguyaMiMapBandBin.trn");
  PvlToPvlTranslationManager bandBinXlater(label, QString::fromStdString(transFile.expanded()));
  bandBinXlater.Auto(otherLabels);

  transFile = transDir + "KaguyaMiMapInstrument.trn";
  PvlToPvlTranslationManager instXlater(label, QString::fromStdString(transFile.expanded()));
  instXlater.Auto(otherLabels);

  PvlKeyword processId = label.findKeyword("PROCESS_VERSION_ID");
  PvlKeyword productVersion = label.findKeyword("PRODUCT_VERSION_ID");

  if (processId[0] == "L3C") {
    transFile = transDir + "KaguyaMil3cArchive.trn";;
  }
  else if (processId[0] == "MAP") {
    transFile = transDir + "KaguyaMiMapArchive.trn";

    if (int(productVersion) == 3) {
      transFile = transDir + "KaguyaMiMap3Archive.trn";
    }
  }

  PvlToPvlTranslationManager archiveXlater(label, QString::fromStdString(transFile.expanded()));
  archiveXlater.Auto(otherLabels);

  if(otherLabels.hasGroup("Mapping") &&
      (otherLabels.findGroup("Mapping").keywords() > 0)) {
    ocube->putGroup(otherLabels.findGroup("Mapping"));
  }
  if(otherLabels.hasGroup("Instrument") &&
      (otherLabels.findGroup("Instrument").keywords() > 0)) {
    ocube->putGroup(otherLabels.findGroup("Instrument"));
  }
  if(otherLabels.hasGroup("BandBin") &&
      (otherLabels.findGroup("BandBin").keywords() > 0)) {
    ocube->putGroup(otherLabels.findGroup("BandBin"));
  }
  if(otherLabels.hasGroup("Archive") &&
      (otherLabels.findGroup("Archive").keywords() > 0)) {
    ocube->putGroup(otherLabels.findGroup("Archive"));
  }

  //  Check for and log any change from the default projection offsets and multipliers
  if (log && p.GetProjectionOffsetChange()) {
    PvlGroup results = p.GetProjectionOffsetGroup();
    results.setName("Results");
    results[0].addComment("Projection offsets and multipliers have been changed from");
    results[0].addComment("defaults. New values are below.");
    Application::Log(results);
  }

  p.EndProcess();

  return;
  }
}
