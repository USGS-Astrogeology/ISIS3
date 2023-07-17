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
#include "Application.h"
#include "Buffer.h"
#include "Cube.h"
#include "FileName.h"
#include "IException.h"
#include "iTime.h"
#include "LineManager.h"
#include "NaifStatus.h"
#include "ProcessByLine.h"
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
  pair<double, double> ckBeginEndTimes(IString ckFileName);
  // methods for converting between lines/times/clock counts
  iTime line2time(double lineNumber, double lineRate, double originalStartEt);
  double et2line(double et, double lineRate, double originalStartEt);
  QString time2clock(iTime time);
  iTime clock2time(QString spacecraftClockCount);
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
    //   Isis::Preference::Preferences(true); // delete ???
    cout << setprecision(25);// ???

    g_cube = cube;
    QString inputFileName = g_cube->fileName();
    // get user inputs for input cube and open
    try {

      // read kernel files and furnish these kernels for naif routines used in
      // originalStartTime() and time2clock()
      IString ckFileName = ui.GetFileName("CK");

      IString lskFileName = "";
      if (ui.WasEntered("LSK")) {
        lskFileName = ui.GetFileName("LSK");
      }
      else {
        FileName lskFile("$base/kernels/lsk/naif????.tls");
        lskFileName = lskFile.highestVersion().expanded();
      }

      IString sclkFileName = "";
      if (ui.WasEntered("SCLK")) {
        sclkFileName = ui.GetFileName("SCLK");
      }
      else {
        FileName sclkFile("$mro/kernels/sclk/MRO_SCLKSCET.?????.65536.tsc");
        sclkFileName = sclkFile.highestVersion().expanded();
      }

      // furnish these kernels
      NaifStatus::CheckErrors();
      furnsh_c(ckFileName.c_str());
      NaifStatus::CheckErrors();
      furnsh_c(sclkFileName.c_str());
      NaifStatus::CheckErrors();
      furnsh_c(lskFileName.c_str());
      NaifStatus::CheckErrors();

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
      iTime timeFromLabelClockCount = clock2time(labelStartClockCount);
      iTime originalStart = actualTime(timeFromLabelClockCount, tdiMode,
                                       unbinnedRate, binMode);
      double originalStartEt = originalStart.Et();

      pair<double, double> ckCoverage = ckBeginEndTimes(ckFileName);
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
      IString actualCropStartClockCount = time2clock(cropStartTime);//???
      IString actualCropStopClockCount = time2clock(cropStopTime); //???

  //???
   // UTC
   // cout << "labelStartClock2time = " << timeFromLabelClockCount.UTC() << endl;
   // cout << "adjustedStartClock2time = " << originalStart.UTC() << endl;
   // cout << "cropped starttime = " << cropStartTime.UTC() << endl;
   // cout << "cropped stoptime = " << cropStopTime.UTC() << endl;
   // cout << "time at 80000.5 = " << line2time(80000.5, lineRate, originalStartEt).UTC() << endl << endl;
   //
   // // ET
   // cout << "labelStartClockEt = " << timeFromLabelClockCount.Et() << endl;// should this be
   // cout << "adjustedStartClockEt = " << originalStartEt << endl;// should this be
   // cout << "cropped starttime = " << cropStartTime.Et() << endl;//??? 264289109.970381856
   // cout << "cropped stoptime = " << cropStopTime.Et() << endl;//??? 264289117.285806835
   // cout << "time at 80000.5 = " << line2time(80000.5, lineRate, originalStartEt).Et() << endl << endl;


      // readjust the time to get the appropriate label value for the
      // spacecraft clock start count for the labels of the cropped cube
      iTime adjustedCropStartTime = labelClockCountTime(cropStartTime, tdiMode,
                                                        unbinnedRate, binMode);
      QString adjustedCropStartClockCount = time2clock(adjustedCropStartTime);
      iTime adjustedCropStopTime = labelClockCountTime(cropStopTime, tdiMode,
                                                       unbinnedRate, binMode);
      QString adjustedCropStopClockCount = time2clock(adjustedCropStopTime);



  //???    string stopClockCount = inputInst["SpacecraftClockStopCount"];
  //???    iTime labelStopTime = clock2time(stopClockCount);
  //???    iTime origStop = actualTime(labelStopTime, tdiMode,
  //???                                unbinnedRate, binMode);
  //???    double endline = et2line(origStop.Et(),  lineRate,  originalStartEt);
  //???    cout << std::setprecision(20);
  //???    cout << "Label Stop Count  = " << stopClockCount << endl;
  //???    cout << "Stop Count Time   = " << labelStopTime.Et() << endl;
  //???    cout << "Actual Stop Count = " << origStop.Et() << endl;
  //???    cout << "Actual End Line   = " << endline << endl;
  //???
  //???    cout << endl << "New End Line      = " << g_cropEndLine << endl;
  //???    cout << "New End Line      = " << et2line(cropStopTime.Et(),  lineRate,  originalStartEt) << endl;
  //???    cout << "New Stop Time     = " << cropStopTime.Et() << endl;
  //???    cout << "Actual Stop Count = " << actualCropStopClockCount << endl;
  //???    cout << "Label Stop Time   = " << adjustedCropStopTime.Et() << endl;
  //???    cout << "Label Stop Count  = " << adjustedCropStopClockCount << endl;


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
      Application::Log(results);

      // Unfurnishes kernel files to prevent file table overflow
      NaifStatus::CheckErrors();
      unload_c(ckFileName.c_str());
      unload_c(sclkFileName.c_str());
      unload_c(lskFileName.c_str());
      NaifStatus::CheckErrors();
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
    int band = 1;
    g_in->SetLine(iline, 1);
    g_cube->read(*g_in);

    // Loop and move appropriate samples
    for (int i = 0; i < out.size(); i++) {
      out[i] = (*g_in)[i];
    }

    if (out.Line() == g_cropLineCount) band++;
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
   * Returns the first and last times that are covered by the given CK file. The
   * SCLK and LSK files must be furnished before this method is called.
   *
   * @param ckFileName String containing the name of the ck file provided by the
   *                   user.
   * @return A pair of doubles, the first is the earliest time covered by the CK
   *         file and the second is the latest time covered by the CK file.
   */
  pair<double, double> ckBeginEndTimes(IString ckFileName) {
    //create a spice cell capable of containing all the objects in the kernel.
    NaifStatus::CheckErrors();
    SPICEINT_CELL(currCell, 1000);
    NaifStatus::CheckErrors();
    //this resizing is done because otherwise a spice cell will append new data
    //to the last "currCell"
    ssize_c(0, &currCell);
    NaifStatus::CheckErrors();
    ssize_c(1000, &currCell);
    NaifStatus::CheckErrors();
    ckobj_c(ckFileName.c_str(), &currCell);
    NaifStatus::CheckErrors();
    int numberOfBodies = card_c(&currCell);
    if (numberOfBodies != 1) {
      IString msg = "Unable to find start and stop times using the given CK "
                    "file [" + ckFileName + "]. This application only works with"
                    "single body CK files.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    //get the NAIF body code
    int body = SPICE_CELL_ELEM_I(&currCell, numberOfBodies-1);
    NaifStatus::CheckErrors();
    //  200,000 is the max coverage window size for a CK kernel
    SPICEDOUBLE_CELL(cover, 200000);
    NaifStatus::CheckErrors();
    ssize_c(0, &cover);
    NaifStatus::CheckErrors();
    ssize_c(200000, &cover);
    NaifStatus::CheckErrors();
    ckcov_c(ckFileName.c_str(), body, SPICEFALSE, "SEGMENT", 0.0, "TDB", &cover);
    NaifStatus::CheckErrors();
    //Get the number of intervals in the object.
    int numberOfIntervals = card_c(&cover) / 2;
    NaifStatus::CheckErrors();
    if (numberOfIntervals != 1) {
      IString msg = "Unable to find start and stop times using the given CK "
                    "file [" + ckFileName + "]. This application only works with "
                    "single interval CK files.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    //Convert the coverage interval start and stop times to TDB
    //Get the endpoints of the interval.
    double begin, end;
    wnfetd_c(&cover, numberOfIntervals-1, &begin, &end);
    NaifStatus::CheckErrors();
    QVariant startTime = begin;//??? why use variants? why not just use begin and end ???
    QVariant stopTime = end;   //??? why use variants? why not just use begin and end ???
    pair< double, double > coverage(startTime.toDouble(),  stopTime.toDouble());
    return coverage;
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
   * Returns the corresponding clock count, in string format, for the given time.
   *
   * HiRise is high precision, so the spacecraft clock format is
   * P/SSSSSSSSSS:FFFFF and the clock id = -74999. (See any mro sclk file
   * for documentation on these values.)
   *
   * @param time The time of the image to be converted.
   *
   * @return A string containing the spacecraft clock count corresponding
   *         to the given time.
   */
  QString time2clock(iTime time) {
    // char
    char stringOutput[19];
    double et = time.Et();
    NaifStatus::CheckErrors();
    sce2s_c(-74999, et, 19, stringOutput);
    NaifStatus::CheckErrors();
    return stringOutput;
  }

  /**
   * Returns the corresponding time for the given spacecraft clock count.
   *
   * @param spacecraftClockCount The clock count of the image to be converted.
   *
   * @return The image time corresponding to the given spacecraft clock count.
   *
   * @see Spice::getClockTime(clockCountString, sclkCode)
   */
  iTime clock2time(QString spacecraftClockCount) {
    // Convert the spacecraft clock count to ephemeris time
    SpiceDouble timeOutput;
    // The -74999 is the code to select the transformation from
    // high-precision MRO SCLK to ET
    NaifStatus::CheckErrors();
    scs2e_c(-74999, spacecraftClockCount.toLatin1().data(), &timeOutput);
    NaifStatus::CheckErrors();
    QVariant clockTime = timeOutput;
    iTime time = clockTime.toDouble();
    return time;
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
