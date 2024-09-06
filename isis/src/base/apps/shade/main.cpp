#include "Isis.h"

#include <cmath>

#include "Angle.h"
#include "Constants.h"
#include "Hillshade.h"
#include "ProcessByBoxcar.h"
#include "SpecialPixel.h"

using namespace std;
using namespace Isis;

Hillshade hillshade;

void shade(Buffer &in, double &v);

void IsisMain() {
  ProcessByBoxcar p;

  UserInterface &ui = Application::GetUserInterface();

  // Open the input cube
  Cube *inCube = p.SetInputCube("FROM");

  // Allocate the output cube
  p.SetOutputCube("TO");

  p.SetBoxcarSize(3, 3);

  // Read user parameters
  hillshade.setAzimuth(Angle(ui.GetDouble("AZIMUTH"), Angle::Degrees));
  hillshade.setZenith(Angle(ui.GetDouble("ZENITH"), Angle::Degrees));

  // Get from labels or from user
  if(ui.WasEntered("PIXELRESOL")) {
    hillshade.setResolution(ui.GetDouble("PIXELRESOL"));
  }
  else {
    if(inCube->label()->findObject("IsisCube").hasGroup("Mapping")) {
      hillshade.setResolution(
          inCube->label()->findObject("IsisCube").findGroup("Mapping")["PixelResolution"]);
    }
    else {
      std::string msg = "The file [" + ui.GetCubeName("FROM") + "] does not have a mapping group,"
                    + " you must enter a Pixel Resolution";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  p.ProcessCube(shade);
  p.EndProcess();
}

// Shade processing routine
void shade(Buffer &in, double &v) {
  v = hillshade.shadedValue(in);
}

