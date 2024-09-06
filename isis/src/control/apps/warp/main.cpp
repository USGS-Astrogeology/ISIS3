/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <fstream>
#include <iostream>

#include "ProcessRubberSheet.h"
#include "WarpTransform.h"
#include "PolynomialBivariate.h"
#include "UserInterface.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  // Warp an image
  ProcessRubberSheet p;

  // Get the control point file
  UserInterface &ui = Application::GetUserInterface();
  QString cfile = ui.GetFileName("CNET");
  ControlNet cn(cfile);

  std::vector<double> inputLine, inputSample, outputLine, outputSample;
  for (int i = 0; i < cn.GetNumPoints(); i++) {
    const ControlPoint *cp = cn.GetPoint(i);
    if (cp->GetNumMeasures() != 2) {
      std::string msg = "Control points must have exactly 2 control measures";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    if (!cp->IsIgnored()) {
      inputLine.push_back(cp->GetMeasure(0)->GetLine());
      inputSample.push_back(cp->GetMeasure(0)->GetSample());
      outputLine.push_back(cp->GetMeasure(1)->GetLine());
      outputSample.push_back(cp->GetMeasure(1)->GetSample());
    }
  }

  // If there are no valid control points,
  //  throw an error
  if (inputLine.size() < 1) {
    std::string msg = "The specified Control Network is empty.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Open the input cube
  Cube *icube = p.SetInputCube("FROM");

  // Determine the size of the output cube
  int onl, ons;
  if (ui.GetString("OSIZE") == "MATCH") {
    Cube c;
    c.open(ui.GetCubeName("CUBE"), "r");
    onl = c.lineCount();
    ons = c.sampleCount();
    c.close();
  }
  else if (ui.GetString("OSIZE") == "COMPUTE") {
    onl = 0;
    ons = 0;
  }
  else {
    onl = ui.GetInteger("ONL");
    ons = ui.GetInteger("ONS");
  }

  // Create the basis function for transforming
  int degree = ui.GetInteger("DEGREE");
  BasisFunction *basisLine = new PolynomialBivariate(degree);
  BasisFunction *basisSamp = new PolynomialBivariate(degree);
  bool weighted = ui.GetBoolean("WEIGHTED");

  // Set up the transform object
  WarpTransform *transform = new WarpTransform(*basisLine, *basisSamp, weighted,
      inputLine, inputSample,
      outputLine, outputSample,
      icube->lineCount(), icube->sampleCount(),
      onl, ons);

  // Allocate the output file, same size as input
  p.SetOutputCube("TO", transform->OutputSamples(),
                  transform->OutputLines(),
                  icube->bandCount());

  // Set up the interpolator
  Interpolator *interp;
  if (ui.GetString("INTERP") == "NEARESTNEIGHBOR") {
    interp = new Interpolator(Interpolator::NearestNeighborType);
  }
  else if (ui.GetString("INTERP") == "BILINEAR") {
    interp = new Interpolator(Interpolator::BiLinearType);
  }
  else if (ui.GetString("INTERP") == "CUBICCONVOLUTION") {
    interp = new Interpolator(Interpolator::CubicConvolutionType);
  }
  else {
    std::string msg = "Unknow value for INTERP [" +
                  ui.GetString("INTERP") + "]";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }

  p.StartProcess(*transform, *interp);
  PvlGroup results = transform->Residuals();
  Application::Log(results);
  p.EndProcess();

  delete transform;
  delete interp;
  delete basisLine;
  delete basisSamp;
}
