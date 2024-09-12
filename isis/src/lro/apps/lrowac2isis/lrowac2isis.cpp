/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "lrowac2isis.h"

#include <cstdio>
#include <QString>

#include <QRegExp>

#include "ProcessImportPds.h"
#include "LineManager.h"
#include "OriginalLabel.h"
#include "Brick.h"
#include "History.h"
#include "Stretch.h"
#include "Application.h"

using namespace std;

namespace Isis {

  static vector<Cube *> outputCubes;
  static vector<int> frameletLines;
  static void separateFramelets(Buffer &in);
  static void writeNullsToFile();
  static void TranslateLabels(Pvl &pdsLab, Pvl &isis3VisEven, Pvl &isis3VisOdd,
                       Pvl &isis3UvEven, Pvl &isis3UvOdd, UserInterface &ui);
  static void ValidateInputLabels(Pvl &pdsLab);

  static std::vector<int> padding;
  static int colorOffset = 0;
  static int inputCubeLines = 0;

  // Output UV Files
  static Cube *uveven = NULL;
  static Cube *uvodd = NULL;

  // Output VIS Files
  static Cube *viseven = NULL;
  static Cube *visodd = NULL;

  static Stretch lookupTable;

  static bool flip = false;

  void lrowac2isis(UserInterface &ui) {
    colorOffset = 0;
    frameletLines.clear();
    outputCubes.clear();

    uveven = NULL;
    uvodd = NULL;
    viseven = NULL;
    visodd = NULL;

    ProcessImportPds p;
    Pvl pdsLab;

    QString fromFile = ui.GetFileName("FROM");

    flip = false;//ui.GetBoolean("FLIP");

    p.SetPdsFile(fromFile, "", pdsLab);
    ValidateInputLabels(pdsLab);
    inputCubeLines = p.Lines();

    lookupTable = Stretch();

    // read the lut if the option is on
    if(ui.GetBoolean("UNLUT") && pdsLab["LRO:LOOKUP_TABLE_TYPE"][0] == "STORED") {
      PvlKeyword lutKeyword = pdsLab["LRO:LOOKUP_CONVERSION_TABLE"];

      for(int i = 0; i < lutKeyword.size(); i ++) {
        IString lutPair = lutKeyword[i];
        lutPair.ConvertWhiteSpace();
        lutPair.Remove("() ");
        QString outValueMin = lutPair.Token(" ,");
        QString outValueMax = lutPair.Token(" ,");
        lookupTable.AddPair(i, (toDouble(outValueMin) + toDouble(outValueMax)) / 2.0);
      }
    }

    QString instModeId = QString::fromStdString(pdsLab["INSTRUMENT_MODE_ID"]);

    // this will be used to convert num input lines to num output lines,
    //   only changed for when both uv and vis exist (varying summing)
    double visOutputLineRatio = 1.0;
    double uvOutputLineRatio = 1.0;

    int numFilters = 0;
    if(ui.GetBoolean("COLOROFFSET")) {
      colorOffset = ui.GetInteger("COLOROFFSETSIZE");
    }

    // Determine our band information based on
    // INSTRUMENT_MODE_ID - FILTER_NUMBER is
    // only going to be used for BW images
    if(instModeId == "COLOR") {
      numFilters = 7;
      frameletLines.push_back(4);
      frameletLines.push_back(4);
      frameletLines.push_back(14);
      frameletLines.push_back(14);
      frameletLines.push_back(14);
      frameletLines.push_back(14);
      frameletLines.push_back(14);

      uveven  = new Cube();
      uvodd   = new Cube();
      viseven = new Cube();
      visodd  = new Cube();

      // 14 output lines (1 framelet) from 5vis/2uv lines
      visOutputLineRatio = 14.0 / (14.0 * 5.0 + 4.0 * 2.0);

      // 4 output lines (1 framelet) from 5vis/2uv lines
      uvOutputLineRatio = 4.0 / (14.0 * 5.0 + 4.0 * 2.0);
    }
    else if(instModeId == "VIS") {
      numFilters = 5;

      frameletLines.push_back(14);
      frameletLines.push_back(14);
      frameletLines.push_back(14);
      frameletLines.push_back(14);
      frameletLines.push_back(14);

      viseven = new Cube();
      visodd  = new Cube();

      // 14 output lines (1 framelet) from 5vis/2uv lines
      visOutputLineRatio = 14.0 / (14.0 * 5.0);
    }
    else if(instModeId == "UV") {
      numFilters = 2;

      frameletLines.push_back(4);
      frameletLines.push_back(4);

      uveven = new Cube();
      uvodd  = new Cube();

      // 4 output lines (1 framelet) from 2uv lines
      uvOutputLineRatio = 4.0 / (4.0 * 2.0);
    }
    else if(instModeId == "BW") {
      numFilters = 1;

      frameletLines.push_back(14);

      viseven = new Cube();
      visodd  = new Cube();
    }

    padding.resize(numFilters);


    for(int filter = 0; filter < numFilters; filter++) {
      padding[filter] = (colorOffset * frameletLines[filter]) * filter;

      // dont count UV for VIS offsetting
      if(instModeId == "COLOR" && filter > 1) {
        padding[filter] -= 2 * colorOffset * frameletLines[filter];
      }
    }

    FileName baseFileName(ui.GetCubeName("TO"));

    if(uveven && uvodd) {
      // padding[1] is max padding for UV
      int numSamples = ((viseven) ? p.Samples() / 4 : p.Samples());
      numSamples     = 128; // UV is alway sum 4 so it is 128 samples
      int numLines   = (int)(uvOutputLineRatio * inputCubeLines + 0.5) + padding[1];
      int numBands   = 2;

      uveven->setDimensions(numSamples, numLines, numBands);
      uveven->setPixelType(Isis::Real);

      QString filename = baseFileName.path() + "/" + baseFileName.baseName() + ".uv.even.cub";
      uveven->create(filename);

      uvodd->setDimensions(numSamples, numLines, numBands);
      uvodd->setPixelType(Isis::Real);

      filename = baseFileName.path() + "/" + baseFileName.baseName() + ".uv.odd.cub";
      uvodd->create(filename);
    }

    if(viseven && visodd) {
      // padding[size-1] is max padding for vis (padding[0] or padding[4] or padding[6])
      int numSamples = p.Samples();
      int numLines   = (int)(visOutputLineRatio * inputCubeLines + 0.5) + padding[padding.size()-1];
      int numBands   = ((uveven) ? padding.size() - 2 : padding.size());

      viseven->setDimensions(numSamples, numLines, numBands);
      viseven->setPixelType(Isis::Real);

      QString filename = baseFileName.path() + "/" + baseFileName.baseName() + ".vis.even.cub";
      viseven->create(filename);

      visodd->setDimensions(numSamples, numLines, numBands);
      visodd->setPixelType(Isis::Real);

      filename = baseFileName.path() + "/" + baseFileName.baseName() + ".vis.odd.cub";
      visodd->create(filename);
    }

    Pvl isis3VisEvenLab, isis3VisOddLab, isis3UvEvenLab, isis3UvOddLab;

    TranslateLabels(pdsLab, isis3VisEvenLab, isis3VisOddLab, isis3UvEvenLab, isis3UvOddLab, ui);

    writeNullsToFile();
    p.StartProcess(separateFramelets);
    p.EndProcess();

    // Add original labels
    OriginalLabel origLabel(pdsLab);

    int numFramelets = padding.size();
    PvlKeyword numFrameletsKeyword("NumFramelets", std::to_string(numFramelets));

    if(uveven) {
      for(int grp = 0; grp < isis3UvEvenLab.groups(); grp++) {
        uveven->putGroup(isis3UvEvenLab.group(grp));
      }

      if(iApp) {
        History history;
        history.AddEntry();
        uveven->write(history);
      }
      uveven->write(origLabel);

      uveven->close();
      delete uveven;
      uveven = NULL;
    }

    if(uvodd) {
      for(int grp = 0; grp < isis3UvOddLab.groups(); grp++) {
        uvodd->putGroup(isis3UvOddLab.group(grp));
      }

      if(iApp) {
        History history;
        history.AddEntry();
        uvodd->write(history);
      }
      uvodd->write(origLabel);

      uvodd->close();
      delete uvodd;
      uvodd = NULL;
    }

    if(viseven) {
      for(int grp = 0; grp < isis3VisEvenLab.groups(); grp++) {
        viseven->putGroup(isis3VisEvenLab.group(grp));
      }

      if(iApp) {
        History history;
        history.AddEntry();
        viseven->write(history);
      }
      viseven->write(origLabel);

      viseven->close();
      delete viseven;
      viseven = NULL;
    }

    if(visodd) {
      for(int grp = 0; grp < isis3VisOddLab.groups(); grp++) {
        visodd->putGroup(isis3VisOddLab.group(grp));
      }

      if(iApp) {
        History history;
        history.AddEntry();
        visodd->write(history);
      }
      visodd->write(origLabel);

      visodd->close();
      delete visodd;
      visodd = NULL;
    }
  }

  /**
   * Get the framelet number line is in.
   *
   * @param line input line number (starting at 1)
   * @param even Set to true if even framelet, otherwise false
   *
   * @return int framelet this line belongs to
   */
  int getFrameletNumber(int line, int &frameletSetNumber, int &frameletSetOffset,
                        int &frameletLineOffset, bool &even) {
    int frameletNumber = -1;

    int frameletSetSize = 0;

    // framelet set = one capture of vis/uv data (1 to 7 framelets)
    for(unsigned int i = 0; i < frameletLines.size(); i++) {
      frameletSetSize += frameletLines[i];
    }

    frameletSetNumber = (line - 1) / frameletSetSize;

    // frameletSetNumber is 0-based, but even is 1-based, so logic reversed from
    //   what would be intuitive. An odd frameletSetNumber means even framelet.
    even = ((frameletSetNumber % 2) == 1);

    // offset into set
    frameletSetOffset = line - (frameletSetSize * frameletSetNumber) - 1;

    int offset = frameletSetOffset;
    for(unsigned int framelet = 0; frameletNumber < 0; framelet++) {
      offset -= frameletLines.at(framelet);

      if(offset < 0) {
        frameletNumber = framelet;
        frameletLineOffset = offset + frameletLines.at(framelet);
      }
    }

    return frameletNumber;
  }

  //! Separates each of the individual WAC framelets into the right place
  void separateFramelets(Buffer &in) {
    // this is true if uv is summed and mixed with unsummed vis
    bool extractMiddleSamples = false;
    // This is the framelet set the line belongs to
    int frameletSet = 0;
    // this is the offset into the set
    int frameletSetOffset = 0;
    // this is true if framelet belongs in an even cube
    bool even = false;
    // line # in current framelet
    int frameletLineOffset = 0;
    // this is the framelet number the current line belongs in
    int framelet = getFrameletNumber(in.Line(), frameletSet, frameletSetOffset, frameletLineOffset, even);
    // this is the output file the current line belongs in
    Cube *outfile = NULL;

    // uv and vis outputs
    if(viseven && uveven) {
      if(framelet < 2 && even) {
        outfile = uveven;
        extractMiddleSamples = true;
      }
      else if(framelet < 2) {
        outfile = uvodd;
        extractMiddleSamples = true;
      }
      else if(even) {
        outfile = viseven;
      }
      else {
        outfile = visodd;
      }
    }
    // vis output
    else if(viseven) {
      if(even) {
        outfile = viseven;
      }
      else {
        outfile = visodd;
      }
    }
    // uv output
    else {
      extractMiddleSamples = true;

      if(even) {
        outfile = uveven;
      }
      else {
        outfile = uvodd;
      }
    }

    // We know our output file now, so get a linemanager for writing
    LineManager mgr(*outfile);

    // line is framelet * frameletLineOffset + frameletSetOffset
    int outLine = 1;
    int outBand = framelet + 1;

    // if both vis & uv on, outLine is a calculation based on the current line
    //   being uv or vis and the general calculation (above) does not work
    if(viseven && uveven) {
      // uv file
      if(framelet < 2) {
        outLine = frameletSet * 4 + 1 + padding[framelet];
      }
      // vis file
      else {
        outLine = frameletSet * 14 + 1 + padding[framelet];
        outBand -= 2; // uv is not in vis file
      }
    }
    // only vis on
    else if(viseven) {
      outLine = frameletSet * 14 + 1 + padding[framelet];
    }
    // only uv on
    else {
      outLine = frameletSet * 4 + 1 + padding[framelet];
    }

    if(flip) {
      outLine = outfile->lineCount() - (outLine - 1);
    }

    outLine += frameletLineOffset;

    mgr.SetLine(outLine, outBand);

    if(!extractMiddleSamples) {
      for(int i = 0; i < in.size(); i++) {
        if(i >= mgr.size()) {
          std::string msg = "The input file has an unexpected number of samples";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }

        mgr[i] = lookupTable.Map(in[i]);
      }
    }
    else {
      // read middle of input...
      int startSamp = (in.size() / 2) - mgr.size() / 2;
      int endSamp = (in.size() / 2) + mgr.size() / 2;

      if(mgr.size() > in.size()) {
        std::string msg = "Output number of samples calculated is invalid";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }

      for(int inputSamp = startSamp; inputSamp < endSamp; inputSamp++) {
        mgr[inputSamp - startSamp] = lookupTable.Map(in[inputSamp]);
      }
    }

    outfile->write(mgr);
  }

  /**
   * This calculates the output labels for each file, which are only valid if the
   * output cubes have been created (uses ns/nl from each file if
   * available). Otherwise the label calculated for that particular file is
   * incomplete and invalid.
   *
   * One input file goes to 2 or 4 output file, so let's calculate everything we
   * can here.
   *
   * @param pdsLab Input File Label
   * @param isis3VisEven Even vis file output label
   * @param isis3VisOdd  Odd vis file output label
   * @param isis3UvEven  Even UV file output label
   * @param isis3UvOdd   Odd UV file output label
   */
  void TranslateLabels(Pvl &pdsLab, Pvl &isis3VisEven, Pvl &isis3VisOdd,
                       Pvl &isis3UvEven, Pvl &isis3UvOdd, UserInterface &ui) {
    // Let's start by running through the generic translations.

    // Translate the instrument group
    FileName transFile("$ISISROOT/appdata/translations/LroWacInstrument.trn");
    PvlToPvlTranslationManager instrumentXlater(pdsLab, transFile.expanded());
    instrumentXlater.Auto(isis3VisEven);
    instrumentXlater.Auto(isis3VisOdd);
    instrumentXlater.Auto(isis3UvEven);
    instrumentXlater.Auto(isis3UvOdd);

    // Translate the Archive group
    transFile = "$ISISROOT/appdata/translations/LroWacArchive.trn";
    PvlToPvlTranslationManager archiveXlater(pdsLab, transFile.expanded());

    archiveXlater.Auto(isis3VisEven);
    archiveXlater.Auto(isis3VisOdd);
    archiveXlater.Auto(isis3UvEven);
    archiveXlater.Auto(isis3UvOdd);

    vector<PvlKeyword> genericInstrument;
    genericInstrument.push_back(PvlKeyword("DataFlipped", "No"));//(ui.GetBoolean("FLIP")? "Yes" : "No")));

    // color offset doesn't apply to BW mode (single band cubes)
    if(colorOffset && viseven && viseven->bandCount() == 1) {
      genericInstrument.push_back(PvlKeyword("ColorOffset", std::to_string(0)));
    }
    else {
      genericInstrument.push_back(PvlKeyword("ColorOffset", std::to_string(colorOffset)));
    }

    genericInstrument.push_back(PvlKeyword("Decompanded", (ui.GetBoolean("UNLUT") ? "Yes" : "No")));

    // We'll need the instrument groups a lot, get a reference right to them
    PvlGroup &visEvenInst = isis3VisEven.findGroup("Instrument", Pvl::Traverse);
    PvlGroup &visOddInst = isis3VisOdd.findGroup("Instrument", Pvl::Traverse);
    PvlGroup &uvEvenInst = isis3UvEven.findGroup("Instrument", Pvl::Traverse);
    PvlGroup &uvOddInst = isis3UvOdd.findGroup("Instrument", Pvl::Traverse);

    // Add user parameters to the instrument group
    for(unsigned int key = 0; key < genericInstrument.size(); key++) {
      visEvenInst.addKeyword(genericInstrument[key]);
      visOddInst.addKeyword(genericInstrument[key]);
      uvEvenInst.addKeyword(genericInstrument[key]);
      uvOddInst.addKeyword(genericInstrument[key]);
    }

    // add labels unique to particular files
    if(viseven) {
      visEvenInst.addKeyword(PvlKeyword("Framelets", "Even"));
      visEvenInst.addKeyword(PvlKeyword("NumFramelets", std::to_string(viseven->lineCount() / 14)));
      visEvenInst.addKeyword(PvlKeyword("InstrumentId", "WAC-VIS"), Pvl::Replace);
      visEvenInst.addKeyword(PvlKeyword("InstrumentModeId", pdsLab["INSTRUMENT_MODE_ID"]));
    }

    if(visodd) {
      visOddInst.addKeyword(PvlKeyword("Framelets", "Odd"));
      visOddInst.addKeyword(PvlKeyword("NumFramelets", std::to_string(visodd->lineCount() / 14)));
      visOddInst.addKeyword(PvlKeyword("InstrumentId", "WAC-VIS"), Pvl::Replace);
      visOddInst.addKeyword(PvlKeyword("InstrumentModeId", pdsLab["INSTRUMENT_MODE_ID"]));
    }

    // **TEMPORARY. This should be done by a translation table.
    // Translate the BandBin group
    PvlGroup visBandBin("BandBin");
    PvlKeyword visWavelength("Center");
    PvlKeyword visFilterNum("FilterNumber");
    PvlKeyword visBandwidth("Width");

    if(viseven && viseven->bandCount() == 1) {
      visWavelength = pdsLab["CENTER_FILTER_WAVELENGTH"][0];
      visFilterNum = pdsLab["FILTER_NUMBER"][0];

      if(pdsLab.hasKeyword("BANDWIDTH")) {
        visBandwidth = pdsLab["BANDWIDTH"][0];
      }
    }
    else {
      for(int i = 0; i < pdsLab["FILTER_NUMBER"].size(); i++) {
        if(std::stoi(pdsLab["FILTER_NUMBER"][i]) > 2) {
          visWavelength += pdsLab["CENTER_FILTER_WAVELENGTH"][i];
          visFilterNum += pdsLab["FILTER_NUMBER"][i];

          if(pdsLab.hasKeyword("BANDWIDTH")) {
            visBandwidth += pdsLab["BANDWIDTH"][i];
          }
        }
      }
    }

    visBandBin += visFilterNum;
    visBandBin += visWavelength;

    if(visBandwidth.size() != 0) {
      visBandBin += visBandwidth;
    }

    isis3VisEven += visBandBin;
    isis3VisOdd += visBandBin;

    PvlGroup visKerns("Kernels");
    visKerns += PvlKeyword("NaifIkCode", "-85621");
    isis3VisEven += visKerns;
    isis3VisOdd += visKerns;

    if(uveven) {
      uvEvenInst.addKeyword(PvlKeyword("Framelets", "Even"));
      uvEvenInst.addKeyword(PvlKeyword("NumFramelets", std::to_string(uveven->lineCount() / 4)));
      uvEvenInst.addKeyword(PvlKeyword("InstrumentId", "WAC-UV"), Pvl::Replace);
      uvEvenInst.addKeyword(PvlKeyword("InstrumentModeId", pdsLab["INSTRUMENT_MODE_ID"]));
    }

    if(uvodd) {
      uvOddInst.addKeyword(PvlKeyword("Framelets", "Odd"));
      uvOddInst.addKeyword(PvlKeyword("NumFramelets", std::to_string(uvodd->lineCount() / 4)));
      uvOddInst.addKeyword(PvlKeyword("InstrumentId", "WAC-UV"), Pvl::Replace);
      uvOddInst.addKeyword(PvlKeyword("InstrumentModeId", pdsLab["INSTRUMENT_MODE_ID"]));
    }

    // Translate the BandBin group
    PvlGroup uvBandBin("BandBin");
    PvlKeyword uvWavelength("Center");
    PvlKeyword uvFilterNum("FilterNumber");
    PvlKeyword uvBandwidth("Width");

    for(int i = 0; i < pdsLab["FILTER_NUMBER"].size(); i++) {
      if(std::stoi(pdsLab["FILTER_NUMBER"][i]) <= 2) {
        uvWavelength += pdsLab["CENTER_FILTER_WAVELENGTH"][i];
        uvFilterNum += pdsLab["FILTER_NUMBER"][i];

        if(pdsLab.hasKeyword("BANDWIDTH")) {
          uvBandwidth += pdsLab["BANDWIDTH"][i];
        }
      }
    }

    uvBandBin += uvFilterNum;
    uvBandBin += uvWavelength;

    if(uvBandwidth.size() != 0) {
      uvBandBin += uvBandwidth;
    }

    isis3UvEven += uvBandBin;
    isis3UvOdd += uvBandBin;

    PvlGroup uvKerns("Kernels");
    uvKerns += PvlKeyword("NaifIkCode", "-85626");

    isis3UvEven += uvKerns;
    isis3UvOdd += uvKerns;
  }

  /**
   * This initializes all of the files will NULL DN's.
   *
   */
  void writeNullsToFile() {
    // have output vis files? initialize files with nulls
    if(viseven && visodd) {
      LineManager evenLineMgr(*viseven);
      LineManager oddLineMgr(*visodd);
      evenLineMgr.SetLine(1, 1);
      oddLineMgr.SetLine(1, 1);

      for(int i = 0; i < evenLineMgr.size(); i++) {
        evenLineMgr[i] = Isis::Null;
        oddLineMgr[i] = Isis::Null;
      }

      while(!evenLineMgr.end()) {
        viseven->write(evenLineMgr);
        visodd->write(oddLineMgr);
        evenLineMgr++;
        oddLineMgr++;
      }
    }

    // have output uv files? initialize files with nulls
    if(uveven && uvodd) {
      LineManager evenLineMgr(*uveven);
      LineManager oddLineMgr(*uvodd);
      evenLineMgr.SetLine(1, 1);
      oddLineMgr.SetLine(1, 1);

      for(int i = 0; i < evenLineMgr.size(); i++) {
        evenLineMgr[i] = Isis::Null;
        oddLineMgr[i] = Isis::Null;
      }

      while(!evenLineMgr.end()) {
        uveven->write(evenLineMgr);
        uvodd->write(oddLineMgr);
        evenLineMgr++;
        oddLineMgr++;
      }
    }
  }

  /**
   * This method ensures the integrety of the input labels and that the file is
   * exactly as expected.
   *
   * @param pdsLab PDS Cube Labels
   */
  void ValidateInputLabels(Pvl &pdsLab) {
    try {
      // Check known values first to verify they match
      PvlKeyword &lut = pdsLab["LRO:LOOKUP_CONVERSION_TABLE"];

      if(lut.size() != 256) {
        std::string msg = "Keyword [LRO:LOOKUP_CONVERSION_TABLE] has the wrong number of values";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }

      PvlKeyword &missionName = pdsLab["MISSION_NAME"];

      if(missionName.size() != 1 || missionName[0] != "LUNAR RECONNAISSANCE ORBITER") {
        std::string msg = "Keyword [MISSION_NAME] does not have a value of [LUNAR RECONNAISSANCER ORBITER]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }

      PvlKeyword &instrumentId = pdsLab["INSTRUMENT_ID"];

      if(instrumentId.size() != 1 || instrumentId[0] != "LROC") {
        std::string msg = "Keyword [INSTRUMENT_ID] does not have a value of [LROC]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }

      // Make sure CENTER_FILTER_WAVELENGTH/FILTER_NUMBER makes sense
      if(pdsLab["FILTER_NUMBER"].size() != pdsLab["CENTER_FILTER_WAVELENGTH"].size()) {
        std::string msg = "Keywords [FILTER_NUMBER,CENTER_FILTER_WAVELENGTH] must have the same number of values";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }

      if(pdsLab["INSTRUMENT_MODE_ID"][0] == "BW" && pdsLab["FILTER_NUMBER"].size() != 1) {
        std::string msg = "Keyword [FILTER_NUMBER] must have size 1 if [INSTRUMENT_MODE_ID] is [BW]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
      else if(pdsLab["INSTRUMENT_MODE_ID"][0] == "COLOR" && (pdsLab["FILTER_NUMBER"].size() < 5
              || pdsLab["FILTER_NUMBER"].size() > 7 || pdsLab["FILTER_NUMBER"].size() == 6)) {
        std::string msg = "Keyword [FILTER_NUMBER] must have size 5 or 7 if [INSTRUMENT_MODE_ID] is [COLOR]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
      else if(pdsLab["INSTRUMENT_MODE_ID"][0] == "UV" && pdsLab["FILTER_NUMBER"].size() != 2) {
        std::string msg = "Keyword [FILTER_NUMBER] must have size 2 if [INSTRUMENT_MODE_ID] is [UV]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
      else if(pdsLab["INSTRUMENT_MODE_ID"][0] == "VIS" && pdsLab["FILTER_NUMBER"].size() != 5) {
        std::string msg = "Keyword [FILTER_NUMBER] must have size 5 if [INSTRUMENT_MODE_ID] is [VIS]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
      else if(pdsLab["INSTRUMENT_MODE_ID"][0] != "BW" && pdsLab["INSTRUMENT_MODE_ID"][0] != "COLOR" &&
              pdsLab["INSTRUMENT_MODE_ID"][0] != "UV" && pdsLab["INSTRUMENT_MODE_ID"][0] != "VIS") {
        std::string msg = "The value of keyword [INSTRUMENT_MODE_ID] is not recognized";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }

      // number/bandwidth matches for filters
      vector< pair<int, QString> > filters;
      filters.push_back(pair<int, QString>(1, "321"));
      filters.push_back(pair<int, QString>(2, "360"));
      filters.push_back(pair<int, QString>(3, "415"));
      filters.push_back(pair<int, QString>(4, "566"));
      filters.push_back(pair<int, QString>(5, "604"));
      filters.push_back(pair<int, QString>(6, "643"));
      filters.push_back(pair<int, QString>(7, "689"));

      for(int i = 0; i < pdsLab["FILTER_NUMBER"].size(); i++) {
        bool found = false;
        bool match = false;
        for(int j = 0; !found && j < (int) filters.size(); j++) {
          if(std::stoi(pdsLab["FILTER_NUMBER"][i]) == filters[j].first) {
            found = true;

            match = (pdsLab["CENTER_FILTER_WAVELENGTH"][i] == filters[j].second.toStdString());
          }
        }

        if(found && !match) {
          std::string msg = "The [FILTER_NUMBER] and [CENTER_FILTER_WAVELENGTH] keywords do not correspond properly";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }

        if(!found) {
          std::string msg = "The value of the keyword [FILTER_NUMBER] is invalid";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }
      }

      // Now make sure keywords that shouldn't exist dont
      QString invalidKeywords[] = {
        "SPACECRAFT_CLOCK_CNT_PARTITION"
      };

      for(unsigned int i = 0; i < sizeof(QString) / sizeof(invalidKeywords); i++) {
        if(pdsLab.hasKeyword(invalidKeywords[i].toStdString())) {
          std::string msg = "Keyword [";
          msg += invalidKeywords[i];
          msg += "] must not exist";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }
      }

      // Now check for keywords that must be integers
      PvlKeyword &orbitNumber = pdsLab["ORBIT_NUMBER"];
      QRegExp integerRegex("[0-9]+");

      if(orbitNumber.size() != 1 || !integerRegex.exactMatch(QString::fromStdString(orbitNumber[0]))) {
        std::string msg = "The value of keyword [ORBIT_NUMBER] is not valid";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }

      // Check for keywords that must be doubles or integers
      QRegExp numberRegex("[-+]{0,1}([0-9]*\\.[0-9]+)|([0-9]+\\.[0-9]*)|([0-9]+)");
      QString numericKeywords[] = {
        "LRO:BEGIN_TEMPERATURE_SCS",
        "LRO:MIDDLE_TEMPERATURE_SCS",
        "LRO:END_TEMPERATURE_SCS",
        "LRO:BEGIN_TEMPERATURE_FPA",
        "LRO:MIDDLE_TEMPERATURE_FPA",
        "LRO:END_TEMPERATURE_FPA",
        "INTERFRAME_DELAY",
        "EXPOSURE_DURATION"
      };

      for(unsigned int i = 0; i < sizeof(numericKeywords) / sizeof(QString); i++) {
        if(pdsLab[numericKeywords[i].toStdString()].size() != 1 || !numberRegex.exactMatch(QString::fromStdString(pdsLab[numericKeywords[i].toStdString()][0]))) {
          std::string msg = "The value of keyword [";
          msg += numericKeywords[i];
          msg += "] is not valid";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }
      }

      // Now check for keywords that must be dateTtime
      QRegExp timeRegex("[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}\\.[0-9]*");
      QString timeKeywords[] = {
        "START_TIME",
        "STOP_TIME"
      };

      for(unsigned int i = 0; i < sizeof(timeKeywords) / sizeof(QString); i++) {
        if(pdsLab[timeKeywords[i].toStdString()].size() != 1 || !timeRegex.exactMatch(QString::fromStdString(pdsLab[timeKeywords[i].toStdString()][0]))) {
          std::string msg = "The value of keyword [";
          msg += timeKeywords[i];
          msg += "] is not valid";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }
      }

      // Now check keywords that must be clock counts
      QRegExp clockRegex("[0-9]+/[0-9]+:[0-9]+\\.{0,1}[0-9]*");
      QString clockKeywords[] = {
        "SPACECRAFT_CLOCK_START_COUNT",
        "SPACECRAFT_CLOCK_STOP_COUNT"
      };

      for(unsigned int i = 0; i < sizeof(clockKeywords) / sizeof(QString); i++) {
        if(pdsLab[clockKeywords[i].toStdString()].size() != 1 || !clockRegex.exactMatch(QString::fromStdString(pdsLab[clockKeywords[i].toStdString()][0]))) {
          std::string msg = "The value of keyword [";
          msg += clockKeywords[i];
          msg += "] is not valid";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }
      }
    }
    catch(IException &e) {
      std::string msg = "The input product is out of date and has invalid labels. Please get an up to date version from the ASU LROC Team";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
  }
}
