/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <algorithm>
#include <stdio.h>

#include <QFile>

#include "Brick.h"
#include "EndianSwapper.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "OriginalLabel.h"
#include "ProcessByLine.h"
#include "ProcessImportPds.h"
#include "Pvl.h"
#include "Table.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;

typedef struct {
  int mi32OrigBandStart;
  int mi32OrigBinEnd;
  int mi32BandCenterStart;
  int mi32BandCenterEnd;
  int mi32NaifFrameCode;
} VIMS;

enum VimsType { VIS, IR };

void ReadVimsBIL(QString inFile, const PvlKeyword &suffixItems, QString outFile);
void TranslateVimsLabels(Pvl &pdsLab, Cube *vimscube, VimsType vType);
void ProcessCube(Buffer &in, Buffer &out);
void ProcessBands(Pvl &pdsLab, Cube *vimscube, VimsType vtype);

//***********************************************************************
//   IsisMain()
//***********************************************************************
void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  FileName in = ui.GetFileName("FROM");
  FileName outIr = ui.GetCubeName("IR");
  FileName outVis = ui.GetCubeName("VIS");
  Pvl lab(in.expanded());

  //Checks if in file is rdr
  if(lab.hasObject("IMAGE_MAP_PROJECTION")) {
    QString msg = "[" + in.name() + "] appears to be an rdr file.";
    msg += " Use pds2isis.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  //Make sure it is a vims cube
  try {
    PvlObject qube(lab.findObject("QUBE"));
    QString id;
    id = (QString)qube["INSTRUMENT_ID"];
    id = id.simplified().trimmed();
    if(id != "VIMS") {
      QString msg = "Invalid INSTRUMENT_ID [" + id + "]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
  }
  catch(IException &e) {
    QString msg = "Input file [" + in.expanded() +
                 "] does not appear to be " +
                 "in VIMS EDR/RDR format";
    throw IException(IException::Io, msg, _FILEINFO_);
  }

  FileName tempname(in.baseName() + ".bsq.cub");
  Pvl pdsLab(in.expanded());

  // It's VIMS, let's figure out if it has the suffix data or not
  if(toInt(lab.findObject("QUBE")["SUFFIX_ITEMS"][0]) == 0) {
    // No suffix data, we can use processimportpds
    ProcessImportPds p;

    p.SetPdsFile(in.expanded(), "", pdsLab);
    // Set up the temporary output cube
    //The temporary cube is set to Real pixeltype, regardless of input pixel type
    Isis::CubeAttributeOutput outatt = CubeAttributeOutput("+Real");
    p.SetOutputCube(tempname.name(), outatt);
    p.StartProcess();
    p.EndProcess();
  }
  else {
    // We do it the hard way
    ReadVimsBIL(in.expanded(), lab.findObject("QUBE")["SUFFIX_ITEMS"], tempname.name());
  }

  // Create holder for original labels
  OriginalLabel origLabel(pdsLab);

  //Now separate the cubes
  ProcessByLine l;

  PvlGroup status("Results");

  //VIS cube
  const PvlObject &qube = lab.findObject("Qube");
  if(qube["SAMPLING_MODE_ID"][1] != "N/A") {
    CubeAttributeInput inattvis = CubeAttributeInput("+1-96");
    l.SetInputCube(tempname.name(), inattvis);
    Cube *oviscube = l.SetOutputCube("VIS");
    oviscube->write(origLabel);
    l.StartProcess(ProcessCube);
    TranslateVimsLabels(pdsLab, oviscube, VIS);
    l.EndProcess();

    status += PvlKeyword("VisCreated", "true");
  }
  else {
    status += PvlKeyword("VisCreated", "false");
  }

  //IR cube
  if(qube["SAMPLING_MODE_ID"][0] != "N/A") {
    CubeAttributeInput inattir = CubeAttributeInput("+97-352");
    l.SetInputCube(tempname.name(), inattir);
    Cube *oircube = l.SetOutputCube("IR");
    oircube->write(origLabel);
    l.StartProcess(ProcessCube);
    TranslateVimsLabels(pdsLab, oircube, IR);
    l.EndProcess();

    status += PvlKeyword("IrCreated", "true");
  }
  else {
    status += PvlKeyword("IrCreated", "false");
  }

  Application::Log(status);

  //Clean up
  QString tmp(tempname.expanded());
  QFile::remove(tmp);
}

/**
 *   We created a method to manually skip the suffix and corner
 *   data for this image to avoid implementing it in
 *   ProcessImport and ProcessImportPds. To fully support this
 *   file format, we would have to re-implement the ISIS2 Cube
 *   IO plus add prefix data features to it. This is a shortcut;
 *   because we know these files have one sideplane and four
 *   backplanes, we know how much data to skip when. This should
 *   be fixed if we ever decide to fully support suffix and
 *   corner data, which would require extensive changes to
 *   ProcessImport/ProcessImportPds. Method written by Steven
 *   Lambright.
 *
 * @param inFileName FileName of the input file
 * @param outFile FileName of the output file
 */
void ReadVimsBIL(QString inFileName, const PvlKeyword &suffixItems, QString outFile) {
  Pvl pdsLabel(inFileName);
  Isis::FileName transFile("$ISISROOT/appdata/translations/pdsQube.trn");
  Isis::PvlToPvlTranslationManager pdsXlater(pdsLabel, transFile.expanded());


  TableField sideplaneLine("Line", Isis::TableField::Integer);
  TableField sideplaneBand("Band", Isis::TableField::Integer);
  TableField sideplaneValue("Value", Isis::TableField::Integer);

  TableRecord record;
  record += sideplaneLine;
  record += sideplaneBand;
  record += sideplaneValue;
  Table sideplaneVisTable("SideplaneVis", record);
  Table sideplaneIrTable("SideplaneIr", record);

  sideplaneVisTable.SetAssociation(Table::Lines);
  sideplaneIrTable.SetAssociation(Table::Lines);

  QString str;
  str = pdsXlater.Translate("CoreBitsPerPixel");
  int bitsPerPixel = toInt(str);
  str = pdsXlater.Translate("CorePixelType");
  PixelType pixelType = Isis::Real;

  if((str == "Real") && (bitsPerPixel == 32)) {
    pixelType = Isis::Real;
  }
  else if((str == "Integer") && (bitsPerPixel == 8)) {
    pixelType = Isis::UnsignedByte;
  }
  else if((str == "Integer") && (bitsPerPixel == 16)) {
    pixelType = Isis::SignedWord;
  }
  else if((str == "Integer") && (bitsPerPixel == 32)) {
    pixelType = Isis::SignedInteger;
  }
  else if((str == "Natural") && (bitsPerPixel == 8)) {
    pixelType = Isis::UnsignedByte;
  }
  else if((str == "Natural") && (bitsPerPixel == 16)) {
    pixelType = Isis::UnsignedWord;
  }
  else if((str == "Natural") && (bitsPerPixel == 16)) {
    pixelType = Isis::SignedWord;
  }
  else if((str == "Natural") && (bitsPerPixel == 32)) {
    pixelType = Isis::UnsignedInteger;
  }
  else {
    QString msg = "Invalid PixelType and BitsPerPixel combination [" + str +
                 ", " + toString(bitsPerPixel) + "]";
    throw IException(IException::Io, msg, _FILEINFO_);
  }

  str = pdsXlater.Translate("CoreByteOrder");
  Isis::ByteOrder byteOrder = Isis::ByteOrderEnumeration(str);

  str = pdsXlater.Translate("CoreSamples", 0);
  int ns = toInt(str);
  str = pdsXlater.Translate("CoreLines", 2);
  int nl = toInt(str);
  str = pdsXlater.Translate("CoreBands", 1);
  int nb = toInt(str);

  std::vector<double> baseList;
  std::vector<double> multList;

  str = pdsXlater.Translate("CoreBase");
  baseList.clear();
  baseList.push_back(toDouble(str));
  str = pdsXlater.Translate("CoreMultiplier");
  multList.clear();
  multList.push_back(toDouble(str));

  Cube outCube;
  outCube.setPixelType(Isis::Real);
  outCube.setDimensions(ns, nl, nb);
  outCube.create(outFile);

  // Figure out the number of bytes to read for a single line
  int readBytes = Isis::SizeOf(pixelType);
  readBytes = readBytes * ns;
  char *in = new char [readBytes];

  // Set up an Isis::EndianSwapper object
  QString tok(Isis::ByteOrderName(byteOrder));
  tok = tok.toUpper();
  Isis::EndianSwapper swapper(tok);

  ifstream fin;
  // Open input file
  Isis::FileName inFile(inFileName);
  fin.open(inFileName.toLatin1().data(), ios::in | ios::binary);
  if(!fin.is_open()) {
    QString msg = "Cannot open input file [" + inFileName + "]";
    throw IException(IException::Io, msg, _FILEINFO_);
  }

  // Handle the file header
  streampos pos = fin.tellg();
  int fileHeaderBytes = (int)(IString)pdsXlater.Translate("DataFileRecordBytes") *
                        ((int)(IString)pdsXlater.Translate("DataStart", 0) - 1);

  fin.seekg(fileHeaderBytes, ios_base::beg);

  // Check the last io
  if(!fin.good()) {
    QString msg = "Cannot read file [" + inFileName + "]. Position [" +
                 toString((int)pos) + "]. Byte count [" +
                 toString(fileHeaderBytes) + "]" ;
    throw IException(IException::Io, msg, _FILEINFO_);
  }

  // Construct a line buffer manager
  Isis::Brick out(ns, 1, 1, Isis::Real);

  // Loop for each line
  for(int line = 0; line < nl; line++) {
    // Loop for each band
    for(int band = 0; band < nb; band++) {
      // Set the base multiplier
      double base, mult;
      if(baseList.size() > 1) {
        base = baseList[band];
        mult = multList[band];
      }
      else {
        base = baseList[0];
        mult = multList[0];
      }

      // Get a line of data from the input file
      pos = fin.tellg();
      fin.read(in, readBytes);

      if(!fin.good()) {
        QString msg = "Cannot read file [" + inFileName + "]. Position [" +
                     toString((int)pos) + "]. Byte count [" +
                     toString(readBytes) + "]" ;
        throw IException(IException::Io, msg, _FILEINFO_);
      }

      // Swap the bytes if necessary and convert any out of bounds pixels
      // to special pixels
      for(int samp = 0; samp < ns; samp++) {
        switch(pixelType) {
          case Isis::UnsignedByte:
            out[samp] = (double)((unsigned char *)in)[samp];
            break;
          case Isis::UnsignedWord:
            out[samp] = (double)swapper.UnsignedShortInt((unsigned short int *)in + samp);
            break;
          case Isis::SignedWord:
            out[samp] = (double)swapper.ShortInt((short int *)in + samp);
            break;
          case Isis::Real:
            out[samp] = (double)swapper.Float((float *)in + samp);
            break;
          default:
            break;
        }

        // Sets out to isis special pixel or leaves it if valid
        out[samp] = TestPixel(out[samp]);

        if(Isis::IsValidPixel(out[samp])) {
          out[samp] = mult * out[samp] + base;
        }
      } // End sample loop

      //Set the buffer position and write the line to the output file
      out.SetBasePosition(1, line + 1, band + 1);
      outCube.write(out);

      if(toInt(suffixItems[0]) != 0) {
        pos = fin.tellg();
        char *sideplaneData = new char[4*toInt(suffixItems[0])];
        fin.read(sideplaneData, 4 * toInt(suffixItems[0]));
        int suffixData = (int)swapper.Int((int *)sideplaneData);
        record[0] = line + 1;
        record[1] = band + 1;
        record[2] = suffixData;

        if(band < 96) {
          sideplaneVisTable += record;

          // set HIS pixels appropriately
          for(int samp = 0; samp < ns; samp++) {
            if(out[samp] >= 4095) {
              out[samp] = Isis::His;
            }
          }
        }
        else {
          record[1] = (band + 1) - 96;
          sideplaneIrTable += record;

          // set HIS pixels appropriately
          for(int samp = 0; samp < ns; samp++) {
            if(out[samp] + suffixData >= 4095) {
              out[samp] = Isis::His;
            }
          }
        }

        delete [] sideplaneData;

        // Check the last io
        if(!fin.good()) {
          QString msg = "Cannot read file [" + inFileName + "]. Position [" +
                       toString((int)pos) + "]. Byte count [" +
                       toString(4) + "]" ;
          throw IException(IException::Io, msg, _FILEINFO_);
        }
      }
    } // End band loop

    int backplaneSize = toInt(suffixItems[1]) * (4 * (ns + toInt(suffixItems[0])));
    fin.seekg(backplaneSize, ios_base::cur);

    // Check the last io
    if(!fin.good()) {
      QString msg = "Cannot read file [" + inFileName + "]. Position [" +
                   toString((int)pos) + "]. Byte count [" +
                   toString(4 * (4 * ns + 4)) + "]" ;
      throw IException(IException::Io, msg, _FILEINFO_);
    }

  } // End line loop

  outCube.write(sideplaneVisTable);
  outCube.write(sideplaneIrTable);

  // Close the file and clean up
  fin.close();
  outCube.close();
  delete [] in;
}

//************************************************************
//  Name:        ProcessCube
//
//  Description: Function to copy cube from input to ouput
//
//  Parameters:  Buffer &in  - Input Cube
//               Buffer &out - Output Cube
//
//  Return:      None
//************************************************************
void ProcessCube(Buffer &in, Buffer &out) {
  for(int i = 0; i < in.size(); i++) {
    out[i] = in[i];
  }
}

//************************************************************
//  Name:        ProcessBands
//
//  Description: Function to process bands for both IR and VIS
//
//  Parameters:  Pvl & pdsLab - Label
//               Cube *vimsCube
//               VimsType vtype - whether VIS/IS cubes
//
//  Return:      None
//************************************************************
void ProcessBands(Pvl &pdsLab, Cube *vimsCube, VimsType vtype) {
  VIMS vims;
  //input band specific information
  if(vtype == VIS) {
    vims.mi32OrigBandStart   = 1;
    vims.mi32OrigBinEnd      = vimsCube->bandCount();
    vims.mi32BandCenterStart = 0;
    vims.mi32BandCenterEnd   = 96;
    vims.mi32NaifFrameCode   = -82370;
  }
  else if(vtype == IR) {
    vims.mi32OrigBandStart   = 97;
    vims.mi32OrigBinEnd      = 352;
    vims.mi32BandCenterStart = 96;
    vims.mi32BandCenterEnd   = 352;
    vims.mi32NaifFrameCode   = -82371;
  }
  PvlObject qube(pdsLab.findObject("Qube"));

//Create the BandBin Group
  PvlGroup bandbin("BandBin");
  PvlKeyword originalBand("OriginalBand");
  for(int i = vims.mi32OrigBandStart; i <= vims.mi32OrigBinEnd; i++) {
    originalBand.addValue(std::to_string(i));
  }
  bandbin += originalBand;
  PvlKeyword center("Center");
  PvlGroup bbin(qube.findGroup("BandBin"));
  for(int i = vims.mi32BandCenterStart; i < vims.mi32BandCenterEnd; i++) {
    center += (QString) bbin["BandBinCenter"][i];
  }
  bandbin += center;

  vimsCube->putGroup(bandbin);

  //Create the Kernels Group
  PvlGroup kern("Kernels");
  kern += PvlKeyword("NaifFrameCode", toString(vims.mi32NaifFrameCode));
  vimsCube->putGroup(kern);
}

//************************************************************
//  Name:        TranslateVimsLabels
//
//  Description: Function to translate Vims label for both  IR
//               & VIS cubes
//
//  Parameters:  Pvl & pdsLab - Label
//               Cube *vimsCube - output VIS/IR cubes
//               VimsType vType - VIS / IR
//
//  History:     2009-10-20 Tracie Sucharski, Corrected indices for
//                             SAMPLING_MODE_ID and GAIN_MODE_ID.
//
//  Return:      None
//************************************************************
void TranslateVimsLabels(Pvl &pdsLab, Cube *vimscube, VimsType vType) {
  Isis::FileName transFile("$ISISROOT/appdata/translations/CassiniVimsPds.trn");
  PvlObject qube(pdsLab.findObject("Qube"));
  Pvl pdsLabel(pdsLab);
  Isis::PvlToPvlTranslationManager labelXlater(pdsLabel, transFile.expanded());

  Pvl outputLabel;
  labelXlater.Auto(outputLabel);

  //Add needed keywords that are not in translation table to cube's instrument group
  PvlGroup &inst = outputLabel.findGroup("Instrument", Pvl::Traverse);

  //trim start and stop time
  QString strTime = inst.findKeyword("StartTime")[0];
  inst.findKeyword("StartTime").setValue(strTime.remove("Z"));
  strTime = QString::fromStdString(qube["StopTime"]);
  inst.findKeyword("StopTime").setValue((strTime).remove("Z").toStdString());

  if(vType == IR) {
    inst += PvlKeyword("SamplingMode", qube["SamplingModeId"][0]);
  }
  else {
    inst += PvlKeyword("SamplingMode", qube["SamplingModeId"][1]);
  }
  if(vType == VIS) {
    inst += PvlKeyword("Channel", "VIS");
  }
  else {
    inst += PvlKeyword("Channel", "IR");
  }

  PvlKeyword expDuration("ExposureDuration");
  expDuration.addValue(qube["ExposureDuration"][0], "IR");
  expDuration.addValue(qube["ExposureDuration"][1], "VIS");
  inst += expDuration;

  if(vType == IR) {
    inst += PvlKeyword("GainMode", qube["GainModeId"][0]);
  }
  else {
    inst += PvlKeyword("GainMode", qube["GainModeId"][1]);
  }

  vimscube->putGroup(inst);

  //Get Archive
  PvlGroup &archive = outputLabel.findGroup("Archive", Pvl::Traverse);
  vimscube->putGroup(archive);

  ProcessBands(pdsLab, vimscube, vType);
}
