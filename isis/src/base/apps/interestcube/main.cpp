#include "Isis.h"
#include "InterestOperator.h"
#include "InterestOperatorFactory.h"
#include "ProcessByBrick.h"
#include "Pixel.h"
#include "Cube.h"
#include "IException.h"

using namespace std;
using namespace Isis;

void Operate(Isis::Buffer &in, Isis::Buffer &out);

InterestOperator *iop;
Cube cube;
int boxcarLines, boxcarSamples;
UniversalGroundMap *unvGMap;

void IsisMain() {
  // We will be processing by brick
  ProcessByBrick p;
  p.SetBrickSize(1, 1, 1);
  p.SetOutputBrickSize(1, 1, 1);
  UserInterface &ui = Application::GetUserInterface();
  
  //initialize the global variables
  iop = NULL;
  boxcarLines = 0;
  boxcarSamples = 0;
  unvGMap = NULL;
  if (cube.isOpen()) {
    cube.close();
  }
  
  // Basic settings
  p.SetInputCube("FROM");
  p.SetOutputCube("TO");
  Pvl pvl = Pvl(ui.GetFileName("PVL").toStdString());

  cube.open(ui.GetCubeName("FROM"));

  try {
    // Get info from the operator group
    // Set the pvlkeywords that need to be set to zero
    PvlGroup &op = pvl.findGroup("Operator", Pvl::Traverse);
    boxcarSamples = op["Samples"];
    boxcarLines = op["Lines"];
    op["DeltaLine"] = "0";
    op["DeltaSamp"] = "0";
    op["MinimumInterest"] = "0.0";
    Application::Log(op);
    
  }
  catch(IException &e) {
    cube.close();
    std::string msg = "Improper format for InterestOperator PVL [" + pvl.fileName() + "]";
    throw IException(e, IException::User, msg, _FILEINFO_);
    
  }

  iop = InterestOperatorFactory::Create(pvl);

  // Get the universal ground map of this cube
  unvGMap = new UniversalGroundMap(cube);

  // Start the processing
  p.StartProcess(Operate);
  p.EndProcess();

  delete unvGMap;
  cube.close();
}

// Call Operate once per pixel to get the interest for every pixel in the input cube.
void Operate(Isis::Buffer &in, Isis::Buffer &out) {
  try {
    int sample = in.Sample();
    int line = in.Line();

    iop->Operate(cube, *unvGMap, sample, line);

    out[0] = iop->InterestAmount();

  }
  catch(IException &e) {
    e.print();
  }
}
