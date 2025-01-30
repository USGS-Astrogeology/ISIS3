/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <cmath>

#include "ProcessByBrick.h"
#include "Pixel.h"
#include "IException.h"

using namespace std;
using namespace Isis;

void NoiseFilter(Buffer &in, Buffer &out);

void IsisMain() {
  // We will be processing by brick
  ProcessByBrick p;
// UserInterface &ui = Application::GetUserInterface();

  // Setup the input and output cubes
  Cube *icube = p.SetInputCube("FROM");
  p.SetOutputCube("TO");

  // Start the processing
  p.SetBrickSize(icube->sampleCount(), icube->lineCount(), 1);
  p.StartProcess(NoiseFilter);
  p.EndProcess();
}

void NoiseFilter(Buffer &in, Buffer &out) {
  double TOL1 = 9.0;
  double TOL2 = 3.0;
  int NPOS = 100;
  double diff;
  double diffSum;
  double diffAvg;
  int dCount;

  for(int il = 0; il < in.LineDimension(); il++) {
    for(int is = 0; is < in.SampleDimension(); is++) {
      int index = in.SampleDimension() * il + is;
      out[index] = in[index];
    }
    // Determine if a noise pattern exists samples 2,6,10,14,...
    diffSum = 0.0;
    dCount = 0;
    for(int is = 1; is < in.SampleDimension() - 4; is += 4) {
      int index = in.SampleDimension() * il + is;
      if(!Pixel::IsSpecial(in[index]) &&
          !Pixel::IsSpecial(in[index]) &&
          !Pixel::IsSpecial(in[index])) {
        diff = abs((in[index] - in[index-1]) + (in[index] - in[index+1]));
        diffSum += diff;
        dCount += 2;
      }
    }
    diffAvg = 0.0;
    if(dCount > 1) {
      diffAvg = diffSum / dCount;
    }
    if(diffAvg > TOL1) {
      for(int is = 1; is < in.SampleDimension() - 4; is += 4) {
        int index = in.SampleDimension() * il + is;
        out[index] = Isis::Null;
      }
    }

    // Determine if a noise pattern exists samples 4,8,12,16,...
    diffSum = 0.0;
    dCount = 0;
    for(int is = 4; is < in.SampleDimension() - 4; is += 4) {
      int index = in.SampleDimension() * il + is;
      if(!Pixel::IsSpecial(in[index]) &&
          !Pixel::IsSpecial(in[index]) &&
          !Pixel::IsSpecial(in[index])) {
        diff = (in[index] - in[index-1]) + (in[index] - in[index+1]);
        if(diff < 0.0) {
          diffSum += diff;
          dCount += 2;
        }
      }
    }
    diffAvg = 0.0;
    if(dCount > 1) {
      diffAvg = diffSum / dCount;
    }
    if(diffAvg > TOL2 && dCount > NPOS) {
      for(int is = 4; is < in.SampleDimension() - 4; is += 4) {
        int index = in.SampleDimension() * il + is;
        out[index] = Isis::Null;
      }
    }
  }
}
