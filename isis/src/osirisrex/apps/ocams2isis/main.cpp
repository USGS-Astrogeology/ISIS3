/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

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
using namespace Isis;

void IsisMain() {

  UserInterface &ui = Application::GetUserInterface();

  std::string error_message = "WARNING: This camera model is out of date. See ocam2isis documentation for details.";
  std::cerr << error_message << std::endl;

  ProcessImportFits importFits;

  importFits.setFitsFile(FileName(ui.GetFileName("FROM")));

  importFits.setProcessFileStructure(0);

  Cube *output = importFits.SetOutputCube("TO");

  // Get the directory where the OSIRIS-REx translation tables are.
  QString transDir = "$ISISROOT/appdata/translations/";

  // Temp storage of translated labels
  Pvl outLabel;

  // Get the FITS label
  Pvl fitsLabel;
  fitsLabel.addGroup(importFits.fitsImageLabel(0));

  // Create an Instrument group
  FileName insTransFile(transDir + "OsirisRexOcamsInstrument_fit.trn");
  PvlToPvlTranslationManager insXlater(fitsLabel, insTransFile.expanded());
  insXlater.Auto(outLabel);
  output->putGroup(outLabel.findGroup("Instrument", Pvl::Traverse));


  // Create a Band Bin group
  FileName bandTransFile(transDir + "OsirisRexOcamsBandBin_fit.trn");
  PvlToPvlTranslationManager bandBinXlater(fitsLabel, bandTransFile.expanded());
  bandBinXlater.Auto(outLabel);
  output->putGroup(outLabel.findGroup("BandBin", Pvl::Traverse));

  // Create a Kernels group
  FileName kernelsTransFile(transDir + "OsirisRexOcamsKernels_fit.trn");
  PvlToPvlTranslationManager kernelsXlater(fitsLabel, kernelsTransFile.expanded());
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
