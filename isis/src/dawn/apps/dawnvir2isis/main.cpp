/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <cstdio>
#include <QString>

#include "FileName.h"
#include "ImportPdsTable.h"
#include "ProcessImportPds.h"
#include "Table.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;

void IsisMain ()
{
  ProcessImportPds p;
  Pvl pdsLabel;
  UserInterface &ui = Application::GetUserInterface();

  FileName inFile = ui.GetFileName("FROM");
  QString imageFile("");
  if (ui.WasEntered("IMAGE")) {
    imageFile = ui.GetFileName("IMAGE");
  }


  // Generate the housekeeping filenames
  QString hkLabel("");
  QString hkData("");
  if (ui.WasEntered("HKFROM") ) {
    hkLabel = ui.GetFileName("HKFROM");
  }
  else {
    hkLabel = inFile.originalPath() + "/" + inFile.baseName() + "_HK.LBL";
    // Determine the housekeeping file
    FileName hkFile(hkLabel);
    if (!hkFile.fileExists()) {
      hkFile = hkLabel.replace("_1B_", "_1A_");
      if (hkFile.fileExists()) hkLabel = hkFile.expanded();
    }
  }

  if (ui.WasEntered("HKTABLE")) {
    hkData = ui.GetFileName("HKTABLE");
  }

  QString instid;
  QString missid;

  try {
    Pvl lab(inFile.expanded());
    instid = QString::fromStdString(lab.findKeyword ("CHANNEL_ID"));
    missid = QString::fromStdString(lab.findKeyword ("INSTRUMENT_HOST_ID"));
  }
  catch (IException &e) {
    std::string msg = "Unable to read [INSTRUMENT_ID] or [MISSION_ID] from input file [" +
                 inFile.expanded() + "]";
    throw IException(e, IException::Io,msg, _FILEINFO_);
  }

  instid = instid.simplified().trimmed();
  missid = missid.simplified().trimmed();
  if (missid != "DAWN" && instid != "VIS" && instid != "IR") {
    std::string msg = "Input file [" + inFile.expanded() + "] does not appear to be a " +
                 "DAWN Visual and InfraRed Mapping Spectrometer (VIR) EDR or RDR file.";
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }

  QString target;
  if (ui.WasEntered("TARGET")) {
    target = ui.GetString("TARGET");
  }

//  p.SetPdsFile (inFile.expanded(),imageFile,pdsLabel);
//  QString labelFile = ui.GetFileName("FROM");
  p.SetPdsFile (inFile.expanded(),imageFile,pdsLabel);
  p.SetOrganization(Isis::ProcessImport::BIP);
  Cube *outcube = p.SetOutputCube ("TO");
//  p.SaveFileHeader();

  Pvl labelPvl (inFile.expanded());

  p.StartProcess ();

  // Get the directory where the DAWN translation tables are.
  QString transDir = "$ISISROOT/appdata/translations/";

  // Create a PVL to store the translated labels in
  Pvl outLabel;

  // Translate the BandBin group
  FileName transFile (transDir + "DawnVirBandBin.trn");
  PvlToPvlTranslationManager bandBinXlater (labelPvl, transFile.expanded());
  bandBinXlater.Auto(outLabel);

  // Translate the Archive group
  transFile = transDir + "DawnVirArchive.trn";
  PvlToPvlTranslationManager archiveXlater (labelPvl, transFile.expanded());
  archiveXlater.Auto(outLabel);

  // Translate the Instrument group
  transFile = transDir + "DawnVirInstrument.trn";
  PvlToPvlTranslationManager instrumentXlater (labelPvl, transFile.expanded());
  instrumentXlater.Auto(outLabel);

  //  Update target if user specifies it
  if (!target.isEmpty()) {
    PvlGroup &igrp = outLabel.findGroup("Instrument",Pvl::Traverse);
    igrp["TargetName"] = target.toStdString();
  }

  // Write the BandBin, Archive, and Instrument groups
  // to the output cube label
  outcube->putGroup(outLabel.findGroup("BandBin",Pvl::Traverse));
  outcube->putGroup(outLabel.findGroup("Archive",Pvl::Traverse));
  outcube->putGroup(outLabel.findGroup("Instrument",Pvl::Traverse));

  PvlGroup kerns("Kernels");
  if (instid == "VIS") {
    kerns += PvlKeyword("NaifFrameCode","-203211");
  } else if (instid == "IR") {
    kerns += PvlKeyword("NaifFrameCode","-203213");
  } else {
    std::string msg = "Input file [" + inFile.expanded() + "] has an invalid " +
                 "InstrumentId.";
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }
  outcube->putGroup(kerns);

  // Now handle generation of housekeeping data
 try {
   ImportPdsTable hktable(hkLabel, hkData);
   hktable.setType("ScetTimeClock", "CHARACTER");
   hktable.setType("ShutterStatus", "CHARACTER");
   hktable.setType("MirrorSin", "DOUBLE");
   hktable.setType("MirrorCos", "DOUBLE");
   Table hktab = hktable.importTable("ScetTimeClock,ShutterStatus,MirrorSin,MirrorCos",
                                      "VIRHouseKeeping");
   hktab.Label().addKeyword(PvlKeyword("SourceFile", hkLabel.toStdString()));
   outcube->write(hktab);
 }
 catch (IException &e) {
   std::string mess = "Cannot read/open housekeeping data";
   throw IException(e, IException::User, mess, _FILEINFO_);
 }

  p.EndProcess ();
}
