/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>
#include <ctime>
/*#define USE_GUI_QAPP 1*/

#include <QApplication>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QFile>
#include <QTextStream>
#include <QTime>

// OpenCV stuff
#include "opencv2/core.hpp"

// boost library
#include <boost/foreach.hpp>

#define HAVE_ISNAN

#include "Application.h"
#include "Camera.h"
#include "ControlMeasure.h"
#include "ControlMeasureLogData.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "Cube.h"
#include "Distance.h"
#include "Environment.h"

#include "FastGeom.h"
#include "FeatureAlgorithmFactory.h"
#include "FileList.h"
#include "GenericTransform.h"
#include "ID.h"
#include "ImageSource.h"
#include "ImageTransform.h"
#include "ScalingTransform.h"

#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "Latitude.h"
#include "Longitude.h"
#include "MatcherSolution.h"
#include "MatchImage.h"
#include "MatchMaker.h"

#include "Process.h"
#include "Progress.h"
#include "PvlFlatMap.h"
#include "PvlGroup.h"
#include "QDebugLogger.h"
#include "RobustMatcher.h"
#include "SerialNumber.h"
#include "ScharrTransform.h"
#include "SobelTransform.h"
#include "Statistics.h"
#include "SurfacePoint.h"
#include "TextFile.h"
#include "UserInterface.h"
#include "Pvl.h"

using namespace std;
namespace Isis {

  inline PvlGroup pvlmap_to_group( const PvlFlatMap &pvlmap, const QString &grpnam ) {
    PvlGroup pgrp( grpnam );
    for ( auto pkey : pvlmap.values() ) {
      pgrp.addKeyword( pkey );
    }
    return ( pgrp );
  }


  static void writeInfo(const QString &toname, Pvl &data, UserInterface &ui, Pvl *log) {
      if ( !toname.isEmpty() ) {
        FileName toinfo(toname);
        QString fname = toinfo.expanded();
        data.write(fname);
      }
      else {
        if ( !ui.IsInteractive()  ) {
          std::cout << data << "\n";
        }
        else {
          if (log){
            log->addObject(data);
          }
        }
      }
      return;
  }

  /**
   * @brief Loads train images and applies geometry if provided
   *
   * This function loads a trainer image and optionally applies geometry using
   * a FastGeom transform.
   *
   * A MatchImage is added to the MatchMaker as a trainer image. YOU SHOULD NOT
   * USE THIS FUNCTION TO SET THE QUERY IMAGE. Once loaded, a FastGeom is
   * optionally applied to the image before adding it to the matcher.
   *
   * Errors are trapped here if the file cannot be loaded or the geometry proess
   * in FastGeom fails. In this case, false is returned without adding the image
   * to the matcher.
   *
   * @author 2021-10-29 Kris J. Becker
   * @history 2022-02-08 Kris J. Becker Clarified parameter description and added
   *                       explanation of return function return condition; modified
   *                       return logic
   *
   * @param matcher    The source reponsible for managing the matcher process
   * @param trainfile  File name to load
   * @param fastgeom   Apply FastGeom if provided
   *
   * @return bool      Any errors encountered will return false without adding
   *                      the train image to the matcher
   */
  static bool load_train_with_geom(MatchMaker &matcher, const QString trainfile,
                                  QDebugStream &logger,
                                  const FastGeom *fastgeom = 0) {

    try {
      MatchImage t_image = MatchImage( ImageSource(trainfile) );

      // Compute a FastGeom transform if requested
      if ( fastgeom ) {
        // This will typically throw an exception for bad geometry.
        //  Will return false with no addition to matcher.

        // Note if FastGeom is successful, it adds a transform on
        // the trainer that computes geometric relationships with
        // the query image.
        fastgeom->apply(matcher.query(), t_image, QLogger(logger));
      }

      // If alls good...expection should be thrown above otherwise
      matcher.addTrainImage( t_image );
    }
    catch ( IException &ie ) {
      logger->dbugout() << "Failed to load " << trainfile << "\n\n";
      return ( false );
    }

    return ( true );
  }

  void findfeatures(UserInterface &ui, Pvl *log) {

    //  Program constants
    const QString findfeatures_program = "findfeatures";
    const QString findfeatures_version = "1.2";
    const QString findfeatures_revision = "2023-06-21";  // Now is the date of revision!
    // const QString findfeatures_runtime = Application::DateTime();

    // Track runtime...
    QTime runTime = QTime::currentTime();

    // Get time for findfeatures_runtime
    time_t startTime = time(NULL);
    struct tm *tmbuf = localtime(&startTime);
    char timestr[80];
    strftime(timestr, 80, "%Y-%m-%dT%H:%M:%S", tmbuf);
    const QString findfeatures_runtime = (QString) timestr;

    QString toinfo;
    if ( ui.WasEntered("TOINFO")) {
      toinfo = ui.GetAsString("TOINFO");
    }

    // Set up program debugging and logging
    QDebugStream logger( QDebugLogger::null() );
    bool p_debug( ui.GetBoolean("DEBUG") );
    if ( p_debug ) {
      // User specified a file by name
      if ( ui.WasEntered("DEBUGLOG") ) {
        logger = QDebugLogger::create( ui.GetAsString("DEBUGLOG") );
      }
      else {
        // User wants debugging but entered no file so give'em std::cout.
        logger = QDebugLogger::toStdOut();
      }
    }

    // Write out program logger information
    logger->dbugout() << "\n\n---------------------------------------------------\n";
    logger->dbugout() << "Program:        " << findfeatures_program << "\n";
    logger->dbugout() << "Version         " << findfeatures_version << "\n";
    logger->dbugout() << "Revision:       " << findfeatures_revision << "\n";
    logger->dbugout() << "RunTime:        " << findfeatures_runtime << "\n";
    logger->dbugout() << "OpenCV_Version: " << CV_VERSION << "\n";
    logger->flush();

    // Set up for info requests
    FeatureAlgorithmFactory *factory = FeatureAlgorithmFactory::getInstance();
    if ( ui.GetBoolean("LISTALL") ) {
      Pvl info;
      QStringList algorithms = factory->getListAll();
      // cout << algorithms.join("\n") << "\n";
      info.addObject( factory->info(algorithms) );
      writeInfo(toinfo, info, ui, log);
      return;
    }


    // Get parameters from user
    PvlFlatMap parameters;
    // Check for parameters file provided by user
    if ( ui.WasEntered("PARAMETERS") ) {
      QString pfilename = ui.GetAsString("PARAMETERS");
      Pvl pfile(pfilename);
      PvlFlatMap parms =  PvlFlatMap(pfile);
      parameters.merge( parms );
      parameters.add("ParameterFile", pfilename);

      // Log parameters loaded from the PARAMETERS file
      if ( log ) {
        auto parmgrp = pvlmap_to_group( parms, "Parameters");
        log->addLogGroup( parmgrp );
      }
    }

    // Get individual parameters if provided
    QStringList parmlist;
    parmlist << "Ratio" << "EpiTolerance" << "EpiConfidence" << "HmgTolerance"
             << "MaxPoints" << "FastGeom" << "FastGeomPoints" << "GeomType"
             << "GeomSource" << "Filter";
    BOOST_FOREACH (QString p, parmlist ) {
      parameters.add(p, ui.GetAsString(p));
    }

    // Got all parameters.  Add them now and they don't need to be considered
    // from here on.  Parameters specified in input algorithm specs take
    // precedence (in MatchMaker)
    factory->setGlobalParameters(parameters);

    // Retrieve the ALGORITHM specification (if requested)
    QString aspec;
    if ( ui.WasEntered("ALGORITHM") ) {
      aspec = ui.GetString("ALGORITHM");
    }

    // Now check for file containing algorithms
    if ( ui.WasEntered("ALGOSPECFILE") ) {
      QString specfile = ui.GetAsString("ALGOSPECFILE");
      TextFile sFile(specfile);
      sFile.OpenChk(true);
      QString algorithm;
      QStringList algos;
      while ( sFile.GetLine(algorithm, true) ) {
        algos.append(algorithm);
      }

      if ( !aspec.isEmpty() ) { aspec.append("|");  }
      aspec.append( algos.join("|") );
    }

    // Now reset any global parameters provided by the user
    if ( ui.WasEntered("GLOBALS") ) {
      QString gblparms = ui.GetString("GLOBALS");
      PvlFlatMap globals = factory->parseGlobalParameters(gblparms);
      factory->addGlobalParameters( globals );
      factory->addParameter("GLOBALS", gblparms);

        // Load values parsed from the GLOBALS string
       if ( log ) {
        auto globalgrp = pvlmap_to_group( globals, "Globals");
        log->addLogGroup( globalgrp );
      }
    }

    // Now report the list of all global parameters in the pool
    if ( log ) {
      auto gpool = pvlmap_to_group( factory->globalParameters(), "GlobalParameterPool");
      log->addLogGroup( gpool );
    }

    // Create a list of algorithm specifications from user specs and log it
    // if requested
    RobustMatcherList algorithms = factory->create(aspec);
    if ( ui.GetBoolean("LISTSPEC") ) {

      Pvl info;
      info.addObject(factory->info(algorithms));
      writeInfo(toinfo, info, ui, log);

      // If no input files are provided exit here
      if ( !( ui.WasEntered("MATCH") && ( ui.WasEntered("FROM") ||
                                          ui.WasEntered("FROMLIST") ) ) ) {
        return;
      }
    }

    // First see what we can do about threads if your user is resource conscience
    int nCPUs = cv::getNumberOfCPUs();
    int nthreads = cv::getNumThreads();
    logger->dbugout() << "\nSystem Environment...\n";
    logger->dbugout() << "Number available CPUs:     " << nCPUs << "\n";
    logger->dbugout() << "Number default threads:    " << nthreads << "\n";

    // See if user wants to restrict the number of threads used
    int uthreads = nthreads;
    if ( ui.WasEntered("MAXTHREADS") ) {
      uthreads = ui.GetInteger("MAXTHREADS");
      if (uthreads < nthreads) cv::setNumThreads(uthreads);
      logger->dbugout() << "User restricted threads:   " << uthreads << "\n";
    }
    int total_threads = cv::getNumThreads();
    logger->dbugout() << "Total threads:             " << total_threads << "\n";
    logger->flush();


    //-------------Matching Business--------------------------
    // Make the matcher class
    MatchMaker matcher(ui.GetString("NETWORKID"), factory->globalParameters() );
    matcher.setDebugLogger( logger, p_debug );

    // *** Set up fast geom processing ***
    // Define which geometry source we should use.  None is the default
    QString geomsource = ui.GetString("GEOMSOURCE").toLower();
    if ( "match" == geomsource ) { matcher.setGeometrySourceFlag(MatchMaker::Query); }
    if ( "from"  == geomsource ) { matcher.setGeometrySourceFlag(MatchMaker::Train); }
    if ( "both"  == geomsource ) { matcher.setGeometrySourceFlag(MatchMaker::Both); }

    // Trap load errors. Maintain a bad geom/load file list
    FileList badgeom;
    try {

      logger->dbugout() << "\nImage load started at  " << Application::DateTime() << "\n";

      // Check for FASTGEOM option
      QScopedPointer<FastGeom> fastgeom;
      if ( ui.GetBoolean("FASTGEOM") ) {
        fastgeom.reset( new FastGeom( factory->globalParameters() ) );
      }

      // Acquire query image. Do not use load_train_with_geom()!!!
      matcher.setQueryImage(MatchImage(ImageSource(ui.GetAsString("MATCH"))));

      // Get the trainer image and apply geom if requested
      if ( ui.WasEntered("FROM") ) {
        QString tname = ui.GetAsString("FROM");
        if ( !load_train_with_geom(matcher, tname, logger, fastgeom.data()) ) {
          badgeom.append(tname);
        }
      }

      // If there is a list provided, get that too
      if ( ui.WasEntered("FROMLIST") ) {
        FileList trainers(ui.GetFileName("FROMLIST"));
        BOOST_FOREACH ( FileName tfile, trainers ) {
          if ( !load_train_with_geom(matcher, tfile.original(), logger,
                                     fastgeom.data()) ) {
            badgeom.append(tfile.original());
          }
        }
      }

      // Load complete
      logger->dbugout() << "Image load complete at " << Application::DateTime() << "\n";

      // Check load/geom status and report bad ones before potential
      // app abort
      if ( badgeom.size() > 0 ) {
        logger->dbugout() << "\nTotal failed image loads/FastGeoms excluded: " << badgeom.size() << "\n";
        for ( int f = 0 ; f < badgeom.size() ; f++ ) {
          logger->dbugout() << badgeom[f].toString() << "\n";
        }

        if ( ui.WasEntered("TONOGEOM") ) {
          QString tonogeom(ui.GetAsString("TONOGEOM"));
          badgeom.write(tonogeom);
          logger->dbugout() << "\nSee also " << tonogeom << "\n\n";
        }
      }

      // Force output to show progress
      logger->flush();
    }
    catch (IException &ie) {
      QString msg = "Fatal load errors encountered";
      logger->dbugout() << "\n\n### " << msg << " - aborting..." << "\n";
      throw IException(ie, IException::Programmer, msg, _FILEINFO_);
    }

    // Got to have both file names provided at this point
    if ( matcher.size() <= 0 ) {
      logger->dbugout() << "\n\n###   No valid files loaded - aborting...\n";
      logger->dbugout() <<     "Time: " << Application::DateTime() << "\n";
      QString msg = "Input cubes (" + QString::number(badgeom.size()) + ") failed to load. " +
                    "Must provide valid FROM/FROMLIST and MATCH cube or image filenames";
      throw IException(IException::User, msg,  _FILEINFO_);
    }

    // Check for Sobel/Scharr filtering options for both Train and Images
    QString filter = factory->globalParameters().get("FILTER", "").toLower();
    // Apply the Sobel filter to all image
    if ( "sobel" == filter ) {
      matcher.query().addTransform(new SobelTransform("SobelTransform"));
      for (int i = 0 ; i < matcher.size() ; i++) {
        matcher.train(i).addTransform(new SobelTransform("SobelTransform"));
      }
    }

    // Add the Scharr filter to all images
    if ( "scharr" == filter ) {
      matcher.query().addTransform(new ScharrTransform("ScharrTransform"));
      for (int i = 0 ; i < matcher.size() ; i++) {
        matcher.train(i).addTransform(new ScharrTransform("ScharrTransform"));
      }
    }


    //  Apply all matcher/transform permutations
    logger->dbugout() << "\nTotal Algorithms to Run:     " << algorithms.size() << "\n";
    MatcherSolutionList matches = matcher.match(algorithms);
    const MatcherSolution *best = MatcherSolution::best(matches);
    logger->dbugout().flush();

    // If all failed, we're done
    if ( !best ) {
      logger->dbugout() << "Bummer! No matches were found!\n";
      QString mess = "NO MATCHES WERE FOUND!!!";
      throw IException(IException::User, mess, _FILEINFO_);
    }

    // Not valid results
    if ( best->size() <= 0 ) {
      logger->dbugout() << "Shucks! Insufficient matches were found ("
                        << best->size() << ")\n";
      QString mess = "Shucks! Insufficient matches were found (" +
                      QString::number(best->size()) + ")";
      throw IException(IException::User, mess, _FILEINFO_);
    }

    // Got some matches so lets process them
    Statistics quality = best->qualityStatistics();
    PvlGroup bestinfo("MatchSolution");
    bestinfo += PvlKeyword("Matcher", best->matcher()->name());
    bestinfo += PvlKeyword("MatchedPairs", toString(best->size()));
    bestinfo += PvlKeyword("ValidPairs", toString(quality.ValidPixels()));
    bestinfo += PvlKeyword("Efficiency",  toString(quality.Average()));
    if ( quality.ValidPixels() > 1 ) {
      bestinfo += PvlKeyword("StdDevEfficiency",
                              toString(quality.StandardDeviation()));
    }

    Application::Log(bestinfo);

    // If a cnet file was entered, write the ControlNet file of the specified
    // type.  Note that it was created as an image-to-image network. Must make
    // adjustments if a ground network is requested.
    if ( ui.WasEntered("ONET") ) {
      ControlNet cnet;
      cnet.SetNetworkId(ui.GetString("NETWORKID"));
      cnet.SetUserName(Isis::Environment::userName());
      cnet.SetDescription(best->matcher()->name());
      cnet.SetCreatedDate(findfeatures_runtime);
      QString target = ( ui.WasEntered("TARGET") ) ? ui.GetString("TARGET") :
                                                      best->target();
      cnet.SetTarget( target );
      ID pointId( ui.GetString("POINTID"), ui.GetInteger("POINTINDEX") );

      PvlGroup cnetinfo = matcher.network(cnet, *best, pointId);

      if ( cnet.GetNumPoints() <= 0 ) {
        QString mess = "No control points found!!";
        logger->dbugout() << mess << "\n";

        // Get total elapded time
        QTime totalE(0,0);
        totalE = totalE.addMSecs(runTime.msecsTo(QTime::currentTime()));
        logger->dbugout() << "\nSession complete in " << totalE.toString("hh:mm:ss.zzz")
                          << " of elapsed time\n";

        throw IException(IException::User, mess, _FILEINFO_);
      }

      // Umm..have to check this. Probably only makes sense with two images
      if ( ui.GetString("NETTYPE").toLower() == "ground" ) {
        cnetinfo += PvlKeyword("SpecialNetType", "Ground");
        for ( int i = 0 ; i < cnet.GetNumPoints() ; i++ ) {
          ControlPoint *point = cnet.GetPoint(i);
          point->SetType(ControlPoint::Fixed);
          point->Delete(matcher.query().id());
          point->SetRefMeasure(0);
        }
      }

      // Write out control network
      cnet.Write( ui.GetFileName("ONET") );
      Application::Log(cnetinfo);
    }

    // If user wants a list of matched images, write the list to the TOLIST filename
    if ( ui.WasEntered("TOLIST") ) {
      QLogger fout( QDebugLogger::create( ui.GetAsString("TOLIST"),
                                                (QIODevice::WriteOnly |
                                                QIODevice::Truncate) ) );
      fout.logger() << matcher.query().name() << "\n";
      MatcherSolution::MatchPairConstIterator mpair = best->begin();
      while ( mpair != best->end() ) {
        if ( mpair->size() > 0 ) {
            fout.logger() << mpair->train().source().name() << "\n";
        }
        ++mpair;
      }
    }

    // If user wants a list of failed matched images, write the list to the
    // TONOTMATCHED file if any are found
    if ( ui.WasEntered("TONOTMATCHED") ) {
      QStringList nomatches;

      // Search for unmatched files in the matcher network
      MatcherSolution::MatchPairConstIterator mpair = best->begin();
      while ( mpair != best->end() ) {
        if ( mpair->size() == 0 ) {
          nomatches.append( mpair->train().source().name() );
        }
        ++mpair;
      }

      // Only write the output file if there are any unmatched image files
      if ( nomatches.size() > 0) {
        QLogger fout( QDebugLogger::create( ui.GetAsString("TONOTMATCHED"),
                                                  (QIODevice::WriteOnly |
                                                  QIODevice::Truncate) ) );
        for ( auto const &imgfile : nomatches ) {
          fout.logger() << imgfile << "\n";
        }
      }
    }

    // Get total elapded time
    QTime totalT(0,0);
    totalT = totalT.addMSecs(runTime.msecsTo(QTime::currentTime()));
    logger->dbugout() << "\nSession complete in " << totalT.toString("hh:mm:ss.zzz")
                    << " of elapsed time\n";

    return;
  }
}