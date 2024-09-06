#include "Isis.h"

#include <float.h>
#include <cmath>

#include "ProcessByLine.h"
#include "LineManager.h"

using namespace std;
using namespace Isis;

void lowpass(Buffer &in, Buffer &out);
void highpass(Buffer &in, Buffer &out);
void bandpass(Buffer &in, Buffer &out);
void bandstop(Buffer &in, Buffer &out);
double radius(int x1, int y1, int x2, int y2);

int x, y, g;
double d, dw;

void IsisMain() {
  ProcessByLine p;

  UserInterface &ui = Application::GetUserInterface();

  //  Initialize the input and output cubes
  Isis::Cube *icube = p.SetInputCube("FROM");

  // get the center pixels coordinates
  x = (icube->sampleCount() + 1) / 2;
  y = (icube->lineCount() + 1) / 2;

  p.SetOutputCube("TO");

  // Get the input values
  d = ui.GetDouble("CUTOFF");
  dw = ui.GetDouble("BANDWIDTH");
  g = ui.GetInteger("ORDER");

  // checks the type and runs the appropriate filter
  if(ui.GetString("TYPE") == "LOWPASS") {
    p.StartProcess(lowpass);
  }
  else if(ui.GetString("TYPE") == "HIGHPASS") {
    p.StartProcess(highpass);
  }
  else if(ui.GetString("TYPE") == "BANDPASS") {
    p.StartProcess(bandpass);
  }
  else if(ui.GetString("TYPE") == "BANDSTOP") {
    p.StartProcess(bandstop);
  }
  else {
    std::string msg = "Unknow value for TYPE [" +
                  ui.GetString("TYPE") + "]";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }

  p.EndProcess();
}

// Applies a lowpass filter to an image in the frequency domain.
void lowpass(Buffer &in, Buffer &out) {
  double dist = 0.0;
  double B = 0.0;

  for(int i = 0; i < in.size(); i++) {
    dist = radius(in.Sample(i), in.Line(i), x, y);
    B = 1 / (1 + pow(dist / d, 2 * g));

    out[i] = B * in[i];
  }
}

// Applies a highpass filter to an image in the frequency domain.
void highpass(Buffer &in, Buffer &out) {
  double dist = 0.0;
  double B = 0.0;

  for(int i = 0; i < in.size(); i++) {
    dist = radius(in.Sample(i), in.Line(i), x, y);
    B = 1 / (1 + pow(d / dist, 2 * g));

    out[i] = B * in[i];
  }
}

// Applies a bandpass filter to an image in the frequency domain.
void bandpass(Buffer &in, Buffer &out) {
  double dist = 0.0;
  double B = 0.0;

  for(int i = 0; i < in.size(); i++) {
    dist = radius(in.Sample(i), in.Line(i), x, y);
    B = 1 - 1 / (1 + pow(dw * dist / (dist * dist - d * d), 2 * g));

    out[i] = B * in[i];
  }
}

// Applies a bandstop filter to an image in the frequency domain.
void bandstop(Buffer &in, Buffer &out) {
  double dist = 0.0;
  double B = 0.0;

  for(int i = 0; i < in.size(); i++) {
    dist = radius(in.Sample(i), in.Line(i), x, y);
    B = 1 / (1 + pow(dw * dist / (dist * dist - d * d), 2 * g));

    out[i] = B * in[i];
  }
}

// measures the distance (or radius lenght) between the
// point (x1, y1) and (x2,y2)
double radius(int x1, int y1, int x2, int y2) {
  return sqrt((double)((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)));
}

