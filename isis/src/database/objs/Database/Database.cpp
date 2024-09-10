/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <string>
#include <vector>
#include <iostream>
#include "DbProfile.h"
#include "Database.h"
#include "DatabaseFactory.h"
#include "IString.h"

#include <QStringList>
#include <QSqlError>

using namespace std;

namespace Isis {

  QString Database::_actualConnectionName = "";

  /**
   * @brief Default database constructor
   *
   * This constructor does not interact at all with the DatabaseFactory class but
   * uses the default condition for the Qt QSqlDatabase state.
   *
   */
  Database::Database() : QSqlDatabase(), _name("") {  }

  /**
   * @brief Construction of the unamed database with optional connection
   *
   * This Database constructor essentially will attempt to invoke the default
   * profile as provided by the DatabaseFactory class.  That profile is read when
   * the factory is created using the IsisPreferences class. If there is a
   * Database object there and it contains a specification of an AccessConfig
   * profile, the contents of the profile mentioned there govern the action of
   * this constructor.
   *
   * If the caller provides true to this class, it will immediately attempt a
   * connection.  Otherwise it will not attempt a connection to the database.
   *
   * Use the isOpen Qt method to check for the status of the connection.
   *
   * @param dbConn If Connect, an immediate connection is attempted, otherwise
   *               connection is deferred.
   */
  Database::Database(Database::Access dbConn) : QSqlDatabase(init()), _name("") {
    _name = _actualConnectionName;
    if((dbConn == Connect) && isValid()) {
      if(!open()) {
        QString mess = "Failed to open database default database [" + _name;
        tossDbError(mess, _FILEINFO_);
      }
    }
//  Name cannot be set, so set to determined name
  }

  /**
   * @brief Create a named database object
   *
   * This construction scheme assumes the named database either already exists as
   * a persistant database connection or exists as a user specified profile in the
   * DatabaseFactory environment.
   *
   * @param name    Name of the desired database connection to establish
   * @param dbConn If Connect, an immediate connection is attempted, otherwise
   *               connection is deferred.
   */
  Database::Database(const QString &name, Database::Access dbConn) :
    QSqlDatabase(init(name)), _name(name) {
    _name = _actualConnectionName;
    if((dbConn == Connect) && isValid()) {
      if(!open()) {
        QString mess = "Failed to open database specified as " + _name;
        tossDbError(mess, _FILEINFO_);
      }
    }
  }

  /**
   * @brief Create database connection specified by name and driver type
   *
   * This constructor is useful for creating a named database with a specific
   * driver type.  The following example creates a PostgreSQL database named
   * "sparky".
   * @code
   *   Database db("sparky", "postgresql");
   * @endcode
   *
   * If you do not provide a driver (driverType = ""), then it will attempt to
   * find an existing database connection named "sparky" and use it or it will
   * attempt to resolve the request by searching for a DbProfile named "sparky".
   *
   * @param connName   Name of connect to create or return
   * @param driverType Type of database to created.  This is typically MySQL,
   *                   PostgreSQL or SQLite.
   */
  Database::Database(const QString &connName, const QString &driverType) :
    QSqlDatabase(init(connName, driverType)), _name(connName) {
    _name = _actualConnectionName;
  }

  /**
   * @brief Create database connection using the supplied DbProfile
   *
   * This constructor accepts a DbProfile that contains sufficient information to
   * create a complete database connection.  The caller can optional request that
   * the connection be established meaning that the profile contain enough
   * information to do so.  If connect = false, then upon return, the caller can
   * further add or modify connection parameters as needed.
   *
   * @param profile  DbProfile containing a single database connection profile or
   *                 one that provides enough information to determine appropriate
   *                 access information.
   * @param dbConn If Connect, an immediate connection is attempted, otherwise
   *               connection is deferred.
   */
  Database::Database(const DbProfile &profile, Database::Access dbConn) :
    QSqlDatabase(init(profile, DoNotConnect)),
    _name(profile.Name()) {
    _name = _actualConnectionName;
    if((dbConn == Connect) && isValid()) {
      if(!open()) {
        QString mess = "Failed to open database with profile " + _name;
        tossDbError(mess, _FILEINFO_);
      }
    }
  }

  /**
   * @brief Constructor creates a clone/copy from an existing one
   *
   * This constructor creates a clone or copy of an existing one.  You can be sure
   * that you can send it a Database object as well as a Qt QSqlDatabase since the
   * Database class inherits the QT QSqlDatabase class.
   *
   * @param other   Database to clone from this one
   * @param newName New name of the cloned database (it can't be the same name)
   */
  Database::Database(const QSqlDatabase &other, const QString &newName) :
    QSqlDatabase(QSqlDatabase::cloneDatabase(other, newName)),
    _name(newName) {  }


  /**
   * @brief Database destructor
   *
   * This will close the Database connection if it is still open, and, if it is
   * not marked as persistant, it is removed from the named Database pool.  It
   * is not completely removed (from the Qt QSqlDatabase pool), however.  Use
   * the remove() method to ensure it is completely destroyed/removed from the
   * pool.
   */
  Database::~Database() {
    DatabaseFactory *factory = DatabaseFactory::getInstance();
    if(!factory->isPersistant(_name)) {
      if(isOpen()) {
        close();
      }
      factory->remove(_name);
    }
  }

  /**
   * @brief Makes this instance persistant
   *
   * Database persistancy in this context means the database remains in whatever
   * state the user leaves it in, such as open, and ensures that the configuration
   * remains available for other uses of the same connection.
   *
   * This feature is useful if you have a long running application that will make
   * prepeated attempts to access the database using the same configuration
   * parameters.  It saves overhead and provides a guaranteed state of access.  It
   * can and perhaps should be closed when not used in between long accesses.
   * This will prevent timeouts from the database.
   *
   * The intended usefulness of the persistant database state is so that at
   * anytime in the life or processing point in the program, the database
   * connection is available.
   *
   * Note that this uses the DatabaseFactory class to retain its persistancy.
   *
   * @see DatabaseFactory
   */
  void Database::makePersistant() {
    DatabaseFactory *factory = DatabaseFactory::getInstance();
    if(!factory->isPersistant(_name)) {
      factory->add(*this, _name);
    }
    return;
  }

  /**
   * @brief Checks persistancy state of a database instantiation
   *
   * This method tests to determine if this database connection is persistant so
   * that future access can be utilized in this state.
   *
   * @return bool True if persistant, otherwise false
   */
  bool Database::isPersistant() const {
    DatabaseFactory *factory = DatabaseFactory::getInstance();
    return (factory->isPersistant(_name));
  }

  /**
   * @brief Sets this database connection/profile as the default
   *
   * Calling this method sets this database instance/connection as the default
   * connection.  It is added to the list of persistant connections and can be
   * retreived at will at any point in an application.  This will be true even if
   * this instance is released.
   *
   * It uses the DatabaseFactory class to register it as the default.  Note that
   * there is only one default ever and it is designated by name.  By definition
   * it is also marked as a persistant connection.
   *
   * @see DatabaseFactory
   */
  void Database::setAsDefault() {
    DatabaseFactory *factory = DatabaseFactory::getInstance();
    if(!factory->isPersistant(_name)) {
      factory->add(*this, _name);
    }
    factory->setDefault(_name);
    return;
  }

  /**
   * @brief Removes the named database from pool
   *
   * This static method is required in order to remove a previous used
   * Database from the database pool.  Database configurations hang around
   * after they are used.  To completely remove them from application
   * space, you must call this method.

   * NOTE:  The Database destructor only ensure the connection is closed.
   * It does not complete remove them.  Persistant databases have their
   * connect state preserved from one Database construction/instantiation
   * to the next.  This method is the only way to completely remove a
   * database from global application space connectivity.
   *
   * WARNING:  Do not attempt to remove an active Database!  This will cause
   * a spurious warning from Qt and render the database inoperative!
   *
   * @param name Name of database to remove (and destroy)
   */
  void Database::remove(const QString &name)  {
    DatabaseFactory *factory = DatabaseFactory::getInstance();
    factory->destroy(name);
    return;
  }

  /**
   * @brief Adds a user specifed access configuration file to system
   *
   * This method accepts a file name that contains a Database access configuration
   * file and adds it to the database access profile system.  This is actually
   * performed by the DatabaseFactory class.
   *
   * @see DatabaseFactory::addAccessProfile()
   *
   * @param confFile Name of file to add.  This can have any valid Isis or
   *                 environment variable as part of the file specfication.
   *
   * @return bool True if successful, false if the file could not be opened or an
   *         error was found in the file.
   */
  bool Database::addAccessConfig(const QString &confFile) {
    DatabaseFactory *factory = DatabaseFactory::getInstance();
    return (factory->addAccessProfile(confFile));
  }

  /**
   * @brief Retrieves the named database access profile
   *
   * This method is provided to the calling environment to retrieve any named
   * profile.  If an empty string is provided, it returns the default as
   * determined by the DatabaseFactory class rules.
   *
   * This can be used to determine the default and potentially augment its
   * contents prior to creating a database connection.
   *
   * For example, here is a small code segment that retrieves the default access
   * profile and tests for its validity.  If it is not valid, chances are there is
   * no default established.
   *
   * @code
   *   DbProfile default = Database::getProfile();
   *   if (!default.isValid()) {
   *      cerr << "No default access profile established!" << endl;
   *   }
   *
   *   //  Open the database (after optional modification)
   *   Database mydb(default);
   * @endcode
   *
   * @see DatabaseFactory::getProfile()
   *
   * @param name  Name of profile to retrieve.  An empty string will return the
   *              default profile.
   *
   * @return DbProfile Requested profile.  Test its validity using the
   *         DbProfile::isValid() method.
   */
  DbProfile Database::getProfile(const QString &name) {
    DatabaseFactory *factory = DatabaseFactory::getInstance();
    return (factory->getProfile(name));
  }

  /**
   * @brief Initializes a database by connection name and driver type
   *
   * This method accepts (optional) connection name and driver type to establish a
   * database connection.  If both passed string parameters are empty, then either
   * the default will be returned or a new database connection is returned using
   * the default profile - if one is established.  If neither of these conditions
   * are met, this routine will throw an error.
   *
   * If only a connection name is given but no driver, then either a persistant
   * connection or a default profile must exist.
   *
   * If both a name and driver is provided, then a clean database object is
   * returned without any connection parameters set and the application programmer
   * must set them.
   *
   * NOTE:  This method is implemented in such a way that it assumes it is part of
   * the QSqlDatabase initialization phase upon object construction.  You will see
   * some implementation decisions based upon this expeectation.
   *
   * @param connName   Name of the connection to create
   * @param driverType Type of driver/database to create.  This is typically
   *                   MySQL, PostgreSQL or SQLite.
   *
   * @return QSqlDatabase The created database
   */
  QSqlDatabase Database::init(const QString &connName,
                              const QString &driverType) {

    _actualConnectionName = connName;
    DatabaseFactory *factory = DatabaseFactory::getInstance();

    // First test for condition where both name and type are not provided.
    //  This tests for the default profile and returns it if it exists,
    // otherwise it returns a default database.
    if(connName.isEmpty() && driverType.isEmpty()) {
      if(factory->isAvailable(factory->getDefault())) {
        _actualConnectionName = factory->getDefault();
        return (factory->create(_actualConnectionName));
      }

      //  No default is established so retreive the default profile
      DbProfile profile =  factory->getProfile();
      if(profile.isValid()) {
        return (init(profile, DoNotConnect));
      }
    }

    // If only the name and no driver is provided, get an existing connection
    if((!connName.isEmpty()) && (driverType.isEmpty())) {
      if(factory->isAvailable(connName)) {
        _actualConnectionName = connName;
        return (factory->create(connName));
      }
      else {
        //  See if the database exists by profile
        DbProfile profile = factory->getProfile(connName);
        return (init(profile, DoNotConnect));
      }
    }

    //  Finally, a driver and optional name is provided.  This condition sets up
    //  a named database for subsequent definition later
    return (factory->create(driverType, connName));
  }


  /**
   * @brief Create and initialize a new database connection from a DbProfile
   *
   * This init method accepts a DbProfile database access profile that is assumed
   * to contain sufficient information to establish a connection and open it.
   * Note that the connection is opened only if the connect = true.  Otherwise,
   * the parameters from teh profile is set but the database is returned without
   * initiating a connection to the database - this so the caller can adjust or
   * provide additional parameters.
   *
   * NOTE:  This method is implemented in such a way that it assumes it is part of
   * the QSqlDatabase initialization phase upon object construction.  You will see
   * some implementation decisions based upon this expeectation.
   *
   * @param profile  A valid database profile specifying access parameters.
   * @param dbConn If Connect, an immediate connection is attempted, otherwise
   *               connection is deferred.
   * @return QSqlDatabase  A Qt database object with access parameters set
   */
  QSqlDatabase Database::init(const DbProfile &profile, Database::Access dbConn) {
    if(!profile.isValid()) {
      ostringstream mess;
      mess << "Database/profile [" << profile.Name().toStdString() << "] is not valid!" << ends;
      throw IException(IException::Programmer, mess.str(), _FILEINFO_);
    }

    _actualConnectionName = profile.Name();
    DatabaseFactory *factory = DatabaseFactory::getInstance();

    //  initialize the database
    try {

      //  If we reach here, it is a valid profile.  Create the database and
      //  return it as initialized from the profile contents
      QSqlDatabase db = factory->create(profile("Type"), profile("Name"));
      _actualConnectionName = profile("Name");
      configureAccess(db, profile);

      //  Go ahead and connect if requested
      if(dbConn == Connect) {
        if(!db.open()) {
          QString mess = "Failed to connect to database using profile " +
                        profile("Name");
          tossDbError(mess, _FILEINFO_);
        }
      }
      return (db);
    }
    catch(IException &ie) {
      std::string mess = "Unable to create database from " + profile.Name().toStdString();
      throw IException(ie, IException::User, mess, _FILEINFO_);
    }
    catch(...) {
      std::string mess = "Unknown exception while creating database from profile "
                    + profile.Name().toStdString();
      throw IException(IException::User, mess, _FILEINFO_);
    }
  }

  /**
   * @brief Set access parameters from a database DbProfile access specification
   *
   * This method takes a database and a database access configuration setup and
   * applies the parameters to it setting up access.  This method does not intiate
   * the connection, only sets known, common parameters.  These parameters are
   * Host, DbName, User, password, Port and Options.  They follow the
   * specifications of the Qt SQL QSqlDatabase class methods.
   *
   * @param db      The Qt database object to set access parameters for.
   * @param profile The database access parameter source.
   */
  void Database::configureAccess(QSqlDatabase &db, const DbProfile &profile) {
    if(profile.exists("Host")) {
      db.setHostName(profile("Host"));
    }

    if(profile.exists("DbName")) {
      db.setDatabaseName(profile("DbName"));
    }

    if(profile.exists("User")) {
      db.setUserName(profile("User"));
    }

    if(profile.exists("Password")) {
      db.setPassword(profile("Password"));
    }

    if(profile.exists("Port")) {
      bool ok;
      db.setPort(profile("Port").toInt(&ok));
      if(!ok) {
        ostringstream mess;
        mess << "Invalid port number [" << profile("Port").toStdString() << "] in profile "
             << profile("Name").toStdString() << ends;
        throw IException(IException::User, mess.str().c_str(), _FILEINFO_);
      }
    }

    if(profile.exists("Options")) {
      db.setConnectOptions(profile("Options"));
    }
    return;
  }

  /**
   * @brief Clones this database into another giving it another name
   *
   * This database object is cloned into another one and names it the provided
   * name.  All access parameters are retained as initiallyt set up.
   *
   * @param name  Name to give the cloned database.
   *
   * @return The cloned Database
   */
  Database Database::clone(const QString &name) const {
    return (Database(*this, name));
  }

  /**
   * @brief Returns a vector string containing all the tables in the database
   *
   * This method returns a complete list of accessable tables within the database.
   * It is assumed the database connections is established and open.
   *
   * @return std::vector<QString>  List of tables in the database
   */
  QStringList Database::getTables() const {
    return (tables(QSql::Tables));
  }

  /**
   * @brief Returns a vector string containing all views within the database
   *
   * This method returns a vector of strings with all views accessable to the user
   * in each element in the vector.
   *
   * @return std::vector<QString>  List of all accessable views in the
   *         database
   */
  QStringList Database::getViews() const {
    return (tables(QSql::Views));
  }

  /**
   * @brief Returns vector strings of all available system tables in the database
   *
   * This method returns a vector of strings containing a list of all system
   * tables accessable to the user within the database.
   *
   * @return std::vector<QString>  List of system tables with the database
   */
  QStringList Database::getSystemTables() const {
    return (tables(QSql::SystemTables));
  }

  /**
   * @brief Generic exception tosser
   *
   * This method is used from within this class to construct and deploy an
   * exception when an error occurs in some of the methods in this class.
   *
   * @param message  Text of message to include in exception
   * @param f        Name of method initiating the exception
   * @param l        Line number the error occured
   */
  void Database::tossDbError(const QString &message, const char *f, int l) const {
    std::string errmess = message.toStdString() + " - DatabaseError = " +
                      lastError().text().toStdString();
    throw IException(IException::Programmer, errmess, f, l);
  }

}
