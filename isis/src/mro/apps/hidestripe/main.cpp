/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

// system include files go first
#include <QString>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>

// Isis specific include files go next
#include "LineManager.h"
#include "ProcessByLine.h"
#include "Statistics.h"
#include "IException.h"
#include "Pipeline.h"
#include "Pvl.h"
#include "Table.h"

using namespace std;
using namespace Isis;

// These global vectors are used to keep track of info for columns or rows
// of image data.  For example, a 100 sample x 200 line x 2 band cube will
// have a vectors of 200 columns when processing in the column direction
// (100 samples x 2 bands).  Likewise, the vectors will have 400 rows when
// processing in the line direction (200 lines x 2 bands)
Statistics stats;
static Statistics lineStats[4];
static vector<Statistics> lines[4];

// Size of the cube
static unsigned int myIndex = 0;
static int offset;
static int mode = 1;

// function prototypes
void getStats(Buffer &in);
void fix(Buffer &in, Buffer &out);
void DestripeForOtherBinningModes(int piSamples);

int channel0Phases[] = {252, 515, 778, 1024};
int channel1Phases[] = {247, 510, 773, 1024};
static int *phases;
const int num_phases = 4;

// Main Program
void IsisMain() {

  UserInterface &ui = Application::GetUserInterface();
  Isis::FileName fromFile = ui.GetCubeName("FROM");

  Isis::Cube inputCube;
  inputCube.open(fromFile.expanded());

  //Check to make sure we got the cube properly
  if(!inputCube.isOpen()) {
    std::string msg = "Could not open FROM cube" + fromFile.expanded();
    throw IException(IException::User, msg, _FILEINFO_);
  }

  ProcessByLine processByLine;
  Cube *icube = processByLine.SetInputCube("FROM");
  int totalSamples = icube->sampleCount();

  //We'll be going through the cube by line, manually differentiating
  // between phases
  Isis::LineManager lineManager(inputCube);
  lineManager.begin();


  Table hifix("HiRISE Ancillary");
  int channel = icube->group("Instrument")["ChannelNumber"];

  if(channel == 0) {
    phases = channel0Phases;
  }
  else {
    phases = channel1Phases;
  }
  int binning_mode = icube->group("Instrument")["Summing"];
  if(binning_mode != 1 && binning_mode != 2) {
    /*IString msg = "You may only use input with binning mode 1 or 2, not";
    msg += binning_mode;
    throw iException::Message(iException::User, msg, _FILEINFO_);*/
    DestripeForOtherBinningModes(totalSamples);
  }
  else {
    //Adjust phase breaks based on the binning mode
    for(int i = 0 ; i < num_phases ; i++) {
      phases[i] /= binning_mode;
    }

    //Phases must be able to stretch across the entire cube
    if(totalSamples != phases[3]) {
      QString required_samples(QString::number(phases[3]));
      QString bin_QString(QString::number(binning_mode));
      std::string msg = "image must have exactly ";
      msg += required_samples;
      msg += " samples per line for binning mode ";
      msg += bin_QString;
      throw IException(IException::User, msg, _FILEINFO_);
    }

    //Index starts at 1 and will go up to totalLines. This must be done since
    // lines go into different statistics vectors based on their index
    myIndex = 1;
    processByLine.StartProcess(getStats);

    //This program is trying to find horizontal striping in the image that occurs
    // in every other line, but at runtime we do not know whether that striping
    // occurs on the odd numbered lines (1, 3, 5, etc.) or the even numbered
    // ones (2, 4, 6, etc.). The below algorithm determines which of these is the
    // case.

    QString parity = ui.GetString("PARITY");
    if(parity == "EVEN") {
      offset = 1;
    }
    else if(parity == "ODD") {
      offset = 0;
    }
    else {
      //PRECONDITION: getStats must have been run
      long double maxDiff = 0;
      int maxDiffIndex = 0;
      for(int i = 0 ; i < num_phases ; i++) {
        long double thisDiff;
        thisDiff = lineStats[i].Average() - stats.Average();
        if(thisDiff < 0) {
          thisDiff *= -1;
        }
        if(thisDiff > maxDiff) {
          maxDiff = thisDiff;
          maxDiffIndex = i;
        }
      }
      if(maxDiffIndex == 1 || maxDiffIndex == 3) {
        offset = 1;
      }
      else {
        offset = 0;
      }
    }

    //Again we must reset the index, because we apply corrections only on every
    // other line and the fix processing function has no concept of where it is
    // in the cube.
    myIndex = 1;

    mode = (ui.GetString("CORRECTION") == "MULTIPLY");

    processByLine.SetOutputCube("TO");
    processByLine.StartProcess(fix);
    processByLine.EndProcess();
  }
}

//**********************************************************
// Get statistics on a line of pixels and break it into phases
//**********************************************************
//add all the data to the stats statistics object. When we compare which of
// the lines (%4 = 0, %4 = 1, %4 = 2, %4 = 3) is the furtherest from the
// total average, we use stats for the "total average"
void getStats(Buffer &in) {
  stats.AddData(in.DoubleBuffer(), in.size());

  //Phase 1 processing
  {
    Buffer proc(phases[0], 1, 1, in.PixelType());
    for(int quad1 = 0 ; quad1 < phases[0] ; quad1++) {
      proc[quad1] = in[quad1];
    }
    Statistics temp;
    temp.AddData(proc.DoubleBuffer(), proc.size());
    lines[0].push_back(temp);
    stats.AddData(proc.DoubleBuffer(), proc.size());
    lineStats[0].AddData(proc.DoubleBuffer(), proc.size());
  }

  //Phase 2 processing
  {
    Buffer proc(phases[1] - phases[0], 1, 1, in.PixelType());
    for(int quad2 = phases[0] ; quad2 < phases[1] ; quad2++) {
      proc[quad2 - phases[0]] = in[quad2];
    }
    Statistics temp;
    temp.AddData(proc.DoubleBuffer(), proc.size());
    lines[1].push_back(temp);
    stats.AddData(proc.DoubleBuffer(), proc.size());
    lineStats[1].AddData(proc.DoubleBuffer(), proc.size());
  }

  //Phase 3 processing
  {
    Buffer proc(phases[2] - phases[1], 1, 1, in.PixelType());
    for(int quad3 = phases[1] ; quad3 < phases[2] ; quad3++) {
      proc[quad3 - phases[1]] = in[quad3];
    }
    Statistics temp;
    temp.AddData(proc.DoubleBuffer(), proc.size());
    lines[2].push_back(temp);
    stats.AddData(proc.DoubleBuffer(), proc.size());
    lineStats[2].AddData(proc.DoubleBuffer(), proc.size());
  }

  //Phase 4 processing
  {
    Buffer proc(phases[3] - phases[2], 1, 1, in.PixelType());
    for(int quad4 = phases[2] ; quad4 < phases[3] ; quad4++) {
      proc[quad4 - phases[2]] = in[quad4];
    }
    Statistics temp;
    temp.AddData(proc.DoubleBuffer(), proc.size());
    lines[3].push_back(temp);
    stats.AddData(proc.DoubleBuffer(), proc.size());
    lineStats[3].AddData(proc.DoubleBuffer(), proc.size());
  }

  myIndex++;
}

// Apply coefficients based on mode
void fix(Buffer &in, Buffer &out) {
//If this is an "off" line, no correction needs to be applied
  if(myIndex % 2 == (unsigned int)offset) {
    //This is an "off" line, so just copy the data from the input
    for(int i = 0 ; i < in.size() ; i++) {
      out[i] = in[i];
    }
  }
  else {
    //This is not an "off" line, so apply the correction
    for(int i = 0; i < in.size(); i++) {
      int focusPhase = 0;
      if(i < phases[0]) {
        focusPhase = 0; /*First phase*/
      }
      else if(i < phases[1]) {
        focusPhase = 1; /*Second phase*/
      }
      else if(i < phases[2]) {
        focusPhase = 2; /*Third phase*/
      }
      else {
        focusPhase = 3; /*Fourth phase*/
      }

      if(IsSpecial(in[i])) {
        out[i] = in[i]; /* No correction on special pixels*/
      }
      else {
        double coeff;
        if(myIndex == 1) {
          coeff = lines[focusPhase][myIndex].Average();
        }
        else if((unsigned int)myIndex == lines[focusPhase].size()) {
          coeff = lines[focusPhase][myIndex-2].Average();
        }
        else {
          coeff = (lines[focusPhase][myIndex-2].Average() +
                   lines[focusPhase][myIndex].Average()) / 2;
        }

        out[i] = Null;
        //Valid coefficient
        if(coeff != 0.0 && IsValidPixel(coeff)) {
          double average = lines[focusPhase][myIndex-1].Average();
          //Apply multiplicative correction
          if(mode == 1) {
            out[i] = (in[i] / average) * coeff;
          }
          //Apply additive correction
          else {
            out[i] = (in[i] - average) + coeff;
          }
        }
      }
    }
  }
  myIndex++;
}

/**
 * Destripe for images which have summing greater than 2
 *
 * @author Sharmila Prasad (12/21/2010)
 *
 * @param piSamples - Image Sample size
 */
void DestripeForOtherBinningModes(int piSamples)
{
  int iBoxSample = (2 * piSamples) - 1;
  QString sSamples(toString(iBoxSample));

  Pipeline p("hidestripe");
  p.SetInputFile("FROM");
  p.SetOutputFile("TO");
  p.KeepTemporaryFiles(false);
  p.AddOriginalBranch("lpf");
  p.AddOriginalBranch("hpf");

  p.AddToPipeline("lowpass");
  p.Application("lowpass").SetInputParameter("FROM", false);
  p.Application("lowpass").SetOutputParameter("TO", "lowpass");
  p.Application("lowpass").EnableBranch("lpf", true);
  p.Application("lowpass").EnableBranch("hpf", false);

  // Set parameters for "lpf" branch
  p.Application("lowpass").AddConstParameter("lpf", "SAMPLES", sSamples);
  p.Application("lowpass").AddConstParameter("lpf", "LINES", "3");
  p.Application("lowpass").AddConstParameter("lpf", "NULL", "FALSE");
  p.Application("lowpass").AddConstParameter("lpf", "HRS",  "FALSE");
  p.Application("lowpass").AddConstParameter("lpf", "HIS",  "FALSE");
  p.Application("lowpass").AddConstParameter("lpf", "LRS",  "FALSE");
  p.Application("lowpass").AddConstParameter("lpf", "LIS",  "FALSE");

  p.AddToPipeline("highpass");
  p.Application("highpass").SetInputParameter("FROM", false);
  p.Application("highpass").SetOutputParameter("TO", "highpass");
  p.Application("highpass").EnableBranch("lpf", false);
  p.Application("highpass").EnableBranch("hpf", true);

  // Set parameters for "hpf" branch
  p.Application("highpass").AddConstParameter("hpf", "SAMPLES", sSamples);
  p.Application("highpass").AddConstParameter("hpf", "LINES", "1");
  p.Application("highpass").AddConstParameter("hpf", "PROPAGATE", "TRUE");

  p.AddToPipeline("fx");
  p.Application("fx").SetInputParameter("FROMLIST", PipelineApplication::LastAppOutputList, false);
  p.Application("fx").SetOutputParameter("TO", "add");
  p.Application("fx").AddConstParameter("MODE", "LIST");
  p.Application("fx").AddConstParameter("EQUATION", "f1+f2");

  //cout << p;

  p.Run();
}
