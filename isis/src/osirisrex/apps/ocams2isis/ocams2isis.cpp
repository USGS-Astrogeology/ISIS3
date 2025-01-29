/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ocams2isis.h"

#include <fstream>
#include <iostream>

#include <QString>

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

using namespace std;

namespace Isis {

  /**
   * Ingest OSIRIS-REx OCams fits file as ISIS cube.
   *
   * @param ui UserInterface object containing parameters
   */
  void ocams2isis(UserInterface &ui) {

    FileName fitsFileName(ui.GetFileName("FROM"));

    return ocams2isis(fitsFileName, ui);
  }


  void ocams2isis(FileName &fitsFileName, UserInterface &ui) {

    // open fits file
    ProcessImportFits importFits;
    importFits.setFitsFile(fitsFileName);

    importFits.setProcessFileStructure(0);

    // Get the primary FITS label so we can confirm its the proper format
    // and get some values for processing
    Pvl fLabel;
    fLabel.addGroup(importFits.fitsImageLabel(0));

    // instantiate output cube
    Cube *output = importFits.SetOutputCube("TO", ui);

    // Get the directory where the OSIRIS-REx translation tables are.
    QString transDir = "$ISISROOT/appdata/translations/";

    // Temp storage of translated labels
    Pvl outLabel;

    // Create an Instrument group
    FileName insTransFile(transDir + "OsirisRexOcamsInstrument_fit.trn");
    PvlToPvlTranslationManager insXlater(fLabel, insTransFile.expanded());
    insXlater.Auto(outLabel);
    output->putGroup(outLabel.findGroup("Instrument", Pvl::Traverse));


    // Create a Band Bin group
    FileName bandTransFile(transDir + "OsirisRexOcamsBandBin_fit.trn");
    PvlToPvlTranslationManager bandBinXlater(fLabel, bandTransFile.expanded());
    bandBinXlater.Auto(outLabel);
    output->putGroup(outLabel.findGroup("BandBin", Pvl::Traverse));

    // Create a Kernels group
    FileName kernelsTransFile(transDir + "OsirisRexOcamsKernels_fit.trn");
    PvlToPvlTranslationManager kernelsXlater(fLabel, kernelsTransFile.expanded());
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
