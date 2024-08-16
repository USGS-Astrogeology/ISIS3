/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <cstdio>
#include <QString>

#include "ProcessImportPds.h"

#include "UserInterface.h"
#include "FileName.h"
#include "Spice.h"
#include "Target.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  ProcessImportPds p;
  Pvl label;
  UserInterface &ui = Application::GetUserInterface();

  QString labelFile = ui.GetFileName("FROM");

  p.SetPdsFile(labelFile, "", label);
  Cube *ocube = p.SetOutputCube("TO");

  p.StartProcess();

  // Get the mapping labels
  Pvl otherLabels;
  p.TranslatePdsProjection(otherLabels);

  // Get the path where the MRO HiRISE RDR translation tables are.
  QString transDir = "$ISISROOT/appdata/translations/";

  // Translate the BandBin group
  FileName transFile(transDir + "MroHiriseRdrBandBin.trn");
  PvlToPvlTranslationManager bandBinXlater(label, transFile.expanded());
  bandBinXlater.Auto(otherLabels);

  // Translate the Mosaic group
  transFile = transDir + "MroHiriseRdrMosaic.trn";
  PvlToPvlTranslationManager archiveXlater(label, transFile.expanded());
  archiveXlater.Auto(otherLabels);

  // Write the BandBin, Archive, and Mapping groups to the output cube label
  ocube->putGroup(otherLabels.findGroup("BandBin"));

  // Reorder CPMM keywords back to original arrangement.  This copies the values
  //  back to keywords in place.
  PvlGroup mosgrp = otherLabels.findGroup("Mosaic");
  PvlKeyword &ccdFlag = mosgrp.findKeyword("SpecialProcessingFlag");
  PvlKeyword &ccdBin = mosgrp.findKeyword("cpmmSummingFlag");
  PvlKeyword &ccdTdi = mosgrp.findKeyword("cpmmTdiFlag");

  //  Make temp copies of keywords
  PvlKeyword tempccdFlag = ccdFlag;
  PvlKeyword tempccdBin = ccdBin;
  PvlKeyword tempccdTdi = ccdTdi;
  const unsigned int cpmmByCcd[] = {0, 1, 2, 3, 5, 8, 10, 11, 12, 13, 6, 7, 4, 9};
  for (int ccd = 0; ccd < 14; ++ccd) {
    ccdFlag[cpmmByCcd[ccd]] = tempccdFlag[ccd];
    ccdBin[cpmmByCcd[ccd]]  = tempccdBin[ccd];
    ccdTdi[cpmmByCcd[ccd]]  = tempccdTdi[ccd];
  }
  ocube->putGroup(mosgrp);

//  Modify the output Mosaic group if the Projection is of type
//  Equirectangular.
  PvlGroup mapgrp = otherLabels.findGroup("Mapping");
  if (QString::fromStdString(mapgrp["ProjectionName"][0]).toUpper() == "EQUIRECTANGULAR") {

    //  Get the target and check for validity
    QString target = QString::fromStdString(label.findKeyword("TargetName", PvlObject::Traverse)[0]);
    PvlGroup radii = Target::radiiGroup(target);

    // Set existing radius to CenterLatitudeRadius
    PvlKeyword &eqRadius =  mapgrp.findKeyword("EquatorialRadius");
    PvlKeyword &polRadius = mapgrp.findKeyword("PolarRadius");

    // Derive (copy, actually) the center radius from the equator radii and
    // update the name
    PvlKeyword clatrad =  eqRadius;
    clatrad.setName("CenterLatitudeRadius");

    //  Assign the proper radii to the group keywords
    eqRadius = radii["EquatorialRadius"];
    polRadius = radii["PolarRadius"];
    mapgrp += clatrad;  // Don't do this before updating the above
    // keyword references!  Bad things happen!
  }

  //  Write the group to the label
  ocube->putGroup(mapgrp);

  p.EndProcess();

  return;
}
