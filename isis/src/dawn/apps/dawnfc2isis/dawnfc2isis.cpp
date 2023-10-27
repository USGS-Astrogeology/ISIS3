/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cstdio>

#include <QFile>
#include <QString>

#include "ProcessImportPds.h"
#include "ProcessBySample.h"
#include "UserInterface.h"
#include "FileName.h"

using namespace std;

namespace Isis {

  void flipbyline(Buffer &in, Buffer &out);

  void dawnfc2isis(UserInterface &ui) {
    ProcessImportPds p;
    Pvl pdsLabel;

    FileName inFile = ui.GetFileName("FROM");
    QString instid;
    QString missid;

    try {
      Pvl lab(inFile.expanded().toStdString());
      instid = QString::fromStdString(lab.findKeyword("INSTRUMENT_ID"));
      missid = QString::fromStdString(lab.findKeyword("MISSION_ID"));
    }
    catch(IException &e) {
      QString msg = "Unable to read [INSTRUMENT_ID] or [MISSION_ID] from input file [" +
                   inFile.expanded() + "]";
      throw IException(IException::Io, msg, _FILEINFO_);
    }

    instid = instid.simplified().trimmed();
    missid = missid.simplified().trimmed();
    if(missid != "DAWN" && instid != "FC1" && instid != "FC2") {
      QString msg = "Input file [" + inFile.expanded() + "] does not appear to be " +
                   "a DAWN Framing Camera (FC) EDR or RDR file.";
      throw IException(IException::Io, msg, _FILEINFO_);
    }

    QString target;
    if(ui.WasEntered("TARGET")) {
      target = ui.GetString("TARGET");
    }


    p.SetPdsFile(inFile.expanded(), "", pdsLabel);
    p.SetOrganization(Isis::ProcessImport::BSQ);
    QString tmpName = "$TEMPORARY/" + inFile.baseName() + ".tmp.cub";
    FileName tmpFile(tmpName);
    CubeAttributeOutput outatt = CubeAttributeOutput("+Real");
    p.SetOutputCube(tmpFile.expanded(), outatt);
    p.SaveFileHeader();

    Pvl labelPvl(inFile.expanded().toStdString());

    p.StartProcess();
    p.EndProcess();

    ProcessBySample p2;
    CubeAttributeInput inatt;
    p2.SetInputCube(tmpFile.expanded(), inatt);
    Cube *outcube = p2.SetOutputCube("TO");

    // Get the directory where the DAWN translation tables are.
    QString transDir = "$ISISROOT/appdata/translations/";

    // Create a PVL to store the translated labels in
    Pvl outLabel;

    // Translate the BandBin group
    FileName transFile(transDir + "DawnFcBandBin.trn");
    PvlToPvlTranslationManager bandBinXlater(labelPvl, transFile.expanded());
    bandBinXlater.Auto(outLabel);

    // Translate the Archive group
    transFile = transDir + "DawnFcArchive.trn";
    PvlToPvlTranslationManager archiveXlater(labelPvl, transFile.expanded());
    archiveXlater.Auto(outLabel);

    // Translate the Instrument group
    transFile = transDir + "DawnFcInstrument.trn";
    PvlToPvlTranslationManager instrumentXlater(labelPvl, transFile.expanded());
    instrumentXlater.Auto(outLabel);

    //  Update target if user specifies it
    if (!target.isEmpty()) {
      PvlGroup &igrp = outLabel.findGroup("Instrument",Pvl::Traverse);
      igrp["TargetName"] = target.toStdString();
    }

    // Write the BandBin, Archive, and Instrument groups
    // to the output cube label
    outcube->putGroup(outLabel.findGroup("BandBin", Pvl::Traverse));
    outcube->putGroup(outLabel.findGroup("Archive", Pvl::Traverse));
    outcube->putGroup(outLabel.findGroup("Instrument", Pvl::Traverse));

    // Set the BandBin filter name, center, and width values based on the
    // FilterNumber.
    PvlGroup &bbGrp(outLabel.findGroup("BandBin", Pvl::Traverse));
    int filtno = bbGrp["FilterNumber"];
    int center;
    int width;
    QString filtname;
    if(filtno == 1) {
      center = 700;
      width = 700;
      filtname = "Clear_F1";
    }
    else if(filtno == 2) {
      center = 555;
      width = 43;
      filtname = "Green_F2";
    }
    else if(filtno == 3) {
      center = 749;
      width = 44;
      filtname = "Red_F3";
    }
    else if(filtno == 4) {
      center = 917;
      width = 45;
      filtname = "NIR_F4";
    }
    else if(filtno == 5) {
      center = 965;
      width = 85;
      filtname = "NIR_F5";
    }
    else if(filtno == 6) {
      center = 829;
      width = 33;
      filtname = "NIR_F6";
    }
    else if(filtno == 7) {
      center = 653;
      width = 42;
      filtname = "Red_F7";
    }
    else if(filtno == 8) {
      center = 438;
      width = 40;
      filtname = "Blue_F8";
    }
    else {
      QString msg = "Input file [" + inFile.expanded() + "] has an invalid " +
                   "FilterNumber. The FilterNumber must fall in the range 1 to 8.";
      throw IException(IException::Io, msg, _FILEINFO_);
    }
    bbGrp.addKeyword(PvlKeyword("Center", std::to_string(center)));
    bbGrp.addKeyword(PvlKeyword("Width", std::to_string(width)));
    bbGrp.addKeyword(PvlKeyword("FilterName", filtname.toStdString()));
    outcube->putGroup(bbGrp);

    PvlGroup kerns("Kernels");
    if(instid == "FC1") {
      kerns += PvlKeyword("NaifFrameCode", std::to_string(-203110-filtno));
    }
    else if(instid == "FC2") {
      kerns += PvlKeyword("NaifFrameCode", std::to_string(-203120-filtno));
    }
    else {
      QString msg = "Input file [" + inFile.expanded() + "] has an invalid " +
                   "InstrumentId.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    outcube->putGroup(kerns);

    p2.StartProcess(flipbyline);
    p2.EndProcess();

    QString tmp(tmpFile.expanded());
    QFile::remove(tmp);
  }

  // Flip image by line
  void flipbyline(Buffer &in, Buffer &out) {
    int index = in.size() - 1;
    for(int i = 0; i < in.size(); i++) {
      out[i] = in[index - i];
    }
  }
}
