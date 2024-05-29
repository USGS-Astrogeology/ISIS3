#ifndef PvlObject_h
#define PvlObject_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <algorithm>

#include "PvlContainer.h"
#include "PvlGroup.h"

#include <QMetaType>

#include <nlohmann/json.hpp>


template<typename T> class QList;

namespace Isis {
  /**
   * @brief Contains Pvl Groups and Pvl Objects.
   *
   * Contains Pvl groups and objects. Organizes text on output to be indented
   * correctly.
   *
   * @ingroup Parsing
   *
   * @author 2002-10-11 Jeff Anderson
   *
   * @internal
   *  @history 2005-04-04 Leah Dahmer wrote class documentation.
   *  @history 2006-04-21 Jacob Danton Added format templates abilities.
   *  @history 2006-05-18 Jacob Danton Added DeleteGroup by index
   *  @history 2006-09-11 Stuart Sides Added formatting ability
   *  @history 2007-08-11 Stuart Sides Added recursive FindKeyword and
   *                      HasKeyword
   *  @history 2007-08-14 Steven Koechle Fixed missing pointers in FindKeyword
   *                      and HasKeyword
   *  @history 2008-06-18 Steven Lambright Fixed documentation
   *  @history 2008-07-03 Steven Lambright Added const functionality and fixed +=
   *  @history 2008-07-10 Steven Lambright Compensated for PvlKeyword no longer
   *                      being a parent
   *  @history 2008-09-30 Christopher Austin Replaced all std::endl in the <<
   *                      operator with PvlFormat.FormatEOL()
   *  @history 2009-10-28 Travis Addair Fixed bug causing a format
   *                      template's object comments to not appear in the output
   *  @history 2009-12-17 Steven Lambright Added reading methods
   *  @history 2009-12-28 Steven Lambright Implemented const FindObject
   *  @history 2010-04-13 Eric Hyer - Added copy constructor
   *                                  Added assignment operator
   *  @history 2010-09-27 Sharmila Prasad - Validate an Object comparing with the
   *                                        corresponding Template Object
   *  @history 2013-03-11 Steven Lambright and Mathew Eis - Brought method names and member variable
   *                          names up to the current Isis 3 coding standards. Fixes #1533.
   *  @history 2016-08-24 Kelvin Rodriguez - Unit test properly converts indices
   *                          to strings inside loops when appending to strings. Silences
   *                          -Wstring-plus-int warnings on Clang. Part of porting to OS X 10.11
   */
  class PvlObject : public Isis::PvlContainer {
    public:
      PvlObject();
      PvlObject(const QString &name);
      PvlObject(const PvlObject &other);
      PvlObject(const QString &name, const nlohmann::json &jsonobj);

      friend std::ostream &operator<<(std::ostream &os, Isis::PvlObject &object);
      friend std::istream &operator>>(std::istream &is, PvlObject &result);

      /**
       * Returns the number of groups contained.
       * @return The number of groups.
       */
      int groups() const {
        return m_groups.size();
      };

      PvlGroup &group(const int index);
      const PvlGroup &group(const int index) const;

      //! The counter for groups.
      typedef QList<Isis::PvlGroup>::iterator PvlGroupIterator;
      typedef QList<Isis::PvlGroup>::const_iterator ConstPvlGroupIterator;


      /**
       * Returns the beginning group index.
       * @return The iterator of the beginning group..
       */
      PvlGroupIterator beginGroup() {
        return m_groups.begin();
      };


      /**
       * Returns the beginning group index.
       * @return The iterator of the beginning group..
       */
      ConstPvlGroupIterator beginGroup() const {
        return m_groups.begin();
      };


      /**
       * Returns the ending group index.
       * @return The iterator of the ending group.
       */
      PvlGroupIterator endGroup() {
        return m_groups.end();
      };


      /**
       * Returns the const ending group index.
       * @return The iterator of the ending group.
       */
      ConstPvlGroupIterator endGroup() const {
        return m_groups.end();
      };


      /**
       * Find a group with the specified name, within these indexes.
       * @param name The name of the group to look for.
       * @param beg The lower index
       * @param end The higher index
       */
      PvlGroupIterator findGroup(const QString &name,
                                 PvlGroupIterator beg,
                                 PvlGroupIterator end) {
        Isis::PvlGroup temp(name);
        return std::find(beg, end, temp);
      }


      /**
       * Find a group with the specified name, within these indexes.
       * @param name The name of the group to look for.
       * @param beg The lower index
       * @param end The higher index
       */
      ConstPvlGroupIterator findGroup(const QString &name,
                                      ConstPvlGroupIterator beg,
                                      ConstPvlGroupIterator end) const {
        Isis::PvlGroup temp(name);
        return std::find(beg, end, temp);
      }


      /**
       * A collection of options to use when finding.
       */
      enum FindOptions {
        //! Search only the current level
        None,
        //! Search child objects
        Traverse
      };

      // The using statements below are used to make the PvlContainer's version
      // of FindKeyword and HasKeyword visible to other code that otherwise would not be
      // able to see those versions.
      using PvlContainer::findKeyword;


      PvlKeyword &findKeyword(const QString &kname,
                              FindOptions opts);


      using PvlContainer::hasKeyword;


      bool hasKeyword(const QString &kname,
                      FindOptions opts) const;

      Isis::PvlGroup &findGroup(const QString &name,
                                FindOptions opts = None);

      const Isis::PvlGroup &findGroup(const QString &name,
                                      FindOptions opts = None) const;
      /**
       * Add a group to the object.
       * @param group The PvlGroup object to add.
       */
      void addGroup(const Isis::PvlGroup &group) {
        m_groups.push_back(group);
        //m_groups[m_groups.size()-1].SetFileName(FileName());
      };

      using PvlContainer::operator+=;
      void operator+= (const Isis::PvlGroup &group) {
        addGroup(group);
      }
      void operator+= (const Isis::PvlObject &obj) {
        addObject(obj);
      }

      void deleteGroup(const QString &name);

      void deleteGroup(const int index);


      /**
       * Returns a boolean value based on whether the object has the specified
       * group or not.
       * @param name The name of the group to look for.
       * @return True if the object has the group, false if not.
       */
      bool hasGroup(const QString &name) const {
        if(findGroup(name, beginGroup(), endGroup()) == endGroup()) return false;
        return true;
      }

      /**
       * Returns the number of objects.
       * @return The number of objects.
       */
      int objects() const {
        return m_objects.size();
      };

      PvlObject &object(const int index);
      const PvlObject &object(const int index) const;

      //! The counter for objects.
      typedef QList<PvlObject>::iterator PvlObjectIterator;
      typedef QList<PvlObject>::const_iterator ConstPvlObjectIterator;


      /**
       * Returns the index of the beginning object.
       * @return The beginning object's index.
       */
      PvlObjectIterator beginObject() {
        return m_objects.begin();
      };


      /**
       * Returns the const index of the beginning object.
       * @return The beginning object's index.
       */
      ConstPvlObjectIterator beginObject() const {
        return m_objects.begin();
      };


      /**
       * Returns the index of the ending object.
       * @return The ending object's index.
       */
      PvlObjectIterator endObject() {
        return m_objects.end();
      };


      /**
       * Returns the const index of the ending object.
       * @return The ending object's index.
       */
      ConstPvlObjectIterator endObject() const {
        return m_objects.end();
      };


      /**
       * Find the index of object with a specified name, between two indexes.
       * @param name The name of the object to find.
       * @param beg The lower index.
       * @param end The higher index.
       * @return The index of the object.
       */
      PvlObjectIterator findObject(const QString &name,
                                   PvlObjectIterator beg,
                                   PvlObjectIterator end) {
        PvlObject temp(name);
        return std::find(beg, end, temp);
      }


      /**
       * Find the index of object with a specified name, between two indexes.
       * @param name The name of the object to find.
       * @param beg The lower index.
       * @param end The higher index.
       * @return The index of the object.
       */
      ConstPvlObjectIterator findObject(const QString &name,
                                        ConstPvlObjectIterator beg,
                                        ConstPvlObjectIterator end) const {
        PvlObject temp(name);
        return std::find(beg, end, temp);
      }


      PvlObject &findObject(const QString &name,
                            FindOptions opts = None);

      const PvlObject &findObject(const QString &name,
                                  FindOptions opts = None) const;

      /**
       * Add a PvlObject.
       * @param object The PvlObject to add.
       */
      void addObject(const PvlObject &object) {
        m_objects.push_back(object);
        m_objects[m_objects.size()-1].setFileName(fileName());
      }

      void deleteObject(const QString &name);
      void deleteObject(const int index);


      /**
       * Returns a boolean value based on whether the object exists in the current
       * PvlObject or not.
       * @param name The name of the object to search for.
       * @return True if the current PvlObject has the specified object, false
       * if it does not.
       */
      bool hasObject(const QString &name) const {
        if(findObject(name, beginObject(), endObject()) == endObject()) return false;
        return true;
      }


      /**
       * Compares two PvlObjects. Returns a boolean value based on the
       * StringEqual() method.
       * @param object The PvlObject to compare.
       * @return True if they are equal, false if not.
       */
      bool operator==(const PvlObject &object) const {
        return PvlKeyword::stringEqual(object.name(), this->name());
      }


      //! Remove everything from the current PvlObject.
      void clear() {
        Isis::PvlContainer::clear();
        m_objects.clear();
        m_groups.clear();
      }

      const PvlObject &operator=(const PvlObject &other);

      //! Validate Object
      void validateObject(PvlObject & pPvlObj);

    private:
      QList<PvlObject> m_objects;    /**<A vector of PvlObjects contained
                                                in the current PvlObject. */
      QList<PvlGroup> m_groups;/**<A vector of PvlGroups contained
                                                in the current PvlObject. */
  };
}

Q_DECLARE_METATYPE(Isis::PvlObject);

#endif
