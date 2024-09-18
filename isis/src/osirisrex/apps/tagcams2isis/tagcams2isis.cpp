/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "tagcams2isis.h"

#include <fstream>
#include <iostream>
#include <string>
#include <bitset>

#include <QString>
#include <QObject>

#include "Cube.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "OriginalLabel.h"
#include "ProcessImportFits.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "UserInterface.h"

using namespace std;

namespace Isis {

  /**
   * Ingest OSIRIS-REx fits file as ISIS cube.
   *
   * @param ui UserInterface object containing parameters
   */
  void tagcams2isis(UserInterface &ui) {

    FileName fitsFileName(ui.GetFileName("FROM").toStdString());

    return tagcams2isis(fitsFileName, ui);
  }


  void tagcams2isis(FileName &fitsFileName, UserInterface &ui) {
    
    // open fits file
    ProcessImportFits importFits;
    importFits.setFitsFile(fitsFileName);

    // Get the primary FITS label so we can confirm its the proper format
    // and get some values for processing
    Pvl flabel;
    flabel.addGroup(importFits.fitsImageLabel(0));

    // Do we remove image boundary pixels?
    bool removeCal = ui.GetBoolean("REMOVECALPIXELS");

    // Collect some raw label values. If these fail, then its likely
    // this is not a TAGCAMS image
    int rawSamples, rawLines, summing, binning, rawcamT, tcmode;
    QString instId;
    try {
      rawSamples = (int)flabel.findKeyword("NAXIS1", Pvl::Traverse);
      rawLines = (int) flabel.findKeyword("NAXIS2", Pvl::Traverse);
      summing  = (int) flabel.findKeyword("TCSUM", Pvl::Traverse);
      binning   = (int) flabel.findKeyword("TCSSMPL", Pvl::Traverse);
      rawcamT  = (int) flabel.findKeyword("TCCHTEMP", Pvl::Traverse);
      tcmode  = (int) flabel.findKeyword("TCMODE", Pvl::Traverse);
      instId  = QString::fromStdString((flabel.findKeyword("INSTRUME", Pvl::Traverse))).simplified();
    }
    catch (IException &ie) {
      std::string msg = "Unable to retrieve expected TAGCAMS keywords."
                                "The file provided in FROM is likely not a TAGCAMS image.";
      throw IException(ie, IException::User, msg, _FILEINFO_);
    }

    // Check for summing conditions
    summing = ( summing <= 1 )  ?  1 : 2;
    if (binning == 1) {
      binning = 2;
    }
    else if (binning == 4 ) {
      binning = 4;
    }
    else if (binning >= 16) {
      binning = binning - 14;
    }
    else { // ( binning <= 0)  || binning is defined
      binning = 1;
    }
 
    // True pixel scaling
    int pixScale = summing * binning;

    // Was boundary data included? Bit 4 is set if so...
    std::bitset<8> darkbit(std::string("00010000"));
    bool hasDark = darkbit.to_ulong() & (unsigned long) tcmode;
    // cout << "HasDark: " << hasDark << "\n";

    // Full input frame size before dark removal (if present/requested)
    int ns(rawSamples), nl(rawLines), nb(1);

    // Compute the boundary pixels if present and requested.
    if ( removeCal && hasDark ) {
      QString bitpix = QString::fromStdString(flabel.findKeyword("BITPIX", Pvl::Traverse));
      int bytesPerPix = abs(bitpix.toInt()) / 8;
      importFits.SetDataHeaderBytes(bytesPerPix * ((54 * rawSamples) / pixScale));
      importFits.SetDataPrefixBytes(bytesPerPix * (144 / pixScale));
      importFits.SetDataSuffixBytes(bytesPerPix * (16  / pixScale));
      // importFits.SetDataTrailerBytes(bytesPerPix * ((6 * (rawSamples-16))  / pixScale));

      // Adjust the number of pixels removed by binning (NOTE this may not
      // be correct as we don't have any images to test that have any
      // summing or binning)
      ns -= (144 + 16) / pixScale;
      nl -= (54 + 6) / pixScale;
    }

    // Now set the output file characteristics
    importFits.setProcessFileStructure(0);
    importFits.SetDimensions(ns,nl,nb);
    Cube *output = importFits.SetOutputCube("TO", ui);

    QString target;
    if(ui.WasEntered("TARGET")) {
      target = ui.GetString("TARGET");
    }

    // Get the directory where the OSIRIS-REx translation tables are.
    std::string transDir = "$ISISROOT/appdata/translations/";

    // Temp storage of translated labels
    Pvl outLabel;

    // Get the FITS label
    Pvl fitsLabel;
    fitsLabel.addGroup(importFits.fitsImageLabel(0));

    // Create an Instrument group
    FileName insTransFile(transDir + "OsirisRexTagcamsInstrument_fit.trn");
    PvlToPvlTranslationManager insXlater(fitsLabel, QString::fromStdString(insTransFile.expanded()));
    insXlater.Auto(outLabel);
    PvlGroup &instGrp(outLabel.findGroup("Instrument", Pvl::Traverse));

    // Create an Archive group
    FileName archTransFile(transDir + "OsirisRexTagcamsArchive_fit.trn");
    PvlToPvlTranslationManager archXlater(fitsLabel, QString::fromStdString(archTransFile.expanded()));
    archXlater.Auto(outLabel);
    PvlGroup &archiveGrp(outLabel.findGroup("Archive", Pvl::Traverse));

    // Add product id which is just the filename base
    FileName from(ui.GetFileName("FROM").toStdString());
    QString prodid = QString::fromStdString(from.baseName());
    archiveGrp.addKeyword(PvlKeyword("SourceProductId", prodid.toStdString()), archiveGrp.begin());

    // Create YearDoy keyword in Archive group
    iTime stime(QString::fromStdString(instGrp["StartTime"][0]));
    PvlKeyword yeardoy("YearDoy", std::to_string(stime.Year()*1000 + stime.DayOfYear()));
    archiveGrp.addKeyword(yeardoy);

    output->putGroup(archiveGrp);

    //  If the user specifed the target explicitly or it doesn't exist, create
    //  something so the camera will always work
    if (instGrp.findKeyword("TargetName").isNull() || (!target.isEmpty())) {
      if (!target.isEmpty()) {
        instGrp["TargetName"] = target.toStdString();
      }
      else {
        instGrp["TargetName"] = "Sky";
      }
    }

    // Convert raw temp DN to celsius - from UA-SIS-9.4.4-322, Rev. 3.0
    // Updated b values provided by NAV team.
    double a = 0.15259;
    double b = -273.14;
    if ( "NCM" == instId ) b = -275.02;
    if ( "NFT" == instId ) b = -273.43;

    double camHeadTempC = a * ((double)rawcamT) + b;
    instGrp.addKeyword(PvlKeyword("CameraHeadTemperature", std::to_string(camHeadTempC), "celsius"));
    output->putGroup(instGrp);

    // Create a Band Bin group
    FileName bandTransFile(transDir + "OsirisRexTagcamsBandBin_fit.trn");
    PvlToPvlTranslationManager bandBinXlater(fitsLabel, QString::fromStdString(bandTransFile.expanded()));
    bandBinXlater.Auto(outLabel);
    output->putGroup(outLabel.findGroup("BandBin", Pvl::Traverse));

    // Create a Kernels group
    FileName kernelsTransFile(transDir + "OsirisRexTagcamsKernels_fit.trn");
    PvlToPvlTranslationManager kernelsXlater(fitsLabel, QString::fromStdString(kernelsTransFile.expanded()));
    kernelsXlater.Auto(outLabel);
    output->putGroup(outLabel.findGroup("Kernels", Pvl::Traverse));

    // Save the input FITS label in the Cube original labels
    Pvl pvl;
    pvl += importFits.fitsImageLabel(0);
    OriginalLabel originals(pvl);
    output->write(originals);

    // Convert the image data
    importFits.StartProcess();
    importFits.Finalize();
  }
}
