/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#define GUIHELPERS

#include "Isis.h"

#include "ControlNet.h"
#include "ControlPoint.h"
#include "Distance.h"
#include "ID.h"
#include "Latitude.h"
#include "Longitude.h"
#include "Progress.h"
#include "ProjectionFactory.h"
#include "SurfacePoint.h"
#include "Target.h"
#include "TProjection.h"

using namespace std;
using namespace Isis;

void checkLatitude(double minLat, double maxLat);
void checkLongitude(double minLon, double maxLon, int lonDomain);
void printMap();

map<QString, void *> GuiHelpers() {
  map<QString, void *> helper;
  helper["PrintMap"] = (void *) printMap;
  return helper;
}

void IsisMain() {

  // Get the map projection file provided by the user
  UserInterface &ui = Application::GetUserInterface();

  // get the pvl containing a mapping group
  Pvl userMap;
  userMap.read(ui.GetFileName("MAP").toStdString());
  PvlGroup &mapGroup = userMap.findGroup("Mapping", Pvl::Traverse);

  QString target;
  if (ui.WasEntered("TARGET")) {
    target = ui.GetString("TARGET");
  }
  else if (mapGroup.hasKeyword("TargetName")) {
    target = QString::fromStdString(mapGroup.findKeyword("TargetName")[0]);
    ui.PutAsString("TARGET", target);
  }
  else {
    std::string msg = "A target must be specified either by the [TARGET] "
        "parameter or included as a value for keyword [TargetName] in the "
        "projection file [MAP].";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  mapGroup.addKeyword(PvlKeyword("TargetName", target.toStdString()), PvlContainer::Replace);

  // Use the target name to create the control net to store the points in.
  ControlNet cnet;
  cnet.SetTarget(target);

  // If the mapping group doesn't have the target radii, try to get them from the Target class.
  if (!mapGroup.hasKeyword("EquatorialRadius")) {
    try {
      PvlGroup pvlRadii = Target::radiiGroup(target);
      mapGroup += PvlKeyword("EquatorialRadius", pvlRadii["EquatorialRadius"], "Meters");
      // if we successfully found equatorial radius, then polar should have worked too.
      mapGroup += PvlKeyword("PolarRadius", pvlRadii["PolarRadius"], "Meters");
    }
    catch (IException &e) {
      std::string msg = "Unable to get target radii values from the given target [" + target.toStdString() + "]. "
                    "User must add EquatorialRadius and PolarRadius values to the input MAP file.";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
  }
  double equatorialRadius = IString::ToDouble(mapGroup.findKeyword("EquatorialRadius")[0]);

  QString networkId;
  if (ui.WasEntered("NETWORKID")) {
    networkId = ui.GetString("NETWORKID");
    cnet.SetNetworkId(networkId);
  }
  cnet.SetUserName(Isis::Application::UserName());
  if (ui.WasEntered("DESCRIPTION")) {
    cnet.SetDescription(ui.GetString("DESCRIPTION"));
  }

  // Set up an automatic id generator for the point ids
  ID pointId = ID(ui.GetString("POINTID"));

  // lat/lon boundaries
  double minLat = ui.GetDouble("MINLAT");
  double maxLat = ui.GetDouble("MAXLAT");
  double minLon = ui.GetDouble("MINLON");
  double maxLon = ui.GetDouble("MAXLON");
  checkLatitude(minLat, maxLat);
  int lonDomain =
      (mapGroup.hasKeyword("LongitudeDomain") ?
          IString::ToInteger(mapGroup.findKeyword("LongitudeDomain")[0]) :
          360);
  checkLongitude(minLon, maxLon, lonDomain);

  if (QString::compare(ui.GetString("SPACING"), "METER") == 0) {

    // To construct a Projection for converting between Lon/Lat and
    // X/Y, we first add appropriate keywords to the userMap.

    // append/replace more keywords
    if (!ui.WasEntered("MAP")) { // if the default map template was kept
      mapGroup += PvlKeyword("LatitudeType", "Planetocentric");
      mapGroup += PvlKeyword("LongitudeDirection", "PositiveEast");
      mapGroup += PvlKeyword("LongitudeDomain", "360");
      mapGroup += PvlKeyword("CenterLatitude", "0.0");
      mapGroup += PvlKeyword("CenterLongitude", "0.0");
    }

    mapGroup.addKeyword(PvlKeyword("MinimumLatitude", toString(minLat)), Pvl::Replace);
    mapGroup.addKeyword(PvlKeyword("MaximumLatitude", toString(maxLat)), Pvl::Replace);
    mapGroup.addKeyword(PvlKeyword("MinimumLongitude", toString(minLon)), Pvl::Replace);
    mapGroup.addKeyword(PvlKeyword("MaximumLongitude", toString(maxLon)), Pvl::Replace);

    // create the projection from the editted map
    TProjection *proj = (TProjection *) ProjectionFactory::Create(userMap);

    // Convert the Lat/Lon range to an X/Y range.
    double minX, minY, maxX, maxY;
    bool foundRange = proj->XYRange(minX, maxX, minY, maxY);
    if (!foundRange) {
      std::string msg = "Cannot convert Lat/Long range to an X/Y range";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    double xStepSize = ui.GetDouble("XSTEP");
    double yStepSize = ui.GetDouble("YSTEP");

    Progress gridStatus;

    int maxSteps = 0;
    double x = minX;
    while (x <= maxX) {
      double y = minY;
      while (y <= maxY) {
        maxSteps++;
        y += yStepSize;
      }
      x += xStepSize;
    }

    if (maxSteps > 0) {
      gridStatus.SetMaximumSteps(maxSteps);
      gridStatus.SetText("Seeding Grid");
      gridStatus.CheckStatus();
    }

    x = minX;
    while (x <= maxX) {
      double y = minY;
      while (y <= maxY) {
        proj->SetCoordinate(x, y);
        if (!proj->IsSky() && proj->Latitude() < ui.GetDouble("MAXLAT") &&
            proj->Longitude() < ui.GetDouble("MAXLON") &&
            proj->Latitude() > ui.GetDouble("MINLAT") &&
            proj->Longitude() > ui.GetDouble("MINLON")) {
          SurfacePoint pt(Latitude(proj->Latitude(), Angle::Degrees),
                          Longitude(proj->Longitude(), Angle::Degrees),
                          Distance(proj->EquatorialRadius(), Distance::Meters));
          ControlPoint * control = new ControlPoint;
          control->SetId(pointId.Next());
          control->SetIgnored(true);
          control->SetAprioriSurfacePoint(pt);
          cnet.AddPoint(control);
        }
        y += yStepSize;
        gridStatus.CheckStatus();
      }
      x += xStepSize;
    }
  }
  else {

    double latStep = ui.GetDouble("LATSTEP");
    double lonStep = ui.GetDouble("LONSTEP");

    Progress gridStatus;

    int maxSteps = 0;
    double lon = minLon;
    while (lon <= maxLon) {
      double lat = minLat;
      while (lat <= maxLat) {
        maxSteps++;
        lat += latStep;
      }
      lat = minLat;
      lon += lonStep;
    }

    if (maxSteps > 0) {
      gridStatus.SetMaximumSteps(maxSteps);
      gridStatus.SetText("Seeding Grid");
      gridStatus.CheckStatus();
    }

    lon = minLon;
    while (lon <= maxLon) {
      double lat = minLat;
      while (lat <= maxLat) {
        ControlPoint * control = new ControlPoint;
        control->SetId(pointId.Next());
        control->SetIgnored(true);
        SurfacePoint pt(Latitude(lat, Angle::Degrees),
                        Longitude(lon, Angle::Degrees),
                        Distance(equatorialRadius, Distance::Meters));
        control->SetAprioriSurfacePoint(pt);
        cnet.AddPoint(control);

        lat += latStep;
        gridStatus.CheckStatus();
      }
      lon += lonStep;
    }
  }

  PvlGroup results("Results");
  results += PvlKeyword("EquatorialRadius", toString(equatorialRadius));
  results += PvlKeyword("NumberControlPoints", toString(cnet.GetNumPoints()));
  Application::Log(results);

  cnet.Write(ui.GetFileName("ONET"));
}


void checkLatitude(double minLat, double maxLat) {
  if (minLat > maxLat) {
    std::string msg = "MINLAT [" + toString(minLat) +
      "] is greater than MAXLAT [" + toString(maxLat) + "]";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  if (minLat < -90) {
    std::string msg = "MINLAT [" + toString(minLat) + "] is less than -90";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  if (maxLat > 90) {
    std::string msg = "MAXLAT [" + toString(maxLat) + "] is greater than 90";
    throw IException(IException::User, msg, _FILEINFO_);
  }
}


void checkLongitude(double minLon, double maxLon, int lonDomain) {
  if (minLon > maxLon) {
    double suggestedMaxLon = maxLon + lonDomain + (lonDomain - 360);
    std::string msg = "MINLON [" + toString(minLon) +
      "] is greater than MAXLON [" + toString(maxLon) + "].  " +
      "If you meant to wrap around the [" + toString(lonDomain) +
      "] longitude " + "boundary, use a MAXLON of [" +
      toString(suggestedMaxLon) + "]";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  if (minLon < lonDomain - 360) {
    std::string msg = "MINLON [" + toString(minLon) +
      "] is less than [" + toString(lonDomain) + "] domain minimum [" +
      toString(lonDomain - 360) + "]";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  if (maxLon - minLon > 360) {
    int range = (int) (maxLon - minLon - 1);
    int loops = range / 360 + 1;
    std::string msg = "The specified longitude range [" + toString(minLon) +
      "] to [" + toString(maxLon) + "] seeds that same area of the target [" +
      toString(loops) + "] times";
    throw IException(IException::User, msg, _FILEINFO_);
  }
}


// Helper function to print out mapfile to session log
void printMap() {
  UserInterface &ui = Application::GetUserInterface();

  // Get mapping group from map file
  Pvl userMap;
  userMap.read(ui.GetFileName("MAP").toStdString());
  PvlGroup &userGrp = userMap.findGroup("Mapping", Pvl::Traverse);

  //Write map file out to the log
  Isis::Application::GuiLog(userGrp);
}
