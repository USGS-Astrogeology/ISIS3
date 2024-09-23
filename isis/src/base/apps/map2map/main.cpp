#define GUIHELPERS
#include "Isis.h"

#include "Application.h"
#include "map2map.h"

using namespace Isis;
using namespace std;

void PrintMap();
void LoadMapRange();

map <QString, void *> GuiHelpers() {
  map <QString, void *> helper;
  helper ["PrintMap"] = (void *) PrintMap;
  helper ["LoadMapRange"] = (void *) LoadMapRange;
  return helper;
}

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  Pvl appLog;
  map2map(ui, &appLog); 
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

void LoadMapRange() {
UserInterface &ui = Application::GetUserInterface();

// Get map file
Pvl userMap;

try {
userMap.read(ui.GetFileName("MAP").toStdString());
}
catch(IException &e) {
}

// Get input cube
Pvl fromMap;

try {
fromMap.read(ui.GetCubeName("FROM").toStdString());
}
catch(IException &e) {
}

// Try to get the mapping groups
PvlGroup fromMapping("Mapping");

try {
fromMapping = fromMap.findGroup("Mapping", Pvl::Traverse);
}
catch(IException &e) {
}

PvlGroup userMapping("Mapping");

try {
userMapping = userMap.findGroup("Mapping", Pvl::Traverse);
}
catch(IException &e) {
}

// Do conversions on from map

// Longitude conversions first
if(userMapping.hasKeyword("LongitudeDirection")) {
if((QString::fromStdString(userMapping["LongitudeDirection"])).compare(QString::fromStdString(fromMapping["LongitudeDirection"])) != 0) {
  double minLon = fromMapping["MinimumLongitude"];
  double maxLon = fromMapping["MaximumLongitude"];
  int domain = fromMapping["LongitudeDomain"];

  if(userMapping.hasKeyword("LongitudeDomain")) {
    domain = userMapping["LongitudeDomain"];
  }

  if(QString::fromStdString(userMapping["LongitudeDirection"]) == "PositiveEast") {
    fromMapping["MaximumLongitude"] = Isis::toString(TProjection::ToPositiveEast(minLon, domain));
    fromMapping["MinimumLongitude"] = Isis::toString(TProjection::ToPositiveEast(maxLon, domain));
  }
  else if(QString::fromStdString(userMapping["LongitudeDirection"]) == "PositiveWest") {
    fromMapping["MaximumLongitude"] = Isis::toString(TProjection::ToPositiveWest(minLon, domain));
    fromMapping["MinimumLongitude"] = Isis::toString(TProjection::ToPositiveWest(maxLon, domain));
  }
}
}

// Latitude conversions now
if(userMapping.hasKeyword("LatitudeType")) { // user set a new domain?
if((QString::fromStdString(userMapping["LatitudeType"])).compare(QString::fromStdString(fromMapping["LatitudeType"])) != 0) { // new lat type different?
  if((QString::fromStdString(userMapping["LatitudeType"])).compare("Planetographic") == 0) {
    fromMapping["MinimumLatitude"] = Isis::toString(TProjection::ToPlanetographic(
                                       (double)fromMapping["MinimumLatitude"],
                                       (double)fromMapping["EquatorialRadius"],
                                       (double)fromMapping["PolarRadius"]));
    fromMapping["MaximumLatitude"] = Isis::toString(TProjection::ToPlanetographic(
                                       (double)fromMapping["MaximumLatitude"],
                                       (double)fromMapping["EquatorialRadius"],
                                       (double)fromMapping["PolarRadius"]));
  }
  else {
    fromMapping["MinimumLatitude"] = Isis::toString(TProjection::ToPlanetocentric(
                                       (double)fromMapping["MinimumLatitude"],
                                       (double)fromMapping["EquatorialRadius"],
                                       (double)fromMapping["PolarRadius"]));
    fromMapping["MaximumLatitude"] = Isis::toString(TProjection::ToPlanetocentric(
                                       (double)fromMapping["MaximumLatitude"],
                                       (double)fromMapping["EquatorialRadius"],
                                       (double)fromMapping["PolarRadius"]));
  }
}
}

// Failed at longitudes, use our originals!
if((double)fromMapping["MinimumLongitude"] >= (double)fromMapping["MaximumLongitude"]) {
try {
  fromMapping["MinimumLongitude"] = fromMap.findGroup("Mapping", Pvl::Traverse)["MinimumLongitude"];
  fromMapping["MaximumLongitude"] = fromMap.findGroup("Mapping", Pvl::Traverse)["MaximumLongitude"];
}
catch(IException &e) {
}
}

// Overlay lat/lons in map file (if DEFAULTRANGE=MAP)
if(ui.GetString("DEFAULTRANGE") == "MAP") {
if(userMapping.hasKeyword("MinimumLatitude")) {
  fromMapping["MinimumLatitude"] = userMapping["MinimumLatitude"];
}

if(userMapping.hasKeyword("MaximumLatitude")) {
  fromMapping["MaximumLatitude"] = userMapping["MaximumLatitude"];
}

if(userMapping.hasKeyword("MinimumLongitude")) {
  fromMapping["MinimumLongitude"] = userMapping["MinimumLongitude"];
}

if(userMapping.hasKeyword("MaximumLongitude")) {
  fromMapping["MaximumLongitude"] = userMapping["MaximumLongitude"];
}
}

if(ui.WasEntered("MINLAT")) {
ui.Clear("MINLAT");
}

if(ui.WasEntered("MAXLAT")) {
ui.Clear("MAXLAT");
}

if(ui.WasEntered("MINLON")) {
ui.Clear("MINLON");
}

if(ui.WasEntered("MAXLON")) {
ui.Clear("MAXLON");
}

ui.PutDouble("MINLAT", fromMapping["MinimumLatitude"]);
ui.PutDouble("MAXLAT", fromMapping["MaximumLatitude"]);
ui.PutDouble("MINLON", fromMapping["MinimumLongitude"]);
ui.PutDouble("MAXLON", fromMapping["MaximumLongitude"]);
}
