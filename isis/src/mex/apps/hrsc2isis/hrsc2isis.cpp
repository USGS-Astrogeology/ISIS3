/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ProcessImportPds.h"
#include "hrsc2isis.h"
#include "UserInterface.h"
#include "EndianSwapper.h"
#include "Table.h"
#include "CubeAttribute.h"
#include "LineManager.h"
#include "IException.h"
#include "Application.h"
#include "OriginalLabel.h"

using namespace std;
namespace Isis{
  void ImportHrscStereoImage(ProcessImportPds &p, Pvl &label, UserInterface &ui, Pvl &originalLab);
  void ImportHrscSrcImage(ProcessImportPds &p, Pvl &label, UserInterface &ui);

  void IgnoreData(Isis::Buffer &buf);
  void WriteOutput(Isis::Buffer &buf);

  void TranslateHrscLabels(Pvl &inLabels, Pvl &outLabel);

  Cube *outCube = NULL;
  long numLinesSkipped = 0;
  std::vector< bool > lineInFile;


  void hrsc2isis(UserInterface &ui) {
    ProcessImportPds p;
    Pvl label;
    p.SetPdsFile(ui.GetFileName("FROM"), "", label);

    // Decide if the file is an HRSC image or something else
    if (label["INSTRUMENT_ID"][0] != "HRSC") {
      IString msg = "File [" + ui.GetFileName("FROM") + "] with [INSTRUMENT_ID = " +
                     label["INSTRUMENT_ID"][0] +
                    "] does not appear to be a Mars Express HRSC image. " +
                    "Consider using pds2isis to import the image.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Decide if the file is an HRSC SRC, HRSC Stereo (S2) or something else
    bool isSrcFile;
    if (label["DETECTOR_ID"][0] == "MEX_HRSC_SRC") {
      isSrcFile = true;
    }
    else if ((label["DETECTOR_ID"][0] == "MEX_HRSC_S2") ||
             (label["DETECTOR_ID"][0] == "MEX_HRSC_RED") ||
             (label["DETECTOR_ID"][0] == "MEX_HRSC_P2") ||
             (label["DETECTOR_ID"][0] == "MEX_HRSC_BLUE") ||
             (label["DETECTOR_ID"][0] == "MEX_HRSC_NADIR") ||
             (label["DETECTOR_ID"][0] == "MEX_HRSC_GREEN") ||
             (label["DETECTOR_ID"][0] == "MEX_HRSC_P1") ||
             (label["DETECTOR_ID"][0] == "MEX_HRSC_IR") ||
             (label["DETECTOR_ID"][0] == "MEX_HRSC_S1")) {
      isSrcFile = false;
    }
    else {
      QString msg = "File [" + ui.GetFileName("FROM");
      msg += "] does not appear to be a Mars Express stereo or SRC file. Label keyword [DETECTOR_ID = ";
      msg += label["DETECTOR_ID"][0] + "] is not recognized.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if ((int)label["PROCESSING_LEVEL_ID"] >= 4) {
      QString msg = "File [" + ui.GetFileName("FROM");
      msg += "] has keyword [PROCESSING_LEVEL_ID = " + label["PROCESSING_LEVEL_ID"][0] + "]";
      msg += " and can not be read by this program.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // The processing for Stereo and SRC are significantly different. Call the
    // appropriate processing function
    if (isSrcFile) {
      ImportHrscSrcImage(p, label, ui);
    }
    else {
      ImportHrscStereoImage(p, label, ui, label);
    }


  }

  // Import a PDS3, HRSC, SRC Camera image.
  void ImportHrscSrcImage(ProcessImportPds &p, Pvl &label, UserInterface &ui) {
    CubeAttributeOutput &att = ui.GetOutputAttribute("TO");
    outCube = p.SetOutputCube(ui.GetCubeName("TO"), att);
    p.StartProcess();

    Pvl otherLabels;
    TranslateHrscLabels(label, otherLabels);

    if (otherLabels.hasGroup("Instrument") &&
        (otherLabels.findGroup("Instrument").keywords() > 0)) {
      outCube->putGroup(otherLabels.findGroup("Instrument"));
    }

    if (otherLabels.hasGroup("BandBin") &&
        (otherLabels.findGroup("BandBin").keywords() > 0)) {
      outCube->putGroup(otherLabels.findGroup("BandBin"));
    }

    if (otherLabels.hasGroup("Archive") &&
        (otherLabels.findGroup("Archive").keywords() > 0)) {
      outCube->putGroup(otherLabels.findGroup("Archive"));
    }

    if (otherLabels.hasGroup("Kernels") &&
        (otherLabels.findGroup("Kernels").keywords() > 0)) {
      outCube->putGroup(otherLabels.findGroup("Kernels"));
    }

    p.EndProcess();
  }


  /**
   * This function reads Mars Express HRSC Stereo files
   *
   * First pass through the file is a Process called with the IgnoreData() function in order to
   * get the import class to collect prefix data. Once the prefix data is populated, we look for
   * "gaps" - HRSC files with prefix data give us a time and exposure duration for each line, we
   * look for where the time + exposure duration != next line's time. We populate a table
   * (LineScanTimes) with the prefix data and lineInFile with whether or not a missing line should
   * be inserted.
   *
   * Second pass through the file is a Process called with the WriteOutput function. If there were
   * gaps, WriteOutput will add lines at the appropriate positions in the output cube. Finally, we
   * translate the labels and put the LineScanTimes (if necessary) in the output cube.
   *
   * This is a two-pass system for files with prefix data, one-pass for files without.
   *
   * NOTE: Regardless of the input file's byte order IMAGE-SAMPLE_TYPE, the prefix data byte order
   * is always LSB.
   */
  void ImportHrscStereoImage(ProcessImportPds &p, Pvl &label, UserInterface &ui, Pvl &originalLab) {
    lineInFile.clear();
    numLinesSkipped = 0;

    CubeAttributeOutput outAtt(ui.GetCubeName("TO"));
    outCube = new Cube();

    outCube->setByteOrder(outAtt.byteOrder());
    outCube->setFormat(outAtt.fileFormat());
    outCube->setLabelsAttached(outAtt.labelAttachment());

    TableField ephTimeField("EphemerisTime", TableField::Double);
    TableField expTimeField("ExposureTime", TableField::Double);
    TableField lineStartField("LineStart", TableField::Integer);

    TableRecord timesRecord;
    timesRecord += ephTimeField;
    timesRecord += expTimeField;
    timesRecord += lineStartField;

    Table timesTable("LineScanTimes", timesRecord);

    p.SetDataPrefixBytes((int)label.findObject("IMAGE")["LINE_PREFIX_BYTES"]);
    p.SaveDataPrefix();

    p.Progress()->SetText("Reading Prefix Data");
    p.StartProcess(IgnoreData);

    // Get the prefix data from the Process
    // The prefix data is always in LSB format, regardless of the overall file format
    EndianSwapper swapper("LSB");

    std::vector<double> ephemerisTimes;
    std::vector<double> exposureTimes;
    std::vector< std::vector<char *> > prefix = p.DataPrefix();

    for (int line = 0; line < p.Lines(); line++) {
      double ephTime = swapper.Double((double *)prefix[0][line]);
      double expTime = swapper.Float((float *)(prefix[0][line] + 8)) / 1000.0;

      if (line > 0) {
        /**
         * We know how many skipped lines with this equation. We take the
         * difference in the current line and the last line's time, which will
         * ideally be equal to the last line's exposure duration. We divide this by
         * the last line's exposure duration, and the result is the 1-based count of
         * how many exposures there were between the last line and the current line.
         * We subtract one in order to remove the known exposure, and the remaining should
         * be the 1-based count of how many lines were skipped. Add 0.5 to round up.
         */
        int skippedLines = (int)((ephTime - ephemerisTimes.back()) / exposureTimes.back() - 1.0 + 0.5);

        for (int i = 0; i < skippedLines; i++) {
          ephemerisTimes.push_back(ephemerisTimes.back() + exposureTimes.back());
          exposureTimes.push_back(exposureTimes.back());
          lineInFile.push_back(false);
        }
      }

      ephemerisTimes.push_back(ephTime);
      exposureTimes.push_back(expTime);
      lineInFile.push_back(true);
    }

    double lastExp = 0.0;
    for (unsigned int i = 0; i < ephemerisTimes.size(); i++) {
      if (lastExp != exposureTimes[i]) {
        lastExp = exposureTimes[i];
        timesRecord[0] = ephemerisTimes[i];
        timesRecord[1] = exposureTimes[i];
        timesRecord[2] = (int)i + 1;
        timesTable += timesRecord;
      }
    }

    outCube->setDimensions(p.Samples(), lineInFile.size(), p.Bands());

    p.Progress()->SetText("Importing");
    outCube->create(ui.GetCubeName("TO"));
    p.StartProcess(WriteOutput);

    outCube->write(timesTable);

    // Translate the PDS labels into ISIS labels and add them to the cube
    Pvl otherLabels;

    TranslateHrscLabels(label, otherLabels);

    if (otherLabels.hasGroup("Instrument") &&
        (otherLabels.findGroup("Instrument").keywords() > 0)) {
      outCube->putGroup(otherLabels.findGroup("Instrument"));
    }

    if (otherLabels.hasGroup("BandBin") &&
        (otherLabels.findGroup("BandBin").keywords() > 0)) {
      outCube->putGroup(otherLabels.findGroup("BandBin"));
    }

    if (otherLabels.hasGroup("Archive") &&
        (otherLabels.findGroup("Archive").keywords() > 0)) {
      outCube->putGroup(otherLabels.findGroup("Archive"));
    }

    if (otherLabels.hasGroup("Kernels") &&
        (otherLabels.findGroup("Kernels").keywords() > 0)) {
      outCube->putGroup(otherLabels.findGroup("Kernels"));
    }


    OriginalLabel ol(originalLab);
    outCube->write(ol);

    p.EndProcess();

    outCube->close();
    delete outCube;
    outCube = NULL;
    lineInFile.clear();
  }


  // Processing function called by ProcessImportPds:StartProcess for each HRSC Stereo instrumennt
  // line. It ignores the image data and returns. This is used to get the ProcessImportPds object
  // to collect the prefix bytes.
  void IgnoreData(Isis::Buffer &buf) {
    return;
  }


  // Processing function called by ProcessImportPds:StartProcess for each HRSC Stereo instrument
  // line in the IMG file.
  void WriteOutput(Isis::Buffer &buf) {

    LineManager outLines(*outCube);

    if (lineInFile.size()) {
      for (int i = 0; i < outLines.size(); i++) {
        outLines[i] = Isis::Null;
      }

      while (!lineInFile[(buf.Line()+numLinesSkipped) % lineInFile.size()]) {
        outLines.SetLine(buf.Line() + numLinesSkipped, buf.Band());
        outCube->write(outLines);
        numLinesSkipped++;
      }
    }

    outLines.SetLine(buf.Line() + numLinesSkipped, buf.Band());

    for (int i = 0; i < outLines.size(); i++) {
      outLines[i] = buf[i];
    }

    outCube->write(outLines);

    return;
  }


  // Translate HRSC Stereo lables into ISIS labels
  void TranslateHrscLabels(Pvl &inLabels, Pvl &outLabel) {

    // Get the directory where the translation tables are.
    QString transDir = "$ISISROOT/appdata/translations/";

    // Translate the Instrument group
    FileName transFile(transDir + "MexHrscInstrument.trn");
    PvlToPvlTranslationManager instrumentXlater(inLabels, transFile.expanded());
    instrumentXlater.Auto(outLabel);

    // Remove 'Z' from times
    QString startTime = outLabel.findGroup("Instrument", Pvl::Traverse)["StartTime"][0];
    startTime = startTime.mid(0, startTime.size() - 1);
    outLabel.findGroup("Instrument", Pvl::Traverse)["StartTime"] = startTime;

    QString stopTime = outLabel.findGroup("Instrument", Pvl::Traverse)["StopTime"][0];
    stopTime = stopTime.mid(0, stopTime.size() - 1);
    outLabel.findGroup("Instrument", Pvl::Traverse)["StopTime"] = stopTime;

    // Translate the BandBin group
    transFile  = transDir + "MexHrscBandBin.trn";
    PvlToPvlTranslationManager bandBinXlater(inLabels, transFile.expanded());
    bandBinXlater.Auto(outLabel);

    // Translate the Archive group
    transFile  = transDir + "MexHrscArchive.trn";
    PvlToPvlTranslationManager archiveXlater(inLabels, transFile.expanded());
    archiveXlater.Auto(outLabel);

    // Translate the Kernels group
    transFile  = transDir + "MexHrscKernels.trn";
    PvlToPvlTranslationManager kernelsXlater(inLabels, transFile.expanded());
    kernelsXlater.Auto(outLabel);

  }
}
