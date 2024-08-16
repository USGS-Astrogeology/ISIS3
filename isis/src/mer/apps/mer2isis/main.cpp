/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "ProcessImportPds.h"


/**
 * @author 2006-01-23 Brian Lipkowitz
 * @author 2006-01-23 Robert Wallace
 */

using namespace std;
using namespace Isis;

void IsisMain() {
  void TranslateMerEdrLabels(FileName & labelFile, Cube * ocube);

  UserInterface &ui = Application::GetUserInterface();

  FileName input = FileName(ui.GetFileName("FROM"));

  //Checks if in file is rdr
  Pvl lab(input.expanded().toStdString());
  if(lab.hasObject("IMAGE_MAP_PROJECTION")) {
    QString msg = "[" + input.name() + "] has already been projected.";
    msg += " Use pds2isis.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  QString output;
  if(ui.WasEntered("TO")) {
    output = ui.GetCubeName("TO");
  }
  else {
    output = input.path() + input.baseName() + ".cub";
  }

  Pvl inputFile;

  QString paramaters = "FROM=" + input.expanded();
  paramaters += " TO=" + output;

  ProcessImportPds p;
  p.SetPdsFile(input.expanded(), "", inputFile);

  Cube *ocube = p.SetOutputCube("TO");
  p.StartProcess();
  TranslateMerEdrLabels(input, ocube);
  p.EndProcess();
}

void TranslateMerEdrLabels(FileName &labelFile, Cube *ocube) {
  //Create a PVL to store the translated labels
  Pvl outLabel;

  // Get the directory where the MER translation tables are.
  QString transDir = "$ISISROOT/appdata/translations/";

  // Get a filename for the MESSENGER EDR label
  Pvl labelPvl(labelFile.expanded().toStdString());
  FileName transFile;

  // Translate the Archive group
  transFile = transDir + "MerStructure.trn";
  PvlToPvlTranslationManager structXlater(labelPvl, transFile.expanded());
  structXlater.Auto(outLabel);
  ocube->putGroup(outLabel.findGroup("ARCHIVE", Pvl::Traverse));

  // Translate the Instrument group
  transFile = transDir + "MerInstrument.trn";
  PvlToPvlTranslationManager instrumentXlater(labelPvl, transFile.expanded());
  instrumentXlater.Auto(outLabel);
  ocube->putGroup(outLabel.findGroup("INSTRUMENT", Pvl::Traverse));

  // Pull out MiCCD and MiElectronic from the TemperatureName in the instrument group
  void MiFixLab(PvlGroup & instGroup);
  PvlGroup instGroup = ocube->group("INSTRUMENT");
  MiFixLab(instGroup);
  ocube->putGroup(instGroup);

  // Translate the Image_Request group
  transFile = transDir + "MerImageRequest.trn";
  PvlToPvlTranslationManager imageReqXlater(labelPvl, transFile.expanded());
  imageReqXlater.Auto(outLabel);
  ocube->putGroup(outLabel.findGroup("MER_IMAGE_REQUEST_PARMS", Pvl::Traverse));

  // Translate the Subframe group
  transFile = transDir + "MerSubframe.trn";
  PvlToPvlTranslationManager subframeXlater(labelPvl, transFile.expanded());
  subframeXlater.Auto(outLabel);
  ocube->putGroup(outLabel.findGroup("MER_SUBFRAME_REQUEST_PARMS", Pvl::Traverse));
}

void MiFixLab(PvlGroup &instGroup) {
  // code to get instrment and electronics temperatures.
  PvlKeyword temp = instGroup.findKeyword("InstrumentTemperature");
  PvlKeyword tempName = instGroup.findKeyword("InstrumentTemperatureName");

  PvlKeyword miCCD;
  miCCD.setName("TemperatureMiCCD");
  miCCD.setValue(temp[6]);
  instGroup.addKeyword(miCCD);

  PvlKeyword miElectornic;
  miElectornic.setName("TemperatureMiElectronics");
  miElectornic.setValue(temp[7]);
  instGroup.addKeyword(miElectornic);

  //Code to remove "Z" from the StartTime and StopTime keywords
  //StartTime code
  QString Newstarttime =  QString::fromStdString(instGroup.findKeyword("StartTime"));
  Newstarttime.remove("Z");
  instGroup.findKeyword("StartTime").setValue(Newstarttime.toStdString());
  //StopTime code
  QString Newstoptime =  QString::fromStdString(instGroup.findKeyword("StopTime"));
  Newstoptime.remove("Z");
  instGroup.findKeyword("StopTime").setValue(Newstoptime.toStdString());
}
