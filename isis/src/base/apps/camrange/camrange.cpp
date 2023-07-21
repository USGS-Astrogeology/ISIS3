#include "Application.h"
#include "Camera.h"
#include "Target.h"
#include "Distance.h"
#include "Process.h"
#include "Pvl.h"
#include "UserInterface.h"

#include "camrange.h"

using namespace Isis;

namespace Isis {
  
  void camrange(UserInterface &ui, Pvl *log) {
    Cube *cube = new Cube( ui.GetCubeName("FROM"), "r");
    camrange(cube, ui, log);
  }

  void camrange(Cube *incube, UserInterface &ui, Pvl *log) {
    Process p;

    // Set the input image, get the camera model, and a basic mapping
    // group
    Camera *cam = incube->camera();
    Pvl mapping;
    cam->BasicMapping(mapping);
    PvlGroup &mapgrp = mapping.findGroup("Mapping");

    // Setup the log->results by first adding the filename
    // Get the radii
    Distance radii[3];
    cam->radii(radii);
    Target *camTarget = cam->target();
    PvlGroup target("Target");
    target += PvlKeyword("From", ui.GetCubeName("FROM"));
    target += PvlKeyword("TargetName", camTarget->name());
    target += PvlKeyword("RadiusA", toString(radii[0].meters()), "meters");
    target += PvlKeyword("RadiusB", toString(radii[1].meters()), "meters");
    target += PvlKeyword("RadiusC", toString(radii[2].meters()), "meters");

    // Get resolution
    PvlGroup res("PixelResolution");
    double lowres = cam->LowestImageResolution();
    double hires = cam->HighestImageResolution();
    res += PvlKeyword("Lowest", toString(lowres), "meters");
    res += PvlKeyword("Highest", toString(hires), "meters");

    // Get the universal ground range
    PvlGroup ugr("UniversalGroundRange");
    double minlat, maxlat, minlon, maxlon;
    cam->GroundRange(minlat, maxlat, minlon, maxlon, mapping);
    ugr += PvlKeyword("LatitudeType", "Planetocentric");
    ugr += PvlKeyword("LongitudeDirection", "PositiveEast");
    ugr += PvlKeyword("LongitudeDomain", "360");
    ugr += PvlKeyword("MinimumLatitude", toString(minlat));
    ugr += PvlKeyword("MaximumLatitude", toString(maxlat));
    ugr += PvlKeyword("MinimumLongitude", toString(minlon));
    ugr += PvlKeyword("MaximumLongitude", toString(maxlon));

    // Get the ographic latitude range
    mapgrp.addKeyword(PvlKeyword("LatitudeType", "Planetographic"),
                      Pvl::Replace);
    cam->GroundRange(minlat, maxlat, minlon, maxlon, mapping);
    PvlGroup ogr("LatitudeRange");
    ogr += PvlKeyword("LatitudeType", "Planetographic");
    ogr += PvlKeyword("MinimumLatitude", toString(minlat));
    ogr += PvlKeyword("MaximumLatitude", toString(maxlat));

    // Get positive west longitude coordinates in 360 domain
    mapgrp.addKeyword(PvlKeyword("LongitudeDirection", "PositiveWest"),
                      Pvl::Replace);
    cam->GroundRange(minlat, maxlat, minlon, maxlon, mapping);
    PvlGroup pos360("PositiveWest360");
    pos360 += PvlKeyword("LongitudeDirection", "PositiveWest");
    pos360 += PvlKeyword("LongitudeDomain", "360");
    pos360 += PvlKeyword("MinimumLongitude", toString(minlon));
    pos360 += PvlKeyword("MaximumLongitude", toString(maxlon));

    // Get positive east longitude coordinates in 180 domain
    mapgrp.addKeyword(PvlKeyword("LongitudeDirection", "PositiveEast"),
                      Pvl::Replace);
    mapgrp.addKeyword(PvlKeyword("LongitudeDomain", "180"),
                      Pvl::Replace);
    cam->GroundRange(minlat, maxlat, minlon, maxlon, mapping);
    PvlGroup pos180("PositiveEast180");
    pos180 += PvlKeyword("LongitudeDirection", "PositiveEast");
    pos180 += PvlKeyword("LongitudeDomain", "180");
    pos180 += PvlKeyword("MinimumLongitude", toString(minlon));
    pos180 += PvlKeyword("MaximumLongitude", toString(maxlon));

    // Get positive west longitude coordinates in 180 domain
    mapgrp.addKeyword(PvlKeyword("LongitudeDirection", "PositiveWest"),
                      Pvl::Replace);
    cam->GroundRange(minlat, maxlat, minlon, maxlon, mapping);
    PvlGroup neg180("PositiveWest180");
    neg180 += PvlKeyword("LongitudeDirection", "PositiveWest");
    neg180 += PvlKeyword("LongitudeDomain", "180");
    neg180 += PvlKeyword("MinimumLongitude", toString(minlon));
    neg180 += PvlKeyword("MaximumLongitude", toString(maxlon));

    Application::AppendAndLog(target, log);
    Application::AppendAndLog(res, log);
    Application::AppendAndLog(ugr, log);
    Application::AppendAndLog(ogr, log);
    Application::AppendAndLog(pos360, log);
    Application::AppendAndLog(pos180, log);
    Application::AppendAndLog(neg180, log);

    // Write the log->file if requested
    if(ui.WasEntered("TO")) {
      log->write(ui.GetFileName("TO", "txt"));
    }

    p.EndProcess();
  }
}
