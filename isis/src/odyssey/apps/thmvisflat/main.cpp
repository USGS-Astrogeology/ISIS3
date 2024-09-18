/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"
#include "Blob.h"
#include "Cube.h"
#include "CubeAttribute.h"
#include "LineManager.h"
#include "Progress.h"
#include "SpecialPixel.h"

using namespace Isis;
using namespace std;

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  CubeAttributeInput inAtt = ui.GetInputAttribute("FROM");
  Cube icube;

  if (inAtt.bands().size() != 0) {
    icube.setVirtualBands(inAtt.bands());
  }

  icube.open(QString::fromStdString(FileName(ui.GetCubeName("FROM").toStdString()).expanded()));

  // Make sure it is a Themis EDR/RDR
  FileName inFileName = ui.GetCubeName("FROM").toStdString();
  try {
    if(icube.group("Instrument")["InstrumentID"][0] != "THEMIS_VIS") {
      std::string msg = "This program is intended for use on THEMIS VIS images only. [";
      msg += inFileName.expanded() + "] does not appear to be a THEMIS VIS image.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }
  catch(IException &e) {
    std::string msg = "This program is intended for use on THEMIS VIS images only. [";
    msg += inFileName.expanded() + "] does not appear to be a THEMIS VIS image.";
    throw IException(e, IException::User, msg, _FILEINFO_);
  }

  vector<Cube *> flatcubes;
  vector<LineManager *> fcubeMgrs;
  int summing = std::stoi(icube.group("Instrument")["SpatialSumming"][0]);

  for(int filt = 0; filt < 5; filt++) {
    QString filePattern = "$odyssey/calibration/flat_filter_";
    filePattern += QString::number(filt + 1) + "_summing_";
    filePattern += QString::number(summing) + "_v????.cub";
    FileName flatFile = FileName(filePattern.toStdString()).highestVersion();
    Cube *fcube = new Cube();
    fcube->open(QString::fromStdString(flatFile.expanded()));
    flatcubes.push_back(fcube);

    LineManager *fcubeMgr = new LineManager(*fcube);
    fcubeMgr->SetLine(1, 1);
    fcubeMgrs.push_back(fcubeMgr);
  }

  Cube ocube;

  CubeAttributeOutput outAtt = ui.GetOutputAttribute("TO");
  ocube.setDimensions(icube.sampleCount(), icube.lineCount(), icube.bandCount());
  ocube.setByteOrder(outAtt.byteOrder());
  ocube.setFormat(outAtt.fileFormat());
  ocube.setLabelsAttached(outAtt.labelAttachment() == AttachedLabel);
  ocube.setPixelType(outAtt.pixelType());

  ocube.create(QString::fromStdString(FileName(ui.GetCubeName("TO").toStdString()).expanded()));

  LineManager icubeMgr(icube);
  vector<int> filter;

  PvlKeyword &filtNums =
      icube.label()->findGroup("BandBin", Pvl::Traverse)["FilterNumber"];
  for(int i = 0; i < filtNums.size(); i++) {
    filter.push_back(std::stoi(filtNums[i]));
  }

  LineManager ocubeMgr(ocube);
  ocubeMgr.SetLine(1, 1);

  Progress prog;
  prog.SetText("Applying Flat-Field Correction");
  prog.SetMaximumSteps(ocube.lineCount() * ocube.bandCount());
  prog.CheckStatus();

  do {
    icube.read(icubeMgr);
    ocube.read(ocubeMgr);

    int fcubeIndex = filter[ocubeMgr.Band()-1] - 1;
    flatcubes[fcubeIndex]->read((*fcubeMgrs[fcubeIndex]));

    for(int i = 0; i < ocubeMgr.size(); i++) {
      if(IsSpecial((*fcubeMgrs[fcubeIndex])[i]) || (*fcubeMgrs[fcubeIndex])[i] == 0.0) {
        ocubeMgr[i] = Isis::Null;
      }
      else if(IsSpecial(icubeMgr[i])) {
        ocubeMgr[i] = icubeMgr[i];
      }
      else {
        ocubeMgr[i] = icubeMgr[i] / (*fcubeMgrs[fcubeIndex])[i];
      }
    }

    ocube.write(ocubeMgr);

    icubeMgr++;
    ocubeMgr++;

    for(int i = 0; i < (int)fcubeMgrs.size(); i++) {
      (*fcubeMgrs[i]) ++;

      if(fcubeMgrs[i]->end()) {
        fcubeMgrs[i]->SetLine(1, 1);
      }
    }

    prog.CheckStatus();
  }
  while (!ocubeMgr.end());

  // Propagate labels and objects (in case of spice data)
  PvlObject &inCubeObj = icube.label()->findObject("IsisCube");
  PvlObject &outCubeObj = ocube.label()->findObject("IsisCube");

  for(int g = 0; g < inCubeObj.groups(); g++) {
    outCubeObj.addGroup(inCubeObj.group(g));
  }

  for(int o = 0; o < icube.label()->objects(); o++) {
    if(icube.label()->object(o).isNamed("Table")) {
      Blob t(icube.label()->object(o)["Name"],
             icube.label()->object(o).name());
      icube.read(t);
      ocube.write(t);
    }
  }

  icube.close();
  ocube.close();

  for(int i = 0; i < (int)flatcubes.size(); i++) {
    delete fcubeMgrs[i];
    delete flatcubes[i];
  }

  fcubeMgrs.clear();
  flatcubes.clear();
}
