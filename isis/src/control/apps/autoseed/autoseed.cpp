/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "autoseed.h"

#include <map>
#include <sstream>

#include <QString>

#include "geos/util/GEOSException.h"

#include "Application.h"
#include "Brick.h"
#include "Camera.h"
#include "CameraFactory.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "Cube.h"
#include "ID.h"
#include "IException.h"
#include "ImageOverlap.h"
#include "ImageOverlapSet.h"
#include "IString.h"
#include "PolygonSeeder.h"
#include "PolygonSeederFactory.h"
#include "PolygonTools.h"
#include "Progress.h"
#include "ProjectionFactory.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "SerialNumberList.h"
#include "SpecialPixel.h"
#include "Target.h"
#include "TProjection.h"
#include "UniversalGroundMap.h"
#include "UserInterface.h"

using namespace std;

namespace Isis {

  void autoseed(UserInterface &ui, Pvl *log) {
    SerialNumberList serialNumbers(ui.GetFileName("FROMLIST"));

    if (ui.WasEntered("CNET")) {
      ControlNet precnet(ui.GetFileName("CNET"));
      autoseed(ui, serialNumbers, &precnet, log);
      return;
    }
    autoseed(ui, serialNumbers, nullptr, log);
  }

  void autoseed(UserInterface &ui, SerialNumberList &serialNumbers, ControlNet *precnet, Pvl *log) {
    // Get the AutoSeed PVL internalized
    Pvl seedDef(ui.GetFileName("DEFFILE"));

    PolygonSeeder *seeder = PolygonSeederFactory::Create(seedDef);
    Pvl invalidInput = seeder->InvalidInput();
    PvlGroup &unusedDefKeywords = invalidInput.findGroup(
                                    "PolygonSeederAlgorithm", Pvl::Traverse);

    // Get the distance from the edge of an image a measure must be
    double pixelsFromEdge = -1.0;
    if (seedDef.hasKeyword("PixelsFromEdge", Pvl::Traverse)) {
      pixelsFromEdge = seedDef.findKeyword("PixelsFromEdge", Pvl::Traverse);
      if (unusedDefKeywords.hasKeyword("PixelsFromEdge"))
        unusedDefKeywords.deleteKeyword("PixelsFromEdge");
    }

    // Get the Emission range
    double minEmission = 0.0;
    double maxEmission = 180.0;
    if (seedDef.hasKeyword("MinEmission", Pvl::Traverse)) {
      minEmission = seedDef.findKeyword("MinEmission", Pvl::Traverse);
      if (unusedDefKeywords.hasKeyword("MinEmission"))
        unusedDefKeywords.deleteKeyword("MinEmission");
    }
    if (seedDef.hasKeyword("MaxEmission", Pvl::Traverse)) {
      maxEmission = seedDef.findKeyword("MaxEmission", Pvl::Traverse);
      if (unusedDefKeywords.hasKeyword("MaxEmission"))
        unusedDefKeywords.deleteKeyword("MaxEmission");
    }

    // Get the Incidence range
    double minIncidence = 0.0;
    double maxIncidence = 180.0;
    if (seedDef.hasKeyword("MinIncidence", Pvl::Traverse)) {
      minIncidence = seedDef.findKeyword("MinIncidence", Pvl::Traverse);
      if (unusedDefKeywords.hasKeyword("MinIncidence"))
        unusedDefKeywords.deleteKeyword("MinIncidence");
    }
    if (seedDef.hasKeyword("MaxIncidence", Pvl::Traverse)) {
      maxIncidence = seedDef.findKeyword("MaxIncidence", Pvl::Traverse);
      if (unusedDefKeywords.hasKeyword("MaxIncidence"))
        unusedDefKeywords.deleteKeyword("MaxIncidence");
    }

    // Get the DN range
    bool hasDNRestriction = false;
    double minDN = -DBL_MAX;
    double maxDN = DBL_MAX;
    if (seedDef.hasKeyword("MinDN", Pvl::Traverse)) {
      minDN = seedDef.findKeyword("MinDN", Pvl::Traverse);
      hasDNRestriction = true;
      if (unusedDefKeywords.hasKeyword("MinDN"))
        unusedDefKeywords.deleteKeyword("MinDN");
    }
    if (seedDef.hasKeyword("MaxDN", Pvl::Traverse)) {
      maxDN = seedDef.findKeyword("MaxDN", Pvl::Traverse);
      hasDNRestriction = true;
      if (unusedDefKeywords.hasKeyword("MaxDN"))
        unusedDefKeywords.deleteKeyword("MaxDN");
    }

    // Get the resolution
    double minResolution = 0.0;
    double maxResolution = 0.0;
    if (seedDef.hasKeyword("MinResolution", Pvl::Traverse)) {
      minResolution  = seedDef.findKeyword("MinResolution", Pvl::Traverse);
      if (unusedDefKeywords.hasKeyword("MinResolution"))
        unusedDefKeywords.deleteKeyword("MinResolution");
    }
    if (seedDef.hasKeyword("MaxResolution", Pvl::Traverse)) {
      maxResolution  = seedDef.findKeyword("MaxResolution", Pvl::Traverse);
      if (unusedDefKeywords.hasKeyword("MaxResolution"))
        unusedDefKeywords.deleteKeyword("MaxResolution");
    }

    // Get seed domain for unit conversion, no keyword == XY
    SeedDomain seedDomain = XY;
    if (seedDef.hasKeyword("SeedDomain", Pvl::Traverse)) {
      IString domain = (QString) seedDef.findKeyword("SeedDomain", Pvl::Traverse);
      if (unusedDefKeywords.hasKeyword("SeedDomain"))
        unusedDefKeywords.deleteKeyword("SeedDomain");

      if (domain.UpCase() == "SAMPLELINE") {
        seedDomain = SampleLine;
      }
      else if (domain.UpCase() != "XY") {
        IString msg = "Invalid value provided for keywork [SeedDomain]";
        msg += " Possible values include [XY, SampleLine]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    // Grab the labels from the first filename in the SerialNumberList to get
    // some info
    Pvl cubeLab(serialNumbers.fileName(0));

    // Construct a Projection for converting between Lon/Lat and X/Y
    // This is used inside the seeding algorithms.
    // Note: Should this be an option to include this in the program?
    Pvl maplab;
    maplab.addGroup(PvlGroup("Mapping"));
    PvlGroup &mapGroup = maplab.findGroup("Mapping");

    // overwrite empty mapping group with TargetName, EquatorialRadius, PolarRadius
    mapGroup = Target::radiiGroup(cubeLab, mapGroup);
    // add rest of keywords to new mapping group
    mapGroup += PvlKeyword("LatitudeType", "Planetocentric");
    mapGroup += PvlKeyword("LongitudeDirection", "PositiveEast");
    mapGroup += PvlKeyword("LongitudeDomain", "360");
    mapGroup += PvlKeyword("CenterLatitude", "0.0");
    mapGroup += PvlKeyword("CenterLongitude", "0.0");
    mapGroup += PvlKeyword("ProjectionName", "Sinusoidal");
    //PolygonSeeder *seeder = PolygonSeederFactory::Create(seedDef);

    TProjection *proj = NULL;
    UniversalGroundMap *ugmap = NULL;
    mapGroup = Target::radiiGroup(cubeLab, mapGroup);
    if (seedDomain == XY) {
      proj = (TProjection *) ProjectionFactory::Create(maplab);
    }
    else if (seedDomain == SampleLine) {
      Cube cube;
      cube.open(serialNumbers.fileName(0));
      ugmap = new UniversalGroundMap(cube);
    }

    // Create the control net to store the points in.
    ControlNet cnet;
    cnet.SetTarget(maplab);
    cnet.SetNetworkId(ui.GetString("NETWORKID"));
    cnet.SetUserName(Application::UserName());
    cnet.SetDescription(ui.GetString("DESCRIPTION"));

    // Set up an automatic id generator for the point ids
    ID pointId = ID(ui.GetString("POINTID"));

    // Find all the overlaps between the images in the FROMLIST
    // The overlap polygon coordinates are in Lon/Lat order
    ImageOverlapSet overlaps;
    overlaps.ReadImageOverlaps(ui.GetFileName("OVERLAPLIST"));

    // Create a Universal Ground Map (UGM) for each image in the list
    int stats_noOverlap = 0;
    int stats_tolerance = 0;

    map<QString, UniversalGroundMap *> gMaps;
    for (int sn = 0; sn < serialNumbers.size(); ++sn) {
      // Create the UGM for the cube associated with this SN
      Cube cube(serialNumbers.fileName(sn), "r");
      gMaps.insert(std::pair<QString, UniversalGroundMap *>
                   (serialNumbers.serialNumber(sn), new UniversalGroundMap(cube)));
    }

    stringstream errors(stringstream::in | stringstream::out);
    int errorNum = 0;

    // Process each overlap area
    //   Seed measurments into it
    //   Store the measurments in the control network

    vector< geos::geom::Point *> points;
    if (precnet) {

      Progress progress;
      progress.SetText("Calculating Provided Control Net");
      progress.SetMaximumSteps(precnet->GetNumPoints());
      progress.CheckStatus();

      for (int i = 0 ; i < precnet->GetNumPoints(); i ++) {
        ControlPoint *cp = precnet->GetPoint(i);
        ControlMeasure *cm = cp->GetRefMeasure();
        QString c = serialNumbers.fileName(cm->GetCubeSerialNumber());
        Cube cube(c);
        Camera *cam = CameraFactory::Create(cube);
        cam->SetImage(cm->GetSample(), cm->GetLine());


        points.push_back(Isis::globalFactory->createPoint(geos::geom::Coordinate(
                           cam->UniversalLongitude(), cam->UniversalLatitude())).release());

        delete cam;
        cam = NULL;

        progress.CheckStatus();
      }

    }

    Progress progress;
    progress.SetText("Seeding Points");
    progress.SetMaximumSteps(overlaps.Size());
    progress.CheckStatus();

    int cpIgnoredCount = 0;
    int cmIgnoredCount = 0;

    for (int ov = 0; ov < overlaps.Size(); ++ov) {
      progress.CheckStatus();

      if (overlaps[ov]->Size() == 1) {
        stats_noOverlap++;
        continue;
      }

      // Checks if this overlap was already seeded
      if (precnet) {

        // Grabs the Multipolygon's Envelope for Lat/Lon comparison
        const geos::geom::MultiPolygon *lonLatPoly = overlaps[ov]->Polygon();

        bool overlapSeeded = false;
        for (unsigned int j = 0; j < lonLatPoly->getNumGeometries()  &&  !overlapSeeded; j ++) {
          const geos::geom::Geometry *lonLatGeom = lonLatPoly->getGeometryN(j);

          // Checks if Control Point is in the MultiPolygon using Lon/Lat
          for (unsigned int i = 0 ; i < points.size()  &&  !overlapSeeded; i ++) {
            if (lonLatGeom->contains(points[i])) overlapSeeded = true;
          }
        }

        if (overlapSeeded) continue;
      }

      // Seed this overlap with points
      const geos::geom::MultiPolygon *polygonOverlaps = overlaps[ov]->Polygon();
      std::vector<geos::geom::Point *> points;

      try {
        geos::geom::MultiPolygon *mp = NULL;
        if (seedDomain == XY) {
          mp = PolygonTools::LatLonToXY(*polygonOverlaps, proj);
        }
        else if (seedDomain == SampleLine) {
          mp = PolygonTools::LatLonToSampleLine(*polygonOverlaps, ugmap);
        }
        points = seeder->Seed(mp);
      }
      catch (IException &e) {

        if (ui.WasEntered("ERRORS")) {

          if (errorNum > 0) {
            errors << endl;
          }
          errorNum ++;

          errors << e.toPvl().group(0).findKeyword("Message")[0];
          for (int serNum = 0; serNum < overlaps[ov]->Size(); serNum++) {
            if (serNum == 0) {
              errors << ": ";
            }
            else {
              errors << ", ";
            }
            errors << (*overlaps[ov])[serNum];
          }
        }

        continue;
      }

      // No points were seeded in this polygon, so collect some stats and move on
      if (points.size() == 0) {
        stats_tolerance++;
        continue;
      }

      vector<geos::geom::Point *> seed;
      if (seedDomain == XY) {
        // Convert the X/Y points back to Lat/Lon points
        for (unsigned int pt = 0; pt < points.size(); pt ++) {
          if (proj->SetCoordinate(points[pt]->getX(), points[pt]->getY())) {
            seed.push_back(Isis::globalFactory->createPoint(
                             geos::geom::Coordinate(proj->UniversalLongitude(),
                                                    proj->UniversalLatitude())).release());
          }
          else {
            IString msg = "Unable to convert from X/Y to a (lon,lat)";
            throw IException(IException::Unknown, msg, _FILEINFO_);
          }
        }
      }
      else if (seedDomain == SampleLine) {
        // Convert the Sample/Line points back to Lat/Lon points
        for (unsigned int pt = 0; pt < points.size(); pt ++) {
          if (ugmap->SetImage(points[pt]->getX(), points[pt]->getY())) {
            seed.push_back(Isis::globalFactory->createPoint(
                             geos::geom::Coordinate(ugmap->UniversalLongitude(),
                                                    ugmap->UniversalLatitude())).release());
          }
          else {
            IString msg = "Unable to convert from Sample/Line to a (lon,lat)";
            throw IException(IException::Unknown, msg, _FILEINFO_);
          }
        }
      }

      //   Create a control point for each seeded point in this overlap
      for (unsigned int point = 0; point < seed.size(); ++point) {

        ControlPoint *controlpt = new ControlPoint();
        controlpt->SetId(pointId.Next());
        controlpt->SetType(ControlPoint::Free);

        // Create a measurment at this point for each image in the overlap area
        for (int sn = 0; sn < overlaps[ov]->Size(); ++sn) {
          bool ignore = false;

          // Get the line/sample of the lat/lon for this cube
          UniversalGroundMap *gmap = gMaps[(*overlaps[ov])[sn]];

          if (!gmap) {
            QString msg = "Unable to create a Universal Ground for Serial Number [";
            msg += (*overlaps[ov])[sn] + "] The associated image is more than ";
            msg += "likely missing from your FROMLIST.";
            throw IException(IException::User, msg, _FILEINFO_);
          }

          if (!gmap->SetUniversalGround(seed[point]->getY(), seed[point]->getX())) {
            // This error is more than likely due to floating point roundoff
            continue;
          }

          // Check the line/sample with the gmap for image edge
          if (pixelsFromEdge > gmap->Sample() || pixelsFromEdge > gmap->Line()
              || gmap->Sample() > gmap->Camera()->Samples() - pixelsFromEdge
              || gmap->Line() > gmap->Camera()->Lines() - pixelsFromEdge) {
            ignore = true;
          }

          // Check the Emission/Incidence Angle with the camera from the gmap
          if (gmap->Camera()->EmissionAngle() < minEmission ||
              gmap->Camera()->EmissionAngle() > maxEmission) {
            ignore = true;
          }
          if (gmap->Camera()->IncidenceAngle() < minIncidence ||
              gmap->Camera()->IncidenceAngle() > maxIncidence) {
            ignore = true;
          }

          // Check the DNs with the cube, Note: this is costly to do
          if (hasDNRestriction) {
            Cube cube;
            QString c = serialNumbers.fileName((*overlaps[ov])[sn]);
            cube.open(c);
            Brick brick(1, 1, 1, cube.pixelType());
            brick.SetBasePosition((int)gmap->Camera()->Sample(), (int)gmap->Camera()->Line(), (int)gmap->Camera()->Band());
            cube.read(brick);
            if (Isis::IsSpecial(brick[0]) || brick[0] > maxDN || brick[0] < minDN) {
              ignore = true;
            }
          }

          // Check the Resolution with the camera from the gmap
          if (gmap->Resolution() < minResolution ||
              (maxResolution > 0.0 && gmap->Resolution() > maxResolution)) {
            ignore = true;
          }

          // Put the line/samp into a measurment
          ControlMeasure *measurement = new ControlMeasure();
          measurement->SetAprioriSample(gmap->Sample());
          measurement->SetAprioriLine(gmap->Line());
          measurement->SetCoordinate(gmap->Sample(), gmap->Line(),
                                    ControlMeasure::Candidate);

          measurement->SetType(ControlMeasure::Candidate);
          measurement->SetCubeSerialNumber((*(overlaps[ov]))[sn]);
          measurement->SetIgnored(ignore);

          if (ignore) {
            cmIgnoredCount ++;
          }

          controlpt->Add(measurement); //controlpt takes ownership
          measurement = NULL;
        }

        if (controlpt->GetNumValidMeasures() < 2) {
          controlpt->SetIgnored(true);
          cpIgnoredCount ++;
        }

        if (controlpt->GetNumMeasures() > 0) {
          cnet.AddPoint(controlpt); //cnet takes ownership
        }
        delete seed[point];

      } // End of create control points loop

    } // End of seeding loop

    // All done with the UGMs so delete them
    for (unsigned int sn = 0; sn < gMaps.size(); ++sn) {
      UniversalGroundMap *gmap = gMaps[serialNumbers.serialNumber(sn)];
      delete gmap;
    }
    gMaps.clear();

    for (unsigned int i = 0 ; i < points.size(); i ++) {
      delete points[i];
      points[i] = NULL;
    }

    //Log the ERRORS file
    if (ui.WasEntered("ERRORS") && errorNum > 0) {
      QString errorname = ui.GetFileName("ERRORS");
      std::ofstream errorsfile;
      errorsfile.open(errorname.toLatin1().data());
      errorsfile << errors.str();
      errorsfile.close();
    }

    // Make sure the control network is not empty
    if (cnet.GetNumPoints() == 0) {
      QString msg = "The ouput control network is empty. This is likely due";
      msg += " to the input cubes failing to overlap.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Write the control network out
    cnet.Write(ui.GetFileName("ONET"));

    // create SeedDef group and add to print.prt
    PvlGroup pluginInfo = seeder->PluginParameters("SeedDefinition");
    pluginInfo.addKeyword(PvlKeyword("MaxIncidence", toString(maxIncidence)));
    pluginInfo.addKeyword(PvlKeyword("MaxEmission", toString(maxEmission)));
    Application::Log(pluginInfo);

    // inform user of any unused (invalid) keywords found in the def file
    if (unusedDefKeywords.keywords() != 0) {
      PvlGroup unusedKeywords(unusedDefKeywords);
      unusedKeywords.setName("InvalidKeyordsFoundInDefFile");
      Application::Log(unusedKeywords);
    }

    // calc # of points and measures for results group in print.prt
    int cpCount = cnet.GetNumPoints();
    int msCount = 0;
    for (int i = 0; i < cpCount; i++) {
      msCount += cnet.GetPoint(i)->GetNumMeasures();
    }

    // create Results group and add to print.prt
    PvlKeyword cpCountKeyword("ControlPointCount", toString(cpCount));
    PvlKeyword msCountKeyword("ControlMeasureCount", toString(msCount));
    PvlKeyword cpIgnoredCountKeyword("ControlPointsIgnored", toString(cpIgnoredCount));
    PvlKeyword cmIgnoredCountKeyword("ControlMeasuresIgnored", toString(cmIgnoredCount));

    PvlGroup resultsGrp("Results");
    resultsGrp.addKeyword(cpCountKeyword);
    resultsGrp.addKeyword(msCountKeyword);
    resultsGrp.addKeyword(cpIgnoredCountKeyword);
    resultsGrp.addKeyword(cmIgnoredCountKeyword);
    Application::Log(resultsGrp);

    if (seedDomain == XY) {
      delete proj;
      proj = NULL;
    }
    else if (seedDomain == SampleLine) {
      delete ugmap;
      ugmap = NULL;
    }

    delete seeder;
    seeder = NULL;
  }
}
