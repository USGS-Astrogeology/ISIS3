/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "hicubeit.h"

#include "TextFile.h"
#include "Pvl.h"
#include "Cube.h"
#include "OriginalLabel.h"
#include "ProgramLauncher.h"
#include <QString>

using namespace std;

namespace Isis {

  void hicubeit(UserInterface &ui) {
    QString redFile = ui.GetCubeName("RED");
    QString irFile  = ui.GetCubeName("IR");
    QString bgFile  = ui.GetCubeName("BG");

    FileName tempFile = FileName::createTempFile("$TEMPORARY/hicubeit.temp.lis");
    TextFile tf;
    tf.Open(tempFile.expanded(), "output");
    tf.PutLine(irFile + "\n");
    tf.PutLine(redFile + "\n");
    tf.PutLine(bgFile + "\n");
    tf.Close();

    QString parameters = QString(" FROMLIST = ")    + tempFile.expanded() +
                        QString(" TO = ")      + ui.GetCubeName("TO") +
                        QString(" PROPLAB = ") + redFile;
    ProgramLauncher::RunIsisProgram("cubeit", parameters);
    remove(tempFile.expanded().toLatin1().data());

    // Get the instrument group from each file
    Pvl redLab(redFile.toStdString());
    Pvl irLab(irFile.toStdString());
    Pvl bgLab(bgFile.toStdString());

    PvlGroup redInst = redLab.findGroup("Instrument", Pvl::Traverse);
    PvlGroup irInst  = irLab.findGroup("Instrument", Pvl::Traverse);
    PvlGroup bgInst  = bgLab.findGroup("Instrument", Pvl::Traverse);

    // Error check to make sure the proper ccds are stacked
    if((int)redInst["CpmmNumber"] == 5) {
      if(((int)irInst["CpmmNumber"] != 6) || ((int)bgInst["CpmmNumber"] != 4)) {
        std::string msg = "You can only stack color images with RED4, IR10, and BG12 ";
        msg += "or RED5, IR11, and BG13";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    else if((int)redInst["CpmmNumber"] == 8) {
      if(((int)irInst["CpmmNumber"] != 7) || ((int)bgInst["CpmmNumber"] != 9)) {
        std::string msg = "You can only stack color images with RED4, IR10, and BG12 ";
        msg += "or RED5, IR11, and BG13";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    else {
      std::string msg = "You can only stack color images with RED4, IR10, and BG12 ";
      msg += "or RED5, IR11, and BG13";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Concatenate all the source products into one keyword
    PvlKeyword sourceProductId("SourceProductId");
    sourceProductId += bgInst["StitchedProductIds"][0];
    if(bgInst["StitchedProductIds"].size() > 1) {
      sourceProductId += bgInst["StitchedProductIds"][1];
    }
    sourceProductId += redInst["StitchedProductIds"][0];
    if(redInst["StitchedProductIds"].size() > 1) {
      sourceProductId += redInst["StitchedProductIds"][1];
    }
    sourceProductId += irInst["StitchedProductIds"][0];
    if(irInst["StitchedProductIds"].size() > 1) {
      sourceProductId += irInst["StitchedProductIds"][1];
    }

    // Get min start and max stop time
    PvlKeyword startTime = redInst["StartTime"];
    PvlKeyword stopTime  = redInst["StopTime"];
    PvlKeyword startClk  = redInst["SpacecraftClockStartCount"];
    PvlKeyword stopClk   = redInst["SpacecraftClockStopCount"];

    if((std::string) irInst["StartTime"] < (std::string)startTime) {
      startTime = irInst["StartTime"];
    }
    if((std::string) bgInst["StartTime"] < (std::string)startTime) {
      startTime = bgInst["StartTime"];
    }

    if((std::string) irInst["StopTime"] > (std::string)stopTime) {
      stopTime = irInst["StopTime"];
    }
    if((std::string) bgInst["StopTime"] > (std::string)stopTime) {
      stopTime = bgInst["StopTime"];
    }

    if((std::string) irInst["SpacecraftClockStartCount"] < (std::string)startClk) {
      startClk = irInst["SpacecraftClockStartCount"];
    }
    if((std::string) bgInst["SpacecraftClockStartCount"] < (std::string)startClk) {
      startClk = bgInst["SpacecraftClockStartCount"];
    }

    if((std::string) irInst["SpacecraftClockStopCount"] > (std::string)stopClk) {
      stopClk = irInst["SpacecraftClockStopCount"];
    }
    if((std::string) bgInst["SpacecraftClockStopCount"] > (std::string)stopClk) {
      stopClk = bgInst["SpacecraftClockStopCount"];
    }

    // Concatenate all TDIs into one keyword
    OriginalLabel redOrgLab(redFile);
    OriginalLabel irOrgLab(irFile);
    OriginalLabel bgOrgLab(bgFile);

    PvlGroup redGrp = redOrgLab.ReturnLabels().findGroup("INSTRUMENT_SETTING_PARAMETERS", Pvl::Traverse);
    PvlGroup irGrp = irOrgLab.ReturnLabels().findGroup("INSTRUMENT_SETTING_PARAMETERS", Pvl::Traverse);
    PvlGroup bgGrp = bgOrgLab.ReturnLabels().findGroup("INSTRUMENT_SETTING_PARAMETERS", Pvl::Traverse);

    PvlKeyword cpmmTdiFlag("cpmmTdiFlag");
    for(int i = 0; i < 14; i++) {
      cpmmTdiFlag += "";
    }
    cpmmTdiFlag[(int)redInst["CpmmNumber"]] = (std::string) redGrp["MRO:TDI"];
    cpmmTdiFlag[(int)irInst["CpmmNumber"]] = (std::string) irGrp["MRO:TDI"];
    cpmmTdiFlag[(int)bgInst["CpmmNumber"]] = (std::string) bgGrp["MRO:TDI"];

    // Concatenate all summing modes into one keyword
    PvlKeyword cpmmSummingFlag("cpmmSummingFlag");
    for(int i = 0; i < 14; i++) {
      cpmmSummingFlag += "";
    }
    cpmmSummingFlag[(int)redInst["CpmmNumber"]] = (std::string) redGrp["MRO:BINNING"];
    cpmmSummingFlag[(int)irInst["CpmmNumber"]] = (std::string) irGrp["MRO:BINNING"];
    cpmmSummingFlag[(int)bgInst["CpmmNumber"]] = (std::string) bgGrp["MRO:BINNING"];

    //Concatenate all the Special_Processing_Flag into one keyword
    PvlKeyword specialProcessingFlag("SpecialProcessingFlag");
    for(int i = 0; i < 14; i++) {
      specialProcessingFlag += "";
    }
    //keyword Special_Processing_Flag may not be present so need to test
    // if not present set to NOMINAL
    if(redInst.hasKeyword("Special_Processing_Flag")) {
      specialProcessingFlag[redInst["CpmmNumber"]] = (std::string) redInst["Special_Processing_Flag"];
    }
    else {
      specialProcessingFlag[redInst["CpmmNumber"]] = "NOMINAL";
    }
    if(irInst.hasKeyword("Special_Processing_Flag")) {
      specialProcessingFlag[irInst["CpmmNumber"]] = (std::string) irInst["Special_Processing_Flag"];
    }
    else {
      specialProcessingFlag[irInst["CpmmNumber"]] = "NOMINAL";
    }
    if(bgInst.hasKeyword("Special_Processing_Flag")) {
      specialProcessingFlag[bgInst["CpmmNumber"]] = (std::string) bgInst["Special_Processing_Flag"];
    }
    else {
      specialProcessingFlag[bgInst["CpmmNumber"]] = "NOMINAL";
    }
    // Put them in a group
    PvlGroup mos("Mosaic");
    mos += sourceProductId;
    mos += startTime;
    mos += stopTime;
    mos += startClk;
    mos += stopClk;
    mos += cpmmTdiFlag;
    mos += cpmmSummingFlag;
    mos += specialProcessingFlag;

    // Add the group to the output cube
    Cube c;
    c.open(ui.GetCubeName("TO"), "rw");
    c.label()->findObject("IsisCube", Pvl::Traverse).addGroup(mos);
    c.close();
  }
}
