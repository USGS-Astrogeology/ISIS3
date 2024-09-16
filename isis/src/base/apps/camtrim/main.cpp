#include "Isis.h"
#include "Camera.h"
#include "ProcessByLine.h"
#include "TProjection.h"
#include "ProjectionFactory.h"
#include "SpecialPixel.h"

using namespace std;
using namespace Isis;

// Global variables
Cube *icube;
Camera *cam;
TProjection *proj;
double minlat;
double maxlat;
double minlon;
double maxlon;
int lastBand;

void camtrim(Buffer &in, Buffer &out);

void IsisMain() {
  // We will be processing by line
  ProcessByLine p;

  // Setup the input and get the camera model
  icube = p.SetInputCube("FROM");
  cam = icube->camera();

  UserInterface &ui = Application::GetUserInterface();

  // Make sure the cube isn't projected (i.e. level 2). If it is, the user
  // should be using maptrim instead of this program.
  if (icube->hasGroup("Mapping")) {
    IString msg = "Input cube [" + ui.GetCubeName("FROM").toStdString() + "] is level 2 "
        "(projected). This application is only designed to operate on level 1 "
        "(non-projected) cubes. Please use maptrim instead";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Create the output cube
  p.SetOutputCube("TO");

  // Get the lat/lon range to trim
  minlat = ui.GetDouble("MINLAT");
  maxlat = ui.GetDouble("MAXLAT");
  minlon = ui.GetDouble("MINLON");
  maxlon = ui.GetDouble("MAXLON");

  // Get map projection to determine what type of
  // lat/lons the user wants
  if(ui.WasEntered("MAP")) {
    Pvl lab;
    lab.read(ui.GetFileName("MAP").toStdString());
    proj = (TProjection *) ProjectionFactory::Create(lab);

    // add mapping to print.prt
    PvlGroup mapping = proj->Mapping();
    Application::Log(mapping);
  }
  else {
    proj = NULL;
  }

  // Start the processing
  lastBand = 0;
  p.StartProcess(camtrim);
  p.EndProcess();
}

// Line processing routine
void camtrim(Buffer &in, Buffer &out) {
  // See if there is a change in band which would change the camera model
  if(in.Band() != lastBand) {
    lastBand = in.Band();
    cam->SetBand(icube->physicalBand(lastBand));
  }

  // Loop for each pixel in the line.
  double samp, lat, lon;
  double line = in.Line();
  for(int i = 0; i < in.size(); i++) {
    samp = in.Sample(i);
    cam->SetImage(samp, line);
    if(cam->HasSurfaceIntersection()) {
      lat = cam->UniversalLatitude();
      lon = cam->UniversalLongitude();
      if(proj != NULL) {
        proj->SetUniversalGround(lat, lon);
        lat = proj->Latitude();
        lon = proj->Longitude();
      }
      // Pixel is outside range
      if((lat < minlat) || (lat > maxlat) ||
          (lon < minlon) || (lon > maxlon)) {
        out[i] = NULL8;
      }
      // Pixel inside range
      else {
        out[i] = in[i];
      }
    }
    // Trim outerspace
    else {
      out[i] = NULL8;
    }
  }
}
