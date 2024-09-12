/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"
#include "ProcessImportPds.h"
#include "ProcessByLine.h"
#include "UserInterface.h"
#include "FileName.h"
#include "IException.h"
#include "Preference.h"
#include "IString.h"
#include "Pvl.h"
#include "Table.h"
#include "Stretch.h"
#include "TextFile.h"
#include "PixelType.h"

using namespace std;
using namespace Isis;

// Global variables for processing functions
Stretch stretch;

void FixDns8(Buffer &buf);
void TranslateMroCtxLabels(FileName &labelFile, Cube *ocube);
void SaveDarkData(ProcessImportPds &process, Cube *ocube, int startPix, int endPix);
vector<int> ConvertDarkPixels(int samples, Isis::PixelType pixelType,
                              unsigned char *data);
bool fillGap;

void IsisMain() {
  ProcessImportPds p;

  //Check that the file comes from the right camera
  UserInterface &ui = Application::GetUserInterface();
  FileName inFile = ui.GetFileName("FROM");
  fillGap = false;
  IString id, bitMode;
  int sumMode, editMode;
  bool projected;

  {
    std::string msg;
    try {
      msg = "File could not be opened.";
      Pvl lab(inFile.expanded());

      msg = "PVL Keyword [DATA_SET_ID] not found in label.";
      id = QString::fromStdString(lab.findKeyword("DATA_SET_ID"));
      projected = lab.hasObject("IMAGE_MAP_PROJECTION");

      msg = "PVL Keywords [SPATIAL_SUMMING] and [SAMPLING_FACTOR] not found in label.";
      msg += "The mroctx2isis application requires at least one to exist in order to set summing mode.";
      if(lab.hasKeyword("SPATIAL_SUMMING")) {
        sumMode = (int)lab.findKeyword("SPATIAL_SUMMING");
      }
      else {
        sumMode = (int)lab.findKeyword("SAMPLING_FACTOR");
      }

      msg = "PVL Keyword [SAMPLE_BIT_MODE_ID] not found in label.";
      bitMode = QString::fromStdString(lab.findKeyword("SAMPLE_BIT_MODE_ID"));

      msg = "PVL Keywords [EDIT_MODE_ID] and [SAMPLE_FIRST_PIXEL] not found in label.";
      msg += "The mroctx2isis application requires at least one to exist in order to set edit mode.";
      if(lab.hasKeyword("EDIT_MODE_ID")) {
        editMode = (int)lab.findKeyword("EDIT_MODE_ID");
      }
      else {
        editMode = (int)lab.findKeyword("SAMPLE_FIRST_PIXEL");
      }
    }
    catch(IException &e) {
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
  }




  //Checks if in file is rdr
  if(projected) {
    std::string msg = "[" + inFile.name() + "] appears to be an rdr file.";
    msg += " Use pds2isis.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  id.ConvertWhiteSpace();
  id.Compress();
  id.Trim(" ");
  if(id != "MRO-M-CTX-2-EDR-L0-V1.0") {
    std::string msg = "Input file [" + inFile.expanded() + "] does not appear to be " +
                  "in MRO-CTX EDR format. DATA_SET_ID is [" + id + "]";
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }

  // Check to make sure the SampleBitModeId is SQROOT
  if(bitMode != "SQROOT") {
    string msg = "Can't handle Sample Bit Mode [" + bitMode + "]";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  //Process the file
  Pvl pdsLab;
  p.SetPdsFile(inFile.expanded(), "", pdsLab);

  int startPix = 0;
  int endPix = 0;
  int suf = 0;

  // Set the data prefix and suffix values
  if(ui.WasEntered("PREFIX")) {
    endPix = ui.GetInteger("PREFIX");
  }
  else {
    if(sumMode == 1) {
      if(editMode == 0) {
        startPix = 14;
        endPix = 37;
      }
      else {
        startPix = 0;
        endPix = 15;
      }
    }
    else if(sumMode == 2) {
      if(editMode == 0) {
        startPix = 7;
        endPix = 18;
      }
      else {
        startPix = 0;
        endPix = 7;
      }
    }
  }
  if(ui.WasEntered("SUFFIX")) {
    suf = ui.GetInteger("SUFFIX");
  }
  else {
    if(sumMode == 1) {
      if(editMode == 0) {
        suf = 18;
      }
      else {
        suf = 0;
      }
    }
    else if(sumMode == 2) {
      if(editMode == 0) {
        suf = 9;
      }
      else {
        suf = 0;
      }
    }
    ui.PutInteger("SUFFIX", suf);
  }
  p.SetDataPrefixBytes(endPix + 1);
  p.SetDataSuffixBytes(suf);
  int samps = p.Samples() - endPix - suf - 1;
  p.SetDimensions(samps, p.Lines(), p.Bands());

  // Save off the dark pixel data
  p.SaveDataPrefix();

  // Set the output bit type to Real
  CubeAttributeOutput &outAtt = ui.GetOutputAttribute("TO");
  outAtt.setPixelType(Isis::SignedWord);
  outAtt.setMinimum((double)VALID_MIN2);
  outAtt.setMaximum((double)VALID_MAX2);
  Cube *ocube = p.SetOutputCube(ui.GetCubeName("TO"), outAtt);

  // Translate the labels
  p.StartProcess();
  TranslateMroCtxLabels(inFile, ocube);

  // Set up the strech for the 8 to 12 bit conversion from file
  FileName *temp = new FileName("$mro/calibration/ctxsqroot_???.lut");
  *temp = temp->highestVersion();
  TextFile *stretchPairs = new TextFile(temp->expanded());

  // Create the stretch pairs
  stretch.ClearPairs();
  for(int i = 0; i < stretchPairs->LineCount(); i++) {
    QString line;
    stretchPairs->GetLine(line, true);
    int temp1 = toInt(line.split(" ").first());
    int temp2 = toInt(line.split(" ").last());
    stretch.AddPair(temp1, temp2);
  }

  stretchPairs->Close();

  SaveDarkData(p, ocube, startPix, endPix);
  p.EndProcess();

  // Do 8 bit to 12 bit conversion
  fillGap = ui.GetBoolean("FILLGAP");
  ProcessByLine p2;
  QString ioFile = ui.GetCubeName("TO");
  CubeAttributeInput att;
  p2.SetInputCube(ioFile, att, ReadWrite);
  p2.Progress()->SetText("Converting 8 bit pixels to 16 bit");
  p2.StartProcess(FixDns8);
  p2.EndProcess();
}

// The input buffer has a raw 16 bit buffer but the values are still 0 to 255
void FixDns8(Buffer &buf) {
  for(int i = 0; i < buf.size(); i++) {
    if(!fillGap || buf[i] != 0) {
      buf[i] = stretch.Map(buf[i]);
    }
    else {
      buf[i] = Isis::Null;
    }
  }
}

//Function to translate the labels
void TranslateMroCtxLabels(FileName &labelFile, Cube *ocube) {

  //Pvl to store the labels
  Pvl outLabel;
  QString transDir = "$ISISROOT/appdata/translations/";
  Pvl labelPvl(labelFile.expanded());

  //Translate the Instrument group
  FileName transFile(transDir + "MroCtxInstrument.trn");
  PvlToPvlTranslationManager instrumentXlator(labelPvl, transFile.expanded());
  instrumentXlator.Auto(outLabel);

  //Translate the Archive grooup
  transFile  = transDir + "MroCtxArchive.trn";
  PvlToPvlTranslationManager archiveXlater(labelPvl, transFile.expanded());
  archiveXlater.Auto(outLabel);

  // Set up the BandBin groups
  PvlGroup bbin("BandBin");
  bbin += PvlKeyword("FilterName", "BroadBand");
  bbin += PvlKeyword("Center", std::to_string(0.650), "micrometers");
  bbin += PvlKeyword("Width", std::to_string(0.150), "micrometers");

  //Set up the Kernels group
  PvlGroup kern("Kernels");
  kern += PvlKeyword("NaifFrameCode", std::to_string(-74021));

  Pvl lab(labelFile.expanded());
  int sumMode, startSamp;
  if(lab.hasKeyword("SPATIAL_SUMMING")) {
    sumMode = (int)lab.findKeyword("SPATIAL_SUMMING");
  }
  else {
    sumMode = (int)lab.findKeyword("SAMPLING_FACTOR");
  }
  if(lab.hasKeyword("EDIT_MODE_ID")) {
    startSamp = (int)lab.findKeyword("EDIT_MODE_ID");
  }
  else {
    startSamp = (int)lab.findKeyword("SAMPLE_FIRST_PIXEL");
  }
  PvlGroup inst = outLabel.findGroup("Instrument", Pvl::Traverse);
  inst += PvlKeyword("SpatialSumming", std::to_string(sumMode));
  inst += PvlKeyword("SampleFirstPixel", std::to_string(startSamp));

  //Add all groups to the output cube
  ocube->putGroup(inst);
  ocube->putGroup(outLabel.findGroup("Archive", Pvl::Traverse));
  ocube->putGroup(bbin);
  ocube->putGroup(kern);
}

void SaveDarkData(ProcessImportPds &process, Cube *ocube, int startPix, int endPix) {
  int pixNum = endPix - startPix + 1;
  TableField dark("DarkPixels", TableField::Integer, pixNum);
  TableRecord darkRecord;
  darkRecord += dark;
  Table darkTable("Ctx Prefix Dark Pixels", darkRecord);
  vector<vector<char *> > pre = process.DataPrefix();
  vector<char *> prefix = pre.at(0);

  for(int l = 0; l < (int)prefix.size(); l++) {
    unsigned char *linePrefix = (unsigned char *)(prefix[l] + startPix * SizeOf(process.PixelType()));
    darkRecord[0] = ConvertDarkPixels(pixNum, process.PixelType(), linePrefix);
    darkTable += darkRecord;
  }
  ocube->write(darkTable);
}

vector<int> ConvertDarkPixels(int samples, Isis::PixelType pixelType,
                              unsigned char *data) {
  Isis::Buffer pixelBuf(samples, 1, 1, Isis::SignedWord);

  for(int b = 0; b < samples; b++) {
    short int pixel = data[b];
    ((short int *)pixelBuf.RawBuffer())[b] = pixel;
    pixelBuf[b] = pixel;
  }
  FixDns8(pixelBuf);

  // Move the calibration pixels from the buffer to a vector
  vector<int> calibrationPixels;
  double pixel;
  for(int b = 0; b < samples; b++) {
    pixel = pixelBuf[b];
    if(pixel == NULL8) calibrationPixels.push_back(NULL2);
    else if(pixel == LOW_REPR_SAT8) calibrationPixels.push_back(LOW_REPR_SAT2);
    else if(pixel == LOW_INSTR_SAT8) calibrationPixels.push_back(LOW_INSTR_SAT2);
    else if(pixel == HIGH_INSTR_SAT8) calibrationPixels.push_back(HIGH_INSTR_SAT2);
    else if(pixel == HIGH_REPR_SAT8) calibrationPixels.push_back(HIGH_REPR_SAT2);
    else calibrationPixels.push_back((int)(pixel + 0.5));
  }

  return calibrationPixels;
}
