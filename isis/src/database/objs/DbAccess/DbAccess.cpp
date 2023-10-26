/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <string>
#include <vector>
#include <sstream>
#include <iostream>

using std::ostringstream;

#include "DbAccess.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlObject.h"
#include "IString.h"

namespace Isis {

  /**
   * @brief Construct with a given database access configuration file
   *
   * This constructor accepts the name of a Pvl formatted file that must contain a
   * object named \b Database.  It loads keywords in the order they occur in the
   * object.  Keywords should be unique - if not, previous keywords are silently
   * replaced by subsequent occuring keywords.
   *
   * Then all groups named \b Profile are loaded and established as additional,
   * distinct access profiles.  They all should have unique names.  Subsequent
   * profiles with the same name are replaced.
   *
   * The caller may additionally provide the name of the default profile to use
   * when none is given.  If one is not provided, then should a keyword called
   * \b DefaultProfile is searched for and the value of this keyword serves as the
   * default profile.  See the getProfile() method for details on how this
   * situation is resolved.
   *
   * @param dbaccFile      Name of a Pvl formatted file containing the access
   *                       specifications for a database
   * @param defProfileName Optional name of the default access profile
   */
  DbAccess::DbAccess(const QString &dbaccFile,
                     const QString &defProfileName) : DbProfile("Database"),
    _defProfileName(defProfileName), _profiles() {
    load(dbaccFile);
  }


  /**
   * @brief Constructor that accepts a Database Pvl Object
   *
   * The functionality of this constructor is exactly the same as file constructor
   * except with a PvObject named "Database" as an argument.
   *
   * @param  pvl A Database PvlObject containing access information
   * @param  defProfileName Optional name of the default profile
   */
  DbAccess::DbAccess(PvlObject &pvl, const QString &defProfileName) :
    DbProfile("Database"), _defProfileName(defProfileName),
    _profiles() {
    load(pvl);
  }

  /**
   * @brief Retrieves the specified access profile
   *
   * This method retrieves the named profile.  If no name is provided, the default
   * profile is returned.
   *
   * There are two ways to specify the default.  The first source of a named
   * default comes from within the configuration file.  A keyword specified in the
   * Database object section named \b DefaultProfile can specify a named profile,
   * the value of the Name keyword in a \b Profile group.  The second source comes
   * from the application programmer.  In the constructor call to this object, the
   * application programmer can provide a named profile as the default, which
   * could ultimately come from the user (interface).
   *
   * If no default is specified, then only the keywords contained in the Database
   * object section of the configuration file is returned when requesting an
   * unnamed profile.
   *
   * @param name Optional name of the profile to return
   *
   * @return const DbProfile The specified profile.  One should test the
   *         validatity of the profile returned as this is the only indication of
   *         success.
   */
  const DbProfile DbAccess::getProfile(const QString &name) const {
    QString defName(name);
    if(defName.isEmpty()) {
      defName = getDefaultProfileName();
    }
    else {
      if(!_profiles.exists(defName)) {
        return (DbProfile(defName));
      }
    }

//  We have identified the proper profile
    if(_profiles.exists(defName)) {
      //   Return the composite of this access scheme
      return(DbProfile(*this, _profiles.get(defName), defName));
    }
    else {
      //  Return only the high level database access keys and hope it is enough
      return (DbProfile(*this, DbProfile(), defName));
    }
  }

  /**
   * @brief Returns the nth specified DbProfile in the list
   *
   * This method allows user to iterate through the list of DbProfiles in this
   * access scheme.  If the caller provides an index that exceeds the number
   * contained, an exception is thrown.  Use profileCount() to determine the
   * number of profiles.
   *
   * @param nth Zero-based index of profile to return
   *
   * @return const DbProfile The requested nth profile
   */
  const DbProfile DbAccess::getProfile(int nth) const {
    const DbProfile &p = _profiles.getNth(nth);
    return(DbProfile(*this, p, p.Name()));
  }

  /**
   * @brief Loads a Database access configuration file
   *
   * Given the name of a file, it will open the file using Isis Pvl classes.  See
   * the load(pvl) class for additonal information what takes place in this
   * method.
   *
   * Note the file may use environment variables.
   *
   * @param filename Name of Pvl file to open.
   */
  void DbAccess::load(const QString &filename) {
    Pvl pvl(filename.toStdString());
    PvlObject db = pvl.findObject("Database");
    load(db);
  }

  /**
   * @brief Load a database access profile configuration from a PvlObject
   *
   * This method loads all keywords found in the Object section of the PvlObject
   * and then searches for each Group named Profile.  Profile groups contain
   * augmentations to the object keywords to add to or replace object level access
   * specifications.  Each profile group must contain a \b Name keyword to
   * uniquely identify the (group) access parameters.
   *
   * Profiles are loaded and stored in this object for subsequent access.
   *
   * @param pvl A PvlObject that contains keywords and option Profile groups.
   */
  void DbAccess::load(PvlObject &pvl) {

    //  Load database keywords
    loadkeys(pvl);

    //  Get all database user access profiles
    PvlObject::PvlGroupIterator group = pvl.findGroup("Profile",
                                        pvl.beginGroup(),
                                        pvl.endGroup());
    while(group != pvl.endGroup()) {
      DbProfile dbgroup(*group);
      _profiles.add(dbgroup.Name(), dbgroup);
      group = pvl.findGroup("Profile", ++group, pvl.endGroup());
    }
    return;
  }

  /**
   * @brief Determine the name of the default profile
   *
   * This method is called to determine the real name of the default profile as
   * predetermined at load time.  This determination is made either through the
   * default specified in the configuration Database object, the \b DefaultProfile
   * keyword, or provided by the application progirammer in the constructor.
   *
   * @return QString  Name of default profile it it can be determined
   *                      otherwise an empty string is returned.
   */
  QString DbAccess::getDefaultProfileName() const {
    if(!_defProfileName.isEmpty()) {
      return (_defProfileName);
    }
    else if(exists("DefaultProfile")) {
      return (value("DefaultProfile"));
    }
    return ("");
  }


}
