/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

// std library
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Qt library
#include <QList>
#include <QScopedPointer>
//#include <QString>
#include <QStringList>
#include <QTime>
#include <QElapsedTimer>

// boost library
#include <boost/foreach.hpp>

// ISIS
#include "Application.h"
#include "Database.h"
#include "FileName.h"
#include "GisGeometry.h"
#include "IString.h"
#include "Progress.h"
#include "Pvl.h"
#include "PvlFlatMap.h"
#include "Resource.h"
#include "SqlQuery.h"
#include "SqlRecord.h"
#include "Strategy.h"
#include "StrategyFactory.h"

#include "isisminer.h"

using namespace std; 

namespace Isis {

  //  Program constants
  const QString isisminer_program = "isisminer";
  const QString isisminer_version = "1.0";
  const QString isisminer_revision = "$Revision: 6513 $";
  const QString isisminer_runtime = Application::DateTime();

  /**
   * Isisminer assists in the identification, manipulation,
   * and output of data from a variety of data sources. It
   * runs a series of algorithms (or Strategies) that perform
   * various operations on input sources (or Resources).
   *
   * @param ui UserInterface object containing parameters
   */
  void isisminer(UserInterface &ui) {

    // File containing isisminer configuration run
    QString configFile = ui.GetFileName("CONFIG");

    // open optional, global parameter file if provided
    // file is for use in global variable pool
    if ( ui.WasEntered("GLOBALS") ) {
      Pvl pvl_globals(ui.GetFileName("GLOBALS"));

      return isisminer(configFile, ui, &pvl_globals);
    }

    return isisminer(configFile, ui);
  }


  /**
   * Isisminer assists in the identification, manipulation,
   * and output of data from a variety of data sources. It
   * runs a series of algorithms (or Strategies) that perform
   * various operations on input sources (or Resources).
   * 
   * @param ui UserInterface object containing parameters
   *
   * @throws IException::User "Ill-formed PARAMETERS [PARAMETERS] - use form @key:val"
   *
   */
  void isisminer(QString &configFileName, UserInterface &ui,  
                 Pvl *pvl_globals) {

    StrategyFactory *factory = StrategyFactory::instance();

    SharedResource globals(new Resource("Globals"));
    globals->add("Program", isisminer_program);
    globals->add("Version", isisminer_version);
    globals->add("Revision", isisminer_revision);
    globals->add("RunTime", isisminer_runtime);
  
    // File containing isisminer configuration run
    globals->add("CONFIG", configFileName);

    // Add parameters provided by user to global resources
    if ( ui.WasEntered("PARAMETERS") ) {
      QString parameters = ui.GetString("PARAMETERS");
      globals->add("PARAMETERS", parameters);
      
      // Split by separate parameters
      QStringList parmlist = parameters.split("@", Qt::SkipEmptyParts);
      BOOST_FOREACH (QString parm, parmlist) {
        // Split values from keyword name
        QStringList keyval = parm.split(":", Qt::SkipEmptyParts);
        if ( keyval.size() != 2 ) {
          QString mess = "Ill-formed PARAMETERS (" + parm + ") - use form @key:val";
          throw IException(IException::User, mess, _FILEINFO_);
        }

        // Now split multi string values and construct the Pvl keyword
        QString keyname = keyval[0];
        QStringList values =  keyval[1].split(",", Qt::SkipEmptyParts);
        PvlKeyword keyword(keyname);
        BOOST_FOREACH ( QString val, values) {
          keyword.addValue(val);
        }

        // Add the parameter to global parameters
        globals->add(keyword);
      }
    }

    // Add to factory
    factory->addGlobal(globals);

    // If provided, load optional, global parameter
    // file for use in global variable pool
    if ( pvl_globals != nullptr ) {
      SharedResource gfile(new Resource("GlobalFileResources", PvlFlatMap(*pvl_globals)));
      factory->addGlobal(gfile);
      globals->add("GLOBALS", ui.GetFileName("GLOBALS"));
    }

    // Create strategies (computations, constraints, ranks, sorts, etc...)
    cout << "\nCreating strategies...\n";
    StrategyList strategies = factory->buildRun(configFileName);
    cout << "Finished creating " << factory->manufactured() << " strategies...\n";

    // Input resource list preserved for subsequent processing
    ResourceList resources;
    QTime runTime = QTime::currentTime();
    BOOST_FOREACH ( SharedStrategy strategy, strategies ) {
      QTime stime = QTime::currentTime();
      cout << "\nRunning " << strategy->type() << "::" << strategy->name() 
          << " (TimeIn:  " << stime.toString("hh:mm:ss.zzz") 
          << ")\n"
          << "Description: " << strategy->description() << "\n";
      QElapsedTimer stimer;
      stimer.start();
      int n = strategy->apply(resources);
      unsigned int ntotal = strategy->totalProcessed();
      cout << n << " of " << ntotal << " processed in " 
          << strategy->type() << "::" << strategy->name() 
          << " (TimeOut: " << QTime::currentTime().toString("hh:mm:ss.zzz") << ")\n";
      cout << "ElapsedTime(s): " << stimer.elapsed() / 1000  << "\n";
    }

    // Get total elapsed time
    QTime totalT(0,0);
    totalT = totalT.addMSecs(runTime.msecsTo(QTime::currentTime()));
    cout << "\nSession complete in " << totalT.toString("hh:mm:ss.zzz") 
        << " of elapsed time\n";

      return;
  } 
}

