/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "mvic2isis.h"

#include <fstream>
#include <iostream>

#include <QString>

#include "Cube.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "LineManager.h"
#include "NaifStatus.h"
#include "OriginalLabel.h"
#include "ProcessBySample.h"
#include "ProcessImportFits.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "UserInterface.h"

using namespace std;

namespace Isis {
  void translateLabels(Pvl &fitsLabel, Cube *ocube);

  void mvic2isis(UserInterface &ui, Pvl *log) {
    ProcessImportFits importFits;
    importFits.setFitsFile(FileName(ui.GetFileName("FROM").toStdString()));

    // Get the primary FITS label
    Pvl primaryLabel;
    primaryLabel.addGroup(importFits.fitsImageLabel(0));

    // Make sure this is a New Horizons MVIC image FITS formatted file
    bool mvic = true;
    if (primaryLabel.hasKeyword("MISSION", Pvl::Traverse) &&
        primaryLabel.hasKeyword("INSTRU", Pvl::Traverse)) {
      QString mission = QString::fromStdString(primaryLabel.findKeyword("MISSION", Pvl::Traverse));
      QString instrument = QString::fromStdString(primaryLabel.findKeyword("INSTRU", Pvl::Traverse));
      if (!mission.contains("New Horizons")) {
        mvic = false;
      }

      if (!instrument.contains("mvi")) {
        mvic = false;
      }
    }
    else {
      mvic = false;
    }

    if (!mvic) {
      FileName in = ui.GetFileName("FROM").toStdString();
      std::string msg = "Input file [" + in.expanded() + "] does not appear to be " +
                    "in New Horizons/MVIC FITS format.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Check to see if the undistorted image was requested from the FITS file and that it has the
    // corresponding extension and keywords
    if (ui.WasEntered("UNDISTORTED")) {
      PvlGroup undistortedLabel = importFits.fitsImageLabel(1);
      if (!undistortedLabel.hasKeyword("COMMENT") ||
          !QString::fromStdString(undistortedLabel["COMMENT"][0]).startsWith("This is the bias-subtracted, "
                                                     "flattened, distortion-removed image cube.")) {

        QString msg = QObject::tr("Input file [%1] does not appear to contain an MVIC undistorted "
                                  "image in XTENSION [2]").arg(ui.GetFileName("FROM"));
        throw IException(IException::User, msg.toStdString(), _FILEINFO_);
      }
    }

    // Check to see if the error image was requested from the FITS file and it has the
    // corresponding extension and keywords
    if (ui.WasEntered("ERROR")) {
      PvlGroup errorLabel = importFits.fitsImageLabel(2);
      if (!errorLabel.hasKeyword("COMMENT") ||
          errorLabel["COMMENT"][0] != "1-sigma error per pixel for the image in extension 1.") {

        QString msg = QObject::tr("Input file [%1] does not appear to contain an MVIC Error image "
                                  "in the XTENSION [3]").arg(ui.GetFileName("FROM"));
        throw IException(IException::User, msg.toStdString(), _FILEINFO_);
      }
    }

    // Check to see if the quality image was requested from the FITS file and it has the
    // corresponding extension and keywords
    if (ui.WasEntered("QUALITY")) {
      PvlGroup qualityLabel = importFits.fitsImageLabel(3);
      if (!qualityLabel.hasKeyword("COMMENT") ||
          qualityLabel["COMMENT"][0] != "Data quality flag for the image in extension 1.") {

        QString msg = QObject::tr("Input file [%1] does not appear to contain an MVIC Quality image "
                                  "in extension [3]").arg(ui.GetFileName("FROM"));
        throw IException(IException::User, msg.toStdString(), _FILEINFO_);
      }
    }

    // Convert the primary image
    QString bitpix = QString::fromStdString(primaryLabel.findKeyword("BITPIX", Pvl::Traverse));
    int bytesPerPix = abs(bitpix.toInt()) / 8;
    importFits.SetDataPrefixBytes(bytesPerPix * 12);
    importFits.SetDataSuffixBytes(bytesPerPix * 12);
    importFits.setProcessFileStructure(0);

    // Set up the output cube
    CubeAttributeOutput &att = ui.GetOutputAttribute("TO");
    Cube *output = importFits.SetOutputCube(ui.GetCubeName("TO"), att);

    translateLabels(primaryLabel, output); // Results are put directly into the cube label (output)

    // Save the input FITS label in the output cube original labels
    OriginalLabel originals(primaryLabel);
    output->write(originals);

    // Import the file
    importFits.Progress()->SetText("Importing main MVIC image");
    importFits.StartProcess();
    importFits.EndProcess();


    // Convert the bias-subtracted, flattened, distortion removed image. It is currently assumed
    // to be the 2rd image in the FITS file (i.e., 1st extension)
    if (ui.WasEntered("UNDISTORTED")) {
      PvlGroup undistortedLabel = importFits.fitsImageLabel(1);

      QString bitpix = QString::fromStdString(undistortedLabel.findKeyword("BITPIX"));
      int bytesPerPix = abs(bitpix.toInt()) / 8;
      importFits.SetDataPrefixBytes(bytesPerPix * 12);
      importFits.SetDataSuffixBytes(bytesPerPix * 12);
      importFits.setProcessFileStructure(1);

      Cube *outputError = importFits.SetOutputCube("UNDISTORTED", ui);

      // Save the input FITS label in the Cube original labels
      Pvl pvlError;
      pvlError += undistortedLabel;
      OriginalLabel originals(pvlError);
      outputError->write(originals);

      // Convert the main image data
      importFits.Progress()->SetText("Importing MVIC Undistorted image");
      importFits.StartProcess();
      importFits.ClearCubes();
    }


    // Convert the Error image. It is currently assumed to be the 3rd image in the FITS file
    if (ui.WasEntered("ERROR")) {
      // Get the label of the Error image and make sure this is a New Horizons MVIC Error image
      PvlGroup errorLabel = importFits.fitsImageLabel(2);
      QString bitpix = QString::fromStdString(errorLabel.findKeyword("BITPIX"));
      int bytesPerPix = abs(bitpix.toInt()) / 8;
      importFits.SetDataPrefixBytes(bytesPerPix * 12);
      importFits.SetDataSuffixBytes(bytesPerPix * 12);
      importFits.setProcessFileStructure(2);

      Cube *outputError = importFits.SetOutputCube("ERROR", ui);

      // Save the input FITS label in the Cube original labels
      Pvl pvlError;
      pvlError += errorLabel;
      OriginalLabel originals(pvlError);
      outputError->write(originals);

      // Convert the main image data
      importFits.Progress()->SetText("Importing MVIC Error image");
      importFits.StartProcess();
      importFits.ClearCubes();
    }


    // Convert the Quality image. It is currently assumed to be the 4th image in the FITS file
    if (ui.WasEntered("QUALITY")) {
      // Get the label of the Error image and make sure this is a New Horizons MVIC Quality image
      PvlGroup qualityLabel = importFits.fitsImageLabel(3);
      QString bitpix = QString::fromStdString(qualityLabel.findKeyword("BITPIX"));
      int bytesPerPix = abs(bitpix.toInt()) / 8;
      importFits.SetDataPrefixBytes(bytesPerPix * 12);
      importFits.SetDataSuffixBytes(bytesPerPix * 12);
      importFits.setProcessFileStructure(3);

      Cube *outputError = importFits.SetOutputCube("QUALITY", ui);

      // Save the input FITS label in the Cube original labels
      Pvl pvlError;
      pvlError += qualityLabel;
      OriginalLabel originals(pvlError);
      outputError->write(originals);

      // Convert the main image data
      importFits.Progress()->SetText("Importing MVIC Quality image");
      importFits.StartProcess();
      importFits.ClearCubes();
    }
  }

  void translateLabels(Pvl &fitslabel, Cube *ocube) {

    // Get the path where the New Horizons translation tables are.
    std::string transDir = "$ISISROOT/appdata/translations/";

    Pvl *isisLabel = ocube->label();
    // Create an Instrument group
    FileName insTransFile(transDir + "NewHorizonsMvicInstrument_fit.trn");
    PvlToPvlTranslationManager insXlater(fitslabel, QString::fromStdString(insTransFile.expanded()));
    insXlater.Auto(*(isisLabel));

    // Modify/add Instument group keywords not handled by the translater
    PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);

    QString instId = QString::fromStdString(inst["InstrumentId"]);
    QString scanType = QString::fromStdString(inst["ScanType"]);
    instId = instId + "_" + scanType;
    inst.addKeyword(PvlKeyword("InstrumentId", instId.toStdString()), PvlGroup::Replace);

    // Not tested because we didn't have any files that do this at the time the test were done
    QString target = QString::fromStdString(inst["TargetName"]);
    if (target.startsWith("RADEC=")) {
      inst.addKeyword(PvlKeyword("TargetName", "Sky"), PvlGroup::Replace);
    }

    if (inst.hasKeyword("TdiRate")) {
      inst.findKeyword("TdiRate").setUnits("hz");
    }

    //  Create StartTime (UTC) from the SpacecraftClockStartCount.  Need to load the leapsecond
    //  and spacecraft clock kernels to calculate time.
    NaifStatus::CheckErrors();
    // Leapsecond kernel
    std::string lsk = "$ISISDATA/base/kernels/lsk/naif????.tls";
    FileName lskName(lsk);
    lskName = lskName.highestVersion();
    furnsh_c(lskName.expanded().c_str());

    // Spacecraft clock kernel
    std::string sclk = "$ISISDATA/newhorizons/kernels/sclk/new_horizons_???.tsc";
    FileName sclkName(sclk);
    sclkName = sclkName.highestVersion();
    furnsh_c(sclkName.expanded().c_str());

    SpiceInt sclkCode;
    if (fitslabel.hasKeyword("SPCSCID", Pvl::Traverse)) {
      sclkCode = fitslabel.findKeyword("SPCSCID", Pvl::Traverse);
    }
    else {
      std::string msg = "Input file is missing the spacecraft Naif Id.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    QString scTime = QString::fromStdString(inst["SpacecraftClockStartCount"]);
    double et;
    scs2e_c(sclkCode, scTime.toLatin1().data(), &et);
    SpiceChar utc[30];
    et2utc_c(et, "ISOC", 3, 30, utc);
    inst.addKeyword(PvlKeyword("StartTime", utc));

    // Create a Band Bin group
    FileName bandTransFile(transDir + "NewHorizonsMvicBandBin_fit.trn");
    PvlToPvlTranslationManager bandBinXlater(fitslabel, QString::fromStdString(bandTransFile.expanded()));
    bandBinXlater.Auto(*(isisLabel));
    // Add units and OriginalBand keyword
    PvlGroup &bandBin = isisLabel->findGroup("BandBin", Pvl::Traverse);
    bandBin.addKeyword(PvlKeyword("OriginalBand", "1"));
    PvlKeyword &center = bandBin.findKeyword("Center");
    center.setUnits("nanometers");
    PvlKeyword &width = bandBin.findKeyword("Width");
    width.setUnits("nanometers");

    // If image is in the framing mode, the BandBin keywords will need to be duplicated for each
    // band.  Also, add BandStartTime
    if (scanType.contains("FRAMING")) {
      PvlGroup &bandBin = isisLabel->findGroup("BandBin", Pvl::Traverse);
      QString name = QString::fromStdString(bandBin.findKeyword("Name")[0]);
      QString center = QString::fromStdString(bandBin.findKeyword("Center")[0]);
      QString width = QString::fromStdString(bandBin.findKeyword("Width")[0]);
      for (int i = 1; i < ocube->bandCount(); i++) {
        bandBin.findKeyword("Name").addValue(name.toStdString());
        bandBin.findKeyword("Center").addValue(center.toStdString(), "nanometers");
        bandBin.findKeyword("Width").addValue(width.toStdString(), "nanometers");
        bandBin.findKeyword("OriginalBand").addValue(std::to_string(i+1));
        QString fitsKey = QString("UTCMID%1").arg(i, 2, 10, QChar('0'));
        QString fitsVal = QString::fromStdString(fitslabel.findKeyword(fitsKey.toStdString(), Pvl::Traverse));
        bandBin.findKeyword("UtcTime").addValue(fitsVal.toStdString());
      }
    }

    // Create an Archive group
    FileName archiveTransFile(transDir + "NewHorizonsMvicArchive_fit.trn");
    PvlToPvlTranslationManager archiveXlater(fitslabel, QString::fromStdString(archiveTransFile.expanded()));
    archiveXlater.Auto(*(isisLabel));

    // Create a Kernels group
    FileName kernelsTransFile(transDir + "NewHorizonsMvicKernels_fit.trn");
    PvlToPvlTranslationManager kernelsXlater(fitslabel, QString::fromStdString(kernelsTransFile.expanded()));
    kernelsXlater.Auto(*(isisLabel));

    // If Level 2 product, Create a RadiometricCalibration group
    if (fitslabel.hasKeyword("SOCL2VER", Pvl::Traverse)) {
      FileName calibrationTransFile(transDir + "NewHorizonsMvicCalibration_fit.trn");
      PvlToPvlTranslationManager calibrationXlater(fitslabel, QString::fromStdString(calibrationTransFile.expanded()));
      calibrationXlater.Auto(*(isisLabel));

      //  Add comments to calibration keywords.  This is done by hand because the translation tables
      //  did not handle comments at the time this was written.
      PvlGroup &calibration = isisLabel->findGroup("RadiometricCalibration", Pvl::Traverse);
      if (calibration.hasKeyword("PixelSize")) {
        calibration.findKeyword("PixelSize").setUnits("microns");
      }
      if (calibration.hasKeyword("PixelFov")) {
        calibration.findKeyword("PixelFov").setUnits("microrad/pix");
      }
      if (calibration.hasKeyword("Gain")) {
        calibration.findKeyword("Gain").setUnits("electrons/DN");
      }
      if (calibration.hasKeyword("ReadNoise")) {
        calibration.findKeyword("ReadNoise").setUnits("electrons");
      }
      if (calibration.hasKeyword("TdiRate")) {
        calibration.findKeyword("TdiRate").setUnits("hz");
      }
      // The following do not need hasKeyword tests because the translater creats them everytime
      // due to them having default values if none is in the FITS file.
      calibration.findKeyword("SolarSpectrumResolved").setUnits("(erg/cm^2/s/sr)/(DN/s/pix)");
      calibration.findKeyword("SolarSpectrumUnresolved").setUnits("(erg/cm^2/s/sr)/(DN/s/pix)");
      calibration.findKeyword("PholusSpectrumResolved").setUnits("(erg/cm^2/s/sr)/(DN/s/pix)");
      calibration.findKeyword("PholusSpectrumUnresolved").setUnits("(erg/cm^2/s/sr)/(DN/s/pix)");
      calibration.findKeyword("CharonSpectrumResolved").setUnits("(erg/cm^2/s/sr)/(DN/s/pix)");
      calibration.findKeyword("CharonSpectrumUnresolved").setUnits("(erg/cm^2/s/sr)/(DN/s/pix)");
      calibration.findKeyword("JupiterSpectrumResolved").setUnits("(erg/cm^2/s/sr)/(DN/s/pix)");
      calibration.findKeyword("JupiterSpectrumUnresolved").setUnits("(erg/cm^2/s/sr)/(DN/s/pix)");
      calibration.findKeyword("PlutoSpectrumResolved").setUnits("(erg/cm^2/s/sr)/(DN/s/pix)");
      calibration.findKeyword("PlutoSpectrumUnresolved").setUnits("(erg/cm^2/s/sr)/(DN/s/pix)");
      calibration.findKeyword("SolarPivotWavelength").setUnits("cm");
      calibration.findKeyword("JupiterPivotWavelength").setUnits("cm");
      calibration.findKeyword("PholusPivotWavelength").setUnits("cm");
      calibration.findKeyword("PlutoPivotWavelength").setUnits("cm");
      calibration.findKeyword("CharonPivotWavelength").setUnits("cm");
    }
  }

  //void flip(Buffer &in) {
  //  for(int i = 0; i < in.size() / 2; i++) {
  //    swap(in[i], in[in.size() - i - 1]);
  //  }
  //}
}
