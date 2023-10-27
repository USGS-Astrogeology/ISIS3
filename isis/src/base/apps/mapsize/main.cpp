#include "Isis.h"
#include "Process.h"
#include "ProjectionFactory.h"
#include "Projection.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  Process p;

  // Get the output map projection file and create an output projection object
  UserInterface &ui = Application::GetUserInterface();
  Pvl lab;
  lab.read(ui.GetFileName("MAP").toStdString());

  int samples, lines;
  Projection *outmap = ProjectionFactory::CreateForCube(lab, samples, lines);

  // Obtain x/y min/max distances in meters
  double xmin, xmax, ymin, ymax;
  outmap->XYRange(xmin, xmax, ymin, ymax);
  double xdist = xmax - xmin;
  double ydist = ymax - ymin;
  double scale, width, height;
  double inchesPerMeter = 39.37;

  // See if the user specified inches or scale
  if(ui.GetString("OPTION") == "SCALE") {
    scale = ui.GetDouble("SCALE");
    width = xdist / scale * inchesPerMeter;
    height = ydist / scale * inchesPerMeter;
  }
  else {
    width = ui.GetDouble("WIDTH");
    height = ui.GetDouble("HEIGHT");
    double xscale = xdist / width * inchesPerMeter;
    double yscale = ydist / height * inchesPerMeter;
    scale = max(xscale, yscale);
    // Recompute as one is likely to be reduced
    width = xdist / scale * inchesPerMeter;
    height = ydist / scale * inchesPerMeter;
  }

  // Create a label and log it
  PvlGroup results("Results");
  results += PvlKeyword("Map", ui.GetFileName("MAP").toStdString());
  results += PvlKeyword("Scale", std::to_string(scale));
  results += PvlKeyword("Width", std::to_string(width), "inches");
  results += PvlKeyword("Height", std::to_string(height), "inches");
  results += PvlKeyword("Samples", std::to_string(samples));
  results += PvlKeyword("Lines", std::to_string(lines));
  results += PvlKeyword("RealSize", std::to_string(((Isis::BigInt)lines * samples * 4) / 1024.0), "KB");
  results += PvlKeyword("SignedWordSize", std::to_string(((Isis::BigInt)lines * samples * 2) / 1024.0), "KB");
  results += PvlKeyword("UnsignedByteSize", std::to_string(((Isis::BigInt)lines * samples) / 1024.0), "KB");
  Application::Log(results);

  // Write the output file if requested
  if(ui.WasEntered("TO")) {
    Pvl temp;
    temp.addGroup(results);
    temp.addGroup(lab.findGroup("Mapping", Pvl::Traverse));
    temp.write(ui.GetFileName("TO", "txt").toStdString());
  }

  Application::Log(lab.findGroup("Mapping", Pvl::Traverse));

  p.EndProcess();
}
