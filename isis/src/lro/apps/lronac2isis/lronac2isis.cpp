/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ProcessImportPds.h"
#include "UserInterface.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Pvl.h"
#include "OriginalLabel.h"
#include "History.h"
#include "LineManager.h"
#include "Application.h"

#include <vector>

#include "lronac2isis.h"

#define MAX_INPUT_VALUE 4095

using namespace std;

namespace Isis {

  static void ResetGlobals();
  static void Import(Buffer &buf);
  static void TranslateLrocNacLabels(FileName &labelFile, Cube *ocube);

  // Global variables for processing functions
  Cube *g_ocube;
  std::vector< double> g_xterm, g_bterm, g_mterm;
  bool g_flip = false;

  void lronac2isis(UserInterface &ui) {
    // Initialize variables
    ResetGlobals();

    // Check that the file comes from the right camera
    FileName inFile = ui.GetFileName("FROM").toStdString();
    QString id;
    try {
      Pvl lab(inFile.expanded());
      if(lab.hasKeyword("DATA_SET_ID"))
        id = QString::fromStdString(lab.findKeyword("DATA_SET_ID"));
      else {
        std::string msg = "Unable to read [DATA_SET_ID] from input file [" + inFile.expanded() + "]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
      // Checks if in file is RDR
      bool projected = lab.hasObject("IMAGE_MAP_PROJECTION");
      if(projected) {
        std::string msg = "[" + inFile.name() + "] appears to be an RDR file.";
        msg += " Use pds2isis.";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      // Store the decompanding information
      PvlKeyword xtermKeyword = lab.findKeyword("LRO:XTERM"),
                 mtermKeyword = lab.findKeyword("LRO:MTERM"),
                 btermKeyword = lab.findKeyword("LRO:BTERM");

      if(mtermKeyword.size() != xtermKeyword.size() || btermKeyword.size() != xtermKeyword.size()) {
        std::string msg = "The decompanding terms do not have the same dimensions";
        throw IException(IException::Io, msg, _FILEINFO_);
      }

      for(int i = 0; i < xtermKeyword.size(); i++) {
        g_xterm.push_back(Isis::toDouble(xtermKeyword[i]));
        g_mterm.push_back(Isis::toDouble(mtermKeyword[i]));
        g_bterm.push_back(Isis::toDouble(btermKeyword[i]));
      }

      double versionId = QString::fromStdString(lab.findKeyword("PRODUCT_VERSION_ID")[0]).remove(QRegExp("^v")).toDouble();
      if(lab.findKeyword("FRAME_ID")[0] == "RIGHT" && versionId < 1.30)
        g_flip = true;
      else
        g_flip = false;
    }
    catch(IException &e) {
      std::string msg = "The PDS header is missing important keyword(s).";
      IException finalException(IException::Io, msg, _FILEINFO_);
      finalException.append(e);
      throw finalException;
    }

    id = id.simplified().trimmed();
    if(id.mid(13, 3) != "EDR") {
      std::string msg = "Input file [" + inFile.expanded() + "] does not appear to be "
                   + "in LROC-NAC EDR format. DATA_SET_ID is [" + id.toStdString() + "]"
                   + " Use pds2isis for RDR or CDR.";
      throw IException(IException::Io, msg, _FILEINFO_);
    }

    //Process the file
    Pvl pdsLab;
    ProcessImportPds p;
    p.SetPdsFile(QString::fromStdString(inFile.expanded()), "", pdsLab);
    // Set the output bit type to Real
    CubeAttributeOutput &outAtt = ui.GetOutputAttribute("TO");

    g_ocube = new Cube();
    g_ocube->setByteOrder(outAtt.byteOrder());
    g_ocube->setFormat(outAtt.fileFormat());
    g_ocube->setMinMax((double) VALID_MIN2, (double) VALID_MAX2);
    g_ocube->setLabelsAttached(outAtt.labelAttachment() == AttachedLabel);
    g_ocube->setDimensions(p.Samples(), p.Lines(), p.Bands());
    g_ocube->setPixelType(Isis::Real);
    g_ocube->create(ui.GetCubeName("TO"));
    // Do 8 bit to 12 bit conversion
    // And if NAC-R, flip the frame
    p.StartProcess(Import);

    // Then translate the labels
    TranslateLrocNacLabels(inFile, g_ocube);
    p.EndProcess();

    // Add History
    if (iApp) {
        History history = g_ocube->readHistory();
        history.AddEntry();
        g_ocube->write(history);
    }

    // Add original label
    OriginalLabel origLabel(pdsLab);
    g_ocube->write(origLabel);
    g_ocube->close();
    delete g_ocube;
  }

  // The input buffer has a raw 16 bit buffer but the values are still 0 to 255.
  // See "Appendix B - NAC and WAC Companding Schemes" of the LROC_SOC_SPEC
  // document for reference.
  void Import(Buffer &in) {
    LineManager outLines(*g_ocube);
    outLines.SetLine(in.Line(), in.Band());
    Buffer buf(in.SampleDimension(), in.LineDimension(), in.BandDimension(),
               g_ocube->pixelType());

    // Do the decompanding
    for(int pixin = 0; pixin < in.size(); pixin++) {

      // if pixin < xtermo0, then it is in "segment 0"
      if(in[pixin] < g_xterm[0])
        buf[pixin] = (int) in[pixin];

      // otherwise, it is in segments 1 to 5
      else {
        unsigned int segment = 1;
        while(segment < g_xterm.size() && (in[pixin] - g_bterm[segment - 1]) / g_mterm[segment - 1] >= g_xterm[segment])
          segment++;

        // Compute the upper and lower bin values
        double upper = (in[pixin] + 1 - g_bterm[segment - 1]) / g_mterm[segment - 1] - 1;
        double lower = (in[pixin] - g_bterm[segment - 1]) / g_mterm[segment - 1];

        // Check if the bin is on the upper boundary of the last segment
        if(upper > MAX_INPUT_VALUE)
          upper = MAX_INPUT_VALUE;
        else if(segment < g_xterm.size() && upper >= g_xterm[segment]) {
          if((int)(g_bterm[segment] + g_mterm[segment]*upper) != in[pixin])
            upper = g_xterm[segment] - 1;
        }

        // Check if it is on the lower boundary of a segment
        if(lower < g_xterm[segment-1])
          lower = g_xterm[segment-1];

        // Output the middle bin value
        buf[pixin] = (upper + lower) / 2.0;

      }
    }

    // flip the NAC-R frame
    if(g_flip) {
      Buffer tmpbuf(buf);
      for(int i = 0; i < buf.size(); i++)
        buf[i] = tmpbuf[buf.size() - i - 1];
    }
    outLines.Copy(buf);
    g_ocube->write(outLines);
  }

  //Function to translate the labels
  void TranslateLrocNacLabels(FileName &labelFile, Cube *ocube) {

    //Pvl to store the labels
    Pvl outLabel;
    //Set up the directory where the translations are
    Pvl labelPvl(labelFile.expanded());

    //Translate the Instrument group
    FileName transFile("$ISISROOT/appdata/translations/LroNacInstrument.trn");
    PvlToPvlTranslationManager instrumentXlator(labelPvl, QString::fromStdString(transFile.expanded()));
    instrumentXlator.Auto(outLabel);

    //Translate the Archive group
    transFile = "$ISISROOT/appdata/translations/LroNacArchive.trn";
    PvlToPvlTranslationManager archiveXlater(labelPvl, QString::fromStdString(transFile.expanded()));
    archiveXlater.Auto(outLabel);

    //Translate the BandBin group
    transFile = "$ISISROOT/appdata/translations/LroNacBandBin.trn";
    PvlToPvlTranslationManager bandBinXlater(labelPvl, QString::fromStdString(transFile.expanded()));
    bandBinXlater.Auto(outLabel);

    Pvl lab(labelFile.expanded());

    //Set up the Kernels group
    PvlGroup kern("Kernels");
    if(lab.findKeyword("FRAME_ID")[0] == "LEFT")
      kern += PvlKeyword("NaifFrameCode", "-85600");
    else
      kern += PvlKeyword("NaifFrameCode", "-85610");

    PvlGroup inst = outLabel.findGroup("Instrument", Pvl::Traverse);
    if(lab.findKeyword("FRAME_ID")[0] == "LEFT") {
      inst.findKeyword("InstrumentId") = "NACL";
      inst.findKeyword("InstrumentName") = "LUNAR RECONNAISSANCE ORBITER NARROW ANGLE CAMERA LEFT";
    }
    else {
      inst.findKeyword("InstrumentId") = "NACR";
      inst.findKeyword("InstrumentName") = "LUNAR RECONNAISSANCE ORBITER NARROW ANGLE CAMERA RIGHT";
    }

    //Add all groups to the output cube
    ocube->putGroup(inst);
    ocube->putGroup(outLabel.findGroup("Archive", Pvl::Traverse));
    ocube->putGroup(outLabel.findGroup("BandBin", Pvl::Traverse));
    ocube->putGroup(kern);
  }

  void ResetGlobals() {
    g_ocube = NULL;
    g_xterm.clear();
    g_mterm.clear();
    g_bterm.clear();
    g_flip = false;
  }
}
