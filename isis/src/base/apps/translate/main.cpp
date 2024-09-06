#include "Isis.h"
#include "ProcessRubberSheet.h"

#include "translate.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  ProcessRubberSheet p;

  // Open the input cube
  Cube *icube = p.SetInputCube("FROM");

  // Set up the transform object
  UserInterface &ui = Application::GetUserInterface();
  Transform *transform = new Translate(icube->sampleCount(), icube->lineCount(),
                                       ui.GetDouble("STRANS"),
                                       ui.GetDouble("LTRANS"));

  // Allocate the output file, same size as input
  p.SetOutputCube("TO", icube->sampleCount(), icube->lineCount(), icube->bandCount());

  // Set up the interpolator
  Interpolator *interp;
  if(ui.GetString("INTERP") == "NEARESTNEIGHBOR") {
    interp = new Interpolator(Interpolator::NearestNeighborType);
  }
  else if(ui.GetString("INTERP") == "BILINEAR") {
    interp = new Interpolator(Interpolator::BiLinearType);
  }
  else if(ui.GetString("INTERP") == "CUBICCONVOLUTION") {
    interp = new Interpolator(Interpolator::CubicConvolutionType);
  }
  else {
    std::string msg = "Unknow value for INTERP [" +
                  ui.GetString("INTERP") + "]";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }

  p.StartProcess(*transform, *interp);
  p.EndProcess();

  delete transform;
  delete interp;
}

