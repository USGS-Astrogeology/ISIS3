#ifndef DatabaseFactory_h
#define DatabaseFactory_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <string>
#include <vector>
#include <iostream>

#include <QSqlDatabase>

#include "CollectorMap.h"
#include "IException.h"

namespace Isis {

  class DbAccess;
  class DbProfile;

  /**
   * @brief Create database interfaces using access profiles or generic drivers
   *
   * This class provides two major components for database programming: database
   * drivers and access profiles.  This class is implemented as a Singleton (see
   * GoF Design Patterns).  As such, it serves as a single access point for
   * drivers and profiles.  It should become clear why database access profiles
   * are also provided in this class after reading the subsequent documentation.
   *
   * This class creates Qt QSqlDatabase objects and subsequent use of the
   * resulting objects from this class are dependant on a valid Qt environment.
   * At the time this class was developed, Qt provides database drivers as
   * plugins and this class assumes they are already available within the Qt
   * environment.  See the Qt documentation for details on options to provide
   * database drivers in the Qt environment.  Note, however, that the existance of
   * Qt database drivers in not required to build this class or supporting
   * classes.  Proper use of instantiated classes does require database drivers to
   * exist in the Qt environment.
   *
   * The names of the Qt drivers is an issue.  Qt names their drivers obscurely so
   * that it is not readily obvious what, for example, is the MySQL driver name.
   * This class uses access profiles, described below, that name the type of
   * database you expect to access.  This implies the user must know if the
   * database is Oracle, PostgreSQL or MySQL and specify the proper name in the
   * access profile. The Qt PostgreSQL driver is called \a QPSQL. This class tests
   * for the specifc Qt named drivers and adds formal names to the driver list.
   * For example, if the \a QPSQL exists, a driver named \a PostgreSQL is also
   * added/available.  If the MySQL Qt driver, \a QMYSQL, exists, \a MySQL is also
   * added to the available drivers.  For this reason, it is recommededd that
   * access profiles use the formal names instead of the Qt driver names...just
   * in case we decided to discontinue use of Qt as the database framework and
   * replace it with another.
   *
   * This factory generates database driver instances whereby access is defined by
   * database access profiles.  The primary profile is specified in the
   * IsisPreferences file in the Database group.  Access profiels are established
   * by user preferences as read from the $HOME/.Isis/IsisPreferences. In the
   * \b Database group, the \b AccessConfig keyword contains a full path to a
   * database profile.  This file contains a Database object with a AccessConfig
   * keyword that specifies the full path to a file that contains database access
   * information and optionally a DefaultProfile that indicates the default
   * profile to use when creating an unnamed database.  Note that the database
   * profile file may also indicate a default, but the value of DefaultProfile
   * overrides this default so certain applications and uses can govern behavior
   * if needed.
   *
   * The intended strategy behind this design is to allow the profile file to
   * specify the read only access configuration to the database.  An unnamed
   * request to create a database will use the named default and create a driver
   * to the requesting application when accessing a database.  If an application
   * is designed to update the contents of the database, the user can specify the
   * access to the database and specify the name of the profile in the profile
   * file that provides write access to the database.  The DefaultProfile keyword
   * in the database group is used to specify the name of the profile.
   *
   * Below is an example of a Database group contained within your personal
   * IsisPreferences file describing the location of the Database access profile
   * file is and a commented example of how to specify the default profile to use
   * when an unnamed database instantiation is requested:
   *
   * @code
   * ########################################################
   * # Customize the database configuration upon startup
   * # of any database type application
   * ########################################################
   * Group = Database
   *   AccessConfig = $HOME/.Isis/database/upc.conf
   * # DefaultProfile = upc
   * EndGroup
   * @endcode
   *
   * The specification of AccessConfig indicates the file that this object reads
   * when it is instantiated.  The file, in this case, upc.conf, should contain
   * a Database object and one or more Profile Groups.  Although DefaultProfile is
   * commented, it is also reflected in the profile file as illustrated below.  If
   * the user wants write or update access, he could simple set the value of
   * \b DefaultProfile to \b UpcWrite, uncomment it and the \b UpcWrite profile
   * then becomes the default database connection.
   *
   * Below is an example of the contents of a database access configuration
   * profile file:
   * @code
   * Object = Database
   *   Name = UPC
   *   Dbname = upc
   *   Type = PostgreSQL
   *   Host = "upcdb0.wr.usgs.gov"
   *   Port = 3309
   *   Description = "UPC provides GIS-capable image searches"
   *   AlternateHosts = "upcdb1.wr.usgs.gov"
   *   DefaultProfile = Upc
   *
   *   Group = Profile
   *     Name = Upc
   *     User = "upcread"
   *     Access = ReadOnly
   *     Password = "public"
   *   EndGroup
   *
   *   Group = Profile
   *     Name = UpcWrite
   *     User = "upcwrite"
   *     Access = Update
   *   EndGroup
   *
   *   Group = Profile
   *     Name = UpcAdmin
   *     User = "upcmgr"
   *     Access = Admin
   *   EndGroup
   * EndObject
   * @endcode
   *
   * Not all the keywords are critical/required but some are needed in order to
   * sucessfully acquire access to a specified database.  The \b Dbname keyword is
   * needed to specify the name of the database to access.  Options include
   * \b User which specifies the name of the database user that provides access to
   * the database; \b Password is an optional password if the user account
   * requires one - note that under most conditions, it is unwise to reveal a
   * password in this fashion.  It is only advisable if the database user has read
   * only access specified in its access conditions or if file permissions are set
   * such that no other user can see the contents, which is still not advised.
   * Users can utilize other access methods, such as environment variables, or
   * whatever the database access system provides.  FOr example, PostgreSQL uses
   * the
   * <a href="http://www.postgresql.org/docs/8.1/interactive/libpq.html">libpq</a>
   * library and it has some additional access provisions.  You could set
   * <a href="http://www.postgresql.org/docs/8.1/interactive/libpq-envars.html">
   * environment variables</a> or use the
   * <a href="http://www.postgresql.org/docs/8.1/interactive/libpq-pgpass.html">
   * password file</a> to create a more secure access scheme. Check your database
   * documentations for additional access options; \b Host is used to provide the
   * name of the dababase host computer. And \b Port specifies a specific port
   * number to use to connect. See the Qt QSqlDatabase documentation for access
   * specifics.  This access scheme is used directly in this class.
   *
   * When selecting a specific Profile from within the Database object, all
   * keywords in the Database object are first copied to a new Profile.  Then the
   * requested Profile is merged with new profile and given the name of the group
   * profile. Under this scenario, it is intended to provide a cascading hierarchy
   * of access parameters, whereby the Profile keywords take precendence.  For any
   * keywords that exist in both the Database object section and the specfied
   * profile, the keywords in the Profile group replace those in the Database
   * section.  In the above example, the keyword \b Name exists in both the
   * Database section and each Profile group. When the default profile \b Upc is
   * selected, the \b Name keyword in the Profile group eith the value \b Upc
   * replaces the one in the Database section that has the value \b UPC.
   *
   * For example the code to select the \b Upc profile is:
   *
   * @code
   *   DatabaseFactory *factory = DatabaseFactory::instance();
   *   DbProfile upc = factory->getProfile("upc");
   * @endcode
   *
   * Using the above example configuration scheme, the resulting \b Upc profile
   * looks like this:
   *
   * @code
   *   Group = Profile
   *     Name = Upc
   *     Dbname = upc
   *     Type = PostgreSQL
   *     Host = "upcdb0.wr.usgs.gov"
   *     Port = 3309
   *     Description = "UPC provides GIS-capable image searches"
   *     AlternateHosts = "upcdb1.wr.usgs.gov"
   *     DefaultProfile = Upc
   *     User = "upcread"
   *     Access = ReadOnly
   *     Password = "public"
   *   EndGroup
   * @endcode
   *
   * This allows each Profile group to change any or all access parameters, even
   * the type of database (PostgreSQL to MySQL, for example) it needs.
   *
   * When the DatabaseFactory is invoked for the first time, the users preference
   * file is loaded and the default Database AccessConfig file is read.  It
   * governs all further access unless the programmer specifically codes its own
   * access parameters, which is still possible through this class.
   *
   * This is an example using this class to craft explicit access to a database
   * named \b upc:
   * @code
   *  DatabaseFactory *factory = DatabaseFactory::getInstance();
   *  QSqlDatabase upc = factory->create("Postgresql", "upctest");
   *  upc.setHostName("upcdb0");
   *  upc.setUserName("upcread");
   *  upc.setPassword("public");
   *  upc.setDatabaseName("upc");
   *  if (upc.open()) {
   *    QSqlQuery query("SELECT * FROM pg_tables where schemaname = 'public'",
   *                    upc);
   *  }
   * @endcode
   * In the above example, a database driver for PostgreSQL is created with the
   * name "upctest".  Access parameters are set explicitly and a query is issued
   * if access is successful.  Note that access schemes supported by targeted
   * databases apply.  For example, PostgreSQL will utilize environment variables
   * to supply some access parameters.  MySQL and PostgreSQL also utilize a
   * configuration file whereby access can be acheived through variables set
   * there.  These combinations should supply adequate options to provide flexible
   * options for accessing databases within the Isis system.
   *
   * See the Database class for an example of using a specific profile to provide
   * access to a database.
   *
   * @ingroup Database
   * @author 2006-08-18 Kris Becker
   *
   * @internal
   *   @history 2007-06-05 Brendan George - Modified to work with
   *                           QString/StringTools merge
   *   @history 2009-11-27 Kris Becker - Made argc parameter for
   *                           QCoreApplication so persistence of the parameter
   *                           is preserve as required for Qt.
   */
  class DatabaseFactory  {
    public:
      static DatabaseFactory *getInstance();

      /**
       * @brief Sets the default name of the database
       *
       * This method defines the name of the database to use when none is
       * specifed in subsequent calls to the create methods.  This is typically
       * a named profile, but could be a database driver as well as they are
       * used in the same context.
       *
       * @param name Name of the default database
       */
      void setDefault(const QString &name) {
        _defDatabase = name;
      }

      /**
       * @brief Returns the name of the default database
       *
       * This method returns the name of the current default database.  If a
       * call to the create method is attempted without a name, this is the one
       * used to return an instance of database.
       *
       * @return QString  Name of default database
       */
      QString getDefault() const {
        return (_defDatabase);
      }

      bool addAccessProfile(const QString &profileFile);
      void addProfile(const DbProfile &profile);
      std::vector<QString> getProfileList() const;
      DbProfile getProfile(const QString &name = "") const;

      /**
       * @brief Returns the name of the default profile
       *
       * If a default profile name has been established this will return the
       * name of the default profile.  If none are loaded, an empty string is
       * returned.
       *
       * @return QString  Name of default profile, empty if undetermined
       */
      QString getDefaultProfileName() const {
        return (_defProfName);
      }

      /**
       * @brief Sets the default profile to the name provided
       *
       * This allows the calling environment to establish the default database
       * access profile by name.  It returns true if the named profile exists
       * within the current list of profiles, false if it doesn't;
       *
       * @param name Name of the new default database access profile
       *
       * @return bool True if it named profile exists, false otherwise
       */
      bool setDefaultProfileName(const QString &name) {
        _defProfName = name;
        return (_profiles.exists(name.toStdString()));
      }

      std::vector<QString> available() const;
      bool isDriverAvailable(const QString &driver) const;
      bool isAvailable(const QString &dbname = "") const;
      bool isConnected(const QString &dbname) const;
      bool isPersistant(const QString &name) const;

      QSqlDatabase create(const QString &driver, const QString &dbname);
      QSqlDatabase create(const QString &name);
      void add(const QSqlDatabase &db, const QString &name,
               bool setAsDefault = false);
      void remove(const QString &dbname);
      void destroy(const QString &dbname);

    private:
      //  Gain access through Singleton interface
      DatabaseFactory();
      ~DatabaseFactory();

      static void DieAtExit();

      static DatabaseFactory *_factory;  //!< Pointer to self (singleton)

      /** Define list of drivers and/or databases   */
      typedef CollectorMap<IString, QString, NoCaseStringCompare> Drivers;
      /** Define list of Profiles */
      typedef CollectorMap<IString, DbProfile, NoCaseStringCompare> Profiles;
      /** Define active database maintainer */
      typedef CollectorMap<IString, QSqlDatabase, NoCaseStringCompare> Databases;

      QString _defProfName;         //!<  Default profile name
      Profiles    _profiles;            //!<  Maintain list of profiles
      QString _defDatabase;         //!<  Name of default database
      Databases   _dbList;              //!<  Maintains active databases

      void init();
      void initPreferences();
      void loadDrivers();
      Drivers getResourceList(bool drivers, bool connections) const;
      void selfDestruct();

  };
}
#endif
