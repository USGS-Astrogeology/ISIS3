/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "cnetedit.h"

#include <QFile>
#include <QList>
#include <QMap>
#include <QSet>
#include <QString>
#include <QTextStream>

#include "Application.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlNetValidMeasure.h"
#include "ControlPoint.h"
#include "ControlPointList.h"
#include "Cube.h"
#include "FileName.h"
#include "MeasureValidationResults.h"
#include "Progress.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "SerialNumber.h"
#include "SerialNumberList.h"
#include "IException.h"

using namespace std;

namespace Isis {
  
  // Deletion test
  bool shouldDelete(ControlPoint *point);

  // Mutator methods
  void ignorePoint(ControlNet &cnet, ControlPoint *point, QString cause);
  void ignoreMeasure(ControlNet &cnet, ControlPoint *point,
                     ControlMeasure *measure, QString cause);
  void deletePoint(ControlNet &cnet, int cp);
  void deleteMeasure(ControlPoint *point, int cm);

  // Edit passes
  void populateLog(ControlNet &cnet, bool ignore);

  void unlockPoints(ControlNet &cnet, ControlPointList &cpList);
  void ignorePoints(ControlNet &cnet, ControlPointList &cpList);
  void lockPoints(ControlNet &cnet, ControlPointList &cpList);

  void unlockCubes(ControlNet &cnet, SerialNumberList &snl);
  void ignoreCubes(ControlNet &cnet, SerialNumberList &snl);
  void lockCubes(ControlNet &cnet, SerialNumberList &snl);

  void unlockMeasures(ControlNet &cnet,
      QMap< QString, QSet<QString> * > &editMeasures);
  void ignoreMeasures(ControlNet &cnet,
      QMap< QString, QSet<QString> * > &editMeasures);
  void lockMeasures(ControlNet &cnet,
      QMap< QString, QSet<QString> * > &editMeasures);

  void checkAllMeasureValidity(ControlNet &cnet, SerialNumberList *serialNumbers);

  // Validity test
  MeasureValidationResults validateMeasure(const ControlMeasure *measure,
      Cube *cube, Camera *camera);

  // Logging helpers
  void logResult(QMap<QString, QString> *pointsLog, QString pointId, QString cause);
  void logResult(QMap<QString, PvlGroup> *measuresLog,
      QString pointId, QString serial, QString cause);
  PvlObject createLog(QString label, QMap<QString, QString> *pointsMap);
  PvlObject createLog(QString label,
      QMap<QString, QString> *pointsMap, QMap<QString, PvlGroup> *measuresMap);

  // Global variables
  static int numPointsDeleted;
  static int numMeasuresDeleted;

  static bool deleteIgnored;
  static bool preservePoints;
  static bool retainRef;
  static bool keepLog;
  static bool ignoreAll;

  static QMap<QString, QString> *ignoredPoints;
  static QMap<QString, PvlGroup> *ignoredMeasures;
  static QMap<QString, QString> *retainedReferences;
  static QMap<QString, QString> *editLockedPoints;
  static QMap<QString, PvlGroup> *editLockedMeasures;

  static ControlNetValidMeasure *cneteditValidator;

  /**
   * Edit a control network
   *
   * @param ui UserInterface object containing parameters
   * @return Pvl results log file
   *
   * @throws IException::User "Unable to open MEASURELIST [FILENAME]"
   * @throws IException::User "Line ___ in the MEASURELIST does not contain a Point ID and a cube filename separated by a comma"
   */
  Pvl cnetedit(UserInterface &ui) {

    // Construct control net
    ControlNet cnet(ui.GetFileName("CNET"));

    // List of IDs mapping to points to be edited
    ControlPointList *cpList = nullptr;
    if (ui.WasEntered("POINTLIST") && cnet.GetNumPoints() > 0) {
        cpList = new ControlPointList(ui.GetFileName("POINTLIST"));
    }

    // Serial number list of cubes to be edited
    SerialNumberList *cubeSnList = nullptr;
    if (ui.WasEntered("CUBELIST") && cnet.GetNumPoints() > 0) {
        cubeSnList = new SerialNumberList(ui.GetFileName("CUBELIST"));
    }

    // List of paired Point IDs and cube filenames
    QMap< QString, QSet<QString> * > *editMeasuresList = nullptr;
    if (ui.WasEntered("MEASURELIST") && cnet.GetNumPoints() > 0) {

        QFile file(ui.GetFileName("MEASURELIST"));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString msg = "Unable to open MEASURELIST [" +
                          file.fileName() + "]";
            throw IException(IException::User, msg, _FILEINFO_);
        }

        editMeasuresList = new QMap< QString, QSet<QString> * >;

        QTextStream in(&file);
        int lineNumber = 1;
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList results = line.split(",");
            if (results.size() < 2) {
                QString msg = "Line " + QString::number(lineNumber) + " in the MEASURELIST does "
                                                                      "not contain a Point ID and a cube filename separated by a comma";
                throw IException(IException::User, msg, _FILEINFO_);
            }

            if (!editMeasuresList->contains(results[0]))
                editMeasuresList->insert(results[0], new QSet<QString>);

            FileName cubeName(results[1]);
            QString sn = SerialNumber::Compose(cubeName.expanded());
            (*editMeasuresList)[results[0]]->insert(sn);

            lineNumber++;
        }
    }

    // DefFile and cube serial number list for Validity check
    Pvl *defFile = nullptr;
    SerialNumberList *snValidationList = nullptr;
    if (ui.GetBoolean("CHECKVALID") && cnet.GetNumPoints() > 0) {
        cneteditValidator = nullptr;

        // Open DefFile and validate its' keywords and value type
        defFile = new Pvl(ui.GetFileName("DEFFILE"));

        // create cube serial number validation list
        snValidationList = new SerialNumberList(ui.GetFileName("FROMLIST"));
    }


    // call cnetedit with ControlNet plus list files and/or definition files (if any)
    Pvl results = cnetedit(cnet, ui, cpList, cubeSnList, editMeasuresList, defFile, snValidationList);

    // clean up
    delete cpList;
    cpList = nullptr;

    delete cubeSnList;
    cubeSnList = nullptr;

    if (editMeasuresList != nullptr) {
        QList<QString> points = editMeasuresList->keys();
        for (int i = 0; i < points.size(); i++) delete (*editMeasuresList)[points[i]];
        delete editMeasuresList;
        editMeasuresList = nullptr;
    }

    delete defFile;
    defFile = nullptr;

    delete snValidationList;
    snValidationList = nullptr;

    return results;
  }


  /**
   * Edit a control network
   *
   * @param ControlNet cnet
   * @param UserInterface ui
   * @param ControlPointList cpList
   * @param SerialNumberList cubeSnl
   * @param QMap< QString, QSet<QString> * > editMeasuresList
   * @param Pvl defFile
   * @param SerialNumberList snValidationList
   *
   * @return Pvl results log file
   *
   * @throws IException::User "Invalid Deffile"
   */
  Pvl cnetedit(ControlNet &cnet, UserInterface &ui,
                ControlPointList *cpList,
                SerialNumberList *cubeSnl,
                QMap< QString, QSet<QString> * > *editMeasuresList,
                Pvl *defFile,
                SerialNumberList *snValidationList) {

    Pvl results;

    // 2016-12-08 Ian Humphrey - Set the QHash seed, otherwise output is ALWAYS slightly
    //                different. Note that in Qt4, the seed was constant, but in Qt5, the seed is
    //                created differently for each process. Fixes #4206.
    qSetGlobalQHashSeed(0);

    // Reset the counts of points and measures deleted
    numPointsDeleted = 0;
    numMeasuresDeleted = 0;

    // Get global user parameters
    bool ignore = ui.GetBoolean("IGNORE");
    deleteIgnored = ui.GetBoolean("DELETE");
    preservePoints = ui.GetBoolean("PRESERVE");
    retainRef = ui.GetBoolean("RETAIN_REFERENCE");
    ignoreAll = ui.GetBoolean("IGNOREALL");

    // Data needed to keep track of ignored/deleted points and measures
    keepLog = ui.WasEntered("LOG");
    ignoredPoints = NULL;
    ignoredMeasures = NULL;
    retainedReferences = NULL;
    editLockedPoints = NULL;
    editLockedMeasures = NULL;

    // If the user wants to keep a log, go ahead and populate it with all the
    // existing ignored points and measures
    if (keepLog && cnet.GetNumPoints() > 0)
        populateLog(cnet, ignore);

    // List has Points Ids
    bool processPoints = false;
    if (cpList != nullptr) {
        processPoints = true;
    }

    // List has Cube file names
    bool processCubes = false;
    if (cubeSnl != nullptr) {
        processCubes = true;
    }

    // List has measurelist file names
    bool processMeasures = false;
    if (editMeasuresList != nullptr) {
        processMeasures = true;
    }

    if (ui.GetBoolean("UNLOCK")) {
        if (processPoints) unlockPoints(cnet, *cpList);
        if (processCubes) unlockCubes(cnet, *cubeSnl);
        if (processMeasures) unlockMeasures(cnet, *editMeasuresList);
    }

    /*
    * As a first pass, just try and delete anything that's already ignored
    * in the Control Network, if the user wants to delete ignored points and
    * measures. Originally, this check was performed last, only if the user
    * didn't specify any other deletion methods. However, performing this
    * check first will actually improve the running time in cases where there
    * are already ignored points and measures in the input network. The added
    * cost of doing this check here actually doesn't add to the running time at
    * all, because these same checks would need to have been done later
    * regardless.
    */
    if (deleteIgnored && cnet.GetNumPoints() > 0) {
        Progress progress;
        progress.SetText("Deleting Ignored in Input");
        progress.SetMaximumSteps(cnet.GetNumPoints());
        progress.CheckStatus();

        for (int cp = cnet.GetNumPoints() - 1; cp >= 0; cp--) {
          ControlPoint *point = cnet.GetPoint(cp);
          if (point->IsIgnored()) {
              deletePoint(cnet, cp);
          }
          else {
              for (int cm = point->GetNumMeasures() - 1; cm >= 0; cm--) {
                if (point->GetMeasure(cm)->IsIgnored()) {
                    if (cm == point->IndexOfRefMeasure() && ignoreAll) {
                        // If the reference is ignored and IgnoreAll is set, the point must ignored too
                        ignorePoint(cnet, point, "Reference measure ignored");
                    }
                    else {
                        deleteMeasure(point, cm);
                    }
                }
              }

              // Check if the number of measures in the point is zero or there are too
              // few measures in the point and we don't want to preserve them.
              if (shouldDelete(point)) {
                  deletePoint(cnet, cp);
              }
          }

          progress.CheckStatus();
        }
    }

    if (ignore) {
        if (processPoints) ignorePoints(cnet, *cpList);
        if (processCubes) ignoreCubes(cnet, *cubeSnl);
        if (processMeasures) ignoreMeasures(cnet, *editMeasuresList);

        // Perform validity check
        if (ui.GetBoolean("CHECKVALID") && cnet.GetNumPoints() > 0) {
            cneteditValidator = NULL;

            // First validate DefFile's keywords and value type
            Pvl defFile(ui.GetFileName("DEFFILE"));
            Pvl pvlTemplate("$ISISROOT/appdata/templates/cnet_validmeasure/validmeasure.def");
            Pvl pvlResults;
            pvlTemplate.validatePvl(defFile, pvlResults);
            if (pvlResults.groups() > 0 || pvlResults.keywords() > 0) {
                Application::AppendAndLog(pvlResults.group(0), &results);

                QString sErrMsg = "Invalid Deffile\n";
                throw IException(IException::User, sErrMsg, _FILEINFO_);
            }

            // Construct the validator from the user-specified definition file
            cneteditValidator = new ControlNetValidMeasure(defFile);

            // User also provided a list of all serial numbers corresponding to every
            // cube in the control network
            checkAllMeasureValidity(cnet, snValidationList);

            // Delete the validator
            if (cneteditValidator != NULL) {
                delete cneteditValidator;
                cneteditValidator = NULL;
            }

            // Log the DEFFILE to the print file
            Application::AppendAndLog(defFile.findGroup("ValidMeasure", Pvl::Traverse), &results);
        }
    }

    if (ui.GetBoolean("LOCK")) {
        if (processPoints) lockPoints(cnet, *cpList);
        if (processCubes) lockCubes(cnet, *cubeSnl);
        if (processMeasures) lockMeasures(cnet, *editMeasuresList);
    }

    // Log statistics
    if (keepLog) {
        Pvl outputLog;

        outputLog.addKeyword(PvlKeyword("PointsDeleted", toString(numPointsDeleted)));
        outputLog.addKeyword(PvlKeyword("MeasuresDeleted", toString(numMeasuresDeleted)));

        PvlObject lockedLog = createLog(
            "EditLocked", editLockedPoints, editLockedMeasures);
        outputLog.addObject(lockedLog);

        outputLog.addObject(createLog("RetainedReferences", retainedReferences));

        // Depending on whether the user chose to delete ignored points and
        // measures, the log will either contain reasons for being ignored, or
        // reasons for being deleted
        PvlObject ignoredLog = createLog(
            deleteIgnored ? "Deleted" : "Ignored", ignoredPoints, ignoredMeasures);
        outputLog.addObject(ignoredLog);

        // Write the log
        QString logFileName = ui.GetFileName("LOG");
        outputLog.write(logFileName);

        // Delete the structures keeping track of the ignored points and measures
        delete ignoredPoints;
        ignoredPoints = NULL;

        delete ignoredMeasures;
        ignoredMeasures = NULL;

        delete retainedReferences;
        retainedReferences = NULL;

        delete editLockedPoints;
        editLockedPoints = NULL;

        delete editLockedMeasures;
        editLockedMeasures = NULL;
    }

    // Write the network
    cnet.Write(ui.GetFileName("ONET"));

    return results;
  }


  /**
   * After any modification to a point's measures or ignored status, this check
   * should be performed to determine if the changes should result in the point's
   * deletion.
   *
   * @param point The Control Point recently modified
   *
   * @return Whether or not the point should be deleted
   */
  bool shouldDelete(ControlPoint *point){

      if(!deleteIgnored)
          return false;

      else{

          if ( point->GetNumMeasures() == 0 && !preservePoints)
              return true;


          if (point->GetType() != ControlPoint::Fixed && point->GetNumMeasures()< 2 ){

              if(preservePoints  && point->GetNumMeasures() == 1)
                  return false;

              return true;  //deleteIgnore = TRUE or else we would not be here
          }


          if( point->IsIgnored() )
              return true;

          return false;
      }
  }


  /**
   * Set the point at the given index in the control network to ignored, and add
   * a new keyword to the list of ignored points with a cause for the ignoring,
   * if the user wished to keep a log.
   *
   * @param cnet  The Control Network being modified
   * @param point The Control Point we wish to ignore
   * @param cause A prose description of why the point was ignored (for logging)
   */
  void ignorePoint(ControlNet &cnet, ControlPoint *point, QString cause) {
                   ControlPoint::Status result = point->SetIgnored(true);

    logResult(result == ControlPoint::Success ? ignoredPoints : editLockedPoints,
        point->GetId(), cause);
  }


  /**
   * Set the measure to be ignored, and add a new keyword to the list of ignored
   * measures if the user wished to keep a log.
   *
   * @param cnet    The Control Network being modified
   * @param point   The Control Point of the Control Measure we wish to ignore
   * @param measure The Control Measure we wish to ignore
   * @param cause   A prose description of why the measure was ignored (for
   *                logging)
   */
  void ignoreMeasure(ControlNet &cnet, ControlPoint *point,
                     ControlMeasure *measure, QString cause) {
    ControlMeasure::Status result = measure->SetIgnored(true);

    logResult(
        result == ControlMeasure::Success ? ignoredMeasures : editLockedMeasures,
        point->GetId(), measure->GetCubeSerialNumber(), cause);

    if (ignoreAll && measure->Parent()->GetRefMeasure() == measure) {
      foreach (ControlMeasure *cm, measure->Parent()->getMeasures()) {
        if (!cm->IsIgnored())
          ignoreMeasure(cnet, measure->Parent(), cm, "Reference ignored");
      }
    }
  }


  /**
   * Delete the point, record how many points and measures have been deleted.
   *
   * @param cnet The Control Network being modified
   * @param cp   Index into the Control Network for the point we wish to delete
   */
  void deletePoint(ControlNet &cnet, int cp) {
    ControlPoint *point = cnet.GetPoint(cp);

    // Do the edit lock check up front so we don't accidentally log that a point
    // was deleted when in fact it was not
    if (!point->IsEditLocked()) {
      numMeasuresDeleted += point->GetNumMeasures();
      numPointsDeleted++;

      if (keepLog) {
        // If the point's being deleted but it wasn't set to ignore, it can only
        // be because the point has two few measures remaining
        if (!point->IsIgnored())
          ignorePoint(cnet, point, "Too few measures");

        // For any measures not ignored, mark their cause for deletion as being
        // caused by the point's deletion
        for (int cm = 0; cm < point->GetNumMeasures(); cm++) {
          ControlMeasure *measure = point->GetMeasure(cm);
          if (!measure->IsIgnored())
            ignoreMeasure(cnet, point, measure, "Point deleted");
        }
      }

      cnet.DeletePoint(cp);
    }
    else {
      for (int cm = 0; cm < point->GetNumMeasures(); cm++) {
        if (point->GetMeasure(cm)->IsEditLocked()) {
          ignorePoint(cnet, point, "EditLocked point skipped");
        }
      }
    }
  }


  /**
   * Delete the measure, increment the count of measures deleted.
   *
   * @param point The Control Point of the Control Measure we wish to delete
   * @param cm    Index into the Control Network for the measure we wish to delete
   */
  void deleteMeasure(ControlPoint *point, int cm) {
    if (point->Delete(cm) == ControlMeasure::Success) numMeasuresDeleted++;
  }


  /**
   * Seed the log with points and measures already ignored.
   *
   * @param cnet The Control Network being modified
   */
  void populateLog(ControlNet &cnet, bool ignore) {
    ignoredPoints = new QMap<QString, QString>;
    ignoredMeasures = new QMap<QString, PvlGroup>;

    retainedReferences = new QMap<QString, QString>;

    editLockedPoints = new QMap<QString, QString>;
    editLockedMeasures = new QMap<QString, PvlGroup>;

    Progress progress;
    progress.SetText("Initializing Log File");
    progress.SetMaximumSteps(cnet.GetNumPoints());
    progress.CheckStatus();

    for (int cp = 0; cp < cnet.GetNumPoints(); cp++) {
      ControlPoint *point = cnet.GetPoint(cp);

      if (point->IsIgnored()) {
        ignorePoint(cnet, point, "Ignored from input");
      }

      for (int cm = 0; cm < point->GetNumMeasures(); cm++) {
        ControlMeasure *measure = point->GetMeasure(cm);

        if (measure->IsIgnored()) {
          if (cm == point->IndexOfRefMeasure() && ignoreAll) {
            if (ignore && !point->IsIgnored()) {
              ignorePoint(cnet, point, "Reference measure ignored");
            }
          }

          ignoreMeasure(cnet, point, measure, "Ignored from input");
        }
      }

      progress.CheckStatus();
    }
  }


  void unlockPoints(ControlNet &cnet, ControlPointList &cpList) {
    Progress progress;
    progress.SetText("Unlocking Points");
    progress.SetMaximumSteps(cnet.GetNumPoints());
    progress.CheckStatus();

    for (int cp = cnet.GetNumPoints() - 1; cp >= 0; cp--) {
      ControlPoint *point = cnet.GetPoint(cp);
      if (point->IsEditLocked() && cpList.HasControlPoint(point->GetId())) {
        point->SetEditLock(false);
      }
      progress.CheckStatus();
    }
  }


  /**
   * Iterates over the points in the Control Network looking for a match in the
   * list of Control Points to be ignored.  If a match is found, ignore the
   * point, and if the DELETE option was selected, the point will then be deleted
   * from the network.
   *
   * @param cnet     The Control Network being modified
   * @param cpList   List of Control Points
   */
  void ignorePoints(ControlNet &cnet, ControlPointList &cpList) {
    Progress progress;
    progress.SetText("Comparing Points to POINTLIST");
    progress.SetMaximumSteps(cnet.GetNumPoints());
    progress.CheckStatus();

    for (int cp = cnet.GetNumPoints() - 1; cp >= 0; cp--) {
      ControlPoint *point = cnet.GetPoint(cp);

      // Compare each Point Id listed with the Point in the
      // Control Network for according exclusion
      if (!point->IsIgnored() && cpList.HasControlPoint(point->GetId())) {
        ignorePoint(cnet, point, "Point ID in POINTLIST");
      }

      if (deleteIgnored) {
        //look for previously ignored control points
        if (point->IsIgnored()) {
          deletePoint(cnet, cp);
        }
        else {
          //look for previously ignored control measures
          for (int cm = point->GetNumMeasures() - 1; cm >= 0; cm--) {
            if (point->GetMeasure(cm)->IsIgnored() && deleteIgnored) {
              deleteMeasure(point, cm);
            }
          }
          // Check if there are too few measures in the point or the point was
          // previously ignored
          if (shouldDelete(point)) {
            deletePoint(cnet, cp);
          }
        }
      }

      progress.CheckStatus();
    }
  }


  /**
   * Lock points.
   *
   * @param ControlNet       Input ControlNet
   * @param ControlPointList List of points to lock
   */
  void lockPoints(ControlNet &cnet, ControlPointList &cpList) {
    Progress progress;
    progress.SetText("Locking Points");
    progress.SetMaximumSteps(cnet.GetNumPoints());
    progress.CheckStatus();

    for (int cp = cnet.GetNumPoints() - 1; cp >= 0; cp--) {
      ControlPoint *point = cnet.GetPoint(cp);
      if (!point->IsEditLocked() && cpList.HasControlPoint(point->GetId())) {
        point->SetEditLock(true);
      }
      progress.CheckStatus();
    }
  }


  /**
   * Unlock cubes.
   *
   * @param ControlNet       Input ControlNet
   * @param SerialNumberList List of cubes to unlock
   */
  void unlockCubes(ControlNet &cnet, SerialNumberList &snl) {
    Progress progress;
    progress.SetText("Unlocking Cubes");
    progress.SetMaximumSteps(cnet.GetNumPoints());
    progress.CheckStatus();

    for (int cp = cnet.GetNumPoints() - 1; cp >= 0; cp--) {
      ControlPoint *point = cnet.GetPoint(cp);

      for (int cm = point->GetNumMeasures() - 1; cm >= 0; cm--) {
        ControlMeasure *measure = point->GetMeasure(cm);

        QString serialNumber = measure->GetCubeSerialNumber();
        if (measure->IsEditLocked() && snl.hasSerialNumber(serialNumber)) {
          measure->SetEditLock(false);
        }
      }
      progress.CheckStatus();
    }
  }


  /**
   * Iterates over the list of Control Measures in the Control Network and
   * compares measure serial numbers with those in the input list of serial
   * numbers to be ignored.  If a match is found, ignore the measure.  If the
   * DELETE option was selected, the measure will then be deleted from the
   * network.
   *
   * @param cnet     The Control Network being modified
   * @param snl List of Serial Numbers to be ignored
   */
  void ignoreCubes(ControlNet &cnet, SerialNumberList &snl) {
    Progress progress;
    progress.SetText("Comparing Measures to CUBELIST");
    progress.SetMaximumSteps(cnet.GetNumPoints());
    progress.CheckStatus();

    for (int cp = cnet.GetNumPoints() - 1; cp >= 0; cp--) {
      ControlPoint *point = cnet.GetPoint(cp);

      // Compare each Serial Number listed with the serial number in the
      // Control Measure for according exclusion
      for (int cm = point->GetNumMeasures() - 1; cm >= 0; cm--) {
        ControlMeasure *measure = point->GetMeasure(cm);

        if (!point->IsIgnored() && point->GetMeasure(cm)->IsEditLocked()) {
          ignoreMeasure(cnet, point, measure, "EditLocked measure skipped");
        }

        QString serialNumber = measure->GetCubeSerialNumber();

        if (snl.hasSerialNumber(serialNumber)) {
          QString cause = "Serial Number in CUBELIST";
          if (cm == point->IndexOfRefMeasure() && retainRef) {
            logResult(retainedReferences, point->GetId(), cause);
          }
          else if (!measure->IsIgnored() || cm == point->IndexOfRefMeasure()) {
            ignoreMeasure(cnet, point, measure, cause);

            if (cm == point->IndexOfRefMeasure() && !point->IsIgnored() && ignoreAll) {
              ignorePoint(cnet, point, "Reference measure ignored");
            }
          }
        }

        // also look for previously ignored control measures
        if (deleteIgnored && measure->IsIgnored()) {
          deleteMeasure(point, cm);
        }
      }
      // Check if there are too few measures in the point or the point was
      // previously ignored
      if (shouldDelete(point)) {
        deletePoint(cnet, cp);
      }

      progress.CheckStatus();
    }
  }


  /**
   * Lock cubes.
   *
   * @param ControlNet       Input ControlNet
   * @param SerialNumberList List of cubes to Lock
   */
  void lockCubes(ControlNet &cnet, SerialNumberList &snl) {
    Progress progress;
    progress.SetText("Locking Cubes");
    progress.SetMaximumSteps(cnet.GetNumPoints());
    progress.CheckStatus();

    for (int cp = cnet.GetNumPoints() - 1; cp >= 0; cp--) {
      ControlPoint *point = cnet.GetPoint(cp);

      for (int cm = point->GetNumMeasures() - 1; cm >= 0; cm--) {
        ControlMeasure *measure = point->GetMeasure(cm);

        QString serialNumber = measure->GetCubeSerialNumber();
        if (!measure->IsEditLocked() && snl.hasSerialNumber(serialNumber)) {
          measure->SetEditLock(true);
        }
      }
      progress.CheckStatus();
    }
  }


  /**
   * Unlock measures.
   *
   * @param ControlNet                       Input ControlNet
   * @param QMap< QString, QSet<QString> * > Measures to unlock
   */
  void unlockMeasures(ControlNet &cnet,
      QMap< QString, QSet<QString> * > &editMeasures) {

    Progress progress;
    progress.SetText("Unlocking Measures");
    progress.SetMaximumSteps(cnet.GetNumPoints());
    progress.CheckStatus();

    for (int cp = cnet.GetNumPoints() - 1; cp >= 0; cp--) {
      ControlPoint *point = cnet.GetPoint(cp);

      QString id = point->GetId();
      if (editMeasures.contains(id)) {
        QSet<QString> *measureSet = editMeasures[id];

        for (int cm = point->GetNumMeasures() - 1; cm >= 0; cm--) {
          ControlMeasure *measure = point->GetMeasure(cm);

          QString serialNumber = measure->GetCubeSerialNumber();
          if (measure->IsEditLocked() && measureSet->contains(serialNumber)) {
            measure->SetEditLock(false);
          }
        }
      }
      progress.CheckStatus();
    }
  }


  /**
   * Ignore measures.
   *
   * @param ControlNet                       Input ControlNet
   * @param QMap< QString, QSet<QString> * > Measures to ignore
   */
  void ignoreMeasures(ControlNet &cnet,
      QMap< QString, QSet<QString> * > &editMeasures) {

    Progress progress;
    progress.SetText("Ignoring Measures");
    progress.SetMaximumSteps(cnet.GetNumPoints());
    progress.CheckStatus();

    for (int cp = cnet.GetNumPoints() - 1; cp >= 0; cp--) {
      ControlPoint *point = cnet.GetPoint(cp);

      QString id = point->GetId();
      if (editMeasures.contains(id)) {
        QSet<QString> *measureSet = editMeasures[id];

        // Compare each Serial Number listed with the serial number in the
        // Control Measure for according exclusion
        for (int cm = point->GetNumMeasures() - 1; cm >= 0; cm--) {
          ControlMeasure *measure = point->GetMeasure(cm);

          if (!point->IsIgnored() && point->GetMeasure(cm)->IsEditLocked()) {
            ignoreMeasure(cnet, point, measure, "EditLocked measure skipped");
          }

          QString serialNumber = measure->GetCubeSerialNumber();
          if (measureSet->contains(serialNumber)) {
            QString cause = "Measure in MEASURELIST";
            if (cm == point->IndexOfRefMeasure() && retainRef) {
              logResult(retainedReferences, point->GetId(), cause);
            }
            else if (!measure->IsIgnored() || cm == point->IndexOfRefMeasure()) {
              ignoreMeasure(cnet, point, measure, cause);

              if (cm == point->IndexOfRefMeasure() && !point->IsIgnored() && ignoreAll) {
                ignorePoint(cnet, point, "Reference measure ignored");
              }
            }
          }

          // also look for previously ignored control measures
          if (deleteIgnored && measure->IsIgnored()) {
            deleteMeasure(point, cm);
          }
        }
        // Check if there are too few measures in the point or the point was
        // previously ignored
        if (shouldDelete(point)) {
          deletePoint(cnet, cp);
        }
      }

      progress.CheckStatus();
    }
  }


  /**
   * Lock measures.
   *
   * @param ControlNet                       Input ControlNet
   * @param QMap< QString, QSet<QString> * > Measures to lock
   */
  void lockMeasures(ControlNet &cnet,
      QMap< QString, QSet<QString> * > &editMeasures) {

    Progress progress;
    progress.SetText("Locking Measures");
    progress.SetMaximumSteps(cnet.GetNumPoints());
    progress.CheckStatus();

    for (int cp = cnet.GetNumPoints() - 1; cp >= 0; cp--) {
      ControlPoint *point = cnet.GetPoint(cp);

      QString id = point->GetId();
      if (editMeasures.contains(id)) {
        QSet<QString> *measureSet = editMeasures[id];

        for (int cm = point->GetNumMeasures() - 1; cm >= 0; cm--) {
          ControlMeasure *measure = point->GetMeasure(cm);

          QString serialNumber = measure->GetCubeSerialNumber();
          if (!measure->IsEditLocked() && measureSet->contains(serialNumber)) {
            measure->SetEditLock(true);
          }
        }
      }
      progress.CheckStatus();
    }
  }


  /**
   * Compare each measure in the Control Network against tolerances specified in
   * the input DEFFILE.  Ignore any measure whose values fall outside the valid
   * tolerances, and delete it if the user specified to do so.
   *
   * @param cnet     The Control Network being modified
   * @param serialNumbers List of all Serial Numbers in the network
   */
  void checkAllMeasureValidity(ControlNet &cnet, SerialNumberList *serialNumbers) {

    QList<QString> cnetSerials = cnet.GetCubeSerials();
    Progress progress;
    progress.SetText("Checking Measure Validity");
    progress.SetMaximumSteps(cnetSerials.size());
    progress.CheckStatus();

    foreach (QString serialNumber, cnetSerials) {

      Cube *cube = NULL;
      Camera *camera = NULL;
      if (cneteditValidator->IsCubeRequired()) {
        if (!serialNumbers->hasSerialNumber(serialNumber)) {
          QString msg = "Serial Number [" + serialNumber + "] contains no ";
          msg += "matching cube in FROMLIST";
          throw IException(IException::User, msg, _FILEINFO_);
        }

        cube = new Cube;
        cube->open(serialNumbers->fileName(serialNumber));

        if (cneteditValidator->IsCameraRequired()) {
          try {
            camera = cube->camera();
          }
          catch (IException &e) {
            QString msg = "Cannot Create Camera for Image:" + cube->fileName();
            throw IException(e, IException::User, msg, _FILEINFO_);
          }
        }
      }

      QList<ControlMeasure *> measures = cnet.GetMeasuresInCube(serialNumber);
      for (int cm = 0; cm < measures.size(); cm++) {
        ControlMeasure *measure = measures[cm];
        ControlPoint *point = measure->Parent();

        if (!measure->IsIgnored()) {
          MeasureValidationResults results =
              validateMeasure(measure, cube, camera);

          if (!results.isValid()) {
            QString failure = results.toString();
            QString cause = "Validity Check " + failure;

            if (measure == point->GetRefMeasure() && retainRef) {
              logResult(retainedReferences, point->GetId(), cause);
            }
            else {
              ignoreMeasure(cnet, point, measure, cause);

              if (measure == point->GetRefMeasure() && ignoreAll) {
                ignorePoint(cnet, point, "Reference measure ignored");
              }
            }
          }
        }
      }

      delete cube;
      progress.CheckStatus();
    }

    for (int cp = cnet.GetNumPoints() - 1; cp >= 0; cp--) {
      ControlPoint *point = cnet.GetPoint(cp);

      for (int cm = point->GetNumMeasures() - 1; cm >= 0; cm--) {
        ControlMeasure *measure = point->GetMeasure(cm);

        // Also look for previously ignored control measures
        if (deleteIgnored && measure->IsIgnored()) {
          deleteMeasure(point, cm);
        }
      }

      // Check if there are too few measures in the point or the point was
      // previously ignored
      if (shouldDelete(point))
        deletePoint(cnet, cp);
    }
  }


  /**
   * Test an individual measure against the user-specified tolerances and return
   * the result.
   *
   * @param measure Measure to be tested
   * @param cube    Cube with serial number matching that of the current measure
   * @param camera  Camera associated with cube
   *
   * @return The results of validating the measure as an object containing the
   *         validity and a formatted error (or success) message
   */
  MeasureValidationResults validateMeasure(const ControlMeasure *measure,
      Cube *cube, Camera *camera) {

    MeasureValidationResults results =
        cneteditValidator->ValidStandardOptions(measure, cube, camera);

    return results;
  }


  void logResult(QMap<QString, QString> *pointsLog, QString pointId, QString cause) {
    if (keepLog) {
      // Label the keyword as the Point ID, and make the cause into the value
      (*pointsLog)[pointId] = cause;
    }
  }


  void logResult(QMap<QString, PvlGroup> *measuresLog,
      QString pointId, QString serial, QString cause) {

    if (keepLog) {
      // Make the keyword label the measure Serial Number, and the cause into the
      // value
      PvlKeyword measureMessage(PvlKeyword(serial, cause));

      // Using a map to make accessing by Point ID a O(1) to O(lg n) operation
      if (measuresLog->contains(pointId)) {
        // If the map already has a group for the given Point ID, simply add the
        // new measure to it
        PvlGroup &pointGroup = (*measuresLog)[pointId];
        pointGroup.addKeyword(measureMessage);
      }
      else {
        // Else there is no group for the Point ID of the measure being ignored,
        // so make a new group, add the measure, and insert it into the map
        PvlGroup pointGroup(pointId);
        pointGroup.addKeyword(measureMessage);
        (*measuresLog)[pointId] = pointGroup;
      }
    }
  }


  /**
   * Create points log.
   *
   * @param QString                Label for points log
   * @param QMap<QString, QString> Points map
   *
   * @return PvlObject             Points log
   */
  PvlObject createLog(QString label, QMap<QString, QString> *pointsMap) {
    PvlObject pointsLog(label);

    QList<QString> pointIds = pointsMap->keys();
    for (int i = 0; i < pointIds.size(); i++) {
      QString pointId = pointIds.at(i);
      pointsLog.addKeyword(PvlKeyword(pointId, (*pointsMap)[pointId]));
    }

    return pointsLog;
  }


  /**
   * Create measures log.
   *
   * @param QString                Label for measures log
   * @param QMap<QString, QString> Points map
   * @param QMap<QString, QString> Measures map
   *
   * @return PvlObject             Measures log
   */
  PvlObject createLog(QString label,
      QMap<QString, QString> *pointsMap, QMap<QString, PvlGroup> *measuresMap) {

    PvlObject editLog(label);

    PvlObject pointsLog = createLog("Points", pointsMap);
    editLog.addObject(pointsLog);

    // Get all the groups of measures from the map
    PvlObject measuresLog("Measures");
    QList<PvlGroup> measureGroups = measuresMap->values();

    for (int i = 0; i < measureGroups.size(); i++)
      measuresLog.addGroup(measureGroups.at(i));

    editLog.addObject(measuresLog);
    return editLog;
  }
}


