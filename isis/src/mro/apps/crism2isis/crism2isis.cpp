/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "crism2isis.h"
#include "Application.h"
#include "ProcessImportPds.h"
#include "UserInterface.h"
#include "Pvl.h"
#include "FileName.h"
#include "TextFile.h"

using namespace std;

namespace Isis{
  void crism2isis(UserInterface &ui, Pvl *log) {
    ProcessImportPds p;
    Pvl pdsLabel;
    PvlGroup results;

    FileName inFile = ui.GetFileName("FROM");

    p.SetPdsFile(inFile.expanded(), "", pdsLabel);
    // 65535 is set to NULL
    p.SetNull(65535, 65535);

    CubeAttributeOutput &att = ui.GetOutputAttribute("TO");
    Cube *ocube = p.SetOutputCube(ui.GetCubeName("TO"), att);

    Pvl outLabel;

    Pvl labelPvl(inFile.expanded().toStdString());

    QString prodType;

    if (labelPvl.hasKeyword("PRODUCT_TYPE")) {
      prodType = QString::fromStdString(labelPvl.findKeyword("PRODUCT_TYPE"));
    }
    else {
      QString msg = "Unsupported CRISM file type, supported types are: DDR, MRDR, and TRDR";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if (prodType.toUpper() == "MAP_PROJECTED_MULTISPECTRAL_RDR") {
      QString prodId;
      if (labelPvl.hasKeyword("PRODUCT_ID")) {
        prodId = QString::fromStdString(labelPvl.findKeyword("PRODUCT_ID"));
        prodId = prodId.mid(prodId.indexOf("_") + 1, prodId.indexOf("_"));
      }
      else {
        QString msg = "Could not find label PRODUCT_ID, invalid MRDR";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }

      //If the product type is AL (Lambert albedo) or IF (I/F)
      //Read the wavelength table and put the corresponding
      //widths in the band bin group
      if (prodId.toUpper() == "MRRAL" || prodId.toUpper() == "MRRIF") {
        //If the wavelength file is specified in the label
        if (labelPvl.hasKeyword("MRO:WAVELENGTH_FILE_NAME")) {
          PvlGroup bandBin = PvlGroup("BandBin");
          PvlKeyword origBand = PvlKeyword("OriginalBand");
          PvlKeyword widths = PvlKeyword("Width");
          QString tablePath = QString::fromStdString(labelPvl.findKeyword("MRO:WAVELENGTH_FILE_NAME"));
          tablePath = tablePath.toLower();
          FileName tableFile(inFile.path() + "/" + tablePath);
          //Check if the wavelength file exists
          if (tableFile.fileExists()) {
            TextFile *fin = new TextFile(tableFile.expanded());
            // Open table file
            if (!fin->OpenChk()) {
              QString msg = "Cannot open wavelength table [" + tableFile.expanded() + "]";
              throw IException(IException::Io, msg, _FILEINFO_);
            }

            //For each line in the wavelength table, add the width to
            //The band bin group
            QString st;
            int band = 1;
            while (fin->GetLine(st)) {
              st = st.simplified().trimmed();
              QStringList cols = st.split(",");

              origBand += std::to_string(band);
              widths += cols[2].toStdString();
              band++;
            }
            delete fin;

            bandBin.addKeyword(origBand);
            bandBin.addKeyword(widths);
            ocube->putGroup(bandBin);
          }
          //Otherwise throw an error
          else {
            QString msg = "Cannot find wavelength table [" + tableFile.expanded() + "]";
            throw IException(IException::Io, msg, _FILEINFO_);
          }
        }
      }
      //If the product type is DE (Derived products for I/F) or DL
      //(Derived products for Lambert albedo) write the band names
      //to the band bin group
      else if (prodId.toUpper() == "MRRDE" || prodId.toUpper() == "MRRDL") {
        PvlGroup bandBin = PvlGroup("BandBin");
        PvlKeyword origBand = PvlKeyword("OriginalBand");
        PvlKeyword bandName = PvlKeyword("BandName");
        PvlKeyword bandNames = labelPvl.findObject("IMAGE").findKeyword("BAND_NAME");
        for (int i = 0; i < bandNames.size(); i++) {
          origBand += std::to_string(i + 1);
          bandName += bandNames[i];
        }
        bandBin.addKeyword(origBand);
        bandBin.addKeyword(bandName);
        ocube->putGroup(bandBin);
      }
      //Translate the Mapping group
      p.TranslatePdsProjection(outLabel);
      ocube->putGroup(outLabel.findGroup("Mapping", Pvl::Traverse));

      //  Check for any change from the default projection offsets and multipliers to log

      if (p.GetProjectionOffsetChange()) {
        results = p.GetProjectionOffsetGroup();
        results[0].addComment("Projection offsets and multipliers have been changed from");
        results[0].addComment("defaults. New values are below.");
      }

    }
    else if (prodType.toUpper() == "TARGETED_RDR") {
    }
    else if (prodType.toUpper() == "DDR") {
      PvlGroup bandBin = PvlGroup("BandBin");
      PvlKeyword origBand = PvlKeyword("OriginalBand");
      PvlKeyword bandName = PvlKeyword("BandName");
      PvlKeyword bandNames = labelPvl.findObject("FILE").findObject("IMAGE").findKeyword("BAND_NAME");
      for (int i = 0; i < bandNames.size(); i++) {
        origBand += std::to_string(i + 1);
        bandName += bandNames[i];
      }
      bandBin.addKeyword(origBand);
      bandBin.addKeyword(bandName);
      ocube->putGroup(bandBin);
    }
    else {
      QString msg = "Unsupported CRISM file type, supported types are: DDR, MRDR, and TRDR";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Translate the Instrument group
    FileName transFile("$ISISROOT/appdata/translations/MroCrismInstrument.trn");
    PvlToPvlTranslationManager instrumentXlater(labelPvl, transFile.expanded());
    instrumentXlater.Auto(outLabel);

    // Translate the Archive group
    transFile  = "$ISISROOT/appdata/translations/MroCrismArchive.trn";
    PvlToPvlTranslationManager archiveXlater(labelPvl, transFile.expanded());
    archiveXlater.Auto(outLabel);

    ocube->putGroup(outLabel.findGroup("Instrument", Pvl::Traverse));
    ocube->putGroup(outLabel.findGroup("Archive", Pvl::Traverse));
    ocube->putGroup(outLabel.findGroup("Kernels", Pvl::Traverse));

    p.StartProcess();
    p.EndProcess();

    results.setName("Results");
    results += PvlKeyword("Warning",
                          "When using cam2map or cam2cam, images imported into "
                          "Isis using crism2isis should only be interpolated "
                          "using the nearest-neighbor algorithm due to gimble "
                          "jitter of the MRO CRISM instrument.");

    Application::Log(results);
    return;
  }
}
