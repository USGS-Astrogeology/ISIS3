#define GUIHELPERS
#include "Isis.h"

#include "Application.h"
#include "FileList.h"
#include "IException.h"
#include "SpecialPixel.h"
#include "RingPlaneProjection.h"
#include "ProjectionFactory.h"

#include "ringsautomos.h" // replace with your new header

using namespace Isis;
using namespace std;

void helperButtonCalcRange();
void calcRange(double &minRingRad, double &maxRingRad, double &minRingLon, double &maxRingLon);

map <QString, void *> GuiHelpers() {
  map <QString, void *> helper;
  helper ["helperButtonCalcRange"] = (void *) helperButtonCalcRange;
  return helper;
}

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  Pvl appLog;

  ringsautomos(ui, &appLog); 
}


// Helper function to run calcRange function.
void helperButtonCalcRange() {
  UserInterface &ui = Application::GetUserInterface();
  double minRingRad;
  double maxRingRad;
  double minRingLon;
  double maxRingLon;

  // Run the function calcRange of calculate range info
  calcRange(minRingRad, maxRingRad, minRingLon, maxRingLon);

  // Write ranges to the GUI
  QString use = "USER";
  ui.Clear("GRANGE");
  ui.PutAsString("GRANGE", use);
  ui.Clear("MINRINGRAD");
  ui.PutDouble("MINRINGRAD", minRingRad);
  ui.Clear("MAXRINGRAD");
  ui.PutDouble("MAXRINGRAD", maxRingRad);
  ui.Clear("MINRINGLON");
  ui.PutDouble("MINRINGLON", minRingLon);
  ui.Clear("MAXRINGLON");
  ui.PutDouble("MAXRINGLON", maxRingLon);
}


// Function to calculate the ground range from multiple inputs (list of images)
void calcRange(double &minRingRad, double &maxRingRad, double &minRingLon, double &maxRingLon) {
  UserInterface &ui = Application::GetUserInterface();
  FileList list(FileName(ui.GetFileName("FROMLIST")));
  minRingRad = DBL_MAX;
  maxRingRad = -DBL_MAX;
  minRingLon = DBL_MAX;
  maxRingLon = -DBL_MAX;
  // We will loop through each input cube and do some
  // computations needed for mosaicking
  int nbands = 0;
  RingPlaneProjection *firstProj = NULL;

  for(int i = 0; i < list.size(); i++) {
    // Open the cube and get the maximum number of band in all cubes
    Cube cube;
    cube.open(list[i].toString());
    if(cube.bandCount() > nbands) nbands = cube.bandCount();

    // See if the cube has a projection and make sure it matches
    // previous input cubes
    RingPlaneProjection *proj = (RingPlaneProjection *) Isis::ProjectionFactory::RingsCreateFromCube(*(cube.label()));
    if(firstProj == NULL) {
      firstProj = proj;
    }
    else if(*proj != *firstProj) {
      std::string msg = "Mapping groups do not match between cubes [" + list[0].toString() +
                    "] and [" + list[i].toString() + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if(proj->HasGroundRange()) {
      if(proj->MinimumRingRadius() < minRingRad) minRingRad = proj->MinimumRingRadius();
      if(proj->MaximumRingRadius() > maxRingRad) maxRingRad = proj->MaximumRingRadius();
      if(proj->MinimumRingLongitude() < minRingLon) minRingLon = proj->MinimumRingLongitude();
      if(proj->MaximumRingLongitude() > maxRingLon) maxRingLon = proj->MaximumRingLongitude();
    }

    // Cleanup
    cube.close();
    if(proj != firstProj)
      delete proj;
  }
}