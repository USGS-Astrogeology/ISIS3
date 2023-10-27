/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <map>
#include <set>

#include <QList>
#include <QMap>
#include <QScopedPointer>
#include <QSet>
#include <QString>

#include <geos/geom/Coordinate.h>
#include <geos/geom/Envelope.h>
#include <geos/geom/Geometry.h>
#include <geos/geom/MultiPolygon.h>
#include <geos/geom/Point.h>
#include <geos/index/strtree/STRtree.h>

#include "Camera.h"
#include "CameraFactory.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlNetValidMeasure.h"
#include "ControlPoint.h"
#include "CubeManager.h"
#include "FileList.h"
#include "FileName.h"
#include "ImagePolygon.h"
#include "Latitude.h"
#include "Longitude.h"
#include "Pvl.h"
#include "SerialNumber.h"
#include "SerialNumberList.h"
#include "SurfacePoint.h"
#include "UserInterface.h"
#include "IException.h"
#include "iTime.h"

using geos::geom::Coordinate;
using geos::geom::Envelope;
using geos::geom::Geometry;
using geos::geom::MultiPolygon;
using geos::geom::Point;
using geos::index::strtree::STRtree;

using namespace std;
using namespace Isis;

void setControlPointLatLon(SerialNumberList &snl, ControlNet &cnet);
QList<ControlPoint *> getValidPoints(Cube &cube, STRtree &coordTree);

std::map< QString, SurfacePoint > g_surfacePoints;
QMap< QString, QSet<QString> > g_modifications;


void IsisMain() {

  UserInterface &ui = Application::GetUserInterface();

  FileList addList(ui.GetFileName("ADDLIST"));

  bool log = false;
  FileName logFile;
  if (ui.WasEntered("LOG")) {
    log = true;
    logFile = ui.GetFileName("LOG");
  }
  Pvl results;
  results.setName("cnetadd_Results");
  PvlKeyword added("FilesAdded");
  PvlKeyword omitted("FilesOmitted");
  PvlKeyword pointsModified("PointsModified");

  bool checkMeasureValidity = ui.WasEntered("DEFFILE");
  QScopedPointer<ControlNetValidMeasure> validator;
  if (checkMeasureValidity) {
    Pvl deffile(ui.GetFileName("DEFFILE").toStdString());
    validator.reset(new ControlNetValidMeasure(deffile));
  }

  QScopedPointer<SerialNumberList> fromSerials( (ui.WasEntered("FROMLIST") ?
    new SerialNumberList(ui.GetFileName("FROMLIST")) : new SerialNumberList()));

  ControlNet inNet = ControlNet(ui.GetFileName("CNET"));
  inNet.SetUserName(Application::UserName());
  inNet.SetModifiedDate(iTime::CurrentLocalTime()); //This should be done in ControlNet's Write fn

  QString retrievalOpt = ui.GetString("RETRIEVAL");
  PvlKeyword duplicates("DupSerialNumbers");
  if (retrievalOpt == "REFERENCE") {
    FileList list1(ui.GetFileName("FROMLIST"));
    SerialNumberList addSerials(ui.GetFileName("ADDLIST"));

    //Check for duplicate files in the lists by serial number
    for (int i = 0; i < addSerials.size(); i++) {

      // Check for duplicate SNs accross the lists
      if (fromSerials->hasSerialNumber(addSerials.serialNumber(i))) {
        duplicates.addValue(addSerials.fileName(i).toStdString());
      }

      // Check for duplicate SNs within the addlist
      for (int j = i + 1; j < addSerials.size(); j++) {
        if (addSerials.serialNumber(i) == addSerials.serialNumber(j)) {
          QString msg = "Add list files [" + addSerials.fileName(i) + "] and [";
          msg += addSerials.fileName(j) + "] share the same serial number.";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }
    }

    // Get the lat/long coords from the existing reference measure
    setControlPointLatLon(*fromSerials, inNet);
  }
  else {
    for (int cp = 0; cp < inNet.GetNumPoints(); cp++) {
      // Get the surface point from the current control point
      ControlPoint *point = inNet.GetPoint(cp);
      SurfacePoint surfacePoint = point->GetBestSurfacePoint();

      if (!surfacePoint.Valid()) {
        QString msg = "Unable to retreive lat/lon from Control Point [";
        msg += point->GetId() + "]. RETREIVAL=POINT cannot be used unless ";
        msg += "all Control Points have Latitude/Longitude keywords.";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      g_surfacePoints[point->GetId()] = surfacePoint;
    }
  }

  FileName outNetFile(ui.GetFileName("ONET"));

  Progress progress;
  progress.SetText("Adding Images");
  progress.SetMaximumSteps(addList.size());
  progress.CheckStatus();

  STRtree coordTree;
  QList<ControlPoint *> pointList;
  bool usePolygon = ui.GetBoolean("POLYGON");
  if (usePolygon) {
    for (int cp = 0; cp < inNet.GetNumPoints(); cp++) {
      ControlPoint *point = inNet.GetPoint(cp);
      SurfacePoint surfacePoint = g_surfacePoints[point->GetId()];

      Longitude lon = surfacePoint.GetLongitude();
      Latitude lat = surfacePoint.GetLatitude();

      Coordinate *coord = new Coordinate(lon.degrees(), lat.degrees());
      Envelope *envelope = new Envelope(*coord);
      coordTree.insert(envelope, point);
    }
  }
  else {
    for (int cp = 0; cp < inNet.GetNumPoints(); cp++) {
      pointList.append(inNet.GetPoint(cp));
    }
  }

  // Loop through all the images
  for (int img = 0; img < addList.size(); img++) {
    Cube cube;
    cube.open(addList[img].toString());
    Pvl *cubepvl = cube.label();
    QString sn = SerialNumber::Compose(*cubepvl);
    Camera *cam = cube.camera();

    // Loop through all the control points
    QList<ControlPoint *> validPoints = usePolygon ?
        getValidPoints(cube, coordTree) : pointList;

    bool imageAdded = false;
    for (int cp = 0; cp < validPoints.size(); cp++) {
      ControlPoint *point = validPoints[cp];

      // If the point is locked and Apriori source is "AverageOfMeasures"
      // then do not add the measures.
      if (point->IsEditLocked() &&
          point->GetAprioriSurfacePointSource() ==
              ControlPoint::SurfacePointSource::AverageOfMeasures) {
        continue;
      }

      if (point->HasSerialNumber(sn)) continue;

      // Only use the surface point's latitude and longitude, rely on the DEM
      // for computing the radius.  We do this because otherwise we will receive
      // inconsistent results from successive runs of this program if the
      // different DEMs are used, or the point X, Y, Z was generated from the
      // ellipsoid.
      SurfacePoint surfacePoint = g_surfacePoints[point->GetId()];
      if (cam->SetGround(
              surfacePoint.GetLatitude(), surfacePoint.GetLongitude())) {

        // Make sure the samp & line are inside the image
        if (cam->InCube()) {
          ControlMeasure *newCm = new ControlMeasure();
          newCm->SetCoordinate(cam->Sample(), cam->Line(), ControlMeasure::Candidate);
          newCm->SetAprioriSample(cam->Sample());
          newCm->SetAprioriLine(cam->Line());
          newCm->SetCubeSerialNumber(sn);
          newCm->SetDateTime();
          newCm->SetChooserName("Application cnetadd");

          // Check the measure for DEFFILE validity
          if (checkMeasureValidity) {
            if (!validator->ValidEmissionAngle(cam->EmissionAngle())) {
              //TODO: log that it was Emission Angle that failed the check
              newCm->SetIgnored(true);
            }
            else if (!validator->ValidIncidenceAngle(cam->IncidenceAngle())) {
              //TODO: log that it was Incidence Angle that failed the check
              newCm->SetIgnored(true);
            }
            else if (!validator->ValidResolution(cam->resolution())) {
              //TODO: log that it was Resolution that failed the check
              newCm->SetIgnored(true);
            }
            else if (!validator->PixelsFromEdge((int)cam->Sample(), (int)cam->Line(), &cube)) {
              //TODO: log that it was Pixels from Edge that failed the check
              newCm->SetIgnored(true);
            }
            else {
              Portal portal(1, 1, cube.pixelType());
              portal.SetPosition(cam->Sample(), cam->Line(), 1);
              cube.read(portal);
              if (!validator->ValidDnValue(portal[0])) {
                //TODO: log that it was DN that failed the check
                newCm->SetIgnored(true);
              }
            }
          }

          point->Add(newCm); // Point takes ownership

          // Record the modified Point and Measure
          g_modifications[point->GetId()].insert(newCm->GetCubeSerialNumber());
          newCm = NULL; // Do not delete because the point has ownership

          if (retrievalOpt == "POINT" && point->GetNumMeasures() == 1)
            point->SetIgnored(false);

          imageAdded = true;
        }
      }
    }

    cubepvl = NULL;
    cam = NULL;

    if (log) {
      PvlKeyword &logKeyword = (imageAdded) ? added : omitted;
      logKeyword.addValue(addList[img].baseName().toStdString());
    }

    progress.CheckStatus();
  }

  if (log) {
    // Add the list of modified points to the output log file
    QList<QString> modifiedPointsList = g_modifications.keys();
    for (int i = 0; i < modifiedPointsList.size(); i++)
      pointsModified += modifiedPointsList[i].toStdString();

    results.addKeyword(added);
    results.addKeyword(omitted);
    results.addKeyword(pointsModified);
    if (duplicates.size() > 0) {
      results.addKeyword(duplicates);
    }

    results.write(logFile.expanded().toStdString());
  }

  // List the modified points
  if (ui.WasEntered("MODIFIEDPOINTS")) {
    FileName pointList(ui.GetFileName("MODIFIEDPOINTS"));

    // Set up the output file for writing
    std::ofstream out_stream;
    out_stream.open(pointList.expanded().toLatin1().data(), std::ios::out);
    out_stream.seekp(0, std::ios::beg);   //Start writing from beginning of file

    QList<QString> modifiedPointsList = g_modifications.keys();
    for (int i = 0; i < modifiedPointsList.size(); i++)
      out_stream << modifiedPointsList[i].toStdString() << std::endl;

    out_stream.close();
  }

  // Modify the inNet to only have modified points/measures
  if (ui.GetString("EXTRACT") == "MODIFIED") {
    for (int cp = inNet.GetNumPoints() - 1; cp >= 0; cp--) {
      ControlPoint *point = inNet.GetPoint(cp);

      // If the point was not modified, delete
      // Even get rid of edit locked points in this case
      if (!g_modifications.contains(point->GetId())) {
        point->SetEditLock(false);
        inNet.DeletePoint(cp);
      }
      // Else, remove the unwanted measures from the modified point
      else {
        for (int cm = point->GetNumMeasures() - 1; cm >= 0; cm--) {
          ControlMeasure *measure = point->GetMeasure(cm);

          // Even get rid of edit locked measures in this case
          if (point->GetRefMeasure() != measure &&
              !g_modifications[point->GetId()].contains(
                  measure->GetCubeSerialNumber())) {
            measure->SetEditLock(false);
            point->Delete(cm);
          }
        }
      }
    }
  }

  // Generate the TOLIST if wanted
  if (ui.WasEntered("TOLIST")) {
    SerialNumberList toList;

    SerialNumberList addSerials(ui.GetFileName("ADDLIST"));

    const QList<QString> snList = inNet.GetCubeSerials();
    for (int i = 0; i < snList.size(); i++) {
      QString sn = snList[i];

      if (addSerials.hasSerialNumber(sn))
        toList.add(addSerials.fileName(sn));
      else if (fromSerials->hasSerialNumber(sn))
        toList.add(fromSerials->fileName(sn));
    }

    IString name(ui.GetFileName("TOLIST"));
    std::fstream out_stream;
    out_stream.open(name.c_str(), std::ios::out);
    out_stream.seekp(0, std::ios::beg); //Start writing from beginning of file

    for (int f = 0; f < (int) toList.size(); f++)
      out_stream << toList.fileName(f) << std::endl;

    out_stream.close();
  }

  inNet.Write(outNetFile.expanded());

  // delete fromSerials;
}


/**
 * Calculates the lat/lon of the ControlNet.
 *
 * @param incubes The filename of the list of cubes in the ControlNet
 * @param cnet    The filename of the ControlNet
 */
void setControlPointLatLon(SerialNumberList &snl, ControlNet &cnet) {
  CubeManager manager;
  manager.SetNumOpenCubes(50);   //Should keep memory usage to around 1GB

  Progress progress;
  progress.SetText("Calculating Lat/Lon");
  progress.SetMaximumSteps(cnet.GetNumPoints());
  progress.CheckStatus();

  for (int cp = 0; cp < cnet.GetNumPoints(); cp++) {
    ControlPoint *point = cnet.GetPoint(cp);
    ControlMeasure *cm = point->GetRefMeasure();

    Cube *cube = manager.OpenCube(snl.fileName(cm->GetCubeSerialNumber()));
    try {
      cube->camera()->SetImage(cm->GetSample(), cm->GetLine());
      g_surfacePoints[point->GetId()] = cube->camera()->GetSurfacePoint();
    }
    catch (IException &e) {
      QString msg = "Unable to create camera for cube file [";
      msg += snl.fileName(cm->GetCubeSerialNumber()) + "]";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
    cube = NULL; //Do not delete, manager still has ownership

    progress.CheckStatus();
  }

  manager.CleanCubes();
}


QList<ControlPoint *> getValidPoints(Cube &cube, STRtree &coordTree) {
  ImagePolygon poly = cube.readFootprint();

  std::vector<void *> matches;
  MultiPolygon *polys = poly.Polys();
  for (unsigned int i = 0; i < polys->getNumGeometries(); i++) {
    const Geometry *geometry = polys->getGeometryN(i);
    const Envelope *boundingBox = geometry->getEnvelopeInternal();

    coordTree.query(boundingBox, matches);
  }

  QList<ControlPoint *> results;
  for (unsigned int i = 0; i < matches.size(); i++) {
    results.append((ControlPoint *) matches[i]);
  }

  return results;
}
