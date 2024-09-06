#define GUIHELPERS
#include "Isis.h"

#include "automos.h"

#include "Application.h"
#include "FileList.h"
#include "ProjectionFactory.h"
#include "Pvl.h"
#include "TProjection.h"
#include "SpecialPixel.h"

using namespace std;
using namespace Isis;

static void calcRange(double &minLat, double &maxLat, double &minLon, double &maxLon);
static void helperButtonCalcRange();

map <QString, void *> GuiHelpers() {
  map <QString, void *> helper;
  helper ["helperButtonCalcRange"] = (void *) helperButtonCalcRange;
  return helper;
}

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  Pvl appLog;
  automos(ui, &appLog);
}

// Function to calculate the ground range from multiple inputs (list of images)
void calcRange(double &minLat, double &maxLat, double &minLon, double &maxLon) {
  UserInterface &ui = Application::GetUserInterface();

  FileList list(FileName(ui.GetFileName("FROMLIST")));
  minLat = DBL_MAX;
  maxLat = -DBL_MAX;
  minLon = DBL_MAX;
  maxLon = -DBL_MAX;
  // We will loop through each input cube and do some
  // computations needed for mosaicking
  int nbands = 0;
  TProjection *firstProj = NULL;

  for(int i = 0; i < list.size(); i++) {
    // Open the cube and get the maximum number of band in all cubes
    Cube cube;
    cube.open(list[i].toString());
    if(cube.bandCount() > nbands) nbands = cube.bandCount();

    // See if the cube has a projection and make sure it matches
    // previous input cubes
    TProjection *proj = (TProjection *) Isis::ProjectionFactory::CreateFromCube(*(cube.label()));
    if(firstProj == NULL) {
      firstProj = proj;
    }
    else if(*proj != *firstProj) {
      std::string msg = "Mapping groups do not match between cubes [" + list[0].toString() +
                    "] and [" + list[i].toString() + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if(proj->HasGroundRange()) {
      if(proj->MinimumLatitude() < minLat) minLat = proj->MinimumLatitude();
      if(proj->MaximumLatitude() > maxLat) maxLat = proj->MaximumLatitude();
      if(proj->MinimumLongitude() < minLon) minLon = proj->MinimumLongitude();
      if(proj->MaximumLongitude() > maxLon) maxLon = proj->MaximumLongitude();
    }

    // Cleanup
    cube.close();
    if(proj != firstProj)
      delete proj;
  }
}

// Helper function to run calcRange function.
void helperButtonCalcRange() {
  UserInterface &ui = Application::GetUserInterface();
  double minLat;
  double maxLat;
  double minLon;
  double maxLon;

  // Run the function calcRange of calculate range info
  calcRange(minLat, maxLat, minLon, maxLon);

  // Write ranges to the GUI
  QString use = "USER";
  ui.Clear("GRANGE");
  ui.PutAsString("GRANGE", use);
  ui.Clear("MINLAT");
  ui.PutDouble("MINLAT", minLat);
  ui.Clear("MAXLAT");
  ui.PutDouble("MAXLAT", maxLat);
  ui.Clear("MINLON");
  ui.PutDouble("MINLON", minLon);
  ui.Clear("MAXLON");
  ui.PutDouble("MAXLON", maxLon);
}
