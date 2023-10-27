// $Id: phohillier.cpp,v 1.1 2010/02/24 04:05:19 kbecker Exp $
#include "Isis.h"

#include <string>
#include <memory>

#include "Cube.h"
#include "Hillier.h"
#include "IException.h"
#include "ProcessByLine.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "SpecialPixel.h"

using namespace std;
using namespace Isis;

// Global variables
Hillier *pho;

void hillier(Buffer &in, Buffer &out);

void IsisMain() {
  // We will be processing by line
  ProcessByLine p;

  // Set up the input cube and get camera information
  Cube *icube = p.SetInputCube("FROM");

  // Create the output cube
  Cube *ocube = p.SetOutputCube("TO");

  // Set up the user interface
  UserInterface &ui = Application::GetUserInterface();
  // Get the name of the parameter file
  Pvl par(ui.GetFileName("PHOPAR").toStdString());
  unique_ptr<Hillier> photom = unique_ptr<Hillier> (new Hillier(par,  *icube));
  pho = photom.get();

  // Start the processing
  p.StartProcess(hillier);

  PvlGroup photo("Photometry");
  pho->Report(photo);
  ocube->putGroup(photo);
  Application::Log(photo);
  p.EndProcess();
}

/**
 * @brief Apply Hillier photometric correction
 *
 * Short function dispatched for each line to apply the Hillier photometrc
 * correction function.
 *
 * @author kbecker (2/20/2010)
 *
 * @param in Buffer containing input data
 * @param out Buffer of photometrically corrected data
 */
void hillier(Buffer &in, Buffer &out) {

  for(int i = 0; i < in.size(); i++) {
    //  Don't correct special pixels
    if(IsSpecial(in[i])) {
      out[i] = in[i];
    }
    else {
      // Get correction and test for validity
      double ph = pho->Compute(in.Line(i), in.Sample(i), in.Band(i));
      out[i] = (IsSpecial(ph) ?  Null : in[i] * ph);
    }
  }
  return;
}

