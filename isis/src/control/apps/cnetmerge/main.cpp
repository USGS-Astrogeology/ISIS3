/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <algorithm>

#include <QMap>

#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "FileList.h"
#include "Progress.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "IException.h"
#include "iTime.h"

using namespace Isis;


ControlNet * mergeNetworks(FileList &filelist, PvlObject &conflictLog,
    QString networkId, QString description);
void mergeNetwork(ControlNet &baseNet, ControlNet &newNet,
    PvlObject &cnetLog, Progress progress);

ControlPoint * mergePoint(
    ControlPoint *basePoint, ControlPoint *newPoint,
    PvlObject &cnetLog);
void replacePoint(
    ControlPoint *basePoint, ControlPoint *newPoint,
    PvlObject &pointLog);

void removeMissing(ControlPoint *basePoint, ControlPoint *newPoint,
    PvlGroup &measureLog);
void mergeMeasures(ControlPoint *basePoint, ControlPoint *newPoint,
    PvlGroup &measureLog);

void mergeMeasure(
    ControlPoint *basePoint, ControlPoint *newPoint,
    ControlMeasure *baseMeasure, ControlMeasure *newMeasure,
    PvlGroup &measureLog);
void replaceMeasure(ControlPoint *basePoint, ControlPoint *newPoint,
    ControlMeasure *baseMeasure, ControlMeasure *newMeasure);
void addMeasure(ControlPoint *basePoint, ControlPoint *newPoint,
    ControlMeasure *newMeasure);

PvlObject createNetworkLog(ControlNet &cnet);
PvlObject createPointLog(ControlPoint *point);
PvlGroup createMeasureLog();

void reportConflict(PvlObject &pointLog, QString conflict);
void reportConflict(PvlGroup &measureLog, QString cn, QString conflict);

void addLog(PvlObject &conflictLog, PvlObject &cnetLog);
void addLog(PvlObject &cnetLog, PvlObject &pointLog, PvlGroup &measureLog);


bool overwritePoints;
bool overwriteMeasures;
bool overwriteReference;
bool overwriteMissing;
bool report;
bool mergePoints;

QString logName;


void IsisMain() {
  // Get user parameters
  UserInterface &ui = Application::GetUserInterface();
  FileList filelist;
  if (ui.GetString("INPUTTYPE") == "LIST") {
    filelist.read(ui.GetFileName("CLIST"));

    if (ui.WasEntered("BASE")) {
      // User has chosen an explicit base network
      QString baseName = ui.GetFileName("BASE");
      FileName baseFileName(baseName);

      // Remove the base network from the list if it is present
      for (int i = 0; i < filelist.size(); i++) {
        if (filelist[i].expanded() == baseFileName.expanded()) {
          // FileNames match, so erase it and move on.  We assume it only
          // appears once.  There are currently no checks for duplicate
          // networks.
          filelist.erase(filelist.begin() + i);
          break;
        }
      }

      // Add the explicit base network to the front so it gets added to the
      // output first
      filelist.insert(filelist.begin(), baseName);
    }
    else {
      // So there is a record of which file was used as the BASE in the print
      // file
      ui.PutFileName("BASE", filelist[0].toString());
    }

    // Check after taking into account an explicit base network if we have at
    // least two networks to merge
    if (filelist.size() < 2) {
      QString msg = "CLIST [" + ui.GetFileName("CLIST") + "] and BASE [" +
        (ui.WasEntered("BASE") ? ui.GetFileName("BASE") : "Automatic") +
        "] must total to at least two distinct filenames: "
        "a base network and a new network";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }
  else if (ui.GetString("INPUTTYPE") == "CNETS") {
    // Treat simple case in general way, as a two-cnet list
    filelist.push_back(ui.GetFileName("BASE"));
    filelist.push_back(ui.GetFileName("CNET2"));
  }

  // Get overwrite options, all false by default, and only useable when in
  // MERGE mode
  overwritePoints = false;
  overwriteMeasures = false;
  overwriteReference = false;
  overwriteMissing = false;

  if (ui.GetString("DUPLICATEPOINTS") == "MERGE") {
    overwritePoints = ui.GetBoolean("OVERWRITEPOINTS");
    overwriteMeasures = ui.GetBoolean("OVERWRITEMEASURES");
    overwriteReference = ui.GetBoolean("OVERWRITEREFERENCE");
    overwriteMissing = ui.GetBoolean("OVERWRITEMISSING");
  }

  // Setup a conflict log, which will only be added to and reported if the user
  // specified a log file, but for the sake of simplicity, the objects will be
  // kept around anyway.
  // TODO there is likely a more elegant solution than to keep around unneeded
  // objects, but it's already memory and computationally cheap as is
  PvlObject conflictLog("Conflicts");
  report = ui.WasEntered("LOG");
  logName = report ? ui.GetFileName("LOG") : "";

  // Determine whether the user wants to allow merging duplicate points or
  // throw an error
  mergePoints = ui.GetString("DUPLICATEPOINTS") == "MERGE";

  ControlNet *outNet = mergeNetworks(filelist, conflictLog,
      ui.GetString("NETWORKID"), ui.GetString("DESCRIPTION"));

  // If the user wishes to report on conflicts, write them out to a file
  if (report) {
    Pvl outPvl;
    outPvl.addObject(conflictLog);
    outPvl.write(logName.toStdString());
  }

  // Writes out the final Control Net
  FileName outfile(ui.GetFileName("ONET"));
  outNet->Write(outfile.expanded());
  delete outNet;
}


ControlNet * mergeNetworks(FileList &filelist, PvlObject &conflictLog,
    QString networkId, QString description) {

  if (!mergePoints) {
    bool hasDuplicates = false;
    PvlObject errors("Errors");

    QMap<QString, QString> pointSources;
    for (int n = 0; n < filelist.size(); n++) {
      FileName cnetName(filelist[n]);
      ControlNet network(cnetName.expanded());

      for (int p = 0; p < network.GetNumPoints(); p++) {
        ControlPoint *point = network.GetPoint(p);
        if (pointSources.contains(point->GetId())) {
          hasDuplicates = true;

          if (report) {
            PvlObject duplicate("Duplicate");
            duplicate.addKeyword(PvlKeyword("PointId", point->GetId().toStdString()));
            duplicate.addKeyword(PvlKeyword(
                  "SourceNetwork", pointSources[point->GetId()].toStdString()));
            duplicate.addKeyword(PvlKeyword("AddNetwork", cnetName.name().toStdString()));
            errors.addObject(duplicate);
          }
          else {
            // User has disallowed merging points, so throw an error
            QString msg = "Add network [" + cnetName.name() + "] contains "
              "Control Point with ID [" + point->GetId() + "] already "
              "contained within source network [" +
              pointSources[point->GetId()] + "].  "
              "Set DUPLICATEPOINTS=MERGE to merge conflicting Control Points";
            throw IException(IException::User, msg, _FILEINFO_);
          }
        }
        else {
          pointSources.insert(point->GetId(), cnetName.name());
        }
      }
    }

    if (hasDuplicates && report) {
      Pvl outPvl;
      outPvl.addObject(errors);
      outPvl.write(logName.toStdString());

      QString msg = "Networks contained duplicate points.  See log file [" +
        FileName(logName).name() + "] for details.  "
        "Set DUPLICATEPOINTS=MERGE to merge conflicting Control Points";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  // Show progress using "File X of N" and progress bar for each file.
  Progress progress;
  progress.SetText("Loading base network");
  progress.CheckStatus();

  // The original base network is the first in the list, all successive
  // networks will be added to the base in descending order
  ControlNet *baseNet = new ControlNet(filelist[0].toString(), &progress);
  baseNet->SetNetworkId(networkId);
  baseNet->SetUserName(Isis::Application::UserName());
  baseNet->SetCreatedDate(Isis::Application::DateTime());
  baseNet->SetModifiedDate(Isis::iTime::CurrentLocalTime());
  baseNet->SetDescription(description);


  // Loop through each network in the list and attempt to merge it into the
  // base
  for (int cnetIndex = 1; cnetIndex < (int) filelist.size(); cnetIndex++) {
    FileName currentCnetFileName(filelist[cnetIndex]);
    ControlNet newNet(currentCnetFileName.expanded(), &progress);

    // Networks can only be merged if the targets are the same
    if (baseNet->GetTarget().toLower() != newNet.GetTarget().toLower()) {
      QString msg = "Input [" + newNet.GetNetworkId() + "] does not target the "
          "same target as other Control Network(s)";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Setup a conflict log object for this new network
    PvlObject cnetLog = createNetworkLog(newNet);

    // Merge the network, add the resulting conflicts to the log
    progress.SetText("Merging file " + QString::number(cnetIndex+1)
                                     + " of " + QString::number(filelist.size()));
    mergeNetwork(*baseNet, newNet, cnetLog, progress);
    addLog(conflictLog, cnetLog);

  }

  return baseNet;
}


void mergeNetwork(ControlNet &baseNet, ControlNet &newNet, PvlObject &cnetLog, Progress progress) {
  // Loop through all points in the new network, looking for new points to add
  // and conflicting points to resolve
  progress.SetMaximumSteps(newNet.GetNumPoints());
  for (int newIndex = 0; newIndex < newNet.GetNumPoints(); newIndex++) {
    ControlPoint *newPoint = newNet.GetPoint(newIndex);
    if (baseNet.ContainsPoint(newPoint->GetId())) {
      // The base already has a point with the current ID.  Merge the points
      // together, the result will be a wholly new point that needs to be added
      // to the network, and the old point in base removed.
      ControlPoint *basePoint = baseNet.GetPoint(QString(newPoint->GetId()));
      ControlPoint *outPoint = mergePoint(basePoint, newPoint, cnetLog);
      baseNet.DeletePoint(basePoint);
      baseNet.AddPoint(outPoint);
    }
    else {
      // This point does not exist in the base network yet, so go ahead and add
      // it without concern.  Make sure we copy it, however, to avoid losing the
      // point when the owning network is deleted.
      ControlPoint *outPoint = new ControlPoint(*newPoint);
      baseNet.AddPoint(outPoint);
    }
    progress.CheckStatus();
  }
}


ControlPoint * mergePoint(
    ControlPoint *basePoint, ControlPoint *newPoint, PvlObject &cnetLog) {

  // Start with a copy of the base point to alter until we have a potentially
  // wholly new point
  ControlPoint *outPoint = new ControlPoint(*basePoint);

  // Setup the conflict log for this point
  PvlObject pointLog = createPointLog(newPoint);

  // Overwrite the point info if the user wishes to do so, otherwise report that
  // the info was retained
  if (overwritePoints)
    replacePoint(outPoint, newPoint, pointLog);
  else
    reportConflict(pointLog, "Retained: OVERWRITEPOINTS=false");

  // Setup the log reporting all measure conflicts in the point
  PvlGroup measureLog = createMeasureLog();
  removeMissing(outPoint, newPoint, measureLog);
  mergeMeasures(outPoint, newPoint, measureLog);

  // Report on all conflicts
  addLog(cnetLog, pointLog, measureLog);

  return outPoint;
}


void replacePoint(ControlPoint *basePoint, ControlPoint *newPoint,
    PvlObject &pointLog) {

  // Only transfer over new point info if the point is not edit locked
  if (!basePoint->IsEditLocked()) {
    basePoint->SetId(newPoint->GetId());
    basePoint->SetType(newPoint->GetType());
    basePoint->SetChooserName(newPoint->GetChooserName());
    basePoint->SetEditLock(newPoint->IsEditLocked());
    basePoint->SetIgnored(newPoint->IsIgnored());
    basePoint->SetAprioriSurfacePointSource(
        newPoint->GetAprioriSurfacePointSource());
    basePoint->SetAprioriSurfacePointSourceFile(
        newPoint->GetAprioriSurfacePointSourceFile());
    basePoint->SetAprioriSurfacePointSourceFile(
        newPoint->GetAprioriSurfacePointSourceFile());
    basePoint->SetAprioriRadiusSource(newPoint->GetAprioriRadiusSource());
    basePoint->SetAprioriRadiusSourceFile(
        newPoint->GetAprioriRadiusSourceFile());
    basePoint->SetAprioriSurfacePoint(newPoint->GetAprioriSurfacePoint());
    basePoint->SetAdjustedSurfacePoint(newPoint->GetAdjustedSurfacePoint());
    // TODO basePoint->SetInvalid(newPoint->GetInvalid());

    reportConflict(pointLog, "Replaced: OVERWRITEPOINTS=true");
  }
  else {
    reportConflict(pointLog, "Retained: Edit Lock");
  }
}


void removeMissing(ControlPoint *basePoint, ControlPoint *newPoint,
    PvlGroup &measureLog) {

  // If the user wishes to remove measures not existing in the new point from
  // the base, then go ahead and do that cleanup upfront so we have a smaller
  // point to contend with later
  if (overwriteMissing) {
    for (int baseIndex = 0;
         baseIndex < basePoint->GetNumMeasures();
         baseIndex++) {

      ControlMeasure *baseMeasure = basePoint->GetMeasure(baseIndex);
      if (!newPoint->HasSerialNumber(baseMeasure->GetCubeSerialNumber())) {

        if (baseMeasure != basePoint->GetRefMeasure() || overwriteReference) {
          // New point does not have the current measure, so delete it from the
          // output point if it's not the reference, or if we're overwriting the
          // reference
          reportConflict(measureLog, baseMeasure->GetCubeSerialNumber(),
              "Removed: OVERWRITEMISSING=true");
          basePoint->Delete(baseMeasure);
        }
        else {
          // Missing measure in the new point is the reference in the base
          // point, and we're not overwriting reference, so don't remove it
          reportConflict(measureLog, baseMeasure->GetCubeSerialNumber(),
              "Retained: OVERWRITEREFERENCE=false");
        }
      }
    }
  }
}


void mergeMeasures(ControlPoint *basePoint, ControlPoint *newPoint,
    PvlGroup &measureLog) {

  // Loop through the new point looking for new measures to add and conflicts to
  // resolve
  for (int newIndex = 0; newIndex < newPoint->GetNumMeasures(); newIndex++) {
    ControlMeasure *newMeasure = newPoint->GetMeasure(newIndex);
    if (basePoint->HasSerialNumber(newMeasure->GetCubeSerialNumber())) {
      // Both points have a measure with the same serial number.  This implies a
      // conflict, so go ahead and try to resolve it.
      ControlMeasure *baseMeasure =
          basePoint->GetMeasure(newMeasure->GetCubeSerialNumber());
      mergeMeasure(basePoint, newPoint, baseMeasure, newMeasure, measureLog);
    }
    else {
      // New measure not currently in the base point, so add it to the output
      addMeasure(basePoint, newPoint, newMeasure);
    }
  }
}


void mergeMeasure(
    ControlPoint *basePoint, ControlPoint *newPoint,
    ControlMeasure *baseMeasure, ControlMeasure *newMeasure,
    PvlGroup &measureLog) {

  // Cannot replace an edit locked measure, so don't bother doing any further
  // checks if that's what we have
  if (!baseMeasure->IsEditLocked()) {
    if (baseMeasure == basePoint->GetRefMeasure()) {
      // The base measure is the reference, so only replace it if
      // OVERWRITEREFERENCE=true.  Setting the new reference will be handled
      // automatically by addMeasure().
      if (overwriteReference) {
        replaceMeasure(basePoint, newPoint, baseMeasure, newMeasure);
        reportConflict(measureLog, newMeasure->GetCubeSerialNumber(),
            "Replaced: OVERWRITEREFERENCE=true");
      }
      else {
        reportConflict(measureLog, newMeasure->GetCubeSerialNumber(),
            "Retained: OVERWRITEREFERENCE=false");
      }
    }
    else if (overwriteMeasures) {
      // The base measure is not the reference, OVERWRITEMEASURES=TRUE, so
      // replace the base measure with the new measure
      replaceMeasure(basePoint, newPoint, baseMeasure, newMeasure);
      reportConflict(measureLog, newMeasure->GetCubeSerialNumber(),
          "Replaced: OVERWRITEMEASURES=true");
    }
    else {
      // The base measure is not the reference, and OVERWRITEMEASURES=false, so
      // retain the base measure
      reportConflict(measureLog, newMeasure->GetCubeSerialNumber(),
          "Retained: OVERWRITEMEASURES=false");
    }
  }
  else {
    reportConflict(measureLog, newMeasure->GetCubeSerialNumber(),
        "Retained: Edit Lock");
  }
}


void replaceMeasure(ControlPoint *basePoint, ControlPoint *newPoint,
    ControlMeasure *baseMeasure, ControlMeasure *newMeasure) {

  // First delete the base measure from the point, then add the new measure,
  // setting it to reference if it was the reference in the new point, and
  // OVERWRITEREFERENCE=true
  basePoint->Delete(baseMeasure);
  addMeasure(basePoint, newPoint, newMeasure);
}


void addMeasure(ControlPoint *basePoint, ControlPoint *newPoint,
    ControlMeasure *newMeasure) {

  // Copy the new measure to avoid losing it when the new point is deleted
  ControlMeasure *outMeasure = new ControlMeasure(*newMeasure);

  // Add the copied measure to the base point
  basePoint->Add(outMeasure);

  // Finally, set the output measure to be the reference of the base point if
  // the new measure is the reference in the new point and
  // OVERWRITEREFERENCE=true
  if (overwriteReference && newMeasure == newPoint->GetRefMeasure())
    basePoint->SetRefMeasure(outMeasure);
}


PvlObject createNetworkLog(ControlNet &cnet) {
  PvlObject cnetLog("Network");
  PvlKeyword networkId("NetworkId", cnet.GetNetworkId().toStdString());
  cnetLog.addKeyword(networkId);
  return cnetLog;
}


PvlObject createPointLog(ControlPoint *point) {
  PvlObject pointLog("Point");
  PvlKeyword pointId("PointId", point->GetId().toStdString());
  pointLog.addKeyword(pointId);
  return pointLog;
}


PvlGroup createMeasureLog() {
  PvlGroup measureLog("Measures");
  return measureLog;
}


void reportConflict(PvlObject &pointLog, QString conflict) {
  // Add a point conflict message to the point log if we're reporting these
  // conflicts to a log file
  if (report) {
    PvlKeyword resolution("Resolution", conflict.toStdString());
    pointLog.addKeyword(resolution);
  }
}


void reportConflict(PvlGroup &measureLog, QString sn, QString conflict) {
  // Add a measure conflict message to the measure log if we're reporting these
  // conflicts to a log file
  if (report) {
    PvlKeyword resolution(sn.toStdString(), conflict.toStdString());
    measureLog.addKeyword(resolution);
  }
}


void addLog(PvlObject &conflictLog, PvlObject &cnetLog) {
  // If the network log has at least one point object, then there was at least
  // one conflict, so add it to the log for all conflicts
  if (cnetLog.objects() > 0)
    conflictLog.addObject(cnetLog);
}


void addLog(PvlObject &cnetLog, PvlObject &pointLog, PvlGroup &measureLog) {
  // If the measure log has at least one keyword, then it has at least one
  // conflict, so add it to the point log
  if (measureLog.keywords() > 0)
    pointLog.addGroup(measureLog);

  // If the point log has more keywords than the PointId keyword, then it has a
  // conflict and should be added to the network log.  Likewise, if it has a
  // group then its measures had conflicts and thus the point should be added to
  // the output log.
  if (pointLog.keywords() > 1 || pointLog.groups() > 0)
    cnetLog.addObject(pointLog);
}
