/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <map>

#include "Application.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "FileList.h"
#include "IException.h"
#include "IString.h"
#include "Latitude.h"
#include "Longitude.h"
#include "Preference.h"
#include "Progress.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlObject.h"
#include "SerialNumberList.h"
#include "SpecialPixel.h"
#include "SurfacePoint.h"
#include "TextFile.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;

std::map <int, QString> snMap;
//std::map <string,int> fscMap;

void IsisMain() {
  // The following steps can take a significant amount of time, so
  // set up a progress object, incrementing at 1%, to keep the user informed
  PvlGroup &uip = Preference::Preferences().findGroup("UserInterface");
  uip["ProgressBarPercent"] = "1";
  UserInterface &ui = Application::GetUserInterface();
  Progress progress;

  // Prepare the ISIS2 list of file names
  FileList list2(ui.GetFileName("LIST2").toStdString());

  // Prepare the ISIS SNs, pass the progress object to SerialNumberList
  SerialNumberList snl(ui.GetFileName("LIST3"), true, &progress);
  progress.CheckStatus();

  if (list2.size() != snl.size()) {
    std::string msg = "Invalid input file number of lines. The ISIS2 file list [";
    msg += ui.GetAsString("LIST2").toStdString() + "] must contain the same number of lines ";
    msg += "as the ISIS file list [" + ui.GetAsString("LIST3").toStdString() + "]";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  progress.SetText("Mapping ISIS2 fsc numbers to ISIS serial numbers.");
  progress.SetMaximumSteps(list2.size());
  // Setup a map between ISIS2 image number (fsc) and ISIS sn
  // **NOTE:
  //   The order of the ISIS2 and ISIS lists MUST correspond so that we can map
  //   each ISIS2 FSC to the proper ISIS Serial Number.
  //   Otherwise, we would be required to write a separate routine for each
  //   mission to determine the corresponding serial number for a given FSC.
  //   Jeannie Backer 2011-06-30
  for (int f = 0; f < list2.size(); f++) {
    progress.CheckStatus();
    std::string currFile(list2[f].toString());
    Pvl lab(currFile);
    PvlObject qube(lab.findObject("QUBE"));

    IString fsc;
    if (qube.hasKeyword("IMAGE_NUMBER")) {
      fsc = qube.findKeyword("IMAGE_NUMBER")[0];
    }
    else if (qube.hasKeyword("IMAGE_ID")) {
      fsc = qube.findKeyword("IMAGE_ID")[0];
    }
    else {
      throw IException(IException::Unknown,
          "Can not find required keyword IMAGE_NUMBER or IMAGE_ID "
          "in [" + currFile + "]",
          _FILEINFO_);
    }

    QString sn(snl.serialNumber(f));
    snMap.insert(std::pair<int, QString>((int)fsc, sn));
  }
  progress.CheckStatus();

  // Create a new control network
  ControlNet cnet;
  cnet.SetNetworkId(ui.GetString("NETWORKID"));

  // first try to set target from user entered TargetName
  cnet.SetTarget(ui.GetString("TARGET"));
  cnet.SetUserName(Application::UserName());
  cnet.SetCreatedDate(Application::DateTime());
  cnet.SetDescription(ui.GetString("DESCRIPTION"));

  // Open the match point file
  TextFile mpFile(ui.GetFileName("MATCH"));
  QString currLine;
  int inTotalMeas = 0;

  // Read the first line with the number of measurments
  mpFile.GetLine(currLine);
  currLine = currLine.simplified();
  currLine.remove(0, currLine.indexOf("="));
  currLine.remove(0, currLine.indexOf(" "));
  try {
    inTotalMeas = currLine.toInt();
  }
  catch (IException &e) {
    throw IException(e,
                       IException::User, "Invalid match point file "
                       "header for [" + ui.GetAsString("MATCH").toStdString()
                       + "]. First line does not contain number of "
                       "measurements.",
                     _FILEINFO_);
  }

  // Read line 2, the column header line
  mpFile.GetLine(currLine);
  currLine = currLine.simplified();
  QStringList tokens = currLine.split(" ", Qt::SkipEmptyParts);
  while (!tokens.isEmpty()) {
    QString label = tokens.takeFirst();
    // this line should contain only text labels,
    double error = 0;
    try {
      error = (label).toDouble();
      // if we are able to convert label to a double, we have an error
      throw IException(IException::User, "Invalid match point file "
                         "header for [" + ui.GetAsString("MATCH").toStdString()
                         + "]. Second line does not contain proper "
                         "non-numerical column labels.",
                       _FILEINFO_);
    }
    catch (IException &e) {
      // if this line does not contain a double, continue
      if (error == 0) {
        continue;
      }
      // if this line contains numeric data, throw an error
      else {
        throw;
      }
    }
  }

  // Reset the progress object for feedback about conversion processing
  progress.SetText("Converting match point file");
  progress.SetMaximumSteps(inTotalMeas);

  int line = 2;
  while (mpFile.GetLine(currLine)) {
    line ++;

    // Update the Progress object
    try {
      progress.CheckStatus();
    }
    catch (IException &e) {
      std::string msg = "\"Matchpoint total\" keyword at the top of the match point "
                   "file [";
      msg += ui.GetAsString("MATCH").toStdString() + "] equals [" + toString(inTotalMeas);
      msg += "] and is likely incorrect. Number of measures in match point file"
             " exceeds this value at line [";
      msg += toString(line) + "].";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }


    // Declare the Point and Measure
    ControlPoint *cpoint = NULL;
    ControlMeasure *cmeasure = new ControlMeasure;

    // Section the match point line into the important pieces
    currLine = currLine.simplified();
    QString pid = "";
    QString fsc = "";
    double lineNum, sampNum, diam;
    QString matClass = "";
    try {
      QStringList tokens = currLine.split(" ");

      if (tokens.count() < 6)
        throw IException();

      pid = tokens.takeFirst();                // ID of the point
      fsc = tokens.takeFirst();                // FSC of the ISIS2 cube
      lineNum = (tokens.takeFirst()).toDouble();  // line number
      sampNum = (tokens.takeFirst()).toDouble();  // sample number
      matClass = tokens.takeFirst();           // Match Point Class
      diam = (tokens.takeFirst()).toDouble();     // Diameter, in case of a crater
    }
    catch (IException &e) {
      std::string msg = "Invalid value(s) in match point file [";
      msg += ui.GetAsString("MATCH").toStdString() + "] at line [" + toString(line);
      msg += "]. Verify line, sample, diameter values are doubles.";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }

    // Set the coordinate and serial number for this measure
    cmeasure->SetCoordinate(sampNum, lineNum);
    cmeasure->SetCubeSerialNumber(snMap[fsc.toInt()]);

    if (snMap[fsc.toInt()].isEmpty()) {
      std::string msg = "None of the images specified in the ISIS2 file list [";
      msg += ui.GetAsString("LIST2").toStdString();
      msg += "] have an IMAGE_NUMBER or IMAGE_ID that matches the FSC [" + fsc.toStdString();
      msg += "], from the match point file [" + ui.GetAsString("MATCH").toStdString();
      msg += "] at line [" + toString(line) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    bool isReferenceMeasure = false;

    //Set the Measure Type
    if (matClass.toUpper() == "U") {//Unmeasured -these are ignored in isis2
      cmeasure->SetType(ControlMeasure::Candidate);
      cmeasure->SetIgnored(true);
    }
    else if (matClass.toUpper() == "T") {
      // Truth type, aka reference measure, is no longer a measure type
      // what this means is it has to be handled by the control point.
      // So, further down the boolean set here will be used.
      isReferenceMeasure = true;
    }
    else if (matClass.toUpper() == "S") { //SubPixel
      cmeasure->SetType(ControlMeasure::RegisteredSubPixel);
    }
    else if (matClass.toUpper() == "M") { //Measured
      cmeasure->SetType(ControlMeasure::RegisteredPixel);
    }
    else if (matClass.toUpper() == "A") { //Approximate
      cmeasure->SetType(ControlMeasure::Candidate);
    }
    else {
      std::string msg = "Unknown measurment type [" + matClass.toStdString() + "] ";
      msg += "in match point file [" + ui.GetAsString("MATCH").toStdString() + "] ";
      msg += "at line [" + toString(line) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    //Set Diameter
    try {
      //Check to see if the column was, in fact, a double
      if ((double)IString(diam) != 0.0)
        cmeasure->SetDiameter(diam);
    }
    catch (IException &) {
      //If we get here, diam was not a double,
      // but the error is not important otherwise
    }

    // Check whether we should lock all measures
    cmeasure->SetEditLock(ui.GetBoolean("MEASURELOCK"));

    //Find the point that matches the PointID, create point if it does not exist

    if (cnet.ContainsPoint(pid)) {
      cpoint = cnet.GetPoint((QString) pid);
    }
    else {
      cpoint = new ControlPoint(pid);
      cnet.AddPoint(cpoint);
    }

    //Add the measure
    try {
      cpoint->Add(cmeasure);
      // Equivalent to (IString::Equal(matClass, "T")), as seen above
      if (isReferenceMeasure) {
        cpoint->SetRefMeasure(cmeasure);
      }
    }
    catch (IException &e) {
      std::string msg = "Invalid match point file [" + ui.GetAsString("MATCH").toStdString() +"]";
      msg += ".  Repeated PointID/FSC combination [" + pid.toStdString() + ", " + fsc.toStdString();
      msg += "] in match point file at line [" + toString(line) + "].";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }
  }

  // Update the Progress object
  try {
    progress.CheckStatus();
  }
  catch (IException &e) {
    std::string msg = "\"Matchpoint total\" keyword at the top of the match point "
                 "file [";
    msg += ui.GetAsString("MATCH").toStdString() + "] equals [" + toString(inTotalMeas);
    msg += "] and is likely incorrect. Number of measures in match point file "
           "exceeds this value at line [";
    msg += toString(line) + "].";
    throw IException(e, IException::User, msg, _FILEINFO_);
  }


  // 10/5/2009 Jeannie Walldren - Added RAND PPP file as input
  // 7/12/2011 Jeannie Backer - Added option to lock all points in RAND PPP file

  // Open the RAND PPP file
  if (ui.GetBoolean("INPUTPPP")) {
    int numRandOnly = 0;
    vector<QString> randOnlyIDs;
    TextFile randFile(ui.GetFileName("PPP"));
    progress.SetText("Converting RAND PPP file");
    randFile.GetLine(currLine);
    int inTotalLine = (int)(randFile.Size() / (currLine.size()));
    progress.SetMaximumSteps(inTotalLine);
    randFile.Rewind();

    line = 0;
    while (randFile.GetLine(currLine)) {
      line ++;

      // Update the Progress object
      try {
        progress.CheckStatus();
      }
      catch (IException &e) {
        std::string msg = "RAND PPP file may not be valid. Line count calculated [";
        msg += toString(inTotalLine) + "] for RAND PPP file [";
        msg += ui.GetAsString("PPP").toStdString() + "] appears invalid at line [";
        msg += toString(line) + "].";
        throw IException(e, IException::Programmer, msg, _FILEINFO_);
      }

      // Declare the Point
      ControlPoint *cpoint = NULL;

      // if end of valid data, break, stop processing
      if (currLine.contains("JULIAN")) {
        // Since Progress MaximumSteps was approximated using the number of
        // lines in the RAND PPP file, we need to subtract the number of lines
        // left from the Progress steps since the following lines are not going
        // to be processed
        progress.AddSteps(line - inTotalLine); // line < inTotalLine,
                                               //so this is negative
        break;
      }

      // break columns into substrings since some files have colunms that can't
      // be tokenized easily. This is because some files have columns running
      // into each other without spaces separating them

      // column 1 = latitude, begins the line and is 24 characters
      double lat;
      QString col1 = currLine.mid(0, 24);
      // remove any white space from beginning of string
      //col1.ConvertWhiteSpace();
      //col1.TrimHead(" ");
      try {
        // convert to double
        lat = (col1).toDouble();
      }
      catch (IException &e) {
        std::string msg = "Invalid value(s) in RAND PPP file [";
        msg += ui.GetAsString("PPP").toStdString() + "] at line [" + toString(line);
        msg += "]. Verify latitude value is a double.";
        throw IException(e, IException::User, msg, _FILEINFO_);
      }

      // column 2 = longitude, begins at 25th char and is 24 characters
      double lon;
      QString col2 = currLine.mid(24, 24);
      // remove any white space from beginning of string
      //col2.TrimHead(" ");
      try {
        // convert to double
        lon = col2.toDouble();
      }
      catch (IException &e) {
        std::string msg = "Invalid value(s) in RAND PPP file [";
        msg += ui.GetAsString("PPP").toStdString() + "] at line [" + toString(line);
        msg += "]. Verify longitude value is a double.";
        throw IException(e, IException::User, msg, _FILEINFO_);
      }

      // column 3 = radius, begins at 49th char and is 24 characters
      double rad;
      QString col3 = currLine.mid(48, 24);
      // remove any white space from beginning of string
      //col3.TrimHead(" ");
      try {
        // convert to double and convert km to meters
        rad = col3.toDouble();
        rad = rad * 1000;
      }
      catch (IException &e) {
        std::string msg = "Invalid value(s) in RAND PPP file [";
        msg += ui.GetAsString("PPP").toStdString() + "] at line [" + toString(line);
        msg += "]. Verify radius value is a double.";
        throw IException(e, IException::User, msg, _FILEINFO_);
      }

      // column 4 = point id, begins at 73rd char and is 7 characters
      QString pid = currLine.mid(72);
      // remove any white space from beginning of string
      pid = pid.remove(QRegExp("^ *"));
      if (pid.length() > 7) {
        std::string msg = "Invalid value(s) in RAND PPP file [";
        msg += ui.GetAsString("PPP").toStdString() + "] at line [" + toString(line);
        msg += "]. Point ID [" + pid.toStdString() + "] has more than 7 characters.";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      //Find the point that matches the PointID, if it does not exist alert the
      //user
      if (!cnet.ContainsPoint(pid)) {
        numRandOnly++;
        randOnlyIDs.push_back(currLine);
      }
      else {
        cpoint = cnet.GetPoint((QString) pid);
        // if the point is already added to the control net, it was found in
        // the match point file also.
        if (ui.GetString("POINTTYPE") == "FIXED") {
          // If the POINTTYPE parameter is set to fixed, change point type
          // of points in rand file
          cpoint->SetType(ControlPoint::Fixed);
        }
        cpoint->SetAprioriSurfacePointSource(
          ControlPoint::SurfacePointSource::BundleSolution);
        cpoint->SetAprioriSurfacePointSourceFile(ui.GetAsString("PPP"));
        cpoint->SetAprioriRadiusSource(
          ControlPoint::RadiusSource::BundleSolution);
        cpoint->SetAprioriRadiusSourceFile(ui.GetAsString("PPP"));
      }

      if (cpoint != NULL) {
        //Add the lat,lon,rad to point
        try {
          SurfacePoint surfacePt(Latitude(lat, Angle::Degrees),
              Longitude(lon, Angle::Degrees),
              Distance(rad, Distance::Meters));
          cpoint->SetAprioriSurfacePoint(surfacePt);
          cpoint->SetEditLock(ui.GetBoolean("POINTLOCK"));
        }
        catch (IException &e) {
          std::string msg = "Unable to set universal ground point to control "
                        "network from line [";
          msg += toString(line) + "] of RAND PPP file [";
          msg += ui.GetAsString("PPP").toStdString() + "]";
          throw IException(e, IException::User, msg, _FILEINFO_);
        }
      }
    }

    // Update the Progress object
    try {
      progress.CheckStatus();
    }
    catch (IException &e) {
      std::string msg = "RAND PPP file may not be valid.  Line count calculated [";
      msg += toString(inTotalLine) + "] for RAND PPP file [";
      msg += ui.GetAsString("PPP").toStdString();
      msg += "] appears invalid at line [" + toString(line) + "].";
      throw IException(e, IException::Programmer, msg, _FILEINFO_);
    }

    // Write results to Logs
    // Summary group is created with the counts of RAND PPP only points
    PvlGroup summaryGroup = PvlGroup("Summary");
    summaryGroup.addKeyword(PvlKeyword("RandOnlyPoints", Isis::toString(numRandOnly)));

    bool log;
    FileName logFile;
    // if a filename was entered, use it to create the log
    if (ui.WasEntered("LOG")) {
      log = true;
      logFile = ui.GetFileName("LOG").toStdString();
    }
    // if no filename was entered, but there were some RAND PPP only points,
    // create an log named "pppOnlyPoints" in the current directory
    else if (numRandOnly > 0) {
      log = true;
      logFile = "pppOnlyPoints.log";
    }
    // if all RAND PPP points are found in the MATCH file and no log file is
    // named, only output summary to application log
    else {
      log = false;
    }

    if (log) {
      // Write details in error log
      Pvl results;
      results.setName("Results");
      results.addGroup(summaryGroup);
      if (numRandOnly > 0) {
        // if there are any RAND PPP only points,
        // add comment to the summary log to alert user
        summaryGroup.addComment("Some Point IDs in the RAND PPP file have no "
                                "measures in the MATCH file.");
        summaryGroup.addComment("These Point IDs are contained "
                                "in [" + logFile.name() + "].");
        TextFile outlog(QString::fromStdString(logFile.expanded()), "overwrite", randOnlyIDs);
      }
      else {
        // if there are no RAND PPP only points and user wanted to create a log,
        // add comment to the summary log to alert user
        summaryGroup.addComment("All Point IDs in the RAND PPP file have "
                                "measures in the MATCH file.");
        summaryGroup.addComment("No RAND PPP log was created.");
      }
    }
    // Write summary to application log
    Application::Log(summaryGroup);

  }
  // Write the control network out
  cnet.Write(ui.GetFileName("ONET"));


}
