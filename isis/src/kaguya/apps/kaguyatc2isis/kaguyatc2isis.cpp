/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "kaguyatc2isis.h"

#include <cstdio>
#include <string>

#include "FileName.h"
#include "ProcessImportPds.h"
#include "UserInterface.h"
#include "Pvl.h"

using namespace std;

namespace Isis {
  void kaguyatc2isis(UserInterface &ui, Pvl *log) {
    ProcessImportPds importPds;
    FileName inFile = ui.GetFileName("FROM");
    QString labelFile = inFile.expanded();
    Pvl label(labelFile.toStdString());

    QString dataFile = "";
    if ( inFile.extension().toLower() == "lbl" ) {
      dataFile = inFile.path() + "/" + QString::fromStdString(label.findKeyword("FILE_NAME"));
    }
    else {
      dataFile = labelFile;
    }

    QString id = "";
    try {
      id = QString::fromStdString(label.findKeyword("DATA_SET_ID"));
    }
    catch(IException &e) {
      std::string msg = "Unable to read [DATA_SET_ID] from label file ["
                    + labelFile + "]";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }

    id = id.simplified().trimmed();
    if (id != "TC_MAP"
        && id != "TCO_MAP"
        && id != "TC1_Level2B"
        && id != "TC2_Level2B"
        && id != "SLN-L-TC-3-S-LEVEL2B0-V1.0"
        && id != "SLN-L-TC-3-W-LEVEL2B0-V1.0"
        && id != "SLN-L-TC-3-SP-SUPPORT-LEVEL2B0-V1.0"
        && id != "SLN-L-TC-5-MORNING-MAP-V4.0") {
      std::string msg = "Input file [" + labelFile + "] does not appear to be " +
                    "a supported Kaguya Terrain Camera format. " +
                    "DATA_SET_ID is [" + id + "]" +
                    "Valid formats include [TC_MAP, TCO_MAP, TC1_Level2B, " +
                    "SLN-L-TC-3-S-LEVEL2B0-V1.0, SLN-L-TC-3-W-LEVEL2B0-V1.0, " +
		                "SLN-L-TC-3-SP-SUPPORT-LEVEL2B0-V1.0, SLN-L-TC-5-MORNING-MAP-V4.0]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    if (!label.hasKeyword("TARGET_NAME")) {
      label.addKeyword(PvlKeyword("TARGET_NAME", "MOON"), Pvl::Replace);
    }

    importPds.SetPdsFile(label, dataFile);

    CubeAttributeOutput &att = ui.GetOutputAttribute("TO");
    Cube *outcube = importPds.SetOutputCube(ui.GetCubeName("TO"), att);

    // Get user entered special pixel ranges
    if (ui.GetBoolean("SETNULLRANGE")) {
      importPds.SetNull(ui.GetDouble("NULLMIN"), ui.GetDouble("NULLMAX"));
    }
    if (ui.GetBoolean("SETHRSRANGE")) {
      importPds.SetHRS(ui.GetDouble("HRSMIN"), ui.GetDouble("HRSMAX"));
    }
    if (ui.GetBoolean("SETHISRANGE")) {
      importPds.SetHIS(ui.GetDouble("HISMIN"), ui.GetDouble("HISMAX"));
    }
    if (ui.GetBoolean("SETLRSRANGE")) {
      importPds.SetLRS(ui.GetDouble("LRSMIN"), ui.GetDouble("LRSMAX"));
    }
    if (ui.GetBoolean("SETLISRANGE")) {
      importPds.SetLIS(ui.GetDouble("LISMIN"), ui.GetDouble("LISMAX"));
    }

    importPds.SetOrganization(Isis::ProcessImport::BSQ);

    importPds.StartProcess();

    // Get the mapping labels
    Pvl otherLabels;
    importPds.TranslatePdsProjection(otherLabels);

    // Translate the remaining MI MAP labels
    QString transDir = "$ISISROOT/appdata/translations/";

    FileName transFile(transDir + "KaguyaTcBandBin.trn");
    PvlToPvlTranslationManager bandBinXlater(label, transFile.expanded());
    bandBinXlater.Auto(otherLabels);

    transFile = transDir + "KaguyaTcInstrument.trn";
    PvlToPvlTranslationManager instXlater(label, transFile.expanded());
    instXlater.Auto(otherLabels);

    transFile = transDir + "KaguyaTcArchive.trn";
    PvlToPvlTranslationManager archiveXlater(label, transFile.expanded());
    archiveXlater.Auto(otherLabels);

    transFile = transDir + "KaguyaTcKernels.trn";

    PvlToPvlTranslationManager kernelsXlater(label, transFile.expanded());
    kernelsXlater.Auto(otherLabels);

    if ( otherLabels.hasGroup("Mapping")
         && otherLabels.findGroup("Mapping").keywords() > 0 ) {
      outcube->putGroup(otherLabels.findGroup("Mapping"));
    }
    if ( otherLabels.hasGroup("Instrument")
         && otherLabels.findGroup("Instrument").keywords() > 0 ) {
      PvlGroup &inst = otherLabels.findGroup("Instrument", Pvl::Traverse);
      if (inst.hasKeyword("StartTime")) {
        // Remove trailing "Z" from keyword
        PvlKeyword &startTime = inst["StartTime"];
        QString startTimeString = QString::fromStdString(startTime[0]);
        if (QString::compare(startTimeString.at(startTimeString.size() - 1), "Z", Qt::CaseInsensitive) == 0){
          startTimeString = startTimeString.left(startTimeString.length() - 1);
          startTime.setValue(startTimeString.toStdString());
        }
      }
      if (inst.hasKeyword("StopTime")) {
        // Remove trailing "Z" from keyword
        PvlKeyword &stopTime = inst["StopTime"];
        QString stopTimeString = QString::fromStdString(stopTime[0]);
        if (QString::compare(stopTimeString.at(stopTimeString.size() - 1), "Z", Qt::CaseInsensitive) == 0){
          stopTimeString = stopTimeString.left(stopTimeString.length() - 1);
          stopTime.setValue(stopTimeString.toStdString());
        }
      }
      outcube->putGroup(otherLabels.findGroup("Instrument"));
  /*
      // This code is not needed now, but is included here commented-out in case it becomes necessary
      // to support the swath modes by setting their NaifFrameCodes in the future. The swath mode
      // setting is currently handled entirely via the camera model.

      // add kernels group
      QString instId = inst["InstrumentId"];
      QString encoding = inst["EncodingType"];
      QString swath = inst["SwathModeId"];
      PvlGroup kern("Kernels");
      if (instId == "TC1") {
        if (swath == "Full") {
          if (productSetId == "TC_w_Level2B0") {
            if (encoding == "DCT") {
              kern += PvlKeyword("NaifFrameCode", toString(-131352));
            }
            else { // encoding == "N/A" so no compression
              kern += PvlKeyword("NaifFrameCode", toString(-131353));
            }
          }
          else if (productSetId == "TC_s_Level2B0") {
            if (encoding == "DCT") {
              kern += PvlKeyword("NaifFrameCode", toString(-131354));
            }
            else { // encoding == "N/A" so no compression
              kern += PvlKeyword("NaifFrameCode", toString(-131355));
            }
          }
        }
        else if (swath == "Nominal") {
          if (productSetId == "TC_w_Level2B0") {
            if (encoding == "DCT") {
              kern += PvlKeyword("NaifFrameCode", toString(-131356));
            }
            else { // encoding == "N/A" so no compression
              kern += PvlKeyword("NaifFrameCode", toString(-131357));
            }
          }
          else if (productSetId == "TC_s_Level2B0") {
            if (encoding == "DCT") {
              kern += PvlKeyword("NaifFrameCode", toString(-131358));
            }
            else { // encoding == "N/A" so no compression
              kern += PvlKeyword("NaifFrameCode", toString(-131359));
            }
          }
        }
        else { // swath == "Half"
          if (productSetId == "TC_w_Level2B0") {
            if (encoding == "DCT") {
              kern += PvlKeyword("NaifFrameCode", toString(-131360));
            }
            else { // encoding == "N/A" so no compression
              kern += PvlKeyword("NaifFrameCode", toString(-131361));
            }
          }
          else if (productSetId == "TC_s_Level2B0") {
            if (encoding == "DCT") {
              kern += PvlKeyword("NaifFrameCode", toString(-131362));
            }
            else { // encoding == "N/A" so no compression
              kern += PvlKeyword("NaifFrameCode", toString(-131363));
            }
          }
        }
      }
      if (instId == "TC2") {
        if (swath == "Full") {
          if (productSetId == "TC_w_Level2B0") {
            if (encoding == "DCT") {
              kern += PvlKeyword("NaifFrameCode", toString(-131372));
            }
            else { // encoding == "N/A" so no compression
              kern += PvlKeyword("NaifFrameCode", toString(-131373));
            }
          }
          else if (productSetId == "TC_s_Level2B0") {
            if (encoding == "DCT") {
              kern += PvlKeyword("NaifFrameCode", toString(-131374));
            }
            else { // encoding == "N/A" so no compression
              kern += PvlKeyword("NaifFrameCode", toString(-131375));
            }
          }
        }
        else if (swath == "Nominal") {
          if (productSetId == "TC_w_Level2B0") {
            if (encoding == "DCT") {
              kern += PvlKeyword("NaifFrameCode", toString(-131376));
            }
            else { // encoding == "N/A" so no compression
              kern += PvlKeyword("NaifFrameCode", toString(-131377));
            }
          }
          else if (productSetId == "TC_s_Level2B0") {
            if (encoding == "DCT") {
              kern += PvlKeyword("NaifFrameCode", toString(-131378));
            }
            else { // encoding == "N/A" so no compression
              kern += PvlKeyword("NaifFrameCode", toString(-131379));
            }
          }
        }
        else { // swath == "Half"
          if (productSetId == "TC_w_Level2B0") {
            if (encoding == "DCT") {
              kern += PvlKeyword("NaifFrameCode", toString(-131380));
            }
            else { // encoding == "N/A" so no compression
              kern += PvlKeyword("NaifFrameCode", toString(-131381));
            }
          }
          else if (productSetId == "TC_s_Level2B0") {
            if (encoding == "DCT") {
              kern += PvlKeyword("NaifFrameCode", toString(-131382));
            }
            else { // encoding == "N/A" so no compression
              kern += PvlKeyword("NaifFrameCode", toString(-131383));
            }
          }
        }
      }
      */
    }
    if ( otherLabels.hasGroup("BandBin")
         && otherLabels.findGroup("BandBin").keywords() > 0 ) {

      PvlGroup &bandBinGroup = otherLabels.findGroup("BandBin");
      if (!bandBinGroup.hasKeyword("FilterName")) {
        bandBinGroup += PvlKeyword("FilterName", "BroadBand");
      }
      if (!bandBinGroup.hasKeyword("Center")) {
        bandBinGroup += PvlKeyword("Center", "640", "nanometers");
      }
      if (!bandBinGroup.hasKeyword("Width")) {
        bandBinGroup += PvlKeyword("Width", "420", "nanometers");
      }
      outcube->putGroup(bandBinGroup);
    }
    else {
      // Add the BandBin group
      PvlGroup bandBinGroup("BandBin");
      bandBinGroup += PvlKeyword("FilterName", "BroadBand");
      bandBinGroup += PvlKeyword("Center", "640", "nanometers");
      bandBinGroup += PvlKeyword("Width", "420", "nanometers");
      outcube->putGroup(bandBinGroup);
    }

    if ( otherLabels.hasGroup("Archive")
         && otherLabels.findGroup("Archive").keywords() > 0 ) {
      outcube->putGroup(otherLabels.findGroup("Archive"));
    }
    if ( otherLabels.hasGroup("Kernels")
         && otherLabels.findGroup("Kernels").keywords() > 0 ) {
      outcube->putGroup(otherLabels.findGroup("Kernels", Pvl::Traverse));
    }

    importPds.EndProcess();
  }
}
