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
    cubeFileList.read(ui.GetFileName("FROMLIST").toStdString());

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
      std::string msg = "The list file[" + ui.GetFileName("FROMLIST") +
                    " does not contain any filenames";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    Pvl log;
    Process p;

    QString projection("Equirectangular");
    if(ui.WasEntered("MAP")) {
      Pvl mapfile(ui.GetFileName("MAP").toStdString());
      projection = QString::fromStdString(mapfile.findGroup("Mapping")["ProjectionName"]);
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
          cube.open(QString::fromStdString(cubeFileList[i].toString()));

          int lines = cube.lineCount();
          int samples = cube.sampleCount();

          PvlObject fmap("File");
          fmap += PvlKeyword("Name", cubeFileList[i].toString());
          fmap += PvlKeyword("Lines", std::to_string(lines));
          fmap += PvlKeyword("Samples", std::to_string(samples));

          Camera *cam = cube.camera();
          Pvl mapping;
          cam->BasicMapping(mapping);
          PvlGroup &mapgrp = mapping.findGroup("Mapping");
          mapgrp.addKeyword(PvlKeyword("ProjectionName", projection.toStdString()), Pvl::Replace);
          mapgrp.addKeyword(PvlKeyword("LatitudeType", lattype.toStdString()), Pvl::Replace);
          mapgrp.addKeyword(PvlKeyword("LongitudeDirection", londir.toStdString()), Pvl::Replace);
          mapgrp.addKeyword(PvlKeyword("LongitudeDomain", londom.toStdString()), Pvl::Replace);

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

          mapgrp.addKeyword(PvlKeyword("PixelResolution", std::to_string(pixres)), Pvl::Replace);
          mapgrp.addKeyword(PvlKeyword("Scale", std::to_string(scale), "pixels/degree"), Pvl::Replace);
          mapgrp += PvlKeyword("MinPixelResolution", std::to_string(lowres), "meters/pixel");
          mapgrp += PvlKeyword("MaxPixelResolution", std::to_string(hires), "meters/pixel");
          mapgrp += PvlKeyword("MinObliquePixelResolution", std::to_string(lowObliqueRes), "meters/pixel");
          mapgrp += PvlKeyword("MaxObliquePixelResolution", std::to_string(hiObliqueRes), "meters/pixel");

          // Get the universal ground range
          double minlat, maxlat, minlon, maxlon;
          cam->GroundRange(minlat, maxlat, minlon, maxlon, mapping);
          mapgrp.addKeyword(PvlKeyword("MinimumLatitude", std::to_string(minlat)), Pvl::Replace);
          mapgrp.addKeyword(PvlKeyword("MaximumLatitude", std::to_string(maxlat)), Pvl::Replace);
          mapgrp.addKeyword(PvlKeyword("MinimumLongitude", std::to_string(minlon)), Pvl::Replace);
          mapgrp.addKeyword(PvlKeyword("MaximumLongitude", std::to_string(maxlon)), Pvl::Replace);

          fmap.addGroup(mapgrp);
          fileset.addObject(fmap);

          longitudeStat.AddData(&minlon, 1);
          longitudeStat.AddData(&maxlon, 1);
          latitudeStat.AddData(&minlat, 1);
          latitudeStat.AddData(&maxlat, 1);
        }
        catch(IException &ie) {
          std::string mess = cubeFileList[i].toString() + " - " + ie.what();
          fmap += PvlKeyword("Error", mess);
          errorset.addObject(fmap);

          badfiles.append(qMakePair(QString::fromStdString(cubeFileList[i].toString()), ie.what()));
        }

        p.ClearInputCubes();
        prog.CheckStatus();
    }

    // Now check for error behavior if a problem was encountered
    if ( badfiles.size() > 0 ) {

        if (  ui.WasEntered("ERRORLOG") ) {
          Pvl temp;
          temp.addObject(errorset);
          temp.write(ui.GetFileName("ERRORLOG", "log").toStdString());
        }

        if ( ui.WasEntered("ERRORLIST") ) {
          FileName filename( ui.GetFileName("ERRORLIST").toStdString() );
          QFile logfile(QString::fromStdString(filename.expanded()));
          if ( !logfile.open(QIODevice::WriteOnly | QIODevice::Truncate |
                             QIODevice::Text | QIODevice::Unbuffered) ) {
            std::string mess = "Unable to open/create error list file " + filename.name();
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
          std::string errors("--> Fatal Errors Encountered <___\n");
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

    mapping += PvlKeyword("ProjectionName", projection.toStdString());
    mapping += PvlKeyword("TargetName", target.toStdString());
    mapping += PvlKeyword("EquatorialRadius", std::to_string(eqRad), "meters");
    mapping += PvlKeyword("PolarRadius", std::to_string(poleRad), "meters");
    mapping += PvlKeyword("LatitudeType", lattype.toStdString());
    mapping += PvlKeyword("LongitudeDirection", londir.toStdString());
    mapping += PvlKeyword("LongitudeDomain", londom.toStdString());
    mapping += PvlKeyword("PixelResolution", std::to_string(SetRound(avgPixRes, digits)), "meters/pixel");
    mapping += PvlKeyword("Scale", std::to_string(SetRound(scale, digits)), "pixels/degree");
    mapping += PvlKeyword("MinPixelResolution", std::to_string(scaleStat.Minimum()), "meters/pixel");
    mapping += PvlKeyword("MaxPixelResolution", std::to_string(scaleStat.Maximum()), "meters/pixel");
    mapping += PvlKeyword("MinObliquePixelResolution", std::to_string(obliqueScaleStat.Minimum()),
                          "meters/pixel");
    mapping += PvlKeyword("MaxObliquePixelResolution", std::to_string(obliqueScaleStat.Maximum()),
                          "meters/pixel");
    mapping += PvlKeyword("CenterLongitude", std::to_string(SetRound(avgLon, digits)));
    mapping += PvlKeyword("CenterLatitude",  std::to_string(SetRound(avgLat, digits)));
    mapping += PvlKeyword("MinimumLatitude", std::to_string(std::max(SetFloor(latitudeStat.Minimum(),
                                                                   digits), -90.0)));
    mapping += PvlKeyword("MaximumLatitude", std::to_string(std::min(SetCeil(latitudeStat.Maximum(),
                                                                   digits), 90.0)));
    mapping += PvlKeyword("MinimumLongitude", std::to_string(std::max(SetFloor(longitudeStat.Minimum(),
                                                                    digits), -180.0)));
    mapping += PvlKeyword("MaximumLongitude", std::to_string(std::min(SetCeil(longitudeStat.Maximum(),
                                                                   digits), 360.0)));

    PvlKeyword clat("PreciseCenterLongitude", std::to_string(avgLon));
    clat.addComment("Actual Parameters without precision applied");
    mapping += clat;
    mapping += PvlKeyword("PreciseCenterLatitude",  std::to_string(avgLat));
    mapping += PvlKeyword("PreciseMinimumLatitude", std::to_string(latitudeStat.Minimum()));
    mapping += PvlKeyword("PreciseMaximumLatitude", std::to_string(latitudeStat.Maximum()));
    mapping += PvlKeyword("PreciseMinimumLongitude", std::to_string(longitudeStat.Minimum()));
    mapping += PvlKeyword("PreciseMaximumLongitude", std::to_string(longitudeStat.Maximum()));

    Application::AppendAndLog(mapping, &log);

    // Write the output file if requested
    if(ui.WasEntered("TO")) {
      Pvl temp;
      temp.addGroup(mapping);
      temp.write(ui.GetFileName("TO", "map").toStdString());
    }

    if(ui.WasEntered("LOG")) {
      Pvl temp;
      temp.addObject(fileset);
      temp.write(ui.GetFileName("LOG", "log").toStdString());
    }

    p.EndProcess();

    return log;
  }
}


