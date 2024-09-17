/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "FileName.h"
#include "IException.h"
#include "LineManager.h"
#include "ProcessByLine.h"
#include "SpecialPixel.h"
#include "Table.h"

using namespace std;
using namespace Isis;

// Globals and prototypes
Cube g_cube;
LineManager *g_in;

Table hifix("HiRISE Ancillary");
Table calfix("HiRISE Calibration Ancillary");
Table calimg("HiRISE Calibration Image");

bool g_flip;
void glob(Buffer &out);

void IsisMain() {
  ProcessByLine p;

  // Initialize the output size
  int samples = 0;
  int lines = 0;
  int bands = 0;

  // Open the input cube
  UserInterface &ui = Application::GetUserInterface();
  QString from = ui.GetCubeName("FROM");
  g_cube.open(from);

  samples = g_cube.sampleCount();
  lines = g_cube.lineCount();
  bands = g_cube.bandCount();

  // Get a cube packet to the input file
  Cube *icube = p.SetInputCube("FROM");

  // Get the cube prefix and suffix table
  hifix = icube->readTable("HiRISE Ancillary");

  // Get the calibration prefix and suffix table
  calfix = icube->readTable("HiRISE Calibration Ancillary");

  // Get the calibration image table
  calimg = icube->readTable("HiRISE Calibration Image");

  // Add the number of buffer pixels and dark pixels to the ouput NS
  samples += hifix[0]["BufferPixels"].size() + hifix[0]["DarkPixels"].size();

  // Add the number of lines in the calibration image to the output NL
  lines += calimg.Records();

  // Decide if the calibration and observation data should be flipped.
  g_flip = false;
  PvlGroup &ins = icube->group("Instrument");
  int chan = ins["ChannelNumber"];

  IString flipChan = ui.GetString("FLIP").toStdString();
  if (flipChan.UpCase() != "NONE") {
    if (flipChan.ToInteger() == chan) g_flip = true;
  }

  // Allocate the output file and make sure things get propogated nicely
  p.PropagateTables(false);
  p.SetOutputCube("TO", samples, lines, bands);
  p.ClearInputCubes();

  // Create a buffer for reading the input cube
  g_in = new LineManager(g_cube);

  // Crop the input cube
  p.StartProcess(glob);

  // Cleanup
  p.EndProcess();
  delete g_in;
  g_cube.close();
}

// Line processing routine
void glob(Buffer &out) {
  double int2ToDouble(int value);

  // Transfer the calibration buffer, image, and dark pixels
  if (out.Line() <= calimg.Records()) {
    int outPos = 0;

    // Buffer pixels
    vector<int> buf = calfix[out.Line()-1]["BufferPixels"];
    for (int i = 0; i < (int)buf.size(); i++) {
      out[outPos++] = int2ToDouble(buf[i]);
    }

    // Main calibration pixels
    vector<int> cal = calimg[out.Line()-1]["Calibration"];
    if (g_flip) {
      for (int i = (int)cal.size() - 1; i >= 0; i--) {
        out[outPos++] = int2ToDouble(cal[i]);
      }
    }
    else {
      for (int i = 0; i < (int)cal.size(); i++) {
        out[outPos++] = int2ToDouble(cal[i]);
      }
    }

    // Dark pixels
    vector<int> dark = calfix[out.Line()-1]["DarkPixels"];
    for (int i = 0; i < (int)dark.size(); i++) {
      out[outPos++] = int2ToDouble(dark[i]);
    }

  } // End calibration if

  // We are done with the calibration lines so now
  // transfer the image buffer, observation, and dark pixels
  else {
    int outPos = 0;

    // Buffer pixels
    vector<int> buf = hifix[out.Line()-calimg.Records()-1]["BufferPixels"];
    for (int i = 0; i < (int)buf.size(); i++) {
      out[outPos++] = int2ToDouble(buf[i]);
    }

    // Main observation pixels
    g_in->SetLine(out.Line() - calimg.Records(), 1);
    g_cube.read(*g_in);

    if (g_flip) {
      for (int i = g_in->size() - 1; i >= 0; i--) {
        out[outPos++] = (*g_in)[i];
      }
    }
    else {
      for (int i = 0; i < g_in->size(); i++) {
        out[outPos++] = (*g_in)[i];
      }
    }

    // Dark pixels
    vector<int> dark = hifix[out.Line()-calimg.Records()-1]["DarkPixels"];
    for (int i = 0; i < (int)dark.size(); i++) {
      out[outPos++] = int2ToDouble(dark[i]);
    }

  } // End main image else
} // End function

double int2ToDouble(int value) {
  if (value == NULL2) return NULL8;
  else if (value == LOW_REPR_SAT2) return LOW_REPR_SAT8;
  else if (value == LOW_INSTR_SAT2) return LOW_INSTR_SAT8;
  else if (value == HIGH_INSTR_SAT2) return HIGH_INSTR_SAT8;
  else if (value == HIGH_REPR_SAT2) return HIGH_REPR_SAT8;
  else return value;

}
