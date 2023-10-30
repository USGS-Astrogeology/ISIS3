/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <cstdio>
#include <QString>
#include <algorithm>

#include "AlphaCube.h"
#include "Brick.h"
#include "FileName.h"
#include "AmicaImportFits.h"
#include "iTime.h"
#include "OriginalLabel.h"
#include "PixelType.h"
#include "ProcessImportPds.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;

void IsisMain ()
{
  ProcessImportPds p;
  UserInterface &ui = Application::GetUserInterface();

  // Get input file and set translation processing
  FileName inFile = ui.GetFileName("FROM");
  Pvl label;
  p.SetPdsFile (inFile.expanded(), "", label);

  // Add FITS header
  QString fitsImage = inFile.path() + "/" + QString::fromStdString(label.findKeyword("^IMAGE"));
  FileName fitsFile(fitsImage);
  AmicaImportFits fits(fitsFile, "FitsLabel");
  label.addGroup(fits.label());

  QString instid;
  QString missid;
  try {
    instid = QString::fromStdString(label.findKeyword ("INSTRUMENT_ID", PvlObject::Traverse));
    missid = QString::fromStdString(label.findKeyword ("INSTRUMENT_HOST_NAME", PvlObject::Traverse));
  }
  catch (IException &e) {
    QString msg = "Unable to read [INSTRUMENT_ID] or [INSTRUMENT_HOST_NAME] "
                  "from input file [" + inFile.expanded() + "]";
    throw IException(e, IException::Io,msg, _FILEINFO_);
  }

  instid = instid.simplified().trimmed();
  missid = missid.simplified().trimmed();
  if (missid != "HAYABUSA" && instid != "AMICA") {
    QString msg = "Input file [" + inFile.expanded() +
                  "] does not appear to be a " +
                  "Hayabusa/AMICA PDS label file.";
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }

  QString target;
  if (ui.WasEntered("TARGET")) {
    target = ui.GetString("TARGET");
  }

  // Set up image translation.  Omit the inclusion of this .lbl file and we
  // add the label with the FITS header augmentation as the orginal label
  // further below.
  p.OmitOriginalLabel();
  Cube *outcube = p.SetOutputCube ("TO");

  p.StartProcess();

  // Now flip the image lines since the first line is actually the last line.
  // Easiest way to do this is read the entire image array, flip lines and
  // rewrite the result back out.
  Brick image(outcube->sampleCount(), outcube->lineCount(), 1, Double);

  // Image extents
  int nsamps(outcube->sampleCount());
  int nlines(outcube->lineCount());
  int halfLines(nlines/2);

  //  Just in case there is more than 1 band in the image...
  image.begin();
  while (!image.end() ) {
    outcube->read(image);

    int samp0(0);                  // Index at first pixel of first line
    int samp1((nlines-1)*nsamps);  // Index at first pixel of last line
    for (int line = 0 ; line < halfLines ; line++ ) {
      for (int i = samp0, j = samp1, n = 0 ; n < nsamps ; i++, j++, n++) {
        swap(image[i], image[j]);
      }
      samp0 += nsamps;
      samp1 -= nsamps;
    }

    outcube->write(image);
    image.next();
  }

  // Get the path where the Hayabusa translation tables are.
  QString transDir = "$ISISROOT/appdata/translations/";

  // Create a PVL to store the translated labels in
  Pvl outLabel;

  // Translate the Instrument group
  FileName transFile = transDir + "HayabusaAmicaInstrument.trn";
  PvlToPvlTranslationManager instrumentXlater (label, transFile.expanded());
  instrumentXlater.Auto(outLabel);

  // Translate the Archive group
  transFile = transDir + "HayabusaAmicaArchive.trn";
  PvlToPvlTranslationManager archiveXlater (label, transFile.expanded());
  archiveXlater.Auto(outLabel);

  // Translate the BandBin group
  transFile = transDir + "HayabusaAmicaBandBin.trn";
  PvlToPvlTranslationManager bandBinXlater (label, transFile.expanded());
  bandBinXlater.Auto(outLabel);

  // Translate the Kernels group
  transFile = transDir + "HayabusaAmicaKernels.trn";
  PvlToPvlTranslationManager kernelsXlater (label, transFile.expanded());
  kernelsXlater.Auto(outLabel);

  //  Create YearDoy keyword in Archive group
  iTime stime(QString::fromStdString(outLabel.findGroup("Instrument", Pvl::Traverse)["StartTime"][0]));
  PvlKeyword yeardoy("YearDoy", std::to_string(stime.Year()*1000 + stime.DayOfYear()));
  outLabel.findGroup("Archive", Pvl::Traverse).addKeyword(yeardoy);


  //  Update target if user specifies it
  if (!target.isEmpty()) {
    PvlGroup &igrp = outLabel.findGroup("Instrument",Pvl::Traverse);
    igrp["TargetName"] = target.toStdString();
  }

  QString units = "";
  if (outLabel.findGroup("BandBin", Pvl::Traverse).hasKeyword("Unit")) {
    units = QString::fromStdString(outLabel.findGroup("BandBin", Pvl::Traverse).findKeyword("Unit")[0]).toLower();
  }
  else {
    units = "nanometers";
  }
  outLabel.findGroup("BandBin", Pvl::Traverse).findKeyword("Center").setUnits(units.toStdString());
  outLabel.findGroup("BandBin", Pvl::Traverse).findKeyword("Width").setUnits(units.toStdString());

  // Write the BandBin, Archive, and Instrument groups
  // to the output cube label
  outcube->putGroup(outLabel.findGroup("Instrument",Pvl::Traverse));
  outcube->putGroup(outLabel.findGroup("Archive",Pvl::Traverse));
  outcube->putGroup(outLabel.findGroup("BandBin",Pvl::Traverse));
  outcube->putGroup(outLabel.findGroup("Kernels",Pvl::Traverse));

  // Now write the FITS augmented label as the original label
  OriginalLabel oldLabel(label);
  outcube->write(oldLabel);

#if 0
  // Check for subimage and create an AlphaCube that describes thes subarea and
  // update the cube labels. DO NOT adjust for scale.  Camera model will handle
  // that. Coordinates in label are 0-based.
  if ( (1024 != nsamps) || (1024 != nlines) ) {
     PvlGroup inst = outLabel.findGroup("Instrument",Pvl::Traverse);
     int starting_samp = (int) inst["FirstSample"] + 1;
     int starting_line = (int) inst["FirstLine"] + 1;
     int ending_samp = (int) inst["LastSample"] + 1;
     int ending_line = (int) inst["LastLine"] + 1;

     AlphaCube subarea(1024, 1024,
                       nsamps, nlines,
                       starting_samp - 0.5, starting_line - 0.5,
                       ending_samp + 0.5,   ending_line + 0.5);

     subarea.UpdateGroup(*outcube);
  }
#endif

  //  All done...
   p.EndProcess();
}
