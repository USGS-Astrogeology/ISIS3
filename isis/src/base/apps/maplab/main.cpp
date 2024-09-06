#include "Isis.h"
#include <iostream>
#include <sstream>
#include <QString>
#include "Pvl.h"
#include "Cube.h"
#include "History.h"
#include "TProjection.h"
#include "ProjectionFactory.h"

using namespace Isis;
using namespace std;

void IsisMain() {
  // Access input parameters (user interface)
  UserInterface &ui = Application::GetUserInterface();

  // Open the input cube
  Cube cube;
  cube.open(ui.GetCubeName("FROM"), "rw");

  //Get the map projection file provided by the user
  Pvl userMap;
  userMap.read(ui.GetFileName("MAP").toStdString());
  PvlGroup &mapGrp = userMap.findGroup("Mapping", Pvl::Traverse);

  // Error checking to ensure the map projection file provided contains
  // information pertaining to a target, body radius, and longitude direction
  if(!mapGrp.hasKeyword("TargetName")) {
    std::string msg = "The given MAP [" + userMap.name() +
                  "] does not have the TargetName keyword.";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  else if(!mapGrp.hasKeyword("EquatorialRadius") ||
          !mapGrp.hasKeyword("PolarRadius")) {
    std::string msg = "The given MAP [" + userMap.name() +
                  "] does not have the EquatorialRadius and PolarRadius keywords.";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  else if(!mapGrp.hasKeyword("LongitudeDomain")) {
    std::string msg = "The given MAP [" + userMap.name() +
                  "] does not have the LongitudeDomain keyword.";
    throw IException(IException::User, msg, _FILEINFO_);
  }


  // Get user entered option
  QString option = ui.GetString("COORDINATES");

  double x = 0.0;
  double y = 0.0;
  TProjection * proj = (TProjection *) ProjectionFactory::Create(userMap, false);
  if(option == "XY") {
    x = ui.GetDouble("X");
    y = ui.GetDouble("Y");
  }
  else if(option == "LATLON") {
    proj->SetGround(ui.GetDouble("LAT"), ui.GetDouble("LON"));
    x = proj->XCoord();
    y = proj->YCoord();
  }
  else {
    std::string message = "Invalid option [" + option + "] for parameter COORDINATES";
    throw IException(IException::User, message, _FILEINFO_);
  }

  double res = 0.0;
  double scale = 0.0;

  if(mapGrp.hasKeyword("PixelResolution")) {
    double localRadius = proj->LocalRadius(proj->TrueScaleLatitude());
    res = mapGrp.findKeyword("PixelResolution");
    scale = (2.0 * Isis::PI * localRadius) / (360.0 * res);
  }
  else if(mapGrp.hasKeyword("Scale")) {
    double localRadius = proj->LocalRadius(proj->TrueScaleLatitude());
    scale = mapGrp.findKeyword("Scale");
    res = (2.0 * Isis::PI * localRadius) / (360.0 * scale);
  }
  else {
    std::string msg = "The given MAP[" + userMap.name() +
                 "] does not have the PixelResolution or Scale keywords.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  //Read in line and sample inputs
  double line = ui.GetDouble("LINE");
  double samp = ui.GetDouble("SAMPLE");
  x = x - res * (samp - 0.5);
  y = y + res * (line - 0.5);

  //add origen values to Mapping Group
  mapGrp.addKeyword(PvlKeyword("UpperLeftCornerX", std::to_string(x), "meters"), Pvl::Replace);
  mapGrp.addKeyword(PvlKeyword("UpperLeftCornerY", std::to_string(y), "meters"), Pvl::Replace);

  if(!mapGrp.hasKeyword("PixelResolution")) {
    mapGrp.addKeyword(PvlKeyword("PixelResolution", std::to_string(res), "meters"));
  }
  if(!mapGrp.hasKeyword("Scale")) {
    mapGrp.addKeyword(PvlKeyword("Scale", std::to_string(scale), "pixels/degree"));
  }


  // Output the mapping group used to the Gui session log
  Application::GuiLog(userMap);
  // Extract label from cube file
  Pvl *label = cube.label();
  PvlObject &o = label->findObject("IsisCube");
  // Add Mapping Group to input cube
  if(o.hasGroup("Mapping")) {
    o.deleteGroup("Mapping");
  }
  o.addGroup(mapGrp);

  // keep track of change to labels in history
  History hist = cube.readHistory();
  hist.AddEntry();
  cube.write(hist);

  cube.close();
}
