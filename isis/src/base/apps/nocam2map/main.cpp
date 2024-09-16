#define GUIHELPERS

#include "Isis.h"

#include <algorithm>

#include <QList>
#include <QString>
#include <QStringList>
#include <QVector>

#include <SpiceUsr.h>

#include "Brick.h"
#include "Constants.h"
#include "Cube.h"
#include "IString.h"
#include "LeastSquares.h"
#include "NaifStatus.h"
#include "nocam2map.h"
#include "PolynomialBivariate.h"
#include "ProcessRubberSheet.h"
#include "ProjectionFactory.h"
#include "Statistics.h"
#include "Target.h"
#include "TextFile.h"
#include "TProjection.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis; 

void PrintMap();
void ComputePixRes();
void LoadMapRes();
void ComputeInputRange();
void LoadMapRange();

map <QString, void *> GuiHelpers() {
  map <QString, void *> helper;
  helper ["PrintMap"] = (void *) PrintMap;
  helper ["ComputePixRes"] = (void *) ComputePixRes;
  helper ["LoadMapRes"] = (void *) LoadMapRes;
  helper ["ComputeInputRange"] = (void *) ComputeInputRange;
  helper ["LoadMapRange"] = (void *) LoadMapRange;
  return helper;
}


void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  Pvl appLog;

  nocam2map(ui, &appLog); 

  // in this case, output data are in a "Results" group.
  PvlGroup results = appLog.findGroup("Mapping");
  if( ui.WasEntered("TO") && ui.IsInteractive() ) {
    Application::GuiLog(results);
  }
}


//Helper function to get camera resolution.
void ComputePixRes() {
  Process p;
  UserInterface &ui = Application::GetUserInterface();
  Cube *latCube = p.SetInputCube("LATCUB");
  Cube *lonCube = p.SetInputCube("LONCUB");
  Brick latBrick(1, 1, 1, latCube->pixelType());
  Brick lonBrick(1, 1, 1, lonCube->pixelType());
  latBrick.SetBasePosition(1, 1, 1);
  latCube->read(latBrick);

  lonBrick.SetBasePosition(1, 1, 1);
  lonCube->read(lonBrick);

  double a = latBrick.at(0) * PI / 180.0;
  double c = lonBrick.at(0) * PI / 180.0;

  latBrick.SetBasePosition(latCube->sampleCount(), latCube->lineCount(), 1);
  latCube->read(latBrick);

  lonBrick.SetBasePosition(lonCube->sampleCount(), lonCube->lineCount(), 1);
  lonCube->read(lonBrick);

  double b = latBrick.at(0) * PI / 180.0;
  double d = lonBrick.at(0) * PI / 180.0;

  double angle = acos(cos(a) * cos(b) * cos(c - d) + sin(a) * sin(b));
  angle *= 180 / PI;

  double pixels = sqrt(pow(latCube->sampleCount() - 1.0, 2.0) + pow(latCube->lineCount() - 1.0, 2.0));

  p.EndProcess();

  ui.Clear("RESOLUTION");
  ui.PutDouble("RESOLUTION", pixels / angle);

  ui.Clear("PIXRES");
  ui.PutAsString("PIXRES", "PPD");
}


// Helper function to print out mapfile to session log
void PrintMap() {
  UserInterface &ui = Application::GetUserInterface();

  // Get mapping group from map file
  Pvl userMap;
  userMap.read(ui.GetFileName("MAP").toStdString());
  PvlGroup &userGrp = userMap.findGroup("Mapping", Pvl::Traverse);

  //Write map file out to the log
  Isis::Application::GuiLog(userGrp);
}

// Helper function to get mapping resolution.
void LoadMapRes() {
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

//Helper function to compute input range.
void ComputeInputRange() {
  Process p;
  Cube *latCub = p.SetInputCube("LATCUB");
  Cube *lonCub = p.SetInputCube("LONCUB");

  UserInterface &ui = Application::GetUserInterface();
  Pvl userMap;
  userMap.read(ui.GetFileName("MAP").toStdString());
  PvlGroup &userGrp = userMap.findGroup("Mapping", Pvl::Traverse);

  Statistics *latStats = latCub->statistics();
  Statistics *lonStats = lonCub->statistics();

  double minLat = latStats->Minimum();
  double maxLat = latStats->Maximum();

  int lonDomain = userGrp.hasKeyword("LongitudeDomain") ? (int)userGrp.findKeyword("LongitudeDomain") : 360;
  double minLon = lonDomain == 360 ? TProjection::To360Domain(lonStats->Minimum()) :
                                       TProjection::To180Domain(lonStats->Minimum());
  double maxLon = lonDomain == 360 ? TProjection::To360Domain(lonStats->Maximum()) :
                                       TProjection::To180Domain(lonStats->Maximum());

  if (userGrp.hasKeyword("LatitudeType")) {
    bool isOcentric = ((std::string)userGrp.findKeyword("LatitudeType")) == "Planetocentric";

    double equRadius;
    double polRadius;

    //If the user entered the equatorial and polar radii
    if (ui.WasEntered("EQURADIUS") && ui.WasEntered("POLRADIUS")) {
      equRadius = ui.GetDouble("EQURADIUS");
      polRadius = ui.GetDouble("POLRADIUS");
    }
    //Else read them from the pck
    else {
      QString target;

      //If user entered target
      if (ui.WasEntered("TARGET")) {
        target = ui.GetString("TARGET");
      }
      //Else read the target name from the input cube
      else {
        Pvl fromFile;
        fromFile.read(ui.GetCubeName("FROM").toStdString());
        target = QString::fromStdString(fromFile.findKeyword("TargetName", Pvl::Traverse));
      }

      PvlGroup radii = Target::radiiGroup(target);
      equRadius = double(radii["EquatorialRadius"]);
      polRadius = double(radii["PolarRadius"]);
    }

    if (isOcentric) {
      if (ui.GetString("LATTYPE") != "PLANETOCENTRIC") {
        minLat = TProjection::ToPlanetocentric(minLat, (double)equRadius, (double)polRadius);
        maxLat = TProjection::ToPlanetocentric(maxLat, (double)equRadius, (double)polRadius);
      }
    }
    else {
      if (ui.GetString("LATTYPE") == "PLANETOCENTRIC") {
        minLat = TProjection::ToPlanetographic(minLat, (double)equRadius, (double)polRadius);
        maxLat = TProjection::ToPlanetographic(maxLat, (double)equRadius, (double)polRadius);
      }
    }
  }

  if (userGrp.hasKeyword("LongitudeDirection")) {
    bool isPosEast = ((std::string)userGrp.findKeyword("LongitudeDirection")) == "PositiveEast";

    if (isPosEast) {
      if (ui.GetString("LONDIR") != "POSITIVEEAST") {
        minLon = TProjection::ToPositiveEast(minLon, lonDomain);
        maxLon = TProjection::ToPositiveEast(maxLon, lonDomain);

        if (minLon > maxLon) {
          double temp = minLon;
          minLon = maxLon;
          maxLon = temp;
        }
      }
    }
    else {
      if (ui.GetString("LONDIR") == "POSITIVEEAST") {
        minLon = TProjection::ToPositiveWest(minLon, lonDomain);
        maxLon = TProjection::ToPositiveWest(maxLon, lonDomain);

        if (minLon > maxLon) {
          double temp = minLon;
          minLon = maxLon;
          maxLon = temp;
        }
      }
    }
  }

  // Set ground range parameters in UI
  ui.Clear("MINLAT");
  ui.PutDouble("MINLAT", minLat);
  ui.Clear("MAXLAT");
  ui.PutDouble("MAXLAT", maxLat);
  ui.Clear("MINLON");
  ui.PutDouble("MINLON", minLon);
  ui.Clear("MAXLON");
  ui.PutDouble("MAXLON", maxLon);

  p.EndProcess();

  // Set default ground range param to camera
  ui.Clear("DEFAULTRANGE");
  ui.PutAsString("DEFAULTRANGE", "COMPUTE");
}

//Helper function to get ground range from map file.
void LoadMapRange() {
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
