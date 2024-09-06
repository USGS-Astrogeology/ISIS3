/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "Cube.h"
#include "IException.h"
#include "NirsImportFits.h"
#include "iTime.h"
#include "LineManager.h"
#include "PixelType.h"
#include "Preference.h"
#include "ProcessByLine.h"
#include "ProcessImportPds.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlToPvlTranslationManager.h"
#include "OriginalLabel.h"
#include "UserInterface.h"

#include <QString>
#include <QFile>

using namespace std;
using namespace Isis;

void IsisMain() {

  UserInterface &ui = Application::GetUserInterface();

  ProcessImportPds processPDS;

  FileName detachedLabel = ui.GetFileName("FROM");
  Pvl label;
  processPDS.SetPdsFile (detachedLabel.expanded(), "", label);

  QString fitsImage = detachedLabel.path() + "/" + QString::fromStdString(label.findKeyword("^COMBINED_SPECTRUM"));
  FileName fitsFile(fitsImage);
  NirsImportFits fits(fitsFile, "FitsLabel");
  label += fits.label();

  QString axisCount;
  QString axis1Length;
  QString axis2Length;
  try {
    axisCount   = QString::fromStdString(label.findKeyword ("NAXIS", PvlObject::Traverse));
    axis1Length = QString::fromStdString(label.findKeyword ("NAXIS1", PvlObject::Traverse));
    axis2Length = QString::fromStdString(label.findKeyword ("NAXIS2", PvlObject::Traverse));
  }
  catch (IException &e) {
    std::string msg = "Unable to read [NAXIS], [NAXIS1] or [NAXIS2] "
                  "from FITS label in input [" + fitsImage + "].";
    throw IException(e, IException::Io, msg, _FILEINFO_);
  }
  axisCount = axisCount.simplified().trimmed();
  axis1Length = axis1Length.simplified().trimmed();
  axis2Length = axis2Length.simplified().trimmed();
  if ( !( axisCount   == "2"  &&
          axis1Length == "64" &&
          axis2Length == "2"     ) ) {
    std::string msg = "Input file [" + fitsImage +
                  "] does not have the correct dimensions " +
                  "for a Hayabusa NIRS FITS image.\n" +
                  "Expected dimensions are [2] axes, [64 x 2]. " +
                  "File dimensions are [" + axisCount + "] axes, [" +
                  axis1Length + " x " + axis2Length + "].";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  processPDS.OmitOriginalLabel();

  QString tempCubeName = detachedLabel.baseName() + ".temp.cub";
  CubeAttributeOutput outputAtts = ui.GetOutputAttribute("TO");
  outputAtts.setPixelType( Isis::Real);
  Cube* tempCube = processPDS.SetOutputCube(tempCubeName, outputAtts);

  // Convert the fits file into an ISIS cube
  processPDS.StartProcess();

  // Check the temp cube's dimensions
  if ( (tempCube->sampleCount() != 64) ||
       (tempCube->lineCount()   != 2 ) ||
       (tempCube->bandCount()   != 1 )    ) {
    std::string msg = "Invalid temp cube dimensions. Dimensions "
                  "must be 64 samples, by 2 lines, by 1 band.\n"
                  "Temp cube dimensions are [" +
                  toString( tempCube->sampleCount() ) +
                  "] samples, by [" +
                  toString( tempCube->lineCount() ) +
                  "] lines, by [" +
                  toString( tempCube->bandCount() ) +
                  "] bands.";
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }

  // Write the image data to the output cubes

  Cube* reflectanceCube = processPDS.SetOutputCube(ui.GetCubeName("TO"),
                                                   outputAtts,
                                                   1, 1, 64);
  Cube* stdevCube = processPDS.SetOutputCube(ui.GetCubeName("TOSTDDEV"),
                                             outputAtts,
                                             1, 1, 64);

  LineManager tempManager(*tempCube);
  LineManager reflectanceManager(*reflectanceCube);
  LineManager stdevManager(*reflectanceCube);

  // The first line is reflectance values in reverse order
  tempManager.SetLine(1);
  tempCube->read(tempManager);
  for (int sample = 0; sample < tempCube->sampleCount(); sample++) {
    reflectanceManager.SetLine(1, 64 - sample);
    reflectanceManager[0] = tempManager[sample];
    reflectanceCube->write(reflectanceManager);
  }

  // The second line is standard deviation values in reverse order
  tempManager.SetLine(2);
  tempCube->read(tempManager);
  for (int sample = 0; sample < tempCube->sampleCount(); sample++) {
    stdevManager.SetLine(1, 64 - sample);
    stdevManager[0] = tempManager[sample];
    stdevCube->write(stdevManager);
  }

  QString transDir = "$ISISROOT/appdata/translations/";
  Pvl newLabel;

  QString instTrans = transDir + "HayabusaNirsInstrument.trn";
  Isis::PvlToPvlTranslationManager instXlater(label, instTrans);
  instXlater.Auto(newLabel);

  QString archTrans = transDir + "HayabusaNirsArchive.trn";
  Isis::PvlToPvlTranslationManager archXlater(label, archTrans);
  archXlater.Auto(newLabel);

  QString bandTrans = transDir + "HayabusaNirsBandBin.trn";
  Isis::PvlToPvlTranslationManager bandXlater(label, bandTrans);
  bandXlater.Auto(newLabel);

  QString kernTrans = transDir + "HayabusaNirsKernels.trn";
  Isis::PvlToPvlTranslationManager kernXlater(label, kernTrans);
  kernXlater.Auto(newLabel);

  // Create the bandbin group
  // The following equation is from:
  // Abe et al., 2004. Characteristics and current status of near infrared
  // spectrometer for Hayabusa mission. Lunar & Planet. Sci. XXXV, 1724.
  PvlKeyword filterNumber("FilterNumber");
  PvlKeyword center("Center");
  for (int channelNumber = 1; channelNumber <= 64; channelNumber++) {
    filterNumber += std::to_string( channelNumber );
    center += std::to_string( 2.27144 - 0.02356 * (65 - channelNumber) );
  }
  newLabel.findGroup("BandBin", Pvl::Traverse).addKeyword(filterNumber);
  newLabel.findGroup("BandBin", Pvl::Traverse).addKeyword(center);
  newLabel.findGroup("BandBin", Pvl::Traverse).findKeyword("Width").setUnits("micrometers");

  //  Create YearDoy keyword in Archive group
  iTime stime(QString::fromStdString(newLabel.findGroup("Instrument", Pvl::Traverse)["StartTime"][0]));
  PvlKeyword yeardoy("YearDoy", std::to_string(stime.Year()*1000 + stime.DayOfYear()));
  newLabel.findGroup("Archive", Pvl::Traverse).addKeyword(yeardoy);

  // Add the instrument, band bin, archive, mission data, and kernels
  // groups to the output cube labels
  reflectanceCube->putGroup( newLabel.findGroup("Instrument", Pvl::Traverse) );
  reflectanceCube->putGroup( newLabel.findGroup("BandBin", Pvl::Traverse) );
  reflectanceCube->putGroup( newLabel.findGroup("Archive", Pvl::Traverse) );
  reflectanceCube->putGroup( newLabel.findGroup("MissionData", Pvl::Traverse) );
  reflectanceCube->putGroup( newLabel.findGroup("Kernels", Pvl::Traverse) );
  stdevCube->putGroup( newLabel.findGroup("Instrument", Pvl::Traverse) );
  stdevCube->putGroup( newLabel.findGroup("BandBin", Pvl::Traverse) );
  stdevCube->putGroup( newLabel.findGroup("Archive", Pvl::Traverse) );
  stdevCube->putGroup( newLabel.findGroup("MissionData", Pvl::Traverse) );
  stdevCube->putGroup( newLabel.findGroup("Kernels", Pvl::Traverse) );

  // Attach the original fits label and detached label
  OriginalLabel originalFits(label);
  reflectanceCube->write(originalFits);
  stdevCube->write(originalFits);

  // Clean up
  processPDS.EndProcess();
  QFile(tempCubeName).remove();
}
