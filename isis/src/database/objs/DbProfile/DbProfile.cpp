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

#include "DbProfile.h"
#include "PvlGroup.h"

namespace Isis {

  /**
   * @brief Creates a DbProfile from a Pvl entity
   *
   * This constructor will create a DbProfile by reading keywords from a Pvl
   * container.  Pvl Containers can be an Object or a Group, or simply a list of
   * keywords.  This PvlContainer is expected to point to the appropriate keywords
   * that the caller has pre-established by normal Isis Pvl methods.
   *
   * @param pvl Pvl container of keywords to make a DbProfile from
   */
  DbProfile::DbProfile(PvlContainer &pvl) : _name("Profile"), _keys() {
    loadkeys(pvl);
    if(_keys.exists("Name")) {
      _name = value("Name");
    }
  }


  /**
   * @brief Creates a profile from the merging of two a DbProfiles
   *
   * This constructor will create a new DbProfile from two existing DbProfiles.
   * Its intended use is for the merging of keys in an ordered fashion.  It is
   * useful when a higher level class inherits this class and a specific
   * implementations calls for the keys to come from both the parent and the child
   * DbProfile.
   *
   * It should be noted that this is a convenient way to merge high level
   * database access parameters with individual profile access where the last keys
   * take precedence over the first.  Thus, in the case where the same keys exist
   * in both the first and second profiles, keys in the second profile take
   * precedence.  This is great for defining general database parameters in the
   * top level database configure section and having individual profiles redefine
   * certain access parameters.
   *
   * @param name   Name of new profile
   * @param prof1  First profile to merge
   * @param prof2  Second profile to merge
   */
  DbProfile::DbProfile(const DbProfile &prof1, const DbProfile &prof2,
                       const QString &name) : _name(prof1.Name()),
    _keys(prof1._keys) {
    for(int nk = 0 ; nk < prof2._keys.size() ; nk++) {
      _keys.add(prof2._keys.key(nk), prof2._keys.getNth(nk));
    }

    if(!name.isEmpty()) {
      _name = name;
    }
  }


  /**
   * Adds a keyword and value pair to the profile
   *
   * This method adds a keyword and value pair to the profile if it doesn't exist.
   *  If the keyword already exists,it appends the value to the existing keyword.
   *
   * @param key   Keyword to add or ammend
   * @param value Value to add to the keyword
   */
  void DbProfile::add(const QString &key, const QString &value) {
    if(_keys.exists(key.toStdString())) {
      _keys.get(key.toStdString()).addValue(value.toStdString());
    }
    else {
      _keys.add(key.toStdString(), PvlKeyword(key.toStdString(), value.toStdString()));
    }
  }

  /**
   * Adds a keyword and value pair to the profile
   *
   * This method adds a keyword and value pair to the profile if
   * it doesn't exist.
   *
   * If the keyword already exists, it is deleted and replaced
   * with this new keyword and value.
   *
   * @param key   Keyword to replace
   * @param value Value to add to the keyword
   */
  void DbProfile::replace(const QString &key, const QString &value) {
    _keys.add(key.toStdString(), PvlKeyword(key.toStdString(), value.toStdString()));
  }


  /**
   * Removes a keyword from the profile
   *
   * @param key   Keyword to remove
   */
  void DbProfile::remove(const QString &key) {
    _keys.remove(key.toStdString());
  }

  /**
   * Report number of values in keyword
   *
   * This method will return the number of values in the specified keyword.  If
   * the keyword does not exist, 0 is returned.
   *
   * @param key  Name of key to get value count for
   *
   * @return int Number values in key, or 0 if the key does not exist
   */
  int DbProfile::count(const QString &key) const {
    if(_keys.exists(key.toStdString())) {
      return (_keys.get(key.toStdString()).size());
    }
    return (0);
  }

  /**
   * Returns the specified value for the given keyword
   *
   * This method returns a value from the specified keyword.  If the keyword or
   * the specified value does not exist, an exception is thrown.
   *
   * @param key Name of keyword to return value for.
   *
   * @param nth  Specifies the nth value in the keyword
   *
   * @return QString  The requested value in the keyword
   */
  QString DbProfile::value(const QString &key, int nth) const {
    try {
      return QString::fromStdString(_keys.get(key.toStdString())[nth]);
    }
    catch(IException &ie) {
      ostringstream mess;
      mess << "Error fetching value from key " << key.toStdString();
      if(nth != 0) {
        mess << " (index=" << nth << ")";
      }
      throw IException(ie, IException::Programmer, mess.str(), _FILEINFO_);
    }
  }

  /**
   * Returns the specified value for the given keyword
   *
   * This method returns a value from the specified keyword.  If the keyword or
   * the specified value does not exist, an exception is thrown.
   *
   * @param key Name of keyword to return value for.
   *
   * @param nth  Specifies the nth value in the keyword
   *
   * @return QString  The requested value in the keyword
   */
  QString DbProfile::operator()(const QString &key, int nth) const {
    return (value(key, nth));
  }

  /**
   * @brief Loads DbProfile keys from the given Pvl construct
   *
   * This method iterates through all keywords in the Pvl container and loads them
   * into this property.
   *
   * @param pvl Container of keywords that will be loaded
   */
  void DbProfile::loadkeys(PvlContainer &pvl) {
    PvlContainer::PvlKeywordIterator key;
    for(key = pvl.begin() ; key != pvl.end() ; ++key) {
      _keys.add(key->name(), *key);
    }
  }

#if 0
  void DbProfile::printOn(std::ostream &o) const {

    // Create the ordered list of keywords
    typedef CollectorMap<long, const PvlKeyword *> OrderedKeys;
    OrderedKeys okeys;
    DbProfileKeyList::CollectorConstIter keys;
    for(keys = begin() ; keys != end() ; ++keys) {
      const DbProfileKey &pk = keys->second;
      okeys.add(pk._index, &pk.key);
    }

    //  Now write the keys in order
    PvlGroup propGroup(_group);
    OrderedKeys::CollectorConstIter okItr;
    for(okItr = okeys.begin() ; okItr != okeys.end() ; ++okItr) {
      propGroup.addKeyword(*(okItr->second));
    }

    o << propGroup << std::endl;
  }
#endif


}
