/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

// Qt library includes
#include <QtCore/qmath.h>
#include <QString>

// naif library includes
#include <SpiceUsr.h>

// Isis includes
#include "Buffer.h"
#include "Cube.h"
#include "FileName.h"
#include "IException.h"
#include "iTime.h"
#include "LineManager.h"
#include "NaifStatus.h"
#include "ProcessByLine.h"
#include "RestfulSpice.h"
#include "spiceql.h"
#include "Table.h"
#include "TextFile.h"

#include <iomanip> // delete ??? after setprecision method is removed
#include <iostream> // delete ??? after setprecision method is removed
#include "Preference.h" // delete ??? after setprecision method is removed

#include "hicrop.h"

using namespace std;

namespace Isis {

  // start process by line method
  void crop(Buffer &out);
  // methods for converting between label clock count times and actual times
  iTime actualTime(iTime timeFromInputClockCount, double tdiMode,
                   double unbinnedRate, double binMode);
  iTime labelClockCountTime(iTime actualCalculatedTime, double tdiMode,
                            double unbinnedRate, double binMode);
  // methods for converting between lines/times/clock counts
  iTime line2time(double lineNumber, double lineRate, double originalStartEt);
  double et2line(double et, double lineRate, double originalStartEt);
  // method to validate calculated or user-entered cropped line and time values
  void validateCropLines();
  void validateCropTimes(double cropStart, double  cropStop,
                         double ckFirstTime, double ckLastTime);

  // declare and initialize global variables needed in crop method.
  LineManager *g_in = NULL;
  Cube *g_cube = NULL;
  int g_cropStartLine = 0;
  int g_cropEndLine = 0;
  int g_cropLineCount = 0;

  void hicrop(UserInterface &ui, Pvl *log) {
    QString inputFileName = ui.GetCubeName("FROM");
    CubeAttributeInput inAtt(inputFileName);
    g_cube = new Cube();
    g_cube->setVirtualBands(inAtt.bands());
    inputFileName = ui.GetCubeName("FROM");
    g_cube->open(inputFileName);

    hicrop(g_cube, ui, log);
    delete g_cube;
    g_cube = NULL;
  }

  void hicrop(Cube *cube, UserInterface &ui, Pvl *log) {
    g_cube = cube;
    QString inputFileName = g_cube->fileName();
    // get user inputs for input cube and open
    try {
      // read kernel files and furnish these kernels for naif routines used in
      // originalStartTime() and time2clock()
      IString ckFileName = ui.GetFileName("CK");

      SpiceQL::load(FileName(QString::fromStdString(ckFileName)).expanded().toLatin1().data());

      if (ui.WasEntered("LSK")) {
        IString lskFileName = ui.GetFileName("LSK");
        SpiceQL::load(FileName(QString::fromStdString(lskFileName)).expanded().toLatin1().data());
      }else{
      }

      if (ui.WasEntered("SCLK")) {
        IString sclkFileName = ui.GetFileName("SCLK");
        SpiceQL::load(FileName(QString::fromStdString(sclkFileName)).expanded().toLatin1().data());
      }else{
        // kPool.loadClockKernels();
      }

      // get values from the labels needed to compute the line rate and the
      // actual start time of the input cube
      Pvl &inLabels = *g_cube->label();
      PvlGroup &inputInst = inLabels.findObject("IsisCube").findGroup("Instrument");
      QString instId = (inputInst["InstrumentId"]);
      if (instId.toUpper() != "HIRISE") {
        IString msg = "Input cube has invalid InstrumentId = [" + instId + "]. "
                      "A HiRise image is required.";
        throw IException(IException::Io, msg, _FILEINFO_);
      }
      double tdiMode = inputInst["Tdi"]; //Original Instrument
      QString labelStartClockCount = inputInst["SpacecraftClockStartCount"];
      double binMode = inputInst["Summing"];
      double deltaLineTimerCount = inputInst["DeltaLineTimerCount"];

      // Compute the line rate using the product of the
      // unbinned line rate (converted to seconds) and the downtrack summing
      double unbinnedRate = (74.0 + (deltaLineTimerCount / 16.0)) / 1000000.0;
      double lineRate = unbinnedRate * binMode;

      // get the actual original start time by making adjustments to the
      // spacecraft clock start count in the labels
      iTime timeFromLabelClockCount = Isis::RestfulSpice::strSclkToEt(-74999, labelStartClockCount.toLatin1().data(), "hirise");
      iTime originalStart = actualTime(timeFromLabelClockCount, tdiMode,
                                       unbinnedRate, binMode);
      double originalStartEt = originalStart.Et();


      pair<double, double> ckCoverage = SpiceQL::getTimeIntervals(ckFileName)[0];
      // find the values of the first and last lines to be kept from user inputs
      if (ui.GetString("SOURCE") == "LINEVALUES") {
        g_cropStartLine = ui.GetInteger("LINE");
        g_cropLineCount = ui.GetInteger("NLINES");
        g_cropEndLine = (g_cropStartLine - 1) + g_cropLineCount;
      }
      else {
        // Get the user inputs to determine the start and end times of the cropped
        // output cube.
        double firstValidTime = 0;
        double lastValidTime = 0;
        if (ui.GetString("SOURCE") == "TIME") {
          firstValidTime = ui.GetDouble("STARTTIME");
          lastValidTime = ui.GetDouble("STOPTIME");
        }
        else if (ui.GetString("SOURCE") == "JITTERFILE") {
          // Read the start and end times from the jitter file
          QString jitterFileName = ui.GetFileName("JITTER");
          TextFile jitterFile(jitterFileName);
          jitterFile.SetComment("#");
          QString currLine;
          jitterFile.GetLine(currLine, true);
          QString firstFileLine = currLine.simplified();
          double firstSampleOffset = (double) toDouble(firstFileLine.split(" ")[0]);
          double firstLineOffset = (double) toDouble(firstFileLine.split(" ")[1]);
          if (firstSampleOffset == 0.0 && firstLineOffset == 0.0) {
            jitterFile.GetLine(currLine, true);
          }
          iTime time = (double) toDouble(
                                  currLine.simplified().split(" ").last());
          firstValidTime = time.Et();
          lastValidTime = time.Et();
          while (jitterFile.GetLine(currLine)) {
            time = (double) toDouble(
                              currLine.simplified().split(" ").last());
            if (time.Et() < firstValidTime) {
              firstValidTime = time.Et();
            }
            if (time > lastValidTime) {
              lastValidTime = time.Et();
            }
          }
        }
        else { // Use kernels (CK) to find the start and end times
          firstValidTime = ckCoverage.first;
          lastValidTime = ckCoverage.second;
        }
        validateCropTimes(firstValidTime, lastValidTime,
                          ckCoverage.first, ckCoverage.second);

        // to get the integer value of the start line, we will always round up
        if (originalStartEt > firstValidTime) {
          g_cropStartLine = 1;
        }
        else {
          // get the exact line number associated with this time and find its
          // decimal value.
          double startTime2Line = et2line(firstValidTime, lineRate, originalStartEt);
          double decimalValue = startTime2Line - qFloor(startTime2Line);
          // notice that for each line, n, it covers times corresponding to the
          // exact line values that are between (n-1)+0.5 and n+0.5
          // we need to be sure that the entire first line has coverage,
          if (decimalValue <= 0.5) {
            // if the decimal value is between 0 and 0.5, the time corresponds to
            // the second half of the nth line, from n to n+0.5.
            // round up to the next line number since the first half of the
            // nth line is not covered
            g_cropStartLine = qCeil(startTime2Line);
          }
          else {
            // if the decimal value is above 0.5, the time corresponds to the
            // first half of the nth line from (n-1)+0.5 to n.
            // round up to get the nth line and add 1 since the nth line is not
            // entirely covered
            g_cropStartLine = qCeil(startTime2Line) + 1;
          }
        }

        // Now, to get the integer value of the end line, always round down.
        // Get the exact line number associated with this time and find its
        // decimal value.
        double stopTime2Line = et2line(lastValidTime, lineRate, originalStartEt);
        double decimalValue = stopTime2Line - qFloor(stopTime2Line);
        if (stopTime2Line > (double) g_cube->lineCount()) {
          g_cropEndLine = g_cube->lineCount();
        }
        else {
          // we need to be sure that the entire last ilne has coverage.
          if (decimalValue >= 0.5) {
            // if the decimal value is above 0.5, the time corresponds to the
            // first half of the nth line.  Round down to the previous line
            // since the second half of this line is not covered
            g_cropEndLine = qFloor(stopTime2Line);
          }
          else {
            // if the decimal value is between 0 and 0.5, the time corresponds
            // to the second half of the nth line.  Since the line is not fully
            // covered, round down to n and subtract 1 to get the previous line.
            g_cropEndLine = qFloor(stopTime2Line) - 1;
          }
        }

      }

      g_cropLineCount = (g_cropEndLine - g_cropStartLine + 1);

      // The following error check has been commented out since it is redundant.
      // Error should be caught when the validateCropTimes() method is called.
      // validateCropLines();

      // update start and stop times of the cropped image based on the
      // the first and last line values that will be kept.
      // subtract 0.5 to get the time at the beginning of the first line
      iTime cropStartTime = line2time((double) g_cropStartLine - 0.5,
                                      lineRate, originalStartEt);
      // add 0.5 to get the time at the end of the last line
      iTime cropStopTime = line2time((double) g_cropEndLine + 0.5,
                                     lineRate, originalStartEt);
      validateCropTimes(cropStartTime.Et(), cropStopTime.Et(),
                        ckCoverage.first, ckCoverage.second);

      // HiRise spacecraft clock format is P/SSSSSSSSSS:FFFFF
      IString actualCropStartClockCount = Isis::RestfulSpice::doubleEtToSclk(-74999, cropStartTime.Et(), "hirise");
      IString actualCropStopClockCount = Isis::RestfulSpice::doubleEtToSclk(-74999, cropStopTime.Et(), "hirise");


      // readjust the time to get the appropriate label value for the
      // spacecraft clock start count for the labels of the cropped cube
      iTime adjustedCropStartTime = labelClockCountTime(cropStartTime, tdiMode,
                                                        unbinnedRate, binMode);
      QString adjustedCropStartClockCount = QString::fromStdString(Isis::RestfulSpice::doubleEtToSclk(-74999, adjustedCropStartTime.Et(), "hirise"));
      iTime adjustedCropStopTime = labelClockCountTime(cropStopTime, tdiMode,
                                                       unbinnedRate, binMode);
      QString adjustedCropStopClockCount = QString::fromStdString(Isis::RestfulSpice::doubleEtToSclk(-74999, adjustedCropStopTime.Et(), "hirise"));



      // Allocate the output file and make sure things get propogated nicely
      ProcessByLine p;
      p.SetInputCube(g_cube);
      p.PropagateTables(false);
      int numSamps = g_cube->sampleCount();
      int numBands = g_cube->bandCount();
      int inputLineCount = g_cube->lineCount();

      Isis::CubeAttributeOutput atts = ui.GetOutputAttribute("TO");
      FileName outFileName = ui.GetCubeName("TO");
      Cube *ocube = p.SetOutputCube(outFileName.expanded(), atts, numSamps, g_cropLineCount, numBands);

      p.ClearInputCubes();
      // Loop through the labels looking for object = Table
      for (int labelObj = 0; labelObj < inLabels.objects(); labelObj++) {
        PvlObject &obj = inLabels.object(labelObj);

        if (obj.name() != "Table") continue;

        // Read the table into a table object
        Table table(obj["Name"], inputFileName);

        // Write the table
        ocube->write(table);
      }

      // test to see what happens if I don't have this code ???
      Pvl &outLabels = *ocube->label();
      // Change the start/end times and spacecraft start/stop counts in the labels
      PvlGroup &outputInst = outLabels.findObject("IsisCube").findGroup("Instrument");
      outputInst["StartTime"][0] = cropStartTime.UTC(); //??? use actual or adjusted like clock counts ???
      outputInst["StopTime"][0] = cropStopTime.UTC(); // adjustedCropStopTime ???
      outputInst["SpacecraftClockStartCount"][0] = adjustedCropStartClockCount;
      outputInst["SpacecraftClockStopCount"][0] = adjustedCropStopClockCount;

      // Create a buffer for reading the input cube
      // Crop the input cube
      g_in = new LineManager(*g_cube);
      p.StartProcess(crop);

      // Construct a label with the results
      PvlGroup results("Results");
      results += PvlKeyword("InputLines", toString(inputLineCount));
      results += PvlKeyword("NumberOfLinesCropped", toString(inputLineCount-g_cropLineCount));
      results += PvlKeyword("OututStartingLine", toString(g_cropStartLine));
      results += PvlKeyword("OututEndingLine", toString(g_cropEndLine));
      results += PvlKeyword("OututLineCount", toString(g_cropLineCount));
      results += PvlKeyword("OututStartTime", cropStartTime.UTC());
      results += PvlKeyword("OututStopTime", cropStopTime.UTC()); //??? adjustedCropStopTime
      results += PvlKeyword("OututStartClock", adjustedCropStartClockCount);
      results += PvlKeyword("OututStopClock", adjustedCropStopClockCount);

      // Cleanup
      p.EndProcess();

      delete g_in;
      g_in = NULL;

      // Write the results to the log
      if(log) {
        log->addLogGroup(results);
      }

    }
    catch (IException &e) {
      IString msg = "Unable to crop the given cube [" + inputFileName
                    + "] using the hicrop program.";
      // clean up before throwing exception
      delete g_in;
      g_in = NULL;
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
  }

  /**
   *  Line processing routine.
   */
  void crop(Buffer &out) {
    // Read the input line
    int iline = g_cropStartLine + (out.Line() - 1);
    g_in->SetLine(iline, 1);
    g_cube->read(*g_in);

    // Loop and move appropriate samples
    for (int i = 0; i < out.size(); i++) {
      out[i] = (*g_in)[i];
    }
  }

  /**
   * This method is used to determine the actual start or stop time for the input
   * image.  This is found by using the time corresponding to clock count
   * read from the label of the input cube.
   *
   * @param timeFromInputClockCount This value is passed in after converting the
   *                                clock count value from the labels to time
   * @param tdiMode The Tdi keyword value from the labels of the input cube.
   * @param unbinnedRate Calculated using values from the input label.
   * @param binMode The Summing keyword value from the labels of the input cube.
   *
   * @see HiriseCamera::HiriseCamera(Pvl &lab)
   */
  iTime actualTime(iTime timeFromInputClockCount, double tdiMode,
                   double unbinnedRate, double binMode) {
    SpiceDouble adjustedTime = timeFromInputClockCount.Et();
    // Adjust the start time so that it is the effective time for
    // the first line in the image file.  Note that on 2006-03-29, this
    // time is now subtracted as opposed to adding it.  The computed start
    // time in the EDR is at the first serial line.
    adjustedTime -= unbinnedRate * (((double) tdiMode / 2.0) - 0.5);
    // Effective observation
    // time for all the TDI lines used for the
    // first line before doing binning
    adjustedTime += unbinnedRate * (((double) binMode / 2.0) - 0.5);
    // Effective observation time of the first line
    // in the image file, which is possibly binned

    // Compute effective line number within the CCD (in pixels) for the
    // given TDI mode.
    //   This is the "centered" 0-based line number, where line 0 is the
    //   center of the detector array and line numbers decrease going
    //   towards the serial readout.  Line number +64 sees a spot
    //   on the ground before line number 0 or -64.
    iTime actualStartTime = adjustedTime;
    return actualStartTime;
  }

  /**
   * This method is used to determine the time value that corresponds to the
   * start or stop clock count that will be written to the output cube. This
   * is found by using the actual start/stop time of the output cube. This
   * is done so that when a HiriseCamera object is created from this image,
   * the correct values will be calculated for the start time.
   *
   * @param actualCalculatedTime This value is the actual start/stop time of
   *                             the output cube that will be adjusted for
   *                             the labels.
   * @param tdiMode The Tdi keyword value from the labels of the input cube.
   * @param unbinnedRate Calculated using values from the input label.
   * @param binMode The Summing keyword value from the labels of the input cube.
   *
   * @see HiriseCamera::HiriseCamera(Pvl &lab)
   */
  iTime labelClockCountTime(iTime actualCalculatedTime, double tdiMode,
                            double unbinnedRate, double binMode) {
    iTime labelStartTime = actualCalculatedTime.Et()
                           + unbinnedRate * ((tdiMode / 2.0) - 0.5)
                           - unbinnedRate * ((binMode / 2.0) - 0.5);
    return labelStartTime;
  }


  /**
   * Returns the corresponding time for the given line number.
   *
   * Notice that this method takes an exact double line value, not an int line
   * value. For example, if the ET at the beginning of a starting line is
   * desired, the value passed in should be the integer line number minus 0.5 and
   * if the ET at the end of the last line is desired, the integer end line number
   * plus 0.5 should be passed in.
   *
   * @param lineNumber The exact line number value for which the image time will
   *                   be found
   * @param lineRate The line rate for this image, calculated by using label
   *                 values.
   * @param originalStartEt The actual start time for the image, calculated by
   *                        using label values.
   *
   * @return The image time corresponding to the given exact line number value.
   *
   * @see LineScanDetectorMap::SetParent()
   *
   */
  iTime line2time(double lineNumber, double lineRate, double originalStartEt) {
    iTime et = (double) (originalStartEt + lineRate * (lineNumber - 0.5));
    return et;
  }

  /**
   * Returns the corresponding line number for the given ephemeris time.  Note
   * that this return value is not an integer line, number, it is the exact double
   * line value associated with this time.
   *
   * @param et The ephemeris time of the image whose exact line value will be
   *           found.
   * @param lineRate The line rate for this image, calculated by using label
   *                 values.
   * @param originalStartEt The actual start time for the image, calculated by
   *                        using label values.
   *
   * @return The exact line number value corresponding to the given image time.
   *
   * @see LineScanDetectorMap::SetDetector()
   */
  double et2line(double et, double lineRate, double originalStartEt) {
    return (et - originalStartEt) / lineRate + 0.5;
  }



  /**
   * This method is used to verify that the values found for the start and end
   * lines is valid.
   */
  void validateCropLines() {
    if (g_cropEndLine < g_cropStartLine) {
      string msg = "Calculated Start/End Lines = [" + IString(g_cropStartLine)
                   + ", " + IString(g_cropEndLine) + "] are invalid. "
                  "End line must be greater than or equal to start line.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
  }

  /**
   * This method is used to verify that the values found for the start and end
   * lines is valid.
   */
  void validateCropTimes(double cropStart, double  cropStop, double ckFirstTime, double ckLastTime) {
    if (cropStart < ckFirstTime ||
        cropStop > ckLastTime) {
      IString msg = "Invalid start/stop times [" + IString(cropStart) + ", "
                    + IString(cropStop) + "]. These times fall outside of the "
                    "given CK file's time coverage [" + IString(ckFirstTime)
                    + ", " + IString(ckLastTime) + "].";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    // Now, for user inputs other than line/nlines, use the start and end
    // times we found to calculate the cropped start/end line numbers
    if (cropStart >= cropStop) {
      IString msg = "Invalid start/stop times. The start ET "
                    "value [" + IString(cropStart) + "] is greater "
                    "than or equal to the stop ET value ["
                    + IString(cropStop) + "].";
      throw IException(IException::Io, msg, _FILEINFO_);
    }

  }
}
