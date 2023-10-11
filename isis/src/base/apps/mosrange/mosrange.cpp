/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "mosrange.h"

#include <cmath>

#include <QPair>
#include <QList>
#include <QFile>

#include "Application.h"
#include "Camera.h"
#include "Cube.h"
#include "Distance.h"
#include "Process.h"
#include "Pvl.h"
#include "Statistics.h"
#include "Target.h"

using namespace std;

namespace Isis {

  static inline double SetFloor(double value, const int precision) {
    double scale = pow(10.0, precision);
    value = floor(value * scale) / scale;
    return (value);
  }

  static inline double SetRound(double value, const int precision) {
    double scale = pow(10.0, precision);
    value = rint(value * scale) / scale;
    return (value);
  }

  static inline double SetCeil(double value, const int precision) {
    double scale = pow(10.0, precision);
    value = ceil(value * scale) / scale;
    return (value);
  }

  static inline double Scale(const double pixres, const double polarRadius,
                      const double equiRadius, const double trueLat = 0.0) {
    double lat = trueLat * Isis::PI / 180.0;
    double a = polarRadius * cos(lat);
    double b = equiRadius * sin(lat);
    double localRadius = equiRadius * polarRadius / sqrt(a * a + b * b);
    return (localRadius / pixres * pi_c() / 180.0);
  }

  /**
   * Compute lat/lon range of a set of camera images for mosaicking
   *
   * @param ui UserInterface object containing parameters
   * @return Pvl results log file
   */
  Pvl mosrange(UserInterface &ui) {

    // Get the list of names of input cubes to stitch together
    FileList cubeFileList;
    cubeFileList.read(ui.GetFileName("FROMLIST"));

    return mosrange(cubeFileList, ui);
  }


  /**
   * Compute lat/lon range of a set of camera images for mosaicking
   *
   * @param FileList List of cube filenames
   * @return Pvl results log file
   *
   * @throws IException::User "The list file [FILENAME] does not contain any filenames"
   * @throws IException::User "--> Fatal Errors Encountered <___ [FILENAMES]"
   * @throws IException::User "Unable to open/create error list file [FILENAME]"
   */
  Pvl mosrange(FileList &cubeFileList, UserInterface &ui) {
    if ( cubeFileList.size() < 1)  {
      QString msg = "The list file[" + ui.GetFileName("FROMLIST") +
                    " does not contain any filenames";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    Pvl log;
    Process p;

    QString projection("Equirectangular");
    if(ui.WasEntered("MAP")) {
      Pvl mapfile(ui.GetFileName("MAP"));
      projection = (QString) mapfile.findGroup("Mapping")["ProjectionName"];
    }

    if(ui.WasEntered("PROJECTION")) {
      projection = ui.GetString("PROJECTION");
    }

    // Gather other user inputs to projection
    QString lattype = ui.GetString("LATTYPE");
    QString londir  = ui.GetString("LONDIR");
    QString londom  = ui.GetString("LONDOM");
    int digits = ui.GetInteger("PRECISION");

    // Fix them for mapping group
    lattype = (lattype == "PLANETOCENTRIC") ? "Planetocentric" : "Planetographic";
    londir = (londir == "POSITIVEEAST") ? "PositiveEast" : "PositiveWest";

    Progress prog;
    prog.SetMaximumSteps(cubeFileList.size());
    prog.CheckStatus();

    Statistics scaleStat;
    Statistics obliqueScaleStat;
    Statistics longitudeStat;
    Statistics latitudeStat;
    Statistics equiRadStat;
    Statistics poleRadStat;
    PvlObject fileset("FileSet");
    PvlObject errorset("ErrorSet");

    // Save major equitorial and polar radii for last occurring
    double eqRad;
    double poleRad;

    QString target("Unknown");
    QList<QPair<QString, QString> > badfiles;
    for(int i = 0 ; i < cubeFileList.size() ; i++) {

        PvlObject fmap("File");
        fmap += PvlKeyword("Name", cubeFileList[i].toString());

        try {
          // Set input image, get camera model, and a basic mapping group
          Cube cube;
          cube.open(cubeFileList[i].toString());

          int lines = cube.lineCount();
          int samples = cube.sampleCount();

          PvlObject fmap("File");
          fmap += PvlKeyword("Name", cubeFileList[i].toString());
          fmap += PvlKeyword("Lines", toString(lines));
          fmap += PvlKeyword("Samples", toString(samples));

          Camera *cam = cube.camera();
          Pvl mapping;
          cam->BasicMapping(mapping);
          PvlGroup &mapgrp = mapping.findGroup("Mapping");
          mapgrp.addKeyword(PvlKeyword("ProjectionName", projection), Pvl::Replace);
          mapgrp.addKeyword(PvlKeyword("LatitudeType", lattype), Pvl::Replace);
          mapgrp.addKeyword(PvlKeyword("LongitudeDirection", londir), Pvl::Replace);
          mapgrp.addKeyword(PvlKeyword("LongitudeDomain", londom), Pvl::Replace);

          // Get the radii
          Distance radii[3];
          cam->radii(radii);

          eqRad   = radii[0].meters();
          poleRad = radii[2].meters();

          target = cam->target()->name();
          equiRadStat.AddData(&eqRad, 1);
          poleRadStat.AddData(&poleRad, 1);

          // Get resolution
          double lowres = cam->LowestImageResolution();
          double hires = cam->HighestImageResolution();

          double lowObliqueRes = cam->LowestObliqueImageResolution();
          double hiObliqueRes= cam->HighestObliqueImageResolution();

          scaleStat.AddData(&hires, 1);
          scaleStat.AddData(&lowres, 1);

          obliqueScaleStat.AddData(&hiObliqueRes,1);
          obliqueScaleStat.AddData(&lowObliqueRes,1);

          double pixres = (lowres + hires) / 2.0;
          double scale = Scale(pixres, poleRad, eqRad);

          mapgrp.addKeyword(PvlKeyword("PixelResolution", toString(pixres)), Pvl::Replace);
          mapgrp.addKeyword(PvlKeyword("Scale", toString(scale), "pixels/degree"), Pvl::Replace);
          mapgrp += PvlKeyword("MinPixelResolution", toString(lowres), "meters/pixel");
          mapgrp += PvlKeyword("MaxPixelResolution", toString(hires), "meters/pixel");
          mapgrp += PvlKeyword("MinObliquePixelResolution", toString(lowObliqueRes), "meters/pixel");
          mapgrp += PvlKeyword("MaxObliquePixelResolution", toString(hiObliqueRes), "meters/pixel");

          // Get the universal ground range
          double minlat, maxlat, minlon, maxlon;
          cam->GroundRange(minlat, maxlat, minlon, maxlon, mapping);
          mapgrp.addKeyword(PvlKeyword("MinimumLatitude", toString(minlat)), Pvl::Replace);
          mapgrp.addKeyword(PvlKeyword("MaximumLatitude", toString(maxlat)), Pvl::Replace);
          mapgrp.addKeyword(PvlKeyword("MinimumLongitude", toString(minlon)), Pvl::Replace);
          mapgrp.addKeyword(PvlKeyword("MaximumLongitude", toString(maxlon)), Pvl::Replace);

          fmap.addGroup(mapgrp);
          fileset.addObject(fmap);

          longitudeStat.AddData(&minlon, 1);
          longitudeStat.AddData(&maxlon, 1);
          latitudeStat.AddData(&minlat, 1);
          latitudeStat.AddData(&maxlat, 1);
        }
        catch(IException &ie) {
          QString mess = cubeFileList[i].toString() + " - " + ie.what();
          fmap += PvlKeyword("Error", mess);
          errorset.addObject(fmap);

          badfiles.append(qMakePair(cubeFileList[i].toString(), ie.what()));
        }

        p.ClearInputCubes();
        prog.CheckStatus();
    }

    // Now check for error behavior if a problem was encountered
    if ( badfiles.size() > 0 ) {

        if (  ui.WasEntered("ERRORLOG") ) {
          Pvl temp;
          temp.addObject(errorset);
          temp.write(ui.GetFileName("ERRORLOG", "log"));
        }

        if ( ui.WasEntered("ERRORLIST") ) {
          FileName filename( ui.GetFileName("ERRORLIST") );
          QFile logfile(filename.expanded());
          if ( !logfile.open(QIODevice::WriteOnly | QIODevice::Truncate |
                             QIODevice::Text | QIODevice::Unbuffered) ) {
            QString mess = "Unable to open/create error list file " + filename.name();
            throw IException(IException::User, mess, _FILEINFO_);
          }

          QTextStream lout(&logfile);
          for ( int f = 0  ; f < badfiles.size() ; f++) {
             lout << badfiles[f].first << "\n";
          }
        }

        // Now check onerror status
        if ( ("FAIL" == ui.GetString("ONERROR").toUpper()) ||
             (badfiles.size() == cubeFileList.size()) ) {
          QString errors("--> Fatal Errors Encountered <___\n");
          for (int i = 0 ; i < badfiles.size() ; i++) {
              errors += badfiles[i].first + " - " + badfiles[i].second + "\n";
          }
          throw IException(IException::User, errors, _FILEINFO_);
        }
    }

    // Construct the output mapping group with statistics
    PvlGroup mapping("Mapping");
    double avgPixRes( (scaleStat.Minimum() + scaleStat.Maximum() ) / 2.0);
    double avgLat((latitudeStat.Minimum() + latitudeStat.Maximum()) / 2.0);
    double avgLon((longitudeStat.Minimum() + longitudeStat.Maximum()) / 2.0);
    double avgEqRad((equiRadStat.Minimum() + equiRadStat.Maximum()) / 2.0);
    double avgPoleRad((poleRadStat.Minimum() + poleRadStat.Maximum()) / 2.0);
    double scale  = Scale(avgPixRes, avgPoleRad, avgEqRad);

    mapping += PvlKeyword("ProjectionName", projection);
    mapping += PvlKeyword("TargetName", target);
    mapping += PvlKeyword("EquatorialRadius", toString(eqRad), "meters");
    mapping += PvlKeyword("PolarRadius", toString(poleRad), "meters");
    mapping += PvlKeyword("LatitudeType", lattype);
    mapping += PvlKeyword("LongitudeDirection", londir);
    mapping += PvlKeyword("LongitudeDomain", londom);
    mapping += PvlKeyword("PixelResolution", toString(SetRound(avgPixRes, digits)), "meters/pixel");
    mapping += PvlKeyword("Scale", toString(SetRound(scale, digits)), "pixels/degree");
    mapping += PvlKeyword("MinPixelResolution", toString(scaleStat.Minimum()), "meters/pixel");
    mapping += PvlKeyword("MaxPixelResolution", toString(scaleStat.Maximum()), "meters/pixel");
    mapping += PvlKeyword("MinObliquePixelResolution", toString(obliqueScaleStat.Minimum()),
                          "meters/pixel");
    mapping += PvlKeyword("MaxObliquePixelResolution", toString(obliqueScaleStat.Maximum()),
                          "meters/pixel");
    mapping += PvlKeyword("CenterLongitude", toString(SetRound(avgLon, digits)));
    mapping += PvlKeyword("CenterLatitude",  toString(SetRound(avgLat, digits)));
    mapping += PvlKeyword("MinimumLatitude", toString(std::max(SetFloor(latitudeStat.Minimum(),
                                                                   digits), -90.0)));
    mapping += PvlKeyword("MaximumLatitude", toString(std::min(SetCeil(latitudeStat.Maximum(),
                                                                   digits), 90.0)));
    mapping += PvlKeyword("MinimumLongitude", toString(std::max(SetFloor(longitudeStat.Minimum(),
                                                                    digits), -180.0)));
    mapping += PvlKeyword("MaximumLongitude", toString(std::min(SetCeil(longitudeStat.Maximum(),
                                                                   digits), 360.0)));

    PvlKeyword clat("PreciseCenterLongitude", toString(avgLon));
    clat.addComment("Actual Parameters without precision applied");
    mapping += clat;
    mapping += PvlKeyword("PreciseCenterLatitude",  toString(avgLat));
    mapping += PvlKeyword("PreciseMinimumLatitude", toString(latitudeStat.Minimum()));
    mapping += PvlKeyword("PreciseMaximumLatitude", toString(latitudeStat.Maximum()));
    mapping += PvlKeyword("PreciseMinimumLongitude", toString(longitudeStat.Minimum()));
    mapping += PvlKeyword("PreciseMaximumLongitude", toString(longitudeStat.Maximum()));

    Application::AppendAndLog(mapping, &log);

    // Write the output file if requested
    if(ui.WasEntered("TO")) {
      Pvl temp;
      temp.addGroup(mapping);
      temp.write(ui.GetFileName("TO", "map"));
    }

    if(ui.WasEntered("LOG")) {
      Pvl temp;
      temp.addObject(fileset);
      temp.write(ui.GetFileName("LOG", "log"));
    }

    p.EndProcess();

    return log;
  }
}


