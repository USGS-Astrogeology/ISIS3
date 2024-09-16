/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "Cube.h"
#include "CubeAttribute.h"
#include "IException.h"
#include "IString.h"
#include "ProcessByLine.h"
#include "Pvl.h"
#include "SpecialPixel.h"

#include <vector>

using namespace std;
using namespace Isis;

void clemhirescal(vector <Buffer *> &in, vector <Buffer *> &out);

double offset;
double abscoef;

void IsisMain() {
  // Open the input cube
  ProcessByLine p;
  Cube *input = p.SetInputCube("FROM");

  // Check for filter type of A-D
  Pvl *label = input->label();
  QString wave = QString::fromStdString(label->findGroup("BandBin", Pvl::Traverse)["FilterName"]);
  if((wave != "A") && (wave != "B") && (wave != "C") && (wave != "D")) {
    std::string message = "Invalid FilterName [" + wave.toStdString() + "], can only handle A-D filters";
    throw IException(IException::Unknown, message, _FILEINFO_);
  }
  // Determine and load calibration flat field file
  wave = wave.toLower();
  QString flatFile("$Clementine1/calibration/hires/lh" +
                   wave + "_flat.cub");
  CubeAttributeInput cubeAtt;
  p.SetInputCube(flatFile, cubeAtt);

  // Check the offset mode for validity
  int index = label->findGroup("Instrument", Pvl::Traverse)["OffsetModeID"];
  if(index < 0 || index > 5) {
    std::string message = "Invalid OffsetModeID, can only handle offests 0-5";
    throw IException(IException::Unknown, message, _FILEINFO_);
  }

  // Set the offset (b0) value based on mode
  double dataOffset[] = { -49.172, -41.0799, -32.8988, -24.718, -16.98, -8.0};
  offset = dataOffset[index];

  // Compute the K value to convert to I/F. The functions for K value per MCP
  //  gain state and filter are based on Robinson, M. S., Malaret, E.,
  //  and White, T. ( 2003), A radiometric calibration for the Clementine HIRES
  //  camera, J. Geophys. Res., 108, 5028, doi:10.1029/2000JE001241, E4.
  //  The functions were determined by fitting a line to the data in Table 5 (A filter)
  //  or Table 6 (D filter) of Robinson et al., as described on page 11.
  UserInterface &ui = Application::GetUserInterface();
  if(ui.GetString("KFROM").compare("COMPUTED") == 0) {
    wave = wave.toUpper();
    int MCP = label->findGroup("Instrument", Pvl::Traverse)["MCPGainModeID"];
    // Linear fit of values in Table 5
    if(wave == "A") {
        abscoef = ((-5.33333333333333 * pow(10, -5) * MCP) + 0.00937);
    }
    // Linear fit of values in Table 6
    else if(wave == "D") {
        abscoef = ((-9.75301204819275 * pow(10, -5) * MCP) + 0.0163866265);
    }
    // Other filters not supported for calculated K value
    else {
      std::string message = "Image is of filter [" + wave.toStdString() + "], not supported type A or D, enter your own K value";
      throw IException(IException::User, message, _FILEINFO_);
    }
  }

  // Otherwise get K value from user
  else {
    abscoef = ui.GetDouble("KVALUE");
  }

  // K, Offset, and flat file are defined.  Create output cube and process
  // each line
  p.SetOutputCube("TO");
  p.StartProcess(clemhirescal);
  p.EndProcess();
}


void clemhirescal(vector <Buffer *> &in, vector <Buffer *> &out) {
  Buffer &num = *in[0];   // Input line
  Buffer &den = *in[1];   // Denominator line
  Buffer &rat = *out[0];  // Output line

  for(int i = 0 ; i < num.size() ; i ++) {
    // If it is special, write it out and continue
    if(IsSpecial(num[i])) {
      rat[i] = num[i];
    }
    else {
      //Check denominator for both unusable conditions
      if(IsSpecial(den[i]) || den[i] == 0.0) {
        rat[i] = Isis::Null;
      }
      else {
        //Do ratio and multiply by k constant.
        rat[i] = ((num[i] + offset) / den[i]) * abscoef;
      }
    }
  }
}
