#include "Isis.h"
#include "ProcessByQuickFilter.h"
#include "UserInterface.h"


using namespace std;
using namespace Isis;

// Globals and prototypes
void RemoveNoiseViaStd(Buffer &in, Buffer &out, QuickFilter &filter);
void RemoveNoiseViaDn(Buffer &in, Buffer &out, QuickFilter &filter);

double tolmin;
double tolmax;
double flattol;
bool nullIsNoise;
bool hisIsNoise;
bool hrsIsNoise;
bool lisIsNoise;
bool lrsIsNoise;
int brightPixelsReplaced;
int darkPixelsReplaced;
int specialPixelsReplaced;
bool replaceWithAverage;

// The noisefilter main routine

void IsisMain() {
  // Open the input cube
  ProcessByQuickFilter p;
  Cube *icube = p.SetInputCube("FROM");

  // Setup the output cube
  p.SetOutputCube("TO");

  //  Read tolerances
  UserInterface &ui = Application::GetUserInterface();
  tolmin = ui.GetDouble("TOLMIN");
  tolmax = ui.GetDouble("TOLMAX");

  //  Will noise pixels be replaced with boxcar average or Null?
  replaceWithAverage = true;
  if(ui.GetString("REPLACE") == "NULL") replaceWithAverage = false;

  // Find out how to handle special pixels
  nullIsNoise = ui.GetBoolean("NULLISNOISE");
  hisIsNoise = ui.GetBoolean("HISISNOISE");
  hrsIsNoise = ui.GetBoolean("HRSISNOISE");
  lisIsNoise = ui.GetBoolean("LISISNOISE");
  lrsIsNoise = ui.GetBoolean("LRSISNOISE");

  // Process each line
  brightPixelsReplaced = 0;
  darkPixelsReplaced = 0;
  specialPixelsReplaced = 0;
  if(ui.GetString("TOLDEF") == "STDDEV") {
    flattol = ui.GetDouble("FLATTOL");
    p.StartProcess(RemoveNoiseViaStd);
  }
  else {
    p.StartProcess(RemoveNoiseViaDn);
  }

  // Generate a results group and log it
  PvlGroup results("Results");
  results += PvlKeyword("DarkPixelsReplaced", Isis::toString(darkPixelsReplaced));
  results += PvlKeyword("BrightPixelsReplaced", Isis::toString(brightPixelsReplaced));
  results += PvlKeyword("SpecialPixelsReplaced", Isis::toString(specialPixelsReplaced));
  int pixelsReplaced = darkPixelsReplaced + brightPixelsReplaced + specialPixelsReplaced;
  results += PvlKeyword("TotalPixelsReplaced", Isis::toString(pixelsReplaced));
  double pct = ((double)pixelsReplaced /
                ((double)icube->sampleCount() * (double)icube->lineCount())) * 100.;
  pct = (int)(pct * 10.0) / 10.0;
  results += PvlKeyword("PercentChanged", Isis::toString(pct));
  Application::Log(results);
  p.EndProcess();
}

// Standard deviation line processing routine
void RemoveNoiseViaStd(Buffer &in, Buffer &out, QuickFilter &filter) {
  for(int i = 0; i < filter.Samples(); i++) {
    // Get the average first and remove the center pixel if possible
    double avg = filter.Average(i);
    double goodAvg = NULL8;
    if((avg != NULL8) && (filter.Count(i) != 1)) {
      double sum = (avg * filter.Count(i)) - in[i];
      goodAvg = sum / (filter.Count(i) - 1.0);
    }

    // Deal with special pixels at the middle of the boxcar
    if(IsSpecial(in[i])) {
      if((IsNullPixel(in[i]) && nullIsNoise) ||
          (IsHisPixel(in[i])  && hisIsNoise)   ||
          (IsHrsPixel(in[i])  && hrsIsNoise)   ||
          (IsLisPixel(in[i])  && lisIsNoise)   ||
          (IsLrsPixel(in[i])  && lrsIsNoise)) {
        out[i] = (replaceWithAverage) ? avg : NULL8;
        specialPixelsReplaced++;
      }
      else {
        out[i] = in[i];
      }
      continue;
    }

    // If the average is NULL or if the input pixel varies from the average by
    // less than the flat tolerance, we can't do anything
    if(goodAvg == NULL8 || (fabs(in[i] - goodAvg) < flattol)) {
      out[i] = in[i];
      continue;
    }

    // Ok lets see if we have noise
    bool noisy = false;
    double diff = in[i] - avg;  // don't use goodAvg here
    double sqrDiff = diff * diff;
    double tol = (diff > 0.0) ? tolmax : tolmin;
    if(sqrDiff > tol * tol * filter.Variance(i)) noisy = true;

    // If we have noise replace it
    if(noisy) {
      out[i] = (replaceWithAverage) ? goodAvg : NULL8;
      (diff > 0.0) ? brightPixelsReplaced++ : darkPixelsReplaced++;
    }

    // Not noisy so copy it
    else {
      out[i] = in[i];
    }
  }

  return;
}

// DN noise filter line processing routine
void RemoveNoiseViaDn(Buffer &in, Buffer &out, QuickFilter &filter) {
  for(int i = 0; i < filter.Samples(); i++) {
    // Get the average first and remove the center pixel if possible
    double avg = filter.Average(i);
    double goodAvg = NULL8;
    if((avg != NULL8) && (filter.Count(i) != 1)) {
      double sum = (avg * filter.Count(i)) - in[i];
      goodAvg = sum / (filter.Count(i) - 1.0);
    }

    // Deal with special pixels at the middle of the boxcar
    if(IsSpecial(in[i])) {
      if((IsNullPixel(in[i]) && nullIsNoise) ||
          (IsHisPixel(in[i])  && hisIsNoise)   ||
          (IsHrsPixel(in[i])  && hrsIsNoise)   ||
          (IsLisPixel(in[i])  && lisIsNoise)   ||
          (IsLrsPixel(in[i])  && lrsIsNoise)) {
        out[i] = (replaceWithAverage) ? avg : NULL8;
        specialPixelsReplaced++;
      }
      else {
        out[i] = in[i];
      }
      continue;
    }

    // If the average is NULL we can't do anything
    if(goodAvg == NULL8) {
      out[i] = in[i];
      continue;
    }

    // Ok lets see if we have noise
    bool noisy = false;
    double diff = in[i] - goodAvg;
    double tol = (diff > 0.0) ? tolmax : tolmin;
    if(abs(diff) > tol) noisy = true;

    // If we have noise replace it
    if(noisy) {
      out[i] = (replaceWithAverage) ? goodAvg : NULL8;
      (diff > 0.0) ? brightPixelsReplaced++ : darkPixelsReplaced++;
    }

    // Not noisy so copy it
    else {
      out[i] = in[i];
    }
  }

  return;
}
