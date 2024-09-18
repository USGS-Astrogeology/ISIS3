/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"
#include "ProcessImportPds.h"
#include "UserInterface.h"
#include "FileName.h"
#include "Pvl.h"
#include "PvlToPvlTranslationManager.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  QString labelFile = ui.GetFileName("FROM");

  ProcessImportPds p;
  Pvl label;
  p.SetPdsFile(labelFile, "", label);
  Cube *ocube = p.SetOutputCube("TO");
  p.StartProcess();

  std::string transDir = "$ISISROOT/appdata/translations/";

  Pvl inputLabel(labelFile.toStdString());
  Pvl outputLabel;
  PvlToPvlTranslationManager *translator;

  // translate Mapping group
  FileName transFile = transDir + "RoloMapping.trn";
  translator = new PvlToPvlTranslationManager(inputLabel, QString::fromStdString(transFile.expanded()));
  translator->Auto(outputLabel);
  delete translator;
  translator = NULL;

  // translate Instrument group
  transFile = transDir +  "RoloInstrument.trn";
  translator = new PvlToPvlTranslationManager(inputLabel, QString::fromStdString(transFile.expanded()));
  translator->Auto(outputLabel);
  delete translator;
  translator = NULL;

  // translate BandBin group
  transFile = transDir + "RoloBandBin.trn";
  translator = new PvlToPvlTranslationManager(inputLabel, QString::fromStdString(transFile.expanded()));
  translator->Auto(outputLabel);
  outputLabel.findGroup("BandBin").findKeyword("OriginalBand").setUnits(
    translator->Translate("BandBinUnit").toStdString());
  outputLabel.findGroup("BandBin").findKeyword("Center").setUnits(translator->
      Translate("BandBinUnit").toStdString());
  outputLabel.findGroup("BandBin").findKeyword("Width").setUnits(translator->
      Translate("BandBinUnit").toStdString());
  outputLabel.findGroup("BandBin").findKeyword("Exposure").setUnits(translator->
      Translate("ExposureUnit").toStdString());
  delete translator;
  translator = NULL;

  // translate Archive group
  transFile = transDir + "RoloArchive.trn";
  translator = new PvlToPvlTranslationManager(inputLabel, QString::fromStdString(transFile.expanded()));
  translator->Auto(outputLabel);
  delete translator;
  translator = NULL;

  // add outputLabel to cube
  if(outputLabel.hasGroup("Mapping") &&
      (outputLabel.findGroup("Mapping").keywords() > 0)) {
    ocube->putGroup(outputLabel.findGroup("Mapping"));
  }
  if(outputLabel.hasGroup("Instrument") &&
      (outputLabel.findGroup("Instrument").keywords() > 0)) {
    ocube->putGroup(outputLabel.findGroup("Instrument"));
  }
  if(outputLabel.hasGroup("BandBin") &&
      (outputLabel.findGroup("BandBin").keywords() > 0)) {
    ocube->putGroup(outputLabel.findGroup("BandBin"));
  }
  if(outputLabel.hasGroup("Archive") &&
      (outputLabel.findGroup("Archive").keywords() > 0)) {
    ocube->putGroup(outputLabel.findGroup("Archive"));
  }

  p.EndProcess();

  return;
}
