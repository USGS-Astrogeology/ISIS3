#include "Isis.h"

#include "Cube.h"
#include "LineManager.h"
#include "Pixel.h"
#include "ProcessByLine.h"
#include "Pvl.h"
#include "PvlKeyword.h"
#include "SpecialPixel.h"
#include "SubArea.h"
#include "Table.h"

using namespace std;
using namespace Isis;

int g_minSample, g_maxSample, g_numSamples;
int g_minLine, g_maxLine, g_numLines;
int g_curBand, g_numBands;
bool g_cropNulls, g_cropHrs, g_cropLrs, g_cropHis, g_cropLis;
LineManager *in;
Cube cube;

void findPerimeter(Buffer &in);
void specialRemoval(Buffer &out);


void IsisMain() {
  g_maxSample = 0;
  g_maxLine = 0;
  g_curBand = 1;

  // Open the input cube
  UserInterface &ui = Application::GetUserInterface();
  QString from = ui.GetCubeName("FROM");
  cube.open(from);

  g_cropNulls = ui.GetBoolean("NULL");
  g_cropHrs = ui.GetBoolean("HRS");
  g_cropLrs = ui.GetBoolean("LRS");
  g_cropHis = ui.GetBoolean("HIS");
  g_cropLis = ui.GetBoolean("LIS");

  g_minSample = cube.sampleCount() + 1;
  g_minLine = cube.lineCount() + 1;
  g_numBands = cube.bandCount();

  // Setup the input cube
  ProcessByLine p1;
  p1.SetInputCube("FROM");
  p1.Progress()->SetText("Finding Perimeter");

  // Start the first pass
  p1.StartProcess(findPerimeter);
  p1.EndProcess();

  if (g_minSample == cube.sampleCount() + 1) {
    cube.close();
    std::string msg = "There are no valid pixels in the [FROM] cube";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  g_numSamples = g_maxSample - (g_minSample - 1);
  g_numLines = g_maxLine - (g_minLine - 1);

  // Setup the output cube
  ProcessByLine p2;
  p2.SetInputCube("FROM");
  p2.PropagateTables(false);
  p2.Progress()->SetText("Removing Special Pixels");
  Cube *ocube = p2.SetOutputCube("TO", g_numSamples, g_numLines, g_numBands);
  p2.ClearInputCubes();

  // propagate tables manually
  Pvl &inLabels = *cube.label();

  // Loop through the labels looking for object = Table
  for (int labelObj = 0; labelObj < inLabels.objects(); labelObj++) {
    PvlObject &obj = inLabels.object(labelObj);

    if (obj.name() != "Table") continue;

    // Read the table into a table object
    Table table(obj["Name"], from);

    ocube->write(table);
  }

  // Construct a label with the results
  PvlGroup results("Results");
  results += PvlKeyword("InputLines", toString(cube.lineCount()));
  results += PvlKeyword("InputSamples", toString(cube.sampleCount()));
  results += PvlKeyword("StartingLine", toString(g_minLine));
  results += PvlKeyword("StartingSample", toString(g_minSample));
  results += PvlKeyword("EndingLine", toString(g_maxLine));
  results += PvlKeyword("EndingSample", toString(g_maxSample));
  results += PvlKeyword("OutputLines", toString(g_numLines));
  results += PvlKeyword("OutputSamples", toString(g_numSamples));

  // Create a buffer for reading the input cube
  in = new LineManager(cube);

  // Start the second pass
  p2.StartProcess(specialRemoval);

  // Update the Mapping, Instrument, and AlphaCube groups in the output
  // cube label
  SubArea s;
  s.SetSubArea(cube.lineCount(), cube.sampleCount(), g_minLine, g_minSample, g_minLine + g_numLines - 1,
               g_minSample + g_numSamples - 1, 1.0, 1.0);
  s.UpdateLabel(&cube, ocube, results);

  p2.EndProcess();
  cube.close();

  delete in;
  in = NULL;

  // Write the results to the log
  Application::Log(results);
}

// Process each line to find the min and max lines and samples
void findPerimeter(Buffer &in) {
  for (int i = 0; i < in.size(); i++) {
    // Do nothing until we find a valid pixel, or a pixel we do not want to crop off
    if ( (Pixel::IsValid(in[i])) || (Pixel::IsNull(in[i]) && !g_cropNulls) ||
        (Pixel::IsHrs(in[i]) && !g_cropHrs) || (Pixel::IsLrs(in[i]) && !g_cropLrs) ||
        (Pixel::IsHis(in[i]) && !g_cropHis) || (Pixel::IsLis(in[i]) && !g_cropLis) ) {
      // The current line has a valid pixel and is greater than max, so make it the new max line
      if (in.Line() > g_maxLine) g_maxLine = in.Line();

      // This is the first line to contain a valid pixel, so it's the min line
      if (in.Line() < g_minLine) g_minLine = in.Line();

      int cur_sample = i + 1;

      // We process by line, so the min sample is the valid pixel with the lowest index in all line arrays
      if (cur_sample < g_minSample) g_minSample = cur_sample;

      // Conversely, the max sample is the valid pixel with the highest index
      if (cur_sample > g_maxSample) g_maxSample = cur_sample;
    }
  }
}

// Using the min and max values, create a new cube without the extra specials
void specialRemoval(Buffer &out) {
  int iline = g_minLine + (out.Line() - 1);
  in->SetLine(iline, g_curBand);
  cube.read(*in);

  // Loop and move appropriate samples
  for (int i = 0; i < out.size(); i++) {
    out[i] = (*in)[(g_minSample - 1) + i];
  }

  if (out.Line() == g_numLines) g_curBand++;
}
