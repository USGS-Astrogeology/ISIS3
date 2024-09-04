#ifndef DbProfile_h
#define DbProfile_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <string>
#include <vector>
#include <iostream>

#include "PvlKeyword.h"
#include "PvlContainer.h"
#include "CollectorMap.h"
#include "IException.h"

namespace Isis {

  /**
   * @brief A DbProfile is a container for access parameters to a database
   *
   * This class provides a utility for (keyword) parameter management to access a
   * database system.  Profiles can be used to specify all parameters necessary
   * to apply to programatic interfaces to database software such as Qt's SQL
   * Module class, QSqlDatabase.
   *
   * Using QSqlDatabase as an example, keywords such as \a User, \a Host, \a Port,
   * and \a Dbname can be stored in this object and retrieved easily.
   *
   * It can be easily adapted to any database software API by externally managing
   * the contents of a configuration file.  See DbAccess for additional details on
   * how this scheme can be utilized.
   *
   * @ingroup Utility
   * @author 2006-11-09 Kris Becker
   *
   * @internal
   *   @history 2007-02-15 Kris Becker - Added key() method so one can iterate
   *            through all keys in the profile
   *   @history 2007-12-19 Kris Becker - Added getKeyList method for additional
   *            flexibility to class developers
   *   @history 2008-03-11 Kris Becker - Added more information to exception
   *            caught when attempting to get values from non-existant keys
   *   @history 2008-09-05 Kris Becker - Added the replace and remove methods to
   *            the class.
   */
  class DbProfile {

    protected:
      /* Container for multi-valued keywords in profiles */
      typedef CollectorMap<IString, PvlKeyword, NoCaseStringCompare> KeyList;

    public:
      //  Constructors and Destructor
      DbProfile() : _name("Profile"), _keys() { }
      DbProfile(const QString &name) : _name(name), _keys() { }
      DbProfile(const DbProfile &prof1, const DbProfile &prof2,
                const QString &name = "");
      DbProfile(PvlContainer &pvl);


      /** Destructor ensures everything is cleaned up properly */
      virtual ~DbProfile() { }

      /**
       * @brief Reports if this is a valid profile
       *
       * A valid profile is simply defined to contain keys.  If there are no
       * keys defined for the profile, it is deemed invalid.
       *
       * @return bool True if the profile is valid, therefore containing keys
       */
      bool isValid() const {
        return (size() > 0);
      }

      /**
       * Reports the number of keywords in this user profile
       *
       * @return int Number keywords found in profile
       */
      int size() const {
        return (_keys.size());
      }

      /**
       * Set the name of this profile
       *
       * @param name QString used to set the name of this profile
       */
      void setName(const QString &name) {
        _name = name;
      }

      /**
       * @brief Returns the name of this property
       *
       * @return QString Name of this property
       */
      QString Name() const {
        return (_name);
      }

      /**
       * Checks for the existance of a keyword
       *
       * @param key  Name of keyword to check
       *
       * @return bool True if it exists, false if it doesn't
       */
      bool exists(const QString &key) const {
        return (_keys.exists(key.toStdString()));
      }

      //  Convenience methods for adding keys
      void add(const QString &key, const QString &value = "");
      void replace(const QString &key, const QString &value = "");
      void remove(const QString &key);
      int  count(const QString &key) const;

      /**
       * Returns the nth key in the profile
       *
       * This method returns the name of the nth keyword in the profile so one
       * can iterate through all existing keys.  Note that database passwords
       * could be vulnerable to exposure via this method.
       *
       * Keywords in the profile are sorted in alphabetical order and not in the
       * order in which they are read.
       *
       * @param nth Specifies the nth key in the profile
       *
       * @return QString Name of nth keyword in the profile.
       *
       * @throws Out-of-range exception if the nth keyword does not exist
       */
      QString key(int nth) const {
        return (QString::fromStdString(_keys.key(nth)));
      }
      QString value(const QString &key, int nth = 0) const;
      QString operator()(const QString &key, int nth = 0) const;

    protected:
      void loadkeys(PvlContainer &pvl);

      /**
       * @brief Returns a reference to the key list
       *
       * Direct access to the keyword container allows class developers some
       * additional flexibility whilst maintaining integrity through the
       * public interface.
       *
       * @return const KeyList&  Reference to keyword list
       */
      const KeyList &getKeyList() const {
        return (_keys);
      }

    private:
      QString _name;     //!<  Name of this profile
      KeyList _keys;         //!<  List of keys in profile
  };
}


#endif
