#ifndef DbAccess_h
#define DbAccess_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <string>
#include <vector>
#include <iostream>

#include "CollectorMap.h"
#include "DbProfile.h"
#include "IException.h"

namespace Isis {


  class PvlObject;

  /**
   *  @brief DbAccess manages programatic access to a database through profiles
   *
   *  This class reads a Pvl formatted file and constructs access profiles on the
   *  fly from the contents.  It is intended to specify any and all information
   *  sufficient to establish a database connection in software applications.
   *
   *  The input file to this class is typically created with an editor.  It can
   *  contain any keyword = value combination.  It must contain a \b Database
   *  object and may optionally contain \b Profile groups.  The \b Database object
   *  can contain kewords such as \a User, \a Host, \a DbName, and so forth.  Here
   *  is an example of what the file, named upc.conf, of this type would look
   *  like:
   *
   *  @code
   *  Object = Database
   *    Name = UPC
   *    Dbname = upc
   *    Type = PostgreSQL
   *    Host = "upcdb0.wr.usgs.gov"
   *    Port = 3309
   *    Description = "UPC provides GIS-capable image searches"
   *    AlternateHosts = "upcdb1.wr.usgs.gov"
   *  EndObject
   * @endcode
   *
   * The code used to load and access this profile is:
   *
   * @code
   *   DbAccess upc("upc.conf");
   *   DbProfile default = upc.getProfile();
   * @endcode
   *
   * Additionally, you can add specific profiles that alter some or all of the
   * parameters contained in the Object section of the file.  Simply add one or
   * more \a Profile groups that grant or specify different access profiles for
   * the given database.  For example the example below names the Profile
   * "upcread" and adds an additional user and password to the profile.
   *
   * @code
   *  Object = Database
   *    Name = UPC
   *    Dbname = upc
   *    Type = PostgreSQL
   *    Host = "upcdb0.wr.usgs.gov"
   *    Port = 3309
   *    Description = "UPC provides GIS-capable image searches"
   *    AlternateHosts = "upcdb1.wr.usgs.gov"
   *    DefaultProfile = "upcread"
   *
   *    Group = Profile
   *      Name = "upcread"
   *      User = "upcread"
   *      Password = "public"
   *    EndGroup
   *  EndObject
   * @endcode
   *
   * To access this profile, use:
   *
   * @code
   *   DbProfile upcread = upc.getProfile("upcread");
   * @endcode
   *
   * It will look for the \a Name keyword as the specifed named profile.  What
   * actually happens when the above code is invoked is all the keywords
   * contained in the Database object, such as \a Dbname, \a Type, as well as
   * \a Name are copied to a new \e dynamic profile named "upcread".  Then any
   * keywords found in the actual Profile group with Name = "upcread" are copied
   * to the newly created dynamic one replacing any existing keywords with the
   * ones found in the requested Profile.  This ensures precedence is given to
   * requested profiles and common parameters in the Database object are
   * retained.
   *
   * @ingroup Utility
   * @author 2006-07-01 Kris Becker
   *
   * @internal
   *   @history 2007-06-05 Brendan George - Modified to work with
   *                           QString/StringTool merge
   */
  class DbAccess : public DbProfile {
    private:
      /** Define the container for the DbAccess key word list */
      typedef CollectorMap<IString, DbProfile, NoCaseStringCompare>
      ProfileList;

    public:
      //  Constructors and Destructor
      DbAccess() : DbProfile("Database"), _defProfileName(""), _profiles() { }
      DbAccess(const QString &dbaccFile,
               const QString &defProfileName = "");
      DbAccess(PvlObject &pvl, const QString &defProfileName = "");

      /** Destructor ensures everything is cleaned up properly */
      virtual ~DbAccess() { }

      /**
       * Reports the number of user profiles to access this database
       *
       * @return int Number access profiles
       */
      int profileCount() const {
        return (_profiles.size());
      }

      /**
       * Checks existance of a database user profile
       *
       * @param profile  Name of profile to check for existance
       *
       * @return bool  True if the profile exists, false otherwise
       */
      bool profileExists(const QString &profile) const {
        return (_profiles.exists(profile.toStdString()));
      }

      const DbProfile getProfile(const QString &name = "") const;
      const DbProfile getProfile(int nth) const;


      /**
       * Adds a profile to the database profile
       *
       * Inheritors may add profiles to the user profile list.  Note that
       * duplicate profiles are not allowed, therefore existing profiles with
       * the same name is replaced.
       *
       * @param profile Profile to add
       */
      void addProfile(const DbProfile &profile) {
        _profiles.add(profile.Name().toStdString(), profile);
      }

      void load(const QString &filename);
      void load(PvlObject &pvl);

      QString getDefaultProfileName() const;

    private:
      QString      _defProfileName;  //!<  Name of default profile
      ProfileList      _profiles;        //!<  List of profiles
  };





}
#endif
