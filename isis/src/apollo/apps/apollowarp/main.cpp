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

using namespace std;
using namespace Isis;

void IsisMain() {
  ProcessRubberSheet p;
  UserInterface &ui = Application::GetUserInterface();

  // Open the input cube
  Cube *ipacket = p.SetInputCube ("FROM");

  // Check to see if it is an Apollo image and if the reseaus have been refined
  //    (note: a status of 'Removed' implies it is also 'Refined')
  PvlGroup &reseaus = ipacket->group("Reseaus");
  QString mission = QString::fromStdString(ipacket->group("Instrument")["SpacecraftName"]);
  if (mission.mid(0,6) != "APOLLO") {
    std::string msg = "This application is for use with Apollo spacecrafts only. ";
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }
  if ((std::string)reseaus["Status"] != "Refined" && (std::string)reseaus["Status"] != "Removed") {
    std::string msg = "This application can only be run after findapollorx.";
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }

  // Get the master reseau info
  PvlGroup master = Pvl(reseaus["Master"]).findGroup("MasterReseaus");

  vector<double> inputLine,inputSample,outputLine,outputSample;
  // Setup the parameters for the transform
  for (int i=0; i<reseaus["Sample"].size(); i++) {
    inputLine.push_back(IString::ToDouble(reseaus["Line"][i]));
    inputSample.push_back(IString::ToDouble(reseaus["Sample"][i]));
    outputLine.push_back(IString::ToDouble(master["Line"][i]));
    outputSample.push_back(IString::ToDouble(master["Sample"][i]));

    // Update the cube's reseau information
    reseaus["Line"][i] = toString(outputLine[i]);
    reseaus["Sample"][i] = toString(outputSample[i]);
  }

  // Get the final output image dimensions
  PvlGroup dimensions = Pvl(reseaus["Master"]).findGroup("Dimensions");
  int onl = dimensions["UndistortedSamples"],
      ons = dimensions["UndistortedLines"];

  // Create the basis function for transforming
  int degree = ui.GetInteger("DEGREE");
  BasisFunction *basisLine = new PolynomialBivariate(degree);
  BasisFunction *basisSamp = new PolynomialBivariate(degree);
  bool weighted = ui.GetBoolean("WEIGHTED");

  // Set up the transform object
  WarpTransform *transform = new WarpTransform(*basisLine,*basisSamp,weighted,inputLine,inputSample,outputLine,outputSample,ipacket->lineCount(),ipacket->sampleCount(),onl,ons);

  // Allocate the output file, same size as input
  p.SetOutputCube ("TO",transform->OutputSamples(),transform->OutputLines(),ipacket->bandCount());

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
                 ui.GetString("INTERP").toStdString() + "]";
    throw IException(IException::Programmer, msg,_FILEINFO_);
  }

  // Warp the image
  p.StartProcess(*transform,*interp);
  p.EndProcess();

  delete transform;
  delete interp;
  delete basisLine;
  delete basisSamp;
}
