/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "cnetextract.h"

#include "Application.h"

using namespace std;

namespace Isis {
  void ExtractPointList(ControlNet &outNet, QVector<QString> &nonListedPoints, FileName pointListFile);
  void ExtractLatLonRange(ControlNet &outNet, QVector<QString> &nonLatLonPoints,
                          QVector<QString> &cannotGenerateLatLonPoints,
                          QMap<QString, QString> sn2filename,
                          UserInterface &ui);
  bool NotInLatLonRange(SurfacePoint surfacePt, Latitude minlat,
                        Latitude maxlat, Longitude minlon, Longitude maxlon);
  void WriteCubeOutList(ControlNet cnet,
                        QMap<QString, QString> sn2file,
                        PvlGroup &summary,
                        FileName toList);
  void WriteResults(QString filename,
                    QVector<QString> notExtracted,
                    PvlGroup &results);
  void omit(ControlNet &cnet, int cp);
  void omit(ControlPoint *point, int cm);

  void cnetextract(UserInterface &ui, Pvl *log) {
    ControlNet outNet(ui.GetFileName("CNET"));

    cnetextract(outNet, ui, log);
  }


  // Main program
  void cnetextract(ControlNet outNet, UserInterface &ui, Pvl *log) {
    if(!ui.WasEntered("FROMLIST") && ui.WasEntered("TOLIST")) {
      std::string msg = "To create a [TOLIST] the [FROMLIST] parameter must be provided.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    bool noIgnore          = ui.GetBoolean("NOIGNORE");
    bool noMeasureless     = ui.GetBoolean("NOMEASURELESS");
    bool noSingleMeasure   = ui.GetBoolean("NOSINGLEMEASURES");
    bool reference         = ui.GetBoolean("REFERENCE");
    bool fixed             = ui.GetBoolean("FIXED");
    bool constrained       = ui.GetBoolean("CONSTRAINED");
    bool editLocked        = ui.GetBoolean("EDITLOCKED");
    bool noTolerancePoints = ui.GetBoolean("TOLERANCE");
    bool pointsEntered     = ui.WasEntered("POINTLIST");
    bool cubePoints        = ui.GetBoolean("CUBES");
    bool cubeMeasures      = ui.GetBoolean("CUBEMEASURES");
    bool retainReference   = ui.GetBoolean("RETAIN_REFERENCE");
    bool latLon            = ui.GetBoolean("LATLON");

    if(!(noIgnore || noMeasureless || noSingleMeasure || editLocked || reference || fixed ||
         noTolerancePoints || pointsEntered || cubePoints || constrained || latLon)) {
      std::string msg = "At least one filter must be selected [";
      msg += "NOIGNORE,NOMEASURELESS,NOSINGLEMEASURE,REFERENCE,FIXED,CONSTRAINED,EDITLOCKED,TOLERANCE,";
      msg += "POINTLIST,CUBES,LATLON]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if(cubeMeasures || reference) {
      noMeasureless = true;
    }

    FileList inList;
    if(ui.WasEntered("FROMLIST")) {
      //inList = ui.GetFileName("FROMLIST");
      inList.read(ui.GetFileName("FROMLIST").toStdString());
    }

    int inputPoints = outNet.GetNumPoints();
    int inputMeasures = 0;
    for (int cp = 0; cp < outNet.GetNumPoints(); cp++)
      inputMeasures += outNet.GetPoint(cp)->GetNumMeasures();

    // Set up the Serial Number to FileName mapping
    QMap<QString, QString> sn2filename;
    for(int cubeIndex = 0; cubeIndex < (int)inList.size(); cubeIndex ++) {
      QString sn = SerialNumber::Compose(QString::fromStdString(inList[cubeIndex].toString()));
      sn2filename[sn] = QString::fromStdString(inList[cubeIndex].toString());
    }


    Progress progress;
    progress.SetMaximumSteps(outNet.GetNumPoints());
    progress.CheckStatus();

    // Set up vector records of how points/measures are removed
    QVector<QString> ignoredPoints;
    QVector<QString> ignoredMeasures;
    QVector<QString> singleMeasurePoints;
    QVector<QString> measurelessPoints;
    QVector<QString> tolerancePoints;
    QVector<QString> nonReferenceMeasures;
    QVector<QString> nonFixedPoints;
    QVector<QString> nonConstrainedPoints;
    QVector<QString> nonEditLockedPoints;
    QVector<QString> nonCubePoints;
    QVector<QString> noCubeMeasures;
  // This is commented out since this does not correspond to any filters or the
  //  documentation of this application. I did not delete the code in case we find
  //  that we need it later. J.Backer 2012-06-22
  //
  //  QVector<QString> noMeasurePoints;
    QVector<QString> nonListedPoints;
    QVector<QString> nonLatLonPoints;
    QVector<QString> cannotGenerateLatLonPoints;

    // Set up comparison data
    QVector<QString> serialNumbers;
    if(cubePoints) {
      FileList cubeList(ui.GetFileName("CUBELIST").toStdString());
      for(int cubeIndex = 0; cubeIndex < (int)cubeList.size(); cubeIndex ++) {
        QString sn = SerialNumber::Compose(QString::fromStdString(cubeList[cubeIndex].toString()));
        serialNumbers.push_back(sn);
      }
    }

    double tolerance = 0.0;
    if(noTolerancePoints) {
      tolerance = ui.GetDouble("PIXELTOLERANCE");
    }

    // Set up extracted network values
    if(ui.WasEntered("NETWORKID"))
      outNet.SetNetworkId(ui.GetString("NETWORKID"));

    outNet.SetUserName(Isis::Environment::userName());
    outNet.SetDescription(ui.GetString("DESCRIPTION"));

    for(int cp = outNet.GetNumPoints() - 1; cp >= 0; cp --) {
      progress.CheckStatus();

      ControlPoint *controlpt = outNet.GetPoint(cp);

      // Do preliminary exclusion checks
      if(noIgnore && controlpt->IsIgnored()) {
        ignoredPoints.append(controlpt->GetId());
        omit(outNet, cp);
        continue;
      }
      if(fixed && !(controlpt->GetType() == ControlPoint::Fixed)) {
        nonFixedPoints.append(controlpt->GetId());
        omit(outNet, cp);
        continue;
      }
      if(constrained && !(controlpt->GetType() == ControlPoint::Constrained)) {
        nonConstrainedPoints.append(controlpt->GetId());
        omit(outNet, cp);
        continue;
      }


      if(noSingleMeasure) {
        bool invalidPoint = false;
        invalidPoint |= noIgnore && (controlpt->GetNumValidMeasures() < 2);
        invalidPoint |= controlpt->GetNumMeasures() < 2 && (controlpt->GetType() != ControlPoint::Fixed);

        if(invalidPoint) {
          singleMeasurePoints.append(controlpt->GetId());
          omit(outNet, cp);
          continue;
        }
      }
      if (editLocked && !(controlpt->IsEditLocked())){
        nonEditLockedPoints.append(controlpt->GetId());
        omit(outNet, cp);
        continue;
      }

      // Change the current point into a new point by manipulation of its control measures
      ControlPoint *newPoint = outNet.GetPoint(cp);
      bool replaceLock = false;
      if (newPoint->IsEditLocked()) {
        newPoint->SetEditLock(false);
        replaceLock = true;
      }

      for(int cm = newPoint->GetNumMeasures() - 1; cm >= 0; cm --) {
        const ControlMeasure *newMeasure = newPoint->GetMeasure(cm);

        if(noIgnore && newMeasure->IsIgnored()) {
          //New error with deleting Reference Measures
          QString msg = newPoint->GetId() + "," + newMeasure->GetCubeSerialNumber();
          if(newPoint->GetRefMeasure() != newMeasure) {
            omit(newPoint, cm);
          }
          else{
            msg += ", Ignored measure but extracted since it is Reference";
          }
          ignoredMeasures.append(msg);
        }
        else if(reference && newPoint->GetRefMeasure() != newMeasure) {
          nonReferenceMeasures.append(newPoint->GetId() + "," + newMeasure->GetCubeSerialNumber());
          omit(newPoint, cm);
        }
        else if(cubeMeasures) {
          bool hasSerialNumber = false;
          QString serialNum = newMeasure->GetCubeSerialNumber();
          for(int sn = 0; sn < serialNumbers.size(); sn ++) {
            if(serialNumbers[sn] == serialNum) {
              hasSerialNumber = true;
              break;
            }
          }

          // doesn't have serial number if measure is not associated with cubelist
          // we need to omit appropriate measures
          if(!hasSerialNumber) {
            QString msg = newPoint->GetId() + "," + newMeasure->GetCubeSerialNumber();
            // if this measure is not reference, omit it
            // if this measure is a reference, but retainReference is off, omit it
            if(newPoint->GetRefMeasure() != newMeasure ||
               (newPoint->GetRefMeasure() == newMeasure && !retainReference))
              omit(newPoint, cm);
            else {
              //this is unnecessary if statement. only other possibility for else...
              // if(newPoint->GetRefMeasure() == newMeasure && retainReference) {
                msg += ", Reference not in the cubelist but extracted since "
                       "RETAIN_REFERENCE=true";
              // }
            }
            noCubeMeasures.append(msg);
          }
        }
      }

      if (replaceLock)
        newPoint->SetEditLock(true);

      //outNet.UpdatePoint(newPoint); // Fixed by redesign

      // Check for line/sample errors above provided tolerance
      if(noTolerancePoints) {
        bool hasLowTolerance = true;

        for(int cm = 0; cm < newPoint->GetNumMeasures() && hasLowTolerance; cm ++) {
          const ControlMeasure *newMeasure = newPoint->GetMeasure(cm);
          if(newMeasure->GetSampleResidual() >= tolerance ||
              newMeasure->GetLineResidual() >= tolerance) {
            hasLowTolerance = false;
          }
        }

        if(hasLowTolerance) {
          tolerancePoints.append(newPoint->GetId());
          omit(outNet, cp);
          continue;
        }
      }

      // Do not add outPoint if it has too few measures
      if(noSingleMeasure) {
        bool invalidPoint = false;
        invalidPoint |= noIgnore && (newPoint->GetNumValidMeasures() < 2);
        invalidPoint |= newPoint->GetNumMeasures() < 2 && newPoint->GetType() != ControlPoint::Fixed;

        if(invalidPoint) {
          singleMeasurePoints.append(controlpt->GetId());
          omit(outNet, cp);
          continue;
        }
      }

      // Do not add outPoint if it does not have a cube in CUBELIST as asked
      if(cubePoints) {
        bool hasSerialNumber = false;

        for(int cm = 0; cm < newPoint->GetNumMeasures() && !hasSerialNumber; cm ++) {
          for(int sn = 0; sn < serialNumbers.size() && !hasSerialNumber; sn ++) {
            if(serialNumbers[sn] == newPoint->GetMeasure(cm)->GetCubeSerialNumber())
              hasSerialNumber = true;
          }
        }

        if(!hasSerialNumber) {
          nonCubePoints.append(newPoint->GetId());
          omit(outNet, cp);
          continue;
        }
      }

      if(noMeasureless && newPoint->GetNumMeasures() == 0) {
        measurelessPoints.append(newPoint->GetId());
        omit(outNet, cp);
        continue;
      }
    } // Finished with simple comparisons

     // Use another pass to check for Ids
    if(pointsEntered) {
      ExtractPointList(outNet, nonListedPoints, ui.GetFileName("POINTLIST").toStdString());
    }


     // Use another pass on outNet, because this is by far the most time consuming
     // process, and time could be saved by using the reduced size of outNet
    if(latLon) {
      ExtractLatLonRange(outNet, nonLatLonPoints, cannotGenerateLatLonPoints, sn2filename, ui);
    }

    int outputPoints = outNet.GetNumPoints();
    int outputMeasures = 0;
    for (int cp = 0; cp < outNet.GetNumPoints(); cp++){
      outputMeasures += outNet.GetPoint(cp)->GetNumMeasures();
    }

    // if the output control net is not empty, write it out.
    Progress outProgress;
    outProgress.SetMaximumSteps(3);


    // Adds the remove history to the summary and results group
    PvlGroup summary("ResultSummary");
    PvlGroup results("Results");

    summary.addKeyword(PvlKeyword("InputPoints", std::to_string(inputPoints)));
    summary.addKeyword(PvlKeyword("InputMeasures", std::to_string(inputMeasures)));
    summary.addKeyword(PvlKeyword("OutputPoints", std::to_string(outputPoints)));
    summary.addKeyword(PvlKeyword("OutputMeasures", std::to_string(outputMeasures)));

    if (outputPoints != 0) {
      // Write the filenames associated with outNet
      if (ui.WasEntered("TOLIST") ) {
        WriteCubeOutList(outNet, sn2filename, summary, ui.GetFileName("TOLIST").toStdString());
      }

      outProgress.SetText("Writing Control Network");
      outProgress.CheckStatus();

      // Write the extracted Control Network
      outNet.Write(ui.GetFileName("ONET"));
      outProgress.CheckStatus();
    }
    else {
      summary.addComment("The output control network file, ["
                         + ui.GetFileName("ONET").toStdString() +
                         "],  was not created. "
                         "The provided filters have resulted in no points or "
                         "measures extracted.");
      if (ui.WasEntered("TOLIST")) {
        summary.addComment("The output cube list file, ["
                           + ui.GetFileName("TOLIST").toStdString() +
                           "], was not created. "
                           "The provided filters have resulted in an empty"
                           "Control Network.");
      }
    }


    if(noIgnore) {
      summary.addKeyword(PvlKeyword("IgnoredPoints", std::to_string((int)ignoredPoints.size())));
      summary.addKeyword(PvlKeyword("IgnoredMeasures", std::to_string((int)ignoredMeasures.size())));
    }
    if(noSingleMeasure) {
      summary.addKeyword(PvlKeyword("SingleMeasurePoints", std::to_string((int)singleMeasurePoints.size())));
    }
    if(noMeasureless) {
      summary.addKeyword(PvlKeyword("MeasurelessPoints", std::to_string((int)measurelessPoints.size())));
    }
    if(noTolerancePoints) {
      summary.addKeyword(PvlKeyword("TolerancePoints", std::to_string((int)tolerancePoints.size())));
    }
    if(reference) {
      summary.addKeyword(PvlKeyword("NonReferenceMeasures", std::to_string((int)nonReferenceMeasures.size())));
    }
    if(fixed) {
      summary.addKeyword(PvlKeyword("NonFixedPoints", std::to_string((int)nonFixedPoints.size())));
    }
    if(cubePoints) {
      summary.addKeyword(PvlKeyword("NonCubePoints", std::to_string((int)nonCubePoints.size())));
    }
  // This is commented out since this does not correspond to any filters or the
  //  documentation of this application. I did not delete the code in case we find
  //  that we need it later. J.Backer 2012-06-22
  //
  //  if(noMeasurePoints.size() != 0) {
  //    summary.addKeyword(PvlKeyword("NoCubeMeasure", std::to_string((int)noMeasurePoints.size())));
  //  }
    if(cubeMeasures) {
      summary.addKeyword(PvlKeyword("NonCubeMeasures", std::to_string((int)noCubeMeasures.size())));
    }
    if(pointsEntered) {
      summary.addKeyword(PvlKeyword("NonListedPoints", std::to_string((int)nonListedPoints.size())));
    }
    if(latLon) {
      summary.addKeyword(PvlKeyword("LatLonOutOfRange", std::to_string((int)nonLatLonPoints.size())));
      summary.addKeyword(PvlKeyword("NoLatLonPoints", std::to_string((int)cannotGenerateLatLonPoints.size())));
    }

    outProgress.CheckStatus();

    // Log Control Net results
    Application::AppendAndLog(summary, log);

    outProgress.CheckStatus();

    Progress resultsProgress;
    if(ui.WasEntered("PREFIX")) {
      if (outputPoints == inputPoints && outputMeasures == inputMeasures) {
        results.addComment("No filter reports were created since all points and "
                           "measures from the input control network were "
                           "extracted into the output control network.");
      }
      else {
        resultsProgress.SetText("Writing Results");
        resultsProgress.SetMaximumSteps(11);
        resultsProgress.CheckStatus();

        QString prefix = ui.GetString("PREFIX");

        if(noIgnore) {
          QString namecp = QString::fromStdString(FileName(prefix.toStdString() + "IgnoredPoints.txt").expanded());
          WriteResults(namecp, ignoredPoints, results);
          QString namecm = QString::fromStdString(FileName(prefix.toStdString() + "IgnoredMeasures.txt").expanded());
          WriteResults(namecm, ignoredMeasures, results);
        }

        resultsProgress.CheckStatus();

        if(noSingleMeasure) {
          QString name = QString::fromStdString(FileName(prefix.toStdString() + "SingleMeasurePoints.txt").expanded());
          WriteResults(name, singleMeasurePoints, results);
        }

        resultsProgress.CheckStatus();

        if(noMeasureless) {
          QString name = QString::fromStdString(FileName(prefix.toStdString() + "MeasurelessPoints.txt").expanded());
          WriteResults(name, measurelessPoints, results);
        }

        resultsProgress.CheckStatus();

        if(noTolerancePoints) {
          QString name = QString::fromStdString(FileName(prefix.toStdString() + "TolerancePoints.txt").expanded());
          WriteResults(name, tolerancePoints, results);
        }

        resultsProgress.CheckStatus();

        if(reference) {
          QString name = QString::fromStdString(FileName(prefix.toStdString() + "NonReferenceMeasures.txt").expanded());
          WriteResults(name, nonReferenceMeasures, results);
        }

        resultsProgress.CheckStatus();

        if(fixed) {
          QString name = QString::fromStdString(FileName(prefix.toStdString() + "NonFixedPoints.txt").expanded());
          WriteResults(name, nonFixedPoints, results);
        }

        resultsProgress.CheckStatus();

        if(cubePoints) {
          std::string name = FileName(prefix.toStdString() + "NonCubePoints.txt").expanded();
          WriteResults(QString::fromStdString(name), nonCubePoints, results);
        }

        resultsProgress.CheckStatus();

  // This is commented out since this does not correspond to any filters or the
  //  documentation of this application. I did not delete the code in case we find
  //  that we need it later. J.Backer 2012-06-22
  //
  //      if(noMeasurePoints.size() != 0) {
  //        QString name = FileName(prefix + "NoMeasurePoints.txt").expanded();
  //        WriteResults(name, noMeasurePoints);
  //      }

        resultsProgress.CheckStatus();

        if(cubeMeasures) {
          std::string name = FileName(prefix.toStdString() + "NonCubeMeasures.txt").expanded();
          WriteResults(QString::fromStdString(name), noCubeMeasures, results);
        }

        resultsProgress.CheckStatus();

        if(pointsEntered) {
          std::string name = FileName(prefix.toStdString() + "NonListedPoints.txt").expanded();
          WriteResults(QString::fromStdString(name), nonListedPoints, results);
        }

        resultsProgress.CheckStatus();

        if(latLon) {
          std::string namenon = FileName(prefix.toStdString() + "LatLonOutOfRange.txt").expanded();
          WriteResults(QString::fromStdString(namenon), nonLatLonPoints, results);
          std::string namegen = FileName(prefix.toStdString() + "NoLatLonPoints.txt").expanded();
          WriteResults(QString::fromStdString(namegen), cannotGenerateLatLonPoints, results);
        }

        results.addComment("Each keyword represents a filter parameter used. "
                           "Check the documentation for specific keyword descriptions.");
      }
      Application::AppendAndLog(results, log);


      resultsProgress.CheckStatus();
    }

  }


  /**
   * Removes control points not listed in POINTLIST
   *
   * @param outNet The output control net being removed from
   * @param nonListedPoints The keyword recording all of the control points
   *                        removed due to not being listed
   * @history 2012-06-22 - Jeannie Backer - Changed nonListedPoints parameter to
   *                           reference so that this information could be
   *                           accessed in the main program.
   */
  void ExtractPointList(ControlNet &outNet, QVector<QString> &nonListedPoints, FileName pointListFile) {
    // Use the file list class for functionality
    // (even though this is a list of point IDs)
    FileList listedPoints(pointListFile);

    // loop through control point indices, backwards
    for(int cp = outNet.GetNumPoints() - 1; cp >= 0; cp--) {
      ControlPoint *controlpt = outNet.GetPoint(cp);
      // flag to determine if this controlpoint is in the pointlist
      bool isInList = false;
      // loop through line numbers of POINTLIST until we find a point ID in the
      // list that matches this control point's ID
      for(int i = 0; i < (int)listedPoints.size()  &&  !isInList; i++) {
        QString pointId = QString::fromStdString(listedPoints[i].toString());
        // isInList is true if these strings are equal
        isInList = pointId.toLower() == controlpt->GetId().toLower();
      }
      if(!isInList) {
        nonListedPoints.append(controlpt->GetId());
        omit(outNet, cp);
      }
    }
    return;
  }


  /**
   * Removes control points not in the lat/lon range provided in the input
   * parameters.
   *
   * @param outNet[in] The output control net being removed from
   * @param noLanLonPoint[out] The keyword recording all of the control points removed
   *            due to the control point being out of the lat/lon range.
   * @param cannotGenerateLatLonPoints[out] The keyword recording all of the control points removed
   *            due to the inability to find a cube to calculate the lat/lon for that point.
   * @param sn2filename[in] QMap that maps the serial numbers to (input) file names.
   *
   * @internal
   *   @history 2016-09-29 Ian Humphrey - Reverted r6597, which had reveresed the logic for
   *                           checking the reference measure and other measures (see lines 635,643).
   *                           Modified the QVector parameters to be pass-by-reference OUT parameters,
   *                           since the cnetextract main uses them for summary output.
   */
  void ExtractLatLonRange(ControlNet &outNet,
                          QVector<QString> &nonLatLonPoints,
                          QVector<QString> &cannotGenerateLatLonPoints,
                          QMap<QString, QString> sn2filename,
                          UserInterface &ui) {
    if(outNet.GetNumPoints() == 0) {
      return;
    }

    // Get the lat/lon and fix the range for the internal 0/360
    Latitude minlat(ui.GetDouble("MINLAT"), Angle::Degrees);
    Latitude maxlat(ui.GetDouble("MAXLAT"), Angle::Degrees);
    Longitude minlon = Longitude(ui.GetDouble("MINLON"), Angle::Degrees);
    Longitude maxlon = Longitude(ui.GetDouble("MAXLON"), Angle::Degrees);

    Progress progress;
    progress.SetText("Calculating lat/lon");
    progress.SetMaximumSteps(outNet.GetNumPoints());
    progress.CheckStatus();

    CubeManager manager;
    manager.SetNumOpenCubes(50);   //Should keep memory usage to around 1GB

    bool hasFromList = ui.WasEntered("FROMLIST");
    for(int cp = outNet.GetNumPoints() - 1; cp >= 0; cp --) {
      progress.CheckStatus();
      const ControlPoint *controlPt = outNet.GetPoint(cp);
      SurfacePoint surfacePt = controlPt->GetBestSurfacePoint();

      // If the Contorl Network takes priority, use it
      if(surfacePt.Valid()) {
        if(NotInLatLonRange(surfacePt, minlat, maxlat, minlon, maxlon)) {
          nonLatLonPoints.push_back(controlPt->GetId());
          omit(outNet, cp);
        }
      }

      /**
       * If the lat/lon cannot be determined from the point, then we need to calculate
       * lat/lon on our own
       */
      else if(hasFromList) {

        // Find a cube in the Control Point to get the lat/lon from
        int cm = 0;
        QString sn = "";
        Latitude lat;
        Longitude lon;
        Distance radius;

        // First check the reference Measure
        if(sn2filename[controlPt->GetReferenceSN()].length() != 0) {
          sn = controlPt->GetReferenceSN();
        }

        // Search for other Control Measures if needed
        if(sn.isEmpty()) {
          // Find the Serial Number if it exists
          for(int cm = 0; (cm < controlPt->GetNumMeasures()) && sn.isEmpty(); cm ++) {
            if(sn2filename[controlPt->GetReferenceSN()].length() != 0) {
              sn = controlPt->GetReferenceSN();
            }
          }
        }

        // Cannot find a cube to get the lat/lon from
        if(sn.isEmpty()) {
          cannotGenerateLatLonPoints.push_back(controlPt->GetId());
          omit(outNet, cp);
        }

        // Calculate the lat/lon and check for validity
        else {
          bool remove = false;

          Cube *cube = manager.OpenCube(sn2filename[sn]);
          Camera *camera = cube->camera();

          if(camera == NULL) {
            try {
              TProjection *projection =
                (TProjection *) ProjectionFactory::Create((*(cube->label())));

              if(!projection->SetCoordinate(controlPt->GetMeasure(cm)->GetSample(),
                                            controlPt->GetMeasure(cm)->GetLine())) {
                nonLatLonPoints.push_back(controlPt->GetId());
                remove = true;
              }

              lat = Latitude(projection->Latitude(), Angle::Degrees);
              lon = Longitude(projection->Longitude(), Angle::Degrees);
              radius = Distance(projection->LocalRadius(), Distance::Meters);

              delete projection;
              projection = NULL;
            }
            catch(IException &) {
              remove = true;
            }
          }
          else {
            if(!camera->SetImage(controlPt->GetMeasure(cm)->GetSample(),
                                 controlPt->GetMeasure(cm)->GetLine())) {
              nonLatLonPoints.push_back(controlPt->GetId());
              remove = true;
            }

            lat = camera->GetLatitude();
            lon = camera->GetLongitude();
            radius = camera->LocalRadius();

            camera = NULL;
          }

          cube = NULL;

          bool notInRange = false;
          bool validLatLonRadius = lat.isValid() && lon.isValid() && radius.isValid();
          if(validLatLonRadius) {
            SurfacePoint sfpt(lat, lon, radius);
            notInRange = NotInLatLonRange(sfpt, minlat, maxlat, minlon, maxlon);
          }

          if(remove || notInRange) {
            nonLatLonPoints.push_back(controlPt->GetId());
            omit(outNet, cp);
          }
          else if(validLatLonRadius) { // Add the reference lat/lon/radius to the Control Point
            outNet.GetPoint(cp)->SetAprioriSurfacePoint(SurfacePoint(lat, lon, radius));
          }
        }
      }
      else {
        cannotGenerateLatLonPoints.push_back(controlPt->GetId());
        omit(outNet, cp);
      }

    }

    manager.CleanCubes();
  }


  /**
   * Checks whether the given surface point is in the given lat/lon range,
   * handling the meridian correctly
   *
   * @param lat The latitude to check
   * @param lon The longitude to check
   * @param minlat Minimum Latitude Minimum valid latitude
   * @param maxlat Maximum Latitude Maximum valid latitude
   * @param minlon Minimum Longitude Minimum valid longitude
   * @param maxlon Maximum Longitude Maximum valid longitude
   *
   * @return bool True when the range is valid
   */
  bool NotInLatLonRange(SurfacePoint surfacePtToTest, Latitude minlat,
                        Latitude maxlat, Longitude minlon, Longitude maxlon) {
    Latitude lat = surfacePtToTest.GetLatitude();
    Longitude lon = surfacePtToTest.GetLongitude();

    bool outRange = false;
    try {
      outRange = !lat.inRange(minlat, maxlat) || !lon.inRange(minlon, maxlon);
    }
    catch (IException &e) {
      std::string msg = "Cannot complete lat/lon range test with given filters";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }

    return outRange;
  }


  /**
   * Creates the output list, [TOLIST], if the parameter is entered. This method
   * finds all cubes contained within the given Control Network and lists the
   * corresponding file names for these cubes in the [TOLIST] output file.
   *
   * @param cnet The Control Network from which the cube list will be found and
   *             created.
   * @param sn2file The map for converting the Control Network's serial numbers
   *                to filenames.
   */
  void WriteCubeOutList(ControlNet cnet,
                        QMap<QString, QString> sn2file,
                        PvlGroup &summary,
                        FileName toList) {
    Progress p;
    p.SetText("Writing Cube List");
    try {
      p.SetMaximumSteps(cnet.GetNumPoints());
      p.CheckStatus();
    }
    catch(IException &e) {
      std::string msg = "Unable to write the output cube list, [TOLIST].";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }

    std::set<QString> outputsn;
    for(int cp = 0; cp < cnet.GetNumPoints(); cp ++) {
      for(int cm = 0; cm < cnet.GetPoint(cp)->GetNumMeasures(); cm ++) {
        QString sn = cnet.GetPoint(cp)->GetMeasure(cm)->GetCubeSerialNumber();
        if (sn2file[sn].length() != 0) {
          outputsn.insert(sn);
          }
        }
      p.CheckStatus();
    }
    // Don't create file if it will be empty
    if (outputsn.size() == 0) {
      summary.addComment("The output cube list file, ["
                         + toList.toString() + "], was not created. "
                         "The provided filters have resulted in an empty"
                         "Control Network.");
      return;
    }

    std::ofstream out_stream;
    out_stream.open(toList.toString().c_str(), std::ios::out);
    out_stream.seekp(0, std::ios::beg);   //Start writing from beginning of file

    for(std::set<QString>::iterator sn = outputsn.begin(); sn != outputsn.end(); sn ++) {
      // moved the "if" statement to the previous for loop to prevent creating
      // an empty file
      //if(!sn2file[(*sn)].length() == 0) {
        out_stream << sn2file[(*sn)].toStdString() << std::endl;
      //}
    }

    out_stream.close();
    return;
  }


  /**
   * Creates the results report using the given file name and vector of measures
   * or points that were not extracted.
   *
   * @param filename The name of the report file to create.
   * @param notExtracted  A vector of points and/or measures to be listed in the
   *                      report.
   * @param results  A reference to the results PvlGroup address.
   */
  void WriteResults(QString filename,
                    QVector<QString> notExtracted,
                    PvlGroup &results) {
    // if no points or measures are being extracted,
    // we will not create the filter report.
    if(notExtracted.size() == 0) {
      results.addComment("The output report ["+ filename.toStdString() +"] was not created. "
                         "The corresponding filter found no points/measures that "
                         "would not be extracted.");
      return;
    }

    // Set up the output file for writing
    std::ofstream out_stream;
    out_stream.open(filename.toLatin1().data(), std::ios::out);
    out_stream.seekp(0, std::ios::beg);   //Start writing from beginning of file

    out_stream << notExtracted[0].toStdString();
    for(int index = 1; index < notExtracted.size(); index ++) {
      out_stream << std::endl << notExtracted[index].toStdString();
    }

    out_stream.close();
    results.addKeyword(PvlKeyword("ReportCreated", filename.toStdString()));
    return;
  }

  /**
   * This method removes the given control point from the given control network.
   */
  void omit(ControlNet &cnet, int cp) {
    ControlPoint *point = cnet.GetPoint(cp);
    if (point->IsEditLocked()) point->SetEditLock(false);
    cnet.DeletePoint(cp);
    return;
  }

  /**
   * This method removes the given control measure from the given control point.
   */
  void omit(ControlPoint *point, int cm) {
    ControlMeasure *measure = point->GetMeasure(cm);
    if (measure->IsEditLocked()) measure->SetEditLock(false);
    point->Delete(cm);
    return;
  }


  //   Modified [PREFIX]IgnoredMeasures.txt output to exclude
  //      ignored reference measures since these measures will be extracted.

}
