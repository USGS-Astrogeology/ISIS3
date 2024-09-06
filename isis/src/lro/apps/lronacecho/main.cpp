/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "ProcessByLine.h"
#include "SpecialPixel.h"


using namespace std;
using namespace Isis;

void correct(Buffer &in, Buffer &out);

double g_delta, g_halfDelta, g_smoothingCosntant;
int g_step, g_smoothingRows;

bool g_isLeft, g_isSummed;

void IsisMain() {
  // We will be processing by line
  ProcessByLine p;

  // Setup the input and output files
  UserInterface &ui = Application::GetUserInterface();
  // Setup the input and output cubes
  p.SetInputCube("FROM");
  p.SetOutputCube("TO");

  // Get the coefficient
  g_delta = ui.GetDouble("DELTA");
  g_halfDelta = g_delta/2.0;

  Pvl lab(ui.GetCubeName("FROM").toStdString());
  PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);

  // Check if it is a NAC image
  QString instId =  QString::fromStdString(inst["InstrumentId"]);
  if (instId != "NACL" && instId != "NACR") {
    string msg = "This is not a NAC image. lrocnacecho requires a NAC image.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  if (instId == "NACL")
    g_isLeft = true;
  else
    g_isLeft = false;

  if ((int) inst["SpatialSumming"] == 1){
    g_isSummed = false;
    g_step = 2;
  }
  else {
    g_isSummed = true;
    g_step = 1;
  }

  // Make sure that we aren't passed in a scaled or cropped cub
  if(lab.findObject("IsisCube").hasGroup("AlphaCube")) {
    std::string msg = "This application can not be run on any image that has been geometrically transformed (i.e. scaled, rotated, sheared, or reflected) or cropped.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Pre calulate a constant for the correction's smooth turn on
  g_smoothingRows = ui.GetInteger("SMOOTHINGROWS");
  if (g_isSummed)
      g_smoothingRows /= 2;
  g_smoothingCosntant = PI/(double)g_smoothingRows;

  p.ProcessCube(correct, false);
}

void correct(Buffer &in, Buffer &out) {
  double delta = 0; //<! A delta value used for 'ramping up' the correction at the edge of the image

  if (g_isLeft) {
    for(int i = 0; i < in.size(); i++) {
      if(IsSpecial(in[i]) || IsSpecial(in[i-g_step])) {
        out[i] = in[i];
      }
      // Image starts at Sample 42 for native images
      // Image starts at Sample 21 for summed images
      else if (i >= 21*g_step && i <= (21*g_step) + g_smoothingRows ){
          delta = ((g_halfDelta * (1 - cos(g_smoothingCosntant * (i - 21*g_step) ))));
          //Apply the 'slow' correction
          out[i] = in[i] - (delta * out[i-g_step]);
      }
      else {
          //Apply the 'nominal' correction
          out[i] = in[i] - g_delta * out[i-g_step];
      }
    }
  }
  else {
    for(int i = (in.size() - 1); i >= 0; i--) {

        if(IsSpecial(in[i]) || IsSpecial(in[i+g_step])) {
            out[i] = in[i];
        }

        // Image starts at Sample 5024 for native images
        // Image starts at Sample 2512 for summed images
        else if ((i <=  (2512 * g_step) && (i >= (2512 * g_step) - g_smoothingRows ))){
            delta = (g_halfDelta * (1 - cos(g_smoothingCosntant * ((in.size() - i)  - (21*g_step) ))));
            //Apply the 'slow' correction
            out[i] = in[i] - (delta * out[i+g_step]);
        }
        else {
            out[i] = in[i] - g_delta * out[i+g_step];
        }
    }
  }

  // We must now normalize the entire line
  if (g_isLeft) {
      for(int i = 0; i < out.size(); i++){
          if(IsSpecial(in[i]) || IsSpecial(in[i-g_step])) {
              out[i] = in[i];
          }
          // Image starts at Sample 42 for native images
          // Image starts at Sample 21 for summed images
          else if (i >= 21*g_step && i <= (21*g_step) + g_smoothingRows ){
              delta = ((g_halfDelta * (1 - cos(g_smoothingCosntant * (i - 21*g_step) ))));
              out[i] = (out[i] * (1 + delta));
          }
          else {
              out[i] = (out[i] * (1 + g_delta));
          }
      }
  }
  else {
      for(int i = (in.size() - 1); i >= 0; i--) {
          if(IsSpecial(in[i]) || IsSpecial(in[i+g_step])) {
              out[i] = in[i];
          }
          // Image starts at Sample 5024 for native images
          // Image starts at Sample 2512 for summed images
          else if ((i <=  (2512 * g_step) && (i >= (2512 * g_step) - g_smoothingRows ))){
              delta = (g_halfDelta * (1 - cos(g_smoothingCosntant * ((in.size() - i)  - (21*g_step) ))));
              out[i] = (out[i] * (1 + delta));
          }
          else {
              out[i] = (out[i] * (1 + g_delta));
          }
      }
  }
}
