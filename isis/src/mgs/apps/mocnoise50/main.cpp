/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"
#include "ProcessByLine.h"
#include "SpecialPixel.h"
#include "IException.h"
#include "IException.h"
#include "Pvl.h"
#include "MocLabels.h"

using namespace std;
using namespace Isis;

namespace gbl {
  const int spikeInterval(50);
  int averageWidth;
  vector<int> maxNoiseColumn;
  vector<double> column1Norm;
  vector<double> column2Norm;
  vector<double> column3Norm;
  int delta;
  int ssFirst;
  double avg1, avg2, avg3;

  extern void CollectColumnStats(Buffer &in);
  extern void Copy(Buffer &in, Buffer &out);
  extern void RemoveNoise(Buffer &in, Buffer &out);
}

void IsisMain() {
  // Make sure we have a moc cube.  If not spit out an appropriate error
  // message
  ProcessByLine p;
  Cube *icube = p.SetInputCube("FROM", OneBand);
  MocLabels moc(Application::GetUserInterface().GetCubeName("FROM"));
  int nlines = icube->lineCount();

  // Must be narrow angle
  if(moc.WideAngle()) {
    QString msg = "The 50 sample noise pattern does not occur in ";
    msg += "MOC wide angle images";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Must have crosstrack summing of 1
  if(moc.CrosstrackSumming() != 1) {
    QString msg = "The 50 sample noise pattern does not occur in ";
    msg += "MOC narrow angle images with crosstrack summing greater than one";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Initializations
  gbl::maxNoiseColumn.clear();
  gbl::column1Norm.clear();
  gbl::column2Norm.clear();
  gbl::column3Norm.clear();
  gbl::averageWidth = Application::GetUserInterface().GetInteger("WIDTH");

  // Now collect columnar statistical information about the noise
  // on a line-by-line basis
  p.Progress()->SetText("Collecting statistics");
  p.StartProcess(gbl::CollectColumnStats);
  p.EndProcess();

  // Compute a histogram of the line-by-line difference in max noise column
  const int nhist = gbl::spikeInterval * 2 + 1;
  int noiseHist[nhist];

  for(int i = 0; i < nhist; i++) {
    noiseHist[i] = 0;
  }

  for(int line = 0; line < nlines - 1; line++) {
    int index = gbl::maxNoiseColumn[line+1] - gbl::maxNoiseColumn[line];
    if(abs(index) > abs(gbl::spikeInterval + index)) {
      index = gbl::spikeInterval + index;
    }
    index += gbl::spikeInterval;

    noiseHist[index]++;
  }

  // Use the histogram to compute the delta change in noise position
  // between lines
  int noiseMode = 0;
  gbl::delta = 0;
  for(int i = 0; i < nhist; i++) {
    if(noiseHist[i] > noiseMode) {
      noiseMode = noiseHist[i];
      gbl::delta = i - gbl::spikeInterval;
    }
  }

  // Compute a histogram of the max noise column
  int ssHist[gbl::spikeInterval];
  for(int i = 0; i < gbl::spikeInterval; i++) ssHist[i] = 0;

  for(int line = 0; line < nlines; line++) {
    int imod = gbl::maxNoiseColumn[line] - line * gbl::delta;
    if(imod < 0) {
      imod += gbl::spikeInterval * (1 + (-imod / gbl::spikeInterval));
    }
    else if(imod > gbl::spikeInterval) {
      imod -= gbl::spikeInterval * (imod / gbl::spikeInterval);
    }

    ssHist[imod]++;
  }

  // Use the histogram to compute the starting sample for the first line
  gbl::ssFirst = ssHist[0];
  int ssMode = 0;
  for(int i = 0; i < gbl::spikeInterval; i++) {
    if(ssHist[i] > ssMode) {
      ssMode = ssHist[i];
      gbl::ssFirst = i + 1;
    }
  }

  // Compute the noise correction values
  double sum1 = 0.0;
  double sum2 = 0.0;
  double sum3 = 0.0;
  for(int line = 0; line < nlines; line++) {
    sum1 += gbl::column1Norm[line];
    sum2 += gbl::column2Norm[line];
    sum3 += gbl::column3Norm[line];
  }

  gbl::avg1 = sum1 / nlines;
  gbl::avg2 = sum2 / nlines;
  gbl::avg3 = sum3 / nlines;

  // Prep to remove noise from the image
  p.SetInputCube("FROM", OneBand);
  p.SetOutputCube("TO");

  PvlGroup results("Results");
  results += PvlKeyword("DeltaSample", std::to_string(gbl::delta));
  results += PvlKeyword("StartingSample", std::to_string(gbl::ssFirst));
  results += PvlKeyword("Coefficient1", std::to_string(gbl::avg1));
  results += PvlKeyword("Coefficient2", std::to_string(gbl::avg2));
  results += PvlKeyword("Coefficient3", std::to_string(gbl::avg3));

  // If less than 50% of the lines do not agree on a delta then
  // we will assume no noise so just make a copy
  if(noiseMode <= nlines / 2) {
    p.Progress()->SetText("Copying cube");
    p.StartProcess(gbl::Copy);
    results += PvlKeyword("NoiseRemoved", "No");
    QString reason = "Less than 50% of the lines agreed on a delta sample";
    results += PvlKeyword("Reason", reason.toStdString());
  }

  // If less than 50% of the lines do not agree on a first same then
  // we will assume no noise so just make a copy
  else if(ssMode <= nlines / 2) {
    p.Progress()->SetText("Copying cube");
    p.StartProcess(gbl::Copy);
    results += PvlKeyword("NoiseRemoved", "No");
    QString reason = "Less than 50% of the lines agreed on a starting sample";
    results += PvlKeyword("Reason", reason.toStdString());
  }

  // Remove that noise
  else {
    p.Progress()->SetText("Removing noise");
    p.StartProcess(gbl::RemoveNoise);
    results += PvlKeyword("NoiseRemoved", "Yes");
  }

  // Log information
  Application::Log(results);
  p.EndProcess();
}


// Collect statistics about columns within a line
void gbl::CollectColumnStats(Buffer &in) {
  // Initialize column arrays.
  double columnSum[gbl::spikeInterval];
  double columnAverage[gbl::spikeInterval];
  double columnNorm[gbl::spikeInterval];
  int columnCount[gbl::spikeInterval];

  for(int i = 0; i < gbl::spikeInterval; i++) {
    columnSum[i] = 0.0;
    columnAverage[i] = 0.0;
    columnCount[i] = 0;
  }

  // Loop and get sum and count for each 50 pixel column
  int ncols = in.size() / gbl::spikeInterval;
  for(int j = 0; j < ncols; j++) {
    for(int i = 0; i < gbl::spikeInterval; i++) {
      int index = i + j * gbl::spikeInterval;
      if(index < in.size()) {
        if(IsValidPixel(in[index]) && (in[index] > 0.0)) {
          columnSum[i] += in[index];
          columnCount[i]++;
        }
      }
    }
  }

  // Compute the average of the columns
  for(int i = 0; i < gbl::spikeInterval; i++) {
    if(columnCount[i] != 0) {
      columnAverage[i] = columnSum[i] / columnCount[i];
    }
  }

  // Compute the average of the line
  double sum = 0.0;
  for(int i = 0; i < gbl::spikeInterval; i++) {
    sum += columnAverage[i];
  }
  double avg = sum / gbl::spikeInterval;

  // Normalize the column averages
  for(int i = 0; i < gbl::spikeInterval; i++) {
    columnNorm[i] = columnAverage[i] - avg;
  }

  // Find column with the maximum peak noise and save it for the line
  int column = 0;
  double maxNoise = -100000.0;
  for(int i = 0; i < gbl::spikeInterval; i++) {
    // look at three adjacent columns
    int col1 = i - 1;
    int col2 = i;
    int col3 = i + 1;

    // Make sure we stay in array bounds
    if(col1 < 0) col1 = gbl::spikeInterval - 1;
    if(col3 > (gbl::spikeInterval - 1)) col3 = 0;

    // Compute the change around the center column and see if it is a max
    double change = (columnNorm[col2] - columnNorm[col1]) +
                    (columnNorm[col2] - columnNorm[col3]);
    if(change > maxNoise) {
      maxNoise = change;
      column = col2;
    }
  }
  gbl::maxNoiseColumn.push_back(column);

  // Compute average of column normals surrounding this noise spike
  sum = 0.0;
  for(int i = 0; i < gbl::averageWidth; i++) {
    int col1 = column - 2 - i;
    if(col1 < 0) col1 = gbl::spikeInterval + col1;
    sum += columnNorm[col1];

    int col2 = column + 2 + i;
    if(col2 > gbl::spikeInterval - 1) col2 = col2 - gbl::spikeInterval;
    sum += columnNorm[col2];
  }
  avg = sum / (double)(2 * gbl::averageWidth);

  //  Calculate difference average surrounding the spike and the three
  //  columns within the spike.
  int col1 = column - 1;
  int col2 = column;
  int col3 = column + 1;

  if(col1 < 0) col1 = gbl::spikeInterval - 1;
  if(col3 > (gbl::spikeInterval - 1)) col3 = 0;

  gbl::column1Norm.push_back(columnNorm[col1] - avg);
  gbl::column2Norm.push_back(columnNorm[col2] - avg);
  gbl::column3Norm.push_back(columnNorm[col3] - avg);
}

// Remove the noise
void gbl::RemoveNoise(Buffer &in, Buffer &out) {
  // First copy the entire line to the output
  for(int i = 0; i < in.size(); i++) {
    out[i] = in[i];
  }

  // Now compute the starting sample of the noise for this line
  int ss = gbl::ssFirst - 1 + gbl::delta * (in.Line() - 1);
  if(ss > gbl::spikeInterval) {
    ss = ss - ((ss / gbl::spikeInterval) * gbl::spikeInterval);
  }
  if(ss < 0) {
    ss = gbl::spikeInterval + ss -
         ((ss / gbl::spikeInterval) * gbl::spikeInterval);
  }

  // Loop and remove the noise in each column
  int ncols = in.size() / gbl::spikeInterval;
  for(int i = 0; i <= ncols; i++) {
    int samp2 = ss + i * gbl::spikeInterval;
    int samp1 = samp2 - 1;
    int samp3 = samp2 + 1;

    if((samp1 > 0) && (samp1 < in.size())) {
      if((IsValidPixel(in[samp1])) && (in[samp1] > 0)) {
        out[samp1] = in[samp1] - gbl::avg1;
      }
    }

    if((samp2 > 0) && (samp2 < in.size())) {
      if((IsValidPixel(in[samp2])) && (in[samp2] > 0)) {
        out[samp2] = in[samp2] - gbl::avg2;
      }
    }

    if((samp3 > 0) && (samp3 < in.size())) {
      if((IsValidPixel(in[samp3])) && (in[samp3] > 0)) {
        out[samp3] = in[samp3] - gbl::avg3;
      }
    }
  }
}

// Just copy each line
void gbl::Copy(Buffer &in, Buffer &out) {
  for(int i = 0; i < in.size(); i++) {
    out[i] = in[i];
  }
}
