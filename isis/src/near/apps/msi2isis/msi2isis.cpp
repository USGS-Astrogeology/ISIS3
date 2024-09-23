/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "msi2isis.h"

#include <QString>

#include "Cube.h"
#include "CubeAttribute.h"
#include "Enlarge.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "ProcessBySample.h"
#include "ProcessByBrick.h"
#include "ProcessImportPds.h"
#include "ProcessRubberSheet.h"
#include "Pvl.h"
#include "PvlKeyword.h"
#include "PvlGroup.h"
#include "PvlToPvlTranslationManager.h"
#include <QScopedPointer>
#include "UserInterface.h"

using namespace std;

namespace Isis {


  static void translateMsiLabels(Pvl inputLabelPvl, Pvl *isisLabelPvl);


  void msi2isis( UserInterface &ui, Pvl *log ) {

    // get the label for the input image
    FileName from = ui.GetFileName("FROM").toStdString();
    if (IString(from.extension()).UpCase() != "LBL") {
      from = from.setExtension("lbl");
      if (!from.fileExists()) {
        from = from.setExtension("LBL");
      }
      if (!from.fileExists()) {
        throw IException(IException::Io,
                        "Unable to find PDS label file for ["
                        + ui.GetFileName("FROM").toStdString() + "].",
                        _FILEINFO_);
      }
    }

    // These images are small (537 cols x 244 line) and will be run
    //  through 3 processes.
    //  (1) Import to Isis cube format
    //  (2) Enlarge to 537 cols x 412 lines
    //  (3) Flip over horizontal axis, if LINE_DISPLAY_DIRECTION=UP
    //      and null edges.

    //  The first process will import the image into Cube format as is.
    ProcessImportPds importPds;
    Pvl inputLabelPvl;

    importPds.SetPdsFile(QString::fromStdString(from.expanded()), "", inputLabelPvl);


    // from the pds label, verfify that the image is valid before continuing
    if(inputLabelPvl["INSTRUMENT_ID"][0] != "MSI") {
      throw IException(IException::Io,
                      "The input label [" + from.expanded() + "] has an invalid "
                      "value for INSTRUMENT_ID = ["
                      + inputLabelPvl["INSTRUMENT_ID"][0]
                      + "]. The msi2isis program requires INSTRUMENT_ID = [MSI].",
                      _FILEINFO_);
    }
    int lines = IString::ToInteger(inputLabelPvl.findObject("IMAGE")["LINES"][0]);
    int samples = IString::ToInteger(inputLabelPvl.findObject("IMAGE")["LINE_SAMPLES"][0]);
    if ( (  (lines != 244) && (lines != 412) ) || samples != 537) {
      std::string msg = "The given file [" + from.expanded() + "] does not contain "
                    "a full MSI image. Full NEAR Shoemaker MSI images have "
                    "dimension 537 samples x 244 (or 412) lines. The given image is ["
                    + toString(samples) + "] samples by ["
                    + toString(lines) + "] lines.";
      throw IException(IException::Io, msg, _FILEINFO_);
    }
    if (inputLabelPvl["SAMPLE_DISPLAY_DIRECTION"][0] != "RIGHT") {
      std::string msg = "The input label [" + from.expanded() + "] has an invalid "
                    "value for SAMPLE_DISPLAY_DIRECTION = ["
                    + inputLabelPvl["SAMPLE_DISPLAY_DIRECTION"][0]
                    + "]. The msi2isis program requires "
                    "SAMPLE_DISPLAY_DIRECTION = [RIGHT].";
      throw IException(IException::Io, msg, _FILEINFO_);
    }
    if (inputLabelPvl["LINE_DISPLAY_DIRECTION"][0] != "UP") {
      std::string msg = "The input label [" + from.expanded() + "] has an invalid "
                    "value for LINE_DISPLAY_DIRECTION = ["
                    + inputLabelPvl["LINE_DISPLAY_DIRECTION"][0]
                    + "]. The msi2isis program requires "
                    "LINE_DISPLAY_DIRECTION = [UP].";
      throw IException(IException::Io, msg, _FILEINFO_);
    }
    // Don't import projected image
    if(inputLabelPvl.hasObject("IMAGE_MAP_PROJECTION")) {
      std::string msg = "Unable to import the NEAR Shoemaker MSI image from ["
                    + from.expanded() + "] using msi2isis.This program only "
                    "imports images that have not been projected. Use pds2isis. ";
      throw IException(IException::Io, msg, _FILEINFO_);
    }


    // the given input file appears to be valid, continue with the import process
    FileName importProcessOutCube = FileName::createTempFile("$Temporary/" + from.baseName() + ".import.tmp.cub");
    CubeAttributeOutput outatt = CubeAttributeOutput("+Real");
    importPds.SetOutputCube(QString::fromStdString(importProcessOutCube.expanded()), outatt);
    importPds.StartProcess();
    importPds.Finalize();
    importPds.ClearCubes();


    CubeAttributeInput inatt;
    FileName enlargeProcessOutCube = FileName::createTempFile("$Temporary/" + from.baseName() + ".enlarge.tmp.cub");
    if ( 412 != lines ) {
      // The second process will enlarge the imported cube from 537x244 to 537x412
      ProcessRubberSheet enlargeProcess;
      Cube *cube = enlargeProcess.SetInputCube(QString::fromStdString(importProcessOutCube.expanded()), inatt);
      enlargeProcess.SetOutputCube(QString::fromStdString(enlargeProcessOutCube.expanded()), outatt, 537, 412, 1);

      // Set up the interpolator
      QScopedPointer<Interpolator>  interp;
      if(ui.GetString("INTERP") == "NEARESTNEIGHBOR") {
              interp.reset( new Interpolator(Interpolator::NearestNeighborType) );
      }
      else if(ui.GetString("INTERP") == "BILINEAR") {
              interp.reset( new Interpolator(Interpolator::BiLinearType) );
      }
      else { //if(ui.GetString("INTERP") == "CUBICCONVOLUTION") {
              interp.reset( new Interpolator(Interpolator::CubicConvolutionType) );
      }

      double sampleScale = 1.0;
      double lineScale = 412.0/(double)lines;
      QScopedPointer<Enlarge> enlarge( new Enlarge(cube, sampleScale, lineScale));
      enlargeProcess.StartProcess(*enlarge, *interp);
      enlargeProcess.Finalize();
      enlargeProcess.ClearCubes();

    }
    else {
        // Image has the proper expanded size
        enlargeProcessOutCube = importProcessOutCube;
    }

    // The third (last) process will flip the image lines and set the 33 pixels
    // along each border (top, bottom, left, and right) to null.
    ProcessBySample processSamps;
    processSamps.SetInputCube(QString::fromStdString(enlargeProcessOutCube.expanded()), inatt);

    QString fname = ui.GetCubeName("TO");
    CubeAttributeOutput &atts = ui.GetOutputAttribute("TO");
    Cube *outputCube = processSamps.SetOutputCube(fname, atts);


    // translate labels
    try {
      // translate the band bin and archive groups to this pvl
      Pvl bandBinAndArchivePvl;
      importPds.TranslatePdsLabels(bandBinAndArchivePvl);

      // add translated values from band bin and archive groups to the output cube
      PvlGroup outputBandBinGrp("BandBin");
      PvlGroup outputArchiveGrp("Archive");

      if(bandBinAndArchivePvl.findGroup("BandBin").keywords() > 0) {
        outputBandBinGrp = bandBinAndArchivePvl.findGroup("BandBin");
      }

      // This group will never be empty since we translate INSTRUMENT_ID and this
      // is required for the Instrument group
      outputArchiveGrp = bandBinAndArchivePvl.findGroup("Archive");

      outputCube->putGroup(PvlGroup("Instrument"));
      outputCube->putGroup(outputBandBinGrp);
      outputCube->putGroup(outputArchiveGrp);
      outputCube->putGroup(PvlGroup("Kernels"));

      Pvl *isisLabelPvl = outputCube->label();
      translateMsiLabels(inputLabelPvl, isisLabelPvl);
    }
    catch ( IException &e) {
      remove(importProcessOutCube.expanded().c_str());
      remove(enlargeProcessOutCube.expanded().c_str());
      std::string msg = "Unable to translate the labels from [" + from.expanded()
                    + "] to ISIS format using msi2isis.";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }

    // Determine trim conditions for this image
    int trim = 33;         // Default trim buffer size
    int numSamples = outputCube->sampleCount();

    if ( ui.GetBoolean("TRIM") == false ) { // TRIM=false
      trim = 0;                     // No trimming of lines/samples
    }

//************************************************************************
// Lambda to flip and trim the image data on ingest
//************************************************************************

 /**
   * @brief Flip the lines and trim the edges.
   *
   * This process will flip over the horizontal axis (i.e. flip lines) and trim
   * 33 pixels from the top, bottom, left and right edges of the input buffer.
   *
   * @param in Reference to the input buffer.
   * @param out Reference to the output buffer.
   */
    auto flipAndTrim = [&trim, &numSamples](Buffer &in, Buffer &out)->void {
      if ((in.Sample() <= trim) || (in.Sample() > (numSamples - trim)) ) {
        for(int i = 0; i < in.size(); i++) {
          out[i] = NULL8;
        }
      }
      // Otherwise, if the current sample is between 33 and 504, trim the
      // first and last 33 lines and flip the rest.
      else {
        int lastLineIndex = in.size() - 1;
        for(int i = 0; i < in.size(); i++) {
          if ( (in.Line(i) <= trim) || (in.Line(i) > (in.LineDimension() - trim)) ) {
            out[i] = NULL8;
          }
          else {
            out[i] = in[lastLineIndex - i];
          }
        }
      }
    };


    processSamps.ProcessCube(flipAndTrim);
    processSamps.Finalize();
    processSamps.ClearCubes();


    // clean up temp files and "new" pointers
    remove(importProcessOutCube.expanded().c_str());
    remove(enlargeProcessOutCube.expanded().c_str());
  }


  /**
   * Translate the MSI labels into Isis.
   *
   * This method requires the Instrument, BandBin, and Kernels groups to already
   * exist in the labels.
   *
   * @param inputLabelPvl The PDS input file.
   * @param isisLabelPvl The output Isis cube label Pvl.
   */
  void translateMsiLabels(Pvl inputLabelPvl, Pvl *isisLabelPvl) {
    PvlGroup &instGrp(isisLabelPvl->findGroup("Instrument", Pvl::Traverse));
    PvlGroup &bandBinGrp(isisLabelPvl->findGroup("BandBin", Pvl::Traverse));
    PvlGroup &kernelsGrp(isisLabelPvl->findGroup("Kernels", Pvl::Traverse));

    kernelsGrp += PvlKeyword("NaifFrameCode", "-93001");

    PvlToPvlTranslationManager labelXlater(inputLabelPvl,
                                      "$ISISROOT/appdata/translations/NearMsiImportPdsLabel.trn");
    labelXlater.Auto(*isisLabelPvl);


    // Add units to center wave length
    bandBinGrp.findKeyword("Center").setUnits("nm");

    // Correct SCLK times for use in camera model and sumspice!
    PvlKeyword &sclkStart = instGrp["SpacecraftClockStartCount"];
    PvlKeyword &sclkStop  = instGrp["SpacecraftClockStopCount"];

    // Record the orignal SCLKS
    instGrp += PvlKeyword("OriginalSpacecraftClockStartCount", sclkStart[0]);
    instGrp += PvlKeyword("OriginalSpacecraftClockStopCount",  sclkStop[0]);

    // Correct format of SCLK by remove the .
    // Test if the correction has been made - it should be an integer
    if (QString::fromStdString(sclkStart[0]).contains(".") ) {
      sclkStart = (QString::fromStdString(sclkStart[0]).remove(".")).toStdString();
      sclkStop  = (QString::fromStdString(sclkStop[0]).remove(".")).toStdString();
    }

    // Read DPU deck temperature value from the labels (This value is also given
    // in celcius in the FITS header under the keyword NEAR-049)
    instGrp += PvlKeyword("DpuDeckTemperature",
                      inputLabelPvl["INSTRUMENT_TEMPERATURE"][1],
                      inputLabelPvl["INSTRUMENT_TEMPERATURE"].unit());

    // we might want to read values from the FITS header in the future. For
    // example...
    //
    // importPds.SaveFileHeader();
    // char *fitsHeader = importPds.FileHeader();
    //
    // or
    //
    // importPds.SaveDataHeader();
    // char *fitsHeader = importPds.DataHeader()

    // int startByte = 4251; //if this keyword value was found between 4251:4260
    // int numBytes = 10;
    // string summingMode(numBytes, '\0');
    // for (int byte = 0; byte < numBytes; byte++) {
    //   summingMode[byte] = fitsHeader[startByte + byte];
    // }
  }

} // namespace Isis
