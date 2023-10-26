/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "CubeAttribute.h"
#include "FileName.h"
#include "IException.h"
#include "iTime.h"
#include "ProcessImportPds.h"
#include "OriginalLabel.h"
#include "UserInterface.h"

#include <cstdio>
#include <QString>
#include <QByteArray>
#include <QFileInfo>
#include <QFile>


using namespace std;
using namespace Isis;
bool summed;

Cube *summedOutput;

void translateData(Buffer &inData);
void translateLabels(Pvl &pdsLabel, Cube *ocube);
void fixPvl(QString fileName);

void IsisMain() {
  //initialize globals
  summed = false;
  summedOutput = NULL;
  // Grab the file to import
  ProcessImportPds p;
  UserInterface &ui = Application::GetUserInterface();
  FileName inFile = ui.GetFileName("FROM");
  FileName outFile = ui.GetCubeName("TO");

  // Apply a fix to the gallileo pds labels so they can be read
  fixPvl(inFile.toString());

  // Make sure it is a Galileo SSI image
  Pvl lab(inFile.expanded());

  //Checks if in file is rdr
  if(lab.hasObject("IMAGE_MAP_PROJECTION")) {
    QString msg = "[" + inFile.name() + "] appears to be an rdr file.";
    msg += " Use pds2isis.";
    throw IException(IException::Io, msg, _FILEINFO_);
  }

  // data set id value must contain "SSI-2-REDR-V1.0" or "SSI-2-REDR-V1.1" (valid SSI image)
  // or "SSI-4-REDR-V1.0" or "SSI-4-REDR-V1.1"(reconstructed from garbled SSI image)
  QString dataSetId;
  dataSetId = (QString)lab["DATA_SET_ID"];

  if(!dataSetId.contains("SSI-2-REDR-V1.0") && !dataSetId.contains("SSI-2-REDR-V1.1")
      && !dataSetId.contains("SSI-4-REDR-V1.0") && !dataSetId.contains("SSI-4-REDR-V1.1") ) {
    QString msg = "Invalid DATA_SET_ID [" + dataSetId + "]" + 
    " from input file [" + inFile.expanded() + "]";
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }

  // set summing mode
  if (ui.GetString("FRAMEMODE") == "AUTO") {
    double frameDuration = lab["FRAME_DURATION"];
    // reconstructed images are 800x800 (i.e. not summed)
    // even though they have frame duration of 2.333
    // (which ordinarily indicates a summed image)
    if(dataSetId.contains("SSI-4-REDR-V1.0") || dataSetId.contains("SSI-4-REDR-V1.1")) {
      summed = false;
    }
    else if (frameDuration > 2.0 && frameDuration < 3.0) {
      summed = true;
    }
    // seti documentation implies valid frame duration values are 2.333, 8.667, 30.333, 60.667
    // however some images have value 15.166 (see example 3700R.LBL)
    else if (frameDuration > 15.0 && frameDuration < 16.0) {
      summed = true;
    }
  }
  else if (ui.GetString("FRAMEMODE") == "SUMMED") {
    summed = true;
  }
  else {
    summed = false;
  }

  Progress prog;
  Pvl pdsLabel;
  p.SetPdsFile(inFile.expanded(), "", pdsLabel);

  // If summed handle the image similarly to pds2isis
  // with an extra translation step
  if (!summed) {
    Cube *ocube = p.SetOutputCube("TO");
    p.StartProcess();
    translateLabels(pdsLabel, ocube);
    p.EndProcess();
  }
  else {
  // Otherwise the dimensions of the cube need to be cut in half before
  // processsing. Since we didn't set the output cube we need to take care
  // of writting the original label ourselves
    summedOutput = new Cube();
    summedOutput->setDimensions(p.Samples() / 2, p.Lines() / 2, p.Bands());
    summedOutput->setPixelType(p.PixelType());
    summedOutput->create(outFile.expanded());

    p.StartProcess(translateData);
    translateLabels(pdsLabel, summedOutput);

    OriginalLabel ol(Pvl(inFile.expanded()));
    summedOutput->write(ol);
    summedOutput->close();
    delete summedOutput;
  }
  return;
}

/**
  * This fixes a problem with some of the Pvl files where a comment
  * was left open.  If the file has this error, the comment is closed
  * and the file is saved.  If the file does not, it's closed without
  * changing anything.
  *
  * @param fileName  The full path + file name of the Pvl file
  *     being checked
  *
  */
void fixPvl(QString fileName){

    QFile pvlFile;

    pvlFile.setFileName(fileName);
    pvlFile.open(QFile::ReadWrite | QFile::Text);

    //Read the Pvl file into a byte array
    QByteArray fileData = pvlFile.readAll();

    //Is this one of the messed up files?
    if (!fileData.contains(QByteArray("/* Image Object /") ) ){
            pvlFile.close();

     //No - so return without doing anything
     return;

    }

    //Otherwise, edit the file in place

    fileData.replace(QByteArray("/* Image Object /"),QByteArray("/* Image Object */"));

    pvlFile.seek(0);
    pvlFile.write(fileData);
    pvlFile.close();


}

void translateData(Isis::Buffer &inData) {
  summedOutput->write(inData);
}

void translateLabels(Pvl &pdsLabel, Cube *ocube) {

  // Transfer the instrument group to the output cube
  FileName transFile("$ISISROOT/appdata/translations/GalileoSsi.trn");

  // Get the translation manager ready
  PvlToPvlTranslationManager labelXlater(pdsLabel, transFile.expanded());
  // Pvl outputLabels;
  Pvl *outputLabel = ocube->label();
  labelXlater.Auto(*(outputLabel));

  //Add to the Archive Group
  PvlGroup &arch = outputLabel->findGroup("Archive", Pvl::Traverse);
  PvlGroup &inst = outputLabel->findGroup("Instrument", Pvl::Traverse);
  arch.addKeyword(PvlKeyword("DataType", "RADIANCE"));
  QString CTC = (QString) arch.findKeyword("ObservationId");
  QString CTCout = CTC.mid(0, 2);
  arch.addKeyword(PvlKeyword("CalTargetCode", CTCout));

  // Add to the Instrument Group
  QString itest = (QString) inst.findKeyword("StartTime");
  itest.remove("Z");
  inst.findKeyword("StartTime").setValue(itest);
  //change exposure duration to seconds
  double expDur = inst.findKeyword("exposureDuration");
  double expDurOut = expDur / 1000.0;
  inst.findKeyword("exposureDuration").setValue(std::to_string(expDurOut), "seconds");
  inst.addKeyword(PvlKeyword("FrameDuration",
                             (QString) pdsLabel["frameDuration"], "seconds"));

  //Calculate the Frame_Rate_Id keyword
  QString frameModeId = "FULL";
  int summingMode = 1;

  if(summed) {
    frameModeId = "SUMMATION";
    summingMode = 2;
  }

  inst.addKeyword(PvlKeyword("Summing", toString(summingMode)));
  inst.addKeyword(PvlKeyword("FrameModeId", frameModeId));

  // Create the Band bin Group
  PvlGroup &bandBin = outputLabel->findGroup("BandBin", Pvl::Traverse);
  QString filterName = pdsLabel["FILTER_NAME"];
  QString waveLength = "";
  QString width = "";
  if(filterName == "CLEAR") {
    waveLength = "0.611";
    width = ".44";
  }
  if(filterName == "VIOLET") {
    waveLength = "0.404";
    width = ".05";
  }
  if(filterName == "GREEN") {
    waveLength = "0.559";
    width = ".06";
  }
  if(filterName == "RED") {
    waveLength = "0.671";
    width = ".06";
  }
  if(filterName == "IR-7270") {
    waveLength = "0.734";
    width = ".01";
  }
  if(filterName == "IR-7560") {
    waveLength = "0.756";
    width = ".018";
  }
  if(filterName == "IR-8890") {
    waveLength = "0.887";
    width = ".116";
  }
  if(filterName == "INFRARED") {
    waveLength = "0.986";
    width = ".04";
  }
  bandBin.addKeyword(PvlKeyword("Center", waveLength, "micrometers"));
  bandBin.addKeyword(PvlKeyword("Width", width, "micrometers"));

  //create the kernel group
  PvlGroup kern("Kernels");
  kern += PvlKeyword("NaifFrameCode", "-77001");
  ocube->putGroup(kern);
}
