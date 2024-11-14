
/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iostream>
#include <sstream>
#include <set>

#include <QList>
#include <QMap>
#include <QStack>
#include <QString>
#include <QVector>

#include <geos_c.h>
#include <geos/algorithm/ConvexHull.h>
#include <geos/geom/CoordinateSequence.h>
#include <geos/geom/Envelope.h>
#include <geos/geom/Geometry.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/Polygon.h>

#include "Application.h"
#include "Camera.h"
#include "CameraFactory.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "ControlPointList.h"
#include "CubeManager.h"
#include "FileList.h"
#include "FileName.h"
#include "IString.h"
#include "Progress.h"
#include "ProjectionFactory.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "SerialNumber.h"
#include "SerialNumberList.h"
#include "UserInterface.h"

#include "cnetcheck.h"

using namespace std;

namespace Isis {
  QMap< QString, set<QString> > constructPointSets(
    set<QString> &index, ControlNet innet, UserInterface &ui);
  QVector< set<QString> > findIslands(
    set<QString> &index,
    QMap< QString, set<QString> > adjCubes);

  void writeOutput(SerialNumberList num2cube,
                   QString filename,
                   set<QString> sns,
                   QMap< QString, set<QString> > cps);

  double getControlFitness(ControlNet &cnet, QString sn, double tolerance, Cube * cube);
  void noLatLonCheck(ControlNet &cnet, CubeManager &manager, Progress &progress,
      bool ignore, SerialNumberList &num2cube,
      set<QString> &noLatLonSerialNumbers,
      QMap< QString, set<QString> > &noLatLonControlPoints);

  QString buildRow(SerialNumberList &serials, QString sn);
  QString buildRow(SerialNumberList &serials, QString sn, set<QString> &cps);
  QString buildRow(SerialNumberList &serials, QString sn, double value);
  void outputRow(ofstream &outStream, QString rowText);

  QString g_delimiter;

  /**
   * check control network validity
   *
   * @param ui UserInterface object containing parameters
   * @param(out) log The Pvl that outlier results logged to check
   */
  QString cnetcheck(UserInterface &ui, Pvl *log) {
   ControlNet innet(ui.GetFileName("CNET"));
   FileList inlist(ui.GetFileName("FROMLIST"));

   return cnetcheck(innet, inlist, ui, log);
  }

  /**
   * check control network validity
   *
   * @param innet input control network
   * @param inlist input file list
   * @param ui UserInterface object containing parameters
   * @param(out) log The Pvl that outlier results logged to
   */
  QString cnetcheck(ControlNet &innet, FileList &inlist, UserInterface &ui, Pvl *log) {
    Progress progress;

    QString prefix(ui.GetString("PREFIX"));
    bool ignore = ui.GetBoolean("IGNORE");

    // Set the character to separate the entries
    if (ui.GetString("DELIMIT") == "TAB") {
      g_delimiter = "\t";
    }
    else if (ui.GetString("DELIMIT") == "COMMA") {
      g_delimiter = ",";
    }
    else if (ui.GetString("DELIMIT") == "SPACE") {
      g_delimiter = " ";
    }
    else {
      g_delimiter = ui.GetString("CUSTOM");
    }

    // Sets up the list of serial numbers for
    set<QString> inListNums;
    QMap<QString, int> netSerialNumCount;
    QVector<QString> listedSerialNumbers;
    SerialNumberList num2cube;

    if (inlist.size() > 0) {
      progress.SetText("Initializing");
      progress.SetMaximumSteps(inlist.size());
      progress.CheckStatus();
    }

    for (int index = 0; index < inlist.size(); index++) {
      num2cube.add(inlist[index].toString());
      QString st = num2cube.serialNumber(inlist[index].toString());
      inListNums.insert(st);
      listedSerialNumbers.push_back(st);   // Used with nonListedSerialNumbers
      progress.CheckStatus();
    }

    QVector<QString> nonListedSerialNumbers;

    set<QString> singleMeasureSerialNumbers;
    QMap< QString, set<QString> > singleMeasureControlPoints;

    QMap< QString, set<QString> > duplicateControlPoints;

    set<QString> noLatLonSerialNumbers;
    QMap< QString, set<QString> > noLatLonControlPoints;
    set<QString> latlonchecked;

    QMap< QString, int > cubeMeasureCount;

    // Manage cubes used in NOLATLON
    CubeManager cbman;
    cbman.SetNumOpenCubes(50);

    if (ui.GetBoolean("NOLATLON")) {
      noLatLonCheck(innet, cbman, progress, ignore, num2cube,
          noLatLonSerialNumbers, noLatLonControlPoints);
    }

    // Set calculating progress
    if (innet.GetNumPoints() > 0) {
      progress.SetText("Calculating");
      progress.SetMaximumSteps(innet.GetNumPoints());
      progress.CheckStatus();
    }

    // Loop through all control points in control net
    for (int cp = 0; cp < innet.GetNumPoints(); cp++) {
      if (ignore && innet.GetPoint(cp)->IsIgnored()) continue;
      ControlPoint *controlpt = innet.GetPoint(cp);

      // Checks of the ControlPoint has only 1 Measure
      if (controlpt->GetNumValidMeasures() == 1) {
        QString sn = controlpt->GetMeasure(0)->GetCubeSerialNumber();
        singleMeasureSerialNumbers.insert(sn);
        singleMeasureControlPoints[sn].insert(controlpt->GetId());

        // Records how many times a cube is in the ControlNet
        cubeMeasureCount[sn]++;
      }
      else {
        // Checks for duplicate Measures for the same SerialNumber
        QVector< ControlMeasure * > controlMeasures;
        for (int cm = 0; cm < controlpt->GetNumMeasures(); cm++) {
          ControlMeasure *controlms = controlpt->GetMeasure(cm);

          if (ignore  &&  controlms->IsIgnored()) continue;

          controlMeasures.append(controlms);
          QString currentsn = controlms->GetCubeSerialNumber();

          // Records how many times a cube is in the ControlNet
          cubeMeasureCount[currentsn]++;

          // Removes from the serial number list, cubes that are included in the cnet
          inListNums.erase(currentsn);
          netSerialNumCount[currentsn]++;

          // Records if the currentsnum is not in the input cube list
          bool contains = false;
          for (int sn = 0;
               sn < (int)listedSerialNumbers.size()  &&  !contains;
               sn++) {
            if (currentsn == listedSerialNumbers[sn]) {
              contains = true;
            }
          }
          // Check if already added
          for (int sn = 0;
               sn < (int)nonListedSerialNumbers.size()  &&  !contains;
               sn++) {
            if (currentsn == nonListedSerialNumbers[sn]) {
              contains = true;
            }
          }

          if (!contains) {
            nonListedSerialNumbers.push_back(currentsn);
          }
        }
      }

      progress.CheckStatus();
    }


    // Checks/detects islands
    set<QString> index;
    QMap< QString, set<QString> > adjCubes = constructPointSets(index, innet, ui);
    QVector< set<QString> > islands = findIslands(index, adjCubes);

    // Output islands in the file-by-file format
    //  Islands that have no cubes listed in the input list will
    //  not be shown.
    for (int i = 0; i < (int)islands.size(); i++) {
      QString name(FileName(prefix + "Island." + toString(i + 1)).expanded());
      ofstream out_stream;
      out_stream.open(name.toLatin1().data(), std::ios::out);
      out_stream.seekp(0, std::ios::beg);   //Start writing from beginning of file

      bool hasList = false;
      for (set<QString>::iterator island = islands[i].begin();
           island != islands[i].end();
           island++) {
        if (num2cube.hasSerialNumber(*island)) {
          outputRow(out_stream, buildRow(num2cube, *island));
          hasList = true;
        }
      }

      out_stream.close();

      if (!hasList) {
        remove(name.toLatin1().data());
      }
    }

    // Output the results to screen and files accordingly

    PvlGroup results("Results");

    QString networkName = ui.WasEntered("CNET") ? FileName(ui.GetFileName("CNET")).name() : innet.GetNetworkId();

    stringstream ss(stringstream::in | stringstream::out);

    results.addKeyword(PvlKeyword("Islands", toString((BigInt)islands.size())));
    ss << endl << "----------------------------------------" \
       "----------------------------------------" << endl;
    if (islands.size() == 1) {
      ss << "The cubes are fully connected by the Control Network." << endl;
    }
    else if (islands.size() == 0) {
      ss << "There are no control points in the provided Control Network [";
      ss << networkName << "]" << endl;
    }
    else {
      ss << "The cubes are NOT fully connected by the Control Network." << endl;
      ss << "There are " << islands.size() << " disjoint sets of cubes." << endl;
    }

    if (ui.GetBoolean("SINGLEMEASURE")  &&  singleMeasureSerialNumbers.size() > 0) {
      results.addKeyword(
        PvlKeyword("SingleMeasure", toString((BigInt)singleMeasureSerialNumbers.size())));

      QString name(FileName(prefix + "SinglePointCubes.txt").expanded());
      writeOutput(num2cube, name,
                  singleMeasureSerialNumbers, singleMeasureControlPoints);

      int serials = singleMeasureSerialNumbers.size();
      ss << "----------------------------------------" \
         "----------------------------------------" << endl;
      ss << "There " << ((serials == 1) ? "is " : "are ") << singleMeasureSerialNumbers.size();
      ss << ((serials == 1) ? " cube" : " cubes") << " in Control Points with only a single";
      ss << " Control Measure." << endl;
      ss << "The serial numbers of these measures are listed in [";
      ss <<  FileName(name).name() + "]" << endl;
    }

    if (ui.GetBoolean("NOLATLON")  &&  noLatLonSerialNumbers.size() > 0) {
      results.addKeyword(
        PvlKeyword("NoLatLonCubes", toString((BigInt)noLatLonSerialNumbers.size())));

      QString name(FileName(prefix + "NoLatLon.txt").expanded());
      writeOutput(num2cube, name,
                  noLatLonSerialNumbers, noLatLonControlPoints);

      ss << "----------------------------------------" \
         "----------------------------------------" << endl;
      ss << "There are " << noLatLonSerialNumbers.size();
      ss << " serial numbers in the Control Network which are listed in the";
      ss << " input list and cannot compute latitude and longitudes." << endl;
      ss << "These serial numbers, filenames, and control points are listed in [";
      ss << FileName(name).name() + "]" << endl;
    }

    // Perform Low Coverage check if it was selected
    QString coverageOp = "LowCoverage";
    int failedCoverageCheck = 0;
    if (ui.GetBoolean(QString(coverageOp).toUpper())) {
      QList< QString > netSerials = innet.GetCubeSerials();

      if (netSerials.size() > 0) {
        QString name(FileName(prefix + coverageOp + ".txt").expanded());
        ofstream out_stream;
        out_stream.open(name.toLatin1().data(), std::ios::out);
        out_stream.seekp(0, std::ios::beg); // Start writing from file beginning

        double tolerance = ui.GetDouble("TOLERANCE");
        foreach (QString sn, netSerials) {

          if (num2cube.hasSerialNumber(sn)) {
            // Create a convex hull
            Cube *cube = cbman.OpenCube(num2cube.fileName(sn));
            double controlFitness = getControlFitness(innet, sn, tolerance, cube);

            if (controlFitness < tolerance) {
              outputRow(out_stream, buildRow(num2cube, sn, controlFitness));
              failedCoverageCheck++;
            }
          }
        }

        out_stream.close();

        // Represent the user-specified tolerance as a percentage value
        double tolerancePercent = tolerance * 100.0;
        ss << "----------------------------------------"
          "----------------------------------------" << endl;
        ss << "There are " << failedCoverageCheck << " images in both the "
          "input list and Control Network whose convex hulls cover less than " <<
          tolerancePercent << "\% of the image" << endl;
        ss << "The names of these images, along with the failing convex hull "
          "coverages, are listed in [" << FileName(name).name() << "]" << endl;

        results.addKeyword(
            PvlKeyword(coverageOp, toString((BigInt) failedCoverageCheck)));
      }
    }

    // At this point, inListNums is the list of cubes NOT included in the
    //  ControlNet, and inListNums are their those cube's serial numbers.
    if (ui.GetBoolean("NOCONTROL") && !inListNums.empty()) {
      results.addKeyword(PvlKeyword("NoControl", toString((BigInt)inListNums.size())));

      QString name(FileName(prefix + "NoControl.txt").expanded());
      ofstream out_stream;
      out_stream.open(name.toLatin1().data(), std::ios::out);
      out_stream.seekp(0, std::ios::beg);   //Start writing from beginning of file

      for (set<QString>::iterator sn = inListNums.begin();
           sn != inListNums.end();
           sn++) {
        outputRow(out_stream, buildRow(num2cube, *sn));
      }
      out_stream.close();

      ss << "----------------------------------------" \
         "----------------------------------------" << endl;
      ss << "There are " << inListNums.size();
      ss << " cubes in the input list [" << FileName(ui.GetFileName("FROMLIST")).name();
      ss << "] which do not exist or are ignored in the Control Network [";
      ss << networkName << "]" << endl;
      ss << "These cubes are listed in [" + FileName(name).name() + "]" << endl;
    }

    // In addition, nonListedSerialNumbers should be the SerialNumbers of
    //  ControlMeasures in the ControlNet that do not have a correlating
    //  cube in the input list.
    if (ui.GetBoolean("NOCUBE")  &&  nonListedSerialNumbers.size() > 0) {
      results.addKeyword(
        PvlKeyword("NoCube", toString((BigInt)nonListedSerialNumbers.size())));

      QString name(FileName(prefix + "NoCube.txt").expanded());
      ofstream out_stream;
      out_stream.open(name.toLatin1().data(), std::ios::out);
      out_stream.seekp(0, std::ios::beg);   //Start writing from beginning of file

      for (int sn = 0; sn < (int)nonListedSerialNumbers.size(); sn++) {
        int validMeasureCount = innet.GetValidMeasuresInCube(nonListedSerialNumbers[sn]).size();
        QString rowText = nonListedSerialNumbers[sn] + " (Valid Measures: " +
            toString(validMeasureCount) + ")";
        outputRow(out_stream, rowText);
      }

      out_stream.close();

      ss << "----------------------------------------" \
         "----------------------------------------" << endl;
      ss << "There are " << nonListedSerialNumbers.size();
      ss << " serial numbers in the Control Net [";
      ss << networkName;
      ss << "] \nwhich do not exist in the  input list [";
      ss << FileName(ui.GetFileName("FROMLIST")).name() << "]" << endl;
      ss << "These serial numbers are listed in [";
      ss << FileName(name).name() + "]" << endl;
    }

    // At this point cubeMeasureCount should be equal to the number of
    //  ControlMeasures associated with each serial number.
    if (ui.GetBoolean("SINGLECUBE")) {
      set<QString> singleMeasureCubes;
      for (QMap<QString, int>::iterator cube = cubeMeasureCount.begin();
           cube != cubeMeasureCount.end();
           cube++) {
        if (cube.value() == 1) {
          singleMeasureCubes.insert(cube.key());
        }
      }

      if (singleMeasureCubes.size() > 0) {
        results.addKeyword(
          PvlKeyword("SingleCube", toString((BigInt)singleMeasureCubes.size())));

        QString name(FileName(prefix + "SingleCube.txt").expanded());
        ofstream out_stream;
        out_stream.open(name.toLatin1().data(), std::ios::out);
        out_stream.seekp(0, std::ios::beg);   //Start writing from beginning of file

        for (set<QString>::iterator sn = singleMeasureCubes.begin();
             sn != singleMeasureCubes.end();
             sn++) {
          outputRow(out_stream, buildRow(num2cube, *sn));
        }

        out_stream.close();

        ss << "----------------------------------------" \
           "----------------------------------------" << endl;
        ss << "There are " << singleMeasureCubes.size();
        ss << " serial numbers in the Control Net [";
        ss << networkName;
        ss << "] which only exist in one Control Measure." << endl;
        ss << "These serial numbers are listed in [";
        ss << FileName(name).name() + "]" << endl;
      }
    }

    ss << "----------------------------------------" \
       "----------------------------------------" << endl << endl;
    QString logstr = ss.str().c_str();

    Application::AppendAndLog(results, log);

    if (!ui.IsInteractive()) {
      std::cout << ss.str();
    }

    return logstr;
  }


  // Links cubes to other cubes it shares control points with
  QMap< QString, set<QString> > constructPointSets(set<QString> & index,
      ControlNet innet, UserInterface &ui) {
    QMap< QString, set<QString > > adjPoints;

    bool ignore = ui.GetBoolean("IGNORE");
    for (int cp = 0; cp < innet.GetNumPoints(); cp++) {
      ControlPoint *controlpt = innet.GetPoint(cp);

      if (ignore && controlpt->IsIgnored()) continue;

      if (controlpt->GetNumValidMeasures() < 2) continue;

      // Map SerialNumbers together based on ControlMeasures
      for (int cm1 = 0; cm1 < controlpt->GetNumMeasures(); cm1++) {
        if (ignore && controlpt->IsIgnored()) continue;

        QString sn = controlpt->GetMeasure(cm1)->GetCubeSerialNumber();
        index.insert(sn);
        for (int cm2 = 0; cm2 < controlpt->GetNumMeasures(); cm2++) {
          if (ignore && controlpt->GetMeasure(cm2)->IsIgnored()) continue;

          if (cm1 != cm2) {
            adjPoints[ sn ].insert(controlpt->GetMeasure(cm2)->GetCubeSerialNumber());
          }
        }
      }

    }

    return adjPoints;
  }


  // Uses a depth-first search to construct the islands
  QVector< set<QString> > findIslands(set<QString> & index,
      QMap< QString, set<QString> > adjCubes) {
    QVector< set<QString> > islands;

    while(index.size() != 0) {
      set<QString> connectedSet;

      QStack<QString> str_stack;
      set<QString>::iterator first = index.begin();
      str_stack.push(*first);

      // Depth search
      while(true) {
        index.erase(str_stack.top());
        connectedSet.insert(str_stack.top());

        // Find the first connected unvisited node
        QString nextNode = "";
        set<QString> neighbors = adjCubes[str_stack.top()];
        for (set<QString>::iterator i = neighbors.begin();
             i != neighbors.end();
             i++) {
          if (index.count(*i) == 1) {
            nextNode = *i;
            break;
          }
        }

        if (nextNode != "") {
          // Push the unvisited node
          str_stack.push(nextNode);
        }
        else {
          // Pop the visited node
          str_stack.pop();

          if (str_stack.size() == 0) break;
        }
      }

      islands.push_back(connectedSet);
    }

    return islands;
  }


  // Writes the list of cubes [ SerialNumber, FileName, ControlPoints ] to the output file
  void writeOutput(SerialNumberList num2cube, QString filename,
                   set<QString> sns, QMap< QString, set<QString> > cps) {

    // Set up the output file for writing
    ofstream out_stream;
    out_stream.open(filename.toLatin1().data(), std::ios::out);
    out_stream.seekp(0, std::ios::beg);   //Start writing from beginning of file

    for (set<QString>::iterator sn = sns.begin();
         sn != sns.end();
         sn++) {
      outputRow(out_stream, buildRow(num2cube, *sn, cps[*sn]));
    }

    out_stream.close();
  }


  double getControlFitness(ControlNet &cnet, QString sn, double tolerance, Cube * cube) {
    double controlFitness = 0;

    static  geos::geom::GeometryFactory::Ptr geosFactory = geos::geom::GeometryFactory::create();
    geos::geom::CoordinateSequence pts;
    QList< ControlMeasure * > measures = cnet.GetMeasuresInCube(sn);

    // Populate pts with a list of control points
    foreach (ControlMeasure * measure, measures) {
      pts.add(geos::geom::Coordinate(measure->GetSample(), measure->GetLine()));
    }
    pts.add(geos::geom::Coordinate(measures[0]->GetSample(), measures[0]->GetLine()));

    if (pts.size() >= 4) {

      // Calculate the convex hull
      geos::geom::Geometry * convexHull = geosFactory->createPolygon(
          geosFactory->createLinearRing(pts))->convexHull().release();

      // Calculate the area of the convex hull
      double convexArea = convexHull->getArea();
      double cubeArea = cube->sampleCount() * cube->lineCount();

      controlFitness = convexArea / cubeArea;
    }

    return controlFitness;

  }


  void noLatLonCheck(ControlNet &cnet, CubeManager &manager, Progress &progress,
      bool ignore, SerialNumberList &num2cube,
      set<QString> &noLatLonSerialNumbers,
      QMap< QString, set<QString> > &noLatLonControlPoints) {

    // Set calculating progress
    QList< QString > netSerials = cnet.GetCubeSerials();
    if (netSerials.size() > 0) {
      progress.SetText("Checking for No Lat/Lon");
      progress.SetMaximumSteps(netSerials.size());
      progress.CheckStatus();
    }

    foreach (QString serialNumber, netSerials) {

      if (num2cube.hasSerialNumber(serialNumber)) {
        Cube *cube = manager.OpenCube(num2cube.fileName(serialNumber));

        // Try to create
        Camera *cam = NULL;
        bool createdCamera = true;
        try {
          cam = cube->camera();
        }
        catch (IException &) {
          createdCamera = false;
        }

        QList< ControlMeasure * > measures;
        if (ignore) {
          measures = cnet.GetValidMeasuresInCube(serialNumber);
        } else {
          measures = cnet.GetMeasuresInCube(serialNumber);
        }
        for (int cm = 0; cm < measures.size(); cm++) {
          ControlMeasure *measure = measures[cm];
          ControlPoint *point = measure->Parent();

          if (ignore && point->IsIgnored()) continue;

          // Check the exact measure location
          bool setCamera = false;
          if (createdCamera && measure->GetSample() != Null && measure->GetLine() != Null) {
            setCamera = cam->SetImage(measure->GetSample(), measure->GetLine());
          }

          // Record it if it failed at anything
          if (!createdCamera || !setCamera) {
            noLatLonSerialNumbers.insert(serialNumber);
            noLatLonControlPoints[serialNumber].insert(point->GetId());
          }
        }
      }

      progress.CheckStatus();
    }
  }


  QString buildRow(SerialNumberList &serials, QString sn) {
    QString cubeName = serials.hasSerialNumber(sn) ?
        FileName(serials.fileName(sn)).expanded() : "UnknownFilename";
    return cubeName + g_delimiter + sn;
  }


  QString buildRow(SerialNumberList &serials, QString sn, set<QString> &cps) {
    QString rowText = buildRow(serials, sn);

    // Control Points where the cube was found to have the issue
    for (set<QString>::iterator cp = cps.begin(); cp != cps.end(); cp++) {
      rowText += g_delimiter + *cp;
    }

    return rowText;
  }


  QString buildRow(SerialNumberList &serials, QString sn, double value) {
    return buildRow(serials, sn) + g_delimiter + toString(value);
  }


  void outputRow(ofstream &outStream, QString rowText) {
    outStream << rowText << "\n";
  }
}
