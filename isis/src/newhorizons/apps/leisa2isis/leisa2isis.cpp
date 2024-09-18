/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "leisa2isis.h"

#include "Cube.h"
#include "IException.h"
#include "IString.h"
#include "OriginalLabel.h"
#include "ProcessBySample.h"
#include "ProcessImportFits.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "UserInterface.h"
#include "iTime.h"
#include "ProgramLauncher.h"

#include <fstream>
#include <iostream>
#include <QString>

using namespace std;

namespace Isis {
  void processFunc(Buffer &in);
  Cube *outputCubeForLeisa2Isis;

  void leisa2isis(UserInterface &ui, Pvl *log) {
    ProcessImportFits importFits;

    importFits.setFitsFile(FileName(ui.GetFileName("FROM").toStdString()));
    PvlGroup mainLabel;
    mainLabel = importFits.fitsImageLabel(0);

    // Get the first label and make sure this is a New Horizons LEISA file
    if (!mainLabel.hasKeyword("MISSION") || !mainLabel.hasKeyword("INSTRU")) {
      QString msg = QObject::tr("Input file [%1] does not appear to be a New Horizons LEISA FITS "
                                "file. Input file label key MISSION or INSTRU is missing").
                    arg(ui.GetFileName("FROM"));
      throw IException(IException::User, msg.toStdString(), _FILEINFO_);
    }
    else if (mainLabel["MISSION"][0] != "New Horizons" || mainLabel["INSTRU"][0] != "lei") {
      QString msg = QObject::tr("Input file [%1] does not appear to be a New Horizons LEISA FITS "
                                "file. Input file label value for MISSION is [%2], INSTRU is [%3]").
                    arg(ui.GetFileName("FROM")).arg(QString::fromStdString(mainLabel["MISSION"][0])).
                    arg(QString::fromStdString(mainLabel["INSTRU"][0]));
      throw IException(IException::User, msg.toStdString(), _FILEINFO_);
    }

     bool replace = ui.GetBoolean("REPLACE");

    // Check to see if the calibration error image was requested from the FITS file and
    // that it has the corresponding extension
    if (ui.WasEntered("ERRORMAP")) {
      PvlGroup extensionLabel;
      try {
        extensionLabel = importFits.fitsImageLabel(5);
  /**      if (!extensionLabel.hasKeyword("XTENSION")) {
          std::string msg = QObject::tr("Input file [%1] does not appear to be a calibrated New Horizons "
                        "LEISA FITS file. XTENSION keyword is missing in the fifth extension.")
                        .arg(ui.GetFileName("FROM"));
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }
        if (!extensionLabel.hasKeyword("EXTNAME")) {
          std::string msg = QObject::tr("Input file [%1] does not appear to contain a New Horizons LEISA "
                        "calibrated image. FITS label keyword EXTNAME is missing in the fifth extension").
                        arg(ui.GetFileName("FROM"));
          throw IException(IException::User, msg, _FILEINFO_);
        }
        else if (extensionLabel["EXTNAME"][0] != "ERRORMAP") {
          std::string msg = QObject::tr("Input file [%1] does not appear to contain a New Horizons LEISA "
                                    "calibrated image. FITS label value for EXTNAME is [%2]").
                                 arg(ui.GetFileName("FROM")).arg(extensionLabel["EXTNAME"][0]);
          throw IException(IException::User, msg, _FILEINFO_);
        } **/
      }
      catch (IException &e) {
        QString msg = QObject::tr("Unable to find errormap extension in [%1]").arg(ui.GetFileName("FROM"));
        throw IException(e, IException::Unknown, msg.toStdString(), _FILEINFO_);
      }

    }

    // Check to see if the quality image was requested from the FITS file and
    // that it has the corresponding extension
    if (ui.WasEntered("QUALITY") || replace) {
      PvlGroup  extensionLabel;
      try {
        extensionLabel = importFits.fitsImageLabel(6);
  /**      if (!extensionLabel.hasKeyword("XTENSION")) {
          std::string msg = QObject::tr("Input file [%1] does not appear to be a calibrated New Horizons "
                        "LEISA FITS file. XTENSION keyword is missing in the sixth extension.")
                        .arg(ui.GetFileName("FROM"));
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }
        if (!extensionLabel.hasKeyword("EXTNAME")) {
          std::string msg = QObject::tr("Input file [%1] does not appear to contain a New Horizons LEISA "
                        "calibrated image. FITS label keyword EXTNAME is missing in the sixth extension").
                        arg(ui.GetFileName("FROM"));
          throw IException(IException::User, msg, _FILEINFO_);
        }
        else if (extensionLabel["EXTNAME"][0] != "QUALITY") {
          std::string msg = QObject::tr("Input file [%1] does not appear to contain a New Horizons LEISA "
                                    "calibrated image. FITS label value for EXTNAME is [%2]").
                                 arg(ui.GetFileName("FROM")).arg(extensionLabel["EXTNAME"][0]);
          throw IException(IException::User, msg, _FILEINFO_);
        } **/
      }
      catch (IException &e) {
        QString msg = QObject::tr("Unable to find quality extension in [%1]").arg(ui.GetFileName("FROM"));
        throw IException(e, IException::Unknown, msg.toStdString(), _FILEINFO_);
      }
    }

    // Import the primary image (LEISA raw/calibrated)
    importFits.SetOrganization(ProcessImport::BIL);
    importFits.setProcessFileStructure(0);

    Cube *output = NULL;
    FileName outputFile;

    CubeAttributeOutput &att = ui.GetOutputAttribute("TO");

    if (replace){
      outputFile = FileName::createTempFile(FileName("$TEMPORARY/dn.cub"));
      importFits.SetPixelType(Isis::Real);
      output = importFits.SetOutputCube(QString::fromStdString(outputFile.toString()), att);
    }
    else {
      if (importFits.PixelType() == Isis::None) {
        importFits.SetPixelType(Isis::Real);
      }
      importFits.SetAttributes(att);
      output = importFits.SetOutputCube(ui.GetCubeName("TO"), att);
    }

    // Get the path where the New Horizons translation tables are.
    std::string transDir = "$ISISROOT/appdata/translations/";

    // Temp storage of translated labels
    Pvl outLabel;

    // Get the FITS label
    Pvl fitsLabel;
    fitsLabel.addGroup(importFits.fitsImageLabel(0));
    // Create an Instrument group
    FileName insTransFile(transDir + "NewHorizonsLeisaInstrument_fit.trn");
    PvlToPvlTranslationManager insXlater(fitsLabel, QString::fromStdString(insTransFile.expanded()));
    insXlater.Auto(outLabel);

  // Modify/add Instument group keywords not handled by the translater
  //  PvlGroup &inst = outLabel.findGroup("Instrument", Pvl::Traverse);
  //  QString target = (QString)inst["TargetName"];
  //  if (target.startsWith("RADEC=")) {
  //    inst.addKeyword(PvlKeyword("TargetName", "Sky"), PvlGroup::Replace);
  //  }

    output->putGroup(outLabel.findGroup("Instrument", Pvl::Traverse));

    // Create an Archive group
    FileName archiveTransFile(transDir + "NewHorizonsLeisaArchive_fit.trn");
    PvlToPvlTranslationManager archiveXlater(fitsLabel, QString::fromStdString(archiveTransFile.expanded()));
    archiveXlater.Auto(outLabel);
    output->putGroup(outLabel.findGroup("Archive", Pvl::Traverse));

    //Create a Band Bin Group
    FileName bandTransFile(transDir + "NewHorizonsLeisaBandBin_fit.trn");
    PvlToPvlTranslationManager bandBinXlater(fitsLabel, QString::fromStdString(bandTransFile.expanded()));
    bandBinXlater.Auto(outLabel);
    output->putGroup(outLabel.findGroup("BandBin", Pvl::Traverse));

    // Create a Kernels group
    FileName kernelsTransFile(transDir + "NewHorizonsLeisaKernels_fit.trn");
    PvlToPvlTranslationManager kernelsXlater(fitsLabel, QString::fromStdString(kernelsTransFile.expanded()));
    kernelsXlater.Auto(outLabel);
    output->putGroup(outLabel.findGroup("Kernels", Pvl::Traverse));


    // Modify/add Instument group keywords not handled by the translater
    Pvl *isisLabel = output->label();
    PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
    PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);

    //Add StartTime & EndTime
    QString midTimeStr = QString::fromStdString(archive["MidObservationTime"]);
    iTime midTime(midTimeStr.toDouble());

    QString obsDuration = QString::fromStdString(archive["ObservationDuration"]);
    double obsSeconds = obsDuration.toDouble();
    iTime startTime = midTime - obsSeconds/2.0;
    iTime endTime = midTime + obsSeconds/2.0;
  //  inst.addKeyword(PvlKeyword("StartTime", startTime.EtString()), PvlGroup::Replace);
  //  inst.addKeyword(PvlKeyword("StopTime", endTime.EtString()), PvlGroup::Replace);

    //Need to make sure these times are correct. UTC != ET
    inst.addKeyword(PvlKeyword("StartTime", startTime.UTC().toStdString()), PvlGroup::Replace);
    inst.addKeyword(PvlKeyword("StopTime", endTime.UTC().toStdString()), PvlGroup::Replace);

    QString exposureTime = QString::fromStdString(inst["ExposureDuration"]);
    double frameRate = 1.0/exposureTime.toDouble();
    inst.addKeyword(PvlKeyword("FrameRate", std::to_string(frameRate)), PvlGroup::Replace);
    inst.findKeyword("FrameRate").setUnits("Hz");

    // Save the input FITS label in the Cube original labels
    Pvl pvl;
    pvl += importFits.fitsImageLabel(0);
    OriginalLabel originals(pvl);
    output->write(originals);

    // Convert the main image data
    importFits.Progress()->SetText("Importing main LEISA image");
    importFits.StartProcess();
    importFits.ClearCubes();
    importFits.Finalize();

    // If replace was selected, add this with the bad pixel mask to get the final output file
    if (replace) {
      importFits.SetOrganization(ProcessImport::BIL);
      importFits.setProcessFileStructure(6);

      FileName qualityFile = FileName::createTempFile("$TEMPORARY/quality.cub");
      CubeAttributeOutput &cao = ui.GetOutputAttribute("TO");

      // Set to Isis::Real
      if (cao.propagatePixelType()) {
        cao.setPixelType(Isis::Real);
      }

      outputCubeForLeisa2Isis = importFits.SetOutputCube(QString::fromStdString(qualityFile.toString()), cao);

      importFits.Progress()->SetText("Preparing quality image for comparing against LEISA pixels");
      importFits.StartProcess(processFunc);
      importFits.ClearCubes();
      importFits.Finalize();

      // Now we have the temp cube and want to use fx to add it to the DN cube
      // fx f1=temp_quality.cub f2=temp_dn.cub to=output_name.cub equation="f1 + f2"
      QString parameters;
      parameters += " F1= " + QString::fromStdString(outputFile.toString());
      parameters += " F2= " + QString::fromStdString(qualityFile.toString());
      parameters += " TO= " + ui.GetCubeName("TO");
      parameters += " EQUATION=" + QString("\"f1+f2\"");
      ProgramLauncher::RunIsisProgram("fx", parameters);

      // Cleanup by removing temporary cubes
      remove(qualityFile.toString().c_str());
      remove(outputFile.toString().c_str());
    }

    // Import the ERRORMAP image. It is the 6th image in the FITS file (i.e., 5th extension)
    if (ui.WasEntered("ERRORMAP")) {
      PvlGroup extensionLabel = importFits.fitsImageLabel(5);
      importFits.SetOrganization(ProcessImport::BIL);
      importFits.setProcessFileStructure(5);
      Cube *output = importFits.SetOutputCube("ERRORMAP", ui);

      // Save the input FITS label in the Cube original labels
      Pvl origLabel;
      origLabel += extensionLabel;
      OriginalLabel originals(origLabel);
      output->write(originals);

      // Convert the main image data
      importFits.Progress()->SetText("Importing LEISA errormap image");
      importFits.StartProcess();
      importFits.ClearCubes();
    }

    // Import the quality image. It is the 7th image in the FITS file (i.e., 6th extension)
    if (ui.WasEntered("QUALITY")) {
      PvlGroup extensionLabel = importFits.fitsImageLabel(6);
      importFits.SetOrganization(ProcessImport::BIL);
      importFits.setProcessFileStructure(6);
      Cube *output = importFits.SetOutputCube("QUALITY", ui);

      // Save the input FITS label in the Cube original labels
      Pvl origLabel;
      origLabel += extensionLabel;
      OriginalLabel originals(origLabel);
      output->write(originals);

      // Convert the main image data
      importFits.Progress()->SetText("Importing LEISA quality image");
      importFits.StartProcess();
      importFits.ClearCubes();
    }
  }

  void processFunc(Buffer &in){
    for (int i = 0; i < in.size(); i++) {
      if (((in[i] == 1) || (in[i] == 2)) || (in[i] == 8)) {
        in[i] = Isis::NULL8;
      }
      else {
          in[i] = 0;
      }
    }
    outputCubeForLeisa2Isis->write(in);
  }
}
