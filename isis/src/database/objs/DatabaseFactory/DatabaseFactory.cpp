/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>

using namespace std;

#include <QDebug>
#include <QString>
#include <QStringList>
#include <QCoreApplication>
#include <QSqlDatabase>

#include "DatabaseFactory.h"
#include "DbAccess.h"
#include "Preference.h"
#include "FileName.h"
#include "IString.h"

namespace Isis {


  DatabaseFactory *DatabaseFactory::_factory = 0;

  /**
   * @brief Constructor establishing the startup state of this singleton
   *
   * This constructor sets up the initial state of the DatabaseFactory object.
   * This object is implementated as a singleton and must take measures to ensure
   * that at program termination, all database connections are closed.  This is
   * acheived through setting up a Qt termination function that is executed when
   * the Qt environment is exiting.
   *
   * Note that it is an error to schedule this process outside of Qt (using the
   * atexit() function) because it uses Qt classes that must be properly
   * deallocated.  This cannot be done once the Qt environment exits.
   *
   * Upon invocation of this object, a database access scheme is read.  This is
   * explained in the class documentation and will not be repeated here.
   *
   * As a safety measure, this constructor determines if a Qt application class,
   * Gui or non-Gui is initialized.  If it is not, a non-Gui QCoreApplication is
   * instantiated to properly initialize the Qt environment.  This is required in
   * order to have any Qt-based database plugins loaded and available for
   * subsequent use.  It is highly recommended that this activity occur
   * explicitily at a much higher level as it allows for greater programmer
   * application control.
   */
  DatabaseFactory::DatabaseFactory() : _defProfName(""),  _profiles(),
    _defDatabase(""), _dbList() {

    // insure the drivers are being loaded
    loadDrivers();
    //  Checks the existance of the Qt application core.  This is required in order
    //  ensure database driver plugins are loaded - if they exist.
    QCoreApplication *cApp = QCoreApplication::instance();
    if(cApp == 0) {
      static char **argv = 0;

      if(!argv) {
        argv = new char*[2];
        argv[0] = new char[1024];
        strcpy(argv[0], "DatabaseFactory");
        argv[1] = 0;
      }

      static int argc  = 1;
      new QCoreApplication(argc, argv);
    }

    //  Initializes
    init();

    //  This ensures this singleton is shut down when the application exists,
    //  so existing database connections are terminated.
    qAddPostRoutine(DieAtExit);
  }

  /**
   * @brief Destructor implements self-destruction of this object
   */
  DatabaseFactory::~DatabaseFactory() {
    selfDestruct();
  }

  /**
   * @brief Exit termination routine
   *
   * This (static) method ensure this object is destroyed when Qt exits.  This is
   * required so that persistant database connections can be terminated and
   * cleaned up.
   *
   * Note that it is error to add this to the system _atexit() routine because
   * this object utilizes Qt classes.  At the time the atexit call stack is
   * executed, Qt is long gone resulting in Very Bad Things.  Fortunately, Qt has
   * an exit stack function as well.  This method is added to the Qt exit call
   * stack.  See the DatabaseFactory() constructor.
   */
  void DatabaseFactory::DieAtExit() {
    delete  _factory;
    _factory = 0;
    return;
  }

  /**
   * @brief Returns and instance of this DatabaseFactory singleton
   *
   * This method is the sole source of access to the DatabaseFactory class.  Upon
   * the first call to this method, the DatabaseFactory is created.  Subsequent
   * calls simply return a pointer reference to object which can be used to
   * reference existing databases and database drivers.
   *
   * @return DatabaseFactory*  Returns a pointer reference to a DatabaseFactory
   *         singleton object
   */
  DatabaseFactory *DatabaseFactory::getInstance() {
    if(DatabaseFactory::_factory == 0) {
      DatabaseFactory::_factory = new DatabaseFactory();
    }
    return (DatabaseFactory::_factory);
  }

  /**
   * @brief Establishes an access profile for subsequent database connections
   *
   * This method takes the name of a database access profile file and adds all its
   * profiles to the internally maintained list.  Users of this class can then use
   * any one of profiles in the list as the access scheme for all database
   * creation and connection requests.
   *
   * If a profile of the same name happens to already exist, it is replaced by any
   * new one contained in the access profiles file.
   *
   * \b NOTE that if a default profile is specified in the added acsess scheme it
   * supercedes all other defaults - which includes one loaded at startup from
   * IsisPreferences and one set by the programmer explicitly.  To retain current
   * settings require the user to get the named default prior to adding these
   * profiles and resetting it upon return.
   *
   * @param profileFile Name of profile file to add
   *
   * @return bool True if the new profile is successfully added, false otherwise.
   */
  bool DatabaseFactory::addAccessProfile(const QString &profileFile) {
    try {
      FileName dbconf(profileFile.toStdString());
      if(dbconf.fileExists()) {
        DbAccess acp(profileFile);

        //  Add the top level one - may be replaced
        const DbProfile &topProf = acp.getProfile();
        _profiles.add(topProf.Name(), topProf);

        //  Now add each individual one
        for(int i = 0 ; i < acp.profileCount() ; i++) {
          const DbProfile &newProf = acp.getProfile(i);
          _profiles.add(newProf.Name(), newProf);
        }

        // See if a default exists
        if(acp.exists("DefaultProfile")) {
          _defProfName = acp.value("DefaultProfile");
        }
        return (true);
      }
    }
    catch(...) {
      // upon any failure, we were unsuccessful
      return (false);
    }

    // File did not exist
    return (false);
  }
  /**
   * @brief Adds a database access profile to the list of profiles
   *
   * This method will add a new access profile to the list of existing profiles
   * and make it available for subsequent access requests.
   *
   * \b NOTE that if an profile exists with the same name, it is \b replaced with
   * this one.  The one is no longer accessable.
   *
   * @param profile New access profile to add to internal list
   */
  void DatabaseFactory::addProfile(const DbProfile &profile) {
    _profiles.add(profile.Name(), profile);
    return;
  }

  /**
   * @brief Return list of names of currently available profiles
   *
   * This method will return a list of the names of all currently available
   * database access profiles as a vector of strings.
   *
   * @return std::vector<QString> List of available profile names
   */
  std::vector<QString> DatabaseFactory::getProfileList() const {
    std::vector<QString> plist;
    for(int i = 0 ; i < _profiles.size() ; i++) {
      plist.push_back(_profiles.key(i).ToQt());
    }
    return (plist);
  }

  /**
   * @brief Get the specified database access profile
   *
   * This method provides access profiles from the "system-wide" database access
   * profile.  The primary source of availability of these profiles is established
   * upon the first instance returned from the DatabaseFactory through the
   * preferences. See the DatabaseFactory documentation for how this is
   * established.
   *
   * If the named profile dones not exist, a black one is returned and can be
   * checked via the DbProfile.isValid() method.
   *
   * @param name       Name of the profile to return.  If not provided by the
   *                   caller, then the default will be provided (which is defined
   *                   in the Pvl input Access Profile or is unspecified which
   *                   will result in an invalid profile (typically)).
   *
   * @return DbProfile An database access profile resulting from the request.
   * @see initPreferences().
   */
  DbProfile DatabaseFactory::getProfile(const QString &name) const {
    QString profName(name);
    if(profName.isEmpty()) profName = _defProfName;

    // Refer to user access if provided
    if(!_profiles.exists(profName)) {
      return(DbProfile(profName));
    }
    else {
      return (_profiles.get(profName));
    }
  }

  /**
   * @brief Determine what database drivers are available
   *
   * This method returns a vector of strings that contains the names of all
   * available database drivers.
   *
   * In this list will be Qt named drivers, such as QMYSQL, formal names for
   * drivers such as MySQL and PostgreSQL, and named database connections such as
   * UPC.  The list includes all currently availble database resources.
   *
   * @return std::vector<QString> List of database drivers
   */
  std::vector<QString> DatabaseFactory::available() const {
    Drivers drivers = getResourceList(true, true);
    std::vector<QString> dblist;
    for(int i = 0 ; i < drivers.size() ; i++) {
      dblist.push_back(drivers.key(i).ToQt());
    }
    return (dblist);
  }

  /**
   * @brief Check for the existance of a specific database driver
   *
   * dbname can be a Qt database driver or a formal name of a database, such as
   * MySQL. The name is case insensitve.
   *
   * @param dbname  Name of the database driver to check availability
   *
   * @return bool True if the specifed driver exists, otherwise false
   */
  bool DatabaseFactory::isDriverAvailable(const QString &dbname) const {
    Drivers drivers = getResourceList(true, false);
    if(!drivers.exists(dbname)) {
      return (false);
    }
    return (QSqlDatabase::isDriverAvailable(drivers.get(dbname)));
  }

  /**
   * @brief Check for availablity of a database connection resource
   *
   * This method checks for the existance of a driver for the specified named
   * database resource.
   *
   * @param dbname  Name of the database resource to check availability
   *
   * @return bool True if the connection exists, otherwise false
   */
  bool DatabaseFactory::isAvailable(const QString &dbname) const {
    Drivers dbdrivers = getResourceList(false, true);
    QString name(dbname);
    if(name.isEmpty()) {
      name = _defDatabase;
    }
    return (dbdrivers.exists(name));
  }

  /**
   * @brief Determines if the database resource is connected
   *
   * Checks the named database for existance in the database connection pool.
   *
   * @param dbname Case insensitive name of database connection
   *
   * @return bool True if it exists in the pool, otherwise false
   */
  bool DatabaseFactory::isConnected(const QString &dbname) const {
    QString name(dbname);
    if(name.isEmpty()) {
      name = _defDatabase;
    }
    return (QSqlDatabase::contains(name));
  }

  /**
   * @brief Checks if the database resource is persistant
   *
   * This method tests the database to determine if the connection is persistant.
   * Persistance means that the connection to the database remains open.  A
   * databases persistant state is maintained in this object by holding a
   * reference to it.
   *
   * @param dbname Name of the database to check persistance status
   *
   * @return bool True if the database is persistant, otherwise false
   */
  bool DatabaseFactory::isPersistant(const QString &dbname) const {
    return (_dbList.exists(dbname));
  }


  /**
   * @brief Create a database using the named driver
   *
   * This method creats a database connection using the specified driver.  The
   * driver should be one of the available drivers as identified by the
   * isDriverAvailable() method.
   *
   * The caller provides a name of the database created by this method.  This name
   * is arbitrary and can be anything meaningful to the caller of this method.  IT
   * is intended (and required) for use of named connections that are retained for
   * future use in this object.
   *
   * @param driver  Name of database driver to instantiate for the connection
   * @param dbname  Name of the database connect provided by the caller
   *
   * @return QSqlDatabase A named Qt database object created with the specifed
   *         driver
   */
  QSqlDatabase DatabaseFactory::create(const QString &driver,
                                       const QString &dbname) {


    // Check driver availability
    if(!isDriverAvailable(driver)) {
      std::string mess = "Driver [" + driver + "] for database [" + dbname
                     + "] does not exist";
      throw IException(IException::Unknown, mess, _FILEINFO_);
    }

    //  Create the database with the specified driver and name
    Drivers drivers = getResourceList(true, false);
    return (QSqlDatabase::addDatabase(drivers.get(driver), dbname));
  }

  /**
   * @brief Create a database connection from a named resource
   *
   * This method is used to create a database from an existing resource.  This
   * typically will be a database source that has been added using the add()
   * method.  It provides persistant connections from this object.
   *
   * @param dbname  Name of database resource
   *
   * @return QSqlDatabase  A Database object
   */
  QSqlDatabase DatabaseFactory::create(const QString &dbname) {

    //  Return an existing connection
    if(_dbList.exists(dbname)) {
      return (_dbList.get(dbname));
    }

    // One doesn't exist, throw an error
    std::string mess = "Database [" + dbname + "] does not exist";
    throw IException(IException::Unknown, mess, _FILEINFO_);
  }

  /**
   * @brief Adds the database to the connection pool making it persistant
   *
   * This method can be called after the create() method, handing back the created
   * database object.  In effect, this creates a copy of the database in its
   * current state and makes it available to subsquent create(name) calls.
   *
   * If one calls create() and does not add the database using this method, then
   * the database is destroyed/deallocated when it goes out of scope or no longer
   * has a reference to it.
   *
   * Adding a database to the connection pool using this method essentially makes
   * it persistant and available for subsquent use.
   *
   * @param db  Database object to add to pool
   * @param name Name to associate with the object
   * @param setAsDefault True if this is to become the default connection
   */
  void DatabaseFactory::add(const QSqlDatabase &db, const QString &name,
                            bool setAsDefault) {
    _dbList.add(name, db);
    if(setAsDefault) {
      _defDatabase = name;
    }
    return;
  }

  /**
   * @brief Removes the database from the connection pool and destroys it
   *
   * This method should be invoked only after add() has been called with the named
   * database.  It is removed from the connection pool and destroyed terminating
   * any persistant connection it may have had.
   *
   * Other references to this database are invalidated.
   *
   * @param name  Name of the database to remove
   */
  void DatabaseFactory::destroy(const QString &name) {
    remove(name);
    QSqlDatabase::removeDatabase(name);
    return;
  }

  /**
   * @brief Removes the database from the connection pool
   *
   * This method removes the named database from the connection pool making it a
   * non-persistant database connection.  References to the database are still
   * valid until they are destroyed in the callers environment.
   *
   * @param name  Name of database to remove from the connection pool
   */
  void DatabaseFactory::remove(const QString &name) {
    _dbList.remove(name);
#if 0
    if(IString::Equal(name, _defDatabase)) {
      _defDatabase = "";
    }
#endif
    return;
  }

  /**
   * @brief Initializes this object upon instantiation
   *
   * This method is called to initialize the database pool.  This includes loading
   * any explicit database drivers and loading database access profiles.
   *
   */
  void DatabaseFactory::init() {
    //  Add the PostgreSQL database driver explicitly if it doesn't exist
    loadDrivers();

    //  Use the users Preferences to determine if a default exists
    initPreferences();
    return;
  }

  /**
   * @brief Initializes user database preferences
   *
   * This method is typically called once at object instantiation.  It references
   * Isis user preferences and loads database access specific profiles.  The
   * access profiles and their associated database configuration parameters
   * establish named databases with access paramters.
   *
   * See the main DatabaseFactory documentation for details.
   */
  void DatabaseFactory::initPreferences() {
    Preference &userPref = Preference::Preferences();
    if(userPref.hasGroup("Database")) {
      PvlGroup &dbgroup = userPref.findGroup("Database");
      if(dbgroup.hasKeyword("AccessConfig")) {
        addAccessProfile(QString::fromStdString(dbgroup["AccessConfig"]));
      }
      // Get default profile name for later use
      if(dbgroup.hasKeyword("DefaultProfile"))
        _defProfName = QString::fromStdString(dbgroup["DefaultProfile"]);
    }
    return;
  }

  /**
   * @brief Get a list of available database drivers and connections
   *
   * This method can be called at any time to return a list of available database
   * drivers and current connections.  These data may change over the lifetime of
   * an application.  It returns a snapshot of whats available.
   *
   * One thing this method does in adds formal database names for known Qt drivers
   * that access them.  This provides a generic interface to users needing
   * connections to specific databases.
   *
   * @param drivers  If true, return a list of database drivers
   * @param connections If true, return a list of existing connections
   *
   * @return DatabaseFactory::Drivers Returns a list of the requested resources
   */
  DatabaseFactory::Drivers DatabaseFactory::getResourceList(bool drivers,
      bool connections)
  const {
    QStringList dblist;
    if(drivers)     dblist += QSqlDatabase::drivers();
    if(connections) dblist += QSqlDatabase::connectionNames();
    Drivers dbDrivers;
    for(int i = 0 ; i < dblist.size() ; i++) {
      QString dbname(dblist.at(i));
      dbDrivers.add(dbname, dbname);
    }


    //  Now create pseudonyms for well known databases
    //  PostgreSQL and UPC
    if(dbDrivers.exists("QPSQL")) {
      dbDrivers.add("PostgreSQL", "QPSQL");
    }

    // MySQL and HiCAT
    if(dbDrivers.exists("QMYSQL")) {
      dbDrivers.add("MySQL", "QMYSQL");
    }

    // Oracle
    if(dbDrivers.exists("QOCI")) {
      dbDrivers.add("Oracle", "QOCI");
    }

    // SQLite
    if(dbDrivers.exists("QSQLITE")) {
      dbDrivers.add("SQLite", "QSQLITE");
    }

    return dbDrivers;
  }

  /**
   * @brief Load any drivers explicity
   *
   * This method is intended to be invoked at object instantiation to load
   * database drivers explicitly.
   *
   * At this time, we are relying on Qt database driver plugins to provide this
   * resource.
   */
  void DatabaseFactory::loadDrivers() {
    //  Currently relying on Qt plugins - but that could change
    // Hack to insure drivers are being loaded correctly
    QSqlDatabase::drivers();
    return;
  }

  /**
   * @brief Destroy all elements associated with this object
   *
   * This method deletes the singleton reference of this object and removes all
   * persistant existing database connections.  It is typically executed when the
   * application is terminated but can be invoked safely under other conditions.
   */
  void DatabaseFactory::selfDestruct() {
    while(_dbList.size() > 0) {
      _dbList.remove(_dbList.key(0));
    }
    QStringList dblist = QSqlDatabase::connectionNames();
    for(int i = 0 ; i < dblist.size() ; i++) {
      QSqlDatabase::removeDatabase(dblist[i]);
    }
    return;
  }

}
