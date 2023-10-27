#include "Isis.h"
#include "ProcessMosaic.h"
#include "ProcessByLine.h"
#include "AlphaCube.h"
#include "SpecialPixel.h"
#include "Projection.h"
#include "SubArea.h"

using namespace std;
using namespace Isis;

void CreateBase(Buffer &buf);

void IsisMain() {
  // We will be use a mosaic technique so get the size of the input file
  ProcessMosaic p;
  Cube *icube = p.SetInputCube("FROM", 1, 1, 1, -1, -1, -1);
  int ins = icube->sampleCount();
  int inl = icube->lineCount();
  int inb = icube->bandCount();

  // Retrieve the padding parameters
  UserInterface &ui = Application::GetUserInterface();
  int leftPad = ui.GetInteger("LEFT");
  int rightPad = ui.GetInteger("RIGHT");
  int topPad = ui.GetInteger("TOP");
  int bottomPad = ui.GetInteger("BOTTOM");

  // Compute the output size
  int ns = ins + leftPad + rightPad;
  int nl = inl + topPad + bottomPad;
  int nb = inb;

  // We need to create the output file
  ProcessByLine bl;
  bl.SetInputCube("FROM");  // Do this to match pixelType
  bl.SetOutputCube("TO", ns, nl, nb);
  bl.ClearInputCubes();     // Now get rid of it
  bl.Progress()->SetText("Creating pad");
  bl.StartProcess(CreateBase);
  bl.EndProcess();

  // Place the input in the file we just created
  Cube *ocube = p.SetOutputCube("TO");
  p.Progress()->SetText("Inserting cube");
  p.SetImageOverlay(ProcessMosaic::PlaceImagesOnTop);
  p.SetBandBinMatch(false);
  p.StartProcess(leftPad + 1, topPad + 1, 1);

  // Construct a label with the results
  PvlGroup results("Results");
  results += PvlKeyword("InputLines", std::to_string(inl));
  results += PvlKeyword("InputSamples", std::to_string(ins));
  results += PvlKeyword("LeftPad", std::to_string(leftPad));
  results += PvlKeyword("RightPad", std::to_string(rightPad));
  results += PvlKeyword("TopPad", std::to_string(topPad));
  results += PvlKeyword("BottomPad", std::to_string(bottomPad));
  results += PvlKeyword("OutputLines", std::to_string(nl));
  results += PvlKeyword("OutputSamples", std::to_string(ns));

  // Update the Mapping, Instrument, and AlphaCube groups in the
  // output cube label
  SubArea s;
  s.SetSubArea(inl, ins, 1 - topPad, 1 - leftPad, inl + bottomPad, ins + rightPad, 1.0, 1.0);
  s.UpdateLabel(icube, ocube, results);

  p.EndProcess();

  // Write the results to the log
  Application::Log(results);
}

void CreateBase(Buffer &buf) {
  for(int i = 0; i < buf.size(); i++) {
    buf[i] = NULL8;
  }
}
