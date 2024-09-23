/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <vector>

#include "Process.h"
#include "Pvl.h"
#include "LineManager.h"
#include "Statistics.h"
#include "Table.h"

using namespace std;
using namespace Isis;

PvlGroup PvlStats(Statistics &stats, const QString &name);
void ThrowException(int, int, int, QString);

const int LINES_POSTRAMP = 30;

void IsisMain() {

  //get the number of samples to skip from the left and right ends
  // of the prefix and suffix data
  UserInterface &ui = Application::GetUserInterface();

  int imageLeft      = 0;
  int imageRight     = 0;
  int rampLeft       = 0;
  int rampRight      = 0;
  int calLeftBuffer  = 0;
  int calRightBuffer = 0;
  int calLeftDark    = 0;
  int calRightDark   = 0;
  int leftBuffer     = 0;
  int rightBuffer    = 0;
  int leftDark       = 0;
  int rightDark      = 0;

  if(ui.GetBoolean("USEOFFSETS")) {
    imageLeft      = ui.GetInteger("LEFTIMAGE");
    imageRight     = ui.GetInteger("RIGHTIMAGE");
    rampLeft       = ui.GetInteger("LEFTIMAGE");
    rampRight      = ui.GetInteger("RIGHTIMAGE");
    calLeftBuffer  = ui.GetInteger("LEFTCALBUFFER");
    calRightBuffer = ui.GetInteger("LEFTCALBUFFER");
    calLeftDark    = ui.GetInteger("LEFTCALDARK");
    calRightDark   = ui.GetInteger("RIGHTCALDARK");
    leftBuffer     = ui.GetInteger("LEFTBUFFER");
    rightBuffer    = ui.GetInteger("RIGHTBUFFER");
    leftDark       = ui.GetInteger("LEFTDARK");
    rightDark      = ui.GetInteger("RIGHTDARK");
  }


  Isis::FileName fromFile = ui.GetCubeName("FROM").toStdString();
  Isis::Cube inputCube;
  inputCube.open(QString::fromStdString(fromFile.expanded()));

  //Check to make sure we got the cube properly
  if(!inputCube.isOpen()) {
    std::string msg = "Could not open FROM cube " + fromFile.expanded();
    throw IException(IException::User, msg, _FILEINFO_);
  }

  Process p;
  Cube *icube = p.SetInputCube("FROM");

  // Get statistics from the cube prefix and suffix data
  Table hifix = icube->readTable("HiRISE Ancillary");
  Statistics darkStats, bufStats, rampDarkStats;
  int tdi = icube->group("Instrument")["Tdi"];
  int binning_mode = icube->group("Instrument")["Summing"];

  //This gets us the statistics for the dark and buffer pixels
  // alongside of the image itself
  for(int rec = 2; rec < hifix.Records(); rec++) {
    vector<int> dark = hifix[rec]["DarkPixels"];
    vector<int> buf = hifix[rec]["BufferPixels"];
    if(buf.size() <= (unsigned int)(leftBuffer + rightBuffer)) {
      ThrowException(buf.size(), leftBuffer, rightBuffer, "image buffer");
    }
    if(dark.size() <= (unsigned int)(leftDark + rightDark)) {
      ThrowException(dark.size(), leftDark, rightDark, "image dark reference");
    }

    for(int i = leftDark; i < (int)dark.size() - rightDark; i++) {
      double d;
      if(dark[i] == NULL2) d = NULL8;
      else if(dark[i] == LOW_REPR_SAT2) d = LOW_REPR_SAT8;
      else if(dark[i] == LOW_INSTR_SAT2) d = LOW_INSTR_SAT8;
      else if(dark[i] == HIGH_INSTR_SAT2) d = HIGH_INSTR_SAT8;
      else if(dark[i] == HIGH_REPR_SAT2) d = HIGH_REPR_SAT8;
      else d = dark[i];
      darkStats.AddData(&d, 1);
    }


    for(int i = leftBuffer; i < (int)buf.size() - rightBuffer; i++) {
      double d;
      if(buf[i] == NULL2) d = NULL8;
      else if(buf[i] == LOW_REPR_SAT2) d = LOW_REPR_SAT8;
      else if(buf[i] == LOW_INSTR_SAT2) d = LOW_INSTR_SAT8;
      else if(buf[i] == HIGH_INSTR_SAT2) d = HIGH_INSTR_SAT8;
      else if(buf[i] == HIGH_REPR_SAT2) d = HIGH_REPR_SAT8;
      else d = buf[i];
      bufStats.AddData(&d, 1);
    }
  }

  // Get statistics from the calibration image

  //Calculate boundaries of the reverse readout lines,
  // Masked lines, and ramp lines.

  //There are always 20 reverse readout lines
  int reverseReadoutLines = 20;

  //Number of mask pixels depends on Binning mode
  int maskLines;
  maskLines = 20 / binning_mode;

  //mask lines go after reverse lines
  maskLines += reverseReadoutLines;

// Actual starting line, number Ramp lines
  int rampStart = maskLines;
  int rampLines = tdi / binning_mode;

  Table calimg = icube->readTable("HiRISE Calibration Image");
  Statistics calStats;
  //Statistics for the Reverse readout lines of the cal image
  Statistics reverseStats;
  //Statistics for the masked lines of the cal image
  Statistics maskStats;
  //Statistics for the ramped lines of the cal image
  Statistics rampStats;

  //Iterate through the calibration image

  //Add in reverse data
  for(int rec = 2 ; rec <= 18 ; rec++) { //Lines [2,18]
    vector<int> lineBuffer = calimg[rec]["Calibration"];
    for(unsigned int i = 2 ; i < lineBuffer.size() - 1 ; i++) { //Samples [2, * -1]
      double d = lineBuffer[i];
      if(lineBuffer[i] == NULL2) {
        d = NULL8;
      }
      else if(lineBuffer[i] == LOW_REPR_SAT2)  {
        d = LOW_REPR_SAT8;
      }
      else if(lineBuffer[i] == LOW_INSTR_SAT2) {
        d = LOW_INSTR_SAT8;
      }
      else if(lineBuffer[i] == HIGH_INSTR_SAT2) {
        d = HIGH_INSTR_SAT8;
      }
      else if(lineBuffer[i] == HIGH_REPR_SAT2) {
        d = HIGH_REPR_SAT8;
      }
      reverseStats.AddData(&d, 1);
    }
  }

  //Add in the mask data
  for(int rec = 22 ; rec < maskLines - 1 ; rec++) {//Lines [22, 38] !!!!dependant on bin
    vector<int> lineBuffer = calimg[rec]["Calibration"];
    for(int i = 2 ; i < (int)lineBuffer.size() - 1 ; i++) { //Samples [2, *-1]
      double d = lineBuffer[i];
      if(d == NULL2) {
        d = NULL8;
      }
      else if(d == LOW_REPR_SAT2)  {
        d = LOW_REPR_SAT8;
      }
      else if(d == LOW_INSTR_SAT2) {
        d = LOW_INSTR_SAT8;
      }
      else if(d == HIGH_INSTR_SAT2) {
        d = HIGH_INSTR_SAT8;
      }
      else if(d == HIGH_REPR_SAT2) {
        d = HIGH_REPR_SAT8;
      }
      maskStats.AddData(&d, 1);
    }
  }

  //Add in the ramp data
  for(int rec = maskLines + 2; rec < calimg.Records() - 1; rec++) {
    vector<int> buf = calimg[rec]["Calibration"];
    //loop through all but the first and last sample of the calibration image
    for(int i = rampLeft; i < (int)buf.size() - rampRight; i++) {
      double d;
      if(buf[i] == NULL2) d = NULL8;
      else if(buf[i] == LOW_REPR_SAT2) d = LOW_REPR_SAT8;
      else if(buf[i] == LOW_INSTR_SAT2) d = LOW_INSTR_SAT8;
      else if(buf[i] == HIGH_INSTR_SAT2) d = HIGH_INSTR_SAT8;
      else if(buf[i] == HIGH_REPR_SAT2) d = HIGH_REPR_SAT8;
      else d = buf[i];
      //Determine which group of stats to add to
      rampStats.AddData(&d, 1);
    }
  }

  // Get statistics from the calibration prefix and suffix data
  Table calfix = icube->readTable("HiRISE Calibration Ancillary");
  Statistics calDarkStats, calBufStats;
  int rampLine0 = rampStart + 1;
  int rampLineN = (rampStart + rampLines - 1) - 1;
  rampLineN = calfix.Records() - 1;
  for(int rec = 0; rec < calfix.Records(); rec++) {
    vector<int> dark = calfix[rec]["DarkPixels"];
    vector<int> buf = calfix[rec]["BufferPixels"];
    if(buf.size() <= (unsigned int)(calLeftBuffer + calRightBuffer)) {
      ThrowException(buf.size(), calLeftBuffer, calRightBuffer, "calibration buffer");
    }
    if(dark.size() <= (unsigned int)(calLeftDark + calRightDark)) {
      ThrowException(dark.size(), calLeftDark, calRightDark, "calibration dark reference");
    }
    for(int i = calLeftDark; i < (int)dark.size() - calRightDark; i++) {
      double d;
      if(dark[i] == NULL2) d = NULL8;
      else if(dark[i] == LOW_REPR_SAT2) d = LOW_REPR_SAT8;
      else if(dark[i] == LOW_INSTR_SAT2) d = LOW_INSTR_SAT8;
      else if(dark[i] == HIGH_INSTR_SAT2) d = HIGH_INSTR_SAT8;
      else if(dark[i] == HIGH_REPR_SAT2) d = HIGH_REPR_SAT8;
      else d = dark[i];
      calDarkStats.AddData(&d, 1);
      if((rec > rampLine0) && (rec < rampLineN)) {
        rampDarkStats.AddData(&d, 1);
      }
    }
    for(int i = calLeftBuffer; i < (int)buf.size() - calRightBuffer; i++) {
      double d;
      if(buf[i] == NULL2) d = NULL8;
      else if(buf[i] == LOW_REPR_SAT2) d = LOW_REPR_SAT8;
      else if(buf[i] == LOW_INSTR_SAT2) d = LOW_INSTR_SAT8;
      else if(buf[i] == HIGH_INSTR_SAT2) d = HIGH_INSTR_SAT8;
      else if(buf[i] == HIGH_REPR_SAT2) d = HIGH_REPR_SAT8;
      else d = buf[i];
      calBufStats.AddData(&d, 1);
    }
  }

  Statistics linesPostrampStats;
  Statistics imageStats;
  Isis::LineManager imageBuffer(inputCube);
  imageBuffer.begin();

  Buffer out(imageBuffer.SampleDimension() - (imageLeft + imageRight),
             imageBuffer.LineDimension(),
             imageBuffer.BandDimension(),
             imageBuffer.PixelType());


  for(int postRampLine = 0 ; postRampLine < LINES_POSTRAMP ; postRampLine++) {
    inputCube.read(imageBuffer);
    for(int postRampSamp = 0 ; postRampSamp < out.SampleDimension() ; postRampSamp++) {
      out[postRampSamp] = imageBuffer[postRampSamp + imageLeft];
    }
    linesPostrampStats.AddData(out.DoubleBuffer(), out.size());
    imageBuffer++;
  }

  for(int imageLine = LINES_POSTRAMP; imageLine < inputCube.lineCount(); imageLine++) {
    inputCube.read(imageBuffer);
    for(int imageSample = 0 ; imageSample < out.SampleDimension(); imageSample++) {
      out[imageSample] = imageBuffer[imageSample + imageLeft];
    }
    imageStats.AddData(out.DoubleBuffer(), out.size());
    imageBuffer++;
  }


  // Generate the statistics in pvl form
  const int NUM_GROUPS = 10;
  PvlGroup groups[NUM_GROUPS];
  groups[0] = PvlStats(linesPostrampStats, "IMAGE_POSTRAMP");
  groups[1] = PvlStats(imageStats, "IMAGE");
  groups[2] = PvlStats(darkStats, "IMAGE_DARK");
  groups[3] = PvlStats(bufStats, "IMAGE_BUFFER");
  groups[4] = PvlStats(reverseStats, "CAL_REVERSE");
  groups[5] = PvlStats(maskStats, "CAL_MASK");
  groups[6] = PvlStats(rampStats, "CAL_RAMP");
  groups[7] = PvlStats(calDarkStats, "CAL_DARK");
  groups[8] = PvlStats(rampDarkStats, "CAL_DARK_RAMP");
  groups[9] = PvlStats(calBufStats, "CAL_BUFFER");

  // Write the results to the output file if the user specified one
  if(ui.WasEntered("TO")) {
    Pvl temp;
    for(int i = 0 ; i < NUM_GROUPS ; i++) {
      temp.addGroup(groups[i]);
    }
    temp.write(ui.GetFileName("TO").toStdString());
  }
  else {
    // Log the results
    for(int i = 0 ; i < NUM_GROUPS ; i++) {
      Application::Log(groups[i]);
    }
  }
}

// Return a PVL group containing the statistical information
PvlGroup PvlStats(Statistics &stats, const QString &name) {
  // Construct a label with the results
  PvlGroup results(name.toStdString());
  if(stats.ValidPixels() != 0) {
    results += PvlKeyword("Average", Isis::toString(stats.Average()));
    results += PvlKeyword("StandardDeviation", Isis::toString(stats.StandardDeviation()));
    results += PvlKeyword("Variance", Isis::toString(stats.Variance()));
    results += PvlKeyword("Minimum", Isis::toString(stats.Minimum()));
    results += PvlKeyword("Maximum", Isis::toString(stats.Maximum()));
  }
  results += PvlKeyword("TotalPixels", Isis::toString(stats.TotalPixels()));
  results += PvlKeyword("ValidPixels", Isis::toString(stats.ValidPixels()));
  results += PvlKeyword("NullPixels", Isis::toString(stats.NullPixels()));
  results += PvlKeyword("LisPixels", Isis::toString(stats.LisPixels()));
  results += PvlKeyword("LrsPixels", Isis::toString(stats.LrsPixels()));
  results += PvlKeyword("HisPixels", Isis::toString(stats.HisPixels()));
  results += PvlKeyword("HrsPixels", Isis::toString(stats.HrsPixels()));
  return results;
}

void ThrowException(int vectorSize, int left, int right, QString name) {
  std::string err;
  err = "You are trying to skip as many or more samples of the " + name.toStdString() +
        " than exist";
  throw IException(IException::User, err, _FILEINFO_);
}
