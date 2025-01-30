/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"
#include "ProcessByLine.h"
#include "SpecialPixel.h"
#include <string>

using namespace std;
using namespace Isis;

void fixtrx(Buffer &in, Buffer &out);
static float threshhold;

void IsisMain() {
  // We will be processing by line
  ProcessByLine p;

  // Get the user entered threshhold value
  UserInterface &ui = Application::GetUserInterface();
  threshhold = ui.GetDouble("THRESHLD");

  // Setup the input and output cubes
  p.SetInputCube("FROM");
  p.SetOutputCube("TO");

  // Start the processing
  p.StartProcess(fixtrx);
  p.EndProcess();

}

void fixtrx(Buffer &in, Buffer &out) {

  static int nsamps = in.SampleDimension();

  // Initialize bad tracks to false
  int trk, badtrx[7];
  for(trk = 0; trk < 7; trk++) {
    badtrx[trk] = false;
  }

  // Copy input line to output line
  for(int samp = 0; samp < nsamps; samp++) {
    out[samp] = in[samp];
  }

  // Check for bad tracks in this line
  int isamp, nbad, ntrx;
  for(trk = 0; trk < 7; trk++) {
    for(isamp = trk, nbad = 0, ntrx = 0; isamp < nsamps;
        isamp += 7, ntrx++) {
      if(in[isamp] <= 0) nbad++;
    }
    float pcbad = (float) nbad / (float) ntrx * 100.0;
    if(pcbad >= threshhold) {
      badtrx[trk] = true;
    }
  }

  // Go fix the bad tracks
  for(trk = 0; trk < 7; trk++) {
    if(badtrx[trk] == true) {
      for(isamp = trk; isamp < nsamps; isamp += 7) {
        if(in[isamp] <= 0) {

          // Get the left sample to average
          int lsamp = isamp;
          while(in[lsamp] <= 0 || in[lsamp] >= 255) {
            lsamp--;
            if(lsamp < isamp - 3 || lsamp < 0) {
              lsamp = -1;
              break;
            }
          }

          // Get the right sample to average
          int rsamp = isamp;
          while(in[rsamp] <= 0 || in[rsamp] >= 255) {
            rsamp++;
            if(rsamp > isamp + 3 || rsamp > nsamps - 1) {
              rsamp = -1;
              break;
            }
          }

          // Calculate the output pixel value
          double lweight, rweight, value, weight;
          if(lsamp >= 0 && rsamp >= 0) {
            lweight = rsamp - isamp;
            rweight = isamp - lsamp;
            weight = rsamp - lsamp;
            value = (in[lsamp] * lweight + in[rsamp] * rweight)
                    / weight + 0.5;
            out[isamp] = (unsigned char) value;
          }
        }
      }
    }
  }
}
