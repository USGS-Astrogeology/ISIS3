/**
 * @file
 * $Revision: 6513 $
 * $Date: 2016-01-14 16:04:44 -0700 (Thu, 14 Jan 2016) $
 * $Id: StrategyFactory.cpp 6513 2016-01-14 23:04:44Z kbecker@GS.DOI.NET $
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are
 *   public domain. See individual third-party library and package descriptions
 *   for intellectual property information, user agreements, and related
 *   information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or
 *   implied, is made by the USGS as to the accuracy and functioning of such
 *   software and related material nor shall the fact of distribution
 *   constitute any such warranty, and no responsibility is assumed by the
 *   USGS in connection therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html
 *   in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */
#include "StrategyFactory.h"

// Qt library
#include <QCoreApplication>
#include <QDir>
#include <QLibrary>
#include <QString>

// boost library
#include <boost/foreach.hpp>

// other ISIS
#include "AssetSidebarStrategy.h"
#include "CalculatorStrategy.h"
#include "CnetReaderStrategy.h"
#include "CsvReaderStrategy.h"
#include "CsvWriterStrategy.h"
#include "DatabaseReaderStrategy.h"
#include "FileName.h"
#include "FilterStrategy.h"
#include "GisIntersectStrategy.h"
#include "GisOverlapStrategy.h"
#include "GisUnionStrategy.h"
#include "IException.h"
#include "IsNumericStrategy.h"
#include "LimitStrategy.h"
#include "NumericalSortStrategy.h"
#include "PdsTableCreatorStrategy.h"
#include "PdsTableFormatStrategy.h"
#include "PdsTableReaderStrategy.h"
#include "Pvl.h"
#include "PvlFlatMap.h"
#include "PvlReaderStrategy.h"
#include "PvlObject.h"
#include "Resource.h"
#include "ResourceManagerStrategy.h"
#include "RunCommandStrategy.h"
#include "SidebarStrategy.h"
#include "StereoPairStrategy.h"

using namespace std;

namespace Isis {

  /**
   * A static instance of the strategy factory class. It is initialized to NULL
   * and instantiated when the instance() method is called.
   */
StrategyFactory *StrategyFactory::m_strategymaker = 0;


  /**
   * Private default constructor so that this class is only instatiated through
   * the instance() method. This ensures that only a singleton object is
   * constructed.
   */
  StrategyFactory::StrategyFactory() {
    //  This ensures this singleton is shut down when the application exists,
    //  so the GEOS system can be shut down cleanly.
   qAddPostRoutine(dieAtExit);
   m_numberMade = 0;
   return;
}


  /**
   * Destroys the StrategyFactory object.
   */
  StrategyFactory::~StrategyFactory() {
}


  /**
   * Gets the singleton instance of this class. If it has not been instantiated
   * yet, the default constructor is called.
   *
   * @return StrategyFactory  A pointer to the StrategyFactory singleton object.
   */
  StrategyFactory *StrategyFactory::instance() {
  if (!m_strategymaker) {
    m_strategymaker = new StrategyFactory();
  }
  return (m_strategymaker);
}


/**
 * @brief Add a shared resource to global parameter list
 *
 * Users can add to an internal list of global resource keywords that will be
 * provide to every Strategy created by this factory. This provides a consistent
 * base of keyword substitutions when strategies apply() functions are called.
 *
 * @author 2015-05-08 Kris Becker
 *
 * @param global Shared global resource for parameter substitution
 */
  void StrategyFactory::addGlobal(SharedResource &global) {
    m_globals.append(global);
    return;
  }

/**
 * @brief Return list of current global parameters
 *
 * The list returned by this method will be used to create new strategies
 * providing a consistent approach to keyword parameter substitution.
 *
 * @author 2015-05-08 Kris Becker
 *
 * @return ResourceList List of global parameters
 */
  const ResourceList &StrategyFactory::getGlobals() const {
    return (m_globals);
  }

  StrategyList StrategyFactory::buildRun(const QString &configFile) const {
    return (buildRun(configFile, getGlobals()));
  }


  StrategyList StrategyFactory::buildRun(const PvlObject &config) const {
    return ( buildRun(config, getGlobals()));
  }


  Strategy *StrategyFactory::create(const PvlObject &definition) const {
    return (create(definition, getGlobals()));
  }

  /**
   * Uses the given configuration file and global resource of keywords to build
   * a list of Strategy objects. The configuration file should be in PVL format
   * with an object named "IsisMiner" that contains the configuration for the
   * Strategy objects to be constructed. The IsisMiner PVL is pulled from the
   * file and buildRun(PvlObject, SharedResource) is called.
   *
   * Below is an example of an IsisMiner PVL that, when passed into buildRun(),
   * will create a StrategyList of size 2:
   * @code
   *
   * Object = IsisMiner
   *
   *   Name = StrategyBuilder
   *   ParametersRequired = ( inputdir, outputdir )
   *
   *   Object = Strategy
   *     Name          = TestWithIdentity
   *     Type          = CnetReader
   *     CnetFile      = "%1/Alph_VIS.net"
   *     CnetFileArgs  = "inputdir"
   *     Identity      = "%1"
   *     IdentityArgs  = (PointId)
   *     Description   = "Test the default functionality of CnetReader"
   *   EndObject
   *
   *   Object=Strategy
   *     Name          = WriteCsvTest1
   *     Type          = CsvWriter
   *     CsvFile       = "%1/cnetreader_with_id.csv"
   *     CsvFileArgs   = "outputdir"
   *     Mode          = Create
   *     Header        = True
   *
   *     #  Write all input fields as read in as noop should match exactly
   *     Keywords      = (ChooserName, Created, DateTime, Description,
   *                      LastModified, Line, MeasureType,NetworkId, PointId,
   *                      PointType, Reference, Sample, SerialNumber,
   *                      TargetName, UserName, Version)
   *     Delimiter     = ","
   *     DefaultValue  = "NULL"
   *   EndObject
   * EndObject
   *
   * @endcode
   *
   * @param configFile  The name of the configuration file containing the
   *                    IsisMiner PvlObject with configuration information
   *                    needed to construct each of the Strategy objects.
   * @param globals   List of global keywords to use in argument substitutions
   *
   * @return StrategyList  The list of Strategy objects created from the
   *                       given configuration and resource.
   * @throw IException::User  "Strategy config file does not contain
   *                           IsisMiner strategies object."
   */
  StrategyList StrategyFactory::buildRun(const QString &configFile,
                                         const ResourceList &globals) const {
  Pvl pvl(configFile.toStdString());
  if (!pvl.hasObject("IsisMiner") ) {
    throw IException(IException::User,
                     "Strategy config file [" + configFile.toStdString() +
                     "] does not contain IsisMiner strategies object.",
                     _FILEINFO_);
  }

  return (buildRun(pvl.findObject("IsisMiner"), globals));
}


  /**
   * Uses the given PVL configuration object and global resource of keywords to
   * build a list of Strategy objects. For each Strategy object to be created,
   * the PVL configuration object must contain a PVL object named "Strategy."
   * This method is called by the buildRun(QString, SharedResource) method.
   *
   * @see buildRun(QString, SharedResource)
   *
   * @param config   A PVL object containing the configuration needed for
   *                 constructing each Strategy object.
   * @param globals   List of global keywords to use in argument substitutions
   *
   * @return StrategyList  The list of Strategy objects created from the
   *                       given configuration and resource.
   */
  StrategyList StrategyFactory::buildRun(const PvlObject &config,
                                         const ResourceList &globals) const {

    validateUserParameters(config, globals);
    StrategyList slist;
    for (int i = 0 ; i < config.objects() ; i++) {
      const PvlObject &object = config.object(i);
      if ( object.isNamed("Strategy") ) {
        slist.push_back(SharedStrategy(create(object, globals)));
      }
    }
    return (slist);
  }


  /**
   * Constructs a Strategy object from the given PVL definition object and
   * global resource of keywords. If the PVL does not contain a valid
   * configuration for a known Strategy, then an exception is thrown. Each time
   * this method is successfully invoked, the number of manufactured Strategy
   * objects increments.
   *
   * @param definition  A PVL object containing the configuration needed to
   *                    construct the Strategy object.
   * @param globals   List of global keywords to use in argument substitutions
   *
   * @return Strategy*  A pointer to the Strategy constructed from the
   *                    given definition and globals.
   * @throw IException::User  "Could not find a strategy for type."
   */
  Strategy *StrategyFactory::create(const PvlObject &definition,
                                    const ResourceList &globals) const {
  Strategy *strategy = findStrategy(definition, globals);
  if ( !strategy ) {
    std::string sname("UNKNOWN"), stype("UNKNOWN");
    if ( definition.hasKeyword("Name") ) { 
      sname = definition["Name"][0]; 
    }
    if ( definition.hasKeyword("Type") ) { 
      stype = definition["Type"][0]; 
    }
    std::string mess = "Could not create a " + sname +
                   " strategy for type [" + stype + "].";
    throw IException(IException::User, mess, _FILEINFO_);
  }
    m_numberMade++;
  return (strategy);
}


  /**
   * Gets the number of Strategy objects that have been manufactured.
   *
   * @return int  The number of Strategy objects constructed by this factory.
   */
  int StrategyFactory::manufactured() const {
  return (m_numberMade);
}

/**
 * @brief Check that required user parameters are provided in resource list
 *
 * This method will look for a keyword called RequiredParameter in the provided
 * container.  If it exists, it will check the provided resource list (assumed
 * to be globals) for existance. If any don't exist, an exception is thrown
 * reporting the missing parameters.
 *
 * @author 2015-06-04 Kris Becker
 *
 * @param conf       PvlContainer to look for RequiredParameters keyword and
 *                   validate existance in parameters list.
 * @param parameters List of parameters resources to search for keywords that
 *                   must be provided by users.
 */
  void StrategyFactory::validateUserParameters(const PvlContainer &conf,
                                               const ResourceList &parameters)
                                               const {

    if ( conf.hasKeyword("RequiredParameters") ) {
      PvlKeyword keys = conf.findKeyword("RequiredParameters");
      QStringList missing;
      for (int i = 0 ; i < keys.size() ; i++) {

        // Search Resource list for parameter
        int found(0);
        BOOST_FOREACH ( SharedResource resource, parameters ) {
          if ( resource->exists(QString::fromStdString(keys[i])) ) found++;
        }

        // If it doesn't exist in any resource, this is an error. Save it off
        // for reporting.
        if ( !found) missing.push_back(QString::fromStdString(keys[i]));
      }

      if ( !missing.isEmpty() ) {
        QString mess = "Users of this configuration must provide the "
                       "following parameter(s) but they were not found: " +
                        missing.join(", ");
        throw IException(IException::User, mess.toStdString(), _FILEINFO_);
      }
    }
    return;
  }


  /**
   * Attempts to construct a Strategy from the known Strategy classes using the
   * given information in the PVL definition object and the given global
   * resource of keywords. The Strategy type is pulled from the definition file
   * and compared to the known Strategy types. If found, this method attempts to
   * construct the corresponding Strategy object from the definition and
   * globals.
   *
   * @param definition  A PVL object containing the configuration needed to
   *                    construct the Strategy object.
   * @param globals     A shared pointer to the global resource of keywords used
   *                    to construct the Strategy object.
   *
   * @return Strategy*  A pointer to the Strategy constructed from the
   *                    given definition and globals.
   */
  Strategy *StrategyFactory::findStrategy(const PvlObject &definition,
                                          const ResourceList &globals) const {

    QString stype;
    try {
      stype = QString::fromStdString(definition["Type"][0]).toLower();
    }
    catch (IException &ie) {
      QString sname("UNKNOWN");
      if ( definition.hasKeyword("Name") ) { 
        sname = QString::fromStdString(definition["Name"][0]); 
      }
      std::string mess = "Strategy Type does not exist in configuration for " +
                      sname.toStdString() + " strategy!";
      throw IException(ie, IException::User, mess, _FILEINFO_);
    }

    // Check for plugin-type strategy that is loaded from a runtime library
    if ( definition.hasGroup("Plugin") ) {
      return ( loadStrategyPlugin(definition.findGroup("Plugin"), definition, globals) );
    }
    else if ( "calculator" == stype ) {
      return (new CalculatorStrategy(definition, globals));
    }
    else if ( "cnetreader" == stype ) {
      return (new CnetReaderStrategy(definition, globals));
    }
    else if ( "csvreader" == stype ) {
      return (new CsvReaderStrategy(definition, globals));
    }
    else if ( "csvwriter" == stype ) {
      return (new CsvWriterStrategy(definition, globals));
    }
    else if ( "database" == stype ) {
      return (new DatabaseReaderStrategy(definition, globals));
    }
    else if ( "databasereader" == stype ) {
      return (new DatabaseReaderStrategy(definition, globals));
    }
    else if ( "filter" == stype ) {
      return (new FilterStrategy(definition, globals));
    }
    else if ( "gisintersect" == stype ) {
      return (new GisIntersectStrategy(definition, globals));
    }
    else if ( "gisoverlap" == stype ) {
      return (new GisOverlapStrategy(definition, globals));
    }
    else if ( "gisunion" == stype ) {
      return (new GisUnionStrategy(definition, globals));
    }
    else if ( "isnumeric" == stype ) {
      return (new IsNumericStrategy(definition, globals));
    }
    else if ( "limit" == stype ) {
      return (new LimitStrategy(definition, globals));
    }
    else if ( "numericalsort" == stype ) {
      return (new NumericalSortStrategy(definition, globals));
    }
    else if ( "pdstablecreator" == stype ) {
      return (new PdsTableCreatorStrategy(definition, globals));
    }
    else if ( "pdstableformat" == stype ) {
      return (new PdsTableFormatStrategy(definition, globals));
    }
    else if ( "pdstablereader" == stype ) {
      return (new PdsTableReaderStrategy(definition, globals));
    }
    else if ( "pvlreader" == stype ) {
      return (new PvlReaderStrategy(definition, globals));
    }
    else if ( "resourcemanager" == stype ) {
      return (new ResourceManagerStrategy(definition, globals));
    }
    else if ( "runcommand" == stype ) {
      return (new RunCommandStrategy(definition, globals));
    }
    else if ( "sidebar" == stype ) {
      return (new SidebarStrategy(definition, globals));
    }
    else if ( "assetsidebar" == stype ) {
      return (new AssetSidebarStrategy(definition, globals));
    }
    else if ( "stereopair" == stype ) {
      return (new StereoPairStrategy(definition, globals));
    }

    // Strategy not found
    return (0);
  }

/**
 * @brief Create a strategy from an external plugin library
 *
 * This method will take the plugin group definition and load the strategy from
 * an external library implemented as a plugin.  The definition has the basic
 * qualifications:
 *
 * @code
 *  Group = Plugin
 *    StrategyPluginPath=("../../plugin/src/RunCommandStrategy","plugin/src/RunCommandStrategy",
 *      "/Users/kbecker/IsisDevelopment/MissionSWPush/DeveloperSubmissions/isis/src/base/apps/isisminer/plugin/src/RunCommandStrategy")
 *    Library = RunCommandStrategy
 *    Routine = RunCommandStrategyPlugin
 *  EndGroup
 * @endcode
 *
 * In the above definition, the StrategyPluginPath is added to the search path
 * for finding shared plugin libraries specfified in the Library keyword. The
 * Routine keyword contains the name of the routine implemented to construct and
 * return a Stategy algorith. The Routine must be declared callable via an
*  extern reference.  Here is an example implementation found in
*  RunCommandStrategy.cpp:
 *
 * @code
 *  extern "C" Isis::Strategy *RunCommandStrategyPlugin(const Isis::PvlObject
 *                              &definition,const Isis::ResourceList &globals) {
 *    return new Isis::RunCommandStrategy(definition, globals);
 *  }
 * @endcode
*
*  The Strategy algorithm can be implemented just like any other Strategy be
*  inheriting the Strategy class. Note that when the library is loaded attempts
*  are made to load all the symbols before the Routine is called to instantiate
*  the Strategy.
 *
 * @author 2015-05-08 Kris Becker
 *
 * @param plugindef  PvlGroup container of plugin definition
 * @param definition PvlObject definition of strategy that is implemented in the
 *                   plugin
 * @param globals    List of global keywords to use in argument substitutions
 *
 * @return Strategy* Pointer to strategy invoked from the plugin
 */
 Strategy *StrategyFactory::loadStrategyPlugin(const PvlContainer &plugindef,
                                               const PvlObject &definition,
                                               const ResourceList &globals) const {

   PvlFlatMap pluginkeys(plugindef);
   Strategy *strategy(0);
   try {
     // Create a list of plugin paths starting with no path and local directory first
     QStringList dirlist(".");
     if ( pluginkeys.exists("StrategyPluginPath") ) {
       dirlist.append(pluginkeys.allValues("StrategyPluginPath"));
     }

     BOOST_FOREACH(SharedResource parms, globals) {
       if ( parms->exists("StrategyPluginPath") ) {
         dirlist.append(parms->keys().allValues("StrategyPluginPath"));
       }
     }

     // Append the ISIS library path
     if ( !dirlist.contains("$ISISROOT/lib") ) { 
       dirlist.push_back("$ISISROOT/lib");
     }

     // Attempt to load the library
     QLibrary plugin;
     plugin.setLoadHints(QLibrary::ResolveAllSymbolsHint);
     QString libname = pluginkeys.get("Library");
     BOOST_FOREACH ( QString dir, dirlist ) {
       QDir path(dir);
       FileName library(path.filePath(libname).toStdString());
       plugin.setFileName(QString::fromStdString(library.expanded()));
       if ( plugin.load() ) { break; }  // library successfully loaded!
     }

     // Check for successful load
     if ( !plugin.isLoaded() ) {
       std::string mess = "Cannot find/load Strategy plugin library " + libname.toStdString();
       throw IException(IException::User, mess, _FILEINFO_);
     }

     // Plugin library is loaded, get creator routine
     typedef Strategy *(*StrategyCreator)(const PvlObject &object, const ResourceList &globals);
     QString routine = pluginkeys.get("Routine");
     StrategyCreator maker = (StrategyCreator) plugin.resolve(routine.toLatin1().data());
     if ( !maker ) {
       std::string mess = "Unable to resolve Routine name [" + routine.toStdString() +
                      "] in Strategy plugin [" + plugin.fileName().toStdString() + "]";
       throw IException(IException::User, mess, _FILEINFO_);
     }

     // Create the strategy
     strategy = maker(definition, globals);
   }
   catch (IException &ie) {
      std::string sname("UNKNOWN");
      if ( definition.hasKeyword("Name") ) { 
        sname = definition["Name"][0]; 
      }
      std::string mess = "Failed to load " + sname + " Strategy plugin!";
      throw IException(ie, IException::User, mess, _FILEINFO_);
   }

   return ( strategy );
 }

/**
    * @brief Exit termination routine
    *
    * This (static) method ensure this object is destroyed when Qt exits.
    *
    * Note that it is error to add this to the system _atexit() routine because
    * this object utilizes Qt classes.  At the time the atexit call stack is
    * executed, Qt is long gone resulting in Very Bad Things.  Fortunately, Qt
    * has an exit stack function as well.  This method is added to the Qt exit
    * call stack.
    */
  void StrategyFactory::dieAtExit() {
    delete  m_strategymaker;
    m_strategymaker = 0;
    return;
  }


} // namespace Isis
