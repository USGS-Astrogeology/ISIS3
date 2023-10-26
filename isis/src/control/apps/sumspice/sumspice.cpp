/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


// $Id: sumspice.cpp 6565 2016-02-11 00:15:35Z kbecker@GS.DOI.NET $

#include <QDir>
#include <QFile>
#include <QScopedPointer>
#include <QString>
#include <QtAlgorithms>
#include <QTextStream>

// boost library
#include <boost/foreach.hpp>

// The old original
#include "Application.h"
#include "Camera.h"
#include "CameraFactory.h"
#include "FileList.h"
#include "FileName.h"
#include "IException.h"
#include "iTime.h"
#include "IString.h"
#include "Kernels.h"
#include "NaifStatus.h"
#include "Process.h"
#include "Progress.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "SpecialPixel.h"
#include "SumFile.h"
#include "SumFinder.h"
#include "Target.h"

using namespace std;
namespace Isis {

  inline QString format(const double &d, const int &precision,
                        const QString &defValue = "NULL") {
    if ( IsSpecial(d) ) {
      return ( defValue );
    }
    return (toString(d, precision));
  }


  void sumspice (UserInterface &ui, Pvl *log) {

    //  typedef SumFinder::Options Options;
    typedef SumFinder::TimeStamp TimeStamp;

    //  Program constants
    const QString sumspice_program = "sumspice";
    const QString sumspice_version = "2.0";
    const QString sumspice_revision = "$Revision: 6565 $";
    const QString sumspice_runtime = Application::DateTime();

    //  Get the list of input cubes to be processed
    FileList cubeNameList;
    if ( ui.WasEntered("FROM") ) {
      cubeNameList.append(ui.GetCubeName("FROM"));
    }
    else if ( ui.WasEntered("FROMLIST") ) {
      cubeNameList.read(ui.GetFileName("FROMLIST"));
    }
    else {
      QString message = "User must provide either an input cube file or an input cube file list.";
      throw IException(IException::User, message, _FILEINFO_);
    }

    // get the list of possible sum files to be applied
    FileList sumFileNameList;
    if ( ui.WasEntered("SUMFILE") )  {
      sumFileNameList.append(ui.GetFileName("SUMFILE"));
    }
    else if ( ui.WasEntered("SUMFILELIST") )  {
      sumFileNameList.read(ui.GetFileName("SUMFILELIST"));
    }
    else {
      QString message = "User must provide either a sum file or a sum file list.";
      throw IException(IException::User, message, _FILEINFO_);
    }

    // Get the time as represented in the SUMFILE
    QString sumtime = ui.GetString("SUMTIME").toLower();
    TimeStamp tstamp = ( "start"  == sumtime ) ? SumFinder::Start :
                       ( "center" == sumtime)  ? SumFinder::Center :
                                                 SumFinder::Stop;

    // Load any meta kernels if provided by user
    Kernels meta;
    if ( ui.WasEntered("METAKERNEL") ) {
      QString metafile = ui.GetFileName("METAKERNEL");
      meta.Add(metafile);
      meta.Load();
    }

    // Load sumfiles
    SumFileList sumFiles = loadSumFiles(sumFileNameList);

    // Sort the sum file list in ascending order by ET
    std::sort(sumFiles.begin(), sumFiles.end(), SortEtAscending());

    // check for uniqueness of sum files
    PvlGroup duplicates("SumFileWarnings");
    duplicates.addComment("First file will be used to update cube.");
    for (int sumIndex = 1; sumIndex < sumFiles.size(); sumIndex++) {
      double tdiff = fabs(sumFiles[sumIndex]->et() - sumFiles[sumIndex-1]->et());
      if ( qFuzzyCompare( tdiff+1.0, 0.0+1.0) ) {
        PvlKeyword filePair("SumFilesWithDuplicateTimes", sumFiles[sumIndex-1]->name().toStdString());
        filePair.addValue(sumFiles[sumIndex]->name().toStdString());
        duplicates += filePair;
      }
    }

    Application::Log(duplicates);

    // Determine the update mode
    QString update = ui.GetString("UPDATE").toLower();
    unsigned int options = 0;  // == SumFinder::None
    if ( "times"    == update ) options |= (unsigned int) SumFinder::Times;
    if ( "spice"    == update ) options |= (unsigned int) SumFinder::Spice;
    // These are unnecessary if UPDATE=SPICE!
    if ( "pointing" == update ) options |= (unsigned int) SumFinder::Pointing;
    if ( "position" == update ) options |= (unsigned int) SumFinder::Position;
    if ( "reset"    == update ) options |= (unsigned int) SumFinder::Reset;


    // Determine observation time tolerances. Default is to find the closest one
    double tolerance = DBL_MAX;
    if ( ui.WasEntered("TIMEDIFF") ) {
      tolerance = ui.GetDouble("TIMEDIFF");
    }

    // loop through the input cubes
    Progress progress;
    progress.SetText("Updating " + update + "...");
    progress.SetMaximumSteps(cubeNameList.size());
    progress.CheckStatus();

    // Accumulate the results of the processing...
    typedef QSharedPointer<SumFinder>  SharedFinder;
    typedef QList<SharedFinder>        ListOfFinders;

    ListOfFinders resultSet;
    QStringList warnings;
    Process process;

    for (int cubeIndex = 0; cubeIndex < cubeNameList.size(); cubeIndex++) {

      // Find the proper SUMFILE for the cube
      QString filename(cubeNameList[cubeIndex].expanded());
      SharedFinder cubesum( new SumFinder(filename, sumFiles, tolerance, tstamp) );

      // Format a warning and save it off for later
      if ( !cubesum->isFound() ) {
        QString mess = "No SUMFILE found for " + cubesum->name() +
                        " - closest time: " +
                        Isis::toString(cubesum->closest(), 10) +
                        " <seconds>";
        warnings <<  mess;
      }
      else {
        if ( !cubesum->update(options) ) {
          QString msg = "Failed to apply SUMFILE updates on cube " + filename;
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }

      // This will update the history blob and close the cube,
      // but retain all the pertinent info
      cubesum->resetCube();
      resultSet.append(cubesum);

      Isis::CubeAttributeInput att(filename);
      Cube *cube = process.SetInputCube(filename, att, Isis::ReadWrite);
      process.WriteHistory(*cube);

      progress.CheckStatus();
    }
    process.EndProcess();

    if (warnings.size() > 0) {
      PvlKeyword message("Unmatched");
      BOOST_FOREACH ( QString mess, warnings ) {
        message.addValue(mess.toStdString());
      }
      PvlGroup loggrp("Warnings");
      loggrp.addKeyword(message);
      Application::Log(loggrp);
    }


    // Log the results of processing
    if ( ui.WasEntered("TOLOG") ) {
      FileName filename( ui.GetFileName("TOLOG") );
      bool exists = filename.fileExists();
      QFile logfile(filename.expanded());
      if ( !logfile.open(QIODevice::WriteOnly | QIODevice::Append |
                         QIODevice::Text | QIODevice::Unbuffered) ) {
        QString mess = "Unable to open/create log file " + filename.name();
        throw IException(IException::User, mess, _FILEINFO_);
      }

      QTextStream lout(&logfile);
      if ( !exists) {
        lout << "Filename,SUMFILE,SumTime,Update,CubeSumDeltaTime, "
             << "ExposureTime,CubeStartTime,CubeCenterTime,CubeStopTime,"
             << "SumStartTime,SumCenterTime,SumStopTime\n";
      }

      BOOST_FOREACH (SharedFinder &cubesum, resultSet ) {
        lout << cubesum->name()<< ",";

        if ( !cubesum->isFound() ) {
          lout << "NULL," << sumtime << "," << update << ","
               << format(cubesum->closest(), 7) << ","
               << format(cubesum->exposureTime(), 7) << ","
               << cubesum->cubeStartTime().UTC() << ","
               << cubesum->cubeCenterTime().UTC() << ","
               << cubesum->cubeStopTime().UTC() << ","
               << "NULL,NULL,NULL";
        }
        else {
          lout << cubesum->sumfile()->name() << ","
               << sumtime << "," << update << ","
               << format(cubesum->deltaT(), 7) << ","
               << format(cubesum->exposureTime(), 7) << ","
               << cubesum->cubeStartTime().UTC() << ","
               << cubesum->cubeCenterTime().UTC() << ","
               << cubesum->cubeStopTime().UTC() << ","
               << iTime(cubesum->sumStartTime()).UTC() << ","
               << iTime(cubesum->sumCenterTime()).UTC() << ","
               << iTime(cubesum->sumStopTime()).UTC();
        }

        lout << "\n";
      }

    }

    // Unload meta kernels - automatic, but done for completeness
    meta.UnLoad();
  }
}
