/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <fstream>
#include <iostream>

#include "ControlNet.h"
#include "FileName.h"
#include "NumericalApproximation.h"
#include "ProcessRubberSheet.h"
#include "SlitherTransform.h"
#include "UserInterface.h"
#include "iTime.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  // Warp an image
  ProcessRubberSheet p;

  // Open the input cube
  Cube *icube = p.SetInputCube("FROM");

  // Get the control point file
  UserInterface &ui = Application::GetUserInterface();
  QString cfile = ui.GetFileName("CONTROL");
  ControlNet cn(cfile);

  //  Set default type to Cubic spline interpolation
  NumericalApproximation::InterpType iType(NumericalApproximation::CubicNatural);
  QString splineType = ui.GetString("SPLINE");
  if(splineType == "LINEAR") {
    iType = NumericalApproximation::Linear;
  }
  else if(splineType == "POLYNOMIAL") {
    iType = NumericalApproximation::Polynomial;
  }
  else if(splineType == "AKIMA") {
    iType = NumericalApproximation::Akima;
  }

  // Get the sample and line shifts
  double sampleOffset = ui.GetDouble("SAMPLEOFFSET");
  double lineOffset = ui.GetDouble("LINEOFFSET");

  // Set up the transform object
  SlitherTransform transform(*icube, cn, iType, iType);
  transform.addLineOffset(lineOffset);
  transform.addSampleOffset(sampleOffset);
  QString splineDir = ui.GetString("DIRECTION");
  if(splineDir == "REVERSE") {
    transform.setReverse();
  }

  //  Dump the transform statistics
  if(ui.WasEntered("RESULTS")) {
    // Get the control point file
    QString rFile = QString::fromStdString(FileName(ui.GetFileName("RESULTS").toStdString()).expanded());
    ofstream os;
    os.open(rFile.toLatin1().data(), ios::out);
    os << "#  Slither Transform Results\n"
       << "#  RunDate: " << iTime::CurrentLocalTime().toStdString() << endl
       << "#    FROM:     " << icube->fileName().toStdString() << endl
       << "#    CNETFILE: " << cfile.toStdString() << endl << endl;

    transform.dumpState(os);
  }

  // Allocate the output file, same size as input
  p.SetOutputCube("TO", transform.OutputSamples(),
                  transform.OutputLines(),
                  icube->bandCount());

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
    std::string msg = "Unknown value for INTERP [" +
                 ui.GetString("INTERP").toStdString() + "]";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }

  // Create the output file
  p.StartProcess(transform, *interp);

  // All done!!
  p.EndProcess();
  delete interp;
}
