#define GUIHELPERS
#include "Isis.h"

#include "cam2map.h"

#include "Camera.h"
#include "IException.h"
#include "IString.h"
#include "ProcessRubberSheet.h"
#include "ProjectionFactory.h"
#include "PushFrameCameraDetectorMap.h"
#include "Pvl.h"
#include "Target.h"
#include "TProjection.h"

using namespace std;
using namespace Isis;

void printMap();
void loadMapRes();
void loadCameraRes();
void loadMapRange();
void loadCameraRange();

map <QString, void *> GuiHelpers() {
  map <QString, void *> helper;
  helper ["PrintMap"] = (void *) printMap;
  helper ["LoadMapRes"] = (void *) loadMapRes;
  helper ["LoadCameraRes"] = (void *) loadCameraRes;
  helper ["LoadMapRange"] = (void *) loadMapRange;
  helper ["LoadCameraRange"] = (void *) loadCameraRange;
  return helper;
}

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  Pvl appLog;
  cam2map(ui, &appLog);
}

// Helper function to print out mapfile to session log
void printMap() {
  UserInterface &ui = Application::GetUserInterface();

  // Get mapping group from map file
  Pvl userMap;
  userMap.read(ui.GetFileName("MAP").toStdString());
  PvlGroup &userGrp = userMap.findGroup("Mapping", Pvl::Traverse);

  //Write map file out to the log
  Application::GuiLog(userGrp);
}

// Helper function to get mapping resolution.
void loadMapRes() {
  UserInterface &ui = Application::GetUserInterface();

  // Get mapping group from map file
  Pvl userMap;
  userMap.read(ui.GetFileName("MAP").toStdString());
  PvlGroup &userGrp = userMap.findGroup("Mapping", Pvl::Traverse);

  // Set resolution
  if (userGrp.hasKeyword("Scale")) {
    ui.Clear("RESOLUTION");
    ui.PutDouble("RESOLUTION", userGrp["Scale"]);
    ui.Clear("PIXRES");
    ui.PutAsString("PIXRES", "PPD");
  }
  else if (userGrp.hasKeyword("PixelResolution")) {
    ui.Clear("RESOLUTION");
    ui.PutDouble("RESOLUTION", userGrp["PixelResolution"]);
    ui.Clear("PIXRES");
    ui.PutAsString("PIXRES", "MPP");
  }
  else {
    std::string msg = "No resolution value found in [" + ui.GetFileName("MAP").toStdString() + "]";
    throw IException(IException::User, msg, _FILEINFO_);
  }
}

//Helper function to get camera resolution.
void loadCameraRes() {
  UserInterface &ui = Application::GetUserInterface();
  QString file = ui.GetCubeName("FROM");

  // Open the input cube, get the camera object, and the cam map projection
  Cube c;
  c.open(file);
  Camera *cam = c.camera();
  Pvl camMap;
  cam->BasicMapping(camMap);
  PvlGroup &camGrp = camMap.findGroup("Mapping");

  ui.Clear("RESOLUTION");
  ui.PutDouble("RESOLUTION", camGrp["PixelResolution"]);

  ui.Clear("PIXRES");
  ui.PutAsString("PIXRES", "MPP");
}

//Helper function to get ground range from map file.
void loadMapRange() {
  UserInterface &ui = Application::GetUserInterface();

  // Get map file
  Pvl userMap;
  userMap.read(ui.GetFileName("MAP").toStdString());
  PvlGroup &userGrp = userMap.findGroup("Mapping", Pvl::Traverse);

  // Set ground range keywords that are found in mapfile
  int count = 0;
  ui.Clear("MINLAT");
  ui.Clear("MAXLAT");
  ui.Clear("MINLON");
  ui.Clear("MAXLON");
  if (userGrp.hasKeyword("MinimumLatitude")) {
    ui.PutDouble("MINLAT", userGrp["MinimumLatitude"]);
    count++;
  }
  if (userGrp.hasKeyword("MaximumLatitude")) {
    ui.PutDouble("MAXLAT", userGrp["MaximumLatitude"]);
    count++;
  }
  if (userGrp.hasKeyword("MinimumLongitude")) {
    ui.PutDouble("MINLON", userGrp["MinimumLongitude"]);
    count++;
  }
  if (userGrp.hasKeyword("MaximumLongitude")) {
    ui.PutDouble("MAXLON", userGrp["MaximumLongitude"]);
    count++;
  }

  // Set default ground range param to map
  ui.Clear("DEFAULTRANGE");
  ui.PutAsString("DEFAULTRANGE", "MAP");

  if (count < 4) {
    std::string msg = "One or more of the values for the ground range was not found";
    msg += " in [" + ui.GetFileName("MAP").toStdString() + "]";
    throw IException(IException::User, msg, _FILEINFO_);
  }
}

//Helper function to load camera range.
void loadCameraRange() {
  UserInterface &ui = Application::GetUserInterface();
  QString file = ui.GetCubeName("FROM");

  // Get the map projection file provided by the user
  Pvl userMap;
  userMap.read(ui.GetFileName("MAP").toStdString());

  // Open the input cube, get the camera object, and the cam map projection
  Cube c;
  c.open(file);
  Camera *cam = c.camera();

  // Make the target info match the user mapfile
  double minlat, maxlat, minlon, maxlon;
  cam->GroundRange(minlat, maxlat, minlon, maxlon, userMap);

  // Set ground range parameters in UI
  ui.Clear("MINLAT");
  ui.PutDouble("MINLAT", minlat);
  ui.Clear("MAXLAT");
  ui.PutDouble("MAXLAT", maxlat);
  ui.Clear("MINLON");
  ui.PutDouble("MINLON", minlon);
  ui.Clear("MAXLON");
  ui.PutDouble("MAXLON", maxlon);

  // Set default ground range param to camera
  ui.Clear("DEFAULTRANGE");
  ui.PutAsString("DEFAULTRANGE", "CAMERA");
}
